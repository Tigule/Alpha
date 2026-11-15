#pragma once

#if BUILD_PORTABLE
#include <math.h>
#endif

static float OneHalfOffset = 0.5f;

#if !BUILD_PORTABLE
inline int Fast_ftol(float fval) {
  int result;
  __asm {
    fld fval
    fsub OneHalfOffset
    fistp result
  }
  return result;
}
#else
inline int Fast_ftol(float fval) {
  return lrint((double)fval - OneHalfOffset);
}
#endif
