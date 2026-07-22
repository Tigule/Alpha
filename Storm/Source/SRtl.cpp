#include <storm.h>

void __fastcall StormRtlDestroy() {
  SErrSetLogCallback(NULL);
  SMemDestroy();
  SErrDestroy();
  SLogDestroy();
}

void __fastcall StormRtlInitialize() {
  SLogInitialize();
  SMemInitialize();
}
