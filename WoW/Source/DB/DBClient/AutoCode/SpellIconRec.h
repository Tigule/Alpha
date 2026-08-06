#pragma once

#include <DB/WowClientDB.h>

class SpellIconRec {
 public:
  SpellIconRec();
  ~SpellIconRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 2;
  }

  static UINT GetRowSize() {
    return 8;
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
  LPCSTR m_textureFilename;
};

extern WowClientDB<SpellIconRec> g_spellIconDB;
