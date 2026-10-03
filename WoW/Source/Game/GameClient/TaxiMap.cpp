#include <Base/Base.h>
#include <Gx/Gx.h>
#include <MapDefs.h>
#include <WorldClient/World.h>
#include <WowConst.h>
#include <DayNight.h>

#include "TaxiMap.h"

#include "DB/DBClient/AutoCode/TaxiPathRec.h"
#include "DB/DBClient/AutoCode/TaxiNodesRec.h"

#include <Base/Handle.h>
#include <Base/Status.h>
#include <Services/SysMessage.h>
#include <Services/Texture.h>
#include <Tempest/cimvector.h>
#include <Tempest/crect.h>
#include <stpl.h>
#include <storm.h>

#include <malloc.h>

static HTEXTURE                  s_texture;
static C4Pixel                   s_textureData[512 * 512];
static const TaxiPathRec        *s_taxiPathCosts[63][63];
static int                       s_continent = -1;
static NTempest::CRect           s_taxiTextureRect;
static NTempest::CRect           s_visibleWorldRect;
static int                       s_currentTaxiNode;
static LONGLONG                  s_currentReachable;
static LONGLONG                  s_knownNodes;
static HTEXTURE                  s_solidColor;
static TSGrowableArray<TAXILINE> s_lines;

#include <Model/ModelInternal.h>

static void FixupRegionRect(NTempest::CRect &rect) {
  float xSlide = 0.0f;
  float ySlide = 0.0f;
  if (rect.r > 17066.666f) {
    xSlide = -(rect.r - 17066.666f);
  } else if (rect.l < -17066.666f) {
    xSlide = -17066.666f - rect.l;
  }
  if (rect.b > 17066.666f) {
    ySlide = -(rect.b - 17066.666f);
  } else if (rect.t < -17066.666f) {
    ySlide = -17066.666f - rect.t;
  }
  rect.r += xSlide;
  rect.l += xSlide;
  rect.b += ySlide;
  rect.t += ySlide;
}

static void TextureUpdateFunc(EGxTexCommand cmd, UINT w, UINT h, UINT d, UINT mipLevel, LPVOID userArg, UINT &texelStrideInBytes, LPCVOID &texels) {
  if (cmd == GxTex_Latch) {
    texelStrideInBytes = 4 * w;
    texels = s_textureData;
  }
}

static void UglifyMapTexture() {
  UINT index;

  for (index = 0; index < 512 * 512; ++index) {
    s_textureData[index] = C4Pixel(0xFF, 0x00, 0xFF, 0xFF);
  }
}

static bool UpdateTexture(int continentID) {
  if (continentID == s_continent) {
    return true;
  }

  char fileName[MAX_PATH];
  SStrPrintf(fileName, sizeof(fileName), "Textures\\TaxiMaps\\TaxiMap%02d", continentID);
  s_continent = continentID;
  if (!fileName[0]) {
    return false;
  }

  UINT     width;
  UINT     height;
  UINT     format;
  BOOL     isOpaque;
  CStatus  status;
  MipBits *bits = TextureLoadImage(fileName, &width, &height, &format, &isOpaque, &status, 0);
  if (!bits) {
    return false;
  }
  if (width != 512 || height != 512) {
    SysMsgPrintf(SYSMSG_ERROR, 4, "TAXIMAPFILEWRONGSIZE|%s|%d|%d|%d|%d", fileName, 512, 512, width, height);
    return false;
  }

  C4Pixel *src = bits->mip[0];
  C4Pixel *dst = s_textureData;
  for (UINT y = 0; y < 512; ++y) {
    for (UINT x = 0; x < 512; ++x) {
      dst[x] = src[x];
    }
    src += 512;
    dst += 512;
  }
  CGxTex *tex = TextureGetGxTex(s_texture, 1, 0);
  if (tex) {
    GxTexUpdate(tex, 0, 0, 511, 511, 1);
  }
  TextureUnloadImage(bits);
  return true;
}

static NTempest::C2Vector CalculateNormalizedCoords(const NTempest::C2Vector &vec) {
  NTempest::C2Vector v = vec;
  v = NTempest::C2Vector(s_visibleWorldRect.b - v.y, v.x - s_visibleWorldRect.l);
  v.x /= s_visibleWorldRect.b - s_visibleWorldRect.t;
  v.y /= s_visibleWorldRect.r - s_visibleWorldRect.l;
  return v;
}

