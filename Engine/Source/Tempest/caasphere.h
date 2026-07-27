#pragma once

#include "Tempest/c3vector.h"

namespace NTempest {

  class CAaBox;

  template <class T>
  class CDynTable;

  class CAaSphere {
   public:
    CAaSphere(const C3Vector &center = C3Vector(0.0f), float radius = 0.0f) : c(center), r(radius) {
    }

    const float *Access() const;
    float       *Access();
    void         Get(C3Vector &center, float &radius) const;
    void         Set(const C3Vector &center, float radius);
    float        SquaredD(const CAaSphere &sphere) const;
    float        SquaredD(const C3Vector &point) const;
    unsigned char NotEmpty() const;
    unsigned char Empty() const;
    unsigned char Intersects(const CAaSphere &sphere) const;
    unsigned char Intersects(const C3Vector &point) const;
    unsigned char Encloses(const CAaSphere &sphere) const;
    unsigned char Encloses(const C3Vector &point) const;
    unsigned char Contains(const CAaSphere &sphere) const;
    unsigned char Contains(const C3Vector &point) const;
    float          Diameter() const;
    float          Area() const;
    float          Volume() const;
    C3Vector       Minimum() const;
    C3Vector       Maximum() const;

    static CAaSphere Lerp(const CAaSphere &a, const CAaSphere &b, const CAaSphere &t);
    static CAaSphere Bounding(const CDynTable<CAaSphere> &spheres);
    static CAaSphere Bounding(const CAaSphere *spheres, unsigned long count);
    static CAaSphere Bounding(const CDynTable<unsigned long> &indices, const CDynTable<C3Vector> &vectors);
    static CAaSphere Bounding(const CDynTable<C3Vector> &vectors);
    static CAaSphere Bounding(const C3Vector *vectors, unsigned long count);

   protected:
    static float  square_(float value);
    static float  cube_(float value);
    static CAaBox FindExtrema(const CAaSphere *spheres, unsigned long count);

   public:
    C3Vector c;
    float    r;
  };

}  // namespace NTempest
