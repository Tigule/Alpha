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

#include <float.h>

#include "Os/W32/Debugging.h"
#include "Services/AsyncFileRead.h"

static const UINT ALPHA_BIT_MASK[2] = {0x0F, 0xF0};
static const UINT ALPHA_BIT_LSHIFT[2] = {4, 0};
static const UINT SHADOW_BIT_MASK[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
static const UINT SHADOW_BIT_RSHIFT[8] = {0, 1, 2, 3, 4, 5, 6, 7};
static const BYTE SHADOW_VALUES[2] = {0xFF, 0x00};

enum {
  SPRIM_TYPE_STRIP = 0,
  SPRIM_TYPE_TRI = 1
};

struct STPrimRemap {
  WORD  nIndicies;
  WORD *indicies;
};

struct STPrimGroup {
  WORD  nIndicies;
  WORD  primType;
  WORD *indicies;
};

static WORD s_vertexRemap0[10] = {0, 3, 8, 1, 72, 2, 136, 4, 144, 0};
static WORD s_primGroup0_0[5] = {0, 1, 2, 3, 4};
static WORD s_primGroup0_1[3] = {2, 4, 0};

static WORD s_vertexRemap1[26] = {0, 0, 4, 4, 8, 6, 36, 2, 40, 5, 68, 1, 72, 3, 76, 7, 104, 10, 108, 8, 136, 9, 140, 11, 144, 12};
static WORD s_primGroup1_0[25] = {0, 1, 2, 3, 4, 5, 6, 7, 7, 5, 5, 3, 7, 8, 8, 9, 9, 9, 1, 10, 3, 11, 8, 12, 7};
static WORD s_primGroup1_1[6] = {2, 4, 0, 10, 9, 11};

static WORD s_vertexRemap2[82] = {0,  34,  2,  35,  4,  37,  6,  38,  8,  40,  18, 26,  20,  36,  22,  18,  24,  39,  34,  27, 36,
                                  25, 38,  17, 40,  0,  42,  1,  52,  28, 54,  24, 56,  16,  58,  2,   68,  29,  70,  23,  72, 15,
                                  74, 4,   76, 3,   86, 30,  88, 22,  90, 14,  92, 5,   102, 31,  104, 21,  106, 13,  108, 6,  110,
                                  7,  120, 32, 122, 20, 124, 12, 126, 8,  136, 33, 138, 19,  140, 11,  142, 10,  144, 9};
static WORD s_primGroup2_0[69] = {0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 10, 10, 10, 10, 11, 12, 13, 6,  14, 4,  15,
                                  16, 17, 0,  18, 18, 19, 19, 19, 11, 20, 13, 21, 22, 23, 15, 24, 17, 25, 25, 26, 26, 25, 27,
                                  28, 29, 23, 30, 21, 31, 32, 33, 19, 19, 27, 27, 34, 26, 35, 25, 36, 17, 37, 18, 38, 0,  39};
static WORD s_primGroup2_1[48] = {39, 0,  1,  39, 1, 40, 39, 40, 38, 16, 4,  0,  2,  0,  4,  28, 23, 25, 24, 25, 23, 14, 15, 13,
                                  22, 13, 15, 8,  6, 10, 12, 10, 6,  20, 21, 19, 32, 19, 21, 36, 37, 35, 5,  7,  3,  30, 29, 31};

static WORD s_vertexRemap3[290] = {
    0,   75,  1,   77,  2,   78,  3,   80,  4,   81,  5,   84,  6,   86,  7,   2,   8,   4,   9,   76,  10,  72,  11,  79,  12,  73,  13,  82,  14,
    85,  15,  0,   16,  3,   17,  64,  18,  66,  19,  68,  20,  70,  21,  74,  22,  83,  23,  87,  24,  1,   25,  5,   26,  65,  27,  67,  28,  69,
    29,  71,  30,  90,  31,  88,  32,  7,   33,  6,   34,  57,  35,  58,  36,  60,  37,  62,  38,  89,  39,  91,  40,  92,  41,  8,   42,  9,   43,
    56,  44,  59,  45,  61,  46,  63,  47,  34,  48,  93,  49,  10,  50,  12,  51,  55,  52,  49,  53,  46,  54,  40,  55,  33,  56,  21,  57,  19,
    58,  11,  59,  13,  60,  54,  61,  45,  62,  44,  63,  39,  64,  32,  65,  20,  66,  18,  67,  125, 68,  53,  69,  48,  70,  43,  71,  38,  72,
    31,  73,  22,  74,  16,  75,  17,  76,  126, 77,  50,  78,  47,  79,  42,  80,  36,  81,  30,  82,  23,  83,  15,  84,  127, 85,  52,  86,  51,
    87,  41,  88,  37,  89,  35,  90,  24,  91,  14,  92,  128, 93,  129, 94,  124, 95,  113, 96,  112, 97,  106, 98,  25,  99,  27,  100, 29,  101,
    130, 102, 123, 103, 118, 104, 111, 105, 105, 106, 100, 107, 26,  108, 28,  109, 132, 110, 131, 111, 119, 112, 117, 113, 110, 114, 101, 115, 99,
    116, 144, 117, 142, 118, 133, 119, 122, 120, 116, 121, 109, 122, 104, 123, 97,  124, 98,  125, 141, 126, 134, 127, 135, 128, 121, 129, 115, 130,
    107, 131, 103, 132, 96,  133, 143, 134, 140, 135, 136, 136, 120, 137, 114, 138, 108, 139, 102, 140, 95,  141, 94,  142, 139, 143, 138, 144, 137
};
static WORD s_primGroup3_0[392] = {
    0,   1,   2,   3,   4,   5,   5,   6,   6,   5,   1,   3,   3,   7,   7,   7,   1,   8,   6,   9,   5,   5,   10,  10,  8,   11,  12,  13,
    9,   9,   14,  14,  14,  15,  16,  17,  18,  11,  19,  10,  10,  20,  20,  16,  19,  18,  18,  19,  19,  21,  20,  22,  16,  23,  14,  24,
    24,  25,  25,  26,  24,  27,  14,  28,  29,  29,  24,  24,  30,  22,  31,  32,  33,  21,  34,  34,  30,  30,  30,  31,  35,  36,  36,  37,
    37,  36,  38,  31,  39,  33,  40,  40,  41,  41,  41,  37,  42,  38,  43,  44,  44,  45,  45,  43,  46,  44,  40,  38,  39,  39,  41,  41,
    47,  43,  48,  45,  49,  46,  46,  50,  50,  51,  48,  47,  47,  52,  52,  50,  53,  48,  54,  49,  55,  56,  56,  55,  55,  55,  57,  56,
    58,  49,  59,  46,  60,  61,  62,  40,  63,  63,  57,  57,  64,  65,  66,  58,  67,  60,  68,  69,  70,  62,  71,  71,  72,  72,  72,  66,
    68,  67,  67,  73,  73,  70,  74,  71,  71,  75,  75,  64,  76,  66,  77,  72,  78,  68,  79,  70,  80,  73,  73,  80,  80,  73,  81,  74,
    82,  83,  84,  85,  86,  87,  0,   1,   1,   85,  85,  83,  87,  88,  88,  62,  62,  62,  71,  89,  74,  90,  83,  91,  88,  92,  87,  7,
    1,   1,   40,  40,  63,  33,  89,  34,  91,  21,  93,  19,  92,  10,  8,   8,   94,  94,  95,  96,  97,  98,  99,  26,  100, 25,  35,  24,
    30,  30,  101, 101, 101, 97,  100, 99,  99,  102, 102, 95,  103, 97,  104, 101, 105, 100, 106, 35,  37,  36,  36,  103, 103, 104, 102, 107,
    107, 102, 102, 102, 108, 107, 109, 104, 110, 105, 111, 112, 112, 113, 113, 111, 41,  112, 37,  105, 106, 106, 108, 108, 114, 115, 116, 109,
    117, 111, 118, 113, 51,  41,  47,  47,  119, 119, 119, 116, 118, 117, 117, 120, 120, 114, 121, 116, 122, 119, 123, 118, 124, 51,  52,  50,
    50,  11,  11,  11,  13,  125, 126, 17,  127, 128, 129, 130, 131, 132, 133, 133, 132, 132, 133, 134, 135, 136, 137, 138, 138, 139, 139, 138,
    140, 134, 141, 142, 28,  132, 29,  128, 14,  15,  15,  94,  94,  139, 143, 141, 98,  144, 26,  28,  27,  27,  143, 143, 143, 98,  94,  96
};
static WORD s_primGroup3_1[90] = {136, 134, 138, 142, 134, 132, 130, 128, 132, 15,  128, 17,  125, 11, 17, 65,  57,  58,  59, 60, 58,  69,  60,
                                  62,  63,  89,  62,  90,  89,  91,  93,  92,  91,  7,   92,  8,   12, 9,  8,   32,  22,  21, 23, 22,  24,  110,
                                  111, 109, 115, 108, 109, 79,  80,  78,  61,  46,  40,  54,  55,  53, 82, 84,  81,  0,   2,  86, 127, 129, 126,
                                  133, 135, 131, 140, 141, 139, 144, 141, 28,  124, 52,  123, 42,  43, 41, 121, 122, 120, 76, 77, 75};

static STPrimRemap s_tPrimRemap[4] = {
    {  5, s_vertexRemap0},
    { 13, s_vertexRemap1},
    { 41, s_vertexRemap2},
    {145, s_vertexRemap3}
};

static STPrimGroup s_tPrimGroups[4][2] = {
    {  {5, 0, s_primGroup0_0},  {3, 1, s_primGroup0_1}},
    { {25, 0, s_primGroup1_0},  {6, 1, s_primGroup1_1}},
    { {69, 0, s_primGroup2_0}, {48, 1, s_primGroup2_1}},
    {{392, 0, s_primGroup3_0}, {90, 1, s_primGroup3_1}}
};

static UINT g_gxBufCreateCount;
static UINT g_gxBufDestroyCount;

UINT CMapChunk::cornerVertexIndex[4] = {0, 8, 136, 144};
UINT CMapChunk::farCornerIndex;

static int iIndiciesP[4][2] = {
    {17,  0},
    { 0,  1},
    {18, 17},
    { 1, 18}
};
BYTE                      CMapChunk::syncLoadBuffer[15000];
NTempest::C2Vector        CMapChunk::texCoordList[145];
NTempest::C2Vector        CMapChunk::texCoordList2[145];
NTempest::C2Vector        CMapChunk::rmTexCoordList[4][145];
NTempest::C2Vector        CMapChunk::rmTexCoordList2[4][145];
CGxBatch                  CMapChunk::rmGxBatchList[4][2];
WORD                      CMapChunk::primList[768];
WORD                     *CMapChunk::primPtr;
TSGrowableArray<CGxBuf *> CMapChunk::gxBufFreeList;
CGxBuf                   *CMapChunk::gxBufDyn;
TSGrowableArray<CGxTex *> CMapChunk::gxAlphaTexFreeList;
TSGrowableArray<CGxTex *> CMapChunk::gxShadowTexFreeList;
void (*CMapChunk::soundEmitterCreateHandler)(CWSoundEmitter &);
void (*CMapChunk::soundEmitterDestroyHandler)(DWORD);

static TSCArray<BYTE, 15000> s_syncLoadBuffer;
static SCritSect             s_fileCritSect;
static TSCArray<BYTE, 15000> s_asyncLoadBuffers[16];
static BYTE                 *s_freeAsyncBuffer;
static BYTE                  s_asyncBuffersInitialized;
static LISTDECLEX(CAsyncObject, link, s_asyncLoadList);

static void ValidateAsyncReadBuffer(BYTE *buffer) {
  for (UINT index = 0; index < 16; ++index) {
    if (buffer == s_asyncLoadBuffers[index].Ptr()) {
      return;
    }
  }

  FATALERROR(("%08x is not a valid map chunk async read buffer", buffer));
}

void CMapChunk::FreeAsyncLoadBuffer(BYTE *buffer) {
  ValidateAsyncReadBuffer(buffer);
  *(BYTE **)buffer = s_freeAsyncBuffer;
  s_freeAsyncBuffer = buffer;
}

void CMapChunk::InitAsyncLoadBuffers() {
  for (UINT index = 0; index < 16; ++index) {
    FreeAsyncLoadBuffer(s_asyncLoadBuffers[index].Ptr());
  }
}

BYTE *CMapChunk::AllocAsyncLoadBuffer() {
  if (!s_asyncBuffersInitialized) {
    InitAsyncLoadBuffers();
    s_asyncBuffersInitialized = 1;
  }

  if (!s_freeAsyncBuffer) {
    return 0;
  }

  BYTE *buffer = s_freeAsyncBuffer;
  ValidateAsyncReadBuffer(buffer);
  s_freeAsyncBuffer = *(BYTE **)buffer;
  if (s_freeAsyncBuffer) {
    ValidateAsyncReadBuffer(s_freeAsyncBuffer);
  }
  return buffer;
}

CChunkTex::CChunkTex() {
}

CChunkTex::~CChunkTex() {
}

CChunkLayer::CChunkLayer() {
  props = 0;
  texId = 0;
  offsAlpha = 0;
  tex = 0;
  gxTexture = 0;
  chunk = 0;
  effectId = 0;
}

CChunkLayer::~CChunkLayer() {
  ASSERT(gxTexture==0);
  ASSERT(tex == 0);
}

void CMapChunk::Initialize() {
  CreateRenderLists();
  gxBufFreeList.SetChunkSize(0x100);
  gxAlphaTexFreeList.SetChunkSize(0x100);
  gxShadowTexFreeList.SetChunkSize(0x100);

  gxBufDyn = GxBufCreate(GxBWF_Dynamic, GxVBF_PN, 145, 768, GxBufDynFillCallback, 0);
  ASSERT(gxBufDyn);

  soundEmitterCreateHandler = 0;
  soundEmitterDestroyHandler = 0;
  AsyncFileReadAddHandler(AsyncPollHandler);
}

void CMapChunk::Destroy() {
  FreeLists();
  GxBufDestroy(gxBufDyn);
}

void CMapChunk::FreeLists() {
  UINT index;

  for (index = 0; index < gxBufFreeList.Count(); ++index) {
    GxBufDestroy(gxBufFreeList[index]);
    ++g_gxBufDestroyCount;
  }
  gxBufFreeList.SetCount(0);

  ASSERT(g_gxBufCreateCount==g_gxBufDestroyCount);

  for (index = 0; index < gxAlphaTexFreeList.Count(); ++index) {
    GxTexDestroy(gxAlphaTexFreeList[index]);
  }
  gxAlphaTexFreeList.SetCount(0);

  for (index = 0; index < gxShadowTexFreeList.Count(); ++index) {
    GxTexDestroy(gxShadowTexFreeList[index]);
  }
  gxShadowTexFreeList.SetCount(0);
}

void CMapChunk::AsyncPollHandler() {
  CAsyncObject *object = s_asyncLoadList.Head();

  while (object) {
    BYTE *buffer = AllocAsyncLoadBuffer();
    if (!buffer) {
      break;
    }

    CAsyncObject *next = object->link.Next();
    s_asyncLoadList.UnlinkNode(object);
    object->buffer = buffer;
    object->canReorder = 1;
    AsyncFileReadObject(object);
    object = next;
  }
}

CGxBuf *CMapChunk::AllocGxBuf(UINT indexCount) {
  CGxBuf *gxBuf;

  if (!gxBufFreeList.Count()) {
    gxBuf = GxBufCreate(GxBWF_Low, GxVBF_PN, 145, indexCount, GxBufFillCallback, 0);
    ++g_gxBufCreateCount;
    return gxBuf;
  }

  gxBuf = gxBufFreeList[gxBufFreeList.Count() - 1];
  FATALASSERT(gxBuf);
  gxBufFreeList.SetCount(gxBufFreeList.Count() - 1);
  gxBuf->CountSet(145, indexCount);
  gxBuf->Invalidate(CGxBuf::S_INVALID_DISCARD, CGxBuf::S_INVALID_DISCARD);
  return gxBuf;
}

void CMapChunk::FreeGxBuf(CGxBuf *gxBuf) {
  FATALASSERT(gxBuf);

  gxBufFreeList.Add(&gxBuf);
}

CGxTex *CMapChunk::AllocAlphaGxTex(LPVOID userArg, void (*userFunc)(EGxTexCommand, UINT, UINT, UINT, UINT, LPVOID, UINT &, LPCVOID &)) {
  CGxTex *gxTex;

  if (!gxAlphaTexFreeList.Count()) {
    UINT sizeX;
    UINT sizeY;
    if (CWorld::alphaMipLevel == 1) {
      sizeX = 32;
      sizeY = 32;
    } else {
      sizeX = 64;
      sizeY = 64;
    }

    GxTexCreate(GxTex_2d, sizeX, sizeY, 0, GxTex_Argb4444, GxTex_Argb8888, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), userArg, userFunc, gxTex);
    FATALASSERT(gxTex);
  } else {
    gxTex = gxAlphaTexFreeList[gxAlphaTexFreeList.Count() - 1];
    FATALASSERT(gxTex);
    gxAlphaTexFreeList.SetCount(gxAlphaTexFreeList.Count() - 1);
    GxTexSetUserData(gxTex, userFunc, userArg);
  }

  return gxTex;
}

