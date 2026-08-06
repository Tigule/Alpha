#pragma once

#include <DB/WowClientDB.h>

class SpellVisualPrecastTransitionsRec {
 public:
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

  int    m_ID;
  LPCSTR m_PrecastLoadAnimName;
  LPCSTR m_PrecastHoldAnimName;
};

extern WowClientDB<SpellVisualPrecastTransitionsRec> g_spellVisualPrecastTransitionsDB;
