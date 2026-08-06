#pragma once

#include <DB/WowClientDB.h>

class ChrRacesRec {
 public:
  ChrRacesRec();
  ~ChrRacesRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 25;
  }

  static UINT GetRowSize() {
    return 100;
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
  int    m_factionID;
  int    m_MaleDisplayId;
  int    m_FemaleDisplayId;
  LPCSTR m_ClientPrefix;
  float  m_MountScale;
  int    m_BaseLanguage;
  int    m_creatureType;
  int    m_LoginEffectSpellID;
  int    m_CombatStunSpellID;
  int    m_ResSicknessSpellID;
  int    m_SplashSoundID;
  int    m_startingTaxiNodes;
  LPCSTR m_clientFileString;
  int    m_cinematicSequenceID;
  LPCSTR m_name_lang[NUM_LOCALES];
  int    m_name_flag;
};

extern WowClientDB<ChrRacesRec> g_chrRacesDB;
