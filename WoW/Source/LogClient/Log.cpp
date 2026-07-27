int WowLogInitialize() {
  return 1;
}

void WowLogDestroy() {
}

void __cdecl WLog(unsigned int logMask, unsigned int priority, const char* fmt, ...) {
}

void WVLog(unsigned int logMask, unsigned int priority, const char *fmt, char *arglist) {
}

void WLogDumpHex(unsigned int logMask, unsigned int priority, unsigned char* data, unsigned long len) {
}
