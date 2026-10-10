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

#include <Ftol.h>

#include "ObjectMgrClient/ObjectMgrClient.h"
#include "UIUtil/InputControl.h"
#include "Ui/WorldFrame.h"

#include "Model/IModel.h"
#include "Services/SysMessage.h"
#include "Tempest/c33matrix.h"
#include "Tempest/c34matrix.h"
#include "Tempest/cmath.h"

#include <new>
#include <string.h>

void ShadowRender(HMODEL hModel, const NTempest::C44Matrix &basis, LPVOID param);

static const float CBSCALE = 64.0f;
static const float CBDIVISOR = 0.03f;

static UINT s_boxCornerIndicesX[8] = {0, 1, 1, 0, 0, 1, 1, 0};
static UINT s_boxCornerIndicesY[8] = {0, 0, 1, 1, 0, 0, 1, 1};
static UINT s_boxCornerIndicesZ[8] = {0, 0, 0, 0, 1, 1, 1, 1};

NTempest::C4Vector    CWorldScene::clipVertexBuffer[9];
CSortTable            CWorldScene::sortTable;
float                 CWorldScene::clipBuffer[128];
CMapEntity           *CWorldScene::camTargEntity;
CMapObjDef           *CWorldScene::viewerMapObjDef;
TSGrowableArray<UINT> CWorldScene::viewerMapObjGroups;
UINT                  CWorldScene::bspStateBits;
float                 CWorldScene::cullSmallThreshold = 0.01f;
float                 CWorldScene::cullDistance = 500.0f;
NTempest::CAaBox      CWorldScene::camFrustumBounds;
NTempest::C3Vector    CWorldScene::camFrustumCorners[8];
CWFrustum             CWorldScene::frustumStack[16];
int                   CWorldScene::frustumIndex;
NTempest::C3Vector    CWorldScene::camPos;
NTempest::C3Vector    CWorldScene::camTarg;
NTempest::C3Vector    CWorldScene::camVec;
NTempest::C4Plane     CWorldScene::camPlaneXY;
UINT                  CWorldScene::camLiquid;
CMapObjDef           *CWorldScene::camMapObjDef;
CMapObj              *CWorldScene::camMapObj;
CMapObjGroup         *CWorldScene::camMapObjGroup;
NTempest::C44Matrix   CWorldScene::mvp;
NTempest::C44Matrix   CWorldScene::mv;
NTempest::C44Matrix   CWorldScene::mp;
NTempest::C3Vector    CWorldScene::vpMinPos;
NTempest::C3Vector    CWorldScene::vpMaxPos;
NTempest::C4Plane     CWorldScene::vpPlanes[4];
NTempest::C4Vector    CWorldScene::mvpCol3;
NTempest::C44Matrix   CWorldScene::gxViewMat;
LISTDECLEX(CWFrustum, sceneLink, CWorldScene::frustumFreeList);
UINT                  CWorldScene::nPrimsRendered;
UINT                  CWorldScene::nChunksRendered;
UINT                  CWorldScene::nDoodadsRendered;
UINT                  CWorldScene::nObjectsRendered;
char                  CWorldScene::currentChunkName[64];

void CSortTable::Initialize() {
}

void CSortTable::Destroy() {
}

void CSortTable::Clear() {
  for (UINT index = 0; index < sizeof(table) / sizeof(table[0]); ++index) {
    CSortEntry *entry = &table[index];

    SAFEITERATELIST(CMapChunk, entry->chunkList, chunk) {
      chunk->sceneLink.Unlink();
    }

    for (UINT type = 0; type < LQ_LAST; ++type) {
      SAFEITERATELIST(CChunkLiquid, entry->liquidList[type], liquid) {
        liquid->sceneLink.Unlink();
      }
    }

    SAFEITERATELIST(CMapDoodadDef, entry->doodadDefList, doodadDef) {
      doodadDef->sceneLink.Unlink();
    }

    SAFEITERATELIST(CMapObjDef, entry->mapObjDefList, mapObjDef) {
      mapObjDef->sceneLink.Unlink();
    }

    SAFEITERATELIST(CMapEntity, entry->entityList, entity) {
      entity->flagVisible = 0;
      entity->sceneLink.Unlink();
      nonVisEntityList.LinkNode(entity, LIST_TAIL, 0);
    }
  }
}

void CWorldScene::Initialize() {
  camTargEntity = 0;
  camLiquid = 15;

  vpPlanes[0].n = NTempest::C3Vector(1.0f, 0.0f, 1.0f);
  vpPlanes[0].d = 0.0f;
  vpPlanes[1].n = NTempest::C3Vector(-1.0f, 0.0f, 1.0f);
  vpPlanes[1].d = 0.0f;
  vpPlanes[2].n = NTempest::C3Vector(0.0f, 1.0f, 1.0f);
  vpPlanes[2].d = 0.0f;
  vpPlanes[3].n = NTempest::C3Vector(0.0f, -1.0f, 1.0f);
  vpPlanes[3].d = 0.0f;

  vpPlanes[0].n.Normalize();
  vpPlanes[1].n.Normalize();
  vpPlanes[2].n.Normalize();
  vpPlanes[3].n.Normalize();
}

void CWorldScene::Destroy() {
  frustumFreeList.Clear();
}

CWFrustum *CWorldScene::AllocFrustum() {
  CWFrustum *frustum = frustumFreeList.Head();
  if (!frustum) {
    frustum = frustumFreeList.NewNode(LIST_TAIL, 0, 0);
    FATALASSERT(frustum);
  }

  frustum->sceneLink.Unlink();
  return frustum;
}

void CWorldScene::FreeFrustum(CWFrustum *frustum) {
  FATALASSERT(frustum);
  frustum->sceneLink.Unlink();
  frustumFreeList.LinkNode(frustum, LIST_TAIL, 0);
}

void CWorldScene::PrepareRenderLiquid() {
  NTempest::C3Vector camQueryPos = camPos;
  for (UINT i = 0; i <= 3; ++i) {
    camQueryPos.z = min(camQueryPos.z, camFrustumCorners[i].z);
  }

  NTempest::C3Vector lqDir(0.0f, 0.0f, 0.0f);
  float              lqSurface;
  UINT               newLiquid = 15;
  if (camMapObjDef && camMapObj && camMapObjGroup) {
    camMapObjGroup->QueryLiquidStatus(camQueryPos * camMapObjDef->invMat, newLiquid, lqSurface, lqDir);
  } else {
    CWorld::QueryLiquidStatus(camQueryPos, newLiquid, lqSurface, lqDir);
  }

  int forceFullUpdate = 0;
  if (newLiquid == 15) {
    if (camLiquid != newLiquid) {
      forceFullUpdate = 1;
    }
  } else {
    switch (newLiquid & 3) {
      case 0:
      case 1:
        if (newLiquid != camLiquid && (CWorld::enables & CWorld::Enable_Particulates)) {
          CWorld::particulate->InitParticles(newLiquid);
          CWorld::particulate->SetScale(1.0f / 36.0f);
          CWorld::particulate->Show(1);
        }
        break;
      case 2:
        if (newLiquid != camLiquid && (CWorld::enables & CWorld::Enable_Particulates)) {
          CWorld::particulate->InitParticles(newLiquid);
          CWorld::particulate->SetScale(1.0f / 9.0f);
          CWorld::particulate->Show(1);
        }
        break;
      case 3:
        CWorld::particulate->Show(0);
        break;
    }
  }

  camLiquid = newLiquid;

  if (forceFullUpdate) {
    DayNightForceFullUpdate();
  }

  CMap::riverDiffTexUpdated = false;
  CMap::oceanDiffTexUpdated = false;
}

