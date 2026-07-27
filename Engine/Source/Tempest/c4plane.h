#pragma once

#include "Tempest/c3vector.h"
#include "Tempest/c4vector.h"

namespace NTempest {

  class C4Plane {
   public:
    C4Plane() : n(0.0f, 0.0f, 1.0f), d(0.0f) {
    }

    C4Plane(const C3Vector &n, float d) : n(n), d(d) {
    }

    C4Plane(const C3Vector &normal, const C3Vector &point) : n(normal), d(-C3Vector::Dot(normal, point)) {
    }

    C4Plane(const C3Vector &a, const C3Vector &b, const C3Vector &c) {
      From3Pos(a, b, c);
    }

    C4Plane(float x, float y, float z, float distance) : n(x, y, z), d(distance) {
    }

    void From3Pos(const C3Vector &a, const C3Vector &b, const C3Vector &c) {
      n = C3Vector::Cross(b - a, c - a);
      n.Normalize();
      d = -C3Vector::Dot(n, a);
    }

    const float *Access() const {
      return &n.x;
    }

    float *Access() {
      return &n.x;
    }

    void Get(float &x, float &y, float &z, float &distance) const {
      x = n.x;
      y = n.y;
      z = n.z;
      distance = d;
    }

    void Get(C3Vector &normal, float &distance) const {
      normal = n;
      distance = d;
    }

    void Set(const C3Vector &pn, const C3Vector &a) {
      n = pn;
      d = -C3Vector::Dot(pn, a);
    }

    void Set(const C3Vector &a, const C3Vector &b, const C3Vector &c) {
      From3Pos(a, b, c);
    }

    void Set(float x, float y, float z, float distance) {
      n.Set(x, y, z);
      d = distance;
    }

    void Set(const C3Vector &normal, float distance) {
      n = normal;
      d = distance;
    }

    void Translate(const C3Vector &translation) {
      d -= C3Vector::Dot(n, translation);
    }

    operator C4Vector() const {
      return C4Vector(n.x, n.y, n.z, d);
    }

    C4Plane operator-() const {
      return C4Plane(-n.x, -n.y, -n.z, -d);
    }

    float DistSigned(const C3Vector &point) const {
      return C3Vector::Dot(n, point) + d;
    }

    float DistSquared(const C3Vector &point) const {
      float distance = DistSigned(point);
      return distance < 0.0f ? -distance * distance : distance * distance;
    }

    float Dist(const C3Vector &point) const {
      return CMath::fabs_(DistSigned(point));
    }

    float SolveForX(float y, float z) const {
      return -(n.y * y + n.z * z + d) / n.x;
    }

    float SolveForY(float x, float z) const {
      return -(n.x * x + n.z * z + d) / n.y;
    }

    float SolveForZ(float x, float y) const {
      return -(n.x * x + n.y * y + d) / n.z;
    }

    C3Vector n;
    float    d;
  };

}  // namespace NTempest
