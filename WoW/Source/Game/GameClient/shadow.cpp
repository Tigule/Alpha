#include <Base/Base.h>
#include <Gx/Gx.h>
#include <MapDefs.h>
#include <WorldClient/World.h>
#include <WowConst.h>
#include <DayNight.h>

#include "WorldClient/World.h"

#include "Base/Status.h"
#include "Console/ConsoleClient.h"
#include "Console/ConsoleCommand.h"
#include "Gx/Gx.h"
#include "Model/IModel.h"
#include "Services/Texture.h"
#include "Ui/WorldFrame.h"

#include "Tempest/c33matrix.h"
#include "Tempest/c44matrix.h"
#include "Tempest/crect.h"

#include <math.h>

static float FeetToWorld(float ft);

static const float EXTENT_SCALE = 1.0f;
static const float SHADOW_ALPHA_SCALE = 1.5f;
static const float BLOB_BELOW = FeetToWorld(5.0f), BLOB_ABOVE = FeetToWorld(3.0f);
static const float SHADOW_POLY_OFFSET = 0.0625f;

static float FeetToWorld(float ft) {
  return ft * 0.33333334f;
}

static CGxTex                            *s_fadeTex;
static HTEXTURE                           s_hTexture;
static TSCArray<NTempest::CImVector, 512> s_texels;
static CWFacetData                        s_facetData;
static const CWTriData::Batch            *s_batch;
static NTempest::C3Vector                 s_zup(0.0f, 0.0f, 1.0f);
static NTempest::CImVector                s_color;
static int                                s_shadowLOD = 1;

static BOOL ConsoleCommand_ShadowLOD(LPCSTR, LPCSTR args);

CGxTex *ProjectTex2dGetFade() {
  return s_fadeTex;
}

void ProjectTex2dMakeMatrices(
    NTempest::C44Matrix       &texmat0,
    NTempest::C44Matrix       &texmat1,
    const NTempest::CAaBox    &box,
    const NTempest::C44Matrix *basis,
    float                      fadeOffset,
    int                        inWorldSpace
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
  NTempest::C44Matrix worldToTexture = NTempest::C44Matrix::Rotation(PI * -0.5f, NTempest::C3Vector(0.0f, 0.0f, 1.0f), 1);
  texmat0 = worldTransMat * texScale * worldToTexture;
  if (basis) {
    texmat0 *= *basis;
  }
  texmat0.d0 += 0.5f;
  texmat0.d1 += 0.5f;

  NTempest::C44Matrix scaleTransMapRtoS(
      0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f / boxSize.z, 0.0f, 1.0f / boxSize.z, 0.0f, fadeOffset, 0.5f, fadeOffset, 1.0f
  );
  texmat1 = worldTransMat * scaleTransMapRtoS;
}

static void ProjectTexRenderVerticesPN(const CGxBufCommand &cmd, CGxBuf *buf) {
  CGxVertexPN *vertices = 0;
  switch (cmd.vertex.op) {
    case GxBufOp_Assign:
      vertices = (CGxVertexPN *)GxAllocVertexMem(buf->VertexCount() * sizeof(*vertices));
      *cmd.vertex.mem[GxVM_Position] = &vertices->p;
      *cmd.vertex.mem[GxVM_Normal] = &vertices->n;
      break;
    case GxBufOp_Fill:
      vertices = (CGxVertexPN *)*cmd.vertex.mem[GxVM_Position];
      break;
    case GxBufOp_Nop:
      FATALASSERT(0);
      return;
  }

  WORD vidx = s_batch->GetMinIndex();
  for (UINT i = 0; i < s_batch->GetVertexCount(); ++i, ++vertices, ++vidx) {
    vertices->p = s_batch->GetVertex(vidx);
    vertices->n = s_zup;
  }
}

static void ProjectTexRenderVerticesPC(const CGxBufCommand &cmd, CGxBuf *buf) {
  CGxVertexPC *vertices = 0;
  switch (cmd.vertex.op) {
    case GxBufOp_Assign:
      vertices = (CGxVertexPC *)GxAllocVertexMem(buf->VertexCount() * sizeof(*vertices));
      *cmd.vertex.mem[GxVM_Position] = &vertices->p;
      *cmd.vertex.mem[GxVM_Color] = &vertices->c;
      break;
    case GxBufOp_Fill:
      vertices = (CGxVertexPC *)*cmd.vertex.mem[GxVM_Position];
      break;
    case GxBufOp_Nop:
      FATALASSERT(0);
      return;
  }

  WORD vidx = s_batch->GetMinIndex();
  for (UINT i = 0; i < s_batch->GetVertexCount(); ++i, ++vertices, ++vidx) {
    vertices->p = s_batch->GetVertex(vidx);
    vertices->c = s_color;
  }
}

