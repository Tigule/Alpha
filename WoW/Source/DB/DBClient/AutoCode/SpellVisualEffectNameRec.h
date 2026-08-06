#pragma once

#include <DB/WowClientDB.h>

class SpellVisualEffectNameRec {
 public:
  SpellVisualEffectNameRec();
  ~SpellVisualEffectNameRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 6;
  }

  static UINT GetRowSize() {
    return 24;
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
  LPCSTR m_fileName;
  int    m_specialID;
  int    m_specialAttachPoint;
  float  m_areaEffectSize;
  int    m_VisualEffectNameFlags;
};

extern WowClientDB<SpellVisualEffectNameRec> g_spellVisualEffectNameDB;
