#include "Base/Base.h"
#include "Gx/Gx.h"
#include "Services/ParticleSystem2.h"
#include <WowConst.h>
#include "AaBsp.h"
#include <MapDefs.h>

#include "WorldClient/World.h"
#include "WorldClient/CMapObj.h"
#include "WorldClient/WorldParam.h"
#include "WorldClient/DetailDoodad.h"
#include "WorldClient/CSimpleDoodad.h"
#include "DayNight.h"

#include <Ftol.h>

#include "WorldCommon/WorldMath.h"

#include "Model/IModel.h"
#include "Tempest/cfacet.h"

#include <Model/CollisionData.h>
#include <string.h>

NTempest::CiRect NTempest::CiRect::Intersection(const CiRect &left, const CiRect &right) {
  return CiRect(
      left.t > right.t ? left.t : right.t, left.l > right.l ? left.l : right.l, left.b < right.b ? left.b : right.b,
      left.r < right.r ? left.r : right.r
  );
}

static const float OO_COORD_TO_SUBCHUNK = 1.0f / (150.0f / 36.0f);

static void AddDoodadFacets(const NTempest::CAaBox &aaBox, CMapDoodadDef *doodadDef, CWFacetData *facetData);
static void AddGameObjFacets(const NTempest::CAaBox &aaBox, const WorldObjCollisionHandlerData &data, DWORDLONG guid, CWFacetData *facetData);

UINT g_holeMask[4][4] = {
    {   1,    2,     4,     8},
    {  16,   32,    64,   128},
    { 256,  512,  1024,  2048},
    {4096, 8192, 16384, 32768}
};

static int iIndiciesP[4][2] = {
    {17,  0},
    { 0,  1},
    {18, 17},
    { 1, 18}
};

static WORD idxoffs[36] = {0, 1, 2, 0, 2, 3, 0, 4, 5, 0, 5, 1, 3, 7, 4, 3, 4, 0, 2, 6, 7, 2, 7, 3, 2, 6, 5, 2, 5, 1, 4, 5, 6, 4, 6, 7};

static int s_vertexIndex[4][3] = {
    {17,  9,  0},
    { 9,  1,  0},
    { 9, 17, 18},
    { 9, 18,  1}
};

static int s_vertexIndexFlat[5] = {0, 9, 17, 1, 18};

UINT            CMap::uniqueId;
bool            CMap::enablePixelShaders;
bool            CMap::enableSpecular;
bool            CMap::enableSpecularTerrain;
bool            CMap::enableTerrainShader;
bool            CMap::enableSpecularWater;
CGxPixelShader *CMap::psSpecTerrain;
CGxShaderParam *CMap::psSpecTerrain_LayerMask;
CGxPixelShader *CMap::psTerrain;
CGxShaderParam *CMap::psTerrain_LayerMask;
CGxPixelShader *CMap::psSpecUTerrain;
CGxShaderParam *CMap::psSpecUTerrain_LayerMask;
CGxPixelShader *CMap::psUTerrain;
CGxShaderParam *CMap::psUTerrain_LayerMask;
CGxBuf         *CMap::gxBufDynLowDetail;
SFile          *CMap::wdtFile;
DWORD           CMap::version;
SMMapHeader     CMap::header;
int             CMap::bActive;
int             CMap::bPreload;
int             CMap::bDungeon;
SMAreaInfo      CMap::areaInfo[4096];
CMapArea       *CMap::areaTable[4096];
DWORD           CMap::areaLowOffsets[4096];
CMapAreaLow    *CMap::areaLowTable[4096];
HASHKEY_NONE          CMap::nullHashKey;
TSGrowableArray<char> CMap::doodadNames;
TSGrowableArray<UINT> CMap::doodadNamesIndex;
TSGrowableArray<char> CMap::mapObjNames;
TSGrowableArray<UINT> CMap::mapObjNamesIndex;
TSGrowableArray<UINT> CMap::scCollideList;
UINT                  CMap::scCollideCnt;
UINT                  CMap::mapGetFacetsCount;
int (*CMap::entityHandler)(LPVOID, DWORD, DWORDLONG, DWORD);
LPVOID CMap::entityHandlerParam;
int (*CMap::entityCollisionHandler)(DWORDLONG, DWORD, WorldObjCollisionHandlerData *);
TSGrowableArray<CGxVertexPC> CMap::testQueryVerts;
TSGrowableArray<WORD>        CMap::testQueryIndices;
LISTDECLEX(CMapBaseObjLink, refLink, CMap::areaLinkList);
LISTDECLEX(CMapBaseObjLink, refLink, CMap::doodadDefLinkList);
LISTDECLEX(CMapBaseObjLink, refLink, CMap::mapObjDefLinkList);
UINT                         CMap::cCount;
UINT                         CMap::bspRecurseCount;
UINT                         CMap::nChunksPrepared;
UINT                         CMap::nGbChunksPrepared;
LPVOID                       CMap::oldSelectLightParm;
CMapLight                   *CMap::sunLight;
char                         CMap::wdtFilename[256];
char                         CMap::wobFilename[256];
char                         CMap::mapPath[256];
char                         CMap::mapName[256];

void CMap::Initialize() {
  CMapArea::Initialize();
  CMapChunk::Initialize();
  CMapObj::Initialize();
  CDetailDoodad::Initialize();
  CSimpleDoodad::Initialize();
  SetLightFuncs();

  memset(counts, 0, 11 * sizeof(counts[0]));
  memset(freeCounts, 0, 11 * sizeof(freeCounts[0]));
  memset(areaTable, 0, sizeof(areaTable));

  for (int i = 0; i < 4096; ++i) {
    areaInfo[i].offset = 0;
    areaInfo[i].size = 0;
    areaInfo[i].flags = 0;
  }

  scCollideList.SetCount(2048);
  scCollideCnt = 0;
  cCount = 0;
  uniqueId = -2;
  bDungeon = 0;
  bActive = 0;
  wdtFile = 0;
  oldSelectLightParm = 0;

  CMapLight::CreatePointAtten();
  WaterInitialize();

  GxPixelShaderCreate(psSpecTerrain, "Shaders\\Pixel\\SpecTerrain.bls");
  psSpecTerrain_LayerMask = psSpecTerrain->GetParam("layerMask");
  GxPixelShaderCreate(psTerrain, "Shaders\\Pixel\\Terrain.bls");
  psTerrain_LayerMask = psTerrain->GetParam("layerMask");
  GxPixelShaderCreate(psSpecUTerrain, "Shaders\\Pixel\\SpecUTerrain.bls");
  psSpecUTerrain_LayerMask = psSpecUTerrain->GetParam("layerMask");
  GxPixelShaderCreate(psUTerrain, "Shaders\\Pixel\\UTerrain.bls");
  psUTerrain_LayerMask = psUTerrain->GetParam("layerMask");

  gxBufDynLowDetail = GxBufCreate(GxBWF_Dynamic, GxVBF_PC, 0x221, 0xC00, GxBufDynLowDetailCallback, 0);
  ASSERT(gxBufDynLowDetail);
}

void CMap::Destroy() {
  if (bActive) {
    Unload();
  }

  FATALASSERT(areaList.Head() == 0);
  FATALASSERT(chunkList.Head() == 0);
  FATALASSERT(entityList.Head() == 0);
  FATALASSERT(mapObjDefHash.Head() == 0);
  FATALASSERT(mapObjDefGroupList.Head() == 0);
  FATALASSERT(doodadDefHash.Head() == 0);
  FATALASSERT(chunkLiquidList.Head() == 0);
  FATALASSERT(areaLinkList.Head() == 0);
  FATALASSERT(doodadDefLinkList.Head() == 0);
  FATALASSERT(mapObjDefLinkList.Head() == 0);

  CMapLight::DestroyPointAtten();
  WaterDestroy();
  CMapObj::Destroy();
  CMapChunk::Destroy();
  CMapArea::Destroy();
  CDetailDoodad::Destroy();
  CSimpleDoodad::Destroy();

  FATALASSERT(lightList.Head() == 0);

  cacheLightFreeList.Clear();
  lightFreeList.Clear();
  entityFreeList.Clear();
  baseObjLinkFreeList.Clear();
  areaFreeList.Clear();
  doodadDefFreeList.Clear();
  chunkFreeList.Clear();
  chunkLayerFreeList.Clear();
  chunkTexFreeList.Clear();
  chunkLiquidFreeList.Clear();
  soundEmitterFreeList.Clear();
  mapObjDefFreeList.Clear();
  mapObjDefGroupFreeList.Clear();
  mapObjFreeList.Clear();
  mapObjGroupFreeList.Clear();

  GxPixelShaderDestroy(psSpecTerrain);
  psSpecTerrain_LayerMask = 0;
  GxPixelShaderDestroy(psTerrain);
  psTerrain_LayerMask = 0;
  GxPixelShaderDestroy(psSpecUTerrain);
  psSpecUTerrain_LayerMask = 0;
  GxPixelShaderDestroy(psUTerrain);
  psUTerrain_LayerMask = 0;
  GxBufDestroy(gxBufDynLowDetail);
  gxBufDynLowDetail = 0;
}

