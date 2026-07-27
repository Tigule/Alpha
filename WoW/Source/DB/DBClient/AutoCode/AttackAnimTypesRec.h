#pragma once

#include <DB/WowClientDB.h>

class AttackAnimTypesRec {
 public:
  AttackAnimTypesRec();
  ~AttackAnimTypesRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 2;
  }

  static unsigned int GetRowSize() {
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

  bool Read(SFile *f, const char *stringBuffer);

  int         m_AnimID;
  const char *m_AnimName;
};

extern WowClientDB<AttackAnimTypesRec> g_attackAnimTypesDB;
