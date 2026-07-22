#ifndef WOW_SOURCE_OBJECT_ITEMSTATS_H
#define WOW_SOURCE_OBJECT_ITEMSTATS_H

#include <string.h>

class CDataStore;

class ItemStats {
 public:
  ItemStats() {
    memset(m_displayName, 0, sizeof(m_displayName));
    m_description = 0;
  }

  ~ItemStats() {
    int i;

    for (i = 0; i < 4; ++i) {
      FREEIFUSED(m_displayName[i]);
    }

    FREEIFUSED(m_description);
  }

  int Version() {
    return 1;
  }
  void Pack(CDataStore *msg);

  int   m_class;
  int   m_subclass;
  char *m_displayName[4];
  int   m_displayInfoID;
  int   m_overallQualityID;
  int   m_flags;
  int   m_buyPrice;
  int   m_sellPrice;
  int   m_inventoryType;
  int   m_allowableClass;
  int   m_allowableRace;
  int   m_itemLevel;
  int   m_requiredLevel;
  int   m_requiredSkill;
  int   m_requiredSkillRank;
  int   m_maxCount;
  int   m_stackable;
  int   m_containerSlots;
  int   m_bonusStat[10];
  int   m_bonusAmount[10];
  int   m_minDamage[5];
  int   m_maxDamage[5];
  int   m_damageType[5];
  int   m_resistances[6];
  int   m_delay;
  int   m_ammunitionType;
  int   m_maxDurability;
  int   m_spellID[5];
  int   m_spellTrigger[5];
  int   m_spellCharges[5];
  int   m_spellCooldown[5];
  int   m_spellCategory[5];
  int   m_spellCategoryCooldown[5];
  int   m_bonding;
  char *m_description;
  int   m_pageText;
  int   m_languageID;
  int   m_pageMaterial;
  int   m_startQuestID;
  int   m_lockID;
  int   m_material;
  int   m_sheatheType;
};

class ItemStats_C : public ItemStats {
 public:
  ItemStats_C() {
    memset(this, 0, sizeof(*this));
  }

  void Unpack(CDataStore *msg);
};

#endif
