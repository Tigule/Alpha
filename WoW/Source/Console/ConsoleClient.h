#pragma once

#include "ConsoleCommand.h"

#include <stdarg.h>

void ConsoleDeviceInitialize(const char *title, bool multithreaded);
void ConsoleDeviceDestroy();

void ConsoleScreenInitialize(const char *title);
void ConsoleScreenDestroy();

int ConsoleIsActive();
void ConsoleSetTitle(const char *title);

void ConsoleWrite(const char *str, COLOR_T color);
void __cdecl    ConsoleWriteA(const char *str, COLOR_T color, ...);
void __cdecl    ConsolePrintf(const char *str, ...);
void ConsoleWriteV(const char *str, va_list arglist);
void ConsolePostClose();