void CMapChunk::FreeAlphaGxTex(CGxTex *gxTex) {
  FATALASSERT(gxTex);

  GxTexSetUserData(gxTex, UpdateTextureDefault, 0);
  gxAlphaTexFreeList.Add(&gxTex);
}

CGxTex *CMapChunk::AllocShadowGxTex(LPVOID userArg, void (*userFunc)(EGxTexCommand, UINT, UINT, UINT, UINT, LPVOID, UINT &, LPCVOID &)) {
  CGxTex *gxTex;

  if (!gxShadowTexFreeList.Count()) {
    UINT sizeX;
    UINT sizeY;
    if (CWorld::shadowMipLevel == 1) {
      sizeX = 32;
      sizeY = 32;
    } else {
      sizeX = 64;
      sizeY = 64;
    }

    GxTexCreate(GxTex_2d, sizeX, sizeY, 0, GxTex_Argb4444, GxTex_Argb8888, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), userArg, userFunc, gxTex);
    FATALASSERT(gxTex);
  } else {
    gxTex = gxShadowTexFreeList[gxShadowTexFreeList.Count() - 1];
    FATALASSERT(gxTex);
    gxShadowTexFreeList.SetCount(gxShadowTexFreeList.Count() - 1);
    GxTexSetUserData(gxTex, userFunc, userArg);
  }

  return gxTex;
}

