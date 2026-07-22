#include "WorldClient/World.h"

#include "Base/Status.h"
#include "Console/ConsoleClient.h"
#include "Console/ConsoleCommand.h"
#include "Gx/CGxDevice.h"
#include "Gx/Gx.h"
#include "Model/IModel.h"
#include "Services/Texture.h"
#include "Ui/WorldFrame.h"

#include "Tempest/c33matrix.h"
#include "Tempest/c44matrix.h"
#include "Tempest/crect.h"

#include <math.h>

static CGxTex                            *s_fadeTex;
static HTEXTURE                           s_hTexture;
static TSCArray<NTempest::CImVector, 512> s_texels;
static CWTriData::Batch                  *s_batch;
static NTempest::C3Vector                 s_zup(0.0f, 0.0f, 1.0f);
static NTempest::CImVector                s_color;
static int                                s_shadowLOD = 1;

static float FeetToWorld(float ft) {
  return ft * 0.33333334f;
}

static const float EXTENT_SCALE = 1.0f;
static const float SHADOW_ALPHA_SCALE = 1.5f;
static const float BLOB_BELOW = FeetToWorld(5.0f);
static const float BLOB_ABOVE = FeetToWorld(3.0f);
static const float SHADOW_POLY_OFFSET = 0.0625f;

static int __fastcall ConsoleCommand_ShadowLOD(const char *__formal, const char *args);

CGxTex *ProjectTex2dGetFade() {
  return s_fadeTex;
}

void __fastcall ProjectTex2dMakeMatrices(
    NTempest::C44Matrix &texmat0,
    NTempest::C44Matrix &texmat1,
    NTempest::CAaBox    &box,
    NTempest::C44Matrix *basis,
    float                fadeOffset,
    int                  inWorldSpace
) {
  NTempest::C3Vector cameraPos;
  CGWorldFrame::GetCameraPosition(&cameraPos);

  const NTempest::C3Vector boxCenter = (box.b + box.t) * 0.5f;
  NTempest::C44Matrix      worldTransMat;
  worldTransMat.Translate(inWorldSpace ? -boxCenter : cameraPos - boxCenter);

  const NTempest::C3Vector boxSize = box.t - box.b;
  if (boxSize.x < 0.001f || boxSize.y < 0.001f) {
    return;
  }

  NTempest::C44Matrix texScale;
  texScale.a0 = 1.0f / boxSize.x;
  texScale.b1 = 1.0f / boxSize.y;
  NTempest::C44Matrix scaleTransMapRtoS = NTempest::C44Matrix::Rotation(-3.1415927f * 0.5f, s_zup, 1);
  texmat0 = worldTransMat * texScale * scaleTransMapRtoS;
  if (basis) {
    texmat0 *= *basis;
  }
  texmat0.d0 += 0.5f;
  texmat0.d1 += 0.5f;

  NTempest::C44Matrix worldToTexture(
      0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f / boxSize.z, 0.0f, 1.0f / boxSize.z, 0.0f, fadeOffset, 0.5f, fadeOffset, 1.0f
  );
  texmat1 = worldTransMat * worldToTexture;
}

static void __fastcall ProjectTexRenderVerticesPN(CGxBufCommand &cmd, CGxBuf *buf) {
  CGxVertexPN *vertices = 0;
  switch (cmd.vertex.op) {
    case GxBufOp_Fill:
      vertices = static_cast<CGxVertexPN *>(*cmd.vertex.mem[GxVM_Position]);
      break;
    case GxBufOp_Assign:
      vertices = static_cast<CGxVertexPN *>(GxAllocVertexMem(buf->VertexCount() * sizeof(*vertices)));
      *cmd.vertex.mem[GxVM_Position] = &vertices->p;
      *cmd.vertex.mem[GxVM_Normal] = &vertices->n;
      break;
    default:
      FATALASSERT(0);
  }

  unsigned short vidx = s_batch->GetMinIndex();
  for (unsigned int i = 0; i < s_batch->GetVertexCount(); ++i, ++vidx) {
    vertices[i].p = s_batch->GetVertex(vidx);
    vertices[i].n = s_batch->GetNormal(vidx);
  }
}

