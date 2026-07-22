#include "WorldClient/World.h"
#include "WorldClient/CMapObj.h"

#include "Base/Base.h"

static void __fastcall GetHeightFlow(
    CChunkLiquid        *cl,
    NTempest::C3Vector  &point,
    NTempest::C2Vector  &frac,
    NTempest::C2iVector &lsub,
    float               &surface,
    NTempest::C3Vector  &flow
) {
  int   index = lsub.x + 9 * lsub.y;
  float h0 = cl->verts[index].waterVert.height + (cl->verts[index + 1].waterVert.height - cl->verts[index].waterVert.height) * frac.x;
  float h1 = cl->verts[index + 9].waterVert.height + (cl->verts[index + 10].waterVert.height - cl->verts[index + 9].waterVert.height) * frac.x;
  surface = h0 + (h1 - h0) * frac.y;
  if (surface <= point.z) {
    return;
  }

  if (!cl->nFlowvs) {
    flow = NTempest::C3Vector(0.0f);
  } else if (cl->nFlowvs == 1) {
    NTempest::C3Vector delta = point - cl->flowvs[0].sphere.c;
    if (delta.SquaredMag() <= cl->flowvs[0].sphere.r * cl->flowvs[0].sphere.r) {
      flow = cl->flowvs[0].dir * cl->flowvs[0].velocity;
    } else {
      flow = NTempest::C3Vector(0.0f);
    }
  } else if (cl->nFlowvs == 2) {
    NTempest::C3Vector delta0 = point - cl->flowvs[0].sphere.c;
    NTempest::C3Vector delta1 = point - cl->flowvs[1].sphere.c;
    int                inside0 = delta0.SquaredMag() <= cl->flowvs[0].sphere.r * cl->flowvs[0].sphere.r;
    int                inside1 = delta1.SquaredMag() <= cl->flowvs[1].sphere.r * cl->flowvs[1].sphere.r;

    if (inside0 && inside1) {
      flow = cl->flowvs[0].dir + cl->flowvs[1].dir;
      flow.Normalize();
      flow *= (cl->flowvs[0].velocity + cl->flowvs[1].velocity) * 0.5f;
    } else if (inside0) {
      flow = cl->flowvs[0].dir * cl->flowvs[0].velocity;
    } else if (inside1) {
      flow = cl->flowvs[1].dir * cl->flowvs[1].velocity;
    } else {
      flow = NTempest::C3Vector(0.0f);
    }
  }
}

unsigned int __fastcall CMap::QueryLiquidStatusMapObjsExt(
    NTempest::C3Vector &point,
    unsigned int       &liquid,
    float              &surface,
    NTempest::C3Vector &waterDir
) {
  CMapObjDef *mapObjDef = CMap::mapObjDefHash.Head();
  while (mapObjDef) {
    FATALASSERT(mapObjDef->mapObj);
    NTempest::C3Vector p = point * mapObjDef->invMat;
    NTempest::C3Vector out;
    if (mapObjDef->mapObj->QueryLiquidStatus(0x2000, p, liquid, surface, out)) {
      waterDir = out * mapObjDef->mat;
      return 1;
    }
    mapObjDef = CMap::mapObjDefHash.Next(mapObjDef);
  }
  return 0;
}

unsigned int __fastcall CMap::QueryAreaId(float x, float y) {
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

unsigned int __fastcall CMap::QueryShadow(NTempest::C3Vector &pos) {
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

unsigned int __fastcall CMap::QueryLiquidStatus(
    NTempest::C3Vector &point,
    unsigned int       &liquid,
    float              &surface,
    NTempest::C3Vector &waterDir,
    int                &deep
) {
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

  for (unsigned int i = 0; i < 4; ++i) {
    CChunkLiquid *cl = chunk->liquids[i];
    if (!cl) {
      continue;
    }

    unsigned char tile = cl->tiles.flags[lsub.x + 8 * lsub.y];
    unsigned int  liquidType = tile & 3;
    deep = tile >> 7;

    if (liquidType == 1) {
      if (point.z >= 0.0f) {
        continue;
      }
      surface = 0.0f;
      waterDir = NTempest::C3Vector(0.0f);
    } else if (liquidType == 0 || liquidType == 2) {
      GetHeightFlow(cl, point, frac, lsub, surface, waterDir);
      if (point.z >= surface) {
        continue;
      }
    } else {
      continue;
    }

    liquid = liquidType;
    return 1;
  }

  return 0;
}
