#pragma once

#include <DB/WowClientDB.h>

class SoundWaterTypeRec {
 public:
  int m_ID;
  int m_soundType;
  int m_soundSubtype;
  int m_SoundID;

  SoundWaterTypeRec();
  ~SoundWaterTypeRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 4;
  }

  static UINT GetRowSize() {
    return 16;
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
};

extern WowClientDB<SoundWaterTypeRec> g_soundWaterTypeDB;
