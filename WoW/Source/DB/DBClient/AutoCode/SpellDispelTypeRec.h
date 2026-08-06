#pragma once

#include <DB/WowClientDB.h>

class SpellDispelTypeRec {
 public:
  SpellDispelTypeRec();
  ~SpellDispelTypeRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 10;
  }

  static UINT GetRowSize() {
    return 40;
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
  LPCSTR m_name_lang[8];
  int    m_name_flag;
};

extern WowClientDB<SpellDispelTypeRec> g_spellDispelTypeDB;
