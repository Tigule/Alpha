#include <storm.h>

void StormRtlDestroy() {
  SErrSetLogCallback(NULL);
  SMemDestroy();
  SErrDestroy();
  SLogDestroy();
}

void StormRtlInitialize() {
  SLogInitialize();
  SMemInitialize();
}