static void GenerateRouteInfo(LONGLONG allNodes, int currentContinent) {
  int i;
  int j;

  if (!(allNodes & (allNodes - 1))) {
    return;
  }

  for (i = 0; i < 64U; ++i) {
    LONGLONG mask = static_cast<LONGLONG>(1) << i;
    if (allNodes & mask) {
      const TaxiNodesRec *node = g_taxiNodesDB.GetRecord(i + 1);
      if (!node || node->m_ContinentID != currentContinent) {
        allNodes &= ~mask;
      }
    }
  }

  char grid[64][64];
  memset(grid, 0, sizeof(grid));
  for (i = g_taxiPathDB.GetNumRecords(); i--;) {
    const TaxiPathRec *path = g_taxiPathDB.GetRecordByIndex(i);
    LONGLONG           mask = (static_cast<LONGLONG>(1) << (path->m_FromTaxiNode - 1)) | (static_cast<LONGLONG>(1) << (path->m_ToTaxiNode - 1));
    if ((allNodes & mask) == mask) {
      int src = min(path->m_FromTaxiNode, path->m_ToTaxiNode);
      int dst = max(path->m_FromTaxiNode, path->m_ToTaxiNode);
      grid[src][dst] = 1;
    }
  }

  s_lines.SetCount(0);
  for (i = 0; i < 64U; ++i) {
    for (j = 0; j < 64U; ++j) {
      if (grid[i][j]) {
        const TaxiNodesRec *src = g_taxiNodesDB.GetRecord(i);
        const TaxiNodesRec *dst = g_taxiNodesDB.GetRecord(j);
        FATALASSERT(src && dst);
        TAXILINE *line = s_lines.New();
        line->src = NTempest::C2Vector(src->m_X, src->m_Y);
        line->dst = NTempest::C2Vector(dst->m_X, dst->m_Y);
      }
    }
  }
}

void TaxiMapInitialize() {
  if (s_texture) {
    HandleClose(s_texture);
  }
  s_texture = 0;

  CGxTexParmsEx params;
  params.target = GxTex_2d;
  params.width = 512;
  params.height = 512;
  params.depth = 0;
  params.dataFormat = GxTex_Argb8888;
  params.format = GxTex_Rgb565;
  params.flags = CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1);
  params.userArg = 0;
  params.userFunc = TextureUpdateFunc;

  CGxTex *tex;
  if (GxTexCreate(params, tex) && tex) {
    int index;

    s_texture = TextureCreate(tex);
    memset(s_taxiPathCosts, 0, sizeof(s_taxiPathCosts));

    for (index = g_taxiPathDB.GetNumRecords(); index--;) {
      const TaxiPathRec *rec = g_taxiPathDB.GetRecordByIndex(index);

      ASSERT(rec);
      if (rec->m_FromTaxiNode && rec->m_ToTaxiNode) {
        if (rec->m_FromTaxiNode <= 63 && rec->m_ToTaxiNode <= 63) {
          s_taxiPathCosts[rec->m_FromTaxiNode - 1][rec->m_ToTaxiNode - 1] = rec;
        }
      }
    }

    s_taxiTextureRect.l = 0.0f;
    s_taxiTextureRect.r = 1.0f;
    s_taxiTextureRect.t = 0.0f;
    s_taxiTextureRect.b = 1.0f;
    UglifyMapTexture();

    CStatus status;
    s_solidColor = TextureCreateSolid(NTempest::CImVector(0x00404040UL), &status);
  }
}

void TaxiMapShutdown() {
  if (s_texture) {
    HandleClose(s_texture);
  }
  s_texture = 0;

  s_continent = -1;
  s_currentTaxiNode = 0;
  s_currentReachable = 0;
  s_knownNodes = 0;
  s_lines.Clear();

  if (s_solidColor) {
    HandleClose(s_solidColor);
  }
  s_solidColor = 0;
}

HTEXTURE TaxiMapGetTexture() {
  return s_texture;
}

BOOL TaxiMapUpdatePosition(int currentTaxiNode, LONGLONG reachable, LONGLONG known, NTempest::CRect &rect) {
  const TaxiNodesRec *currentNode = g_taxiNodesDB.GetRecord(currentTaxiNode);
  if (!currentNode || !UpdateTexture(currentNode->m_ContinentID)) {
    UglifyMapTexture();
    s_taxiTextureRect.l = 0.0f;
    s_taxiTextureRect.r = 1.0f;
    s_taxiTextureRect.t = 0.0f;
    s_taxiTextureRect.b = 1.0f;
    return 0;
  }

  s_currentTaxiNode = currentTaxiNode;
  s_currentReachable = reachable & known;
  s_knownNodes = known;

  NTempest::CRect regionRect;
  regionRect.l = currentNode->m_X - 8533.333f;
  regionRect.r = currentNode->m_X + 8533.333f;
  regionRect.t = currentNode->m_Y - 8533.333f;
  regionRect.b = currentNode->m_Y + 8533.333f;
  FixupRegionRect(regionRect);
  rect = regionRect;
  s_visibleWorldRect = regionRect;

  regionRect.t *= -1.0f;
  regionRect.l *= -1.0f;
  regionRect.b *= -1.0f;
  regionRect.r *= -1.0f;
  s_taxiTextureRect.r = regionRect.t + 17066.666f;
  s_taxiTextureRect.l = regionRect.b + 17066.666f;
  s_taxiTextureRect.b = regionRect.l + 17066.666f;
  s_taxiTextureRect.t = regionRect.r + 17066.666f;
  s_taxiTextureRect.r *= 0.000029296876f;
  s_taxiTextureRect.l *= 0.000029296876f;
  s_taxiTextureRect.b *= 0.000029296876f;
  s_taxiTextureRect.t *= 0.000029296876f;
  GenerateRouteInfo(known, currentNode->m_ContinentID);
  return 1;
}

