#include <Base/Base.h>
#include <WowConst.h>
#include <MapDefs.h>

#include <storm.h>

#include "DB/DBClient/AutoCode/ChrClassesRec.h"
#include "DB/DBClient/AutoCode/ChrRacesRec.h"
#include "DB/DBClient/AutoCode/SpellDurationRec.h"
#include "DB/DBClient/AutoCode/SpellRadiusRec.h"
#include "DB/DBClient/AutoCode/SpellRangeRec.h"
#include "DB/DBClient/AutoCode/SpellRec.h"
#include "DB/DBClient/DBCacheInstances.h"
#include "Object/ObjectClient/Unit_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"

#include <FrameScript/FrameScript.h>

static const char *token;
static int         s_lastNumber;

int Spell_C_GetSpellLevel(int id, int isPet);
int Spell_C_GetManaCost(int id, int isPet);
int Spell_C_GetManaCostPerSecond(int id, int isPet);
void Spell_C_GetMinMaxPoints(const SpellRec *srec, int effectIndex, int *min, int *max, unsigned int level, int isPet);

bool QuestParserParseText(const char *text, char *buf, unsigned int size, const unsigned __int64 &target, int restoreToken);

bool QuestParserGenderConditional(char *buf, unsigned int size, const unsigned __int64 &target, const NameCache *nc) {
  char        temp[1024];
  const char *semi;
  CGUnit_C   *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(target, __FILE__, __LINE__));

  if (!unit && !nc) {
    return false;
  }
  if (unit && !(unit->GetType() & TYPE_PLAYER)) {
    nc = 0;
  }

  while (*token == ' ') {
    ++token;
  }
  if (!*token) {
    return true;
  }

  semi = SStrChr(token, ':');
  if (!semi) {
    return true;
  }

  if (unit ? unit->GetUnitData()->sex : nc->m_sex) {
    token = semi + 1;
    while (*token == ' ') {
      ++token;
    }
  }

  semi = SStrChr(semi, ';');
  if (!semi) {
    return true;
  }

  if (semi != token) {
    unsigned int length = SStrLen(buf);
    SStrPack(buf, token, size);
    buf[length + semi - token] = 0;
    while (length < SStrLen(buf) && buf[SStrLen(buf) - 1] == ' ') {
      buf[SStrLen(buf) - 1] = 0;
    }
  }

  token = semi + 1;
  SStrCopy(temp, buf, sizeof(temp));
  QuestParserParseText(temp, buf, size, target, 1);
  return true;
}

bool QuestParserReplaceText(char *buf, unsigned int size, const unsigned __int64 &target, const NameCache *nc) {
  char      race[32];
  char      classStr[32];
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(target, __FILE__, __LINE__));

  if (!unit && !nc) {
    return false;
  }
  if (unit && !(unit->GetType() & TYPE_PLAYER)) {
    nc = 0;
  }

  switch (*token) {
    case 'B':
    case 'b':
      SStrPack(buf, "\n", size);
      break;

    case 'C':
    case 'c': {
      unsigned int   classID = unit ? unit->GetUnitData()->classId : nc->m_race;
      const ChrClassesRec *classRec = g_chrClassesDB.GetRecord(classID);
      SStrCopy(classStr, classRec->m_name_lang[CURRENT_LANGUAGE], sizeof(classStr));
      if (*token == 'c') {
        SStrLower(classStr);
      }
      SStrPack(buf, classStr, size);
      break;
    }

    case 'G':
    case 'g':
      if (*token) {
        ++token;
      }
      return QuestParserGenderConditional(buf, size, target, nc);

    case 'N':
    case 'n':
      SStrPack(buf, unit ? unit->GetUnitName() : nc->m_name, size);
      break;

    case 'R':
    case 'r':
      if (!(unit->GetType() & TYPE_PLAYER)) {
        SStrPack(buf, unit->GetUnitName(), size);
      } else {
        const ChrRacesRec *raceRec = g_chrRacesDB.GetRecord(unit->GetUnitData()->race);
        SStrCopy(race, raceRec->m_name_lang[CURRENT_LANGUAGE], sizeof(race));
        if (*token == 'r') {
          SStrLower(race);
        }
        SStrPack(buf, race, size);
      }
      break;

    default:
      return false;
  }

  if (*token) {
    ++token;
  }
  return true;
}

