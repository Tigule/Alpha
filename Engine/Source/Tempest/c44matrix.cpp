#include "Tempest/c44matrix.h"

#include "Tempest/c3vector.h"
#include "Tempest/c4quaternion.h"
#include "Tempest/c4vector.h"

static float Row0Col0_(const NTempest::C44Matrix &l, const NTempest::C44Matrix &r) {
  return l.a0 * r.a0 + l.a1 * r.b0 + l.a2 * r.c0 + l.a3 * r.d0;
}

namespace NTempest {

  bool operator==(
      const C44Matrix &l,
      const C44Matrix &r
  ) {
    return l.a0 == r.a0 && l.a1 == r.a1
        && l.a2 == r.a2 && l.a3 == r.a3
        && l.b0 == r.b0 && l.b1 == r.b1
        && l.b2 == r.b2 && l.b3 == r.b3
        && l.c0 == r.c0 && l.c1 == r.c1
        && l.c2 == r.c2 && l.c3 == r.c3
        && l.d0 == r.d0 && l.d1 == r.d1
        && l.d2 == r.d2 && l.d3 == r.d3;
  }

  bool operator!=(
      const C44Matrix &l,
      const C44Matrix &r
  ) {
    return l.a0 != r.a0 || l.a1 != r.a1
        || l.a2 != r.a2 || l.a3 != r.a3
        || l.b0 != r.b0 || l.b1 != r.b1
        || l.b2 != r.b2 || l.b3 != r.b3
        || l.c0 != r.c0 || l.c1 != r.c1
        || l.c2 != r.c2 || l.c3 != r.c3
        || l.d0 != r.d0 || l.d1 != r.d1
        || l.d2 != r.d2 || l.d3 != r.d3;
  }

  C44Matrix operator+(
      const C44Matrix &l,
      const C44Matrix &r
  ) {
    return C44Matrix(
        l.a0 + r.a0, l.a1 + r.a1, l.a2 + r.a2, l.a3 + r.a3,
        l.b0 + r.b0, l.b1 + r.b1, l.b2 + r.b2, l.b3 + r.b3,
        l.c0 + r.c0, l.c1 + r.c1, l.c2 + r.c2, l.c3 + r.c3,
        l.d0 + r.d0, l.d1 + r.d1, l.d2 + r.d2, l.d3 + r.d3
    );
  }

  C44Matrix operator+(
      const C44Matrix &l,
      float r
  ) {
    return C44Matrix(
        l.a0 + r, l.a1 + r, l.a2 + r, l.a3 + r,
        l.b0 + r, l.b1 + r, l.b2 + r, l.b3 + r,
        l.c0 + r, l.c1 + r, l.c2 + r, l.c3 + r,
        l.d0 + r, l.d1 + r, l.d2 + r, l.d3 + r
    );
  }

  C44Matrix operator+(float l, const C44Matrix &r) {
    return r + l;
  }

  C44Matrix operator-(
      const C44Matrix &l,
      const C44Matrix &r
  ) {
    return C44Matrix(
        l.a0 - r.a0, l.a1 - r.a1, l.a2 - r.a2, l.a3 - r.a3,
        l.b0 - r.b0, l.b1 - r.b1, l.b2 - r.b2, l.b3 - r.b3,
        l.c0 - r.c0, l.c1 - r.c1, l.c2 - r.c2, l.c3 - r.c3,
        l.d0 - r.d0, l.d1 - r.d1, l.d2 - r.d2, l.d3 - r.d3
    );
  }

  C44Matrix operator-(
      const C44Matrix &l,
      float r
  ) {
    return C44Matrix(
        l.a0 - r, l.a1 - r, l.a2 - r, l.a3 - r,
        l.b0 - r, l.b1 - r, l.b2 - r, l.b3 - r,
        l.c0 - r, l.c1 - r, l.c2 - r, l.c3 - r,
        l.d0 - r, l.d1 - r, l.d2 - r, l.d3 - r
    );
  }

