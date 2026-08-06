#pragma once

#include <DB/WowClientDB.h>

class DeathThudLookupsRec {
 public:
  DeathThudLookupsRec();
  ~DeathThudLookupsRec();

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
  int m_SizeClass;
  int m_TerrainTypeSoundID;
  int m_SoundEntryID;
  int m_SoundEntryIDWater;
};

extern WowClientDB<DeathThudLookupsRec> g_deathThudLookupsDB;
