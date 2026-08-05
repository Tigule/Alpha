#include <Base/Base.h>
#include <WowConst.h>
#include <MapDefs.h>

#include "World.h"
#include "CMapObj.h"
#include "DetailDoodad.h"
#include "Ui/WorldFrame.h"

#include "DayNight.h"
#include "Gx/Gx.h"
#include "Model/IModel.h"
#include "Services/SysMessage.h"
#include "Tempest/c33matrix.h"
#include "Tempest/c34matrix.h"
#include "Tempest/cmath.h"

#include <new>
#include <string.h>

void ShadowRender(HMODEL hModel, const NTempest::C44Matrix &basis, void *param);

LISTDECLEX(CWFrustum, sceneLink, CWorldScene::frustumFreeList);
CSortTable                      CWorldScene::sortTable;
NTempest::C4Vector              CWorldScene::clipVertexBuffer[9];
float                           CWorldScene::clipBuffer[128];
CMapEntity                     *CWorldScene::camTargEntity;
CMapObjDef                     *CWorldScene::viewerMapObjDef;
TSGrowableArray<unsigned int>   CWorldScene::viewerMapObjGroups;
unsigned int                    CWorldScene::bspStateBits;
float                           CWorldScene::cullSmallThreshold = 0.01f;
float                           CWorldScene::cullDistance = 500.0f;
NTempest::CAaBox                CWorldScene::camFrustumBounds;
NTempest::C3Vector              CWorldScene::camFrustumCorners[8];
CWFrustum                       CWorldScene::frustumStack[16];
int                             CWorldScene::frustumIndex;
NTempest::C3Vector              CWorldScene::camPos;
NTempest::C3Vector              CWorldScene::camTarg;
NTempest::C3Vector              CWorldScene::camVec;
NTempest::C4Plane               CWorldScene::camPlaneXY;
unsigned int                    CWorldScene::camLiquid;
CMapObjDef                     *CWorldScene::camMapObjDef;
CMapObj                        *CWorldScene::camMapObj;
CMapObjGroup                   *CWorldScene::camMapObjGroup;
NTempest::C44Matrix             CWorldScene::mvp;
NTempest::C44Matrix             CWorldScene::mv;
NTempest::C44Matrix             CWorldScene::mp;
NTempest::C3Vector              CWorldScene::vpMinPos;
NTempest::C3Vector              CWorldScene::vpMaxPos;
NTempest::C4Plane               CWorldScene::vpPlanes[4];
NTempest::C4Vector              CWorldScene::mvpCol3;
NTempest::C44Matrix             CWorldScene::gxViewMat;
unsigned int                    CWorldScene::nPrimsRendered;
unsigned int                    CWorldScene::nChunksRendered;
unsigned int                    CWorldScene::nDoodadsRendered;
unsigned int                    CWorldScene::nObjectsRendered;
char                            CWorldScene::currentChunkName[64];

void CSortTable::Initialize() {
}

void CSortTable::Destroy() {
}

void CSortTable::Clear() {
  for (unsigned int index = 0; index < 26; ++index) {
    CMapChunk *chunk = table[index].chunkList.Head();
    while (chunk) {
      CMapChunk *next = table[index].chunkList.Next(chunk);
      table[index].chunkList.UnlinkNode(chunk);
      chunk = next;
    }

    for (unsigned int type = 0; type < 4; ++type) {
      CChunkLiquid *liquid = table[index].liquidList[type].Head();
      while (liquid) {
        CChunkLiquid *next = table[index].liquidList[type].Next(liquid);
        table[index].liquidList[type].UnlinkNode(liquid);
        liquid = next;
      }
    }

    CMapDoodadDef *doodadDef = table[index].doodadDefList.Head();
    while (doodadDef) {
      CMapDoodadDef *next = table[index].doodadDefList.Next(doodadDef);
      table[index].doodadDefList.UnlinkNode(doodadDef);
      doodadDef = next;
    }

    CMapObjDef *mapObjDef = table[index].mapObjDefList.Head();
    while (mapObjDef) {
      CMapObjDef *next = table[index].mapObjDefList.Next(mapObjDef);
      table[index].mapObjDefList.UnlinkNode(mapObjDef);
      mapObjDef = next;
    }

    CMapEntity *entity = table[index].entityList.Head();
    while (entity) {
      CMapEntity *next = table[index].entityList.Next(entity);
      entity->flagVisible = 0;
      table[index].entityList.UnlinkNode(entity);
      nonVisEntityList.LinkNode(entity, LIST_TAIL, 0);
      entity = next;
    }
  }
}

void CWorldScene::Initialize() {
  vpPlanes[0].n = NTempest::C3Vector(1.0f, 0.0f, 1.0f);
  vpPlanes[1].n = NTempest::C3Vector(-1.0f, 0.0f, 1.0f);
  vpPlanes[2].n = NTempest::C3Vector(0.0f, 1.0f, 1.0f);
  vpPlanes[3].n = NTempest::C3Vector(0.0f, -1.0f, 1.0f);

  camTargEntity = 0;
  camLiquid = 15;

  vpPlanes[0].d = 0.0f;
  vpPlanes[1].d = 0.0f;
  vpPlanes[2].d = 0.0f;
  vpPlanes[3].d = 0.0f;

  vpPlanes[0].n.Normalize();
  vpPlanes[1].n.Normalize();
  vpPlanes[2].n.Normalize();
  vpPlanes[3].n.Normalize();
}

void CWorldScene::Destroy() {
  while (CWFrustum *frustum = frustumFreeList.Head()) {
    frustumFreeList.UnlinkNode(frustum);
    frustum->~CWFrustum();
    SMemFree(frustum, typeid(CWFrustum).INTERNALRAWNAME(), SERR_LINECODE_OBJECT, 0);
  }
}

CWFrustum *CWorldScene::AllocFrustum() {
  CWFrustum *frustum = frustumFreeList.Head();
  if (!frustum) {
    void *storage = SMemAlloc(sizeof(CWFrustum), typeid(CWFrustum).INTERNALRAWNAME(), -2, 8);
    frustum = storage ? new (storage) CWFrustum : 0;
    frustumFreeList.LinkNode(frustum, LIST_TAIL, 0);
    FATALASSERT(frustum);
  }

  frustumFreeList.UnlinkNode(frustum);
  return frustum;
}

void CWorldScene::FreeFrustum(CWFrustum *frustum) {
  FATALASSERT(frustum);
  frustum->sceneLink.Unlink();
  frustumFreeList.LinkNode(frustum, LIST_TAIL, 0);
}

