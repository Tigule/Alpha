#pragma once

#include <DB/WowClientDB.h>

class SpellEffectNamesRec {
 public:
  SpellEffectNamesRec();
  ~SpellEffectNamesRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 10;
  }

  static unsigned int GetRowSize() {
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

  bool Read(SFile *f, const char *stringBuffer);

  int         m_EnumID;
  const char *m_name_lang[8];
  int         m_name_flag;
  int         m_generatedID;
};

extern WowClientDB<SpellEffectNamesRec> g_spellEffectNamesDB;
