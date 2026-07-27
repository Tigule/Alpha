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

    C4Quaternion(float angle, const C3Vector &axis) {
      FromAngleAxis(angle, axis);
    }

    C4Quaternion(const C4Vector &vector)
        : C4Vector(vector.x, vector.y, vector.z, vector.w) {
    }

    ~C4Quaternion() {
    }

    C3Vector Vector() const {
      return C3Vector(x, y, z);
    }

    float Real() const {
      return w;
    }

    void SetVector(const C3Vector &vector) {
      x = vector.x;
      y = vector.y;
      z = vector.z;
    }

    void SetReal(float real) {
      w = real;
    }

    void Set(float real, const C3Vector &vector) {
      w = real;
      SetVector(vector);
    }

    void Identity() {
      x = y = z = 0.0f;
      w = 1.0f;
    }

    void Zero() {
      x = y = z = w = 0.0f;
    }

    unsigned char IsValid() const {
      return x == x && y == y && z == z && w == w;
    }

    operator C33Matrix() const {
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

    C4Quaternion &operator*=(const C4Quaternion &a) {
      float oldW = w;
      float oldX = x;
      float oldY = y;
      float oldZ = z;
      w = oldW * a.w - oldX * a.x - oldY * a.y - oldZ * a.z;
      x = oldW * a.x + oldX * a.w + oldY * a.z - oldZ * a.y;
      y = oldW * a.y + oldY * a.w + oldZ * a.x - oldX * a.z;
      z = oldW * a.z + oldZ * a.w + oldX * a.y - oldY * a.x;
      return *this;
    }

    C4Quaternion operator*(float a) const {
      return C4Quaternion(w * a, x * a, y * a, z * a);
    }

    C3Vector operator*(const C3Vector &vector) const {
      return static_cast<C33Matrix>(*this) * vector;
    }

    float Norm() const {
      return SquaredMag();
    }

    float Abs() const {
      return Mag();
    }

    void FromRotationMatrix(const C33Matrix &r);
    void FromRotationMatrixInv(const C33Matrix &r);
    void ToRotationMatrix(C33Matrix &r) const {
      r = static_cast<C33Matrix>(*this);
    }
    void ToRotationMatrixInv(C33Matrix &r) const {
      r = static_cast<C33Matrix>(*this).Transpose();
    }
    void FromAngleAxis(const float angle, const C3Vector &axis);
    void ToAngleAxis(float &angle, C3Vector &axis) const;
    C4Quaternion Conjugate() const {
      return C4Quaternion(w, -x, -y, -z);
    }
    C4Quaternion Inverse() const;
    C4Quaternion UnitInverse() const {
      ASSERT(IsUnit());
      return Conjugate();
    }
    C4Quaternion Exp() const;
    C4Quaternion Log() const;

    static C4Quaternion Slerp(float ratio, const C4Quaternion &start, const C4Quaternion &end);
    static void SquadInterm(
        const C4Quaternion &q0,
        const C4Quaternion &q1,
        const C4Quaternion &q2,
        C4Quaternion &a,
        C4Quaternion &b
    );
    static void SquadIntermMaxCompat(
        const C4Quaternion &q0,
        const C4Quaternion &q1,
        const C4Quaternion &q2,
        C4Quaternion &a,
        C4Quaternion &b
    );
    static void SquadIntermTCB(
        const C4Quaternion &q0,
        const C4Quaternion &q1,
        const C4Quaternion &q2,
        float time0,
        float time1,
        float time2,
        float tension,
        float continuity,
        float bias,
        C4Quaternion &a,
        C4Quaternion &b
    );
    static C4Quaternion
    Squad(float ratio, const C4Quaternion &start, const C4Quaternion &end, const C4Quaternion &outTangent, const C4Quaternion &inTangent);
  };

  inline C4Quaternion operator*(const C4Quaternion &l, const C4Quaternion &r) {
    return C4Quaternion(
        l.w * r.w - l.x * r.x - l.y * r.y - l.z * r.z,
        l.w * r.x + l.x * r.w + l.y * r.z - l.z * r.y,
        l.w * r.y + l.y * r.w + l.z * r.x - l.x * r.z,
        l.w * r.z + l.z * r.w + l.x * r.y - l.y * r.x
    );
  }

}  // namespace NTempest
