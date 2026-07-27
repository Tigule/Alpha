#pragma once

#include <DB/WowClientDB.h>

class WeaponSwingSounds2Rec {
 public:
  WeaponSwingSounds2Rec();
  ~WeaponSwingSounds2Rec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 4;
  }

  static unsigned int GetRowSize() {
    return 16;
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
  int m_SwingType;
  int m_Crit;
  int m_SoundID;
};

extern WowClientDB<WeaponSwingSounds2Rec> g_weaponSwingSounds2DB;
