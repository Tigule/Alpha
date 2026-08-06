#pragma once

#include <DB/WowClientDB.h>

class WorldSafeLocsRec {
 public:
  WorldSafeLocsRec();
  ~WorldSafeLocsRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 14;
  }

  static UINT GetRowSize() {
    return 56;
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
  int    m_continent;
  float  m_locX;
  float  m_locY;
  float  m_locZ;
  LPCSTR m_AreaName_lang[8];
  int    m_AreaName_flag;
};

extern WowClientDB<WorldSafeLocsRec> g_worldSafeLocsDB;
