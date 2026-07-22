#pragma once

#include <DB/WowClientDB.h>

class WorldMapAreaRec {
 public:
  WorldMapAreaRec();
  ~WorldMapAreaRec();

  static const char *__fastcall GetFilename();

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

  int         m_ID;
  int         m_mapID;
  int         m_areaID;
  int         m_leftBoundary;
  int         m_rightBoundary;
  int         m_topBoundary;
  int         m_bottomBoundary;
  const char *m_areaName;
};

extern WowClientDB<WorldMapAreaRec> g_worldMapAreaDB;