void CMap::GetCounts(int counts[]) {
  memcpy(counts, CMap::counts, sizeof(CMap::counts));
  memcpy(counts + Cnt_Num, freeCounts, sizeof(freeCounts));
}

void CMap::CalcMem() {
}

DWORD CMap::GetTextureUseage() {
  UINT texUseage = 0;

  for (CMapBaseObjLink *areaLink = areaLinkList.Head(), *areaLinknext_node;
       (int)areaLink > 0 ? (areaLinknext_node = areaLinkList.RawNext(areaLink), 1) : 0; areaLink = areaLinknext_node) {
    CMapArea *area = static_cast<CMapArea *>(areaLink->owner);
    ASSERT(area);

    for (CMapBaseObjLink *chunkLink = area->chunkLinkList.Head(), *chunkLinknext_node;
         (int)chunkLink > 0 ? (chunkLinknext_node = area->chunkLinkList.RawNext(chunkLink), 1) : 0; chunkLink = chunkLinknext_node) {
      CMapChunk *chunk = static_cast<CMapChunk *>(chunkLink->owner);
      ASSERT(chunk);

      if (chunk->shadowGxTexture) {
        if (!CWorld::shadowMipLevel) {
          texUseage += 0x2000;
        } else {
          texUseage += 0x800;
        }
      }

      for (UINT i = 0; i < chunk->nLayers; ++i) {
        CChunkLayer *layer = chunk->layerList[i];
        ASSERT(layer);

        if (layer->props & 0x0100) {
          if (!CWorld::alphaMipLevel) {
            texUseage += 0x2000;
          } else {
            texUseage += 0x800;
          }
        }
      }
    }
  }

  return texUseage;
}

void CMap::ClearDetailDoodads() {
  for (CMapChunk *chunk = chunkList.Head(); chunk; chunk = static_cast<CMapChunk *>(chunk->lameAssLink.Next())) {
    if (chunk->detailDoodadInst) {
      CDetailDoodad::FreeInst(chunk->detailDoodadInst);
      chunk->detailDoodadInst = 0;
    }
  }
}

float CMap::PointIntersect(float wx, float wy, float radius) {
  float mx = -(wy - 17066.666f);
  float my = -(wx - 17066.666f);

  ASSERT(mx >= 0.0f && my >= 0.0f);
  ASSERT(mx < ((64*16)*((150.0f/36.0f)*8)) && my < ((64*16)*((150.0f/36.0f)*8)));

  mx = OO_COORD_TO_SUBCHUNK * mx;
  int mxIndex = Fast_ftol(mx);
  my = OO_COORD_TO_SUBCHUNK * my;
  int myIndex = Fast_ftol(my);
  CMapArea *area = areaTable[((myIndex >> 7) & 0x3F) * 64 + ((mxIndex >> 7) & 0x3F)];
  if (!area) {
    return 0.0f;
  }

  CMapChunk *chunk = area->chunkTable[((myIndex >> 3) & 0xF) * 16 + ((mxIndex >> 3) & 0xF)];
  if (!chunk) {
    return 0.0f;
  }

  mxIndex &= 7;
  myIndex &= 7;
  if (chunk->holes & g_holeMask[myIndex >> 1][mxIndex >> 1]) {
    return 0.0f;
  }

  float               lx = wx - chunk->corner.x;
  float               ly = wy - chunk->corner.y;
  NTempest::C4Plane  *p = &chunk->planeList[4 * (mxIndex + 8 * myIndex)];
  NTempest::C3Vector *v = &chunk->vertexList[mxIndex + 17 * myIndex];
  UINT                triangle = 0;
  if ((v[18].x - v[0].x) * (v[18].y - ly) - (v[18].y - v[0].y) * (v[18].x - lx) <= 0.0f) {
    triangle = 1;
  }
  if ((v[17].x - v[1].x) * (v[17].y - ly) - (v[17].y - v[1].y) * (v[17].x - lx) <= 0.0f) {
    triangle += 2;
  }

  p += triangle;
  return chunk->corner.z - (ly * p->n.y + lx * p->n.x + p->d) / p->n.z;
}

bool CMap::GetPlane(float wx, float wy, NTempest::C4Plane &plane) {
  float mx = -(wy - 17066.666f);
  float my = -(wx - 17066.666f);

  ASSERT(mx >= 0.0f && my >= 0.0f);
  ASSERT(mx < ((64*16)*((150.0f/36.0f)*8)) && my < ((64*16)*((150.0f/36.0f)*8)));

  mx = OO_COORD_TO_SUBCHUNK * mx;
  int mxIndex = Fast_ftol(mx);
  my = OO_COORD_TO_SUBCHUNK * my;
  int myIndex = Fast_ftol(my);
  CMapArea *area = areaTable[((myIndex >> 7) & 0x3F) * 64 + ((mxIndex >> 7) & 0x3F)];
  if (!area) {
    return false;
  }

  CMapChunk *chunk = area->chunkTable[((myIndex >> 3) & 0xF) * 16 + ((mxIndex >> 3) & 0xF)];
  if (!chunk) {
    return false;
  }

  mxIndex &= 7;
  myIndex &= 7;
  if (chunk->holes & g_holeMask[myIndex >> 1][mxIndex >> 1]) {
    return false;
  }

  float                    lx = wx - chunk->corner.x;
  float                    ly = wy - chunk->corner.y;
  const NTempest::C4Plane *p = &chunk->planeList[4 * (mxIndex + 8 * myIndex)];
  NTempest::C3Vector      *v = &chunk->vertexList[mxIndex + 17 * myIndex];
  UINT                     triangle = 0;
  if ((v[18].x - v[0].x) * (v[18].y - ly) - (v[18].y - v[0].y) * (v[18].x - lx) <= 0.0f) {
    triangle = 1;
  }
  if ((v[17].x - v[1].x) * (v[17].y - ly) - (v[17].y - v[1].y) * (v[17].x - lx) <= 0.0f) {
    triangle += 2;
  }

  plane = p[triangle];
  plane.d -= chunk->corner.z * plane.n.z + chunk->corner.y * plane.n.y + chunk->corner.x * plane.n.x;
  return true;
}

bool CMap::VectorIntersectTerrain(const NTempest::C3Vector *p0, const NTempest::C3Vector *p1, float *t, UINT queryFlags, CMapChunk **chunk) {
  NTempest::C2Vector v0(-(p0->y - 17066.666f), -(p0->x - 17066.666f));
  NTempest::C2Vector v1(-(p1->y - 17066.666f), -(p1->x - 17066.666f));
  float              dx = v1.x - v0.x;
  float              dy = v1.y - v0.y;
  NTempest::CiRect   sRect(
      Fast_ftol(OO_COORD_TO_SUBCHUNK * v0.y), Fast_ftol(OO_COORD_TO_SUBCHUNK * v0.x), Fast_ftol(OO_COORD_TO_SUBCHUNK * v1.y),
      Fast_ftol(OO_COORD_TO_SUBCHUNK * v1.x)
  );

  scCollideCnt = 0;

  if (NTempest::CMath::fabs_(dx) < 2.3841858e-7f || sRect.l == sRect.r) {
    VectorIntersectSY(sRect);
  } else if (NTempest::CMath::fabs_(dy) < 2.3841858e-7f || sRect.t == sRect.b) {
    VectorIntersectSX(sRect);
  } else if (NTempest::CMath::fabs_(dx) > NTempest::CMath::fabs_(dy)) {
    VectorIntersectDX(v0, v1, sRect);
  } else {
    VectorIntersectDY(v0, v1, sRect);
  }

  return VectorIntersectSubchunks(p0, p1, t, queryFlags, chunk);
}

bool CMap::VectorIntersect(const NTempest::C3Vector *p0, const NTempest::C3Vector *p1, NTempest::C3Vector *ip, float *dist, UINT queryFlags) {
  FATALASSERT(p0);
  FATALASSERT(p1);
  FATALASSERT(ip);
  FATALASSERT(dist);
  FATALASSERT(*dist >= 0.0f && *dist <= 1.0f);

  bool hit = false;
  ++cCount;

  if (queryFlags & 0xF0) {
    UINT polyIgnoreFlags = 0;
    if (!(queryFlags & 0x10)) {
      polyIgnoreFlags = 8;
    }
    if (queryFlags & 0x40) {
      polyIgnoreFlags |= 2;
    }
    if (VectorIntersectMapObjs(p0, p1, queryFlags, polyIgnoreFlags, 0, dist, 0, 0)) {
      hit = true;
    }
  }

  if ((queryFlags & 0xF00) && VectorIntersectTerrain(p0, p1, dist, queryFlags, 0)) {
    hit = true;
  }

  if (hit) {
    *ip = *p0 + (*p1 - *p0) * *dist;
  }
  return hit;
}

