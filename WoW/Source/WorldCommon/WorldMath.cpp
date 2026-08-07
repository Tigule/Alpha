#include <Base/Base.h>
#include <WowConst.h>
#include <MapDefs.h>

#include "WorldCommon/WorldMath.h"

#include <math.h>

BOOL CWorldMath::EdgeIntersectEdge(
    const NTempest::C2Vector &a,
    const NTempest::C2Vector &b,
    const NTempest::C2Vector &c,
    const NTempest::C3Vector &d,
    NTempest::C2Vector       &p
) {
  float denominator = (a.y - b.y) * c.x + (c.y - d.y) * b.x + (b.y - a.y) * d.x + (d.y - c.y) * a.x;
  if (denominator == 0.0f) {
    return 0;
  }

  float inverse = 1.0f / denominator;
  float s = ((a.y - d.y) * c.x + (c.y - a.y) * d.x + (d.y - c.y) * a.x) * inverse;
  float t = -((c.y - b.y) * a.x + (a.y - c.y) * b.x + (b.y - a.y) * c.x) * inverse;
  if (s >= 0.0f && s <= 1.0f && t >= 0.0f && t <= 1.0f) {
    return 1;
  }

  p.x = (b.x - a.x) * s + a.x;
  p.y = (b.y - a.y) * s + a.y;
  return 0;
}

BOOL CWorldMath::RayIntersectTri(
    const NTempest::C3Vector &rayOrig,
    const NTempest::C3Vector &rayDir,
    const NTempest::C3Vector &v0,
    const NTempest::C3Vector &v1,
    const NTempest::C3Vector &v2,
    float                    &dist
) {
  NTempest::C3Vector edge1 = v1 - v0;
  NTempest::C3Vector edge2 = v2 - v0;
  NTempest::C3Vector pvec = NTempest::C3Vector::Cross(rayDir, edge2);
  float              det = NTempest::C3Vector::Dot(edge1, pvec);
  NTempest::C3Vector tvec = rayOrig - v0;
  NTempest::C3Vector qvec = NTempest::C3Vector::Cross(tvec, edge1);
  float              u = NTempest::C3Vector::Dot(tvec, pvec);
  float              v = NTempest::C3Vector::Dot(rayDir, qvec);

  if (det > 0.000001f) {
    if (u < 0.0f || u > det || v < 0.0f || u + v > det) {
      return 0;
    }
  } else if (det < -0.000001f) {
    if (u > 0.0f || u < det || v > 0.0f || u + v < det) {
      return 0;
    }
  } else {
    return 0;
  }

  dist = NTempest::C3Vector::Dot(edge2, qvec) / det;
  return 1;
}

static void TransformAABox(
    const NTempest::C3Vector *row0,
    const NTempest::C3Vector *row1,
    const NTempest::C3Vector *row2,
    const NTempest::CAaBox   &box,
    NTempest::CAaBox         &nBox
) {
  const float *m[3] = {&row0->x, &row1->x, &row2->x};

  for (UINT i = 0; i < 3; ++i) {
    for (UINT j = 0; j < 3; ++j) {
      float a = box.b[j] * m[j][i];
      float b = box.t[j] * m[j][i];

      if (a < b) {
        nBox.b[i] += a;
        nBox.t[i] += b;
      } else {
        nBox.b[i] += b;
        nBox.t[i] += a;
      }
    }
  }
}

void CWorldMath::TransformAABox(const NTempest::C33Matrix &m, const NTempest::CAaBox &box, NTempest::CAaBox &nBox) {
  nBox.b = NTempest::C3Vector(0.0f);
  nBox.t = NTempest::C3Vector(0.0f);

  const NTempest::C3Vector &row0 = *reinterpret_cast<const NTempest::C3Vector *>(&m.a0);
  const NTempest::C3Vector &row1 = *reinterpret_cast<const NTempest::C3Vector *>(&m.b0);
  const NTempest::C3Vector &row2 = *reinterpret_cast<const NTempest::C3Vector *>(&m.c0);
  ::TransformAABox(&row0, &row1, &row2, box, nBox);
}

void CWorldMath::TransformAABox(const NTempest::C34Matrix &m, const NTempest::CAaBox &box, NTempest::CAaBox &nBox) {
  nBox.b = NTempest::C3Vector(m.d0, m.d1, m.d2);
  nBox.t = nBox.b;

  const NTempest::C3Vector &row0 = *reinterpret_cast<const NTempest::C3Vector *>(&m.a0);
  const NTempest::C3Vector &row1 = *reinterpret_cast<const NTempest::C3Vector *>(&m.b0);
  const NTempest::C3Vector &row2 = *reinterpret_cast<const NTempest::C3Vector *>(&m.c0);
  ::TransformAABox(&row0, &row1, &row2, box, nBox);
}

