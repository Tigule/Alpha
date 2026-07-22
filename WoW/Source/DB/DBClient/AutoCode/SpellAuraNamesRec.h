#pragma once

#include <DB/WowClientDB.h>

class SpellAuraNamesRec {
 public:
  SpellAuraNamesRec();
  ~SpellAuraNamesRec();

  static const char *__fastcall GetFilename();

  static unsigned int GetNumColumns() {
    return 12;
  }

  static unsigned int GetRowSize() {
    return 48;
  }

  int GetID() {
    return m_generatedID;
  }

  bool NeedIDAssigned() {
    return true;
  }

  void SetID(int id) {
    m_generatedID = id;
  }

  bool Read(SFile *f, const char *stringBuffer);

  int         m_EnumID;
  int         m_specialMiscValue;
  const char *m_globalstrings_tag;
  const char *m_name_lang[8];
  int         m_name_flag;
  int         m_generatedID;
};

extern WowClientDB<SpellAuraNamesRec> g_spellAuraNamesDB;
