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

  C3Vector::EAxis C3Vector::MajorAxis() const {
    float fx = CMath::fabs_(x);
    float fy = CMath::fabs_(y);
    float fz = CMath::fabs_(z);

    if (fx > fy) {
      if (fx > fz) {
        return C3AXIS_X;
      }
      return C3AXIS_Z;
    }

    if (fy > fz) {
      return C3AXIS_Y;
    }
    return C3AXIS_Z;
  }

  C3Vector::EAxis C3Vector::MinorAxis() const {
    float fx = CMath::fabs_(x);
    float fy = CMath::fabs_(y);
    float fz = CMath::fabs_(z);

    if (fx < fy) {
      if (fx < fz) {
        return C3AXIS_X;
      }
      return C3AXIS_Z;
    }

    if (fz < fy) {
      return C3AXIS_Z;
    }
    return C3AXIS_Y;
  }

}  // namespace NTempest
