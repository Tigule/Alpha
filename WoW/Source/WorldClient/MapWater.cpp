#include "WorldClient/World.h"

#include "DayNight.h"
#include "Base/Base.h"
#include "Base/Handle.h"
#include "Base/Status.h"
#include "Gx/Gx.h"
#include "Services/SysMessage.h"
#include "Services/Texture.h"
#include "Tempest/cmath.h"

#include "Client.h"

#include <math.h>
#include <new>
#include <string.h>
#include <typeinfo>

struct LODIndexFix {
  unsigned short from;
  unsigned short to;
};

struct LODArrays {
  TSGrowableArray<NTempest::C2Vector> geov;
  TSGrowableArray<NTempest::C2Vector> texv;
  TSGrowableArray<unsigned short>     idx;
  unsigned int                        nFixes;
  TSGrowableArray<LODIndexFix>        fixes;

  void GenFixes(unsigned int p_nFixes, unsigned int vertsPerSide, unsigned int tilesPerSide);
  void GenVerts(unsigned int lod);
};

static unsigned int               s_lodSubdivs[5] = {0, 1, 3, 7, 15};
static TSGrowableArray<LODArrays> s_lodArrays;
static NTempest::CImVector       *pixels;
static const float                kDeepDarken = 0.75f;
static const float                MD_OCEAN_RAW_SCALE = -21.0f;
static const float                MD_OCEAN_DEPTH_SCALE = MD_OCEAN_RAW_SCALE * (-1.0f / 36.0f);
static const float                MD_MAX_DEPTH = MD_OCEAN_DEPTH_SCALE * -255.0f;
static const float                MD_RIVER_DEPTH_SCALE = 1.0f / 9.0f;
static const float                Gx_MinTexAspect = 0.125f;
static float                      s_oceanDepthCoordTable[256];
static float                      s_riverDepthCoordTable[256];
static NTempest::CImVector        s_reflectivity[256];

CGxTex                           *CMap::skyTexid;
CGxTex                           *CMap::riverDiffTexid;
CGxTex                           *CMap::oceanDiffTexid;
const unsigned int                CMap::SKYTEX_HEIGHT = 64;
const unsigned int                CMap::WATERTEX_HEIGHT = 64;
TSFixedArray<NTempest::CImVector> CMap::skyTexels;
HTEXTURE__                       *CMap::liquidTex[LIQUID_COUNT][LIQUID_TEXTURE_COUNT];
bool                              CMap::liquidTexLoaded[LIQUID_COUNT];
float                             CMap::liquidLastShown[LIQUID_COUNT];
const float                       CMap::liquidTexLoopTime[LIQUID_COUNT] = {1.25f, 1.25f, 1.25f, 1.25f, 1.25f, 1.25f, 1.25f, 1.25f, 1.25f};
const char                       *CMap::liquidTexBaseName[LIQUID_COUNT] = {"XTextures\\river\\lake_a.%d.blp", "XTextures\\ocean\\ocean_h.%d.blp",
                                                                           "XTextures\\lava\\lava.%d.blp",    "XTextures\\slime\\slime.%d.blp",
                                                                           "XTextures\\river\\lake_a.%d.blp", 0,
                                                                           "XTextures\\lava\\lava.%d.blp",    "XTextures\\slime\\slime.%d.blp",
                                                                           "XTextures\\river\\fast_a.%d.blp"};
bool                              CMap::riverDiffTexUpdated;
bool                              CMap::oceanDiffTexUpdated;
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
unsigned int Particulate::s_tcSub[4][8] = {
    {0,  1,  2,  3, 4,  5,  6,  7},
    {0,  1,  2,  3, 4,  5,  6,  7},
    {9, 10, 11, 12, 9, 10, 11, 12},
    {0,  0,  0,  0, 0,  0,  0,  0}
};
TSList<WaterRadWave, TSGetLink<WaterRadWave> > CMap::waterRipplesFree;
TSList<WaterRadWave, TSGetLink<WaterRadWave> > CMap::waterRipplesActive;
CGxPixelShader                                *CMap::psOcean0;

