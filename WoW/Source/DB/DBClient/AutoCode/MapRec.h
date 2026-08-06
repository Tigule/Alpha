#pragma once

#include <DB/WowClientDB.h>

class MapRec {
 public:
  MapRec();
  ~MapRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 13;
  }

  static UINT GetRowSize() {
    return 52;
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
  LPCSTR m_Directory;
  int    m_PVP;
  int    m_IsInMap;
  LPCSTR m_MapName_lang[8];
  int    m_MapName_flag;
};

extern WowClientDB<MapRec> g_mapDB;
