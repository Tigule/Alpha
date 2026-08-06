#pragma once

#include <DB/WowClientDB.h>

class UnitBloodRec {
 public:
  UnitBloodRec();
  ~UnitBloodRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 10;
  }

  static UINT GetRowSize() {
    return 40;
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
  int    m_CombatBloodSpurtFront[2];
  int    m_CombatBloodSpurtBack[2];
  LPCSTR m_GroundBlood[5];
};

extern WowClientDB<UnitBloodRec> g_unitBloodDB;
