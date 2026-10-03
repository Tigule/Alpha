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

#include "Base/Handle.h"
#include "Base/Status.h"
#include "Services/SysMessage.h"
#include "Services/Texture.h"
#include "Tempest/cmath.h"
#include "Tempest/tempest_intersect.h"

#include "Client.h"

#include <math.h>
#include <new>
#include <string.h>

struct LODIndexFix {
  WORD from;
  WORD to;

  void Set(WORD, WORD);
};

struct LODArrays {
  TSGrowableArray<NTempest::C2Vector> geov;
  TSGrowableArray<NTempest::C2Vector> texv;
  TSGrowableArray<WORD>               idx;
  UINT                                nFixes;
  TSGrowableArray<LODIndexFix>        fixes;

  void GenFixes(UINT p_nFixes, UINT vertsPerSide, UINT tilesPerSide);
  void GenVerts(UINT lod);
};

struct ChunkLodIdx {
  UINT ComputeIndexCount(UINT lod);
  void GenEdgeIndices(UINT edgeTris, UINT r1, UINT r1Delta, UINT r2, UINT r2Delta, WORD *idx);
  void GenLinkIndices(UINT numQuads, UINT r1, UINT r1Delta, UINT r2, UINT r2Delta, WORD *idx);
  void GenCenterIndices(UINT centerQuads, UINT rowVerts, WORD *idx);
  void GenCenterIndicesRow(UINT nQuads, UINT r[2], UINT i0, UINT i1, WORD *&idx);

  struct StartCount {
    WORD start;
    WORD count;
  };

  TSGrowableArray<WORD> indices;
  StartCount            edges[4];
  StartCount            links[4];
  StartCount            center;

  void GenIndices(UINT lod);
};

CGxTex                           *CMap::skyTexid;
CGxTex                           *CMap::riverDiffTexid;
CGxTex                           *CMap::oceanDiffTexid;
const UINT                        CMap::SKYTEX_HEIGHT = 64;
const UINT                        CMap::WATERTEX_HEIGHT = 64;
const float                       CMap::LIQUID_TEX_PURGE_TIME = 20.0f;
const float                       CMap::WATER_SPEC_EXP = 6.0f;
LISTDECL(WaterRadWave, CMap::waterRipplesActive);
LISTDECL(WaterRadWave, CMap::waterRipplesFree);
TSFixedArray<NTempest::CImVector> CMap::skyTexels;
HTEXTURE CMap::liquidTex[LIQUID_COUNT][LIQUID_TEXTURE_COUNT];
bool                              CMap::liquidTexLoaded[LIQUID_COUNT];
float                             CMap::liquidLastShown[LIQUID_COUNT];
const float                       CMap::liquidTexLoopTime[LIQUID_COUNT] = {1.25f, 1.25f, 1.25f, 1.25f, 1.25f, 1.25f, 1.25f, 1.25f, 1.25f};
LPCSTR                            CMap::liquidTexBaseName[LIQUID_COUNT] = {"XTextures\\river\\lake_a.%d.blp", "XTextures\\ocean\\ocean_h.%d.blp",
                                                                           "XTextures\\lava\\lava.%d.blp",    "XTextures\\slime\\slime.%d.blp",
                                                                           "XTextures\\river\\lake_a.%d.blp", 0,
                                                                           "XTextures\\lava\\lava.%d.blp",    "XTextures\\slime\\slime.%d.blp",
                                                                           "XTextures\\river\\fast_a.%d.blp"};
bool                              CMap::riverDiffTexUpdated;
bool                              CMap::oceanDiffTexUpdated;
CGxPixelShader           *CMap::psOcean0;

static UINT                       s_lodSubdivs[5] = {0, 1, 3, 7, 15};
static TSGrowableArray<LODArrays> s_lodArrays;
static const UINT                 MAX_SUBDIVS = s_lodSubdivs[4];
static NTempest::CImVector       *pixels;
static const float                kDeepDarken = 0.75f;
static const float                MD_RIVER_DEPTH_SCALE = 1.0f / 9.0f;
static const float                Gx_MinTexAspect = 0.125f;
static float                      s_oceanDepthCoordTable[256];
static float                      s_riverDepthCoordTable[256];
static NTempest::CImVector        s_reflectivity[256];

const float WaterRadWave::PERTURB = 40.0f;

BOOL WaterRadWave::Update(float deltat) {
  curTime += deltat;
  if (curTime > timeLength) {
    return 0;
  }

  rb = curTime * velocity;
  ra = rb - length;
  decay = 1.0f - curTime * ooTimeLength;
  return 1;
}

int CMapArea::ccWaterLOD = -1;
int CMapArea::ccWaterMaxLOD = 4;
int CMapArea::ccWaterWaves = 2;
int CMapArea::ccWaterSpecular = 1;
int CMapArea::ccWaterRipples = 1;

void WaterRadWave::Init(const NTempest::C3Vector &p_pos, float len, float time, float amp, float vel, float freq) {
  pos = p_pos;
  length = len;
  timeLength = time;
  amplitude = amp;
  velocity = vel;
  frequency = freq;
  curTime = 0.0f;
  rb = 0.0f;
  ooLength = 1.0f / len;
  ooTimeLength = 1.0f / time;
  ra = -len;
}

void LODArrays::GenFixes(UINT p_nFixes, UINT vertsPerSide, UINT tilesPerSide) {
  nFixes = p_nFixes;
  fixes.SetCount(4 * p_nFixes);

  UINT index = 0;
  WORD from = 1;
  UINT i;
  for (i = 0; i < nFixes; ++i) {
    fixes[index].from = from;
    fixes[index].to = from - 1;
    ++index;
    from += 2;
  }

  from = static_cast<WORD>(2 * vertsPerSide - 1);
  WORD to = 0;
  WORD to2 = static_cast<WORD>(4 * vertsPerSide);
  for (i = 0; i < nFixes / 2; ++i) {
    fixes[index].from = from;
    fixes[index].to = to;
    ++index;
    fixes[index].from = static_cast<WORD>(from + 2 * vertsPerSide);
    fixes[index].to = to2;
    ++index;
    from = static_cast<WORD>(from + 4 * vertsPerSide);
    to = static_cast<WORD>(to + 4 * vertsPerSide);
    to2 = static_cast<WORD>(to2 + 4 * vertsPerSide);
  }

  from = static_cast<WORD>(tilesPerSide * vertsPerSide + 1);
  to = static_cast<WORD>(tilesPerSide * vertsPerSide);
  for (i = 0; i < nFixes; ++i) {
    fixes[index].from = from;
    fixes[index].to = to;
    ++index;
    from += 2;
    to += 2;
  }

  from = static_cast<WORD>(tilesPerSide * tilesPerSide - 1);
  to = static_cast<WORD>(tilesPerSide * tilesPerSide - 2);
  for (i = 0; i < nFixes; ++i) {
    fixes[index].from = from;
    fixes[index].to = to;
    ++index;
    from = static_cast<WORD>(from - 2 * vertsPerSide);
    to = from - 1;
  }
}

void LODArrays::GenVerts(UINT lod) {
  UINT vertsPerSide = lod + 2;
  UINT vertexCount = vertsPerSide * vertsPerSide;
  geov.SetCount(vertexCount);
  texv.SetCount(vertexCount);

  UINT  vertex = 0;
  UINT  y = 0;
  UINT  x;
  float ooTiles = 1.0f / static_cast<float>(lod + 1);
  while (y < vertsPerSide) {
    float ty = static_cast<float>(y) * ooTiles;
    for (x = 0; x < vertsPerSide; ++x) {
      float tx = static_cast<float>(x) * ooTiles;
      geov[vertex].x = tx * -4.1666665f;
      geov[vertex].y = ty * -4.1666665f;
      texv[vertex].x = tx;
      texv[vertex].y = ty;
      ++vertex;
    }

    ++y;
    if (y >= vertsPerSide) {
      break;
    }

    ty = static_cast<float>(y) * ooTiles;
    for (x = vertsPerSide; x; --x) {
      float tx = static_cast<float>(x - 1) * ooTiles;
      geov[vertex].x = tx * -4.1666665f;
      geov[vertex].y = ty * -4.1666665f;
      texv[vertex].x = tx;
      texv[vertex].y = ty;
      ++vertex;
    }
    ++y;
  }

  UINT indexCount = 2 * vertsPerSide * (lod + 1) + 2;
  idx.SetCount(indexCount);
  UINT index = 0;
  WORD low = 0;
  WORD high = static_cast<WORD>(2 * vertsPerSide - 1);
  for (UINT row = 0; row < lod + 1; ++row) {
    for (UINT x = 0; x < vertsPerSide; ++x) {
      idx[index++] = low++;
      idx[index++] = high--;
    }
    low = high + 1;
    high = static_cast<WORD>(high + 2 * vertsPerSide);
  }
  idx[index++] = low;
  idx[index] = (lod + 1) & 1 ? static_cast<WORD>(low + lod + 1) : low;

  switch (lod) {
    case 1:
      nFixes = 1;
      fixes.SetCount(4);
      fixes[0].from = 1;
      fixes[0].to = 0;
      fixes[1].from = 5;
      fixes[1].to = 6;
      fixes[2].from = 7;
      fixes[2].to = 6;
      fixes[3].from = 3;
      fixes[3].to = 8;
      break;
    case 3:
    case 7:
    case 15:
      GenFixes((lod + 1) / 2, lod + 2, lod + 1);
      break;
    default:
      nFixes = 0;
      break;
  }
}