static void __fastcall ProjectTexRenderVerticesPC(CGxBufCommand &cmd, CGxBuf *buf) {
  CGxVertexPC *vertices = 0;
  switch (cmd.vertex.op) {
    case GxBufOp_Fill:
      vertices = static_cast<CGxVertexPC *>(*cmd.vertex.mem[GxVM_Position]);
      break;
    case GxBufOp_Assign:
      vertices = static_cast<CGxVertexPC *>(GxAllocVertexMem(buf->VertexCount() * sizeof(*vertices)));
      *cmd.vertex.mem[GxVM_Position] = &vertices->p;
      *cmd.vertex.mem[GxVM_Color] = &vertices->c;
      break;
    default:
      FATALASSERT(0);
  }

  unsigned short vidx = s_batch->GetMinIndex();
  for (unsigned int i = 0; i < s_batch->GetVertexCount(); ++i, ++vidx) {
    vertices[i].p = s_batch->GetVertex(vidx);
    vertices[i].c = s_color;
  }
}

static void __fastcall ProjectTexRenderIndices(CGxBufCommand &cmd, CGxBuf *buf) {
  unsigned short *indices = 0;
  switch (cmd.index.op) {
    case GxBufOp_Fill:
      indices = static_cast<unsigned short *>(*cmd.index.mem[GxVM_Indices]);
      break;
    case GxBufOp_Assign:
      indices = static_cast<unsigned short *>(GxAllocIndexMem(buf->IndexCount() * sizeof(*indices)));
      *cmd.index.mem[GxVM_Indices] = indices;
      break;
    default:
      FATALASSERT(0);
  }

  for (unsigned int i = 0; i < s_batch->GetIndexCount(); ++i) {
    indices[i] = s_batch->GetIndex(i) - s_batch->GetMinIndex();
  }
}

static void __fastcall ProjectTexRenderPC(CGxBufCommand &cmd, CGxBuf *buf) {
  ProjectTexRenderVerticesPC(cmd, buf);
  ProjectTexRenderIndices(cmd, buf);
}

static void __fastcall ProjectTexRenderPN(CGxBufCommand &cmd, CGxBuf *buf) {
  ProjectTexRenderVerticesPN(cmd, buf);
  ProjectTexRenderIndices(cmd, buf);
}

void __fastcall ProjectTex2d(NTempest::CAaBox &box, NTempest::CImVector color, NTempest::C44Matrix *basis, float fadeOffset) {
  CWTriData triData;
  if (!CWorld::GetTris(box, triData, 0x122)) {
    return;
  }

  NTempest::C44Matrix texmat0;
  NTempest::C44Matrix texmat1;
  ProjectTex2dMakeMatrices(texmat0, texmat1, box, basis, fadeOffset, 0);
  GxXformPush(GxXform_Tex0, texmat0);
  GxXformPush(GxXform_Tex1, texmat1);
  GxVertexShaderSelect(GxVS_PassThru);
  GxRsPush();
  GxRsSet(GxRs_Texture1, s_fadeTex);
  GxRsSet(GxRs_PolygonOffset, SHADOW_POLY_OFFSET);
  GxRsSet(GxRs_TexGen0, GxTexGen_World);
  GxRsSet(GxRs_TexGen1, GxTexGen_World);
  GxRsSet(GxRs_TextureShader0, GxTS_Affine);
  GxRsSet(GxRs_TextureShader1, GxTS_Affine);

  int lighting;
  GxRsGet(GxRs_Lighting, lighting);
  CGxBuf *gxBuf;
  if (lighting == 1) {
    GxRsSet(GxRs_MatDiffuse, color);
    gxBuf = GxBufGetDynamic(GxVBF_PN);
    gxBuf->UserCallbackSet(ProjectTexRenderPN);
  } else {
    s_color = color;
    if (GxCaps().m_colorFormat == GxCF_rgba) {
      s_color.Set(s_color.a, s_color.b, s_color.g, s_color.r);
    }
    gxBuf = GxBufGetDynamic(GxVBF_PC);
    gxBuf->UserCallbackSet(ProjectTexRenderPC);
  }

  NTempest::C3Vector cameraPos;
  CGWorldFrame::GetCameraPosition(&cameraPos);
  NTempest::C44Matrix worldMtx;
  worldMtx.Translate(-cameraPos);
  GxXformPush(GxXform_World);

  for (unsigned int i = 0; i < triData.GetNumBatches(); ++i) {
    const CWTriData::Batch &batch = triData.GetBatch(i);
    if ((batch.GetMinIndex() == 0xFFFF || batch.GetVertexCount() <= Gx_MaxVertices) && batch.GetIndexCount() <= Gx_MaxIndices) {
      NTempest::C44Matrix batchMtx = *batch.matrix * worldMtx;
      GxXformSet(GxXform_World, batchMtx);
      gxBuf->CountSet(batch.GetVertexCount(), batch.GetIndexCount());
      s_batch = const_cast<CWTriData::Batch *>(&batch);
      GxBufLock(gxBuf);
      CGxBatch gxBatch(GxPrim_Triangles, batch.GetIndexCount(), 0, -1, -1);
      GxBufRender(gxBatch);
      GxBufUnlock();
    }
  }

  GxXformPop(GxXform_World);
  GxXformPop(GxXform_Tex0);
  GxXformPop(GxXform_Tex1);
  GxRsPop();
}

