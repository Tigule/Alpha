#include <Base/Base.h>
#include <WowConst.h>
#include <MapDefs.h>

#include "Object/MovementData.h"

struct CARgbColor {
  BYTE a;
  BYTE r;
  BYTE g;
  BYTE b;

  operator NTempest::CImVector() const {
    return NTempest::CImVector(a, r, g, b);
  }
};

static CARgbColor s_facetColor[NUM_FACET_COLORS] = {
    {0x80, 0x00, 0xFF, 0xFF},
    {0x80, 0x00, 0xFF, 0x00},
    {0x80, 0xFF, 0xFF, 0x00},
    {0x80, 0xFF, 0x00, 0x00}
};

TSGrowableArray<WORD>                g_debugBoxIndices;
TSGrowableArray<NTempest::C3Vector>  g_debugBoxNormals;
TSGrowableArray<WORD>                g_debugIndices;
TSGrowableArray<NTempest::C3Vector>  g_debugNormalVerts;
TSGrowableArray<NTempest::C3Vector>  g_debugVerts;
TSGrowableArray<WORD>                g_debugNormalIndices;
static TSGrowableArray<enum FACET_COLOR>  s_debugFacetColors;
TSGrowableArray<NTempest::C3Vector>  g_debugBoxVerts;
TSGrowableArray<NTempest::CImVector> g_debugVertColors;
static DWORDLONG                     s_currentWatchGUID;
static int                           s_acceptingFacets;

void AddTriangle(
    const NTempest::CFacet               &face,
    FACET_COLOR                           color,
    TSGrowableArray<NTempest::C3Vector>  *debugVerts,
    TSGrowableArray<WORD>                *debugIndices,
    TSGrowableArray<NTempest::CImVector> *debugVertColors
) {
  UINT numVerts = debugVerts->Count();
  UINT numIndices = debugIndices->Count();

  debugVerts->SetCount(numVerts + 3);
  (*debugVerts)[numVerts] = face.vertices[0];
  (*debugVerts)[numVerts + 1] = face.vertices[1];
  (*debugVerts)[numVerts + 2] = face.vertices[2];

  debugIndices->SetCount(numIndices + 3);
  (*debugIndices)[numIndices] = static_cast<WORD>(numVerts);
  (*debugIndices)[numIndices + 1] = static_cast<WORD>(numVerts + 1);
  (*debugIndices)[numIndices + 2] = static_cast<WORD>(numVerts + 2);

  debugVertColors->SetCount(numVerts + 3);
  (*debugVertColors)[numVerts] = s_facetColor[color];
  (*debugVertColors)[numVerts + 1] = s_facetColor[color];
  (*debugVertColors)[numVerts + 2] = s_facetColor[color];
}

void AddNormalLine(
    const NTempest::C3Vector            &normal,
    const NTempest::C3Vector            &position,
    float                                scale,
    TSGrowableArray<NTempest::C3Vector> *debugVerts,
    TSGrowableArray<WORD>               *debugIndices
) {
  NTempest::C3Vector normalVert = position + normal * scale;
  UINT               numVerts = debugVerts->Count();
  UINT               numIndices = debugIndices->Count();

  debugVerts->SetCount(numVerts + 2);
  (*debugVerts)[numVerts] = position;
  (*debugVerts)[numVerts + 1] = normalVert;
  debugIndices->SetCount(numIndices + 2);
  (*debugIndices)[numIndices] = static_cast<WORD>(numVerts);
  (*debugIndices)[numIndices + 1] = static_cast<WORD>(numVerts + 1);
}

void AddNormalLine(const NTempest::CFacet &face) {
  AddNormalLine(
      face.plane.n, (face.vertices[0] + face.vertices[1] + face.vertices[2]) * 0.33333334f, 0.83333331f, &g_debugNormalVerts, &g_debugNormalIndices
  );
}

