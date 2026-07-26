#ifndef ENGINE_SOURCE_TEMPEST_TEMPEST_INTERSECT_H
#define ENGINE_SOURCE_TEMPEST_TEMPEST_INTERSECT_H

#include "Tempest/c3vector.h"

namespace NTempest {

  class C3Ray;
  class C2Vector;
  class C4Plane;
  class CAaBox;
  class CAaSphere;
  class CCone;
  class CObBox;
  struct CFacet;

  enum SolidIntersect {
    SI_HollowHollow,
    SI_HollowSolid,
    SI_SolidHollow,
    SI_SolidSolid
  };

  bool __fastcall Intersect(const C3Ray &ray, const CAaBox &box, float *t, C3Vector *p);
  bool __fastcall Intersect(const C3Ray &ray, const CAaSphere &sphere, float *t, C3Vector *p);
  bool __fastcall Intersect(const CAaBox &box, const CAaSphere &sphere, SolidIntersect mode);
  bool __fastcall Intersect2d(const CAaBox &box, const CAaSphere &sphere, SolidIntersect mode);
  bool __fastcall Intersect(const C3Ray &ray, const C4Plane &plane, float *t, C3Vector *p);
  bool __fastcall Intersect(const C3Vector &point, const C3Vector *polygon, unsigned int nPoints, C3Vector::EAxis axis);
  bool __fastcall Intersect(const C3Vector &point, const C3Vector *polygon, const unsigned short *indices, unsigned int nPoints, C3Vector::EAxis axis);
  bool __fastcall Intersect(const C3Vector &point, const C3Vector *polygon, const unsigned long *indices, unsigned int nPoints, C3Vector::EAxis axis);
  bool __fastcall Intersect(const C3Ray &ray, const CFacet &facet, float *t, C3Vector *p);
  bool __fastcall Intersect(const C3Ray &ray, const C3Vector *verts, float *t, C2Vector *bary);
  bool __fastcall IntersectCull(const C3Ray &ray, const C3Vector *verts, float *t, C2Vector *bary);
  bool __fastcall Intersect(const C3Vector &point, const CCone &cone);
  bool __fastcall Intersect(const C3Ray &ray, const CCone &cone, float *t, C3Vector *p);
  bool __fastcall Intersect(const CObBox &a, const CObBox &b);
  bool __fastcall Intersect(const C2Vector &a0, const C2Vector &a1, const C2Vector &b0, const C2Vector &b1);
  bool __fastcall Intersect(const C2Vector &a0, const C2Vector &a1, const C2Vector &b0, const C2Vector &b1, C2Vector &point);
  bool __fastcall Intersect(const CFacet &facet0, const CFacet &facet1);

}  // namespace NTempest

#endif
