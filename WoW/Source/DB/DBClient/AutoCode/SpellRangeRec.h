#pragma once

#include <DB/WowClientDB.h>

class SpellRangeRec {
 public:
  SpellRangeRec();
  ~SpellRangeRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 22;
  }

  static UINT GetRowSize() {
    return 88;
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
  float  m_rangeMin;
  float  m_rangeMax;
  int    m_flags;
  LPCSTR m_displayName_lang[8];
  int    m_displayName_flag;
  LPCSTR m_displayNameShort_lang[8];
  int    m_displayNameShort_flag;
};

extern WowClientDB<SpellRangeRec> g_spellRangeDB;
