#pragma once

#include <math.h>
#include <windows.h>

namespace NTempest {

  class C2Vector;

  class C2iVector {
   public:
    enum {
      eComponents = 2
    };

    C2iVector(long value = 0) : x(value), y(value) {
    }

    C2iVector(long xValue, long yValue) : x(xValue), y(yValue) {
    }

    C2iVector(const tagPOINT &point) : x(point.x), y(point.y) {
    }

    C2iVector(const C2Vector &vector);

    ~C2iVector() {
    }

    void Get(long &xValue, long &yValue) const {
      xValue = x;
      yValue = y;
    }

    void Set(long xValue, long yValue) {
      x = xValue;
      y = yValue;
    }

    operator tagPOINT() const {
      tagPOINT point = {x, y};
      return point;
    }

    static C2iVector Min(const C2iVector &a, const C2iVector &b) {
      return C2iVector(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y);
    }

    static C2iVector Max(const C2iVector &a, const C2iVector &b) {
      return C2iVector(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y);
    }

    static long Dot(const C2iVector &a, const C2iVector &b) {
      return a.x * b.x + a.y * b.y;
    }

    C2iVector &operator+=(long a) {
      x += a;
      y += a;
      return *this;
    }
    C2iVector &operator+=(const C2iVector &a) {
      x += a.x;
      y += a.y;
      return *this;
    }
    C2iVector &operator-=(long a) {
      x -= a;
      y -= a;
      return *this;
    }
    C2iVector &operator-=(const C2iVector &a) {
      x -= a.x;
      y -= a.y;
      return *this;
    }
    C2iVector &operator*=(long a) {
      x *= a;
      y *= a;
      return *this;
    }
    C2iVector &operator*=(const C2iVector &a) {
      x *= a.x;
      y *= a.y;
      return *this;
    }
    C2iVector &operator/=(long a) {
      x /= a;
      y /= a;
      return *this;
    }
    C2iVector &operator/=(const C2iVector &a) {
      x /= a.x;
      y /= a.y;
      return *this;
    }
    C2iVector &operator>>=(long a) {
      x >>= a;
      y >>= a;
      return *this;
    }
    C2iVector &operator>>=(const C2iVector &a) {
      x >>= a.x;
      y >>= a.y;
      return *this;
    }
    C2iVector &operator<<=(long a) {
      x <<= a;
      y <<= a;
      return *this;
    }
    C2iVector &operator<<=(const C2iVector &a) {
      x <<= a.x;
      y <<= a.y;
      return *this;
    }
    C2iVector operator-() const {
      return C2iVector(-x, -y);
    }

    long SquaredMag() const {
      return x * x + y * y;
    }
    long Mag() const {
      return static_cast<long>(sqrt(static_cast<double>(SquaredMag())));
    }
    long SumC() const {
      return x + y;
    }
    bool IsUnit() const {
      return SquaredMag() == 1;
    }
    void Normalize() {
      *this /= Mag();
    }
    void Scale(const long magnitude) {
      Normalize();
      *this *= magnitude;
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

    long x;
    long y;
  };

}  // namespace NTempest
