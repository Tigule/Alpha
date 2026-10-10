#include <Base/Base.h>

#include "Tempest/c44matrix.h"

#include "Tempest/c3vector.h"
#include "Tempest/c4quaternion.h"
#include "Tempest/c4vector.h"

float Row0Col0_(const NTempest::C44Matrix &l, const NTempest::C44Matrix &r) {
  return l.a0 * r.a0 + l.a1 * r.b0 + l.a2 * r.c0 + l.a3 * r.d0;
}

namespace NTempest {

  float C44Matrix::Det(float a, float b, float c, float d, float e, float f, float g, float h, float i) {
    return b * f * g + c * d * h + a * e * i - c * e * g - b * d * i - a * f * h;
  }

  bool operator==(const C44Matrix &l, const C44Matrix &r) {
    return l.a0 == r.a0 && l.a1 == r.a1 && l.a2 == r.a2 && l.a3 == r.a3 && l.b0 == r.b0 && l.b1 == r.b1 && l.b2 == r.b2 && l.b3 == r.b3 &&
           l.c0 == r.c0 && l.c1 == r.c1 && l.c2 == r.c2 && l.c3 == r.c3 && l.d0 == r.d0 && l.d1 == r.d1 && l.d2 == r.d2 && l.d3 == r.d3;
  }

  bool operator!=(const C44Matrix &l, const C44Matrix &r) {
    return l.a0 != r.a0 || l.a1 != r.a1 || l.a2 != r.a2 || l.a3 != r.a3 || l.b0 != r.b0 || l.b1 != r.b1 || l.b2 != r.b2 || l.b3 != r.b3 ||
           l.c0 != r.c0 || l.c1 != r.c1 || l.c2 != r.c2 || l.c3 != r.c3 || l.d0 != r.d0 || l.d1 != r.d1 || l.d2 != r.d2 || l.d3 != r.d3;
  }

  C44Matrix operator+(const C44Matrix &l, const C44Matrix &r) {
    return C44Matrix(
        l.a0 + r.a0, l.a1 + r.a1, l.a2 + r.a2, l.a3 + r.a3, l.b0 + r.b0, l.b1 + r.b1, l.b2 + r.b2, l.b3 + r.b3, l.c0 + r.c0, l.c1 + r.c1, l.c2 + r.c2,
        l.c3 + r.c3, l.d0 + r.d0, l.d1 + r.d1, l.d2 + r.d2, l.d3 + r.d3
    );
  }

  C44Matrix operator+(const C44Matrix &l, float r) {
    return C44Matrix(
        l.a0 + r, l.a1 + r, l.a2 + r, l.a3 + r, l.b0 + r, l.b1 + r, l.b2 + r, l.b3 + r, l.c0 + r, l.c1 + r, l.c2 + r, l.c3 + r, l.d0 + r, l.d1 + r,
        l.d2 + r, l.d3 + r
    );
  }

  C44Matrix operator+(float a, const C44Matrix &r) {
    return C44Matrix(
        a + r.a0, a + r.a1, a + r.a2, a + r.a3,
        a + r.b0, a + r.b1, a + r.b2, a + r.b3,
        a + r.c0, a + r.c1, a + r.c2, a + r.c3,
        a + r.d0, a + r.d1, a + r.d2, a + r.d3
    );
  }

  C44Matrix operator-(const C44Matrix &l, const C44Matrix &r) {
    return C44Matrix(
        l.a0 - r.a0, l.a1 - r.a1, l.a2 - r.a2, l.a3 - r.a3, l.b0 - r.b0, l.b1 - r.b1, l.b2 - r.b2, l.b3 - r.b3, l.c0 - r.c0, l.c1 - r.c1, l.c2 - r.c2,
        l.c3 - r.c3, l.d0 - r.d0, l.d1 - r.d1, l.d2 - r.d2, l.d3 - r.d3
    );
  }

