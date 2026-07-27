#pragma once

#include <DB/WowClientDB.h>

class WorldSafeLocsRec {
 public:
  WorldSafeLocsRec();
  ~WorldSafeLocsRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 14;
  }

  static unsigned int GetRowSize() {
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

  bool Read(SFile *f, const char *stringBuffer);

  int         m_ID;
  int         m_continent;
  float       m_locX;
  float       m_locY;
  float       m_locZ;
  const char *m_AreaName_lang[8];
  int         m_AreaName_flag;
};

extern WowClientDB<WorldSafeLocsRec> g_worldSafeLocsDB;
