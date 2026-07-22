#ifndef WOW_SOURCE_GAME_VALIDATENAME_H
#define WOW_SOURCE_GAME_VALIDATENAME_H

#include "DB/WowClientDB.h"

enum VALIDATE_NAME_RESULT {
  NAME_NO_NAME = 0,
  NAME_TOO_SHORT = 1,
  NAME_TOO_LONG = 2,
  NAME_STARTS_WITH_GRAVE = 3,
  NAME_TWO_GRAVES = 4,
  NAME_INVALID_CHARACTER = 5,
  NAME_MIXED_LANGUAGES = 6,
  NAME_PROFANE = 7,
  NAME_RESERVED = 8,
  NAME_FAILURE = 9,
  NAME_SUCCESS = 10,
  NUM_NAME_RESULTS = 11
};

enum CHARSET {
  CHARSET_UNKNOWN = 0,
  CHARSET_LATIN1 = 1,
  CHARSET_KOREAN = 2
};

void __fastcall                 ValidateNameInitialize();
void __fastcall                 ValidateNameDestroy();
VALIDATE_NAME_RESULT __fastcall ValidateCharacterName(WOW_LOCALE locale, const char *name);

#endif
