#include <Base/Base.h>

#include "Model/CollisionData.h"

#include "Services/Texture.h"
#include "Tempest/c44matrix.h"

#include <malloc.h>

static const unsigned short boxIndices[36] = {12, 18, 0,  0,  18, 6,  13, 1, 16, 16, 1, 4,  2,  8,  5,  5,  8,  11,
                                              7,  19, 10, 10, 19, 22, 3,  9, 15, 15, 9, 21, 17, 23, 14, 14, 23, 20};

static void CollisionDataRenderAABox(const NTempest::CAaBox &box, const NTempest::C34Matrix &cameraSpace);

static HMODEL CreateSimpleModel(
    const NTempest::C3Vector *positions,
    const NTempest::C3Vector *normals,
    unsigned int              numVertices,
    const unsigned short     *primVertIndices,
    unsigned int              numIndices
) {
  NTempest::CImVector textureColor;
  textureColor.Set(0x7FFFFFFF);
  HTEXTURE                         texture = TextureCreateSolid(textureColor, 0);
  TSStackArray<NTempest::C2Vector> texCoords(_alloca(numVertices * sizeof(NTempest::C2Vector)), numVertices, numVertices);

  NTempest::CImVector color(255, 255, 255, 255);
  HMODEL              model = ModelCreateSimpleMesh(
      "Collision Debug", numVertices, positions, normals, texCoords.Ptr(), GxPrim_Triangles, primVertIndices, numIndices, texture, GxBlend_Alpha, 0,
      color, 0
  );
  HandleClose(texture);
  return model;
}

static int CreateCollisionDisplayNormals(const CCollisionData &collide, HMODEL model) {
  unsigned int                     numFacets = collide.surfaceNormals.Count();
  TSStackArray<NTempest::C3Vector> position(_alloca(numFacets * 2 * sizeof(NTempest::C3Vector)), numFacets * 2, numFacets * 2);
  TSStackArray<NTempest::C3Vector> normal(_alloca(numFacets * 2 * sizeof(NTempest::C3Vector)), numFacets * 2, numFacets * 2);
  TSStackArray<NTempest::C2Vector> texCoord(_alloca(numFacets * 2 * sizeof(NTempest::C2Vector)), numFacets * 2, numFacets * 2);
  TSStackArray<unsigned short>     indices(_alloca(numFacets * 2 * sizeof(unsigned short)), numFacets * 2, numFacets * 2);
  normal.Zero();
  texCoord.Zero();

  NTempest::C3Vector dimensions(
      collide.extents.t.x - collide.extents.b.x, collide.extents.t.y - collide.extents.b.y, collide.extents.t.z - collide.extents.b.z
  );
  float normalScale = dimensions.x;
  if (dimensions.y < normalScale) {
    normalScale = dimensions.y;
  }
  if (dimensions.z < normalScale) {
    normalScale = dimensions.z;
  }
  if (normalScale <= 1.0f) {
    normalScale = 1.0f;
  }

  unsigned int i;
  for (i = 0; i < numFacets; ++i) {
    NTempest::C3Vector average = collide.vertices[collide.indices[i * 3]];
    average += collide.vertices[collide.indices[i * 3 + 1]];
    average += collide.vertices[collide.indices[i * 3 + 2]];
    average.x *= 0.33333334f;
    average.y *= 0.33333334f;
    average.z *= 0.33333334f;

    position[i * 2] = average;
    position[i * 2 + 1] = NTempest::C3Vector(
        average.x + collide.surfaceNormals[i].x * normalScale, average.y + collide.surfaceNormals[i].y * normalScale,
        average.z + collide.surfaceNormals[i].z * normalScale
    );
    indices[i * 2] = static_cast<unsigned short>(i * 2);
    indices[i * 2 + 1] = static_cast<unsigned short>(i * 2 + 1);
  }

  NTempest::CImVector textureColor;
  textureColor.Set(static_cast<unsigned char>(255), static_cast<unsigned char>(255), static_cast<unsigned char>(0), static_cast<unsigned char>(0));
  HTEXTURE            texture = TextureCreateSolid(textureColor, 0);
  NTempest::CImVector color(255, 255, 255, 255);
  int                 result = ModelGeosetAdd(
      model, position.Count(), position.Ptr(), normal.Ptr(), texCoord.Ptr(), GxPrim_Lines, indices.Ptr(), indices.Count(), texture, GxBlend_Opaque, 1,
      color, 0
  );
  HandleClose(texture);
  return result;
}

