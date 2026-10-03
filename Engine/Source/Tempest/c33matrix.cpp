#include <Base/Base.h>

#include "Tempest/c33matrix.h"

#include "Tempest/c2vector.h"
#include "Tempest/c3vector.h"
#include "Tempest/c4quaternion.h"

namespace NTempest {

  C33Matrix operator*(const C33Matrix &l, const C33Matrix &r) {
    return C33Matrix(
        l.a0 * r.a0 + l.a1 * r.b0 + l.a2 * r.c0, l.a0 * r.a1 + l.a1 * r.b1 + l.a2 * r.c1, l.a0 * r.a2 + l.a1 * r.b2 + l.a2 * r.c2,
        l.b0 * r.a0 + l.b1 * r.b0 + l.b2 * r.c0, l.b0 * r.a1 + l.b1 * r.b1 + l.b2 * r.c1, l.b0 * r.a2 + l.b1 * r.b2 + l.b2 * r.c2,
        l.c0 * r.a0 + l.c1 * r.b0 + l.c2 * r.c0, l.c0 * r.a1 + l.c1 * r.b1 + l.c2 * r.c1, l.c0 * r.a2 + l.c1 * r.b2 + l.c2 * r.c2
    );
  }

  float C33Matrix::Determinant() const {
    return a1 * b2 * c0 + a2 * b0 * c1 + a0 * b1 * c2 - a2 * b1 * c0 - a1 * b0 * c2 - a0 * b2 * c1;
  }

  C33Matrix C33Matrix::Cofactors() const {
    return C33Matrix(
        Det(b1, b2, c1, c2), -Det(b0, b2, c0, c2), Det(b0, b1, c0, c1), -Det(a1, a2, c1, c2), Det(a0, a2, c0, c2), -Det(a0, a1, c0, c1),
        Det(a1, a2, b1, b2), -Det(a0, a2, b0, b2), Det(a0, a1, b0, b1)
    );
  }

  C33Matrix C33Matrix::Adjoint() const {
    return C33Matrix(
        Det(b1, b2, c1, c2), -Det(a1, a2, c1, c2), Det(a1, a2, b1, b2), -Det(b0, b2, c0, c2), Det(a0, a2, c0, c2), -Det(a0, a2, b0, b2),
        Det(b0, b1, c0, c1), -Det(a0, a1, c0, c1), Det(a0, a1, b0, b1)
    );
  }

  C33Matrix C33Matrix::Inverse(float det) const {
    ASSERT(CMath::fequal_(det, 0.0f) == false);
    return Adjoint() * (1.0f / det);
  }

  C33Matrix C33Matrix::AffineInverse(float scale) const {
    if (CMath::fequal4_(scale, 1.0f)) {
      return Transpose();
    }
    C33Matrix matrix = Transpose();
    matrix.Scale(1.0f / (scale * scale));
    return matrix;
  }

  C33Matrix C33Matrix::AffineInverse(const C3Vector &scale) const {
    C3Vector  s(1.0f / scale.x, 1.0f / scale.y, 1.0f / scale.z);
    C33Matrix rotationScale = *this;
    rotationScale.Scale(s);
    C33Matrix matrix = rotationScale.Transpose();
    matrix.Scale(s);
    return matrix;
  }

  C33Matrix C33Matrix::Rotation(float angle, const C3Vector &axis, bool unit) {
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

    return C33Matrix(
        (axis_.x * axis_.x) * one_c + c, one_c * xy + zs, one_c * xz - ys, one_c * xy - zs, (axis_.y * axis_.y) * one_c + c, one_c * yz + xs,
        one_c * xz + ys, one_c * yz - xs, (axis_.z * axis_.z) * one_c + c
    );
  }

  C33Matrix C33Matrix::Rotation(float angle) {
    float sine;
    float cosine;
    CMath::sincos_(angle, sine, cosine);
    return C33Matrix(cosine, sine, 0.0f, -sine, cosine, 0.0f, 0.0f, 0.0f, 1.0f);
  }

  void C33Matrix::Scale(float scale) {
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
    *Row0AsVec3() *= scale.x;
    *Row1AsVec3() *= scale.y;
    *Row2AsVec3() *= scale.z;
  }

  void C33Matrix::Rotate(float angle, const C3Vector &axis, bool unit) {
    *this = Rotation(angle, axis, unit) * *this;
  }

  void C33Matrix::Rotate(const C4Quaternion &rotation) {
    *this = static_cast<C33Matrix>(rotation) * *this;
  }

