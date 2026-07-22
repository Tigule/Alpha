#pragma once

#include <DB/WowClientDB.h>

class SpellVisualAnimNameRec {
 public:
  SpellVisualAnimNameRec();
  ~SpellVisualAnimNameRec();

  static const char *__fastcall GetFilename();

  static unsigned int GetNumColumns() {
    return 2;
  }

  static unsigned int GetRowSize() {
    return 8;
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

  int         m_AnimID;
  const char *m_name;
  int         m_generatedID;
};

extern WowClientDB<SpellVisualAnimNameRec> g_spellVisualAnimNameDB;
