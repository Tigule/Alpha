#pragma once

#include <DB/WowClientDB.h>

class WorldMapContinentRec {
 public:
  WorldMapContinentRec();
  ~WorldMapContinentRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 8;
  }

  static UINT GetRowSize() {
    return 32;
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
  int   m_mapID;
  int   m_leftBoundary;
  int   m_rightBoundary;
  int   m_topBoundary;
  int   m_bottomBoundary;
  float m_continentOffsetX;
  float m_continentOffsetY;
};

extern WowClientDB<WorldMapContinentRec> g_worldMapContinentDB;
