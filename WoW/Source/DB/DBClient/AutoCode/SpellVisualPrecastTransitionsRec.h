#pragma once

#include <DB/WowClientDB.h>

class SpellVisualPrecastTransitionsRec {
 public:
  SpellVisualPrecastTransitionsRec();
  ~SpellVisualPrecastTransitionsRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 3;
  }

  static unsigned int GetRowSize() {
    return 12;
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
  const char *m_PrecastLoadAnimName;
  const char *m_PrecastHoldAnimName;
};

extern WowClientDB<SpellVisualPrecastTransitionsRec> g_spellVisualPrecastTransitionsDB;
