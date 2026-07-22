#ifndef ENGINE_SOURCE_TEMPEST_CFACET_H
#define ENGINE_SOURCE_TEMPEST_CFACET_H

#include "Tempest/c4plane.h"

namespace NTempest {

  struct CFacet {
    CFacet(float a = 0.0f) {
      Set(a);
    }

    CFacet(C3Vector &a, C3Vector &b, C3Vector &c) {
      Set(a, b, c);
    }

    void Set(float a);
    void Set(C3Vector &a, C3Vector &b, C3Vector &c);

    C4Plane  plane;
    C3Vector vertices[3];
  };

}  // namespace NTempest

#endif
