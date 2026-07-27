#pragma once

#include <DB/WowClientDB.h>

class SkillLineAbilityRec {
 public:
  SkillLineAbilityRec();
  ~SkillLineAbilityRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 12;
  }

  static unsigned int GetRowSize() {
    return 48;
  }

  int GetID() {
    return m_ID;
  }

  bool NeedIDAssigned() {
    return false;
  }

  void SetID(int id) {
  }

  bool Read(SFile *f, const char *stringBuffer);

  int m_ID;
  int m_skillLine;
  int m_spell;
  int m_raceMask;
  int m_classMask;
  int m_excludeRace;
  int m_excludeClass;
  int m_minSkillLineRank;
  int m_supercededBySpell;
  int m_trivialSkillLineRankHigh;
  int m_trivialSkillLineRankLow;
  int m_abandonable;
};

extern WowClientDB<SkillLineAbilityRec> g_skillLineAbilityDB;
