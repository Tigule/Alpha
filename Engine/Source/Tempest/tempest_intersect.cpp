#include <Base/Base.h>

#include "Tempest/tempest_intersect.h"

#include "Tempest/c2ivector.h"
#include "Tempest/caabox.h"
#include "Tempest/caasphere.h"
#include "Tempest/ccone.h"
#include "Tempest/cobbox.h"
#include "Tempest/c3ray.h"
#include "Tempest/c4plane.h"
#include "Tempest/cfacet.h"
#include "Tempest/cimvector.h"
#include "Tempest/c2vector.h"

#include <math.h>

static NTempest::C2iVector projAxisTable[3] = {NTempest::C2iVector(1, 2), NTempest::C2iVector(2, 0), NTempest::C2iVector(0, 1)};

namespace NTempest {

  bool Intersect(const C3Ray &ray, const CAaBox &box, float *t, C3Vector *p) {
    float    lt;
    C3Vector lp;
    float   *time = t ? t : &lt;
    float   *point = p ? &p->x : &lp.x;
    bool     inside = true;
    char     quadrant[3];
    float    candidate[3];
    float    maxT[3];
    int      plane;
    int      i;

    for (i = 0; i < 3; ++i) {
      if ((&ray.origin.x)[i] < (&box.b.x)[i]) {
        quadrant[i] = 1;
        candidate[i] = (&box.b.x)[i];
        inside = false;
      } else if ((&ray.origin.x)[i] > (&box.t.x)[i]) {
        quadrant[i] = 0;
        candidate[i] = (&box.t.x)[i];
        inside = false;
      } else {
        quadrant[i] = 2;
      }
    }

    if (inside) {
      *time = 0.0f;
      for (i = 0; i < 3; ++i) {
        point[i] = (&ray.origin.x)[i];
      }
      return true;
    }

    for (i = 0; i < 3; ++i) {
      if (quadrant[i] != 2 && (&ray.dir.x)[i] != 0.0f) {
        maxT[i] = (candidate[i] - (&ray.origin.x)[i]) / (&ray.dir.x)[i];
      } else {
        maxT[i] = -1.0f;
      }
    }

    plane = 0;
    for (i = 1; i < 3; ++i) {
      if (maxT[plane] < maxT[i]) {
        plane = i;
      }
    }

    if (maxT[plane] < 0.0f) {
      return false;
    }

    *time = maxT[plane];
    for (i = 0; i < 3; ++i) {
      if (plane != i) {
        point[i] = (&ray.origin.x)[i] + maxT[plane] * (&ray.dir.x)[i];
        if (point[i] < (&box.b.x)[i] || point[i] > (&box.t.x)[i]) {
          return false;
        }
      } else {
        point[i] = candidate[i];
      }
    }
    return true;
  }

  bool Intersect(const C3Ray &ray, const CAaSphere &sphere, float *t, C3Vector *p) {
    C3Vector toCenter = sphere.c - ray.origin;
    float    projection = C3Vector::Dot(toCenter, ray.dir);
    float    centerDistanceSquared = toCenter.SquaredMag();
    float    radiusSquared = sphere.r * sphere.r;

    if (projection < 0.0f && centerDistanceSquared > radiusSquared) {
      return false;
    }

    float perpendicularSquared = centerDistanceSquared - projection * projection;
    if (perpendicularSquared > radiusSquared) {
      return false;
    }

    if (t) {
      float offset = sqrt(radiusSquared - perpendicularSquared);
      if (centerDistanceSquared > radiusSquared) {
        *t = projection - offset;
      } else {
        *t = projection + offset;
      }
    }
    if (t && p) {
      *p = C3Vector(ray.dir.x * *t + ray.origin.x, ray.dir.y * *t + ray.origin.y, ray.dir.z * *t + ray.origin.z);
    }
    return true;
  }