  C44Matrix operator*(const C44Matrix &l, const C44Matrix &r) {
    return C44Matrix(
        Row0Col0_(l, r), l.a0 * r.a1 + l.a1 * r.b1 + l.a2 * r.c1 + l.a3 * r.d1, l.a0 * r.a2 + l.a1 * r.b2 + l.a2 * r.c2 + l.a3 * r.d2,
        l.a0 * r.a3 + l.a1 * r.b3 + l.a2 * r.c3 + l.a3 * r.d3, l.b0 * r.a0 + l.b1 * r.b0 + l.b2 * r.c0 + l.b3 * r.d0,
        l.b0 * r.a1 + l.b1 * r.b1 + l.b2 * r.c1 + l.b3 * r.d1, l.b0 * r.a2 + l.b1 * r.b2 + l.b2 * r.c2 + l.b3 * r.d2,
        l.b0 * r.a3 + l.b1 * r.b3 + l.b2 * r.c3 + l.b3 * r.d3, l.c0 * r.a0 + l.c1 * r.b0 + l.c2 * r.c0 + l.c3 * r.d0,
        l.c0 * r.a1 + l.c1 * r.b1 + l.c2 * r.c1 + l.c3 * r.d1, l.c0 * r.a2 + l.c1 * r.b2 + l.c2 * r.c2 + l.c3 * r.d2,
        l.c0 * r.a3 + l.c1 * r.b3 + l.c2 * r.c3 + l.c3 * r.d3, l.d0 * r.a0 + l.d1 * r.b0 + l.d2 * r.c0 + l.d3 * r.d0,
        l.d0 * r.a1 + l.d1 * r.b1 + l.d2 * r.c1 + l.d3 * r.d1, l.d0 * r.a2 + l.d1 * r.b2 + l.d2 * r.c2 + l.d3 * r.d2,
        l.d0 * r.a3 + l.d1 * r.b3 + l.d2 * r.c3 + l.d3 * r.d3
    );
  }

  C44Matrix operator*(const C44Matrix &l, float r) {
    return C44Matrix(
        l.a0 * r, l.a1 * r, l.a2 * r, l.a3 * r, l.b0 * r, l.b1 * r, l.b2 * r, l.b3 * r, l.c0 * r, l.c1 * r, l.c2 * r, l.c3 * r, l.d0 * r, l.d1 * r,
        l.d2 * r, l.d3 * r
    );
  }

  C44Matrix operator*(float l, const C44Matrix &r) {
    return r * l;
  }

  C44Matrix operator/(const C44Matrix &l, float r) {
    ASSERT(!CMath::fequal_(r, 0.0f));
    return l * (1.0f / r);
  }

  C3Vector operator*(const C3Vector &v, const C44Matrix &r) {
    C3Vector result = C44Matrix::mul3v33m_(v, r);
    result += C3Vector(r.d0, r.d1, r.d2);
    return result;
  }

  C3Vector operator*(
      const C44Matrix &l,
      const C3Vector &v
  ) {
    C3Vector result = C44Matrix::mul3v33m_(l, v);
    result += C3Vector(l.a3, l.b3, l.c3);
    return result;
  }

  C3Vector operator*=(C3Vector &v, const C44Matrix &r) {
    return v = v * r;
  }

  C4Vector operator*(const C4Vector &v, const C44Matrix &r) {
    return C4Vector(
        v.x * r.a0 + v.y * r.b0 + v.z * r.c0 + v.w * r.d0, v.x * r.a1 + v.y * r.b1 + v.z * r.c1 + v.w * r.d1,
        v.x * r.a2 + v.y * r.b2 + v.z * r.c2 + v.w * r.d2, v.x * r.a3 + v.y * r.b3 + v.z * r.c3 + v.w * r.d3
    );
  }

  C4Vector operator*(
      const C44Matrix &l,
      const C4Vector &v
  ) {
    return C4Vector(
        l.a0 * v.x + l.a1 * v.y + l.a2 * v.z + l.a3 * v.w,
        l.b0 * v.x + l.b1 * v.y + l.b2 * v.z + l.b3 * v.w,
        l.c0 * v.x + l.c1 * v.y + l.c2 * v.z + l.c3 * v.w,
        l.d0 * v.x + l.d1 * v.y + l.d2 * v.z + l.d3 * v.w
    );
  }

  C44Matrix &C44Matrix::operator+=(const C44Matrix &a) {
    *this = *this + a;
    return *this;
  }

  C44Matrix &C44Matrix::operator-=(const C44Matrix &a) {
    *this = *this - a;
    return *this;
  }

  C44Matrix &C44Matrix::operator*=(const C44Matrix &a) {
    *this = *this * a;
    return *this;
  }

  C44Matrix &C44Matrix::operator*=(float a) {
    *this = *this * a;
    return *this;
  }

  C44Matrix &C44Matrix::operator/=(float a) {
    *this = *this / a;
    return *this;
  }

  C44Matrix C44Matrix::Transpose() const {
    return C44Matrix(
        a0, b0, c0, d0,
        a1, b1, c1, d1,
        a2, b2, c2, d2,
        a3, b3, c3, d3
    );
  }

