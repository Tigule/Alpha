#include "TaxiMap.h"

#include "DB/DBClient/AutoCode/TaxiPathRec.h"
#include "DB/DBClient/AutoCode/TaxiNodesRec.h"

#include <Base/Handle.h>
#include <Base/Status.h>
#include <Services/SysMessage.h>
#include <Services/Texture.h>
#include <Model/ModelInternal.h>
#include <Tempest/cimvector.h>
#include <Tempest/crect.h>
#include <stpl.h>
#include <storm.h>

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
  rect.l += xSlide;
  rect.r += xSlide;
  rect.t += ySlide;
  rect.b += ySlide;
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
    s_textureData[index] = C4Pixel(0xFFFF00FF);
  }
}

static bool UpdateTexture(int continentID) {
  if (continentID == s_continent) {
    return true;
  }

  char fileName[260];
  SStrPrintf(fileName, sizeof(fileName), "Textures\\TaxiMaps\\TaxiMap%02d", continentID);
  s_continent = continentID;
  if (!fileName[0]) {
    return false;
  }

  UINT     width;
  UINT     height;
  UINT     format;
  int      isOpaque;
  CStatus  status;
  MipBits *bits = TextureLoadImage(fileName, &width, &height, &format, &isOpaque, &status, 0);
  if (!bits) {
    return false;
  }
  if (width != 512 || height != 512) {
    SysMsgPrintf(SYSMSG_ERROR, 4, "TAXIMAPFILEWRONGSIZE|%s|%d|%d|%d|%d", fileName, 512, 512, width, height);
    return false;
  }

  for (UINT y = 0; y < 512; ++y) {
    C4Pixel *src = &bits->mip[0][y * 512];
    C4Pixel *dst = &s_textureData[y * 512];
    for (UINT x = 0; x < 512; ++x) {
      dst[x] = src[x];
    }
  }
  CGxTex *tex = TextureGetGxTex(s_texture, 1, 0);
  if (tex) {
    GxTexUpdate(tex, 0, 0, 511, 511, 1);
  }
  TextureUnloadImage(bits);
  return true;
}

static NTempest::C2Vector CalculateNormalizedCoords(const NTempest::C2Vector &vec) {
  NTempest::C2Vector v;
  v.x = (vec.x - s_visibleWorldRect.l) / s_visibleWorldRect.Width();
  v.y = (s_visibleWorldRect.b - vec.y) / s_visibleWorldRect.Height();
  return v;
}

static void GenerateRouteInfo(LONGLONG allNodes, int currentContinent) {
  int i;
  int j;

  if (!(allNodes & (allNodes - 1))) {
    return;
  }

  for (i = 0; i < 64; ++i) {
    LONGLONG            mask = static_cast<LONGLONG>(1) << i;
    const TaxiNodesRec *node = g_taxiNodesDB.GetRecord(i + 1);
    if ((allNodes & mask) && (!node || node->m_ContinentID != currentContinent)) {
      allNodes &= ~mask;
    }
  }

  char grid[64][64];
  memset(grid, 0, sizeof(grid));
  for (i = g_taxiPathDB.GetNumRecords() - 1; i >= 0; --i) {
    const TaxiPathRec *path = g_taxiPathDB.GetRecordByIndex(i);
    LONGLONG           srcMask = static_cast<LONGLONG>(1) << (path->m_FromTaxiNode - 1);
    LONGLONG           dstMask = static_cast<LONGLONG>(1) << (path->m_ToTaxiNode - 1);
    if ((allNodes & (srcMask | dstMask)) == (srcMask | dstMask)) {
      int src = path->m_FromTaxiNode - 1;
      int dst = path->m_ToTaxiNode - 1;
      if (src > dst) {
        int temp = src;
        src = dst;
        dst = temp;
      }
      grid[src][dst] = 1;
    }
  }

  s_lines.Clear();
  for (i = 0; i < 64; ++i) {
    for (j = 0; j < 64; ++j) {
      if (!grid[i][j]) {
        continue;
      }
      const TaxiNodesRec *src = g_taxiNodesDB.GetRecord(i + 1);
      const TaxiNodesRec *dst = g_taxiNodesDB.GetRecord(j + 1);
      FATALASSERT(src && dst);
      TAXILINE          *line = s_lines.New();
      NTempest::C2Vector srcPos(src->m_X, src->m_Y);
      NTempest::C2Vector dstPos(dst->m_X, dst->m_Y);
      line->src = CalculateNormalizedCoords(srcPos);
      line->dst = CalculateNormalizedCoords(dstPos);
    }
  }
}

