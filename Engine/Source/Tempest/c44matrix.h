#pragma once

#include "Tempest/c33matrix.h"

#include <string.h>

namespace NTempest {

  class C3Vector;
  class C4Vector;
  class C4Quaternion;

  class C44Matrix {
   public:
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

    ~C44Matrix() {
    }

    C44Matrix &operator=(const C44Matrix &a) {
      memcpy(this, &a, sizeof(*this));
      return *this;
    }

    C44Matrix &operator*=(const C44Matrix &a);

    float     Determinant() const;
    C44Matrix Adjoint() const;
    C44Matrix Inverse(float det) const;
    C44Matrix AffineInverse() const;

    static C44Matrix __fastcall Rotation(float angle, const C3Vector &axis, unsigned int unit);

    void Translate(const C3Vector &move);
    void Scale(float scale);
    void Scale(const C3Vector &scale);
    void Rotate(float angle, const C3Vector &axis, unsigned int unit);
    void Rotate(const C4Quaternion &rotation);
  };

  C3Vector __fastcall     operator*(const C3Vector &v, const C44Matrix &r);
  C3Vector __fastcall     operator*=(C3Vector &v, const C44Matrix &r);
  C4Vector __fastcall     operator*(const C4Vector &v, const C44Matrix &r);
  C44Matrix __fastcall    operator*(const C44Matrix &l, const C44Matrix &r);
  C44Matrix __fastcall    operator*(const C44Matrix &l, float r);
  unsigned int __fastcall operator!=(C44Matrix &l, C44Matrix &r);

}  // namespace NTempest
