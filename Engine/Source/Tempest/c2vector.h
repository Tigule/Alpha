#pragma once

#include "Tempest/cmath.h"

namespace NTempest {

  class C2Vector {
   public:
    C2Vector(float value = 0.0f) : x(value), y(value) {
    }

    C2Vector(float x, float y) : x(x), y(y) {
    }

    ~C2Vector() {
    }

    float SquaredMag() const {
      return x * x + y * y;
    }

    float Mag() const {
      return CMath::sqrt_(SquaredMag());
    }

    void Normalize() {
      float ooMag = 1.0f / Mag();

      x *= ooMag;
      y *= ooMag;
    }

    float x;
    float y;
  };

}  // namespace NTempest
