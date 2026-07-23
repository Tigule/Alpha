#include "OsGui.h"
#include "OsIME.h"

#include <windows.h>
#include <imm.h>

#ifndef IACE_DEFAULT
#define IACE_DEFAULT 0x0010
extern "C" BOOL WINAPI ImmAssociateContextEx(HWND, HIMC, DWORD);
#endif

static HIMC s_IMC;

OS_IME_LANGUAGEMODE __fastcall OsIMEGetLanguageMode() {
    // TODO: implement
    return OS_IME_LANGUAGEMODE();
}

static int GetCompositionString(int which, char* string, int maxlen) {
    // TODO: implement
    return 0;
}

int __fastcall OsIMEGetCompositionString(char* string, unsigned int maxlen) {
    // TODO: implement
    return 0;
}

int __fastcall OsIMEGetCompositionResult(char* string, unsigned int maxlen) {
    // TODO: implement
    return 0;
}

int __fastcall OsIMEGetClauseInfo(unsigned int& clauseLeft, unsigned int& clauseRight, unsigned int& cursorPos) {
    // TODO: implement
    return 0;
}

void __fastcall OsIMEEnable(int enabled) {
    // TODO: implement
}

void __fastcall OsIMEInitialize() {
  s_IMC = ImmAssociateContext((HWND)OsGuiGetWindow(0), 0);
}

void __fastcall OsIMEDestroy() {
  ImmAssociateContextEx((HWND)OsGuiGetWindow(0), 0, IACE_DEFAULT);
  s_IMC = 0;
}