void CMapArea::InitWater() {
  if (!s_lodArrays.Count()) {
    s_lodArrays.SetCount(5);
    for (UINT i = 0; i < 5; ++i) {
      s_lodArrays[i].GenVerts(s_lodSubdivs[i]);
    }
  }
}

void CMap::WaterDiffTexCallback(EGxTexCommand cmd, UINT w, UINT h, UINT d, UINT mipLevel, LPVOID userArg, UINT &texelStrideInBytes, LPCVOID &texels) {
  ASSERT(mipLevel == 0);
  ASSERT(h == 64);

  switch (cmd) {
    case GxTex_Lock:
      pixels = static_cast<NTempest::CImVector *>(GxAllocPixelMem(w * h * sizeof(NTempest::CImVector)));
      break;

    case GxTex_Latch: {
      UINT                 index = 2 * reinterpret_cast<UINT>(userArg);
      DNInfo              *dnInfo = DayNightGetInfo();
      NTempest::CImVector  shallowClr = dnInfo->light.WaterArray[index];
      NTempest::CImVector  deepClr = dnInfo->light.WaterArray[index + 1];
      UINT                 redDelta = ((deepClr.r - shallowClr.r) << 8) >> 6;
      UINT                 greenDelta = ((deepClr.g - shallowClr.g) << 8) >> 6;
      UINT                 blueDelta = ((deepClr.b - shallowClr.b) << 8) >> 6;
      UINT                 red = shallowClr.r << 8;
      UINT                 green = shallowClr.g << 8;
      UINT                 blue = shallowClr.b << 8;
      NTempest::CImVector *tex = pixels;

      for (UINT y = 0; y < h; ++y) {
        NTempest::CImVector rowColor;

        rowColor.Set(0xFF, static_cast<BYTE>(red >> 8), static_cast<BYTE>(green >> 8), static_cast<BYTE>(blue >> 8));

        if (y == h - 1 && !userArg) {
          NTempest::C3Vector rgb = rowColor;
          NTempest::C3Vector hsv;

          NTempest::RGBtoHSV(rgb, hsv);
          hsv.z *= kDeepDarken;
          NTempest::HSVtoRGB(hsv, rgb);
          rowColor = rgb;
        }

        for (UINT x = 0; x < w; ++x) {
          tex[x] = rowColor;
        }

        red += redDelta;
        green += greenDelta;
        blue += blueDelta;
        tex += w;
      }

      texelStrideInBytes = w * sizeof(NTempest::CImVector);
      texels = pixels;
      break;
    }

    case GxTex_Unlock:
      pixels = 0;
      break;
  }
}

HTEXTURE CMap::GetLiquidTexture(UINT liquid) {
  char filename[256];
  bool allLoaded;

  ASSERT(liquid < LIQUID_COUNT);

  const float secsPerLoop = liquidTexLoopTime[liquid];
  UINT texture = Fast_ftol(fmodf(CWorld::GetCurTimeSec(), secsPerLoop) / secsPerLoop * static_cast<float>(LIQUID_TEXTURE_COUNT));

  if (!liquidTexLoaded[liquid]) {
    allLoaded = true;
    for (int i = 0; i < LIQUID_TEXTURE_COUNT; ++i) {
      if (!liquidTex[liquid][i]) {
        EGxTexFilter filter = GxTex_LinearMipNearest;
        if (CWorld::enables & CWorld::Enable_Anisotropic) {
          filter = GxTex_Anisotropic;
        } else if (CWorld::enables & CWorld::Enable_Trilinear) {
          filter = GxTex_LinearMipLinear;
        }

        CStatus status;
        FATALASSERT(liquidTexBaseName[liquid]);
        SStrPrintf(filename, sizeof(filename), liquidTexBaseName[liquid], i + 1);
        liquidTex[liquid][i] = TextureCreate(filename, CGxTexFlags(filter, 1, 1, 0, 0, 0, CWorld::texMaxAnisotropy), &status, 0);
        SysMsgAdd(status, 2);
      }

      if (!TextureGetGxTex(liquidTex[liquid][i], 0, 0)) {
        allLoaded = false;
      }
    }
    liquidTexLoaded[liquid] = allLoaded;
  }

  liquidLastShown[liquid] = CWorld::GetCurTimeSec();
  return liquidTex[liquid][texture];
}

void CMap::UnloadLiquidTexture(UINT liquid) {
  ASSERT(liquid < LIQUID_COUNT);

  for (UINT texture = 0; texture < LIQUID_TEXTURE_COUNT; ++texture) {
    if (liquidTex[liquid][texture]) {
      HandleClose(liquidTex[liquid][texture]);
      liquidTex[liquid][texture] = 0;
    }
  }

  liquidTexLoaded[liquid] = false;
}

void CMap::UpdateLiquidTextures() {
}

void CMapObj::QueryLiquidSounds(
    UINT                      groupIdx,
    UINT                      parentIdx,
    UINT                      rlevel,
    UINT                     &closestExtLevel,
    const NTempest::C3Vector &pos,
    int                      *lbool,
    NTempest::C3Vector       *ldelta,
    float                    *ldsquared
) {
  if (rlevel > MAX_SOUND_RLEVEL) {
    return;
  }

  const NTempest::CAaBox &box = GetGroupInfo(groupIdx)->aaBox;
  if (pos.x <= box.b.x || pos.y <= box.b.y || pos.z <= box.b.z || pos.x >= box.t.x || pos.y >= box.t.y || pos.z >= box.t.z) {
    return;
  }

  CMapObjGroup *group = GetGroup(groupIdx, 0);
  if (!group) {
    return;
  }
  if ((group->flags & 8) && rlevel < closestExtLevel) {
    closestExtLevel = rlevel;
  }
  group->QueryLiquidSounds(pos, lbool, ldelta, ldsquared);

  for (UINT i = 0; i < group->portalCount; ++i) {
    UINT nextGroup = portalRefList[group->portalStart + i].groupIndex;
    if (nextGroup != 0xFFFF && nextGroup != parentIdx) {
      QueryLiquidSounds(nextGroup, groupIdx, rlevel + 1, closestExtLevel, pos, lbool, ldelta, ldsquared);
    }
  }
}

void CMapObjGroup::QueryLiquidSounds(const NTempest::C3Vector &pos, int *lbool, NTempest::C3Vector *ldelta, float *ldsquared) {
  for (int y = 0; y < liquidTiles.y; ++y) {
    for (int x = 0; x < liquidTiles.x; ++x) {
      UINT tile = liquidTileList[y * liquidTiles.x + x].GetLiquid();
      if (tile == LIQUID_NONE) {
        continue;
      }

      UINT  liquidType = tile & 3;
      float height;
      if (liquidType == 1) {
        ASSERT(!"CMapObjGroup::QueryLiquidSounds()\n");
        height = 0.0f;
      } else {
        height = liquidVertexList[y * liquidVerts.x + x].waterVert.height;
      }
      NTempest::C3Vector d;
      d.x = liquidCorner.x - static_cast<float>(x) * 4.1666665f - pos.x;
      d.y = liquidCorner.y + static_cast<float>(y) * 4.1666665f - pos.y;
      d.z = height - pos.z;
      lbool[tile] = 1;
      float distanceSquared = d.SquaredMag();
      if (distanceSquared < ldsquared[tile]) {
        ldsquared[tile] = distanceSquared;
        ldelta[tile] = d;
      }
    }
  }
}

