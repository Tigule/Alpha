#ifndef ENGINE_SOURCE_TEMPEST_CFACET_H
#define ENGINE_SOURCE_TEMPEST_CFACET_H

#include "Tempest/c4plane.h"

namespace NTempest {

  struct CFacet {
    enum {
      eComponents = 13
    };

    CFacet(float a = 0.0f) {
      Set(a);
    }

    CFacet(const C3Vector &a, const C3Vector &b, const C3Vector &c) {
      Set(a, b, c);
    }

    ~CFacet() {
    }

    void Get(C3Vector *vertices) const;
    void Get(C4Plane &plane) const;
    void Get(C4Plane &plane, C3Vector *vertices) const;
    void Set(float a);
    void Set(const C3Vector *vertices);
    void Set(const C3Vector *vertices, const C4Plane &plane);
    void Set(const C3Vector &a, const C3Vector &b, const C3Vector &c);

    C4Plane  plane;
    C3Vector vertices[3];
  };

}  // namespace NTempest

#endif
