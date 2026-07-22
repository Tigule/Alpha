#pragma once

#include "ConsoleCommand.h"

#include <stdarg.h>

void __fastcall ConsoleDeviceInitialize(const char *title, bool multithreaded);
void __fastcall ConsoleDeviceDestroy();

void __fastcall ConsoleScreenInitialize(const char *title);
void __fastcall ConsoleScreenDestroy();

int __fastcall  ConsoleIsActive();
void __fastcall ConsoleSetTitle(const char *title);

void __fastcall ConsoleWrite(const char *str, COLOR_T color);
void __cdecl    ConsoleWriteA(const char *str, COLOR_T color, ...);
void __cdecl    ConsolePrintf(const char *str, ...);
void __fastcall ConsoleWriteV(const char *str, va_list arglist);
void __fastcall ConsolePostClose();
