#include <Base/Base.h>

#include "Tempest/c34matrix.h"

#include "Tempest/c33matrix.h"
#include "Tempest/c3vector.h"
#include "Tempest/c4quaternion.h"
#include "Tempest/c4vector.h"
#include "Tempest/cmath.h"

#include <storm.h>

namespace NTempest {

  bool operator==(const C34Matrix &l, const C34Matrix &r) {
    return l.a0 == r.a0 && l.a1 == r.a1 && l.a2 == r.a2 && l.b0 == r.b0 && l.b1 == r.b1 && l.b2 == r.b2 && l.c0 == r.c0 && l.c1 == r.c1 &&
           l.c2 == r.c2 && l.d0 == r.d0 && l.d1 == r.d1 && l.d2 == r.d2;
  }

  bool operator!=(const C34Matrix &l, const C34Matrix &r) {
    return l.a0 != r.a0 || l.a1 != r.a1 || l.a2 != r.a2 || l.b0 != r.b0 || l.b1 != r.b1 || l.b2 != r.b2 || l.c0 != r.c0 || l.c1 != r.c1 ||
           l.c2 != r.c2 || l.d0 != r.d0 || l.d1 != r.d1 || l.d2 != r.d2;
  }

  C34Matrix operator+(const C34Matrix &l, const C34Matrix &r) {
    return C34Matrix(
        l.a0 + r.a0, l.a1 + r.a1, l.a2 + r.a2, l.b0 + r.b0, l.b1 + r.b1, l.b2 + r.b2, l.c0 + r.c0, l.c1 + r.c1, l.c2 + r.c2, l.d0 + r.d0, l.d1 + r.d1,
        l.d2 + r.d2
    );
  }

  C34Matrix operator+(const C34Matrix &l, float a) {
    return C34Matrix(l.a0 + a, l.a1 + a, l.a2 + a, l.b0 + a, l.b1 + a, l.b2 + a, l.c0 + a, l.c1 + a, l.c2 + a, l.d0 + a, l.d1 + a, l.d2 + a);
  }

  C34Matrix operator+(float a, const C34Matrix &r) {
    return C34Matrix(
        a + r.a0, a + r.a1, a + r.a2,
        a + r.b0, a + r.b1, a + r.b2,
        a + r.c0, a + r.c1, a + r.c2,
        a + r.d0, a + r.d1, a + r.d2
    );
  }

  C34Matrix operator-(const C34Matrix &l, const C34Matrix &r) {
    return C34Matrix(
        l.a0 - r.a0, l.a1 - r.a1, l.a2 - r.a2, l.b0 - r.b0, l.b1 - r.b1, l.b2 - r.b2, l.c0 - r.c0, l.c1 - r.c1, l.c2 - r.c2, l.d0 - r.d0, l.d1 - r.d1,
        l.d2 - r.d2
    );
  }

  C34Matrix operator-(const C34Matrix &l, float a) {
    return C34Matrix(l.a0 - a, l.a1 - a, l.a2 - a, l.b0 - a, l.b1 - a, l.b2 - a, l.c0 - a, l.c1 - a, l.c2 - a, l.d0 - a, l.d1 - a, l.d2 - a);
  }

  C34Matrix operator*(const C34Matrix &l, const C34Matrix &r) {
    return C34Matrix(
        l.a0 * r.a0 + l.a1 * r.b0 + l.a2 * r.c0, l.a0 * r.a1 + l.a1 * r.b1 + l.a2 * r.c1, l.a0 * r.a2 + l.a1 * r.b2 + l.a2 * r.c2,
        l.b0 * r.a0 + l.b1 * r.b0 + l.b2 * r.c0, l.b0 * r.a1 + l.b1 * r.b1 + l.b2 * r.c1, l.b0 * r.a2 + l.b1 * r.b2 + l.b2 * r.c2,
        l.c0 * r.a0 + l.c1 * r.b0 + l.c2 * r.c0, l.c0 * r.a1 + l.c1 * r.b1 + l.c2 * r.c1, l.c0 * r.a2 + l.c1 * r.b2 + l.c2 * r.c2,
        l.d0 * r.a0 + l.d1 * r.b0 + l.d2 * r.c0 + r.d0, l.d0 * r.a1 + l.d1 * r.b1 + l.d2 * r.c1 + r.d1, l.d0 * r.a2 + l.d1 * r.b2 + l.d2 * r.c2 + r.d2
    );
  }

  C34Matrix operator*(const C34Matrix &l, float a) {
    return C34Matrix(l.a0 * a, l.a1 * a, l.a2 * a, l.b0 * a, l.b1 * a, l.b2 * a, l.c0 * a, l.c1 * a, l.c2 * a, l.d0 * a, l.d1 * a, l.d2 * a);
  }

  C34Matrix operator*(float a, const C34Matrix &r) {
    return C34Matrix(
        a * r.a0, a * r.a1, a * r.a2,
        a * r.b0, a * r.b1, a * r.b2,
        a * r.c0, a * r.c1, a * r.c2,
        a * r.d0, a * r.d1, a * r.d2
    );
  }

  C3Vector operator*(const C3Vector &l, const C34Matrix &r) {
    return C3Vector(
        l.x * r.a0 + l.y * r.b0 + l.z * r.c0 + r.d0,
        l.x * r.a1 + l.y * r.b1 + l.z * r.c1 + r.d1,
        l.x * r.a2 + l.y * r.b2 + l.z * r.c2 + r.d2
    );
  }

  C3Vector operator*(const C34Matrix &l, const C3Vector &r) {
    return C34Matrix::mul3v33m_(l, r);
  }

