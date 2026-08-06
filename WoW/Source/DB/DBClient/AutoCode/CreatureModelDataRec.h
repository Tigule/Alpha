#pragma once

#include <DB/WowClientDB.h>

class CreatureModelDataRec {
 public:
  CreatureModelDataRec();
  ~CreatureModelDataRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 14;
  }

  static UINT GetRowSize() {
    return 56;
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
  int    m_flags;
  LPCSTR m_ModelName;
  int    m_sizeClass;
  float  m_modelScale;
  int    m_bloodID;
  int    m_footprintTextureID;
  float  m_footprintTextureLength;
  float  m_footprintTextureWidth;
  float  m_footprintParticleScale;
  int    m_foleyMaterialID;
  int    m_footstepShakeSize;
  int    m_deathThudShakeSize;
  int    m_soundID;
};

extern WowClientDB<CreatureModelDataRec> g_creatureModelDataDB;
