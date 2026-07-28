#include "WorldClient/World.h"
#include "WorldClient/CMapObj.h"
#include "WorldClient/DetailDoodad.h"

#include <float.h>

#include "Base/Base.h"
#include "Gx/Gx.h"
#include "Os/W32/Debugging.h"
#include "Services/AsyncFileRead.h"

typedef unsigned int uint32;

struct STPrimRemap {
  unsigned short  nIndicies;
  unsigned short *indicies;
};

struct STPrimGroup {
  unsigned short  nIndicies;
  unsigned short  primType;
  unsigned short *indicies;
};

static unsigned short s_vertexRemap0[10] = {0, 3, 8, 1, 72, 2, 136, 4, 144, 0};
static unsigned short s_primGroup0_0[5] = {0, 1, 2, 3, 4};
static unsigned short s_primGroup0_1[3] = {2, 4, 0};

static unsigned short s_vertexRemap1[26] = {0, 0, 4, 4, 8, 6, 36, 2, 40, 5, 68, 1, 72, 3, 76, 7, 104, 10, 108, 8, 136, 9, 140, 11, 144, 12};
static unsigned short s_primGroup1_0[25] = {0, 1, 2, 3, 4, 5, 6, 7, 7, 5, 5, 3, 7, 8, 8, 9, 9, 9, 1, 10, 3, 11, 8, 12, 7};
static unsigned short s_primGroup1_1[6] = {2, 4, 0, 10, 9, 11};

static unsigned short s_vertexRemap2[82] = {0,  34,  2,  35,  4,  37,  6,  38,  8,  40,  18, 26,  20,  36,  22,  18,  24,  39,  34,  27, 36,
                                            25, 38,  17, 40,  0,  42,  1,  52,  28, 54,  24, 56,  16,  58,  2,   68,  29,  70,  23,  72, 15,
                                            74, 4,   76, 3,   86, 30,  88, 22,  90, 14,  92, 5,   102, 31,  104, 21,  106, 13,  108, 6,  110,
                                            7,  120, 32, 122, 20, 124, 12, 126, 8,  136, 33, 138, 19,  140, 11,  142, 10,  144, 9};
static unsigned short s_primGroup2_0[69] = {0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 10, 10, 10, 10, 11, 12, 13, 6,  14, 4,  15,
                                            16, 17, 0,  18, 18, 19, 19, 19, 11, 20, 13, 21, 22, 23, 15, 24, 17, 25, 25, 26, 26, 25, 27,
                                            28, 29, 23, 30, 21, 31, 32, 33, 19, 19, 27, 27, 34, 26, 35, 25, 36, 17, 37, 18, 38, 0,  39};
static unsigned short s_primGroup2_1[48] = {39, 0,  1,  39, 1, 40, 39, 40, 38, 16, 4,  0,  2,  0,  4,  28, 23, 25, 24, 25, 23, 14, 15, 13,
                                            22, 13, 15, 8,  6, 10, 12, 10, 6,  20, 21, 19, 32, 19, 21, 36, 37, 35, 5,  7,  3,  30, 29, 31};