  C4Vector operator*(const C4Vector &l, const C34Matrix &r) {
    return C4Vector(
        l.x * r.a0 + l.y * r.b0 + l.z * r.c0 + l.w * r.d0, l.x * r.a1 + l.y * r.b1 + l.z * r.c1 + l.w * r.d1,
        l.x * r.a2 + l.y * r.b2 + l.z * r.c2 + l.w * r.d2, l.w
    );
  }

  C4Vector operator*(const C34Matrix &l, const C4Vector &r) {
    return C4Vector(
        l.a0 * r.x + l.a1 * r.y + l.a2 * r.z, l.b0 * r.x + l.b1 * r.y + l.b2 * r.z, l.c0 * r.x + l.c1 * r.y + l.c2 * r.z,
        l.d0 * r.x + l.d1 * r.y + l.d2 * r.z + r.w
    );
  }

  C3Vector operator*=(C3Vector &v, const C34Matrix &r) {
    v.Set(
        v.x * r.a0 + v.y * r.b0 + v.z * r.c0 + r.d0,
        v.x * r.a1 + v.y * r.b1 + v.z * r.c1 + r.d1,
        v.x * r.a2 + v.y * r.b2 + v.z * r.c2 + r.d2
    );
    return v;
  }

  C34Matrix operator/(const C34Matrix &l, float a) {
    a = 1.0f / a;
    return C34Matrix(l.a0 * a, l.a1 * a, l.a2 * a, l.b0 * a, l.b1 * a, l.b2 * a, l.c0 * a, l.c1 * a, l.c2 * a, l.d0 * a, l.d1 * a, l.d2 * a);
  }

  C34Matrix &C34Matrix::operator+=(const C34Matrix &a) {
    *this = *this + a;
    return *this;
  }

  C34Matrix &C34Matrix::operator-=(const C34Matrix &a) {
    *this = *this - a;
    return *this;
  }

  C34Matrix &C34Matrix::operator*=(const C34Matrix &a) {
    *this = *this * a;
    return *this;
  }

  C34Matrix &C34Matrix::operator*=(float a) {
    *this = *this * a;
    return *this;
  }

  C34Matrix &C34Matrix::operator/=(float a) {
    *this = *this / a;
    return *this;
  }

  C34Matrix C34Matrix::AffineInverse() const {
    C34Matrix matrix(((C33Matrix)*this).Transpose());
    matrix.Translate(C3Vector(-d0, -d1, -d2));
    return matrix;
  }

  C34Matrix C34Matrix::AffineInverse(float uniformScale) const {
    if (CMath::fequal4_(uniformScale, 1.0f)) {
      return AffineInverse();
    }

    C34Matrix matrix(((C33Matrix)*this).Transpose());
    matrix.Scale(1.0f / (uniformScale * uniformScale));
    matrix.Translate(NTempest::C3Vector(-d0, -d1, -d2));
    return matrix;
  }

  C34Matrix C34Matrix::AffineInverse(const C3Vector &nonUniformScale) const {
    C3Vector  s(1.0f / nonUniformScale.x, 1.0f / nonUniformScale.y, 1.0f / nonUniformScale.z);
    C33Matrix rotationScale = *this;
    rotationScale.Scale(s);

    C34Matrix matrix(rotationScale.Transpose());
    matrix.Scale(s);
    matrix.Translate(C3Vector(-d0, -d1, -d2));
    return matrix;
  }

  C34Matrix C34Matrix::Rotation(float angle, const C3Vector &axis, bool unit) {
    C3Vector axis_(axis);

    if (!unit) {
      axis_.Normalize();
    }

    ASSERT(CMath::fequal_(axis_.Mag(), 1.0f));

    float s = CMath::sin_(angle);
    float c = CMath::cos_(angle);
    float xy = axis_.x * axis_.y;
    float yz = axis_.z * axis_.y;
    float xz = axis_.z * axis_.x;
    float xs = axis_.x * s;
    float ys = axis_.y * s;
    float zs = axis_.z * s;
    float one_c = 1.0f - c;

    return C34Matrix(
        (axis_.x * axis_.x) * one_c + c, one_c * xy + zs, one_c * xz - ys, one_c * xy - zs, (axis_.y * axis_.y) * one_c + c, one_c * yz + xs,
        one_c * xz + ys, one_c * yz - xs, (axis_.z * axis_.z) * one_c + c, 0.0f, 0.0f, 0.0f
    );
  }

  void C34Matrix::Translate(const C3Vector &move) {
    d0 += a0 * move.x + b0 * move.y + c0 * move.z;
    d1 += a1 * move.x + b1 * move.y + c1 * move.z;
    d2 += a2 * move.x + b2 * move.y + c2 * move.z;
  }

  void C34Matrix::Scale(const C3Vector &scale) {
    *Row0AsVec3() *= scale.x;
    *Row1AsVec3() *= scale.y;
    *Row2AsVec3() *= scale.z;
  }

  void C34Matrix::Scale(float scale) {
    a0 *= scale;
    a1 *= scale;
    a2 *= scale;
    b0 *= scale;
    b1 *= scale;
    b2 *= scale;
    c0 *= scale;
    c1 *= scale;
    c2 *= scale;
  }

  void C34Matrix::Rotate(const C4Quaternion &rotation) {
    *this = C34Matrix(rotation) * *this;
  }

  void C34Matrix::Rotate(float angle, const C3Vector &axis, bool unit) {
    *this = Rotation(angle, axis, unit) * *this;
  }

}  // namespace NTempest
