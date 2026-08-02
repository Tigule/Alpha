#include <WowConst.h>
#include <MapDefs.h>

#include "WorldClient/World.h"

#include "Base/Base.h"
#include "Gx/Gx.h"
#include "Object/ObjectClient/ZoneDebug.h"
#include "Services/Texture.h"
#include "WorldClient/DetailDoodad.h"

#include "DB/DBClient/AutoCode/GroundEffectTextureRec.h"

#include <string.h>

extern unsigned int g_holeMask[4][4];

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

static float s_tempTexSpeed[8] = {64.0f, 48.0f, 32.0f, 16.0f, 8.0f, 4.0f, 2.0f, 1.0f};

static int          s_neighborMask[4] = {0xFFFFFF00, 0xFFFF00FF, 0xFF00FFFF, 0x00FFFFFF};
static int          s_neighborShft[4] = {0, 8, 16, 24};
static unsigned int s_realPrimCnt[4] = {4, 16, 64, 256};

static const float OO_COORD_TO_SHADOW = 0.24f;
static const float DETAIL_VARY = 2.0833333f;

const float        CMapChunk::TERRAIN_SPEC_EXP = 20.0f;
NTempest::C4Vector CMapChunk::psLayerMask[4] = {
    NTempest::C4Vector(0.0f, 0.0f, 0.0f, 0.0f), NTempest::C4Vector(1.0f, 0.0f, 0.0f, 0.0f), NTempest::C4Vector(0.0f, 1.0f, 0.0f, 0.0f),
    NTempest::C4Vector(0.0f, 0.0f, 1.0f, 0.0f)
};

void CMapChunk::GxBufDynFillCallback(CGxBufCommand &cmd, CGxBuf *buf) {
  CMapChunk *mapChunk = static_cast<CMapChunk *>(buf->UserArg());
  ASSERT(mapChunk);
  mapChunk->FillGxBufDynVertex(cmd, buf);
  mapChunk->FillGxBufDynIndex(cmd, buf);
}

void CMapChunk::GxBufFillCallback(CGxBufCommand &cmd, CGxBuf *buf) {
  CMapChunk *mapChunk = static_cast<CMapChunk *>(buf->UserArg());
  FATALASSERT(mapChunk);
  mapChunk->FillGxBufVertex(cmd, buf);
  mapChunk->FillGxBufIndex(cmd, buf);
}

void CMapChunk::Render() {
  int neighborLOD = 0;

  if (CWorld::enables & CWorld::Enable_Lod) {
    for (unsigned int i = 0; i < 4; ++i) {
      if (neighbor[i] && neighbor[i]->lod > lod) {
        neighborLOD = (neighborLOD & s_neighborMask[i]) | (neighbor[i]->lod << s_neighborShft[i]);
      }
    }
  }

  if (neighborLOD || holes) {
    if (gxBuf) {
      FreeGxBuf(gxBuf);
      gxBuf = 0;
    }

    primPtr = primList;
    LodCreateTree(0, lod, neighborLOD, holes, 4, 4);
    gxBufDyn->UserArgSet(this);
    gxBufDyn->CountSet(145, primPtr - primList);
    remapLod = 0x80000000;
    RenderLayersDyn();
    return;
  }

  unsigned int indexCount = s_tPrimGroups[lod][0].nIndicies + s_tPrimGroups[lod][1].nIndicies;
  if (!gxBuf) {
    gxBuf = AllocGxBuf(indexCount);
    gxBuf->UserArgSet(this);
    remapLod = static_cast<unsigned int>(-1);
  }

  if (remapLod != lod) {
    gxBuf->CountSet(s_tPrimRemap[lod].nIndicies, indexCount);
    gxBuf->Invalidate(CGxBuf::S_INVALID_DISCARD, CGxBuf::S_INVALID_DISCARD);
    remapLod = lod;
  }

  RenderLayers();
}

