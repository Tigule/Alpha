#pragma once

#include <DB/WowClientDB.h>

class SkillLineRec {
 public:
  SkillLineRec();
  ~SkillLineRec();

  static const char *__fastcall GetFilename();

  static unsigned int GetNumColumns() {
    return 19;
  }

  static unsigned int GetRowSize() {
    return 76;
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

  int         m_ID;
  int         m_raceMask;
  int         m_classMask;
  int         m_excludeRace;
  int         m_excludeClass;
  int         m_categoryID;
  int         m_skillType;
  int         m_minCharLevel;
  int         m_maxRank;
  int         m_abandonable;
  const char *m_displayName_lang[8];
  int         m_displayName_flag;
};

extern WowClientDB<SkillLineRec> g_skillLineDB;
