#pragma once

#include "Tempest/c3vector.h"

namespace NTempest {

  class C2Vector;
  class C4Quaternion;

  class C33Matrix {
   public:
    enum {
      eComponents = 9
    };

    C33Matrix() : a0(1.0f), a1(0.0f), a2(0.0f), b0(0.0f), b1(1.0f), b2(0.0f), c0(0.0f), c1(0.0f), c2(1.0f) {
    }

    C33Matrix(float a0, float a1, float a2, float b0, float b1, float b2, float c0, float c1, float c2)
        : a0(a0), a1(a1), a2(a2), b0(b0), b1(b1), b2(b2), c0(c0), c1(c1), c2(c2) {
    }
    explicit C33Matrix(float value) : a0(value), a1(value), a2(value), b0(value), b1(value), b2(value), c0(value), c1(value), c2(value) {
    }
    C33Matrix(const C3Vector &a, const C3Vector &b, const C3Vector &c)
        : a0(a.x), a1(a.y), a2(a.z), b0(b.x), b1(b.y), b2(b.z), c0(c.x), c1(c.y), c2(c.z) {
    }

    ~C33Matrix() {
    }

    C33Matrix asC33Matrix() const {
      return *this;
    }
    const C33Matrix *asFloatPtr() const {
      return this;
    }
    const float *Access() const {
      return &a0;
    }
    float *Access() {
      return &a0;
    }
    const float *operator[](UINT row) const {
      return &a0 + row * 3;
    }
    float *operator[](UINT row) {
      return &a0 + row * 3;
    }
    C2Vector *Row0AsVec2() {
      return reinterpret_cast<C2Vector *>(&a0);
    }
    C2Vector *Row1AsVec2() {
      return reinterpret_cast<C2Vector *>(&b0);
    }
    C2Vector *Row2AsVec2() {
      return reinterpret_cast<C2Vector *>(&c0);
    }
    const C3Vector *Row0AsVec3() const {
      return reinterpret_cast<const C3Vector *>(&a0);
    }
    C3Vector *Row0AsVec3() {
      return reinterpret_cast<C3Vector *>(&a0);
    }
    const C3Vector *Row1AsVec3() const {
      return reinterpret_cast<const C3Vector *>(&b0);
    }
    C3Vector *Row1AsVec3() {
      return reinterpret_cast<C3Vector *>(&b0);
    }
    const C3Vector *Row2AsVec3() const {
      return reinterpret_cast<const C3Vector *>(&c0);
    }
    C3Vector *Row2AsVec3() {
      return reinterpret_cast<C3Vector *>(&c0);
    }
    C3Vector Row0() const {
      return C3Vector(a0, a1, a2);
    }
    C3Vector Row1() const {
      return C3Vector(b0, b1, b2);
    }
    C3Vector Row2() const {
      return C3Vector(c0, c1, c2);
    }
    C3Vector Col0() const {
      return C3Vector(a0, b0, c0);
    }
    C3Vector Col1() const {
      return C3Vector(a1, b1, c1);
    }
    C3Vector Col2() const {
      return C3Vector(a2, b2, c2);
    }

    C33Matrix &operator+=(const C33Matrix &a) {
      a0 += a.a0;
      a1 += a.a1;
      a2 += a.a2;
      b0 += a.b0;
      b1 += a.b1;
      b2 += a.b2;
      c0 += a.c0;
      c1 += a.c1;
      c2 += a.c2;
      return *this;
    }
    C33Matrix &operator-=(const C33Matrix &a) {
      a0 -= a.a0;
      a1 -= a.a1;
      a2 -= a.a2;
      b0 -= a.b0;
      b1 -= a.b1;
      b2 -= a.b2;
      c0 -= a.c0;
      c1 -= a.c1;
      c2 -= a.c2;
      return *this;
    }
    C33Matrix &operator*=(float a) {
      a0 *= a;
      a1 *= a;
      a2 *= a;
      b0 *= a;
      b1 *= a;
      b2 *= a;
      c0 *= a;
      c1 *= a;
      c2 *= a;
      return *this;
    }
    C33Matrix &operator*=(const C33Matrix &a);
    C33Matrix &operator/=(float a) {
      return *this *= 1.0f / a;
    }
    void Zero() {
      a0 = a1 = a2 = b0 = b1 = b2 = c0 = c1 = c2 = 0.0f;
    }
    void Identity() {
      Zero();
      a0 = b1 = c2 = 1.0f;
    }
    float Trace() const {
      return a0 + b1 + c2;
    }

   protected:
    static float Det(float a, float b, float c, float d) {
      return a * d - b * c;
    }

   public:
    static C33Matrix Rotation(float angle, const C3Vector &axis, bool unit);
    static C33Matrix Rotation(float angle);
    float            Determinant() const;
    C33Matrix        Cofactors() const;
    C33Matrix        Adjoint() const;
    C33Matrix        Inverse(float determinant) const;
    C33Matrix        Inverse() const {
      return Inverse(Determinant());
    }
    C33Matrix AffineInverse() const {
      return Transpose();
    }
    C33Matrix AffineInverse(float scale) const;
    C33Matrix AffineInverse(const C3Vector &scale) const;
    void      Scale(float x, float y, float z);
    void      Scale(float scale);
    void      Scale(const C3Vector &scale);
    void      Scale(float x, float y);
    void      Scale(const C2Vector &scale);
    void      Rotate(float angle, const C3Vector &axis, bool unit);
    void      Rotate(const C4Quaternion &rotation);
    void      Rotate(float angle);
    void      Translate(const C2Vector &move);
    bool      ToEulerAnglesXYZ(float &x, float &y, float &z) const;
    bool      ToEulerAnglesXZY(float &x, float &z, float &y) const;
    bool      ToEulerAnglesYXZ(float &y, float &x, float &z) const;
    bool      ToEulerAnglesYZX(float &y, float &z, float &x) const;
    bool      ToEulerAnglesZXY(float &z, float &x, float &y) const;
    bool      ToEulerAnglesZYX(float &z, float &y, float &x) const;
    void      FromEulerAnglesXYZ(float x, float y, float z);
    void      FromEulerAnglesXZY(float x, float z, float y);
    void      FromEulerAnglesYXZ(float y, float x, float z);
    void      FromEulerAnglesYZX(float y, float z, float x);
    void      FromEulerAnglesZXY(float z, float x, float y);
    void      FromEulerAnglesZYX(float yaw, float pitch, float roll);
    C33Matrix Transpose() const;

    float a0;
    float a1;
    float a2;
    float b0;
    float b1;
    float b2;
    float c0;
    float c1;
    float c2;
  };

  C33Matrix operator*(const C33Matrix &l, const C33Matrix &r);

  inline C33Matrix &C33Matrix::operator*=(const C33Matrix &a) {
    *this = *this * a;
    return *this;
  }

  inline C33Matrix operator*(const C33Matrix &l, float a) {
    return C33Matrix(l.a0 * a, l.a1 * a, l.a2 * a, l.b0 * a, l.b1 * a, l.b2 * a, l.c0 * a, l.c1 * a, l.c2 * a);
  }

  inline C33Matrix operator/(const C33Matrix &l, float a) {
    return l * (1.0f / a);
  }

  inline C3Vector operator*(const C33Matrix &l, const C3Vector &v) {
    return C3Vector(v.x * l.a0 + v.y * l.b0 + v.z * l.c0, v.x * l.a1 + v.y * l.b1 + v.z * l.c1, v.x * l.a2 + v.y * l.b2 + v.z * l.c2);
  }

}  // namespace NTempest
