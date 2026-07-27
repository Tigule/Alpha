#pragma once

#include <DB/WowClientDB.h>

class SpellFocusObjectRec {
 public:
  SpellFocusObjectRec();
  ~SpellFocusObjectRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 10;
  }

  static unsigned int GetRowSize() {
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

  bool Read(SFile *f, const char *stringBuffer);

  int         m_ID;
  const char *m_name_lang[NUM_LOCALES];
  int         m_name_flag;
};

extern WowClientDB<SpellFocusObjectRec> g_spellFocusObjectDB;
