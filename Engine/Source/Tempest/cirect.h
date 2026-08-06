#pragma once

#include "Tempest/c2ivector.h"

#include <windows.h>

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

    CiRect(const C2iVector &value);
    CiRect(const C2iVector &topLeft, const C2iVector &bottomRight);
    CiRect(const tagRECT &value);

    ~CiRect() {
    }

    void    Get(long &top, long &left, long &bottom, long &right) const;
    void    Set(long top, long left, long bottom, long right);
            operator tagRECT() const;
    CiRect &operator+=(const CiRect &value);
    CiRect &operator-=(const CiRect &value);
    CiRect &operator*=(const CiRect &value);
    CiRect &operator/=(const CiRect &value);
    CiRect  operator-() const;
    void    Stretch(const C2iVector &value);
    void    Stretch(long horizontal, long vertical);
    void    Offset(const C2iVector &value);
    void    Offset(long horizontal, long vertical);
    bool    NotEmpty() const;
    bool    Empty() const;
    bool    Invalid() const;
    bool    NotInvalid() const;
    bool    Encloses(const CiRect &value) const;
    bool    Encloses(const C2iVector &value) const;
    bool    Contains(const CiRect &value) const;
    bool    Contains(const C2iVector &value) const;
    bool    InOpenR(const CiRect &value) const;
    bool    InOpenR(const C2iVector &value) const;

    long Width() const {
      return r - l;
    }

    long Height() const {
      return b - t;
    }

    void          SetWidth(long value);
    void          SetHeight(long value);
    C2iVector     TopLeft() const;
    C2iVector     TopRight() const;
    C2iVector     BottomLeft() const;
    C2iVector     BottomRight() const;
    void          Center(const CiRect &value);
    C2iVector     Center() const;
    C2iVector     Diagonal() const;
    void          CenterV(const CiRect &value);
    void          CenterH(const CiRect &value);
    void          AlignTop(const CiRect &value);
    void          AlignLeft(const CiRect &value);
    void          AlignBottom(const CiRect &value);
    void          AlignRight(const CiRect &value);
    static CiRect Intersection(const CiRect &a, const CiRect &b, const CiRect &clip);
    static CiRect Intersection(const CiRect &left, const CiRect &right);
    static CiRect Union(const CiRect &left, const CiRect &right);
    static CiRect ClippedLocal(const CiRect &value, const CiRect &clip);
    static DWORD  Difference(const CiRect &left, const CiRect &right, CiRect *result);
    CiRect        Intersect(const CiRect &right);
    CiRect        Unite(const CiRect &right);
  };

}  // namespace NTempest