bool CMap::VectorIntersectMapObjs(
    const NTempest::C3Vector *p0,
    const NTempest::C3Vector *p1,
    UINT                      queryFlags,
    UINT                      polyIgnoreFlags,
    UINT                      groupIgnoreFlags,
    float                    *t,
    SMOPoly                 **poly,
    CMapObj                 **qMapObj
) {
  FATALASSERT(*t >= 0.0f && *t <= 1.0f);

  bool hit = false;
  ITERATELIST(CMapObjDef, mapObjDefHash, mapObjDef) {
    if (mapObjDef->flags & CMapBaseObj::Flag_NoCollision) {
      continue;
    }

    NTempest::C3Vector v0 = *p0 * mapObjDef->invMat;
    NTempest::C3Vector v1 = *p1 * mapObjDef->invMat;
    CMapObj           *mapObj = mapObjDef->mapObj;
    if (!mapObj) {
      continue;
    }

    if (mapObj->VectorIntersect(mapObjDef, &v0, &v1, queryFlags, polyIgnoreFlags, groupIgnoreFlags, t, poly)) {
      hit = true;
      if (qMapObj) {
        *qMapObj = mapObj;
      }
    }
  }
  return hit;
}

bool CMap::LocateViewerMapObjs(
    const NTempest::C3Vector &lCen,
    const NTempest::C3Vector &lEnd,
    float                    &maxT,
    CMapObjDef              *&hitMapObjDef,
    UINT                      hitGroupIDs[]
) {
  hitMapObjDef = 0;
  hitGroupIDs[1] = 0xFFFF;
  hitGroupIDs[0] = 0xFFFF;

  ITERATELIST(CMapObjDef, mapObjDefHash, mapObjDef) {
    if (!(mapObjDef->flags & CMapBaseObj::Flag_NoCollision) && mapObjDef->TestAABox(lCen, lEnd)) {
      CMapObj *mapObj = mapObjDef->mapObj;
      if (mapObj) {
        NTempest::C3Vector v0 = lCen * mapObjDef->invMat;
        NTempest::C3Vector v1 = lEnd * mapObjDef->invMat;

        ITERATELIST(CMapBaseObjLink, mapObjDef->groupLinkList, link) {
          CMapObjDefGroup *mapObjDefGroup = static_cast<CMapObjDefGroup *>(link->owner);
          if (mapObj->TestGroupBounds(v0, v1, mapObjDefGroup->groupNum)) {
            CMapObjGroup *mapObjGroup = mapObj->GetGroup(mapObjDefGroup->groupNum, 0);
            if (mapObjGroup) {
              CWTriData triData;
              if (mapObjGroup->GetTris(triData, NTempest::C3Segment(v0, v1), maxT, mapObjDef, 0)) {
                hitMapObjDef = mapObjDef;
                hitGroupIDs[0] = mapObjDefGroup->groupNum;
                hitGroupIDs[1] = 0xFFFF;
              }
            }
          }
        }

        float portalT = 1.0f;
        UINT  portalGroups[2];
        if (mapObj->VectorIntersectPortals(NTempest::C3Segment(v0, v1), portalT, portalGroups) && portalT - maxT < 0.0001f) {
          maxT = portalT;
          if (!(mapObj->GetGroupInfo(portalGroups[0])->flags & 8)) {
            hitMapObjDef = mapObjDef;
            hitGroupIDs[0] = portalGroups[0];
            if (!(mapObj->GetGroupInfo(portalGroups[1])->flags & 8)) {
              hitGroupIDs[1] = portalGroups[1];
            } else {
              hitGroupIDs[1] = 0xFFFF;
            }
          }
        }

        if (hitMapObjDef == mapObjDef && mapObj->GetGroupInfo(hitGroupIDs[0])->flags & 8) {
          hitMapObjDef = 0;
        }
      }
    }
  }

  return hitMapObjDef;
}

void CMap::VectorIntersectSX(NTempest::CiRect &sRect) {
  long sx;

  if (sRect.r < sRect.l) {
    for (sx = sRect.l; sx >= sRect.r; --sx) {
      scCollideList[scCollideCnt++] = sx;
      scCollideList[scCollideCnt++] = sRect.t;
    }
  } else {
    for (sx = sRect.l; sx <= sRect.r; ++sx) {
      scCollideList[scCollideCnt++] = sx;
      scCollideList[scCollideCnt++] = sRect.t;
    }
  }
}

void CMap::VectorIntersectSY(NTempest::CiRect &sRect) {
  long sy;

  if (sRect.b < sRect.t) {
    for (sy = sRect.t; sy >= sRect.b; --sy) {
      scCollideList[scCollideCnt++] = sRect.l;
      scCollideList[scCollideCnt++] = sy;
    }
  } else {
    for (sy = sRect.t; sy <= sRect.b; ++sy) {
      scCollideList[scCollideCnt++] = sRect.l;
      scCollideList[scCollideCnt++] = sy;
    }
  }
}

void CMap::VectorIntersectDX(const NTempest::C3Vector &p0, const NTempest::C3Vector &p1, NTempest::CiRect &sRect) {
  int   step;
  float edge;
  float m = (p1.y - p0.y) / (p1.x - p0.x);
  float b = -(m * p0.x);
  b += p0.y;
  int   x = sRect.l;
  int   y = sRect.t;

  if (sRect.r > x) {
    edge = (x + 1) * (150.0f / 36.0f);
    step = 1;
  } else {
    edge = x * (150.0f / 36.0f);
    step = -1;
  }

  scCollideList[scCollideCnt++] = x;
  scCollideList[scCollideCnt++] = y;

  while (x != sRect.r + step) {
    int sx = Fast_ftol((edge * m + b) * OO_COORD_TO_SUBCHUNK);

    if (sx != y) {
      scCollideList[scCollideCnt++] = x;
      scCollideList[scCollideCnt++] = sx;
    }

    x += step;
    scCollideList[scCollideCnt++] = x;
    scCollideList[scCollideCnt++] = sx;
    y = sx;
    edge += step * (150.0f / 36.0f);
  }

  if (y != sRect.b) {
    scCollideList[scCollideCnt++] = sRect.r;
    scCollideList[scCollideCnt++] = sRect.b;
  }
}

void CMap::VectorIntersectDY(const NTempest::C3Vector &p0, const NTempest::C3Vector &p1, NTempest::CiRect &sRect) {
  int   step;
  float edge;
  float m = (p1.y - p0.y) / (p1.x - p0.x);
  float b = -(m * p0.x);
  b += p0.y;
  float oom = 1.0f / m;
  int   x = sRect.l;
  int   y = sRect.t;

  if (sRect.b > sRect.t) {
    edge = (y + 1) * (150.0f / 36.0f);
    step = 1;
  } else {
    edge = y * (150.0f / 36.0f);
    step = -1;
  }

  scCollideList[scCollideCnt++] = sRect.l;
  scCollideList[scCollideCnt++] = sRect.t;

  while (y != sRect.b + step) {
    int sy = Fast_ftol(((edge - b) * oom) * OO_COORD_TO_SUBCHUNK);

    if (sy != x) {
      scCollideList[scCollideCnt++] = sy;
      scCollideList[scCollideCnt++] = y;
    }

    scCollideList[scCollideCnt++] = sy;
    scCollideList[scCollideCnt++] = y + step;
    y += step;
    x = sy;
    edge += step * (150.0f / 36.0f);
  }

  if (x != sRect.r) {
    scCollideList[scCollideCnt++] = sRect.r;
    scCollideList[scCollideCnt++] = sRect.b;
  }
}

