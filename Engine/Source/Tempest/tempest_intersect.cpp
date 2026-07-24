#include "Tempest/tempest_intersect.h"

#include "Tempest/c2ivector.h"
#include "Tempest/c3ray.h"
#include "Tempest/c4plane.h"
#include "Tempest/cfacet.h"
#include "Tempest/c2vector.h"

NTempest::C2iVector projAxisTable[3] = {
    NTempest::C2iVector(1, 2),
    NTempest::C2iVector(2, 0),
    NTempest::C2iVector(0, 1)
};

namespace NTempest {

  bool __fastcall Intersect(const C3Ray &ray, const C4Plane &plane, float *t, C3Vector *p) {
    float denom = C3Vector::Dot(plane.n, ray.dir);
    if (CMath::fabs_(denom) < 0.0001f) {
      if (CMath::fabs_(C3Vector::Dot(plane.n, ray.origin) + plane.d) >= 0.01f) {
        return false;
      }

      if (t) {
        *t = 0.0f;
      }
      if (p) {
        *p = ray.origin;
      }
      return true;
    }

    if (t || p) {
      float distance = C3Vector::Dot(plane.n, ray.origin) + plane.d;
      float thisT = CMath::fabs_(distance) >= 0.01f ? -(distance / denom) : 0.0f;
      if (t) {
        *t = thisT;
      }
      if (p) {
        *p = ray.origin + ray.dir * thisT;
      }
    }
    return true;
  }

  bool __fastcall Intersect(const C3Vector &point, const C3Vector *polygon, unsigned int nPoints, C3Vector::EAxis axis) {
    FATALASSERT(axis <= C3Vector::C3AXIS_Z);

    unsigned int x = projAxisTable[axis].x;
    unsigned int y = projAxisTable[axis].y;
    bool         inside = false;
    bool         y0 = polygon[nPoints - 1][y] >= point[y];
    unsigned int previous = nPoints - 1;

    for (unsigned int i = 0; i < nPoints; ++i) {
      bool y1 = polygon[i][y] >= point[y];
      if (y0 != y1 &&
          (((polygon[previous][y] - polygon[i][y]) * (polygon[i][x] - point[x]) <=
            (polygon[previous][x] - polygon[i][x]) * (polygon[i][y] - point[y])) == y1))
      {
        inside = !inside;
      }

      y0 = y1;
      previous = i;
    }

    return inside;
  }

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

static int EdgeIntersectTriEdge(NTempest::C2Vector& a0, NTempest::C2Vector& a1, NTempest::C2Vector& b0, NTempest::C2Vector& b1, NTempest::C2Vector& b2) {
    // TODO: implement
    return 0;
}

static int PointInTri(NTempest::C2Vector& p, NTempest::C2Vector& a0, NTempest::C2Vector& a1, NTempest::C2Vector& a2) {
    // TODO: implement
    return 0;
}

static unsigned char CoplanarTriIntersectTri(const NTempest::CFacet& facet0, const NTempest::CFacet& facet1) {
    // TODO: implement
    return 0;
}
