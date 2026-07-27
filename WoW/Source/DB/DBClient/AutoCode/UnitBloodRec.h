#pragma once

#include <DB/WowClientDB.h>

class UnitBloodRec {
 public:
  UnitBloodRec();
  ~UnitBloodRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 10;
  }

  static unsigned int GetRowSize() {
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

  bool Read(SFile *f, const char *stringBuffer);

  int         m_ID;
  int         m_CombatBloodSpurtFront[2];
  int         m_CombatBloodSpurtBack[2];
  const char *m_GroundBlood[5];
};

extern WowClientDB<UnitBloodRec> g_unitBloodDB;
