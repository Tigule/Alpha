#pragma once

#include <DB/WowClientDB.h>

class VocalUISoundsRec {
 public:
  VocalUISoundsRec();
  ~VocalUISoundsRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 7;
  }

  static UINT GetRowSize() {
    return 28;
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
  int m_vocalUIEnum;
  int m_raceID;
  int m_NormalSoundID[2];
  int m_PissedSoundID[2];
};

extern WowClientDB<VocalUISoundsRec> g_vocalUISoundsDB;
