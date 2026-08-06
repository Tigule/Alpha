#pragma once

#include "Tempest/c3vector.h"
#include "Tempest/cdyntable.h"

namespace NTempest {

  class CAaBox;

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
    BYTE         NotEmpty() const;
    BYTE         Empty() const;
    BYTE         Intersects(const CAaSphere &sphere) const;
    BYTE         Intersects(const C3Vector &point) const;
    BYTE         Encloses(const CAaSphere &sphere) const;
    BYTE         Encloses(const C3Vector &point) const;
    BYTE         Contains(const CAaSphere &sphere) const;
    BYTE         Contains(const C3Vector &point) const;
    float        Diameter() const;
    float        Area() const;
    float        Volume() const;
    C3Vector     Minimum() const;
    C3Vector     Maximum() const;

    static CAaSphere Lerp(const CAaSphere &a, const CAaSphere &b, const CAaSphere &t);
    static CAaSphere Bounding(const CDynTable<CAaSphere> &spheres);
    static CAaSphere Bounding(const CAaSphere *spheres, DWORD count);
    static CAaSphere Bounding(const CDynTable<DWORD> &indices, const CDynTable<C3Vector> &vectors);
    static CAaSphere Bounding(const CDynTable<C3Vector> &vectors);
    static CAaSphere Bounding(const C3Vector *vectors, DWORD count);

   protected:
    static float  square_(float value);
    static float  cube_(float value);
    static CAaBox FindExtrema(const CAaSphere *spheres, DWORD count);

   public:
    C3Vector c;
    float    r;
  };

}  // namespace NTempest
