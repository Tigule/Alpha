#pragma once

#include <DB/WowClientDB.h>

class NPCSoundsRec {
 public:
  NPCSoundsRec();
  ~NPCSoundsRec();

  static const char *__fastcall GetFilename();

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
  int m_SoundID[4];
};

extern WowClientDB<NPCSoundsRec> g_nPCSoundsDB;
