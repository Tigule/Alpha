#include <Base/Base.h>

#include "Os/W32/Debugging.h"

#include <storm.h>

#include <stdarg.h>
#include <stdio.h>

static DWORD s_outputDebugString = (DWORD)-1;

void __cdecl OsOutputDebugString(LPCSTR format, ...) {
  char    buffer[256];
  va_list args;

  if (s_outputDebugString == (DWORD)-1) {
    s_outputDebugString = 0;
    SRegLoadValue("Internal", "OutputDebugString", 0, &s_outputDebugString);
  }

  if (s_outputDebugString) {
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    printf(buffer);
  }
}
