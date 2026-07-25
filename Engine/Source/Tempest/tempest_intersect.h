#ifndef ENGINE_SOURCE_TEMPEST_TEMPEST_INTERSECT_H
#define ENGINE_SOURCE_TEMPEST_TEMPEST_INTERSECT_H

#include "Tempest/c3vector.h"

namespace NTempest {

  class C3Ray;
  class C2Vector;
  class C4Plane;
  struct CFacet;

  bool __fastcall Intersect(const C3Ray &ray, const C4Plane &plane, float *t, C3Vector *p);
  bool __fastcall Intersect(const C3Vector &point, const C3Vector *polygon, unsigned int nPoints, C3Vector::EAxis axis);
  bool __fastcall Intersect(const C3Ray &ray, const CFacet &facet, float *t, C3Vector *p);
  bool __fastcall Intersect(const C2Vector &a0, const C2Vector &a1, const C2Vector &b0, const C2Vector &b1);

}  // namespace NTempest

#endif