void CWorldMath::TransformAABox(const NTempest::C44Matrix &m, const NTempest::CAaBox &box, NTempest::CAaBox &nBox) {
  nBox.b = NTempest::C3Vector(m.d0, m.d1, m.d2);
  nBox.t = nBox.b;

  const NTempest::C3Vector &row0 = *reinterpret_cast<const NTempest::C3Vector *>(&m.a0);
  const NTempest::C3Vector &row1 = *reinterpret_cast<const NTempest::C3Vector *>(&m.b0);
  const NTempest::C3Vector &row2 = *reinterpret_cast<const NTempest::C3Vector *>(&m.c0);
  ::TransformAABox(&row0, &row1, &row2, box, nBox);
}

BOOL CWorldMath::SphereIntersectAABox(const NTempest::CAaBox &box, const NTempest::C3Vector &center, float radius) {
  float        squaredDistance = 0.0f;
  const float *bottom = &box.b.x;
  const float *top = &box.t.x;
  const float *point = &center.x;

  for (UINT i = 0; i < 3; ++i) {
    float delta;
    if (point[i] < bottom[i]) {
      delta = point[i] - bottom[i];
    } else if (point[i] > top[i]) {
      delta = point[i] - top[i];
    } else {
      continue;
    }
    squaredDistance += delta * delta;
  }

  return squaredDistance <= radius * radius;
}

UINT CWorldMath::AABoxIntersectPlane(const NTempest::CAaBox &box, const NTempest::C4Plane &plane) {
  float        diagMin[3];
  float        diagMax[3];
  const float *normal = plane.Access();
  const float *bottom = &box.b.x;
  const float *top = &box.t.x;

  for (UINT i = 0; i < 3; ++i) {
    if (normal[i] < 0.0f) {
      diagMin[i] = top[i];
      diagMax[i] = bottom[i];
    } else {
      diagMin[i] = bottom[i];
      diagMax[i] = top[i];
    }
  }

  float dists[2] = {
      diagMin[0] * normal[0] + diagMin[1] * normal[1] + diagMin[2] * normal[2] + normal[3],
      diagMax[0] * normal[0] + diagMax[1] * normal[1] + diagMax[2] * normal[2] + normal[3]
  };
  const float epsilon = 0.019444443f;
  if ((dists[0] < 0.0f) != (dists[1] < 0.0f) || fabs(dists[0]) < epsilon || fabs(dists[1]) < epsilon) {
    return SIDE_ON;
  }
  return dists[0] <= epsilon ? SIDE_NEG : SIDE_POS;
}

