#pragma once

#include <DB/WowClientDB.h>

class LanguagesRec {
 public:
  LanguagesRec();
  ~LanguagesRec();

  static const char *GetFilename();

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
  const char *m_name_lang[8];
  int         m_name_flag;
};

extern WowClientDB<LanguagesRec> g_languagesDB;
