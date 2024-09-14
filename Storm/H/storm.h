#pragma once

#ifndef _WIN32
#include <otherwin32.h>
#else
// clang-format off
#include <windows.h>
#include <StormHook.h>
// clang-format on
#endif

#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" LPVOID APIENTRY SMemAlloc(DWORD bytes, LPCSTR filename = NULL, int linenumber = 0, DWORD flags = 0);
extern "C" LPVOID APIENTRY SMemReAlloc(LPVOID ptr, DWORD bytes, LPCSTR filename = NULL, int linenumber = 0, DWORD flags = 0);
extern "C" BOOL APIENTRY   SMemFree(LPVOID ptr, LPCSTR filename = NULL, int linenumber = 0);
