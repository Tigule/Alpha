#pragma once

#ifdef WOWHOOK
// clang-format off
#include <windows.h>
#include <detours.h>
// clang-format on

#define WOWHOOK_ATTACH(func, addr)                     \
  {                                                    \
    PVOID temp = (PVOID)(void (*)())(0x400000 + addr); \
    DetourAttach(&temp, (PVOID)(func));                \
  }

void StormHook();
void StormHook_SMem();
#endif
