#pragma once

#include <DB/WowClientDB.h>

class SpellRangeRec {
 public:
  SpellRangeRec();
  ~SpellRangeRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 22;
  }

  static unsigned int GetRowSize() {
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

  bool Read(SFile *f, const char *stringBuffer);

  int         m_ID;
  float       m_rangeMin;
  float       m_rangeMax;
  int         m_flags;
  const char *m_displayName_lang[8];
  int         m_displayName_flag;
  const char *m_displayNameShort_lang[8];
  int         m_displayNameShort_flag;
};

extern WowClientDB<SpellRangeRec> g_spellRangeDB;