void CMapChunk::FillGxBufVertex(const CGxBufCommand &cmd, CGxBuf *buf) {
  CGxVertexPN *vtxBase = 0;

  switch (cmd.vertex.op) {
    case GxBufOp_Nop:
      return;

    case GxBufOp_Fill:
      vtxBase = static_cast<CGxVertexPN *>(*cmd.vertex.mem[GxVM_Position]);
      break;

    case GxBufOp_Assign:
      vtxBase = static_cast<CGxVertexPN *>(GxAllocVertexMem(buf->VertexCount() * sizeof(CGxVertexPN)));
      *cmd.vertex.mem[GxVM_Position] = &vtxBase->p;
      *cmd.vertex.mem[GxVM_Normal] = &vtxBase->n;
      break;
  }

  FATALASSERT(vtxBase);

  for (unsigned int j = 0; j < s_tPrimRemap[lod].nIndicies; ++j) {
    vtxBase[s_tPrimRemap[lod].indicies[2 * j + 1]].p = vertexList[s_tPrimRemap[lod].indicies[2 * j]];
    vtxBase[s_tPrimRemap[lod].indicies[2 * j + 1]].n = normalList[s_tPrimRemap[lod].indicies[2 * j]];
  }
}

void CMapChunk::FillGxBufIndex(const CGxBufCommand &cmd, CGxBuf *buf) {
  unsigned short *indices = 0;

  switch (cmd.index.op) {
    case GxBufOp_Nop:
      return;

    case GxBufOp_Fill:
      indices = static_cast<unsigned short *>(*cmd.index.mem[GxVM_Indices]);
      break;

    case GxBufOp_Assign:
      indices = static_cast<unsigned short *>(GxAllocIndexMem(buf->IndexCount() * sizeof(unsigned short)));
      *cmd.index.mem[GxVM_Indices] = indices;
      break;
  }

  FATALASSERT(indices);

  memcpy(indices, s_tPrimGroups[lod][0].indicies, s_tPrimGroups[lod][0].nIndicies * sizeof(unsigned short));
  memcpy(indices + s_tPrimGroups[lod][0].nIndicies, s_tPrimGroups[lod][1].indicies, s_tPrimGroups[lod][1].nIndicies * sizeof(unsigned short));
}

void CMapChunk::FillGxBufDynVertex(const CGxBufCommand &cmd, CGxBuf *buf) {
  CGxVertexPN *vertices = 0;

  switch (cmd.vertex.op) {
    case GxBufOp_Nop:
      return;

    case GxBufOp_Fill:
      vertices = static_cast<CGxVertexPN *>(*cmd.vertex.mem[GxVM_Position]);
      break;

    case GxBufOp_Assign:
      vertices = static_cast<CGxVertexPN *>(GxAllocVertexMem(buf->VertexCount() * sizeof(CGxVertexPN)));
      *cmd.vertex.mem[GxVM_Position] = &vertices->p;
      *cmd.vertex.mem[GxVM_Normal] = &vertices->n;
      break;
  }

  ASSERT(vertices);
  for (unsigned int index = 0; index < 145; ++index) {
    vertices[index].p = vertexList[index];
    vertices[index].n = normalList[index];
  }
}

void CMapChunk::FillGxBufDynIndex(const CGxBufCommand &cmd, CGxBuf *buf) {
  unsigned short *indices = 0;

  switch (cmd.index.op) {
    case GxBufOp_Nop:
      return;

    case GxBufOp_Fill:
      indices = static_cast<unsigned short *>(*cmd.index.mem[GxVM_Indices]);
      break;

    case GxBufOp_Assign:
      indices = static_cast<unsigned short *>(GxAllocIndexMem(buf->IndexCount() * sizeof(unsigned short)));
      *cmd.index.mem[GxVM_Indices] = indices;
      break;
  }

  ASSERT(indices);
  memcpy(indices, primList, (primPtr - primList) * sizeof(unsigned short));
}