void CMap::QueryLiquidSounds(const NTempest::C3Vector &worldPos, float radius, int *lbool, NTempest::C3Vector *ldelta, float *ldsquared) {
  float mx = -(worldPos.y - 17066.666f);
  float my = -(worldPos.x - 17066.666f);
  FATALASSERT(mx >= 0.0f && my >= 0.0f);
  FATALASSERT(mx < ((64*16)*((150.0f/36.0f)*8)) && my < ((64*16)*((150.0f/36.0f)*8)));

  int areaX = (Fast_ftol(mx * 0.24f) >> 7) & 0x3F;
  int areaY = (Fast_ftol(my * 0.24f) >> 7) & 0x3F;
  int a[4];
  a[0] = areaX > 0 ? areaX - 1 : 0;
  a[1] = areaX + 1 < 63 ? areaX + 1 : 63;
  a[2] = areaY > 0 ? areaY - 1 : 0;
  a[3] = areaY + 1 < 63 ? areaY + 1 : 63;

  NTempest::CAaSphere querySphere;
  querySphere.c.x = worldPos.x;
  querySphere.c.y = worldPos.y;
  querySphere.c.z = 0.0f;
  querySphere.r = radius;

  for (int y = a[2]; y <= a[3]; ++y) {
    for (int x = a[0]; x <= a[1]; ++x) {
      CMapArea *area = areaTable[y * 64 + x];
      if (area) {
        NTempest::CAaBox areaBox(
            NTempest::C3Vector(area->corner.x - 533.33331f, area->corner.y - 533.33331f, 0.0f), NTempest::C3Vector(area->corner.x, area->corner.y, 0.0f)
        );
        if (NTempest::Intersect2d(areaBox, querySphere, NTempest::SI_SolidSolid)) {
          area->QueryLiquidSounds(worldPos, radius, lbool, ldelta, ldsquared);
        }
      }
    }
  }
}

void CMapArea::QueryLiquidSounds(const NTempest::C3Vector &worldPos, float radius, int *lbool, NTempest::C3Vector *ldelta, float *ldsquared) {
  float mx = -(worldPos.y - 17066.666f);
  float my = -(worldPos.x - 17066.666f);
  FATALASSERT(mx >= 0.0f && my >= 0.0f);
  FATALASSERT(mx < ((64*16)*((150.0f/36.0f)*8)) && my < ((64*16)*((150.0f/36.0f)*8)));

  int chunkX = Fast_ftol(mx * 0.03f) & 0xF;
  int chunkY = Fast_ftol(my * 0.03f) & 0xF;
  FATALASSERT(radius / (150.0f/36.0f) < 256.0f);
  int chunkRadius = static_cast<int>(ceil(radius / 4.1666665f));
  int minChunkX = chunkX - chunkRadius > 0 ? chunkX - chunkRadius : 0;
  int minChunkY = chunkY - chunkRadius > 0 ? chunkY - chunkRadius : 0;
  int maxChunkX = chunkX + chunkRadius < 15 ? chunkX + chunkRadius : 15;
  int maxChunkY = chunkY + chunkRadius < 15 ? chunkY + chunkRadius : 15;

  for (int y = minChunkY; y <= maxChunkY; ++y) {
    for (int x = minChunkX; x <= maxChunkX; ++x) {
      CMapChunk *chunk = chunkTable[y * 16 + x];
      if (!chunk) {
        continue;
      }
      for (UINT liquidIndex = 0; liquidIndex < 4; ++liquidIndex) {
        CChunkLiquid *liquid = chunk->liquids[liquidIndex];
        if (!liquid) {
          continue;
        }
        for (UINT tileY = 0; tileY < 8; ++tileY) {
          for (UINT tileX = 0; tileX < 8; ++tileX) {
            UINT tile = liquid->tiles.tiles[tileY][tileX] & 0xF;
            if (tile == 0xF) {
              continue;
            }

            lbool[tile] = 1;
            NTempest::C3Vector delta;
            delta.x = chunk->corner.x - static_cast<float>(tileY) * 4.1666665f - worldPos.x;
            delta.y = chunk->corner.y - static_cast<float>(tileX) * 4.1666665f - worldPos.y;
            delta.z = chunk->corner.z - worldPos.z;
            float distanceSquared = delta.SquaredMag();
            if (distanceSquared < ldsquared[tile]) {
              ldsquared[tile] = distanceSquared;
              ldelta[tile] = delta;
            }
          }
        }
      }
    }
  }
}

static void fft2(float data[], DWORD nn[], int ndim, float isign);

static WaterVert          sWave2(NTempest::C3Vector(1.414f, 1.414f, 0.0f), 0.5f, 1.0f / 18.0f, 0.0f);
static NTempest::C2Vector oceanfft[4096];
static float              phase;
static DWORD              nn[2] = {64, 64};
static float              phase2;

void CMap::OceanFFT() {
  memset(oceanfft, 0, sizeof(oceanfft));

  phase += CWorld::tickTimeSec * 0.2f;
  phase2 += CWorld::tickTimeSec * 0.92000002f;

  float c = cos(phase);
  float s = sin(phase);
  float c2 = cos(phase2);
  float s2 = sin(phase2);

  oceanfft[770] = NTempest::C2Vector(2.2f * c, 2.2f * s);
  oceanfft[896] = NTempest::C2Vector(2.02f * c2, 2.1f * s2);
  oceanfft[180] = NTempest::C2Vector(2.1f * c2, 2.1f * s2);
  oceanfft[3846] = NTempest::C2Vector(2.0f * c, 2.0f * s);
  oceanfft[1403] = NTempest::C2Vector(1.3f * c, 1.2f * s);
  oceanfft[3797] = NTempest::C2Vector(1.5f * c, 1.4f * s);
  oceanfft[1424] = NTempest::C2Vector(1.4f * c2, 1.4f * s2);
  oceanfft[254] = NTempest::C2Vector(1.6f * c2, 1.6f * s2);

  fft2(reinterpret_cast<float *>(oceanfft) - 1, nn - 1, 2, -1.0f);

  for (UINT i = 0; i < 4096; ++i) {
    oceanfft[i].x *= 0.015625f;
    oceanfft[i].y *= 0.015625f;
  }
}

static void fft2(float data[], DWORD nn[], int ndim, float isign) {
  DWORD ntot = 1;
  int   idim;
  for (idim = 1; idim <= ndim; ++idim) {
    ntot *= nn[idim];
  }

  DWORD nprev = 1;
  for (idim = ndim; idim >= 1; --idim) {
    DWORD n = nn[idim];
    DWORD nrem = ntot / (n * nprev);
    DWORD ip1 = nprev << 1;
    DWORD ip2 = ip1 * n;
    DWORD ip3 = ip2 * nrem;
    DWORD i2rev = 1;
    DWORD i2;
    for (i2 = 1; i2 <= ip2; i2 += ip1) {
      if (i2 < i2rev) {
        DWORD i1;
        for (i1 = i2; i1 <= i2 + ip1 - 2; i1 += 2) {
          DWORD i3;
          for (i3 = i1; i3 <= ip3; i3 += ip2) {
            DWORD i3rev = i2rev + i3 - i2;
            float temp = data[i3];
            data[i3] = data[i3rev];
            data[i3rev] = temp;
            temp = data[i3 + 1];
            data[i3 + 1] = data[i3rev + 1];
            data[i3rev + 1] = temp;
          }
        }
      }
      DWORD ibit = ip2 >> 1;
      while (ibit >= ip1 && i2rev > ibit) {
        i2rev -= ibit;
        ibit >>= 1;
      }
      i2rev += ibit;
    }

    DWORD ifp1 = ip1;
    while (ifp1 < ip2) {
      DWORD  ifp2 = ifp1 << 1;
      double theta = isign * 6.28318530717958647692 / (ifp2 / ip1);
      double wtemp = sin(0.5 * theta);
      double wpr = -2.0 * wtemp * wtemp;
      double wpi = sin(theta);
      double wr = 1.0;
      double wi = 0.0;
      DWORD  i3;
      for (i3 = 1; i3 <= ifp1; i3 += ip1) {
        DWORD i1;
        for (i1 = i3; i1 <= i3 + ip1 - 2; i1 += 2) {
          DWORD i2a;
          for (i2a = i1; i2a <= ip3; i2a += ifp2) {
            DWORD  k1 = i2a + ifp1;
            double tempr = wr * data[k1] - wi * data[k1 + 1];
            double tempi = wr * data[k1 + 1] + wi * data[k1];
            data[k1] = static_cast<float>(data[i2a] - tempr);
            data[k1 + 1] = static_cast<float>(data[i2a + 1] - tempi);
            data[i2a] = static_cast<float>(data[i2a] + tempr);
            data[i2a + 1] = static_cast<float>(data[i2a + 1] + tempi);
          }
        }
        wtemp = wr;
        wr = wr * wpr - wi * wpi + wr;
        wi = wi * wpr + wtemp * wpi + wi;
      }
      ifp1 = ifp2;
    }
    nprev *= n;
  }
}