void BuildDisplayBox(
    const NTempest::C3Vector             boxVerts[8],
    const NTempest::C3Vector             boxNormals[6],
    TSGrowableArray<NTempest::C3Vector> *debugVerts,
    TSGrowableArray<NTempest::C3Vector> *debugNormals,
    TSGrowableArray<WORD>               *debugIndices,
    int                                  displayNormals
) {
  NTempest::C3Vector  normZ[2] = {boxNormals[5], boxNormals[4]};
  NTempest::C3Vector  normX[2] = {boxNormals[1], boxNormals[0]};
  NTempest::C3Vector  normY[2] = {boxNormals[2], boxNormals[3]};
  const WORD          boxIndices[36] = {12, 18, 0,  0,  18, 6,  13, 1, 16, 16, 1, 4,  2,  8,  5,  5,  8,  11,
                                        7,  19, 10, 10, 19, 22, 3,  9, 15, 15, 9, 21, 17, 23, 14, 14, 23, 20};
  NTempest::C3Vector *norm;
  NTempest::C3Vector *dst;
  UINT                index;
  UINT                z;
  UINT                y;
  UINT                x;

  debugVerts->SetCount(24);
  dst = debugVerts->Ptr();
  for (index = 0; index < 8; ++index) {
    *dst++ = boxVerts[index];
    *dst++ = boxVerts[index];
    *dst++ = boxVerts[index];
  }

  debugNormals->SetCount(24);
  dst = debugNormals->Ptr();
  for (z = 0; z < 2; ++z) {
    for (y = 0; y < 2; ++y) {
      for (x = 0; x < 2; ++x) {
        *dst++ = normX[x];
        *dst++ = normY[y];
        *dst++ = normZ[z];
      }
    }
  }

  debugIndices->SetCount(36);
  for (index = 0; index < 36; ++index) {
    (*debugIndices)[index] = boxIndices[index];
  }

  if (displayNormals) {
    norm = const_cast<NTempest::C3Vector *>(boxNormals);
    AddNormalLine(*norm++, (boxVerts[1] + boxVerts[5] + boxVerts[3] + boxVerts[7]) * 0.25f, 0.83333331f, &g_debugNormalVerts, &g_debugNormalIndices);
    AddNormalLine(*norm++, (boxVerts[0] + boxVerts[4] + boxVerts[2] + boxVerts[6]) * 0.25f, 0.83333331f, &g_debugNormalVerts, &g_debugNormalIndices);
    AddNormalLine(*norm++, (boxVerts[4] + boxVerts[5] + boxVerts[0] + boxVerts[1]) * 0.25f, 0.83333331f, &g_debugNormalVerts, &g_debugNormalIndices);
    AddNormalLine(*norm++, (boxVerts[2] + boxVerts[3] + boxVerts[6] + boxVerts[7]) * 0.25f, 0.83333331f, &g_debugNormalVerts, &g_debugNormalIndices);
    AddNormalLine(*norm++, (boxVerts[6] + boxVerts[7] + boxVerts[4] + boxVerts[5]) * 0.25f, 0.83333331f, &g_debugNormalVerts, &g_debugNormalIndices);
    AddNormalLine(*norm, (boxVerts[0] + boxVerts[1] + boxVerts[2] + boxVerts[3]) * 0.25f, 0.83333331f, &g_debugNormalVerts, &g_debugNormalIndices);
  }
}

void CollisionInfoSetWatchGUID(const DWORDLONG &guid) {
  s_currentWatchGUID = guid;
}

void CollisionInfoReset() {
  g_debugVerts.SetCount(0);
  g_debugIndices.SetCount(0);
  g_debugVertColors.SetCount(0);
  g_debugNormalVerts.SetCount(0);
  g_debugNormalIndices.SetCount(0);
  s_debugFacetColors.SetCount(0);
  g_debugBoxVerts.SetCount(0);
  g_debugBoxIndices.SetCount(0);
  g_debugBoxNormals.SetCount(0);
}