void CMapChunk::FreeShadowGxTex(CGxTex *gxTex) {
  FATALASSERT(gxTex);

  gxShadowTexFreeList.Add(&gxTex);
  GxTexSetUserData(gxTex, UpdateTextureDefault, 0);
}

void CMapChunk::CreateRenderLists() {
  float               texCoordY = 0.0f;
  float               texCoordHalfY = 0.0625f;
  NTempest::C2Vector *texCoord = texCoordList;

  int row;
  int column;
  for (row = 0; row < 9; ++row) {
    float texCoordX = 0.0f;
    for (column = 0; column < 9; ++column) {
      texCoord->x = texCoordX * 8.0f;
      texCoord->y = texCoordY * 8.0f;
      texCoordX += 0.125f;
      ++texCoord;
    }

    texCoordY += 0.125f;
    if (row < 8) {
      float texCoordHalfX = 0.0625f;
      for (column = 0; column < 8; ++column) {
        texCoord->x = texCoordHalfX * 8.0f;
        texCoord->y = texCoordHalfY * 8.0f;
        texCoordHalfX += 0.125f;
        ++texCoord;
      }
      texCoordHalfY += 0.125f;
    }
  }

  texCoord = texCoordList2;
  texCoordY = 0.0f;
  texCoordHalfY = 0.0625f;
  for (row = 0; row < 9; ++row) {
    float texCoordX = 0.0f;
    for (column = 0; column < 9; ++column) {
      texCoord->x = texCoordX;
      texCoord->y = texCoordY;
      texCoordX += 0.1220703125f;
      ++texCoord;
    }

    texCoordY += 0.1220703125f;
    if (row < 8) {
      float texCoordHalfX = 0.0625f;
      for (column = 0; column < 8; ++column) {
        texCoord->x = texCoordHalfX;
        texCoord->y = texCoordHalfY;
        texCoordHalfX += 0.1220703125f;
        ++texCoord;
      }
      texCoordHalfY += 0.1220703125f;
    }
  }

  UINT lod;
  for (lod = 0; lod < 4; ++lod) {
    WORD *indicies = s_tPrimRemap[lod].indicies;
    for (UINT remapIndex = 0; remapIndex < s_tPrimRemap[lod].nIndicies; ++remapIndex) {
      WORD sourceIndex = *indicies++;
      WORD destIndex = *indicies++;
      rmTexCoordList[lod][destIndex] = texCoordList[sourceIndex];
      rmTexCoordList2[lod][destIndex] = texCoordList2[sourceIndex];
    }
  }

  for (lod = 0; lod < 4; ++lod) {
    UINT stripCount = s_tPrimGroups[lod][0].nIndicies;
    rmGxBatchList[lod][0].m_primType = GxPrim_TriangleStrip;
    rmGxBatchList[lod][0].m_count = stripCount;
    rmGxBatchList[lod][0].m_start = 0;
    rmGxBatchList[lod][1].m_primType = GxPrim_Triangles;
    rmGxBatchList[lod][1].m_count = s_tPrimGroups[lod][1].nIndicies;
    rmGxBatchList[lod][1].m_start = stripCount;
  }

  for (lod = 0; lod < 4; ++lod) {
    for (WORD index = 0; index < s_tPrimRemap[lod].nIndicies - 1; ++index) {
      WORD *indicies = s_tPrimRemap[lod].indicies;
      int   candidate = index + 1;
      WORD *entry = &indicies[index * 2];
      WORD *lowest = entry;
      for (; candidate < s_tPrimRemap[lod].nIndicies; ++candidate) {
        if (indicies[candidate * 2 + 1] < lowest[1]) {
          lowest = &indicies[candidate * 2];
        }
      }

      WORD sourceIndex = entry[0];
      WORD destIndex = entry[1];
      entry[0] = lowest[0];
      entry[1] = lowest[1];
      lowest[0] = sourceIndex;
      lowest[1] = destIndex;
    }
  }
}