void CWorldScene::PrepareRender(const NTempest::C3Vector &position, const NTempest::C3Vector &target) {
  camPos = position;
  camTarg = target;
  camVec = target - position;
  camVec.Normalize();

  NTempest::C3Vector camPlaneVectXY = camVec;
  camPlaneVectXY.z = 0.0f;
  camPlaneXY.Set(camPlaneVectXY, position);

  GxXformView(mv);
  GxXformProjection(mp);
  GxXformViewport(vpMinPos.x, vpMaxPos.x, vpMinPos.y, vpMaxPos.y, vpMinPos.z, vpMaxPos.z);
  mv.Translate(-camPos);
  mvp = mv * mp;
  mvpCol3 = NTempest::C4Vector(mvp.a3, mvp.b3, mvp.c3, mvp.d3);
  GxuXformCalcFrustumCorners(mv, mp, camFrustumCorners);
  camFrustumBounds = NTempest::CAaBox::Bounding(camFrustumCorners, 8);
  frustumIndex = 0;
  PrepareRenderLiquid();
}

void CWorldScene::Update() {
}

void CWorldScene::Render() {
  GxRsPush();

  NTempest::C3Vector saveMin;
  NTempest::C3Vector saveMax;
  GxXformViewport(saveMin.x, saveMax.x, saveMin.y, saveMax.y, saveMin.z, saveMax.z);

  NTempest::C3Vector max = saveMax;
  max.z = saveMax.z - (saveMax.z - saveMin.z) * 0.05f;
  GxXformSetViewport(saveMin.x, max.x, saveMin.y, max.y, saveMin.z, max.z);

  const CGxCaps &caps = GxCaps();

  for (UINT i = 0; i < caps.m_numTmus; ++i) {
    GxRsSet((EGxRenderState)(GxRs_TexLodBias0 + i), -CWorld::texLodBias);
  }

  GxXformPush(GxXform_World);
  nPrimsRendered = 0;
  nChunksRendered = 0;
  nDoodadsRendered = 0;
  nObjectsRendered = 0;

  DayNightRenderSky();
  FrustumSet(camFrustumCorners);
  LocateViewer3();

  if (viewerMapObjDef) {
    ClipBufferClear();
    CullMapObjDef(viewerMapObjDef, viewerMapObjGroups);

    static TSCArray<NTempest::CRect, 16> s_extViewList;
    s_extViewList = CMapObj::extViewList;
    for (UINT i = 0; i < s_extViewList.Count(); ++i) {
      ClipBufferClear();
      CullSortTable(s_extViewList[i]);
    }
  } else {
    ClipBufferClear();
    CullSortTable(NTempest::CRect(0.0f, 0.0f, 1.0f, 1.0f));
  }

  sortTable.Clear();
  RenderHorizon();
  RenderChunks();
  RenderMapObjDefGroups();
  RenderOcean();
  RenderWater();
  RenderMagma();

  GxXformPop(GxXform_World);
  GxRsPop();
  RenderDoodads();
  RenderObjects();
  CMap::TestQueryRender();
  CMap::testQueryVerts.SetCount(0);
  CMap::testQueryIndices.SetCount(0);
}

void CWorldScene::RenderAlpha() {
  GxRsPush();
  GxXformPush(GxXform_World);
  GxRsSet(GxRs_TexLodBias0, 0.5f);

  NTempest::C44Matrix cMat;
  SAFEITERATELIST(CMapChunk, sortTable.visChunkList, chunk) {
    if (chunk->detailDoodadInst) {
      cMat.Identity();
      cMat.Translate(chunk->corner - camPos);
      GxXformSet(GxXform_World, cMat);
      CMap::SelectLight(chunk);
      chunk->detailDoodadInst->RenderAlpha();
    }
    chunk->sceneLink.Unlink();
  }

  GxRsSet(GxRs_TexLodBias0, -(CWorld::texLodBias - 0.5f));
  GxXformPop(GxXform_World);
  GxRsPop();
}

void CWorldScene::AddDoodadDef(CMapDoodadDef *doodadDef) {
  FATALASSERT(doodadDef);
  NTempest::CAaSphere bounds;
  doodadDef->GetBounds(bounds);
  doodadDef->camDist = camPlaneXY.DistSigned(bounds.c) - bounds.r;
  int sortIndex = Fast_ftol(doodadDef->camDist * CBDIVISOR);
  if (sortIndex < 0) {
    sortIndex = 0;
  } else if (sortIndex >= 26) {
    SysMsgPrintf(SYSMSG_WARNING, 2, "DOODADDEFTOOFARTOSORT");
    return;
  }
  sortTable.table[sortIndex].doodadDefList.LinkNode(doodadDef, LIST_TAIL, 0);
}

void CWorldScene::AddMapObjDef(CMapObjDef *mapObjDef) {
  FATALASSERT(mapObjDef);
  mapObjDef->camDist = camPlaneXY.DistSigned(mapObjDef->aaSphere.c) - mapObjDef->aaSphere.r;
  int sortIndex = Fast_ftol(mapObjDef->camDist * CBDIVISOR);
  if (sortIndex < 0) {
    sortIndex = 0;
  } else if (sortIndex >= 26) {
    SysMsgPrintf(SYSMSG_WARNING, 2, "MAPOBJDEFTOOFARTOSORT");
    return;
  }
  sortTable.table[sortIndex].mapObjDefList.LinkNode(mapObjDef, LIST_TAIL, 0);
}

void CWorldScene::AddMapChunk(CMapChunk *chunk, float sortDist) {
  FATALASSERT(chunk);

  int sortIndex = Fast_ftol(CBDIVISOR * sortDist);
  if (sortIndex < 0) {
    sortIndex = 0;
  } else if (sortIndex >= 26) {
    SysMsgPrintf(SYSMSG_WARNING, 2, "CHUNKDISTTOOFARTOSORT");
    return;
  }

  CSortEntry *entry = &sortTable.table[sortIndex];
  entry->chunkList.LinkNode(chunk, LIST_TAIL, 0);
}

void CWorldScene::AddChunkLiquid(CChunkLiquid *liquid, UINT type) {
  FATALASSERT(liquid);
  FATALASSERT(type < LQ_LAST);
  int sortIndex = Fast_ftol(CBDIVISOR * liquid->chunk->camDist);
  if (sortIndex < 0) {
    sortIndex = 0;
  } else if (sortIndex >= 26) {
    SysMsgPrintf(SYSMSG_WARNING, 2, "CHUNKDISTTOOFARTOSORT");
    return;
  }
  CSortEntry *entry = &sortTable.table[sortIndex];
  entry->liquidList[type].LinkNode(liquid, LIST_TAIL, 0);
}

void CWorldScene::AddMapEntity(CMapEntity *entity) {
  FATALASSERT(entity);
  entity->camDist = camPlaneXY.DistSigned(entity->aaSphere.c) - entity->aaSphere.r;
  int sortIndex = Fast_ftol(entity->camDist * CBDIVISOR);
  if (sortIndex < 0) {
    sortIndex = 0;
  } else if (sortIndex >= 26) {
    SysMsgPrintf(SYSMSG_WARNING, 2, "ENTITYDISTTOOFARTOSORT");
    return;
  }
  sortTable.table[sortIndex].entityList.LinkNode(entity, LIST_HEAD, 0);
}

void CWorldScene::ClipBufferUpdate(const NTempest::C3Vector *vertices, const int *indicies, const int nVertices, const NTempest::C3Vector &corner) {
  FATALASSERT(vertices);
  FATALASSERT(indicies);

  if (!(CWorld::enables & CWorld::Enable_Culling)) {
    return;
  }

  int                 i;
  NTempest::C4Vector *v = clipVertexBuffer;
  for (i = 0; i < nVertices; ++i, ++v, ++indicies) {
    *v = corner + vertices[*indicies];
    *v = *v * mvp;
    float ooW = 1.0f / v->w;
    v->x *= ooW;
    v->y *= ooW;
    v->x += 1.0f;
  }

  for (i = 0; i < nVertices - 1; ++i) {
    if (clipVertexBuffer[i].w < 2.7777777f || clipVertexBuffer[i + 1].w < 2.7777777f) {
      continue;
    }

    float cullValue = clipVertexBuffer[i].y;
    if (clipVertexBuffer[i + 1].y < cullValue) {
      cullValue = clipVertexBuffer[i + 1].y;
    }

    int x0 = Fast_ftol(CBSCALE * clipVertexBuffer[i].x);
    int x1 = Fast_ftol(CBSCALE * clipVertexBuffer[i + 1].x);
    if (x1 < x0) {
      int x = x1;
      x1 = x0;
      x0 = x;
    }
    if (x0 < 0) {
      x0 = 0;
    }
    if (x1 >= 128) {
      x1 = 127;
    }
    for (int x = x0; x <= x1; ++x) {
      if (clipBuffer[x] < cullValue) {
        clipBuffer[x] = cullValue;
      }
    }
  }
}

