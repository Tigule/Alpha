#pragma once

#include <DB/WowClientDB.h>

class LockTypeRec {
 public:
  LockTypeRec();
  ~LockTypeRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 28;
  }

  static UINT GetRowSize() {
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

  bool Read(SFile *f, LPCSTR stringBuffer);

  int    m_ID;
  LPCSTR m_name_lang[8];
  int    m_name_flag;
  LPCSTR m_resourceName_lang[8];
  int    m_resourceName_flag;
  LPCSTR m_verb_lang[8];
  int    m_verb_flag;
};

extern WowClientDB<LockTypeRec> g_lockTypeDB;