void TaxiMapInitialize() {
  CGxTexParmsEx params;
  CGxTex       *tex;

  if (s_texture) {
    HandleClose(s_texture);
  }
  s_texture = 0;

  params.target = GxTex_2d;
  params.width = 512;
  params.height = 512;
  params.depth = 0;
  params.dataFormat = GxTex_Argb8888;
  params.format = GxTex_Rgb565;
  params.flags = CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1);
  params.userArg = 0;
  params.userFunc = TextureUpdateFunc;

  if (GxTexCreate(params, tex) && tex) {
    int index;

    s_texture = TextureCreate(tex);
    memset(s_taxiPathCosts, 0, sizeof(s_taxiPathCosts));

    for (index = g_taxiPathDB.GetNumRecords() - 1; index >= 0; --index) {
      const TaxiPathRec *rec = g_taxiPathDB.GetRecordByIndex(index);

      ASSERT(rec);
      if (rec->m_FromTaxiNode && rec->m_ToTaxiNode && rec->m_FromTaxiNode <= 63 && rec->m_ToTaxiNode <= 63) {
        s_taxiPathCosts[rec->m_FromTaxiNode][rec->m_ToTaxiNode] = rec;
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

int TaxiMapUpdatePosition(int currentTaxiNode, LONGLONG reachable, LONGLONG known, NTempest::CRect &rect) {
  const TaxiNodesRec *currentNode = g_taxiNodesDB.GetRecord(currentTaxiNode);
  if (currentTaxiNode >= 0 && currentNode && UpdateTexture(currentNode->m_ContinentID)) {
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

    s_taxiTextureRect.t = (-regionRect.r + 17066.666f) * 0.000029296876f;
    s_taxiTextureRect.l = (-regionRect.b + 17066.666f) * 0.000029296876f;
    s_taxiTextureRect.b = (-regionRect.l + 17066.666f) * 0.000029296876f;
    s_taxiTextureRect.r = (-regionRect.t + 17066.666f) * 0.000029296876f;
    GenerateRouteInfo(known, currentNode->m_ContinentID);
    return 1;
  }

  UglifyMapTexture();
  s_taxiTextureRect.Set(0.0f, 0.0f, 1.0f, 1.0f);
  return 0;
}

UINT TaxiNodeCost(UINT srcNode, UINT dstNode) {
  if (srcNode && dstNode && srcNode <= 63 && dstNode <= 63) {
    const TaxiPathRec *path = s_taxiPathCosts[srcNode][dstNode];
    if (path) {
      return path->m_Cost;
    }
  }
  return 0;
}

NTempest::CRect TaxiMapGetRect() {
  return s_taxiTextureRect;
}

TAXNODE_TYPE TaxiNodeGetNodeType(int nodeID) {
  if (nodeID == s_currentTaxiNode) {
    return TAXINODE_CURRENT;
  }

  if (nodeID <= 0 || nodeID > 64) {
    return TAXINODE_NONE;
  }

  LONGLONG mask = static_cast<LONGLONG>(1) << (nodeID - 1);
  if (s_currentReachable & mask) {
    return TAXINODE_REACHABLE;
  }

  const TaxiNodesRec *node = g_taxiNodesDB.GetRecord(nodeID);
  const TaxiNodesRec *current = g_taxiNodesDB.GetRecord(s_currentTaxiNode);
  if ((s_knownNodes & mask) && node && current && node->m_ContinentID == current->m_ContinentID) {
    return TAXINODE_DISTANT;
  }

  return TAXINODE_NONE;
}

HMODEL TaxiGetRouteModel(float width, float height) {
  UINT lines = s_lines.Count();
  if (!lines) {
    return 0;
  }

  TSGrowableArray<NTempest::C3Vector> verts;
  TSGrowableArray<NTempest::C3Vector> normals;
  TSGrowableArray<NTempest::C2Vector> texCoords;
  TSGrowableArray<WORD>               primVerts;
  verts.SetCount(lines * 4);
  normals.SetCount(lines * 4);
  texCoords.SetCount(lines * 4);
  primVerts.SetCount(lines * 6);

  for (UINT i = 0; i < lines; ++i) {
    NTempest::C2Vector bot(s_lines[i].src.x * width, s_lines[i].src.y * height);
    NTempest::C2Vector top(s_lines[i].dst.x * width, s_lines[i].dst.y * height);
    float              dx = top.x - bot.x;
    float              dy = top.y - bot.y;
    float              mag = static_cast<float>(sqrt(dx * dx + dy * dy));
    if (mag == 0.0f) {
      mag = 1.0f;
    }
    float x = -dy / mag;
    float y = dx / mag;
    UINT  vertex = i * 4;
    verts[vertex].Set(bot.x + x, bot.y + y, 0.0f);
    verts[vertex + 1].Set(bot.x - x, bot.y - y, 0.0f);
    verts[vertex + 2].Set(top.x + x, top.y + y, 0.0f);
    verts[vertex + 3].Set(top.x - x, top.y - y, 0.0f);
    for (UINT j = 0; j < 4; ++j) {
      normals[vertex + j].Set(0.0f, 0.0f, 1.0f);
    }
    texCoords[vertex] = NTempest::C2Vector(0.0f, 0.0f);
    texCoords[vertex + 1] = NTempest::C2Vector(1.0f, 0.0f);
    texCoords[vertex + 2] = NTempest::C2Vector(0.0f, 1.0f);
    texCoords[vertex + 3] = NTempest::C2Vector(1.0f, 1.0f);
    UINT index = i * 6;
    primVerts[index] = static_cast<WORD>(vertex);
    primVerts[index + 1] = static_cast<WORD>(vertex + 1);
    primVerts[index + 2] = static_cast<WORD>(vertex + 2);
    primVerts[index + 3] = static_cast<WORD>(vertex + 2);
    primVerts[index + 4] = static_cast<WORD>(vertex + 1);
    primVerts[index + 5] = static_cast<WORD>(vertex + 3);
  }

  return ModelCreateSimpleMesh(
      "TaxiRouteMap", verts.Count(), verts.Ptr(), normals.Ptr(), texCoords.Ptr(), GxPrim_Triangles, primVerts.Ptr(), primVerts.Count(), s_solidColor,
      GxBlend_Alpha, 0, NTempest::CImVector(0xFFFFFFFFUL), 0
  );
}

bool TaxiRouteExists(int fromNode, int toNode) {
  return fromNode && toNode && fromNode <= 63 && toNode <= 63 && s_taxiPathCosts[fromNode][toNode];
}
