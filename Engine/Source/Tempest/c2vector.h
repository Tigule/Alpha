#pragma once

#include "Tempest/c2ivector.h"
#include "Tempest/cmath.h"

namespace NTempest {

  class C2Vector {
   public:
    enum {
      eComponents = 2
    };

    C2Vector(float value = 0.0f) : x(value), y(value) {
    }

    C2Vector(float x, float y) : x(x), y(y) {
    }

    C2Vector(const C2iVector &a) : x(static_cast<float>(a.x)), y(static_cast<float>(a.y)) {
    }

    ~C2Vector() {
    }

    C2Vector asC2Vector() const {
      return *this;
    }
    const C2Vector *asFloatPtr() const {
      return this;
    }
    void Get(float &tx, float &ty) const {
      tx = x;
      ty = y;
    }
    void Set(float tx, float ty) {
      x = tx;
      y = ty;
    }

    static C2Vector FromAxisAngle(float angle, float magnitude) {
      return C2Vector(CMath::cos_(angle) * magnitude, CMath::sin_(angle) * magnitude);
    }
    static float AngleToAxisAngle(float angle) {
      return CMath::fmod_(angle, 6.28318548f);
    }
    static C2Vector Min(const C2Vector &a, const C2Vector &b) {
      return C2Vector(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y);
    }
    static C2Vector Max(const C2Vector &a, const C2Vector &b) {
      return C2Vector(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y);
    }
    static C2Vector Lerp(const C2Vector &a, const C2Vector &b, const C2Vector &t) {
      return C2Vector(a.x + (b.x - a.x) * t.x, a.y + (b.y - a.y) * t.y);
    }
    static float Dot(const C2Vector &a, const C2Vector &b) {
      return a.x * b.x + a.y * b.y;
    }
    static float Cross(const C2Vector &a, const C2Vector &b) {
      return a.x * b.y - a.y * b.x;
    }

    C2Vector &operator+=(float a) {
      x += a;
      y += a;
      return *this;
    }
    C2Vector &operator+=(const C2Vector &a) {
      x += a.x;
      y += a.y;
      return *this;
    }
    C2Vector &operator-=(float a) {
      x -= a;
      y -= a;
      return *this;
    }
    C2Vector &operator-=(const C2Vector &a) {
      x -= a.x;
      y -= a.y;
      return *this;
    }
    C2Vector &operator*=(float a) {
      x *= a;
      y *= a;
      return *this;
    }
    C2Vector &operator*=(const C2Vector &a) {
      x *= a.x;
      y *= a.y;
      return *this;
    }
    C2Vector &operator/=(float a) {
      float inverse = 1.0f / a;
      x *= inverse;
      y *= inverse;
      return *this;
    }
    C2Vector &operator/=(const C2Vector &a) {
      x /= a.x;
      y /= a.y;
      return *this;
    }
    C2Vector operator-() const {
      return C2Vector(-x, -y);
    }
    float &operator[](UINT sub) {
      ASSERT(sub < 2);
      return (&x)[sub];
    }
    const float &operator[](UINT sub) const {
      ASSERT(sub < 2);
      return (&x)[sub];
    }

    float SquaredMag() const {
      return x * x + y * y;
    }

    float Mag() const {
      return CMath::sqrt_(SquaredMag());
    }

    float SumC() const {
      return x + y;
    }
    bool IsUnit() const {
      return CMath::fabs_(SquaredMag() - 1.0f) < 0.0009765625f;
    }
    float AxisAngle() const {
      return CMath::atan2_(y, x);
    }
    float AxisAngle(float angle) const {
      return AngleToAxisAngle(AxisAngle() - angle);
    }

    void Normalize() {
      float ooMag = 1.0f / Mag();

      x *= ooMag;
      y *= ooMag;
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
    void Minimize(const C2Vector &a) {
      if (a.x < x)
        x = a.x;
      if (a.y < y)
        y = a.y;
    }
    void Maximize(const C2Vector &a) {
      if (a.x > x)
        x = a.x;
      if (a.y > y)
        y = a.y;
    }

    float x;
    float y;
  };

  inline C2Vector operator-(const C2Vector &l, const C2Vector &r) {
    return C2Vector(l.x - r.x, l.y - r.y);
  }

  inline C2Vector operator*(float l, const C2Vector &r) {
    return C2Vector(l * r.x, l * r.y);
  }

  inline C2iVector::C2iVector(const C2Vector &vector) : x(static_cast<long>(vector.x)), y(static_cast<long>(vector.y)) {
  }

}  // namespace NTempest
