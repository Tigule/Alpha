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

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_skillLine) ||
      !SFileReadTyped(f, &m_spell) ||
      !SFileReadTyped(f, &m_raceMask) ||
      !SFileReadTyped(f, &m_classMask) ||
      !SFileReadTyped(f, &m_excludeRace) ||
      !SFileReadTyped(f, &m_excludeClass) ||
      !SFileReadTyped(f, &m_minSkillLineRank) ||
      !SFileReadTyped(f, &m_supercededBySpell) ||
      !SFileReadTyped(f, &m_trivialSkillLineRankHigh) ||
      !SFileReadTyped(f, &m_trivialSkillLineRankLow) ||
      !SFileReadTyped(f, &m_abandonable)) {
    ConsoleWrite("Error reading SkillLineAbilityRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
