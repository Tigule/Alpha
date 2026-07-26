#ifndef WOW_SOURCE_WORLDCOMMON_WORLDMATH_H
#define WOW_SOURCE_WORLDCOMMON_WORLDMATH_H

#include "Tempest/c33matrix.h"
#include "Tempest/c34matrix.h"
#include "Tempest/c3segment.h"
#include "Tempest/c44matrix.h"
#include "Tempest/caabox.h"

class CWorldMath {
 public:
  static void __fastcall TransformAABox(const NTempest::C33Matrix &m, const NTempest::CAaBox &box, NTempest::CAaBox &nBox);
  static void __fastcall TransformAABox(const NTempest::C34Matrix &m, const NTempest::CAaBox &box, NTempest::CAaBox &nBox);
  static void __fastcall TransformAABox(const NTempest::C44Matrix &m, const NTempest::CAaBox &box, NTempest::CAaBox &nBox);

  static int __fastcall VectorIntersectAABox2(const NTempest::CAaBox &box, const NTempest::C3Vector &start, const NTempest::C3Vector &end);
  static int __fastcall VectorIntersectAABox2(const NTempest::CAaBox &box, const NTempest::C3Segment &seg);
};

#endif
