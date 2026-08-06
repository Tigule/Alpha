#ifndef WOW_SOURCE_WORLDCOMMON_WORLDMATH_H
#define WOW_SOURCE_WORLDCOMMON_WORLDMATH_H

#include "Tempest/c2vector.h"
#include "Tempest/c33matrix.h"
#include "Tempest/c34matrix.h"
#include "Tempest/c3segment.h"
#include "Tempest/c44matrix.h"
#include "Tempest/c4plane.h"
#include "Tempest/caabox.h"

class CWorldMath {
 public:
  enum {
    SIDE_ON = 0,
    SIDE_POS = 1,
    SIDE_NEG = 2
  };

  static int EdgeIntersectEdge(
      const NTempest::C2Vector &a,
      const NTempest::C2Vector &b,
      const NTempest::C2Vector &c,
      const NTempest::C3Vector &d,
      NTempest::C2Vector       &p
  );
  static int RayIntersectTri(
      const NTempest::C3Vector &rayOrig,
      const NTempest::C3Vector &rayDir,
      const NTempest::C3Vector &v0,
      const NTempest::C3Vector &v1,
      const NTempest::C3Vector &v2,
      float                    &dist
  );
  static void TransformAABox(const NTempest::C33Matrix &m, const NTempest::CAaBox &box, NTempest::CAaBox &nBox);
  static void TransformAABox(const NTempest::C34Matrix &m, const NTempest::CAaBox &box, NTempest::CAaBox &nBox);
  static void TransformAABox(const NTempest::C44Matrix &m, const NTempest::CAaBox &box, NTempest::CAaBox &nBox);

  static int  VectorIntersectAABox2(const NTempest::CAaBox &box, const NTempest::C3Vector &start, const NTempest::C3Vector &end);
  static int  VectorIntersectAABox2(const NTempest::CAaBox &box, const NTempest::C3Segment &seg);
  static int  SphereIntersectAABox(const NTempest::CAaBox &box, const NTempest::C3Vector &center, float radius);
  static UINT AABoxIntersectPlane(const NTempest::CAaBox &box, const NTempest::C4Plane &plane);
  static float
  TriSqrDistance(const NTempest::C3Vector &point, const NTempest::C3Vector &origin, const NTempest::C3Vector &edge0, const NTempest::C3Vector &edge1);
};

#endif