void WaterRadWave::Init(NTempest::C3Vector &p_pos, float len, float time, float amp, float vel, float freq) {
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

int CMapArea::ccWaterLOD = -1;
int CMapArea::ccWaterMaxLOD = 4;
int CMapArea::ccWaterWaves = 2;
int CMapArea::ccWaterSpecular = 1;
int CMapArea::ccWaterRipples = 1;

void LODArrays::GenFixes(unsigned int p_nFixes, unsigned int vertsPerSide, unsigned int tilesPerSide) {
  nFixes = p_nFixes;
  fixes.SetCount(4 * p_nFixes);

  unsigned int   index = 0;
  unsigned short from = 1;
  for (unsigned int i = 0; i < nFixes; ++i) {
    fixes[index].from = from;
    fixes[index].to = from - 1;
    ++index;
    from += 2;
  }

  from = static_cast<unsigned short>(2 * vertsPerSide - 1);
  unsigned short to = 0;
  for (i = 0; i < nFixes / 2; ++i) {
    fixes[index].from = from;
    fixes[index].to = to;
    ++index;
    fixes[index].from = static_cast<unsigned short>(from + 2 * vertsPerSide);
    fixes[index].to = static_cast<unsigned short>(to + 4 * vertsPerSide);
    ++index;
    from = static_cast<unsigned short>(from + 4 * vertsPerSide);
    to = static_cast<unsigned short>(to + 4 * vertsPerSide);
  }

  from = static_cast<unsigned short>(tilesPerSide * vertsPerSide + 1);
  to = static_cast<unsigned short>(tilesPerSide * vertsPerSide);
  for (i = 0; i < nFixes; ++i) {
    fixes[index].from = from;
    fixes[index].to = to;
    ++index;
    from += 2;
    to += 2;
  }

  from = static_cast<unsigned short>(tilesPerSide * tilesPerSide - 1);
  to = static_cast<unsigned short>(tilesPerSide * tilesPerSide - 2);
  for (i = 0; i < nFixes; ++i) {
    fixes[index].from = from;
    fixes[index].to = to;
    ++index;
    from = static_cast<unsigned short>(from - 2 * vertsPerSide);
    to = from - 1;
  }
}

void LODArrays::GenVerts(unsigned int lod) {
  unsigned int vertsPerSide = lod + 2;
  unsigned int vertexCount = vertsPerSide * vertsPerSide;
  geov.SetCount(vertexCount);
  texv.SetCount(vertexCount);

  unsigned int vertex = 0;
  unsigned int y = 0;
  unsigned int x;
  float        ooTiles = 1.0f / static_cast<float>(lod + 1);
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

  unsigned int indexCount = 2 * vertsPerSide * (lod + 1) + 2;
  idx.SetCount(indexCount);
  unsigned int   index = 0;
  unsigned short low = 0;
  unsigned short high = static_cast<unsigned short>(2 * vertsPerSide - 1);
  for (unsigned int row = 0; row < lod + 1; ++row) {
    for (unsigned int x = 0; x < vertsPerSide; ++x) {
      idx[index++] = low++;
      idx[index++] = high--;
    }
    low = high + 1;
    high = static_cast<unsigned short>(high + 2 * vertsPerSide);
  }
  idx[index++] = low;
  idx[index] = (lod + 1) & 1 ? static_cast<unsigned short>(low + lod + 1) : low;

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
    for (unsigned int i = 0; i < 5; ++i) {
      s_lodArrays[i].GenVerts(s_lodSubdivs[i]);
    }
  }
}

