#pragma once

#include <DB/WowClientDB.h>

class UnitBloodLevelsRec {
 public:
  UnitBloodLevelsRec();
  ~UnitBloodLevelsRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 4;
  }

  static unsigned int GetRowSize() {
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

  bool Read(SFile *f, const char *stringBuffer);

  int m_ID;
  int m_Violencelevel[3];
};

extern WowClientDB<UnitBloodLevelsRec> g_unitBloodLevelsDB;
