#pragma once

#include <DB/WowClientDB.h>

class FootstepTerrainLookupRec {
 public:
  FootstepTerrainLookupRec();
  ~FootstepTerrainLookupRec();

  static const char *__fastcall GetFilename();

  static unsigned int GetNumColumns() {
    return 5;
  }

  static unsigned int GetRowSize() {
    return 20;
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
  int m_CreatureFootstepID;
  int m_TerrainSoundID;
  int m_SoundID;
  int m_SoundIDSplash;
};

extern WowClientDB<FootstepTerrainLookupRec> g_footstepTerrainLookupDB;
