#include <windows.h>
#include <dsound.h>
#include <storm.h>
#include <stdio.h>
#include <time.h>

extern "C" void APIENTRY StormInitialize() {
  SStrInitialize();
  SErrInitialize();
  SServerInitialize();
}

extern "C" void APIENTRY StormDestroy() {
  SRgnDestroy();
  SMsgDestroy();
  SEvtDestroy();
  SCmdDestroy();
  SFileDestroy();
  SFile::Destroy();
  SCompDestroy();
  SStrDestroy();
}

extern "C" HINSTANCE APIENTRY StormGetInstance() {
  return GetModuleHandleA(NULL);
}
