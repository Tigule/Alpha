#include <Base/Base.h>

#include "Os/W32/OsIME.h"

int OsIMEGetCompositionString(char *string, UINT maxlen) {
  return 0;
}

int OsIMEGetCompositionResult(char *string, UINT maxlen) {
  return 0;
}

BOOL OsIMEGetClauseInfo(UINT &clauseLeft, UINT &clauseRight, UINT &cursorPos) {
  return FALSE;
}

BOOL OsIMEGetCandidates(DWORD which, UINT &pageSize, UINT &count, UINT &selection, TSGrowableArray<OsIMECandidate> &candidates) {
  return FALSE;
}

void OsIMEEnable(int enabled) {
}

void OsIMEInitialize() {
}

void OsIMEDestroy() {
}
