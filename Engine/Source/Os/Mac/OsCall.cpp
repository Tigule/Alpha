#include <Base/Base.h>

void OsCallInitialize(LPCSTR threadName) {
}

void OsCallDestroy() {
}

LPVOID OsCallInitializeContext(LPCSTR contextName) {
  return NULL;
}

void OsCallDestroyContext(LPVOID contextDataPtr) {
}

void OsCallSetContext(LPVOID contextDataPtr) {
}

void OsCallResetContext(LPVOID contextDataPtr) {
}