void __fastcall CMap::WaterDiffTexCallback(
    EGxTexCommand cmd,
    unsigned int  w,
    unsigned int  h,
    unsigned int  d,
    unsigned int  mipLevel,
    void         *userArg,
    unsigned int &texelStrideInBytes,
    const void  *&texels
) {
  ASSERT(mipLevel == 0);
  ASSERT(h == 64);

  switch (cmd) {
    case GxTex_Lock:
      pixels = static_cast<NTempest::CImVector *>(GxAllocPixelMem(w * h * sizeof(NTempest::CImVector)));
      break;

    case GxTex_Latch: {
      unsigned int         index = 2 * reinterpret_cast<unsigned int>(userArg);
      DNInfo              *dnInfo = DayNightGetInfo();
      NTempest::CImVector  shallowClr = dnInfo->light.WaterArray[index];
      NTempest::CImVector  deepClr = dnInfo->light.WaterArray[index + 1];
      unsigned int         redDelta = ((deepClr.r - shallowClr.r) << 8) >> 6;
      unsigned int         greenDelta = ((deepClr.g - shallowClr.g) << 8) >> 6;
      unsigned int         blueDelta = ((deepClr.b - shallowClr.b) << 8) >> 6;
      unsigned int         red = shallowClr.r << 8;
      unsigned int         green = shallowClr.g << 8;
      unsigned int         blue = shallowClr.b << 8;
      NTempest::CImVector *tex = pixels;

      for (unsigned int y = 0; y < h; ++y) {
        NTempest::CImVector rowColor;

        rowColor.Set(0xFF, static_cast<unsigned char>(red >> 8), static_cast<unsigned char>(green >> 8), static_cast<unsigned char>(blue >> 8));

        if (y == h - 1 && !userArg) {
          NTempest::C3Vector rgb = rowColor;
          NTempest::C3Vector hsv;

          NTempest::RGBtoHSV(rgb, hsv);
          hsv.z *= kDeepDarken;
          NTempest::HSVtoRGB(hsv, rgb);
          rowColor = rgb;
        }

        for (unsigned int x = 0; x < w; ++x) {
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

HTEXTURE__ *__fastcall CMap::GetLiquidTexture(unsigned int liquid) {
  char         filename[256];
  CStatus      status;
  const float  secsPerLoop = liquidTexLoopTime[liquid];
  unsigned int allLoaded;

  ASSERT(liquid < LIQUID_COUNT);

  unsigned int texture = static_cast<unsigned int>(fmod(CWorld::GetCurTimeSec(), secsPerLoop) / secsPerLoop * LIQUID_TEXTURE_COUNT - 0.5f);

  if (!liquidTexLoaded[liquid]) {
    allLoaded = 1;
    for (unsigned int i = 0; i < LIQUID_TEXTURE_COUNT; ++i) {
      if (!liquidTex[liquid][i]) {
        EGxTexFilter filter;
        if (CWorld::enables & CWorld::Enable_Anisotropic) {
          filter = GxTex_Anisotropic;
        } else if (CWorld::enables & CWorld::Enable_Trilinear) {
          filter = GxTex_LinearMipLinear;
        } else {
          filter = GxTex_LinearMipNearest;
        }

        FATALASSERT(liquidTexBaseName[liquid]);
        SStrPrintf(filename, sizeof(filename), liquidTexBaseName[liquid], i + 1);
        CGxTexFlags flags(filter, 1, 1, 0, 0, 0, CWorld::texMaxAnisotropy);
        liquidTex[liquid][i] = TextureCreate(filename, flags, &status, 0);
        SysMsgAdd(status, 2);
      }

      if (!TextureGetGxTex(liquidTex[liquid][i], 0, 0)) {
        allLoaded = 0;
      }
    }
    liquidTexLoaded[liquid] = allLoaded != 0;
  }

  liquidLastShown[liquid] = CWorld::GetCurTimeSec();
  return liquidTex[liquid][texture];
}

void __fastcall CMap::UnloadLiquidTexture(unsigned int liquid) {
  ASSERT(liquid < LIQUID_COUNT);

  for (unsigned int texture = 0; texture < LIQUID_TEXTURE_COUNT; ++texture) {
    if (liquidTex[liquid][texture]) {
      HandleClose(liquidTex[liquid][texture]);
      liquidTex[liquid][texture] = 0;
    }
  }

  liquidTexLoaded[liquid] = false;
}

void __fastcall CMap::UpdateLiquidTextures() {
}

static void fft2(float* data, unsigned long* nn, int ndim, float isign) {
    // TODO: implement
}

void __fastcall CMap::WaterRipple(NTempest::C3Vector &pos, float len, float time, float amp, float vel, float freq) {
  if (!CMapArea::ccWaterRipples || waterRipplesFree.IsEmpty()) {
    return;
  }

  WaterRadWave *wave = waterRipplesFree.Head();
  waterRipplesFree.UnlinkNode(wave);
  waterRipplesActive.LinkNode(wave, LIST_TAIL, 0);
  wave->Init(pos, len, time, amp, vel, freq);
}

void __fastcall CMap::WaterInitialize() {
  skyTexid = 0;
  riverDiffTexid = 0;
  oceanDiffTexid = 0;

  {
    for (unsigned int i = 0; i < NUM_RIPPLES; ++i) {
      void         *storage = SMemAlloc(sizeof(WaterRadWave), typeid(WaterRadWave).raw_name(), SERR_LINECODE_OBJECT, SMEM_FLAG_ZEROMEMORY);
      WaterRadWave *wave = storage ? new (storage) WaterRadWave : 0;

      waterRipplesFree.LinkNode(wave, LIST_TAIL, 0);
    }
  }

  memset(liquidTex, 0, sizeof(liquidTex));
  memset(liquidTexLoaded, 0, sizeof(liquidTexLoaded));

  {
    for (unsigned int i = 0; i < 256; ++i) {
      float oceanDepth = MD_OCEAN_DEPTH_SCALE * i;
      float riverDepth = MD_RIVER_DEPTH_SCALE * i;

      if (oceanDepth <= 26.666666f) {
        s_oceanDepthCoordTable[i] = oceanDepth * 0.037500001f;
      } else {
        s_oceanDepthCoordTable[i] = 1.0f;
      }

      if (riverDepth <= 4.6666665f) {
        s_riverDepthCoordTable[i] = riverDepth * 0.21428572f;
      } else {
        s_riverDepthCoordTable[i] = 1.0f;
      }
    }
  }

  {
    for (unsigned int i = 0; i < 256; ++i) {
      float thetai = static_cast<float>(acos(static_cast<float>(i) * 0.0039215689f));
      float thetat = static_cast<float>(asin(sin(thetai) * 0.74626863f));
      float fs;

      if (thetai == 0.0f) {
        fs = 0.021111846f;
      } else {
        float sinRatio = static_cast<float>(sin(thetat - thetai) / sin(thetat + thetai));
        float tanRatio = static_cast<float>(tan(thetat - thetai) / tan(thetat + thetai));

        fs = 0.5f * (sinRatio * sinRatio + tanRatio * tanRatio);
      }

      unsigned char value = static_cast<unsigned char>(fs * 255.0f);
      s_reflectivity[i].Set(value, value, value, value);
    }
  }

  if (!skyTexid) {
    skyTexels.SetCount(static_cast<unsigned int>(Gx_MinTexAspect * SKYTEX_HEIGHT * SKYTEX_HEIGHT));
    GxTexCreate(
        static_cast<unsigned int>(Gx_MinTexAspect * SKYTEX_HEIGHT), SKYTEX_HEIGHT, GxTex_Argb8888, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1),
        &skyTexels[0], DayNightSkyTexCallback, skyTexid
    );
  }

  GxTexCreate(
      static_cast<unsigned int>(Gx_MinTexAspect * WATERTEX_HEIGHT), WATERTEX_HEIGHT, GxTex_Argb8888, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1),
      reinterpret_cast<void *>(1), WaterDiffTexCallback, riverDiffTexid
  );
  GxTexCreate(
      static_cast<unsigned int>(Gx_MinTexAspect * WATERTEX_HEIGHT), WATERTEX_HEIGHT, GxTex_Argb8888, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), 0,
      WaterDiffTexCallback, oceanDiffTexid
  );
  GxPixelShaderCreate(psOcean0, "Shaders\\Pixel\\Ocean0.bls");
}

void __fastcall CMap::WaterDestroy() {
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

  for (unsigned int liquid = 0; liquid < LIQUID_COUNT; ++liquid) {
    UnloadLiquidTexture(liquid);
  }

  waterRipplesFree.Clear();
  waterRipplesActive.Clear();

  GxPixelShaderDestroy(psOcean0);
}

void CChunkLiquid::RenderOcean0V(CGxVertexPNT0 *vtx) {
  unsigned int       tx;
  unsigned int       vrowx;
  float              fx;
  unsigned int       ty;
  float              dy;
  NTempest::C3Vector vertWorldPos;
  NTempest::C2Vector farCorner;
  float              dx;
  NTempest::C3Vector dumbNormal(0.0f, 0.0f, 1.0f);
  float              temp;

  fx = static_cast<float>(chunk->aIndex.x + 1) * 33.333332f;
  temp = static_cast<float>(chunk->aIndex.y + 1) * 33.333332f;
  farCorner = NTempest::C2Vector(fx, temp);
  temp = 17066.666f - farCorner.x;
  farCorner.x = 17066.666f - farCorner.y;
  farCorner.y = temp;
  dy = (farCorner.x - chunk->corner.x) / 8.0f;
  dx = (farCorner.y - chunk->corner.y) / 8.0f;

  for (ty = 0; ty < 9; ++ty) {
    vrowx = 9 * ty;
    vertWorldPos.x = static_cast<float>(ty) * dy + chunk->corner.x;
    for (tx = 0; tx < 9; ++tx) {
      vertWorldPos.y = static_cast<float>(tx) * dx + chunk->corner.y;
      vtx->p = vertWorldPos - CWorldScene::camPos;
      vtx->n = dumbNormal;
      vtx->tc[0] = NTempest::C2Vector(0.5f, s_oceanDepthCoordTable[verts[vrowx + tx].oceanVert.depth]);
      ++vtx;
    }
  }
}

static void __fastcall SetupBufCmd(CGxBuf *gxBuf, CGxBufCommand &cmd, CGxVertexPNT0 *&vtx, unsigned short *&idx) {
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
    idx = static_cast<unsigned short *>(*cmd.index.mem[GxVM_Position]);
  } else {
    idx = static_cast<unsigned short *>(GxAllocIndexMem(gxBuf->IndexCount() * sizeof(*idx)));
    *cmd.index.mem[GxVM_Position] = idx;
  }
}

static void __fastcall SetupBufCmd(CGxBuf *gxBuf, CGxBufCommand &cmd, CGxVertexPCT0 *&vtx, unsigned short *&idx) {
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
    idx = static_cast<unsigned short *>(*cmd.index.mem[GxVM_Position]);
  } else {
    idx = static_cast<unsigned short *>(GxAllocIndexMem(gxBuf->IndexCount() * sizeof(*idx)));
    *cmd.index.mem[GxVM_Position] = idx;
  }
}

