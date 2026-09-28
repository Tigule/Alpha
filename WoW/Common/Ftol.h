#pragma once

static float OneHalfOffset = 0.5f;

inline int Fast_ftol(float fval) {
  int result;
  __asm {
    fld fval
    fsub OneHalfOffset
    fistp result
  }
  return result;
}
