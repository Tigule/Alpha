#pragma once

namespace NTempest {

  class CRect {
   public:
    union {
      float t;
      float miny;
    };
    union {
      float l;
      float minx;
    };
    union {
      float b;
      float maxy;
    };
    union {
      float r;
      float maxx;
    };

    CRect(float value = 0.0f) {
      t = value;
      l = value;
      b = value;
      r = value;
    }

    CRect(float top, float left, float bottom, float right) {
      t = top;
      l = left;
      b = bottom;
      r = right;
    }

    void Set(float top, float left, float bottom, float right) {
      t = top;
      l = left;
      b = bottom;
      r = right;
    }

    bool NotEmpty() const {
      return t < b && l < r;
    }

    float Width() const {
      return r - l;
    }

    float Height() const {
      return b - t;
    }

    static CRect Intersection(const CRect &left, const CRect &right) {
      CRect result;

      result.t = left.t > right.t ? left.t : right.t;
      result.l = left.l > right.l ? left.l : right.l;
      result.b = left.b < right.b ? left.b : right.b;
      result.r = left.r < right.r ? left.r : right.r;
      return result;
    }

    CRect Intersect(const CRect &right) {
      return Intersection(*this, right);
    }
  };

}  // namespace NTempest