void CollisionInfoSetFaces(const DWORDLONG &guid, const TSGrowableArray<NTempest::CFacet> &faces) {
  UINT numFaces;

  if (guid != s_currentWatchGUID) {
    s_acceptingFacets = 0;
    return;
  }

  s_acceptingFacets = 1;
  g_debugVerts.SetCount(0);
  g_debugIndices.SetCount(0);
  g_debugVertColors.SetCount(0);
  g_debugNormalVerts.SetCount(0);
  g_debugNormalIndices.SetCount(0);
  s_debugFacetColors.SetCount(0);

  numFaces = faces.Count();
  for (UINT faceId = 0; faceId < numFaces; ++faceId) {
    AddTriangle(faces[faceId], FACET_UNTESTED, &g_debugVerts, &g_debugIndices, &g_debugVertColors);
    *s_debugFacetColors.NewElement() = FACET_UNTESTED;
    AddNormalLine(faces[faceId]);
  }
}

void CollisionInfoColorFace(UINT faceId, FACET_COLOR color) {
  if (!s_acceptingFacets || color <= s_debugFacetColors[faceId]) {
    return;
  }

  s_debugFacetColors[faceId] = color;
  g_debugVertColors[faceId * 3] = s_facetColor[color];
  g_debugVertColors[faceId * 3 + 1] = s_facetColor[color];
  g_debugVertColors[faceId * 3 + 2] = s_facetColor[color];
}

void CollisionInfoSetFallBox(const NTempest::C3Vector &position, float boxHalfDepth, float boxHeight) {
  NTempest::C3Vector normY[2] = {NTempest::C3Vector(0.0f, -1.0f, 0.0f), NTempest::C3Vector(0.0f, 1.0f, 0.0f)};
  NTempest::C3Vector normX[2] = {NTempest::C3Vector(-1.0f, 0.0f, 0.0f), NTempest::C3Vector(1.0f, 0.0f, 0.0f)};
  const WORD         boxIndices[42] = {0, 6,  12, 12, 6,  20, 1, 13, 4, 4,  13, 17, 21, 7,  25, 25, 7,  10, 16, 24, 3,
                                       3, 24, 9,  5,  11, 2,  2, 11, 8, 15, 29, 19, 18, 28, 26, 27, 31, 23, 22, 30, 14};
  NTempest::C3Vector normZY[2] = {NTempest::C3Vector(0.0f, -0.87964189f, -0.4756366f), NTempest::C3Vector(0.0f, 0.87964189f, -0.4756366f)};
  NTempest::C3Vector start;
  float              halfBoxHeight;
  NTempest::C3Vector normZX[2] = {NTempest::C3Vector(-0.87964189f, 0.0f, -0.4756366f), NTempest::C3Vector(0.87964189f, 0.0f, -0.4756366f)};
  NTempest::C3Vector verts[2];
  NTempest::C3Vector normal;
  UINT               y;
  UINT               x;
  UINT               repeat;
  UINT               index;

  if (!s_acceptingFacets) {
    return;
  }

  verts[0] = NTempest::C3Vector(position.x - boxHalfDepth, position.y - boxHalfDepth, position.z + boxHalfDepth * 1.849399f);
  verts[1] = NTempest::C3Vector(position.x + boxHalfDepth, position.y + boxHalfDepth, position.z + boxHeight);

  g_debugBoxVerts.SetCount(32);
  for (y = 0; y < 2; ++y) {
    for (x = 0; x < 2; ++x) {
      for (repeat = 0; repeat < 3; ++repeat) {
        g_debugBoxVerts[(y * 2 + x) * 3 + repeat] = NTempest::C3Vector(verts[x].x, verts[y].y, verts[1].z);
      }
    }
  }
  for (y = 0; y < 2; ++y) {
    for (x = 0; x < 2; ++x) {
      for (repeat = 0; repeat < 4; ++repeat) {
        g_debugBoxVerts[12 + (y * 2 + x) * 4 + repeat] = NTempest::C3Vector(verts[x].x, verts[y].y, verts[0].z);
      }
    }
  }
  for (index = 28; index < 32; ++index) {
    g_debugBoxVerts[index] = position;
  }

  g_debugBoxNormals.SetCount(32);
  for (y = 0; y < 2; ++y) {
    for (x = 0; x < 2; ++x) {
      index = (y * 2 + x) * 3;
      g_debugBoxNormals[index] = normX[x];
      g_debugBoxNormals[index + 1] = normY[y];
      g_debugBoxNormals[index + 2] = NTempest::C3Vector(0.0f, 0.0f, 1.0f);
    }
  }
  for (y = 0; y < 2; ++y) {
    for (x = 0; x < 2; ++x) {
      index = 12 + (y * 2 + x) * 4;
      g_debugBoxNormals[index] = normX[x];
      g_debugBoxNormals[index + 1] = normY[y];
      g_debugBoxNormals[index + 2] = normZX[x];
      g_debugBoxNormals[index + 3] = normZY[y];
    }
  }
  g_debugBoxNormals[28] = normZX[0];
  g_debugBoxNormals[29] = normZY[0];
  g_debugBoxNormals[30] = normZX[1];
  g_debugBoxNormals[31] = normZY[1];

  g_debugBoxIndices.SetCount(42);
  for (index = 0; index < 42; ++index) {
    g_debugBoxIndices[index] = boxIndices[index];
  }

  halfBoxHeight = position.z + (boxHalfDepth * 1.849399f + boxHeight) * 0.5f;
  normal = NTempest::C3Vector(-1.0f, 0.0f, 0.0f);
  start = NTempest::C3Vector(position.x - boxHalfDepth, position.y, halfBoxHeight);
  AddNormalLine(normal, start, 0.83333331f, &g_debugNormalVerts, &g_debugNormalIndices);
  normal = NTempest::C3Vector(1.0f, 0.0f, 0.0f);
  start.x = position.x + boxHalfDepth;
  AddNormalLine(normal, start, 0.83333331f, &g_debugNormalVerts, &g_debugNormalIndices);
  normal = NTempest::C3Vector(0.0f, -1.0f, 0.0f);
  start = NTempest::C3Vector(position.x, position.y - boxHalfDepth, halfBoxHeight);
  AddNormalLine(normal, start, 0.83333331f, &g_debugNormalVerts, &g_debugNormalIndices);
  normal = NTempest::C3Vector(0.0f, 1.0f, 0.0f);
  start.y = position.y + boxHalfDepth;
  AddNormalLine(normal, start, 0.83333331f, &g_debugNormalVerts, &g_debugNormalIndices);
  normal = NTempest::C3Vector(0.0f, 0.0f, 1.0f);
  start = NTempest::C3Vector(position.x, position.y, position.z + boxHeight);
  AddNormalLine(normal, start, 0.83333331f, &g_debugNormalVerts, &g_debugNormalIndices);
}

