#pragma once

#include <DB/WowClientDB.h>

class CreatureSoundDataRec {
 public:
  CreatureSoundDataRec();
  ~CreatureSoundDataRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 27;
  }

  static UINT GetRowSize() {
    return 108;
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
  int m_soundExertionID;
  int m_soundExertionCriticalID;
  int m_soundInjuryID;
  int m_soundInjuryCriticalID;
  int m_soundInjuryCrushingBlowID;
  int m_soundDeathID;
  int m_soundStunID;
  int m_soundStandID;
  int m_soundFootstepID;
  int m_soundAggroID;
  int m_soundWingFlapID;
  int m_soundWingGlideID;
  int m_soundAlertID;
  int m_soundFidget[4];
  int m_customAttack[4];
  int m_NPCSoundID;
  int m_loopSoundID;
  int m_creatureImpactType;
  int m_soundJumpStartID;
  int m_soundJumpEndID;
};

extern WowClientDB<CreatureSoundDataRec> g_creatureSoundDataDB;
