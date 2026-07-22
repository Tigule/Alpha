#include "Tempest/cmath.h"

namespace NTempest {

  unsigned long __fastcall CMath::mulhwu_(unsigned long x, unsigned long y) {
    return static_cast<unsigned long>((static_cast<unsigned __int64>(x) * y) >> 32);
  }

}  // namespace NTempest
