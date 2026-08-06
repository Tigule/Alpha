#pragma once

#include <DB/WowClientDB.h>

class SpellRadiusRec {
 public:
  SpellRadiusRec();
  ~SpellRadiusRec();

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

  int   m_ID;
  float m_radius;
  float m_radiusPerLevel;
  float m_radiusMax;
};

extern WowClientDB<SpellRadiusRec> g_spellRadiusDB;