void CWorldScene::ClipPortal(NTempest::C4Vector *inList, UINT &inCount) {
  static NTempest::C4Vector tv[16];
  static float              dist[16];
  static UINT               side[16];

  NTempest::C4Vector *v[2] = {inList, tv};
  UINT                c[2] = {inCount, 0};

  for (UINT p = 0; p < 4; ++p) {
    NTempest::C4Plane plane = vpPlanes[p];
    UINT              idx = p & 1;
    UINT              idx1 = (p - 1) & 1;

    NTempest::C4Vector *v0 = v[idx];
    NTempest::C4Vector *v1 = v[idx1];
    UINT                i;
    for (i = 0; i < c[idx]; ++i) {
      dist[i] = plane.n.x * v0[i].x + plane.n.y * v0[i].y + plane.n.z * v0[i].w;
      if (dist[i] > 0.019444443f) {
        side[i] = 1;
      } else if (dist[i] < -0.019444443f) {
        side[i] = 2;
      } else {
        side[i] = 0;
      }
    }

    dist[i] = dist[0];
    side[i] = side[0];
    UINT cnt = 0;
    for (i = 0; i < c[idx]; ++i) {
      if (side[i] == 0) {
        v1[cnt] = v0[i];
        ++cnt;
        continue;
      }

      if (side[i] == 1) {
        v1[cnt] = v0[i];
        ++cnt;
      }

      if (side[i + 1] != 0 && side[i + 1] != side[i]) {
        UINT  j = (i + 1) % c[idx];
        float t = dist[i] / (dist[i] - dist[j]);
        v1[cnt].x = (v0[j].x - v0[i].x) * t + v0[i].x;
        v1[cnt].y = (v0[j].y - v0[i].y) * t + v0[i].y;
        v1[cnt].w = (v0[j].w - v0[i].w) * t + v0[i].w;
        ++cnt;
      }
    }

    if (!cnt) {
      inCount = 0;
      return;
    }

    inCount = c[idx1] = cnt;
  }
}

void CWorldScene::CalcFrustumCorners(NTempest::C3Vector corners[]) {
  NTempest::C44Matrix lMv;
  NTempest::C44Matrix lMp;
  GxXformView(lMv);
  GxXformProjection(lMp);

  lMv.Translate(-camPos);
  GxuXformCalcFrustumCorners(lMv, lMp, corners);
}

void CWorldScene::LocateViewer() {
  viewerMapObjDef = 0;
  viewerMapObjGroups.SetCount(0);
  currentChunkName[0] = 0;

  SAFEITERATELIST(CMapObjDef, sortTable.table[0].mapObjDefList, mapObjDef) {
    CMapObj *mapObj = mapObjDef->mapObj;
    FATALASSERT(mapObj);
    mapObj->LocateViewer(mapObjDef->invMat, viewerMapObjGroups);
    if (viewerMapObjGroups.Count()) {
      viewerMapObjDef = mapObjDef;
      mapObjDef->sceneLink.Unlink();
      return;
    }
  }
}

void CWorldScene::AddViewerGroup2(UINT groupNum) {
  for (UINT i = 0; i < viewerMapObjGroups.Count(); ++i) {
    if (viewerMapObjGroups[i] == groupNum) {
      return;
    }
  }

  viewerMapObjGroups.Add(&groupNum);
}

void CWorldScene::LocateViewer3() {
  viewerMapObjDef = 0;
  viewerMapObjGroups.SetCount(0);
  currentChunkName[0] = 0;
  camMapObj = 0;
  camMapObjGroup = 0;
  camMapObjDef = 0;

  if (!(CWorld::enables & CWorld::Enable_MapObjs)) {
    return;
  }

  NTempest::C3Vector lCen = camPos;
  NTempest::C3Vector lEnd = camPos;
  lEnd.z -= 1760.0f;

  CMapChunk  *chunk;
  float       chunkT = 1.0f;
  float       mapObjT = 1.0f;
  CMapObjDef *mapObjDef = 0;
  UINT        mapObjDefGroupIDs[2];
  bool        hitChunk = CMap::VectorIntersectTerrain(&lCen, &lEnd, &chunkT, 0, &chunk);
  bool        hitMapObj = CMap::LocateViewerMapObjs(lCen, lEnd, mapObjT, mapObjDef, mapObjDefGroupIDs);

  if ((!hitChunk || (hitMapObj && !(chunkT < mapObjT))) && hitMapObj) {
    camMapObjDef = mapObjDef;
    camMapObj = mapObjDef->mapObj;
    FATALASSERT(camMapObj);

    camMapObjGroup = camMapObj->GetGroup(mapObjDefGroupIDs[0], 0);
    FATALASSERT(camMapObjGroup);

    SStrCopy(currentChunkName, camMapObj->GetGroupName(mapObjDefGroupIDs[0]), sizeof(currentChunkName));

    SAFEITERATELIST(CMapObjDef, sortTable.table[0].mapObjDefList, mapObjDefNode) {
      if (mapObjDefNode == mapObjDef) {
        viewerMapObjDef = mapObjDefNode;
        mapObjDefNode->sceneLink.Unlink();
      }
    }

    FATALASSERT(viewerMapObjDef);

    AddViewerGroup2(mapObjDefGroupIDs[0]);
    if (mapObjDefGroupIDs[1] != 0xFFFF) {
      AddViewerGroup2(mapObjDefGroupIDs[1]);
    }
  }
}

void CWorldScene::LocateViewer2() {
}

void CWorldScene::FrustumPush() {
  FATALASSERT(frustumIndex < 15);
  ++frustumIndex;
  frustumStack[frustumIndex] = frustumStack[frustumIndex - 1];
}

void CWorldScene::FrustumSet(const NTempest::CRect &sRect) {
  enum {
    FRUST_BL = 0,
    FRUST_TL = 1,
    FRUST_TR = 2,
    FRUST_BR = 3
  };

  NTempest::C3Vector newCorners[8];
  for (UINT i = 0; i < 8; i += 4) {
    NTempest::C3Vector td = camFrustumCorners[i + FRUST_TR] - camFrustumCorners[i + FRUST_TL];
    NTempest::C3Vector tl = camFrustumCorners[i + FRUST_TL] + td * sRect.l;
    NTempest::C3Vector tr = camFrustumCorners[i + FRUST_TL] + td * sRect.r;

    NTempest::C3Vector bd = camFrustumCorners[i + FRUST_BR] - camFrustumCorners[i + FRUST_BL];
    NTempest::C3Vector bl = camFrustumCorners[i + FRUST_BL] + bd * sRect.l;
    NTempest::C3Vector br = camFrustumCorners[i + FRUST_BL] + bd * sRect.r;

    NTempest::C3Vector ld = tl - bl;
    newCorners[i + FRUST_BL] = bl + ld * sRect.t;
    newCorners[i + FRUST_TL] = bl + ld * sRect.b;

    NTempest::C3Vector rd = tr - br;
    newCorners[i + FRUST_BR] = br + rd * sRect.t;
    newCorners[i + FRUST_TR] = br + rd * sRect.b;
  }

  frustumStack[frustumIndex].CalcPlanesFromCorners(newCorners);
}

void CWorldScene::FrustumSet(const NTempest::C3Vector corners[]) {
  frustumStack[frustumIndex].CalcPlanesFromCorners(corners);
}