bool CMap::VectorIntersectSubchunks(const NTempest::C3Vector *p0, const NTempest::C3Vector *p1, float *t, UINT queryFlags, CMapChunk **retChunk) {
  UINT *scPtr = scCollideList.Ptr();
  FATALASSERT(scPtr);

  UINT       scCnt = scCollideCnt;
  UINT       bMaskY = scPtr[0] & 0x2000;
  UINT       bMaskX = scPtr[1] & 0x2000;
  CMapChunk *chunk = 0;
  CMapChunk *hitChunk = 0;
  float      hitT = *t;

  while (scCnt) {
    UINT sx = scPtr[0];
    UINT sy = scPtr[1];
    scPtr += 2;
    scCnt -= 2;

    if (sx > 0x2000 || sy > 0x2000) {
      return false;
    }

    if ((sx & 0x1FF8) != bMaskX || (sy & 0x1FF8) != bMaskY) {
      CMapArea *area = areaTable[((sy >> 7) & 0x3F) * 64 + ((sx >> 7) & 0x3F)];
      if (!area) {
        return false;
      }

      chunk = area->chunkTable[((sy >> 3) & 0xF) * 16 + ((sx >> 3) & 0xF)];
      if (!chunk) {
        return false;
      }

      bMaskY = sy & 0x1FF8;
      bMaskX = sx & 0x1FF8;

      if (queryFlags & 0xF) {
        float thisdHitT = 1.0f;
        if (VectorIntersectDoodadDefLinkList(chunk->doodadDefLinkList, p0, p1, &thisdHitT, queryFlags) && thisdHitT < hitT) {
          hitT = thisdHitT;
        }
        if (VectorIntersectGameObjLinkList(chunk->entityLinkList, p0, p1, &thisdHitT, queryFlags) && thisdHitT < hitT) {
          hitT = thisdHitT;
        }
      }
    }

    sx &= 7;
    sy &= 7;
    NTempest::C3Vector lp0 = *p0 - chunk->corner;
    NTempest::C3Vector lp1 = *p1 - chunk->corner;

    if (chunk->holes & g_holeMask[sy >> 1][sx >> 1]) {
      continue;
    }

    NTempest::C4Plane  *p = &chunk->planeList[4 * (sx + 8 * sy)];
    NTempest::C3Vector *v = &chunk->vertexList[sx + 17 * sy];
    for (int triangle = 0; triangle < 4; ++triangle, ++p) {
      float ip0 = lp0.x * p->n.x + lp0.y * p->n.y + lp0.z * p->n.z + p->d;
      float ip1 = lp1.x * p->n.x + lp1.y * p->n.y + lp1.z * p->n.z + p->d;

      if (ip0 > 0.0f && ip1 > 0.0f) {
        continue;
      }
      if (ip0 < 0.0f && ip1 < 0.0f) {
        continue;
      }

      float it = ip0 / (ip0 - ip1);
      if (it < 0.0f || it > 1.0f) {
        continue;
      }

      NTempest::C3Vector tempIp((lp1.x - lp0.x) * it + lp0.x, (lp1.y - lp0.y) * it + lp0.y, (lp1.z - lp0.z) * it + lp0.z);
      if (VectorIntersectTri(&tempIp, &v[9], &v[iIndiciesP[triangle][0]], &v[iIndiciesP[triangle][1]], &p->n) && it < hitT) {
        hitT = it;
        hitChunk = chunk;
      }
    }
  }

  if (hitT < *t) {
    *t = hitT;
    if (retChunk) {
      *retChunk = hitChunk;
    }
    return true;
  }
  return false;
}

bool CMap::VectorIntersectDoodadDefLinkList(
    LISTEX(CMapBaseObjLink, refLink) & doodadDefLinkList,
    const NTempest::C3Vector *p0,
    const NTempest::C3Vector *p1,
    float                    *t,
    UINT                      queryFlags
) {
  NTempest::C3Vector camrelP0 = *p0 - CWorldScene::camPos;
  NTempest::C3Vector camrelP1 = *p1 - CWorldScene::camPos;
  float              oovmag = 1.0f / (camrelP1 - camrelP0).Mag();
  float              hitT = *t;
  bool               hit = false;

  ITERATELIST(CMapBaseObjLink, doodadDefLinkList, link) {
    CMapDoodadDef *doodadDef = static_cast<CMapDoodadDef *>(link->owner);
    FATALASSERT(doodadDef);

    if ((doodadDef->flags & CMapBaseObj::Flag_NoCollision) || doodadDef->cCount == cCount || !doodadDef->model) {
      continue;
    }

    doodadDef->cCount = cCount;
    NTempest::CAaBox bounds(0.0f);
    doodadDef->GetBounds(bounds);
    if (!CWorldMath::VectorIntersectAABox2(bounds, *p0, *p1)) {
      continue;
    }

    float it;
    if (queryFlags & 1) {
      if (!doodadDef->model) {
        continue;
      }
      if (ModelCollisionVectorIntersect(doodadDef->model, doodadDef->mat, *p0, *p1, it)) {
        if (it < hitT) {
          hitT = it;
        }
        hit = true;
      }
    } else if (ModelHitTestGeometry(doodadDef->model, doodadDef->scale, camrelP0, camrelP1, 0, &it)) {
      it *= oovmag;
      if (it <= 1.0f && it < hitT) {
        hitT = it;
        hit = true;
      }
    }
  }

  if (hit) {
    *t = hitT;
  }
  return hit;
}

bool CMap::VectorIntersectGameObjLinkList(
    LISTEX(CMapBaseObjLink, refLink) & gameObjLinkList,
    const NTempest::C3Vector *p0,
    const NTempest::C3Vector *p1,
    float                    *t,
    UINT                      queryFlags
) {
  if (!entityCollisionHandler) {
    return false;
  }

  NTempest::C3Vector camrelP0 = *p0 - CWorldScene::camPos;
  NTempest::C3Vector camrelP1 = *p1 - CWorldScene::camPos;
  float              oovmag = 1.0f / (camrelP1 - camrelP0).Mag();
  float              hitT = *t;
  bool               hit = false;

  ITERATELIST(CMapBaseObjLink, gameObjLinkList, link) {
    CMapEntity *entity = static_cast<CMapEntity *>(link->owner);
    if (!entity->flagCollidable) {
      continue;
    }

    WorldObjCollisionHandlerData data;
    if (entityCollisionHandler(entity->param64, entity->param32, &data)) {
      if (CWorldMath::VectorIntersectAABox2(data.collideExt, *p0, *p1)) {
        float it;
        if (queryFlags & 1) {
          if (data.model) {
            if (ModelCollisionVectorIntersect(data.model, data.matrix, *p0, *p1, it)) {
              if (it < hitT) {
                hitT = it;
              }
              hit = true;
            }
          }
        } else if (ModelHitTestGeometry(data.model, data.scale, camrelP0, camrelP1, 0, &it)) {
          it *= oovmag;
          if (it <= 1.0f && it < hitT) {
            hitT = it;
            hit = true;
          }
        }
      }
    }
  }

  if (hit) {
    *t = hitT;
  }
  return hit;
}

bool CMap::VectorIntersectTri(
    const NTempest::C3Vector *p,
    const NTempest::C3Vector *v0,
    const NTempest::C3Vector *v1,
    const NTempest::C3Vector *v2,
    const NTempest::C3Vector *n
) {
  float *t[4] = {const_cast<float *>(&v0->x), const_cast<float *>(&v1->x), const_cast<float *>(&v2->x), const_cast<float *>(&p->x)};
  int    i0 = 0;
  int    i1 = 2;
  float  fac = -n->y;
  float  nx = NTempest::CMath::fabs_(n->x);
  float  ny = NTempest::CMath::fabs_(n->y);
  float  nz = NTempest::CMath::fabs_(n->z);

  if (nx > ny) {
    i0 = 2;
    i1 = 1;
    fac = -n->x;
    ny = nx;
  }

  if (nz > ny) {
    i0 = 0;
    i1 = 1;
    fac = n->z;
  }

  int j = 1;
  for (int i = 0; i < 3; ++i) {
    if (fac * ((t[j][i0] - t[i][i0]) * (t[3][i1] - t[j][i1]) - (t[j][i1] - t[i][i1]) * (t[3][i0] - t[j][i0])) > 0.019444443f) {
      return false;
    }

    ++j;
    if (j > 2) {
      j = 0;
    }
  }

  return true;
}

void CMap::TestQueryAdd(const NTempest::CFacet &facet, NTempest::CImVector color, const NTempest::C44Matrix *basis) {
  NTempest::C44Matrix        id;
  UINT                       sub = testQueryVerts.Count();
  const NTempest::C44Matrix *mtx = basis ? basis : &id;

  for (UINT i = 0; i < 3; ++i) {
    CGxVertexPC *v = testQueryVerts.NewElement();
    v->p = facet.vertices[i] * *mtx;
    v->c = color;
  }

  testQueryIndices.Add(reinterpret_cast<WORD *>(&sub));
  ++sub;
  testQueryIndices.Add(reinterpret_cast<WORD *>(&sub));
  ++sub;
  testQueryIndices.Add(reinterpret_cast<WORD *>(&sub));
}

void CMap::TestQueryAdd(const CWFrustum &frustum, NTempest::CImVector color, const NTempest::C44Matrix *basis) {
  NTempest::C44Matrix        id;
  UINT                       sub = testQueryVerts.Count();
  const NTempest::C44Matrix *mtx = basis ? basis : &id;
  UINT                       i;

  for (i = 0; i < 8; ++i) {
    CGxVertexPC *v = testQueryVerts.NewElement();
    v->p = frustum.corners[i] * *mtx;
    v->c = color;
  }

  for (i = 0; i < 36; ++i) {
    WORD index = static_cast<WORD>(sub + idxoffs[i]);
    testQueryIndices.Add(&index);
  }
}

void CMap::TestQueryAdd(const NTempest::CAaBox &aabox, NTempest::CImVector color, const NTempest::C44Matrix *basis) {
  NTempest::C44Matrix id;
  UINT                sub = testQueryVerts.Count();
  if (!basis) {
    basis = &id;
  }

  CGxVertexPC *v = testQueryVerts.NewElement();
  v->p = NTempest::C3Vector(aabox.b.x, aabox.b.y, aabox.b.z) * *basis;
  v->c = color;
  v = testQueryVerts.NewElement();
  v->p = NTempest::C3Vector(aabox.t.x, aabox.b.y, aabox.b.z) * *basis;
  v->c = color;
  v = testQueryVerts.NewElement();
  v->p = NTempest::C3Vector(aabox.t.x, aabox.t.y, aabox.b.z) * *basis;
  v->c = color;
  v = testQueryVerts.NewElement();
  v->p = NTempest::C3Vector(aabox.b.x, aabox.t.y, aabox.b.z) * *basis;
  v->c = color;
  v = testQueryVerts.NewElement();
  v->p = NTempest::C3Vector(aabox.b.x, aabox.b.y, aabox.t.z) * *basis;
  v->c = color;
  v = testQueryVerts.NewElement();
  v->p = NTempest::C3Vector(aabox.t.x, aabox.b.y, aabox.t.z) * *basis;
  v->c = color;
  v = testQueryVerts.NewElement();
  v->p = NTempest::C3Vector(aabox.t.x, aabox.t.y, aabox.t.z) * *basis;
  v->c = color;
  v = testQueryVerts.NewElement();
  v->p = NTempest::C3Vector(aabox.b.x, aabox.t.y, aabox.t.z) * *basis;
  v->c = color;

  for (UINT i = 0; i < 36; ++i) {
    WORD index = static_cast<WORD>(sub + idxoffs[i]);
    testQueryIndices.Add(&index);
  }
}

