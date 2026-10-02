#include "SkillLineAbilityRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR SkillLineAbilityRec::GetFilename() {
  return "DBFilesClient\\SkillLineAbility.dbc";
}

SkillLineAbilityRec::SkillLineAbilityRec() {
}

SkillLineAbilityRec::~SkillLineAbilityRec() {
}

bool SkillLineAbilityRec::Read(SFile *f, LPCSTR stringBuffer) {
  int error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_skillLine) == 0);
  error |= (SFileReadTyped(f, &m_spell) == 0);
  error |= (SFileReadTyped(f, &m_raceMask) == 0);
  error |= (SFileReadTyped(f, &m_classMask) == 0);
  error |= (SFileReadTyped(f, &m_excludeRace) == 0);
  error |= (SFileReadTyped(f, &m_excludeClass) == 0);
  error |= (SFileReadTyped(f, &m_minSkillLineRank) == 0);
  error |= (SFileReadTyped(f, &m_supercededBySpell) == 0);
  error |= (SFileReadTyped(f, &m_trivialSkillLineRankHigh) == 0);
  error |= (SFileReadTyped(f, &m_trivialSkillLineRankLow) == 0);
  error |= (SFileReadTyped(f, &m_abandonable) == 0);

  if (error) {
    ConsoleWrite("Error reading SkillLineAbilityRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
