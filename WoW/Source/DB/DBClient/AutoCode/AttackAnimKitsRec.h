#pragma once

#include <DB/WowClientDB.h>

class AttackAnimKitsRec {
 public:
  AttackAnimKitsRec();
  ~AttackAnimKitsRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 5;
  }

  static unsigned int GetRowSize() {
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

  bool Read(SFile *f, const char *stringBuffer);

  int m_ID;
  int m_ItemSubclassID;
  int m_AnimTypeID;
  int m_AnimFrequency;
  int m_WhichHand;
};

extern WowClientDB<AttackAnimKitsRec> g_attackAnimKitsDB;