void __fastcall ShadowRender_LOD1(HMODEL hModel, NTempest::C44Matrix &basis, void *param) {
  NTempest::C3Vector cameraPos;
  CGWorldFrame::GetCameraPosition(&cameraPos);

  NTempest::CAaBox extents;
  if (!ModelGetSeqExtents(hModel, 0, &extents)) {
    return;
  }

  NTempest::C3Vector scaleVect(basis.a0, basis.a1, basis.a2);
  float              matScale = scaleVect.Mag();
  extents.b *= EXTENT_SCALE * matScale;
  extents.t *= EXTENT_SCALE * matScale;
  float height = (extents.t.z - extents.b.z) / FeetToWorld(6.0f);

  NTempest::C33Matrix normBasis(basis.a0, basis.a1, basis.a2, basis.b0, basis.b1, basis.b2, basis.c0, basis.c1, basis.c2);
  if (fabs(matScale - 1.0f) >= 2.3841858e-7f) {
    scaleVect = NTempest::C3Vector(1.0f / matScale);
    normBasis.Scale(scaleVect);
  }

  NTempest::C3Vector basedExtents[4];
  basedExtents[0] = normBasis * NTempest::C3Vector(extents.t.x, extents.t.y, 0.0f);
  basedExtents[1] = normBasis * NTempest::C3Vector(extents.t.x, extents.b.y, 0.0f);
  basedExtents[2] = normBasis * NTempest::C3Vector(extents.b.x, extents.b.y, 0.0f);
  basedExtents[3] = normBasis * NTempest::C3Vector(extents.b.x, extents.t.y, 0.0f);

  NTempest::CAaBox   srWorldBox = NTempest::CAaBox::Bounding(basedExtents, 4);
  NTempest::C3Vector position(cameraPos.x + basis.d0, cameraPos.y + basis.d1, cameraPos.z + basis.d2);
  srWorldBox.b += position;
  srWorldBox.t += position;
  srWorldBox.b.z = position.z - BLOB_BELOW * height;
  srWorldBox.t.z = position.z + BLOB_ABOVE * height;

  NTempest::C33Matrix undoScaleMat;
  scaleVect = srWorldBox.t - srWorldBox.b;
  undoScaleMat.Scale(scaleVect);

  NTempest::C33Matrix shadowScaleMat;
  shadowScaleMat.Scale(1.0f / fabs(extents.t.y - extents.b.y), 1.0f / fabs(extents.t.x - extents.b.x), 1.0f);

  NTempest::C33Matrix basisRotMat = normBasis.Transpose();
  NTempest::C33Matrix shadowMat = undoScaleMat * (basisRotMat * shadowScaleMat);
  NTempest::C44Matrix sunTexMat(
      shadowMat.a0, shadowMat.a1, shadowMat.a2, 0.0f, shadowMat.b0, shadowMat.b1, shadowMat.b2, 0.0f, shadowMat.c0, shadowMat.c1, shadowMat.c2, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f
  );

  NTempest::CImVector shadowColor = CWorld::shadowColor;
  unsigned int        alpha = static_cast<unsigned int>(shadowColor.a * SHADOW_ALPHA_SCALE);
  if (alpha > 255) {
    alpha = 255;
  }
  shadowColor.a = static_cast<unsigned char>(alpha);
  shadowColor.r = static_cast<unsigned char>(shadowColor.r * 0.65f);
  shadowColor.g = static_cast<unsigned char>(shadowColor.g * 0.65f);
  shadowColor.b = static_cast<unsigned char>(shadowColor.b * 0.65f);

  GxRsPush();
  GxRsSet(GxRs_Blend, GxBlend_Alpha);
  GxRsSet(GxRs_Culling, 0);
  GxRsSet(GxRs_TexGen0, GxTexGen_Disable);
  GxRsSet(GxRs_Lighting, 0);
  GxRsSet(GxRs_Texture0, TextureGetGxTex(s_hTexture, 1, 0));
  ProjectTex2d(srWorldBox, shadowColor, &sunTexMat, 0.5f);
  GxRsPop();
}

