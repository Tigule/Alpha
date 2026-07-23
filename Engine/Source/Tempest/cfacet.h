#ifndef ENGINE_SOURCE_TEMPEST_CFACET_H
#define ENGINE_SOURCE_TEMPEST_CFACET_H

#include "Tempest/c4plane.h"

namespace NTempest {

  struct CFacet {
    CFacet(float a = 0.0f) {
      Set(a);
    }

    CFacet(const C3Vector &a, const C3Vector &b, const C3Vector &c) {
      Set(a, b, c);
    }

    void Set(float a);
    void Set(const C3Vector &a, const C3Vector &b, const C3Vector &c);

    C4Plane  plane;
    C3Vector vertices[3];
  };

}  // namespace NTempest

#endif