void CWorldScene::PrepareRenderLiquid() {
  NTempest::C3Vector lqDir(0.0f, 0.0f, 0.0f);
  NTempest::C3Vector camQueryPos = camPos;
  float              lqSurface;
  unsigned int       newLiquid = 15;
  int                forceFullUpdate = 0;

  for (unsigned int i = 0; i < 4; ++i) {
    if (camFrustumCorners[i].z < camQueryPos.z) {
      camQueryPos.z = camFrustumCorners[i].z;
    }
  }

  if (camMapObjDef && camMapObj && camMapObjGroup) {
    camMapObjGroup->QueryLiquidStatus(camQueryPos * camMapObjDef->invMat, newLiquid, lqSurface, lqDir);
  } else {
    CWorld::QueryLiquidStatus(camQueryPos, newLiquid, lqSurface, lqDir);
  }

  if (newLiquid == 15) {
    forceFullUpdate = camLiquid != 15;
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
  NTempest::C3Vector camPlaneVectXY;

  camPos = position;
  camTarg = target;
  camVec = target - position;
  camVec.Normalize();
  camPlaneVectXY = NTempest::C3Vector(camVec.x, camVec.y, 0.0f);
  camPlaneXY.n = camPlaneVectXY;
  camPlaneXY.d = -NTempest::C3Vector::Dot(camPlaneVectXY, position);

  GxXformView(mv);
  GxXformProjection(mp);
  GxXformViewport(vpMinPos.x, vpMaxPos.x, vpMinPos.y, vpMaxPos.y, vpMinPos.z, vpMaxPos.z);
  mv.Translate(-camPos);
  mvp = mv * mp;
  mvpCol3 = NTempest::C4Vector(mvp.a3, mvp.b3, mvp.c3, mvp.d3);
  CalcFrustumCorners(camFrustumCorners);
  camFrustumBounds = NTempest::CAaBox::Bounding(camFrustumCorners, 8);
  frustumIndex = 0;
  PrepareRenderLiquid();
}

void CWorldScene::Update() {
}

void CWorldScene::Render() {
  NTempest::C3Vector max;
  NTempest::C3Vector saveMin;
  NTempest::C3Vector saveMax;

  GxRsPush();
  GxXformViewport(saveMin.x, saveMax.x, saveMin.y, saveMax.y, saveMin.z, saveMax.z);
  max = saveMax - (saveMax - saveMin) * 0.05f;
  GxXformSetViewport(saveMin.x, saveMax.x, saveMin.y, saveMax.y, saveMin.z, max.z);

  for (unsigned int i = 0; i < GxCaps().m_numTmus; ++i) {
    GxRsSet(static_cast<EGxRenderState>(GxRs_TexLodBias0 + i), -CWorld::texLodBias);
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
    static TSCArray<NTempest::CRect, 16> s_extViewList;

    ClipBufferClear();
    CullMapObjDef(viewerMapObjDef, viewerMapObjGroups);
    s_extViewList.Set(CMapObj::extViewList.Count(), CMapObj::extViewList.Ptr());
    for (unsigned int i = 0; i < s_extViewList.Count(); ++i) {
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
  NTempest::C44Matrix cMat;

  GxRsPush();
  GxXformPush(GxXform_World);
  GxRsSet(GxRs_TexLodBias0, 0.5f);

  CMapChunk *chunk = sortTable.visChunkList.Head();
  while (chunk) {
    if (chunk->detailDoodadInst) {
      cMat = NTempest::C44Matrix();
      cMat.Translate(chunk->corner - camPos);
      GxXformSet(GxXform_World, cMat);
      CMap::SelectLight(chunk);
      chunk->detailDoodadInst->RenderAlpha();
    }
    sortTable.visChunkList.UnlinkNode(chunk);
    chunk = sortTable.visChunkList.Head();
  }

  GxRsSet(GxRs_TexLodBias0, -(CWorld::texLodBias - 0.5f));
  GxXformPop(GxXform_World);
  GxRsPop();
}

void CWorldScene::AddDoodadDef(CMapDoodadDef *doodadDef) {
  NTempest::CAaSphere bounds;
  FATALASSERT(doodadDef);
  doodadDef->GetBounds(bounds);
  doodadDef->camDist = camPlaneXY.DistSigned(bounds.c) - bounds.r;
  int sortIndex = static_cast<int>(doodadDef->camDist * 0.03f - 0.5f);
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
  int sortIndex = static_cast<int>(mapObjDef->camDist * 0.03f - 0.5f);
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

  int sortIndex = static_cast<int>(sortDist * 0.03f - 0.5f);
  if (sortIndex < 0) {
    sortIndex = 0;
  } else if (sortIndex >= 26) {
    SysMsgPrintf(SYSMSG_WARNING, 2, "CHUNKDISTTOOFARTOSORT");
    return;
  }

  sortTable.table[sortIndex].chunkList.LinkNode(chunk, LIST_TAIL, 0);
}

void CWorldScene::AddChunkLiquid(CChunkLiquid *liquid, unsigned int type) {
  FATALASSERT(liquid);
  FATALASSERT(type < 4);
  int sortIndex = static_cast<int>(liquid->chunk->camDist * 0.03f - 0.5f);
  if (sortIndex < 0) {
    sortIndex = 0;
  } else if (sortIndex >= 26) {
    SysMsgPrintf(SYSMSG_WARNING, 2, "CHUNKDISTTOOFARTOSORT");
    return;
  }
  sortTable.table[sortIndex].liquidList[type].LinkNode(liquid, LIST_TAIL, 0);
}

void CWorldScene::AddMapEntity(CMapEntity *entity) {
  FATALASSERT(entity);
  entity->camDist = camPlaneXY.DistSigned(entity->aaSphere.c) - entity->aaSphere.r;
  int sortIndex = static_cast<int>(entity->camDist * 0.03f - 0.5f);
  if (sortIndex < 0) {
    sortIndex = 0;
  } else if (sortIndex >= 26) {
    SysMsgPrintf(SYSMSG_WARNING, 2, "ENTITYDISTTOOFARTOSORT");
    return;
  }
  sortTable.table[sortIndex].entityList.LinkNode(entity, LIST_TAIL, 0);
}

void CWorldScene::ClipBufferUpdate(
    const NTempest::C3Vector *vertices,
    const int                *indicies,
    const int                 nVertices,
    const NTempest::C3Vector &corner
) {
  FATALASSERT(vertices);
  FATALASSERT(indicies);
  int i;

  if (!(CWorld::enables & CWorld::Enable_Culling)) {
    return;
  }

  for (i = 0; i < nVertices; ++i) {
    clipVertexBuffer[i] = vertices[indicies[i]] + corner;
    clipVertexBuffer[i].w = 1.0f;
    clipVertexBuffer[i] = clipVertexBuffer[i] * mvp;
    float ooW = 1.0f / clipVertexBuffer[i].w;
    clipVertexBuffer[i].x = clipVertexBuffer[i].x * ooW + 1.0f;
    clipVertexBuffer[i].y = clipVertexBuffer[i].y * ooW;
  }

  for (i = 0; i < nVertices - 1; ++i) {
    NTempest::C4Vector &v0 = clipVertexBuffer[i];
    NTempest::C4Vector &v1 = clipVertexBuffer[i + 1];
    if (v0.w < 2.7777777f || v1.w < 2.7777777f) {
      continue;
    }

    float cullValue = v0.y < v1.y ? v0.y : v1.y;
    int   x0 = static_cast<int>(v0.x * 64.0f - 0.5f);
    int   x1 = static_cast<int>(v1.x * 64.0f - 0.5f);
    if (x1 < x0) {
      int x = x0;
      x0 = x1;
      x1 = x;
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

void CWorldScene::ClipPortal(NTempest::C4Vector *inList, unsigned int &inCount) {
  static NTempest::C4Vector outList[16];
  NTempest::C4Plane         plane;
  unsigned int              c[2] = {inCount, 0};
  NTempest::C4Vector       *v[2] = {inList, outList};

  for (unsigned int p = 0; p < 4; ++p) {
    plane = vpPlanes[p];
    unsigned int from = p & 1;
    unsigned int to = (p - 1) & 1;
    c[to] = 0;
    if (!c[from]) {
      inCount = 0;
      return;
    }

    for (unsigned int cnt = 0; cnt < c[from]; ++cnt) {
      unsigned int        idx1 = (cnt + 1) % c[from];
      NTempest::C4Vector *v0 = &v[from][cnt];
      NTempest::C4Vector *v1 = &v[from][idx1];
      float               d0 = plane.n.x * v0->x + plane.n.y * v0->y + plane.n.z * v0->w;
      float               d1 = plane.n.x * v1->x + plane.n.y * v1->y + plane.n.z * v1->w;
      int                 side0 = d0 > 0.019444443f ? 1 : (d0 < -0.019444443f ? 2 : 0);
      int                 side1 = d1 > 0.019444443f ? 1 : (d1 < -0.019444443f ? 2 : 0);

      if (side0 != 2) {
        v[to][c[to]++] = *v0;
      }
      if (side1 && side1 != side0) {
        float               t = d0 / (d0 - d1);
        NTempest::C4Vector &out = v[to][c[to]++];
        out.x = v0->x + (v1->x - v0->x) * t;
        out.y = v0->y + (v1->y - v0->y) * t;
        out.w = v0->w + (v1->w - v0->w) * t;
      }
    }
  }

  inCount = c[0];
}

void CWorldScene::CalcFrustumCorners(NTempest::C3Vector *corners) {
  NTempest::C44Matrix lMp;
  NTempest::C44Matrix lMv;
  GxXformView(lMv);
  GxXformProjection(lMp);

  lMv.Translate(-camPos);
  GxuXformCalcFrustumCorners(lMv, lMp, corners);
}

void CWorldScene::LocateViewer() {
  viewerMapObjDef = 0;
  viewerMapObjGroups.SetCount(0);
  currentChunkName[0] = 0;

  CMapObjDef *mapObjDef = sortTable.table[0].mapObjDefList.Head();
  while (mapObjDef) {
    CMapObjDef *mapObjDefnext_node = sortTable.table[0].mapObjDefList.Next(mapObjDef);
    CMapObj    *mapObj = mapObjDef->mapObj;
    FATALASSERT(mapObj);
    mapObj->LocateViewer(mapObjDef->invMat, viewerMapObjGroups);
    if (viewerMapObjGroups.Count()) {
      viewerMapObjDef = mapObjDef;
      sortTable.table[0].mapObjDefList.UnlinkNode(mapObjDef);
      return;
    }
    mapObjDef = mapObjDefnext_node;
  }
}

void CWorldScene::AddViewerGroup2(unsigned int groupNum) {
  for (unsigned int i = 0; i < viewerMapObjGroups.Count(); ++i) {
    if (viewerMapObjGroups[i] == groupNum) {
      return;
    }
  }

  viewerMapObjGroups.Add(&groupNum);
}

void CWorldScene::LocateViewer2() {
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

  CMapChunk   *chunk;
  float        chunkT = 1.0f;
  unsigned int hitChunk = CMap::VectorIntersectTerrain(&lCen, &lEnd, &chunkT, 0, &chunk);

  CMapObjDef  *mapObjDef = 0;
  unsigned int mapObjDefGroupIDs[2];
  float        mapObjT = 1.0f;
  unsigned int hitMapObj = CMap::LocateViewerMapObjs(lCen, lEnd, mapObjT, mapObjDef, mapObjDefGroupIDs);

  if ((!hitChunk || (hitMapObj && chunkT >= mapObjT)) && hitMapObj) {
    camMapObjDef = mapObjDef;
    camMapObj = mapObjDef->mapObj;
    FATALASSERT(camMapObj);

    camMapObjGroup = camMapObj->GetGroup(mapObjDefGroupIDs[0], 0);
    FATALASSERT(camMapObjGroup);

    SStrCopy(currentChunkName, camMapObj->GetGroupName(mapObjDefGroupIDs[0]), sizeof(currentChunkName));

    CMapObjDef *mapObjDefNode = sortTable.table[0].mapObjDefList.Head();
    while (mapObjDefNode) {
      CMapObjDef *mapObjDefNext = sortTable.table[0].mapObjDefList.Next(mapObjDefNode);
      if (mapObjDefNode == mapObjDef) {
        viewerMapObjDef = mapObjDefNode;
        sortTable.table[0].mapObjDefList.UnlinkNode(mapObjDefNode);
      }
      mapObjDefNode = mapObjDefNext;
    }

    FATALASSERT(viewerMapObjDef);

    AddViewerGroup2(mapObjDefGroupIDs[0]);
    if (mapObjDefGroupIDs[1] != 0xFFFF) {
      AddViewerGroup2(mapObjDefGroupIDs[1]);
    }
  }
}

void CWorldScene::FrustumPush() {
  FATALASSERT(frustumIndex < 15);
  frustumStack[frustumIndex + 1] = frustumStack[frustumIndex];
  ++frustumIndex;
}

void CWorldScene::FrustumSet(const NTempest::CRect &sRect) {
  enum {
    FRUST_BL = 0,
    FRUST_TL = 1,
    FRUST_TR = 2,
    FRUST_BR = 3
  };

  NTempest::C3Vector newCorners[8];
  NTempest::C3Vector tr;
  NTempest::C3Vector tl;
  NTempest::C3Vector rd;
  NTempest::C3Vector bd;
  NTempest::C3Vector ld;
  NTempest::C3Vector td;
  NTempest::C3Vector br;
  NTempest::C3Vector bl;

  for (unsigned int i = 0; i < 8; i += 4) {
    td = camFrustumCorners[i + FRUST_TR] - camFrustumCorners[i + FRUST_TL];
    tl = camFrustumCorners[i + FRUST_TL] + td * sRect.l;
    tr = camFrustumCorners[i + FRUST_TL] + td * sRect.r;
    bd = camFrustumCorners[i + FRUST_BR] - camFrustumCorners[i + FRUST_BL];
    bl = camFrustumCorners[i + FRUST_BL] + bd * sRect.l;
    br = camFrustumCorners[i + FRUST_BL] + bd * sRect.r;
    ld = tl - bl;
    rd = tr - br;
    newCorners[i + FRUST_BL] = bl + ld * sRect.t;
    newCorners[i + FRUST_TL] = bl + ld * sRect.b;
    newCorners[i + FRUST_TR] = br + rd * sRect.b;
    newCorners[i + FRUST_BR] = br + rd * sRect.t;
  }

  FrustumGet().CalcPlanesFromCorners(newCorners);
}

void CWorldScene::FrustumSet(const NTempest::C3Vector *corners) {
  FrustumGet().CalcPlanesFromCorners(corners);
}

void CWorldScene::FrustumSet(const NTempest::C3Vector *corners, const NTempest::CRect &sRect) {
  NTempest::C3Vector n;
  NTempest::C3Vector newCorners[8];
  NTempest::C3Vector tl;
  NTempest::C3Vector tr;
  NTempest::C3Vector bd;
  NTempest::C3Vector td;
  NTempest::C3Vector bl;
  NTempest::C3Vector br;
  NTempest::C3Vector a;
  NTempest::C3Vector ld;
  NTempest::C3Vector b;
  NTempest::C3Vector rd;

  for (unsigned int i = 0; i < 8; i += 4) {
    td = corners[i + 2] - corners[i + 1];
    tl = corners[i + 1] + td * sRect.l;
    tr = corners[i + 1] + td * sRect.r;
    bd = corners[i + 3] - corners[i];
    bl = corners[i] + bd * sRect.l;
    br = corners[i] + bd * sRect.r;
    ld = tl - bl;
    rd = tr - br;
    a = bl + ld * sRect.b;
    b = br + rd * sRect.b;
    newCorners[i] = bl + ld * sRect.t;
    newCorners[i + 1] = a;
    newCorners[i + 2] = b;
    newCorners[i + 3] = br + rd * sRect.t;
  }

  n = NTempest::C3Vector::Cross(newCorners[1] - newCorners[2], newCorners[0] - newCorners[2]);
  if (n != NTempest::C3Vector(0.0f)) {
    FrustumGet().CalcPlanesFromCorners(newCorners);
  }
}

void CWorldScene::FrustumSet(const CWFrustum &frustum) {
  FrustumGet() = frustum;
}

CWFrustum &CWorldScene::FrustumGet() {
  return frustumStack[frustumIndex];
}

void CWorldScene::FrustumXform(const NTempest::C44Matrix &mat) {
  FrustumGet().Transform(mat);
}

int CWorldScene::FrustumCull(const NTempest::C3Vector &center, float radius) {
  return FrustumGet().Cull(center, radius) == WorldCull_outside;
}

int CWorldScene::FrustumCull(const NTempest::CAaBox &aaBox) {
  return FrustumGet().Cull(aaBox) == WorldCull_outside;
}

int CWorldScene::FrustumCull(const NTempest::CAaBox &aaBox, NTempest::C33Matrix &basis, NTempest::C3Vector &pos) {
  return FrustumGet().Cull(aaBox, basis, pos) == WorldCull_outside;
}

void CWorldScene::FrustumPop() {
  FATALASSERT(frustumIndex > 0);
  --frustumIndex;
}

void CWorldScene::CullSortTable(const NTempest::CRect &sRect) {
  FrustumPush();
  FrustumSet(sRect);
  for (unsigned int index = 0; index < 26; ++index) {
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
  NTempest::C3Vector  corners[8];
  NTempest::C44Matrix projMat;
  NTempest::C44Matrix viewMat;
  float               fov;
  NTempest::CiRect    areaRect;
  float               farZ;
  float               aspect;
  float               nearZ;

  areaRect.minx = CWorld::areaRect.minx - 3;
  areaRect.miny = CWorld::areaRect.miny - 3;
  areaRect.maxx = CWorld::areaRect.maxx + 3;
  areaRect.maxy = CWorld::areaRect.maxy + 3;

  if (areaRect.minx < 0) {
    areaRect.minx = 0;
  }
  if (areaRect.miny < 0) {
    areaRect.miny = 0;
  }
  if (areaRect.maxx > 63) {
    areaRect.maxx = 63;
  }
  if (areaRect.maxy > 63) {
    areaRect.maxy = 63;
  }

  farZ = CGWorldFrame::GetActiveCamera()->FarZ();
  fov = CGWorldFrame::GetActiveCamera()->FOV();
  aspect = CGWorldFrame::GetActiveCamera()->Aspect();
  nearZ = farZ - 33.0f;
  farZ += 2000.0f;

  GxuXformCreateProjection(fov, aspect, nearZ, farZ, projMat);
  GxXformView(viewMat);
  viewMat.Translate(-camPos);
  memset(corners, 0, sizeof(corners));
  GxuXformCalcFrustumCorners(viewMat, projMat, corners);

  FrustumPush();
  FrustumSet(corners, sRect);
  for (long areaX = areaRect.minx; areaX <= areaRect.maxx; ++areaX) {
    for (long areaY = areaRect.miny; areaY <= areaRect.maxy; ++areaY) {
      CMapAreaLow *areaLow = CMap::areaLowTable[areaY + 64 * areaX];
      if (areaLow && !areaLow->sceneLink.IsLinked() && !FrustumCull(areaLow->aaBox) && !ClipBufferCull(areaLow->aaBox, 8)) {
        sortTable.visAreaLowList.LinkNode(areaLow, LIST_TAIL, 0);
      }
    }
  }
  FrustumPop();
}

void CWorldScene::CullEntitys(CSortEntry *sortEntry) {
  FATALASSERT(sortEntry);
  CMapEntity *entity = sortEntry->entityList.Head();
  while (entity) {
    CMapEntity *next = sortEntry->entityList.Next(entity);
    if (entity->camDist <= CWorld::unitDrawDist && !FrustumCull(entity->aaSphere.c, entity->aaSphere.r) &&
        !ClipBufferCull(entity->aaSphere.c, entity->aaSphere.r, 0))
    {
      entity->flagVisible = 1;
      sortEntry->entityList.UnlinkNode(entity);
      sortTable.visEntityList.LinkNode(entity, LIST_TAIL, 0);
    } else {
      entity->flagVisible = 0;
      sortEntry->entityList.UnlinkNode(entity);
      sortTable.nonVisEntityList.LinkNode(entity, LIST_TAIL, 0);
    }
    entity = next;
  }
}

void CWorldScene::CullDoodads(CSortEntry *sortEntry) {
  NTempest::CAaSphere doodadSphere;
  FATALASSERT(sortEntry);
  CMapDoodadDef *doodadDef = sortEntry->doodadDefList.Head();
  while (doodadDef) {
    CMapDoodadDef *next = sortEntry->doodadDefList.Next(doodadDef);
    if ((!doodadDef->model && !doodadDef->RenderCB) || (doodadDef->flags & CMapBaseObj::Flag_LoadFailed)) {
      doodadDef = next;
      continue;
    }

    doodadDef->GetBounds(doodadSphere);
    if (!FrustumCull(doodadSphere.c, doodadSphere.r) && !ClipBufferCull(doodadSphere.c, doodadSphere.r, 0)) {
      sortEntry->doodadDefList.UnlinkNode(doodadDef);
      sortTable.visDoodadList.LinkNode(doodadDef, LIST_TAIL, 0);
      ++nDoodadsRendered;
    } else {
      if (doodadDef->rCount != CWorld::frameCnt) {
        if (doodadDef->flagAlwaysAnimate && doodadDef->model) {
          ModelAdvanceTime(doodadDef->model);
        }
        doodadDef->rCount = CWorld::frameCnt;
      }
    }
    doodadDef = next;
  }
}

void CWorldScene::CullDoodads(LISTEX(CMapBaseObjLink, refLink) &doodadDefLinkList) {
  NTempest::CAaSphere doodadSphere;
  ITERATELIST(CMapBaseObjLink, doodadDefLinkList, link) {
    CMapDoodadDef *doodadDef = static_cast<CMapDoodadDef *>(link->owner);
    FATALASSERT(doodadDef);
    if ((doodadDef->model || doodadDef->RenderCB) && !(doodadDef->flags & CMapBaseObj::Flag_LoadFailed) && !doodadDef->sceneLink.IsLinked()) {
      doodadDef->GetBounds(doodadSphere);
      doodadDef->camDist = camPlaneXY.DistSigned(doodadSphere.c) - doodadSphere.r;
      if (!FrustumCull(doodadSphere.c, doodadSphere.r) && !ClipBufferCull(doodadSphere.c, doodadSphere.r, 0)) {
        sortTable.visDoodadList.LinkNode(doodadDef, LIST_TAIL, 0);
        ++nDoodadsRendered;
      } else {
        if (doodadDef->rCount != CWorld::frameCnt) {
          if (doodadDef->flagAlwaysAnimate && doodadDef->model) {
            ModelAdvanceTime(doodadDef->model);
          }
          doodadDef->rCount = CWorld::frameCnt;
        }
      }
    }
  }
}

void CWorldScene::CullChunkLiquid(CSortEntry *sortEntry, unsigned int type) {
  NTempest::CAaBox aaBox;
  FATALASSERT(sortEntry);
  CChunkLiquid *liquid = sortEntry->liquidList[type].Head();
  while (liquid) {
    CChunkLiquid *next = sortEntry->liquidList[type].Next(liquid);
    aaBox.b = liquid->chunk->aaBox.b;
    aaBox.t = liquid->chunk->aaBox.t;
    aaBox.b.z = liquid->height.l;
    aaBox.t.z = liquid->height.h;
    if (!FrustumCull(aaBox) && !ClipBufferCull(aaBox, 0)) {
      sortEntry->liquidList[type].UnlinkNode(liquid);
      sortTable.visLiquidList[type].LinkNode(liquid, LIST_TAIL, 0);
    }
    liquid = next;
  }
}

void CWorldScene::CullChunks(CSortEntry *sortEntry) {
  float maxClipBufferUpdateDist = cullDistance - 33.333332f;
  FATALASSERT(sortEntry);
  CMapChunk *chunk = sortEntry->chunkList.Head();
  while (chunk) {
    CMapChunk *next = sortEntry->chunkList.Next(chunk);
    if (chunk->holes != 0xFFFF && !FrustumCull(chunk->aaBox) && !ClipBufferCull(chunk->aaBox, 0)) {
      ++nChunksRendered;
      sortEntry->chunkList.UnlinkNode(chunk);
      if (!(CWorld::enables & CWorld::Enable_Culling) || chunk->holes) {
        sortTable.visChunkList.LinkNode(chunk, LIST_TAIL, 0);
      } else {
        sortTable.updateChunkList.LinkNode(chunk, LIST_TAIL, 0);
      }
    }
    chunk = next;
  }

  if (CWorld::enables & CWorld::Enable_Culling) {
    chunk = sortTable.updateChunkList.Head();
    while (chunk) {
      CMapChunk *next = sortTable.updateChunkList.Next(chunk);
      if (chunk->camDist < maxClipBufferUpdateDist) {
        chunk->UpdateClipBuffer();
      }
      sortTable.updateChunkList.UnlinkNode(chunk);
      sortTable.visChunkList.LinkNode(chunk, LIST_TAIL, 0);
      chunk = next;
    }
  }
}

void CWorldScene::RenderObjects() {
  CMapEntity *entity = sortTable.visEntityList.Head();
  while (entity) {
    CMapEntity *next = sortTable.visEntityList.Next(entity);
    sortTable.visEntityList.UnlinkNode(entity);

    NTempest::C44Matrix basis(static_cast<NTempest::C33Matrix>(entity->rot));
    basis.Scale(entity->scale);
    basis.d0 = entity->pos.x - camPos.x;
    basis.d1 = entity->pos.y - camPos.y;
    basis.d2 = entity->pos.z - camPos.z;

    if (!entity->flagHidden && entity->flagCastShadow) {
      ShadowRender(entity->model, basis, 0);
    }

    if (CMap::entityHandler) {
      CMap::entityHandler(CMap::entityHandlerParam, 1, entity->param64, entity->param32);
    }
    ++nObjectsRendered;
    entity = next;
  }

  entity = sortTable.nonVisEntityList.Head();
  while (entity) {
    CMapEntity *next = sortTable.nonVisEntityList.Next(entity);
    sortTable.nonVisEntityList.UnlinkNode(entity);
    if (CMap::entityHandler) {
      CMap::entityHandler(CMap::entityHandlerParam, 2, entity->param64, entity->param32);
    }
    ++nObjectsRendered;
    entity = next;
  }
}

void CWorldScene::RenderDoodads() {
  if (!(CWorld::enables & (CWorld::Enable_Doodads | CWorld::Enable_Collision | CWorld::Enable_AABoxes))) {
    return;
  }

  CMapDoodadDef *doodadDef = sortTable.visDoodadList.Head();
  while (doodadDef) {
    CMapDoodadDef *doodadDefnext_node = sortTable.visDoodadList.Next(doodadDef);
    sortTable.visDoodadList.UnlinkNode(doodadDef);

    if (doodadDef->model) {
      ModelShowCollision(doodadDef->model, CWorld::enables & CWorld::Enable_Collision);
      ModelShowCollisionAaBox(doodadDef->model, CWorld::enables & CWorld::Enable_AABoxes);
      ModelShowModel(doodadDef->model, CWorld::enables & CWorld::Enable_Doodads);

      if (ModelAdvanceTime(doodadDef->model)) {
        NTempest::C44Matrix transform = doodadDef->mat;
        transform.d0 -= camPos.x;
        transform.d1 -= camPos.y;
        transform.d2 -= camPos.z;

        ModelAnimate(
            doodadDef->model,
            NTempest::C34Matrix(
                transform.a0, transform.a1, transform.a2, transform.b0, transform.b1, transform.b2,
                transform.c0, transform.c1, transform.c2, transform.d0, transform.d1, transform.d2),
            doodadDef->scale,
            camPos,
            camTarg - camPos);

        transform.d0 += camPos.x;
        transform.d1 += camPos.y;
        transform.d2 += camPos.z;
        ModelProcessEvents(
            doodadDef->model,
            NTempest::C34Matrix(
                transform.a0, transform.a1, transform.a2, transform.b0, transform.b1, transform.b2,
                transform.c0, transform.c1, transform.c2, transform.d0, transform.d1, transform.d2));
        ModelAddToScene(doodadDef->model, doodadDef->camDist <= CWorld::farFog ? 0 : 7);
      }
    }

    if (doodadDef->RenderCB) {
      doodadDef->RenderCB(doodadDef->renderCBParam, doodadDef->mat);
    }

    doodadDef = doodadDefnext_node;
  }
}

void CWorldScene::RenderHorizon() {
  NTempest::C44Matrix cMat;
  NTempest::C44Matrix saveProjMat;
  NTempest::C44Matrix projMat;
  NTempest::C3Vector  saveMin;
  NTempest::C3Vector  saveMax;

  CGCamera *camera = CGWorldFrame::GetActiveCamera();
  float     farZ = camera->FarZ();
  float     fov = camera->FOV();
  float     aspect = camera->Aspect();

  GxXformProjection(saveProjMat);
  GxuXformCreateProjection(fov, aspect, farZ - 33.0f, farZ + 2000.0f, projMat);
  GxXformSetProjection(projMat);

  GxXformViewport(saveMin.x, saveMax.x, saveMin.y, saveMax.y, saveMin.z, saveMax.z);
  GxXformSetViewport(saveMin.x, saveMax.x, saveMin.y, saveMax.y, saveMax.z, 1.0f);

  cMat = NTempest::C44Matrix();
  cMat.Translate(-camPos);
  GxXformPush(GxXform_World, cMat);

  CMapAreaLow *areaLow = sortTable.visAreaLowList.Head();
  while (areaLow) {
    CMapAreaLow *next = sortTable.visAreaLowList.Next(areaLow);
    sortTable.visAreaLowList.UnlinkNode(areaLow);

    if (CWorld::enables & CWorld::Enable_LowDetail) {
      CMap::RenderAreaLow(areaLow);
    }

    areaLow = next;
  }

  GxXformPop(GxXform_World);
  GxXformSetViewport(saveMin.x, saveMax.x, saveMin.y, saveMax.y, saveMin.z, saveMax.z);
  GxXformSetProjection(saveProjMat);
}

void CWorldScene::RenderChunks() {
  NTempest::C44Matrix cMat;

  CMapChunk *chunk = sortTable.visChunkList.Head();
  while (chunk) {
    CMapChunk *chunknext_node = sortTable.visChunkList.Next(chunk);

    if (CWorld::enables & CWorld::Enable_Chunks) {
      cMat = NTempest::C44Matrix();
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
      sortTable.visChunkList.UnlinkNode(chunk);
    }

    chunk = chunknext_node;
  }
}

int CWorldScene::ClipBufferCull(const NTempest::C3Vector &center, float radius, unsigned int cullFlags) {
  NTempest::C4Vector v(center.x, center.y, center.z, 1.0f);
  NTempest::C4Vector vr(radius, radius, 0.0f, 0.0f);
  if (!(CWorld::enables & CWorld::Enable_Culling) || NTempest::CMath::fabs_(radius) < 2.38418579e-7f) {
    return 0;
  }

  v = v * mvp;
  vr = vr * mp;
  if (!(cullFlags & 8) && v.w < 50.0f) {
    return 0;
  }

  float ooW = 1.0f / v.w;
  float left = (v.x - NTempest::CMath::fabs_(vr.x)) * ooW + 1.0f;
  float right = (v.x + NTempest::CMath::fabs_(vr.x)) * ooW + 1.0f;
  float top = (v.y + NTempest::CMath::fabs_(vr.y)) * ooW;
  int   first = static_cast<int>(left * 64.0f - 0.5f);
  int   last = static_cast<int>(right * 64.0f - 0.5f) + 1;
  if (first < 0) {
    first = 0;
  }
  if (last > 127) {
    last = 127;
  }
  while (first <= last && clipBuffer[first] >= top) {
    ++first;
  }
  return first > last;
}

int CWorldScene::ClipBufferCull(const NTempest::CAaBox &aaBox, unsigned int cullFlags) {
  NTempest::C3Vector  aaBoxMin = aaBox.b;
  NTempest::C3Vector  aaBoxMax = aaBox.t;
  NTempest::C3Vector *aaBoxMinMax[2] = {&aaBoxMin, &aaBoxMax};
  if (!(CWorld::enables & CWorld::Enable_Culling)) {
    return 0;
  }

  float minX = 3.4028235e38f;
  float maxX = -3.4028235e38f;
  float maxY = -3.4028235e38f;
  for (unsigned int i = 0; i < 8; ++i) {
    NTempest::C3Vector corner(aaBoxMinMax[(i >> 0) & 1]->x, aaBoxMinMax[(i >> 1) & 1]->y, aaBoxMinMax[(i >> 2) & 1]->z);
    NTempest::C4Vector v(corner.x, corner.y, corner.z, 1.0f);
    v = v * mvp;
    if (!(cullFlags & 8) && v.w < 50.0f) {
      return 0;
    }
    float ooW = 1.0f / v.w;
    float x = v.x * ooW;
    float y = v.y * ooW;
    if (x < minX)
      minX = x;
    if (x > maxX)
      maxX = x;
    if (y > maxY)
      maxY = y;
  }

  int first = static_cast<int>((minX + 1.0f) * 64.0f - 0.5f);
  int last = static_cast<int>((maxX + 1.0f) * 64.0f - 0.5f) + 1;
  if (first < 0)
    first = 0;
  if (last > 127)
    last = 127;
  while (first <= last && clipBuffer[first] >= maxY) {
    ++first;
  }
  return first > last;
}

void CWorldScene::RenderMapObjDefGroups() {
  NTempest::C44Matrix mapObjM;
  CMapObjDefGroup    *mapObjDefGroupnext_node;
  NTempest::C44Matrix gxWm;
  CMapObjDefGroup    *mapObjDefGroup;

  FrustumPush();
  mapObjM = NTempest::C44Matrix();

  mapObjDefGroup = sortTable.visMapObjDefGroupList.Head();
  while (mapObjDefGroup) {
    mapObjDefGroupnext_node = sortTable.visMapObjDefGroupList.Next(mapObjDefGroup);
    sortTable.visMapObjDefGroupList.UnlinkNode(mapObjDefGroup);

    if (CWorld::enables & CWorld::Enable_MapObjs) {
      CMapBaseObjLink *parentLink = mapObjDefGroup->parentLinkList.Head();
      FATALASSERT(parentLink);
      CMapObjDef *mapObjDef = static_cast<CMapObjDef *>(parentLink->ref);
      FATALASSERT(mapObjDef);

      mapObjM = NTempest::C44Matrix();
      mapObjM.Translate(-camPos);
      gxWm = mapObjDef->mat * mapObjM;
      GxXformSet(GxXform_World, gxWm);

      mapObjDefGroup->SelectLights();
      FATALASSERT(mapObjDef->mapObj);
      mapObjDef->mapObj->RenderGroup(
          mapObjDefGroup->groupNum, mapObjDefGroup->rDrawSharedLiquidToggle, mapObjDef->invMat, mapObjDefGroup->frustumList
      );
    }

    CWFrustum *frustum = mapObjDefGroup->frustumList.Head();
    while (frustum) {
      CWFrustum *next = mapObjDefGroup->frustumList.Next(frustum);
      mapObjDefGroup->frustumList.UnlinkNode(frustum);
      FreeFrustum(frustum);
      frustum = next;
    }

    mapObjDefGroup = mapObjDefGroupnext_node;
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
    GxRsSet(GxRs_MatSpecularExp, 6.0f);
  } else {
    GxRsSet(GxRs_TexBlend1, GxTexBlend_Add);
  }

  CChunkLiquid *liquid = sortTable.visLiquidList[1].Head();
  while (liquid) {
    CChunkLiquid *next = sortTable.visLiquidList[1].Next(liquid);
    sortTable.visLiquidList[1].UnlinkNode(liquid);

    if (CWorld::enables & CWorld::Enable_Water) {
      GxXformIdentity(GxXform_World);
      GxXformTranslate(GxXform_World, liquid->chunk->corner - camPos);
      CMap::SelectLight(liquid->chunk);
      liquid->Render(1);
    }

    liquid = next;
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
    GxRsSet(GxRs_MatSpecularExp, 6.0f);
  } else {
    GxRsSet(GxRs_TexBlend1, GxTexBlend_Add);
  }

  CChunkLiquid *liquid = sortTable.visLiquidList[0].Head();
  while (liquid) {
    CChunkLiquid *next = sortTable.visLiquidList[0].Next(liquid);
    sortTable.visLiquidList[0].UnlinkNode(liquid);

    if (CWorld::enables & CWorld::Enable_Water) {
      GxXformIdentity(GxXform_World);
      GxXformTranslate(GxXform_World, liquid->chunk->corner - camPos);
      CMap::SelectLight(liquid->chunk);
      liquid->Render(0);
    }

    liquid = next;
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

  CChunkLiquid *liquid = sortTable.visLiquidList[2].Head();
  while (liquid) {
    CChunkLiquid *next = sortTable.visLiquidList[2].Next(liquid);
    sortTable.visLiquidList[2].UnlinkNode(liquid);

    if (CWorld::enables & CWorld::Enable_Water) {
      GxXformIdentity(GxXform_World);
      GxXformTranslate(GxXform_World, liquid->chunk->corner - camPos);
      CMap::SelectLight(liquid->chunk);
      liquid->Render(2);
    }

    liquid = next;
  }

  GxRsPop();
}

void CWorldScene::CullMapObjDefs(CSortEntry *sortEntry, const NTempest::CRect &sRect) {
  NTempest::C44Matrix mapObjM;
  CMapObj            *mapObj;
  CMapObjDef         *mapObjDefnext_node;
  NTempest::C44Matrix gxWm;

  CMapObjDef *mapObjDef = sortEntry->mapObjDefList.Head();
  while (mapObjDef) {
    mapObjDefnext_node = sortEntry->mapObjDefList.Next(mapObjDef);
    mapObj = mapObjDef->mapObj;
    FATALASSERT(mapObj);

    if (!FrustumCull(mapObjDef->aaBox) && !ClipBufferCull(mapObjDef->aaBox, 1)) {
      mapObjM = NTempest::C44Matrix();
      mapObjM.Translate(-camPos);
      gxWm = mapObjDef->mat * mapObjM;
      GxXformSet(GxXform_World, gxWm);

      CMapObj::localCamPos = camPos * mapObjDef->invMat;
      CMapObj::SetGroupRenderCallback(CullMapObjDefGroup, mapObjDef);
      CMapObj::curMapObjDef = mapObjDef;
      mapObj->ExtRender(mapObjDef->mat, sRect);
      sortEntry->mapObjDefList.UnlinkNode(mapObjDef);
    }

    mapObjDef = mapObjDefnext_node;
  }
}

void CWorldScene::CullMapObjDef(CMapObjDef *mapObjDef, TSGrowableArray<unsigned int> &inGroups) {
  NTempest::C44Matrix mapObjM;
  CMapObj            *mapObj;
  NTempest::C44Matrix gxWm;

  FATALASSERT(mapObjDef);
  FATALASSERT(inGroups.Count() != 0);
  mapObj = mapObjDef->mapObj;
  FATALASSERT(mapObj);

  mapObjM = NTempest::C44Matrix();
  mapObjM.Translate(-camPos);
  gxWm = mapObjDef->mat * mapObjM;
  GxXformSet(GxXform_World, gxWm);

  CMapObj::localCamPos = camPos * mapObjDef->invMat;
  CMapObj::SetGroupRenderCallback(CullMapObjDefGroup, mapObjDef);
  CMapObj::curMapObjDef = mapObjDef;
  mapObj->IntRender(mapObjDef->mat, inGroups);
  mapObjDef->sceneLink.Unlink();
}

void CWorldScene::CullMapObjDefGroup(const unsigned int groupNum, const void *userParam, const int rDrawSharedLiquidToggle) {
  CMapObjDef      *mapObjDef = const_cast<CMapObjDef *>(static_cast<const CMapObjDef *>(userParam));
  FATALASSERT(mapObjDef);

  CMapObjDefGroup *mapObjDefGroup = 0;
  {
    ITERATELIST(CMapBaseObjLink, mapObjDef->groupLinkList, groupLink) {
      mapObjDefGroup = static_cast<CMapObjDefGroup *>(groupLink->owner);
      if (mapObjDefGroup->groupNum == groupNum) {
        break;
      }
      mapObjDefGroup = 0;
    }
  }
  FATALASSERT(mapObjDefGroup);

  if (mapObjDefGroup->sceneLink.IsLinked()) {
    CWFrustum *frustum = AllocFrustum();
    memcpy(frustum, &FrustumGet(), sizeof(*frustum) - sizeof(frustum->sceneLink));
    mapObjDefGroup->frustumList.LinkNode(frustum, LIST_TAIL, 0);
  } else {
    sortTable.visMapObjDefGroupList.LinkNode(mapObjDefGroup, LIST_TAIL, 0);
    mapObjDefGroup->ambient = mapObjDef->ambient;
    mapObjDefGroup->rDrawSharedLiquidToggle = rDrawSharedLiquidToggle;
    FATALASSERT(mapObjDefGroup->frustumList.Head() == 0);
    CWFrustum *frustum = AllocFrustum();
    memcpy(frustum, &FrustumGet(), sizeof(*frustum) - sizeof(frustum->sceneLink));
    mapObjDefGroup->frustumList.LinkNode(frustum, LIST_TAIL, 0);
  }

  if (CWorld::enables & 0x400081) {
    CullDoodads(mapObjDefGroup->doodadDefLinkList);
  }

  {
    ITERATELIST(CMapBaseObjLink, mapObjDefGroup->entityLinkList, entityLink) {
      CMapEntity *entity = static_cast<CMapEntity *>(entityLink->owner);
      if (!entity->flagVisible) {
        entity->camDist = camPlaneXY.DistSigned(entity->aaSphere.c) - entity->aaSphere.r;
        if (entity->camDist <= CWorld::unitDrawDist && !FrustumCull(entity->aaSphere.c, entity->aaSphere.r)) {
          entity->flagVisible = 1;
          entity->sceneLink.Unlink();
          sortTable.visEntityList.LinkNode(entity, LIST_TAIL, 0);
        }
      }
    }
  }
}

void CWorldScene::ClipBufferClear() {
  for (unsigned int i = 0; i < 128; ++i) {
    clipBuffer[i] = -1.0f;
  }
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
  GxuXformCreateProjection(p_fovy * 0.017453292f, p_aspect, p_minz, p_maxz, projMat);
  GxuXformCalcFrustumCorners(viewMat, projMat, corners);
  CalcPlanesFromCorners();
}

CWFrustum::CWFrustum(const NTempest::C3Vector *c) {
  CalcPlanesFromCorners(c);
}

void CWFrustum::CalcPlanesFromCorners(const NTempest::C3Vector *c) {
  for (unsigned int i = 0; i < 8; ++i) {
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
  for (unsigned int i = 0; i < 8; ++i) {
    corners[i] = corners[i] + t;
  }
  for (i = 0; i < 6; ++i) {
    planes[i].Translate(t);
  }
  lookPos = lookPos + t;
  lookAt = lookAt + t;
}

void CWFrustum::Transform(const NTempest::C44Matrix &mat) {
  for (unsigned int i = 0; i < 8; ++i) {
    corners[i] = corners[i] * mat;
  }
  CalcPlanesFromCorners();
  lookPos = lookPos * mat;
  lookAt = lookAt * mat;
  lookUp = lookUp * mat;
}

WorldCullStatus CWFrustum::Cull(const NTempest::CAaBox &aabox) const {
  const float *corner[2] = {&aabox.t.x, &aabox.b.x};
  for (unsigned int p = 0; p < 6; ++p) {
    NTempest::C3Vector point(corner[planes[p].n.x < 0.0f][0], corner[planes[p].n.y < 0.0f][1], corner[planes[p].n.z < 0.0f][2]);
    if (planes[p].DistSigned(point) < -0.019444443f) {
      return WorldCull_outside;
    }
  }
  return WorldCull_notOutside;
}

WorldCullStatus CWFrustum::Cull(const NTempest::CAaBox &box, NTempest::C33Matrix &basis, NTempest::C3Vector &pos) {
  NTempest::C33Matrix m = basis.Transpose();
  for (int p = 0; p < 6; ++p) {
    NTempest::C3Vector wv = m * planes[p].n;
    NTempest::C3Vector corner(wv.x < 0.0f ? box.b.x : box.t.x, wv.y < 0.0f ? box.b.y : box.t.y, wv.z < 0.0f ? box.b.z : box.t.z);
    corner = basis * corner + pos;
    if (planes[p].DistSigned(corner) < -0.019444443f) {
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
  for (unsigned int p = 0; p < 6; ++p) {
    if (planes[p].DistSigned(sphere.c) < -sphere.r) {
      return WorldCull_outside;
    }
  }
  return WorldCull_notOutside;
}

WorldCullStatus CWFrustum::Cull(const NTempest::C3Vector &point) const {
  for (unsigned int p = 0; p < 6; ++p) {
    if (planes[p].DistSigned(point) < -0.019444443f) {
      return WorldCull_outside;
    }
  }
  return WorldCull_notOutside;
}

void CWFrustum::Cull(const NTempest::C3Vector &point, unsigned int &cullFlags) const {
  cullFlags = 0;
  for (unsigned int p = 0; p < 6; ++p) {
    if (planes[p].DistSigned(point) < -0.019444443f) {
      cullFlags |= 1 << p;
    }
  }
}

WorldCullStatus CWFrustum::Cull(const NTempest::C4Plane &plane) const {
  unsigned int outside = 0;
  unsigned int inside = 0;
  for (unsigned int i = 0; i < 8; ++i) {
    float distance = plane.DistSigned(corners[i]);
    if (distance > 0.019444443f) {
      ++inside;
    } else if (distance < -0.019444443f) {
      ++outside;
    } else {
      return WorldCull_intersect;
    }
  }
  if (!inside) {
    return WorldCull_outside;
  }
  return outside ? WorldCull_intersect : WorldCull_inside;
}

struct ClipInfo {
  float        bc[6];
  unsigned int mask;
  unsigned int filler;

  void Set(const NTempest::C3Vector *v);
};

void ClipInfo::Set(const NTempest::C3Vector *v) {
  bc[0] = v->x;
  bc[1] = 1.0f - v->x;
  bc[2] = v->y;
  bc[3] = 1.0f - v->y;
  bc[4] = v->z;
  bc[5] = 1.0f - v->z;
  mask = 0;
  for (unsigned int i = 0; i < 6; ++i) {
    if (bc[i] < 0.0f) {
      mask |= 0x80000000 >> i;
    }
  }
}

struct ClipFrame {
  NTempest::C3Vector **points;
  ClipInfo           **info;
  unsigned int         count;

  ClipFrame(NTempest::C3Vector **points, ClipInfo **info, unsigned int count) : points(points), info(info), count(count) {
  }
};

static NTempest::C3Vector  sPointPool[32];
static NTempest::C3Vector *sInPointPtrs[32];
static NTempest::C3Vector *sOutPointPtrs[32];

int CWorld::NDCClip(NTempest::C3Vector *p_inVerts, unsigned int p_inCount, NTempest::C3Vector **&p_outVerts, unsigned int &p_outCount) {
  ClipInfo  sInInfo[32];
  ClipInfo  infoPool[32];
  ClipInfo *inInfoPtrs[32];
  ClipInfo *outInfoPtrs[32];

  FATALASSERT(p_inCount < 32);
  if (!p_inCount) {
    return 0;
  }

  unsigned int andMask = 0xFFFFFFFF;
  unsigned int orMask = 0;
  for (unsigned int i = 0; i < p_inCount; ++i) {
    sInInfo[i].Set(&p_inVerts[i]);
    inInfoPtrs[i] = &sInInfo[i];
    sInPointPtrs[i] = &p_inVerts[i];
    andMask &= sInInfo[i].mask;
    orMask |= sInInfo[i].mask;
  }

  if (andMask) {
    return 0;
  }
  if (!orMask) {
    p_outVerts = sInPointPtrs;
    p_outCount = p_inCount;
    return 1;
  }

  ClipFrame           inFrame(sInPointPtrs, inInfoPtrs, p_inCount);
  ClipFrame           outFrame(sOutPointPtrs, outInfoPtrs, 0);
  ClipFrame          *in = &inFrame;
  ClipFrame          *out = &outFrame;
  NTempest::C3Vector *pointPool = sPointPool;
  ClipInfo           *sInfoPool = infoPool;

  unsigned int planeMask = 0x80000000;
  for (unsigned int plane = 0; plane < 6; ++plane, planeMask >>= 1) {
    if (!(orMask & planeMask)) {
      continue;
    }

    out->count = 0;
    unsigned int from = in->count - 1;
    unsigned int fromMask = in->info[from]->mask & planeMask;
    for (unsigned int to = 0; to < in->count; ++to) {
      unsigned int toMask = in->info[to]->mask & planeMask;
      if (fromMask != toMask) {
        float denominator = in->info[from]->bc[plane] - in->info[to]->bc[plane];
        if (denominator == 0.0f) {
          denominator = 0.0001f;
        }
        float t = in->info[from]->bc[plane] / denominator;
        *pointPool = *in->points[from] + (*in->points[to] - *in->points[from]) * t;
        sInfoPool->Set(pointPool);
        out->points[out->count] = pointPool++;
        out->info[out->count++] = sInfoPool++;
      }
      if (!toMask) {
        out->points[out->count] = in->points[to];
        out->info[out->count++] = in->info[to];
      }

      FATALASSERT(out->count < 32);
      FATALASSERT(pointPool - sPointPool < 32);
      from = to;
      fromMask = toMask;
    }

    if (!out->count) {
      return 0;
    }

    ClipFrame *temp = in;
    in = out;
    out = temp;
  }

  p_outVerts = in->points;
  p_outCount = in->count;
  return 1;
}

bool CWorld::NDCXform(const CWFrustum &frustum, NTempest::C44Matrix &xf, bool translate) {
  NTempest::C3Vector forward = frustum.corners[3] - frustum.corners[0];
  NTempest::C3Vector up = frustum.corners[1] - frustum.corners[0];
  NTempest::C3Vector right = frustum.corners[4] - frustum.corners[0];

  xf = NTempest::C44Matrix(
      forward.x, forward.y, forward.z, 0.0f, up.x, up.y, up.z, 0.0f, right.x, right.y, right.z, 0.0f, translate ? frustum.corners[0].x : 0.0f,
      translate ? frustum.corners[0].y : 0.0f, translate ? frustum.corners[0].z : 0.0f, 1.0f
  );

  float det = xf.Determinant();
  if (NTempest::CMath::fabs_(det) < 0.00000023841858f) {
    xf = NTempest::C44Matrix();
    return 0;
  }

  xf = xf.Inverse(det);
  return 1;
}
