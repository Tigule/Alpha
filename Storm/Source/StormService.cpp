#include <windows.h>
#include <dsound.h>
#include <storm.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#define STORM_SERVICE_SMEM_GET_ALLOCATOR   1
#define STORM_SERVICE_SMEM_GENERATE_REPORT 2
#define STORM_SERVICE_SSTR_I64_TO_STRING   3
#define STORM_SERVICE_SMEM_MARK_ALL_HEAPS  4

char *Int64ToString(LONGLONG num, char *buf, DWORD destsize);

int ISMemGetAllocator(va_list arglist);
int ISMemGenerateReport(va_list arglist);
int ISMemMarkAllHeaps(va_list arglist);
int ISStrI64ToString(va_list arglist);

extern "C" int __cdecl StormCallService(int selector, ...) {
  va_list args;
  int     result;

  result = FALSE;
  va_start(args, selector);

  switch (selector) {
    case STORM_SERVICE_SMEM_GET_ALLOCATOR:
      result = ISMemGetAllocator(args);
      break;

    case STORM_SERVICE_SMEM_GENERATE_REPORT:
      result = ISMemGenerateReport(args);
      break;

    case STORM_SERVICE_SSTR_I64_TO_STRING:
      result = ISStrI64ToString(args);
      break;

    case STORM_SERVICE_SMEM_MARK_ALL_HEAPS:
      result = ISMemMarkAllHeaps(args);
      break;

    default:
      break;
  }

  va_end(args);
  return result;
}

int ISMemGetAllocator(va_list arglist) {
  LPVOID *allocator;

  allocator = va_arg(arglist, LPVOID *);
  *allocator = NULL;
  return TRUE;
}

int ISMemGenerateReport(va_list arglist) {
  return SMemDumpStateEx(arglist);
}

int ISMemMarkAllHeaps(va_list arglist) {
  return SMemMarkAllHeapsEx(arglist);
}

int ISStrI64ToString(va_list arglist) {
  LONGLONG num = va_arg(arglist, LONGLONG);
  char    *buf = va_arg(arglist, char *);
  DWORD    destsize = va_arg(arglist, DWORD);
  Int64ToString(num, buf, destsize);
  return TRUE;
}
