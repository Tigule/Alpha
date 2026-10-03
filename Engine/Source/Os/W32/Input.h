#pragma once

#include "Event/EvtApi.h"

#include <windows.h>

enum {
  OS_INPUT_IME_NOTIFYLANGUAGE = 0,
  OS_INPUT_IME_STARTCOMPOSITION = 1,
  OS_INPUT_IME_COMPOSITION = 2,
  OS_INPUT_IME_OPENCANDIDATES = 3,
  OS_INPUT_IME_CHANGECANDIDATES = 4,
  OS_INPUT_IME_CLOSECANDIDATES = 5,
  OS_INPUT_IME_ENDCOMPOSITION = 6
};

BOOL OsGetDefaultWindowRect(RECT *rect);

BOOL OsInputGet(OSINPUT *id, int *param0, int *param1, int *param2, int *param3);
void OsInputNotifyScreenResize(int x, int y);
void OsInputSetScreenIsWindow(int inVal);
void OsInputSetMouseMode(OS_MOUSE_MODE mode);
void OsInputGetMousePosition(int *x, int *y);
void OsInputSetMousePosition(int x, int y);
UINT OsInputGetCodePage();
void OsInputInitialize();
void OsInputDestroy();