  bool Intersect(const CAaBox &box, const CAaSphere &sphere, SolidIntersect mode) {
    float        r = sphere.r;
    float        r2 = r * r;
    const float *c = &sphere.c.x;
    const float *minB = &box.b.x;
    const float *maxB = &box.t.x;
    float        dmin;
    float        dmax;
    float        a;
    float        b;
    BOOL         face;
    UINT         i;

    switch (mode) {
      case SI_HollowHollow:
        dmin = 0.0f;
        dmax = 0.0f;
        face = 0;
        for (i = 0; i < 2; ++i) {
          a = (c[i] - minB[i]) * (c[i] - minB[i]);
          b = (c[i] - maxB[i]) * (c[i] - maxB[i]);
          dmax += max(a, b);
          if (c[i] < minB[i]) {
            face = 1;
            dmin += a;
          } else if (c[i] > maxB[i]) {
            face = 1;
            dmin += b;
          } else if (min(a, b) <= r2) {
            face = 1;
          }
        }
        if (face && dmin <= r2 && r2 <= dmax) {
          return true;
        }
        break;
      case SI_HollowSolid:
        dmin = 0.0f;
        face = 0;
        for (i = 0; i < 2; ++i) {
          if (c[i] < minB[i]) {
            face = 1;
            dmin += (c[i] - minB[i]) * (c[i] - minB[i]);
          } else if (c[i] > maxB[i]) {
            face = 1;
            dmin += (c[i] - maxB[i]) * (c[i] - maxB[i]);
          } else if (c[i] - minB[i] <= r) {
            face = 1;
          } else if (maxB[i] - c[i] <= r) {
            face = 1;
          }
        }
        if (face && dmin <= r2) {
          return true;
        }
        break;
      case SI_SolidHollow:
        dmax = 0.0f;
        dmin = 0.0f;
        for (i = 0; i < 2; ++i) {
          a = (c[i] - minB[i]) * (c[i] - minB[i]);
          b = (c[i] - maxB[i]) * (c[i] - maxB[i]);
          dmax += max(a, b);
          if (c[i] < minB[i]) {
            dmin += a;
          } else if (c[i] > maxB[i]) {
            dmin += b;
          }
        }
        if (dmin <= r2 && r2 <= dmax) {
          return true;
        }
        break;
      case SI_SolidSolid:
        dmin = 0.0f;
        for (i = 0; i < 2; ++i) {
          if (c[i] < minB[i]) {
            dmin += (c[i] - minB[i]) * (c[i] - minB[i]);
          } else if (c[i] > maxB[i]) {
            dmin += (c[i] - maxB[i]) * (c[i] - maxB[i]);
          }
        }
        if (dmin <= r2) {
          return true;
        }
        break;
      default:
        FATALASSERT(0);
        break;
    }
    return false;
  }

  bool Intersect2d(const CAaBox &box, const CAaSphere &sphere, SolidIntersect mode) {
    float        r = sphere.r;
    float        r2 = r * r;
    const float *c = &sphere.c.x;
    const float *minB = &box.b.x;
    const float *maxB = &box.t.x;
    float        dmin;
    float        dmax;
    float        a;
    float        b;
    BOOL         face;
    UINT         i;

    switch (mode) {
      case SI_HollowHollow:
        dmin = 0.0f;
        dmax = 0.0f;
        face = 0;
        for (i = 0; i < 2; ++i) {
          a = (c[i] - minB[i]) * (c[i] - minB[i]);
          b = (c[i] - maxB[i]) * (c[i] - maxB[i]);
          dmax += max(a, b);
          if (c[i] < minB[i]) {
            face = 1;
            dmin += a;
          } else if (c[i] > maxB[i]) {
            face = 1;
            dmin += b;
          } else if (min(a, b) <= r2) {
            face = 1;
          }
        }
        if (face && dmin <= r2 && r2 <= dmax) {
          return true;
        }
        break;
      case SI_HollowSolid:
        dmin = 0.0f;
        face = 0;
        for (i = 0; i < 2; ++i) {
          if (c[i] < minB[i]) {
            face = 1;
            dmin += (c[i] - minB[i]) * (c[i] - minB[i]);
          } else if (c[i] > maxB[i]) {
            face = 1;
            dmin += (c[i] - maxB[i]) * (c[i] - maxB[i]);
          } else if (c[i] - minB[i] <= r) {
            face = 1;
          } else if (maxB[i] - c[i] <= r) {
            face = 1;
          }
        }
        if (face && dmin <= r2) {
          return true;
        }
        break;
      case SI_SolidHollow:
        dmax = 0.0f;
        dmin = 0.0f;
        for (i = 0; i < 2; ++i) {
          a = (c[i] - minB[i]) * (c[i] - minB[i]);
          b = (c[i] - maxB[i]) * (c[i] - maxB[i]);
          dmax += max(a, b);
          if (c[i] < minB[i]) {
            dmin += a;
          } else if (c[i] > maxB[i]) {
            dmin += b;
          }
        }
        if (dmin <= r2 && r2 <= dmax) {
          return true;
        }
        break;
      case SI_SolidSolid:
        dmin = 0.0f;
        for (i = 0; i < 2; ++i) {
          if (c[i] < minB[i]) {
            dmin += (c[i] - minB[i]) * (c[i] - minB[i]);
          } else if (c[i] > maxB[i]) {
            dmin += (c[i] - maxB[i]) * (c[i] - maxB[i]);
          }
        }
        if (dmin <= r2) {
          return true;
        }
        break;
      default:
        FATALASSERT(0);
        break;
    }
    return false;
  }

