#pragma once

namespace NTempest {

  class CiRange {
   public:
    LONG l;
    LONG h;

    enum {
      eComponents = 2
    };

    CiRange(LONG value = 0);
    CiRange(LONG low, LONG high);
    ~CiRange() {
    }

    void Get(LONG &low, LONG &high) const {
      low = l;
      high = h;
    }

    void Set(LONG low, LONG high) {
      l = low;
      h = high;
    }

    LONG     Low() const;
    LONG     High() const;
    CiRange &operator+=(const CiRange &value);
    CiRange &operator-=(const CiRange &value);
    CiRange &operator*=(const CiRange &value);
    CiRange &operator/=(const CiRange &value);
    CiRange  operator-() const;
    BYTE     Empty() const;
    BYTE     NotEmpty() const;
    BYTE     Invalid() const;
    BYTE     NotInvalid() const;
    BYTE     Encloses(const CiRange &value) const;
    BYTE     Contains(const CiRange &value) const;
    BYTE     InClosedRange(LONG value) const;
    BYTE     InOpenRange(LONG value) const;
    LONG     Magnitude() const;
    LONG     Center() const;
    void     Center(const CiRange &value);
    void     Stretch(LONG value);
    void     Offset(LONG value);
    void     AlignLow(const CiRange &value);
    void     AlignHigh(const CiRange &value);
    LONG     ClampClosed(LONG value) const;
    LONG     ClampOpen(LONG value) const;

    static CiRange Intersection(const CiRange &a, const CiRange &b);
    static CiRange Union(const CiRange &a, const CiRange &b);

    CiRange Intersect(const CiRange &value);
    CiRange Unite(const CiRange &value);

    static BYTE    InRange(LONG value, LONG low, LONG high);
  };

  inline CiRange::CiRange(LONG value) : l(value), h(value) {
  }

  inline CiRange::CiRange(LONG low, LONG high) : l(low), h(high) {
  }

  inline LONG CiRange::Low() const {
    return l;
  }

  inline LONG CiRange::High() const {
    return h;
  }

  inline LONG CiRange::Magnitude() const {
    return h - l;
  }

}  // namespace NTempest
