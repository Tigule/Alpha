#include <Base/Base.h>

#include "Model/ModelInternal.h"
#include "Model/CollisionData.h"

#include "Gx/Gx.h"
#include "MDLFile/MDLTypes.h"
#include "Tempest/c33matrix.h"
#include "Tempest/cfacet.h"

#include <float.h>
#include <string.h>

BYTE *MDLFileBinarySeek(BYTE *fileData, UINT fileBytes, DWORD sectionTag);

static WORD vertIndices[36] = {4, 6, 0, 0, 6, 2, 4, 0, 5, 5, 0, 1, 0, 2, 1, 1, 2, 3, 2, 6, 3, 3, 6, 7, 1, 3, 5, 5, 3, 7, 5, 7, 4, 4, 7, 6};

static BOOL TriangleIsClippedOut(const NTempest::CAaBox &bounds, const NTempest::C3Vector *triVerts) {
  if (triVerts[0].x < bounds.b.x && triVerts[1].x < bounds.b.x && triVerts[2].x < bounds.b.x) {
    return 1;
  }
  if (triVerts[0].x > bounds.t.x && triVerts[1].x > bounds.t.x && triVerts[2].x > bounds.t.x) {
    return 1;
  }
  if (triVerts[0].y < bounds.b.y && triVerts[1].y < bounds.b.y && triVerts[2].y < bounds.b.y) {
    return 1;
  }
  if (triVerts[0].y > bounds.t.y && triVerts[1].y > bounds.t.y && triVerts[2].y > bounds.t.y) {
    return 1;
  }
  if (triVerts[0].z < bounds.b.z && triVerts[1].z < bounds.b.z && triVerts[2].z < bounds.b.z) {
    return 1;
  }
  if (triVerts[0].z > bounds.t.z && triVerts[1].z > bounds.t.z && triVerts[2].z > bounds.t.z) {
    return 1;
  }
  return 0;
}

static void CollisionDataAddFacets(
    HCOLLISIONDATA                     handle,
    const NTempest::C34Matrix         &toWorld,
    const float                        scale,
    const NTempest::CAaBox            &worldBox,
    TSGrowableArray<NTempest::CFacet> *facets
) {
  CCollisionData *collide = (CCollisionData *)handle;
  VALIDATEBEGIN;
  VALIDATE(collide);
  VALIDATE(facets);
  VALIDATEENDVOID;

  NTempest::C33Matrix rotation = toWorld;
  rotation = rotation / scale;
  UINT existing = facets->Count();
  UINT numFacets = collide->surfaceNormals.Count();
  facets->SetCount(numFacets + existing);
  UINT rejects = 0;
  for (UINT i = 0; i < numFacets; ++i) {
    UINT index = i - rejects + existing;
    (*facets)[index].vertices[0] = collide->vertices[collide->indices[i * 3]] * toWorld;
    (*facets)[index].vertices[1] = collide->vertices[collide->indices[i * 3 + 1]] * toWorld;
    (*facets)[index].vertices[2] = collide->vertices[collide->indices[i * 3 + 2]] * toWorld;
    if (TriangleIsClippedOut(worldBox, (*facets)[index].vertices)) {
      ++rejects;
      continue;
    }

    (*facets)[index].plane.Set(collide->surfaceNormals[i] * rotation, (*facets)[index].vertices[0]);
  }
  facets->SetCount(numFacets - rejects + existing);
}

static BOOL CollisionDataVectorIntersect(
    HCOLLISIONDATA__          *hDC,
    const NTempest::C34Matrix &basis,
    const NTempest::C3Vector  &p0,
    const NTempest::C3Vector  &p1,
    float                     &t
) {
  CCollisionData *collide = (CCollisionData *)hDC;
  VALIDATEBEGIN;
  VALIDATE(collide);
  VALIDATEEND;

  t = FLT_MAX;
  NTempest::C3Vector dir = p1 - p0;
  float              ooMag = 1.0f / dir.Mag();
  dir *= ooMag;

  UINT numSurfaces = collide->surfaceNormals.Count();
  for (UINT i = 0; i < numSurfaces; ++i) {
    float triT;
    if (GxuTestRayAndTriangle(
            p0,
            dir,
            collide->vertices[collide->indices[i * 3]] * basis,
            collide->vertices[collide->indices[i * 3 + 1]] * basis,
            collide->vertices[collide->indices[i * 3 + 2]] * basis,
            triT
        ) &&
        triT >= 0.0f && triT < t) {
      t = triT;
    }
  }

  if (t >= FLT_MAX) {
    return 0;
  }

  t *= ooMag;
  return t <= 1.0f;
}