void CChunkLiquid::RenderRiver0V(CGxVertexPNT0 *vtx) {
  NTempest::C3Vector diffv;
  float              dsq;
  NTempest::C3Vector vertWorldPos;
  unsigned int       vrow1;
  unsigned int       tx;
  float              fx;
  unsigned int       ty;
  float              dy;
  NTempest::C2Vector farCorner;
  float              dx;
  NTempest::C3Vector dumbNormal(0.0f, 0.0f, 1.0f);
  float              temp;
  const float        OO_MAX_RIVER_COLOR_DSQ = 1.0f / 225.0f;

  fx = static_cast<float>(chunk->aIndex.x + 1) * 33.333332f;
  temp = static_cast<float>(chunk->aIndex.y + 1) * 33.333332f;
  farCorner = NTempest::C2Vector(fx, temp);
  temp = 17066.666f - farCorner.x;
  farCorner.x = 17066.666f - farCorner.y;
  farCorner.y = temp;
  dy = (farCorner.x - chunk->corner.x) / 8.0f;
  dx = (farCorner.y - chunk->corner.y) / 8.0f;

  for (ty = 0; ty < 9; ++ty) {
    vrow1 = 9 * ty;
    vertWorldPos.x = static_cast<float>(ty) * dy + chunk->corner.x;
    for (tx = 0; tx < 9; ++tx) {
      SWVert &waterVert = verts[vrow1 + tx].waterVert;
      vertWorldPos.y = static_cast<float>(tx) * dx + chunk->corner.y;
      vertWorldPos.z = waterVert.height;
      vtx->p = vertWorldPos - CWorldScene::camPos;
      vtx->n = dumbNormal;
      if (CWorldScene::camLiquid) {
        vtx->tc[0] = NTempest::C2Vector(0.5f, s_riverDepthCoordTable[waterVert.depth]);
      } else {
        diffv = vertWorldPos - CWorldScene::camPos;
        dsq = NTempest::C3Vector::Dot(diffv, diffv);
        vtx->tc[0] = NTempest::C2Vector(0.5f, dsq * OO_MAX_RIVER_COLOR_DSQ);
      }
      ++vtx;
    }
  }
}

