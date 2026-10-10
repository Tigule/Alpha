#include "Base/Base.h"
#include "Gx/Gx.h"
#include "Services/ParticleSystem2.h"
#include <WowConst.h>
#include "AaBsp.h"
#include <MapDefs.h>

#include "WorldClient/World.h"
#include "WorldClient/Map.h"
#include "WorldClient/WorldParam.h"
#include "WorldClient/DetailDoodad.h"
#include "WorldClient/CSimpleDoodad.h"
#include "DayNight.h"

#include "WorldClient/Map.h"

#include "ObjectMgrClient/ObjectMgrClient.h"
#include "UIUtil/InputControl.h"
#include "Ui/WorldFrame.h"

#include "Model/IModel.h"
#include "Services/Texture.h"
#include "Tempest/c34matrix.h"

#include <storm.h>

struct SGroupPtr {
  CMapObjDef      *mapObjDef;
  CMapObjDefGroup *mapObjDefGroup;
};

static TSGrowableArray<SGroupPtr> s_groupPtrList;
static NTempest::C44Matrix        s_gxViewMat;
static NTempest::C44Matrix        s_gxWorldMat;

void CMap::TestQueryRender() {
  if (!testQueryVerts.Count()) {
    return;
  }

  GxRsPush();
  GxRsSet(GxRs_PolygonOffset, 1.0f);
  NTempest::C44Matrix cMat;
  cMat.Translate(-CWorldScene::camPos);
  GxXformPush(GxXform_World, cMat);
  GxRsSet(GxRs_Blend, GxBlend_Alpha);
  GxRsSet(GxRs_Lighting, 0);
  GxRsSet(GxRs_DepthWrite, 0);
  GxRsSet(GxRs_DepthTest, 0);
  GxRsSet(GxRs_Culling, 0);
  GxVertexShaderSelect(GxVS_PassThru);
  GxPrimLockVertexPtrs(
      testQueryVerts.Count(), &testQueryVerts[0].p, sizeof(CGxVertexPC), 0, 0, &testQueryVerts[0].c, sizeof(CGxVertexPC), 0, 0, 0, 0, 0, 0
  );
  GxPrimDrawElements(GxPrim_Triangles, testQueryIndices.Count(), testQueryIndices.Ptr());
  GxPrimUnlockVertexPtrs();
  GxXformPop(GxXform_World);
  GxRsPop();
}

void CMap::RenderLow() {
}

