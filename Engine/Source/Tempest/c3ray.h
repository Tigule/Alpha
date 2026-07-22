#ifndef ENGINE_SOURCE_TEMPEST_C3RAY_H
#define ENGINE_SOURCE_TEMPEST_C3RAY_H

#include "Tempest/c3vector.h"

namespace NTempest {

  class C3Ray {
   public:
    C3Ray() {
    }

    C3Ray(const C3Vector &origin, const C3Vector &dir) : origin(origin), dir(dir) {
    }

    C3Vector origin;
    C3Vector dir;
  };

}  // namespace NTempest

#endif