void CChunkLiquid::RenderMagma0V(CGxVertexPCT0 *vtx) {
  NTempest::C3Vector  vertWorldPos;
  unsigned int        vrow1;
  float               fx;
  float               dy;
  const float         MAGMA_TILES = 3.0f;
  NTempest::C2Vector  farCorner;
  float               dx;
  float               scrollx;
  float               cycles;
  float               temp;
  const float         MAGMA_TEX_SCALE = MAGMA_TILES / 256.0f;
  NTempest::C2iVector v;
  NTempest::C2iVector t;
  const float         MAGMA_SCROLL_RATE = 0.025f;

  cycles = CWorld::GetCurTimeSec() * MAGMA_SCROLL_RATE;
  scrollx = cycles - static_cast<float>(static_cast<int>(cycles));
  fx = static_cast<float>(chunk->aIndex.x + 1) * 33.333332f;
  temp = static_cast<float>(chunk->aIndex.y + 1) * 33.333332f;
  farCorner = NTempest::C2Vector(fx, temp);
  temp = 17066.666f - farCorner.x;
  farCorner.x = 17066.666f - farCorner.y;
  farCorner.y = temp;
  dy = (farCorner.x - chunk->corner.x) / 8.0f;
  dx = (farCorner.y - chunk->corner.y) / 8.0f;

  v = NTempest::C2iVector(0);
  t = NTempest::C2iVector(0);
  while (static_cast<unsigned int>(v.y) < 9) {
    vertWorldPos.x = static_cast<float>(v.y) * dy + chunk->corner.x;
    v.x = 0;
    t.x = 0;
    while (static_cast<unsigned int>(v.x) < 9) {
      vrow1 = v.x + 9 * v.y;
      SMVert &magmaVert = verts[vrow1].magmaVert;
      vertWorldPos.y = static_cast<float>(v.x) * dx + chunk->corner.y;
      vertWorldPos.z = magmaVert.height;
      vtx->p = vertWorldPos - CWorldScene::camPos;
      vtx->c.Set(0xFFFFFFFFUL);
      vtx->tc[0] = NTempest::C2Vector(static_cast<float>(magmaVert.s) * MAGMA_TEX_SCALE + scrollx, static_cast<float>(magmaVert.t) * MAGMA_TEX_SCALE);
      t.x += v.x > 0;
      ++v.x;
      ++vtx;
    }
    t.y += v.y > 0;
    ++v.y;
  }
}

