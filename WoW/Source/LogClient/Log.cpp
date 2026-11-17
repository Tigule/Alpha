#include <storm.h>

BOOL WowLogInitialize() {
  return TRUE;
}

void WowLogDestroy() {
}

void __cdecl WLog(UINT logMask, UINT priority, LPCSTR fmt, ...) {
}

void WVLog(UINT logMask, UINT priority, LPCSTR fmt, va_list arglist) {
}

void WLogDumpHex(UINT logMask, UINT priority, BYTE *data, DWORD len) {
}
