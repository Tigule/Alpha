#pragma once

#include "Tempest/c2vector.h"

#include <windows.h>

namespace NTempest {

  class CiRect;

  class CRect {
   public:
    enum {
      eComponents = 4
    };

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

    CRect(const C2Vector &value);
    CRect(const C2Vector &topLeft, const C2Vector &bottomRight);
    CRect(const CiRect &value);
    CRect(const tagRECT &value);

    ~CRect() {
    }

    void Set(float top, float left, float bottom, float right) {
      t = top;
      l = left;
      b = bottom;
      r = right;
    }

    void Get(float &top, float &left, float &bottom, float &right) const;
    operator tagRECT() const;
    CRect asCRect() const;
    const CRect *asFloatPtr() const;
    CRect &operator+=(const CRect &value);
    CRect &operator-=(const CRect &value);
    CRect &operator*=(const CRect &value);
    CRect &operator/=(const CRect &value);
    CRect operator-() const;
    void Stretch(const C2Vector &value);
    void Stretch(float horizontal, float vertical);
    void Offset(const C2Vector &value);
    void Offset(float horizontal, float vertical);

    bool NotEmpty() const {
      return t < b && l < r;
    }

    bool Empty() const;
    bool Invalid() const;
    bool NotInvalid() const;
    bool Encloses(const CRect &value) const;
    bool Encloses(const C2Vector &value) const;
    bool Contains(const CRect &value) const;
    bool Contains(const C2Vector &value) const;
    bool InOpenR(const CRect &value) const;
    bool InOpenR(const C2Vector &value) const;

    float Width() const {
      return r - l;
    }

    float Height() const {
      return b - t;
    }

    void SetWidth(float value);
    void SetHeight(float value);
    C2Vector TopLeft() const;
    C2Vector TopRight() const;
    C2Vector BottomLeft() const;
    C2Vector BottomRight() const;
    void Center(const CRect &value);
    C2Vector Center() const;
    C2Vector Diagonal() const;
    void CenterV(const CRect &value);
    void CenterH(const CRect &value);
    void AlignTop(const CRect &value);
    void AlignLeft(const CRect &value);
    void AlignBottom(const CRect &value);
    void AlignRight(const CRect &value);

    static CRect Lerp(const CRect &a, const CRect &b, const CRect &t);
    static CRect Intersection(const CRect &a, const CRect &b, const CRect &clip);
    static CRect Intersection(const CRect &left, const CRect &right) {
      CRect result;

      result.t = left.t > right.t ? left.t : right.t;
      result.l = left.l > right.l ? left.l : right.l;
      result.b = left.b < right.b ? left.b : right.b;
      result.r = left.r < right.r ? left.r : right.r;
      return result;
    }

    static CRect Union(const CRect &left, const CRect &right);
    static CRect ClippedLocal(const CRect &value, const CRect &clip);
    static unsigned long Difference(const CRect &left, const CRect &right, CRect *result);

    CRect Intersect(const CRect &right) {
      return Intersection(*this, right);
    }

    CRect Unite(const CRect &right);
  };

}  // namespace NTempest
