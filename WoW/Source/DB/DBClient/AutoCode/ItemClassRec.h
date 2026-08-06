#pragma once

#include <DB/WowClientDB.h>

class ItemClassRec {
 public:
  ItemClassRec();
  ~ItemClassRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 12;
  }

  static UINT GetRowSize() {
    return 48;
  }

  int GetID() const {
    return m_generatedID;
  }

  bool NeedIDAssigned() {
    return true;
  }

  void SetID(int id) {
    m_generatedID = id;
  }

  bool Read(SFile *f, LPCSTR stringBuffer);

  int    m_classID;
  int    m_subclassMapID;
  int    m_flags;
  LPCSTR m_className_lang[NUM_LOCALES];
  int    m_className_flag;
  int    m_generatedID;
};

extern WowClientDB<ItemClassRec> g_itemClassDB;
