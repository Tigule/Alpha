#include <Base/EngineHook.h>
#include <WowHook/WowHook.h>
#include <storm.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

void SetupConsole() {
  if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
    AllocConsole();
  }

  FILE *file = NULL;
  freopen_s(&file, "CONIN$", "r", stdin);
  freopen_s(&file, "CONOUT$", "w", stdout);
  freopen_s(&file, "CONOUT$", "w", stderr);

  COORD consoleBufferSize = {80, 32766};
  SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), consoleBufferSize);
  SetConsoleScreenBufferSize(GetStdHandle(STD_ERROR_HANDLE), consoleBufferSize);
}

#pragma region Win32 APIs
void(__stdcall *fn_OutputDebugStringW)(LPCWSTR lpOutputString) = OutputDebugStringW;
void __stdcall OutputDebugStringW_hook(LPCWSTR lpOutputString) {
  printf("%ls", lpOutputString);
  fn_OutputDebugStringW(lpOutputString);
}

void(__stdcall *fn_OutputDebugStringA)(LPCSTR lpOutputString) = OutputDebugStringA;
void __stdcall OutputDebugStringA_hook(LPCSTR lpOutputString) {
  printf("%s", lpOutputString);
  fn_OutputDebugStringA(lpOutputString);
}

void Win32Hook() {
  DetourAttach(&(PVOID &)fn_OutputDebugStringW, (PVOID)OutputDebugStringW_hook);
  DetourAttach(&(PVOID &)fn_OutputDebugStringA, (PVOID)OutputDebugStringA_hook);
}
#pragma endregion

void Init() {
  SetupConsole();

  DetourTransactionBegin();
  DetourUpdateThread(GetCurrentThread());

  // Win32Hook();
  EngineHook();
  StormHook();
  WowHook();

  LONG error = DetourTransactionCommit();
  if (error != NO_ERROR) {
    printf("Error attaching hooks\n");
    exit(1);
  }
}

BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD fdwReason, LPVOID lpvReserved) {
  if (DetourIsHelperProcess()) {
    return TRUE;
  }

  switch (fdwReason) {
    case DLL_PROCESS_ATTACH: {
      DetourRestoreAfterWith();
      Init();
      break;
    }

    case DLL_THREAD_ATTACH:
      break;

    case DLL_THREAD_DETACH:
      break;

    case DLL_PROCESS_DETACH:
      break;
  }

  return TRUE;
}
