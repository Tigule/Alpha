#pragma once

#include <DB/WowClientDB.h>

class NamesProfanityRec {
 public:
  int    m_ID;
  LPCSTR m_Name;

  NamesProfanityRec();
  ~NamesProfanityRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 2;
  }

  static UINT GetRowSize() {
    return 8;
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
};

extern WowClientDB<NamesProfanityRec> g_namesProfanityDB;
