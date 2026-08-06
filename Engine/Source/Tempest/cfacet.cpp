#include <Base/Base.h>

#include "Tempest/cfacet.h"

#include <math.h>

namespace NTempest {

  void CFacet::Set(float a) {
    for (UINT i = 0; i < 3; ++i) {
      vertices[i].x = a;
      vertices[i].y = a;
      vertices[i].z = a;
    }
    plane.n.Set(0.0f, 0.0f, 1.0f);
    plane.d = -C3Vector::Dot(plane.n, vertices[0]);
  }

  void CFacet::Set(const C3Vector &a, const C3Vector &b, const C3Vector &c) {
    vertices[0] = a;
    vertices[1] = b;
    vertices[2] = c;
    plane.From3Pos(a, b, c);
  }

}  // namespace NTempest
