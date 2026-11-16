#pragma once

#include "Tempest/c2ivector.h"
#include "Tempest/c3ivector.h"
#include "Tempest/cmath.h"

namespace NTempest {

  class C4Vector;

  class C4iVector {
   public:
    LONG x;
    LONG y;
    LONG z;
    LONG w;

    enum {
      eComponents = 4
    };

    C4iVector(LONG value = 0) : x(value), y(value), z(value), w(value) {
    }
    C4iVector(LONG x, LONG y, LONG z, LONG w) : x(x), y(y), z(z), w(w) {
    }
    C4iVector(const C2iVector &a) : x(a.x), y(a.y), z(0), w(0) {
    }
    C4iVector(const C3iVector &a) : x(a.x), y(a.y), z(a.z), w(0) {
    }
    C4iVector(const C4Vector &a);

    ~C4iVector() {
    }

    void Get(LONG &tx, LONG &ty, LONG &tz, LONG &tw) const {
      tx = x;
      ty = y;
      tz = z;
      tw = w;
    }
    void Set(LONG tx, LONG ty, LONG tz, LONG tw) {
      x = tx;
      y = ty;
      z = tz;
      w = tw;
    }
    operator C2iVector() const {
      return C2iVector(x, y);
    }
    operator C3iVector() const {
      return C3iVector(x, y, z);
    }
    C4iVector &operator+=(const C4iVector &a) {
      x += a.x;
      y += a.y;
      z += a.z;
      w += a.w;
      return *this;
    }

    C4iVector &operator+=(LONG a) {
      x += a;
      y += a;
      z += a;
      w += a;
      return *this;
    }
    C4iVector &operator-=(const C4iVector &a) {
      x -= a.x;
      y -= a.y;
      z -= a.z;
      w -= a.w;
      return *this;
    }

    C4iVector &operator-=(LONG a) {
      x -= a;
      y -= a;
      z -= a;
      w -= a;
      return *this;
    }
    C4iVector &operator*=(const C4iVector &a) {
      x *= a.x;
      y *= a.y;
      z *= a.z;
      w *= a.w;
      return *this;
    }

    C4iVector &operator*=(LONG a) {
      x *= a;
      y *= a;
      z *= a;
      w *= a;
      return *this;
    }
    C4iVector &operator/=(const C4iVector &a) {
      x /= a.x;
      y /= a.y;
      z /= a.z;
      w /= a.w;
      return *this;
    }

    C4iVector &operator/=(LONG a) {
      x /= a;
      y /= a;
      z /= a;
      w /= a;
      return *this;
    }
    C4iVector &operator>>=(const C4iVector &a) {
      x >>= a.x;
      y >>= a.y;
      z >>= a.z;
      w >>= a.w;
      return *this;
    }

    C4iVector &operator>>=(LONG a) {
      x >>= a;
      y >>= a;
      z >>= a;
      w >>= a;
      return *this;
    }
    C4iVector &operator<<=(const C4iVector &a) {
      x <<= a.x;
      y <<= a.y;
      z <<= a.z;
      w <<= a.w;
      return *this;
    }

    C4iVector &operator<<=(LONG a) {
      x <<= a;
      y <<= a;
      z <<= a;
      w <<= a;
      return *this;
    }
    C4iVector operator-() const {
      return C4iVector(-x, -y, -z, -w);
    }
    LONG SquaredMag() const {
      return x * x + y * y + z * z + w * w;
    }
    LONG Mag() const {
      return (LONG)CMath::sqrt_((float)SquaredMag());
    }
    LONG SumC() const {
      return x + y + z + w;
    }
    bool IsUnit() const {
      return SquaredMag() == 1;
    }
    void Normalize() {
      LONG magnitude = Mag();
      x /= magnitude;
      y /= magnitude;
      z /= magnitude;
      w /= magnitude;
    }
    void Scale(const LONG magnitude) {
      Normalize();
      *this *= magnitude;
    }
    static C4iVector Min(const C4iVector &a, const C4iVector &b) {
      return C4iVector(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z, a.w < b.w ? a.w : b.w);
    }

    static C4iVector Max(const C4iVector &a, const C4iVector &b) {
      return C4iVector(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z, a.w > b.w ? a.w : b.w);
    }

    void Minimize(const C4iVector &a) {
      if (a.x < x)
        x = a.x;
      if (a.y < y)
        y = a.y;
      if (a.z < z)
        z = a.z;
      if (a.w < w)
        w = a.w;
    }
    void Maximize(const C4iVector &a) {
      if (a.x > x)
        x = a.x;
      if (a.y > y)
        y = a.y;
      if (a.z > z)
        z = a.z;
      if (a.w > w)
        w = a.w;
    }

    static LONG Dot(const C4iVector &a, const C4iVector &b) {
      return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    }
  };

}  // namespace NTempest
