#pragma once

namespace NTempest {

  class C4Vector {
   public:
    C4Vector(float value = 0.0f) : x(value), y(value), z(value), w(value) {
    }

    C4Vector(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {
    }

    float x;
    float y;
    float z;
    float w;
  };

}  // namespace NTempest
