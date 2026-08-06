#pragma once

#include <DB/WowClientDB.h>

class HelmetGeosetVisDataRec {
 public:
  HelmetGeosetVisDataRec();
  ~HelmetGeosetVisDataRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 97;
  }

  static UINT GetRowSize() {
    return 388;
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
  int m_DefaultFlags[32];
  int m_PreferredFlags[32];
  int m_HideFlags[32];
};

extern WowClientDB<HelmetGeosetVisDataRec> g_helmetGeosetVisDataDB;
