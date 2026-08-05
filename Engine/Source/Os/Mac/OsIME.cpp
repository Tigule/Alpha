#include <Base/Base.h>

#include "Os/W32/OsIME.h"

int OsIMEGetCompositionString(char *string, unsigned int maxlen) {
  return 0;
}

int OsIMEGetCompositionResult(char *string, unsigned int maxlen) {
  return 0;
}

int OsIMEGetClauseInfo(unsigned int &clauseLeft, unsigned int &clauseRight, unsigned int &cursorPos) {
  return 0;
}

int OsIMEGetCandidates(
    DWORD                            which,
    unsigned int                    &pageSize,
    unsigned int                    &count,
    unsigned int                    &selection,
    TSGrowableArray<OsIMECandidate> &candidates
) {
  return 0;
}

void OsIMEEnable(int enabled) {
}

void OsIMEInitialize() {
}

void OsIMEDestroy() {
}
