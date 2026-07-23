#include "Tempest/c33matrix.h"

#include "Tempest/c3vector.h"

namespace NTempest {

  C33Matrix __fastcall operator*(const C33Matrix &l, const C33Matrix &r) {
    return C33Matrix(
        l.a0 * r.a0 + l.a1 * r.b0 + l.a2 * r.c0, l.a0 * r.a1 + l.a1 * r.b1 + l.a2 * r.c1, l.a0 * r.a2 + l.a1 * r.b2 + l.a2 * r.c2,
        l.b0 * r.a0 + l.b1 * r.b0 + l.b2 * r.c0, l.b0 * r.a1 + l.b1 * r.b1 + l.b2 * r.c1, l.b0 * r.a2 + l.b1 * r.b2 + l.b2 * r.c2,
        l.c0 * r.a0 + l.c1 * r.b0 + l.c2 * r.c0, l.c0 * r.a1 + l.c1 * r.b1 + l.c2 * r.c1, l.c0 * r.a2 + l.c1 * r.b2 + l.c2 * r.c2
    );
  }

  C33Matrix __fastcall C33Matrix::Rotation(float angle, const C3Vector &axis, bool unit) {
    C3Vector axis_(axis);

    if (!unit) {
      axis_.Normalize();
    }

    ASSERT(CMath::fequal_(axis_.Mag(), 1.0f));

    float s = CMath::sin_(angle);
    float c = CMath::cos_(angle);
    float xs = axis_.x * s;
    float ys = axis_.y * s;
    float zs = axis_.z * s;
    float one_c = 1.0f - c;

    return C33Matrix(
        axis_.x * axis_.x * one_c + c, axis_.x * axis_.y * one_c + zs, axis_.x * axis_.z * one_c - ys, axis_.x * axis_.y * one_c - zs,
        axis_.y * axis_.y * one_c + c, axis_.y * axis_.z * one_c + xs, axis_.x * axis_.z * one_c + ys, axis_.y * axis_.z * one_c - xs,
        axis_.z * axis_.z * one_c + c
    );
  }

  void C33Matrix::Scale(float x, float y, float z) {
    a0 *= x;
    a1 *= x;
    a2 *= x;
    b0 *= y;
    b1 *= y;
    b2 *= y;
    c0 *= z;
    c1 *= z;
    c2 *= z;
  }

  void C33Matrix::Scale(const C3Vector &scale) {
    Scale(scale.x, scale.y, scale.z);
  }

  void C33Matrix::Rotate(float angle, const C3Vector &axis, bool unit) {
    *this = Rotation(angle, axis, unit) * *this;
  }

  void C33Matrix::FromEulerAnglesZYX(float yaw, float pitch, float roll) {
    C33Matrix x_(1.0f, 0.0f, 0.0f, 0.0f, CMath::cos_(roll), -CMath::sin_(roll), 0.0f, CMath::sin_(roll), CMath::cos_(roll));
    C33Matrix z_(CMath::cos_(yaw), -CMath::sin_(yaw), 0.0f, CMath::sin_(yaw), CMath::cos_(yaw), 0.0f, 0.0f, 0.0f, 1.0f);
    C33Matrix y_(CMath::cos_(pitch), 0.0f, CMath::sin_(pitch), 0.0f, 1.0f, 0.0f, -CMath::sin_(pitch), 0.0f, CMath::cos_(pitch));

    *this = (z_ * (y_ * x_)).Transpose();
  }

  C33Matrix C33Matrix::Transpose() {
    return C33Matrix(a0, b0, c0, a1, b1, c1, a2, b2, c2);
  }

}  // namespace NTempest
