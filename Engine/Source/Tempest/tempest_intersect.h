#ifndef ENGINE_SOURCE_TEMPEST_TEMPEST_INTERSECT_H
#define ENGINE_SOURCE_TEMPEST_TEMPEST_INTERSECT_H

namespace NTempest {

  class C3Ray;
  class C3Vector;
  struct CFacet;

  bool __fastcall Intersect(const C3Ray &ray, const CFacet &facet, float *t, C3Vector *p);

}  // namespace NTempest

#endif
