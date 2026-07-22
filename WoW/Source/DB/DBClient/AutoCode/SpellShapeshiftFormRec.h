#pragma once

#include <DB/WowClientDB.h>

class SpellShapeshiftFormRec {
 public:
  SpellShapeshiftFormRec();
  ~SpellShapeshiftFormRec();

  static const char *__fastcall GetFilename();

  static unsigned int GetNumColumns() {
    return 12;
  }

  static unsigned int GetRowSize() {
    return 48;
  }

  int GetID() {
    return m_ID;
  }

  bool NeedIDAssigned() {
    return false;
  }

  void SetID(int id) {
  }

  bool Read(SFile *f, const char *stringBuffer);

  int         m_ID;
  int         m_bonusActionBar;
  const char *m_name_lang[NUM_LOCALES];
  int         m_name_flag;
  int         m_flags;
};

extern WowClientDB<SpellShapeshiftFormRec> g_spellShapeshiftFormDB;
