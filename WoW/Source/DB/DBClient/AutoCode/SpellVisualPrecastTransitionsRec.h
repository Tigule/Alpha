#pragma once

#include <DB/WowClientDB.h>

class SpellVisualPrecastTransitionsRec {
 public:
  int    m_ID;
  LPCSTR m_PrecastLoadAnimName;
  LPCSTR m_PrecastHoldAnimName;

  SpellVisualPrecastTransitionsRec();
  ~SpellVisualPrecastTransitionsRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 3;
  }

  static UINT GetRowSize() {
    return 12;
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

extern WowClientDB<SpellVisualPrecastTransitionsRec> g_spellVisualPrecastTransitionsDB;