void CMapChunk::AsyncCallback(LPVOID userArg) {
  CMapChunk *chunk = (CMapChunk *)userArg;
  FATALASSERT(chunk);

  chunk->Create((BYTE *)chunk->asyncObject->buffer);
  FreeAsyncLoadBuffer((BYTE *)chunk->asyncObject->buffer);
  chunk->asyncObject->buffer = 0;
  AsyncFileReadDestroyObject(chunk->asyncObject);
  chunk->asyncObject = 0;
}

CMapChunk::CMapChunk() {
  shadowTexture = 0;
  shadowGxTexture = 0;
  shaderTexture = 0;
  shaderGxTexture = 0;
  detailDoodadInst = 0;
  memset(liquids, 0, sizeof(liquids));
  type |= Type_Chunk;
  nLayers = 0;
  gxBuf = 0;
  asyncObject = 0;
}

CMapChunk::~CMapChunk() {
  ASSERT(detailDoodadInst==0);
  ASSERT(asyncObject==0);
  ASSERT(refCount==0);
  ASSERT(gxBuf==0);
  ASSERT(shaderGxTexture==0);
  ASSERT(shadowGxTexture==0);

  for (UINT i = 0; i < 4; ++i) {
    ASSERT(liquids[i]==0);
  }
}

void CMapChunk::Load(SMChunkInfo *chunkInfo) {
  FATALASSERT(chunkInfo);
  FATALASSERT(chunkInfo->size < 15000);

  nLayers = 0;
  asyncObject = 0;
  bLoaded = 0;
  lod = -1;
  remapLod = -1;
  detailDoodadInst = 0;
  gxBuf = 0;
  freeTime = 0.0f;
  flags = 0;
  fileOffset = chunkInfo->offset;
  fileSize = chunkInfo->size;

  if (CMap::bPreload) {
    OsOutputDebugString("CMapChunk::Load() preload\n");
    s_fileCritSect.Enter();
    SFile::SetFilePointer(CMap::wdtFile, fileOffset, 0, FILE_BEGIN);
    SFile::Read(CMap::wdtFile, s_syncLoadBuffer.Ptr(), fileSize, 0, 0, 0);
    Create(s_syncLoadBuffer.Ptr());
    s_fileCritSect.Leave();
  } else {
    asyncObject = AsyncFileReadCreateObject();
    FATALASSERT(asyncObject);
    asyncObject->file = CMap::wdtFile;
    asyncObject->buffer = AllocAsyncLoadBuffer();
    asyncObject->offset = chunkInfo->offset;
    asyncObject->size = chunkInfo->size;
    asyncObject->userArg = this;
    asyncObject->userPostloadCallback = AsyncCallback;
    asyncObject->critSect = &s_fileCritSect;

    if (asyncObject->buffer) {
      AsyncFileReadObject(asyncObject);
    } else {
      asyncObject->canReorder = 0;
      s_asyncLoadList.LinkNode(asyncObject, LIST_TAIL, 0);
    }
    chunkInfo->asyncId = (UINT)asyncObject;
  }
}

void CMapChunk::SyncLoadLayer(CChunkLayer *layer) {
  SMChunk *mChunk = 0;
  SMLayer *mLayer = 0;
  BYTE    *shadowTex = 0;
  BYTE    *alphaTex = 0;

  s_fileCritSect.Enter();
  SyncLoad(mChunk, mLayer, shadowTex, alphaTex);

  UINT i;
  for (i = 0; i < nLayers; i++, mLayer++) {
    if (layerList[i] == layer) {
      break;
    }
  }
  FATALASSERT(i < nLayers);

  layer->offsAlpha = alphaTex + mLayer->offsAlpha;
  CreateChunkLayerTex(layer);
  s_fileCritSect.Leave();
}

void CMapChunk::SyncLoadShadow() {
  SMChunk *mChunk = 0;
  SMLayer *mLayer = 0;
  BYTE    *shadowTex = 0;
  BYTE    *alphaTex = 0;

  s_fileCritSect.Enter();
  SyncLoad(mChunk, mLayer, shadowTex, alphaTex);
  shadowOffs = shadowTex;
  shadowSize = mChunk->sizeShadow;
  CreateChunkShadowTex();
  s_fileCritSect.Leave();
}

