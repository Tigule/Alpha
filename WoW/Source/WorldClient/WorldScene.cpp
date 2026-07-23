#include "World.h"
#include "CMapObj.h"
#include "DetailDoodad.h"
#include "Ui/WorldFrame.h"

#include "DayNight.h"
#include "Gx/Gx.h"
#include "Model/IModel.h"
#include "Tempest/c33matrix.h"
#include "Tempest/c34matrix.h"
#include "Tempest/cmath.h"

#include <new>
#include <string.h>
#include <typeinfo>

void __fastcall ShadowRender(HMODEL hModel, NTempest::C44Matrix &basis, void *param);

TSExplicitList<CWFrustum, 0xF4> CWorldScene::frustumFreeList;
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
    CMapChunk *chunk;
    while ((chunk = table[index].chunkList.Head())) {
      table[index].chunkList.UnlinkNode(chunk);
    }

    for (unsigned int type = 0; type < 4; ++type) {
      CChunkLiquid *liquid;
      while ((liquid = table[index].liquidList[type].Head())) {
        table[index].liquidList[type].UnlinkNode(liquid);
      }
    }

    CMapDoodadDef *doodadDef;
    while ((doodadDef = table[index].doodadDefList.Head())) {
      table[index].doodadDefList.UnlinkNode(doodadDef);
    }

    CMapObjDef *mapObjDef;
    while ((mapObjDef = table[index].mapObjDefList.Head())) {
      table[index].mapObjDefList.UnlinkNode(mapObjDef);
    }

    CMapEntity *entity;
    while ((entity = table[index].entityList.Head())) {
      entity->flagVisible = 0;
      table[index].entityList.UnlinkNode(entity);
      nonVisEntityList.LinkNode(entity, LIST_TAIL, 0);
    }
  }
}

