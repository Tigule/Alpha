#pragma once

#include <DB/WowClientDB.h>

class SpellVisualRec {
 public:
  SpellVisualRec();
  ~SpellVisualRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 21;
  }

  static UINT GetRowSize() {
    return 69;
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

  int  m_ID;
  int  m_precastKit;
  int  m_castKit;
  int  m_impactKit;
  int  m_stateKit;
  int  m_channelKit;
  int  m_hasMissile;
  int  m_missileModel;
  int  m_missilePathType;
  int  m_missileDestinationAttachment;
  int  m_missileSound;
  int  m_hasAreaEffect;
  int  m_areaModel;
  int  m_areaKit;
  int  m_animEventSoundID;
  BYTE m_weaponTrailRed;
  BYTE m_weaponTrailGreen;
  BYTE m_weaponTrailBlue;
  BYTE m_weaponTrailAlpha;
  BYTE m_weaponTrailFadeoutRate;
  int  m_weaponTrailDuration;
};

extern WowClientDB<SpellVisualRec> g_spellVisualDB;
