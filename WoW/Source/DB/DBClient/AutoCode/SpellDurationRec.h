#pragma once

#include <DB/WowClientDB.h>

class SpellDurationRec {
 public:
  SpellDurationRec();
  ~SpellDurationRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 4;
  }

  static unsigned int GetRowSize() {
    return 16;
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

  int m_ID;
  int m_duration;
  int m_durationPerLevel;
  int m_maxDuration;
};

extern WowClientDB<SpellDurationRec> g_spellDurationDB;
