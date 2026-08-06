#include <storm.h>

typedef BOOL(WINAPI *TRYENTERCRITSECT)(LPCRITICAL_SECTION);

static TRYENTERCRITSECT s_tryEnterPtr;
static HMODULE          s_kernel32lib;

void SServerInitialize() {
  if (!s_tryEnterPtr) {
    s_kernel32lib = LoadLibraryA("kernel32.dll");
    if (s_kernel32lib) {
      s_tryEnterPtr = (TRYENTERCRITSECT)GetProcAddress(s_kernel32lib, "TryEnterCriticalSection");
    }
  }
}

void SServerDestroy() {
  if (s_kernel32lib) {
    FreeLibrary(s_kernel32lib);
  }
}

int STryEnterCriticalSection(LPVOID opaqueData) {
  if (!s_tryEnterPtr) {
    FATALERROR(("TryEnterCriticalSection not found on this OS."));
  }

  return s_tryEnterPtr((LPCRITICAL_SECTION)opaqueData);
}