void CWorldScene::FrustumSet(const NTempest::C3Vector corners[], const NTempest::CRect &sRect) {
  enum {
    FRUST_BL = 0,
    FRUST_TL = 1,
    FRUST_TR = 2,
    FRUST_BR = 3
  };

  NTempest::C3Vector newCorners[8];

  for (UINT i = 0; i < 8; i += 4) {
    NTempest::C3Vector td = corners[i + FRUST_TR] - corners[i + FRUST_TL];
    NTempest::C3Vector tl = corners[i + FRUST_TL] + td * sRect.l;
    NTempest::C3Vector tr = corners[i + FRUST_TL] + td * sRect.r;
    NTempest::C3Vector bd = corners[i + FRUST_BR] - corners[i + FRUST_BL];
    NTempest::C3Vector bl = corners[i + FRUST_BL] + bd * sRect.l;
    NTempest::C3Vector br = corners[i + FRUST_BL] + bd * sRect.r;
    NTempest::C3Vector ld = tl - bl;
    newCorners[i + FRUST_BL] = bl + ld * sRect.t;
    newCorners[i + FRUST_TL] = bl + ld * sRect.b;
    NTempest::C3Vector rd = tr - br;
    newCorners[i + FRUST_BR] = br + rd * sRect.t;
    newCorners[i + FRUST_TR] = br + rd * sRect.b;
  }

  NTempest::C3Vector a = newCorners[FRUST_TR];
  NTempest::C3Vector b = newCorners[FRUST_BL];
  NTempest::C3Vector n = NTempest::C3Vector::Cross(b - a, newCorners[FRUST_TL] - newCorners[FRUST_TR]);
  if (n == NTempest::C3Vector(0.0f, 0.0f, 0.0f)) {
    return;
  }

  frustumStack[frustumIndex].CalcPlanesFromCorners(newCorners);
}

void CWorldScene::FrustumSet(const CWFrustum &frustum) {
  frustumStack[frustumIndex] = frustum;
}

CWFrustum &CWorldScene::FrustumGet() {
  return frustumStack[frustumIndex];
}

void CWorldScene::FrustumXform(const NTempest::C44Matrix &mat) {
  frustumStack[frustumIndex].Transform(mat);
}

BOOL CWorldScene::FrustumCull(const NTempest::C3Vector &center, float radius) {
  return frustumStack[frustumIndex].Cull(NTempest::CAaSphere(center, radius)) == WorldCull_outside;
}

BOOL CWorldScene::FrustumCull(const NTempest::CAaBox &aaBox) {
  return frustumStack[frustumIndex].Cull(aaBox) == WorldCull_outside;
}

BOOL CWorldScene::FrustumCull(const NTempest::CAaBox &aaBox, NTempest::C33Matrix &basis, NTempest::C3Vector &pos) {
  return frustumStack[frustumIndex].Cull(aaBox, basis, pos) == WorldCull_outside;
}

void CWorldScene::FrustumPop() {
  FATALASSERT(frustumIndex > 0);
  --frustumIndex;
}

void CWorldScene::CullSortTable(const NTempest::CRect &sRect) {
  FrustumPush();
  FrustumSet(sRect);
  for (UINT index = 0; index < 26; ++index) {
    CSortEntry *sortEntry = &sortTable.table[index];
    CullEntitys(sortEntry);
    CullDoodads(sortEntry);
    CullMapObjDefs(sortEntry, sRect);
    CullChunkLiquid(sortEntry, 1);
    CullChunkLiquid(sortEntry, 0);
    CullChunkLiquid(sortEntry, 2);
    CullChunks(sortEntry);
  }
  FrustumPop();
  CullHorizon(sRect);
}

void CWorldScene::CullHorizon(const NTempest::CRect &sRect) {
  NTempest::CiRect areaRect = CWorld::areaRect;
  areaRect.miny -= 3;
  areaRect.minx -= 3;
  areaRect.maxy += 3;
  areaRect.maxx += 3;

  if (areaRect.miny < 0) {
    areaRect.miny = 0;
  }
  if (areaRect.minx < 0) {
    areaRect.minx = 0;
  }
  if (areaRect.maxx >= 64) {
    areaRect.maxx = 63;
  }
  if (areaRect.maxy >= 64) {
    areaRect.maxy = 63;
  }

  CGCamera *camera = CGWorldFrame::GetActiveCamera();
  float     farZ = camera->FarZ();
  float     fov = camera->FOV();
  float     aspect = camera->Aspect();
  float     nearZ = farZ - 33.0f;
  farZ += 2000.0f;

  NTempest::C44Matrix projMat;
  GxuXformCreateProjection(fov, aspect, nearZ, farZ, projMat);

  NTempest::C44Matrix viewMat;
  GxXformView(viewMat);
  viewMat.Translate(-camPos);

  NTempest::C3Vector corners[8];
  GxuXformCalcFrustumCorners(viewMat, projMat, corners);

  FrustumPush();
  FrustumSet(corners, sRect);
  for (long areaY = areaRect.miny; areaY <= areaRect.maxy; ++areaY) {
    for (long areaX = areaRect.minx; areaX <= areaRect.maxx; ++areaX) {
      CMapAreaLow *areaLow = CMap::areaLowTable[areaY * 64 + areaX];
      if (areaLow) {
        if (!areaLow->sceneLink.IsLinked()) {
          if (!FrustumCull(areaLow->aaBox)) {
            if (!ClipBufferCull(areaLow->aaBox, 8)) {
              sortTable.visAreaLowList.LinkNode(areaLow, LIST_TAIL, 0);
            }
          }
        }
      }
    }
  }
  FrustumPop();
}

void CWorldScene::CullEntitys(CSortEntry *sortEntry) {
  FATALASSERT(sortEntry);
  SAFEITERATELIST(CMapEntity, sortEntry->entityList, entity) {
    if (entity->camDist > CWorld::unitDrawDist) {
      entity->flagVisible = 0;
      entity->sceneLink.Unlink();
      sortTable.nonVisEntityList.LinkNode(entity, LIST_TAIL, 0);
    } else if (!FrustumCull(entity->aaSphere.c, entity->aaSphere.r) && !ClipBufferCull(entity->aaSphere.c, entity->aaSphere.r, 0)) {
      entity->flagVisible = 1;
      entity->sceneLink.Unlink();
      sortTable.visEntityList.LinkNode(entity, LIST_TAIL, 0);
    }
  }
}

void CWorldScene::CullDoodads(CSortEntry *sortEntry) {
  FATALASSERT(sortEntry);
  SAFEITERATELIST(CMapDoodadDef, sortEntry->doodadDefList, doodadDef) {
    if ((doodadDef->model || doodadDef->RenderCB) && !(doodadDef->flags & CMapBaseObj::Flag_LoadFailed)) {
      NTempest::CAaSphere doodadSphere;
      doodadDef->GetBounds(doodadSphere);
      if (!FrustumCull(doodadSphere.c, doodadSphere.r) && !ClipBufferCull(doodadSphere.c, doodadSphere.r, 5)) {
        doodadDef->sceneLink.Unlink();
        sortTable.visDoodadList.LinkNode(doodadDef, LIST_TAIL, 0);
        ++nDoodadsRendered;
      } else {
        if (doodadDef->rCount != CWorld::frameCnt && doodadDef->flagAlwaysAnimate && doodadDef->model) {
          ModelAdvanceTime(doodadDef->model);
        }
        doodadDef->rCount = CWorld::frameCnt;
      }
    }
  }
}

void CWorldScene::CullDoodads(LISTEX(CMapBaseObjLink, refLink) & doodadDefLinkList) {
  ITERATELIST(CMapBaseObjLink, doodadDefLinkList, link) {
    CMapDoodadDef *doodadDef = (CMapDoodadDef *)link->owner;
    FATALASSERT(doodadDef);
    if ((doodadDef->model || doodadDef->RenderCB) && !(doodadDef->flags & CMapBaseObj::Flag_LoadFailed) && !doodadDef->sceneLink.IsLinked()) {
      NTempest::CAaSphere doodadSphere;
      doodadDef->GetBounds(doodadSphere);
      doodadDef->camDist = camPlaneXY.DistSigned(doodadSphere.c) - doodadSphere.r;
      if (!FrustumCull(doodadSphere.c, doodadSphere.r) && !ClipBufferCull(doodadSphere.c, doodadSphere.r, 5)) {
        sortTable.visDoodadList.LinkNode(doodadDef, LIST_TAIL, 0);
        ++nDoodadsRendered;
      } else {
        if (doodadDef->rCount != CWorld::frameCnt && doodadDef->flagAlwaysAnimate && doodadDef->model) {
          ModelAdvanceTime(doodadDef->model);
        }
        doodadDef->rCount = CWorld::frameCnt;
      }
    }
  }
}