void CMapChunk::RenderLayers() {
  if (((CWorld::enables & CWorld::Enable_ZoneBounds) && !ZoneDebugIsInCurrentZone(aaSphere.c.x, aaSphere.c.y)) || !nLayers || camDist >= CWorld::farFog) {
    RenderLayersColor();
    return;
  }

  GxVertexShaderSelect(GxVS_PassThru);
  GxBufLock(gxBuf);
  GxRsSet(GxRs_Blend, GxBlend_Opaque);
  GxRsSet(GxRs_TexBlend0, GxTexBlend_Mod);

  const float         GEO_TO_TEX = -1.0f / vertexList[1].y;
  NTempest::C44Matrix amtx;
  amtx.Scale(-GEO_TO_TEX);
  float a0 = amtx.a0;
  float a1 = amtx.a1;
  float a2 = amtx.a2;
  float a3 = amtx.a3;
  amtx.a0 = amtx.b0;
  amtx.a1 = amtx.b1;
  amtx.a2 = amtx.b2;
  amtx.a3 = amtx.b3;
  amtx.b0 = a0;
  amtx.b1 = a1;
  amtx.b2 = a2;
  amtx.b3 = a3;

  NTempest::C3Vector texVect(CWorldScene::camPos.x - corner.x, CWorldScene::camPos.y - corner.y, CWorldScene::camPos.z - corner.z);
  amtx.Translate(texVect);

  NTempest::C44Matrix dmtx;
  dmtx.Scale(GEO_TO_TEX * -0.1220703125f);
  a0 = dmtx.a0;
  a1 = dmtx.a1;
  a2 = dmtx.a2;
  a3 = dmtx.a3;
  dmtx.a0 = dmtx.b0;
  dmtx.a1 = dmtx.b1;
  dmtx.a2 = dmtx.b2;
  dmtx.a3 = dmtx.b3;
  dmtx.b0 = a0;
  dmtx.b1 = a1;
  dmtx.b2 = a2;
  dmtx.b3 = a3;
  dmtx.Translate(texVect);

  GxXformPush(GxXform_Tex0, amtx);
  GxXformPush(GxXform_Tex1, dmtx);
  GxRsSet(GxRs_TexGen0, GxTexGen_World);
  GxRsSet(GxRs_TexGen1, GxTexGen_World);
  GxRsSet(GxRs_TextureShader0, GxTS_Affine);
  GxRsSet(GxRs_TextureShader1, GxTS_Affine);

  if (CMap::EnableSpecularTerrain()) {
    GxRsSet(GxRs_Texture1, shaderGxTexture);
    GxRsSet(GxRs_PixelShader, CMap::psSpecTerrain);
    GxRsSet(GxRs_MatSpecular, NTempest::CImVector(0xFFFFFFFF));
    GxRsSet(GxRs_MatSpecularExp, TERRAIN_SPEC_EXP);
  } else if (CMap::EnableTerrainShader()) {
    GxRsSet(GxRs_Texture1, shaderGxTexture);
    GxRsSet(GxRs_PixelShader, CMap::psTerrain);
  }

  unsigned int        nLayersTest = nLayers;
  NTempest::CImVector mattDiffuse(0xFFFFFFFF);
  if (camDist > CWorld::textureLodDist) {
    float fade = camDist - CWorld::textureLodDist;
    if (fade < 64.0f) {
      mattDiffuse.a = static_cast<unsigned char>(NTempest::CMath::fuint_n((64.0f - fade) * 0.015625f * 255.0f));
    } else {
      nLayersTest = 1;
    }
  }
  GxRsSet(GxRs_MatDiffuse, mattDiffuse);

  for (unsigned int i = 0; i < nLayersTest; ++i) {
    CChunkLayer *layer = layerList[i];
    CGxTex      *texture = TextureGetGxTex(layer->texId, 0, 0);
    if (!texture) {
      continue;
    }

    if (layer->props & 0x80) {
      GxRsSet(GxRs_Lighting, 0);
    }
    if (layer->props & 0x40) {
      float scale = 1.0f / GEO_TO_TEX / s_tempTexSpeed[(layer->props >> 3) & 7];
      texVect.x = CWorld::texVect[layer->props & 7].x * scale;
      texVect.y = CWorld::texVect[layer->props & 7].y * scale;
      texVect.z = CWorld::texVect[layer->props & 7].z * scale;
      GxXformPush(GxXform_Tex0, amtx);
      GxXformTranslate(GxXform_Tex0, texVect);
    }

    GxRsSet(GxRs_Texture0, texture);
    if (CMap::EnableSpecularTerrain()) {
      CMap::psSpecTerrain->SetParam(CMap::psSpecTerrain_LayerMask, psLayerMask[i]);
      GxRsSet(GxRs_Blend, i ? GxBlend_Alpha : GxBlend_Opaque);
    } else if (CMap::EnableTerrainShader()) {
      CMap::psTerrain->SetParam(CMap::psTerrain_LayerMask, psLayerMask[i]);
      GxRsSet(GxRs_Blend, i ? GxBlend_Alpha : GxBlend_Opaque);
    } else if (layer->gxTexture) {
      GxRsSet(GxRs_Blend, GxBlend_Alpha);
      GxRsSet(GxRs_TexBlend1, GxTexBlend_Mod);
      GxRsSet(GxRs_Texture1, layer->gxTexture);
    } else {
      GxRsSet(GxRs_Blend, GxBlend_Opaque);
      GxRsSet(GxRs_Texture1, static_cast<void *>(0));
    }

    GxBufRender(rmGxBatchList[lod], 2);

    if (layer->props & 0x80) {
      GxRsSet(GxRs_Lighting, 1);
    }
    if (layer->props & 0x40) {
      GxXformPop(GxXform_Tex0);
    }
  }

  if (CMap::EnableSpecularTerrain()) {
    GxRsSet(GxRs_PixelShader, static_cast<void *>(0));
    GxRsSet(GxRs_MatSpecular, NTempest::CImVector(0ul));
    GxRsSet(GxRs_MatSpecularExp, 0.0f);
  } else if (CMap::EnableTerrainShader()) {
    GxRsSet(GxRs_PixelShader, static_cast<void *>(0));
  } else if (shadowGxTexture && (CWorld::enables & CWorld::Enable_Shadow)) {
    GxRsSet(GxRs_MatDiffuse, CWorld::shadowColor);
    GxRsSet(GxRs_Blend, GxBlend_Alpha);
    GxRsSet(GxRs_Texture0, CWorld::shadowModGxTex);
    GxRsSet(GxRs_TexBlend1, GxTexBlend_Mod);
    GxRsSet(GxRs_Texture1, shadowGxTexture);
    GxBufRender(rmGxBatchList[lod], 2);
    GxRsSet(GxRs_MatDiffuse, NTempest::CImVector(0xFFFFFFFF));
  }

  GxRsSet(GxRs_Texture1, static_cast<void *>(0));
  GxBufUnlock();
  GxXformPop(GxXform_Tex0);
  GxXformPop(GxXform_Tex1);
  GxRsSet(GxRs_TextureShader0, GxTS_PassThru);
  GxRsSet(GxRs_TextureShader1, GxTS_PassThru);
  GxRsSet(GxRs_TexGen0, GxTexGen_Disable);
  GxRsSet(GxRs_TexGen1, GxTexGen_Disable);
}

