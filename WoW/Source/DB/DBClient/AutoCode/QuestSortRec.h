#pragma once

#include <DB/WowClientDB.h>

class QuestSortRec {
 public:
  QuestSortRec();
  ~QuestSortRec();

  static const char *__fastcall GetFilename();

  static unsigned int GetNumColumns() {
    return 10;
  }

  static unsigned int GetRowSize() {
    return 40;
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

  int         m_ID;
  const char *m_SortName_lang[NUM_LOCALES];
  int         m_SortName_flag;
};

extern WowClientDB<QuestSortRec> g_questSortDB;
