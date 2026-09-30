#pragma once

#include <DB/WowClientDB.h>

class NamesReservedRec {
 public:
  int    m_ID;
  LPCSTR m_Name;

  NamesReservedRec();
  ~NamesReservedRec();

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

extern WowClientDB<NamesReservedRec> g_namesReservedDB;
