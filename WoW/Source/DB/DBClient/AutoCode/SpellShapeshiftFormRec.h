#pragma once

#include <DB/WowClientDB.h>

class SpellShapeshiftFormRec {
 public:
  SpellShapeshiftFormRec();
  ~SpellShapeshiftFormRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 12;
  }

  static UINT GetRowSize() {
    return 48;
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
  int    m_bonusActionBar;
  LPCSTR m_name_lang[NUM_LOCALES];
  int    m_name_flag;
  int    m_flags;
};

extern WowClientDB<SpellShapeshiftFormRec> g_spellShapeshiftFormDB;
