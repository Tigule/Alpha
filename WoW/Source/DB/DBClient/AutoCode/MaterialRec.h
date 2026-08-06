#pragma once

#include <DB/WowClientDB.h>

class MaterialRec {
 public:
  MaterialRec();
  ~MaterialRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 3;
  }

  static UINT GetRowSize() {
    return 12;
  }

  int GetID() const {
    return m_materialID;
  }

  bool NeedIDAssigned() {
    return false;
  }

  void SetID(int id) {
  }

  bool Read(SFile *f, LPCSTR stringBuffer);

  int m_materialID;
  int m_flags;
  int m_foleySoundID;
};

extern WowClientDB<MaterialRec> g_materialDB;
