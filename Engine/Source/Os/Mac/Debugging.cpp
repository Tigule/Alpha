#include <Base/Base.h>

#include "Os/W32/Debugging.h"

#include <storm.h>

#include <stdarg.h>
#include <stdio.h>

void __cdecl SOutputDebugString(LPCSTR format, ...);

BOOL OsBeep(DWORD dwFreq, DWORD dwDuration) {
  return 0;
}

void __cdecl OsOutputDebugString(LPCSTR format, ...) {
  char    buffer[256];
  va_list args;

  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  SOutputDebugString("%s", buffer);
}

void OsOutputDebugStringV(LPCSTR format, char *args) {
  char buffer[256];

  vsnprintf(buffer, sizeof(buffer), format, args);
  SOutputDebugString("%s", buffer);
}