bool QuestParserParseText(const char *text, char *buf, unsigned int size, const unsigned __int64 &target, int restoreToken) {
  const char      *oldToken;
  const NameCache *nc;
  unsigned int     length;
  unsigned int     oldLen;
  unsigned int     error;

  FATALASSERT(text);
  FATALASSERT(buf);

  oldToken = token;
  error = 0;
  nc = g_nameDBCache.GetRecord(target, target, 0, 0);
  buf[0] = 0;
  token = SStrChr(text, '$');

  while (token && *token) {
    length = token - text;
    if (length) {
      oldLen = SStrLen(buf);
      SStrPack(buf, text, size);
      buf[oldLen + length] = 0;
    }

    ++token;
    if (!QuestParserReplaceText(buf, size, target, nc)) {
      SStrPack(buf, "$", size);
      error = 1;
    }

    text = token;
    token = SStrChr(token, '$');
  }

  SStrPack(buf, text, size);
  if (restoreToken) {
    token = oldToken;
  }
  return error == 0;
}

bool SpellParserGenderConditional(char *buf, unsigned int size) {
  CGUnit_C *player = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return false;
  }
  while (*token == ' ') {
    ++token;
  }
  if (!*token) {
    return true;
  }
  const char *semi = SStrChr(token, ':');
  if (!semi) {
    return true;
  }
  const char *end = SStrChr(semi, ';');
  if (!end) {
    return true;
  }
  const char *text = token;
  const char *stop = semi;
  if (player->GetUnitData()->sex) {
    text = semi + 1;
    while (*text == ' ') {
      ++text;
    }
    stop = end;
  }
  if (stop != text) {
    unsigned int oldLen = SStrLen(buf);
    SStrPack(buf, text, size);
    buf[oldLen + stop - text] = 0;
    while (oldLen < SStrLen(buf) && buf[SStrLen(buf) - 1] == ' ') {
      buf[SStrLen(buf) - 1] = 0;
    }
  }
  token = end + 1;
  return true;
}

bool SpellParserPluralConditional(char *buf, unsigned int size, int ordinal) {
  while (*token == ' ') {
    ++token;
  }
  if (!*token) {
    return true;
  }
  const char *semi = SStrChr(token, ':');
  if (!semi) {
    return true;
  }
  const char *end = SStrChr(semi, ';');
  if (!end) {
    return true;
  }
  const char *text = token;
  const char *stop = semi;
  if (FrameScript_GetPluralIndex(ordinal)) {
    text = semi + 1;
    while (*text == ' ') {
      ++text;
    }
    stop = end;
  }
  if (stop != text) {
    unsigned int oldLen = SStrLen(buf);
    SStrPack(buf, text, size);
    buf[oldLen + stop - text] = 0;
    while (oldLen < SStrLen(buf) && buf[SStrLen(buf) - 1] == ' ') {
      buf[SStrLen(buf) - 1] = 0;
    }
  }
  token = end + 1;
  return true;
}

