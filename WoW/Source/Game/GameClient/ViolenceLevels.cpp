#include <Console/ConsoleCommand.h>
#include <Console/ConsoleVar.h>
#include <DB/WowLocale.h>

static CVar *s_violenceLevel;
static int   s_maxViolenceLevels[8] = {2, 1, 1, 1, 1, 1, 1, 1};

void ViolenceLevelsShutdown() {
}

void ViolenceLevelsInitialize() {
  char buffer[12];

  SStrPrintf(buffer, sizeof(buffer), "%d", 2);
  s_violenceLevel = CVar::Register("violenceLevel", "Sets the violence level of the game", 0, buffer, 0, DEFAULT, false, 0);
}

int ViolenceGetLevel() {
  int level = s_violenceLevel->m_intValue;
  int maxLevel = s_maxViolenceLevels[CURRENT_LANGUAGE];
  if (level >= maxLevel) {
    level = maxLevel;
  }
  return level < 0 ? 0 : level;
}
