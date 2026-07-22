#include "WorldClient/World.h"

#include "DayNight.h"
#include "Model/IModel.h"
#include "Services/Texture.h"
#include "Tempest/c34matrix.h"

#include <storm.h>

void __fastcall CMap::TestQueryRender() {
  NTempest::C44Matrix cMat;

  if (!testQueryVerts.Count()) {
    return;
  }

  GxRsPush();
  GxRsSet(GxRs_TexLodBias0, 1.0f);
  cMat.Translate(-CWorldScene::camPos);
  GxXformSet(GxXform_World, cMat);
  GxRsSet(GxRs_Blend, GxBlend_Alpha);
  GxRsSet(GxRs_Lighting, 0);
  GxRsSet(GxRs_DepthWrite, 0);
  GxRsSet(GxRs_DepthTest, 0);
  GxRsSet(GxRs_Culling, 0);
  GxVertexShaderSelect(GxVS_PassThru);
  GxPrimLockVertexPtrs(
      testQueryVerts.Count(), &testQueryVerts[0].p, sizeof(CGxVertexPC), 0, 0, &testQueryVerts[0].c, sizeof(CGxVertexPC), 0, 0, 0, 0, 0, 0
  );
  GxPrimDrawElements(GxPrim_Triangles, testQueryIndices.Count(), &testQueryIndices[0]);
  GxPrimUnlockVertexPtrs();
  GxRsPop();
  testQueryVerts.SetCount(0);
  testQueryIndices.SetCount(0);
}

void __fastcall CMap::RenderLow() {
}

void __fastcall CMap::RenderAreaLow(CMapAreaLow *areaLow) {
  GxRsPush();
  GxRsSet(GxRs_Blend, GxBlend_Opaque);
  GxRsSet(GxRs_Lighting, 0);
  GxRsSet(GxRs_Culling, 0);
  GxVertexShaderSelect(GxVS_PassThru);
  gxBufDynLowDetail->UserArgSet(areaLow);
  GxBufLock(gxBufDynLowDetail);

  CGxBatch gxBatch(GxPrim_Triangles, 3072, 0, -1, -1);
  GxBufRender(gxBatch);
  GxBufUnlock();
  GxRsPop();
}

void __fastcall CMap::GxBufDynLowDetailCallback(CGxBufCommand &cmd, CGxBuf *buf) {
  CMapAreaLow *areaLow = static_cast<CMapAreaLow *>(buf->UserArg());

  ASSERT(areaLow);
  CreateAreaLowDetailVertices(areaLow, cmd, buf);
  CreateAreaLowDetailIndices(areaLow, cmd, buf);
}

void __fastcall CMap::CreateAreaLowDetailVertices(CMapAreaLow *areaLow, const CGxBufCommand &cmd, CGxBuf *buf) {
  CGxVertexPC *vtxBase = 0;
  unsigned int row;
  unsigned int column;

  ASSERT(areaLow);

  switch (cmd.vertex.op) {
    case GxBufOp_Nop:
      return;

    case GxBufOp_Fill:
      vtxBase = static_cast<CGxVertexPC *>(*cmd.vertex.mem[GxVM_Position]);
      break;

    case GxBufOp_Assign:
      vtxBase = static_cast<CGxVertexPC *>(GxAllocVertexMem(buf->VertexCount() * sizeof(CGxVertexPC)));
      *cmd.vertex.mem[GxVM_Position] = &vtxBase->p;
      *cmd.vertex.mem[GxVM_Color] = &vtxBase->c;
      break;
  }

  ASSERT(vtxBase);

  DNInfo *dnInfo = DayNightGetInfo();
  ASSERT(dnInfo);

  float x = areaLow->corner.x;
  for (row = 0; row < 17; ++row) {
    float y = areaLow->corner.y;

    for (column = 0; column < 17; ++column) {
      vtxBase->p.x = x;
      vtxBase->p.y = y;
      vtxBase->p.z = areaLow->heights[row * 17 + column];
      vtxBase->c = dnInfo->fogInfo.color;
      ++vtxBase;
      y -= 33.333332f;
    }

    x -= 33.333332f;
  }

  x = areaLow->corner.x - 16.666666f;
  for (row = 0; row < 16; ++row) {
    float y = areaLow->corner.y - 16.666666f;

    for (column = 0; column < 16; ++column) {
      vtxBase->p.x = x;
      vtxBase->p.y = y;
      vtxBase->p.z = areaLow->heights[289 + row * 16 + column];
      vtxBase->c = dnInfo->fogInfo.color;
      ++vtxBase;
      y -= 33.333332f;
    }

    x -= 33.333332f;
  }
}

