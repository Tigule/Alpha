#pragma once

#include <math.h>

#include "Tempest/c2ivector.h"

namespace NTempest {

  class C3Vector;

  class C3iVector {
   public:
    enum {
      eComponents = 3
    };

    C3iVector(long value = 0) : x(value), y(value), z(value) {
    }

    C3iVector(long xValue, long yValue, long zValue) : x(xValue), y(yValue), z(zValue) {
    }

    C3iVector(const C2iVector &vector) : x(vector.x), y(vector.y), z(0) {
    }

    C3iVector(const C3Vector &vector);

    ~C3iVector() {
    }

    void Get(long &xValue, long &yValue, long &zValue) const {
      xValue = x;
      yValue = y;
      zValue = z;
    }

    void Set(long xValue, long yValue, long zValue) {
      x = xValue;
      y = yValue;
      z = zValue;
    }

    operator C2iVector() const {
      return C2iVector(x, y);
    }

    static C3iVector Min(const C3iVector &a, const C3iVector &b) {
      return C3iVector(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z);
    }

    static C3iVector Max(const C3iVector &a, const C3iVector &b) {
      return C3iVector(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z);
    }

    static long Dot(const C3iVector &a, const C3iVector &b) {
      return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    static C3iVector Cross(const C3iVector &a, const C3iVector &b) {
      return C3iVector(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
    }

    C3iVector &operator+=(long a) { x += a; y += a; z += a; return *this; }
    C3iVector &operator+=(const C3iVector &a) { x += a.x; y += a.y; z += a.z; return *this; }
    C3iVector &operator-=(long a) { x -= a; y -= a; z -= a; return *this; }
    C3iVector &operator-=(const C3iVector &a) { x -= a.x; y -= a.y; z -= a.z; return *this; }
    C3iVector &operator*=(long a) { x *= a; y *= a; z *= a; return *this; }
    C3iVector &operator*=(const C3iVector &a) { x *= a.x; y *= a.y; z *= a.z; return *this; }
    C3iVector &operator/=(long a) { x /= a; y /= a; z /= a; return *this; }
    C3iVector &operator/=(const C3iVector &a) { x /= a.x; y /= a.y; z /= a.z; return *this; }
    C3iVector &operator>>=(long a) { x >>= a; y >>= a; z >>= a; return *this; }
    C3iVector &operator>>=(const C3iVector &a) { x >>= a.x; y >>= a.y; z >>= a.z; return *this; }
    C3iVector &operator<<=(long a) { x <<= a; y <<= a; z <<= a; return *this; }
    C3iVector &operator<<=(const C3iVector &a) { x <<= a.x; y <<= a.y; z <<= a.z; return *this; }
    C3iVector operator-() const { return C3iVector(-x, -y, -z); }
    long &operator[](unsigned int index) { ASSERT(index < 3); return (&x)[index]; }
    const long &operator[](unsigned int index) const { ASSERT(index < 3); return (&x)[index]; }

    long SquaredMag() const { return x * x + y * y + z * z; }
    long Mag() const { return static_cast<long>(sqrt(static_cast<double>(SquaredMag()))); }
    long SumC() const { return x + y + z; }
    bool IsUnit() const { return SquaredMag() == 1; }
    void Normalize() { *this /= Mag(); }
    void Scale(const long magnitude) { Normalize(); *this *= magnitude; }
    void Minimize(const C3iVector &a) { if (a.x < x) x = a.x; if (a.y < y) y = a.y; if (a.z < z) z = a.z; }
    void Maximize(const C3iVector &a) { if (a.x > x) x = a.x; if (a.y > y) y = a.y; if (a.z > z) z = a.z; }

    long x;
    long y;
    long z;
  };

}  // namespace NTempest
