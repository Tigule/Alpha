#pragma once

#include <DB/WowClientDB.h>

class SpellRec {
 public:
  SpellRec();
  ~SpellRec();

  static const char *__fastcall GetFilename();

  static unsigned int GetNumColumns() {
    return 135;
  }

  static unsigned int GetRowSize() {
    return 540;
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
  int         m_school;
  int         m_category;
  int         m_castUI;
  int         m_attributes;
  int         m_attributesEx;
  int         m_shapeshiftMask;
  int         m_targets;
  int         m_targetCreatureType;
  int         m_requiresSpellFocus;
  int         m_casterAuraState;
  int         m_targetAuraState;
  int         m_castingTimeIndex;
  int         m_recoveryTime;
  int         m_categoryRecoveryTime;
  int         m_interruptFlags;
  int         m_auraInterruptFlags;
  int         m_channelInterruptFlags;
  int         m_procFlags;
  int         m_procChance;
  int         m_procCharges;
  int         m_maxLevel;
  int         m_baseLevel;
  int         m_spellLevel;
  int         m_durationIndex;
  int         m_powerType;
  int         m_manaCost;
  int         m_manaCostPerLevel;
  int         m_manaPerSecond;
  int         m_manaPerSecondPerLevel;
  int         m_rangeIndex;
  float       m_speed;
  int         m_modalNextSpell;
  int         m_totem[2];
  int         m_reagent[8];
  int         m_reagentCount[8];
  int         m_equippedItemClass;
  int         m_equippedItemSubclass;
  int         m_effect[3];
  int         m_effectDieSides[3];
  int         m_effectBaseDice[3];
  int         m_effectDicePerLevel[3];
  float       m_effectRealPointsPerLevel[3];
  int         m_effectBasePoints[3];
  int         m_implicitTargetA[3];
  int         m_implicitTargetB[3];
  int         m_effectRadiusIndex[3];
  int         m_effectAura[3];
  int         m_effectAuraPeriod[3];
  float       m_effectAmplitude[3];
  int         m_effectChainTargets[3];
  int         m_effectItemType[3];
  int         m_effectMiscValue[3];
  int         m_effectTriggerSpell[3];
  int         m_spellVisualID;
  int         m_spellIconID;
  int         m_activeIconID;
  int         m_spellPriority;
  const char *m_name_lang[8];
  int         m_name_flag;
  const char *m_nameSubtext_lang[8];
  int         m_nameSubtext_flag;
  const char *m_description_lang[8];
  int         m_description_flag;
  int         m_manaCostPct;
  int         m_startRecoveryCategory;
  int         m_startRecoveryTime;
};

extern WowClientDB<SpellRec> g_spellDB;
