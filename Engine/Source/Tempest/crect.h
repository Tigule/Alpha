#pragma once

#include "Tempest/c2vector.h"

#include <windows.h>

namespace NTempest {

  class CiRect;

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

    enum {
      eComponents = 4
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

    void         Get(float &top, float &left, float &bottom, float &right) const;

    void Set(float top, float left, float bottom, float right) {
      t = top;
      l = left;
      b = bottom;
      r = right;
    }

    operator tagRECT() const;
    CRect        asCRect() const;
    const CRect *asFloatPtr() const;
    CRect       &operator+=(const CRect &value);
    CRect       &operator-=(const CRect &value);
    CRect       &operator*=(const CRect &value);
    CRect       &operator/=(const CRect &value);
    CRect        operator-() const;
    void         Stretch(float horizontal, float vertical);
    void         Stretch(const C2Vector &value);
    void Offset(float horizontal, float vertical) {
      t += vertical;
      l += horizontal;
      b += vertical;
      r += horizontal;
    }

    void         Offset(const C2Vector &value);

    bool NotEmpty() const {
      return t < b && l < r;
    }

    bool Empty() const;
    bool Invalid() const;
    bool NotInvalid() const;
    bool Encloses(const C2Vector &value) const;
    bool Encloses(const CRect &value) const;
    bool Contains(const C2Vector &value) const {
      return value.x >= l && value.x <= r && value.y >= t && value.y <= b;
    }
    bool Contains(const CRect &value) const;
    bool InOpenR(const C2Vector &value) const;
    bool InOpenR(const CRect &value) const;

    float Width() const {
      return r - l;
    }

    float Height() const {
      return b - t;
    }

    void     SetWidth(float value);
    void     SetHeight(float value);
    C2Vector TopLeft() const;
    C2Vector TopRight() const;
    C2Vector BottomLeft() const;
    C2Vector BottomRight() const;
    C2Vector Center() const;
    void     Center(const CRect &value);
    C2Vector Diagonal() const;
    void     CenterV(const CRect &value);
    void     CenterH(const CRect &value);
    void     AlignTop(const CRect &value);
    void     AlignLeft(const CRect &value);
    void     AlignBottom(const CRect &value);
    void     AlignRight(const CRect &value);
    static CRect Lerp(const CRect &a, const CRect &b, const CRect &t);

    static CRect Intersection(const CRect &l, const CRect &r) {
      return CRect(max(l.t, r.t), max(l.l, r.l), min(l.b, r.b), min(l.r, r.r));
    }

    static CRect Intersection(const CRect &a, const CRect &b, const CRect &clip);
    static CRect Union(const CRect &left, const CRect &right);

    CRect Intersect(const CRect &r) {
      *this = Intersection(*this, r);
      return *this;
    }

    CRect Unite(const CRect &right);
    static CRect ClippedLocal(const CRect &value, const CRect &clip);
    static DWORD Difference(const CRect &left, const CRect &right, CRect *result);
  };

}  // namespace NTempest