void __fastcall CMap::CreateAreaLowDetailIndices(CMapAreaLow *areaLow, const CGxBufCommand &cmd, CGxBuf *buf) {
  unsigned short *idx = 0;
  unsigned int    row;
  unsigned int    column;

  ASSERT(areaLow);

  switch (cmd.index.op) {
    case GxBufOp_Nop:
      return;

    case GxBufOp_Fill:
      idx = static_cast<unsigned short *>(*cmd.index.mem[GxVM_Indices]);
      break;

    case GxBufOp_Assign:
      idx = static_cast<unsigned short *>(GxAllocIndexMem(buf->IndexCount() * sizeof(unsigned short)));
      *cmd.index.mem[GxVM_Indices] = idx;
      break;
  }

  ASSERT(idx);

  for (row = 0; row < 16; ++row) {
    for (column = 0; column < 16; ++column) {
      unsigned short topLeft = row * 17 + column;
      unsigned short topRight = topLeft + 1;
      unsigned short bottomLeft = topLeft + 17;
      unsigned short bottomRight = topLeft + 18;
      unsigned short center = 289 + row * 16 + column;

      *idx++ = center;
      *idx++ = topRight;
      *idx++ = topLeft;

      *idx++ = center;
      *idx++ = bottomRight;
      *idx++ = topRight;

      *idx++ = center;
      *idx++ = bottomLeft;
      *idx++ = bottomRight;

      *idx++ = center;
      *idx++ = topLeft;
      *idx++ = bottomLeft;
    }
  }
}

static void __fastcall Billboard(const NTempest::C3Vector &dir, NTempest::C44Matrix &mat) {
  mat.a0 = dir.x;
  mat.a1 = dir.y;
  mat.a2 = dir.z;

  NTempest::C3Vector basisZ(mat.a0, mat.a1, mat.a2);
  basisZ.Normalize();
  mat.a0 = basisZ.x;
  mat.a1 = basisZ.y;
  mat.a2 = basisZ.z;

  mat.b0 = -mat.a1;
  mat.b1 = mat.a0;
  mat.b2 = 0.0f;
  float ooMag = 1.0f / NTempest::CMath::sqrt_(mat.b0 * mat.b0 + mat.b1 * mat.b1);
  mat.b0 *= ooMag;
  mat.b1 *= ooMag;

  mat.c0 = mat.b2 * mat.a1 - mat.b1 * mat.a2;
  mat.c1 = mat.a2 * mat.b0 - mat.a0 * mat.b2;
  mat.c2 = mat.b1 * mat.a0 - mat.b0 * mat.a1;
}

int DNGlare::IsVisible() {
  if (!m_masterEnable || !m_enabled || !m_color.a) {
    return 0;
  }

  DNInfo            *dnInfo = DayNightGetInfo();
  NTempest::C3Vector glareDir = m_pos - dnInfo->cameraPos;
  glareDir.Normalize();

  NTempest::C3Vector startPt = dnInfo->cameraPos + glareDir * 0.5f;
  NTempest::C3Vector testPt = dnInfo->cameraPos + glareDir * dnInfo->farClip;
  NTempest::C3Vector hitPt(0.0f, 0.0f, 0.0f);
  float              hitDist = 1.0f;

  if (CWorld::Intersect(&startPt, &testPt, 0.0f, &hitPt, &hitDist, 0x1112)) {
    return 0;
  }

  return 1;
}

