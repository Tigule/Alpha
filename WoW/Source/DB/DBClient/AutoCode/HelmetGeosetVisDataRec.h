#pragma once

#include <DB/WowClientDB.h>

class HelmetGeosetVisDataRec {
 public:
  HelmetGeosetVisDataRec();
  ~HelmetGeosetVisDataRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 97;
  }

  static unsigned int GetRowSize() {
    return 388;
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

  int m_ID;
  int m_DefaultFlags[32];
  int m_PreferredFlags[32];
  int m_HideFlags[32];
};

extern WowClientDB<HelmetGeosetVisDataRec> g_helmetGeosetVisDataDB;
