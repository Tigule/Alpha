#pragma once

int          OsBeep(DWORD dwFreq, DWORD dwDuration);
void __cdecl OsOutputDebugString(LPCSTR format, ...);
void         OsOutputDebugStringV(LPCSTR format, char *args);