  bool Intersect(const C3Ray &ray, const C4Plane &plane, float *t, C3Vector *p) {
    float denom = C3Vector::Dot(plane.n, ray.dir);
    if (CMath::fabs_(denom) < 0.0001) {
      if (CMath::fabs_(C3Vector::Dot(plane.n, ray.origin) + plane.d) < 0.01f) {
        if (t) {
          *t = 0.0f;
        }
        if (p) {
          *p = ray.origin;
        }
        return true;
      }
      return false;
    }

    if (t || p) {
      float distance = C3Vector::Dot(ray.origin, plane.n) + plane.d;
      float thisT = CMath::fabs_(distance) < 0.01f ? 0.0f : -(distance / denom);
      if (t) {
        *t = thisT;
      }
      if (p) {
        *p = ray.origin + ray.dir * thisT;
      }
    }
    return true;
  }

  bool Intersect(const C3Vector &point, const C3Vector *polygon, UINT nPoints, C3Vector::EAxis axis) {
    FATALASSERT(axis <= C3Vector::C3AXIS_Z);

    bool inside = false;
    UINT e0 = nPoints - 1;
    bool y0 = polygon[e0][projAxisTable[axis].y] >= point[projAxisTable[axis].y];

    for (UINT e1 = 0; e1 < nPoints; ++e1) {
      bool y1 = polygon[e1][projAxisTable[axis].y] >= point[projAxisTable[axis].y];
      if (y0 != y1) {
        if (((polygon[e1][projAxisTable[axis].y] - point[projAxisTable[axis].y]) * (polygon[e0][projAxisTable[axis].x] - polygon[e1][projAxisTable[axis].x]) >=
             (polygon[e1][projAxisTable[axis].x] - point[projAxisTable[axis].x]) * (polygon[e0][projAxisTable[axis].y] - polygon[e1][projAxisTable[axis].y])) == y1)
        {
          inside = !inside;
        }
      }

      y0 = y1;
      e0 = e1;
    }

    return inside;
  }

  bool Intersect(const C3Vector &point, const C3Vector *polygon, const WORD *indices, UINT nPoints, C3Vector::EAxis axis) {
    FATALASSERT(axis <= C3Vector::C3AXIS_Z);

    bool inside = false;
    UINT idx = 0;
    UINT e0 = indices[nPoints - 1];
    UINT e1 = indices[0];
    bool y0 = polygon[e0][projAxisTable[axis].y] >= point[projAxisTable[axis].y];

    for (; idx < nPoints; e0 = e1, e1 = indices[++idx]) {
      bool y1 = polygon[e1][projAxisTable[axis].y] >= point[projAxisTable[axis].y];
      if (y0 != y1) {
        if (((polygon[e1][projAxisTable[axis].y] - point[projAxisTable[axis].y]) * (polygon[e0][projAxisTable[axis].x] - polygon[e1][projAxisTable[axis].x]) >=
             (polygon[e1][projAxisTable[axis].x] - point[projAxisTable[axis].x]) * (polygon[e0][projAxisTable[axis].y] - polygon[e1][projAxisTable[axis].y])) == y1)
        {
          inside = !inside;
        }
      }

      y0 = y1;
    }

    return inside;
  }

