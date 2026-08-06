#pragma once

#include <DB/WowClientDB.h>

class EmotesTextRec {
 public:
  EmotesTextRec();
  ~EmotesTextRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 19;
  }

  static UINT GetRowSize() {
    return 76;
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
  LPCSTR m_name;
  int    m_emoteID;
  int    m_emoteText[16];
};

extern WowClientDB<EmotesTextRec> g_emotesTextDB;
