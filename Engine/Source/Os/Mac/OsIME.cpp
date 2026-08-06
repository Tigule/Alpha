#include <Base/Base.h>

#include "Os/W32/OsIME.h"

int OsIMEGetCompositionString(char *string, UINT maxlen) {
  return 0;
}

int OsIMEGetCompositionResult(char *string, UINT maxlen) {
  return 0;
}

int OsIMEGetClauseInfo(UINT &clauseLeft, UINT &clauseRight, UINT &cursorPos) {
  return 0;
}

int OsIMEGetCandidates(DWORD which, UINT &pageSize, UINT &count, UINT &selection, TSGrowableArray<OsIMECandidate> &candidates) {
  return 0;
}

void OsIMEEnable(int enabled) {
}

void OsIMEInitialize() {
}

void OsIMEDestroy() {
}