unsigned short CChunkLiquid::Render0I(unsigned short *idxBase, unsigned int liquidType) {
  unsigned short  i2;
  unsigned int    ty;
  unsigned short  lastRenderedVtx = 0;
  unsigned int    inStrip = 0;
  unsigned short *idx = idxBase;

  for (ty = 0; ty < 8; ++ty) {
    unsigned short i0 = static_cast<unsigned short>(9 * ty);
    unsigned short i1 = static_cast<unsigned short>(i0 + 9);
    i2 = static_cast<unsigned short>(i1 + 1);
    for (unsigned int tx = 0; tx < 8; ++tx) {
      FATALASSERT(tx < 8);
      if ((tiles.flags[tx + 8 * ty] & 0xF) == liquidType) {
        if (!inStrip) {
          *idx++ = i0;
          *idx++ = i0;
          *idx++ = i1;
          inStrip = 1;
        }
        *idx++ = static_cast<unsigned short>(i0 + 1);
        *idx++ = i2;
        lastRenderedVtx = i2;
      } else if (inStrip) {
        *idx++ = lastRenderedVtx;
        inStrip = 0;
      }
      ++i0;
      ++i1;
      ++i2;
    }
    if (inStrip) {
      *idx++ = lastRenderedVtx;
      inStrip = 0;
    }
  }
  return static_cast<unsigned short>(idx - idxBase);
}

void __fastcall CChunkLiquid::RenderOcean0Callback(CGxBufCommand &cmd, CGxBuf *gxBuf) {
  unsigned short *idx;
  CGxVertexPNT0  *vtx;
  UserArg        *arg = static_cast<UserArg *>(gxBuf->UserArg());
  SetupBufCmd(gxBuf, cmd, vtx, idx);
  arg->liquid->RenderOcean0V(vtx);
  arg->indexCount = arg->liquid->Render0I(idx, arg->liquidType);
  FATALASSERT(arg->indexCount <= gxBuf->IndexCount());
}

void __fastcall CChunkLiquid::RenderRiver0Callback(CGxBufCommand &cmd, CGxBuf *gxBuf) {
  unsigned short *idx;
  CGxVertexPNT0  *vtx;
  UserArg        *arg = static_cast<UserArg *>(gxBuf->UserArg());
  SetupBufCmd(gxBuf, cmd, vtx, idx);
  arg->liquid->RenderRiver0V(vtx);
  arg->indexCount = arg->liquid->Render0I(idx, arg->liquidType);
  FATALASSERT(arg->indexCount <= gxBuf->IndexCount());
}

