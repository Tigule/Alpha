#include <Base/Base.h>

int WowLogInitialize() {
  return 1;
}

void WowLogDestroy() {
}

void __cdecl WLog(UINT logMask, UINT priority, LPCSTR fmt, ...) {
}

void WVLog(UINT logMask, UINT priority, LPCSTR fmt, char *arglist) {
}

void WLogDumpHex(UINT logMask, UINT priority, BYTE *data, DWORD len) {
}
