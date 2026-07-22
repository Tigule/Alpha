#include "Model/CollisionData.h"

#include "Tempest/c33matrix.h"
#include "Tempest/cfacet.h"

#include <string.h>

unsigned char *__fastcall MDLFileBinarySeek(unsigned char *fileData, unsigned int fileBytes, unsigned long sectionTag);

static const unsigned short vertIndices[36] = {4, 6, 0, 0, 6, 2, 4, 0, 5, 5, 0, 1, 0, 2, 1, 1, 2, 3,
                                               2, 6, 3, 3, 6, 7, 1, 3, 5, 5, 3, 7, 5, 7, 4, 4, 7, 6};

int __fastcall TriangleIsClippedOut(const NTempest::CAaBox &bounds, const NTempest::C3Vector *triVerts) {
  return (triVerts[0].x < bounds.b.x && triVerts[1].x < bounds.b.x && triVerts[2].x < bounds.b.x) ||
         (triVerts[0].x > bounds.t.x && triVerts[1].x > bounds.t.x && triVerts[2].x > bounds.t.x) ||
         (triVerts[0].y < bounds.b.y && triVerts[1].y < bounds.b.y && triVerts[2].y < bounds.b.y) ||
         (triVerts[0].y > bounds.t.y && triVerts[1].y > bounds.t.y && triVerts[2].y > bounds.t.y) ||
         (triVerts[0].z < bounds.b.z && triVerts[1].z < bounds.b.z && triVerts[2].z < bounds.b.z) ||
         (triVerts[0].z > bounds.t.z && triVerts[1].z > bounds.t.z && triVerts[2].z > bounds.t.z);
}

void __fastcall CollisionDataAddFacets(
    HCOLLISIONDATA                     handle,
    const NTempest::C34Matrix         &toWorld,
    float                              scale,
    const NTempest::CAaBox            &worldBox,
    TSGrowableArray<NTempest::CFacet> *facets
) {
  FATALASSERT(handle);
  FATALASSERT(facets);

  NTempest::C33Matrix rotation(
      toWorld.a0 * scale, toWorld.a1 * scale, toWorld.a2 * scale, toWorld.b0 * scale, toWorld.b1 * scale, toWorld.b2 * scale, toWorld.c0 * scale,
      toWorld.c1 * scale, toWorld.c2 * scale
  );
  unsigned int existing = facets->Count();
  unsigned int rejects = 0;
  unsigned int numFacets = reinterpret_cast<CCollisionData *>(handle)->surfaceNormals.Count();
  facets->SetCount(existing + numFacets);

  for (unsigned int i = 0; i < numFacets; ++i) {
    (*facets)[existing + i - rejects].vertices[0] =
        reinterpret_cast<CCollisionData *>(handle)->vertices[reinterpret_cast<CCollisionData *>(handle)->indices[i * 3]] * toWorld;
    (*facets)[existing + i - rejects].vertices[1] =
        reinterpret_cast<CCollisionData *>(handle)->vertices[reinterpret_cast<CCollisionData *>(handle)->indices[i * 3 + 1]] * toWorld;
    (*facets)[existing + i - rejects].vertices[2] =
        reinterpret_cast<CCollisionData *>(handle)->vertices[reinterpret_cast<CCollisionData *>(handle)->indices[i * 3 + 2]] * toWorld;

    if (TriangleIsClippedOut(worldBox, (*facets)[existing + i - rejects].vertices)) {
      ++rejects;
    } else {
      (*facets)[existing + i - rejects].plane.n.x = reinterpret_cast<CCollisionData *>(handle)->surfaceNormals[i].x * rotation.a0 +
                                                    reinterpret_cast<CCollisionData *>(handle)->surfaceNormals[i].y * rotation.b0 +
                                                    reinterpret_cast<CCollisionData *>(handle)->surfaceNormals[i].z * rotation.c0;
      (*facets)[existing + i - rejects].plane.n.y = reinterpret_cast<CCollisionData *>(handle)->surfaceNormals[i].x * rotation.a1 +
                                                    reinterpret_cast<CCollisionData *>(handle)->surfaceNormals[i].y * rotation.b1 +
                                                    reinterpret_cast<CCollisionData *>(handle)->surfaceNormals[i].z * rotation.c1;
      (*facets)[existing + i - rejects].plane.n.z = reinterpret_cast<CCollisionData *>(handle)->surfaceNormals[i].x * rotation.a2 +
                                                    reinterpret_cast<CCollisionData *>(handle)->surfaceNormals[i].y * rotation.b2 +
                                                    reinterpret_cast<CCollisionData *>(handle)->surfaceNormals[i].z * rotation.c2;
      (*facets)[existing + i - rejects].plane.d =
          -NTempest::C3Vector::Dot((*facets)[existing + i - rejects].plane.n, (*facets)[existing + i - rejects].vertices[0]);
    }
  }

  facets->SetCount(existing + numFacets - rejects);
}

