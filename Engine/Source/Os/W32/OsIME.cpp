#include "OsGui.h"

#include <windows.h>
#include <imm.h>

#ifndef IACE_DEFAULT
#define IACE_DEFAULT 0x0010
extern "C" BOOL WINAPI ImmAssociateContextEx(HWND, HIMC, DWORD);
#endif

static HIMC s_IMC;

void __fastcall OsIMEInitialize() {
  s_IMC = ImmAssociateContext((HWND)OsGuiGetWindow(0), 0);
}

void __fastcall OsIMEDestroy() {
  ImmAssociateContextEx((HWND)OsGuiGetWindow(0), 0, IACE_DEFAULT);
  s_IMC = 0;
}