void CMap::RenderAreaLow(CMapAreaLow *areaLow) {
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

void CMap::GxBufDynLowDetailCallback(CGxBufCommand &cmd, CGxBuf *buf) {
  CMapAreaLow *areaLow = (CMapAreaLow *)buf->UserArg();

  ASSERT(areaLow);
  CreateAreaLowDetailVertices(areaLow, cmd, buf);
  CreateAreaLowDetailIndices(areaLow, cmd, buf);
}

void CMap::CreateAreaLowDetailVertices(CMapAreaLow *areaLow, const CGxBufCommand &cmd, CGxBuf *buf) {
  UINT row;
  UINT column;

  ASSERT(areaLow);

  CGxVertexPC *vtxBase = 0;
  switch (cmd.vertex.op) {
    case GxBufOp_Nop:
      return;

    case GxBufOp_Fill:
      vtxBase = (CGxVertexPC *)*cmd.vertex.mem[GxVM_Position];
      break;

    case GxBufOp_Assign:
      vtxBase = (CGxVertexPC *)GxAllocVertexMem(buf->VertexCount() * sizeof(CGxVertexPC));
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

void CMap::CreateAreaLowDetailIndices(CMapAreaLow *areaLow, const CGxBufCommand &cmd, CGxBuf *buf) {
  UINT row;
  UINT column;

  ASSERT(areaLow);

  WORD *idx = 0;
  switch (cmd.index.op) {
    case GxBufOp_Nop:
      return;

    case GxBufOp_Fill:
      idx = (WORD *)*cmd.index.mem[GxVM_Indices];
      break;

    case GxBufOp_Assign:
      idx = (WORD *)GxAllocIndexMem(buf->IndexCount() * sizeof(WORD));
      *cmd.index.mem[GxVM_Indices] = idx;
      break;
  }

  ASSERT(idx);

  UINT center = 289;
  UINT topLeft = 0;
  for (row = 0; row < 16; ++row, ++topLeft) {
    UINT bottomRight = topLeft + 18;
    for (column = 0; column < 16; ++column, ++center, ++topLeft, ++bottomRight) {
      *idx++ = center;
      *idx++ = bottomRight - 17;
      *idx++ = topLeft;

      *idx++ = center;
      *idx++ = bottomRight;
      *idx++ = bottomRight - 17;

      *idx++ = center;
      *idx++ = bottomRight - 1;
      *idx++ = bottomRight;

      *idx++ = center;
      *idx++ = topLeft;
      *idx++ = bottomRight - 1;
    }
  }
}

static void Billboard(const NTempest::C3Vector &dir, NTempest::C44Matrix &mat) {
  NTempest::C3Vector &basisX = *mat.Row0AsVec3();
  NTempest::C3Vector &basisY = *mat.Row1AsVec3();
  NTempest::C3Vector &basisZ = *mat.Row2AsVec3();

  basisX = dir;
  basisX.Normalize();
  basisY.Set(-basisX.y, basisX.x, 0.0f);
  NTempest::CMath::normalize_(basisY.x, basisY.y);
  basisZ = NTempest::C3Vector(
      basisY.z * basisX.y - basisY.y * basisX.z, basisX.z * basisY.x - basisX.x * basisY.z, basisY.y * basisX.x - basisY.x * basisX.y
  );
}

BOOL DNGlare::IsVisible() {
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
  *worldMat.Row3AsVec3() = m_pos - dnInfo->cameraPos;
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
  WORD                billbIdx[8];
  DWORD               idxCount;
  DWORD               vertCount;

  GenGeometry(billbGeov, billbTexv, billbClrv, billbIdx, vertCount, idxCount);

  if (!vertCount) {
    return;
  }

  GxRsPush();

  DNInfo            *dnInfo = DayNightGetInfo();
  NTempest::C3Vector worldFaceDir = m_pos - dnInfo->cameraPos;

  NTempest::C44Matrix worldMat;
  Billboard(worldFaceDir, worldMat);
  *worldMat.Row3AsVec3() = m_pos - dnInfo->cameraPos;
  GxXformPush(GxXform_World, worldMat);
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
  if (m_nLayers <= 0) {
    return;
  }

  GxRsPush();
  GxRsSet(GxRs_FogStart, m_fogInfo.start);
  GxRsSet(GxRs_FogEnd, m_fogInfo.end);
  GxRsSet(GxRs_FogDensity, 0.0f);
  GxRsSet(GxRs_FogColor, m_fogInfo.color);
  GxVertexShaderSelect(GxVS_PassThru);
  GxRsSet(GxRs_Blend, GxBlend_Alpha);
  GxRsSet(GxRs_Lighting, 0);
  GxRsSet(GxRs_DepthWrite, 0);
  GxRsSet(GxRs_DepthTest, 0);
  GxRsSet(GxRs_Culling, 0);
  GxRsSet(GxRs_Texture0, m_texid);
  GxRsSet(GxRs_TexBlend0, GxTexBlend_Mod);
  NTempest::CImVector white(0xFFFFFFFF);
  GxPrimLockVertexPtrs(
      m_nVerts, &m_geoVerts[0], sizeof(NTempest::C3Vector), 0, 0, &white, 0, 0, 0, &m_texVerts[0], sizeof(NTempest::C2Vector), 0, 0
  );
  GxPrimDrawElements(GxPrim_TriangleStrip, m_nIndices, &m_indices[0]);
  GxPrimUnlockVertexPtrs();
  GxRsPop();
}

void DNSky::Render() {
  float vp[6];

  GxXformViewport(vp[0], vp[1], vp[2], vp[3], vp[4], vp[5]);
  GxXformSetViewport(vp[0], vp[1], vp[2], vp[3], 1.0f, 1.0f);

  NTempest::C44Matrix worldScale;
  worldScale.Scale(CWorld::farClip * 0.5f);
  GxXformPush(GxXform_World, worldScale);

  NTempest::C44Matrix saveViewMat;
  GxXformView(saveViewMat);

  NTempest::C3Vector zv(1.0f, 0.0f, saveViewMat.c2);
  zv.Normalize();

  NTempest::C44Matrix viewMat;
  GxuXformCreateLookAtSgCompat(NTempest::C3Vector(0.0f, 0.0f, 0.0f), zv, NTempest::C3Vector(0.0f, 0.0f, 1.0f), viewMat);
  GxXformSetView(viewMat);

  GxRsPush();
  GxVertexShaderSelect(GxVS_PassThru);
  GxRsSet(GxRs_Lighting, 0);
  GxRsSet(GxRs_Fog, 0);
  GxRsSet(GxRs_DepthTest, 0);
  GxRsSet(GxRs_Culling, 0);
  GxPrimLockVertexPtrs(m_nVerts, &m_geoVerts[0], sizeof(NTempest::C3Vector), 0, 0, &m_clrVerts[0], sizeof(NTempest::CImVector), 0, 0, 0, 0, 0, 0);
  GxPrimDrawElements(GxPrim_TriangleStrip, m_nIndices, &m_indices[0]);
  GxPrimUnlockVertexPtrs();
  GxXformSetView(saveViewMat);
  GxXformPop(GxXform_World);
  GxXformSetViewport(vp[0], vp[1], vp[2], vp[3], vp[4], vp[5]);
  GxRsPop();
}

void DNStars::Render() {
  if (m_color.a > 1) {
    ModelAnimate(m_hModel, NTempest::C34Matrix(), 1.0f, NTempest::C3Vector(), NTempest::C3Vector());
    ModelSetVertexAlpha(m_hModel, m_color.a, 0);
    ModelRender(m_hModel, 0, 0);
  }
}
