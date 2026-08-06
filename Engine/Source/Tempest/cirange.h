#pragma once

namespace NTempest {

  class CiRange {
   public:
    enum {
      eComponents = 2
    };

    CiRange(long value = 0);
    CiRange(long low, long high);
    ~CiRange() {
    }

    void Get(long &low, long &high) const {
      low = l;
      high = h;
    }

    void Set(long low, long high) {
      l = low;
      h = high;
    }

    long     Low() const;
    long     High() const;
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
    BYTE     InClosedRange(long value) const;
    BYTE     InOpenRange(long value) const;
    long     Magnitude() const;
    void     Center(const CiRange &value);
    long     Center() const;
    void     Stretch(long value);
    void     Offset(long value);
    void     AlignLow(const CiRange &value);
    void     AlignHigh(const CiRange &value);
    long     ClampClosed(long value) const;
    long     ClampOpen(long value) const;
    CiRange  Intersect(const CiRange &value);
    CiRange  Unite(const CiRange &value);

    static CiRange Intersection(const CiRange &a, const CiRange &b);
    static CiRange Union(const CiRange &a, const CiRange &b);
    static BYTE    InRange(long value, long low, long high);

    long l;
    long h;
  };

  inline CiRange::CiRange(long value) : l(value), h(value) {
  }

  inline CiRange::CiRange(long low, long high) : l(low), h(high) {
  }

}  // namespace NTempest