bool CMap::GetFacet(const NTempest::C3Segment &seg, float &t, NTempest::C4Plane &facet, UINT queryFlags) {
  bool hit = false;
  if (queryFlags & 0xFF) {
    hit = GetFacetMapObjs(seg, t, facet, queryFlags);
  }
  if (queryFlags & 0xF0F) {
    hit |= GetFacetTerrain(seg, t, facet, queryFlags);
  }
  return hit;
}

bool CMap::GetFacetMapObjs(const NTempest::C3Segment &seg, float &t, NTempest::C4Plane &facet, UINT queryFlags) {
  bool hit = false;

  ITERATELIST(CMapObjDef, mapObjDefHash, mapObjDef) {
    if (mapObjDef->flags & CMapBaseObj::Flag_NoCollision) {
      continue;
    }

    CMapObj *mapObj = mapObjDef->mapObj;
    if (!mapObj) {
      continue;
    }

    NTempest::C3Segment relSeg(seg.start * mapObjDef->invMat, seg.end * mapObjDef->invMat);
    UINT                flags = 0;
    if (queryFlags & 0x10) {
      flags = 8;
    } else if (queryFlags & 0x20) {
      flags = 32;
    }

    CWTriData triData;
    if (!mapObj->GetTris(triData, relSeg, t, mapObjDef, flags)) {
      continue;
    }

    hit = true;
    const CWTriData::Batch &batch = triData.GetBatch(0);
    const WORD             *indices = batch.vertexIndices;
    facet.n = NTempest::C3Vector::Cross(
        batch.vertices[indices[1]] * *batch.matrix - batch.vertices[indices[0]] * *batch.matrix,
        batch.vertices[indices[2]] * *batch.matrix - batch.vertices[indices[0]] * *batch.matrix
    );
    facet.n.Normalize();
    facet.d = -NTempest::C3Vector::Dot(batch.vertices[indices[0]] * *batch.matrix, facet.n);
  }

  return hit;
}

bool CMap::GetFacetTerrain(const NTempest::C3Segment &seg, float &t, NTempest::C4Plane &facet, UINT queryFlags) {
  NTempest::C3Segment nb;
  NTempest::C2Vector  v0(-(seg.start.y - 17066.666f), -(seg.start.x - 17066.666f));
  NTempest::C2Vector  v1(-(seg.end.y - 17066.666f), -(seg.end.x - 17066.666f));
  float               dy = v1.y - v0.y;
  float               dx = v1.x - v0.x;
  NTempest::CiRect    sRect(
      Fast_ftol(OO_COORD_TO_SUBCHUNK * v0.y), Fast_ftol(OO_COORD_TO_SUBCHUNK * v0.x),
      Fast_ftol(OO_COORD_TO_SUBCHUNK * v1.y), Fast_ftol(OO_COORD_TO_SUBCHUNK * v1.x)
  );

  scCollideCnt = 0;

  if (NTempest::CMath::fabs_(dx) < 2.3841858e-7f || sRect.l == sRect.r) {
    VectorIntersectSY(sRect);
  } else if (NTempest::CMath::fabs_(dy) < 2.3841858e-7f || sRect.t == sRect.b) {
    VectorIntersectSX(sRect);
  } else if (NTempest::CMath::fabs_(dx) > NTempest::CMath::fabs_(dy)) {
    VectorIntersectDX(v0, v1, sRect);
  } else {
    VectorIntersectDY(v0, v1, sRect);
  }

  float nt = t;
  nb.start = seg.start;
  nb.end = nb.start + (seg.end - seg.start) * nt;

  if (GetFacetSubchunks(nb, nt, facet, queryFlags)) {
    t = nt * t;
    return true;
  }

  return false;
}

bool CMap::GetFacetSubchunks(const NTempest::C3Segment &seg, float &t, NTempest::C4Plane &facet, UINT queryFlags) {
  UINT *scPtr = scCollideList.Ptr();
  ASSERT(scPtr);

  UINT       scCnt = scCollideCnt;
  UINT       bMaskY = scPtr[0] & 0x2000;
  UINT       bMaskX = scPtr[1] & 0x2000;
  CMapChunk *chunk = 0;
  bool       hit = false;

  if (!scCnt) {
    return false;
  }

  do {
    UINT sx = scPtr[0];
    UINT sy = scPtr[1];
    scPtr += 2;
    scCnt -= 2;

    if (sx > 0x2000 || sy > 0x2000) {
      return false;
    }

    if ((sx & 0x1FF8) != bMaskX || (sy & 0x1FF8) != bMaskY) {
      CMapArea *area = areaTable[((sy >> 7) & 0x3F) * 64 + ((sx >> 7) & 0x3F)];
      if (!area) {
        return false;
      }

      chunk = area->chunkTable[((sy >> 3) & 0xF) * 16 + ((sx >> 3) & 0xF)];
      if (!chunk) {
        return false;
      }

      bMaskX = sx & 0x1FF8;
      bMaskY = sy & 0x1FF8;
    }

    sx &= 7;
    sy &= 7;
    NTempest::C3Segment localSeg;
    localSeg.start = seg.start - chunk->corner;
    localSeg.end = seg.end - chunk->corner;

    if (chunk->holes & g_holeMask[sy >> 1][sx >> 1]) {
      continue;
    }

    NTempest::C4Plane  *p = &chunk->planeList[4 * (sx + 8 * sy)];
    NTempest::C3Vector *v = &chunk->vertexList[sx + 17 * sy];
    for (int triangle = 0; triangle < 4; ++triangle, ++p) {
      float ip0 = NTempest::C3Vector::Dot(p->n, localSeg.start) + p->d;
      float ip1 = NTempest::C3Vector::Dot(p->n, localSeg.end) + p->d;

      if (ip0 > 0.0f && ip1 > 0.0f) {
        continue;
      }
      if (ip0 < 0.0f && ip1 < 0.0f) {
        continue;
      }

      float it = ip0 / (ip0 - ip1);
      if (it < 0.0f || it > 1.0f) {
        continue;
      }

      NTempest::C3Vector tempIp = (localSeg.end - localSeg.start) * it + localSeg.start;
      if (VectorIntersectTri(&tempIp, &v[9], &v[iIndiciesP[triangle][0]], &v[iIndiciesP[triangle][1]], &p->n) && it < t) {
        t = it;
        facet = *p;
        hit = true;
      }
    }
  } while (scCnt);

  return hit;
}

bool CMap::GetFacets(const NTempest::CAaBox &aaBox, CWFacetData *facetData, UINT queryFlags) {
  FATALASSERT(facetData);

  ++mapGetFacetsCount;
  ++cCount;
  facetData->facets.SetCount(0);
  facetData->facets.SetChunkSize(32);

  if (queryFlags & 0xF0) {
    GetFacetsMapObjs(aaBox, facetData, queryFlags);
  }

  NTempest::CRect tLocation(-(aaBox.t.x - 17066.666f), -(aaBox.t.y - 17066.666f), -(aaBox.b.x - 17066.666f), -(aaBox.b.y - 17066.666f));

  if (tLocation.l < 0.0f || tLocation.t < 0.0f || tLocation.r >= 34133.332f || tLocation.b >= 34133.332f) {
    FATALERROR(("Request for facets off edge of map: (%g,%g,%g,%g)", aaBox.b.x, aaBox.b.y, aaBox.t.x, aaBox.t.y));
    return false;
  }

  NTempest::CiRect sRect(
      Fast_ftol(OO_COORD_TO_SUBCHUNK * tLocation.t), Fast_ftol(OO_COORD_TO_SUBCHUNK * tLocation.l),
      Fast_ftol(OO_COORD_TO_SUBCHUNK * tLocation.b), Fast_ftol(OO_COORD_TO_SUBCHUNK * tLocation.r)
  );
  NTempest::CiRect cRect(sRect.t >> 3, sRect.l >> 3, sRect.b >> 3, sRect.r >> 3);

  for (int cy = cRect.t; cy <= cRect.b; ++cy) {
    for (int cx = cRect.l; cx <= cRect.r; ++cx) {
      GetChunkFacets(cx, cy, sRect, aaBox, facetData, queryFlags);
    }
  }
  return facetData->facets.Count() > 0;
}

