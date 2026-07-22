#pragma once

#include <DB/WowClientDB.h>

class EmotesTextRec {
 public:
  EmotesTextRec();
  ~EmotesTextRec();

  static const char *__fastcall GetFilename();

  static unsigned int GetNumColumns() {
    return 19;
  }

  static unsigned int GetRowSize() {
    return 76;
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
  const char *m_name;
  int         m_emoteID;
  int         m_emoteText[16];
};

extern WowClientDB<EmotesTextRec> g_emotesTextDB;
