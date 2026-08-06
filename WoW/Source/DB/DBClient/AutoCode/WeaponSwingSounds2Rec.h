#pragma once

#include <DB/WowClientDB.h>

class WeaponSwingSounds2Rec {
 public:
  WeaponSwingSounds2Rec();
  ~WeaponSwingSounds2Rec();

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

  int m_ID;
  int m_SwingType;
  int m_Crit;
  int m_SoundID;
};

extern WowClientDB<WeaponSwingSounds2Rec> g_weaponSwingSounds2DB;
