#pragma once

#include <DB/WowClientDB.h>

class SpellVisualKitRec {
 public:
  SpellVisualKitRec();
  ~SpellVisualKitRec();

  static const char *__fastcall GetFilename();

  static unsigned int GetNumColumns() {
    return 19;
  }

  static unsigned int GetRowSize() {
    return 76;
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

  int   m_ID;
  int   m_kitType;
  int   m_anim;
  int   m_headEffect;
  int   m_chestEffect;
  int   m_baseEffect;
  int   m_leftHandEffect;
  int   m_rightHandEffect;
  int   m_breathEffect;
  int   m_specialEffect[3];
  int   m_characterProcedure;
  float m_characterParam[4];
  int   m_soundID;
  int   m_shakeID;
};

extern WowClientDB<SpellVisualKitRec> g_spellVisualKitDB;
