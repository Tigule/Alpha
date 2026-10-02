#include "SpellRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR SpellRec::GetFilename() {
  return "DBFilesClient\\Spell.dbc";
}

SpellRec::SpellRec() {
}

SpellRec::~SpellRec() {
}

bool SpellRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempname_langIndices[8];
  UINT tempnameSubtext_langIndices[8];
  UINT tempdescription_langIndices[8];
  int  error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_school) == 0);
  error |= (SFileReadTyped(f, &m_category) == 0);
  error |= (SFileReadTyped(f, &m_castUI) == 0);
  error |= (SFileReadTyped(f, &m_attributes) == 0);
  error |= (SFileReadTyped(f, &m_attributesEx) == 0);
  error |= (SFileReadTyped(f, &m_shapeshiftMask) == 0);
  error |= (SFileReadTyped(f, &m_targets) == 0);
  error |= (SFileReadTyped(f, &m_targetCreatureType) == 0);
  error |= (SFileReadTyped(f, &m_requiresSpellFocus) == 0);
  error |= (SFileReadTyped(f, &m_casterAuraState) == 0);
  error |= (SFileReadTyped(f, &m_targetAuraState) == 0);
  error |= (SFileReadTyped(f, &m_castingTimeIndex) == 0);
  error |= (SFileReadTyped(f, &m_recoveryTime) == 0);
  error |= (SFileReadTyped(f, &m_categoryRecoveryTime) == 0);
  error |= (SFileReadTyped(f, &m_interruptFlags) == 0);
  error |= (SFileReadTyped(f, &m_auraInterruptFlags) == 0);
  error |= (SFileReadTyped(f, &m_channelInterruptFlags) == 0);
  error |= (SFileReadTyped(f, &m_procFlags) == 0);
  error |= (SFileReadTyped(f, &m_procChance) == 0);
  error |= (SFileReadTyped(f, &m_procCharges) == 0);
  error |= (SFileReadTyped(f, &m_maxLevel) == 0);
  error |= (SFileReadTyped(f, &m_baseLevel) == 0);
  error |= (SFileReadTyped(f, &m_spellLevel) == 0);
  error |= (SFileReadTyped(f, &m_durationIndex) == 0);
  error |= (SFileReadTyped(f, &m_powerType) == 0);
  error |= (SFileReadTyped(f, &m_manaCost) == 0);
  error |= (SFileReadTyped(f, &m_manaCostPerLevel) == 0);
  error |= (SFileReadTyped(f, &m_manaPerSecond) == 0);
  error |= (SFileReadTyped(f, &m_manaPerSecondPerLevel) == 0);
  error |= (SFileReadTyped(f, &m_rangeIndex) == 0);
  error |= (SFileReadTyped(f, &m_speed) == 0);
  error |= (SFileReadTyped(f, &m_modalNextSpell) == 0);
  error |= (SFileReadTyped(f, &m_totem) == 0);
  error |= (SFileReadTyped(f, &m_reagent) == 0);
  error |= (SFileReadTyped(f, &m_reagentCount) == 0);
  error |= (SFileReadTyped(f, &m_equippedItemClass) == 0);
  error |= (SFileReadTyped(f, &m_equippedItemSubclass) == 0);
  error |= (SFileReadTyped(f, &m_effect) == 0);
  error |= (SFileReadTyped(f, &m_effectDieSides) == 0);
  error |= (SFileReadTyped(f, &m_effectBaseDice) == 0);
  error |= (SFileReadTyped(f, &m_effectDicePerLevel) == 0);
  error |= (SFileReadTyped(f, &m_effectRealPointsPerLevel) == 0);
  error |= (SFileReadTyped(f, &m_effectBasePoints) == 0);
  error |= (SFileReadTyped(f, &m_implicitTargetA) == 0);
  error |= (SFileReadTyped(f, &m_implicitTargetB) == 0);
  error |= (SFileReadTyped(f, &m_effectRadiusIndex) == 0);
  error |= (SFileReadTyped(f, &m_effectAura) == 0);
  error |= (SFileReadTyped(f, &m_effectAuraPeriod) == 0);
  error |= (SFileReadTyped(f, &m_effectAmplitude) == 0);
  error |= (SFileReadTyped(f, &m_effectChainTargets) == 0);
  error |= (SFileReadTyped(f, &m_effectItemType) == 0);
  error |= (SFileReadTyped(f, &m_effectMiscValue) == 0);
  error |= (SFileReadTyped(f, &m_effectTriggerSpell) == 0);
  error |= (SFileReadTyped(f, &m_spellVisualID) == 0);
  error |= (SFileReadTyped(f, &m_spellIconID) == 0);
  error |= (SFileReadTyped(f, &m_activeIconID) == 0);
  error |= (SFileReadTyped(f, &m_spellPriority) == 0);
  error |= (SFileReadTyped(f, &tempname_langIndices[0]) == 0);
  error |= (SFileReadTyped(f, &tempname_langIndices[1]) == 0);
  error |= (SFileReadTyped(f, &tempname_langIndices[2]) == 0);
  error |= (SFileReadTyped(f, &tempname_langIndices[3]) == 0);
  error |= (SFileReadTyped(f, &tempname_langIndices[4]) == 0);
  error |= (SFileReadTyped(f, &tempname_langIndices[5]) == 0);
  error |= (SFileReadTyped(f, &tempname_langIndices[6]) == 0);
  error |= (SFileReadTyped(f, &tempname_langIndices[7]) == 0);
  error |= (SFileReadTyped(f, &m_name_flag) == 0);
  error |= (SFileReadTyped(f, &tempnameSubtext_langIndices[0]) == 0);
  error |= (SFileReadTyped(f, &tempnameSubtext_langIndices[1]) == 0);
  error |= (SFileReadTyped(f, &tempnameSubtext_langIndices[2]) == 0);
  error |= (SFileReadTyped(f, &tempnameSubtext_langIndices[3]) == 0);
  error |= (SFileReadTyped(f, &tempnameSubtext_langIndices[4]) == 0);
  error |= (SFileReadTyped(f, &tempnameSubtext_langIndices[5]) == 0);
  error |= (SFileReadTyped(f, &tempnameSubtext_langIndices[6]) == 0);
  error |= (SFileReadTyped(f, &tempnameSubtext_langIndices[7]) == 0);
  error |= (SFileReadTyped(f, &m_nameSubtext_flag) == 0);
  error |= (SFileReadTyped(f, &tempdescription_langIndices[0]) == 0);
  error |= (SFileReadTyped(f, &tempdescription_langIndices[1]) == 0);
  error |= (SFileReadTyped(f, &tempdescription_langIndices[2]) == 0);
  error |= (SFileReadTyped(f, &tempdescription_langIndices[3]) == 0);
  error |= (SFileReadTyped(f, &tempdescription_langIndices[4]) == 0);
  error |= (SFileReadTyped(f, &tempdescription_langIndices[5]) == 0);
  error |= (SFileReadTyped(f, &tempdescription_langIndices[6]) == 0);
  error |= (SFileReadTyped(f, &tempdescription_langIndices[7]) == 0);
  error |= (SFileReadTyped(f, &m_description_flag) == 0);
  error |= (SFileReadTyped(f, &m_manaCostPct) == 0);
  error |= (SFileReadTyped(f, &m_startRecoveryCategory) == 0);
  error |= (SFileReadTyped(f, &m_startRecoveryTime) == 0);

  if (error) {
    ConsoleWrite("Error reading SpellRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_name_lang[0] = stringBuffer + tempname_langIndices[0];
    m_name_lang[1] = stringBuffer + tempname_langIndices[1];
    m_name_lang[2] = stringBuffer + tempname_langIndices[2];
    m_name_lang[3] = stringBuffer + tempname_langIndices[3];
    m_name_lang[4] = stringBuffer + tempname_langIndices[4];
    m_name_lang[5] = stringBuffer + tempname_langIndices[5];
    m_name_lang[6] = stringBuffer + tempname_langIndices[6];
    m_name_lang[7] = stringBuffer + tempname_langIndices[7];
    m_nameSubtext_lang[0] = stringBuffer + tempnameSubtext_langIndices[0];
    m_nameSubtext_lang[1] = stringBuffer + tempnameSubtext_langIndices[1];
    m_nameSubtext_lang[2] = stringBuffer + tempnameSubtext_langIndices[2];
    m_nameSubtext_lang[3] = stringBuffer + tempnameSubtext_langIndices[3];
    m_nameSubtext_lang[4] = stringBuffer + tempnameSubtext_langIndices[4];
    m_nameSubtext_lang[5] = stringBuffer + tempnameSubtext_langIndices[5];
    m_nameSubtext_lang[6] = stringBuffer + tempnameSubtext_langIndices[6];
    m_nameSubtext_lang[7] = stringBuffer + tempnameSubtext_langIndices[7];
    m_description_lang[0] = stringBuffer + tempdescription_langIndices[0];
    m_description_lang[1] = stringBuffer + tempdescription_langIndices[1];
    m_description_lang[2] = stringBuffer + tempdescription_langIndices[2];
    m_description_lang[3] = stringBuffer + tempdescription_langIndices[3];
    m_description_lang[4] = stringBuffer + tempdescription_langIndices[4];
    m_description_lang[5] = stringBuffer + tempdescription_langIndices[5];
    m_description_lang[6] = stringBuffer + tempdescription_langIndices[6];
    m_description_lang[7] = stringBuffer + tempdescription_langIndices[7];
  } else {
    m_name_lang[0] = "";
    m_name_lang[1] = "";
    m_name_lang[2] = "";
    m_name_lang[3] = "";
    m_name_lang[4] = "";
    m_name_lang[5] = "";
    m_name_lang[6] = "";
    m_name_lang[7] = "";
    m_nameSubtext_lang[0] = "";
    m_nameSubtext_lang[1] = "";
    m_nameSubtext_lang[2] = "";
    m_nameSubtext_lang[3] = "";
    m_nameSubtext_lang[4] = "";
    m_nameSubtext_lang[5] = "";
    m_nameSubtext_lang[6] = "";
    m_nameSubtext_lang[7] = "";
    m_description_lang[0] = "";
    m_description_lang[1] = "";
    m_description_lang[2] = "";
    m_description_lang[3] = "";
    m_description_lang[4] = "";
    m_description_lang[5] = "";
    m_description_lang[6] = "";
    m_description_lang[7] = "";
  }

  return true;
}
