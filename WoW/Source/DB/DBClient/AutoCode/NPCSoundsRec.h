#pragma once

#include <DB/WowClientDB.h>

class NPCSoundsRec {
 public:
  int m_ID;
  int m_SoundID[4];

  NPCSoundsRec();
  ~NPCSoundsRec();

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
};

extern WowClientDB<NPCSoundsRec> g_nPCSoundsDB;
