#include <storm.h>

#include <stdarg.h>

#define STORM_SERVICE_SMEM_GET_ALLOCATOR   1
#define STORM_SERVICE_SMEM_GENERATE_REPORT 2
#define STORM_SERVICE_SSTR_I64_TO_STRING   3
#define STORM_SERVICE_SMEM_MARK_ALL_HEAPS  4

char *__fastcall Int64ToString(__int64 num, char *buf, DWORD destsize);

static int __fastcall ISMemGetAllocator(char *arglist);
static int __fastcall ISMemGenerateReport(char *arglist);
static int __fastcall ISMemMarkAllHeaps(char *arglist);
static int __fastcall ISStrI64ToString(char *arglist);

extern "C" int __cdecl StormCallService(int selector, ...) {
  va_list args;
  int     result;

  result = FALSE;
  va_start(args, selector);

  switch (selector) {
    case STORM_SERVICE_SMEM_GET_ALLOCATOR:
      result = ISMemGetAllocator((char *)args);
      break;

    case STORM_SERVICE_SMEM_GENERATE_REPORT:
      result = ISMemGenerateReport((char *)args);
      break;

    case STORM_SERVICE_SSTR_I64_TO_STRING:
      result = ISStrI64ToString((char *)args);
      break;

    case STORM_SERVICE_SMEM_MARK_ALL_HEAPS:
      result = ISMemMarkAllHeaps((char *)args);
      break;

    default:
      break;
  }

  va_end(args);
  return result;
}

static int __fastcall ISMemGetAllocator(char *arglist) {
  void **allocator;

  allocator = va_arg(arglist, void **);
  *allocator = NULL;
  return TRUE;
}

static int __fastcall ISMemGenerateReport(char *arglist) {
  return SMemDumpStateEx(arglist);
}

static int __fastcall ISMemMarkAllHeaps(char *arglist) {
  return SMemMarkAllHeapsEx(arglist);
}

static int __fastcall ISStrI64ToString(char *arglist) {
  Int64ToString(*(__int64 *)arglist, *(char **)(arglist + 8), *(DWORD *)(arglist + 12));
  return TRUE;
}
