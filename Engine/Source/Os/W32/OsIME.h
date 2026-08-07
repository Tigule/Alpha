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
int                 OsIMEGetCompositionString(char *string, UINT maxlen);
int                 OsIMEGetCompositionResult(char *string, UINT maxlen);
BOOL                OsIMEGetClauseInfo(UINT &clauseLeft, UINT &clauseRight, UINT &cursorPos);
BOOL                OsIMEGetCandidates(DWORD which, UINT &pageSize, UINT &count, UINT &selection, TSGrowableArray<OsIMECandidate> &candidates);
void                OsIMEEnable(int enabled);
void                OsIMEInitialize();
void                OsIMEDestroy();
