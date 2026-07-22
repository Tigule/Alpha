#include "Tempest/c3vector.h"

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

    if (fy <= fz) {
      return C3AXIS_Y;
    }
    return C3AXIS_Z;
  }

}  // namespace NTempest
