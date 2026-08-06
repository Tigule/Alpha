#pragma once

#include <DB/WowClientDB.h>

class TabardBackgroundTexturesRec {
 public:
  TabardBackgroundTexturesRec();
  ~TabardBackgroundTexturesRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 3;
  }

  static UINT GetRowSize() {
    return 12;
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
  LPCSTR m_TorsoTexture[2];
};

extern WowClientDB<TabardBackgroundTexturesRec> g_tabardBackgroundTexturesDB;