static void ProjectTexRenderIndices(const CGxBufCommand &cmd, CGxBuf *buf) {
  WORD *indices = 0;
  switch (cmd.index.op) {
    case GxBufOp_Assign:
      indices = (WORD *)GxAllocIndexMem(buf->IndexCount() * sizeof(*indices));
      *cmd.index.mem[GxVM_Indices] = indices;
      break;
    case GxBufOp_Fill:
      indices = (WORD *)*cmd.index.mem[GxVM_Indices];
      break;
    case GxBufOp_Nop:
      FATALASSERT(0);
      return;
  }

  for (UINT i = 0; i < s_batch->GetIndexCount(); ++i) {
    indices[i] = s_batch->GetIndex(i) - s_batch->GetMinIndex();
  }
}

static void ProjectTexRenderPC(CGxBufCommand &cmd, CGxBuf *buf) {
  ProjectTexRenderVerticesPC(cmd, buf);
  ProjectTexRenderIndices(cmd, buf);
}

static void ProjectTexRenderPN(CGxBufCommand &cmd, CGxBuf *buf) {
  ProjectTexRenderVerticesPN(cmd, buf);
  ProjectTexRenderIndices(cmd, buf);
}

void ProjectTex2d(const NTempest::CAaBox &box, NTempest::CImVector color, const NTempest::C44Matrix *basis, float fadeOffset) {
  CWTriData triData;
  if (!CWorld::GetTris(box, triData, 0x122)) {
    return;
  }

  NTempest::C44Matrix texmtx0;
  NTempest::C44Matrix texmtx1;
  ProjectTex2dMakeMatrices(texmtx0, texmtx1, box, basis, fadeOffset, 0);
  GxXformPush(GxXform_Tex0, texmtx0);
  GxXformPush(GxXform_Tex1, texmtx1);
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
      s_color = NTempest::CImVector(s_color.a, s_color.b, s_color.g, s_color.r);
    }
    gxBuf = GxBufGetDynamic(GxVBF_PC);
    gxBuf->UserCallbackSet(ProjectTexRenderPC);
  }

  NTempest::C3Vector cameraPos;
  CGWorldFrame::GetCameraPosition(&cameraPos);
  NTempest::C44Matrix worldMtx;
  *worldMtx.Row3AsVec3() = -cameraPos;
  GxXformPush(GxXform_World);

  for (UINT i = 0; i < triData.GetNumBatches(); ++i) {
    const CWTriData::Batch &batch = triData.GetBatch(i);
    if (batch.GetVertexCount() <= Gx_MaxVertices && batch.GetIndexCount() <= Gx_MaxIndices) {
      NTempest::C44Matrix batchMtx = *batch.matrix * worldMtx;
      GxXformSet(GxXform_World, batchMtx);
      gxBuf->CountSet(batch.GetVertexCount(), batch.GetIndexCount());
      s_batch = (CWTriData::Batch *)&batch;
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

static void ShadowRender_LOD1(HMODEL hModel, const NTempest::C44Matrix &basis, LPVOID param) {
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
  if (NTempest::CMath::fnotequal_(matScale, 1.0f)) {
    normBasis.Scale(NTempest::C3Vector(1.0f / matScale));
  }

  NTempest::C3Vector basedExtents[4];
  basedExtents[0] = NTempest::C3Vector(extents.t.x, extents.t.y, 0.0f) * normBasis;
  basedExtents[1] = NTempest::C3Vector(extents.t.x, extents.b.y, 0.0f) * normBasis;
  basedExtents[2] = NTempest::C3Vector(extents.b.x, extents.b.y, 0.0f) * normBasis;
  basedExtents[3] = NTempest::C3Vector(extents.b.x, extents.t.y, 0.0f) * normBasis;

  NTempest::CAaBox srWorldBox = NTempest::CAaBox::Bounding(basedExtents, 4);
  srWorldBox.b += cameraPos + *basis.Row3AsVec3();
  srWorldBox.t += cameraPos + *basis.Row3AsVec3();
  srWorldBox.b.z = cameraPos.z + basis.d2 - BLOB_BELOW * height;
  srWorldBox.t.z = BLOB_ABOVE * height + basis.d2 + cameraPos.z;

  NTempest::C33Matrix undoScaleMat;
  undoScaleMat.Scale(NTempest::C3Vector(srWorldBox.t.y - srWorldBox.b.y, srWorldBox.t.x - srWorldBox.b.x, 1.0f));

  NTempest::C33Matrix shadowScaleMat;
  shadowScaleMat.Scale(1.0f / fabs(extents.t.y - extents.b.y), 1.0f / fabs(extents.t.x - extents.b.x), 1.0f);

  NTempest::C33Matrix basisRotMat = normBasis.Transpose();
  NTempest::C44Matrix sunTexMat(undoScaleMat * basisRotMat * shadowScaleMat);

  NTempest::CImVector shadowColor = DayNightGetInfo()->shadowClr;
  shadowColor.a = shadowColor.a * SHADOW_ALPHA_SCALE > 255.0f ? 255.0f : shadowColor.a * SHADOW_ALPHA_SCALE;
  shadowColor.r = shadowColor.r * 0.65f;
  shadowColor.g = shadowColor.g * 0.65f;
  shadowColor.b = shadowColor.b * 0.65f;

  GxRsPush();
  GxRsSet(GxRs_Blend, GxBlend_Alpha);
  GxRsSet(GxRs_Lighting, 0);
  GxRsSet(GxRs_Culling, 0);
  GxRsSet(GxRs_DepthWrite, 0);
  GxRsSet(GxRs_Texture0, TextureGetGxTex(s_hTexture, 1, 0));
  ProjectTex2d(srWorldBox, shadowColor, &sunTexMat, 0.5f);
  GxRsPop();
}

static void s_ProjFadeTex(EGxTexCommand cmd, UINT w, UINT h, UINT d, UINT mipLevel, LPVOID userArg, UINT &texelStrideInBytes, LPCVOID &texels) {
  switch (cmd) {
    case GxTex_Latch:
      if (!mipLevel) {
        texelStrideInBytes = w * sizeof(NTempest::CImVector);
        texels = s_texels.Ptr();

        UINT fadeCount = w * 0.05f;
        UINT fadeBase = w - fadeCount - 1;
        for (UINT y = 0; y < h; ++y) {
          NTempest::CImVector *tex = s_texels.Ptr() + y * w;
          for (UINT x = 0; x < w; ++x) {
            if (x == 0 || x == w - 1) {
              tex[x] = NTempest::CImVector(0, 255, 255, 255);
            } else if (x >= fadeBase) {
              tex[x] = NTempest::CImVector((1.0f - (float)(x - fadeBase) / fadeCount) * 255.0f, 255, 255, 255);
            } else {
              tex[x] = NTempest::CImVector(255, 255, 255, 255);
            }
          }
        }
      }
      break;

    case GxTex_Lock:
      s_texels.SetCount(w * h);
      break;
  }
}

static void s_BlobFadeTex(EGxTexCommand cmd, UINT w, UINT h, UINT d, UINT mipLevel, LPVOID userArg, UINT &texelStrideInBytes, LPCVOID &texels) {
  switch (cmd) {
    case GxTex_Latch:
      if (!mipLevel) {
        texelStrideInBytes = w * sizeof(NTempest::CImVector);
        texels = s_texels.Ptr();

        for (UINT y = 0; y < h; ++y) {
          NTempest::CImVector *tex = s_texels.Ptr() + y * w;
          for (UINT x = 0; x < w; ++x) {
            float position = (float)x / (float)(w - 1) * 12.0f;
            float alpha;
            if (position < 2.0f) {
              alpha = position * 0.5f;
            } else if (position < 10.0f) {
              alpha = 1.0f;
            } else {
              alpha = __max((12.0f - position) * 0.5f, 0.0f);
            }
            tex[x] = NTempest::CImVector(alpha * 255.0f, 255, 255, 255);
          }
        }
      }
      break;

    case GxTex_Lock:
      s_texels.SetCount(w * h);
      break;
  }
}

void ShadowRender(HMODEL hModel, const NTempest::C44Matrix &basis, LPVOID param) {
  if (hModel) {
    switch (s_shadowLOD) {
      case 1:
        ShadowRender_LOD1(hModel, basis, param);
        break;
    }
  }
}

void ShadowInit() {
  CStatus status;
  s_hTexture = TextureCreate("Textures\\ShadowBlob.blp", CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &status, 0);
  GxTexCreate(64, 8, GxTex_Argb8888, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), 0, s_BlobFadeTex, s_fadeTex);
  ConsoleCommandRegister("shadowLOD", ConsoleCommand_ShadowLOD, GRAPHICS, "0=none, 1=blob");
}

void ShadowDestroy() {
  HandleClose(s_hTexture);
  GxTexDestroy(s_fadeTex);
  s_fadeTex = 0;
}

static BOOL ConsoleCommand_ShadowLOD(LPCSTR, LPCSTR args) {
  int lod;
  sscanf(args, "%d", &lod);
  if (lod >= 0 && lod <= 1) {
    s_shadowLOD = lod;
    ConsoleWrite("Shadow LOD set", DEFAULT_COLOR);

    NTempest::CiRect updRect(0, 0, 8, 64);
    switch (lod) {
      case 1:
        GxTexSetUserData(s_fadeTex, s_BlobFadeTex, 0);
        GxTexUpdate(s_fadeTex, updRect, 0);
        break;

      case 2:
        GxTexSetUserData(s_fadeTex, s_ProjFadeTex, 0);
        GxTexUpdate(s_fadeTex, updRect, 0);
        break;
    }
    return 1;
  }

  ConsoleWrite("Shadow LOD must be in the range (0, 1)", DEFAULT_COLOR);
  return 0;
}
