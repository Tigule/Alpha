#pragma once

#include "Tempest/c2vector.h"
#include "Tempest/c3ivector.h"
#include "Tempest/cmath.h"

class CDataStore;

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

    C3Vector(const C2Vector &a) : x(a.x), y(a.y), z(0.0f) {
    }

    C3Vector(const C3iVector &a) : x(static_cast<float>(a.x)), y(static_cast<float>(a.y)), z(static_cast<float>(a.z)) {
    }

    ~C3Vector() {
    }

    C3Vector asC3Vector() const { return *this; }
    const C3Vector *asFloatPtr() const { return this; }
    void Get(float &tx, float &ty, float &tz) const { tx = x; ty = y; tz = z; }
    operator C2Vector() const { return C2Vector(x, y); }

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

    static C3Vector Min(const C3Vector &a, const C3Vector &b) {
      return C3Vector(b.x <= a.x ? b.x : a.x, b.y <= a.y ? b.y : a.y, b.z <= a.z ? b.z : a.z);
    }

    static C3Vector Max(const C3Vector &a, const C3Vector &b) {
      return C3Vector(b.x >= a.x ? b.x : a.x, b.y >= a.y ? b.y : a.y, b.z >= a.z ? b.z : a.z);
    }

    static C3Vector Lerp(const C3Vector &a, const C3Vector &b, const C3Vector &t) {
      return C3Vector(
          a.x + (b.x - a.x) * t.x,
          a.y + (b.y - a.y) * t.y,
          a.z + (b.z - a.z) * t.z
      );
    }

    void Maximize(const C3Vector &a) {
      x = a.x > x ? a.x : x;
      y = a.y > y ? a.y : y;
      z = a.z > z ? a.z : z;
    }

    static float Dot(const C3Vector &l, const C3Vector &r) {
      return l.x * r.x + l.y * r.y + l.z * r.z;
    }

    static C3Vector Cross(const C3Vector &l, const C3Vector &r) {
      return C3Vector(l.y * r.z - l.z * r.y, l.z * r.x - l.x * r.z, l.x * r.y - l.y * r.x);
    }

    static C3Vector Cross(const C3Vector &l, const C2Vector &r) {
      return C3Vector(-l.z * r.y, l.z * r.x, l.x * r.y - l.y * r.x);
    }

    static C3Vector Cross(const C2Vector &l, const C3Vector &r) {
      return C3Vector(l.y * r.z, -l.x * r.z, l.x * r.y - l.y * r.x);
    }

    static C3Vector ProjectionOnPlane(const C3Vector &vector, const C3Vector &normal) {
      float distance = Dot(vector, normal);
      return C3Vector(
          vector.x - normal.x * distance,
          vector.y - normal.y * distance,
          vector.z - normal.z * distance
      );
    }

    static C3Vector NearestOnPlane(const C3Vector &point, const C3Vector &planePoint, const C3Vector &normal) {
      C3Vector offset(point.x - planePoint.x, point.y - planePoint.y, point.z - planePoint.z);
      float distance = Dot(offset, normal);
      return C3Vector(
          point.x - normal.x * distance,
          point.y - normal.y * distance,
          point.z - normal.z * distance
      );
    }

    C3Vector &operator+=(float a) {
      x += a; y += a; z += a; return *this;
    }

    C3Vector &operator+=(const C3Vector &a) {
      x += a.x;
      y += a.y;
      z += a.z;
      return *this;
    }

    C3Vector &operator-=(float a) {
      x -= a; y -= a; z -= a; return *this;
    }

    C3Vector &operator-=(const C3Vector &a) {
      x -= a.x; y -= a.y; z -= a.z; return *this;
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

    C3Vector &operator/=(float a) {
      return *this *= 1.0f / a;
    }

    C3Vector &operator/=(const C3Vector &a) {
      x /= a.x; y /= a.y; z /= a.z; return *this;
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

    float SumC() const { return x + y + z; }
    bool IsUnit() const { return CMath::fabs_(SquaredMag() - 1.0f) < 0.0009765625f; }
    void SafeNormalize() {
      float squaredMag = SquaredMag();
      if (squaredMag > 0.0f) {
        *this *= CMath::sqrtinv_(squaredMag);
      }
    }
    void Scale(float magnitude) {
      SafeNormalize();
      *this *= magnitude;
    }
    void Minimize(const C3Vector &a) {
      x = a.x < x ? a.x : x;
      y = a.y < y ? a.y : y;
      z = a.z < z ? a.z : z;
    }

    float x;
    float y;
    float z;
  };

  inline int IsUnitVector(const C3Vector &vector) {
    return CMath::fabs_(vector.SquaredMag() - 1.0f) < 0.0009765625f;
  }

  inline bool operator==(const C3Vector &l, const C3Vector &r) {
    return l.x == r.x && l.y == r.y && l.z == r.z;
  }

  inline bool operator!=(const C3Vector &l, const C3Vector &r) {
    return l.x != r.x || l.y != r.y || l.z != r.z;
  }

  inline C3Vector operator+(const C3Vector &l, const C3Vector &r) {
    return C3Vector(l.x + r.x, l.y + r.y, l.z + r.z);
  }

  inline C3Vector operator-(const C3Vector &l, const C3Vector &r) {
    return C3Vector(l.x - r.x, l.y - r.y, l.z - r.z);
  }

  inline C3Vector operator*(const C3Vector &l, float r) {
    return C3Vector(l.x * r, l.y * r, l.z * r);
  }

  inline C3Vector operator*(float l, const C3Vector &r) {
    return C3Vector(l * r.x, l * r.y, l * r.z);
  }

  inline C3Vector operator/(const C3Vector &l, float r) {
    float inverse = 1.0f / r;
    return C3Vector(l.x * inverse, l.y * inverse, l.z * inverse);
  }

  inline bool operator<=(const C3Vector &l, const C3Vector &r) {
    return l.x <= r.x && l.y <= r.y && l.z <= r.z;
  }

  inline bool operator>=(const C3Vector &l, const C3Vector &r) {
    return l.x >= r.x && l.y >= r.y && l.z >= r.z;
  }

  CDataStore &operator<<(CDataStore &store, const C3Vector &vector);

  inline C3iVector::C3iVector(const C3Vector &vector)
      : x(static_cast<long>(vector.x)), y(static_cast<long>(vector.y)),
        z(static_cast<long>(vector.z)) {
  }

}  // namespace NTempest