static void ComputeSurfaceNormals(CCollisionData *collide, UINT numSurfaces) {
  collide->surfaceNormals.SetCount(numSurfaces);
  for (UINT i = 0; i < numSurfaces; ++i) {
    NTempest::C3Vector edge1 = collide->vertices[collide->indices[i * 3 + 1]] - collide->vertices[collide->indices[i * 3]];
    NTempest::C3Vector edge2 = collide->vertices[collide->indices[i * 3 + 2]] - collide->vertices[collide->indices[i * 3]];
    collide->surfaceNormals[i] = NTempest::C3Vector::Cross(edge1, edge2);
    collide->surfaceNormals[i].SafeNormalize();
  }
}

HCOLLISIONDATA CollisionDataCreate(const NTempest::CAaBox &bounds) {
  CCollisionData *collision = NEWHANDLE(HCOLLISIONDATA, CCollisionData);
  if (!collision) {
    return 0;
  }

  collision->vertices.SetCount(8);
  NTempest::C3Vector *vertex = collision->vertices.Ptr();
  for (UINT z = 0; z < 2; ++z) {
    for (UINT y = 0; y < 2; ++y) {
      for (UINT x = 0; x < 2; ++x, ++vertex) {
        *vertex = NTempest::C3Vector((&bounds.b)[x].x, (&bounds.b)[y].y, (&bounds.b)[z].z);
      }
    }
  }

  collision->indices.Set(36, vertIndices);

  ComputeSurfaceNormals(collision, 12);
  collision->extents = bounds;

  return CREATEHANDLE(HCOLLISIONDATA, collision);
}

HCOLLISIONDATA CollisionDataCreate(const MDLDATA &data) {
  if (!data.collision.vertices.Count()) {
    return 0;
  }

  CCollisionData *collision = NEWHANDLE(HCOLLISIONDATA, CCollisionData);
  if (!collision) {
    return 0;
  }

  collision->vertices = data.collision.vertices;
  collision->indices = data.collision.triIndices;
  collision->surfaceNormals = data.collision.facetNormals;
  collision->extents = NTempest::CAaBox::Bounding(collision->vertices.Ptr(), collision->vertices.Count());

  return CREATEHANDLE(HCOLLISIONDATA, collision);
}

void ModelCustGeosetAdd(
    HMODEL                    model,
    const NTempest::C3Vector &modelSpacePosition,
    void (*renderCallback)(HMODEL, const NTempest::C34Matrix &, LPVOID),
    LPVOID renderParam,
    UINT  *custGeosetId
);
void ModelCustGeosetRemove(HMODEL model, UINT custGeosetId);

HCOLLISIONDATA CollisionDataCreate(BYTE *fileData, UINT fileBytes) {
  fileData = MDLFileBinarySeek(fileData, fileBytes, 'DILC');
  if (!fileData) {
    return 0;
  }

  CCollisionData *collision = NEWHANDLE(HCOLLISIONDATA, CCollisionData);
  if (!collision) {
    return 0;
  }

  UINT sectionBytes = *(UINT *)fileData;
  fileData += 4;
  BYTE *sectionDone = fileData + sectionBytes;

  ASSERT(*((ULONG *) (fileData)) == 'XTRV');
  fileData += 4;

  UINT numVertices = *(UINT *)fileData;
  ASSERT(numVertices <= 0xffff);
  fileData += 4;

  collision->vertices.SetCount(numVertices);
  memcpy(collision->vertices.Ptr(), fileData, numVertices * sizeof(NTempest::C3Vector));
  fileData += collision->vertices.Bytes();

  ASSERT(*((ULONG *) (fileData)) == ' IRT');
  fileData += 4;

  UINT numVertIndices = *(UINT *)fileData;
  fileData += 4;
  collision->indices.SetCount(numVertIndices);
  memcpy(collision->indices.Ptr(), fileData, numVertIndices * sizeof(WORD));
  fileData += collision->indices.Bytes();

  ASSERT(*((ULONG *) (fileData)) == 'SMRN');
  fileData += 4;

  UINT numSurfaceNormals = *(UINT *)fileData;
  fileData += 4;
  collision->surfaceNormals.SetCount(numSurfaceNormals);
  memcpy(collision->surfaceNormals.Ptr(), fileData, numSurfaceNormals * sizeof(NTempest::C3Vector));
  fileData += collision->surfaceNormals.Bytes();

  ASSERT(fileData == sectionDone);
  collision->extents = NTempest::CAaBox::Bounding(collision->vertices.Ptr(), collision->vertices.Count());

  return CREATEHANDLE(HCOLLISIONDATA, collision);
}

