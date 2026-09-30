#pragma once

#include <DB/WowClientDB.h>

class SpellEffectCameraShakesRec {
 public:
  int m_ID;
  int m_CameraShake[3];

  SpellEffectCameraShakesRec();
  ~SpellEffectCameraShakesRec();

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

extern WowClientDB<SpellEffectCameraShakesRec> g_spellEffectCameraShakesDB;