static unsigned short s_vertexRemap3[290] = {
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
static unsigned short s_primGroup3_0[392] = {
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
static unsigned short s_primGroup3_1[90] = {136, 134, 138, 142, 134, 132, 130, 128, 132, 15,  128, 17,  125, 11,  17,  65,  57,  58,
                                            59,  60,  58,  69,  60,  62,  63,  89,  62,  90,  89,  91,  93,  92,  91,  7,   92,  8,
                                            12,  9,   8,   32,  22,  21,  23,  22,  24,  110, 111, 109, 115, 108, 109, 79,  80,  78,
                                            61,  46,  40,  54,  55,  53,  82,  84,  81,  0,   2,   86,  127, 129, 126, 133, 135, 131,
                                            140, 141, 139, 144, 141, 28,  124, 52,  123, 42,  43,  41,  121, 122, 120, 76,  77,  75};

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

static unsigned int g_gxBufCreateCount;
static unsigned int g_gxBufDestroyCount;

static TSCArray<unsigned char, 15000>   s_syncLoadBuffer;
static SCritSect                        s_fileCritSect;
static TSCArray<unsigned char, 15000>   s_asyncLoadBuffers[16];
static unsigned char                   *s_freeAsyncBuffer;
static unsigned char                    s_asyncBuffersInitialized;
static LISTDECLEX(CAsyncObject, link, s_asyncLoadList);

unsigned int              CMapChunk::cornerVertexIndex[4] = {0, 8, 136, 144};
unsigned int              CMapChunk::farCornerIndex;

static int iIndiciesP[4][2] = {
    {17,  0},
    { 0,  1},
    {18, 17},
    { 1, 18}
};
unsigned char             CMapChunk::syncLoadBuffer[15000];
NTempest::C2Vector        CMapChunk::texCoordList[145];
NTempest::C2Vector        CMapChunk::texCoordList2[145];
NTempest::C2Vector        CMapChunk::rmTexCoordList[4][145];
NTempest::C2Vector        CMapChunk::rmTexCoordList2[4][145];
CGxBatch                  CMapChunk::rmGxBatchList[4][2];
unsigned short            CMapChunk::primList[768];
unsigned short           *CMapChunk::primPtr;
TSGrowableArray<CGxBuf *> CMapChunk::gxBufFreeList;
CGxBuf                   *CMapChunk::gxBufDyn;
TSGrowableArray<CGxTex *> CMapChunk::gxAlphaTexFreeList;
TSGrowableArray<CGxTex *> CMapChunk::gxShadowTexFreeList;
void(*CMapChunk::soundEmitterCreateHandler)(CWSoundEmitter &);
void(*CMapChunk::soundEmitterDestroyHandler)(unsigned long);

static void ValidateAsyncReadBuffer(unsigned char *buffer) {
  for (unsigned int index = 0; index < 16; ++index) {
    if (buffer == s_asyncLoadBuffers[index].Ptr()) {
      return;
    }
  }

  FATALERROR(("%08x is not a valid map chunk async read buffer", buffer));
}

void CMapChunk::FreeAsyncLoadBuffer(unsigned char *buffer) {
  ValidateAsyncReadBuffer(buffer);
  *reinterpret_cast<unsigned char **>(buffer) = s_freeAsyncBuffer;
  s_freeAsyncBuffer = buffer;
}

void CMapChunk::InitAsyncLoadBuffers() {
  for (unsigned int index = 0; index < 16; ++index) {
    FreeAsyncLoadBuffer(s_asyncLoadBuffers[index].Ptr());
  }
}

unsigned char *CMapChunk::AllocAsyncLoadBuffer() {
  if (!s_asyncBuffersInitialized) {
    InitAsyncLoadBuffers();
    s_asyncBuffersInitialized = 1;
  }

  unsigned char *buffer = s_freeAsyncBuffer;
  if (!buffer) {
    return 0;
  }

  ValidateAsyncReadBuffer(buffer);
  s_freeAsyncBuffer = *reinterpret_cast<unsigned char **>(buffer);
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
  ASSERT(gxTexture == 0);
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
  unsigned int index;

  for (index = 0; index < gxBufFreeList.Count(); ++index) {
    GxBufDestroy(gxBufFreeList[index]);
    ++g_gxBufDestroyCount;
  }
  gxBufFreeList.SetCount(0);

  ASSERT(g_gxBufCreateCount == g_gxBufDestroyCount);

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
    unsigned char *buffer = AllocAsyncLoadBuffer();
    if (!buffer) {
      break;
    }

    CAsyncObject *next = s_asyncLoadList.Next(object);
    object->link.Unlink();
    object->buffer = buffer;
    object->canReorder = 1;
    AsyncFileReadObject(object);
    object = next;
  }
}

CGxBuf *CMapChunk::AllocGxBuf(unsigned int indexCount) {
  CGxBuf *gxBuf;

  if (gxBufFreeList.Count()) {
    gxBuf = gxBufFreeList[gxBufFreeList.Count() - 1];
    FATALASSERT(gxBuf);
    gxBufFreeList.SetCount(gxBufFreeList.Count() - 1);
    gxBuf->CountSet(145, indexCount);
    gxBuf->Invalidate(CGxBuf::S_INVALID_DISCARD, CGxBuf::S_INVALID_DISCARD);
  } else {
    gxBuf = GxBufCreate(GxBWF_Low, GxVBF_PN, 145, indexCount, GxBufFillCallback, 0);
    ++g_gxBufCreateCount;
  }

  return gxBuf;
}

void CMapChunk::FreeGxBuf(CGxBuf *gxBuf) {
  FATALASSERT(gxBuf);

  gxBufFreeList.Add(&gxBuf);
}

CGxTex *CMapChunk::AllocAlphaGxTex(
    void *userArg,
    void(*userFunc)(EGxTexCommand, unsigned int, unsigned int, unsigned int, unsigned int, void *, unsigned int &, const void *&)
) {
  CGxTex *gxTex;

  if (gxAlphaTexFreeList.Count()) {
    gxTex = gxAlphaTexFreeList[gxAlphaTexFreeList.Count() - 1];
    FATALASSERT(gxTex);
    gxAlphaTexFreeList.SetCount(gxAlphaTexFreeList.Count() - 1);
    GxTexSetUserData(gxTex, userFunc, userArg);
  } else {
    unsigned int size = CWorld::alphaMipLevel == 1 ? 32 : 64;
    CGxTexFlags  texFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1);
    GxTexCreate(
        GxTex_2d,
        size,
        size,
        0,
        GxTex_Argb4444,
        GxTex_Argb8888,
        texFlags,
        userArg,
        userFunc,
        gxTex
    );
    FATALASSERT(gxTex);
  }

  return gxTex;
}

void CMapChunk::FreeAlphaGxTex(CGxTex *gxTex) {
  FATALASSERT(gxTex);

  GxTexSetUserData(gxTex, UpdateTextureDefault, 0);
  gxAlphaTexFreeList.Add(&gxTex);
}

