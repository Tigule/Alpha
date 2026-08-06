#include <Base/Base.h>

#include "Tempest/c22matrix.h"

namespace NTempest {

  C22Matrix C22Matrix::Rotation(float angle) {
    float cosine = CMath::cos_(angle);
    float sine = CMath::sin_(angle);
    return C22Matrix(cosine, sine, -sine, cosine);
  }

}  // namespace NTempest