HCOLLISIONDATA __fastcall CollisionDataCreate(const NTempest::CAaBox &bounds) {
  void           *storage = SMemAlloc(sizeof(CCollisionData), "HCOLLISIONDATA", -2, 0);
  CCollisionData *collision = storage ? new (storage) CCollisionData : 0;
  if (!collision) {
    return 0;
  }

  collision->vertices.SetCount(8);
  unsigned int vertex = 0;
  for (unsigned int z = 0; z < 2; ++z) {
    for (unsigned int y = 0; y < 2; ++y) {
      for (unsigned int x = 0; x < 2; ++x) {
        collision->vertices[vertex++] = NTempest::C3Vector(x ? bounds.t.x : bounds.b.x, y ? bounds.t.y : bounds.b.y, z ? bounds.t.z : bounds.b.z);
      }
    }
  }

  collision->indices.SetCount(36);
  memcpy(collision->indices.Ptr(), vertIndices, sizeof(vertIndices));

  collision->surfaceNormals.SetCount(12);
  for (unsigned int surface = 0; surface < 12; ++surface) {
    const NTempest::C3Vector &a = collision->vertices[collision->indices[surface * 3]];
    const NTempest::C3Vector &b = collision->vertices[collision->indices[surface * 3 + 1]];
    const NTempest::C3Vector &c = collision->vertices[collision->indices[surface * 3 + 2]];
    NTempest::C3Vector        ab(b.x - a.x, b.y - a.y, b.z - a.z);
    NTempest::C3Vector        ac(c.x - a.x, c.y - a.y, c.z - a.z);
    collision->surfaceNormals[surface] = NTempest::C3Vector::Cross(ab, ac);
    collision->surfaceNormals[surface].Normalize();
  }
  collision->extents = bounds;

  return reinterpret_cast<HCOLLISIONDATA>(HandleCreate(collision, "HCOLLISIONDATA"));
}

void __fastcall ModelCustGeosetAdd(
    HMODEL                    model,
    const NTempest::C3Vector &modelSpacePosition,
    void(__fastcall *renderCallback)(HMODEL, const NTempest::C34Matrix &, void *),
    void         *renderParam,
    unsigned int *custGeosetId
);
void __fastcall ModelCustGeosetRemove(HMODEL model, unsigned int custGeosetId);