CGxTex *CMapChunk::AllocShadowGxTex(
    void *userArg,
    void(*userFunc)(EGxTexCommand, unsigned int, unsigned int, unsigned int, unsigned int, void *, unsigned int &, const void *&)
) {
  CGxTex *gxTex;

  if (gxShadowTexFreeList.Count()) {
    gxTex = gxShadowTexFreeList[gxShadowTexFreeList.Count() - 1];
    FATALASSERT(gxTex);
    gxShadowTexFreeList.SetCount(gxShadowTexFreeList.Count() - 1);
    GxTexSetUserData(gxTex, userFunc, userArg);
  } else {
    unsigned int size = CWorld::shadowMipLevel == 1 ? 32 : 64;
    CGxTexFlags  texFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1);
    GxTexCreate(
        GxTex_2d,
        size,
        size,
        0,
        GxTex_Argb4444,
        GxTex_Argb8888,
        texFlags,
        userArg,
        userFunc,
        gxTex
    );
    FATALASSERT(gxTex);
  }

  return gxTex;
}

void CMapChunk::FreeShadowGxTex(CGxTex *gxTex) {
  FATALASSERT(gxTex);

  gxShadowTexFreeList.Add(&gxTex);
  GxTexSetUserData(gxTex, UpdateTextureDefault, 0);
}

void CMapChunk::CreateRenderLists() {
  unsigned short index = 0;
  float          texCoordY = 0.0f;
  float          texCoordHalfY = 0.0625f;

  for (unsigned int row = 0; row < 9; ++row) {
    float texCoordX = 0.0f;
    for (unsigned int column = 0; column < 9; ++column) {
      texCoordList[index].x = texCoordX * 8.0f;
      texCoordList[index].y = texCoordY * 8.0f;
      ++index;
      texCoordX += 0.125f;
    }

    texCoordY += 0.125f;
    if (row < 8) {
      float texCoordHalfX = 0.0625f;
      for (unsigned int column = 0; column < 8; ++column) {
        texCoordList[index].x = texCoordHalfX * 8.0f;
        texCoordList[index].y = texCoordHalfY * 8.0f;
        ++index;
        texCoordHalfX += 0.125f;
      }
      texCoordHalfY += 0.125f;
    }
  }

  index = 0;
  texCoordY = 0.0f;
  texCoordHalfY = 0.0625f;
  {
    for (unsigned int row = 0; row < 9; ++row) {
      float texCoordX = 0.0f;
      for (unsigned int column = 0; column < 9; ++column) {
        texCoordList2[index].x = texCoordX;
        texCoordList2[index].y = texCoordY;
        ++index;
        texCoordX += 0.1220703125f;
      }

      texCoordY += 0.1220703125f;
      if (row < 8) {
        float texCoordHalfX = 0.0625f;
        for (unsigned int column = 0; column < 8; ++column) {
          texCoordList2[index].x = texCoordHalfX;
          texCoordList2[index].y = texCoordHalfY;
          ++index;
          texCoordHalfX += 0.1220703125f;
        }
        texCoordHalfY += 0.1220703125f;
      }
    }
  }

  for (unsigned int lod = 0; lod < 4; ++lod) {
    STPrimRemap &remap = s_tPrimRemap[lod];
    for (unsigned int remapIndex = 0; remapIndex < remap.nIndicies; ++remapIndex) {
      unsigned short sourceIndex = remap.indicies[remapIndex * 2];
      unsigned short destIndex = remap.indicies[remapIndex * 2 + 1];
      rmTexCoordList[lod][destIndex] = texCoordList[sourceIndex];
      rmTexCoordList2[lod][destIndex] = texCoordList2[sourceIndex];
    }
  }

  {
    for (unsigned int lod = 0; lod < 4; ++lod) {
      unsigned int stripCount = s_tPrimGroups[lod][0].nIndicies;
      rmGxBatchList[lod][0].m_primType = GxPrim_TriangleStrip;
      rmGxBatchList[lod][0].m_count = stripCount;
      rmGxBatchList[lod][0].m_start = 0;
      rmGxBatchList[lod][1].m_primType = GxPrim_Triangles;
      rmGxBatchList[lod][1].m_count = s_tPrimGroups[lod][1].nIndicies;
      rmGxBatchList[lod][1].m_start = stripCount;
    }
  }

  {
    for (unsigned int lod = 0; lod < 4; ++lod) {
      STPrimRemap &remap = s_tPrimRemap[lod];
      for (unsigned short remapIndex = 0; remapIndex < remap.nIndicies - 1; ++remapIndex) {
        unsigned short minIndex = remapIndex;
        for (unsigned short candidate = remapIndex + 1; candidate < remap.nIndicies; ++candidate) {
          if (remap.indicies[candidate * 2 + 1] < remap.indicies[minIndex * 2 + 1]) {
            minIndex = candidate;
          }
        }

        unsigned short sourceIndex = remap.indicies[remapIndex * 2];
        unsigned short destIndex = remap.indicies[remapIndex * 2 + 1];
        remap.indicies[remapIndex * 2] = remap.indicies[minIndex * 2];
        remap.indicies[remapIndex * 2 + 1] = remap.indicies[minIndex * 2 + 1];
        remap.indicies[minIndex * 2] = sourceIndex;
        remap.indicies[minIndex * 2 + 1] = destIndex;
      }
    }
  }
}