void CMapChunk::SyncLoadShader() {
  SMChunk *mChunk = 0;
  SMLayer *mLayer = 0;
  BYTE    *shadowTex = 0;
  BYTE    *alphaTex = 0;

  s_fileCritSect.Enter();
  SyncLoad(mChunk, mLayer, shadowTex, alphaTex);

  for (UINT i = 0; i < mChunk->nLayers; i++, mLayer++) {
    layerList[i]->offsAlpha = alphaTex + mLayer->offsAlpha;
  }
  shadowOffs = shadowTex;
  shadowSize = mChunk->sizeShadow;
  CreateChunkShaderTex();
  s_fileCritSect.Leave();
}

void CMapChunk::SyncLoad(SMChunk *&mChunk, SMLayer *&mLayer, BYTE *&shadowTex, BYTE *&alphaTex) {
  FATALASSERT(CMap::wdtFile);

  SFile::SetFilePointer(CMap::wdtFile, fileOffset, 0, FILE_BEGIN);
  SFile::Read(CMap::wdtFile, s_syncLoadBuffer.Ptr(), fileSize, 0, 0, 0);

  SIffChunk *iffChunk = (SIffChunk *)s_syncLoadBuffer.Ptr();
  FATALASSERT(iffChunk->token=='MCNK');
  mChunk = (SMChunk *)(iffChunk + 1);

  float    *mHeights = (float *)(mChunk + 1);
  SMNormal *mNormals = (SMNormal *)(mHeights + 145);
  iffChunk = (SIffChunk *)(mNormals + 1);
  FATALASSERT(iffChunk->token=='MCLY');
  mLayer = (SMLayer *)(iffChunk + 1);

  iffChunk = (SIffChunk *)((BYTE *)mLayer + iffChunk->size);
  shadowTex = (BYTE *)(iffChunk + 1) + iffChunk->size;
  alphaTex = shadowTex + mChunk->sizeShadow;
}

void CMapChunk::Create(BYTE *data) {
  FATALASSERT(data);
  FATALASSERT(CMap::bActive);
  FATALASSERT(bLoaded == 0);

  SIffChunk *iffChunk = (SIffChunk *)data;
  FATALASSERT(iffChunk->token=='MCNK');
  data += sizeof(SIffChunk);
  SMChunk *mChunk = (SMChunk *)data;
  data += sizeof(SMChunk);
  float *mHeights = (float *)data;
  data += sizeof(float) * 145;
  SMNormal *mNormals = (SMNormal *)data;
  data += sizeof(SMNormal);

  iffChunk = (SIffChunk *)data;
  FATALASSERT(iffChunk->token=='MCLY');
  data += sizeof(SIffChunk);
  SMLayer *mLayer = (SMLayer *)data;
  data += iffChunk->size;

  iffChunk = (SIffChunk *)data;
  FATALASSERT(iffChunk->token=='MCRF');
  data += sizeof(SIffChunk);
  UINT *mRef = (UINT *)data;
  data += iffChunk->size;
  BYTE *shadowTex = data;
  data += mChunk->sizeShadow;
  BYTE *alphaTex = data;
  data += mChunk->sizeAlpha;

  UINT  i;
  DWORD mask = 4;
  for (i = 0; i < 4; i++, mask <<= 1) {
    if (mChunk->flags & mask) {
      if (!liquids[i]) {
        liquids[i] = CMap::AllocChunkLiquid();
      }
      CChunkLiquid *liquid = liquids[i];
      liquid->height = *((NTempest::CRange *&)data)++;
      memcpy(liquid->verts, data, sizeof(liquid->verts));
      data += sizeof(liquid->verts);
      memcpy(&liquid->tiles, data, sizeof(liquid->tiles));
      data += sizeof(liquid->tiles);
      liquid->nFlowvs = *((UINT *&)data)++;
      memcpy(liquid->flowvs, data, sizeof(liquid->flowvs));
      data += sizeof(liquid->flowvs);
      liquid->chunk = this;
    } else if (liquids[i]) {
      CMap::FreeChunkLiquid(liquids[i]);
    }
  }

  if (soundEmitterCreateHandler) {
    for (i = 0; i < mChunk->nSndEmitters; i++) {
      CMapSoundEmitter *emitter = CMap::AllocSoundEmitter();
      emitter->data.soundPointID = *(DWORD *)(data + i * 52);
      emitter->data.soundNameID = *(DWORD *)(data + i * 52 + 4);
      emitter->data.pos.x = *(float *)(data + i * 52 + 8);
      emitter->data.pos.y = *(float *)(data + i * 52 + 12);
      emitter->data.pos.z = *(float *)(data + i * 52 + 16);
      emitter->data.minDistance = *(float *)(data + i * 52 + 20);
      emitter->data.maxDistance = *(float *)(data + i * 52 + 24);
      emitter->data.cutoffDistance = *(float *)(data + i * 52 + 28);
      emitter->data.startTime = *(WORD *)(data + i * 52 + 32);
      emitter->data.endTime = *(WORD *)(data + i * 52 + 34);
      emitter->data.mode = *(WORD *)(data + i * 52 + 36);
      emitter->data.groupSilenceMin = *(WORD *)(data + i * 52 + 40);
      emitter->data.groupSilenceMax = *(WORD *)(data + i * 52 + 42);
      emitter->data.playInstancesMin = *(WORD *)(data + i * 52 + 44);
      emitter->data.playInstancesMax = *(WORD *)(data + i * 52 + 46);
      emitter->data.loopCountMin = *(data + i * 52 + 38);
      emitter->data.loopCountMax = *(data + i * 52 + 39);
      emitter->data.interSoundGapMin = *(WORD *)(data + i * 52 + 48);
      emitter->data.interSoundGapMax = *(WORD *)(data + i * 52 + 50);
      soundEmitterList.LinkNode(emitter, LIST_TAIL, 0);
      soundEmitterCreateHandler(emitter->data);
    }
  }

  CMap::CreateChunkNeighborPtrs(this);
  corner.x = (float)cOffset.x * 33.333332f;
  corner.y = (float)cOffset.y * 33.333332f;
  corner.z = *mHeights;
  float temp = (-corner.x) + 17066.666f;
  corner.x = (-corner.y) + 17066.666f;
  corner.y = temp;
  zoneId = mChunk->areaid;
  holes = mChunk->holes;
  flags = 0;
  if (mChunk->flags & 2) {
    flags = CMapBaseObj::Flag_Impassable;
  }
  rSeed.SetSeed(cOffset.x | (cOffset.y << 16));
  memcpy(predTex, mChunk->predTex, sizeof(predTex));
  memcpy(noEffectDoodad, mChunk->noEffectDoodad, sizeof(mChunk->noEffectDoodad));
  memset(shadowBits, 0, sizeof(shadowBits));

  CreateVertices(mHeights);
  CreateNormals((signed char *)mNormals);
  CreateFacePlanes();

  CMapBaseObjLink *areaLink = parentLinkList.Head();
  CMapArea        *area = (CMapArea *)areaLink->ref;
  FATALASSERT(mChunk->indexX == (uint32)aIndex.x);
  FATALASSERT(mChunk->indexY == (uint32)aIndex.y);
  CreateRefs(area, mRef, mChunk->nDoodadRefs, mChunk->nMapObjRefs);

  for (i = 0; i < mChunk->nLayers; i++, mLayer++) {
    CreateLayer(area, mLayer, alphaTex);
  }

  if ((mChunk->flags & 1) && (CWorld::enables & CWorld::Enable_Shadow)) {
    shadowSize = mChunk->sizeShadow;
    CreateShadow(shadowTex);
  } else {
    shadowSize = 0;
    shadowOffs = 0;
  }

  if (CMap::EnableTerrainShader() || CMap::EnableSpecularTerrain()) {
    CreateAlphaShadow();
  }
  flags |= CMapBaseObj::Flag_LightUpdate;
  FindLights();

  area->chunkInfo[infoIndex].asyncId = 0;
  area->chunkTable[infoIndex] = this;
}