void ChunkLodIdx::GenEdgeIndices(UINT edgeTris, UINT r1, UINT r1Delta, UINT r2, UINT r2Delta, WORD *idx) {
  UINT i;
  UINT count = edgeTris / 2;

  *idx++ = r1;
  for (i = 0; i < count; ++i) {
    *idx++ = r1;
    r1 += r1Delta;
    *idx++ = r2;
    r2 += r2Delta;
  }

  *idx++ = r1 - r1Delta;
  for (i = 0; i < count; ++i) {
    *idx++ = r2;
    r2 += r2Delta;
    *idx++ = r1;
    r1 += r1Delta;
  }

  *idx = r2 - r2Delta;
}

void ChunkLodIdx::GenLinkIndices(UINT numQuads, UINT r1, UINT r1Delta, UINT r2, UINT r2Delta, WORD *idx) {
  UINT i;
  UINT halfQuads = numQuads / 2 - 1;
  WORD r1Delta2x = r1Delta * 2;

  *idx++ = r1;
  for (i = 0; i < halfQuads; ++i) {
    *idx++ = r1;
    *idx++ = r2;
    r1 += r1Delta2x;
    r2 += r2Delta;
    *idx++ = r1;
    *idx++ = r2;
    r2 += r2Delta;
    *idx++ = r1;
  }

  *idx++ = r1;
  r1 += r1Delta2x;
  *idx++ = r2;
  *idx++ = r1;
  *idx = r1;
}

void ChunkLodIdx::GenCenterIndicesRow(UINT nQuads, UINT r[2], UINT i0, UINT i1, WORD *&idx) {
  UINT i;
  UINT halfQuads = nQuads / 2;

  *idx++ = r[i1];
  *idx++ = r[i1]++;
  *idx++ = r[i0]++;
  for (i = 0; i < halfQuads; ++i) {
    *idx++ = r[i1]++;
    *idx++ = r[i0]++;
  }

  *idx++ = --r[i1];
  for (i = 0; i < halfQuads; ++i) {
    *idx++ = r[i0]++;
    *idx++ = r[i1]++;
  }

  *idx++ = r[i1] - 1;
}

void ChunkLodIdx::GenCenterIndices(UINT centerQuads, UINT rowVerts, WORD *idx) {
  UINT r[2];
  UINT i;

  r[0] = rowVerts + 1;
  r[1] = r[0] + rowVerts;
  for (i = 0; i < centerQuads / 2; ++i) {
    r[0] = (i + 1) * rowVerts + 1;
    r[1] = r[0] + rowVerts;
    GenCenterIndicesRow(centerQuads / 2, r, 1, 0, idx);
  }

  for (i = 0; i < centerQuads / 2; ++i) {
    r[0] = (centerQuads / 2 + i + 1) * rowVerts + 1;
    r[1] = r[0] + rowVerts;
    GenCenterIndicesRow(centerQuads / 2, r, 0, 1, idx);
  }
}

void ChunkLodIdx::GenIndices(UINT lod) {
  UINT i;
  UINT quads = 1 << lod;
  UINT edgeTris = 2 * quads - 2;
  UINT edgeIdxs = edgeTris + 5;
  UINT rowVerts = quads + 1;
  UINT linkIdxs = edgeTris - quads / 2 + 5;
  UINT centerQuads = quads - 2;
  UINT centerIdxs = (2 * centerQuads + 5) * centerQuads;

  indices.SetCount(centerIdxs + 4 * (linkIdxs + edgeIdxs));

  UINT start = 0;
  for (i = 0; i < 4; ++i) {
    edges[i].start = start;
    edges[i].count = edgeIdxs;
    start += edgeIdxs;
  }

  GenEdgeIndices(edgeTris, 0, 1, rowVerts + 1, 1, &indices[edges[0].start]);
  GenEdgeIndices(edgeTris, rowVerts - 1, rowVerts, 2 * rowVerts - 1, rowVerts, &indices[edges[1].start]);
  GenEdgeIndices(edgeTris, 3 * rowVerts + 1, 1, 4 * rowVerts, 1, &indices[edges[2].start]);
  GenEdgeIndices(edgeTris, 0, rowVerts, rowVerts + 1, rowVerts, &indices[edges[3].start]);

  for (i = 0; i < 4; ++i) {
    links[i].start = start;
    links[i].count = linkIdxs;
    start += linkIdxs;
  }

  GenLinkIndices(edgeTris, 0, 1, rowVerts + 1, 1, &indices[links[0].start]);
  GenLinkIndices(edgeTris, rowVerts - 1, rowVerts, 2 * rowVerts - 1, rowVerts, &indices[links[1].start]);
  GenLinkIndices(edgeTris, 3 * rowVerts + 1, 1, 4 * rowVerts, 1, &indices[links[2].start]);
  GenLinkIndices(edgeTris, 0, rowVerts, rowVerts + 1, rowVerts, &indices[links[3].start]);

  center.start = start;
  center.count = centerIdxs;
  GenCenterIndices(centerQuads, rowVerts, &indices[center.start]);
}

WaveTrain train;

void CMap::WaterRipple(const NTempest::C3Vector &pos, float len, float time, float amp, float vel, float freq) {
  if (CMapArea::ccWaterRipples) {
    ITERATELIST(WaterRadWave, waterRipplesFree, wave) {
      waterRipplesActive.LinkNode(wave, LIST_TAIL, 0);
      wave->Init(pos, len, time, amp, vel, freq);
      break;
    }
  }
}

const float        WaveTrain::PHASE_GRID_SIZE = 8.333333f;
static const float MAX_WAVE_DEPTH = 150.0f;
const float        WaveTrain::DEPTH_RANGE_SCALE = 2.75f / MAX_WAVE_DEPTH;

void WaveTrain::Move(float deltat) {
  NTempest::C2Vector velocity = speed * *localToWorld.Row0AsVec2();

  pos += deltat * velocity;

  NTempest::C2Vector corner = pos - NTempest::C2Vector(
                                        halfSize.x * localToWorld.a0 + halfSize.y * localToWorld.b0,
                                        halfSize.x * localToWorld.a1 + halfSize.y * localToWorld.b1
                                    );
  float              deltax = deltat * speed;
  float             *grid = phaseGrid;

  for (int phy = 0; phy < phaseSize.y; ++phy) {
    for (int phx = 0; phx < phaseSize.x; ++phx) {
      NTempest::C2Vector localPos(phx * PHASE_GRID_SIZE, phy * PHASE_GRID_SIZE);
      NTempest::C2Vector worldPos(
          localPos.x * localToWorld.a0 + localPos.y * localToWorld.b0, localPos.x * localToWorld.a1 + localPos.y * localToWorld.b1
      );

      worldPos += corner;
      float depth = -CWorld::CalcAltitude(worldPos.x, worldPos.y, 1.0f);

      depth = min(depth, MAX_WAVE_DEPTH);
      depth = DEPTH_RANGE_SCALE * depth;
      depth = max(depth, 0.1f);
      depth = static_cast<float>(tanh(depth * 0.0025f));
      *grid++ += (1.0f / NTempest::CMath::sqrt_(depth) - 1.0f) * deltax * 0.0025f;
    }
  }
}

