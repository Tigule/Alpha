#pragma once

#include <DB/WowClientDB.h>

class VocalUISoundsRec {
 public:
  VocalUISoundsRec();
  ~VocalUISoundsRec();

  static const char *__fastcall GetFilename();

  static unsigned int GetNumColumns() {
    return 7;
  }

  static unsigned int GetRowSize() {
    return 28;
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
  int m_vocalUIEnum;
  int m_raceID;
  int m_NormalSoundID[2];
  int m_PissedSoundID[2];
};

extern WowClientDB<VocalUISoundsRec> g_vocalUISoundsDB;
