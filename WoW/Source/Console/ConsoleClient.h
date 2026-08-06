#pragma once

#include "ConsoleCommand.h"

#include <stdarg.h>

void ConsoleDeviceInitialize(LPCSTR title, bool multithreaded);
void ConsoleDeviceDestroy();

void ConsoleScreenInitialize(LPCSTR title);
void ConsoleScreenDestroy();

int  ConsoleIsActive();
void ConsoleSetTitle(LPCSTR title);

void         ConsoleWrite(LPCSTR str, COLOR_T color);
void __cdecl ConsoleWriteA(LPCSTR str, COLOR_T color, ...);
void __cdecl ConsolePrintf(LPCSTR str, ...);
void         ConsoleWriteV(LPCSTR str, va_list arglist);
void         ConsolePostClose();
