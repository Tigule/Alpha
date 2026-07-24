#pragma once

#include "Model/ModelInternal.h"
#include "Tempest/cfacet.h"

struct CCollisionData : public CHandleObject {
  TSFixedArray<NTempest::C3Vector> vertices;
  TSFixedArray<unsigned short>     indices;
  TSFixedArray<NTempest::C3Vector> surfaceNormals;
  NTempest::CAaBox                 extents;
};

HCOLLISIONDATA __fastcall CollisionDataCreate(const NTempest::CAaBox &bounds);
HMODEL __fastcall         CollisionDataCreateModel(HCOLLISIONDATA collide);
void __fastcall           CollisionDataAABoxRenderCallback(HMODEL model, const NTempest::C34Matrix &basis, void *param);
void __fastcall           ModelGetCollisionExtents(HMODEL model, NTempest::CAaBox *extents);
int __fastcall            ModelCollisionVectorIntersect(
    HMODEL model, const NTempest::C34Matrix &basis, const NTempest::C3Vector &p0, const NTempest::C3Vector &p1, float &t
);
void __fastcall           ModelAddCollisionFacets(
    HMODEL                             model,
    const NTempest::C34Matrix         &toWorld,
    float                              scale,
    const NTempest::CAaBox            &worldBox,
    TSGrowableArray<NTempest::CFacet> *facets
);
void __fastcall CollisionDataAddFacets(
    HCOLLISIONDATA                     collide,
    const NTempest::C34Matrix         &toWorld,
    float                              scale,
    const NTempest::CAaBox            &worldBox,
    TSGrowableArray<NTempest::CFacet> *facets
);
int __fastcall TriangleIsClippedOut(const NTempest::CAaBox &bounds, const NTempest::C3Vector *triVerts);
