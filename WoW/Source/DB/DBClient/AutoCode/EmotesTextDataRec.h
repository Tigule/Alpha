#pragma once

#include <DB/WowClientDB.h>

class EmotesTextDataRec {
 public:
  EmotesTextDataRec();
  ~EmotesTextDataRec();

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
  LPCSTR m_text_lang[NUM_LOCALES];
  int    m_text_flag;
};

extern WowClientDB<EmotesTextDataRec> g_emotesTextDataDB;
