#pragma once

#include <DB/WowClientDB.h>

class LockRec {
 public:
  LockRec();
  ~LockRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 17;
  }

  static UINT GetRowSize() {
    return 68;
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
  int m_Type[4];
  int m_Index[4];
  int m_Skill[4];
  int m_Action[4];
};

extern WowClientDB<LockRec> g_lockDB;
