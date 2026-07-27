#pragma once

namespace NTempest {

  class CRndSeed;

  class CRange {
   protected:
    float _rnd(float value, CRndSeed &seed) const;

   public:
    enum {
      eComponents = 2
    };

    CRange(float value = 0.0f) : l(value), h(value) {
    }

    CRange(float low, float high) : l(low), h(high) {
    }

    ~CRange() {
    }

    void Get(float &low, float &high) const {
      low = l;
      high = h;
    }

    void Set(float low, float high) {
      l = low;
      h = high;
    }

    float  Low() const;
    float  High() const;
    CRange &operator+=(const CRange &value);
    CRange &operator-=(const CRange &value);
    CRange &operator*=(const CRange &value);
    CRange &operator/=(const CRange &value);
    CRange operator-() const;
    unsigned char Empty() const;
    unsigned char NotEmpty() const;
    unsigned char Invalid() const;
    unsigned char NotInvalid() const;
    unsigned char Encloses(const CRange &value) const;
    unsigned char Contains(const CRange &value) const;
    unsigned char InClosedRange(float value) const;
    unsigned char InOpenRange(float value) const;
    float Magnitude() const;
    void  Center(const CRange &value);
    float Center() const;
    void  Stretch(float value);
    void  Offset(float value);
    void  AlignLow(const CRange &value);
    void  AlignHigh(const CRange &value);
    float ClampClosed(float value) const;
    float ClampOpen(float value) const;
    CRange Intersect(const CRange &value);
    CRange Unite(const CRange &value);
    float Value(CRndSeed &seed) const;

    static CRange Lerp(const CRange &a, const CRange &b, const CRange &t);
    static CRange Intersection(const CRange &a, const CRange &b);
    static CRange Union(const CRange &a, const CRange &b);
    static unsigned char InRange(float value, float low, float high);

    float l;
    float h;
  };

}  // namespace NTempest
