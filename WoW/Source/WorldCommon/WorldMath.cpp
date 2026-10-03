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
  float t = (-((c.y - b.y) * a.x + (a.y - c.y) * b.x + (b.y - a.y) * c.x)) * inverse;
  if (0.0f <= s && s <= 1.0f && 0.0f <= t && t <= 1.0f) {
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
  float              inverseDet = 1.0f / det;
  NTempest::C3Vector qvec = NTempest::C3Vector::Cross(tvec, edge1);
  float              u;
  float              v;

  if (det > 0.000001f) {
    u = NTempest::C3Vector::Dot(tvec, pvec);
    if (u < 0.0f || u > det) {
      return 0;
    }

    v = NTempest::C3Vector::Dot(rayDir, qvec);
    if (v < 0.0f || u + v > det) {
      return 0;
    }
  } else if (det < -0.000001f) {
    u = NTempest::C3Vector::Dot(tvec, pvec);
    if (u > 0.0f || u < det) {
      return 0;
    }

    v = NTempest::C3Vector::Dot(rayDir, qvec);
    if (v > 0.0f || u + v < det) {
      return 0;
    }
  } else {
    return 0;
  }

  dist = NTempest::C3Vector::Dot(edge2, qvec) * inverseDet;
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
  nBox.b = *m.Row3AsVec3();
  nBox.t = *m.Row3AsVec3();

  const NTempest::C3Vector &row0 = *reinterpret_cast<const NTempest::C3Vector *>(&m.a0);
  const NTempest::C3Vector &row1 = *reinterpret_cast<const NTempest::C3Vector *>(&m.b0);
  const NTempest::C3Vector &row2 = *reinterpret_cast<const NTempest::C3Vector *>(&m.c0);
  ::TransformAABox(&row0, &row1, &row2, box, nBox);
}

void CWorldMath::TransformAABox(const NTempest::C44Matrix &m, const NTempest::CAaBox &box, NTempest::CAaBox &nBox) {
  nBox.b = *m.Row3AsVec3();
  nBox.t = *m.Row3AsVec3();

  const NTempest::C3Vector &row0 = *reinterpret_cast<const NTempest::C3Vector *>(&m.a0);
  const NTempest::C3Vector &row1 = *reinterpret_cast<const NTempest::C3Vector *>(&m.b0);
  const NTempest::C3Vector &row2 = *reinterpret_cast<const NTempest::C3Vector *>(&m.c0);
  ::TransformAABox(&row0, &row1, &row2, box, nBox);
}

BOOL CWorldMath::SphereIntersectAABox(const NTempest::CAaBox &box, const NTempest::C3Vector &center, float radius) {
  float        squaredDistance = 0.0f;
  float        radiusSquared = radius * radius;
  const float *bottom = &box.b.x;
  const float *top = &box.t.x;
  const float *point = &center.x;

  for (UINT i = 0; i < 3; ++i) {
    if (point[i] < bottom[i]) {
      float delta = point[i] - bottom[i];
      squaredDistance += delta * delta;
    } else if (point[i] > top[i]) {
      float delta = point[i] - top[i];
      squaredDistance += delta * delta;
    }
  }

  if (squaredDistance <= radiusSquared) {
    return 1;
  }

  return 0;
}

UINT CWorldMath::AABoxIntersectPlane(const NTempest::CAaBox &box, const NTempest::C4Plane &plane) {
  float        diagMin[3];
  float        dists[2];
  float        diagMax[3];
  const float *normal = plane.Access();
  const float *bottom = &box.b.x;
  const float *top = &box.t.x;

  for (UINT i = 0; i < 3; ++i) {
    if (normal[i] >= 0.0f) {
      diagMin[i] = bottom[i];
      diagMax[i] = top[i];
    } else {
      diagMin[i] = top[i];
      diagMax[i] = bottom[i];
    }
  }

  dists[0] = diagMin[0] * normal[0] + diagMin[1] * normal[1] + diagMin[2] * normal[2] + normal[3];
  dists[1] = diagMax[0] * normal[0] + diagMax[1] * normal[1] + diagMax[2] * normal[2] + normal[3];
  if (((*(DWORD *)&dists[0] ^ *(DWORD *)&dists[1]) & 0x80000000) || NTempest::CMath::fequalz_(0.0f, dists[0], 0.019444443f) ||
      NTempest::CMath::fequalz_(0.0f, dists[1], 0.019444443f)) {
    return SIDE_ON;
  }
  if (dists[0] > 0.019444443f) {
    return SIDE_POS;
  }
  return SIDE_NEG;
}