void CMapChunk::SelectLights() {
  flags &= ~1u;

  GxLightSet(0, CMap::sunLight->gxLight, CWorldScene::camPos);

  UINT whichLight = 1;

  ITERATELIST(CMapBaseObjLink, lightLinkList, link) {
    GxLightSet(whichLight, ((CMapLight *)link->owner)->gxLight, CWorldScene::camPos);
    ++whichLight;
    if (whichLight == 8) {
      return;
    }
  }

  do {
    GxLightEnable(whichLight, 0);
    ++whichLight;
  } while (whichLight != 8);
}

void CMapChunk::UpdateLights() {
  flags |= 1u;

  {
    ITERATELIST(CMapBaseObjLink, doodadDefLinkList, link) {
      link->owner->flags |= 1u;
    }
  }

  {
    ITERATELIST(CMapBaseObjLink, entityLinkList, link) {
      link->owner->flags |= 1u;
    }
  }
}

void CMapChunk::Update() {
  freeTime += CWorld::tickTimeSec;
  if (freeTime > 5.0f) {
    if (gxBuf) {
      FreeGxBuf(gxBuf);
      gxBuf = 0;
    }

    if (detailDoodadInst && detailDoodadInst->HasBufs()) {
      detailDoodadInst->FreeBufs();
    }
  }

  if (detailDoodadInst && freeTime > 10.0f) {
    CDetailDoodad::FreeInst(detailDoodadInst);
    detailDoodadInst = 0;
  }

  if (CWorldScene::camFrustumBounds.Intersects(aaBox)) {
    freeTime = 0.0f;
    CWorldScene::AddMapChunk(this, CWorldScene::camPlaneXY.DistSigned(vertexList[cornerVertexIndex[farCornerIndex]] + corner));

    ITERATELIST(CMapBaseObjLink, doodadDefLinkList, link) {
      CMapDoodadDef *doodadDef = (CMapDoodadDef *)link->owner;
      FATALASSERT(doodadDef);
      if ((doodadDef->model || doodadDef->RenderCB) && !(doodadDef->flags & CMapBaseObj::Flag_LoadFailed) && !doodadDef->sceneLink.IsLinked()) {
        CWorldScene::AddDoodadDef(doodadDef);
      }
    }
  }

  for (UINT i = 0; i < 4; ++i) {
    if (liquids[i]) {
      NTempest::CAaBox aaBox;
      liquids[i]->GetAaBox(aaBox);
      if (aaBox.b <= CWorldScene::camFrustumBounds.t && aaBox.t >= CWorldScene::camFrustumBounds.b) {
        CWorldScene::AddChunkLiquid(liquids[i], i);
      }
    }
  }
}

void CMapChunk::FindLights() {
  ITERATELIST(CMapLight, CMap::lightList, light) {
    if (!(light->aaBox.b.x > aaBox.t.x || light->aaBox.t.x < aaBox.b.x || light->aaBox.b.y > aaBox.t.y || light->aaBox.t.y < aaBox.b.y ||
          light->aaBox.b.z > aaBox.t.z || light->aaBox.t.z < aaBox.b.z))
    {
      CMapBaseObjLink *link = CMap::AllocBaseObjLink(light);
      link->ref = this;
      lightLinkList.LinkNode(link, LIST_TAIL, 0);
    }
  }
}

void CMapChunk::CreateVertices(float *heights) {
  FATALASSERT(heights);

  aaBox.b = FLT_MAX;
  aaBox.t = -FLT_MAX;

  NTempest::C3Vector wCorner((float)cOffset.x * 33.333332f, (float)cOffset.y * 33.333332f, 0.0f);
  float              temp = (-wCorner.x) + 17066.666f;
  wCorner.x = (-wCorner.y) + 17066.666f;
  wCorner.y = temp;

  NTempest::C3Vector wCornerP((float)(cOffset.x + 1) * 33.333332f, (float)(cOffset.y + 1) * 33.333332f, 0.0f);
  temp = (-wCornerP.x) + 17066.666f;
  wCornerP.x = (-wCornerP.y) + 17066.666f;
  wCornerP.y = temp;

  float               dx = (wCornerP.x - wCorner.x) / 8.0f;
  float               dx2 = dx / 2.0f;
  float               dy = (wCornerP.y - wCorner.y) / 8.0f;
  float               dy2 = dy / 2.0f;
  float              *he = heights;
  float              *ho = heights + 81;
  NTempest::C3Vector *v = vertexList;

  for (int x = 0; x < 9; ++x) {
    float fx = (float)x * dx + wCorner.x;
    for (int y = 0; y < 9; ++y, ++v) {
      v->x = fx;
      v->y = (float)y * dy + wCorner.y;
      v->z = *he;
      ++he;
      aaBox.Enclose(*v);
      *v -= corner;
    }

    if (x < 8) {
      for (int y = 0; y < 8; ++y, ++v) {
        v->x = fx + dx2;
        v->y = (float)y * dy + wCorner.y + dy2;
        v->z = *ho;
        ++ho;
        aaBox.Enclose(*v);
        *v -= corner;
      }
    }
  }

  aaSphere.c = (aaBox.b + aaBox.t) / 2.0f;
  NTempest::C3Vector rv = aaBox.t - aaSphere.c;
  aaSphere.r = rv.Mag();
}

void CMapChunk::CreateNormals(signed char *normals) {
  FATALASSERT(normals);

  signed char        *outer = normals;
  signed char        *inner = outer + 243;
  NTempest::C3Vector *normal = normalList;

  for (int fixed = 0; fixed < 9; ++fixed) {
    for (int index = 0; index < 9; ++index) {
      normal->y = (float)(*outer++) * -0.0078740157f;
      normal->z = (float)(*outer++) * 0.0078740157f;
      normal->x = (float)(*outer++) * -0.0078740157f;
      ++normal;
    }

    if (fixed < 8) {
      for (int index = 0; index < 8; ++index) {
        normal->y = (float)(*inner++) * -0.0078740157f;
        normal->z = (float)(*inner++) * 0.0078740157f;
        normal->x = (float)(*inner++) * -0.0078740157f;
        ++normal;
      }
    }
  }
}

