#pragma once

#include "Model/IModel.h"
#include "Tempest/caabox.h"
#include "Tempest/c34matrix.h"
#include "Tempest/cfacet.h"

#include <stpl.h>

struct HCOLLISIONDATA__;
typedef HCOLLISIONDATA__ *HCOLLISIONDATA;

struct CCollisionData : public CHandleObject {
  TSFixedArray<NTempest::C3Vector> vertices;
  TSFixedArray<WORD>               indices;
  TSFixedArray<NTempest::C3Vector> surfaceNormals;
  NTempest::CAaBox                 extents;
};

HCOLLISIONDATA CollisionDataCreate(const NTempest::CAaBox &bounds);
HMODEL         CollisionDataCreateModel(HCOLLISIONDATA collide);
void           CollisionDataAABoxRenderCallback(HMODEL model, const NTempest::C34Matrix &basis, LPVOID param);
void           ModelGetCollisionExtents(HMODEL model, NTempest::CAaBox *extents);
int            ModelCollisionVectorIntersect(
    HMODEL                     model,
    const NTempest::C34Matrix &basis,
    const NTempest::C3Vector  &p0,
    const NTempest::C3Vector  &p1,
    float                     &t
);
void ModelAddCollisionFacets(
    HMODEL                             model,
    const NTempest::C34Matrix         &toWorld,
    const float                        scale,
    const NTempest::CAaBox            &worldBox,
    TSGrowableArray<NTempest::CFacet> *facets
);