BOOL WaveTrain::Phase(const NTempest::C2Vector &worldPos, float &phase) {
  NTempest::C2Vector delta = worldPos - pos;
  NTempest::C2Vector localPos(
      delta.x * localToWorld.a0 + delta.y * localToWorld.a1, delta.x * localToWorld.b0 + delta.y * localToWorld.b1
  );

  if (-halfSize.x > localPos.x || localPos.x > halfSize.x || -halfSize.y > localPos.y || localPos.y > halfSize.y) {
    return 0;
  }

  localPos += halfSize;
  localPos *= 1.0f / PHASE_GRID_SIZE;

  NTempest::C2iVector ipos(localPos);
  FATALASSERT(ipos.x >= 0 || ipos.y >= 0 || ipos.x < phaseSize.x-1 || ipos.y < phaseSize.y-1);

  localPos.x -= ipos.x;
  localPos.y -= ipos.y;

  float x00 = phaseGrid[ipos.y * phaseSize.x + ipos.x];
  float x10 = phaseGrid[(ipos.y + 1) * phaseSize.x + ipos.x];
  float x0 = x00 + (phaseGrid[ipos.y * phaseSize.x + ipos.x + 1] - x00) * localPos.x;
  float x1 = x10 + (phaseGrid[(ipos.y + 1) * phaseSize.x + ipos.x + 1] - x10) * localPos.x;

  phase = x0 + (x1 - x0) * localPos.y;
  return 1;
}

void WaveTrain::Init(const NTempest::C2Vector &pPos, const NTempest::C2Vector &pSize, const float radAngle, const float pSpeed) {
  pos = pPos;
  halfSize = 0.5f * pSize;

  phaseSize.x = static_cast<int>(ceil(pSize.x * (1.0f / PHASE_GRID_SIZE))) + 1;
  phaseSize.y = static_cast<int>(ceil(pSize.y * (1.0f / PHASE_GRID_SIZE))) + 1;
  speed = pSpeed;
  localToWorld = NTempest::C22Matrix::Rotation(radAngle);

  float radius = pSize.y < pSize.x ? pSize.x : pSize.y;
  radiusSq = radius * radius;

  NTempest::C2Vector phasePos = pos - halfSize;
  float             *grid = phaseGrid;

  for (int phy = 0; phy < phaseSize.y; ++phy) {
    for (int phx = 0; phx < phaseSize.x; ++phx) {
      *grid++ = phx + phasePos.x;
    }
  }
}

BOOL WaveTrain::Contains(const NTempest::C2Vector &worldPos) {
  return (worldPos - pos).SquaredMag() < radiusSq;
}

void CMap::WaterInitialize() {
  skyTexid = 0;
  riverDiffTexid = 0;
  oceanDiffTexid = 0;

  {
    for (UINT i = 0; i < NUM_RIPPLES; ++i) {
      LPVOID        storage = SMemAlloc(sizeof(WaterRadWave), typeid(WaterRadWave).INTERNALRAWNAME(), SERR_LINECODE_OBJECT, SMEM_FLAG_ZEROMEMORY);
      WaterRadWave *wave = storage ? new (storage) WaterRadWave : 0;

      waterRipplesFree.LinkNode(wave, LIST_TAIL, 0);
    }
  }

  memset(liquidTex, 0, sizeof(liquidTex));
  memset(liquidTexLoaded, 0, sizeof(liquidTexLoaded));

  {
    for (UINT i = 0; i < 256; ++i) {
      if (MD_OCEAN_DEPTH_SCALE * i <= 26.666666f) {
        s_oceanDepthCoordTable[i] = MD_OCEAN_DEPTH_SCALE * i * 0.037500001f;
      } else {
        s_oceanDepthCoordTable[i] = 1.0f;
      }

      if (MD_RIVER_DEPTH_SCALE * i <= 4.6666665f) {
        s_riverDepthCoordTable[i] = MD_RIVER_DEPTH_SCALE * i * 0.21428572f;
      } else {
        s_riverDepthCoordTable[i] = 1.0f;
      }
    }
  }

  {
    for (UINT i = 0; i < 256; ++i) {
      float thetai = static_cast<float>(acos(static_cast<float>(i) * 0.0039215689f));
      float thetat = static_cast<float>(asin(sin(thetai) * 0.74626863f));
      float fs;

      if (thetai == 0.0f) {
        fs = 0.021111846f;
      } else {
        fs = static_cast<float>(sin(thetat - thetai) / sin(thetat + thetai));
        float tanRatio = static_cast<float>(tan(thetat - thetai) / tan(thetat + thetai));

        fs = 0.5f * (fs * fs + tanRatio * tanRatio);
      }

      s_reflectivity[i].Set(
          static_cast<BYTE>(fs * 255.0f), static_cast<BYTE>(fs * 255.0f), static_cast<BYTE>(fs * 255.0f), static_cast<BYTE>(fs * 255.0f)
      );
    }
  }

  if (!skyTexid) {
    skyTexels.SetCount(static_cast<UINT>(Gx_MinTexAspect * SKYTEX_HEIGHT * SKYTEX_HEIGHT));
    GxTexCreate(
        static_cast<UINT>(Gx_MinTexAspect * SKYTEX_HEIGHT), SKYTEX_HEIGHT, GxTex_Argb8888, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &skyTexels[0],
        DayNightSkyTexCallback, skyTexid
    );
  }

  GxTexCreate(
      static_cast<UINT>(Gx_MinTexAspect * WATERTEX_HEIGHT), WATERTEX_HEIGHT, GxTex_Argb8888, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1),
      reinterpret_cast<LPVOID>(1), WaterDiffTexCallback, riverDiffTexid
  );
  GxTexCreate(
      static_cast<UINT>(Gx_MinTexAspect * WATERTEX_HEIGHT), WATERTEX_HEIGHT, GxTex_Argb8888, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), 0,
      WaterDiffTexCallback, oceanDiffTexid
  );
  GxPixelShaderCreate(psOcean0, "Shaders\\Pixel\\Ocean0.bls");
}

void CMap::WaterDestroy() {
  if (skyTexid) {
    GxTexDestroy(skyTexid);
  }
  skyTexid = 0;

  if (riverDiffTexid) {
    GxTexDestroy(riverDiffTexid);
  }
  riverDiffTexid = 0;

  if (oceanDiffTexid) {
    GxTexDestroy(oceanDiffTexid);
  }
  oceanDiffTexid = 0;

  for (UINT liquid = 0; liquid < LIQUID_COUNT; ++liquid) {
    UnloadLiquidTexture(liquid);
  }

  waterRipplesFree.Clear();
  waterRipplesActive.Clear();

  GxPixelShaderDestroy(psOcean0);
}

static void SetupBufCmd(CGxBuf *gxBuf, CGxBufCommand &cmd, CGxVertexPNT0 *&vtx, WORD *&idx) {
  FATALASSERT(cmd.vertex.op != GxBufOp_Nop);
  FATALASSERT(cmd.index.op != GxBufOp_Nop);

  if (cmd.vertex.op == GxBufOp_Fill) {
    vtx = static_cast<CGxVertexPNT0 *>(*cmd.vertex.mem[GxVM_Position]);
  } else {
    vtx = static_cast<CGxVertexPNT0 *>(GxAllocVertexMem(gxBuf->VertexCount() * sizeof(*vtx)));
    *cmd.vertex.mem[GxVM_Position] = &vtx->p;
    *cmd.vertex.mem[GxVM_Normal] = &vtx->n;
    *cmd.vertex.mem[GxVM_Texture0] = &vtx->tc[0];
  }

  if (cmd.index.op == GxBufOp_Fill) {
    idx = static_cast<WORD *>(*cmd.index.mem[GxVM_Position]);
  } else {
    idx = static_cast<WORD *>(GxAllocIndexMem(gxBuf->IndexCount() * sizeof(*idx)));
    *cmd.index.mem[GxVM_Position] = idx;
  }
}

static void SetupBufCmd(CGxBuf *gxBuf, CGxBufCommand &cmd, CGxVertexPCT0 *&vtx, WORD *&idx) {
  FATALASSERT(cmd.vertex.op != GxBufOp_Nop);
  FATALASSERT(cmd.index.op != GxBufOp_Nop);

  if (cmd.vertex.op == GxBufOp_Fill) {
    vtx = static_cast<CGxVertexPCT0 *>(*cmd.vertex.mem[GxVM_Position]);
  } else {
    vtx = static_cast<CGxVertexPCT0 *>(GxAllocVertexMem(gxBuf->VertexCount() * sizeof(*vtx)));
    *cmd.vertex.mem[GxVM_Position] = &vtx->p;
    *cmd.vertex.mem[GxVM_Color] = &vtx->c;
    *cmd.vertex.mem[GxVM_Texture0] = &vtx->tc[0];
  }

  if (cmd.index.op == GxBufOp_Fill) {
    idx = static_cast<WORD *>(*cmd.index.mem[GxVM_Position]);
  } else {
    idx = static_cast<WORD *>(GxAllocIndexMem(gxBuf->IndexCount() * sizeof(*idx)));
    *cmd.index.mem[GxVM_Position] = idx;
  }
}

