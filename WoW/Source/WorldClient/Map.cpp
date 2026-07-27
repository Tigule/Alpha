#include "WorldClient/World.h"

#include "WorldClient/CMapObj.h"
#include "WorldClient/CSimpleDoodad.h"
#include "WorldClient/DetailDoodad.h"
#include "WorldCommon/WorldMath.h"

#include "Model/CollisionData.h"
#include "Model/IModel.h"
#include "Tempest/cfacet.h"

#include <string.h>

static const float OO_COORD_TO_SUBCHUNK = 1.0f / (150.0f / 36.0f);

void AddDoodadFacets(NTempest::CAaBox &aaBox, CMapDoodadDef *doodadDef, CWFacetData *facetData);
void
AddGameObjFacets(NTempest::CAaBox &aaBox, WorldObjCollisionHandlerData &data, unsigned __int64 guid, CWFacetData *facetData);

unsigned int g_holeMask[4][4] = {
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

static unsigned short idxoffs[36] = {0, 1, 2, 0, 2, 3, 0, 4, 5, 0, 5, 1, 3, 7, 4, 3, 4, 0, 2, 6, 7, 2, 7, 3, 2, 6, 5, 2, 5, 1, 4, 5, 6, 4, 6, 7};

static int s_vertexIndex[4][3] = {
    {17,  9,  0},
    { 9,  1,  0},
    { 9, 17, 18},
    { 9, 18,  1}
};

static int s_vertexIndexFlat[5] = {9, 17, 1, 18, 0};

unsigned int                       CMap::uniqueId;
bool                               CMap::enablePixelShaders;
bool                               CMap::enableSpecular;
bool                               CMap::enableSpecularTerrain;
bool                               CMap::enableTerrainShader;
bool                               CMap::enableSpecularWater;
CGxPixelShader                    *CMap::psSpecTerrain;
CGxShaderParam                    *CMap::psSpecTerrain_LayerMask;
CGxPixelShader                    *CMap::psTerrain;
CGxShaderParam                    *CMap::psTerrain_LayerMask;
CGxPixelShader                    *CMap::psSpecUTerrain;
CGxShaderParam                    *CMap::psSpecUTerrain_LayerMask;
CGxPixelShader                    *CMap::psUTerrain;
CGxShaderParam                    *CMap::psUTerrain_LayerMask;
CGxBuf                            *CMap::gxBufDynLowDetail;
SFile                             *CMap::wdtFile;
unsigned long                      CMap::version;
SMMapHeader                        CMap::header;
int                                CMap::bActive;
int                                CMap::bPreload;
int                                CMap::bDungeon;
SMAreaInfo                         CMap::areaInfo[4096];
CMapArea                          *CMap::areaTable[4096];
unsigned long                      CMap::areaLowOffsets[4096];
CMapAreaLow                       *CMap::areaLowTable[4096];
TSExplicitList<CMapBaseObjLink, 8> CMap::areaLinkList;
TSExplicitList<CMapBaseObjLink, 8> CMap::doodadDefLinkList;
TSExplicitList<CMapBaseObjLink, 8> CMap::mapObjDefLinkList;
HASHKEY_NONE                       CMap::nullHashKey;
TSGrowableArray<char>              CMap::doodadNames;
TSGrowableArray<unsigned int>      CMap::doodadNamesIndex;
TSGrowableArray<char>              CMap::mapObjNames;
TSGrowableArray<unsigned int>      CMap::mapObjNamesIndex;
TSGrowableArray<unsigned int>      CMap::scCollideList;
unsigned int                       CMap::scCollideCnt;
unsigned int                       CMap::mapGetFacetsCount;
int(*CMap::entityHandler)(void *, unsigned long, unsigned __int64, unsigned long);
void *CMap::entityHandlerParam;
int(*CMap::entityCollisionHandler)(unsigned __int64, unsigned long, WorldObjCollisionHandlerData *);
TSGrowableArray<CGxVertexPC>    CMap::testQueryVerts;
TSGrowableArray<unsigned short> CMap::testQueryIndices;
unsigned int                    CMap::cCount;
unsigned int                    CMap::bspRecurseCount;
unsigned int                    CMap::nChunksPrepared;
unsigned int                    CMap::nGbChunksPrepared;
void                           *CMap::oldSelectLightParm;
CMapLight                      *CMap::sunLight;
char                            CMap::wdtFilename[256];
char                            CMap::wobFilename[256];
char                            CMap::mapPath[256];
char                            CMap::mapName[256];

void CMap::ProjectLights() {
}

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

  for (unsigned int i = 0; i < 4096; ++i) {
    areaInfo[i].offset = 0;
    areaInfo[i].size = 0;
    areaInfo[i].flags = 0;
  }

  scCollideList.SetCount(2048);
  scCollideCnt = 0;
  cCount = 0;
  uniqueId = static_cast<unsigned int>(-2);
  bDungeon = 0;
  bActive = 0;
  bPreload = 0;
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
}

void CMap::GetCounts(int *const counts) {
  memcpy(counts, counts, 11 * sizeof(counts[0]));
  memcpy(counts + 11, freeCounts, 11 * sizeof(freeCounts[0]));
}

unsigned long CMap::GetTextureUseage() {
  CMapBaseObjLink *areaLinknext_node;
  CMapBaseObjLink *chunkLinknext_node;
  CMapArea        *area;
  CMapChunk       *chunk;
  unsigned int     texUseage = 0;

  for (CMapBaseObjLink *areaLink = areaLinkList.Head(); areaLink; areaLink = areaLinknext_node) {
    areaLinknext_node = areaLinkList.Next(areaLink);
    area = static_cast<CMapArea *>(areaLink->owner);
    ASSERT(area);

    for (CMapBaseObjLink *chunkLink = area->chunkLinkList.Head(); chunkLink; chunkLink = chunkLinknext_node) {
      chunkLinknext_node = area->chunkLinkList.Next(chunkLink);
      chunk = static_cast<CMapChunk *>(chunkLink->owner);
      ASSERT(chunk);

      if (chunk->shadowTexture) {
        texUseage += CWorld::shadowMipLevel ? 0x800 : 0x2000;
      }

      for (unsigned int i = 0; i < chunk->nLayers; ++i) {
        CChunkLayer *layer = chunk->layerList[i];
        ASSERT(layer);

        if (layer->props & 0x0100) {
          texUseage += CWorld::alphaMipLevel ? 0x800 : 0x2000;
        }
      }
    }
  }

  return texUseage;
}