void CWorldScene::CullChunkLiquid(CSortEntry *sortEntry, UINT type) {
  FATALASSERT(sortEntry);
  SAFEITERATELIST(CChunkLiquid, sortEntry->liquidList[type], liquid) {
    NTempest::CAaBox aaBox;
    liquid->GetAaBox(aaBox);
    if (!FrustumCull(aaBox) && !ClipBufferCull(aaBox, 0)) {
      liquid->sceneLink.Unlink();
      sortTable.visLiquidList[type].LinkNode(liquid, LIST_TAIL, 0);
    }
  }
}

void CWorldScene::CullChunks(CSortEntry *sortEntry) {
  FATALASSERT(sortEntry);
  float maxClipBufferUpdateDist = CWorld::farClip - 33.333332f;
  {
    SAFEITERATELIST(CMapChunk, sortEntry->chunkList, chunk) {
      if (chunk->holes != 0xFFFF && !FrustumCull(chunk->aaBox) && !ClipBufferCull(chunk->aaBox, 0)) {
        ++nChunksRendered;
        chunk->sceneLink.Unlink();
        if ((CWorld::enables & CWorld::Enable_Culling) && !chunk->holes) {
          sortTable.updateChunkList.LinkNode(chunk, LIST_TAIL, 0);
        } else {
          sortTable.visChunkList.LinkNode(chunk, LIST_TAIL, 0);
        }
      }
    }
  }

  if (CWorld::enables & CWorld::Enable_Culling) {
    SAFEITERATELIST(CMapChunk, sortTable.updateChunkList, chunk) {
      if (chunk->camDist < maxClipBufferUpdateDist) {
        chunk->UpdateClipBuffer();
      }
      chunk->sceneLink.Unlink();
      sortTable.visChunkList.LinkNode(chunk, LIST_TAIL, 0);
    }
  }
}

void CWorldScene::RenderObjects() {
  SAFEITERATELIST(CMapEntity, sortTable.visEntityList, entity) {
    entity->sceneLink.Unlink();

    NTempest::C44Matrix basis((NTempest::C33Matrix)entity->rot);
    basis.Scale(entity->scale);
    *basis.Row3AsVec3() = entity->pos - camPos;

    if (!entity->flagHidden && entity->flagCastShadow) {
      ShadowRender(entity->model, basis, 0);
    }

    if (CMap::entityHandler) {
      CMap::entityHandler(CMap::entityHandlerParam, 1, entity->param64, entity->param32);
    }
    ++nObjectsRendered;
  }

  {
    SAFEITERATELIST(CMapEntity, sortTable.nonVisEntityList, entity) {
      entity->sceneLink.Unlink();
      if (CMap::entityHandler) {
        CMap::entityHandler(CMap::entityHandlerParam, 2, entity->param64, entity->param32);
      }
      ++nObjectsRendered;
    }
  }
}

void CWorldScene::RenderDoodads() {
  if (!(CWorld::enables & (CWorld::Enable_Doodads | CWorld::Enable_Collision | CWorld::Enable_AABoxes))) {
    return;
  }

  SAFEITERATELIST(CMapDoodadDef, sortTable.visDoodadList, doodadDef) {
    doodadDef->sceneLink.Unlink();

    if (doodadDef->model) {
      ModelShowCollision(doodadDef->model, CWorld::enables & CWorld::Enable_Collision);
      ModelShowCollisionAaBox(doodadDef->model, CWorld::enables & CWorld::Enable_AABoxes);
      ModelShowModel(doodadDef->model, CWorld::enables & CWorld::Enable_Doodads);

      if (ModelAdvanceTime(doodadDef->model)) {
        NTempest::C44Matrix transform = doodadDef->mat;
        *transform.Row3AsVec3() -= camPos;
        ModelAnimate(doodadDef->model, transform, doodadDef->scale, camPos, camTarg - camPos);

        *transform.Row3AsVec3() += camPos;
        ModelProcessEvents(doodadDef->model, transform);

        if (doodadDef->camDist > CWorld::farFog) {
          ModelAddToScene(doodadDef->model, 7);
        } else {
          ModelAddToScene(doodadDef->model, 0);
        }
      }
    }

    if (doodadDef->RenderCB) {
      doodadDef->RenderCB(doodadDef->renderCBParam, doodadDef->mat);
    }
  }
}

void CWorldScene::RenderHorizon() {
  CGCamera *camera = CGWorldFrame::GetActiveCamera();
  float     farZ = camera->FarZ();
  float     fov = camera->FOV();
  float     aspect = camera->Aspect();

  NTempest::C44Matrix saveProjMat;
  GxXformProjection(saveProjMat);

  NTempest::C44Matrix projMat;
  GxuXformCreateProjection(fov, aspect, farZ - 33.0f, farZ + 2000.0f, projMat);
  GxXformSetProjection(projMat);

  NTempest::C3Vector saveMin;
  NTempest::C3Vector saveMax;
  GxXformViewport(saveMin.x, saveMax.x, saveMin.y, saveMax.y, saveMin.z, saveMax.z);
  GxXformSetViewport(saveMin.x, saveMax.x, saveMin.y, saveMax.y, saveMax.z, 1.0f);

  NTempest::C44Matrix cMat;
  cMat.Translate(-camPos);
  GxXformPush(GxXform_World, cMat);

  SAFEITERATELIST(CMapAreaLow, sortTable.visAreaLowList, areaLow) {
    areaLow->sceneLink.Unlink();

    if (CWorld::enables & CWorld::Enable_LowDetail) {
      CMap::RenderAreaLow(areaLow);
    }
  }

  GxXformPop(GxXform_World);
  GxXformSetViewport(saveMin.x, saveMax.x, saveMin.y, saveMax.y, saveMin.z, saveMax.z);
  GxXformSetProjection(saveProjMat);
}

void CWorldScene::RenderChunks() {
  NTempest::C44Matrix cMat;

  SAFEITERATELIST(CMapChunk, sortTable.visChunkList, chunk) {
    if (CWorld::enables & CWorld::Enable_Chunks) {
      cMat.Identity();
      cMat.Translate(chunk->corner - camPos);
      GxXformSet(GxXform_World, cMat);
      CMap::SelectLight(chunk);
      chunk->Render();
    }

    if ((CWorld::enables & CWorld::Enable_DetailDoodads) && chunk->camDist < CWorld::detailDoodadDist) {
      if (!chunk->detailDoodadInst) {
        chunk->CreateDetailDoodads();
      }
    } else {
      chunk->sceneLink.Unlink();
    }
  }
}

void CWorldScene::RenderMapObjDefGroups() {
  FrustumPush();

  NTempest::C44Matrix gxWm;
  NTempest::C44Matrix mapObjM;
  SAFEITERATELIST(CMapObjDefGroup, sortTable.visMapObjDefGroupList, mapObjDefGroup) {
    mapObjDefGroup->sceneLink.Unlink();

    if (CWorld::enables & CWorld::Enable_MapObjs) {
      CMapBaseObjLink *parentLink = mapObjDefGroup->parentLinkList.Head();
      FATALASSERT(parentLink);
      CMapObjDef *mapObjDef = (CMapObjDef *)parentLink->ref;
      FATALASSERT(mapObjDef);

      gxWm.Identity();
      gxWm.Translate(-camPos);
      mapObjM = mapObjDef->mat * gxWm;
      GxXformSet(GxXform_World, mapObjM);

      mapObjDefGroup->SelectLights();
      CMapObj *mapObj = mapObjDef->mapObj;
      FATALASSERT(mapObj);
      mapObj->RenderGroup(mapObjDefGroup->groupNum, mapObjDefGroup->rDrawSharedLiquidToggle, mapObjDef->invMat, mapObjDefGroup->frustumList);
    }

    SAFEITERATELIST(CWFrustum, mapObjDefGroup->frustumList, frustum) {
      frustum->sceneLink.Unlink();
      FreeFrustum(frustum);
    }
  }

  FrustumPop();
}