int SpellParserReplaceText(char *buf, unsigned int size, const SpellRec *spell, int level, int isPet) {
  if (!spell) {
    return 0;
  }
  unsigned int effect = 0;
  int          indexed = token[1] >= '1' && token[1] <= '9';
  if (indexed) {
    effect = token[1] - '1';
    if (effect >= 3) {
      effect = 0;
    }
  }
  char string[32];
  switch (*token) {
    case 'A':
    case 'a': {
      const SpellRadiusRec *radius = g_spellRadiusDB.GetRecord(spell->m_effectRadiusIndex[effect]);
      SStrPrintf(string, sizeof(string), "%d", radius ? static_cast<int>(radius->m_radius) : 0);
      SStrPack(buf, string, size);
      break;
    }
    case 'C':
    case 'c':
      SStrPrintf(string, sizeof(string), "%d", Spell_C_GetManaCost(spell->m_ID, isPet));
      SStrPack(buf, string, size);
      break;
    case 'D':
    case 'd': {
      const SpellDurationRec *duration = g_spellDurationDB.GetRecord(spell->m_durationIndex);
      if (duration) {
        int milliseconds = duration->m_duration + level * duration->m_durationPerLevel;
        if (milliseconds >= duration->m_maxDuration) {
          milliseconds = duration->m_maxDuration;
        }
        if (milliseconds <= 0) {
          SStrPack(buf, FrameScript_GetText("SPELL_DURATION_UNTIL_CANCELLED", -1, GENDER_NOT_APPLICABLE), size);
        } else {
          char format[64];
          char formatted[64];
          SStrCopy(
              format,
              FrameScript_GetText(milliseconds < 60000 ? "SPELL_DURATION_SEC" : "SPELL_DURATION_MIN", -1, GENDER_NOT_APPLICABLE),
              sizeof(format)
          );
          SStrPrintf(formatted, sizeof(formatted), format, milliseconds / (milliseconds >= 60000 ? 60000 : 1000));
          SStrPack(buf, formatted, size);
        }
      }
      break;
    }
    case 'G':
    case 'g':
      ++token;
      return SpellParserGenderConditional(buf, size);
    case 'L':
    case 'l':
      ++token;
      return SpellParserPluralConditional(buf, size, s_lastNumber);
    case 'M':
    case 'O':
    case 'S':
    case 'm':
    case 'o':
    case 's': {
      int min;
      int max;
      Spell_C_GetMinMaxPoints(spell, effect, &min, &max, level, isPet);

      if (*token == 'O' || *token == 'o') {
        int period = spell->m_effectAuraPeriod[effect];
        if (!period) {
          period = 5000;
        }
        if (period > 0) {
          const SpellDurationRec *duration = g_spellDurationDB.GetRecord(spell->m_durationIndex);
          if (duration) {
            int milliseconds = duration->m_duration + level * duration->m_durationPerLevel;
            if (milliseconds >= duration->m_maxDuration) {
              milliseconds = duration->m_maxDuration;
            }
            if (milliseconds > 0) {
              min = min * milliseconds / period;
              max = max * milliseconds / period;
            } else {
              min = 0;
              max = 0;
            }
          }
        } else {
          min = 0;
          max = 0;
        }
      }

      min = abs(min);
      max = abs(max);
      s_lastNumber = *token == 'M' ? max : min;
      if (*token == 'm') {
        SStrPrintf(string, sizeof(string), "%d", min);
      } else if (*token == 'M') {
        SStrPrintf(string, sizeof(string), "%d", max);
      } else if (min == max) {
        SStrPrintf(string, sizeof(string), "%d", min);
      } else {
        SStrPrintf(
            string,
            sizeof(string),
            FrameScript_GetText("SPELL_POINTS_SPREAD_TEMPLATE", -1, GENDER_NOT_APPLICABLE),
            min,
            max
        );
      }
      SStrPack(buf, string, size);
      break;
    }
    case 'P':
    case 'p':
      SStrPrintf(string, sizeof(string), "%d", Spell_C_GetManaCostPerSecond(spell->m_ID, isPet));
      SStrPack(buf, string, size);
      break;
    case 'R':
    case 'r': {
      const SpellRangeRec *range = g_spellRangeDB.GetRecord(spell->m_rangeIndex > 1 ? spell->m_rangeIndex : 1);
      if (range) {
        SStrPack(buf, range->m_displayName_lang[CURRENT_LANGUAGE], size);
      }
      break;
    }
    case 'T':
    case 't':
      SStrPrintf(string, sizeof(string), "%d", ((spell->m_procFlags & 8) ? 5000 : spell->m_effectAuraPeriod[effect]) / 1000);
      SStrPack(buf, string, size);
      break;
    case 'X':
    case 'x':
      s_lastNumber = spell->m_effectChainTargets[effect];
      SStrPrintf(string, sizeof(string), "%d", s_lastNumber);
      SStrPack(buf, string, size);
      break;
    default:
      return 0;
  }
  if (*token) {
    ++token;
    if (indexed) {
      ++token;
    }
  }
  return 1;
}

int SpellParserParseText(const SpellRec *spell, char *buf, unsigned int size, int isPet) {
  FATALASSERT(spell);
  FATALASSERT(buf);
  buf[0] = 0;
  const char *text = spell->m_description_lang[CURRENT_LANGUAGE];
  token = SStrChr(text, '$');
  int level = Spell_C_GetSpellLevel(spell->m_ID, isPet);
  int error = 0;
  while (token && *token) {
    unsigned int length = token - text;
    if (length) {
      unsigned int oldLen = SStrLen(buf);
      SStrPack(buf, text, size);
      buf[oldLen + length] = 0;
    }
    ++token;
    if (!SpellParserReplaceText(buf, size, spell, level, isPet)) {
      SStrPack(buf, "$", size);
      error = 1;
    }
    text = token;
    token = SStrChr(token, '$');
  }
  SStrPack(buf, text, size);
  return error == 0;
}