void CMapChunk::AsyncCallback(void *userArg) {
  CMapChunk *chunk = static_cast<CMapChunk *>(userArg);
  FATALASSERT(chunk);

  chunk->Create(static_cast<unsigned char *>(chunk->asyncObject->buffer));
  FreeAsyncLoadBuffer(static_cast<unsigned char *>(chunk->asyncObject->buffer));
  chunk->asyncObject->buffer = 0;
  AsyncFileReadDestroyObject(chunk->asyncObject);
  chunk->asyncObject = 0;
}

CMapChunk::CMapChunk() {
  detailDoodadInst = 0;
  for (unsigned int i = 0; i < 4; ++i) {
    liquids[i] = 0;
  }

  nLayers = 0;
  shadowTexture = 0;
  shadowGxTexture = 0;
  gxBuf = 0;
  shaderTexture = 0;
  shaderGxTexture = 0;
  asyncObject = 0;
  flags |= 4;
}

CMapChunk::~CMapChunk() {
  ASSERT(detailDoodadInst == 0);
  ASSERT(asyncObject == 0);
  ASSERT(refCount == 0);
  ASSERT(gxBuf == 0);
  ASSERT(shaderGxTexture == 0);
  ASSERT(shadowGxTexture == 0);

  for (unsigned int i = 0; i < 4; ++i) {
    ASSERT(liquids[i] == 0);
  }
}

void CMapChunk::Load(SMChunkInfo *chunkInfo) {
  FATALASSERT(chunkInfo);
  FATALASSERT(chunkInfo->size < 15000);

  nLayers = 0;
  asyncObject = 0;
  gxBuf = 0;
  lod = -1;
  remapLod = -1;
  detailDoodadInst = 0;
  freeTime = 0.0f;
  bLoaded = 0;
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
    chunkInfo->asyncId = reinterpret_cast<unsigned int>(asyncObject);
  }
}

void CMapChunk::SyncLoadLayer(CChunkLayer *layer) {
  SMChunk      *mChunk = 0;
  SMLayer      *mLayer = 0;
  unsigned char *shadowTex = 0;
  unsigned char *alphaTex = 0;

  s_fileCritSect.Enter();
  SyncLoad(mChunk, mLayer, shadowTex, alphaTex);

  unsigned int i;
  for (i = 0; i < nLayers; ++i) {
    if (layerList[i] == layer) {
      break;
    }
  }
  FATALASSERT(i < nLayers);

  layer->offsAlpha = alphaTex + mLayer[i].offsAlpha;
  CreateChunkLayerTex(layer);
  s_fileCritSect.Leave();
}

void CMapChunk::SyncLoadShadow() {
  SMChunk      *mChunk = 0;
  SMLayer      *mLayer = 0;
  unsigned char *shadowTex = 0;
  unsigned char *alphaTex = 0;

  s_fileCritSect.Enter();
  SyncLoad(mChunk, mLayer, shadowTex, alphaTex);
  shadowOffs = shadowTex;
  shadowSize = mChunk->sizeShadow;
  CreateChunkShadowTex();
  s_fileCritSect.Leave();
}

void CMapChunk::SyncLoadShader() {
  SMChunk      *mChunk = 0;
  SMLayer      *mLayer = 0;
  unsigned char *shadowTex = 0;
  unsigned char *alphaTex = 0;

  s_fileCritSect.Enter();
  SyncLoad(mChunk, mLayer, shadowTex, alphaTex);

  for (unsigned int i = 0; i < mChunk->nLayers; ++i) {
    layerList[i]->offsAlpha = alphaTex + mLayer[i].offsAlpha;
  }
  shadowOffs = shadowTex;
  shadowSize = mChunk->sizeShadow;
  CreateChunkShaderTex();
  s_fileCritSect.Leave();
}

void CMapChunk::SyncLoad(SMChunk *&mChunk, SMLayer *&mLayer, unsigned char *&shadowTex, unsigned char *&alphaTex) {
  FATALASSERT(CMap::wdtFile);

  SFile::SetFilePointer(CMap::wdtFile, fileOffset, 0, FILE_BEGIN);
  SFile::Read(CMap::wdtFile, s_syncLoadBuffer.Ptr(), fileSize, 0, 0, 0);

  SIffChunk *iffChunk = reinterpret_cast<SIffChunk *>(s_syncLoadBuffer.Ptr());
  FATALASSERT(iffChunk->token=='MCNK');
  mChunk = reinterpret_cast<SMChunk *>(iffChunk + 1);

  float    *mHeights = reinterpret_cast<float *>(mChunk + 1);
  SMNormal *mNormals = reinterpret_cast<SMNormal *>(mHeights + 145);
  iffChunk = reinterpret_cast<SIffChunk *>(mNormals + 1);
  FATALASSERT(iffChunk->token=='MCLY');
  mLayer = reinterpret_cast<SMLayer *>(iffChunk + 1);

  iffChunk = reinterpret_cast<SIffChunk *>(reinterpret_cast<unsigned char *>(mLayer) + iffChunk->size);
  shadowTex = reinterpret_cast<unsigned char *>(iffChunk + 1) + iffChunk->size;
  alphaTex = shadowTex + mChunk->sizeShadow;
}

