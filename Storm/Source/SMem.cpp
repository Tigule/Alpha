#include <storm.h>

LPVOID APIENTRY SMemAlloc(DWORD bytes, LPCSTR filename, int linenumber, DWORD flags) {
  return NULL;
}

BOOL APIENTRY SMemIsValidPointer(LPVOID address, DWORD *size, BOOL forWriting) {
  return TRUE;
}

LPVOID APIENTRY SMemReAlloc(LPVOID ptr, DWORD bytes, LPCSTR filename, int linenumber, DWORD flags) {
  return NULL;
}

BOOL APIENTRY SMemFree(LPVOID ptr, LPCSTR filename, int linenumber) {
  return TRUE;
}

#ifdef WOWHOOK
void StormHook_SMem() {
  // WOWHOOK_ATTACH(SMemAlloc, 0x2348f0);
  // WOWHOOK_ATTACH(SMemIsValidPointer, 0x236d20);
  // WOWHOOK_ATTACH(SMemReAlloc, 0x236d60);
  // WOWHOOK_ATTACH(SMemFree, 0x2361b0);
}
#endif