bool CMap::GetTrisTerrain(const NTempest::CAaBox &aaBox, CWTriData &triData, UINT queryFlags) {
  NTempest::CRect tLocation(-(aaBox.t.x - 17066.666f), -(aaBox.t.y - 17066.666f), -(aaBox.b.x - 17066.666f), -(aaBox.b.y - 17066.666f));
  FATALASSERT(tLocation.minx >= 0.0f && tLocation.miny >= 0.0f);
  FATALASSERT(tLocation.maxy < ((64*16)*((150.0f/36.0f)*8)) && tLocation.maxy < ((64*16)*((150.0f/36.0f)*8)));

  NTempest::CiRect sRect(
      Fast_ftol(OO_COORD_TO_SUBCHUNK * tLocation.t), Fast_ftol(OO_COORD_TO_SUBCHUNK * tLocation.l),
      Fast_ftol(OO_COORD_TO_SUBCHUNK * tLocation.b), Fast_ftol(OO_COORD_TO_SUBCHUNK * tLocation.r)
  );
  NTempest::CiRect cRect(sRect.t >> 3, sRect.l >> 3, sRect.b >> 3, sRect.r >> 3);

  bool got = false;
  for (int cy = cRect.t; cy <= cRect.b; ++cy) {
    for (int cx = cRect.l; cx <= cRect.r; ++cx) {
      got |= GetTrisChunk(cx, cy, sRect, aaBox, triData, queryFlags);
    }
  }
  return got;
}

bool CMap::GetTris(const NTempest::CAaBox &aaBox, CWTriData &triData, UINT queryFlags) {
  bool got = false;
  if (queryFlags & 0xF0) {
    got = GetTrisMapObjs(aaBox, triData, queryFlags);
  }
  if (queryFlags & 0xF00) {
    got |= GetTrisTerrain(aaBox, triData, queryFlags);
  }
  return got;
}

bool CMap::GetTrisMapObjs(const NTempest::CAaBox &aaBox, CWTriData &triData, UINT queryFlags) {
  NTempest::C3Vector lCen = (aaBox.b + aaBox.t) * 0.5f;
  NTempest::CAaBox   lBox = aaBox;
  NTempest::C3Vector tCen(-lCen.x, -lCen.y, -lCen.z);
  lBox.b += tCen;
  lBox.t += tCen;

  bool got = false;
  ITERATELIST(CMapObjDef, mapObjDefHash, mapObjDef) {
    if (mapObjDef->flags & CMapBaseObj::Flag_NoCollision) {
      continue;
    }

    tCen = lCen * mapObjDef->invMat;
    NTempest::C33Matrix tMat(
        mapObjDef->invMat.a0, mapObjDef->invMat.a1, mapObjDef->invMat.a2, mapObjDef->invMat.b0, mapObjDef->invMat.b1, mapObjDef->invMat.b2,
        mapObjDef->invMat.c0, mapObjDef->invMat.c1, mapObjDef->invMat.c2
    );
    NTempest::CAaBox tBox;
    CWorldMath::TransformAABox(tMat, lBox, tBox);
    tBox.b += tCen;
    tBox.t += tCen;

    CMapObj *mapObj = mapObjDef->mapObj;
    if (mapObj && mapObj->TestBounds(tBox)) {
      got |= mapObj->GetTris(triData, tBox, mapObjDef, queryFlags);
    }
  }
  return got;
}

bool CMap::GetFacetsMapObjs(const NTempest::CAaBox &aaBox, CWFacetData *facetData, UINT queryFlags) {
  UINT               origFacetCount = facetData->facets.Count();
  NTempest::C3Vector lCen = (aaBox.b + aaBox.t) * 0.5f;
  NTempest::CAaBox   lBox = aaBox;
  NTempest::C3Vector tCen(-lCen.x, -lCen.y, -lCen.z);
  lBox.b += tCen;
  lBox.t += tCen;

  ITERATELIST(CMapObjDef, mapObjDefHash, mapObjDef) {
    if (mapObjDef->flags & CMapBaseObj::Flag_NoCollision || !(aaBox.b <= mapObjDef->aaBox.t && aaBox.t >= mapObjDef->aaBox.b)) {
      continue;
    }

    tCen = lCen * mapObjDef->invMat;
    NTempest::C33Matrix tMat(
        mapObjDef->invMat.a0, mapObjDef->invMat.a1, mapObjDef->invMat.a2, mapObjDef->invMat.b0, mapObjDef->invMat.b1, mapObjDef->invMat.b2,
        mapObjDef->invMat.c0, mapObjDef->invMat.c1, mapObjDef->invMat.c2
    );
    NTempest::CAaBox tBox;
    CWorldMath::TransformAABox(tMat, lBox, tBox);
    tBox.b += tCen;
    tBox.t += tCen;

    CMapObj *mapObj = mapObjDef->mapObj;
    if (!mapObj || !mapObj->TestBounds(tBox)) {
      continue;
    }

    CWTriData triData;
    mapObj->GetTris(triData, tBox, mapObjDef, queryFlags);
    CWorld::TriDataToFacetData(triData, *facetData, mapObjDef->param64);

    ITERATELIST(CMapBaseObjLink, mapObjDef->groupLinkList, groupLink) {
      CMapObjDefGroup *mapObjDefGroup = static_cast<CMapObjDefGroup *>(groupLink->owner);
      if (!mapObj->TestGroupBounds(tBox, mapObjDefGroup->groupNum)) {
        continue;
      }

      if ((queryFlags & 1) && !(queryFlags & 0x2000)) {
        ITERATELIST(CMapBaseObjLink, mapObjDefGroup->doodadDefLinkList, doodadDefLink) {
          CMapDoodadDef *doodadDef = static_cast<CMapDoodadDef *>(doodadDefLink->owner);
          FATALASSERT(doodadDef);
          if (doodadDef->cCount != cCount && doodadDef->model) {
            NTempest::CAaBox collideExt;
            doodadDef->GetCollideExt(collideExt);
            if (collideExt.b <= aaBox.t && collideExt.t >= aaBox.b) {
              AddDoodadFacets(aaBox, doodadDef, facetData);
              doodadDef->cCount = cCount;
            }
          }
        }
      }

      if (entityCollisionHandler) {
        ITERATELIST(CMapBaseObjLink, mapObjDefGroup->entityLinkList, entityLink) {
          CMapEntity *entity = static_cast<CMapEntity *>(entityLink->owner);
          if (!entity->flagCollidable) {
            continue;
          }

          WorldObjCollisionHandlerData data;
          if (entityCollisionHandler(entity->param64, entity->param32, &data) && data.collideExt.b <= aaBox.t && data.collideExt.t >= aaBox.b) {
            AddGameObjFacets(aaBox, data, entity->param64, facetData);
          }
        }
      }
    }
  }

  return origFacetCount != facetData->facets.Count();
}

static void AddDoodadFacets(const NTempest::CAaBox &aaBox, CMapDoodadDef *doodadDef, CWFacetData *facetData) {
  UINT existing = facetData->facets.Count();
  ModelAddCollisionFacets(doodadDef->model, doodadDef->mat, doodadDef->scale, aaBox, &facetData->facets);

  UINT count = facetData->facets.Count();
  facetData->gameObjects.SetCount(count);
  if (count != existing) {
    memset(&facetData->gameObjects[existing], 0, (count - existing) * sizeof(facetData->gameObjects[0]));
  }
}

static void AddGameObjFacets(const NTempest::CAaBox &aaBox, const WorldObjCollisionHandlerData &data, DWORDLONG guid, CWFacetData *facetData) {
  UINT existing = facetData->facets.Count();
  ModelAddCollisionFacets(data.model, data.matrix, data.scale, aaBox, &facetData->facets);

  UINT count = facetData->facets.Count();
  facetData->gameObjects.SetCount(count);
  for (UINT index = existing; index < count; ++index) {
    facetData->gameObjects[index] = guid;
  }
}

