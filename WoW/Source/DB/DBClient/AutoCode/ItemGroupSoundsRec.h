#pragma once

#include <DB/WowClientDB.h>

class ItemGroupSoundsRec {
 public:
  int m_ID;
  int m_sound[4];

  ItemGroupSoundsRec();
  ~ItemGroupSoundsRec();

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

extern WowClientDB<ItemGroupSoundsRec> g_itemGroupSoundsDB;
