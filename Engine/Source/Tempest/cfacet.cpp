#include <Base/Base.h>

#include "Tempest/cfacet.h"

#include <math.h>

namespace NTempest {

  void CFacet::Set(float a) {
    vertices[0] = vertices[1] = vertices[2] = C3Vector(a);
    plane.Set(C3Vector(0.0f, 0.0f, 1.0f), vertices[0]);
  }

  void CFacet::Set(const C3Vector &a, const C3Vector &b, const C3Vector &c) {
    vertices[0] = a;
    vertices[1] = b;
    vertices[2] = c;
    plane.Set(a, b, c);
  }

}  // namespace NTempest
