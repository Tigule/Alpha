#include "Tempest/c34matrix.h"

#include "Tempest/c33matrix.h"
#include "Tempest/c3vector.h"
#include "Tempest/c4quaternion.h"
#include "Tempest/c4vector.h"
#include "Tempest/cmath.h"

#include <storm.h>

namespace NTempest {

  bool operator==(const C34Matrix &l, const C34Matrix &r) {
    return l.a0 == r.a0 && l.a1 == r.a1 && l.a2 == r.a2
        && l.b0 == r.b0 && l.b1 == r.b1 && l.b2 == r.b2
        && l.c0 == r.c0 && l.c1 == r.c1 && l.c2 == r.c2
        && l.d0 == r.d0 && l.d1 == r.d1 && l.d2 == r.d2;
  }

  bool operator!=(const C34Matrix &l, const C34Matrix &r) {
    return l.a0 != r.a0 || l.a1 != r.a1 || l.a2 != r.a2
        || l.b0 != r.b0 || l.b1 != r.b1 || l.b2 != r.b2
        || l.c0 != r.c0 || l.c1 != r.c1 || l.c2 != r.c2
        || l.d0 != r.d0 || l.d1 != r.d1 || l.d2 != r.d2;
  }

  C34Matrix operator+(const C34Matrix &l, const C34Matrix &r) {
    return C34Matrix(
        l.a0 + r.a0, l.a1 + r.a1, l.a2 + r.a2, l.b0 + r.b0, l.b1 + r.b1, l.b2 + r.b2, l.c0 + r.c0, l.c1 + r.c1, l.c2 + r.c2, l.d0 + r.d0, l.d1 + r.d1,
        l.d2 + r.d2
    );
  }

  C34Matrix operator+(const C34Matrix &l, float a) {
    return C34Matrix(
        l.a0 + a, l.a1 + a, l.a2 + a,
        l.b0 + a, l.b1 + a, l.b2 + a,
        l.c0 + a, l.c1 + a, l.c2 + a,
        l.d0 + a, l.d1 + a, l.d2 + a
    );
  }

  C34Matrix operator+(float a, const C34Matrix &r) {
    return r + a;
  }

  C34Matrix operator-(const C34Matrix &l, const C34Matrix &r) {
    return C34Matrix(
        l.a0 - r.a0, l.a1 - r.a1, l.a2 - r.a2,
        l.b0 - r.b0, l.b1 - r.b1, l.b2 - r.b2,
        l.c0 - r.c0, l.c1 - r.c1, l.c2 - r.c2,
        l.d0 - r.d0, l.d1 - r.d1, l.d2 - r.d2
    );
  }

  C34Matrix operator-(const C34Matrix &l, float a) {
    return C34Matrix(
        l.a0 - a, l.a1 - a, l.a2 - a,
        l.b0 - a, l.b1 - a, l.b2 - a,
        l.c0 - a, l.c1 - a, l.c2 - a,
        l.d0 - a, l.d1 - a, l.d2 - a
    );
  }

  C34Matrix operator*(const C34Matrix &l, const C34Matrix &r) {
    C34Matrix result;

    result.a0 = l.a0 * r.a0 + l.a1 * r.b0 + l.a2 * r.c0;
    result.a1 = l.a0 * r.a1 + l.a1 * r.b1 + l.a2 * r.c1;
    result.a2 = l.a0 * r.a2 + l.a1 * r.b2 + l.a2 * r.c2;

    result.b0 = l.b0 * r.a0 + l.b1 * r.b0 + l.b2 * r.c0;
    result.b1 = l.b0 * r.a1 + l.b1 * r.b1 + l.b2 * r.c1;
    result.b2 = l.b0 * r.a2 + l.b1 * r.b2 + l.b2 * r.c2;

    result.c0 = l.c0 * r.a0 + l.c1 * r.b0 + l.c2 * r.c0;
    result.c1 = l.c0 * r.a1 + l.c1 * r.b1 + l.c2 * r.c1;
    result.c2 = l.c0 * r.a2 + l.c1 * r.b2 + l.c2 * r.c2;

    result.d0 = l.d0 * r.a0 + l.d1 * r.b0 + l.d2 * r.c0 + r.d0;
    result.d1 = l.d0 * r.a1 + l.d1 * r.b1 + l.d2 * r.c1 + r.d1;
    result.d2 = l.d0 * r.a2 + l.d1 * r.b2 + l.d2 * r.c2 + r.d2;

    return result;
  }

  C34Matrix operator*(const C34Matrix &l, float a) {
    return C34Matrix(
        l.a0 * a, l.a1 * a, l.a2 * a,
        l.b0 * a, l.b1 * a, l.b2 * a,
        l.c0 * a, l.c1 * a, l.c2 * a,
        l.d0 * a, l.d1 * a, l.d2 * a
    );
  }

  C34Matrix operator*(float a, const C34Matrix &r) {
    return r * a;
  }

  C3Vector operator*(const C3Vector &l, const C34Matrix &r) {
    C3Vector result = C34Matrix::mul3v33m_(l, r);
    result += C3Vector(r.d0, r.d1, r.d2);
    return result;
  }

  C3Vector operator*(const C34Matrix &l, const C3Vector &r) {
    return C34Matrix::mul3v33m_(l, r);
  }

  C4Vector operator*(const C4Vector &l, const C34Matrix &r) {
    return C4Vector(
        l.x * r.a0 + l.y * r.b0 + l.z * r.c0 + l.w * r.d0,
        l.x * r.a1 + l.y * r.b1 + l.z * r.c1 + l.w * r.d1,
        l.x * r.a2 + l.y * r.b2 + l.z * r.c2 + l.w * r.d2,
        l.w
    );
  }

