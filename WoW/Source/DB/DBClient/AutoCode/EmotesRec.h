#pragma once

#include <DB/WowClientDB.h>

class EmotesRec {
 public:
  EmotesRec();
  ~EmotesRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 5;
  }

  static unsigned int GetRowSize() {
    return 20;
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

  int m_ID;
  int m_EmoteAnimID;
  int m_EmoteFlags;
  int m_EmoteSpecProc;
  int m_EmoteSpecProcParam;
};

extern WowClientDB<EmotesRec> g_emotesDB;
