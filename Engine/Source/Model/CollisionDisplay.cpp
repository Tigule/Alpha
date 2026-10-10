#include <Base/Base.h>

#include "Model/ModelInternal.h"
#include "Model/CollisionData.h"

#include "Services/Texture.h"
#include "Tempest/c44matrix.h"

#include <malloc.h>

static const WORD boxIndices[36] = {12, 18, 0,  0,  18, 6,  13, 1, 16, 16, 1, 4,  2,  8,  5,  5,  8,  11,
                                    7,  19, 10, 10, 19, 22, 3,  9, 15, 15, 9, 21, 17, 23, 14, 14, 23, 20};

void CollisionDataRenderAABox(const NTempest::CAaBox &box, const NTempest::C34Matrix &cameraSpace);

static HMODEL CreateSimpleModel(
    const NTempest::C3Vector *positions,
    const NTempest::C3Vector *normals,
    UINT                      numVertices,
    const WORD               *primVertIndices,
    UINT                      numIndices
) {
  HTEXTURE                         texture = TextureCreateSolid(NTempest::CImVector(0x7FFFFFFF), 0);
  TSStackArray<NTempest::C2Vector> texCoords(_alloca(numVertices * sizeof(NTempest::C2Vector)), numVertices, numVertices);

  HMODEL model = ModelCreateSimpleMesh(
      "Collision Debug", numVertices, positions, normals, texCoords.Ptr(), GxPrim_Triangles, primVertIndices, numIndices, texture, GxBlend_Alpha, 0,
      NTempest::CImVector(255, 255, 255, 255), 0
  );
  HandleClose(texture);
  return model;
}

static int CreateCollisionDisplayNormals(const CCollisionData &collide, HMODEL model) {
  UINT                             numFacets = collide.surfaceNormals.Count();
  TSStackArray<NTempest::C3Vector> position(_alloca(numFacets * 2 * sizeof(NTempest::C3Vector)), numFacets * 2, numFacets * 2);
  TSStackArray<NTempest::C3Vector> normal(_alloca(numFacets * 2 * sizeof(NTempest::C3Vector)), numFacets * 2, numFacets * 2);
  TSStackArray<NTempest::C2Vector> texCoord(_alloca(numFacets * 2 * sizeof(NTempest::C2Vector)), numFacets * 2, numFacets * 2);
  TSStackArray<WORD>               indices(_alloca(numFacets * 2 * sizeof(WORD)), numFacets * 2, numFacets * 2);
  normal.Zero();
  texCoord.Zero();

  NTempest::C3Vector dimensions = collide.extents.t - collide.extents.b;
  float              normalScale = max(min(dimensions.x, min(dimensions.y, dimensions.z)), 1.0f);

  UINT i;
  for (i = 0; i < numFacets; ++i) {
    NTempest::C3Vector average = collide.vertices[collide.indices[i * 3]];
    average += collide.vertices[collide.indices[i * 3 + 1]];
    average += collide.vertices[collide.indices[i * 3 + 2]];
    average *= 1.0f / 3.0f;

    position[i * 2] = average;
    position[i * 2 + 1] = average + collide.surfaceNormals[i] * normalScale;
    indices[i * 2] = i * 2;
    indices[i * 2 + 1] = i * 2 + 1;
  }

  HTEXTURE texture = TextureCreateSolid(NTempest::CImVector(255, 255, 0, 0), 0);
  int      result = ModelGeosetAdd(
      model, position.Count(), position.Ptr(), normal.Ptr(), texCoord.Ptr(), GxPrim_Lines, indices.Ptr(), indices.Count(), texture, GxBlend_Opaque, 1,
      NTempest::CImVector(255, 255, 255, 255), 0
  );
  HandleClose(texture);
  return result;
}

