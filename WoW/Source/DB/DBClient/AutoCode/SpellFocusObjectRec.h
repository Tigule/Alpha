#pragma once

#include <DB/WowClientDB.h>

class SpellFocusObjectRec {
 public:
  int    m_ID;
  LPCSTR m_name_lang[NUM_LOCALES];
  int    m_name_flag;

  SpellFocusObjectRec();
  ~SpellFocusObjectRec();

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
};

extern WowClientDB<SpellFocusObjectRec> g_spellFocusObjectDB;
