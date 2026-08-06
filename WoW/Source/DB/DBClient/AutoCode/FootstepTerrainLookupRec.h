#pragma once

#include <DB/WowClientDB.h>

class FootstepTerrainLookupRec {
 public:
  FootstepTerrainLookupRec();
  ~FootstepTerrainLookupRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 5;
  }

  static UINT GetRowSize() {
    return 20;
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
  int m_CreatureFootstepID;
  int m_TerrainSoundID;
  int m_SoundID;
  int m_SoundIDSplash;
};

extern WowClientDB<FootstepTerrainLookupRec> g_footstepTerrainLookupDB;