void CMapChunk::Create(unsigned char *data) {
  FATALASSERT(data);
  FATALASSERT(CMap::bActive);
  FATALASSERT(bLoaded == 0);

  SIffChunk *iffChunk = reinterpret_cast<SIffChunk *>(data);
  FATALASSERT(iffChunk->token=='MCNK');
  SMChunk  *mChunk = reinterpret_cast<SMChunk *>(iffChunk + 1);
  float    *mHeights = reinterpret_cast<float *>(mChunk + 1);
  SMNormal *mNormals = reinterpret_cast<SMNormal *>(mHeights + 145);

  iffChunk = reinterpret_cast<SIffChunk *>(mNormals + 1);
  FATALASSERT(iffChunk->token=='MCLY');
  SMLayer *mLayer = reinterpret_cast<SMLayer *>(iffChunk + 1);

  iffChunk = reinterpret_cast<SIffChunk *>(reinterpret_cast<unsigned char *>(mLayer) + iffChunk->size);
  FATALASSERT(iffChunk->token=='MCRF');
  unsigned int  *mRef = reinterpret_cast<unsigned int *>(iffChunk + 1);
  unsigned char *shadowTex = reinterpret_cast<unsigned char *>(mRef) + iffChunk->size;
  unsigned char *alphaTex = shadowTex + mChunk->sizeShadow;
  unsigned char *liquidData = alphaTex + mChunk->sizeAlpha;

  unsigned long mask = 4;
  for (unsigned int i = 0; i < 4; ++i) {
    if (mChunk->flags & mask) {
      if (!liquids[i]) {
        liquids[i] = CMap::AllocChunkLiquid();
      }
      memcpy(liquids[i], liquidData, 804);
      liquids[i]->chunk = this;
      liquidData += 804;
    } else if (liquids[i]) {
      CMap::FreeChunkLiquid(liquids[i]);
    }
    mask <<= 1;
  }

  if (soundEmitterCreateHandler) {
    unsigned char *emitterData = liquidData;
    for (unsigned int i = 0; i < mChunk->nSndEmitters; ++i) {
      CMapSoundEmitter *emitter = CMap::AllocSoundEmitter();
      memcpy(&emitter->data, emitterData, 32);
      emitter->data.startTime = *reinterpret_cast<unsigned short *>(emitterData + 32);
      emitter->data.endTime = *reinterpret_cast<unsigned short *>(emitterData + 34);
      emitter->data.mode = *reinterpret_cast<unsigned short *>(emitterData + 36);
      emitter->data.groupSilenceMin = *reinterpret_cast<unsigned short *>(emitterData + 40);
      emitter->data.groupSilenceMax = *reinterpret_cast<unsigned short *>(emitterData + 42);
      emitter->data.playInstancesMin = *reinterpret_cast<unsigned short *>(emitterData + 44);
      emitter->data.playInstancesMax = *reinterpret_cast<unsigned short *>(emitterData + 46);
      emitter->data.loopCountMin = emitterData[38];
      emitter->data.loopCountMax = emitterData[39];
      emitter->data.interSoundGapMin = *reinterpret_cast<unsigned short *>(emitterData + 48);
      emitter->data.interSoundGapMax = *reinterpret_cast<unsigned short *>(emitterData + 50);
      soundEmitterList.LinkNode(emitter, LIST_TAIL, 0);
      soundEmitterCreateHandler(emitter->data);
      emitterData += 52;
    }
  }

  CMap::CreateChunkNeighborPtrs(this);
  corner.x = 17066.666f - static_cast<float>(cOffset.y) * 33.333332f;
  corner.y = 17066.666f - static_cast<float>(cOffset.x) * 33.333332f;
  corner.z = *mHeights;
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
  CreateNormals(reinterpret_cast<signed char *>(mNormals));
  CreateFacePlanes();

  CMapBaseObjLink *areaLink = parentLinkList.Head();
  CMapArea *area = static_cast<CMapArea *>(areaLink->ref);
  FATALASSERT(mChunk->indexX == (uint32)aIndex.x);
  FATALASSERT(mChunk->indexY == (uint32)aIndex.y);
  CreateRefs(area, mRef, mChunk->nDoodadRefs, mChunk->nMapObjRefs);

  for (unsigned int layerIndex = 0; layerIndex < mChunk->nLayers; ++layerIndex) {
    CreateLayer(area, &mLayer[layerIndex], alphaTex);
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

  unsigned int     whichLight = 1;

  ITERATELIST(CMapBaseObjLink, lightLinkList, link) {
    if (whichLight >= 8) {
      break;
    }
    CMapLight *light = static_cast<CMapLight *>(link->owner);
    GxLightSet(whichLight, light->gxLight, CWorldScene::camPos);
    ++whichLight;
  }

  while (whichLight < 8) {
    GxLightEnable(whichLight, 0);
    ++whichLight;
  }
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
  NTempest::CAaBox aaBox;

  freeTime += CWorld::tickTimeSec;
  if (freeTime > 5.0f) {
    if (gxBuf) {
      FreeGxBuf(gxBuf);
      gxBuf = 0;
    }

    if (detailDoodadInst && (detailDoodadInst->gxBuf[0] || detailDoodadInst->gxBuf[1])) {
      detailDoodadInst->FreeBufs();
    }
  }

  if (detailDoodadInst && freeTime > 10.0f) {
    CDetailDoodad::FreeInst(detailDoodadInst);
    detailDoodadInst = 0;
  }

  if (this->aaBox.b <= CWorldScene::camFrustumBounds.t && this->aaBox.t >= CWorldScene::camFrustumBounds.b) {
    freeTime = 0.0f;
    CWorldScene::AddMapChunk(this, CWorldScene::camPlaneXY.DistSigned(vertexList[cornerVertexIndex[farCornerIndex]] + corner));

    ITERATELIST(CMapBaseObjLink, doodadDefLinkList, link) {
      CMapDoodadDef *doodadDef = static_cast<CMapDoodadDef *>(link->owner);
      FATALASSERT(doodadDef);
      if ((doodadDef->model || doodadDef->RenderCB) && !(doodadDef->flags & CMapBaseObj::Flag_LoadFailed) && !doodadDef->sceneLink.IsLinked()) {
        CWorldScene::AddDoodadDef(doodadDef);
      }
    }
  }

  for (unsigned int i = 0; i < 4; ++i) {
    if (liquids[i]) {
      liquids[i]->GetAaBox(aaBox);
      if (aaBox.b <= CWorldScene::camFrustumBounds.t && aaBox.t >= CWorldScene::camFrustumBounds.b) {
        CWorldScene::AddChunkLiquid(liquids[i], i);
      }
    }
  }
}

void CMapChunk::FindLights() {
  ITERATELIST(CMapLight, CMap::lightList, light) {
    if (light->aaBox.b.x <= aaBox.t.x && light->aaBox.t.x >= aaBox.b.x && light->aaBox.b.y <= aaBox.t.y && light->aaBox.t.y >= aaBox.b.y &&
        light->aaBox.b.z <= aaBox.t.z && light->aaBox.t.z >= aaBox.b.z)
    {
      CMapBaseObjLink *link = CMap::AllocBaseObjLink(light);
      link->ref = this;
      lightLinkList.LinkNode(link, LIST_TAIL, 0);
    }
  }
}

void CMapChunk::CreateVertices(float *heights) {
  FATALASSERT(heights);

  aaBox.b = NTempest::C3Vector(FLT_MAX);
  aaBox.t = NTempest::C3Vector(-FLT_MAX);

  NTempest::C3Vector wCorner(static_cast<float>(cOffset.x) * 33.333332f, static_cast<float>(cOffset.y) * 33.333332f, 0.0f);
  float              temp = 17066.666f - wCorner.x;
  wCorner.x = 17066.666f - wCorner.y;
  wCorner.y = temp;

  NTempest::C3Vector wCornerP(static_cast<float>(cOffset.x + 1) * 33.333332f, static_cast<float>(cOffset.y + 1) * 33.333332f, 0.0f);
  temp = 17066.666f - wCornerP.x;
  wCornerP.x = 17066.666f - wCornerP.y;
  wCornerP.y = temp;

  float               dx = (wCornerP.x - wCorner.x) / 8.0f;
  float               dx2 = dx / 2.0f;
  float               dy = (wCornerP.y - wCorner.y) / 8.0f;
  float               dy2 = dy / 2.0f;
  float              *he = heights;
  float              *ho = heights + 81;
  NTempest::C3Vector *v = vertexList;

  for (int y = 0; y < 9; ++y) {
    float fx = static_cast<float>(y) * dx + wCorner.x;
    for (int x = 0; x < 9; ++x) {
      v->Set(fx, static_cast<float>(x) * dy + wCorner.y, *he++);
      aaBox.Enclose(*v);
      *v = *v - corner;
      ++v;
    }

    if (y < 8) {
      for (int x = 0; x < 8; ++x) {
        v->Set(fx + dx2, static_cast<float>(x) * dy + wCorner.y + dy2, *ho++);
        aaBox.Enclose(*v);
        *v = *v - corner;
        ++v;
      }
    }
  }

  NTempest::C3Vector rv = aaBox.b + aaBox.t;
  aaSphere.c = rv * 0.5f;
  rv = aaBox.t - aaSphere.c;
  aaSphere.r = rv.Mag();
}

void CMapChunk::CreateNormals(signed char *normals) {
  FATALASSERT(normals);

  signed char        *outer = normals;
  signed char        *inner = outer + 243;
  NTempest::C3Vector *normal = normalList;

  for (int fixed = 0; fixed < 9; ++fixed) {
    for (int index = 0; index < 9; ++index) {
      normal->x = static_cast<float>(outer[2]) * -0.0078740157f;
      normal->y = static_cast<float>(outer[0]) * -0.0078740157f;
      normal->z = static_cast<float>(outer[1]) * 0.0078740157f;
      outer += 3;
      ++normal;
    }

    if (fixed < 8) {
      for (int index = 0; index < 8; ++index) {
        normal->x = static_cast<float>(inner[2]) * -0.0078740157f;
        normal->y = static_cast<float>(inner[0]) * -0.0078740157f;
        normal->z = static_cast<float>(inner[1]) * 0.0078740157f;
        inner += 3;
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
      for (unsigned int face = 0; face < 4; ++face) {
        NTempest::C3Vector normal =
            NTempest::C3Vector::Cross(v[iIndiciesP[face][1]] - *center, v[iIndiciesP[face][0]] - *center);
        normal.Normalize();
        p->Set(normal, *center);
        ++p;
      }
      ++v;
    }
    v += 9;
  }
}

void CMapChunk::CreateLayer(CMapArea *area, SMLayer *layer, unsigned char *alphaTex) {
  FATALASSERT(area);
  FATALASSERT(layer);
  FATALASSERT(nLayers <= 4);

  CChunkLayer *chunkLayer = CMap::GetLayer();
  layerList[nLayers] = chunkLayer;
  chunkLayer->chunk = this;
  ++nLayers;

  chunkLayer->texId = area->texIdTable[layer->textureId];
  chunkLayer->props = static_cast<unsigned short>(layer->props);
  chunkLayer->offsAlpha = alphaTex + layer->offsAlpha;
  chunkLayer->effectId = layer->effectId;

  if (!CMap::EnableTerrainShader() && !CMap::EnableSpecularTerrain()) {
    int sizeY = CWorld::alphaMipLevel == 1 ? 32 : 64;
    if (chunkLayer->props & 0x100) {
      chunkLayer->gxTexture = AllocAlphaGxTex(chunkLayer, UpdateLayerGxTexture);
      FATALASSERT(chunkLayer->gxTexture);
      CreateChunkLayerTex(chunkLayer);
      GxTexUpdate(chunkLayer->gxTexture, 0, 0, sizeY, sizeY, 1);
    }
  }
}

void CMapChunk::CreateShadow(unsigned char *shadowTex) {
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

void CMapChunk::CreateRefs(CMapArea *area, unsigned int *ref, unsigned int doodadCnt, unsigned int mapObjCnt) {
  FATALASSERT(area);
  FATALASSERT(ref);

  NTempest::C3Vector pos(17066.666f, 17066.666f, 0.0f);
  while (doodadCnt) {
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

  while (mapObjCnt) {
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

  const unsigned char *alpha[4];
  for (unsigned int i = 0; i < 4; ++i) {
    alpha[i] = 0;
    if (i < nLayers && (layerList[i]->props & 0x100)) {
      alpha[i] = layerList[i]->offsAlpha;
    }
  }

  UnpackAlphaShadowBits(reinterpret_cast<NTempest::CImVector *>(shaderTexture->pixels), shadowBits, alpha, shadowOffs);
}

void CMapChunk::UnpackAlphaShadowBits(
    NTempest::CImVector       *texels,
    unsigned long             *bits,
    const unsigned char *const *const alpha,
    const unsigned char        *shadow
) {
  unsigned int coordDelta = 1;
  if (CWorld::shadowMipLevel == 1) {
    coordDelta = 2;
  }

  const unsigned char *alphaBytes[4] = {alpha[0], alpha[1], alpha[2], alpha[3]};
  const unsigned char *shadowBytes = shadow;
  unsigned long       *pixels = reinterpret_cast<unsigned long *>(texels);
  unsigned int         dst = 0;

  for (unsigned int y = 0; y < 64; y += coordDelta) {
    for (unsigned int x = 0; x < 64; x += coordDelta) {
      unsigned int coord = y * 64 + x;
      unsigned int alphaBitMask = (coord & 1) ? 0xF0 : 0x0F;
      unsigned int alphaBitLShift = (coord & 1) ? 0 : 4;
      unsigned int channel1 = alphaBytes[1] ? (alphaBytes[1][coord >> 1] & alphaBitMask) << alphaBitLShift : 0xFF;
      unsigned int channel2 = alphaBytes[2] ? (alphaBytes[2][coord >> 1] & alphaBitMask) << alphaBitLShift : 0xFF;
      unsigned int channel3 = alphaBytes[3] ? (alphaBytes[3][coord >> 1] & alphaBitMask) << alphaBitLShift : 0xFF;
      unsigned int shadowValue = shadowBytes ? ((shadowBytes[coord >> 3] & (1u << (coord & 7))) ? 0xFF : 0) : 0xFF;
      pixels[dst++] = channel3 | (channel2 << 8) | (channel1 << 16) | (shadowValue << 24);
    }
  }

  unsigned int shadowCoord = 0;
  for (unsigned int bitsY = 0; bitsY < 32; ++bitsY) {
    for (unsigned int bitsX = 0; bitsX < 32; ++bitsX) {
      unsigned int sourceCoord = bitsY * 128 + bitsX * 2;
      if (shadowBytes && (shadowBytes[sourceCoord >> 3] & (1u << (sourceCoord & 7)))) {
        bits[shadowCoord >> 5] |= 1u << (shadowCoord & 31);
      }
      ++shadowCoord;
    }
  }
}

void CMapChunk::UnpackAlphaBits(unsigned long *pixels, const unsigned char *alphaPixels) {
  FATALASSERT(pixels);
  FATALASSERT(alphaPixels);

  const unsigned char *alpha = alphaPixels;
  if (CWorld::alphaMipLevel == 1) {
    unsigned int source = 0;
    unsigned int dest = 0;
    for (unsigned int y = 0; y < 32; ++y) {
      for (unsigned int x = 0; x < 32; ++x) {
        pixels[dest++] = 0x00FFFFFF | (alpha[source++] << 28);
      }
      source += 32;
    }
  } else {
    for (unsigned int i = 0; i < 4096; ++i) {
      unsigned int value = alpha[i >> 1];
      if (i & 1) {
        value &= 0xF0;
      }
      pixels[i] = 0x00FFFFFF | (value << ((i & 1) ? 24 : 28));
    }
  }
}

void CMapChunk::UnpackShadowBits(unsigned long *pixels, unsigned long *shadowBits, const unsigned char *shadow) {
  FATALASSERT(pixels);
  FATALASSERT(shadowBits);
  FATALASSERT(shadow);

  const unsigned char *shadowBytes = shadow;
  unsigned int         shadowIndex = 0;
  if (CWorld::shadowMipLevel == 1) {
    for (unsigned int y = 0; y < 32; ++y) {
      for (unsigned int x = 0; x < 32; ++x) {
        unsigned int source = y * 128 + x * 2;
        unsigned int set = shadowBytes[source >> 3] & (1u << (source & 7));
        pixels[shadowIndex] = set ? 0xFFFFFFFF : 0;
        if (set) {
          shadowBits[shadowIndex >> 5] |= 1u << (shadowIndex & 31);
        }
        ++shadowIndex;
      }
    }
  } else {
    for (unsigned int y = 0; y < 64; ++y) {
      for (unsigned int x = 0; x < 64; ++x) {
        unsigned int source = y * 64 + x;
        unsigned int set = shadowBytes[source >> 3] & (1u << (source & 7));
        pixels[source] = set ? 0xFFFFFFFF : 0;
        if (!(y & 1) && !(x & 1)) {
          if (set) {
            shadowBits[shadowIndex >> 5] |= 1u << (shadowIndex & 31);
          }
          ++shadowIndex;
        }
      }
    }
  }
}

void CMapChunk::UpdateLayerGxTexture(
    EGxTexCommand cmd,
    unsigned int  w,
    unsigned int  h,
    unsigned int  d,
    unsigned int  mipLevel,
    void         *userArg,
    unsigned int &texelStrideInBytes,
    const void  *&texels
) {
  CChunkLayer *layer = static_cast<CChunkLayer *>(userArg);
  FATALASSERT(layer);

  if (cmd == GxTex_Lock) {
    if (!layer->tex) {
      layer->chunk->SyncLoadLayer(layer);
    }
  } else if (cmd == GxTex_Latch) {
    texelStrideInBytes = 4 * w;
    texels = layer->tex->pixels;
  } else if (cmd == GxTex_Unlock) {
    CMap::FreeTex(layer->tex);
    layer->tex = 0;
  }
}

void CMapChunk::UpdateShadowGxTexture(
    EGxTexCommand cmd,
    unsigned int  w,
    unsigned int  h,
    unsigned int  d,
    unsigned int  mipLevel,
    void         *userArg,
    unsigned int &texelStrideInBytes,
    const void  *&texels
) {
  CMapChunk *chunk = static_cast<CMapChunk *>(userArg);
  FATALASSERT(chunk);

  if (cmd == GxTex_Lock) {
    if (!chunk->shadowTexture) {
      chunk->SyncLoadShadow();
    }
  } else if (cmd == GxTex_Latch) {
    texelStrideInBytes = 4 * w;
    texels = chunk->shadowTexture->pixels;
  } else if (cmd == GxTex_Unlock) {
    CMap::FreeTex(chunk->shadowTexture);
    chunk->shadowTexture = 0;
  }
}

void CMapChunk::UpdateShaderGxTexture(
    EGxTexCommand cmd,
    unsigned int  w,
    unsigned int  h,
    unsigned int  d,
    unsigned int  mipLevel,
    void         *userArg,
    unsigned int &texelStrideInBytes,
    const void  *&texels
) {
  CMapChunk *chunk = static_cast<CMapChunk *>(userArg);
  FATALASSERT(chunk);

  if (cmd == GxTex_Lock) {
    if (!chunk->shaderTexture) {
      chunk->SyncLoadShader();
    }
  } else if (cmd == GxTex_Latch) {
    texelStrideInBytes = 4 * w;
    texels = chunk->shaderTexture->pixels;
  } else if (cmd == GxTex_Unlock) {
    CMap::FreeTex(chunk->shaderTexture);
    chunk->shaderTexture = 0;
  }
}

void CMapChunk::UpdateTextureDefault(
    EGxTexCommand cmd,
    unsigned int  w,
    unsigned int  h,
    unsigned int  d,
    unsigned int  mipLevel,
    void         *userArg,
    unsigned int &texelStrideInBytes,
    const void  *&texels
) {
  if (userArg) {
    FATALERROR(("1"));
  }
}
