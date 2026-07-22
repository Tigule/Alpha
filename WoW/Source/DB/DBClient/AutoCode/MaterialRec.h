#pragma once

#include <DB/WowClientDB.h>

class MaterialRec {
 public:
  MaterialRec();
  ~MaterialRec();

  static const char *__fastcall GetFilename();

  static unsigned int GetNumColumns() {
    return 3;
  }

  static unsigned int GetRowSize() {
    return 12;
  }

  int GetID() {
    return m_materialID;
  }

  bool NeedIDAssigned() {
    return false;
  }

  void SetID(int id) {
  }

  bool Read(SFile *f, const char *stringBuffer);

  int m_materialID;
  int m_flags;
  int m_foleySoundID;
};

extern WowClientDB<MaterialRec> g_materialDB;
