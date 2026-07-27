#include "Game/ValidateName.h"

#include <DB/DBClient/AutoCode/NamesProfanityRec.h>
#include <DB/DBClient/AutoCode/NamesReservedRec.h>
#include <regex/regex.h>
#include <storm.h>
#include <stpl.h>

static const char GRAVE = '`';
static unsigned short Valid_Korean[0x92E] = {
#include "Game/ValidKorean.inl"
};

TSFixedArray<regex_t> g_profanityTokens;

static VALIDATE_NAME_RESULT
ValidateName(WOW_LOCALE locale, unsigned short *validChars, const char *name, unsigned int &nameLength, CHARSET &charset);
static CHARSET GetCharSet(unsigned short ch);
static bool IsAlpha(WOW_LOCALE locale, unsigned short ch);
static bool IsLatin1(unsigned short ch);
static bool IsAlphaLatin1(unsigned short ch);
static bool IsKorean(unsigned short ch);
static bool IsAlphaKorean(unsigned short ch);

void ValidateNameInitialize() {
  unsigned int numRecords = g_namesProfanityDB.GetNumRecords();
  unsigned int i;

  g_profanityTokens.SetCount(numRecords);

  for (i = 0; i < numRecords; ++i) {
    const NamesProfanityRec *rec = g_namesProfanityDB.GetRecordByIndex(i);

    if (regcomp(&g_profanityTokens[i], rec->m_Name, REG_EXTENDED | REG_ICASE)) {
      FATALERROR(("Error, invalid profanity filter expression: \"%s\"", rec->m_Name));
    }
  }
}

void ValidateNameDestroy() {
  g_profanityTokens.Clear();
}

static VALIDATE_NAME_RESULT
ValidateName(WOW_LOCALE locale, unsigned short *validChars, const char *name, unsigned int &nameLength, CHARSET &charset) {
  unsigned short uniName[0x400];
  regmatch_t     match;
  unsigned int   usedGrave;

  if (!name) {
    return NAME_TOO_SHORT;
  }

  if (SUniConvertUTF8to16(uniName, 0x400, name, 0x7FFFFFFF, 0, 0)) {
    return NAME_FAILURE;
  }

  if (*name == GRAVE) {
    return NAME_STARTS_WITH_GRAVE;
  }

  nameLength = SStrLen(uniName);
  charset = CHARSET_UNKNOWN;
  usedGrave = 0;

  for (unsigned short *ch = uniName; *ch; ++ch) {
    if (IsAlpha(locale, *ch)) {
      CHARSET current = GetCharSet(*ch);
      if (charset == CHARSET_UNKNOWN) {
        charset = current;
      } else if (charset != current) {
        return NAME_MIXED_LANGUAGES;
      }
    } else {
      unsigned short *valid = validChars;
      while (valid && *valid && *valid != *ch) {
        ++valid;
      }

      if (!valid || !*valid) {
        if (*ch != GRAVE) {
          return NAME_INVALID_CHARACTER;
        }
        if (usedGrave) {
          return NAME_TWO_GRAVES;
        }
        usedGrave = 1;
      }
    }
  }

  if (usedGrave) {
    --nameLength;
  }
  if (nameLength < 3) {
    return nameLength ? NAME_TOO_SHORT : NAME_NO_NAME;
  }

  for (unsigned int i = 0; i < g_profanityTokens.Count(); ++i) {
    if (!regexec(&g_profanityTokens[i], name, 1, &match, 0) && match.rm_so >= 0) {
      return NAME_PROFANE;
    }
  }

  unsigned int numReserved = g_namesReservedDB.GetNumRecords();
  for (unsigned int reservedIndex = 0; reservedIndex < numReserved; ++reservedIndex) {
    const NamesReservedRec *reserved = g_namesReservedDB.GetRecordByIndex(reservedIndex);
    if (!SStrCmpI(name, reserved->m_Name, 0x7FFFFFFF)) {
      return NAME_RESERVED;
    }
  }

  return NAME_SUCCESS;
}

VALIDATE_NAME_RESULT ValidateCharacterName(WOW_LOCALE locale, const char *name) {
  unsigned int         length;
  CHARSET              charset;
  VALIDATE_NAME_RESULT result = ValidateName(locale, 0, name, length, charset);

  if (result == NAME_SUCCESS) {
    unsigned int maxLength = charset == CHARSET_LATIN1 ? 12 : 8;
    if (length > maxLength) {
      result = NAME_TOO_LONG;
    }
  }

  return result;
}

static CHARSET GetCharSet(unsigned short ch) {
  if (IsLatin1(ch)) {
    return CHARSET_LATIN1;
  }
  return IsKorean(ch) ? CHARSET_KOREAN : CHARSET_UNKNOWN;
}

static bool IsAlpha(WOW_LOCALE locale, unsigned short ch) {
  if (IsLatin1(ch)) {
    return IsAlphaLatin1(ch);
  }
  if (locale == LOCALE_ko_KR && IsKorean(ch)) {
    return IsAlphaKorean(ch);
  }
  return 0;
}

static bool IsLatin1(unsigned short ch) {
  return ch <= 0x00FF;
}

static bool IsAlphaLatin1(unsigned short ch) {
  return (ch >= 0x0041 && ch <= 0x005A) || (ch >= 0x0061 && ch <= 0x007A) || (ch >= 0x00C0 && ch <= 0x00DD) || (ch >= 0x00E0 && ch <= 0x00FF);
}

static bool IsKorean(unsigned short ch) {
  return (ch >= 0x1100 && ch <= 0x11FF) || (ch >= 0x3130 && ch <= 0x318F) || (ch >= 0xAC00 && ch <= 0xD7A3);
}

static bool IsAlphaKorean(unsigned short ch) {
  if (ch < 0xAC00 || ch > 0xD7A3) {
    return 0;
  }

  for (unsigned int i = 0; i < 0x92E; ++i) {
    if (ch == Valid_Korean[i]) {
      return 1;
    }
  }

  return 0;
}
