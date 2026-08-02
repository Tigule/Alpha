#include <Base/Base.h>
#include <WowConst.h>
#include <MapDefs.h>

#include <Base/CDataStore.h>

#include "Object/ItemStats.h"

void ItemStats::Pack(CDataStore *msg) {
  int i;

  msg->Put(m_class);
  msg->Put(m_subclass);

  for (i = 0; i < 4; ++i) {
    msg->PutString(m_displayName[i]);
  }

  msg->Put(m_displayInfoID);
  msg->Put(m_overallQualityID);
  msg->Put(m_flags);
  msg->Put(m_buyPrice);
  msg->Put(m_sellPrice);
  msg->Put(m_inventoryType);
  msg->Put(m_allowableClass);
  msg->Put(m_allowableRace);
  msg->Put(m_itemLevel);
  msg->Put(m_requiredLevel);
  msg->Put(m_requiredSkill);
  msg->Put(m_requiredSkillRank);
  msg->Put(m_maxCount);
  msg->Put(m_stackable);
  msg->Put(m_containerSlots);

  for (i = 0; i < 10; ++i) {
    msg->Put(m_bonusStat[i]);
    msg->Put(m_bonusAmount[i]);
  }

  for (i = 0; i < 5; ++i) {
    msg->Put(m_minDamage[i]);
    msg->Put(m_maxDamage[i]);
    msg->Put(m_damageType[i]);
  }

  for (i = 0; i < 6; ++i) {
    msg->Put(m_resistances[i]);
  }

  msg->Put(m_delay);
  msg->Put(m_ammunitionType);
  msg->Put(m_maxDurability);

  for (i = 0; i < 5; ++i) {
    msg->Put(m_spellID[i]);
    msg->Put(m_spellTrigger[i]);
    msg->Put(m_spellCharges[i]);
    msg->Put(m_spellCooldown[i]);
    msg->Put(m_spellCategory[i]);
    msg->Put(m_spellCategoryCooldown[i]);
  }

  msg->Put(m_bonding);
  msg->PutString(m_description ? m_description : "");
  msg->Put(m_pageText);
  msg->Put(m_languageID);
  msg->Put(m_pageMaterial);
  msg->Put(m_startQuestID);
  msg->Put(m_lockID);
  msg->Put(m_material);
  msg->Put(m_sheatheType);
}

void ItemStats_C::Unpack(CDataStore *msg) {
  int  i;
  char itemName[0x100];
  char description[0x80];

  msg->Get(m_class);
  msg->Get(m_subclass);

  for (i = 0; i < 4; ++i) {
    msg->GetString(itemName, 0x100);

    if (!itemName[0] && i > 0) {
      m_displayName[i] = SStrDupA(m_displayName[i - 1], __FILE__, __LINE__);
    } else {
      m_displayName[i] = SStrDupA(itemName, __FILE__, __LINE__);
    }
  }

  msg->Get(m_displayInfoID);
  msg->Get(m_overallQualityID);
  msg->Get(m_flags);
  msg->Get(m_buyPrice);
  msg->Get(m_sellPrice);
  msg->Get(m_inventoryType);
  msg->Get(m_allowableClass);
  msg->Get(m_allowableRace);
  msg->Get(m_itemLevel);
  msg->Get(m_requiredLevel);
  msg->Get(m_requiredSkill);
  msg->Get(m_requiredSkillRank);
  msg->Get(m_maxCount);
  msg->Get(m_stackable);
  msg->Get(m_containerSlots);

  for (i = 0; i < 10; ++i) {
    msg->Get(m_bonusStat[i]);
    msg->Get(m_bonusAmount[i]);
  }

  for (i = 0; i < 5; ++i) {
    msg->Get(m_minDamage[i]);
    msg->Get(m_maxDamage[i]);
    msg->Get(m_damageType[i]);
  }

  for (i = 0; i < 6; ++i) {
    msg->Get(m_resistances[i]);
  }

  msg->Get(m_delay);
  msg->Get(m_ammunitionType);
  msg->Get(m_maxDurability);

  for (i = 0; i < 5; ++i) {
    msg->Get(m_spellID[i]);
    msg->Get(m_spellTrigger[i]);
    msg->Get(m_spellCharges[i]);
    msg->Get(m_spellCooldown[i]);
    msg->Get(m_spellCategory[i]);
    msg->Get(m_spellCategoryCooldown[i]);
  }

  msg->Get(m_bonding);
  msg->GetString(description, 0x80);

  if (description[0]) {
    m_description = SStrDupA(description, __FILE__, __LINE__);
  }

  msg->Get(m_pageText);
  msg->Get(m_languageID);
  msg->Get(m_pageMaterial);
  msg->Get(m_startQuestID);
  msg->Get(m_lockID);
  msg->Get(m_material);
  msg->Get(m_sheatheType);
}