void CWorldScene::RenderOcean() {
  if (!sortTable.visLiquidList[1].Head()) {
    return;
  }

  GxRsPush();
  GxRsSet(GxRs_Culling, 0);
  GxRsSet(GxRs_Texture0, CMap::oceanDiffTexid);
  GxRsSet(GxRs_TexGen1, GxTexGen_World);
  GxRsSet(GxRs_TextureShader1, GxTS_Affine);
  GxRsSet(GxRs_Blend, GxBlend_Opaque);
  GxXformPush(GxXform_Tex1);

  GxXformScale(GxXform_Tex1, NTempest::C3Vector(0.11f, 0.11f, 0.11f));
  GxXformTranslate(GxXform_Tex1, camPos);

  if (CMap::EnableSpecularWater()) {
    GxRsSet(GxRs_PixelShader, CMap::psOcean0);
    GxRsSet(GxRs_MatSpecular, NTempest::CImVector(-1));
    GxRsSet(GxRs_MatSpecularExp, CMap::WATER_SPEC_EXP);
  } else {
    GxRsSet(GxRs_TexBlend1, GxTexBlend_Add);
  }

  SAFEITERATELIST(CChunkLiquid, sortTable.visLiquidList[1], liquid) {
    liquid->sceneLink.Unlink();

    if (CWorld::enables & CWorld::Enable_Water) {
      GxXformIdentity(GxXform_World);
      GxXformTranslate(GxXform_World, liquid->chunk->corner - camPos);
      CMap::SelectLight(liquid->chunk);
      liquid->Render(1);
    }
  }

  GxXformPop(GxXform_Tex1);
  GxRsPop();
}

void CWorldScene::RenderWater() {
  if (!sortTable.visLiquidList[0].Head()) {
    return;
  }

  GxRsPush();
  GxRsSet(GxRs_Culling, 0);
  GxRsSet(GxRs_Texture0, CMap::riverDiffTexid);
  GxRsSet(GxRs_TexGen1, GxTexGen_World);
  GxRsSet(GxRs_TextureShader1, GxTS_Affine);
  GxRsSet(GxRs_Blend, GxBlend_Opaque);
  GxXformPush(GxXform_Tex1);

  GxXformScale(GxXform_Tex1, NTempest::C3Vector(0.14f, 0.14f, 0.14f));
  GxXformTranslate(GxXform_Tex1, camPos);

  if (CMap::EnableSpecularWater()) {
    GxRsSet(GxRs_PixelShader, CMap::psOcean0);
    GxRsSet(GxRs_MatSpecular, NTempest::CImVector(-1));
    GxRsSet(GxRs_MatSpecularExp, CMap::WATER_SPEC_EXP);
  } else {
    GxRsSet(GxRs_TexBlend1, GxTexBlend_Add);
  }

  SAFEITERATELIST(CChunkLiquid, sortTable.visLiquidList[0], liquid) {
    liquid->sceneLink.Unlink();

    if (CWorld::enables & CWorld::Enable_Water) {
      GxXformIdentity(GxXform_World);
      GxXformTranslate(GxXform_World, liquid->chunk->corner - camPos);
      CMap::SelectLight(liquid->chunk);
      liquid->Render(0);
    }
  }

  GxXformPop(GxXform_Tex1);
  GxRsPop();
}

void CWorldScene::RenderMagma() {
  if (!sortTable.visLiquidList[2].Head()) {
    return;
  }

  GxRsPush();
  GxRsSet(GxRs_Culling, 0);
  GxRsSet(GxRs_Lighting, 0);
  GxRsSet(GxRs_Blend, GxBlend_Opaque);

  SAFEITERATELIST(CChunkLiquid, sortTable.visLiquidList[2], liquid) {
    liquid->sceneLink.Unlink();

    if (CWorld::enables & CWorld::Enable_Water) {
      GxXformIdentity(GxXform_World);
      GxXformTranslate(GxXform_World, liquid->chunk->corner - camPos);
      CMap::SelectLight(liquid->chunk);
      liquid->Render(2);
    }
  }

  GxRsPop();
}

void CWorldScene::CullMapObjDefs(CSortEntry *sortEntry, const NTempest::CRect &sRect) {
  NTempest::C44Matrix gxWm;
  NTempest::C44Matrix mapObjM;

  SAFEITERATELIST(CMapObjDef, sortEntry->mapObjDefList, mapObjDef) {
    CMapObj *mapObj = mapObjDef->mapObj;
    FATALASSERT(mapObj);

    NTempest::C33Matrix basis = mapObjDef->mat;

    if (FrustumCull(mapObjDef->aaBox)) {
      continue;
    }

    if (ClipBufferCull(mapObjDef->aaBox, 1)) {
      continue;
    }

    gxWm.Identity();
    gxWm.Translate(-camPos);
    mapObjM = mapObjDef->mat * gxWm;
    GxXformSet(GxXform_World, mapObjM);

    CMapObj::localCamPos = camPos * mapObjDef->invMat;
    CMapObj::SetGroupRenderCallback(CullMapObjDefGroup, mapObjDef);
    CMapObj::curMapObjDef = mapObjDef;
    mapObj->ExtRender(mapObjDef->mat, sRect);
    mapObjDef->sceneLink.Unlink();
  }
}

void CWorldScene::CullMapObjDef(CMapObjDef *mapObjDef, TSGrowableArray<UINT> &inGroups) {
  FATALASSERT(mapObjDef);
  FATALASSERT(inGroups.Count() != 0);

  NTempest::C44Matrix gxWm;
  NTempest::C44Matrix mapObjM;
  CMapObj            *mapObj = mapObjDef->mapObj;
  FATALASSERT(mapObj);

  gxWm.Identity();
  gxWm.Translate(-camPos);
  mapObjM = mapObjDef->mat * gxWm;
  GxXformSet(GxXform_World, mapObjM);

  CMapObj::localCamPos = camPos * mapObjDef->invMat;
  CMapObj::SetGroupRenderCallback(CullMapObjDefGroup, mapObjDef);
  CMapObj::curMapObjDef = mapObjDef;
  mapObj->IntRender(mapObjDef->mat, inGroups);
  mapObjDef->sceneLink.Unlink();
}

void CWorldScene::CullMapObjDefGroup(const UINT groupNum, LPCVOID userParam, const int rDrawSharedLiquidToggle) {
  CMapObjDef *mapObjDef = (CMapObjDef *)userParam;
  FATALASSERT(mapObjDef);

  CMapObjDefGroup *mapObjDefGroup = 0;
  {
    ITERATELIST(CMapBaseObjLink, mapObjDef->groupLinkList, groupLink) {
      mapObjDefGroup = (CMapObjDefGroup *)groupLink->owner;
      if (mapObjDefGroup->groupNum == groupNum) {
        break;
      }
    }
  }
  FATALASSERT(mapObjDefGroup);

  if (mapObjDefGroup->sceneLink.IsLinked()) {
    CWFrustum *frustum = AllocFrustum();
    *frustum = FrustumGet();
    mapObjDefGroup->frustumList.LinkNode(frustum, LIST_TAIL, 0);
  } else {
    sortTable.visMapObjDefGroupList.LinkNode(mapObjDefGroup, LIST_TAIL, 0);
    mapObjDefGroup->ambient = mapObjDef->ambient;
    mapObjDefGroup->rDrawSharedLiquidToggle = rDrawSharedLiquidToggle;
    FATALASSERT(mapObjDefGroup->frustumList.Head() == 0);
    CWFrustum *frustum = AllocFrustum();
    *frustum = FrustumGet();
    mapObjDefGroup->frustumList.LinkNode(frustum, LIST_TAIL, 0);
  }

  if (CWorld::enables & 0x400081) {
    CullDoodads(mapObjDefGroup->doodadDefLinkList);
  }

  {
    ITERATELIST(CMapBaseObjLink, mapObjDefGroup->entityLinkList, entityLink) {
      CMapEntity *entity = (CMapEntity *)entityLink->owner;
      if (entity->flagVisible != 1) {
        entity->camDist = camPlaneXY.DistSigned(entity->aaSphere.c) - entity->aaSphere.r;
        if (entity->camDist <= CWorld::unitDrawDist) {
          if (!FrustumCull(entity->aaSphere.c, entity->aaSphere.r)) {
            entity->flagVisible = 1;
            entity->sceneLink.Unlink();
            sortTable.visEntityList.LinkNode(entity, LIST_TAIL, 0);
          }
        }
      }
    }
  }
}

