#pragma once

#include <DB/WowClientDB.h>

class EmotesRec {
 public:
  EmotesRec();
  ~EmotesRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 5;
  }

  static UINT GetRowSize() {
    return 20;
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

  int m_ID;
  int m_EmoteAnimID;
  int m_EmoteFlags;
  int m_EmoteSpecProc;
  int m_EmoteSpecProcParam;
};

extern WowClientDB<EmotesRec> g_emotesDB;