  float C44Matrix::Det(
      float a, float b, float c,
      float d, float e, float f,
      float g, float h, float i
  ) {
    return a * (e * i - f * h)
        - b * (d * i - f * g)
        + c * (d * h - e * g);
  }

  float C44Matrix::Determinant() const {
    return a0 * Det(b1, b2, b3, c1, c2, c3, d1, d2, d3)
        - a1 * Det(b0, b2, b3, c0, c2, c3, d0, d2, d3)
        + a2 * Det(b0, b1, b3, c0, c1, c3, d0, d1, d3)
        - a3 * Det(b0, b1, b2, c0, c1, c2, d0, d1, d2);
  }

  C44Matrix C44Matrix::Cofactors() const {
    return C44Matrix(
         Det(b1,b2,b3,c1,c2,c3,d1,d2,d3),
        -Det(b0,b2,b3,c0,c2,c3,d0,d2,d3),
         Det(b0,b1,b3,c0,c1,c3,d0,d1,d3),
        -Det(b0,b1,b2,c0,c1,c2,d0,d1,d2),
        -Det(a1,a2,a3,c1,c2,c3,d1,d2,d3),
         Det(a0,a2,a3,c0,c2,c3,d0,d2,d3),
        -Det(a0,a1,a3,c0,c1,c3,d0,d1,d3),
         Det(a0,a1,a2,c0,c1,c2,d0,d1,d2),
         Det(a1,a2,a3,b1,b2,b3,d1,d2,d3),
        -Det(a0,a2,a3,b0,b2,b3,d0,d2,d3),
         Det(a0,a1,a3,b0,b1,b3,d0,d1,d3),
        -Det(a0,a1,a2,b0,b1,b2,d0,d1,d2),
        -Det(a1,a2,a3,b1,b2,b3,c1,c2,c3),
         Det(a0,a2,a3,b0,b2,b3,c0,c2,c3),
        -Det(a0,a1,a3,b0,b1,b3,c0,c1,c3),
         Det(a0,a1,a2,b0,b1,b2,c0,c1,c2)
    );
  }

  C44Matrix C44Matrix::Adjoint() const {
    return Cofactors().Transpose();
  }

  C44Matrix C44Matrix::Inverse(float det) const {
    ASSERT(CMath::fequal_(det, 0.0f) == false);
    return Adjoint() * (1.0f / det);
  }

  C44Matrix C44Matrix::AffineInverse() const {
    C44Matrix result(a0, b0, c0, 0.0f, a1, b1, c1, 0.0f, a2, b2, c2, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    result.Translate(C3Vector(-d0, -d1, -d2));
    return result;
  }

  C44Matrix C44Matrix::AffineInverse(float scale) const {
    if (CMath::fequal4_(scale, 1.0f)) {
      return AffineInverse();
    }
    C44Matrix result(
        a0, b0, c0, 0.0f,
        a1, b1, c1, 0.0f,
        a2, b2, c2, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
    result.Scale(1.0f / (scale * scale));
    result.Translate(C3Vector(-d0, -d1, -d2));
    return result;
  }

  C44Matrix C44Matrix::AffineInverse(const C3Vector &scale) const {
    C3Vector inverseScale(
        1.0f / scale.x,
        1.0f / scale.y,
        1.0f / scale.z
    );
    C33Matrix rotationScale(
        a0, a1, a2,
        b0, b1, b2,
        c0, c1, c2
    );
    rotationScale.Scale(inverseScale);
    rotationScale = rotationScale.Transpose();
    C44Matrix result(rotationScale);
    result.Scale(inverseScale);
    result.Translate(C3Vector(-d0, -d1, -d2));
    return result;
  }

  C44Matrix C44Matrix::Rotation(float angle, const C3Vector &axis, bool unit) {
    C3Vector axis_(axis);

    if (!unit) {
      axis_.Normalize();
    }

    ASSERT(CMath::fequal_(axis_.Mag(), 1.0f));

    C44Matrix result;
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

  void C44Matrix::Translate(const C3Vector &move) {
    d0 += a0 * move.x + b0 * move.y + c0 * move.z;
    d1 += a1 * move.x + b1 * move.y + c1 * move.z;
    d2 += a2 * move.x + b2 * move.y + c2 * move.z;
  }

  void C44Matrix::Scale(float scale) {
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

  void C44Matrix::Scale(const C3Vector &scale) {
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

  void C44Matrix::Rotate(float angle, const C3Vector &axis, bool unit) {
    *this = Rotation(angle, axis, unit) * *this;
  }

  void C44Matrix::Rotate(const C4Quaternion &rotation) {
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

    C44Matrix matrix;
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

}  // namespace NTempest
