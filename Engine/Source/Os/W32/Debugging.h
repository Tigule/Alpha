#pragma once

int OsBeep(unsigned long dwFreq, unsigned long dwDuration);
void __cdecl    OsOutputDebugString(const char *format, ...);
void OsOutputDebugStringV(const char *format, char *args);