void __fastcall CChunkLiquid::RenderMagma0Callback(CGxBufCommand &cmd, CGxBuf *gxBuf) {
  unsigned short *idx;
  CGxVertexPCT0  *vtx;
  UserArg        *arg = static_cast<UserArg *>(gxBuf->UserArg());
  SetupBufCmd(gxBuf, cmd, vtx, idx);
  arg->liquid->RenderMagma0V(vtx);
  arg->indexCount = arg->liquid->Render0I(idx, arg->liquidType);
  FATALASSERT(arg->indexCount <= gxBuf->IndexCount());
}

void CChunkLiquid::RenderOcean0() {
  UserArg arg(this, 1);
  CGxTex *texture = TextureGetGxTex(CMap::GetLiquidTexture(1), 0, 0);
  if (CMap::liquidTexLoaded[1]) {
    GxRsSet(GxRs_Texture1, texture);
    CGxBuf *gxBuf = GxBufGetDynamic(GxVBF_PNT0);
    gxBuf->UserArgSet(&arg);
    gxBuf->UserCallbackSet(RenderOcean0Callback);
    gxBuf->CountSet(81, 192);
    GxBufLock(gxBuf);
    FATALASSERT(arg.indexCount <= gxBuf->IndexCount());
    CGxBatch batch(GxPrim_TriangleStrip, arg.indexCount, 0, 0, gxBuf->VertexCount() - 1);
    GxBufRender(batch);
    GxBufUnlock();
  }
}

void CChunkLiquid::RenderRiver0(unsigned int type) {
  UserArg arg(this, 4);
  CGxTex *texture = TextureGetGxTex(CMap::GetLiquidTexture(4), 0, 0);
  if (CMap::liquidTexLoaded[4]) {
    GxRsSet(GxRs_Texture1, texture);
    CGxBuf *gxBuf = GxBufGetDynamic(GxVBF_PNT0);
    gxBuf->UserArgSet(&arg);
    gxBuf->UserCallbackSet(RenderRiver0Callback);
    gxBuf->CountSet(81, 192);
    GxBufLock(gxBuf);
    FATALASSERT(arg.indexCount <= gxBuf->IndexCount());
    CGxBatch batch(GxPrim_TriangleStrip, arg.indexCount, 0, 0, gxBuf->VertexCount() - 1);
    GxBufRender(batch);
    GxBufUnlock();
  }
}

void CChunkLiquid::RenderMagma0(unsigned int type) {
  UserArg arg(this, 6);
  CGxTex *texture = TextureGetGxTex(CMap::GetLiquidTexture(6), 0, 0);
  if (CMap::liquidTexLoaded[6]) {
    GxRsSet(GxRs_Texture0, texture);
    CGxBuf *gxBuf = GxBufGetDynamic(GxVBF_PCT0);
    gxBuf->UserArgSet(&arg);
    gxBuf->UserCallbackSet(RenderMagma0Callback);
    gxBuf->CountSet(81, 192);
    GxBufLock(gxBuf);
    FATALASSERT(arg.indexCount <= gxBuf->IndexCount());
    CGxBatch batch(GxPrim_TriangleStrip, arg.indexCount, 0, 0, gxBuf->VertexCount() - 1);
    GxBufRender(batch);
    GxBufUnlock();
  }
}

