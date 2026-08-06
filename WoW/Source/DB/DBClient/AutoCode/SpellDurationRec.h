#pragma once

#include <DB/WowClientDB.h>

class SpellDurationRec {
 public:
  SpellDurationRec();
  ~SpellDurationRec();

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

  int m_ID;
  int m_duration;
  int m_durationPerLevel;
  int m_maxDuration;
};

extern WowClientDB<SpellDurationRec> g_spellDurationDB;
