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

#include "DB/DBClient/AutoCode/GroundEffectTextureRec.h"

#include <Ftol.h>

static const float OO_COORD_TO_SUBCHUNK = 1.0f / (150.0f / 36.0f);
static const float OO_COORD_TO_CHUNK = 1.0f / ((150.0f / 36.0f) * 8);
static const float OO_COORD_TO_SHADOW = 0.96f;

extern UINT g_holeMask[4][4];

WORD  g_2bitSplatMask[8] = {0x0003, 0x000C, 0x0030, 0x00C0, 0x0300, 0x0C00, 0x3000, 0xC000};
DWORD g_2bitSplatShft[8] = {0, 2, 4, 6, 8, 10, 12, 14};
WORD  g_1bitSplatMask[8] = {0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080};
DWORD g_1bitSplatShft[8] = {0, 1, 2, 3, 4, 5, 6, 7};

UINT CMap::QueryAreaId(float x, float y) {
  float mx = -(y - 17066.666f);
  float my = -(x - 17066.666f);
  ASSERT(mx >= 0.0f && my >= 0.0f);
  ASSERT(mx < ((64*16)*((150.0f/36.0f)*8)) && my < ((64*16)*((150.0f/36.0f)*8)));

  int mxIndex = Fast_ftol(OO_COORD_TO_CHUNK * mx);
  int myIndex = Fast_ftol(OO_COORD_TO_CHUNK * my);

  CMapArea *area = areaTable[((myIndex >> 4) & 0x3F) * 64 + ((mxIndex >> 4) & 0x3F)];
  if (!area) {
    return 0;
  }

  mxIndex &= 0xF;
  myIndex &= 0xF;
  CMapChunk *chunk = area->chunkTable[myIndex * 16 + mxIndex];
  if (!chunk) {
    return 0;
  }

  return chunk->zoneId;
}

