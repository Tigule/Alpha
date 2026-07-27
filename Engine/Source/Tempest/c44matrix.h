#pragma once

#include "Tempest/c34matrix.h"
#include "Tempest/c4vector.h"

#include <string.h>

namespace NTempest {

  class C4Quaternion;

  class C44Matrix {
   public:
    static C3Vector mul3v33m_(const C3Vector &v, const C44Matrix &m) {
      return C3Vector(
          v.x * m.a0 + v.y * m.b0 + v.z * m.c0,
          v.x * m.a1 + v.y * m.b1 + v.z * m.c1,
          v.x * m.a2 + v.y * m.b2 + v.z * m.c2
      );
    }

    static C3Vector mul3v33m_(const C44Matrix &m, const C3Vector &v) {
      return C3Vector(
          m.a0 * v.x + m.a1 * v.y + m.a2 * v.z,
          m.b0 * v.x + m.b1 * v.y + m.b2 * v.z,
          m.c0 * v.x + m.c1 * v.y + m.c2 * v.z
      );
    }

    float a0;
    float a1;
    float a2;
    float a3;
    float b0;
    float b1;
    float b2;
    float b3;
    float c0;
    float c1;
    float c2;
    float c3;
    float d0;
    float d1;
    float d2;
    float d3;

    C44Matrix() {
      a0 = 1.0f;
      a1 = 0.0f;
      a2 = 0.0f;
      a3 = 0.0f;
      b0 = 0.0f;
      b1 = 1.0f;
      b2 = 0.0f;
      b3 = 0.0f;
      c0 = 0.0f;
      c1 = 0.0f;
      c2 = 1.0f;
      c3 = 0.0f;
      d0 = 0.0f;
      d1 = 0.0f;
      d2 = 0.0f;
      d3 = 1.0f;
    }

    C44Matrix(
        float a0,
        float a1,
        float a2,
        float a3,
        float b0,
        float b1,
        float b2,
        float b3,
        float c0,
        float c1,
        float c2,
        float c3,
        float d0,
        float d1,
        float d2,
        float d3
    )
        : a0(a0), a1(a1), a2(a2), a3(a3), b0(b0), b1(b1), b2(b2), b3(b3), c0(c0), c1(c1), c2(c2), c3(c3), d0(d0), d1(d1), d2(d2), d3(d3) {
    }

    explicit C44Matrix(float value)
        : a0(value), a1(value), a2(value), a3(value),
          b0(value), b1(value), b2(value), b3(value),
          c0(value), c1(value), c2(value), c3(value),
          d0(value), d1(value), d2(value), d3(value) {
    }

    C44Matrix(const C4Vector &a, const C4Vector &b, const C4Vector &c, const C4Vector &d)
        : a0(a.x), a1(a.y), a2(a.z), a3(a.w),
          b0(b.x), b1(b.y), b2(b.z), b3(b.w),
          c0(c.x), c1(c.y), c2(c.z), c3(c.w),
          d0(d.x), d1(d.y), d2(d.z), d3(d.w) {
    }

    C44Matrix(const C3Vector &a, const C3Vector &b, const C3Vector &c)
        : a0(a.x), a1(a.y), a2(a.z), a3(0.0f),
          b0(b.x), b1(b.y), b2(b.z), b3(0.0f),
          c0(c.x), c1(c.y), c2(c.z), c3(0.0f),
          d0(0.0f), d1(0.0f), d2(0.0f), d3(1.0f) {
    }

    C44Matrix(const C3Vector &a, const C3Vector &b, const C3Vector &c, const C3Vector &d)
        : a0(a.x), a1(a.y), a2(a.z), a3(0.0f),
          b0(b.x), b1(b.y), b2(b.z), b3(0.0f),
          c0(c.x), c1(c.y), c2(c.z), c3(0.0f),
          d0(d.x), d1(d.y), d2(d.z), d3(1.0f) {
    }

    C44Matrix(const C33Matrix &m)
        : a0(m.a0),
          a1(m.a1),
          a2(m.a2),
          a3(0.0f),
          b0(m.b0),
          b1(m.b1),
          b2(m.b2),
          b3(0.0f),
          c0(m.c0),
          c1(m.c1),
          c2(m.c2),
          c3(0.0f),
          d0(0.0f),
          d1(0.0f),
          d2(0.0f),
          d3(1.0f) {
    }

    C44Matrix(const C34Matrix &m)
        : a0(m.a0), a1(m.a1), a2(m.a2), a3(0.0f),
          b0(m.b0), b1(m.b1), b2(m.b2), b3(0.0f),
          c0(m.c0), c1(m.c1), c2(m.c2), c3(0.0f),
          d0(m.d0), d1(m.d1), d2(m.d2), d3(1.0f) {
    }

    ~C44Matrix() {
    }

