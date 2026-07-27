#pragma once

#include <DB/WowClientDB.h>

class EmotesTextDataRec {
 public:
  EmotesTextDataRec();
  ~EmotesTextDataRec();

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
  const char *m_text_lang[NUM_LOCALES];
  int         m_text_flag;
};

extern WowClientDB<EmotesTextDataRec> g_emotesTextDataDB;
