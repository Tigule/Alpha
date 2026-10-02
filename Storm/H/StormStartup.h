#pragma once

void StormRtlInitialize();
void StormRtlDestroy();

extern "C" void __cdecl WinMainCRTStartup();

extern "C" void __cdecl StormStaticEntryPoint() {
  StormRtlInitialize();
  WinMainCRTStartup();
  StormRtlDestroy();
}
