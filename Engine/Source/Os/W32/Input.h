#pragma once

#include "Event/EvtApi.h"

#include <windows.h>

int __fastcall OsGetDefaultWindowRect(RECT *rect);

int __fastcall          OsInputGet(OSINPUT *id, int *param0, int *param1, int *param2, int *param3);
void __fastcall         OsInputNotifyScreenResize(int x, int y);
void __fastcall         OsInputSetScreenIsWindow(int inVal);
void __fastcall         OsInputSetMouseMode(OS_MOUSE_MODE mode);
void __fastcall         OsInputGetMousePosition(int *x, int *y);
void __fastcall         OsInputSetMousePosition(int x, int y);
unsigned int __fastcall OsInputGetCodePage();
void __fastcall         OsInputInitialize();
void __fastcall         OsInputDestroy();