UINT TaxiNodeCost(UINT srcNode, UINT dstNode) {
  if (!srcNode || !dstNode || srcNode > 63 || dstNode > 63 || !s_taxiPathCosts[srcNode - 1][dstNode - 1]) {
    return 0;
  }
  return s_taxiPathCosts[srcNode - 1][dstNode - 1]->m_Cost;
}

NTempest::CRect TaxiMapGetRect() {
  return s_taxiTextureRect;
}

TAXNODE_TYPE TaxiNodeGetNodeType(int nodeID) {
  const TaxiNodesRec *node = g_taxiNodesDB.GetRecord(nodeID);
  if (node && s_currentTaxiNode == node->m_ID) {
    return TAXINODE_CURRENT;
  }

  LONGLONG mask = static_cast<LONGLONG>(1) << (nodeID - 1);
  if (s_currentReachable & mask) {
    return TAXINODE_REACHABLE;
  }

  const TaxiNodesRec *current = g_taxiNodesDB.GetRecord(s_currentTaxiNode);
  if (current && (s_knownNodes & mask) && current->m_ContinentID == node->m_ContinentID) {
    return TAXINODE_DISTANT;
  }

  return TAXINODE_NONE;
}

HMODEL TaxiGetRouteModel(float width, float height) {
  int lines = s_lines.Count();
  if (!lines) {
    return 0;
  }

  NTempest::C33Matrix mat;
  mat.Translate(-NTempest::C2Vector(width * 0.5f, height * 0.5f));
  mat.Scale(NTempest::C2Vector(width, height));

  int i;
  for (i = 0; i < lines; ++i) {
    s_lines[i].src = mat * NTempest::C3Vector(CalculateNormalizedCoords(s_lines[i].src));
    s_lines[i].dst = mat * NTempest::C3Vector(CalculateNormalizedCoords(s_lines[i].dst));
  }

  int                                numVertices = lines * 2;
  TSStackArray<NTempest::C3Vector>   verts(_alloca(numVertices * sizeof(NTempest::C3Vector)), numVertices, numVertices);
  TSStackArray<NTempest::C3Vector>   normals(_alloca(numVertices * sizeof(NTempest::C3Vector)), numVertices, numVertices);
  TSStackArray<NTempest::C2Vector>   texCoords(_alloca(numVertices * sizeof(NTempest::C2Vector)), numVertices, numVertices);
  TSStackArray<WORD>                 primVerts(_alloca(numVertices * sizeof(WORD)), numVertices, numVertices);
  NTempest::C3Vector                 normal(0.0f, 0.0f, 1.0f);
  NTempest::C2Vector                 top(0.0f, 0.0f);
  NTempest::C2Vector                 bot(1.0f, 1.0f);

  for (i = 0; i < lines; ++i) {
    verts[i * 2] = NTempest::C3Vector(s_lines[i].src);
    verts[i * 2 + 1] = NTempest::C3Vector(s_lines[i].dst);

    normals[i * 2] = normal;
    normals[i * 2 + 1] = normal;

    texCoords[i * 2] = top;
    texCoords[i * 2 + 1] = bot;
  }

  for (WORD index = 0; index < static_cast<WORD>(numVertices); ++index) {
    primVerts[index] = index;
  }

  return ModelCreateSimpleMesh(
      "TaxiRouteMap", numVertices, verts.Ptr(), normals.Ptr(), texCoords.Ptr(), GxPrim_Lines, primVerts.Ptr(), numVertices, s_solidColor, GxBlend_Opaque, 0x21,
      NTempest::CImVector(0xFF, 0xFF, 0xFF, 0xFF), 0
  );
}

bool TaxiRouteExists(int fromNode, int toNode) {
  return fromNode && toNode && fromNode <= 63 && toNode <= 63 && s_taxiPathCosts[fromNode - 1][toNode - 1];
}
