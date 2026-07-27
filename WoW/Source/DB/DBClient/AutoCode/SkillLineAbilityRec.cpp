#include "SkillLineAbilityRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *SkillLineAbilityRec::GetFilename() {
  return "DBFilesClient\\SkillLineAbility.dbc";
}

SkillLineAbilityRec::SkillLineAbilityRec() {
}

SkillLineAbilityRec::~SkillLineAbilityRec() {
}

bool SkillLineAbilityRec::Read(SFile *f, const char *stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_skillLine, sizeof(m_skillLine), 0, 0, 0) && result;
  result = SFile::Read(f, &m_spell, sizeof(m_spell), 0, 0, 0) && result;
  result = SFile::Read(f, &m_raceMask, sizeof(m_raceMask), 0, 0, 0) && result;
  result = SFile::Read(f, &m_classMask, sizeof(m_classMask), 0, 0, 0) && result;
  result = SFile::Read(f, &m_excludeRace, sizeof(m_excludeRace), 0, 0, 0) && result;
  result = SFile::Read(f, &m_excludeClass, sizeof(m_excludeClass), 0, 0, 0) && result;
  result = SFile::Read(f, &m_minSkillLineRank, sizeof(m_minSkillLineRank), 0, 0, 0) && result;
  result = SFile::Read(f, &m_supercededBySpell, sizeof(m_supercededBySpell), 0, 0, 0) && result;
  result = SFile::Read(f, &m_trivialSkillLineRankHigh, sizeof(m_trivialSkillLineRankHigh), 0, 0, 0) && result;
  result = SFile::Read(f, &m_trivialSkillLineRankLow, sizeof(m_trivialSkillLineRankLow), 0, 0, 0) && result;
  result = SFile::Read(f, &m_abandonable, sizeof(m_abandonable), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading SkillLineAbilityRec", DEFAULT_COLOR);
  }

  return result;
}
