#pragma once

#include <stpl.h>

enum OS_IME_LANGUAGEMODE {
  OS_IME_MODE_ROMAN = 0,
  OS_IME_MODE_JAPANESE = 1,
  OS_IME_MODE_KOREAN = 2,
  OS_IME_MODE_CHINESE = 3
};

struct OsIMECandidate {
  char candidate[1024];
};

OS_IME_LANGUAGEMODE OsIMEGetLanguageMode();
int OsIMEGetCompositionString(char *string, unsigned int maxlen);
int OsIMEGetCompositionResult(char *string, unsigned int maxlen);
int OsIMEGetClauseInfo(unsigned int &clauseLeft, unsigned int &clauseRight, unsigned int &cursorPos);
int OsIMEGetCandidates(
    unsigned long which,
    unsigned int &pageSize,
    unsigned int &count,
    unsigned int &selection,
    TSGrowableArray<OsIMECandidate> &candidates);
void OsIMEEnable(int enabled);
void OsIMEInitialize();
void OsIMEDestroy();
