#pragma once

#include <math.h>
#include <storm.h>

namespace NTempest {

  class C2Vector;

  class C2iVector {
   public:
    LONG x;
    LONG y;

    enum {
      eComponents = 2
    };

    C2iVector(LONG value = 0) : x(value), y(value) {
    }

    C2iVector(LONG xValue, LONG yValue) : x(xValue), y(yValue) {
    }

    C2iVector(const C2Vector &vector);

    C2iVector(const tagPOINT &point) : x(point.x), y(point.y) {
    }

    ~C2iVector() {
    }

    void Get(LONG &xValue, LONG &yValue) const {
      xValue = x;
      yValue = y;
    }

    void Set(LONG xValue, LONG yValue) {
      x = xValue;
      y = yValue;
    }

    operator tagPOINT() const {
      tagPOINT point = {x, y};
      return point;
    }

    C2iVector &operator+=(const C2iVector &a) {
      x += a.x;
      y += a.y;
      return *this;
    }

    C2iVector &operator+=(LONG a) {
      x += a;
      y += a;
      return *this;
    }
    C2iVector &operator-=(const C2iVector &a) {
      x -= a.x;
      y -= a.y;
      return *this;
    }

    C2iVector &operator-=(LONG a) {
      x -= a;
      y -= a;
      return *this;
    }
    C2iVector &operator*=(const C2iVector &a) {
      x *= a.x;
      y *= a.y;
      return *this;
    }

    C2iVector &operator*=(LONG a) {
      x *= a;
      y *= a;
      return *this;
    }
    C2iVector &operator/=(const C2iVector &a) {
      x /= a.x;
      y /= a.y;
      return *this;
    }

    C2iVector &operator/=(LONG a) {
      x /= a;
      y /= a;
      return *this;
    }
    C2iVector &operator>>=(const C2iVector &a) {
      x >>= a.x;
      y >>= a.y;
      return *this;
    }

    C2iVector &operator>>=(LONG a) {
      x >>= a;
      y >>= a;
      return *this;
    }
    C2iVector &operator<<=(const C2iVector &a) {
      x <<= a.x;
      y <<= a.y;
      return *this;
    }

    C2iVector &operator<<=(LONG a) {
      x <<= a;
      y <<= a;
      return *this;
    }
    C2iVector operator-() const {
      return C2iVector(-x, -y);
    }

    LONG SquaredMag() const {
      return x * x + y * y;
    }
    LONG Mag() const {
      return (LONG)sqrt((double)SquaredMag());
    }
    LONG SumC() const {
      return x + y;
    }
    bool IsUnit() const {
      return SquaredMag() == 1;
    }
    void Normalize() {
      *this /= Mag();
    }
    void Scale(const LONG magnitude) {
      Normalize();
      *this *= magnitude;
    }
    static C2iVector Min(const C2iVector &a, const C2iVector &b) {
      return C2iVector(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y);
    }

    static C2iVector Max(const C2iVector &a, const C2iVector &b) {
      return C2iVector(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y);
    }

    void Minimize(const C2iVector &a) {
      if (a.x < x)
        x = a.x;
      if (a.y < y)
        y = a.y;
    }
    void Maximize(const C2iVector &a) {
      if (a.x > x)
        x = a.x;
      if (a.y > y)
        y = a.y;
    }

    static LONG Dot(const C2iVector &a, const C2iVector &b) {
      return a.x * b.x + a.y * b.y;
    }
  };

}  // namespace NTempest
