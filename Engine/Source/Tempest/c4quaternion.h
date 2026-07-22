#pragma once

#include "Tempest/c33matrix.h"
#include "Tempest/c4vector.h"

namespace NTempest {

  class C4Quaternion : public C4Vector {
   public:
    C4Quaternion() : C4Vector(0.0f, 0.0f, 0.0f, 1.0f) {
    }

    C4Quaternion(float w, float x, float y, float z) : C4Vector(x, y, z, w) {
    }

    operator C33Matrix() {
      float x2 = x + x;
      float y2 = y + y;
      float z2 = z + z;
      float wx = x2 * w;
      float wy = y2 * w;
      float wz = z2 * w;
      float xx = x2 * x;
      float xy = y2 * x;
      float xz = z2 * x;
      float yy = y2 * y;
      float yz = z2 * y;
      float zz = z2 * z;

      return C33Matrix(1.0f - (yy + zz), xy + wz, xz - wy, xy - wz, 1.0f - (xx + zz), yz + wx, xz + wy, yz - wx, 1.0f - (xx + yy));
    }

    void FromRotationMatrix(C33Matrix &r);
    void FromRotationMatrixInv(C33Matrix &r);

    static C4Quaternion __fastcall Slerp(float ratio, const C4Quaternion &start, const C4Quaternion &end);
    static C4Quaternion __fastcall
    Squad(float ratio, const C4Quaternion &start, const C4Quaternion &end, const C4Quaternion &outTangent, const C4Quaternion &inTangent);
  };

  C4Quaternion __fastcall operator*(const C4Quaternion &l, const C4Quaternion &r);

}  // namespace NTempest