float CWorldMath::TriSqrDistance(
    const NTempest::C3Vector &point,
    const NTempest::C3Vector &origin,
    const NTempest::C3Vector &edge0,
    const NTempest::C3Vector &edge1
) {
  NTempest::C3Vector kDiff = origin - point;
  float              fA00 = edge0.SquaredMag();
  float              fA11 = edge1.SquaredMag();
  float              fC = kDiff.SquaredMag();
  float              fA01 = NTempest::C3Vector::Dot(edge0, edge1);
  float              fB0 = kDiff.x * edge0.x + kDiff.y * edge0.y + kDiff.z * edge0.z;
  float              fB1 = kDiff.x * edge1.x + kDiff.y * edge1.y + kDiff.z * edge1.z;
  float              fDet = fabs(fA11 * fA00 - fA01 * fA01);
  float              fS = fB1 * fA01 - fB0 * fA11;
  float              fT = fB0 * fA01 - fB1 * fA00;
  float              squaredDistance;

  if (fS + fT <= fDet) {
    if (fS < 0.0f) {
      if (fT < 0.0f) {
        if (fB0 < 0.0f) {
          fT = 0.0f;
          if (-fB0 >= fA00) {
            fS = 1.0f;
            squaredDistance = fA00 + 2.0f * fB0 + fC;
          } else {
            fS = -fB0 / fA00;
            squaredDistance = fB0 * fS + fC;
          }
        } else {
          fS = 0.0f;
          if (fB1 >= 0.0f) {
            fT = 0.0f;
            squaredDistance = fC;
          } else if (-fB1 >= fA11) {
            fT = 1.0f;
            squaredDistance = fA11 + 2.0f * fB1 + fC;
          } else {
            fT = -fB1 / fA11;
            squaredDistance = fB1 * fT + fC;
          }
        }
      } else {
        fS = 0.0f;
        if (fB1 >= 0.0f) {
          fT = 0.0f;
          squaredDistance = fC;
        } else if (-fB1 >= fA11) {
          fT = 1.0f;
          squaredDistance = fA11 + 2.0f * fB1 + fC;
        } else {
          fT = -fB1 / fA11;
          squaredDistance = fB1 * fT + fC;
        }
      }
    } else if (fT < 0.0f) {
      fT = 0.0f;
      if (fB0 >= 0.0f) {
        fS = 0.0f;
        squaredDistance = fC;
      } else if (-fB0 >= fA00) {
        fS = 1.0f;
        squaredDistance = fA00 + 2.0f * fB0 + fC;
      } else {
        fS = -fB0 / fA00;
        squaredDistance = fB0 * fS + fC;
      }
    } else {
      float inverseDet = 1.0f / fDet;
      fS *= inverseDet;
      fT *= inverseDet;
      squaredDistance = fS * (fA00 * fS + fA01 * fT + 2.0f * fB0) + fT * (fA01 * fS + fA11 * fT + 2.0f * fB1) + fC;
    }
  } else {
    float fTmp0;
    float tmp1;
    float fNumer;
    float denominator;

    if (fS < 0.0f) {
      fTmp0 = fA01 + fB0;
      tmp1 = fA11 + fB1;
      if (tmp1 > fTmp0) {
        fNumer = tmp1 - fTmp0;
        denominator = fA00 - 2.0f * fA01 + fA11;
        if (fNumer >= denominator) {
          fS = 1.0f;
          fT = 0.0f;
          squaredDistance = fA00 + 2.0f * fB0 + fC;
        } else {
          fS = fNumer / denominator;
          fT = 1.0f - fS;
          squaredDistance = fS * (fA00 * fS + fA01 * fT + 2.0f * fB0) + fT * (fA01 * fS + fA11 * fT + 2.0f * fB1) + fC;
        }
      } else {
        fS = 0.0f;
        if (tmp1 <= 0.0f) {
          fT = 1.0f;
          squaredDistance = fA11 + 2.0f * fB1 + fC;
        } else if (fB1 >= 0.0f) {
          fT = 0.0f;
          squaredDistance = fC;
        } else {
          fT = -fB1 / fA11;
          squaredDistance = fB1 * fT + fC;
        }
      }
    } else if (fT < 0.0f) {
      fTmp0 = fA01 + fB1;
      tmp1 = fA00 + fB0;
      if (tmp1 > fTmp0) {
        fNumer = tmp1 - fTmp0;
        denominator = fA00 - 2.0f * fA01 + fA11;
        if (fNumer >= denominator) {
          fT = 1.0f;
          fS = 0.0f;
          squaredDistance = fA11 + 2.0f * fB1 + fC;
        } else {
          fT = fNumer / denominator;
          fS = 1.0f - fT;
          squaredDistance = fS * (fA00 * fS + fA01 * fT + 2.0f * fB0) + fT * (fA01 * fS + fA11 * fT + 2.0f * fB1) + fC;
        }
      } else {
        fT = 0.0f;
        if (tmp1 <= 0.0f) {
          fS = 1.0f;
          squaredDistance = fA00 + 2.0f * fB0 + fC;
        } else if (fB0 >= 0.0f) {
          fS = 0.0f;
          squaredDistance = fC;
        } else {
          fS = -fB0 / fA00;
          squaredDistance = fB0 * fS + fC;
        }
      }
    } else {
      fNumer = fA11 + fB1 - fA01 - fB0;
      if (fNumer <= 0.0f) {
        fS = 0.0f;
        fT = 1.0f;
        squaredDistance = fA11 + 2.0f * fB1 + fC;
      } else {
        denominator = fA00 - 2.0f * fA01 + fA11;
        if (fNumer >= denominator) {
          fS = 1.0f;
          fT = 0.0f;
          squaredDistance = fA00 + 2.0f * fB0 + fC;
        } else {
          fS = fNumer / denominator;
          fT = 1.0f - fS;
          squaredDistance = fS * (fA00 * fS + fA01 * fT + 2.0f * fB0) + fT * (fA01 * fS + fA11 * fT + 2.0f * fB1) + fC;
        }
      }
    }
  }

  return fabs(squaredDistance);
}

