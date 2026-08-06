#pragma once

#include "Tempest/c2vector.h"

namespace NTempest {

  class C22Matrix {
   public:
    enum {
      eComponents = 4
    };

    C22Matrix() : a0(1.0f), a1(0.0f), b0(0.0f), b1(1.0f) {
    }
    explicit C22Matrix(float value) : a0(value), a1(value), b0(value), b1(value) {
    }
    C22Matrix(float a0, float a1, float b0, float b1) : a0(a0), a1(a1), b0(b0), b1(b1) {
    }
    C22Matrix(const C2Vector &a, const C2Vector &b) : a0(a.x), a1(a.y), b0(b.x), b1(b.y) {
    }
    ~C22Matrix() {
    }

    static C22Matrix Rotation(float angle);
    C22Matrix        asC22Matrix() const {
      return *this;
    }
    const C22Matrix *asFloatPtr() const {
      return this;
    }
    float *Access() {
      return &a0;
    }
    const float *Access() const {
      return &a0;
    }
    float *operator[](DWORD row) {
      return &a0 + row * 2;
    }
    const float *operator[](DWORD row) const {
      return &a0 + row * 2;
    }
    C2Vector *Row0AsVec2() {
      return reinterpret_cast<C2Vector *>(&a0);
    }
    C2Vector *Row1AsVec2() {
      return reinterpret_cast<C2Vector *>(&b0);
    }
    C2Vector Row0() const {
      return C2Vector(a0, a1);
    }
    C2Vector Row1() const {
      return C2Vector(b0, b1);
    }
    C2Vector Col0() const {
      return C2Vector(a0, b0);
    }
    C2Vector Col1() const {
      return C2Vector(a1, b1);
    }
    C22Matrix &operator+=(const C22Matrix &m) {
      a0 += m.a0;
      a1 += m.a1;
      b0 += m.b0;
      b1 += m.b1;
      return *this;
    }
    C22Matrix &operator-=(const C22Matrix &m) {
      a0 -= m.a0;
      a1 -= m.a1;
      b0 -= m.b0;
      b1 -= m.b1;
      return *this;
    }
    C22Matrix &operator*=(float value) {
      a0 *= value;
      a1 *= value;
      b0 *= value;
      b1 *= value;
      return *this;
    }
    C22Matrix &operator*=(const C22Matrix &m) {
      *this = C22Matrix(a0 * m.a0 + a1 * m.b0, a0 * m.a1 + a1 * m.b1, b0 * m.a0 + b1 * m.b0, b0 * m.a1 + b1 * m.b1);
      return *this;
    }
    C22Matrix &operator/=(float value) {
      return *this *= 1.0f / value;
    }
    void Zero() {
      a0 = a1 = b0 = b1 = 0.0f;
    }
    void Identity() {
      a0 = b1 = 1.0f;
      a1 = b0 = 0.0f;
    }
    float Trace() const {
      return a0 + b1;
    }
    C22Matrix Transpose() const {
      return C22Matrix(a0, b0, a1, b1);
    }
    float Determinant() const {
      return a0 * b1 - a1 * b0;
    }
    C22Matrix Inverse() const {
      return Inverse(Determinant());
    }
    C22Matrix Inverse(float determinant) const {
      float inverse = 1.0f / determinant;
      return C22Matrix(b1 * inverse, -a1 * inverse, -b0 * inverse, a0 * inverse);
    }

    float a0;
    float a1;
    float b0;
    float b1;
  };

}  // namespace NTempest