bool CMap::QueryGroundType(const NTempest::C3Vector &pos, UINT &groundType) {
  float mx = -(pos.y - 17066.666f);
  float my = -(pos.x - 17066.666f);
  ASSERT(mx >= 0.0f && my >= 0.0f);
  ASSERT(mx < ((64*16)*((150.0f/36.0f)*8)) && my < ((64*16)*((150.0f/36.0f)*8)));

  int mxIndex = Fast_ftol(OO_COORD_TO_SUBCHUNK * mx);
  int myIndex = Fast_ftol(OO_COORD_TO_SUBCHUNK * my);

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

  if (!chunk->nLayers) {
    return false;
  }

  int effectId = chunk->layerList[(chunk->predTex[myIndex] & g_2bitSplatMask[mxIndex]) >> g_2bitSplatShft[mxIndex]]->effectId;
  if (effectId == -1 || effectId == 0xFFFF) {
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
  if (mx < 0.0f || my < 0.0f) {
    return false;
  }

  if (mx >= ((64*16)*((150.0f/36.0f)*8)) || my >= ((64*16)*((150.0f/36.0f)*8))) {
    return false;
  }

  int mxIndex = Fast_ftol(OO_COORD_TO_CHUNK * mx);
  int myIndex = Fast_ftol(OO_COORD_TO_CHUNK * my);

  CMapArea *area = areaTable[((myIndex >> 4) & 0x3F) * 64 + ((mxIndex >> 4) & 0x3F)];
  if (!area) {
    return false;
  }

  mxIndex &= 0xF;
  myIndex &= 0xF;
  CMapChunk *chunk = area->chunkTable[myIndex * 16 + mxIndex];
  if (!chunk) {
    return false;
  }

  int sx = Fast_ftol(OO_COORD_TO_SHADOW * mx) & 0x1F;
  int sy = Fast_ftol(OO_COORD_TO_SHADOW * my) & 0x1F;
  return (chunk->shadowBits[sy] & (1 << sx)) ? true : false;
}

bool CMap::QueryLiquidFishableMapObjsExt(const NTempest::C3Vector &point, int &fishable) {
  ITERATELIST(CMapObjDef, mapObjDefHash, mapObjDef) {
    NTempest::C3Vector p = point * mapObjDef->invMat;
    CMapObj           *mapObj = mapObjDef->mapObj;
    ASSERT(mapObj);
    if (mapObj->QueryLiquidFishable(0x2000, p, fishable)) {
      return true;
    }
  }
  return false;
}

bool CMap::QueryLiquidStatusMapObjsExt(const NTempest::C3Vector &point, UINT &liquid, float &surface, NTempest::C3Vector &waterDir) {
  ITERATELIST(CMapObjDef, mapObjDefHash, mapObjDef) {
    NTempest::C3Vector p = point * mapObjDef->invMat;
    CMapObj           *mapObj = mapObjDef->mapObj;
    ASSERT(mapObj);
    if (mapObj->QueryLiquidStatus(0x2000, p, liquid, surface, waterDir)) {
      NTempest::C3Vector out(0.0f, 0.0f, surface);
      out *= mapObjDef->mat;
      surface = out.z;
      return true;
    }
  }
  return false;
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
  if (surface > point.z) {
    switch (cl->nFlowvs) {
      case 0:
        flow = NTempest::C3Vector(0.0f);
        break;

      case 1:
        if (cl->flowvs[0].sphere.Intersects(point)) {
          flow = cl->flowvs[0].dir * cl->flowvs[0].velocity;
        } else {
          flow = NTempest::C3Vector(0.0f);
        }
        break;

      case 2: {
        int inside[2] = {cl->flowvs[0].sphere.Intersects(point), cl->flowvs[1].sphere.Intersects(point)};
        if (inside[0] && inside[1]) {
          flow = cl->flowvs[0].dir + cl->flowvs[1].dir;
          flow.Normalize();
          flow *= (cl->flowvs[0].velocity + cl->flowvs[1].velocity) * 0.5f;
        } else if (inside[0]) {
          flow = cl->flowvs[0].dir * cl->flowvs[0].velocity;
        } else if (inside[1]) {
          flow = cl->flowvs[1].dir * cl->flowvs[1].velocity;
        } else {
          flow = NTempest::C3Vector(0.0f);
        }
        break;
      }
    }
  }
}

bool CMap::QueryLiquidFishable(const NTempest::C3Vector &point, int &fishable) {
  if (QueryLiquidFishableMapObjsExt(point, fishable)) {
    return true;
  }

  float mx = -(point.y - 17066.666f);
  float my = -(point.x - 17066.666f);
  ASSERT(mx >= 0.0f && my >= 0.0f);
  ASSERT(mx < ((64*16)*((150.0f/36.0f)*8)) && my < ((64*16)*((150.0f/36.0f)*8)));

  float msx = OO_COORD_TO_SUBCHUNK * mx;
  float msy = OO_COORD_TO_SUBCHUNK * my;
  int   sx = Fast_ftol(msx);
  int   sy = Fast_ftol(msy);

  CMapArea *area = areaTable[((sy >> 7) & 0x3F) * 64 + ((sx >> 7) & 0x3F)];
  if (!area) {
    return false;
  }

  CMapChunk *chunk = area->chunkTable[((sy >> 3) & 0xF) * 16 + ((sx >> 3) & 0xF)];
  if (!chunk) {
    return false;
  }

  NTempest::C2iVector lsub(sx & 7, sy & 7);
  for (UINT i = 0; i < 4; ++i) {
    CChunkLiquid *cl = chunk->liquids[i];
    if (cl) {
      UINT liquid;
      int  fish;
      int  deep;
      if (cl->tiles.GetLiquid(lsub, liquid, fish, deep)) {
        fishable = (bool)fish;
        return true;
      }
    }
  }

  return false;
}

bool CMap::QueryLiquidStatus(const NTempest::C3Vector &point, UINT &liquid, float &surface, NTempest::C3Vector &waterDir, int &deep) {
  if (QueryLiquidStatusMapObjsExt(point, liquid, surface, waterDir)) {
    deep = 0;
    return true;
  }

  float mx = -(point.y - 17066.666f);
  float my = -(point.x - 17066.666f);
  ASSERT(mx >= 0.0f && my >= 0.0f);
  ASSERT(mx < ((64*16)*((150.0f/36.0f)*8)) && my < ((64*16)*((150.0f/36.0f)*8)));

  float msx = OO_COORD_TO_SUBCHUNK * mx;
  float msy = OO_COORD_TO_SUBCHUNK * my;
  int   sx = Fast_ftol(msx);
  int   sy = Fast_ftol(msy);

  CMapArea *area = areaTable[((sy >> 7) & 0x3F) * 64 + ((sx >> 7) & 0x3F)];
  if (!area) {
    return false;
  }

  CMapChunk *chunk = area->chunkTable[((sy >> 3) & 0xF) * 16 + ((sx >> 3) & 0xF)];
  if (!chunk) {
    return false;
  }

  NTempest::C2Vector  frac(msx - sx, msy - sy);
  NTempest::C2iVector lsub(sx & 7, sy & 7);
  for (UINT i = 0; i < 4; ++i) {
    CChunkLiquid *cl = chunk->liquids[i];
    if (cl) {
      UINT type;
      int  fishable;
      cl->tiles.GetLiquid(lsub, type, fishable, deep);

      switch (type & 3) {
        case 1:
          if (point.z < 0.0f) {
            surface = 0.0f;
            waterDir = NTempest::C3Vector(0.0f);
            liquid = type & 3;
            return true;
          }
          break;

        case 0:
          GetHeightFlow(cl, point, frac, lsub, surface, waterDir);
          if (point.z < surface) {
            liquid = type & 3;
            return true;
          }
          break;

        case 2:
          GetHeightFlow(cl, point, frac, lsub, surface, waterDir);
          if (point.z < surface) {
            liquid = type & 3;
            return true;
          }
          break;
      }
    }
  }

  return false;
}
