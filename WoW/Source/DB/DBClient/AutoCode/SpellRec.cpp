#include "SpellRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

LPCSTR SpellRec::GetFilename() {
  return "DBFilesClient\\Spell.dbc";
}

SpellRec::SpellRec() {
}

SpellRec::~SpellRec() {
}

bool SpellRec::Read(SFile *f, LPCSTR stringBuffer) {
  bool result = true;
  UINT tempname_langIndices[8];
  UINT tempnameSubtext_langIndices[8];
  UINT tempdescription_langIndices[8];

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_school, sizeof(m_school), 0, 0, 0) && result;
  result = SFile::Read(f, &m_category, sizeof(m_category), 0, 0, 0) && result;
  result = SFile::Read(f, &m_castUI, sizeof(m_castUI), 0, 0, 0) && result;
  result = SFile::Read(f, &m_attributes, sizeof(m_attributes), 0, 0, 0) && result;
  result = SFile::Read(f, &m_attributesEx, sizeof(m_attributesEx), 0, 0, 0) && result;
  result = SFile::Read(f, &m_shapeshiftMask, sizeof(m_shapeshiftMask), 0, 0, 0) && result;
  result = SFile::Read(f, &m_targets, sizeof(m_targets), 0, 0, 0) && result;
  result = SFile::Read(f, &m_targetCreatureType, sizeof(m_targetCreatureType), 0, 0, 0) && result;
  result = SFile::Read(f, &m_requiresSpellFocus, sizeof(m_requiresSpellFocus), 0, 0, 0) && result;
  result = SFile::Read(f, &m_casterAuraState, sizeof(m_casterAuraState), 0, 0, 0) && result;
  result = SFile::Read(f, &m_targetAuraState, sizeof(m_targetAuraState), 0, 0, 0) && result;
  result = SFile::Read(f, &m_castingTimeIndex, sizeof(m_castingTimeIndex), 0, 0, 0) && result;
  result = SFile::Read(f, &m_recoveryTime, sizeof(m_recoveryTime), 0, 0, 0) && result;
  result = SFile::Read(f, &m_categoryRecoveryTime, sizeof(m_categoryRecoveryTime), 0, 0, 0) && result;
  result = SFile::Read(f, &m_interruptFlags, sizeof(m_interruptFlags), 0, 0, 0) && result;
  result = SFile::Read(f, &m_auraInterruptFlags, sizeof(m_auraInterruptFlags), 0, 0, 0) && result;
  result = SFile::Read(f, &m_channelInterruptFlags, sizeof(m_channelInterruptFlags), 0, 0, 0) && result;
  result = SFile::Read(f, &m_procFlags, sizeof(m_procFlags), 0, 0, 0) && result;
  result = SFile::Read(f, &m_procChance, sizeof(m_procChance), 0, 0, 0) && result;
  result = SFile::Read(f, &m_procCharges, sizeof(m_procCharges), 0, 0, 0) && result;
  result = SFile::Read(f, &m_maxLevel, sizeof(m_maxLevel), 0, 0, 0) && result;
  result = SFile::Read(f, &m_baseLevel, sizeof(m_baseLevel), 0, 0, 0) && result;
  result = SFile::Read(f, &m_spellLevel, sizeof(m_spellLevel), 0, 0, 0) && result;
  result = SFile::Read(f, &m_durationIndex, sizeof(m_durationIndex), 0, 0, 0) && result;
  result = SFile::Read(f, &m_powerType, sizeof(m_powerType), 0, 0, 0) && result;
  result = SFile::Read(f, &m_manaCost, sizeof(m_manaCost), 0, 0, 0) && result;
  result = SFile::Read(f, &m_manaCostPerLevel, sizeof(m_manaCostPerLevel), 0, 0, 0) && result;
  result = SFile::Read(f, &m_manaPerSecond, sizeof(m_manaPerSecond), 0, 0, 0) && result;
  result = SFile::Read(f, &m_manaPerSecondPerLevel, sizeof(m_manaPerSecondPerLevel), 0, 0, 0) && result;
  result = SFile::Read(f, &m_rangeIndex, sizeof(m_rangeIndex), 0, 0, 0) && result;
  result = SFile::Read(f, &m_speed, sizeof(m_speed), 0, 0, 0) && result;
  result = SFile::Read(f, &m_modalNextSpell, sizeof(m_modalNextSpell), 0, 0, 0) && result;
  result = SFile::Read(f, &m_totem[0], sizeof(m_totem), 0, 0, 0) && result;
  result = SFile::Read(f, &m_reagent[0], sizeof(m_reagent), 0, 0, 0) && result;
  result = SFile::Read(f, &m_reagentCount[0], sizeof(m_reagentCount), 0, 0, 0) && result;
  result = SFile::Read(f, &m_equippedItemClass, sizeof(m_equippedItemClass), 0, 0, 0) && result;
  result = SFile::Read(f, &m_equippedItemSubclass, sizeof(m_equippedItemSubclass), 0, 0, 0) && result;
  result = SFile::Read(f, &m_effect[0], sizeof(m_effect), 0, 0, 0) && result;
  result = SFile::Read(f, &m_effectDieSides[0], sizeof(m_effectDieSides), 0, 0, 0) && result;
  result = SFile::Read(f, &m_effectBaseDice[0], sizeof(m_effectBaseDice), 0, 0, 0) && result;
  result = SFile::Read(f, &m_effectDicePerLevel[0], sizeof(m_effectDicePerLevel), 0, 0, 0) && result;
  result = SFile::Read(f, &m_effectRealPointsPerLevel[0], sizeof(m_effectRealPointsPerLevel), 0, 0, 0) && result;
  result = SFile::Read(f, &m_effectBasePoints[0], sizeof(m_effectBasePoints), 0, 0, 0) && result;
  result = SFile::Read(f, &m_implicitTargetA[0], sizeof(m_implicitTargetA), 0, 0, 0) && result;
  result = SFile::Read(f, &m_implicitTargetB[0], sizeof(m_implicitTargetB), 0, 0, 0) && result;
  result = SFile::Read(f, &m_effectRadiusIndex[0], sizeof(m_effectRadiusIndex), 0, 0, 0) && result;
  result = SFile::Read(f, &m_effectAura[0], sizeof(m_effectAura), 0, 0, 0) && result;
  result = SFile::Read(f, &m_effectAuraPeriod[0], sizeof(m_effectAuraPeriod), 0, 0, 0) && result;
  result = SFile::Read(f, &m_effectAmplitude[0], sizeof(m_effectAmplitude), 0, 0, 0) && result;
  result = SFile::Read(f, &m_effectChainTargets[0], sizeof(m_effectChainTargets), 0, 0, 0) && result;
  result = SFile::Read(f, &m_effectItemType[0], sizeof(m_effectItemType), 0, 0, 0) && result;
  result = SFile::Read(f, &m_effectMiscValue[0], sizeof(m_effectMiscValue), 0, 0, 0) && result;
  result = SFile::Read(f, &m_effectTriggerSpell[0], sizeof(m_effectTriggerSpell), 0, 0, 0) && result;
  result = SFile::Read(f, &m_spellVisualID, sizeof(m_spellVisualID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_spellIconID, sizeof(m_spellIconID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_activeIconID, sizeof(m_activeIconID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_spellPriority, sizeof(m_spellPriority), 0, 0, 0) && result;
  result = SFile::Read(f, &tempname_langIndices[0], sizeof(tempname_langIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempname_langIndices[1], sizeof(tempname_langIndices[1]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempname_langIndices[2], sizeof(tempname_langIndices[2]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempname_langIndices[3], sizeof(tempname_langIndices[3]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempname_langIndices[4], sizeof(tempname_langIndices[4]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempname_langIndices[5], sizeof(tempname_langIndices[5]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempname_langIndices[6], sizeof(tempname_langIndices[6]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempname_langIndices[7], sizeof(tempname_langIndices[7]), 0, 0, 0) && result;
  result = SFile::Read(f, &m_name_flag, sizeof(m_name_flag), 0, 0, 0) && result;
  result = SFile::Read(f, &tempnameSubtext_langIndices[0], sizeof(tempnameSubtext_langIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempnameSubtext_langIndices[1], sizeof(tempnameSubtext_langIndices[1]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempnameSubtext_langIndices[2], sizeof(tempnameSubtext_langIndices[2]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempnameSubtext_langIndices[3], sizeof(tempnameSubtext_langIndices[3]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempnameSubtext_langIndices[4], sizeof(tempnameSubtext_langIndices[4]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempnameSubtext_langIndices[5], sizeof(tempnameSubtext_langIndices[5]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempnameSubtext_langIndices[6], sizeof(tempnameSubtext_langIndices[6]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempnameSubtext_langIndices[7], sizeof(tempnameSubtext_langIndices[7]), 0, 0, 0) && result;
  result = SFile::Read(f, &m_nameSubtext_flag, sizeof(m_nameSubtext_flag), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdescription_langIndices[0], sizeof(tempdescription_langIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdescription_langIndices[1], sizeof(tempdescription_langIndices[1]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdescription_langIndices[2], sizeof(tempdescription_langIndices[2]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdescription_langIndices[3], sizeof(tempdescription_langIndices[3]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdescription_langIndices[4], sizeof(tempdescription_langIndices[4]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdescription_langIndices[5], sizeof(tempdescription_langIndices[5]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdescription_langIndices[6], sizeof(tempdescription_langIndices[6]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdescription_langIndices[7], sizeof(tempdescription_langIndices[7]), 0, 0, 0) && result;
  result = SFile::Read(f, &m_description_flag, sizeof(m_description_flag), 0, 0, 0) && result;
  result = SFile::Read(f, &m_manaCostPct, sizeof(m_manaCostPct), 0, 0, 0) && result;
  result = SFile::Read(f, &m_startRecoveryCategory, sizeof(m_startRecoveryCategory), 0, 0, 0) && result;
  result = SFile::Read(f, &m_startRecoveryTime, sizeof(m_startRecoveryTime), 0, 0, 0) && result;

  if (!result) {
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
