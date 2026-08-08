#include <Console/ConsoleCommand.h>
#include <Console/ConsoleVar.h>
#include <DB/WowLocale.h>

static CVar *s_violenceLevel;
enum VIOLENCELEVELS {
  VIOLENCELEVEL_NONE = 0,
  VIOLENCELEVEL_MEDIUM = 1,
  VIOLENCELEVEL_HIGH = 2,
  VIOLENCELEVEL_NUMVIOLENCELEVELS = 3
};

static VIOLENCELEVELS s_maxViolenceLevels[8] = {VIOLENCELEVEL_HIGH,   VIOLENCELEVEL_MEDIUM, VIOLENCELEVEL_MEDIUM, VIOLENCELEVEL_MEDIUM,
                                                VIOLENCELEVEL_MEDIUM, VIOLENCELEVEL_MEDIUM, VIOLENCELEVEL_MEDIUM, VIOLENCELEVEL_MEDIUM};

void ViolenceLevelsShutdown() {
}

void ViolenceLevelsInitialize() {
  char buffer[12];

  SStrPrintf(buffer, sizeof(buffer), "%d", 2);
  s_violenceLevel = CVar::Register("violenceLevel", "Sets the violence level of the game", 0, buffer, 0, DEFAULT, false, 0);
}

int ViolenceGetLevel() {
  int level = s_violenceLevel->GetInt();
  int maxLevel = s_maxViolenceLevels[CURRENT_LANGUAGE];
  if (level >= maxLevel) {
    level = maxLevel;
  }
  return level < 0 ? 0 : level;
}
