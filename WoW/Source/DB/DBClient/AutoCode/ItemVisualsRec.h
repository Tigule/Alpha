#pragma once

#include <DB/WowClientDB.h>

class ItemVisualsRec {
 public:
  ItemVisualsRec();
  ~ItemVisualsRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 6;
  }

  static UINT GetRowSize() {
    return 24;
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

  int m_ID;
  int m_Slot[5];
};

extern WowClientDB<ItemVisualsRec> g_itemVisualsDB;