void CWorldScene::ClipBufferClear() {
  for (UINT i = 0; i < sizeof(clipBuffer) / sizeof(clipBuffer[0]); ++i) {
    clipBuffer[i] = -1.0f;
  }
}

int CWorldScene::ClipBufferCull(const NTempest::C3Vector &center, float radius, UINT cullFlags) {
  if (!(CWorld::enables & CWorld::Enable_Culling)) {
    return 0;
  }
  if (NTempest::CMath::fabs_(radius) < 2.38418579e-7f) {
    return 0;
  }

  NTempest::C4Vector v(center);
  NTempest::C4Vector vr(radius, radius, 0.0f, 1.0f);
  v = v * mvp;
  vr = vr * mp;
  if (!(cullFlags & 8) && v.w < 50.0f) {
    return 0;
  }

  float ooW = 1.0f / v.w;
  v.x *= ooW;
  v.y *= ooW;
  v.x += 1.0f;
  vr.x *= ooW;
  vr.y *= ooW;

  v.y += vr.x;
  if (v.y > 1.0f) {
    v.y = 1.0f;
  }

  if (cullFlags & 4) {
    if (v.w - radius > cullDistance) {
      return 1;
    }
  }
  if (cullFlags & 1) {
    if (vr.x < cullSmallThreshold && vr.y < cullSmallThreshold) {
      return 1;
    }
  }

  int first = Fast_ftol((v.x - vr.y) * CBSCALE);
  int last = Fast_ftol((vr.y + v.x) * CBSCALE) + 1;

  if (first < 0) {
    first = 0;
  }
  if (last >= 128) {
    last = 127;
  }

  for (; first <= last; ++first) {
    if (clipBuffer[first] < v.y) {
      return 0;
    }
  }
  return 1;
}

int CWorldScene::ClipBufferCull(const NTempest::CAaBox &aaBox, UINT cullFlags) {
  if (!(CWorld::enables & CWorld::Enable_Culling)) {
    return 0;
  }

  NTempest::C3Vector aaBoxMin(3.4028235e38f);
  NTempest::C3Vector aaBoxMax(-3.4028235e38f);
  const NTempest::C3Vector *aaBoxMinMax[2] = {&aaBox.b, &aaBox.t};
  for (UINT i = 0; i < 8; ++i) {
    NTempest::C4Vector v(aaBoxMinMax[s_boxCornerIndicesX[i]]->x, aaBoxMinMax[s_boxCornerIndicesY[i]]->y, aaBoxMinMax[s_boxCornerIndicesZ[i]]->z, 1.0f);
    v = v * mvp;
    if (!(cullFlags & 8) && v.w < 50.0f) {
      return 0;
    }
    float ooW = 1.0f / v.w;
    v.x *= ooW;
    v.y *= ooW;
    if (v.x < aaBoxMin.x)
      aaBoxMin.x = v.x;
    if (v.x > aaBoxMax.x)
      aaBoxMax.x = v.x;
    if (v.y < aaBoxMin.y)
      aaBoxMin.y = v.y;
    if (v.y > aaBoxMax.y)
      aaBoxMax.y = v.y;
    if (v.w < aaBoxMin.z)
      aaBoxMin.z = v.w;
  }

  aaBoxMin.x += 1.0f;
  aaBoxMax.x += 1.0f;
  if (aaBoxMax.y > 1.0f) {
    aaBoxMax.y = 1.0f;
  }

  if ((cullFlags & 4) && aaBoxMin.z > cullDistance) {
    return 1;
  }

  int first = Fast_ftol(CBSCALE * aaBoxMin.x);
  int last = Fast_ftol(CBSCALE * aaBoxMax.x) + 1;

  if (first < 0) {
    first = 0;
  }
  if (last >= 128) {
    last = 127;
  }

  for (; first <= last; ++first) {
    if (clipBuffer[first] < aaBoxMax.y) {
      return 0;
    }
  }
  return 1;
}

CWFrustum::CWFrustum(
    const NTempest::C3Vector &lPos,
    const NTempest::C3Vector &lAt,
    const NTempest::C3Vector &lUp,
    float                     p_fovy,
    float                     p_aspect,
    float                     p_minz,
    float                     p_maxz
)
    : lookPos(lPos), lookAt(lAt), lookUp(lUp), fovy(p_fovy), aspect(p_aspect), minz(p_minz), maxz(p_maxz) {
  NTempest::C44Matrix viewMat;
  NTempest::C44Matrix projMat;
  GxuXformCreateLookAtSgCompat(lPos, lAt, lUp, viewMat);
  GxuXformCreateProjection(fovy * 0.017453292f, aspect, minz, maxz, projMat);
  GxuXformCalcFrustumCorners(viewMat, projMat, corners);
  CalcPlanesFromCorners();
}

CWFrustum::CWFrustum(const NTempest::C3Vector *c) {
  for (UINT i = 0; i < NUM_CORNERS; ++i) {
    corners[i] = c[i];
  }

  CalcPlanesFromCorners();
}

void CWFrustum::CalcPlanesFromCorners(const NTempest::C3Vector *c) {
  for (UINT i = 0; i < 8; ++i) {
    corners[i] = c[i];
  }
  CalcPlanesFromCorners();
}

void CWFrustum::CalcPlanesFromCorners() {
  planes[0].Set(corners[1], corners[5], corners[6]);
  planes[1].Set(corners[0], corners[7], corners[4]);
  planes[2].Set(corners[0], corners[4], corners[5]);
  planes[3].Set(corners[3], corners[6], corners[7]);
  planes[4].Set(corners[5], corners[4], corners[6]);
  planes[5].Set(corners[2], corners[0], corners[1]);
}

void CWFrustum::Translate(const NTempest::C3Vector &t) {
  UINT i;
  for (i = 0; i < 8; ++i) {
    corners[i] += t;
  }
  for (i = 0; i < 6; ++i) {
    planes[i].Translate(t);
  }
  lookPos += t;
  lookAt += t;
}

void CWFrustum::Transform(const NTempest::C44Matrix &mat) {
  for (UINT i = 0; i < 8; ++i) {
    corners[i] *= mat;
  }
  CalcPlanesFromCorners();
  lookPos *= mat;
  lookAt *= mat;
}

WorldCullStatus CWFrustum::Cull(const NTempest::CAaBox &aabox) const {
  float *corner[2] = {(float *)&aabox.t.x, (float *)&aabox.b.x};
  for (int p = 0; p < 6; ++p) {
    if (corner[*(const DWORD *)&planes[p].n.x >> 31][0] * planes[p].n.x +
            corner[*(const DWORD *)&planes[p].n.z >> 31][2] * planes[p].n.z +
            corner[*(const DWORD *)&planes[p].n.y >> 31][1] * planes[p].n.y + planes[p].d <
        -0.019444443f) {
      return WorldCull_outside;
    }
  }
  return WorldCull_notOutside;
}

