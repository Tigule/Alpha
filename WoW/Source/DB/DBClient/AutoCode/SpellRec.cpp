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

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_school) ||
      !SFileReadTyped(f, &m_category) ||
      !SFileReadTyped(f, &m_castUI) ||
      !SFileReadTyped(f, &m_attributes) ||
      !SFileReadTyped(f, &m_attributesEx) ||
      !SFileReadTyped(f, &m_shapeshiftMask) ||
      !SFileReadTyped(f, &m_targets) ||
      !SFileReadTyped(f, &m_targetCreatureType) ||
      !SFileReadTyped(f, &m_requiresSpellFocus) ||
      !SFileReadTyped(f, &m_casterAuraState) ||
      !SFileReadTyped(f, &m_targetAuraState) ||
      !SFileReadTyped(f, &m_castingTimeIndex) ||
      !SFileReadTyped(f, &m_recoveryTime) ||
      !SFileReadTyped(f, &m_categoryRecoveryTime) ||
      !SFileReadTyped(f, &m_interruptFlags) ||
      !SFileReadTyped(f, &m_auraInterruptFlags) ||
      !SFileReadTyped(f, &m_channelInterruptFlags) ||
      !SFileReadTyped(f, &m_procFlags) ||
      !SFileReadTyped(f, &m_procChance) ||
      !SFileReadTyped(f, &m_procCharges) ||
      !SFileReadTyped(f, &m_maxLevel) ||
      !SFileReadTyped(f, &m_baseLevel) ||
      !SFileReadTyped(f, &m_spellLevel) ||
      !SFileReadTyped(f, &m_durationIndex) ||
      !SFileReadTyped(f, &m_powerType) ||
      !SFileReadTyped(f, &m_manaCost) ||
      !SFileReadTyped(f, &m_manaCostPerLevel) ||
      !SFileReadTyped(f, &m_manaPerSecond) ||
      !SFileReadTyped(f, &m_manaPerSecondPerLevel) ||
      !SFileReadTyped(f, &m_rangeIndex) ||
      !SFileReadTyped(f, &m_speed) ||
      !SFileReadTyped(f, &m_modalNextSpell) ||
      !SFile::Read(f, &m_totem[0], sizeof(m_totem), 0, 0, 0) ||
      !SFile::Read(f, &m_reagent[0], sizeof(m_reagent), 0, 0, 0) ||
      !SFile::Read(f, &m_reagentCount[0], sizeof(m_reagentCount), 0, 0, 0) ||
      !SFileReadTyped(f, &m_equippedItemClass) ||
      !SFileReadTyped(f, &m_equippedItemSubclass) ||
      !SFile::Read(f, &m_effect[0], sizeof(m_effect), 0, 0, 0) ||
      !SFile::Read(f, &m_effectDieSides[0], sizeof(m_effectDieSides), 0, 0, 0) ||
      !SFile::Read(f, &m_effectBaseDice[0], sizeof(m_effectBaseDice), 0, 0, 0) ||
      !SFile::Read(f, &m_effectDicePerLevel[0], sizeof(m_effectDicePerLevel), 0, 0, 0) ||
      !SFile::Read(f, &m_effectRealPointsPerLevel[0], sizeof(m_effectRealPointsPerLevel), 0, 0, 0) ||
      !SFile::Read(f, &m_effectBasePoints[0], sizeof(m_effectBasePoints), 0, 0, 0) ||
      !SFile::Read(f, &m_implicitTargetA[0], sizeof(m_implicitTargetA), 0, 0, 0) ||
      !SFile::Read(f, &m_implicitTargetB[0], sizeof(m_implicitTargetB), 0, 0, 0) ||
      !SFile::Read(f, &m_effectRadiusIndex[0], sizeof(m_effectRadiusIndex), 0, 0, 0) ||
      !SFile::Read(f, &m_effectAura[0], sizeof(m_effectAura), 0, 0, 0) ||
      !SFile::Read(f, &m_effectAuraPeriod[0], sizeof(m_effectAuraPeriod), 0, 0, 0) ||
      !SFile::Read(f, &m_effectAmplitude[0], sizeof(m_effectAmplitude), 0, 0, 0) ||
      !SFile::Read(f, &m_effectChainTargets[0], sizeof(m_effectChainTargets), 0, 0, 0) ||
      !SFile::Read(f, &m_effectItemType[0], sizeof(m_effectItemType), 0, 0, 0) ||
      !SFile::Read(f, &m_effectMiscValue[0], sizeof(m_effectMiscValue), 0, 0, 0) ||
      !SFile::Read(f, &m_effectTriggerSpell[0], sizeof(m_effectTriggerSpell), 0, 0, 0) ||
      !SFileReadTyped(f, &m_spellVisualID) ||
      !SFileReadTyped(f, &m_spellIconID) ||
      !SFileReadTyped(f, &m_activeIconID) ||
      !SFileReadTyped(f, &m_spellPriority) ||
      !SFileReadTyped(f, &tempname_langIndices[0]) ||
      !SFileReadTyped(f, &tempname_langIndices[1]) ||
      !SFileReadTyped(f, &tempname_langIndices[2]) ||
      !SFileReadTyped(f, &tempname_langIndices[3]) ||
      !SFileReadTyped(f, &tempname_langIndices[4]) ||
      !SFileReadTyped(f, &tempname_langIndices[5]) ||
      !SFileReadTyped(f, &tempname_langIndices[6]) ||
      !SFileReadTyped(f, &tempname_langIndices[7]) ||
      !SFileReadTyped(f, &m_name_flag) ||
      !SFileReadTyped(f, &tempnameSubtext_langIndices[0]) ||
      !SFileReadTyped(f, &tempnameSubtext_langIndices[1]) ||
      !SFileReadTyped(f, &tempnameSubtext_langIndices[2]) ||
      !SFileReadTyped(f, &tempnameSubtext_langIndices[3]) ||
      !SFileReadTyped(f, &tempnameSubtext_langIndices[4]) ||
      !SFileReadTyped(f, &tempnameSubtext_langIndices[5]) ||
      !SFileReadTyped(f, &tempnameSubtext_langIndices[6]) ||
      !SFileReadTyped(f, &tempnameSubtext_langIndices[7]) ||
      !SFileReadTyped(f, &m_nameSubtext_flag) ||
      !SFileReadTyped(f, &tempdescription_langIndices[0]) ||
      !SFileReadTyped(f, &tempdescription_langIndices[1]) ||
      !SFileReadTyped(f, &tempdescription_langIndices[2]) ||
      !SFileReadTyped(f, &tempdescription_langIndices[3]) ||
      !SFileReadTyped(f, &tempdescription_langIndices[4]) ||
      !SFileReadTyped(f, &tempdescription_langIndices[5]) ||
      !SFileReadTyped(f, &tempdescription_langIndices[6]) ||
      !SFileReadTyped(f, &tempdescription_langIndices[7]) ||
      !SFileReadTyped(f, &m_description_flag) ||
      !SFileReadTyped(f, &m_manaCostPct) ||
      !SFileReadTyped(f, &m_startRecoveryCategory) ||
      !SFileReadTyped(f, &m_startRecoveryTime)) {
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
