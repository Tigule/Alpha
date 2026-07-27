#pragma once

#include <DB/WowClientDB.h>

class LockTypeRec {
 public:
  LockTypeRec();
  ~LockTypeRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 28;
  }

  static unsigned int GetRowSize() {
    return 112;
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
  const char *m_name_lang[8];
  int         m_name_flag;
  const char *m_resourceName_lang[8];
  int         m_resourceName_flag;
  const char *m_verb_lang[8];
  int         m_verb_flag;
};

extern WowClientDB<LockTypeRec> g_lockTypeDB;