HCOLLISIONDATA __fastcall CollisionDataCreate(unsigned char *fileData, unsigned int fileBytes) {
  unsigned char *section = MDLFileBinarySeek(fileData, fileBytes, 0x44494C43);
  if (!section) {
    return 0;
  }

  void *storage = SMemAlloc(sizeof(CCollisionData), "HCOLLISIONDATA", -2, 0);
  if (!storage) {
    return 0;
  }

  CCollisionData *collision = new (storage) CCollisionData;
  unsigned char  *data = section + 4;
  unsigned char  *sectionDone = data + *reinterpret_cast<unsigned int *>(section);

  ASSERT(*reinterpret_cast<unsigned int *>(data) == 0x58545256);
  data += 4;

  unsigned int numVertices = *reinterpret_cast<unsigned int *>(data);
  data += 4;
  ASSERT(numVertices <= 0xFFFF);

  collision->vertices.SetCount(numVertices);
  memcpy(collision->vertices.Ptr(), data, numVertices * sizeof(NTempest::C3Vector));
  data += numVertices * sizeof(NTempest::C3Vector);

  ASSERT(*reinterpret_cast<unsigned int *>(data) == 0x20495254);
  data += 4;

  unsigned int numVertIndices = *reinterpret_cast<unsigned int *>(data);
  data += 4;
  collision->indices.SetCount(numVertIndices);
  memcpy(collision->indices.Ptr(), data, numVertIndices * sizeof(unsigned short));
  data += numVertIndices * sizeof(unsigned short);

  ASSERT(*reinterpret_cast<unsigned int *>(data) == 0x534D524E);
  data += 4;

  unsigned int numSurfaceNormals = *reinterpret_cast<unsigned int *>(data);
  data += 4;
  collision->surfaceNormals.SetCount(numSurfaceNormals);
  memcpy(collision->surfaceNormals.Ptr(), data, numSurfaceNormals * sizeof(NTempest::C3Vector));
  data += numSurfaceNormals * sizeof(NTempest::C3Vector);

  ASSERT(data == sectionDone);
  collision->extents = NTempest::CAaBox::Bounding(collision->vertices.Ptr(), collision->vertices.Count());

  return reinterpret_cast<HCOLLISIONDATA>(HandleCreate(collision, "HCOLLISIONDATA"));
}

void __fastcall ModelShowCollision(HMODEL model, int show) {
  FATALASSERT(model);

  CModelBase   *unique;
  CModelShared *shared;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique, &shared)) {
    EnqueueModelCommand(reinterpret_cast<CModel *>(model), MODEL_SHOW_COLLISION, show);
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

void __fastcall ModelShowCollisionAaBox(HMODEL model, int show) {
  FATALASSERT(model);

  CModelBase   *unique;
  CModelShared *shared;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique, &shared)) {
    EnqueueModelCommand(reinterpret_cast<CModel *>(model), MODEL_SHOW_COLLISION_AABOX, show);
    return;
  }

  if (!shared->collision) {
    return;
  }

  if (show) {
    if (unique->m_aaBoxCustGeoId == static_cast<unsigned int>(-1)) {
      ModelCustGeosetAdd(model, NTempest::C3Vector(0.0f), CollisionDataAABoxRenderCallback, 0, &unique->m_aaBoxCustGeoId);
    }
  } else if (unique->m_aaBoxCustGeoId != static_cast<unsigned int>(-1)) {
    ModelCustGeosetRemove(model, unique->m_aaBoxCustGeoId);
    unique->m_aaBoxCustGeoId = static_cast<unsigned int>(-1);
  }
}

void __fastcall ModelShowModel(HMODEL model, int show) {
  FATALASSERT(model);

  CModelBase *unique;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    EnqueueModelCommand(reinterpret_cast<CModel *>(model), MODEL_SHOW_MODEL, show);
    return;
  }

  if (show) {
    unique->m_flags &= ~0x10u;
  } else {
    unique->m_flags |= 0x10;
  }
}

void __fastcall ModelGetCollisionExtents(HMODEL model, NTempest::CAaBox *extents) {
  CModelShared *shared;
  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &shared) && shared->collision) {
    *extents = reinterpret_cast<CCollisionData *>(shared->collision)->extents;
  } else {
    *extents = NTempest::CAaBox(0.0f);
  }
}

void __fastcall ModelAddCollisionFacets(
    HMODEL                             model,
    const NTempest::C34Matrix         &toWorld,
    float                              scale,
    const NTempest::CAaBox            &worldBox,
    TSGrowableArray<NTempest::CFacet> *facets
) {
  CModelShared *shared;
  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &shared) && shared->collision) {
    CollisionDataAddFacets(shared->collision, toWorld, scale, worldBox, facets);
  }
}