void CMapChunk::CreateFacePlanes() {
  NTempest::C3Vector *v = vertexList;
  NTempest::C4Plane  *p = planeList;

  for (int y = 0; y < 8; ++y) {
    for (int x = 0; x < 8; ++x) {
      NTempest::C3Vector *center = v + 9;
      for (UINT face = 0; face < 4; ++face) {
        NTempest::C3Vector normal = NTempest::C3Vector::Cross(v[iIndiciesP[face][1]] - *center, v[iIndiciesP[face][0]] - *center);
        normal.Normalize();
        p->Set(normal, *center);
        ++p;
      }
      ++v;
    }
    v += 9;
  }
}

void CMapChunk::CreateLayer(CMapArea *area, SMLayer *layer, BYTE *alphaTex) {
  FATALASSERT(area);
  FATALASSERT(layer);
  FATALASSERT(nLayers <= 4);

  CChunkLayer *chunkLayer = CMap::GetLayer();
  layerList[nLayers] = chunkLayer;
  chunkLayer->chunk = this;
  ++nLayers;

  chunkLayer->texId = area->texIdTable[layer->textureId];
  chunkLayer->props = layer->props;
  chunkLayer->offsAlpha = alphaTex + layer->offsAlpha;
  chunkLayer->effectId = layer->effectId;

  if (!CMap::EnableTerrainShader() && !CMap::EnableSpecularTerrain()) {
    int sizeX;
    int sizeY;
    if (CWorld::alphaMipLevel == 1) {
      sizeX = 32;
      sizeY = 32;
    } else {
      sizeX = 64;
      sizeY = 64;
    }

    if (chunkLayer->props & 0x100) {
      chunkLayer->gxTexture = AllocAlphaGxTex(chunkLayer, UpdateLayerGxTexture);
      FATALASSERT(chunkLayer->gxTexture);
      CreateChunkLayerTex(chunkLayer);
      GxTexUpdate(chunkLayer->gxTexture, 0, 0, sizeX, sizeY, 1);
    }
  }
}

void CMapChunk::CreateShadow(BYTE *shadowTex) {
  shadowOffs = shadowTex;
  if (!CMap::EnableTerrainShader() && !CMap::EnableSpecularTerrain()) {
    int sizeX;
    int sizeY;
    if (CWorld::shadowMipLevel == 1) {
      sizeX = 32;
      sizeY = 32;
    } else {
      sizeX = 64;
      sizeY = 64;
    }

    shadowGxTexture = AllocShadowGxTex(this, UpdateShadowGxTexture);
    FATALASSERT(shadowGxTexture);
    CreateChunkShadowTex();
    GxTexUpdate(shadowGxTexture, 0, 0, sizeX, sizeY, 1);
  }
}

void CMapChunk::CreateAlphaShadow() {
  FATALASSERT(CMap::EnableTerrainShader() || CMap::EnableSpecularTerrain());

  int sizeX;
  int sizeY;
  if (CWorld::shadowMipLevel == 1) {
    sizeX = 32;
    sizeY = 32;
  } else {
    sizeX = 64;
    sizeY = 64;
  }

  shaderGxTexture = AllocShadowGxTex(this, UpdateShaderGxTexture);
  CreateChunkShaderTex();
  GxTexUpdate(shaderGxTexture, 0, 0, sizeX, sizeY, 1);
}

void CMapChunk::CreateRefs(CMapArea *area, UINT *ref, UINT doodadCnt, UINT mapObjCnt) {
  FATALASSERT(area);
  FATALASSERT(ref);

  NTempest::C3Vector pos(17066.666f, 17066.666f, 0.0f);
  while (doodadCnt > 0) {
    CMapDoodadDef *doodadDef = CMap::CreateDoodadDef(area->doodadDefList[*ref], pos);
    if (doodadDef) {
      CMapBaseObjLink *link = CMap::AllocBaseObjLink(doodadDef);
      link->ref = this;
      doodadDefLinkList.LinkNode(link, LIST_TAIL, 0);
      doodadDef->flags |= CMapBaseObj::Flag_ExteriorLit;
      doodadDef->dirLightScale = 1.0f;
    }
    ++ref;
    --doodadCnt;
  }

  while (mapObjCnt > 0) {
    CMapObjDef      *mapObjDef = CMap::CreateMapObjDef(area->mapObjDefList[*ref], pos);
    CMapBaseObjLink *link = CMap::AllocBaseObjLink(mapObjDef);
    link->ref = this;
    mapObjDefLinkList.LinkNode(link, LIST_TAIL, 0);
    ++ref;
    --mapObjCnt;
  }
}

void CMapChunk::CreateChunkShadowTex() {
  CChunkTex *tex = CMap::GetTex();
  FATALASSERT(tex);
  shadowTexture = tex;
  UnpackShadowBits(shadowTexture->pixels, shadowBits, shadowOffs);
}

void CMapChunk::CreateChunkLayerTex(CChunkLayer *layer) {
  FATALASSERT(layer);
  CChunkTex *tex = CMap::GetTex();
  FATALASSERT(tex);
  layer->tex = tex;
  UnpackAlphaBits(layer->tex->pixels, layer->offsAlpha);
}

void CMapChunk::CreateChunkShaderTex() {
  shaderTexture = CMap::GetTex();
  FATALASSERT(shaderTexture);

  NTempest::CImVector *texels = (NTempest::CImVector *)shaderTexture->pixels;
  const BYTE          *alpha[4];
  for (UINT i = 0; i < 4; ++i) {
    alpha[i] = 0;
    if (i < nLayers && (layerList[i]->props & 0x100)) {
      alpha[i] = layerList[i]->offsAlpha;
    }
  }

  UnpackAlphaShadowBits(texels, shadowBits, alpha, shadowOffs);
}