#pragma optimize("", off)

void CChunkLiquid::RenderOcean0V(CGxVertexPNT0 *vtx) {
  UINT               tx;
  UINT               vrowx;
  float              fx;
  UINT               ty;
  float              dy;
  NTempest::C3Vector vertWorldPos;
  NTempest::C3Vector dumbNormal(0.0f, 0.0f, 1.0f);
  NTempest::C2Vector farCorner(
      static_cast<float>(chunk->cOffset.x + 1) * 33.333332f, static_cast<float>(chunk->cOffset.y + 1) * 33.333332f
  );
  float              dx;
  float              temp;

  temp = -farCorner.x + 17066.666f;
  farCorner.x = -farCorner.y + 17066.666f;
  farCorner.y = temp;
  dy = (farCorner.x - chunk->corner.x) / 8.0f;
  dx = (farCorner.y - chunk->corner.y) / 8.0f;

  for (ty = 0; ty < 9; ++ty) {
    vrowx = ty * 9;
    fx = chunk->corner.x + static_cast<float>(ty) * dy;
    for (tx = 0; tx < 9; ++tx, ++vtx) {
      vertWorldPos.x = fx;
      vertWorldPos.y = chunk->corner.y + static_cast<float>(tx) * dx;
      vtx->p = vertWorldPos - chunk->corner;
      vtx->n = dumbNormal;
      vtx->tc[0] = NTempest::C2Vector(0.5f, s_oceanDepthCoordTable[verts[vrowx + tx].oceanVert.depth]);
    }
  }
}

void CChunkLiquid::RenderRiver0V(CGxVertexPNT0 *vtx) {
  float              dsq;
  UINT               vrow1;
  UINT               tx;
  float              fx;
  UINT               ty;
  float              dy;
  const float        OO_MAX_RIVER_COLOR_DSQ = 1.0f / 225.0f;
  NTempest::C3Vector dumbNormal(0.0f, 0.0f, 1.0f);
  NTempest::C2Vector farCorner(
      static_cast<float>(chunk->cOffset.x + 1) * 33.333332f, static_cast<float>(chunk->cOffset.y + 1) * 33.333332f
  );
  float              dx;
  float              temp;

  temp = -farCorner.x + 17066.666f;
  farCorner.x = -farCorner.y + 17066.666f;
  farCorner.y = temp;
  dy = (farCorner.x - chunk->corner.x) / 8.0f;
  dx = (farCorner.y - chunk->corner.y) / 8.0f;

  for (ty = 0; ty < 9; ++ty) {
    fx = chunk->corner.x + static_cast<float>(ty) * dy;
    for (tx = 0; tx < 9; ++tx, ++vtx) {
      vrow1 = ty * 9 + tx;
      SWVert            &waterVert = verts[vrow1].waterVert;
      NTempest::C3Vector vertWorldPos(fx, chunk->corner.y + static_cast<float>(tx) * dx, waterVert.height);
      vtx->p = vertWorldPos - chunk->corner;
      vtx->n = dumbNormal;
      if (!CWorldScene::camLiquid) {
        NTempest::C3Vector diffv = vertWorldPos - CWorldScene::camPos;
        dsq = NTempest::C3Vector::Dot(diffv, diffv);
        vtx->tc[0] = NTempest::C2Vector(0.5f, dsq * OO_MAX_RIVER_COLOR_DSQ);
      } else {
        vtx->tc[0] = NTempest::C2Vector(0.5f, s_riverDepthCoordTable[waterVert.depth]);
      }
    }
  }
}

void CChunkLiquid::RenderMagma0V(CGxVertexPCT0 *vtx) {
  UINT                vrow1;
  float               fx;
  float               dy;
  const float         MAGMA_TILES = 3.0f;
  const float         MAGMA_TEX_SCALE = MAGMA_TILES / 256.0f;
  const float         MAGMA_SCROLL_RATE = 0.025f;
  float               dx;
  float               scrollx;
  float               cycles;
  float               temp;

  cycles = CWorld::GetCurTimeSec() * MAGMA_SCROLL_RATE;
  scrollx = cycles - static_cast<float>(Fast_ftol(cycles));
  NTempest::C2Vector farCorner(
      static_cast<float>(chunk->cOffset.x + 1) * 33.333332f, static_cast<float>(chunk->cOffset.y + 1) * 33.333332f
  );
  temp = -farCorner.x + 17066.666f;
  farCorner.x = -farCorner.y + 17066.666f;
  farCorner.y = temp;
  dy = (farCorner.x - chunk->corner.x) / 8.0f;
  dx = (farCorner.y - chunk->corner.y) / 8.0f;

  NTempest::C2iVector t(0);
  NTempest::C2iVector v(0);
  for (v.y = 0, t.y = 0; static_cast<UINT>(v.y) < 9; ++v.y) {
    fx = chunk->corner.x + static_cast<float>(v.y) * dy;
    for (v.x = 0, t.x = 0; static_cast<UINT>(v.x) < 9; ++v.x, ++vtx) {
      vrow1 = v.y * 9 + v.x;
      SMVert            &magmaVert = verts[vrow1].magmaVert;
      NTempest::C3Vector vertWorldPos(fx, chunk->corner.y + static_cast<float>(v.x) * dx, magmaVert.height);
      vtx->p = vertWorldPos - chunk->corner;
      vtx->c = 0xFFFFFFFF;
      vtx->tc[0] = NTempest::C2Vector(static_cast<float>(magmaVert.s) * MAGMA_TEX_SCALE + scrollx, static_cast<float>(magmaVert.t) * MAGMA_TEX_SCALE);
      t.x += v.x > 0;
    }
    t.y += v.y > 0;
  }
}

#pragma optimize("", on)

WORD CChunkLiquid::Render0I(WORD *idxBase, UINT liquidType) {
  WORD  i2;
  UINT  ty;
  WORD  lastRenderedVtx = 0;
  BYTE  inStrip = 0;
  WORD *idx = idxBase;

  for (ty = 0; ty < 8; ++ty) {
    WORD i0 = static_cast<WORD>(9 * ty);
    i2 = static_cast<WORD>(i0 + 10);
    while (i0 < 9 * ty + 8) {
      if ((tiles.tiles[ty][i0 - 9 * ty] & 0xF) == liquidType) {
        if (!inStrip) {
          *idx++ = i0;
          *idx++ = i0;
          *idx++ = static_cast<WORD>(i0 + 9);
          inStrip = 1;
        }
        *idx++ = static_cast<WORD>(i0 + 1);
        *idx++ = i2;
        lastRenderedVtx = i2;
      } else if (inStrip) {
        *idx++ = lastRenderedVtx;
        inStrip = 0;
      }
      ++i0;
      ++i2;
    }
    if (inStrip) {
      *idx++ = lastRenderedVtx;
      inStrip = 0;
    }
  }
  return static_cast<WORD>(idx - idxBase);
}

void CChunkLiquid::RenderOcean0Callback(CGxBufCommand &cmd, CGxBuf *gxBuf) {
  WORD          *idx;
  CGxVertexPNT0 *vtx;
  UserArg       *arg = static_cast<UserArg *>(gxBuf->UserArg());
  SetupBufCmd(gxBuf, cmd, vtx, idx);
  arg->liquid->RenderOcean0V(vtx);
  arg->indexCount = arg->liquid->Render0I(idx, arg->liquidType);
  FATALASSERT(arg->indexCount <= gxBuf->IndexCount());
}

void CChunkLiquid::RenderRiver0Callback(CGxBufCommand &cmd, CGxBuf *gxBuf) {
  WORD          *idx;
  CGxVertexPNT0 *vtx;
  UserArg       *arg = static_cast<UserArg *>(gxBuf->UserArg());
  SetupBufCmd(gxBuf, cmd, vtx, idx);
  arg->liquid->RenderRiver0V(vtx);
  arg->indexCount = arg->liquid->Render0I(idx, arg->liquidType);
  FATALASSERT(arg->indexCount <= gxBuf->IndexCount());
}

