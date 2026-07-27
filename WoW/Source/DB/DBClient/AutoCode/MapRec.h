#pragma once

#include <DB/WowClientDB.h>

class MapRec {
 public:
  MapRec();
  ~MapRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 13;
  }

  static unsigned int GetRowSize() {
    return 52;
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
  const char *m_Directory;
  int         m_PVP;
  int         m_IsInMap;
  const char *m_MapName_lang[8];
  int         m_MapName_flag;
};

extern WowClientDB<MapRec> g_mapDB;
