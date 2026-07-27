#pragma once

#include "Event/EvtApi.h"

#include <windows.h>

int OsGetDefaultWindowRect(RECT *rect);

int OsInputGet(OSINPUT *id, int *param0, int *param1, int *param2, int *param3);
void OsInputNotifyScreenResize(int x, int y);
void OsInputSetScreenIsWindow(int inVal);
void OsInputSetMouseMode(OS_MOUSE_MODE mode);
void OsInputGetMousePosition(int *x, int *y);
void OsInputSetMousePosition(int x, int y);
unsigned int OsInputGetCodePage();
void OsInputInitialize();
void OsInputDestroy();
