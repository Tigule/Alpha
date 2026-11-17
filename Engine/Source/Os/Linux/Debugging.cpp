#include <Base/Base.h>

#include "Os/W32/Debugging.h"

#include <storm.h>

#include <stdarg.h>
#include <stdio.h>

void __cdecl OsOutputDebugString(LPCSTR format, ...) {
  char    buffer[256];
  va_list args;

  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  fprintf(stderr, "%s", buffer);
}