void CMapChunk::RenderLayersDyn() {
  if (((CWorld::enables & CWorld::Enable_ZoneBounds) && !ZoneDebugIsInCurrentZone(aaSphere.c.x, aaSphere.c.y)) || !nLayers || camDist >= CWorld::farFog) {
    RenderLayersColorDyn();
    return;
  }

  CGxBatch gxBatch(GxPrim_Triangles, primPtr - primList, 0, -1, -1);
  GxVertexShaderSelect(GxVS_PassThru);
  GxBufLock(gxBufDyn);
  GxRsSet(GxRs_Blend, GxBlend_Opaque);
  GxRsSet(GxRs_TexBlend0, GxTexBlend_Mod);

  const float         GEO_TO_TEX = -1.0f / vertexList[1].y;
  NTempest::C44Matrix amtx;
  amtx.Scale(-GEO_TO_TEX);
  float a0 = amtx.a0;
  float a1 = amtx.a1;
  float a2 = amtx.a2;
  float a3 = amtx.a3;
  amtx.a0 = amtx.b0;
  amtx.a1 = amtx.b1;
  amtx.a2 = amtx.b2;
  amtx.a3 = amtx.b3;
  amtx.b0 = a0;
  amtx.b1 = a1;
  amtx.b2 = a2;
  amtx.b3 = a3;
  NTempest::C3Vector texVect(CWorldScene::camPos.x - corner.x, CWorldScene::camPos.y - corner.y, CWorldScene::camPos.z - corner.z);
  amtx.Translate(texVect);

  NTempest::C44Matrix dmtx;
  dmtx.Scale(GEO_TO_TEX * -0.1220703125f);
  a0 = dmtx.a0;
  a1 = dmtx.a1;
  a2 = dmtx.a2;
  a3 = dmtx.a3;
  dmtx.a0 = dmtx.b0;
  dmtx.a1 = dmtx.b1;
  dmtx.a2 = dmtx.b2;
  dmtx.a3 = dmtx.b3;
  dmtx.b0 = a0;
  dmtx.b1 = a1;
  dmtx.b2 = a2;
  dmtx.b3 = a3;
  dmtx.Translate(texVect);

  GxXformPush(GxXform_Tex0, amtx);
  GxXformPush(GxXform_Tex1, dmtx);
  GxRsSet(GxRs_TexGen0, GxTexGen_World);
  GxRsSet(GxRs_TexGen1, GxTexGen_World);
  GxRsSet(GxRs_TextureShader0, GxTS_Affine);
  GxRsSet(GxRs_TextureShader1, GxTS_Affine);

  if (CMap::EnableSpecularTerrain()) {
    GxRsSet(GxRs_Texture1, shaderGxTexture);
    GxRsSet(GxRs_PixelShader, CMap::psSpecTerrain);
    GxRsSet(GxRs_MatSpecular, NTempest::CImVector(0xFFFFFFFF));
    GxRsSet(GxRs_MatSpecularExp, TERRAIN_SPEC_EXP);
  } else if (CMap::EnableTerrainShader()) {
    GxRsSet(GxRs_Texture1, shaderGxTexture);
    GxRsSet(GxRs_PixelShader, CMap::psTerrain);
  }

  unsigned int        nLayersTest = nLayers;
  NTempest::CImVector mattDiffuse(0xFFFFFFFF);
  if (camDist > CWorld::textureLodDist) {
    float fade = camDist - CWorld::textureLodDist;
    if (fade < 64.0f) {
      mattDiffuse.a = static_cast<unsigned char>(NTempest::CMath::fuint_n((64.0f - fade) * 0.015625f * 255.0f));
    } else {
      nLayersTest = 1;
    }
  }
  GxRsSet(GxRs_MatDiffuse, mattDiffuse);

  for (unsigned int i = 0; i < nLayersTest; ++i) {
    CChunkLayer *layer = layerList[i];
    CGxTex      *texture = TextureGetGxTex(layer->texId, 0, 0);
    if (!texture) {
      continue;
    }
    if (layer->props & 0x80) {
      GxRsSet(GxRs_Lighting, 0);
    }
    if (layer->props & 0x40) {
      float scale = 1.0f / GEO_TO_TEX / s_tempTexSpeed[(layer->props >> 3) & 7];
      texVect.x = CWorld::texVect[layer->props & 7].x * scale;
      texVect.y = CWorld::texVect[layer->props & 7].y * scale;
      texVect.z = CWorld::texVect[layer->props & 7].z * scale;
      GxXformPush(GxXform_Tex0, amtx);
      GxXformTranslate(GxXform_Tex0, texVect);
    }
    GxRsSet(GxRs_Texture0, texture);
    if (CMap::EnableSpecularTerrain()) {
      CMap::psSpecTerrain->SetParam(CMap::psSpecTerrain_LayerMask, psLayerMask[i]);
      GxRsSet(GxRs_Blend, i ? GxBlend_Alpha : GxBlend_Opaque);
    } else if (CMap::EnableTerrainShader()) {
      CMap::psTerrain->SetParam(CMap::psTerrain_LayerMask, psLayerMask[i]);
      GxRsSet(GxRs_Blend, i ? GxBlend_Alpha : GxBlend_Opaque);
    } else if (layer->gxTexture) {
      GxRsSet(GxRs_Blend, GxBlend_Alpha);
      GxRsSet(GxRs_TexBlend1, GxTexBlend_Mod);
      GxRsSet(GxRs_Texture1, layer->gxTexture);
    } else {
      GxRsSet(GxRs_Blend, GxBlend_Opaque);
      GxRsSet(GxRs_Texture1, static_cast<void *>(0));
    }
    GxBufRender(gxBatch);
    if (layer->props & 0x80) {
      GxRsSet(GxRs_Lighting, 1);
    }
    if (layer->props & 0x40) {
      GxXformPop(GxXform_Tex0);
    }
  }

  if (CMap::EnableSpecularTerrain()) {
    GxRsSet(GxRs_PixelShader, static_cast<void *>(0));
    GxRsSet(GxRs_MatSpecular, NTempest::CImVector(0ul));
    GxRsSet(GxRs_MatSpecularExp, 0.0f);
  } else if (CMap::EnableTerrainShader()) {
    GxRsSet(GxRs_PixelShader, static_cast<void *>(0));
  } else if (shadowGxTexture && (CWorld::enables & CWorld::Enable_Shadow)) {
    GxRsSet(GxRs_MatDiffuse, CWorld::shadowColor);
    GxRsSet(GxRs_Blend, GxBlend_Alpha);
    GxRsSet(GxRs_Texture0, CWorld::shadowModGxTex);
    GxRsSet(GxRs_TexBlend1, GxTexBlend_Mod);
    GxRsSet(GxRs_Texture1, shadowGxTexture);
    GxBufRender(gxBatch);
    GxRsSet(GxRs_MatDiffuse, NTempest::CImVector(0xFFFFFFFF));
  }

  GxRsSet(GxRs_Texture1, static_cast<void *>(0));
  GxBufUnlock();
  GxXformPop(GxXform_Tex0);
  GxXformPop(GxXform_Tex1);
  GxRsSet(GxRs_TextureShader0, GxTS_PassThru);
  GxRsSet(GxRs_TextureShader1, GxTS_PassThru);
  GxRsSet(GxRs_TexGen0, GxTexGen_Disable);
  GxRsSet(GxRs_TexGen1, GxTexGen_Disable);
}

