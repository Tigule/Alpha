#include <storm.h>

#include <stdarg.h>

#define STORM_SERVICE_SMEM_GET_ALLOCATOR   1
#define STORM_SERVICE_SMEM_GENERATE_REPORT 2
#define STORM_SERVICE_SSTR_I64_TO_STRING   3
#define STORM_SERVICE_SMEM_MARK_ALL_HEAPS  4

char *Int64ToString(LONGLONG num, char *buf, DWORD destsize);

int ISMemGetAllocator(char *arglist);
int ISMemGenerateReport(char *arglist);
int ISMemMarkAllHeaps(char *arglist);
int ISStrI64ToString(char *arglist);

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

int ISMemGetAllocator(char *arglist) {
  LPVOID *allocator;

  allocator = va_arg(arglist, LPVOID *);
  *allocator = NULL;
  return TRUE;
}

int ISMemGenerateReport(char *arglist) {
  return SMemDumpStateEx(arglist);
}

int ISMemMarkAllHeaps(char *arglist) {
  return SMemMarkAllHeapsEx(arglist);
}

int ISStrI64ToString(char *arglist) {
  LONGLONG num = va_arg(arglist, LONGLONG);
  char    *buf = va_arg(arglist, char *);
  DWORD    destsize = va_arg(arglist, DWORD);
  Int64ToString(num, buf, destsize);
  return TRUE;
}
