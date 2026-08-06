#pragma once

#include <DB/WowClientDB.h>

class AttackAnimTypesRec {
 public:
  AttackAnimTypesRec();
  ~AttackAnimTypesRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 2;
  }

  static UINT GetRowSize() {
    return 8;
  }

  int GetID() const {
    return m_AnimID;
  }

  bool NeedIDAssigned() {
    return false;
  }

  void SetID(int id) {
  }

  bool Read(SFile *f, LPCSTR stringBuffer);

  int    m_AnimID;
  LPCSTR m_AnimName;
};

extern WowClientDB<AttackAnimTypesRec> g_attackAnimTypesDB;
