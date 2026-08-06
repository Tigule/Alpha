#include <Base/Base.h>

void OsCallInitialize(LPCSTR threadName) {
}

void OsCallDestroy() {
}

LPVOID OsCallInitializeContext(LPCSTR contextName) {
  return 0;
}

void OsCallDestroyContext(LPVOID contextDataPtr) {
}

void OsCallSetContext(LPVOID contextDataPtr) {
}

void OsCallResetContext(LPVOID contextDataPtr) {
}