static HMODEL CreateCollisionDisplayMesh(const CCollisionData &collide) {
  UINT                             numTriangles = collide.indices.Count();
  TSStackArray<NTempest::C3Vector> positions(_alloca(numTriangles * sizeof(NTempest::C3Vector)), numTriangles, numTriangles);
  TSStackArray<NTempest::C3Vector> normals(_alloca(numTriangles * sizeof(NTempest::C3Vector)), numTriangles, numTriangles);
  TSStackArray<WORD>               primVertIndices(_alloca(numTriangles * sizeof(WORD)), numTriangles, numTriangles);

  UINT i;
  for (i = 0; i < numTriangles; ++i) {
    primVertIndices[i] = i;
    positions[i] = collide.vertices[collide.indices[i]];
  }

  for (i = 0; i < numTriangles / 3; ++i) {
    normals[i * 3] = collide.surfaceNormals[i];
    normals[i * 3 + 1] = collide.surfaceNormals[i];
    normals[i * 3 + 2] = collide.surfaceNormals[i];
  }

  return CreateSimpleModel(positions.Ptr(), normals.Ptr(), positions.Count(), primVertIndices.Ptr(), numTriangles);
}

static void BuildDisplayBox(
    const NTempest::C3Vector boxVerts[8],
    const NTempest::C3Vector boxNormals[6],
    NTempest::C3Vector      *debugVerts,
    NTempest::C3Vector      *debugNormals,
    const WORD             **debugIndices
) {
  for (UINT i = 0; i < 8; ++i) {
    debugVerts[i * 3] = debugVerts[i * 3 + 1] = debugVerts[i * 3 + 2] = boxVerts[i];
  }

  NTempest::C3Vector normX[2], normY[2], normZ[2];
  normX[0] = boxNormals[1];
  normX[1] = boxNormals[0];
  normY[0] = boxNormals[2];
  normY[1] = boxNormals[3];
  normZ[0] = boxNormals[5];
  normZ[1] = boxNormals[4];
  for (UINT z = 0; z < 2; ++z) {
    for (UINT y = 0; y < 2; ++y) {
      for (UINT x = 0; x < 2; ++x, debugNormals += 3) {
        debugNormals[0] = normX[x];
        debugNormals[1] = normY[y];
        debugNormals[2] = normZ[z];
      }
    }
  }
  *debugIndices = boxIndices;
}

HMODEL CollisionDataCreateModel(HCOLLISIONDATA handle) {
  CCollisionData *collide = (CCollisionData *)handle;
  VALIDATEBEGIN;
  VALIDATE(collide);
  VALIDATEEND;

  HMODEL model = CreateCollisionDisplayMesh(*collide);
  CreateCollisionDisplayNormals(*collide, model);
  return model;
}

void CollisionDataAABoxRenderCallback(HMODEL model, const NTempest::C34Matrix &basis, LPVOID param) {
  CModelShared *shared;
  IModelDerefHandle((CModel *)model, &shared);
  ASSERT(shared);
  CCollisionData *collide = (CCollisionData *)shared->collision;
  ASSERT(collide);
  CollisionDataRenderAABox(collide->extents, basis);
}

void CollisionDataRenderAABox(const NTempest::CAaBox &box, const NTempest::C34Matrix &cameraSpace) {
  NTempest::C3Vector boxVerts[8];

  const NTempest::C3Vector *verts[2] = {&box.b, &box.t};
  NTempest::C3Vector       *dst = boxVerts;
  for (UINT z = 0; z < 2; ++z) {
    for (UINT y = 0; y < 2; ++y) {
      for (UINT x = 0; x < 2; ++x, ++dst) {
        dst->Set(verts[x]->x, verts[y]->y, verts[z]->z);
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

  NTempest::C3Vector renderVerts[24];
  NTempest::C3Vector renderNorms[24];
  const WORD        *renderIndices;

  BuildDisplayBox(boxVerts, boxNormals, renderVerts, renderNorms, &renderIndices);

  GxRsPush();
  GxRsSet(GxRs_Blend, GxBlend_Alpha);
  GxRsSet(GxRs_DepthWrite, 0);
  GxRsSet(GxRs_MatDiffuse, NTempest::CImVector(0x8000FFFF));
  GxRsSet(GxRs_Fog, 0);
  GxXformPush(GxXform_World, NTempest::C44Matrix(cameraSpace));
  GxVertexShaderSelect(GxVS_PassThru);
  GxPrimLockVertexPtrs(24, renderVerts, sizeof(NTempest::C3Vector), renderNorms, sizeof(NTempest::C3Vector), 0, 0, 0, 0, 0, 0, 0, 0);
  GxPrimDrawElements(GxPrim_Triangles, 36, renderIndices);
  GxPrimUnlockVertexPtrs();
  GxXformPop(GxXform_World);
  GxRsPop();
}
