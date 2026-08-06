#pragma once

#include <DB/WowClientDB.h>

class SkillLineRec {
 public:
  SkillLineRec();
  ~SkillLineRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 19;
  }

  static UINT GetRowSize() {
    return 76;
  }

  int GetID() const {
    return m_ID;
  }

  bool NeedIDAssigned() {
    return false;
  }

  void SetID(int id) {
  }

  bool Read(SFile *f, LPCSTR stringBuffer);

  int    m_ID;
  int    m_raceMask;
  int    m_classMask;
  int    m_excludeRace;
  int    m_excludeClass;
  int    m_categoryID;
  int    m_skillType;
  int    m_minCharLevel;
  int    m_maxRank;
  int    m_abandonable;
  LPCSTR m_displayName_lang[8];
  int    m_displayName_flag;
};

extern WowClientDB<SkillLineRec> g_skillLineDB;
