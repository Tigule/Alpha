#pragma once

#include <DB/WowClientDB.h>

class ItemVisualsRec {
 public:
  ItemVisualsRec();
  ~ItemVisualsRec();

  static const char *__fastcall GetFilename();

  static unsigned int GetNumColumns() {
    return 6;
  }

  static unsigned int GetRowSize() {
    return 24;
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
  int m_Slot[5];
};

extern WowClientDB<ItemVisualsRec> g_itemVisualsDB;
