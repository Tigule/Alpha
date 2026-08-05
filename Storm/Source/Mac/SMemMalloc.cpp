#include <storm.h>

#include <stdlib.h>
#include <string.h>

#define SMEM_REALLOC_NOCOPY 0x00000010

typedef size_t SMEMPREFIX;

extern "C" LPVOID APIENTRY SMemAlloc(DWORD bytes, LPCSTR filename, int linenumber, DWORD flags) {
  SMEMPREFIX *block;
  LPVOID      ptr;

  block = static_cast<SMEMPREFIX *>(malloc(bytes + sizeof(SMEMPREFIX)));

  ASSERT(block || !"Out of memory");

  *block = bytes;
  ptr = block + 1;

  if (flags & SMEM_FLAG_ZEROMEMORY) {
    memset(ptr, 0, bytes);
  } else {
    memset(ptr, 0xEE, bytes);
  }

  return ptr;
}

extern "C" LPVOID APIENTRY SMemReAlloc(LPVOID ptr, DWORD bytes, LPCSTR filename, int linenumber, DWORD flags) {
  DWORD  oldBytes;
  LPVOID newPtr;

  if (!ptr) {
    return SMemAlloc(bytes, filename, linenumber, flags);
  }

  oldBytes = static_cast<DWORD>(static_cast<SMEMPREFIX *>(ptr)[-1]);

  if (flags & SMEM_REALLOC_NOCOPY) {
    return 0;
  }

  newPtr = SMemAlloc(bytes, filename, linenumber, flags);
  memcpy(newPtr, ptr, bytes < oldBytes ? bytes : oldBytes);
  SMemFree(ptr, filename, linenumber, 0);

  return newPtr;
}

extern "C" BOOL APIENTRY SMemFree(LPVOID ptr, LPCSTR filename, int linenumber, DWORD flags) {
  SMEMPREFIX *block;

  ASSERT(ptr);

  block = static_cast<SMEMPREFIX *>(ptr) - 1;
  memset(ptr, 0xDD, *block);
  free(block);

  return TRUE;
}

extern "C" BOOL APIENTRY SMemDestroy() {
  return TRUE;
}

extern "C" void APIENTRY SMemSetDebugFlags(DWORD flags, DWORD changeMask) {
}

extern "C" BOOL APIENTRY SMemIsValidPointer(LPCVOID address, DWORD size, BOOL forWriting) {
  return address != 0;
}

extern "C" BOOL APIENTRY SMemFindNextHeap(HSHEAP prevheap, HSHEAP *nextheap, LPSMEMHEAPDETAILS details) {
  return FALSE;
}

extern "C" BOOL APIENTRY SMemDumpState(SMEMDUMPPROC outputproc, HOUTPUTCONTEXT outputcontext) {
  return TRUE;
}

BOOL APIENTRY SMemDumpStateEx(char *arglist) {
  return TRUE;
}

BOOL APIENTRY SMemMarkAllHeapsEx(char *arglist) {
  return TRUE;
}
