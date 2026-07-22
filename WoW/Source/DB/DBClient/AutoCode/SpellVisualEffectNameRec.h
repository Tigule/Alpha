#pragma once

#include <DB/WowClientDB.h>

class SpellVisualEffectNameRec {
 public:
  SpellVisualEffectNameRec();
  ~SpellVisualEffectNameRec();

  static const char *__fastcall GetFilename();

  static unsigned int GetNumColumns() {
    return 6;
  }

  static unsigned int GetRowSize() {
    return 24;
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
  const char *m_fileName;
  int         m_specialID;
  int         m_specialAttachPoint;
  float       m_areaEffectSize;
  int         m_VisualEffectNameFlags;
};

extern WowClientDB<SpellVisualEffectNameRec> g_spellVisualEffectNameDB;