static HMODEL CreateCollisionDisplayMesh(const CCollisionData &collide) {
  unsigned int                     numTriangles = collide.indices.Count();
  TSStackArray<NTempest::C3Vector> positions(_alloca(numTriangles * sizeof(NTempest::C3Vector)), numTriangles, numTriangles);
  TSStackArray<NTempest::C3Vector> normals(_alloca(numTriangles * sizeof(NTempest::C3Vector)), numTriangles, numTriangles);
  TSStackArray<unsigned short>     primVertIndices(_alloca(numTriangles * sizeof(unsigned short)), numTriangles, numTriangles);

  unsigned int i;
  for (i = 0; i < numTriangles; ++i) {
    primVertIndices[i] = static_cast<unsigned short>(i);
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
    const unsigned short   **debugIndices
) {
  unsigned int index;
  for (index = 0; index < 8; ++index) {
    debugVerts[index * 3] = boxVerts[index];
    debugVerts[index * 3 + 1] = boxVerts[index];
    debugVerts[index * 3 + 2] = boxVerts[index];
  }

  NTempest::C3Vector normX[2] = {boxNormals[1], boxNormals[0]};
  NTempest::C3Vector normY[2] = {boxNormals[2], boxNormals[3]};
  NTempest::C3Vector normZ[2] = {boxNormals[5], boxNormals[4]};
  index = 0;
  for (unsigned int z = 0; z < 2; ++z) {
    for (unsigned int y = 0; y < 2; ++y) {
      for (unsigned int x = 0; x < 2; ++x) {
        debugNormals[index++] = normX[x];
        debugNormals[index++] = normY[y];
        debugNormals[index++] = normZ[z];
      }
    }
  }
  *debugIndices = boxIndices;
}

HMODEL CollisionDataCreateModel(HCOLLISIONDATA handle) {
  CCollisionData *collide = reinterpret_cast<CCollisionData *>(handle);
  FATALASSERT(collide);

  HMODEL model = CreateCollisionDisplayMesh(*collide);
  CreateCollisionDisplayNormals(*collide, model);
  return model;
}

void CollisionDataAABoxRenderCallback(HMODEL model, const NTempest::C34Matrix &basis, void *param) {
  CModelShared *shared;
  IModelDerefHandle(reinterpret_cast<CModel *>(model), &shared);
  ASSERT(shared);
  CCollisionData *collide = reinterpret_cast<CCollisionData *>(shared->collision);
  ASSERT(collide);
  CollisionDataRenderAABox(collide->extents, basis);
}

static void CollisionDataRenderAABox(const NTempest::CAaBox &box, const NTempest::C34Matrix &cameraSpace) {
  NTempest::C3Vector        renderVerts[24];
  NTempest::C3Vector        renderNorms[24];
  NTempest::C3Vector        boxVerts[8];
  NTempest::C3Vector        boxNormals[6] = {NTempest::C3Vector(1.0f, 0.0f, 0.0f), NTempest::C3Vector(-1.0f, 0.0f, 0.0f),
                                             NTempest::C3Vector(0.0f, 1.0f, 0.0f), NTempest::C3Vector(0.0f, -1.0f, 0.0f),
                                             NTempest::C3Vector(0.0f, 0.0f, 1.0f), NTempest::C3Vector(0.0f, 0.0f, -1.0f)};
  const unsigned short     *renderIndices;
  const NTempest::C3Vector *verts[2] = {&box.b, &box.t};
  unsigned int              y;
  unsigned int              z;
  for (z = 0; z < 2; ++z) {
    for (y = 0; y < 2; ++y) {
      boxVerts[z * 4 + y * 2] = NTempest::C3Vector(verts[0]->x, verts[y]->y, verts[z]->z);
      boxVerts[z * 4 + y * 2 + 1] = NTempest::C3Vector(verts[1]->x, verts[y]->y, verts[z]->z);
    }
  }

  BuildDisplayBox(boxVerts, boxNormals, renderVerts, renderNorms, &renderIndices);

  GxRsPush();
  GxRsSet(GxRs_Blend, 2);
  GxRsSet(GxRs_DepthWrite, 0);
  NTempest::CImVector diffuse;
  diffuse.Set(0x8000FFFF);
  GxRsSet(GxRs_MatDiffuse, diffuse);
  GxRsSet(GxRs_Fog, 0);

  NTempest::C44Matrix world(
      cameraSpace.a0, cameraSpace.a1, cameraSpace.a2, 0.0f, cameraSpace.b0, cameraSpace.b1, cameraSpace.b2, 0.0f, cameraSpace.c0, cameraSpace.c1,
      cameraSpace.c2, 0.0f, cameraSpace.d0, cameraSpace.d1, cameraSpace.d2, 1.0f
  );
  GxXformPush(GxXform_World, world);
  GxVertexShaderSelect(GxVS_PassThru);
  GxPrimLockVertexPtrs(24, renderVerts, sizeof(NTempest::C3Vector), renderNorms, sizeof(NTempest::C3Vector), 0, 0, 0, 0, 0, 0, 0, 0);
  GxPrimDrawElements(GxPrim_Triangles, 36, renderIndices);
  GxPrimUnlockVertexPtrs();
  GxXformPop(GxXform_World);
  GxRsPop();
}
