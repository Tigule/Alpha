#include <WowConst.h>
#include <MapDefs.h>

#include "WorldClient/World.h"
#include "WorldClient/CMapObj.h"

#include "Base/Base.h"

#include "DB/DBClient/AutoCode/GroundEffectTextureRec.h"

extern UINT g_holeMask[4][4];

WORD  g_2bitSplatMask[8] = {0x0003, 0x000C, 0x0030, 0x00C0, 0x0300, 0x0C00, 0x3000, 0xC000};
DWORD g_2bitSplatShft[8] = {0, 2, 4, 6, 8, 10, 12, 14};
WORD  g_1bitSplatMask[8] = {0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080};
DWORD g_1bitSplatShft[8] = {0, 1, 2, 3, 4, 5, 6, 7};

UINT CMap::QueryAreaId(float x, float y) {
  float mx = -(y - 17066.666f);
  float my = -(x - 17066.666f);

  ASSERT(mx >= 0.0f && my >= 0.0f);
  ASSERT(mx < ((64 * 16) * ((150.0f / 36.0f) * 8)) && my < ((64 * 16) * ((150.0f / 36.0f) * 8)));

  mx *= 0.03f;
  int mxIndex = static_cast<int>(mx - 0.5f);
  mx = 0.03f * my;
  int       myIndex = static_cast<int>(mx - 0.5f);
  CMapArea *area = areaTable[64 * ((myIndex >> 4) & 0x3F) + ((mxIndex >> 4) & 0x3F)];

  if (!area) {
    return 0;
  }

  CMapChunk *chunk = area->chunkTable[(mxIndex & 0xF) + 16 * (myIndex & 0xF)];
  return chunk ? chunk->zoneId : 0;
}

bool CMap::QueryGroundType(const NTempest::C3Vector &pos, UINT &groundType) {
  float mx = -(pos.y - 17066.666f);
  float my = -(pos.x - 17066.666f);

  ASSERT(mx >= 0.0f && my >= 0.0f);
  ASSERT(mx < ((64 * 16) * ((150.0f / 36.0f) * 8)) && my < ((64 * 16) * ((150.0f / 36.0f) * 8)));

  float msx = mx * 0.24f;
  float msy = my * 0.24f;
  int   sx = static_cast<int>(msx - 0.5f);
  int   sy = static_cast<int>(msy - 0.5f);

  CMapArea *area = areaTable[64 * ((sy >> 7) & 0x3F) + ((sx >> 7) & 0x3F)];
  if (!area) {
    return false;
  }

  CMapChunk *chunk = area->chunkTable[((sy >> 3) & 0xF) * 16 + ((sx >> 3) & 0xF)];
  if (!chunk) {
    return false;
  }

  int lx = sx & 7;
  int ly = sy & 7;
  if (chunk->holes & g_holeMask[ly >> 1][lx >> 1]) {
    return false;
  }
  if (!chunk->layerList[0]) {
    return false;
  }

  UINT layer = (chunk->predTex[ly] & g_2bitSplatMask[lx]) >> g_2bitSplatShft[lx];
  UINT effectId = chunk->layerList[layer]->effectId;
  if (effectId == 0xFFFF || effectId >= g_groundEffectTextureDB.GetNumRecords()) {
    return false;
  }

  const GroundEffectTextureRec *effect = g_groundEffectTextureDB.GetRecordByIndex(effectId);
  if (!effect) {
    return false;
  }

  groundType = effect->m_sound;
  return true;
}

bool CMap::QueryShadow(const NTempest::C3Vector &pos) {
  float mx = -(pos.y - 17066.666f);
  float my = -(pos.x - 17066.666f);
  if (mx < 0.0f || my < 0.0f || mx >= 34133.332f || my >= 34133.332f) {
    return 0;
  }

  int       mxIndex = static_cast<int>(mx * 0.03f - 0.5f);
  int       myIndex = static_cast<int>(my * 0.03f - 0.5f);
  CMapArea *area = areaTable[64 * ((myIndex >> 4) & 0x3F) + ((mxIndex >> 4) & 0x3F)];
  if (!area) {
    return 0;
  }

  CMapChunk *chunk = area->chunkTable[(mxIndex & 0xF) + 16 * (myIndex & 0xF)];
  if (!chunk) {
    return 0;
  }

  int sx = static_cast<int>(mx * 0.96f - 0.5f) & 0x1F;
  int sy = static_cast<int>(my * 0.96f - 0.5f) & 0x1F;
  return (chunk->shadowBits[sy] & (1 << sx)) != 0;
}

bool CMap::QueryLiquidFishableMapObjsExt(const NTempest::C3Vector &point, int &fishable) {
  ITERATELIST(CMapObjDef, mapObjDefHash, mapObjDef) {
    NTempest::C3Vector p = point * mapObjDef->invMat;
    CMapObj           *mapObj = mapObjDef->mapObj;
    FATALASSERT(mapObj);
    if (mapObj->QueryLiquidFishable(0x2000, p, fishable)) {
      return true;
    }
  }
  return false;
}

