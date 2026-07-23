#pragma once

enum OS_IME_LANGUAGEMODE {
  OS_IME_MODE_ROMAN = 0,
  OS_IME_MODE_JAPANESE = 1,
  OS_IME_MODE_KOREAN = 2,
  OS_IME_MODE_CHINESE = 3
};

OS_IME_LANGUAGEMODE __fastcall OsIMEGetLanguageMode();
int __fastcall OsIMEGetCompositionString(char *string, unsigned int maxlen);
int __fastcall OsIMEGetCompositionResult(char *string, unsigned int maxlen);
int __fastcall OsIMEGetClauseInfo(unsigned int &clauseLeft, unsigned int &clauseRight, unsigned int &cursorPos);
void __fastcall OsIMEEnable(int enabled);
void __fastcall OsIMEInitialize();
void __fastcall OsIMEDestroy();