BOOL CWorldMath::VectorIntersectAABox2(const NTempest::CAaBox &box, const NTempest::C3Vector &start, const NTempest::C3Vector &end) {
  NTempest::C3Vector dir = end - start;
  DWORD              i;
  int                Inside = 1;
  NTempest::C3Vector maxT(-1.0f, -1.0f, -1.0f);
  float              coord[3];
  const float       *fStart = &start.x;
  const float       *fEnd = &end.x;
  const float       *fMin = &box.b.x;
  const float       *fMax = &box.t.x;

  for (i = 0; i < 3; ++i) {
    if (fStart[i] < fMin[i]) {
      if (fEnd[i] < fMin[i]) {
        return 0;
      }

      coord[i] = fMin[i];
      Inside = 0;

      if (*(DWORD *)&(&dir.x)[i]) {
        (&maxT.x)[i] = (fMin[i] - fStart[i]) / (&dir.x)[i];
      }
    } else if (fStart[i] > fMax[i]) {
      if (fEnd[i] > fMax[i]) {
        return 0;
      }

      coord[i] = fMax[i];
      Inside = 0;

      if (*(DWORD *)&(&dir.x)[i]) {
        (&maxT.x)[i] = (fMax[i] - fStart[i]) / (&dir.x)[i];
      }
    }
  }

  if (Inside) {
    return 1;
  }

  DWORD WhichPlane = 0;
  if ((&maxT.x)[1] > (&maxT.x)[WhichPlane]) {
    WhichPlane = 1;
  }
  if ((&maxT.x)[2] > (&maxT.x)[WhichPlane]) {
    WhichPlane = 2;
  }

  if (*(DWORD *)&(&maxT.x)[WhichPlane] & 0x80000000) {
    return 0;
  }

  for (i = 0; i < 3; ++i) {
    if (i != WhichPlane) {
      coord[i] = fStart[i] + (&maxT.x)[WhichPlane] * (&dir.x)[i];

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