  bool Intersect(const C3Vector &point, const C3Vector *polygon, const DWORD *indices, UINT nPoints, C3Vector::EAxis axis) {
    ASSERT(polygon);
    ASSERT(indices);
    FATALASSERT(axis <= C3Vector::C3AXIS_Z);

    bool inside = false;
    UINT idx = 0;
    UINT e0 = indices[nPoints - 1];
    UINT e1 = indices[0];
    bool y0 = polygon[e0][projAxisTable[axis].y] >= point[projAxisTable[axis].y];

    for (; idx < nPoints; e0 = e1, e1 = indices[++idx]) {
      bool y1 = polygon[e1][projAxisTable[axis].y] >= point[projAxisTable[axis].y];
      if (y0 != y1) {
        if (((polygon[e1][projAxisTable[axis].y] - point[projAxisTable[axis].y]) * (polygon[e0][projAxisTable[axis].x] - polygon[e1][projAxisTable[axis].x]) >=
             (polygon[e1][projAxisTable[axis].x] - point[projAxisTable[axis].x]) * (polygon[e0][projAxisTable[axis].y] - polygon[e1][projAxisTable[axis].y])) == y1)
        {
          inside = !inside;
        }
      }

      y0 = y1;
    }

    return inside;
  }

  bool Intersect(const C3Ray &ray, const CFacet &facet, float *t, C3Vector *p) {
    float    lt = 0.0f;
    C3Vector lp;
    if (Intersect(ray, facet.plane, &lt, &lp) && Intersect(lp, facet.vertices, 3, facet.plane.n.MajorAxis())) {
      if (t) {
        *t = lt;
      }
      if (p) {
        *p = lp;
      }
      return true;
    }
    return false;
  }

  bool Intersect(const C3Ray &ray, const C3Vector *verts, float *t, C2Vector *bary) {
    ASSERT(verts);
    C3Vector edge1 = verts[1] - verts[0];
    C3Vector edge2 = verts[2] - verts[0];
    C3Vector pvec = C3Vector::Cross(ray.dir, edge2);
    float    determinant = C3Vector::Dot(edge1, pvec);
    if (determinant > -0.000001f && determinant < 0.000001f) {
      return false;
    }

    float    inv_det = 1.0f / determinant;
    C3Vector tvec = ray.origin - verts[0];
    float    u = C3Vector::Dot(tvec, pvec) * inv_det;
    if (u < 0.0f || u > 1.0f) {
      return false;
    }

    C3Vector qvec = C3Vector::Cross(tvec, edge1);
    float    v = C3Vector::Dot(ray.dir, qvec) * inv_det;
    if (v < 0.0f || u + v > 1.0f) {
      return false;
    }

    if (t) {
      *t = C3Vector::Dot(edge2, qvec) * inv_det;
    }
    if (bary) {
      *bary = C2Vector(u, v);
    }
    return true;
  }

  bool IntersectCull(const C3Ray &ray, const C3Vector *verts, float *t, C2Vector *bary) {
    ASSERT(verts);
    C3Vector edge1 = verts[1] - verts[0];
    C3Vector edge2 = verts[2] - verts[0];
    C3Vector pvec = C3Vector::Cross(ray.dir, edge2);
    float    det = C3Vector::Dot(edge1, pvec);
    if (det < 0.000001f) {
      return false;
    }

    C3Vector tvec = ray.origin - verts[0];
    float    u = C3Vector::Dot(tvec, pvec);
    if (u < 0.0f || u > det) {
      return false;
    }

    C3Vector qvec = C3Vector::Cross(tvec, edge1);
    float    v = C3Vector::Dot(ray.dir, qvec);
    if (v < 0.0f || u + v > det) {
      return false;
    }
    if (t || bary) {
      float inv_det = 1.0f / det;
      if (t) {
        *t = C3Vector::Dot(edge2, qvec) * inv_det;
      }
      if (bary) {
        *bary = inv_det * C2Vector(u, v);
      }
    }
    return true;
  }

