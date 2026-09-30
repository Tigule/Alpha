#pragma once

#include <DB/WowClientDB.h>

class AttackAnimTypesRec {
 public:
  int    m_AnimID;
  LPCSTR m_AnimName;

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
};

extern WowClientDB<AttackAnimTypesRec> g_attackAnimTypesDB;