void CChunkLiquid::RenderMagma0Callback(CGxBufCommand &cmd, CGxBuf *gxBuf) {
  WORD          *idx;
  CGxVertexPCT0 *vtx;
  UserArg       *arg = static_cast<UserArg *>(gxBuf->UserArg());
  SetupBufCmd(gxBuf, cmd, vtx, idx);
  arg->liquid->RenderMagma0V(vtx);
  arg->indexCount = arg->liquid->Render0I(idx, arg->liquidType);
  FATALASSERT(arg->indexCount <= gxBuf->IndexCount());
}

void CChunkLiquid::RenderOcean0() {
  CGxTex *texture = TextureGetGxTex(CMap::GetLiquidTexture(1), 0, 0);
  if (CMap::liquidTexLoaded[1]) {
    GxRsSet(GxRs_Texture1, texture);
    UserArg arg(this, 1);
    CGxBuf *gxBuf = GxBufGetDynamic(GxVBF_PNT0);
    gxBuf->UserArgSet(&arg);
    gxBuf->UserCallbackSet(RenderOcean0Callback);
    gxBuf->CountSet(81, 192);
    GxBufLock(gxBuf);
    FATALASSERT(arg.indexCount <= gxBuf->IndexCount());
    GxBufRender(CGxBatch(GxPrim_TriangleStrip, arg.indexCount, 0, 0, gxBuf->VertexCount() - 1));
    GxBufUnlock();
  }
}

void CChunkLiquid::RenderRiver0(UINT type) {
  CGxTex *texture = TextureGetGxTex(CMap::GetLiquidTexture(4), 0, 0);
  if (CMap::liquidTexLoaded[4]) {
    GxRsSet(GxRs_Texture1, texture);
    UserArg arg(this, 4);
    CGxBuf *gxBuf = GxBufGetDynamic(GxVBF_PNT0);
    gxBuf->UserArgSet(&arg);
    gxBuf->UserCallbackSet(RenderRiver0Callback);
    gxBuf->CountSet(81, 192);
    GxBufLock(gxBuf);
    FATALASSERT(arg.indexCount <= gxBuf->IndexCount());
    GxBufRender(CGxBatch(GxPrim_TriangleStrip, arg.indexCount, 0, 0, gxBuf->VertexCount() - 1));
    GxBufUnlock();
  }
}

void CChunkLiquid::RenderMagma0(UINT type) {
  CGxTex *texture = TextureGetGxTex(CMap::GetLiquidTexture(6), 0, 0);
  if (CMap::liquidTexLoaded[6]) {
    GxRsSet(GxRs_Texture0, texture);
    UserArg arg(this, 6);
    CGxBuf *gxBuf = GxBufGetDynamic(GxVBF_PCT0);
    gxBuf->UserArgSet(&arg);
    gxBuf->UserCallbackSet(RenderMagma0Callback);
    gxBuf->CountSet(81, 192);
    GxBufLock(gxBuf);
    FATALASSERT(arg.indexCount <= gxBuf->IndexCount());
    GxBufRender(CGxBatch(GxPrim_TriangleStrip, arg.indexCount, 0, 0, gxBuf->VertexCount() - 1));
    GxBufUnlock();
  }
}

void CChunkLiquid::Render(UINT type) {
  switch (type) {
    case 0: {
      if (!CMap::riverDiffTexUpdated) {
        NTempest::CiRect texRect(0, 0, CMap::WATERTEX_HEIGHT, static_cast<int>(Gx_MinTexAspect * CMap::WATERTEX_HEIGHT));
        GxTexUpdate(CMap::riverDiffTexid, texRect, 0);
        CMap::riverDiffTexUpdated = true;
      }
      RenderRiver0(type);
      break;
    }
    case 1: {
      if (!CMap::oceanDiffTexUpdated) {
        NTempest::CiRect texRect(0, 0, CMap::WATERTEX_HEIGHT, static_cast<int>(Gx_MinTexAspect * CMap::WATERTEX_HEIGHT));
        GxTexUpdate(CMap::oceanDiffTexid, texRect, 0);
        CMap::oceanDiffTexUpdated = true;
      }
      RenderOcean0();
      break;
    }
    case 2:
      RenderMagma0(type);
      break;
  }
}

void CChunkLiquid::GetAaBox(NTempest::CAaBox &aaBox) {
  aaBox = chunk->aaBox;
  aaBox.b.z = height.l;
  aaBox.t.z = height.h;
}

void Particulate::InitMovement() {
  const float pi = 3.1415927f;
  float       rotY = NTempest::CRandom::reals_(g_rndSeed) * pi;
  float       rotZ = NTempest::CRandom::reals_(g_rndSeed) * pi;

  movement.dir.y = NTempest::CMath::sin_(rotZ) * NTempest::CMath::sin_(rotY);
  movement.dir.x = NTempest::CMath::cos_(rotZ) * NTempest::CMath::sin_(rotY);
  movement.dir.z = NTempest::CMath::cos_(rotY) * 0.25f;

  if (movement.dir.z < 0.0f) {
    movement.dir.z = -movement.dir.z;
  }

  movement.dir.Normalize();
  movement.time = 0.0f;
  movement.freq = (NTempest::CRandom::real_(g_rndSeed) + 1.0f) * 0.0125f;
  movement.amplitude = (NTempest::CRandom::real_(g_rndSeed) + 1.0f) * 0.005f;
}

NTempest::C3Vector Particulate::ComputeMovement(float elapsedTime) {
  if (liquid <= 1) {
    movement.time += elapsedTime;
    float amp = movement.time * movement.freq;
    if (amp > 0.5f) {
      InitMovement();
      amp = 0.0f;
    }

    amp = NTempest::CMath::sin_(amp * 6.2831855f) * movement.amplitude;
    return movement.dir * amp;
  }

  if (liquid == 2) {
    return NTempest::C3Vector(0.0f, 0.0f, -0.02f * elapsedTime);
  }

  return NTempest::C3Vector(0.0f, 0.0f, 0.0f);
}

Particulate::Particulate(float particleScale, float boxSize, LPCSTR particulateTexture) : show(0) {
  SetPercentage(1.0f);
  SetScale(particleScale);
  SetSize(boxSize);

  texture = 0;
  SetTexture(particulateTexture);
  InitParticles(1);
  InitMovement();
}

Particulate::~Particulate() {
  if (texture) {
    HandleClose(texture);
  }
}

void Particulate::SetPercentage(float percent) {
  ASSERT(percent >= 0.0f && percent <= 1.0f);
  numParticles = static_cast<UINT>(percent * 4000.0f);
}

void Particulate::SetScale(float s) {
  scale = s;
}

void Particulate::SetSize(float units) {
  boxSize = units;
}

void Particulate::SetTexture(LPCSTR name) {
  if (texture) {
    HandleClose(texture);
  }

  CStatus status;
  texture = TextureCreate(name, CGxTexFlags(GxTex_LinearMipNearest, 0, 0, 0, 0, 0, 1), &status, 0);
  SysMsgAdd(status, 2);
}

void Particulate::InitParticles(UINT l) {
  float scaleMin = scale * 0.5f;
  float scaleDiff = scale * 1.5f - scaleMin;
  float halfBoxSize = boxSize * 0.5f;

  for (UINT lp = 0; lp < numParticles; ++lp) {
    particles[lp].pos = NTempest::C3Vector(
        NTempest::CRandom::real_(g_rndSeed) * boxSize - halfBoxSize, NTempest::CRandom::real_(g_rndSeed) * boxSize - halfBoxSize,
        NTempest::CRandom::real_(g_rndSeed) * boxSize - halfBoxSize
    );
    particles[lp].scale = NTempest::CRandom::real_(g_rndSeed) * scaleDiff + scaleMin;
  }

  liquid = l & 3;
}

