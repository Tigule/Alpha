#pragma once

#include <DB/WowClientDB.h>

class ItemGroupSoundsRec {
 public:
  ItemGroupSoundsRec();
  ~ItemGroupSoundsRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 5;
  }

  static unsigned int GetRowSize() {
    return 20;
  }

  int GetID() {
    return m_ID;
  }

  bool NeedIDAssigned() {
    return false;
  }

  void SetID(int id) {
  }

  bool Read(SFile *f, const char *stringBuffer);

  int m_ID;
  int m_sound[4];
};

extern WowClientDB<ItemGroupSoundsRec> g_itemGroupSoundsDB;
