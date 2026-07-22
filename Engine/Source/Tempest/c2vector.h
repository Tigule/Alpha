#pragma once

namespace NTempest {

  class C2Vector {
   public:
    C2Vector(float value = 0.0f) : x(value), y(value) {
    }

    C2Vector(float x, float y) : x(x), y(y) {
    }

    ~C2Vector() {
    }

    float x;
    float y;
  };

}  // namespace NTempest
