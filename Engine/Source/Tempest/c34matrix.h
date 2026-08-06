#ifndef ENGINE_SOURCE_TEMPEST_C34MATRIX_H
#define ENGINE_SOURCE_TEMPEST_C34MATRIX_H

#include "Tempest/c33matrix.h"
#include "Tempest/c4vector.h"

namespace NTempest {

  class C4Quaternion;

  class C34Matrix {
   public:
    enum {
      eComponents = 12
    };

    static C3Vector mul3v33m_(const C3Vector &v, const C34Matrix &m) {
      return C3Vector(v.x * m.a0 + v.y * m.b0 + v.z * m.c0, v.x * m.a1 + v.y * m.b1 + v.z * m.c1, v.x * m.a2 + v.y * m.b2 + v.z * m.c2);
    }

    static C3Vector mul3v33m_(const C34Matrix &m, const C3Vector &v) {
      return C3Vector(m.a0 * v.x + m.a1 * v.y + m.a2 * v.z, m.b0 * v.x + m.b1 * v.y + m.b2 * v.z, m.c0 * v.x + m.c1 * v.y + m.c2 * v.z);
    }

    C34Matrix() {
      a0 = 1.0f;
      a1 = 0.0f;
      a2 = 0.0f;
      b0 = 0.0f;
      b1 = 1.0f;
      b2 = 0.0f;
      c0 = 0.0f;
      c1 = 0.0f;
      c2 = 1.0f;
      d0 = 0.0f;
      d1 = 0.0f;
      d2 = 0.0f;
    }

    C34Matrix(float a0, float a1, float a2, float b0, float b1, float b2, float c0, float c1, float c2, float d0, float d1, float d2)
        : a0(a0), a1(a1), a2(a2), b0(b0), b1(b1), b2(b2), c0(c0), c1(c1), c2(c2), d0(d0), d1(d1), d2(d2) {
    }
    explicit C34Matrix(float value)
        : a0(value), a1(value), a2(value), b0(value), b1(value), b2(value), c0(value), c1(value), c2(value), d0(value), d1(value), d2(value) {
    }
    C34Matrix(const C33Matrix &m)
        : a0(m.a0), a1(m.a1), a2(m.a2), b0(m.b0), b1(m.b1), b2(m.b2), c0(m.c0), c1(m.c1), c2(m.c2), d0(0.0f), d1(0.0f), d2(0.0f) {
    }
    C34Matrix(const C3Vector &a, const C3Vector &b, const C3Vector &c)
        : a0(a.x), a1(a.y), a2(a.z), b0(b.x), b1(b.y), b2(b.z), c0(c.x), c1(c.y), c2(c.z), d0(0.0f), d1(0.0f), d2(0.0f) {
    }
    C34Matrix(const C3Vector &a, const C3Vector &b, const C3Vector &c, const C3Vector &d)
        : a0(a.x), a1(a.y), a2(a.z), b0(b.x), b1(b.y), b2(b.z), c0(c.x), c1(c.y), c2(c.z), d0(d.x), d1(d.y), d2(d.z) {
    }

    ~C34Matrix() {
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
    const C3Vector *Row3AsVec3() const {
      return reinterpret_cast<const C3Vector *>(&d0);
    }
    C3Vector *Row3AsVec3() {
      return reinterpret_cast<C3Vector *>(&d0);
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
    C3Vector Row3() const {
      return C3Vector(d0, d1, d2);
    }
    C4Vector Col0() const {
      return C4Vector(a0, b0, c0, d0);
    }
    C4Vector Col1() const {
      return C4Vector(a1, b1, c1, d1);
    }
    C4Vector Col2() const {
      return C4Vector(a2, b2, c2, d2);
    }
    operator C33Matrix() const {
      return C33Matrix(a0, a1, a2, b0, b1, b2, c0, c1, c2);
    }
    C34Matrix &operator=(const C34Matrix &a) {
      a0 = a.a0;
      a1 = a.a1;
      a2 = a.a2;
      b0 = a.b0;
      b1 = a.b1;
      b2 = a.b2;
      c0 = a.c0;
      c1 = a.c1;
      c2 = a.c2;
      d0 = a.d0;
      d1 = a.d1;
      d2 = a.d2;
      return *this;
    }
    C34Matrix &operator+=(const C34Matrix &a);
    C34Matrix &operator-=(const C34Matrix &a);
    C34Matrix &operator*=(const C34Matrix &a);
    C34Matrix &operator*=(float a);
    C34Matrix &operator/=(float a);

    void Zero() {
      a0 = a1 = a2 = b0 = b1 = b2 = c0 = c1 = c2 = d0 = d1 = d2 = 0.0f;
    }
    void Identity() {
      a0 = 1.0f;
      a1 = 0.0f;
      a2 = 0.0f;
      b0 = 0.0f;
      b1 = 1.0f;
      b2 = 0.0f;
      c0 = 0.0f;
      c1 = 0.0f;
      c2 = 1.0f;
      d0 = 0.0f;
      d1 = 0.0f;
      d2 = 0.0f;
    }
    float Trace() const {
      return a0 + b1 + c2;
    }

    static C34Matrix Rotation(float angle, const C3Vector &axis, bool unit);

    C34Matrix AffineInverse() const;
    C34Matrix AffineInverse(float uniformScale) const;
    C34Matrix AffineInverse(const C3Vector &scale) const;

    void Translate(const C3Vector &move);
    void Scale(const C3Vector &scale);
    void Scale(float scale);
    void Rotate(const C4Quaternion &rotation);
    void Rotate(float angle, const C3Vector &axis, bool unit);

    float a0;
    float a1;
    float a2;
    float b0;
    float b1;
    float b2;
    float c0;
    float c1;
    float c2;
    float d0;
    float d1;
    float d2;
  };

  bool      operator==(const C34Matrix &l, const C34Matrix &r);
  bool      operator!=(const C34Matrix &l, const C34Matrix &r);
  C34Matrix operator+(const C34Matrix &l, const C34Matrix &r);
  C34Matrix operator+(const C34Matrix &l, float a);
  C34Matrix operator+(float a, const C34Matrix &r);
  C34Matrix operator-(const C34Matrix &l, const C34Matrix &r);
  C34Matrix operator-(const C34Matrix &l, float a);
  C34Matrix operator*(const C34Matrix &l, const C34Matrix &r);
  C34Matrix operator*(const C34Matrix &l, float a);
  C34Matrix operator*(float a, const C34Matrix &r);
  C3Vector  operator*(const C3Vector &l, const C34Matrix &r);
  C3Vector  operator*(const C34Matrix &l, const C3Vector &r);
  C4Vector  operator*(const C4Vector &l, const C34Matrix &r);
  C4Vector  operator*(const C34Matrix &l, const C4Vector &r);
  C3Vector  operator*=(C3Vector &l, const C34Matrix &r);
  C34Matrix operator/(const C34Matrix &l, float a);

}  // namespace NTempest

#endif
