#pragma once

#include <DB/WowClientDB.h>

class LockRec {
 public:
  LockRec();
  ~LockRec();

  static const char *__fastcall GetFilename();

  static unsigned int GetNumColumns() {
    return 17;
  }

  static unsigned int GetRowSize() {
    return 68;
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
  int m_Type[4];
  int m_Index[4];
  int m_Skill[4];
  int m_Action[4];
};

extern WowClientDB<LockRec> g_lockDB;
