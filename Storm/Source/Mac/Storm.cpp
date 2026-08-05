#include <storm.h>

void SErrInitialize();

extern "C" BOOL APIENTRY SRegDestroy();

extern "C" void APIENTRY StormInitialize() {
  SErrInitialize();
  SLogInitialize();
}

extern "C" void APIENTRY StormDestroy() {
  SRegDestroy();
  SErrDestroy();
  SLogDestroy();
  SFileDestroy();
  SFile::Destroy();
}
