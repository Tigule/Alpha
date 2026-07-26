#include "Tempest/tempest_intersect.h"

#include "Tempest/c2ivector.h"
#include "Tempest/caabox.h"
#include "Tempest/caasphere.h"
#include "Tempest/ccone.h"
#include "Tempest/cobbox.h"
#include "Tempest/c3ray.h"
#include "Tempest/c4plane.h"
#include "Tempest/cfacet.h"
#include "Tempest/c2vector.h"

#include <math.h>

NTempest::C2iVector projAxisTable[3] = {
    NTempest::C2iVector(1, 2),
    NTempest::C2iVector(2, 0),
    NTempest::C2iVector(0, 1)
};

namespace NTempest {

  bool __fastcall Intersect(const C3Ray &ray, const CAaBox &box, float *t, C3Vector *p) {
    float localT;
    C3Vector localP;
    if (!t) {
      t = &localT;
    }
    if (!p) {
      p = &localP;
    }

    unsigned char quadrant[3];
    float candidate[3];
    bool inside = true;
    for (unsigned int i = 0; i < 3; ++i) {
      if (ray.origin[i] < box.b[i]) {
        quadrant[i] = 1;
        candidate[i] = box.b[i];
        inside = false;
      } else if (ray.origin[i] > box.t[i]) {
        quadrant[i] = 0;
        candidate[i] = box.t[i];
        inside = false;
      } else {
        quadrant[i] = 2;
      }
    }

    if (inside) {
      *t = 0.0f;
      *p = ray.origin;
      return true;
    }

    float maxT[3];
    for (unsigned int axis = 0; axis < 3; ++axis) {
      maxT[axis] = quadrant[axis] == 2 || ray.dir[axis] == 0.0f
          ? -1.0f
          : (candidate[axis] - ray.origin[axis]) / ray.dir[axis];
    }

    unsigned int plane = 0;
    if (maxT[1] > maxT[plane]) {
      plane = 1;
    }
    if (maxT[2] > maxT[plane]) {
      plane = 2;
    }
    if (maxT[plane] < 0.0f) {
      return false;
    }

    *t = maxT[plane];
    for (unsigned int component = 0; component < 3; ++component) {
      if (component == plane) {
        (*p)[component] = candidate[component];
      } else {
        (*p)[component] = ray.origin[component] + maxT[plane] * ray.dir[component];
        if ((*p)[component] < box.b[component] || (*p)[component] > box.t[component]) {
          return false;
        }
      }
    }
    return true;
  }

  bool __fastcall Intersect(const C3Ray &ray, const CAaSphere &sphere, float *t, C3Vector *p) {
    C3Vector toCenter = sphere.c - ray.origin;
    float projection = C3Vector::Dot(toCenter, ray.dir);
    float centerDistanceSquared = toCenter.SquaredMag();
    float radiusSquared = sphere.r * sphere.r;

    if (projection < 0.0f && centerDistanceSquared > radiusSquared) {
      return false;
    }

    float perpendicularSquared = centerDistanceSquared - projection * projection;
    if (perpendicularSquared > radiusSquared) {
      return false;
    }

    if (t) {
      float offset = static_cast<float>(sqrt(radiusSquared - perpendicularSquared));
      *t = centerDistanceSquared <= radiusSquared ? projection + offset : projection - offset;
      if (p) {
        *p = ray.origin + ray.dir * *t;
      }
    }
    return true;
  }

  bool __fastcall Intersect(const CAaBox &box, const CAaSphere &sphere, SolidIntersect mode) {
    float radiusSquared = sphere.r * sphere.r;
    float minDistanceSquared = 0.0f;
    float maxDistanceSquared = 0.0f;
    bool boundaryReachable = false;

    for (unsigned int i = 0; i < 2; ++i) {
      float bottomDistance = sphere.c[i] - box.b[i];
      float topDistance = sphere.c[i] - box.t[i];
      float bottomSquared = bottomDistance * bottomDistance;
      float topSquared = topDistance * topDistance;
      maxDistanceSquared += bottomSquared > topSquared ? bottomSquared : topSquared;

      if (sphere.c[i] < box.b[i]) {
        minDistanceSquared += bottomSquared;
        boundaryReachable = true;
      } else if (sphere.c[i] > box.t[i]) {
        minDistanceSquared += topSquared;
        boundaryReachable = true;
      } else if (bottomSquared <= radiusSquared || topSquared <= radiusSquared) {
        boundaryReachable = true;
      }
    }

    switch (mode) {
      case SI_HollowHollow:
        return boundaryReachable && minDistanceSquared <= radiusSquared && radiusSquared <= maxDistanceSquared;
      case SI_HollowSolid:
        return boundaryReachable && minDistanceSquared <= radiusSquared;
      case SI_SolidHollow:
        return minDistanceSquared <= radiusSquared && radiusSquared <= maxDistanceSquared;
      case SI_SolidSolid:
        return minDistanceSquared <= radiusSquared;
      default:
        FATALASSERT(0);
        return false;
    }
  }

