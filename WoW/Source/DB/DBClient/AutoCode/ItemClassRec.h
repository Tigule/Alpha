#pragma once

#include <DB/WowClientDB.h>

class ItemClassRec {
 public:
  ItemClassRec();
  ~ItemClassRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 12;
  }

  static unsigned int GetRowSize() {
    return 48;
  }

  int GetID() {
    return m_generatedID;
  }

  bool NeedIDAssigned() {
    return true;
  }

  void SetID(int id) {
    m_generatedID = id;
  }

  bool Read(SFile *f, const char *stringBuffer);

  int         m_classID;
  int         m_subclassMapID;
  int         m_flags;
  const char *m_className_lang[NUM_LOCALES];
  int         m_className_flag;
  int         m_generatedID;
};

extern WowClientDB<ItemClassRec> g_itemClassDB;
