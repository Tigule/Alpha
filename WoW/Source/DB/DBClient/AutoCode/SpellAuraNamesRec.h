#pragma once

#include <DB/WowClientDB.h>

class SpellAuraNamesRec {
 public:
  SpellAuraNamesRec();
  ~SpellAuraNamesRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 12;
  }

  static UINT GetRowSize() {
    return 48;
  }

  int GetID() const {
    return m_generatedID;
  }

  bool NeedIDAssigned() {
    return true;
  }

  void SetID(int id) {
    m_generatedID = id;
  }

  bool Read(SFile *f, LPCSTR stringBuffer);

  int    m_EnumID;
  int    m_specialMiscValue;
  LPCSTR m_globalstrings_tag;
  LPCSTR m_name_lang[8];
  int    m_name_flag;
  int    m_generatedID;
};

extern WowClientDB<SpellAuraNamesRec> g_spellAuraNamesDB;
