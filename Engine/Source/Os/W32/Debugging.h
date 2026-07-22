#pragma once

int __fastcall  OsBeep(unsigned long dwFreq, unsigned long dwDuration);
void __cdecl    OsOutputDebugString(const char *format, ...);
void __fastcall OsOutputDebugStringV(const char *format, char *args);