void DNGlare::Render() {
  if (!m_masterEnable || !m_enabled || !m_color.a) {
    return;
  }

  DNInfo             *dnInfo = DayNightGetInfo();
  NTempest::C3Vector  glareDir = m_pos - dnInfo->cameraPos;
  NTempest::C44Matrix worldMat;
  Billboard(glareDir, worldMat);
  worldMat.d0 = glareDir.x;
  worldMat.d1 = glareDir.y;
  worldMat.d2 = glareDir.z;
  worldMat.Scale(NTempest::C3Vector(m_curScale));

  GxXformPush(GxXform_World, worldMat);
  GxRsPush();
  GxVertexShaderSelect(GxVS_PassThru);
  GxRsSet(GxRs_Blend, GxBlend_Add);
  GxRsSet(GxRs_Lighting, 0);
  GxRsSet(GxRs_Fog, 0);
  GxRsSet(GxRs_DepthWrite, 0);
  GxRsSet(GxRs_DepthTest, 0);
  GxRsSet(GxRs_Culling, 0);
  GxRsSet(GxRs_Texture0, TextureGetGxTex(m_texid, 1, 0));
  GxRsSet(GxRs_TexBlend0, GxTexBlend_Mod);
  GxPrimLockVertexPtrs(4, m_geov, sizeof(NTempest::C3Vector), 0, 0, &m_color, 0, 0, 0, m_texv, sizeof(NTempest::C2Vector), 0, 0);
  GxPrimDrawElements(GxPrim_TriangleStrip, 4, m_idx);
  GxPrimUnlockVertexPtrs();
  GxXformPop(GxXform_World);
  GxRsPop();
}

void DNPlanet::Render() {
  NTempest::C3Vector  billbGeov[6];
  NTempest::C2Vector  billbTexv[6];
  NTempest::CImVector billbClrv[6];
  unsigned short      billbIdx[8];
  NTempest::C44Matrix worldMat;
  unsigned long       idxCount;
  unsigned long       vertCount;
  NTempest::C3Vector  worldFaceDir;

  GenGeometry(billbGeov, billbTexv, billbClrv, billbIdx, vertCount, idxCount);

  if (!vertCount) {
    return;
  }

  worldFaceDir = m_pos - DayNightGetInfo()->cameraPos;
  Billboard(worldFaceDir, worldMat);
  worldMat.d0 = worldFaceDir.x;
  worldMat.d1 = worldFaceDir.y;
  worldMat.d2 = worldFaceDir.z;
  GxXformPush(GxXform_World, worldMat);
  GxRsPush();
  GxVertexShaderSelect(GxVS_PassThru);
  GxRsSet(GxRs_Blend, GxBlend_Alpha);
  GxRsSet(GxRs_Lighting, 0);
  GxRsSet(GxRs_Fog, 0);
  GxRsSet(GxRs_DepthWrite, 0);
  GxRsSet(GxRs_DepthTest, 0);
  GxRsSet(GxRs_Culling, 0);
  GxRsSet(GxRs_Texture0, TextureGetGxTex(m_texid, 1, 0));
  GxRsSet(GxRs_TexBlend0, GxTexBlend_Mod);
  GxPrimLockVertexPtrs(
      vertCount, billbGeov, sizeof(billbGeov[0]), 0, 0, billbClrv, sizeof(billbClrv[0]), 0, 0, billbTexv, sizeof(billbTexv[0]), 0, 0
  );
  GxPrimDrawElements(GxPrim_TriangleStrip, idxCount, billbIdx);
  GxPrimUnlockVertexPtrs();
  GxXformPop(GxXform_World);
  GxRsPop();
}

