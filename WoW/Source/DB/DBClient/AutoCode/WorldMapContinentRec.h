#pragma once

#include <DB/WowClientDB.h>

class WorldMapContinentRec {
 public:
  WorldMapContinentRec();
  ~WorldMapContinentRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 8;
  }

  static unsigned int GetRowSize() {
    return 32;
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