  void C33Matrix::Translate(const C2Vector &move) {
    c0 += a0 * move.x + b0 * move.y;
    c1 += a1 * move.x + b1 * move.y;
  }

  void C33Matrix::Scale(float x, float y) {
    a0 *= x;
    a1 *= x;
    b0 *= y;
    b1 *= y;
  }

  void C33Matrix::Scale(const C2Vector &scale) {
    *Row0AsVec2() *= scale.x;
    *Row1AsVec2() *= scale.y;
  }

  void C33Matrix::Rotate(float angle) {
    *this = Rotation(angle) * *this;
  }

  bool C33Matrix::ToEulerAnglesXYZ(float &x, float &y, float &z) const {
    if (c0 < 1.0f) {
      if (c0 > -1.0f) {
        x = static_cast<float>(atan2(-c1, c2));
        y = static_cast<float>(asin(c0));
        z = static_cast<float>(atan2(-b0, a0));
        return true;
      } else {
        x = -static_cast<float>(atan2(a1, b1));
        y = -1.5707964f;
        z = 0.0f;
        return false;
      }
    } else {
      x = static_cast<float>(atan2(a1, b1));
      y = 1.5707964f;
      z = 0.0f;
      return false;
    }
  }

  bool C33Matrix::ToEulerAnglesXZY(float &x, float &z, float &y) const {
    if (b0 < 1.0f) {
      if (b0 > -1.0f) {
        x = static_cast<float>(atan2(b2, b1));
        z = static_cast<float>(asin(-b0));
        y = static_cast<float>(atan2(c0, a0));
        return true;
      } else {
        x = static_cast<float>(atan2(a2, c2));
        z = 1.5707964f;
        y = 0.0f;
        return false;
      }
    } else {
      x = static_cast<float>(atan2(-a2, c2));
      z = -1.5707964f;
      y = 0.0f;
      return false;
    }
  }

  bool C33Matrix::ToEulerAnglesYXZ(float &y, float &x, float &z) const {
    if (c1 < 1.0f) {
      if (c1 > -1.0f) {
        y = static_cast<float>(atan2(c0, c2));
        x = static_cast<float>(asin(-c1));
        z = static_cast<float>(atan2(a1, b1));
        return true;
      } else {
        y = static_cast<float>(atan2(b0, a0));
        x = 1.5707964f;
        z = 0.0f;
        return false;
      }
    } else {
      y = static_cast<float>(atan2(-b0, a0));
      x = -1.5707964f;
      z = 0.0f;
      return false;
    }
  }

  bool C33Matrix::ToEulerAnglesYZX(float &y, float &z, float &x) const {
    if (a1 < 1.0f) {
      if (a1 > -1.0f) {
        y = static_cast<float>(atan2(-a2, a0));
        z = static_cast<float>(asin(a1));
        x = static_cast<float>(atan2(-c1, b1));
        return true;
      } else {
        y = -static_cast<float>(atan2(b2, c2));
        z = -1.5707964f;
        x = 0.0f;
        return false;
      }
    } else {
      y = static_cast<float>(atan2(b2, c2));
      z = 1.5707964f;
      x = 0.0f;
      return false;
    }
  }

  bool C33Matrix::ToEulerAnglesZXY(float &z, float &x, float &y) const {
    if (b2 < 1.0f) {
      if (b2 > -1.0f) {
        z = static_cast<float>(atan2(-b0, b1));
        x = static_cast<float>(asin(b2));
        y = static_cast<float>(atan2(-a2, c2));
        return true;
      } else {
        z = -static_cast<float>(atan2(c0, a0));
        x = -1.5707964f;
        y = 0.0f;
        return false;
      }
    } else {
      z = static_cast<float>(atan2(c0, a0));
      x = 1.5707964f;
      y = 0.0f;
      return false;
    }
  }

  bool C33Matrix::ToEulerAnglesZYX(float &z, float &y, float &x) const {
    if (a2 < 1.0f) {
      if (a2 > -1.0f) {
        z = static_cast<float>(atan2(a1, a0));
        y = static_cast<float>(asin(-a2));
        x = static_cast<float>(atan2(b2, c2));
        return true;
      } else {
        z = -static_cast<float>(atan2(b0, c0));
        y = 1.5707964f;
        x = 0.0f;
        return false;
      }
    } else {
      z = static_cast<float>(atan2(-b0, -c0));
      y = -1.5707964f;
      x = 0.0f;
      return false;
    }
  }