bool CMap::QueryLiquidFishable(const NTempest::C3Vector &point, int &fishable) {
  if (QueryLiquidFishableMapObjsExt(point, fishable)) {
    return true;
  }

  float mx = -(point.y - 17066.666f);
  float my = -(point.x - 17066.666f);
  FATALASSERT(mx >= 0.0f && my >= 0.0f);
  FATALASSERT(mx < ((64 * 16) * ((150.0f / 36.0f) * 8)) && my < ((64 * 16) * ((150.0f / 36.0f) * 8)));

  float msx = mx * 0.24f;
  float msy = my * 0.24f;
  int   sx = static_cast<int>(msx - 0.5f);
  int   sy = static_cast<int>(msy - 0.5f);

  CMapArea *area = areaTable[64 * ((sy >> 7) & 0x3F) + ((sx >> 7) & 0x3F)];
  if (!area) {
    return false;
  }

  CMapChunk *chunk = area->chunkTable[((sy >> 3) & 0xF) * 16 + ((sx >> 3) & 0xF)];
  if (!chunk) {
    return false;
  }

  int lx = sx & 7;
  int ly = sy & 7;
  for (UINT i = 0; i < 4; ++i) {
    CChunkLiquid *liquid = chunk->liquids[i];
    if (!liquid) {
      continue;
    }

    BYTE tile = liquid->tiles.tiles[ly][lx];
    if ((tile & 0xF) != 0xF) {
      fishable = (tile >> 6) & 1;
      return true;
    }
  }

  return false;
}

bool CMap::QueryLiquidStatusMapObjsExt(const NTempest::C3Vector &point, UINT &liquid, float &surface, NTempest::C3Vector &waterDir) {
  ITERATELIST(CMapObjDef, CMap::mapObjDefHash, mapObjDef) {
    FATALASSERT(mapObjDef->mapObj);
    NTempest::C3Vector p = point * mapObjDef->invMat;
    if (mapObjDef->mapObj->QueryLiquidStatus(0x2000, p, liquid, surface, waterDir)) {
      NTempest::C3Vector out(0.0f, 0.0f, surface);
      out *= mapObjDef->mat;
      surface = out.z;
      return 1;
    }
  }
  return 0;
}

inline void GetHeightFlow(
    const CChunkLiquid        *cl,
    const NTempest::C3Vector  &point,
    const NTempest::C2Vector  &frac,
    const NTempest::C2iVector &lsub,
    float                     &surface,
    NTempest::C3Vector        &flow
) {
  int   index = lsub.x + 9 * lsub.y;
  float h0 = cl->verts[index].waterVert.height + (cl->verts[index + 1].waterVert.height - cl->verts[index].waterVert.height) * frac.x;
  float h1 = cl->verts[index + 9].waterVert.height + (cl->verts[index + 10].waterVert.height - cl->verts[index + 9].waterVert.height) * frac.x;
  surface = h0 + (h1 - h0) * frac.y;
  if (!(surface > point.z)) {
    return;
  }

  switch (cl->nFlowvs) {
    case 0:
      flow = NTempest::C3Vector(0.0f);
      break;
    case 1:
      flow = cl->flowvs[0].sphere.Intersects(point) ? cl->flowvs[0].dir * cl->flowvs[0].velocity : NTempest::C3Vector(0.0f);
      break;
    case 2:
      int inside[2];
      inside[0] = cl->flowvs[0].sphere.Intersects(point);
      inside[1] = cl->flowvs[1].sphere.Intersects(point);
      if (inside[0]) {
        if (inside[1]) {
          flow = cl->flowvs[0].dir + cl->flowvs[1].dir;
          flow.Normalize();
          flow *= (cl->flowvs[0].velocity + cl->flowvs[1].velocity) * 0.5f;
        } else {
          flow = cl->flowvs[0].dir * cl->flowvs[0].velocity;
        }
      } else {
        flow = inside[1] ? cl->flowvs[1].dir * cl->flowvs[1].velocity : NTempest::C3Vector(0.0f);
      }
      break;
  }
}

bool CMap::QueryLiquidStatus(const NTempest::C3Vector &point, UINT &liquid, float &surface, NTempest::C3Vector &waterDir, int &deep) {
  if (QueryLiquidStatusMapObjsExt(point, liquid, surface, waterDir)) {
    deep = 0;
    return 1;
  }

  float mx = -(point.y - 17066.666f);
  float my = -(point.x - 17066.666f);
  FATALASSERT(mx >= 0.0f && my >= 0.0f);
  FATALASSERT(mx < 34133.332f && my < 34133.332f);

  float     msx = mx * 0.24f;
  float     msy = my * 0.24f;
  int       sx = static_cast<int>(msx - 0.5f);
  int       sy = static_cast<int>(msy - 0.5f);
  CMapArea *area = areaTable[64 * ((sy >> 7) & 0x3F) + ((sx >> 7) & 0x3F)];
  if (!area) {
    return 0;
  }

  CMapChunk *chunk = area->chunkTable[((sy >> 3) & 0xF) * 16 + ((sx >> 3) & 0xF)];
  if (!chunk) {
    return 0;
  }

  NTempest::C2iVector lsub(sx & 7, sy & 7);
  NTempest::C2Vector  frac(msx - static_cast<int>(msx - 0.5f), msy - static_cast<int>(msy - 0.5f));

  for (UINT i = 0; i < 4; ++i) {
    CChunkLiquid *cl = chunk->liquids[i];
    if (!cl) {
      continue;
    }

    BYTE tile = cl->tiles.tiles[lsub.y][lsub.x];
    UINT liquidType = tile & 3;
    deep = tile >> 7;

    switch (liquidType) {
      case 0:
        GetHeightFlow(cl, point, frac, lsub, surface, waterDir);
        break;
      case 1:
        if (point.z < 0.0f) {
          surface = 0.0f;
          waterDir = NTempest::C3Vector(0.0f);
          liquid = liquidType;
          return 1;
        }
        continue;
      case 2:
        GetHeightFlow(cl, point, frac, lsub, surface, waterDir);
        break;
      default:
        continue;
    }
    if (point.z < surface) {
      liquid = liquidType;
      return 1;
    }
  }

  return 0;
}