    const float *Access() const { return &a0; }
    float *Access() { return &a0; }
    const float *operator[](unsigned int row) const { return &a0 + row * 4; }
    float *operator[](unsigned int row) { return &a0 + row * 4; }
    const C3Vector *Row0AsVec3() const { return reinterpret_cast<const C3Vector *>(&a0); }
    C3Vector *Row0AsVec3() { return reinterpret_cast<C3Vector *>(&a0); }
    const C3Vector *Row1AsVec3() const { return reinterpret_cast<const C3Vector *>(&b0); }
    C3Vector *Row1AsVec3() { return reinterpret_cast<C3Vector *>(&b0); }
    const C3Vector *Row2AsVec3() const { return reinterpret_cast<const C3Vector *>(&c0); }
    C3Vector *Row2AsVec3() { return reinterpret_cast<C3Vector *>(&c0); }
    const C3Vector *Row3AsVec3() const { return reinterpret_cast<const C3Vector *>(&d0); }
    C3Vector *Row3AsVec3() { return reinterpret_cast<C3Vector *>(&d0); }
    C4Vector *Row0AsVec4() { return reinterpret_cast<C4Vector *>(&a0); }
    C4Vector *Row1AsVec4() { return reinterpret_cast<C4Vector *>(&b0); }
    C4Vector *Row2AsVec4() { return reinterpret_cast<C4Vector *>(&c0); }
    C4Vector *Row3AsVec4() { return reinterpret_cast<C4Vector *>(&d0); }
    C4Vector Row0() const { return C4Vector(a0, a1, a2, a3); }
    C4Vector Row1() const { return C4Vector(b0, b1, b2, b3); }
    C4Vector Row2() const { return C4Vector(c0, c1, c2, c3); }
    C4Vector Row3() const { return C4Vector(d0, d1, d2, d3); }
    C4Vector Col0() const { return C4Vector(a0, b0, c0, d0); }
    C4Vector Col1() const { return C4Vector(a1, b1, c1, d1); }
    C4Vector Col2() const { return C4Vector(a2, b2, c2, d2); }
    C4Vector Col3() const { return C4Vector(a3, b3, c3, d3); }
    operator C33Matrix() const {
      return C33Matrix(a0, a1, a2, b0, b1, b2, c0, c1, c2);
    }
    operator C34Matrix() const {
      return C34Matrix(a0, a1, a2, b0, b1, b2, c0, c1, c2, d0, d1, d2);
    }

    C44Matrix &operator=(const C44Matrix &a) {
      memcpy(this, &a, sizeof(*this));
      return *this;
    }

    C44Matrix &operator+=(const C44Matrix &a);
    C44Matrix &operator-=(const C44Matrix &a);
    C44Matrix &operator*=(const C44Matrix &a);
    C44Matrix &operator*=(float a);
    C44Matrix &operator/=(float a);

    void Zero() {
      a0 = a1 = a2 = a3 = b0 = b1 = b2 = b3 =
          c0 = c1 = c2 = c3 = d0 = d1 = d2 = d3 = 0.0f;
    }
    void Identity() {
      Zero();
      a0 = b1 = c2 = d3 = 1.0f;
    }
    float Trace() const {
      return a0 + b1 + c2 + d3;
    }

    C44Matrix Transpose() const;
    float     Determinant() const;
    C44Matrix Cofactors() const;
    C44Matrix Adjoint() const;
    C44Matrix Inverse(float det) const;
    C44Matrix Inverse() const {
      return Inverse(Determinant());
    }
    C44Matrix AffineInverse() const;
    C44Matrix AffineInverse(float scale) const;
    C44Matrix AffineInverse(const C3Vector &scale) const;

    static C44Matrix Rotation(float angle, const C3Vector &axis, unsigned int unit);

    void Translate(const C3Vector &move);
    void Scale(float scale);
    void Scale(const C3Vector &scale);
    void Rotate(float angle, const C3Vector &axis, unsigned int unit);
    void Rotate(const C4Quaternion &rotation);

   protected:
    static float Det(
        float a, float b, float c,
        float d, float e, float f,
        float g, float h, float i
    );
  };

  bool operator==(const C44Matrix &l, const C44Matrix &r);
  bool operator!=(const C44Matrix &l, const C44Matrix &r);
  C44Matrix operator+(const C44Matrix &l, const C44Matrix &r);
  C44Matrix operator+(const C44Matrix &l, float r);
  C44Matrix operator+(float l, const C44Matrix &r);
  C44Matrix operator-(const C44Matrix &l, const C44Matrix &r);
  C44Matrix operator-(const C44Matrix &l, float r);
  C3Vector operator*(const C3Vector &v, const C44Matrix &r);
  C3Vector operator*(const C44Matrix &l, const C3Vector &v);
  C3Vector operator*=(C3Vector &v, const C44Matrix &r);
  C4Vector operator*(const C4Vector &v, const C44Matrix &r);
  C4Vector operator*(const C44Matrix &l, const C4Vector &v);
  C44Matrix operator*(const C44Matrix &l, const C44Matrix &r);
  C44Matrix operator*(const C44Matrix &l, float r);
  C44Matrix operator*(float l, const C44Matrix &r);
  C44Matrix operator/(const C44Matrix &l, float r);

}  // namespace NTempest
