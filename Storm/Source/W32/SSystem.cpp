#include <storm.h>

#include <stdarg.h>
#include <stdio.h>

void __cdecl SOutputDebugString(const char *format, ...) {
  char    buffer[0x100];
  va_list args;

  va_start(args, format);
  _vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  OutputDebugStringA(buffer);
}