static void __fastcall s_BlobFadeTex(
    EGxTexCommand cmd,
    unsigned int  w,
    unsigned int  h,
    unsigned int  d,
    unsigned int  mipLevel,
    void         *userArg,
    unsigned int &texelStrideInBytes,
    const void  *&texels
) {
  if (cmd == GxTex_Lock) {
    s_texels.SetCount(w * h);
  } else if (cmd == GxTex_Latch && !mipLevel) {
    texelStrideInBytes = w * sizeof(NTempest::CImVector);
    texels = s_texels.Ptr();

    unsigned int fadeCount = static_cast<unsigned int>(w * 0.05f);
    unsigned int fadeBase = w - fadeCount - 1;
    for (unsigned int row = 0; row < h; ++row) {
      NTempest::CImVector *texel = s_texels.Ptr() + row * w;
      for (unsigned int column = 0; column < w; ++column) {
        unsigned char alpha;
        if (!column || column == w - 1) {
          alpha = 0;
        } else if (column < fadeBase) {
          alpha = 255;
        } else {
          alpha = static_cast<unsigned char>((1.0f - static_cast<float>(column - fadeBase) / static_cast<float>(fadeCount)) * 255.0f);
        }
        texel[column].Set(alpha, 255, 255, 255);
      }
    }
  }
}

static void __fastcall s_ProjFadeTex(
    EGxTexCommand cmd,
    unsigned int  w,
    unsigned int  h,
    unsigned int  d,
    unsigned int  mipLevel,
    void         *userArg,
    unsigned int &texelStrideInBytes,
    const void  *&texels
) {
  if (cmd == GxTex_Lock) {
    s_texels.SetCount(w * h);
  } else if (cmd == GxTex_Latch && !mipLevel) {
    texelStrideInBytes = w * sizeof(NTempest::CImVector);
    texels = s_texels.Ptr();

    for (unsigned int row = 0; row < h; ++row) {
      NTempest::CImVector *texel = s_texels.Ptr() + row * w;
      for (unsigned int column = 0; column < w; ++column) {
        float position = static_cast<float>(column) / static_cast<float>(w - 1) * 12.0f;
        float alpha;
        if (position < 2.0f) {
          alpha = position * 0.5f;
        } else if (position < 10.0f) {
          alpha = 1.0f;
        } else {
          alpha = (12.0f - position) * 0.5f;
          if (alpha <= 0.0f) {
            alpha = 0.0f;
          }
        }
        texel[column].Set(static_cast<unsigned char>(alpha * 255.0f), 255, 255, 255);
      }
    }
  }
}

void __fastcall ShadowRender(HMODEL hModel, NTempest::C44Matrix &basis, void *param) {
  if (hModel && s_shadowLOD == 1) {
    ShadowRender_LOD1(hModel, basis, param);
  }
}

void __fastcall ShadowInit() {
  CStatus     status;
  CGxTexFlags flags(GxTex_Linear, 0, 0, 0, 0, 0, 1);
  s_hTexture = TextureCreate("Textures\\ShadowBlob.blp", flags, &status, 0);
  GxTexCreate(64, 8, GxTex_Argb8888, flags, 0, s_ProjFadeTex, s_fadeTex);
  ConsoleCommandRegister("shadowLOD", ConsoleCommand_ShadowLOD, GRAPHICS, "0=none, 1=blob");
}

void __fastcall ShadowDestroy() {
  HandleClose(s_hTexture);
  GxTexDestroy(s_fadeTex);
  s_fadeTex = 0;
}

static int __fastcall ConsoleCommand_ShadowLOD(const char *__formal, const char *args) {
  int lod;
  sscanf(args, "%d", &lod);
  if (lod >= 0 && lod <= 1) {
    s_shadowLOD = lod;
    ConsoleWrite("Shadow LOD set", DEFAULT_COLOR);

    NTempest::CiRect updRect(0, 0, 8, 64);
    void(__fastcall * fadeFunc)(EGxTexCommand, unsigned int, unsigned int, unsigned int, unsigned int, void *, unsigned int &, const void *&);
    if (lod == 1) {
      fadeFunc = s_ProjFadeTex;
    } else if (lod == 2) {
      fadeFunc = s_BlobFadeTex;
    } else {
      return 1;
    }
    GxTexSetUserData(s_fadeTex, fadeFunc, 0);
    GxTexUpdate(s_fadeTex, updRect, 0);
    return 1;
  }

  ConsoleWrite("Shadow LOD must be in the range (0, 1)", DEFAULT_COLOR);
  return 0;
}
