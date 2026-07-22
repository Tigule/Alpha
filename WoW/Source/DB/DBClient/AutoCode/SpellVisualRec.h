#pragma once

#include <DB/WowClientDB.h>

class SpellVisualRec {
 public:
  SpellVisualRec();
  ~SpellVisualRec();

  static const char *__fastcall GetFilename();

  static unsigned int GetNumColumns() {
    return 21;
  }

  static unsigned int GetRowSize() {
    return 69;
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

  int           m_ID;
  int           m_precastKit;
  int           m_castKit;
  int           m_impactKit;
  int           m_stateKit;
  int           m_channelKit;
  int           m_hasMissile;
  int           m_missileModel;
  int           m_missilePathType;
  int           m_missileDestinationAttachment;
  int           m_missileSound;
  int           m_hasAreaEffect;
  int           m_areaModel;
  int           m_areaKit;
  int           m_animEventSoundID;
  unsigned char m_weaponTrailRed;
  unsigned char m_weaponTrailGreen;
  unsigned char m_weaponTrailBlue;
  unsigned char m_weaponTrailAlpha;
  unsigned char m_weaponTrailFadeoutRate;
  int           m_weaponTrailDuration;
};

extern WowClientDB<SpellVisualRec> g_spellVisualDB;