void Particulate::Update() {
  if (!show) {
    return;
  }

  float              halfBoxSize = boxSize * 0.5f;
  NTempest::C3Vector delta = lastCamPos - CWorldScene::camPos;
  lastCamPos = CWorldScene::camPos;

  if (delta.SquaredMag() > boxSize * boxSize) {
    InitParticles(liquid);
    delta = NTempest::C3Vector(0.0f, 0.0f, 0.0f);
  }

  delta += ComputeMovement(CWorld::GetTickTimeSec());

  for (UINT lp = 0; lp < numParticles; ++lp) {
    particles[lp].pos += delta;

    if (particles[lp].pos.x > halfBoxSize) {
      particles[lp].pos.x -= boxSize;
    } else if (particles[lp].pos.x < -halfBoxSize) {
      particles[lp].pos.x += boxSize;
    }

    if (particles[lp].pos.y > halfBoxSize) {
      particles[lp].pos.y -= boxSize;
    } else if (particles[lp].pos.y < -halfBoxSize) {
      particles[lp].pos.y += boxSize;
    }

    if (particles[lp].pos.z > halfBoxSize) {
      particles[lp].pos.z -= boxSize;
    } else if (particles[lp].pos.z < -halfBoxSize) {
      particles[lp].pos.z += boxSize;
    }
  }
}

const float                       Particulate::PTSIZE = 0.5f;
NTempest::C3Vector                Particulate::s_vcv[4] = {
    NTempest::C3Vector(-PTSIZE, PTSIZE, 0.0f), NTempest::C3Vector(-PTSIZE, -PTSIZE, 0.0f), NTempest::C3Vector(PTSIZE, PTSIZE, 0.0f),
    NTempest::C3Vector(PTSIZE, -PTSIZE, 0.0f)
};
NTempest::C2Vector Particulate::s_tc[13][4] = {
    {       NTempest::C2Vector(0.0f,        0.0f),        NTempest::C2Vector(0.0f, 0.19921875f), NTempest::C2Vector(0.19921875f,        0.0f),
     NTempest::C2Vector(0.19921875f, 0.19921875f)},
    {NTempest::C2Vector(0.19921875f,        0.0f), NTempest::C2Vector(0.19921875f, 0.19921875f),  NTempest::C2Vector(0.3984375f,        0.0f),
     NTempest::C2Vector(0.3984375f, 0.19921875f) },
    { NTempest::C2Vector(0.3984375f,        0.0f),  NTempest::C2Vector(0.3984375f, 0.19921875f), NTempest::C2Vector(0.59765625f,        0.0f),
     NTempest::C2Vector(0.59765625f, 0.19921875f)},
    {NTempest::C2Vector(0.59765625f,        0.0f), NTempest::C2Vector(0.59765625f, 0.19921875f),   NTempest::C2Vector(0.796875f,        0.0f),
     NTempest::C2Vector(0.796875f, 0.19921875f)  },
    {       NTempest::C2Vector(0.0f, 0.19921875f),        NTempest::C2Vector(0.0f,  0.3984375f), NTempest::C2Vector(0.19921875f, 0.19921875f),
     NTempest::C2Vector(0.19921875f,  0.3984375f)},
    {NTempest::C2Vector(0.19921875f, 0.19921875f), NTempest::C2Vector(0.19921875f,  0.3984375f),  NTempest::C2Vector(0.3984375f, 0.19921875f),
     NTempest::C2Vector(0.3984375f,  0.3984375f) },
    { NTempest::C2Vector(0.3984375f, 0.19921875f),  NTempest::C2Vector(0.3984375f,  0.3984375f), NTempest::C2Vector(0.59765625f, 0.19921875f),
     NTempest::C2Vector(0.59765625f,  0.3984375f)},
    {NTempest::C2Vector(0.59765625f, 0.19921875f), NTempest::C2Vector(0.59765625f,  0.3984375f),   NTempest::C2Vector(0.796875f, 0.19921875f),
     NTempest::C2Vector(0.796875f,  0.3984375f)  },
    {  NTempest::C2Vector(0.796875f,        0.0f),   NTempest::C2Vector(0.796875f, 0.19921875f), NTempest::C2Vector(0.99609375f,        0.0f),
     NTempest::C2Vector(0.99609375f, 0.19921875f)},
    {       NTempest::C2Vector(0.0f,  0.3984375f),        NTempest::C2Vector(0.0f, 0.59765625f), NTempest::C2Vector(0.19921875f,  0.3984375f),
     NTempest::C2Vector(0.19921875f, 0.59765625f)},
    {NTempest::C2Vector(0.19921875f,  0.3984375f), NTempest::C2Vector(0.19921875f, 0.59765625f),  NTempest::C2Vector(0.3984375f,  0.3984375f),
     NTempest::C2Vector(0.3984375f, 0.59765625f) },
    { NTempest::C2Vector(0.3984375f,  0.3984375f),  NTempest::C2Vector(0.3984375f, 0.59765625f), NTempest::C2Vector(0.59765625f,  0.3984375f),
     NTempest::C2Vector(0.59765625f, 0.59765625f)},
    {NTempest::C2Vector(0.59765625f,  0.3984375f), NTempest::C2Vector(0.59765625f, 0.59765625f),   NTempest::C2Vector(0.796875f,  0.3984375f),
     NTempest::C2Vector(0.796875f, 0.59765625f)  }
};
UINT Particulate::s_tcSub[4][8] = {
    {0,  1,  2,  3, 4,  5,  6,  7},
    {0,  1,  2,  3, 4,  5,  6,  7},
    {9, 10, 11, 12, 9, 10, 11, 12},
    {0,  0,  0,  0, 0,  0,  0,  0}
};

void Particulate::Render() {
  if (!show) {
    return;
  }

  CGxVertexPCT0 *vtxBase = static_cast<CGxVertexPCT0 *>(GxAllocVertexMem(2664 * sizeof(*vtxBase)));
  WORD          *idxBase = static_cast<WORD *>(GxAllocIndexMem(3996 * sizeof(*idxBase)));

  NTempest::C44Matrix view;
  GxXformView(view);
  GxXformSetView(NTempest::C44Matrix());

  UINT  nVerts = 0;
  WORD *idx = idxBase;
  UINT  tcSub = 8;
  for (UINT lp = 0; lp < numParticles; ++lp) {
    const Particle    &particle = particles[lp];
    NTempest::C3Vector vp(
        view.a0 * particle.pos.x + view.b0 * particle.pos.y + view.c0 * particle.pos.z,
        view.a1 * particle.pos.x + view.b1 * particle.pos.y + view.c1 * particle.pos.z,
        view.a2 * particle.pos.x + view.b2 * particle.pos.y + view.c2 * particle.pos.z
    );

    if (vp.z > 0.0f && vp.x < vp.z && vp.x > -vp.z && vp.y < vp.z && vp.y > -vp.z) {
      CGxVertexPCT0 *vtx = vtxBase + nVerts;
      for (UINT i = 0; i < 4; ++i) {
        vtx[i].p.x = vp.x + s_vcv[i].x * particle.scale;
        vtx[i].p.y = vp.y + s_vcv[i].y * particle.scale;
        vtx[i].p.z = vp.z;
        vtx[i].c = -1;
        vtx[i].tc[0] = s_tc[tcSub][i];
      }

      idx[0] = nVerts;
      idx[1] = nVerts + 1;
      idx[2] = nVerts + 2;
      idx[3] = nVerts + 3;
      idx[4] = nVerts + 2;
      idx[5] = nVerts + 1;
      idx += 6;
      nVerts += 4;
      if ((nVerts & ~3U) >= 2664) {
        break;
      }
    }

    tcSub = s_tcSub[liquid][lp & 7];
  }

  GxRsPush();
  GxVertexShaderSelect(GxVS_PassThru);
  GxRsSet(GxRs_Texture0, TextureGetGxTex(texture, 1, 0));
  GxRsSet(GxRs_Blend, GxBlend_Alpha);
  GxRsSet(GxRs_Fog, liquid == 2);
  GxRsSet(GxRs_DepthWrite, 0);
  GxRsSet(GxRs_Lighting, 0);
  GxPrimLockVertexPtrs(nVerts, &vtxBase->p, sizeof(*vtxBase), 0, 0, &vtxBase->c, sizeof(*vtxBase), 0, 0, &vtxBase->tc[0], sizeof(*vtxBase), 0, 0);
  GxPrimDrawElements(GxPrim_Triangles, idx - idxBase, idxBase);
  GxPrimUnlockVertexPtrs();
  GxRsPop();
  GxXformSetView(view);
}

void Particulate::CustomRenderCallback(LPVOID p1, int p2) {
  static_cast<Particulate *>(p1)->Render();
}
