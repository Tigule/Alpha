#include <Base/Base.h>

#include "Debugging.h"

#include <windows.h>
#include <stdarg.h>
#include <stdio.h>

int OsBeep(DWORD dwFreq, DWORD dwDuration) {
  return Beep(dwFreq, dwDuration);
}

void __cdecl OsOutputDebugString(LPCSTR format, ...) {
  char    buffer[256];
  va_list args;

  va_start(args, format);
  _vsnprintf(buffer, sizeof(buffer), format, args);
  OutputDebugStringA(buffer);
}

void OsOutputDebugStringV(LPCSTR format, char *args) {
  char buffer[256];

  _vsnprintf(buffer, sizeof(buffer), format, args);
  OutputDebugStringA(buffer);
}
