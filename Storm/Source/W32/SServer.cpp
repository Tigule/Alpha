#include <storm.h>

typedef BOOL(WINAPI *TRYENTERCRITSECT)(LPCRITICAL_SECTION);

static TRYENTERCRITSECT s_tryEnterPtr;
static HMODULE          s_kernel32lib;

void __fastcall SServerInitialize() {
  if (!s_tryEnterPtr) {
    s_kernel32lib = LoadLibraryA("kernel32.dll");
    if (s_kernel32lib) {
      s_tryEnterPtr = (TRYENTERCRITSECT)GetProcAddress(s_kernel32lib, "TryEnterCriticalSection");
    }
  }
}

void __fastcall SServerDestroy() {
  if (s_kernel32lib) {
    FreeLibrary(s_kernel32lib);
  }
}

int __fastcall STryEnterCriticalSection(void *opaqueData) {
  if (!s_tryEnterPtr) {
    FATALERROR(("TryEnterCriticalSection not found on this OS."));
  }

  return s_tryEnterPtr((LPCRITICAL_SECTION)opaqueData);
}