void CMap::ClearDetailDoodads() {
  CMapChunk *chunk;

  for (chunk = chunkList.Head(); chunk; chunk = chunkList.Next(chunk)) {
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
  ASSERT(mx < ((64 * 16) * ((150.0f / 36.0f) * 8)) && my < ((64 * 16) * ((150.0f / 36.0f) * 8)));

  mx *= 0.24f;
  my *= 0.24f;
  int       mxIndex = static_cast<int>(mx - 0.5f);
  int       myIndex = static_cast<int>(my - 0.5f);
  CMapArea *area = areaTable[64 * ((myIndex >> 7) & 0x3F) + ((mxIndex >> 7) & 0x3F)];
  if (!area) {
    return 0.0f;
  }

  CMapChunk *chunk = area->chunkTable[((mxIndex >> 3) & 0xF) + 16 * ((myIndex >> 3) & 0xF)];
  if (!chunk) {
    return 0.0f;
  }

  int x = mxIndex & 7;
  int y = myIndex & 7;
  if (chunk->holes & g_holeMask[y >> 1][x >> 1]) {
    return 0.0f;
  }

  NTempest::C3Vector *v = &chunk->vertexList[x + 17 * y];
  float               lx = wx - chunk->corner.x;
  float               ly = wy - chunk->corner.y;
  unsigned int        triangle = (v[18].x - v[0].x) * (v[18].y - ly) - (v[18].y - v[0].y) * (v[18].x - lx) <= 0.0f;
  if ((v[17].x - v[1].x) * (v[17].y - ly) - (v[17].y - v[1].y) * (v[17].x - lx) <= 0.0f) {
    triangle += 2;
  }

  NTempest::C4Plane &p = chunk->planeList[4 * (x + 8 * y) + triangle];
  return chunk->corner.z - (ly * p.n.y + lx * p.n.x + p.d) / p.n.z;
}

void CMap::TestQueryAdd(const NTempest::CFacet &facet, NTempest::CImVector color, const NTempest::C44Matrix *basis) {
  NTempest::C44Matrix        id;
  unsigned int               sub = testQueryVerts.Count();
  const NTempest::C44Matrix *mtx = basis ? basis : &id;

  for (unsigned int i = 0; i < 3; ++i) {
    CGxVertexPC *v = testQueryVerts.NewElement();
    v->p = facet.vertices[i] * *mtx;
    v->c = color;
  }

  testQueryIndices.Add(reinterpret_cast<unsigned short *>(&sub));
  ++sub;
  testQueryIndices.Add(reinterpret_cast<unsigned short *>(&sub));
  ++sub;
  testQueryIndices.Add(reinterpret_cast<unsigned short *>(&sub));
}

bool CMap::VectorIntersectTerrain(
    const NTempest::C3Vector *p0,
    const NTempest::C3Vector *p1,
    float                    *t,
    unsigned int              queryFlags,
    CMapChunk               **chunk
) {
  NTempest::C2Vector v0(17066.666f - p0->y, 17066.666f - p0->x);
  NTempest::C2Vector v1(17066.666f - p1->y, 17066.666f - p1->x);
  float              dx = v1.x - v0.x;
  float              dy = v1.y - v0.y;
  NTempest::CiRect   sRect(
      NTempest::CMath::fint_mi(v0.y * OO_COORD_TO_SUBCHUNK),
      NTempest::CMath::fint_mi(v0.x * OO_COORD_TO_SUBCHUNK),
      NTempest::CMath::fint_mi(v1.y * OO_COORD_TO_SUBCHUNK),
      NTempest::CMath::fint_mi(v1.x * OO_COORD_TO_SUBCHUNK)
  );

  scCollideCnt = 0;

  if (NTempest::CMath::fabs_(dx) < 2.3841858e-7f || sRect.l == sRect.r) {
    VectorIntersectSY(sRect);
  } else if (NTempest::CMath::fabs_(dy) < 2.3841858e-7f || sRect.t == sRect.b) {
    VectorIntersectSX(sRect);
  } else if (NTempest::CMath::fabs_(dx) > NTempest::CMath::fabs_(dy)) {
    VectorIntersectDX(NTempest::C3Vector(v0.x, v0.y, 0.0f), NTempest::C3Vector(v1.x, v1.y, 0.0f), sRect);
  } else {
    VectorIntersectDY(NTempest::C3Vector(v0.x, v0.y, 0.0f), NTempest::C3Vector(v1.x, v1.y, 0.0f), sRect);
  }

  return VectorIntersectSubchunks(p0, p1, t, queryFlags, chunk);
}

bool CMap::VectorIntersect(
    const NTempest::C3Vector *p0,
    const NTempest::C3Vector *p1,
    NTempest::C3Vector       *ip,
    float                    *dist,
    unsigned int              queryFlags
) {
  FATALASSERT(p0);
  FATALASSERT(p1);
  FATALASSERT(ip);
  FATALASSERT(dist);
  FATALASSERT(*dist >= 0.0f && *dist <= 1.0f);

  bool hit = false;
  ++cCount;

  if (queryFlags & 0xF0) {
    unsigned int polyIgnoreFlags = 0;
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
    unsigned int              queryFlags,
    unsigned int              polyIgnoreFlags,
    unsigned int              groupIgnoreFlags,
    float                    *t,
    SMOPoly                 **poly,
    CMapObj                 **qMapObj
) {
  FATALASSERT(*t >= 0.0f && *t <= 1.0f);

  bool hit = false;
  for (CMapObjDef *mapObjDef = mapObjDefHash.Head(); mapObjDef; mapObjDef = mapObjDefHash.Next(mapObjDef)) {
    if (mapObjDef->flags & CMapBaseObj::Flag_NoCollision) {
      continue;
    }

    NTempest::C3Vector v0 = *p0 * mapObjDef->invMat;
    NTempest::C3Vector v1 = *p1 * mapObjDef->invMat;
    CMapObj            *mapObj = mapObjDef->mapObj;
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

bool CMap::VectorIntersectDoodadDefLinkList(
    TSExplicitList<CMapBaseObjLink, 8> &doodadDefLinkList,
    const NTempest::C3Vector           *p0,
    const NTempest::C3Vector           *p1,
    float                              *t,
    unsigned int                        queryFlags
) {
  NTempest::C3Vector camrelP0 = *p0 - CWorldScene::camPos;
  NTempest::C3Vector camrelP1 = *p1 - CWorldScene::camPos;
  float              oovmag = 1.0f / (*p1 - *p0).Mag();
  float              hitT = *t;
  bool               hit = false;

  for (CMapBaseObjLink *link = doodadDefLinkList.Head(); link; link = doodadDefLinkList.Next(link)) {
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
      NTempest::C34Matrix basis(
          doodadDef->mat.a0, doodadDef->mat.a1, doodadDef->mat.a2,
          doodadDef->mat.b0, doodadDef->mat.b1, doodadDef->mat.b2,
          doodadDef->mat.c0, doodadDef->mat.c1, doodadDef->mat.c2,
          doodadDef->mat.d0, doodadDef->mat.d1, doodadDef->mat.d2
      );
      if (ModelCollisionVectorIntersect(doodadDef->model, basis, *p0, *p1, it)) {
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
    TSExplicitList<CMapBaseObjLink, 8> &gameObjLinkList,
    const NTempest::C3Vector           *p0,
    const NTempest::C3Vector           *p1,
    float                              *t,
    unsigned int                        queryFlags
) {
  if (!entityCollisionHandler) {
    return false;
  }

  NTempest::C3Vector camrelP0 = *p0 - CWorldScene::camPos;
  NTempest::C3Vector camrelP1 = *p1 - CWorldScene::camPos;
  float              oovmag = 1.0f / (*p1 - *p0).Mag();
  float              hitT = *t;
  bool               hit = false;

  for (CMapBaseObjLink *link = gameObjLinkList.Head(); link; link = gameObjLinkList.Next(link)) {
    CMapEntity *entity = static_cast<CMapEntity *>(link->owner);
    if (!entity->flagCollidable) {
      continue;
    }

    WorldObjCollisionHandlerData data;
    data.collideExt = NTempest::CAaBox(0.0f);
    data.matrix = NTempest::C44Matrix();
    if (!entityCollisionHandler(entity->param64, entity->param32, &data) ||
        !CWorldMath::VectorIntersectAABox2(data.collideExt, *p0, *p1))
    {
      continue;
    }

    float it;
    if (queryFlags & 1) {
      if (!data.model) {
        continue;
      }

      NTempest::C34Matrix basis(
          data.matrix.a0, data.matrix.a1, data.matrix.a2,
          data.matrix.b0, data.matrix.b1, data.matrix.b2,
          data.matrix.c0, data.matrix.c1, data.matrix.c2,
          data.matrix.d0, data.matrix.d1, data.matrix.d2
      );
      if (!ModelCollisionVectorIntersect(data.model, basis, *p0, *p1, it)) {
        continue;
      }
      if (it < hitT) {
        hitT = it;
      }
    } else {
      if (!ModelHitTestGeometry(data.model, data.scale, camrelP0, camrelP1, 0, &it)) {
        continue;
      }
      it *= oovmag;
      if (it > 1.0f || it >= hitT) {
        continue;
      }
      hitT = it;
    }
    hit = true;
  }

  if (hit) {
    *t = hitT;
  }
  return hit;
}

void CMap::VectorIntersectSX(NTempest::CiRect &sRect) {
  long sx = sRect.l;

  if (sx > sRect.r) {
    while (sx >= sRect.r) {
      scCollideList[scCollideCnt++] = sx;
      scCollideList[scCollideCnt++] = sRect.t;
      --sx;
    }
  } else if (sx <= sRect.r) {
    while (sx <= sRect.r) {
      scCollideList[scCollideCnt++] = sx;
      scCollideList[scCollideCnt++] = sRect.t;
      ++sx;
    }
  }
}

void CMap::VectorIntersectSY(NTempest::CiRect &sRect) {
  long sy = sRect.t;

  if (sy > sRect.b) {
    while (sy >= sRect.b) {
      scCollideList[scCollideCnt++] = sRect.l;
      scCollideList[scCollideCnt++] = sy;
      --sy;
    }
  } else if (sy <= sRect.b) {
    while (sy <= sRect.b) {
      scCollideList[scCollideCnt++] = sRect.l;
      scCollideList[scCollideCnt++] = sy;
      ++sy;
    }
  }
}

void CMap::VectorIntersectDX(const NTempest::C3Vector &p0, const NTempest::C3Vector &p1, NTempest::CiRect &sRect) {
  int   x = sRect.l;
  int   y = sRect.t;
  int   step;
  float m = (p1.y - p0.y) / (p1.x - p0.x);
  float b = p0.y - m * p0.x;
  float edge;

  if (x < sRect.r) {
    edge = (x + 1) * (150.0f / 36.0f);
    step = 1;
  } else {
    edge = x * (150.0f / 36.0f);
    step = -1;
  }

  scCollideList[scCollideCnt++] = x;
  scCollideList[scCollideCnt++] = y;

  while (x != sRect.r + step) {
    int sx = NTempest::CMath::fint_mi((edge * m + b) * OO_COORD_TO_SUBCHUNK);

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
  int   x = sRect.l;
  int   y = sRect.t;
  int   step;
  float m = (p1.y - p0.y) / (p1.x - p0.x);
  float b = p0.y - m * p0.x;
  float oom = 1.0f / m;
  float edge;

  if (y < sRect.b) {
    edge = (y + 1) * (150.0f / 36.0f);
    step = 1;
  } else {
    edge = y * (150.0f / 36.0f);
    step = -1;
  }

  scCollideList[scCollideCnt++] = x;
  scCollideList[scCollideCnt++] = y;

  while (y != sRect.b + step) {
    int sy = NTempest::CMath::fint_mi((edge - b) * oom * OO_COORD_TO_SUBCHUNK);

    if (sy != x) {
      scCollideList[scCollideCnt++] = sy;
      scCollideList[scCollideCnt++] = y;
    }

    scCollideList[scCollideCnt++] = sy;
    y += step;
    scCollideList[scCollideCnt++] = y;
    x = sy;
    edge += step * (150.0f / 36.0f);
  }

  if (x != sRect.r) {
    scCollideList[scCollideCnt++] = sRect.r;
    scCollideList[scCollideCnt++] = sRect.b;
  }
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

bool CMap::VectorIntersectSubchunks(
    const NTempest::C3Vector *p0,
    const NTempest::C3Vector *p1,
    float                    *t,
    unsigned int              queryFlags,
    CMapChunk               **retChunk
) {
  unsigned int *scPtr = scCollideList.Ptr();
  FATALASSERT(scPtr);

  unsigned int scCnt = scCollideCnt;
  unsigned int bMaskY = scPtr[0] & 0x2000;
  unsigned int bMaskX = scPtr[1] & 0x2000;
  CMapChunk   *chunk = 0;
  CMapChunk   *hitChunk = 0;
  float        hitT = *t;

  while (scCnt) {
    unsigned int sx = scPtr[0];
    unsigned int sy = scPtr[1];
    scPtr += 2;
    scCnt -= 2;

    if (sx > 0x2000 || sy > 0x2000) {
      return false;
    }

    if ((sx & 0x1FF8) != bMaskX || (sy & 0x1FF8) != bMaskY) {
      CMapArea *area = areaTable[64 * ((sy >> 7) & 0x3F) + ((sx >> 7) & 0x3F)];
      if (!area) {
        return false;
      }

      chunk = area->chunkTable[((sx >> 3) & 0xF) + 16 * ((sy >> 3) & 0xF)];
      if (!chunk) {
        return false;
      }

      bMaskY = sy & 0x1FF8;
      bMaskX = sx & 0x1FF8;

      if (queryFlags & 0xF) {
        float thisHitT = 1.0f;
        if (VectorIntersectDoodadDefLinkList(chunk->doodadDefLinkList, p0, p1, &thisHitT, queryFlags) && thisHitT < hitT) {
          hitT = thisHitT;
        }
        if (VectorIntersectGameObjLinkList(chunk->entityLinkList, p0, p1, &thisHitT, queryFlags) && thisHitT < hitT) {
          hitT = thisHitT;
        }
      }
    }

    unsigned int x = sx & 7;
    unsigned int y = sy & 7;
    NTempest::C3Vector lp0 = *p0 - chunk->corner;
    NTempest::C3Vector lp1 = *p1 - chunk->corner;

    if (chunk->holes & g_holeMask[y >> 1][x >> 1]) {
      continue;
    }

    NTempest::C3Vector *v = &chunk->vertexList[x + 17 * y];
    NTempest::C4Plane  *p = &chunk->planeList[4 * (x + 8 * y)];
    int                *indices = &iIndiciesP[0][0];

    while (indices < reinterpret_cast<int *>(idxoffs)) {
      float ip0 = NTempest::C3Vector::Dot(p->n, lp0) + p->d;
      float ip1 = NTempest::C3Vector::Dot(p->n, lp1) + p->d;

      if ((ip0 <= 0.0f || ip1 <= 0.0f) && (ip0 >= 0.0f || ip1 >= 0.0f)) {
        float it = ip0 / (ip0 - ip1);
        if (it >= 0.0f && it <= 1.0f) {
          NTempest::C3Vector tempIp = lp0 + (lp1 - lp0) * it;
          if (VectorIntersectTri(&tempIp, &v[9], &v[indices[0]], &v[indices[1]], &p->n) && it < hitT) {
            hitT = it;
            hitChunk = chunk;
          }
        }
      }

      ++p;
      indices += 2;
    }
  }

  if (hitT >= *t) {
    return false;
  }

  *t = hitT;
  if (retChunk) {
    *retChunk = hitChunk;
  }
  return true;
}

bool CMap::GetFacet(const NTempest::C3Segment &seg, float &t, NTempest::C4Plane &facet, unsigned int queryFlags) {
  if (queryFlags & 0xF0F) {
    return GetFacetTerrain(seg, t, facet, queryFlags);
  }
  return false;
}

bool CMap::GetFacetTerrain(const NTempest::C3Segment &seg, float &t, NTempest::C4Plane &facet, unsigned int queryFlags) {
  NTempest::C3Segment nb;
  NTempest::C2Vector  v0(17066.666f - seg.start.y, 17066.666f - seg.start.x);
  NTempest::C2Vector  v1(17066.666f - seg.end.y, 17066.666f - seg.end.x);
  float               dy = v1.y - v0.y;
  float               dx = v1.x - v0.x;
  NTempest::CiRect    sRect(
      NTempest::CMath::fint_mi(v0.y * OO_COORD_TO_SUBCHUNK), NTempest::CMath::fint_mi(v0.x * OO_COORD_TO_SUBCHUNK),
      NTempest::CMath::fint_mi(v1.y * OO_COORD_TO_SUBCHUNK), NTempest::CMath::fint_mi(v1.x * OO_COORD_TO_SUBCHUNK)
  );

  scCollideCnt = 0;

  if (NTempest::CMath::fabs_(dx) < 2.3841858e-7f || sRect.l == sRect.r) {
    VectorIntersectSY(sRect);
  } else if (NTempest::CMath::fabs_(dy) < 2.3841858e-7f || sRect.t == sRect.b) {
    VectorIntersectSX(sRect);
  } else if (NTempest::CMath::fabs_(dx) > NTempest::CMath::fabs_(dy)) {
    VectorIntersectDX(NTempest::C3Vector(v0.x, v0.y, 0.0f), NTempest::C3Vector(v1.x, v1.y, 0.0f), sRect);
  } else {
    VectorIntersectDY(NTempest::C3Vector(v0.x, v0.y, 0.0f), NTempest::C3Vector(v1.x, v1.y, 0.0f), sRect);
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

bool CMap::GetFacetSubchunks(const NTempest::C3Segment &seg, float &t, NTempest::C4Plane &facet, unsigned int queryFlags) {
  unsigned int *scPtr = scCollideList.Ptr();
  ASSERT(scPtr);

  unsigned int scCnt = scCollideCnt;
  unsigned int bMaskY = scPtr[0] & 0x2000;
  unsigned int bMaskX = scPtr[1] & 0x2000;
  CMapChunk   *chunk = 0;
  unsigned int hit = 0;

  if (!scCnt) {
    return false;
  }

  do {
    unsigned int sx = scPtr[0];
    unsigned int sy = scPtr[1];
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

    unsigned int        x = sx & 7;
    unsigned int        y = sy & 7;
    NTempest::C3Segment localSeg;
    localSeg.start = seg.start - chunk->corner;
    localSeg.end = seg.end - chunk->corner;

    if (chunk->holes & g_holeMask[y >> 1][x >> 1]) {
      continue;
    }

    NTempest::C3Vector *v = &chunk->vertexList[x + 17 * y];
    NTempest::C4Plane  *p = &chunk->planeList[4 * (x + 8 * y)];
    int                *indices = &iIndiciesP[0][0];

    while (indices < reinterpret_cast<int *>(idxoffs)) {
      float ip0 = NTempest::C3Vector::Dot(p->n, localSeg.start) + p->d;
      float ip1 = NTempest::C3Vector::Dot(p->n, localSeg.end) + p->d;

      if ((ip0 <= 0.0f || ip1 <= 0.0f) && (ip0 >= 0.0f || ip1 >= 0.0f)) {
        float it = ip0 / (ip0 - ip1);

        if (it >= 0.0f && it <= 1.0f) {
          NTempest::C3Vector tempIp = localSeg.start + (localSeg.end - localSeg.start) * it;

          if (VectorIntersectTri(&tempIp, &v[9], &v[indices[0]], &v[indices[1]], &p->n)) {
            if (it < t) {
              t = it;
              facet = *p;
              hit = true;
            }
          }
        }
      }

      ++p;
      indices += 2;
    }
  } while (scCnt);

  return hit;
}

unsigned int CMap::GetTrisTerrain(NTempest::CAaBox &aaBox, CWTriData &triData, unsigned int queryFlags) {
  NTempest::CRect tLocation(17066.666f - aaBox.t.x, 17066.666f - aaBox.t.y, 17066.666f - aaBox.b.x, 17066.666f - aaBox.b.y);
  FATALASSERT(tLocation.l >= 0.0f && tLocation.t >= 0.0f);
  FATALASSERT(tLocation.r < 34133.332f && tLocation.b < 34133.332f);

  NTempest::CiRect sRect(
      static_cast<int>(tLocation.t * OO_COORD_TO_SUBCHUNK - 0.5f), static_cast<int>(tLocation.l * OO_COORD_TO_SUBCHUNK - 0.5f),
      static_cast<int>(tLocation.b * OO_COORD_TO_SUBCHUNK - 0.5f), static_cast<int>(tLocation.r * OO_COORD_TO_SUBCHUNK - 0.5f)
  );
  NTempest::CiRect cRect(sRect.t >> 3, sRect.l >> 3, sRect.b >> 3, sRect.r >> 3);

  unsigned int got = 0;
  for (int cy = cRect.t; cy <= cRect.b; ++cy) {
    for (int cx = cRect.l; cx <= cRect.r; ++cx) {
      got |= GetTrisChunk(cx, cy, sRect, aaBox, triData, queryFlags);
    }
  }
  return got;
}

unsigned int CMap::GetTris(NTempest::CAaBox &aaBox, CWTriData &triData, unsigned int queryFlags) {
  unsigned int got = 0;
  if (queryFlags & 0xF0) {
    got = GetTrisMapObjs(aaBox, triData, queryFlags);
  }
  if (queryFlags & 0xF00) {
    got |= GetTrisTerrain(aaBox, triData, queryFlags);
  }
  return got;
}

unsigned int CMap::GetFacets(NTempest::CAaBox &aaBox, CWFacetData *facetData, unsigned int queryFlags) {
  FATALASSERT(facetData);

  ++mapGetFacetsCount;
  ++cCount;
  facetData->facets.SetCount(0);
  facetData->facets.SetChunkSize(32);

  if (queryFlags & 0xF0) {
    GetFacetsMapObjs(aaBox, facetData, queryFlags);
  }

  NTempest::CRect tLocation(17066.666f - aaBox.t.x, 17066.666f - aaBox.t.y, 17066.666f - aaBox.b.x, 17066.666f - aaBox.b.y);

  if (tLocation.l < 0.0f || tLocation.t < 0.0f || tLocation.r >= 34133.332f || tLocation.b >= 34133.332f) {
    FATALERROR(("Request for facets off edge of map: (%g,%g,%g,%g)", aaBox.b.x, aaBox.b.y, aaBox.t.x, aaBox.t.y));
  }

  NTempest::CiRect sRect(
      static_cast<int>(tLocation.t * OO_COORD_TO_SUBCHUNK - 0.5f), static_cast<int>(tLocation.l * OO_COORD_TO_SUBCHUNK - 0.5f),
      static_cast<int>(tLocation.b * OO_COORD_TO_SUBCHUNK - 0.5f), static_cast<int>(tLocation.r * OO_COORD_TO_SUBCHUNK - 0.5f)
  );
  NTempest::CiRect cRect(sRect.t >> 3, sRect.l >> 3, sRect.b >> 3, sRect.r >> 3);

  for (int cy = cRect.t; cy <= cRect.b; ++cy) {
    for (int cx = cRect.l; cx <= cRect.r; ++cx) {
      GetChunkFacets(cx, cy, sRect, aaBox, facetData, queryFlags);
    }
  }
  return facetData->facets.Count() != 0;
}

unsigned int CMap::GetFacetsMapObjs(NTempest::CAaBox &aaBox, CWFacetData *facetData, unsigned int queryFlags) {
  unsigned int origFacetCount = facetData->facets.Count();
  NTempest::C3Vector lCen = (aaBox.b + aaBox.t) * 0.5f;
  NTempest::CAaBox   lBox = aaBox;
  NTempest::C3Vector tCen(-lCen.x, -lCen.y, -lCen.z);
  lBox.b += tCen;
  lBox.t += tCen;

  for (CMapObjDef *mapObjDef = mapObjDefHash.Head(); mapObjDef; mapObjDef = mapObjDefHash.Next(mapObjDef)) {
    if (mapObjDef->flags & CMapBaseObj::Flag_NoCollision || !(aaBox.b <= mapObjDef->aaBox.t && aaBox.t >= mapObjDef->aaBox.b)) {
      continue;
    }

    tCen = lCen * mapObjDef->invMat;
    NTempest::C33Matrix tMat(
        mapObjDef->invMat.a0, mapObjDef->invMat.a1, mapObjDef->invMat.a2,
        mapObjDef->invMat.b0, mapObjDef->invMat.b1, mapObjDef->invMat.b2,
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

    for (CMapBaseObjLink *groupLink = mapObjDef->groupLinkList.Head(); groupLink; groupLink = mapObjDef->groupLinkList.Next(groupLink)) {
      CMapObjDefGroup *mapObjDefGroup = static_cast<CMapObjDefGroup *>(groupLink->owner);
      if (!mapObj->TestGroupBounds(tBox, mapObjDefGroup->groupNum)) {
        continue;
      }

      if ((queryFlags & 1) && !(queryFlags & 0x2000)) {
        for (CMapBaseObjLink *doodadLink = mapObjDefGroup->doodadDefLinkList.Head(); doodadLink;
             doodadLink = mapObjDefGroup->doodadDefLinkList.Next(doodadLink))
        {
          CMapDoodadDef *doodadDef = static_cast<CMapDoodadDef *>(doodadLink->owner);
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
        for (CMapBaseObjLink *entityLink = mapObjDefGroup->entityLinkList.Head(); entityLink;
             entityLink = mapObjDefGroup->entityLinkList.Next(entityLink))
        {
          CMapEntity *entity = static_cast<CMapEntity *>(entityLink->owner);
          if (!entity->flagCollidable) {
            continue;
          }

          WorldObjCollisionHandlerData data;
          if (entityCollisionHandler(entity->param64, entity->param32, &data) &&
              data.collideExt.b <= aaBox.t && data.collideExt.t >= aaBox.b)
          {
            AddGameObjFacets(aaBox, data, entity->param64, facetData);
          }
        }
      }
    }
  }

  return origFacetCount != facetData->facets.Count();
}

void AddDoodadFacets(NTempest::CAaBox &aaBox, CMapDoodadDef *doodadDef, CWFacetData *facetData) {
  unsigned int existing = facetData->facets.Count();
  ModelAddCollisionFacets(
      doodadDef->model,
      NTempest::C34Matrix(
          doodadDef->mat.a0, doodadDef->mat.a1, doodadDef->mat.a2, doodadDef->mat.b0, doodadDef->mat.b1, doodadDef->mat.b2, doodadDef->mat.c0,
          doodadDef->mat.c1, doodadDef->mat.c2, doodadDef->mat.d0, doodadDef->mat.d1, doodadDef->mat.d2
      ),
      doodadDef->scale, aaBox, &facetData->facets
  );

  unsigned int count = facetData->facets.Count();
  facetData->gameObjects.SetCount(count);
  if (count != existing) {
    memset(&facetData->gameObjects[existing], 0, (count - existing) * sizeof(facetData->gameObjects[0]));
  }
}

void AddGameObjFacets(NTempest::CAaBox &aaBox, WorldObjCollisionHandlerData &data, unsigned __int64 guid, CWFacetData *facetData) {
  unsigned int existing = facetData->facets.Count();
  ModelAddCollisionFacets(
      data.model,
      NTempest::C34Matrix(
          data.matrix.a0, data.matrix.a1, data.matrix.a2, data.matrix.b0, data.matrix.b1, data.matrix.b2, data.matrix.c0, data.matrix.c1,
          data.matrix.c2, data.matrix.d0, data.matrix.d1, data.matrix.d2
      ),
      data.scale, aaBox, &facetData->facets
  );

  unsigned int count = facetData->facets.Count();
  facetData->gameObjects.SetCount(count);
  for (unsigned int index = existing; index < count; ++index) {
    facetData->gameObjects[index] = guid;
  }
}

unsigned int CMap::GetTrisMapObjs(NTempest::CAaBox &aaBox, CWTriData &triData, unsigned int queryFlags) {
  NTempest::C3Vector lCen = (aaBox.b + aaBox.t) * 0.5f;
  NTempest::CAaBox   lBox = aaBox;
  NTempest::C3Vector tCen(-lCen.x, -lCen.y, -lCen.z);
  lBox.b += tCen;
  lBox.t += tCen;

  unsigned int got = 0;
  for (CMapObjDef *mapObjDef = mapObjDefHash.Head(); mapObjDef; mapObjDef = mapObjDefHash.Next(mapObjDef)) {
    if (mapObjDef->flags & CMapBaseObj::Flag_NoCollision) {
      continue;
    }

    tCen = lCen * mapObjDef->invMat;
    NTempest::C33Matrix tMat(
        mapObjDef->invMat.a0, mapObjDef->invMat.a1, mapObjDef->invMat.a2,
        mapObjDef->invMat.b0, mapObjDef->invMat.b1, mapObjDef->invMat.b2,
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

unsigned int CMap::GetTrisChunk(
    int               cx,
    int               cy,
    NTempest::CiRect &sRect,
    NTempest::CAaBox &aaBox,
    CWTriData        &triData,
    unsigned int      queryFlags
) {
  CMapArea *area = areaTable[((cy >> 4) & 0x3F) * 64 + ((cx >> 4) & 0x3F)];
  if (!area) {
    return 0;
  }
  CMapChunk *chunk = area->chunkTable[(cy & 0xF) * 16 + (cx & 0xF)];
  if (!chunk) {
    return 0;
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

  NTempest::CAaBox  localBox(aaBox.b - chunk->corner, aaBox.t - chunk->corner);
  CWTriData::Batch *batch = 0;
  unsigned short   *indices = 0;
  unsigned int      indexCount = 0;
  unsigned int      culled[19];

  for (int y = scRect.t; y <= scRect.b; ++y) {
    for (int x = scRect.l; x <= scRect.r; ++x) {
      if (chunk->holes & g_holeMask[y >> 1][x >> 1]) {
        continue;
      }
      NTempest::C3Vector *v = &chunk->vertexList[x + 17 * y];
      for (unsigned int vertex = 0; vertex < 5; ++vertex) {
        int                       vi = s_vertexIndexFlat[vertex];
        const NTempest::C3Vector &p = v[vi];
        unsigned int              mask = 0;
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

      for (unsigned int triangle = 0; triangle < 4; ++triangle) {
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
          batch->sourceID = reinterpret_cast<unsigned long>(chunk);
          unsigned int maxIndices = (scRect.b - scRect.t + 1) * (scRect.r - scRect.l + 1) * 12;
          indices = triData.AllocVertexIndices(maxIndices);
          batch->vertexIndices = indices;
        }

        int       base = x + 17 * y;
        const int triangleIndices[3] = {i0, i1, i2};
        for (unsigned int corner = 0; corner < 3; ++corner) {
          unsigned short index = static_cast<unsigned short>(base + triangleIndices[corner]);
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

unsigned int CMap::GetChunkFacets(
    int               cx,
    int               cy,
    NTempest::CiRect &sRect,
    NTempest::CAaBox &aaBox,
    CWFacetData      *facetData,
    unsigned int      queryFlags
) {
  FATALASSERT(facetData);

  unsigned int origFacetCount = facetData->facets.Count();
  unsigned int areaIndex = ((cy >> 4) & 0x3F) * 64 + ((cx >> 4) & 0x3F);
  CMapArea    *area = areaTable[areaIndex];
  if (!area) {
    return 0;
  }

  unsigned int chunkIndex = (cy & 0xF) * 16 + (cx & 0xF);
  CMapChunk   *chunk = area->chunkTable[chunkIndex];
  if (!chunk) {
    return 0;
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

  NTempest::CAaBox localAaBox(aaBox.b - chunk->corner, aaBox.t - chunk->corner);
  unsigned int     culled[19];

  for (int y = scRect.t; y <= scRect.b; ++y) {
    for (int x = scRect.l; x <= scRect.r; ++x) {
      if (chunk->holes & g_holeMask[y >> 1][x >> 1]) {
        continue;
      }

      NTempest::C3Vector *v = &chunk->vertexList[x + 17 * y];
      for (unsigned int index = 0; index < 5; ++index) {
        int                       vertexIndex = s_vertexIndexFlat[index];
        const NTempest::C3Vector &vertex = v[vertexIndex];
        unsigned int              mask = 0;
        if (vertex.x < localAaBox.b.x - 0.019444443f)
          mask |= 0x01;
        if (vertex.y < localAaBox.b.y - 0.019444443f)
          mask |= 0x02;
        if (vertex.z < localAaBox.b.z - 0.019444443f)
          mask |= 0x04;
        if (vertex.x > localAaBox.t.x + 0.019444443f)
          mask |= 0x08;
        if (vertex.y > localAaBox.t.y + 0.019444443f)
          mask |= 0x10;
        if (vertex.z > localAaBox.t.z + 0.019444443f)
          mask |= 0x20;
        culled[vertexIndex] = mask;
      }

      NTempest::C4Plane *p = &chunk->planeList[4 * (x + 8 * y)];
      for (unsigned int triangle = 0; triangle < 4; ++triangle, ++p) {
        int i0 = s_vertexIndex[triangle][0];
        int i1 = s_vertexIndex[triangle][1];
        int i2 = s_vertexIndex[triangle][2];
        if (culled[i0] & culled[i1] & culled[i2]) {
          continue;
        }

        NTempest::CFacet *facet = facetData->facets.NewElement();
        FATALASSERT(facet);
        facet->plane = *p;
        facet->plane.d -= NTempest::C3Vector::Dot(p->n, chunk->corner);
        facet->vertices[0] = v[i0] + chunk->corner;
        facet->vertices[1] = v[i1] + chunk->corner;
        facet->vertices[2] = v[i2] + chunk->corner;
      }
    }
  }

  unsigned int facetCount = facetData->facets.Count();
  facetData->gameObjects.SetCount(facetCount);
  if (facetCount != origFacetCount) {
    memset(&facetData->gameObjects[origFacetCount], 0, (facetCount - origFacetCount) * sizeof(facetData->gameObjects[0]));
  }

  if (queryFlags & 1) {
    for (CMapBaseObjLink *link = chunk->doodadDefLinkList.Head(); link; link = chunk->doodadDefLinkList.Next(link)) {
      CMapDoodadDef *doodadDef = static_cast<CMapDoodadDef *>(link->owner);
      FATALASSERT(doodadDef);

      if (doodadDef->cCount != cCount && doodadDef->model && doodadDef->collideExt.b.x <= aaBox.t.x && doodadDef->collideExt.b.y <= aaBox.t.y &&
          doodadDef->collideExt.b.z <= aaBox.t.z && doodadDef->collideExt.t.x >= aaBox.b.x && doodadDef->collideExt.t.y >= aaBox.b.y &&
          doodadDef->collideExt.t.z >= aaBox.b.z)
      {
        AddDoodadFacets(aaBox, doodadDef, facetData);
        doodadDef->cCount = cCount;
      }
    }

    if (entityCollisionHandler) {
      for (CMapBaseObjLink *link = chunk->entityLinkList.Head(); link; link = chunk->entityLinkList.Next(link)) {
        CMapEntity *entity = static_cast<CMapEntity *>(link->owner);
        if (!entity->flagCollidable) {
          continue;
        }

        WorldObjCollisionHandlerData data;
        if (entityCollisionHandler(entity->param64, entity->param32, &data) && data.collideExt.b.x <= aaBox.t.x && data.collideExt.b.y <= aaBox.t.y &&
            data.collideExt.b.z <= aaBox.t.z && data.collideExt.t.x >= aaBox.b.x && data.collideExt.t.y >= aaBox.b.y &&
            data.collideExt.t.z >= aaBox.b.z)
        {
          AddGameObjFacets(aaBox, data, entity->param64, facetData);
        }
      }
    }
  }

  return origFacetCount != facetData->facets.Count();
}

void
CreateFacet(CWFacetData *facetData, NTempest::C3Vector &corner, NTempest::C3Vector &normal, NTempest::C3Vector &up, NTempest::C3Vector &right) {
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

void CMap::CreateImpassableFacets(CMapChunk *chunk, NTempest::CAaBox &aaBox, CWFacetData *facetData, unsigned int queryFlags) {
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

unsigned int CMap::GetFacets(CWFrustum &frustum, CWFacetData *facetData, unsigned int queryFlags) {
  FATALASSERT(facetData);

  ++mapGetFacetsCount;
  ++cCount;
  facetData->facets.SetCount(0);
  facetData->facets.SetChunkSize(32);

  if (queryFlags & 0xF0) {
    GetFacetsMapObjs(frustum, facetData, queryFlags);
  }

  NTempest::CAaBox faab = NTempest::CAaBox::Bounding(frustum.corners, 8);
  NTempest::CRect  tLocation(17066.666f - faab.t.x, 17066.666f - faab.t.y, 17066.666f - faab.b.x, 17066.666f - faab.b.y);
  NTempest::CiRect sRect(
      static_cast<int>(tLocation.t * OO_COORD_TO_SUBCHUNK - 0.5f), static_cast<int>(tLocation.l * OO_COORD_TO_SUBCHUNK - 0.5f),
      static_cast<int>(tLocation.b * OO_COORD_TO_SUBCHUNK - 0.5f), static_cast<int>(tLocation.r * OO_COORD_TO_SUBCHUNK - 0.5f)
  );
  NTempest::CiRect cRect(sRect.t >> 3, sRect.l >> 3, sRect.b >> 3, sRect.r >> 3);

  for (int cy = cRect.t; cy <= cRect.b; ++cy) {
    for (int cx = cRect.l; cx <= cRect.r; ++cx) {
      GetChunkFacets(cx, cy, sRect, frustum, facetData);
    }
  }

  return facetData->facets.Count() != 0;
}

unsigned int CMap::GetChunkFacets(int cx, int cy, NTempest::CiRect &sRect, CWFrustum &wFrustum, CWFacetData *facetData) {
  NTempest::CAaBox frustumBox = NTempest::CAaBox::Bounding(wFrustum.corners, 8);
  return GetChunkFacets(cx, cy, sRect, frustumBox, facetData, 0);
}

unsigned int CMap::GetFacetsMapObjs(CWFrustum &frustum, CWFacetData *facetData, unsigned int queryFlags) {
  unsigned int hit = 0;

  for (CMapObjDef *mapObjDef = mapObjDefHash.Head(); mapObjDef; mapObjDef = mapObjDefHash.Next(mapObjDef)) {
    if (mapObjDef->flags & CMapBaseObj::Flag_NoCollision || !frustum.Cull(mapObjDef->aaBox)) {
      continue;
    }

    NTempest::C3Vector moCorners[8];
    for (unsigned int i = 0; i < 8; ++i) {
      moCorners[i] = frustum.corners[i] * mapObjDef->invMat;
    }

    CWFrustum moFrustum(moCorners);
    CMapObj *mapObj = mapObjDef->mapObj;
    if (mapObj) {
      CWTriData triData;
      hit |= mapObj->GetTris(triData, moFrustum, mapObjDef, queryFlags);
      CWorld::TriDataToFacetData(triData, *facetData, 0);
    }
  }

  return hit;
}