WorldCullStatus CWFrustum::Cull(const NTempest::CAaBox &box, NTempest::C33Matrix &basis, NTempest::C3Vector &pos) {
  NTempest::C33Matrix m = basis.Transpose();
  const float *corner[2] = {&box.t.x, &box.b.x};
  for (int p = 0; p < 6; ++p) {
    NTempest::C3Vector wv = planes[p].n * m;
    wv = NTempest::C3Vector(
             corner[(DWORD)NTempest::CMath::realasint32_(wv.x) >> 31][0],
             corner[(DWORD)NTempest::CMath::realasint32_(wv.y) >> 31][1],
             corner[(DWORD)NTempest::CMath::realasint32_(wv.z) >> 31][2]
         ) * basis +
         pos;
    if (planes[p].DistSigned(wv) < -0.019444443f) {
      return WorldCull_outside;
    }
  }
  return WorldCull_notOutside;
}

WorldCullStatus CWFrustum::Cull(const NTempest::C3Vector &center, float radius) const {
  NTempest::CAaSphere sphere;
  sphere.c = center;
  sphere.r = radius;
  return Cull(sphere);
}

WorldCullStatus CWFrustum::Cull(const NTempest::CAaSphere &sphere) const {
  for (UINT p = 0; p != 6; ++p) {
    if (planes[p].DistSigned(sphere.c) < -sphere.r) {
      return WorldCull_outside;
    }
  }
  return WorldCull_notOutside;
}

WorldCullStatus CWFrustum::Cull(const NTempest::C3Vector &point) const {
  for (UINT p = 0; p < 6; ++p) {
    if (planes[p].DistSigned(point) < -0.019444443f) {
      return WorldCull_outside;
    }
  }
  return WorldCull_notOutside;
}

void CWFrustum::Cull(const NTempest::C3Vector &point, UINT &cullFlags) const {
  cullFlags = 0;
  for (UINT p = 0; p < 6; ++p) {
    if (planes[p].DistSigned(point) < -0.019444443f) {
      cullFlags |= 1 << p;
    }
  }
}

WorldCullStatus CWFrustum::Cull(const NTempest::C4Plane &plane) const {
  UINT counts[4];
  memset(counts, 0, sizeof(counts));
  for (UINT i = 0; i < 8; ++i) {
    if (plane.DistSigned(corners[i]) > 0.019444443f) {
      ++counts[1];
    } else if (plane.DistSigned(corners[i]) < -0.019444443f) {
      ++counts[0];
    } else {
      return WorldCull_intersect;
    }
  }
  if (!counts[1]) {
    return WorldCull_outside;
  }
  return counts[0] ? WorldCull_intersect : WorldCull_inside;
}

struct ClipInfo {
  float bc[6];
  UINT  mask;
  UINT  filler;

  void Set(const NTempest::C3Vector *v);
};

void ClipInfo::Set(const NTempest::C3Vector *v) {
  bc[0] = v->x;
  bc[1] = 1.0f - v->x;
  bc[2] = v->y;
  bc[3] = 1.0f - v->y;
  bc[4] = v->z;
  bc[5] = 1.0f - v->z;
  mask = (*(const DWORD *)&bc[0] & 0x80000000) +
         ((*(const DWORD *)&bc[2] >> 2) & 0x20000000) +
         ((*(const DWORD *)&bc[5] >> 5) & 0x04000000) +
         ((*(const DWORD *)&bc[3] >> 3) & 0x10000000) +
         ((*(const DWORD *)&bc[1] >> 1) & 0x40000000) +
         ((*(const DWORD *)&bc[4] >> 4) & 0x08000000);
}

struct ClipFrame {
  NTempest::C3Vector **points;
  ClipInfo           **info;
  UINT                 count;

  ClipFrame(NTempest::C3Vector **points, ClipInfo **info, UINT count) : points(points), info(info), count(count) {
  }
};

enum {
  POOL_SIZE = 32
};

BOOL CWorld::NDCClip(NTempest::C3Vector *p_inVerts, UINT p_inCount, NTempest::C3Vector **&p_outVerts, UINT &p_outCount) {
  static NTempest::C3Vector  sPointPool[POOL_SIZE];
  static NTempest::C3Vector *inPointPtrs[POOL_SIZE];
  static NTempest::C3Vector *outPointPtrs[POOL_SIZE];
  ClipInfo                   sInInfo[POOL_SIZE];
  ClipInfo                   sInfoPool[POOL_SIZE];
  ClipInfo                  *inInfoPtrs[POOL_SIZE];
  ClipInfo                  *outInfoPtrs[POOL_SIZE];
  NTempest::C3Vector        *pointPool = sPointPool;
  ClipInfo                  *infoPool = sInfoPool;
  UINT                       andMask = 0xFFFFFFFF;
  UINT                       orMask = 0;

  FATALASSERT(p_inCount < POOL_SIZE);
  for (UINT i = 0; i < p_inCount; ++i) {
    sInInfo[i].Set(&p_inVerts[i]);
    inInfoPtrs[i] = &sInInfo[i];
    inPointPtrs[i] = &p_inVerts[i];
    andMask &= sInInfo[i].mask;
    orMask |= sInInfo[i].mask;
  }

  if (andMask) {
    return 0;
  }
  if (!orMask) {
    p_outVerts = inPointPtrs;
    p_outCount = p_inCount;
    return 1;
  }

  ClipFrame  inFrame(inPointPtrs, inInfoPtrs, p_inCount);
  ClipFrame  outFrame(outPointPtrs, outInfoPtrs, 0);
  ClipFrame *in = &inFrame;
  ClipFrame *oframe = &outFrame;

  UINT planeMask = 0x80000000;
  for (UINT plane = 0; plane < 6; ++plane, planeMask >>= 1) {
    if (!(orMask & planeMask)) {
      continue;
    }

    oframe->count = 0;
    UINT from = in->count - 1;
    UINT fromMask = in->info[from]->mask & planeMask;
    for (UINT toVert = 0; toVert < in->count; ++toVert) {
      UINT toOut = in->info[toVert]->mask & planeMask;
      if (fromMask != toOut) {
        float denominator = in->info[from]->bc[plane] - in->info[toVert]->bc[plane];
        if (denominator == 0.0f) {
          denominator = 0.0001f;
        }
        float t = in->info[from]->bc[plane] / denominator;
        *pointPool = *in->points[from] + (*in->points[toVert] - *in->points[from]) * t;
        infoPool->Set(pointPool);
        oframe->points[oframe->count] = pointPool++;
        oframe->info[oframe->count++] = infoPool++;
      }
      if (!toOut) {
        oframe->points[oframe->count] = in->points[toVert];
        oframe->info[oframe->count++] = in->info[toVert];
      }

      FATALASSERT(oframe->count < POOL_SIZE);
      FATALASSERT(pointPool - sPointPool < (sizeof(sPointPool) / sizeof(sPointPool[0])));
      from = toVert;
      fromMask = toOut;
    }

    if (!oframe->count) {
      return 0;
    }

    ClipFrame *temp = in;
    in = oframe;
    oframe = temp;
  }

  p_outVerts = in->points;
  p_outCount = in->count;
  return 1;
}

bool CWorld::NDCXform(const CWFrustum &frustum, NTempest::C44Matrix &xf, bool translate) {
  NTempest::C3Vector right = frustum.Corner(CWFrustum::NEAR_LR) - frustum.Corner(CWFrustum::NEAR_LL);
  NTempest::C3Vector up = frustum.Corner(CWFrustum::NEAR_UL) - frustum.Corner(CWFrustum::NEAR_LL);
  NTempest::C3Vector forward = frustum.Corner(CWFrustum::FAR_LL) - frustum.Corner(CWFrustum::NEAR_LL);

  xf.Identity();
  *xf.Row0AsVec3() = right;
  *xf.Row1AsVec3() = up;
  *xf.Row2AsVec3() = forward;
  if (translate) {
    *xf.Row3AsVec3() = frustum.Corner(CWFrustum::NEAR_LL);
  }

  float det = xf.Determinant();
  if (!(NTempest::CMath::fabs_(det) < 0.00000023841858f)) {
    xf = xf.Inverse(det);
  } else {
    xf.Identity();
    return false;
  }
  return true;
}