bool CMap::GetTrisChunk(int cx, int cy, NTempest::CiRect &sRect, const NTempest::CAaBox &aaBox, CWTriData &triData, UINT queryFlags) {
  CMapArea *area = areaTable[((cy >> 4) & 0x3F) * 64 + ((cx >> 4) & 0x3F)];
  if (!area) {
    return 0;
  }
  CMapChunk *chunk = area->chunkTable[(cy & 0xF) * 16 + (cx & 0xF)];
  if (!chunk) {
    return 0;
  }

  NTempest::CiRect scRect(sRect.t - 8 * cy, sRect.l - 8 * cx, sRect.b - 8 * cy, sRect.r - 8 * cx);
  static NTempest::CiRect scBounds(0, 0, 7, 7);
  scRect = NTempest::CiRect::Intersection(scRect, scBounds);

  NTempest::CAaBox  localBox(aaBox.b - chunk->corner, aaBox.t - chunk->corner);
  CWTriData::Batch *batch = 0;
  WORD             *indices = 0;
  UINT              indexCount = 0;
  UINT              culled[19];

  for (int y = scRect.t; y <= scRect.b; ++y) {
    for (int x = scRect.l; x <= scRect.r; ++x) {
      if (chunk->holes & g_holeMask[y >> 1][x >> 1]) {
        continue;
      }
      FATALASSERT(s_vertexIndex[2][2] == 18);
      NTempest::C3Vector *v = &chunk->vertexList[x + 17 * y];
      for (UINT vertex = 0; vertex < 5; ++vertex) {
        int                       vi = s_vertexIndexFlat[vertex];
        const NTempest::C3Vector &p = v[vi];
        UINT                      mask = 0;
        if (p.x < localBox.b.x - 0.019444443f)
          mask |= 0x01;
        if (p.y < localBox.b.y - 0.019444443f)
          mask |= 0x02;
        if (p.z < localBox.b.z - 0.019444443f)
          mask |= 0x04;
        if (p.x > localBox.t.x + 0.019444443f)
          mask |= 0x08;
        if (p.y > localBox.t.y + 0.019444443f)
          mask |= 0x10;
        if (p.z > localBox.t.z + 0.019444443f)
          mask |= 0x20;
        culled[vi] = mask;
      }

      for (UINT triangle = 0; triangle < 4; ++triangle) {
        int i0 = s_vertexIndex[triangle][0];
        int i1 = s_vertexIndex[triangle][1];
        int i2 = s_vertexIndex[triangle][2];
        if (culled[i0] & culled[i1] & culled[i2]) {
          continue;
        }
        if (!batch) {
          batch = triData.AllocBatch();
          NTempest::C44Matrix *matrix = triData.AllocMatrix();
          *matrix = NTempest::C44Matrix();
          matrix->Translate(chunk->corner);
          batch->matrix = matrix;
          batch->vertices = chunk->vertexList;
          batch->normals = chunk->normalList;
          batch->sourceID = reinterpret_cast<DWORD>(chunk);
          UINT maxIndices = (scRect.b - scRect.t + 1) * (scRect.r - scRect.l + 1) * 12;
          indices = triData.AllocVertexIndices(maxIndices);
          batch->vertexIndices = indices;
        }

        int       base = x + 17 * y;
        const int triangleIndices[3] = {i0, i1, i2};
        for (UINT corner = 0; corner < 3; ++corner) {
          WORD index = static_cast<WORD>(base + triangleIndices[corner]);
          indices[indexCount++] = index;
          if (index < batch->minIndex)
            batch->minIndex = index;
          if (index > batch->maxIndex)
            batch->maxIndex = index;
        }
        batch->indexCount += 3;
        ++batch->triCount;
      }
    }
  }
  return batch != 0;
}

bool CMap::GetChunkFacets(int cx, int cy, NTempest::CiRect &sRect, const NTempest::CAaBox &aaBox, CWFacetData *facetData, UINT queryFlags) {
  FATALASSERT(facetData);

  UINT      origFacetCount = facetData->facets.Count();
  UINT      areaIndex = ((cy >> 4) & 0x3F) * 64 + ((cx >> 4) & 0x3F);
  CMapArea *area = areaTable[areaIndex];
  if (!area) {
    return 0;
  }

  UINT       chunkIndex = (cy & 0xF) * 16 + (cx & 0xF);
  CMapChunk *chunk = area->chunkTable[chunkIndex];
  if (!chunk) {
    return 0;
  }

  NTempest::CiRect scRect(sRect.t - 8 * cy, sRect.l - 8 * cx, sRect.b - 8 * cy, sRect.r - 8 * cx);
  if (scRect.t < 0) {
    scRect.t = 0;
  }
  if (scRect.l < 0) {
    scRect.l = 0;
  }
  if (scRect.b >= 8) {
    scRect.b = 7;
  }
  if (scRect.r >= 8) {
    scRect.r = 7;
  }

  NTempest::CAaBox localAaBox = aaBox;
  localAaBox.Offset(-chunk->corner);
  UINT                culled[19];
  NTempest::C4Plane  *p = &chunk->planeList[4 * (scRect.l + 8 * scRect.t)];
  NTempest::C3Vector *v = &chunk->vertexList[scRect.l + 17 * scRect.t];

  for (int y = scRect.t; y <= scRect.b; ++y) {
    for (int x = scRect.l; x <= scRect.r; ++x, ++v) {
      if (chunk->holes & g_holeMask[y >> 1][x >> 1]) {
        continue;
      }

      FATALASSERT(s_vertexIndex[2][2] == 18);
      for (UINT index = 0; index < 5; ++index) {
        int                       vertexIndex = s_vertexIndexFlat[index];
        const NTempest::C3Vector &vertex = v[vertexIndex];
        float                     cmp = vertex.x - localAaBox.b.x + 0.019444443f;
        culled[vertexIndex] = *reinterpret_cast<UINT *>(&cmp) >> 31;
        cmp = vertex.y - localAaBox.b.y + 0.019444443f;
        culled[vertexIndex] |= (*reinterpret_cast<UINT *>(&cmp) >> 30) & 0x02;
        cmp = vertex.z - localAaBox.b.z + 0.019444443f;
        culled[vertexIndex] |= (*reinterpret_cast<UINT *>(&cmp) >> 29) & 0x04;
        cmp = localAaBox.t.x - vertex.x + 0.019444443f;
        culled[vertexIndex] |= (*reinterpret_cast<UINT *>(&cmp) >> 28) & 0x08;
        cmp = localAaBox.t.y - vertex.y + 0.019444443f;
        culled[vertexIndex] |= (*reinterpret_cast<UINT *>(&cmp) >> 27) & 0x10;
        cmp = localAaBox.t.z - vertex.z + 0.019444443f;
        culled[vertexIndex] |= (*reinterpret_cast<UINT *>(&cmp) >> 26) & 0x20;
      }

      for (int triangle = 0; triangle < 4; ++triangle, ++p) {
        if (culled[s_vertexIndex[triangle][0]] & (culled[s_vertexIndex[triangle][1]] & culled[s_vertexIndex[triangle][2]])) {
          if (CWorld::enables & 0x200000) {
            NTempest::CFacet facet;
            facet.plane = *p;
            facet.vertices[0] = v[s_vertexIndex[triangle][0]] + chunk->corner;
            facet.vertices[1] = v[s_vertexIndex[triangle][1]] + chunk->corner;
            facet.vertices[2] = v[s_vertexIndex[triangle][2]] + chunk->corner;
            TestQueryAdd(facet, NTempest::CImVector(0x80FF0000), 0);
          }
        } else {
          NTempest::CFacet *facet = facetData->facets.New();
          FATALASSERT(facet);
          facet->plane = *p;
          facet->plane.d -= chunk->corner.z * facet->plane.n.z + chunk->corner.y * facet->plane.n.y + chunk->corner.x * facet->plane.n.x;
          facet->vertices[0] = v[s_vertexIndex[triangle][0]] + chunk->corner;
          facet->vertices[1] = v[s_vertexIndex[triangle][1]] + chunk->corner;
          facet->vertices[2] = v[s_vertexIndex[triangle][2]] + chunk->corner;
          if (CWorld::enables & 0x200000) {
            TestQueryAdd(*facet, NTempest::CImVector(0x8000FF00), 0);
          }
        }
      }
    }
    v += 17 - (scRect.r - scRect.l + 1);
    p += 4 * (8 - (scRect.r - scRect.l + 1));
  }

  UINT facetCount = facetData->facets.Count();
  facetData->gameObjects.SetCount(facetCount);
  if (facetCount != origFacetCount) {
    memset(&facetData->gameObjects[origFacetCount], 0, (facetCount - origFacetCount) * sizeof(facetData->gameObjects[0]));
  }

  if (queryFlags & 1) {
    ITERATELIST(CMapBaseObjLink, chunk->doodadDefLinkList, link) {
      CMapDoodadDef *doodadDef = static_cast<CMapDoodadDef *>(link->owner);
      FATALASSERT(doodadDef);

      if (doodadDef->cCount != cCount && doodadDef->model) {
        NTempest::CAaBox collideExt;
        doodadDef->GetCollideExt(collideExt);
        if (collideExt.b <= aaBox.t && collideExt.t >= aaBox.b) {
          AddDoodadFacets(aaBox, doodadDef, facetData);
          doodadDef->cCount = cCount;
        }
      }
    }

    if (entityCollisionHandler) {
      ITERATELIST(CMapBaseObjLink, chunk->entityLinkList, link) {
        CMapEntity *entity = static_cast<CMapEntity *>(link->owner);
        if (!entity->flagCollidable) {
          continue;
        }

        WorldObjCollisionHandlerData data;
        if (entityCollisionHandler(entity->param64, entity->param32, &data) && data.collideExt.b <= aaBox.t && data.collideExt.t >= aaBox.b) {
          AddGameObjFacets(aaBox, data, entity->param64, facetData);
        }
      }
    }
  }

  return origFacetCount != facetData->facets.Count();
}

