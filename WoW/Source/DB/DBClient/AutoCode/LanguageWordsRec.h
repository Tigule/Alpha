#pragma once

#include <DB/WowClientDB.h>

class LanguageWordsRec {
 public:
  int    m_ID;
  int    m_languageID;
  LPCSTR m_word;

  LanguageWordsRec();
  ~LanguageWordsRec();

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
};

extern WowClientDB<LanguageWordsRec> g_languageWordsDB;
