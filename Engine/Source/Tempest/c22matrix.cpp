#include <Base/Base.h>

#include "Tempest/c22matrix.h"

namespace NTempest {

  C22Matrix C22Matrix::Rotation(float angle) {
    float sine;
    float cosine;
    CMath::sincos_(angle, sine, cosine);
    return C22Matrix(cosine, sine, -sine, cosine);
  }

}  // namespace NTempest