  void C33Matrix::FromEulerAnglesXYZ(float x, float y, float z) {
    C33Matrix x_(1.0f, 0.0f, 0.0f, 0.0f, CMath::cos_(x), -CMath::sin_(x), 0.0f, CMath::sin_(x), CMath::cos_(x));
    C33Matrix y_(CMath::cos_(y), 0.0f, CMath::sin_(y), 0.0f, 1.0f, 0.0f, -CMath::sin_(y), 0.0f, CMath::cos_(y));
    C33Matrix z_(CMath::cos_(z), -CMath::sin_(z), 0.0f, CMath::sin_(z), CMath::cos_(z), 0.0f, 0.0f, 0.0f, 1.0f);
    *this = (x_ * (y_ * z_)).Transpose();
  }

  void C33Matrix::FromEulerAnglesXZY(float x, float z, float y) {
    C33Matrix x_(1.0f, 0.0f, 0.0f, 0.0f, CMath::cos_(x), -CMath::sin_(x), 0.0f, CMath::sin_(x), CMath::cos_(x));
    C33Matrix z_(CMath::cos_(z), -CMath::sin_(z), 0.0f, CMath::sin_(z), CMath::cos_(z), 0.0f, 0.0f, 0.0f, 1.0f);
    C33Matrix y_(CMath::cos_(y), 0.0f, CMath::sin_(y), 0.0f, 1.0f, 0.0f, -CMath::sin_(y), 0.0f, CMath::cos_(y));
    *this = (x_ * (z_ * y_)).Transpose();
  }

  void C33Matrix::FromEulerAnglesYXZ(float y, float x, float z) {
    C33Matrix y_(CMath::cos_(y), 0.0f, CMath::sin_(y), 0.0f, 1.0f, 0.0f, -CMath::sin_(y), 0.0f, CMath::cos_(y));
    C33Matrix x_(1.0f, 0.0f, 0.0f, 0.0f, CMath::cos_(x), -CMath::sin_(x), 0.0f, CMath::sin_(x), CMath::cos_(x));
    C33Matrix z_(CMath::cos_(z), -CMath::sin_(z), 0.0f, CMath::sin_(z), CMath::cos_(z), 0.0f, 0.0f, 0.0f, 1.0f);
    *this = (y_ * (x_ * z_)).Transpose();
  }

  void C33Matrix::FromEulerAnglesYZX(float y, float z, float x) {
    C33Matrix y_(CMath::cos_(y), 0.0f, CMath::sin_(y), 0.0f, 1.0f, 0.0f, -CMath::sin_(y), 0.0f, CMath::cos_(y));
    C33Matrix z_(CMath::cos_(z), -CMath::sin_(z), 0.0f, CMath::sin_(z), CMath::cos_(z), 0.0f, 0.0f, 0.0f, 1.0f);
    C33Matrix x_(1.0f, 0.0f, 0.0f, 0.0f, CMath::cos_(x), -CMath::sin_(x), 0.0f, CMath::sin_(x), CMath::cos_(x));
    *this = (y_ * (z_ * x_)).Transpose();
  }

  void C33Matrix::FromEulerAnglesZXY(float z, float x, float y) {
    C33Matrix z_(CMath::cos_(z), -CMath::sin_(z), 0.0f, CMath::sin_(z), CMath::cos_(z), 0.0f, 0.0f, 0.0f, 1.0f);
    C33Matrix x_(1.0f, 0.0f, 0.0f, 0.0f, CMath::cos_(x), -CMath::sin_(x), 0.0f, CMath::sin_(x), CMath::cos_(x));
    C33Matrix y_(CMath::cos_(y), 0.0f, CMath::sin_(y), 0.0f, 1.0f, 0.0f, -CMath::sin_(y), 0.0f, CMath::cos_(y));
    *this = (z_ * (x_ * y_)).Transpose();
  }

  void C33Matrix::FromEulerAnglesZYX(float yaw, float pitch, float roll) {
    C33Matrix x_(1.0f, 0.0f, 0.0f, 0.0f, CMath::cos_(roll), -CMath::sin_(roll), 0.0f, CMath::sin_(roll), CMath::cos_(roll));
    C33Matrix z_(CMath::cos_(yaw), -CMath::sin_(yaw), 0.0f, CMath::sin_(yaw), CMath::cos_(yaw), 0.0f, 0.0f, 0.0f, 1.0f);
    C33Matrix y_(CMath::cos_(pitch), 0.0f, CMath::sin_(pitch), 0.0f, 1.0f, 0.0f, -CMath::sin_(pitch), 0.0f, CMath::cos_(pitch));

    *this = (z_ * (y_ * x_)).Transpose();
  }

}  // namespace NTempest