void __fastcall CWorldScene::Initialize() {
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

void __fastcall CWorldScene::Destroy() {
  while (CWFrustum *frustum = frustumFreeList.Head()) {
    frustumFreeList.UnlinkNode(frustum);
    frustum->~CWFrustum();
    SMemFree(frustum, typeid(CWFrustum).raw_name(), -2, 0);
  }
}

CWFrustum *__fastcall CWorldScene::AllocFrustum() {
  CWFrustum *frustum = frustumFreeList.Head();
  if (!frustum) {
    void *storage = SMemAlloc(sizeof(CWFrustum), typeid(CWFrustum).raw_name(), -2, 8);
    frustum = storage ? new (storage) CWFrustum : 0;
    frustumFreeList.LinkNode(frustum, LIST_TAIL, 0);
    FATALASSERT(frustum);
  }

  frustumFreeList.UnlinkNode(frustum);
  return frustum;
}

void __fastcall CWorldScene::FreeFrustum(CWFrustum *frustum) {
  FATALASSERT(frustum);
  frustum->sceneLink.Unlink();
  frustumFreeList.LinkNode(frustum, LIST_TAIL, 0);
}

void __fastcall CWorldScene::PrepareRenderLiquid() {
  NTempest::C3Vector lqDir(0.0f, 0.0f, 0.0f);
  NTempest::C3Vector camQueryPos = camPos;
  float              lqSurface;
  unsigned int       newLiquid = 15;

  for (unsigned int i = 0; i < 4; ++i) {
    if (camFrustumCorners[i].z < camQueryPos.z) {
      camQueryPos.z = camFrustumCorners[i].z;
    }
  }

  if (camMapObjDef && camMapObj && camMapObjGroup) {
    NTempest::C3Vector localPos = camQueryPos * camMapObjDef->invMat;
    camMapObjGroup->QueryLiquidStatus(localPos, newLiquid, lqSurface, lqDir);
  } else {
    CWorld::QueryLiquidStatus(camQueryPos, newLiquid, lqSurface, lqDir);
  }

  camLiquid = newLiquid;
}

void __fastcall CWorldScene::PrepareRender(NTempest::C3Vector &position, NTempest::C3Vector &target) {
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
  NTempest::C3Vector translation = -camPos;
  mv.Translate(translation);
  mvp = mv * mp;
  mvpCol3 = NTempest::C4Vector(mvp.a3, mvp.b3, mvp.c3, mvp.d3);
  CalcFrustumCorners(camFrustumCorners);
  camFrustumBounds = NTempest::CAaBox::Bounding(camFrustumCorners, 8);
  frustumIndex = 0;
  PrepareRenderLiquid();
}

void __fastcall CWorldScene::Update() {
}

void __fastcall CWorldScene::Render() {
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
  LocateViewer();

  if (viewerMapObjDef) {
    ClipBufferClear();
    CullMapObjDef(viewerMapObjDef, viewerMapObjGroups);
    for (unsigned int i = 0; i < CMapObj::extViewList.Count(); ++i) {
      ClipBufferClear();
      CullSortTable(CMapObj::extViewList[i]);
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
}

void __fastcall CWorldScene::RenderAlpha() {
  NTempest::C44Matrix cMat;

  GxRsPush();
  GxXformPush(GxXform_World);
  GxRsSet(GxRs_TexLodBias0, 0.5f);

  CMapChunk *chunk = sortTable.visChunkList.Head();
  while (chunk) {
    CMapChunk *next = sortTable.visChunkList.Next(chunk);
    if (chunk->detailDoodadInst) {
      cMat = NTempest::C44Matrix();
      cMat.Translate(chunk->corner - camPos);
      GxXformSet(GxXform_World, cMat);
      CMap::SelectLight(chunk);
      chunk->detailDoodadInst->RenderAlpha();
    }
    sortTable.visChunkList.UnlinkNode(chunk);
    chunk = next;
  }

  GxRsSet(GxRs_TexLodBias0, -(CWorld::texLodBias - 0.5f));
  GxXformPop(GxXform_World);
  GxRsPop();
}

void __fastcall CWorldScene::AddDoodadDef(CMapDoodadDef *doodadDef) {
  NTempest::CAaSphere bounds;
  FATALASSERT(doodadDef);
  doodadDef->GetBounds(bounds);
  doodadDef->camDist = camPlaneXY.DistSigned(bounds.c) - bounds.r;
  int sortIndex = static_cast<int>(doodadDef->camDist * 0.03f - 0.5f);
  if (sortIndex < 0) {
    sortIndex = 0;
  } else if (sortIndex >= 26) {
    FATALERROR(("DOODADDEFTOOFARTOSORT"));
  }
  sortTable.table[sortIndex].doodadDefList.LinkNode(doodadDef, LIST_TAIL, 0);
}

void __fastcall CWorldScene::AddMapObjDef(CMapObjDef *mapObjDef) {
  FATALASSERT(mapObjDef);
  mapObjDef->camDist = camPlaneXY.DistSigned(mapObjDef->aaSphere.c) - mapObjDef->aaSphere.r;
  int sortIndex = static_cast<int>(mapObjDef->camDist * 0.03f - 0.5f);
  if (sortIndex < 0) {
    sortIndex = 0;
  } else if (sortIndex >= 26) {
    FATALERROR(("MAPOBJDEFTOOFARTOSORT"));
  }
  sortTable.table[sortIndex].mapObjDefList.LinkNode(mapObjDef, LIST_TAIL, 0);
}

void __fastcall CWorldScene::AddMapChunk(CMapChunk *chunk, float sortDist) {
  FATALASSERT(chunk);

  int sortIndex = static_cast<int>(sortDist * 0.03f - 0.5f);
  if (sortIndex < 0) {
    sortIndex = 0;
  } else if (sortIndex >= 26) {
    FATALERROR(("CHUNKDISTTOOFARTOSORT"));
  }

  sortTable.table[sortIndex].chunkList.LinkNode(chunk, LIST_TAIL, 0);
}

void __fastcall CWorldScene::AddChunkLiquid(CChunkLiquid *liquid, unsigned int type) {
  FATALASSERT(liquid);
  FATALASSERT(type < 4);
  int sortIndex = static_cast<int>(liquid->chunk->camDist * 0.03f - 0.5f);
  if (sortIndex < 0) {
    sortIndex = 0;
  } else if (sortIndex >= 26) {
    FATALERROR(("CHUNKDISTTOOFARTOSORT"));
  }
  sortTable.table[sortIndex].liquidList[type].LinkNode(liquid, LIST_TAIL, 0);
}

void __fastcall CWorldScene::AddMapEntity(CMapEntity *entity) {
  FATALASSERT(entity);
  entity->camDist = camPlaneXY.DistSigned(entity->aaSphere.c) - entity->aaSphere.r;
  int sortIndex = static_cast<int>(entity->camDist * 0.03f - 0.5f);
  if (sortIndex < 0) {
    sortIndex = 0;
  } else if (sortIndex >= 26) {
    FATALERROR(("ENTITYDISTTOOFARTOSORT"));
  }
  sortTable.table[sortIndex].entityList.LinkNode(entity, LIST_TAIL, 0);
}

void __fastcall CWorldScene::ClipBufferUpdate(NTempest::C3Vector *vertices, const int *indicies, int nVertices, NTempest::C3Vector &corner) {
  FATALASSERT(vertices);
  FATALASSERT(indicies);
  int i;

  if (!(CWorld::enables & CWorld::Enable_Culling)) {
    return;
  }

  for (i = 0; i < nVertices; ++i) {
    NTempest::C3Vector vertex = vertices[indicies[i]] + corner;
    clipVertexBuffer[i] = NTempest::C4Vector(vertex.x, vertex.y, vertex.z, 1.0f) * mvp;
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

void __fastcall CWorldScene::ClipPortal(NTempest::C4Vector *inList, unsigned int &inCount) {
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
      float               d0 = plane.n.x * v0->x + plane.n.y * v0->y + plane.n.z * v0->z;
      float               d1 = plane.n.x * v1->x + plane.n.y * v1->y + plane.n.z * v1->z;
      int                 side0 = d0 > 0.019444443f ? 1 : (d0 < -0.019444443f ? 2 : 0);
      int                 side1 = d1 > 0.019444443f ? 1 : (d1 < -0.019444443f ? 2 : 0);

      if (side0 != 2) {
        v[to][c[to]++] = *v0;
      }
      if (side0 && side1 && side0 != side1) {
        float               t = d0 / (d0 - d1);
        NTempest::C4Vector &out = v[to][c[to]++];
        out.x = v0->x + (v1->x - v0->x) * t;
        out.y = v0->y + (v1->y - v0->y) * t;
        out.z = v0->z + (v1->z - v0->z) * t;
        out.w = v0->w + (v1->w - v0->w) * t;
      }
    }
  }

  inCount = c[0];
}

void __fastcall CWorldScene::CalcFrustumCorners(NTempest::C3Vector *corners) {
  NTempest::C44Matrix lMp;
  NTempest::C44Matrix lMv;
  GxXformView(lMv);
  GxXformProjection(lMp);

  NTempest::C3Vector translation = -camPos;
  lMv.Translate(translation);
  GxuXformCalcFrustumCorners(lMv, lMp, corners);
}

void __fastcall CWorldScene::LocateViewer() {
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

void __fastcall CWorldScene::FrustumPush() {
  FATALASSERT(frustumIndex < 15);
  frustumStack[frustumIndex + 1] = frustumStack[frustumIndex];
  ++frustumIndex;
}

void __fastcall CWorldScene::FrustumSet(NTempest::CRect &sRect) {
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
    td = camFrustumCorners[i + 2] - camFrustumCorners[i + 1];
    tl = camFrustumCorners[i + 1] + td * sRect.l;
    tr = camFrustumCorners[i + 1] + td * sRect.r;
    bd = camFrustumCorners[i + 3] - camFrustumCorners[i];
    bl = camFrustumCorners[i] + bd * sRect.l;
    br = camFrustumCorners[i] + bd * sRect.r;
    ld = bl - tl;
    rd = br - tr;
    newCorners[i] = tl + ld * sRect.b;
    newCorners[i + 1] = tl + ld * sRect.t;
    newCorners[i + 2] = tr + rd * sRect.t;
    newCorners[i + 3] = tr + rd * sRect.b;
  }

  FrustumGet().CalcPlanesFromCorners(newCorners);
}

void __fastcall CWorldScene::FrustumSet(NTempest::C3Vector *corners) {
  FrustumGet().CalcPlanesFromCorners(corners);
}

void __fastcall CWorldScene::FrustumSet(NTempest::C3Vector *corners, NTempest::CRect &sRect) {
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
    ld = bl - tl;
    rd = br - tr;
    a = tl + ld * sRect.t;
    b = tr + rd * sRect.t;
    newCorners[i] = tl + ld * sRect.b;
    newCorners[i + 1] = a;
    newCorners[i + 2] = b;
    newCorners[i + 3] = tr + rd * sRect.b;
  }

  n = NTempest::C3Vector::Cross(newCorners[1] - newCorners[0], newCorners[4] - newCorners[0]);
  FrustumGet().CalcPlanesFromCorners(newCorners);
}

void __fastcall CWorldScene::FrustumSet(CWFrustum &frustum) {
  FrustumGet() = frustum;
}

CWFrustum &__fastcall CWorldScene::FrustumGet() {
  return frustumStack[frustumIndex];
}

void __fastcall CWorldScene::FrustumXform(NTempest::C44Matrix &mat) {
  FrustumGet().Transform(mat);
}

int __fastcall CWorldScene::FrustumCull(NTempest::C3Vector &center, float radius) {
  return FrustumGet().Cull(center, radius) == WorldCull_outside;
}

int __fastcall CWorldScene::FrustumCull(NTempest::CAaBox &aaBox) {
  return FrustumGet().Cull(aaBox) == WorldCull_outside;
}

int __fastcall CWorldScene::FrustumCull(NTempest::CAaBox &aaBox, NTempest::C33Matrix &basis, NTempest::C3Vector &pos) {
  return FrustumGet().Cull(aaBox, basis, pos) == WorldCull_outside;
}

void __fastcall CWorldScene::FrustumPop() {
  FATALASSERT(frustumIndex > 0);
  --frustumIndex;
}

void __fastcall CWorldScene::CullSortTable(NTempest::CRect &sRect) {
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

void __fastcall CWorldScene::CullHorizon(NTempest::CRect &sRect) {
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

void __fastcall CWorldScene::CullEntitys(CSortEntry *sortEntry) {
  FATALASSERT(sortEntry);
  CMapEntity *entity = sortEntry->entityList.Head();
  while (entity) {
    CMapEntity *next = sortEntry->entityList.Next(entity);
    if (entity->camDist <= cullDistance && !FrustumCull(entity->aaSphere.c, entity->aaSphere.r) &&
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

void __fastcall CWorldScene::CullDoodads(CSortEntry *sortEntry) {
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

void __fastcall CWorldScene::CullDoodads(TSExplicitList<CMapBaseObjLink, 8> &doodadDefLinkList) {
  NTempest::CAaSphere doodadSphere;
  CMapBaseObjLink    *link = doodadDefLinkList.Head();
  while (link) {
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
    link = doodadDefLinkList.Next(link);
  }
}

void __fastcall CWorldScene::CullChunkLiquid(CSortEntry *sortEntry, unsigned int type) {
  NTempest::CAaBox aaBox;
  FATALASSERT(sortEntry);
  CChunkLiquid *liquid = sortEntry->liquidList[type].Head();
  while (liquid) {
    CChunkLiquid *next = sortEntry->liquidList[type].Next(liquid);
    aaBox.b = liquid->chunk->aaBox.b;
    aaBox.t = liquid->chunk->aaBox.t;
    aaBox.b.z = liquid->height.min;
    aaBox.t.z = liquid->height.max;
    if (!FrustumCull(aaBox) && !ClipBufferCull(aaBox, 0)) {
      sortEntry->liquidList[type].UnlinkNode(liquid);
      sortTable.visLiquidList[type].LinkNode(liquid, LIST_TAIL, 0);
    }
    liquid = next;
  }
}

void __fastcall CWorldScene::CullChunks(CSortEntry *sortEntry) {
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

void __fastcall CWorldScene::RenderObjects() {
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

void __fastcall CWorldScene::RenderDoodads() {
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

        NTempest::C34Matrix orientation(
            transform.a0, transform.a1, transform.a2, transform.b0, transform.b1, transform.b2, transform.c0, transform.c1, transform.c2,
            transform.d0, transform.d1, transform.d2
        );
        NTempest::C3Vector cameraVector = camTarg - camPos;
        ModelAnimate(doodadDef->model, orientation, doodadDef->scale, camPos, cameraVector);

        transform.d0 += camPos.x;
        transform.d1 += camPos.y;
        transform.d2 += camPos.z;
        orientation = NTempest::C34Matrix(
            transform.a0, transform.a1, transform.a2, transform.b0, transform.b1, transform.b2, transform.c0, transform.c1, transform.c2,
            transform.d0, transform.d1, transform.d2
        );
        ModelProcessEvents(doodadDef->model, orientation);
        ModelAddToScene(doodadDef->model, doodadDef->camDist <= CWorld::farFog ? 0 : 7);
      }
    }

    if (doodadDef->RenderCB) {
      doodadDef->RenderCB(doodadDef->renderCBParam, doodadDef->mat);
    }

    doodadDef = doodadDefnext_node;
  }
}

void __fastcall CWorldScene::RenderHorizon() {
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

void __fastcall CWorldScene::RenderChunks() {
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

int __fastcall CWorldScene::ClipBufferCull(NTempest::C3Vector &center, float radius, unsigned int cullFlags) {
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

int __fastcall CWorldScene::ClipBufferCull(NTempest::CAaBox &aaBox, unsigned int cullFlags) {
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

void __fastcall CWorldScene::RenderMapObjDefGroups() {
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

void __fastcall CWorldScene::RenderOcean() {
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

  NTempest::C3Vector texScale(0.11f, 0.11f, 0.11f);
  GxXformScale(GxXform_Tex1, texScale);
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

void __fastcall CWorldScene::RenderWater() {
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

  NTempest::C3Vector texScale(0.14f, 0.14f, 0.14f);
  GxXformScale(GxXform_Tex1, texScale);
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

void __fastcall CWorldScene::RenderMagma() {
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

void __fastcall CWorldScene::CullMapObjDefs(CSortEntry *sortEntry, NTempest::CRect &sRect) {
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

void __fastcall CWorldScene::CullMapObjDef(CMapObjDef *mapObjDef, TSGrowableArray<unsigned int> &inGroups) {
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

void __fastcall CWorldScene::CullMapObjDefGroup(const unsigned int groupNum, const void *userParam, const int rDrawSharedLiquidToggle) {
  CMapBaseObjLink *link;
  CMapObjDef      *mapObjDef = const_cast<CMapObjDef *>(static_cast<const CMapObjDef *>(userParam));
  FATALASSERT(mapObjDef);

  CMapObjDefGroup *mapObjDefGroup = 0;
  for (link = mapObjDef->groupLinkList.Head(); link; link = mapObjDef->groupLinkList.Next(link)) {
    mapObjDefGroup = static_cast<CMapObjDefGroup *>(link->owner);
    if (mapObjDefGroup->groupNum == groupNum) {
      break;
    }
    mapObjDefGroup = 0;
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

  link = mapObjDefGroup->entityLinkList.Head();
  while (link) {
    CMapEntity *entity = static_cast<CMapEntity *>(link->owner);
    if (!entity->flagVisible) {
      entity->camDist = camPlaneXY.DistSigned(entity->aaSphere.c) - entity->aaSphere.r;
      if (entity->camDist <= cullDistance && !FrustumCull(entity->aaSphere.c, entity->aaSphere.r)) {
        entity->flagVisible = 1;
        entity->sceneLink.Unlink();
        sortTable.visEntityList.LinkNode(entity, LIST_TAIL, 0);
      }
    }
    link = mapObjDefGroup->entityLinkList.Next(link);
  }
}

void __fastcall CWorldScene::ClipBufferClear() {
  for (unsigned int i = 0; i < 128; ++i) {
    clipBuffer[i] = -1.0f;
  }
}

CWFrustum::CWFrustum() {
}

CWFrustum::CWFrustum(
    NTempest::C3Vector &lPos,
    NTempest::C3Vector &lAt,
    NTempest::C3Vector &lUp,
    float               p_fovy,
    float               p_aspect,
    float               p_minz,
    float               p_maxz
)
    : lookPos(lPos), lookAt(lAt), lookUp(lUp), fovy(p_fovy), aspect(p_aspect), minz(p_minz), maxz(p_maxz) {
  NTempest::C44Matrix viewMat;
  NTempest::C44Matrix projMat;
  GxuXformCreateLookAtSgCompat(lPos, lAt, lUp, viewMat);
  GxuXformCreateProjection(p_fovy * 0.017453292f, p_aspect, p_minz, p_maxz, projMat);
  GxuXformCalcFrustumCorners(viewMat, projMat, corners);
  CalcPlanesFromCorners();
}

CWFrustum::CWFrustum(NTempest::C3Vector *c) {
  CalcPlanesFromCorners(c);
}

void CWFrustum::CalcPlanesFromCorners(NTempest::C3Vector *c) {
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

NTempest::C3Vector *CWFrustum::Corners() {
  return corners;
}

NTempest::C3Vector &CWFrustum::Corner(unsigned int index) {
  FATALASSERT(index < 8);
  return corners[index];
}

NTempest::C4Plane &CWFrustum::Plane(unsigned int index) {
  FATALASSERT(index < 6);
  return planes[index];
}

void CWFrustum::Translate(NTempest::C3Vector &t) {
  for (unsigned int i = 0; i < 8; ++i) {
    corners[i] = corners[i] + t;
  }
  for (i = 0; i < 6; ++i) {
    planes[i].Translate(t);
  }
  lookPos = lookPos + t;
  lookAt = lookAt + t;
}

void CWFrustum::Transform(NTempest::C44Matrix &mat) {
  for (unsigned int i = 0; i < 8; ++i) {
    corners[i] = corners[i] * mat;
  }
  CalcPlanesFromCorners();
  lookPos = lookPos * mat;
  lookAt = lookAt * mat;
  lookUp = lookUp * mat;
}

WorldCullStatus CWFrustum::Cull(NTempest::CAaBox &aabox) {
  float *corner[2] = {&aabox.t.x, &aabox.b.x};
  for (unsigned int p = 0; p < 6; ++p) {
    NTempest::C3Vector point(corner[planes[p].n.x < 0.0f][0], corner[planes[p].n.y < 0.0f][1], corner[planes[p].n.z < 0.0f][2]);
    if (planes[p].DistSigned(point) < -0.019444443f) {
      return WorldCull_outside;
    }
  }
  return WorldCull_notOutside;
}

WorldCullStatus CWFrustum::Cull(NTempest::CAaBox &box, NTempest::C33Matrix &basis, NTempest::C3Vector &pos) {
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

WorldCullStatus CWFrustum::Cull(NTempest::C3Vector &center, float radius) {
  WorldCullStatus result = WorldCull_inside;
  for (unsigned int p = 0; p < 6; ++p) {
    float distance = planes[p].DistSigned(center);
    if (distance < -radius) {
      return WorldCull_outside;
    }
    if (distance < radius) {
      result = WorldCull_intersect;
    }
  }
  return result;
}

WorldCullStatus CWFrustum::Cull(NTempest::CAaSphere &sphere) {
  return Cull(sphere.c, sphere.r);
}

WorldCullStatus CWFrustum::Cull(NTempest::C3Vector &point) {
  for (unsigned int p = 0; p < 6; ++p) {
    if (planes[p].DistSigned(point) < -0.019444443f) {
      return WorldCull_outside;
    }
  }
  return WorldCull_inside;
}

void CWFrustum::Cull(NTempest::C3Vector &point, unsigned int &cullFlags) {
  cullFlags = 0;
  for (unsigned int p = 0; p < 6; ++p) {
    if (planes[p].DistSigned(point) < -0.019444443f) {
      cullFlags |= 1 << p;
    }
  }
}

WorldCullStatus CWFrustum::Cull(NTempest::C4Plane &plane) {
  unsigned int counts[WorldCull_count] = {0, 0, 0, 0};
  for (unsigned int i = 0; i < 8; ++i) {
    ++counts[plane.DistSigned(corners[i]) < 0.0f ? WorldCull_outside : WorldCull_inside];
  }
  if (counts[WorldCull_outside] == 8) {
    return WorldCull_outside;
  }
  if (counts[WorldCull_inside] == 8) {
    return WorldCull_inside;
  }
  return WorldCull_intersect;
}

struct ClipInfo {
  float        bc[6];
  unsigned int mask;
  unsigned int filler;

  void Set(NTempest::C3Vector *v) {
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
};

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

int __fastcall CWorld::NDCClip(NTempest::C3Vector *p_inVerts, unsigned int p_inCount, NTempest::C3Vector **&p_outVerts, unsigned int &p_outCount) {
  ClipInfo  inInfo[32];
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
    inInfo[i].Set(&p_inVerts[i]);
    inInfoPtrs[i] = &inInfo[i];
    sInPointPtrs[i] = &p_inVerts[i];
    andMask &= inInfo[i].mask;
    orMask |= inInfo[i].mask;
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
  ClipInfo           *nextInfo = infoPool;

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
        nextInfo->Set(pointPool);
        out->points[out->count] = pointPool++;
        out->info[out->count++] = nextInfo++;
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

unsigned int __fastcall CWorld::NDCXform(const CWFrustum &frustum, NTempest::C44Matrix &xf, bool translate) {
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