  C4Vector operator*(const C34Matrix &l, const C4Vector &r) {
    return C4Vector(
        l.a0 * r.x + l.a1 * r.y + l.a2 * r.z,
        l.b0 * r.x + l.b1 * r.y + l.b2 * r.z,
        l.c0 * r.x + l.c1 * r.y + l.c2 * r.z,
        l.d0 * r.x + l.d1 * r.y + l.d2 * r.z + r.w
    );
  }

  C3Vector operator*=(C3Vector &l, const C34Matrix &r) {
    const float x = l.x;
    const float y = l.y;
    const float z = l.z;

    l.x = x * r.a0 + y * r.b0 + z * r.c0 + r.d0;
    l.y = x * r.a1 + y * r.b1 + z * r.c1 + r.d1;
    l.z = x * r.a2 + y * r.b2 + z * r.c2 + r.d2;

    return l;
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
    C34Matrix matrix(a0, b0, c0, a1, b1, c1, a2, b2, c2, 0.0f, 0.0f, 0.0f);
    matrix.d0 = -(matrix.a0 * d0 + matrix.b0 * d1 + matrix.c0 * d2);
    matrix.d1 = -(matrix.a1 * d0 + matrix.b1 * d1 + matrix.c1 * d2);
    matrix.d2 = -(matrix.a2 * d0 + matrix.b2 * d1 + matrix.c2 * d2);
    return matrix;
  }

  C34Matrix C34Matrix::AffineInverse(float uniformScale) const {
    if (CMath::fequal4_(uniformScale, 1.0f)) {
      return AffineInverse();
    }

    C34Matrix matrix(a0, b0, c0, a1, b1, c1, a2, b2, c2, 0.0f, 0.0f, 0.0f);
    matrix.Scale(1.0f / (uniformScale * uniformScale));
    matrix.Translate(NTempest::C3Vector(-d0, -d1, -d2));
    return matrix;
  }

  C34Matrix C34Matrix::AffineInverse(const C3Vector &scale) const {
    C3Vector inverseScale(1.0f / scale.x, 1.0f / scale.y, 1.0f / scale.z);
    C33Matrix rotationScale(a0, a1, a2, b0, b1, b2, c0, c1, c2);
    rotationScale.Scale(inverseScale);

    C34Matrix matrix(
        rotationScale.a0, rotationScale.b0, rotationScale.c0,
        rotationScale.a1, rotationScale.b1, rotationScale.c1,
        rotationScale.a2, rotationScale.b2, rotationScale.c2,
        0.0f, 0.0f, 0.0f
    );
    matrix.Scale(inverseScale);
    matrix.Translate(C3Vector(-d0, -d1, -d2));
    return matrix;
  }

  C34Matrix C34Matrix::Rotation(float angle, const C3Vector &axis, bool unit) {
    C3Vector axis_(axis);

    if (!unit) {
      axis_.Normalize();
    }

    ASSERT(CMath::fequal_(axis_.Mag(), 1.0f));

    C34Matrix result;
    float     s = CMath::sin_(angle);
    float     c = CMath::cos_(angle);
    float     xs = axis_.x * s;
    float     ys = axis_.y * s;
    float     zs = axis_.z * s;
    float     one_c = 1.0f - c;

    result.a0 = axis_.x * axis_.x * one_c + c;
    result.a1 = axis_.x * axis_.y * one_c + zs;
    result.a2 = axis_.x * axis_.z * one_c - ys;

    result.b0 = axis_.x * axis_.y * one_c - zs;
    result.b1 = axis_.y * axis_.y * one_c + c;
    result.b2 = axis_.y * axis_.z * one_c + xs;

    result.c0 = axis_.x * axis_.z * one_c + ys;
    result.c1 = axis_.y * axis_.z * one_c - xs;
    result.c2 = axis_.z * axis_.z * one_c + c;

    return result;
  }

  void C34Matrix::Translate(const C3Vector &move) {
    d0 += a0 * move.x + b0 * move.y + c0 * move.z;
    d1 += a1 * move.x + b1 * move.y + c1 * move.z;
    d2 += a2 * move.x + b2 * move.y + c2 * move.z;
  }

  void C34Matrix::Scale(const C3Vector &scale) {
    a0 *= scale.x;
    a1 *= scale.x;
    a2 *= scale.x;
    b0 *= scale.y;
    b1 *= scale.y;
    b2 *= scale.y;
    c0 *= scale.z;
    c1 *= scale.z;
    c2 *= scale.z;
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
    const float x2 = rotation.x + rotation.x;
    const float y2 = rotation.y + rotation.y;
    const float z2 = rotation.z + rotation.z;
    const float xx = rotation.x * x2;
    const float xy = rotation.x * y2;
    const float xz = rotation.x * z2;
    const float yy = rotation.y * y2;
    const float yz = rotation.y * z2;
    const float zz = rotation.z * z2;
    const float xw = rotation.w * x2;
    const float yw = rotation.w * y2;
    const float zw = rotation.w * z2;

    C34Matrix matrix;
    matrix.a0 = 1.0f - (yy + zz);
    matrix.a1 = xy + zw;
    matrix.a2 = xz - yw;
    matrix.b0 = xy - zw;
    matrix.b1 = 1.0f - (xx + zz);
    matrix.b2 = yz + xw;
    matrix.c0 = xz + yw;
    matrix.c1 = yz - xw;
    matrix.c2 = 1.0f - (xx + yy);

    *this = matrix * *this;
  }

  void C34Matrix::Rotate(float angle, const C3Vector &axis, bool unit) {
    *this = Rotation(angle, axis, unit) * *this;
  }

}  // namespace NTempest