void CChunkLiquid::Render(unsigned int type) {
  switch (type) {
    case 0: {
      if (!CMap::riverDiffTexUpdated) {
        NTempest::CiRect texRect(0, 0, 64, 8);
        GxTexUpdate(CMap::riverDiffTexid, texRect, 0);
        CMap::riverDiffTexUpdated = true;
      }
      RenderRiver0(type);
      break;
    }
    case 1: {
      if (!CMap::oceanDiffTexUpdated) {
        NTempest::CiRect texRect(0, 0, 64, 8);
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
  aaBox.b.z = height.min;
  aaBox.t.z = height.max;
}

void Particulate::InitMovement() {
  const float pi = 3.1415927f;
  float       rotY = NTempest::CRandom::reals_(g_rndSeed) * pi;
  float       rotZ = NTempest::CRandom::reals_(g_rndSeed) * pi;
  float       sinRotY = NTempest::CMath::sin_(rotY);

  movement.dir.y = NTempest::CMath::sin_(rotZ) * sinRotY;
  movement.dir.x = NTempest::CMath::cos_(rotZ) * sinRotY;
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

Particulate::Particulate(float particleScale, float boxSize, const char *particulateTexture) : show(0) {
  SetPercentage(1.0f);
  SetScale(particleScale);
  SetSize(boxSize);

  texture = 0;
  SetTexture(particulateTexture);
  InitParticles(1);
  InitMovement();
}

void Particulate::SetPercentage(float percent) {
  ASSERT(percent >= 0.0f && percent <= 1.0f);
  numParticles = static_cast<unsigned int>(percent * 4000.0f);
}

Particulate::~Particulate() {
  if (texture) {
    HandleClose(texture);
  }
}

void Particulate::SetScale(float s) {
  scale = s;
}

void Particulate::SetSize(float units) {
  boxSize = units;
}

void Particulate::SetTexture(const char *name) {
  if (texture) {
    HandleClose(texture);
  }

  CStatus     status;
  CGxTexFlags flags(GxTex_LinearMipNearest, 0, 0, 0, 0, 0, 1);

  texture = TextureCreate(name, flags, &status, 0);
  SysMsgAdd(status, 2);
}

void Particulate::InitParticles(unsigned int l) {
  float scaleMin = scale * 0.5f;
  float scaleDiff = scale * 1.5f - scaleMin;
  float halfBoxSize = boxSize * 0.5f;

  for (unsigned int lp = 0; lp < numParticles; ++lp) {
    particles[lp].pos = NTempest::C3Vector(
        NTempest::CRandom::real_(g_rndSeed) * boxSize - halfBoxSize, NTempest::CRandom::real_(g_rndSeed) * boxSize - halfBoxSize,
        NTempest::CRandom::real_(g_rndSeed) * boxSize - halfBoxSize
    );
    particles[lp].scale = NTempest::CRandom::real_(g_rndSeed) * scaleDiff + scaleMin;
  }

  liquid = l & 3;
}

int WaterRadWave::Update(float deltat) {
  curTime += deltat;
  if (curTime > timeLength) {
    return 0;
  }

  rb = curTime * velocity;
  ra = rb - length;
  decay = 1.0f - curTime * ooTimeLength;
  return 1;
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

  for (unsigned int lp = 0; lp < numParticles; ++lp) {
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

void Particulate::Render() {
  if (!show) {
    return;
  }

  CGxVertexPCT0  *vtxBase = static_cast<CGxVertexPCT0 *>(GxAllocVertexMem(2664 * sizeof(*vtxBase)));
  unsigned short *idxBase = static_cast<unsigned short *>(GxAllocIndexMem(3996 * sizeof(*idxBase)));

  NTempest::C44Matrix view;
  GxXformView(view);
  GxXformSetView(NTempest::C44Matrix());

  unsigned int    nVerts = 0;
  unsigned short *idx = idxBase;
  unsigned int    tcSub = 8;
  for (unsigned int lp = 0; lp < numParticles; ++lp) {
    const Particle    &particle = particles[lp];
    NTempest::C3Vector vp(
        view.a0 * particle.pos.x + view.b0 * particle.pos.y + view.c0 * particle.pos.z,
        view.a1 * particle.pos.x + view.b1 * particle.pos.y + view.c1 * particle.pos.z,
        view.a2 * particle.pos.x + view.b2 * particle.pos.y + view.c2 * particle.pos.z
    );

    if (vp.z > 0.0f && vp.x < vp.z && vp.x > -vp.z && vp.y < vp.z && vp.y > -vp.z) {
      CGxVertexPCT0 *vtx = vtxBase + nVerts;
      for (unsigned int i = 0; i < 4; ++i) {
        vtx[i].p.x = vp.x + s_vcv[i].x * particle.scale;
        vtx[i].p.y = vp.y + s_vcv[i].y * particle.scale;
        vtx[i].p.z = vp.z;
        vtx[i].c = static_cast<unsigned long>(-1);
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

void __fastcall Particulate::CustomRenderCallback(void *p1, int p2) {
  static_cast<Particulate *>(p1)->Render();
}
