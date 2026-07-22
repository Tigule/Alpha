#include "Debugging.h"

#include <windows.h>
#include <stdarg.h>
#include <stdio.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

int __fastcall OsBeep(unsigned long dwFreq, unsigned long dwDuration) {
  return Beep(dwFreq, dwDuration);
}

void __cdecl OsOutputDebugString(const char *format, ...) {
  char    buffer[256];
  va_list args;

  va_start(args, format);
  _vsnprintf(buffer, sizeof(buffer), format, args);
  OutputDebugStringA(buffer);
}

void __fastcall OsOutputDebugStringV(const char *format, char *args) {
  char buffer[256];

  _vsnprintf(buffer, sizeof(buffer), format, args);
  OutputDebugStringA(buffer);
}
