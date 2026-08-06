#pragma once

#include <DB/WowClientDB.h>

class QuestInfoRec {
 public:
  QuestInfoRec();
  ~QuestInfoRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 10;
  }

  static UINT GetRowSize() {
    return 40;
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
  LPCSTR m_InfoName_lang[NUM_LOCALES];
  int    m_InfoName_flag;
};

extern WowClientDB<QuestInfoRec> g_questInfoDB;
