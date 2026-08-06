#pragma once

#include <DB/WowClientDB.h>

class WeaponImpactSoundsRec {
 public:
  WeaponImpactSoundsRec();
  ~WeaponImpactSoundsRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 23;
  }

  static UINT GetRowSize() {
    return 92;
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
  int m_WeaponSubClassID;
  int m_ParrySoundType;
  int m_impactSoundID[10];
  int m_critImpactSoundID[10];
};

extern WowClientDB<WeaponImpactSoundsRec> g_weaponImpactSoundsDB;