void ModelAddCollisionFacets(
    HMODEL                             model,
    const NTempest::C34Matrix         &toWorld,
    const float                        scale,
    const NTempest::CAaBox            &worldBox,
    TSGrowableArray<NTempest::CFacet> *facets
) {
  CModelShared *shared;
  if (IModelDerefHandle((CModel *)model, &shared) && shared->collision) {
    CollisionDataAddFacets(shared->collision, toWorld, scale, worldBox, facets);
  }
}

int ModelCollisionVectorIntersect(
    HMODEL__                  *model,
    const NTempest::C34Matrix &basis,
    const NTempest::C3Vector  &p0,
    const NTempest::C3Vector  &p1,
    float                     &t
) {
  t = FLT_MAX;
  CModelShared *shared;
  if (IModelDerefHandle((CModel *)model, &shared) && shared->collision) {
    return CollisionDataVectorIntersect(shared->collision, basis, p0, p1, t);
  }
  return 0;
}

void ModelShowCollision(HMODEL model, int show) {
  CModel *modelptr = (CModel *)model;
  VALIDATEBEGIN;
  VALIDATE(modelptr);
  VALIDATEENDVOID;

  CModelBase   *unique;
  CModelShared *shared;
  if (!IModelDerefHandle(modelptr, &unique, &shared)) {
    EnqueueModelCommand(modelptr, MODEL_SHOW_COLLISION, show);
    return;
  }

  if (show) {
    if (shared->collision && !unique->m_collideModel) {
      unique->m_collideModel = CollisionDataCreateModel(shared->collision);
    }
  } else if (unique->m_collideModel) {
    HandleClose(unique->m_collideModel);
    unique->m_collideModel = 0;
  }
}

void ModelShowCollisionAaBox(HMODEL model, int show) {
  CModel *modelptr = (CModel *)model;
  VALIDATEBEGIN;
  VALIDATE(modelptr);
  VALIDATEENDVOID;

  CModelBase   *unique;
  CModelShared *shared;
  if (!IModelDerefHandle(modelptr, &unique, &shared)) {
    EnqueueModelCommand(modelptr, MODEL_SHOW_COLLISION_AABOX, show);
    return;
  }

  if (!shared->collision) {
    return;
  }

  if (show) {
    if (unique->m_aaBoxCustGeoId == (UINT)-1) {
      ModelCustGeosetAdd(model, NTempest::C3Vector(0.0f), CollisionDataAABoxRenderCallback, 0, &unique->m_aaBoxCustGeoId);
    }
  } else if (unique->m_aaBoxCustGeoId != (UINT)-1) {
    ModelCustGeosetRemove(model, unique->m_aaBoxCustGeoId);
    unique->m_aaBoxCustGeoId = -1;
  }
}

void ModelShowModel(HMODEL model, int show) {
  CModel *modelptr = (CModel *)model;
  VALIDATEBEGIN;
  VALIDATE(modelptr);
  VALIDATEENDVOID;

  CModelBase *unique;
  if (!IModelDerefHandle(modelptr, &unique)) {
    EnqueueModelCommand(modelptr, MODEL_SHOW_MODEL, show);
    return;
  }

  if (show) {
    unique->m_flags &= ~0x10u;
  } else {
    unique->m_flags |= 0x10;
  }
}

void ModelGetCollisionExtents(HMODEL model, NTempest::CAaBox *extents) {
  CModelShared *shared;
  if (IModelDerefHandle((CModel *)model, &shared) && shared->collision) {
    *extents = ((CCollisionData *)shared->collision)->extents;
  } else {
    *extents = NTempest::CAaBox(0.0f);
  }
}
