#pragma once

namespace NTempest {

  class C2iVector {
   public:
    C2iVector(int value = 0) : x(value), y(value) {
    }

    C2iVector(int xValue, int yValue) : x(xValue), y(yValue) {
    }

    ~C2iVector() {
    }

    int x;
    int y;
  };

}  // namespace NTempest
