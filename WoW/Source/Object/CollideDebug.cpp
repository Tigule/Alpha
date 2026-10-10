#include <Base/Base.h>
#include <Gx/Gx.h>
#include <WowConst.h>
#include <MapDefs.h>
#include "WorldClient/World.h"

#include "Object/MovementData.h"

struct CARgbColor {
  operator NTempest::CImVector() const {
    return NTempest::CImVector(a, r, g, b);
  }

  BYTE a;
  BYTE r;
  BYTE g;
  BYTE b;
};

static CARgbColor s_facetColor[NUM_FACET_COLORS] = {
    {0x80, 0x00, 0xFF, 0xFF},
    {0x80, 0x00, 0xFF, 0x00},
    {0x80, 0xFF, 0xFF, 0x00},
    {0x80, 0xFF, 0x00, 0x00}
};

static TSGrowableArray<enum FACET_COLOR>  s_debugFacetColors;
TSGrowableArray<NTempest::C3Vector>  g_debugVerts;
TSGrowableArray<WORD>                g_debugIndices;
TSGrowableArray<NTempest::CImVector> g_debugVertColors;
TSGrowableArray<NTempest::C3Vector>  g_debugNormalVerts;
TSGrowableArray<WORD>                g_debugNormalIndices;
TSGrowableArray<NTempest::C3Vector>  g_debugBoxVerts;
TSGrowableArray<WORD>                g_debugBoxIndices;
TSGrowableArray<NTempest::C3Vector>  g_debugBoxNormals;
static DWORDLONG                     s_currentWatchGUID;
static int                           s_acceptingFacets;

static void AddTriangle(
    const NTempest::CFacet               &face,
    FACET_COLOR                           color,
    TSGrowableArray<NTempest::C3Vector>  *debugVerts,
    TSGrowableArray<WORD>                *debugIndices,
    TSGrowableArray<NTempest::CImVector> *debugVertColors
) {
  UINT numVerts = debugVerts->Count();
  UINT numIndices = debugIndices->Count();

  debugVerts->Add(3, face.vertices);
  debugIndices->SetCount(numIndices + 3);
  debugVertColors->SetCount(numVerts + 3);

  (*debugIndices)[numIndices] = numVerts;
  (*debugIndices)[numIndices + 1] = numVerts + 1;
  (*debugIndices)[numIndices + 2] = numVerts + 2;

  (*debugVertColors)[numVerts] = s_facetColor[color];
  (*debugVertColors)[numVerts + 1] = s_facetColor[color];
  (*debugVertColors)[numVerts + 2] = s_facetColor[color];
}

static void AddNormalLine(
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
  debugIndices->SetCount(numIndices + 2);

  (*debugVerts)[numVerts] = position;
  (*debugVerts)[numVerts + 1] = normalVert;
  (*debugIndices)[numIndices] = numVerts;
  (*debugIndices)[numIndices + 1] = numVerts + 1;
}

static void AddNormalLine(const NTempest::CFacet &face) {
  AddNormalLine(
      face.plane.n, (face.vertices[0] + face.vertices[1] + face.vertices[2]) * 0.33333334f, 0.83333331f, &g_debugNormalVerts, &g_debugNormalIndices
  );
}

