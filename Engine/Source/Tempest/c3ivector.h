#pragma once

namespace NTempest {

  class C3iVector {
   public:
    C3iVector(long value = 0) : x(value), y(value), z(value) {
    }
    C3iVector(long x, long y, long z) : x(x), y(y), z(z) {
    }
    ~C3iVector() {
    }

    long x;
    long y;
    long z;
  };

}  // namespace NTempest
