#pragma once

#include <DB/WowClientDB.h>

class SpellItemEnchantmentRec {
 public:
  SpellItemEnchantmentRec();
  ~SpellItemEnchantmentRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 23;
  }

  static unsigned int GetRowSize() {
    return 92;
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
  int         m_effect[3];
  int         m_effectPointsMin[3];
  int         m_effectPointsMax[3];
  int         m_effectArg[3];
  const char *m_name_lang[NUM_LOCALES];
  int         m_name_flag;
  int         m_itemVisual;
};

extern WowClientDB<SpellItemEnchantmentRec> g_spellItemEnchantmentDB;
