#pragma once

#include <DB/WowClientDB.h>

class AttackAnimKitsRec {
 public:
  AttackAnimKitsRec();
  ~AttackAnimKitsRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 5;
  }

  static UINT GetRowSize() {
    return 20;
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
  int m_ItemSubclassID;
  int m_AnimTypeID;
  int m_AnimFrequency;
  int m_WhichHand;
};

extern WowClientDB<AttackAnimKitsRec> g_attackAnimKitsDB;