void DNClouds::Render() {
  NTempest::CImVector white(0xFFFFFFFF);

  if (!m_nIndices) {
    return;
  }

  GxRsPush();
  GxRsSet(GxRs_FogStart, m_fogInfo.start);
  GxRsSet(GxRs_FogEnd, m_fogInfo.end);
  GxRsSet(GxRs_FogDensity, 0.0f);
  GxRsSet(GxRs_FogColor, m_fogInfo.color);
  GxVertexShaderSelect(GxVS_PassThru);
  GxRsSet(GxRs_Blend, GxBlend_Opaque);
  GxRsSet(GxRs_Lighting, 0);
  GxRsSet(GxRs_DepthWrite, 0);
  GxRsSet(GxRs_DepthTest, 0);
  GxRsSet(GxRs_Culling, 0);
  GxRsSet(GxRs_Texture0, m_texid);
  GxRsSet(GxRs_TexBlend0, GxTexBlend_Mod);
  GxPrimLockVertexPtrs(
      m_nVerts, m_geoVerts.Ptr(), sizeof(NTempest::C3Vector), 0, 0, &white, 0, 0, 0, m_texVerts.Ptr(), sizeof(NTempest::C2Vector), 0, 0
  );
  GxPrimDrawElements(GxPrim_TriangleStrip, m_nIndices, m_indices.Ptr());
  GxPrimUnlockVertexPtrs();
  GxRsPop();
}

void DNSky::Render() {
  NTempest::C44Matrix viewMat;
  NTempest::C44Matrix worldScale;
  NTempest::C44Matrix saveViewMat;
  float               vp[6];
  NTempest::C3Vector  zv;

  GxXformViewport(vp[0], vp[1], vp[2], vp[3], vp[4], vp[5]);
  GxXformSetViewport(vp[0], vp[1], vp[2], vp[3], 1.0f, 1.0f);

  worldScale = NTempest::C44Matrix();
  worldScale.Scale(DayNightGetInfo()->farClip * 0.5f);
  GxXformPush(GxXform_World, worldScale);

  GxXformView(saveViewMat);
  zv = NTempest::C3Vector(saveViewMat.c0, saveViewMat.c1, saveViewMat.c2);
  zv.Normalize();
  NTempest::C3Vector eye(0.0f, 0.0f, 0.0f);
  NTempest::C3Vector up(0.0f, 0.0f, 1.0f);
  GxuXformCreateLookAtSgCompat(eye, zv, up, viewMat);
  GxXformSetView(viewMat);

  GxRsPush();
  GxVertexShaderSelect(GxVS_PassThru);
  GxRsSet(GxRs_Lighting, 0);
  GxRsSet(GxRs_Fog, 0);
  GxRsSet(GxRs_DepthTest, 0);
  GxRsSet(GxRs_Culling, 0);
  GxPrimLockVertexPtrs(m_nVerts, m_geoVerts.Ptr(), sizeof(NTempest::C3Vector), 0, 0, m_clrVerts.Ptr(), sizeof(NTempest::CImVector), 0, 0, 0, 0, 0, 0);
  GxPrimDrawElements(GxPrim_TriangleStrip, m_nIndices, m_indices.Ptr());
  GxPrimUnlockVertexPtrs();
  GxXformSetView(saveViewMat);
  GxXformPop(GxXform_World);
  GxXformSetViewport(vp[0], vp[1], vp[2], vp[3], vp[4], vp[5]);
  GxRsPop();
}

void DNStars::Render() {
  if (m_color.a > 1) {
    NTempest::C34Matrix orientation;
    NTempest::C3Vector  cameraPos;
    NTempest::C3Vector  cameraVector;
    ModelAnimate(m_hModel, orientation, 1.0f, cameraPos, cameraVector);
    ModelSetVertexAlpha(m_hModel, m_color.a, 0);
    ModelAddToScene(m_hModel, 0);
  }
}
