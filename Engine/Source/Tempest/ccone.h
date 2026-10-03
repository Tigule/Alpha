#pragma once

#include "Tempest/c3vector.h"

namespace NTempest {

  class CCone {
   public:
    C3Vector position;

   private:
    float angle;

   public:
    C3Vector axis;
    float    height;

   private:
    float cosAngle;

   public:
    CCone(const C3Vector &position, float angle, float height, const C3Vector &axis, bool unit)
        : position(position), angle(angle), axis(axis), height(height) {
      if (!unit) {
        this->axis.Normalize();
      }
      Angle(angle);
    }

    float Angle() const {
      return angle;
    }

    void Angle(float value) {
      angle = value;
      cosAngle = CMath::cos_(value);
    }

    float CosAngle() const {
      return cosAngle;
    }
};

}  // namespace NTempest