void CollisionInfoAddBox(const NTempest::C3Vector &boxMin, const NTempest::C3Vector &boxMax) {
  NTempest::C3Vector        boxVerts[8];
  NTempest::C3Vector        boxNormals[6] = {NTempest::C3Vector(1.0f, 0.0f, 0.0f), NTempest::C3Vector(-1.0f, 0.0f, 0.0f),
                                             NTempest::C3Vector(0.0f, 1.0f, 0.0f), NTempest::C3Vector(0.0f, -1.0f, 0.0f),
                                             NTempest::C3Vector(0.0f, 0.0f, 1.0f), NTempest::C3Vector(0.0f, 0.0f, -1.0f)};
  const NTempest::C3Vector *verts[2] = {&boxMin, &boxMax};
  UINT                      z;

  if (!s_acceptingFacets) {
    return;
  }

  for (z = 0; z < 2; ++z) {
    for (UINT y = 0; y < 2; ++y) {
      boxVerts[z * 4 + y * 2] = NTempest::C3Vector(verts[0]->x, verts[y]->y, verts[z]->z);
      boxVerts[z * 4 + y * 2 + 1] = NTempest::C3Vector(verts[1]->x, verts[y]->y, verts[z]->z);
    }
  }

  BuildDisplayBox(boxVerts, boxNormals, &g_debugBoxVerts, &g_debugBoxNormals, &g_debugBoxIndices, 0);
}

void CollisionInfoAddVector(const NTempest::C3Vector &position, const NTempest::C3Vector &vector) {
  if (s_acceptingFacets) {
    AddNormalLine(vector, position, 0.55555558f, &g_debugNormalVerts, &g_debugNormalIndices);
  }
}
