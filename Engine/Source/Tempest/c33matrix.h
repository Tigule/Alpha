#pragma once

#include "Tempest/c3vector.h"

namespace NTempest {

  class C33Matrix {
   public:
    C33Matrix() : a0(1.0f), a1(0.0f), a2(0.0f), b0(0.0f), b1(1.0f), b2(0.0f), c0(0.0f), c1(0.0f), c2(1.0f) {
    }

    C33Matrix(float a0, float a1, float a2, float b0, float b1, float b2, float c0, float c1, float c2)
        : a0(a0), a1(a1), a2(a2), b0(b0), b1(b1), b2(b2), c0(c0), c1(c1), c2(c2) {
    }

    static C33Matrix __fastcall Rotation(float angle, const C3Vector &axis, bool unit);
    void                        Scale(float x, float y, float z);
    void                        Scale(const C3Vector &scale);
    void                        Rotate(float angle, const C3Vector &axis, bool unit);
    void                        FromEulerAnglesZYX(float yaw, float pitch, float roll);
    C33Matrix                   Transpose();

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

  C33Matrix __fastcall operator*(const C33Matrix &l, const C33Matrix &r);

  inline C3Vector __fastcall operator*(const C33Matrix &l, const C3Vector &v) {
    return C3Vector(v.x * l.a0 + v.y * l.b0 + v.z * l.c0, v.x * l.a1 + v.y * l.b1 + v.z * l.c1, v.x * l.a2 + v.y * l.b2 + v.z * l.c2);
  }

}  // namespace NTempest
