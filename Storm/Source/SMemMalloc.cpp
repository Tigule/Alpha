#include <storm.h>

LPVOID APIENTRY SMemAlloc(DWORD bytes, LPCSTR filename, int linenumber, DWORD flags) {
  LPVOID ptr;
  if (flags & 0x8) {
    ptr = calloc(1, bytes + sizeof(DWORD));
  } else {
    ptr = malloc(bytes + sizeof(DWORD));
  }

  ASSERT(ptr);

  *(DWORD *)ptr = bytes;
  ptr = (DWORD *)ptr + 1;

  if ((flags & 0x8) == 0) {
    memset(ptr, 0xCD, bytes);
  }

  return ptr;
}

BOOL APIENTRY SMemFree(LPVOID ptr, LPCSTR filename, int linenumber, DWORD flags) {
  ASSERT(ptr);

  memset(ptr, 0xDD, *((DWORD *)ptr - 1));
  free((DWORD *)ptr - 1);

  return TRUE;
}

BOOL APIENTRY SMemIsValidPointer(LPDWORD address, DWORD size, BOOL forWriting) {
  // intentionally empty
  return TRUE;
}

LPVOID APIENTRY SMemReAlloc(LPVOID ptr, DWORD bytes, LPCSTR filename, int linenumber, DWORD flags) {
  if (!ptr) {
    return SMemAlloc(bytes, filename, linenumber, flags);
  }

  LPVOID result = SMemAlloc(bytes, filename, linenumber, flags);

  DWORD lastalloc = *((DWORD *)ptr - 1);
  DWORD realloc = bytes;
  if (bytes > lastalloc) {
    realloc = lastalloc;
  }

  memcpy(result, ptr, realloc);
  SMemFree(ptr);

  return result;
}

void APIENTRY SMemSetDebugFlags(DWORD flags, DWORD changeMask) {
  // intentionally empty
}
