#include "../Base/Base.h"

#include "Tempest/c22matrix.h"
#include "Tempest/c2ivector.h"
#include "Tempest/c2vector.h"
#include "Tempest/c33matrix.h"
#include "Tempest/c34matrix.h"
#include "Tempest/c3ivector.h"
#include "Tempest/c3ray.h"
#include "Tempest/c3segment.h"
#include "Tempest/c3spline.h"
#include "Tempest/c3vector.h"
#include "Tempest/c44matrix.h"
#include "Tempest/c4ivector.h"
#include "Tempest/c4plane.h"
#include "Tempest/c4quaternion.h"
#include "Tempest/c4quaternioncompressed.h"
#include "Tempest/c4vector.h"
#include "Tempest/caabox.h"
#include "Tempest/caasphere.h"
#include "Tempest/ccone.h"
#include "Tempest/cdyntable.h"
#include "Tempest/cfacet.h"
#include "Tempest/cimvector.h"
#include "Tempest/cirange.h"
#include "Tempest/cirect.h"
#include "Tempest/cmath.h"
#include "Tempest/cmemblock.h"
#include "Tempest/cobbox.h"
#include "Tempest/cpriorityq.h"
#include "Tempest/crandom.h"
#include "Tempest/crange.h"
#include "Tempest/crect.h"
#include "Tempest/tempest_intersect.h"

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
