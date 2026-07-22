#pragma once

namespace NTempest {

  class CiRect {
   public:
    union {
      long t;
      long miny;
    };
    union {
      long l;
      long minx;
    };
    union {
      long b;
      long maxy;
    };
    union {
      long r;
      long maxx;
    };

    enum {
      eComponents = 4
    };

    CiRect(long value = 0) {
      t = value;
      l = value;
      b = value;
      r = value;
    }

    CiRect(long top, long left, long bottom, long right) {
      t = top;
      l = left;
      b = bottom;
      r = right;
    }

    ~CiRect() {
    }

    long Width() const {
      return r - l;
    }

    long Height() const {
      return b - t;
    }
  };

}  // namespace NTempest
