#pragma once

#include "Tempest/c2vector.h"
#include "Tempest/c3vector.h"
#include "Tempest/c4ivector.h"
#include "Tempest/cmath.h"

namespace NTempest {

  class C4Vector {
   public:
    enum {
      eComponents = 4
    };

    C4Vector(float value = 0.0f) : x(value), y(value), z(value), w(value) {
    }

    C4Vector(float xValue, float yValue, float zValue, float wValue) : x(xValue), y(yValue), z(zValue), w(wValue) {
    }

    C4Vector(const C2Vector &vector) : x(vector.x), y(vector.y), z(0.0f), w(0.0f) {
    }

    C4Vector(const C3Vector &vector) : x(vector.x), y(vector.y), z(vector.z), w(0.0f) {
    }

    C4Vector(const C4iVector &vector)
        : x(static_cast<float>(vector.x)), y(static_cast<float>(vector.y)), z(static_cast<float>(vector.z)), w(static_cast<float>(vector.w)) {
    }

    ~C4Vector() {
    }

    C4Vector asC4Vector() const {
      return *this;
    }
    const C4Vector *asFloatPtr() const {
      return this;
    }
    void Get(float &xValue, float &yValue, float &zValue, float &wValue) const {
      xValue = x;
      yValue = y;
      zValue = z;
      wValue = w;
    }
    void Set(float xValue, float yValue, float zValue, float wValue) {
      x = xValue;
      y = yValue;
      z = zValue;
      w = wValue;
    }
    operator C2Vector() const {
      return C2Vector(x, y);
    }
    operator C3Vector() const {
      return C3Vector(x, y, z);
    }

    static C4Vector Min(const C4Vector &a, const C4Vector &b) {
      return C4Vector(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z, a.w < b.w ? a.w : b.w);
    }
    static C4Vector Max(const C4Vector &a, const C4Vector &b) {
      return C4Vector(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z, a.w > b.w ? a.w : b.w);
    }
    static C4Vector Lerp(const C4Vector &a, const C4Vector &b, const C4Vector &t) {
      return C4Vector(a.x + (b.x - a.x) * t.x, a.y + (b.y - a.y) * t.y, a.z + (b.z - a.z) * t.z, a.w + (b.w - a.w) * t.w);
    }
    static float Dot(const C4Vector &a, const C4Vector &b) {
      return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    }

    C4Vector &operator+=(float a) {
      x += a;
      y += a;
      z += a;
      w += a;
      return *this;
    }
    C4Vector &operator+=(const C4Vector &a) {
      x += a.x;
      y += a.y;
      z += a.z;
      w += a.w;
      return *this;
    }
    C4Vector &operator-=(float a) {
      x -= a;
      y -= a;
      z -= a;
      w -= a;
      return *this;
    }
    C4Vector &operator-=(const C4Vector &a) {
      x -= a.x;
      y -= a.y;
      z -= a.z;
      w -= a.w;
      return *this;
    }
    C4Vector &operator*=(float a) {
      x *= a;
      y *= a;
      z *= a;
      w *= a;
      return *this;
    }
    C4Vector &operator*=(const C4Vector &a) {
      x *= a.x;
      y *= a.y;
      z *= a.z;
      w *= a.w;
      return *this;
    }
    C4Vector &operator/=(float a) {
      return *this *= 1.0f / a;
    }
    C4Vector &operator/=(const C4Vector &a) {
      x /= a.x;
      y /= a.y;
      z /= a.z;
      w /= a.w;
      return *this;
    }
    C4Vector operator-() const {
      return C4Vector(-x, -y, -z, -w);
    }
    float &operator[](UINT index) {
      ASSERT(index < 4);
      return (&x)[index];
    }
    const float &operator[](UINT index) const {
      ASSERT(index < 4);
      return (&x)[index];
    }

    float SquaredMag() const {
      return x * x + y * y + z * z + w * w;
    }
    float Mag() const {
      return CMath::sqrt_(SquaredMag());
    }
    float SumC() const {
      return x + y + z + w;
    }
    bool IsUnit() const {
      return CMath::fabs_(SquaredMag() - 1.0f) < 0.0009765625f;
    }
    void Normalize() {
      *this *= 1.0f / Mag();
    }
    void SafeNormalize() {
      float squaredMag = SquaredMag();
      if (squaredMag > 0.0f) {
        *this *= CMath::sqrtinv_(squaredMag);
      }
    }
    void Scale(const float magnitude) {
      SafeNormalize();
      *this *= magnitude;
    }
    void Minimize(const C4Vector &a) {
      if (a.x < x)
        x = a.x;
      if (a.y < y)
        y = a.y;
      if (a.z < z)
        z = a.z;
      if (a.w < w)
        w = a.w;
    }
    void Maximize(const C4Vector &a) {
      if (a.x > x)
        x = a.x;
      if (a.y > y)
        y = a.y;
      if (a.z > z)
        z = a.z;
      if (a.w > w)
        w = a.w;
    }

    float x;
    float y;
    float z;
    float w;
  };

  inline C4Vector operator-(const C4Vector &l, const C4Vector &r) {
    return C4Vector(l.x - r.x, l.y - r.y, l.z - r.z, l.w - r.w);
  }

  inline C4iVector::C4iVector(const C4Vector &vector)
      : x(static_cast<long>(vector.x)), y(static_cast<long>(vector.y)), z(static_cast<long>(vector.z)), w(static_cast<long>(vector.w)) {
  }

}  // namespace NTempest
