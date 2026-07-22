#include "WorldCommon/WorldMath.h"

static void __fastcall TransformAABox(
    const NTempest::C3Vector *row0,
    const NTempest::C3Vector *row1,
    const NTempest::C3Vector *row2,
    const NTempest::CAaBox   &box,
    NTempest::CAaBox         &nBox
) {
  const float *m[3] = {&row0->x, &row1->x, &row2->x};

  for (unsigned int i = 0; i < 3; ++i) {
    for (unsigned int j = 0; j < 3; ++j) {
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

void __fastcall CWorldMath::TransformAABox(const NTempest::C33Matrix &m, const NTempest::CAaBox &box, NTempest::CAaBox &nBox) {
  nBox.b = NTempest::C3Vector(0.0f);
  nBox.t = NTempest::C3Vector(0.0f);

  const NTempest::C3Vector &row0 = *reinterpret_cast<const NTempest::C3Vector *>(&m.a0);
  const NTempest::C3Vector &row1 = *reinterpret_cast<const NTempest::C3Vector *>(&m.b0);
  const NTempest::C3Vector &row2 = *reinterpret_cast<const NTempest::C3Vector *>(&m.c0);
  ::TransformAABox(&row0, &row1, &row2, box, nBox);
}

void __fastcall CWorldMath::TransformAABox(const NTempest::C44Matrix &m, const NTempest::CAaBox &box, NTempest::CAaBox &nBox) {
  nBox.b = NTempest::C3Vector(m.d0, m.d1, m.d2);
  nBox.t = nBox.b;

  const NTempest::C3Vector &row0 = *reinterpret_cast<const NTempest::C3Vector *>(&m.a0);
  const NTempest::C3Vector &row1 = *reinterpret_cast<const NTempest::C3Vector *>(&m.b0);
  const NTempest::C3Vector &row2 = *reinterpret_cast<const NTempest::C3Vector *>(&m.c0);
  ::TransformAABox(&row0, &row1, &row2, box, nBox);
}

int __fastcall CWorldMath::VectorIntersectAABox2(const NTempest::CAaBox &box, const NTempest::C3Vector &start, const NTempest::C3Vector &end) {
  float         dir[3] = {end.x - start.x, end.y - start.y, end.z - start.z};
  unsigned long i;
  int           Inside = 1;
  float         maxT[3] = {-1.0f, -1.0f, -1.0f};
  float         coord[3];
  const float  *fStart = &start.x;
  const float  *fEnd = &end.x;
  const float  *fMin = &box.b.x;
  const float  *fMax = &box.t.x;

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

  unsigned long WhichPlane = 0;
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

int __fastcall CWorldMath::VectorIntersectAABox2(const NTempest::CAaBox &box, const NTempest::C3Segment &seg) {
  return VectorIntersectAABox2(box, seg.start, seg.end);
}
