int __fastcall WowLogInitialize() {
  return 1;
}

void __fastcall WowLogDestroy() {
}

void __cdecl WLog(unsigned int logMask, unsigned int priority, const char* fmt, ...) {
}

void __fastcall WVLog(unsigned int logMask, unsigned int priority, const char *fmt, char *arglist) {
}

void __fastcall WLogDumpHex(unsigned int logMask, unsigned int priority, unsigned char* data, unsigned long len) {
}