  bool __fastcall Intersect2d(const CAaBox &box, const CAaSphere &sphere, SolidIntersect mode) {
    return Intersect(box, sphere, mode);
  }

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

  bool __fastcall Intersect(
      const C3Vector &point, const C3Vector *polygon, const unsigned short *indices, unsigned int nPoints, C3Vector::EAxis axis
  ) {
    FATALASSERT(axis <= C3Vector::C3AXIS_Z);
    unsigned int x = projAxisTable[axis].x;
    unsigned int y = projAxisTable[axis].y;
    bool inside = false;
    unsigned int previous = indices[nPoints - 1];
    bool y0 = polygon[previous][y] >= point[y];

    for (unsigned int i = 0; i < nPoints; ++i) {
      unsigned int current = indices[i];
      bool y1 = polygon[current][y] >= point[y];
      if (y0 != y1 &&
          (((polygon[previous][y] - polygon[current][y]) * (polygon[current][x] - point[x]) <=
            (polygon[previous][x] - polygon[current][x]) * (polygon[current][y] - point[y])) == y1)) {
        inside = !inside;
      }
      y0 = y1;
      previous = current;
    }
    return inside;
  }

  bool __fastcall Intersect(
      const C3Vector &point, const C3Vector *polygon, const unsigned long *indices, unsigned int nPoints, C3Vector::EAxis axis
  ) {
    ASSERT(polygon);
    ASSERT(indices);
    FATALASSERT(axis <= C3Vector::C3AXIS_Z);
    unsigned int x = projAxisTable[axis].x;
    unsigned int y = projAxisTable[axis].y;
    bool inside = false;
    unsigned long previous = indices[nPoints - 1];
    bool y0 = polygon[previous][y] >= point[y];

    for (unsigned int i = 0; i < nPoints; ++i) {
      unsigned long current = indices[i];
      bool y1 = polygon[current][y] >= point[y];
      if (y0 != y1 &&
          (((polygon[previous][y] - polygon[current][y]) * (polygon[current][x] - point[x]) <=
            (polygon[previous][x] - polygon[current][x]) * (polygon[current][y] - point[y])) == y1)) {
        inside = !inside;
      }
      y0 = y1;
      previous = current;
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

  bool __fastcall Intersect(const C3Ray &ray, const C3Vector *verts, float *t, C2Vector *bary) {
    ASSERT(verts);
    C3Vector edge1 = verts[1] - verts[0];
    C3Vector edge2 = verts[2] - verts[0];
    C3Vector pvec = C3Vector::Cross(ray.dir, edge2);
    float determinant = C3Vector::Dot(edge1, pvec);
    if (determinant > -0.000001f && determinant < 0.000001f) {
      return false;
    }

    float inverseDeterminant = 1.0f / determinant;
    C3Vector tvec = ray.origin - verts[0];
    float u = C3Vector::Dot(tvec, pvec) * inverseDeterminant;
    if (u < 0.0f || u > 1.0f) {
      return false;
    }

    C3Vector qvec = C3Vector::Cross(tvec, edge1);
    float v = C3Vector::Dot(ray.dir, qvec) * inverseDeterminant;
    if (v < 0.0f || u + v > 1.0f) {
      return false;
    }

    if (t) {
      *t = C3Vector::Dot(edge2, qvec) * inverseDeterminant;
    }
    if (bary) {
      bary->x = u;
      bary->y = v;
    }
    return true;
  }

  bool __fastcall IntersectCull(const C3Ray &ray, const C3Vector *verts, float *t, C2Vector *bary) {
    ASSERT(verts);
    C3Vector edge1 = verts[1] - verts[0];
    C3Vector edge2 = verts[2] - verts[0];
    C3Vector pvec = C3Vector::Cross(ray.dir, edge2);
    float determinant = C3Vector::Dot(edge1, pvec);
    if (determinant < 0.000001f) {
      return false;
    }

    C3Vector tvec = ray.origin - verts[0];
    float u = C3Vector::Dot(tvec, pvec);
    if (u < 0.0f || u > determinant) {
      return false;
    }

    C3Vector qvec = C3Vector::Cross(tvec, edge1);
    float v = C3Vector::Dot(ray.dir, qvec);
    if (v < 0.0f || u + v > determinant) {
      return false;
    }
    if (!t && !bary) {
      return true;
    }

    float inverseDeterminant = 1.0f / determinant;
    if (t) {
      *t = C3Vector::Dot(edge2, qvec) * inverseDeterminant;
    }
    if (bary) {
      bary->x = u * inverseDeterminant;
      bary->y = v * inverseDeterminant;
    }
    return true;
  }

  bool __fastcall Intersect(const C3Vector &point, const CCone &cone) {
    C3Vector offset = point - cone.position;
    float axialDistance = C3Vector::Dot(offset, cone.axis);
    bool inside = cone.CosAngle() * cone.CosAngle() * offset.SquaredMag() <= axialDistance * axialDistance;
    return cone.height == 0.0f ? inside : axialDistance <= cone.height && inside;
  }

  bool __fastcall Intersect(const C3Ray &ray, const CCone &cone, float *t, C3Vector *p) {
    float localT;
    C3Vector localP;
    if (!t) {
      t = &localT;
    }
    if (!p) {
      p = &localP;
    }

    float directionProjection = C3Vector::Dot(ray.dir, cone.axis);
    float directionSquared = ray.dir.SquaredMag();
    float cosineSquared = cone.CosAngle() * cone.CosAngle();
    C3Vector offset = ray.origin - cone.position;
    float offsetProjection = C3Vector::Dot(offset, cone.axis);
    float c2 = directionProjection * directionProjection - cosineSquared * directionSquared;
    float c1 = directionProjection * offsetProjection - cosineSquared * C3Vector::Dot(offset, ray.dir);
    float c0 = offsetProjection * offsetProjection - cosineSquared * offset.SquaredMag();

    if (CMath::fabs_(c2) >= 0.000001f) {
      float discriminant = c1 * c1 - c0 * c2;
      if (discriminant < 0.0f) {
        return false;
      }
      if (discriminant > 0.0f) {
        float root = static_cast<float>(sqrt(discriminant));
        float inverseC2 = 1.0f / c2;
        float t0 = (-c1 - root) * inverseC2;
        float t1 = (root - c2) * inverseC2;
        *t = t0 < 0.0f ? 3.4028235e38f : t0;
        if (t1 >= 0.0f && t1 < *t) {
          *t = t1;
        }
        *p = ray.origin + ray.dir * *t;
      } else {
        *t = c1 / c2;
        *p = ray.origin - ray.dir * *t;
      }
    } else if (CMath::fabs_(c1) >= 0.000001f) {
      *t = c0 * (0.5f / c1);
      *p = ray.origin - ray.dir * *t;
    } else {
      if (CMath::fabs_(c0) >= 0.000001f) {
        return false;
      }
      *p = ray.origin;
      *t = 0.0f;
      return true;
    }

    float axialDistance = C3Vector::Dot(*p - cone.position, cone.axis);
    if (axialDistance <= 0.0f) {
      return false;
    }
    return cone.height == 0.0f || axialDistance < cone.height;
  }

  bool __fastcall Intersect(const CObBox &a, const CObBox &b) {
    float c[3][3];
    float absC[3][3];
    const C3Vector aAxis[3] = {a.b.Row0(), a.b.Row1(), a.b.Row2()};
    const C3Vector bAxis[3] = {b.b.Row0(), b.b.Row1(), b.b.Row2()};
    C3Vector difference = b.c - a.c;
    float projectedDifference[3];

    for (unsigned int i = 0; i < 3; ++i) {
      projectedDifference[i] = C3Vector::Dot(difference, aAxis[i]);
      for (unsigned int j = 0; j < 3; ++j) {
        c[i][j] = C3Vector::Dot(aAxis[i], bAxis[j]);
        absC[i][j] = CMath::fabs_(c[i][j]);
      }
      if (CMath::fabs_(projectedDifference[i]) >
          a.e[i] + b.e.x * absC[i][0] + b.e.y * absC[i][1] + b.e.z * absC[i][2]) {
        return false;
      }
    }

    for (unsigned int bComponent = 0; bComponent < 3; ++bComponent) {
      float projected = C3Vector::Dot(difference, bAxis[bComponent]);
      if (CMath::fabs_(projected) >
          b.e[bComponent] + a.e.x * absC[0][bComponent] + a.e.y * absC[1][bComponent] + a.e.z * absC[2][bComponent]) {
        return false;
      }
    }

    for (unsigned int crossA = 0; crossA < 3; ++crossA) {
      unsigned int i1 = (crossA + 1) % 3;
      unsigned int i2 = (crossA + 2) % 3;
      for (unsigned int crossB = 0; crossB < 3; ++crossB) {
        unsigned int j1 = (crossB + 1) % 3;
        unsigned int j2 = (crossB + 2) % 3;
        float projected = CMath::fabs_(c[i2][crossB] * projectedDifference[i1] - c[i1][crossB] * projectedDifference[i2]);
        float radius = a.e[i1] * absC[i2][crossB] + a.e[i2] * absC[i1][crossB]
                     + b.e[j1] * absC[crossA][j2] + b.e[j2] * absC[crossA][j1];
        if (projected > radius) {
          return false;
        }
      }
    }
    return true;
  }

}  // namespace NTempest

bool __fastcall NTempest::Intersect(
    const C2Vector &a0, const C2Vector &a1, const C2Vector &b0, const C2Vector &b1
) {
  float ax = a1.x - a0.x;
  float ay = a1.y - a0.y;
  float bx = b0.x - b1.x;
  float by = b0.y - b1.y;
  float cx = a0.x - b0.x;
  float cy = a0.y - b0.y;
  float f = bx * ay - by * ax;
  float d = cx * by - cy * bx;
  if ((f > 0.0f && d >= 0.0f && d <= f) || (f < 0.0f && d <= 0.0f && d >= f)) {
    float e = cy * ax - cx * ay;
    return f > 0.0f ? e >= 0.0f && e <= f : e <= 0.0f && e >= f;
  }
  return false;
}

bool __fastcall NTempest::Intersect(
    const C2Vector &a0, const C2Vector &a1, const C2Vector &b0, const C2Vector &b1, C2Vector &point
) {
  float denominator =
      (a0.y - a1.y) * b0.x +
      (b0.y - b1.y) * a1.x +
      (a1.y - a0.y) * b1.x +
      (b1.y - b0.y) * a0.x;
  if (CMath::fabs_(denominator) < 0.001f) {
    return false;
  }

  float inverse = 1.0f / denominator;
  float s =
      ((a0.y - b1.y) * b0.x +
       (b0.y - a0.y) * b1.x +
       (b1.y - b0.y) * a0.x) * inverse;
  float t =
      -((b0.y - a1.y) * a0.x +
        (a0.y - b0.y) * a1.x +
        (a1.y - a0.y) * b0.x) * inverse;
  point.x = a0.x + (a1.x - a0.x) * s;
  point.y = a0.y + (a1.y - a0.y) * s;
  return s >= 0.0f && s <= 1.0f && t >= 0.0f && t <= 1.0f;
}

static int EdgeIntersectTriEdge(NTempest::C2Vector& a0, NTempest::C2Vector& a1, NTempest::C2Vector& b0, NTempest::C2Vector& b1, NTempest::C2Vector& b2) {
  return NTempest::Intersect(a0, a1, b0, b1)
      || NTempest::Intersect(a0, a1, b1, b2)
      || NTempest::Intersect(a0, a1, b2, b0);
}

static int PointInTri(NTempest::C2Vector& p, NTempest::C2Vector& a0, NTempest::C2Vector& a1, NTempest::C2Vector& a2) {
  float ab = (a1.y - a0.y) * (p.x - a0.x) - (a1.x - a0.x) * (p.y - a0.y);
  float bc = (a2.y - a1.y) * (p.x - a1.x) - (a2.x - a1.x) * (p.y - a1.y);
  float ca = (a0.y - a2.y) * (p.x - a2.x) - (a0.x - a2.x) * (p.y - a2.y);
  return ab * bc >= 0.0f && ab * ca >= 0.0f;
}

static unsigned char CoplanarTriIntersectTri(const NTempest::CFacet& facet0, const NTempest::CFacet& facet1) {
  int i0;
  int i1;
  float nx = static_cast<float>(fabs(facet0.plane.n.x));
  float ny = static_cast<float>(fabs(facet0.plane.n.y));
  float nz = static_cast<float>(fabs(facet0.plane.n.z));
  if (nx > ny && nx > nz) {
    i0 = 1;
    i1 = 2;
  } else if (ny > nz) {
    i0 = 0;
    i1 = 2;
  } else {
    i0 = 0;
    i1 = 1;
  }

  NTempest::C2Vector v[3];
  NTempest::C2Vector u[3];
  for (int i = 0; i < 3; ++i) {
    v[i] = NTempest::C2Vector(facet0.vertices[i][i0], facet0.vertices[i][i1]);
    u[i] = NTempest::C2Vector(facet1.vertices[i][i0], facet1.vertices[i][i1]);
  }
  if (EdgeIntersectTriEdge(v[0], v[1], u[0], u[1], u[2])
      || EdgeIntersectTriEdge(v[1], v[2], u[0], u[1], u[2])
      || EdgeIntersectTriEdge(v[2], v[0], u[0], u[1], u[2])) {
    return 1;
  }
  return PointInTri(v[0], u[0], u[1], u[2]) || PointInTri(u[0], v[0], v[1], v[2]);
}

bool __fastcall NTempest::Intersect(const CFacet &facet0, const CFacet &facet1) {
  float distances[3];
  int sides[3];
  int counts[3] = {0, 0, 0};

  for (unsigned int i = 0; i < 3; ++i) {
    distances[i] = facet1.plane.DistSigned(facet0.vertices[i]);
    if (distances[i] > 0.01f) {
      sides[i] = 0;
    } else if (distances[i] < -0.01f) {
      sides[i] = 1;
    } else {
      sides[i] = 2;
    }
    ++counts[sides[i]];
  }

  if (counts[2] == 3) {
    return CoplanarTriIntersectTri(facet0, facet1) != 0;
  }

  C3Vector segment[2];
  unsigned int segmentCount = 0;
  for (unsigned int edge = 0; edge < 3; ++edge) {
    unsigned int next = edge == 2 ? 0 : edge + 1;
    if (sides[edge] != sides[next]) {
      float amount = distances[edge] / (distances[edge] - distances[next]);
      segment[segmentCount++] = facet0.vertices[edge] + (facet0.vertices[next] - facet0.vertices[edge]) * amount;
    }
  }

  C3Vector absoluteNormal(
      CMath::fabs_(facet0.plane.n.x),
      CMath::fabs_(facet0.plane.n.y),
      CMath::fabs_(facet0.plane.n.z)
  );
  int i0;
  int i1;
  if (absoluteNormal.x > absoluteNormal.y && absoluteNormal.x > absoluteNormal.z) {
    i0 = 1;
    i1 = 2;
  } else if (absoluteNormal.y > absoluteNormal.z) {
    i0 = 0;
    i1 = 2;
  } else {
    i0 = 0;
    i1 = 1;
  }

  C2Vector edge0(segment[0][i0], segment[0][i1]);
  C2Vector edge1(segment[1][i0], segment[1][i1]);
  C2Vector triangle[3] = {
      C2Vector(facet1.vertices[0][i0], facet1.vertices[0][i1]),
      C2Vector(facet1.vertices[1][i0], facet1.vertices[1][i1]),
      C2Vector(facet1.vertices[2][i0], facet1.vertices[2][i1])
  };
  return EdgeIntersectTriEdge(edge0, edge1, triangle[0], triangle[1], triangle[2]) != 0
      || PointInTri(edge0, triangle[0], triangle[1], triangle[2]) != 0;
}