void CMapChunk::RenderLayersColor() {
  GxVertexShaderSelect(GxVS_PassThru);
  GxRsSet(GxRs_MatDiffuse, NTempest::CImVector(0xFFFFFFFF));
  GxRsSet(GxRs_Blend, GxBlend_Opaque);
  GxRsSet(GxRs_Texture0, static_cast<void *>(0));
  GxRsSet(GxRs_Texture1, static_cast<void *>(0));
  GxBufLock(gxBuf);
  GxBufRender(rmGxBatchList[lod], 2);
  GxBufUnlock();
}

void CMapChunk::RenderLayersColorDyn() {
  CGxBatch gxBatch(GxPrim_Triangles, primPtr - primList, 0, -1, -1);
  GxVertexShaderSelect(GxVS_PassThru);
  GxRsSet(GxRs_MatDiffuse, NTempest::CImVector(0xFFFFFFFF));
  GxRsSet(GxRs_Blend, GxBlend_Opaque);
  GxRsSet(GxRs_Texture0, static_cast<void *>(0));
  GxRsSet(GxRs_Texture1, static_cast<void *>(0));
  GxBufLock(gxBufDyn);
  GxBufRender(gxBatch);
  GxBufUnlock();
}

void CMapChunk::CreateDetailDoodads() {
  NTempest::C2iVector splatList[128];
  unsigned int        i;

  if (!nLayers) {
    return;
  }

  detailDoodadInst = CDetailDoodad::AllocInst();
  FATALASSERT(detailDoodadInst);

  unsigned int n = CWorld::detailDoodadTest ? 64 : CWorld::detailDoodadDensity;

  for (i = 0; i < n; ++i) {
    if (CWorld::detailDoodadTest) {
      splatList[i].x = i & 7;
      splatList[i].y = i >> 3;
    } else {
      splatList[i].x = NTempest::CRandom::uint32_(rSeed) & 7;
      splatList[i].y = NTempest::CRandom::uint32_(rSeed) & 7;
    }
  }

  const float smolTileSize = 150.0f / 36.0f;
  for (i = 0; i < n; ++i) {
    NTempest::C2iVector splat = splatList[i];
    unsigned int        x = splat.x;
    unsigned int        y = splat.y;
    unsigned int        noEffect = reinterpret_cast<unsigned char *>(noEffectDoodad)[y];
    if ((noEffect & (1 << x)) || (holes & g_holeMask[y >> 1][x >> 1])) {
      continue;
    }

    unsigned int layerIndex = (predTex[y] >> (2 * x)) & 3;
    if (!layerList[layerIndex]) {
      continue;
    }
    unsigned int            effectId = layerList[layerIndex]->effectId;
    const GroundEffectTextureRec *effectTex = g_groundEffectTextureDB.GetRecordByIndex(effectId);
    if (!effectTex) {
      continue;
    }

    unsigned long clumpDensity = effectTex->m_density;
    if (!clumpDensity) {
      clumpDensity = 8;
    }

    for (unsigned int d = 0; d < clumpDensity; ++d) {
      int doodadId = effectTex->m_doodadId[(i + d) & 3];
      if (doodadId == -1) {
        continue;
      }

      float         fx = (NTempest::CRandom::reals_(rSeed) + 1.0f) * (smolTileSize * 0.5f);
      float         fy = (NTempest::CRandom::reals_(rSeed) + 1.0f) * (smolTileSize * 0.5f);
      float         sx = (fx + x * smolTileSize) * DETAIL_VARY;
      float         sy = (fy + y * smolTileSize) * DETAIL_VARY;
      int           shadowX = static_cast<int>(sx - 0.5f);
      int           shadowY = static_cast<int>(sy - 0.5f);
      unsigned long flags = shadowX >= 0 && shadowX < 32 && shadowY >= 0 && shadowY < 32 && (shadowBits[shadowY] & (1UL << shadowX)) ? 1 : 0;

      NTempest::C3Vector cPos(-fy - y * smolTileSize, -fx - x * smolTileSize, 0.0f);
      int                triangle = cPos.y - cPos.x < 0.0f;
      if (-cPos.y - smolTileSize - cPos.x > 0.0f) {
        triangle += 2;
      }

      NTempest::C4Plane plane = planeList[4 * (x + 8 * y) + triangle];
      cPos.z = -(plane.n.x * cPos.x + plane.n.y * cPos.y + plane.d) / plane.n.z;
      detailDoodadInst->AddDoodad(doodadId, cPos, flags, plane);
    }
  }
}
