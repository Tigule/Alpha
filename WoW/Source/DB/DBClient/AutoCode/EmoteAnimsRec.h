#pragma once

#include <DB/WowClientDB.h>

class EmoteAnimsRec {
 public:
  EmoteAnimsRec();
  ~EmoteAnimsRec();

  static const char *__fastcall GetFilename();

  static unsigned int GetNumColumns() {
    return 3;
  }

  static unsigned int GetRowSize() {
    return 12;
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
  int         m_ProcessedAnimIndex;
  const char *m_AnimName;
};

extern WowClientDB<EmoteAnimsRec> g_emoteAnimsDB;