float CWorldMath::TriSqrDistance(
    const NTempest::C3Vector &point,
    const NTempest::C3Vector &origin,
    const NTempest::C3Vector &edge0,
    const NTempest::C3Vector &edge1
) {
  NTempest::C3Vector diff = origin - point;
  float              a00 = NTempest::C3Vector::Dot(edge0, edge0);
  float              a01 = NTempest::C3Vector::Dot(edge0, edge1);
  float              a11 = NTempest::C3Vector::Dot(edge1, edge1);
  float              b0 = NTempest::C3Vector::Dot(diff, edge0);
  float              b1 = NTempest::C3Vector::Dot(diff, edge1);
  float              c = NTempest::C3Vector::Dot(diff, diff);
  float              det = fabs(a00 * a11 - a01 * a01);
  float              s = a01 * b1 - a11 * b0;
  float              t = a01 * b0 - a00 * b1;
  float              squaredDistance;

  if (s + t <= det) {
    if (s < 0.0f) {
      if (t < 0.0f) {
        if (b0 < 0.0f) {
          t = 0.0f;
          if (-b0 >= a00) {
            s = 1.0f;
            squaredDistance = a00 + 2.0f * b0 + c;
          } else {
            s = -b0 / a00;
            squaredDistance = b0 * s + c;
          }
        } else {
          s = 0.0f;
          if (b1 >= 0.0f) {
            t = 0.0f;
            squaredDistance = c;
          } else if (-b1 >= a11) {
            t = 1.0f;
            squaredDistance = a11 + 2.0f * b1 + c;
          } else {
            t = -b1 / a11;
            squaredDistance = b1 * t + c;
          }
        }
      } else {
        s = 0.0f;
        if (b1 >= 0.0f) {
          t = 0.0f;
          squaredDistance = c;
        } else if (-b1 >= a11) {
          t = 1.0f;
          squaredDistance = a11 + 2.0f * b1 + c;
        } else {
          t = -b1 / a11;
          squaredDistance = b1 * t + c;
        }
      }
    } else if (t < 0.0f) {
      t = 0.0f;
      if (b0 >= 0.0f) {
        s = 0.0f;
        squaredDistance = c;
      } else if (-b0 >= a00) {
        s = 1.0f;
        squaredDistance = a00 + 2.0f * b0 + c;
      } else {
        s = -b0 / a00;
        squaredDistance = b0 * s + c;
      }
    } else {
      float inverseDet = 1.0f / det;
      s *= inverseDet;
      t *= inverseDet;
      squaredDistance = s * (a00 * s + a01 * t + 2.0f * b0) + t * (a01 * s + a11 * t + 2.0f * b1) + c;
    }
  } else {
    if (s < 0.0f) {
      float tmp0 = a01 + b0;
      float tmp1 = a11 + b1;
      if (tmp1 > tmp0) {
        float numerator = tmp1 - tmp0;
        float denominator = a00 - 2.0f * a01 + a11;
        if (numerator >= denominator) {
          s = 1.0f;
          t = 0.0f;
          squaredDistance = a00 + 2.0f * b0 + c;
        } else {
          s = numerator / denominator;
          t = 1.0f - s;
          squaredDistance = s * (a00 * s + a01 * t + 2.0f * b0) + t * (a01 * s + a11 * t + 2.0f * b1) + c;
        }
      } else {
        s = 0.0f;
        if (tmp1 <= 0.0f) {
          t = 1.0f;
          squaredDistance = a11 + 2.0f * b1 + c;
        } else if (b1 >= 0.0f) {
          t = 0.0f;
          squaredDistance = c;
        } else {
          t = -b1 / a11;
          squaredDistance = b1 * t + c;
        }
      }
    } else if (t < 0.0f) {
      float tmp0 = a01 + b1;
      float tmp1 = a00 + b0;
      if (tmp1 > tmp0) {
        float numerator = tmp1 - tmp0;
        float denominator = a00 - 2.0f * a01 + a11;
        if (numerator >= denominator) {
          t = 1.0f;
          s = 0.0f;
          squaredDistance = a11 + 2.0f * b1 + c;
        } else {
          t = numerator / denominator;
          s = 1.0f - t;
          squaredDistance = s * (a00 * s + a01 * t + 2.0f * b0) + t * (a01 * s + a11 * t + 2.0f * b1) + c;
        }
      } else {
        t = 0.0f;
        if (tmp1 <= 0.0f) {
          s = 1.0f;
          squaredDistance = a00 + 2.0f * b0 + c;
        } else if (b0 >= 0.0f) {
          s = 0.0f;
          squaredDistance = c;
        } else {
          s = -b0 / a00;
          squaredDistance = b0 * s + c;
        }
      }
    } else {
      float numerator = a11 + b1 - a01 - b0;
      if (numerator <= 0.0f) {
        s = 0.0f;
        t = 1.0f;
        squaredDistance = a11 + 2.0f * b1 + c;
      } else {
        float denominator = a00 - 2.0f * a01 + a11;
        if (numerator >= denominator) {
          s = 1.0f;
          t = 0.0f;
          squaredDistance = a00 + 2.0f * b0 + c;
        } else {
          s = numerator / denominator;
          t = 1.0f - s;
          squaredDistance = s * (a00 * s + a01 * t + 2.0f * b0) + t * (a01 * s + a11 * t + 2.0f * b1) + c;
        }
      }
    }
  }

  return fabs(squaredDistance);
}

BOOL CWorldMath::VectorIntersectAABox2(const NTempest::CAaBox &box, const NTempest::C3Vector &start, const NTempest::C3Vector &end) {
  float        dir[3] = {end.x - start.x, end.y - start.y, end.z - start.z};
  DWORD        i;
  int          Inside = 1;
  float        maxT[3] = {-1.0f, -1.0f, -1.0f};
  float        coord[3];
  const float *fStart = &start.x;
  const float *fEnd = &end.x;
  const float *fMin = &box.b.x;
  const float *fMax = &box.t.x;

  for (i = 0; i < 3; ++i) {
    if (fStart[i] < fMin[i]) {
      if (fEnd[i] < fMin[i]) {
        return 0;
      }

      coord[i] = fMin[i];
      Inside = 0;

      if (dir[i] != 0.0f) {
        maxT[i] = (fMin[i] - fStart[i]) / dir[i];
      }
    } else if (fStart[i] > fMax[i]) {
      if (fEnd[i] > fMax[i]) {
        return 0;
      }

      coord[i] = fMax[i];
      Inside = 0;

      if (dir[i] != 0.0f) {
        maxT[i] = (fMax[i] - fStart[i]) / dir[i];
      }
    }
  }

  if (Inside) {
    return 1;
  }

  DWORD WhichPlane = 0;
  if (maxT[1] > maxT[0]) {
    WhichPlane = 1;
  }
  if (maxT[2] > maxT[WhichPlane]) {
    WhichPlane = 2;
  }

  if (maxT[WhichPlane] < 0.0f) {
    return 0;
  }

  for (i = 0; i < 3; ++i) {
    if (i != WhichPlane) {
      coord[i] = fStart[i] + dir[i] * maxT[WhichPlane];

      if (coord[i] < fMin[i] - 0.00001f || coord[i] > fMax[i] + 0.00001f) {
        return 0;
      }
    }
  }

  return 1;
}

BOOL CWorldMath::VectorIntersectAABox2(const NTempest::CAaBox &box, const NTempest::C3Segment &seg) {
  return VectorIntersectAABox2(box, seg.start, seg.end);
}
