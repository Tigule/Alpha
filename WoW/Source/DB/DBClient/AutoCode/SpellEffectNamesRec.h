#pragma once

#include <DB/WowClientDB.h>

class SpellEffectNamesRec {
 public:
  SpellEffectNamesRec();
  ~SpellEffectNamesRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 10;
  }

  static UINT GetRowSize() {
    return 40;
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
  LPCSTR m_name_lang[8];
  int    m_name_flag;
  int    m_generatedID;
};

extern WowClientDB<SpellEffectNamesRec> g_spellEffectNamesDB;
