#pragma once

#include <DB/WowClientDB.h>

class WorldMapAreaRec {
 public:
  WorldMapAreaRec();
  ~WorldMapAreaRec();

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

  int    m_ID;
  int    m_mapID;
  int    m_areaID;
  int    m_leftBoundary;
  int    m_rightBoundary;
  int    m_topBoundary;
  int    m_bottomBoundary;
  LPCSTR m_areaName;
};

extern WowClientDB<WorldMapAreaRec> g_worldMapAreaDB;
