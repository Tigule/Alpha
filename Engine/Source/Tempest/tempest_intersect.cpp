#include "Tempest/tempest_intersect.h"

#include "Tempest/c3ray.h"
#include "Tempest/cfacet.h"

namespace NTempest {

  bool __fastcall Intersect(const C3Ray &ray, const CFacet &facet, float *t, C3Vector *p) {
    float denom = C3Vector::Dot(facet.plane.n, ray.dir);
    if (denom == 0.0f) {
      return false;
    }

    float    thisT = -(C3Vector::Dot(facet.plane.n, ray.origin) + facet.plane.d) / denom;
    C3Vector point(ray.origin.x + ray.dir.x * thisT, ray.origin.y + ray.dir.y * thisT, ray.origin.z + ray.dir.z * thisT);
    C3Vector edge0 = facet.vertices[1] - facet.vertices[0];
    C3Vector edge1 = facet.vertices[2] - facet.vertices[1];
    C3Vector edge2 = facet.vertices[0] - facet.vertices[2];
    C3Vector side0 = point - facet.vertices[0];
    C3Vector side1 = point - facet.vertices[1];
    C3Vector side2 = point - facet.vertices[2];

    if (C3Vector::Dot(C3Vector::Cross(edge0, side0), facet.plane.n) < 0.0f || C3Vector::Dot(C3Vector::Cross(edge1, side1), facet.plane.n) < 0.0f ||
        C3Vector::Dot(C3Vector::Cross(edge2, side2), facet.plane.n) < 0.0f)
    {
      return false;
    }

    if (t) {
      *t = thisT;
    }
    if (p) {
      *p = point;
    }
    return true;
  }

}  // namespace NTempest