  bool Intersect(const C3Vector &point, const CCone &cone) {
    C3Vector offset = point - cone.position;
    float    adotl = C3Vector::Dot(offset, cone.axis);
    bool     inside = adotl * adotl >= offset.SquaredMag() * (cone.CosAngle() * cone.CosAngle());
    return cone.height != 0.0f ? adotl <= cone.height && inside : inside;
  }

  bool Intersect(const C3Ray &ray, const CCone &cone, float *t, C3Vector *p) {
    float    localT;
    C3Vector localP;
    if (!t) {
      t = &localT;
    }
    if (!p) {
      p = &localP;
    }

    float    directionProjection = C3Vector::Dot(ray.dir, cone.axis);
    float    directionSquared = ray.dir.SquaredMag();
    float    cosineSquared = cone.CosAngle() * cone.CosAngle();
    C3Vector offset = ray.origin - cone.position;
    float    offsetProjection = C3Vector::Dot(offset, cone.axis);
    float    c2 = directionProjection * directionProjection - cosineSquared * directionSquared;
    float    c1 = directionProjection * offsetProjection - cosineSquared * C3Vector::Dot(offset, ray.dir);
    float    c0 = offsetProjection * offsetProjection - cosineSquared * offset.SquaredMag();

    if (CMath::fabs_(c2) >= 0.000001f) {
      float discriminant = c1 * c1 - c0 * c2;
      if (discriminant < 0.0f) {
        return false;
      }
      if (discriminant > 0.0f) {
        float root = sqrt(discriminant);
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

  bool Intersect(const CObBox &a, const CObBox &b) {
    const C3Vector *aAxis = a.b.Row0AsVec3();
    const C3Vector *bAxis = b.b.Row0AsVec3();
    C3Vector        kD = b.c - a.c;
    float           aafC[3][3];
    float           aafAbsC[3][3];
    float           afAD[3];
    float           fR;
    int             i;

    for (i = 0; i < 3; ++i) {
      aafC[0][i] = C3Vector::Dot(aAxis[0], bAxis[i]);
      aafAbsC[0][i] = CMath::fabs_(aafC[0][i]);
    }
    afAD[0] = C3Vector::Dot(aAxis[0], kD);
    fR = CMath::fabs_(afAD[0]);
    if (fR > a.e.x + (b.e.x * aafAbsC[0][0] + b.e.y * aafAbsC[0][1] + b.e.z * aafAbsC[0][2])) {
      return false;
    }

    for (i = 0; i < 3; ++i) {
      aafC[1][i] = C3Vector::Dot(aAxis[1], bAxis[i]);
      aafAbsC[1][i] = CMath::fabs_(aafC[1][i]);
    }
    afAD[1] = C3Vector::Dot(aAxis[1], kD);
    fR = CMath::fabs_(afAD[1]);
    if (fR > a.e.y + (b.e.x * aafAbsC[1][0] + b.e.y * aafAbsC[1][1] + b.e.z * aafAbsC[1][2])) {
      return false;
    }

    for (i = 0; i < 3; ++i) {
      aafC[2][i] = C3Vector::Dot(aAxis[2], bAxis[i]);
      aafAbsC[2][i] = CMath::fabs_(aafC[2][i]);
    }
    afAD[2] = C3Vector::Dot(aAxis[2], kD);
    fR = CMath::fabs_(afAD[2]);
    if (fR > a.e.z + (b.e.x * aafAbsC[2][0] + b.e.y * aafAbsC[2][1] + b.e.z * aafAbsC[2][2])) {
      return false;
    }

    fR = CMath::fabs_(C3Vector::Dot(bAxis[0], kD));
    if (fR > a.e.x * aafAbsC[0][0] + a.e.y * aafAbsC[1][0] + a.e.z * aafAbsC[2][0] + b.e.x) {
      return false;
    }

    fR = CMath::fabs_(C3Vector::Dot(bAxis[1], kD));
    if (fR > a.e.x * aafAbsC[0][1] + a.e.y * aafAbsC[1][1] + a.e.z * aafAbsC[2][1] + b.e.y) {
      return false;
    }

    fR = CMath::fabs_(C3Vector::Dot(bAxis[2], kD));
    if (fR > a.e.x * aafAbsC[0][2] + a.e.y * aafAbsC[1][2] + a.e.z * aafAbsC[2][2] + b.e.z) {
      return false;
    }

    fR = CMath::fabs_(afAD[2] * aafC[1][0] - afAD[1] * aafC[2][0]);
    if (fR > (a.e.y * aafAbsC[2][0] + a.e.z * aafAbsC[1][0]) + (b.e.y * aafAbsC[0][2] + b.e.z * aafAbsC[0][1])) {
      return false;
    }

    fR = CMath::fabs_(afAD[2] * aafC[1][1] - afAD[1] * aafC[2][1]);
    if (fR > (a.e.y * aafAbsC[2][1] + a.e.z * aafAbsC[1][1]) + (b.e.x * aafAbsC[0][2] + b.e.z * aafAbsC[0][0])) {
      return false;
    }

    fR = CMath::fabs_(afAD[2] * aafC[1][2] - afAD[1] * aafC[2][2]);
    if (fR > (a.e.y * aafAbsC[2][2] + a.e.z * aafAbsC[1][2]) + (b.e.x * aafAbsC[0][1] + b.e.y * aafAbsC[0][0])) {
      return false;
    }

    fR = CMath::fabs_(afAD[0] * aafC[2][0] - afAD[2] * aafC[0][0]);
    if (fR > (a.e.x * aafAbsC[2][0] + a.e.z * aafAbsC[0][0]) + (b.e.y * aafAbsC[1][2] + b.e.z * aafAbsC[1][1])) {
      return false;
    }

    fR = CMath::fabs_(afAD[0] * aafC[2][1] - afAD[2] * aafC[0][1]);
    if (fR > (a.e.x * aafAbsC[2][1] + a.e.z * aafAbsC[0][1]) + (b.e.x * aafAbsC[1][2] + b.e.z * aafAbsC[1][0])) {
      return false;
    }

    fR = CMath::fabs_(afAD[0] * aafC[2][2] - afAD[2] * aafC[0][2]);
    if (fR > (a.e.x * aafAbsC[2][2] + a.e.z * aafAbsC[0][2]) + (b.e.x * aafAbsC[1][1] + b.e.y * aafAbsC[1][0])) {
      return false;
    }

    fR = CMath::fabs_(afAD[1] * aafC[0][0] - afAD[0] * aafC[1][0]);
    if (fR > (a.e.x * aafAbsC[1][0] + a.e.y * aafAbsC[0][0]) + (b.e.y * aafAbsC[2][2] + b.e.z * aafAbsC[2][1])) {
      return false;
    }

    fR = CMath::fabs_(afAD[1] * aafC[0][1] - afAD[0] * aafC[1][1]);
    if (fR > (a.e.x * aafAbsC[1][1] + a.e.y * aafAbsC[0][1]) + (b.e.x * aafAbsC[2][2] + b.e.z * aafAbsC[2][0])) {
      return false;
    }

    fR = CMath::fabs_(afAD[1] * aafC[0][2] - afAD[0] * aafC[1][2]);
    if (fR > (a.e.x * aafAbsC[1][2] + a.e.y * aafAbsC[0][2]) + (b.e.x * aafAbsC[2][1] + b.e.y * aafAbsC[2][0])) {
      return false;
    }

    return true;
  }

}  // namespace NTempest

bool NTempest::Intersect(const C2Vector &a0, const C2Vector &a1, const C2Vector &b0, const C2Vector &b1) {
  float Ax = a1.x - a0.x;
  float Ay = a1.y - a0.y;
  float bx = b0.x - b1.x;
  float by = b0.y - b1.y;
  float Cx = a0.x - b0.x;
  float Cy = a0.y - b0.y;
  float f = bx * Ay - by * Ax;
  float d = Cx * by - Cy * bx;
  if ((f > 0.0f && d >= 0.0f && d <= f) || (f < 0.0f && d <= 0.0f && d >= f)) {
    float e = Cy * Ax - Cx * Ay;
    if (f > 0.0f) {
      if (e >= 0.0f && e <= f) {
        return true;
      }
    } else if (e <= 0.0f && e >= f) {
      return true;
    }
  }
  return false;
}

bool NTempest::Intersect(const C2Vector &a0, const C2Vector &a1, const C2Vector &b0, const C2Vector &b1, C2Vector &i) {
  float denominator = (a0.y - a1.y) * b0.x + (b0.y - b1.y) * a1.x + (a1.y - a0.y) * b1.x + (b1.y - b0.y) * a0.x;
  if (CMath::fabs_(denominator) < 0.001f) {
    return false;
  }

  float inverse = 1.0f / denominator;
  float s = ((a0.y - b1.y) * b0.x + (b0.y - a0.y) * b1.x + (b1.y - b0.y) * a0.x) * inverse;
  float t = -((b0.y - a1.y) * a0.x + (a0.y - b0.y) * a1.x + (a1.y - a0.y) * b0.x);
  t *= inverse;
  i.x = a0.x + (a1.x - a0.x) * s;
  i.y = a0.y + (a1.y - a0.y) * s;
  if (0.0f <= s && s <= 1.0f && 0.0f <= t && t <= 1.0f) {
    return true;
  }
  return false;
}

static BOOL EdgeIntersectTriEdge(
    NTempest::C2Vector &a0, NTempest::C2Vector &a1, NTempest::C2Vector &b0, NTempest::C2Vector &b1, NTempest::C2Vector &b2
);
static BOOL PointInTri(NTempest::C2Vector &p, NTempest::C2Vector &a0, NTempest::C2Vector &a1, NTempest::C2Vector &a2);
static bool CoplanarTriIntersectTri(const NTempest::CFacet &facet0, const NTempest::CFacet &facet1);

bool NTempest::Intersect(const CFacet &facet0, const CFacet &facet1) {
  float dists[3];
  int   sides[3];
  int   counts[3] = {0, 0, 0};

  for (UINT i = 0; i < 3; ++i) {
    dists[i] = facet1.plane.DistSigned(facet0.vertices[i]);
    if (dists[i] > 0.01f) {
      sides[i] = 0;
    } else if (dists[i] < -0.01f) {
      sides[i] = 1;
    } else {
      sides[i] = 2;
    }
    ++counts[sides[i]];
  }

  if (counts[2] == 3) {
    return CoplanarTriIntersectTri(facet0, facet1);
  }

  C3Vector edge[2];
  UINT     n = 0;
  for (i = 0; i < 3; ++i) {
    if (sides[i] != sides[i + 1]) {
      UINT  next = i == 2 ? 0 : i + 1;
      float t = dists[i] / (dists[i] - dists[next]);
      edge[n].x = (facet0.vertices[next].x - facet0.vertices[i].x) * t + facet0.vertices[i].x;
      edge[n].y = (facet0.vertices[next].y - facet0.vertices[i].y) * t + facet0.vertices[i].y;
      edge[n].z = (facet0.vertices[next].z - facet0.vertices[i].z) * t + facet0.vertices[i].z;
      ++n;
    }
  }

  int      i0;
  int      i1;
  C3Vector axis(CMath::fabs_(facet0.plane.n.x), CMath::fabs_(facet0.plane.n.y), CMath::fabs_(facet0.plane.n.z));
  if (axis.x > axis.y) {
    if (axis.x > axis.z) {
      i0 = 1;
      i1 = 2;
    } else {
      i0 = 0;
      i1 = 1;
    }
  } else {
    if (axis.z > axis.y) {
      i0 = 0;
      i1 = 1;
    } else {
      i0 = 0;
      i1 = 2;
    }
  }

  C2Vector e0(edge[0][i0], edge[0][i1]);
  C2Vector e1(edge[1][i0], edge[1][i1]);
  C2Vector u0(facet1.vertices[0][i0], facet1.vertices[0][i1]);
  C2Vector u1(facet1.vertices[1][i0], facet1.vertices[1][i1]);
  C2Vector u2(facet1.vertices[2][i0], facet1.vertices[2][i1]);
  if (EdgeIntersectTriEdge(e0, e1, u0, u1, u2)) {
    return true;
  }
  return PointInTri(e0, u0, u1, u2) != 0;
}

static BOOL
EdgeIntersectTriEdge(NTempest::C2Vector &a0, NTempest::C2Vector &a1, NTempest::C2Vector &b0, NTempest::C2Vector &b1, NTempest::C2Vector &b2) {
  if (NTempest::Intersect(a0, a1, b0, b1)) {
    return 1;
  }
  if (NTempest::Intersect(a0, a1, b1, b2)) {
    return 1;
  }
  return NTempest::Intersect(a0, a1, b2, b0) != 0;
}

static BOOL PointInTri(NTempest::C2Vector &p, NTempest::C2Vector &a0, NTempest::C2Vector &a1, NTempest::C2Vector &a2) {
  float a;
  float b;
  float c;
  float d0;
  float d1;
  float d2;

  a = a1.y - a0.y;
  b = -(a1.x - a0.x);
  c = -a * a0.x - b * a0.y;
  d0 = a * p.x + b * p.y + c;

  a = a2.y - a1.y;
  b = -(a2.x - a1.x);
  c = -a * a1.x - b * a1.y;
  d1 = a * p.x + b * p.y + c;

  a = a0.y - a2.y;
  b = -(a0.x - a2.x);
  c = -a * a2.x - b * a2.y;
  d2 = a * p.x + b * p.y + c;

  if (d0 * d1 >= 0.0) {
    if (d0 * d2 >= 0.0) {
      return 1;
    }
  }
  return 0;
}

static bool CoplanarTriIntersectTri(const NTempest::CFacet &facet0, const NTempest::CFacet &facet1) {
  int                i0;
  int                i1;
  NTempest::C3Vector axis(NTempest::CMath::fabs_(facet0.plane.n.x), NTempest::CMath::fabs_(facet0.plane.n.y), NTempest::CMath::fabs_(facet0.plane.n.z));
  if (axis.x > axis.y) {
    if (axis.x > axis.z) {
      i0 = 1;
      i1 = 2;
    } else {
      i0 = 0;
      i1 = 1;
    }
  } else {
    if (axis.z > axis.y) {
      i0 = 0;
      i1 = 1;
    } else {
      i0 = 0;
      i1 = 2;
    }
  }

  NTempest::C2Vector v0(facet0.vertices[0][i0], facet0.vertices[0][i1]);
  NTempest::C2Vector v1(facet0.vertices[1][i0], facet0.vertices[1][i1]);
  NTempest::C2Vector v2(facet0.vertices[2][i0], facet0.vertices[2][i1]);
  NTempest::C2Vector u0(facet1.vertices[0][i0], facet1.vertices[0][i1]);
  NTempest::C2Vector u1(facet1.vertices[1][i0], facet1.vertices[1][i1]);
  NTempest::C2Vector u2(facet1.vertices[2][i0], facet1.vertices[2][i1]);

  if (EdgeIntersectTriEdge(v0, v1, u0, u1, u2)) {
    return true;
  }
  if (EdgeIntersectTriEdge(v1, v2, u0, u1, u2)) {
    return true;
  }
  if (EdgeIntersectTriEdge(v2, v0, u0, u1, u2)) {
    return true;
  }
  if (PointInTri(v0, u0, u1, u2)) {
    return true;
  }
  return PointInTri(u0, v0, v1, v2) != 0;
}