static void CreateFacet(CWFacetData *facetData, NTempest::C3Vector &corner, NTempest::C3Vector &normal, NTempest::C3Vector &up, NTempest::C3Vector &right) {
  NTempest::CFacet *facet = facetData->facets.NewElement();
  facet->plane.n = normal;
  facet->plane.d = -NTempest::C3Vector::Dot(normal, corner);
  facet->vertices[0] = corner;
  facet->vertices[1] = corner + right;
  facet->vertices[2] = corner + right + up;

  facet = facetData->facets.NewElement();
  facet->plane.n = normal;
  facet->plane.d = -NTempest::C3Vector::Dot(normal, corner);
  facet->vertices[0] = corner;
  facet->vertices[1] = corner + right + up;
  facet->vertices[2] = corner + up;
}

void CMap::CreateImpassableFacets(CMapChunk *chunk, const NTempest::CAaBox &aaBox, CWFacetData *facetData, UINT queryFlags) {
  NTempest::C3Vector up(0.0f, 0.0f, 10000.0f);
  NTempest::C3Vector normal;
  NTempest::C3Vector right;
  NTempest::C3Vector corner;

  if (aaBox.b.y < chunk->aaBox.b.y) {
    corner = chunk->aaBox.b;
    normal = NTempest::C3Vector(0.0f, -1.0f, 0.0f);
    right = NTempest::C3Vector(chunk->aaBox.t.x - chunk->aaBox.b.x, 0.0f, 0.0f);
    CreateFacet(facetData, corner, normal, up, right);
  }

  if (aaBox.t.y > chunk->aaBox.t.y) {
    corner = NTempest::C3Vector(chunk->aaBox.t.x, chunk->aaBox.t.y, chunk->aaBox.b.z);
    normal = NTempest::C3Vector(0.0f, 1.0f, 0.0f);
    right = NTempest::C3Vector(chunk->aaBox.b.x - chunk->aaBox.t.x, 0.0f, 0.0f);
    CreateFacet(facetData, corner, normal, up, right);
  }

  if (aaBox.b.x < chunk->aaBox.b.x) {
    corner = NTempest::C3Vector(chunk->aaBox.b.x, chunk->aaBox.t.y, chunk->aaBox.b.z);
    normal = NTempest::C3Vector(-1.0f, 0.0f, 0.0f);
    right = NTempest::C3Vector(0.0f, chunk->aaBox.b.y - chunk->aaBox.t.y, 0.0f);
    CreateFacet(facetData, corner, normal, up, right);
  }

  if (aaBox.t.x > chunk->aaBox.t.x) {
    corner = NTempest::C3Vector(chunk->aaBox.t.x, chunk->aaBox.b.y, chunk->aaBox.b.z);
    normal = NTempest::C3Vector(1.0f, 0.0f, 0.0f);
    right = NTempest::C3Vector(0.0f, chunk->aaBox.t.y - chunk->aaBox.b.y, 0.0f);
    CreateFacet(facetData, corner, normal, up, right);
  }
}

bool CMap::GetFacets(const CWFrustum &frustum, CWFacetData *facetData, UINT queryFlags) {
  FATALASSERT(facetData);

  ++cCount;
  facetData->facets.SetCount(0);
  facetData->facets.SetChunkSize(32);

  NTempest::CAaBox faab = NTempest::CAaBox::Bounding(frustum.corners, 8);
  NTempest::CRect  tLocation(-(faab.t.x - 17066.666f), -(faab.t.y - 17066.666f), -(faab.b.x - 17066.666f), -(faab.b.y - 17066.666f));
  FATALASSERT(tLocation.minx >= 0.0f && tLocation.miny >= 0.0f);
  FATALASSERT(tLocation.maxy < ((64*16)*((150.0f/36.0f)*8)) && tLocation.maxy < ((64*16)*((150.0f/36.0f)*8)));

  NTempest::CiRect sRect(
      Fast_ftol(OO_COORD_TO_SUBCHUNK * tLocation.t), Fast_ftol(OO_COORD_TO_SUBCHUNK * tLocation.l),
      Fast_ftol(OO_COORD_TO_SUBCHUNK * tLocation.b), Fast_ftol(OO_COORD_TO_SUBCHUNK * tLocation.r)
  );
  NTempest::CiRect cRect(sRect.t >> 3, sRect.l >> 3, sRect.b >> 3, sRect.r >> 3);

  for (int cy = cRect.t; cy <= cRect.b; ++cy) {
    for (int cx = cRect.l; cx <= cRect.r; ++cx) {
      GetChunkFacets(cx, cy, sRect, frustum, facetData);
    }
  }

  if (queryFlags & 0xF0) {
    GetFacetsMapObjs(frustum, facetData, queryFlags);
  }

  return facetData->facets.Count() > 0;
}

bool CMap::GetChunkFacets(int cx, int cy, const NTempest::CiRect &sRect, const CWFrustum &wFrustum, CWFacetData *facetData) {
  FATALASSERT(facetData);

  UINT      origFacetCount = facetData->facets.Count();
  UINT      areaIndex = ((cy >> 4) & 0x3F) * 64 + ((cx >> 4) & 0x3F);
  CMapArea *area = areaTable[areaIndex];
  if (!area) {
    return false;
  }

  UINT       chunkIndex = (cy & 0xF) * 16 + (cx & 0xF);
  CMapChunk *chunk = area->chunkTable[chunkIndex];
  if (!chunk) {
    return false;
  }

  NTempest::CiRect scRect(sRect.t - 8 * cy, sRect.l - 8 * cx, sRect.b - 8 * cy, sRect.r - 8 * cx);
  if (scRect.t < 0)
    scRect.t = 0;
  if (scRect.l < 0)
    scRect.l = 0;
  if (scRect.b > 7)
    scRect.b = 7;
  if (scRect.r > 7)
    scRect.r = 7;

  CWFrustum lFrustum;
  lFrustum = wFrustum;
  lFrustum.Translate(-chunk->corner);

  UINT culled[19];
  for (int y = scRect.t; y <= scRect.b; ++y) {
    for (int x = scRect.l; x <= scRect.r; ++x) {
      if (chunk->holes & g_holeMask[y >> 1][x >> 1]) {
        continue;
      }

      FATALASSERT(s_vertexIndex[2][2] == 18);
      NTempest::C3Vector *v = &chunk->vertexList[x + 17 * y];
      for (UINT index = 0; index < 5; ++index) {
        int vertexIndex = s_vertexIndexFlat[index];
        lFrustum.Cull(v[vertexIndex], culled[vertexIndex]);
      }

      NTempest::C4Plane *p = &chunk->planeList[4 * (x + 8 * y)];
      for (UINT triangle = 0; triangle < 4; ++triangle, ++p) {
        int i0 = s_vertexIndex[triangle][0];
        int i1 = s_vertexIndex[triangle][1];
        int i2 = s_vertexIndex[triangle][2];
        if (culled[i0] & culled[i1] & culled[i2]) {
          continue;
        }

        NTempest::CFacet *facet = facetData->facets.NewElement();
        facet->plane = *p;
        facet->plane.d -= NTempest::C3Vector::Dot(p->n, chunk->corner);
        facet->vertices[0] = v[i0] + chunk->corner;
        facet->vertices[1] = v[i1] + chunk->corner;
        facet->vertices[2] = v[i2] + chunk->corner;
      }
    }
  }

  NTempest::CAaBox frustumBox = NTempest::CAaBox::Bounding(wFrustum.corners, 8);
  ITERATELIST(CMapBaseObjLink, chunk->doodadDefLinkList, link) {
    CMapDoodadDef *doodadDef = static_cast<CMapDoodadDef *>(link->owner);
    FATALASSERT(doodadDef);
    if ((doodadDef->flags & CMapBaseObj::Flag_NoCollision) || doodadDef->cCount == cCount || !doodadDef->model) {
      continue;
    }

    NTempest::CAaBox collideExt;
    doodadDef->GetCollideExt(collideExt);
    if (collideExt.b <= frustumBox.t && collideExt.t >= frustumBox.b) {
      ModelAddCollisionFacets(doodadDef->model, doodadDef->mat, doodadDef->scale, frustumBox, &facetData->facets);
      doodadDef->cCount = cCount;
    }
  }

  return origFacetCount != facetData->facets.Count();
}

bool CMap::GetFacetsMapObjs(const CWFrustum &frustum, CWFacetData *facetData, UINT queryFlags) {
  bool hit = false;

  ITERATELIST(CMapObjDef, mapObjDefHash, mapObjDef) {
    if (mapObjDef->flags & CMapBaseObj::Flag_NoCollision || !frustum.Cull(mapObjDef->aaBox)) {
      continue;
    }

    NTempest::C3Vector moCorners[8];
    for (UINT i = 0; i < 8; ++i) {
      moCorners[i] = frustum.corners[i] * mapObjDef->invMat;
    }

    CWFrustum moFrustum(moCorners);
    CMapObj  *mapObj = mapObjDef->mapObj;
    if (mapObj) {
      CWTriData triData;
      hit |= mapObj->GetTris(triData, moFrustum, mapObjDef, queryFlags);
      CWorld::TriDataToFacetData(triData, *facetData, 0);
    }
  }

  return hit;
}
