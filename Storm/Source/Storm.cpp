#include <storm.h>

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
