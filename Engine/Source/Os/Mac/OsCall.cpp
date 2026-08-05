#include <Base/Base.h>

void OsCallInitialize(const char *threadName) {
}

void OsCallDestroy() {
}

void *OsCallInitializeContext(const char *contextName) {
  return 0;
}

void OsCallDestroyContext(void *contextDataPtr) {
}

void OsCallSetContext(void *contextDataPtr) {
}

void OsCallResetContext(void *contextDataPtr) {
}