void CMapChunk::UnpackAlphaShadowBits(NTempest::CImVector *texels, DWORD *bits, const BYTE *const alpha[], const BYTE *shadow) {
  UINT coordDelta = 1;
  UINT ySrcDelta = 0;
  UINT xSrcDelta = 1;
  UINT coord = 0;
  UINT y;
  UINT x;

  if (CWorld::shadowMipLevel == 1) {
    coordDelta = 2;
    ySrcDelta = 64;
    xSrcDelta = 2;
  }

  for (y = 0; y < 64; y += coordDelta) {
    for (x = 0; x < 64; x += coordDelta) {
      UINT offset = coord >> 1;
      UINT shadowCoord = coord >> 3;
      UINT alphaBitMask = ALPHA_BIT_MASK[coord & 1];
      UINT alphaBitLShift = ALPHA_BIT_LSHIFT[coord & 1];
      UINT shadowBitMask = SHADOW_BIT_MASK[coord & 7];
      UINT shadowBitRShift = SHADOW_BIT_RSHIFT[coord & 7];

      texels++->Set(
          shadow ? SHADOW_VALUES[(shadow[shadowCoord] & shadowBitMask) >> shadowBitRShift] : SHADOW_VALUES[0],
          alpha[1] ? (alpha[1][offset] & alphaBitMask) << alphaBitLShift : 0xFF,
          alpha[2] ? (alpha[2][offset] & alphaBitMask) << alphaBitLShift : 0xFF,
          alpha[3] ? (alpha[3][offset] & alphaBitMask) << alphaBitLShift : 0xFF
      );
      coord += xSrcDelta;
    }
    coord += ySrcDelta;
  }

  coord = 0;
  for (y = 0; y < 64; y += 2) {
    for (x = 0; x < 64; x += 2) {
      *bits |= (shadow ? (shadow[coord >> 3] & SHADOW_BIT_MASK[coord & 7]) >> SHADOW_BIT_RSHIFT[coord & 7] : 0) << (x >> 1);
      coord += 2;
    }
    ++bits;
    coord += 64;
  }
}

void CMapChunk::UnpackAlphaBits(DWORD *pixels, const BYTE *alphaPixels) {
  FATALASSERT(pixels);
  FATALASSERT(alphaPixels);

  UINT source = 0;
  if (CWorld::alphaMipLevel == 1) {
    UINT dest = 0;
    for (UINT y = 0; y < 32; ++y) {
      for (UINT x = 0; x < 32; ++x) {
        pixels[dest] = 0x00FFFFFF | (alphaPixels[source++] << 28);
        ++dest;
      }
      source += 32;
    }
  } else {
    for (UINT i = 0; i < 4096; ++i) {
      pixels[i] = (i & 1) ? 0x00FFFFFF | ((*alphaPixels++ & 0xF0) << 24) : 0x00FFFFFF | (*alphaPixels << 28);
    }
  }
}

void CMapChunk::UnpackShadowBits(DWORD *pixels, DWORD *shadowBits, const BYTE *shadow) {
  FATALASSERT(pixels);
  FATALASSERT(shadowBits);
  FATALASSERT(shadow);

  int  yIdx = 0;
  int  index = 0;
  UINT srcMask = 0x01010101;
  UINT dstMask;
  int  y;
  int  x;
  int  idx;

  if (CWorld::shadowMipLevel == 1) {
    for (y = 0; y < 32; ++y) {
      srcMask = 0x01010101;
      dstMask = 1;
      idx = yIdx;
      for (x = 0; x < 32; ++x) {
        if (shadow[idx >> 3] & srcMask) {
          pixels[index] = 0xFFFFFFFF;
          shadowBits[index >> 5] |= dstMask;
        } else {
          pixels[index] = 0;
        }
        idx += 2;
        ++index;
        dstMask = _rotl(dstMask, 1);
        srcMask = _rotl(srcMask, 2);
      }
      yIdx += 128;
    }
  } else {
    for (y = 0; y < 64; ++y, yIdx += 64) {
      idx = yIdx;
      for (x = 0; x < 64; ++x) {
        if (shadow[idx >> 3] & srcMask) {
          pixels[idx] = 0xFFFFFFFF;
        } else {
          pixels[idx] = 0;
        }
        srcMask = _rotl(srcMask, 1);
        ++idx;
      }

      if (!(y & 1)) {
        idx = yIdx;
        srcMask = 0x01010101;
        dstMask = 1;
        for (x = 0; x < 32; ++x) {
          if (shadow[idx >> 3] & srcMask) {
            shadowBits[index >> 5] |= dstMask;
          }
          idx += 2;
          ++index;
          dstMask = _rotl(dstMask, 1);
          srcMask = _rotl(srcMask, 2);
        }
      }
    }
  }
}

void CMapChunk::UpdateLayerGxTexture(
    EGxTexCommand cmd,
    UINT          w,
    UINT          h,
    UINT          d,
    UINT          mipLevel,
    LPVOID        userArg,
    UINT         &texelStrideInBytes,
    LPCVOID      &texels
) {
  CChunkLayer *layer = (CChunkLayer *)userArg;
  FATALASSERT(layer);

  switch (cmd) {
    case GxTex_Lock:
      if (!layer->tex) {
        layer->chunk->SyncLoadLayer(layer);
      }
      return;

    case GxTex_Latch:
      texelStrideInBytes = 4 * w;
      texels = layer->tex->pixels;
      return;

    case GxTex_Unlock:
      CMap::FreeTex(layer->tex);
      layer->tex = 0;
      return;
  }
}

void CMapChunk::UpdateShadowGxTexture(
    EGxTexCommand cmd,
    UINT          w,
    UINT          h,
    UINT          d,
    UINT          mipLevel,
    LPVOID        userArg,
    UINT         &texelStrideInBytes,
    LPCVOID      &texels
) {
  CMapChunk *chunk = (CMapChunk *)userArg;
  FATALASSERT(chunk);

  switch (cmd) {
    case GxTex_Lock:
      if (!chunk->shadowTexture) {
        chunk->SyncLoadShadow();
      }
      return;

    case GxTex_Latch:
      texelStrideInBytes = 4 * w;
      texels = chunk->shadowTexture->pixels;
      return;

    case GxTex_Unlock:
      CMap::FreeTex(chunk->shadowTexture);
      chunk->shadowTexture = 0;
      return;
  }
}

void CMapChunk::UpdateShaderGxTexture(
    EGxTexCommand cmd,
    UINT          w,
    UINT          h,
    UINT          d,
    UINT          mipLevel,
    LPVOID        userArg,
    UINT         &texelStrideInBytes,
    LPCVOID      &texels
) {
  CMapChunk *chunk = (CMapChunk *)userArg;
  FATALASSERT(chunk);

  switch (cmd) {
    case GxTex_Lock:
      if (!chunk->shaderTexture) {
        chunk->SyncLoadShader();
      }
      return;

    case GxTex_Latch:
      texelStrideInBytes = 4 * w;
      texels = chunk->shaderTexture->pixels;
      return;

    case GxTex_Unlock:
      CMap::FreeTex(chunk->shaderTexture);
      chunk->shaderTexture = 0;
      return;
  }
}

void CMapChunk::UpdateTextureDefault(
    EGxTexCommand cmd,
    UINT          w,
    UINT          h,
    UINT          d,
    UINT          mipLevel,
    LPVOID        userArg,
    UINT         &texelStrideInBytes,
    LPCVOID      &texels
) {
  if (userArg) {
    SErrDisplayError(STORM_ERROR_ASSERTION, __FILE__, __LINE__, "1", FALSE, 1);
  }
}
