#pragma once

#include <DB/WowClientDB.h>

class CreatureModelDataRec {
 public:
  CreatureModelDataRec();
  ~CreatureModelDataRec();

  static const char *__fastcall GetFilename();

  static unsigned int GetNumColumns() {
    return 14;
  }

  static unsigned int GetRowSize() {
    return 56;
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

  int         m_ID;
  int         m_flags;
  const char *m_ModelName;
  int         m_sizeClass;
  float       m_modelScale;
  int         m_bloodID;
  int         m_footprintTextureID;
  float       m_footprintTextureLength;
  float       m_footprintTextureWidth;
  float       m_footprintParticleScale;
  int         m_foleyMaterialID;
  int         m_footstepShakeSize;
  int         m_deathThudShakeSize;
  int         m_soundID;
};

extern WowClientDB<CreatureModelDataRec> g_creatureModelDataDB;