static void BuildDisplayBox(
    const NTempest::C3Vector             boxVerts[8],
    const NTempest::C3Vector             boxNormals[6],
    TSGrowableArray<NTempest::C3Vector> *debugVerts,
    TSGrowableArray<NTempest::C3Vector> *debugNormals,
    TSGrowableArray<WORD>               *debugIndices,
    int                                  displayNormals
) {
  NTempest::C3Vector *dst;
  NTempest::C3Vector *norm;
  UINT                index;

  debugVerts->SetCount(24);
  dst = debugVerts->Ptr();
  for (index = 0; index < 8; ++index, dst += 3) {
    dst[0] = dst[1] = dst[2] = boxVerts[index];
  }

  debugNormals->SetCount(24);

  NTempest::C3Vector normX[2];
  NTempest::C3Vector normY[2];
  NTempest::C3Vector normZ[2];

  normX[0] = boxNormals[1];
  normX[1] = boxNormals[0];
  normY[0] = boxNormals[2];
  normY[1] = boxNormals[3];
  normZ[0] = boxNormals[5];
  normZ[1] = boxNormals[4];

  norm = debugNormals->Ptr();
  for (UINT z = 0; z < 2; ++z) {
    for (UINT y = 0; y < 2; ++y) {
      for (UINT x = 0; x < 2; ++x, norm += 3) {
        norm[0] = normX[x];
        norm[1] = normY[y];
        norm[2] = normZ[z];
      }
    }
  }

  const WORD boxIndices[36] = {12, 18, 0,  0,  18, 6,  13, 1, 16, 16, 1, 4,  2,  8,  5,  5,  8,  11,
                               7,  19, 10, 10, 19, 22, 3,  9, 15, 15, 9, 21, 17, 23, 14, 14, 23, 20};

  debugIndices->Set(36, boxIndices);

  if (displayNormals) {
    AddNormalLine(boxNormals[0], (boxVerts[1] + boxVerts[5] + boxVerts[3] + boxVerts[7]) * 0.25f, 0.83333331f, &g_debugNormalVerts, &g_debugNormalIndices);
    AddNormalLine(boxNormals[1], (boxVerts[4] + boxVerts[0] + boxVerts[6] + boxVerts[2]) * 0.25f, 0.83333331f, &g_debugNormalVerts, &g_debugNormalIndices);
    AddNormalLine(boxNormals[2], (boxVerts[4] + boxVerts[5] + boxVerts[0] + boxVerts[1]) * 0.25f, 0.83333331f, &g_debugNormalVerts, &g_debugNormalIndices);
    AddNormalLine(boxNormals[3], (boxVerts[2] + boxVerts[3] + boxVerts[6] + boxVerts[7]) * 0.25f, 0.83333331f, &g_debugNormalVerts, &g_debugNormalIndices);
    AddNormalLine(boxNormals[4], (boxVerts[6] + boxVerts[7] + boxVerts[4] + boxVerts[5]) * 0.25f, 0.83333331f, &g_debugNormalVerts, &g_debugNormalIndices);
    AddNormalLine(boxNormals[5], (boxVerts[0] + boxVerts[1] + boxVerts[2] + boxVerts[3]) * 0.25f, 0.83333331f, &g_debugNormalVerts, &g_debugNormalIndices);
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

  g_debugVertColors[faceId * 3] = s_facetColor[color];
  g_debugVertColors[faceId * 3 + 1] = s_facetColor[color];
  g_debugVertColors[faceId * 3 + 2] = s_facetColor[color];
}

void CollisionInfoSetFallBox(const NTempest::C3Vector &position, float boxHalfDepth, float boxHeight) {
  if (!s_acceptingFacets) {
    return;
  }

  UINT                y;
  UINT                x;
  UINT                index;
  NTempest::C3Vector *dst;

  g_debugBoxVerts.SetCount(32);
  dst = g_debugBoxVerts.Ptr();
  {
    NTempest::C3Vector verts[2];

    verts[0] = verts[1] = position;
    verts[0].x -= boxHalfDepth;
    verts[0].y -= boxHalfDepth;
    verts[0].z += boxHalfDepth * 1.849399f;
    verts[1].x += boxHalfDepth;
    verts[1].y += boxHalfDepth;
    verts[1].z += boxHeight;

    for (y = 0; y < 2; ++y) {
      for (x = 0; x < 2; ++x, dst += 3) {
        dst[0].Set(verts[x].x, verts[y].y, verts[1].z);
        dst[1] = dst[0];
        dst[2] = dst[0];
      }
    }
    for (y = 0; y < 2; ++y) {
      for (x = 0; x < 2; ++x, dst += 4) {
        dst[0].Set(verts[x].x, verts[y].y, verts[0].z);
        dst[1] = dst[0];
        dst[2] = dst[0];
        dst[3] = dst[0];
      }
    }
  }
  for (index = 28; index < 32; ++index) {
    g_debugBoxVerts[index] = position;
  }

  g_debugBoxNormals.SetCount(32);

  NTempest::C3Vector normX[2];
  NTempest::C3Vector normY[2];

  normX[0].Set(-1.0f, 0.0f, 0.0f);
  normX[1].Set(1.0f, 0.0f, 0.0f);
  normY[0].Set(0.0f, -1.0f, 0.0f);
  normY[1].Set(0.0f, 1.0f, 0.0f);

  dst = g_debugBoxNormals.Ptr();
  for (y = 0; y < 2; ++y) {
    for (x = 0; x < 2; ++x, dst += 3) {
      dst[0] = normX[x];
      dst[1] = normY[y];
      dst[2].Set(0.0f, 0.0f, 1.0f);
    }
  }

  NTempest::C3Vector normZX[2];
  NTempest::C3Vector normZY[2];

  normZX[0].Set(-0.87964189f, 0.0f, -0.4756366f);
  normZX[1].Set(0.87964189f, 0.0f, -0.4756366f);
  normZY[0].Set(0.0f, -0.87964189f, -0.4756366f);
  normZY[1].Set(0.0f, 0.87964189f, -0.4756366f);

  for (y = 0; y < 2; ++y) {
    for (x = 0; x < 2; ++x, dst += 3) {
      dst[0] = normX[x];
      dst[1] = normY[y];
      dst[2] = normZX[x];
      dst[3] = normZY[y];
    }
  }
  dst[0] = normZX[0];
  dst[1] = normZY[0];
  dst[2] = normZX[1];
  dst[3] = normZY[1];

  const WORD boxIndices[42] = {0, 6,  12, 12, 6,  20, 1, 13, 4, 4,  13, 17, 21, 7,  25, 25, 7,  10, 16, 24, 3,
                               3, 24, 9,  5,  11, 2,  2, 11, 8, 15, 29, 19, 18, 28, 26, 27, 31, 23, 22, 30, 14};

  g_debugBoxIndices.Set(42, boxIndices);

  float              halfBoxHeight = (boxHalfDepth * 1.849399f + boxHeight) * 0.5f + position.z;
  NTempest::C3Vector normal;
  NTempest::C3Vector start;

  start.Set(position.x - boxHalfDepth, position.y, halfBoxHeight);
  normal.Set(-1.0f, 0.0f, 0.0f);
  AddNormalLine(normal, start, 0.83333331f, &g_debugNormalVerts, &g_debugNormalIndices);
  start.Set(boxHalfDepth + position.x, position.y, halfBoxHeight);
  normal.Set(1.0f, 0.0f, 0.0f);
  AddNormalLine(normal, start, 0.83333331f, &g_debugNormalVerts, &g_debugNormalIndices);
  start.Set(position.x, position.y - boxHalfDepth, halfBoxHeight);
  normal.Set(0.0f, -1.0f, 0.0f);
  AddNormalLine(normal, start, 0.83333331f, &g_debugNormalVerts, &g_debugNormalIndices);
  start.Set(position.x, boxHalfDepth + position.y, halfBoxHeight);
  normal.Set(0.0f, 1.0f, 0.0f);
  AddNormalLine(normal, start, 0.83333331f, &g_debugNormalVerts, &g_debugNormalIndices);
  start.Set(position.x, position.y, boxHeight + position.z);
  normal.Set(0.0f, 0.0f, 1.0f);
  AddNormalLine(normal, start, 0.83333331f, &g_debugNormalVerts, &g_debugNormalIndices);
}

void CollisionInfoAddBox(const NTempest::C3Vector &boxMin, const NTempest::C3Vector &boxMax) {
  if (!s_acceptingFacets) {
    return;
  }

  NTempest::C3Vector        boxVerts[8];
  const NTempest::C3Vector *verts[2] = {&boxMin, &boxMax};
  UINT                      z;

  for (z = 0; z < 2; ++z) {
    for (UINT y = 0; y < 2; ++y) {
      for (UINT x = 0; x < 2; ++x) {
        boxVerts[z * 4 + y * 2 + x].Set(verts[x]->x, verts[y]->y, verts[z]->z);
      }
    }
  }

  NTempest::C3Vector boxNormals[6];

  boxNormals[0].Set(1.0f, 0.0f, 0.0f);
  boxNormals[1].Set(-1.0f, 0.0f, 0.0f);
  boxNormals[2].Set(0.0f, 1.0f, 0.0f);
  boxNormals[3].Set(0.0f, -1.0f, 0.0f);
  boxNormals[4].Set(0.0f, 0.0f, 1.0f);
  boxNormals[5].Set(0.0f, 0.0f, -1.0f);

  BuildDisplayBox(boxVerts, boxNormals, &g_debugBoxVerts, &g_debugBoxNormals, &g_debugBoxIndices, 0);
}

void CollisionInfoAddVector(const NTempest::C3Vector &position, const NTempest::C3Vector &vector) {
  if (s_acceptingFacets) {
    AddNormalLine(vector, position, 0.55555558f, &g_debugNormalVerts, &g_debugNormalIndices);
  }
}
