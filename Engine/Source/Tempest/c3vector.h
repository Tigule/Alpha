#pragma once

#include "Tempest/cmath.h"

namespace NTempest {

  class C3Vector {
   public:
    enum EAxis {
      C3AXIS_X = 0,
      C3AXIS_Y = 1,
      C3AXIS_Z = 2
    };

    enum {
      eComponents = 3
    };

    C3Vector(float value = 0.0f) : x(value), y(value), z(value) {
    }

    C3Vector(float x, float y, float z) : x(x), y(y), z(z) {
    }

    ~C3Vector() {
    }

    void Set(float tx, float ty, float tz) {
      x = tx;
      y = ty;
      z = tz;
    }

    float SquaredMag() const {
      return x * x + y * y + z * z;
    }

    float Mag() const {
      return CMath::sqrt_(SquaredMag());
    }

    void Normalize() {
      float ooMag = 1.0f / Mag();

      x *= ooMag;
      y *= ooMag;
      z *= ooMag;
    }

    EAxis MajorAxis() const;
    EAxis MinorAxis() const;

    static C3Vector __fastcall Min(const C3Vector &a, const C3Vector &b) {
      return C3Vector(b.x <= a.x ? b.x : a.x, b.y <= a.y ? b.y : a.y, b.z <= a.z ? b.z : a.z);
    }

    static C3Vector __fastcall Max(const C3Vector &a, const C3Vector &b) {
      return C3Vector(b.x >= a.x ? b.x : a.x, b.y >= a.y ? b.y : a.y, b.z >= a.z ? b.z : a.z);
    }

    void Maximize(const C3Vector &a) {
      x = a.x > x ? a.x : x;
      y = a.y > y ? a.y : y;
      z = a.z > z ? a.z : z;
    }

    static float __fastcall Dot(const C3Vector &l, const C3Vector &r) {
      return l.x * r.x + l.y * r.y + l.z * r.z;
    }

    static C3Vector __fastcall Cross(const C3Vector &l, const C3Vector &r) {
      return C3Vector(l.y * r.z - l.z * r.y, l.z * r.x - l.x * r.z, l.x * r.y - l.y * r.x);
    }

    C3Vector &operator+=(const C3Vector &a) {
      x += a.x;
      y += a.y;
      z += a.z;
      return *this;
    }

    C3Vector &operator*=(const C3Vector &a) {
      x *= a.x;
      y *= a.y;
      z *= a.z;
      return *this;
    }

    C3Vector &operator*=(float a) {
      x *= a;
      y *= a;
      z *= a;
      return *this;
    }

    bool operator==(const C3Vector &a) const {
      return x == a.x && y == a.y && z == a.z;
    }

    bool operator!=(const C3Vector &a) const {
      return !(*this == a);
    }

    C3Vector operator-() const {
      return C3Vector(-x, -y, -z);
    }

    const float &operator[](unsigned int sub) const {
      ASSERT(sub < eComponents);
      return (&x)[sub];
    }

    float &operator[](unsigned int sub) {
      ASSERT(sub < eComponents);
      return (&x)[sub];
    }

    float x;
    float y;
    float z;
  };

  inline int __fastcall IsUnitVector(const C3Vector &vector) {
    return CMath::fabs_(vector.SquaredMag() - 1.0f) < 0.0009765625f;
  }

  inline C3Vector __fastcall operator+(const C3Vector &l, const C3Vector &r) {
    return C3Vector(l.x + r.x, l.y + r.y, l.z + r.z);
  }

  inline C3Vector __fastcall operator-(const C3Vector &l, const C3Vector &r) {
    return C3Vector(l.x - r.x, l.y - r.y, l.z - r.z);
  }

  inline C3Vector __fastcall operator*(const C3Vector &l, float r) {
    return C3Vector(l.x * r, l.y * r, l.z * r);
  }

  inline bool __fastcall operator<=(const C3Vector &l, const C3Vector &r) {
    return l.x <= r.x && l.y <= r.y && l.z <= r.z;
  }

  inline bool __fastcall operator>=(const C3Vector &l, const C3Vector &r) {
    return l.x >= r.x && l.y >= r.y && l.z >= r.z;
  }

}  // namespace NTempest
