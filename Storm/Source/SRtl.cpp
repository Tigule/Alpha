#include <windows.h>
#include <dsound.h>
#include <storm.h>
#include <stdio.h>
#include <time.h>

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
