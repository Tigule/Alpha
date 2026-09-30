#pragma once

#include <DB/WowClientDB.h>

class SpellCastTimesRec {
 public:
  int m_ID;
  int m_base;
  int m_perLevel;
  int m_minimum;

  SpellCastTimesRec();
  ~SpellCastTimesRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 4;
  }

  static UINT GetRowSize() {
    return 16;
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

extern WowClientDB<SpellCastTimesRec> g_spellCastTimesDB;