  C44Matrix operator-(const C44Matrix &l, float r) {
    return C44Matrix(
        l.a0 - r, l.a1 - r, l.a2 - r, l.a3 - r, l.b0 - r, l.b1 - r, l.b2 - r, l.b3 - r, l.c0 - r, l.c1 - r, l.c2 - r, l.c3 - r, l.d0 - r, l.d1 - r,
        l.d2 - r, l.d3 - r
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

  C44Matrix operator*(float a, const C44Matrix &r) {
    return C44Matrix(
        a * r.a0, a * r.a1, a * r.a2, a * r.a3,
        a * r.b0, a * r.b1, a * r.b2, a * r.b3,
        a * r.c0, a * r.c1, a * r.c2, a * r.c3,
        a * r.d0, a * r.d1, a * r.d2, a * r.d3
    );
  }

  C44Matrix operator/(const C44Matrix &l, float a) {
    float a_ = 1.0f / a;
    return C44Matrix(
        l.a0 * a_, l.a1 * a_, l.a2 * a_, l.a3 * a_, l.b0 * a_, l.b1 * a_, l.b2 * a_, l.b3 * a_, l.c0 * a_, l.c1 * a_, l.c2 * a_, l.c3 * a_,
        l.d0 * a_, l.d1 * a_, l.d2 * a_, l.d3 * a_
    );
  }

  C3Vector operator*(const C3Vector &v, const C44Matrix &r) {
    return C3Vector(
        v.x * r.a0 + v.y * r.b0 + v.z * r.c0 + r.d0,
        v.x * r.a1 + v.y * r.b1 + v.z * r.c1 + r.d1,
        v.x * r.a2 + v.y * r.b2 + v.z * r.c2 + r.d2
    );
  }

  C3Vector operator*(const C44Matrix &l, const C3Vector &v) {
    return C3Vector(
        l.a0 * v.x + l.a1 * v.y + l.a2 * v.z + l.a3,
        l.b0 * v.x + l.b1 * v.y + l.b2 * v.z + l.b3,
        l.c0 * v.x + l.c1 * v.y + l.c2 * v.z + l.c3
    );
  }

  C3Vector operator*=(C3Vector &v, const C44Matrix &r) {
    v.Set(
        v.x * r.a0 + v.y * r.b0 + v.z * r.c0 + r.d0,
        v.x * r.a1 + v.y * r.b1 + v.z * r.c1 + r.d1,
        v.x * r.a2 + v.y * r.b2 + v.z * r.c2 + r.d2
    );
    return v;
  }

  C4Vector operator*(const C4Vector &v, const C44Matrix &r) {
    return C4Vector(
        v.x * r.a0 + v.y * r.b0 + v.z * r.c0 + v.w * r.d0, v.x * r.a1 + v.y * r.b1 + v.z * r.c1 + v.w * r.d1,
        v.x * r.a2 + v.y * r.b2 + v.z * r.c2 + v.w * r.d2, v.x * r.a3 + v.y * r.b3 + v.z * r.c3 + v.w * r.d3
    );
  }

  C4Vector operator*(const C44Matrix &l, const C4Vector &v) {
    return C4Vector(
        l.a0 * v.x + l.a1 * v.y + l.a2 * v.z + l.a3 * v.w, l.b0 * v.x + l.b1 * v.y + l.b2 * v.z + l.b3 * v.w,
        l.c0 * v.x + l.c1 * v.y + l.c2 * v.z + l.c3 * v.w, l.d0 * v.x + l.d1 * v.y + l.d2 * v.z + l.d3 * v.w
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
    return C44Matrix(a0, b0, c0, d0, a1, b1, c1, d1, a2, b2, c2, d2, a3, b3, c3, d3);
  }

  float C44Matrix::Determinant() const {
    return a0 * Det(b1, b2, b3, c1, c2, c3, d1, d2, d3) - a1 * Det(b0, b2, b3, c0, c2, c3, d0, d2, d3) +
           a2 * Det(b0, b1, b3, c0, c1, c3, d0, d1, d3) - a3 * Det(b0, b1, b2, c0, c1, c2, d0, d1, d2);
  }

  C44Matrix C44Matrix::Cofactors() const {
    return C44Matrix(
        Det(b1, b2, b3, c1, c2, c3, d1, d2, d3), -Det(b0, b2, b3, c0, c2, c3, d0, d2, d3), Det(b0, b1, b3, c0, c1, c3, d0, d1, d3),
        -Det(b0, b1, b2, c0, c1, c2, d0, d1, d2), -Det(a1, a2, a3, c1, c2, c3, d1, d2, d3), Det(a0, a2, a3, c0, c2, c3, d0, d2, d3),
        -Det(a0, a1, a3, c0, c1, c3, d0, d1, d3), Det(a0, a1, a2, c0, c1, c2, d0, d1, d2), Det(a1, a2, a3, b1, b2, b3, d1, d2, d3),
        -Det(a0, a2, a3, b0, b2, b3, d0, d2, d3), Det(a0, a1, a3, b0, b1, b3, d0, d1, d3), -Det(a0, a1, a2, b0, b1, b2, d0, d1, d2),
        -Det(a1, a2, a3, b1, b2, b3, c1, c2, c3), Det(a0, a2, a3, b0, b2, b3, c0, c2, c3), -Det(a0, a1, a3, b0, b1, b3, c0, c1, c3),
        Det(a0, a1, a2, b0, b1, b2, c0, c1, c2)
    );
  }

  C44Matrix C44Matrix::Adjoint() const {
    return C44Matrix(
        Det(b1, b2, b3, c1, c2, c3, d1, d2, d3), -Det(a1, a2, a3, c1, c2, c3, d1, d2, d3), Det(a1, a2, a3, b1, b2, b3, d1, d2, d3),
        -Det(a1, a2, a3, b1, b2, b3, c1, c2, c3), -Det(b0, b2, b3, c0, c2, c3, d0, d2, d3), Det(a0, a2, a3, c0, c2, c3, d0, d2, d3),
        -Det(a0, a2, a3, b0, b2, b3, d0, d2, d3), Det(a0, a2, a3, b0, b2, b3, c0, c2, c3), Det(b0, b1, b3, c0, c1, c3, d0, d1, d3),
        -Det(a0, a1, a3, c0, c1, c3, d0, d1, d3), Det(a0, a1, a3, b0, b1, b3, d0, d1, d3), -Det(a0, a1, a3, b0, b1, b3, c0, c1, c3),
        -Det(b0, b1, b2, c0, c1, c2, d0, d1, d2), Det(a0, a1, a2, c0, c1, c2, d0, d1, d2), -Det(a0, a1, a2, b0, b1, b2, d0, d1, d2),
        Det(a0, a1, a2, b0, b1, b2, c0, c1, c2)
    );
  }

  C44Matrix C44Matrix::Inverse(float det) const {
    ASSERT(CMath::fequal_(det, 0.0f) == false);
    return Adjoint() * (1.0f / det);
  }

  C44Matrix C44Matrix::AffineInverse() const {
    C44Matrix matrix(((C33Matrix)*this).Transpose());
    matrix.Translate(C3Vector(-d0, -d1, -d2));
    return matrix;
  }

  C44Matrix C44Matrix::AffineInverse(float uniformScale) const {
    if (CMath::fequal4_(uniformScale, 1.0f)) {
      return AffineInverse();
    }
    C44Matrix matrix(((C33Matrix)*this).Transpose());
    matrix.Scale(1.0f / (uniformScale * uniformScale));
    matrix.Translate(C3Vector(-d0, -d1, -d2));
    return matrix;
  }

  C44Matrix C44Matrix::AffineInverse(const C3Vector &nonUniformScale) const {
    C3Vector  s(1.0f / nonUniformScale.x, 1.0f / nonUniformScale.y, 1.0f / nonUniformScale.z);
    C33Matrix rotationScale = *this;
    rotationScale.Scale(s);
    C44Matrix matrix(rotationScale.Transpose());
    matrix.Scale(s);
    matrix.Translate(C3Vector(-d0, -d1, -d2));
    return matrix;
  }

  C44Matrix C44Matrix::Rotation(float angle, const C3Vector &axis, bool unit) {
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

    return C44Matrix(
        (axis_.x * axis_.x) * one_c + c, one_c * xy + zs, one_c * xz - ys, 0.0f, one_c * xy - zs, (axis_.y * axis_.y) * one_c + c,
        one_c * yz + xs, 0.0f, one_c * xz + ys, one_c * yz - xs, (axis_.z * axis_.z) * one_c + c, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f
    );
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
    *Row0AsVec3() *= scale.x;
    *Row1AsVec3() *= scale.y;
    *Row2AsVec3() *= scale.z;
  }

  void C44Matrix::Rotate(float angle, const C3Vector &axis, bool unit) {
    *this = Rotation(angle, axis, unit) * *this;
  }

  void C44Matrix::Rotate(const C4Quaternion &rotation) {
    *this = C44Matrix(rotation) * *this;
  }

}  // namespace NTempest
