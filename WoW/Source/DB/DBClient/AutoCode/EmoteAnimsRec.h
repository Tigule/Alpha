#pragma once

#include <DB/WowClientDB.h>

class EmoteAnimsRec {
 public:
  EmoteAnimsRec();
  ~EmoteAnimsRec();

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

  int    m_ID;
  int    m_ProcessedAnimIndex;
  LPCSTR m_AnimName;
};

extern WowClientDB<EmoteAnimsRec> g_emoteAnimsDB;
