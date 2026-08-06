#pragma once

#include <DB/WowClientDB.h>

class SpellVisualAnimNameRec {
 public:
  SpellVisualAnimNameRec();
  ~SpellVisualAnimNameRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 2;
  }

  static UINT GetRowSize() {
    return 8;
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

  int    m_AnimID;
  LPCSTR m_name;
  int    m_generatedID;
};

extern WowClientDB<SpellVisualAnimNameRec> g_spellVisualAnimNameDB;
