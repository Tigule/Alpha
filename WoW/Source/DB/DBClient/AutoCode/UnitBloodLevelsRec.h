#pragma once

#include <DB/WowClientDB.h>

class UnitBloodLevelsRec {
 public:
  int m_ID;
  int m_Violencelevel[3];

  UnitBloodLevelsRec();
  ~UnitBloodLevelsRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 4;
  }

  static UINT GetRowSize() {
    return 16;
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
};

extern WowClientDB<UnitBloodLevelsRec> g_unitBloodLevelsDB;
