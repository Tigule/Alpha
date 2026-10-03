#include <WowConst.h>

#include <storm.h>

enum {
  GAMEOBJECT_NUM_TRIGGERS = 2
};

struct ObjectInfo {
  int                                typeId;
  LPCSTR                             name;
  int                                numProperties;
  int                               *propertyInfo;
  const CGameObjectDef::ValueInfo  **valueInfo;
};

struct PropertyInfo {
  int    typeId;
  LPCSTR name;
  int    valueType;
  int    valueBaseType;
};

static const PropertyInfo s_propertyInfo[CGameObjectDef::NUM_PROP] = {
    {0, "type", 0, 1},
    {1, "startOpen", 1, 1},
    {2, "startDestroyed", 1, 1},
    {3, "autoClose", 3, 0},
    {4, "open", 5, 0},
    {5, "questList", 6, 0},
    {6, "chestLoot", 7, 0},
    {7, "chestRestockTime", 2, 0},
    {8, "consumable", 1, 1},
    {9, "charges", 2, 0},
    {10, "spell", 8, 0},
    {11, "chairslots", 2, 0},
    {12, "chairheight", 2, 0},
    {13, "lootedEvent", 2, 0},
    {14, "spellFocusType", 2, 0},
    {15, "pageID", 9, 0},
    {16, "language", 2, 0},
    {17, "pageMaterial", 10, 0},
    {18, "highlight", 1, 1},
    {19, "floatingTooltip", 1, 1},
    {20, "questID", 2, 0},
    {21, "eventID", 2, 0},
    {22, "customAnim", 2, 0},
    {23, "cooldown", 2, 0},
    {24, "radius", 2, 0},
    {25, "minRestock", 2, 0},
    {26, "maxRestock", 2, 0},
    {27, "damageMin", 2, 0},
    {28, "damageMax", 2, 0},
    {29, "damageSchool", 2, 0},
    {30, "linkedTrap", 11, 0},
    {31, "trapLevel", 2, 0},
    {32, "startDelay", 2, 0},
    {33, "camera", 12, 0},
    {34, "casters", 2, 0},
    {35, "taxiPathID1", 2, 0},
    {36, "taxiPathID2", 2, 0},
    {37, "moveSpeed", 2, 0}
};

LPCSTR CGameObjectDef::NameFromPropId(int propId) {
  FATALASSERT(propId >= 0 && propId < NUM_PROP);
  return propId >= 0 && propId < NUM_PROP ? s_propertyInfo[propId].name : 0;
}

int CGameObjectDef::PropIdFromName(LPCSTR string) {
  for (int propId = 0; propId < NUM_PROP; ++propId) {
    if (!SStrCmpI(string, s_propertyInfo[propId].name, 0x7FFFFFFF)) {
      return propId;
    }
  }
  return -1;
}

int CGameObjectDef::GetPropValueType(int propId) {
  FATALASSERT(propId >= 0 && propId < NUM_PROP);
  return s_propertyInfo[propId].valueType;
}

int CGameObjectDef::GetPropValueBaseType(int propId) {
  FATALASSERT(propId >= 0 && propId < NUM_PROP);
  return s_propertyInfo[propId].valueBaseType;
}

static LPCSTR s_boolEnumValues[] = {"false", "true"};

static int s_doorPropertiesList[] = {1, 4, 3};
static const CGameObjectDef::EnumValue s_doorStartOpenValue = {2, s_boolEnumValues, 0};
static const CGameObjectDef::NumberValue s_doorLockValue = {0.0f, 65535.0f, 1.0f, 0.0f};
static const CGameObjectDef::NumberValue s_doorAutoCloseValue = {0.0f, 65535.0f, 0.1f, 3.0f};
static const CGameObjectDef::ValueInfo *s_doorPropertyValues[] = {
    (const CGameObjectDef::ValueInfo *)&s_doorStartOpenValue,
    (const CGameObjectDef::ValueInfo *)&s_doorLockValue,
    (const CGameObjectDef::ValueInfo *)&s_doorAutoCloseValue
};

static int s_buttonPropertiesList[] = {1, 4, 3};
static const CGameObjectDef::EnumValue s_buttonStartOpenValue = {2, s_boolEnumValues, 0};
static const CGameObjectDef::NumberValue s_buttonLockValue = {0.0f, 65535.0f, 1.0f, 0.0f};
static const CGameObjectDef::NumberValue s_buttonAutoCloseValue = {0.0f, 65535.0f, 0.1f, 3.0f};
static const CGameObjectDef::ValueInfo *s_buttonPropertyValues[] = {
    (const CGameObjectDef::ValueInfo *)&s_buttonStartOpenValue,
    (const CGameObjectDef::ValueInfo *)&s_buttonLockValue,
    (const CGameObjectDef::ValueInfo *)&s_buttonAutoCloseValue
};

static int s_questGiverPropertiesList[] = {4, 5, 17};
static const CGameObjectDef::NumberValue s_questGiverLockValue = {0.0f, 65535.0f, 0.0f, 0.0f};
static const CGameObjectDef::NumberValue s_questGiverQuestListIDValue = {0.0f, 65535.0f, 1.0f, 0.0f};
static const CGameObjectDef::NumberValue s_questGiverTextMaterialValue = {0.0f, 65535.0f, 1.0f, 0.0f};
static const CGameObjectDef::ValueInfo *s_questGiverPropertyValues[] = {
    (const CGameObjectDef::ValueInfo *)&s_questGiverLockValue,
    (const CGameObjectDef::ValueInfo *)&s_questGiverQuestListIDValue,
    (const CGameObjectDef::ValueInfo *)&s_questGiverTextMaterialValue
};

static int s_chestPropertiesList[] = {4, 6, 7, 8, 25, 26, 13, 30};
static const CGameObjectDef::NumberValue s_chestLockValue = {0.0f, 65535.0f, 0.0f, 0.0f};
static const CGameObjectDef::NumberValue s_chestLootValue = {0.0f, 65535.0f, 1.0f, 0.0f};
static const CGameObjectDef::NumberValue s_chestLootTimeValue = {0.0f, 1800000.0f, 1.0f, 30.0f};
static const CGameObjectDef::EnumValue s_chestConsumableValue = {2, s_boolEnumValues, 0};
static const CGameObjectDef::NumberValue s_chestMinRestockValue = {0.0f, 65535.0f, 1.0f, 0.0f};
static const CGameObjectDef::NumberValue s_chestMaxRestockValue = {0.0f, 65535.0f, 1.0f, 0.0f};
static const CGameObjectDef::NumberValue s_chestLootEventValue = {0.0f, 65535.0f, 1.0f, 0.0f};
static const CGameObjectDef::NumberValue s_chestLinkedTrapValue = {0.0f, 65535.0f, 0.0f, 0.0f};
static const CGameObjectDef::ValueInfo *s_chestPropertyValues[] = {
    (const CGameObjectDef::ValueInfo *)&s_chestLockValue,
    (const CGameObjectDef::ValueInfo *)&s_chestLootValue,
    (const CGameObjectDef::ValueInfo *)&s_chestLootTimeValue,
    (const CGameObjectDef::ValueInfo *)&s_chestConsumableValue,
    (const CGameObjectDef::ValueInfo *)&s_chestMinRestockValue,
    (const CGameObjectDef::ValueInfo *)&s_chestMaxRestockValue,
    (const CGameObjectDef::ValueInfo *)&s_chestLootEventValue,
    (const CGameObjectDef::ValueInfo *)&s_chestLinkedTrapValue
};

static int s_chairPropertiesList[] = {11, 12};
static const CGameObjectDef::NumberValue s_chairHeightValue = {0.0f, 2.0f, 1.0f, 1.0f};
static const CGameObjectDef::NumberValue s_chairSlotValue = {1.0f, 5.0f, 1.0f, 1.0f};
static const CGameObjectDef::ValueInfo *s_chairPropertyValues[] = {
    (const CGameObjectDef::ValueInfo *)&s_chairSlotValue,
    (const CGameObjectDef::ValueInfo *)&s_chairHeightValue
};

static int s_trapPropertiesList[] = {4, 31, 24, 10, 9, 23, 3, 32};
static const CGameObjectDef::NumberValue s_trapLockValue = {0.0f, 65535.0f, 0.0f, 0.0f};
static const CGameObjectDef::NumberValue s_trapLevelValue = {0.0f, 65535.0f, 0.0f, 0.0f};
static const CGameObjectDef::NumberValue s_trapRadiusValue = {0.0f, 50.0f, 1.0f, 0.0f};
static const CGameObjectDef::NumberValue s_trapSpellValue = {0.0f, 65535.0f, 0.0f, 0.0f};
static const CGameObjectDef::NumberValue s_trapChargesValue = {0.0f, 65535.0f, 1.0f, 1.0f};
static const CGameObjectDef::NumberValue s_trapCooldownValue = {0.0f, 65535.0f, 1.0f, 0.0f};
static const CGameObjectDef::NumberValue s_trapAutoCloseValue = {0.0f, 65535.0f, 1.0f, 0.0f};
static const CGameObjectDef::NumberValue s_trapStartDelayValue = {0.0f, 65535.0f, 1.0f, 0.0f};
static const CGameObjectDef::ValueInfo *s_trapPropertyValues[] = {
    (const CGameObjectDef::ValueInfo *)&s_trapLockValue,
    (const CGameObjectDef::ValueInfo *)&s_trapLevelValue,
    (const CGameObjectDef::ValueInfo *)&s_trapRadiusValue,
    (const CGameObjectDef::ValueInfo *)&s_trapSpellValue,
    (const CGameObjectDef::ValueInfo *)&s_trapChargesValue,
    (const CGameObjectDef::ValueInfo *)&s_trapCooldownValue,
    (const CGameObjectDef::ValueInfo *)&s_trapAutoCloseValue,
    (const CGameObjectDef::ValueInfo *)&s_trapStartDelayValue
};

static int s_areaDamagePropertiesList[] = {4, 24, 27, 28, 29, 3};
static const CGameObjectDef::NumberValue s_areaDamageLockValue = {0.0f, 65535.0f, 0.0f, 0.0f};
static const CGameObjectDef::NumberValue s_areaDamageRadiusValue = {0.0f, 50.0f, 1.0f, 3.0f};
static const CGameObjectDef::NumberValue s_areaDamageMin = {0.0f, 65535.0f, 1.0f, 0.0f};
static const CGameObjectDef::NumberValue s_areaDamageMax = {0.0f, 65535.0f, 1.0f, 0.0f};
static const CGameObjectDef::NumberValue s_areaDamageSchool = {0.0f, 65535.0f, 1.0f, 0.0f};
static const CGameObjectDef::NumberValue s_areaDamageAutoCloseValue = {0.0f, 65535.0f, 1.0f, 0.0f};
static const CGameObjectDef::ValueInfo *s_areaDamagePropertyValues[] = {
    (const CGameObjectDef::ValueInfo *)&s_areaDamageLockValue,
    (const CGameObjectDef::ValueInfo *)&s_areaDamageRadiusValue,
    (const CGameObjectDef::ValueInfo *)&s_areaDamageMin,
    (const CGameObjectDef::ValueInfo *)&s_areaDamageMax,
    (const CGameObjectDef::ValueInfo *)&s_areaDamageSchool,
    (const CGameObjectDef::ValueInfo *)&s_areaDamageAutoCloseValue
};

static int s_spellFocusPropertiesList[] = {14, 24, 30};
static const CGameObjectDef::NumberValue s_spellFocusTypeValue = {0.0f, 65535.0f, 0.0f, 0.0f};
static const CGameObjectDef::NumberValue s_spellFocusRadiusValue = {0.0f, 50.0f, 1.0f, 10.0f};
static const CGameObjectDef::NumberValue s_spellFocusLinkedTrapValue = {0.0f, 65535.0f, 0.0f, 0.0f};
static const CGameObjectDef::ValueInfo *s_spellFocusPropertyValues[] = {
    (const CGameObjectDef::ValueInfo *)&s_spellFocusTypeValue,
    (const CGameObjectDef::ValueInfo *)&s_spellFocusRadiusValue,
    (const CGameObjectDef::ValueInfo *)&s_spellFocusLinkedTrapValue
};

static int s_textPropertiesList[] = {15, 16, 17};
static const CGameObjectDef::NumberValue s_textTypeValue = {0.0f, 10000.0f, 1.0f, 0.0f};
static const CGameObjectDef::NumberValue s_languageTypeValue = {0.0f, 10000.0f, 1.0f, 0.0f};
static const CGameObjectDef::NumberValue s_textMaterialValue = {0.0f, 65535.0f, 1.0f, 0.0f};
static const CGameObjectDef::ValueInfo *s_textPropertyValues[] = {
    (const CGameObjectDef::ValueInfo *)&s_textTypeValue,
    (const CGameObjectDef::ValueInfo *)&s_languageTypeValue,
    (const CGameObjectDef::ValueInfo *)&s_textMaterialValue
};

static int s_gooberPropertiesList[] = {4, 20, 21, 3, 22, 8, 23, 15, 16, 17};
static const CGameObjectDef::NumberValue s_gooberLockValue = {0.0f, 65535.0f, 1.0f, 0.0f};
static const CGameObjectDef::NumberValue s_gooberQuestIDValue = {-1.0f, 65535.0f, 1.0f, 0.0f};
static const CGameObjectDef::NumberValue s_gooberEventIDValue = {0.0f, 65535.0f, 1.0f, 0.0f};
static const CGameObjectDef::NumberValue s_gooberAutoCloseValue = {0.0f, 65535.0f, 0.1f, 3.0f};
static const CGameObjectDef::NumberValue s_gooberCustomAnimValue = {0.0f, 4.0f, 1.0f, 0.0f};
static const CGameObjectDef::EnumValue s_gooberConsumableValue = {2, s_boolEnumValues, 0};
static const CGameObjectDef::NumberValue s_gooberCooldownValue = {0.0f, 65535.0f, 1.0f, 0.0f};
static const CGameObjectDef::ValueInfo *s_gooberPropertyValues[] = {
    (const CGameObjectDef::ValueInfo *)&s_gooberLockValue,
    (const CGameObjectDef::ValueInfo *)&s_gooberQuestIDValue,
    (const CGameObjectDef::ValueInfo *)&s_gooberEventIDValue,
    (const CGameObjectDef::ValueInfo *)&s_gooberAutoCloseValue,
    (const CGameObjectDef::ValueInfo *)&s_gooberCustomAnimValue,
    (const CGameObjectDef::ValueInfo *)&s_gooberConsumableValue,
    (const CGameObjectDef::ValueInfo *)&s_gooberCooldownValue,
    (const CGameObjectDef::ValueInfo *)&s_textTypeValue,
    (const CGameObjectDef::ValueInfo *)&s_languageTypeValue,
    (const CGameObjectDef::ValueInfo *)&s_textMaterialValue
};

static int s_genericPropertiesList[] = {19, 18};
static const CGameObjectDef::EnumValue s_genericFloatingTooltipValue = {2, s_boolEnumValues, 0};
static const CGameObjectDef::EnumValue s_genericHighlightValue = {2, s_boolEnumValues, 1};
static const CGameObjectDef::ValueInfo *s_genericPropertiesValues[] = {
    (const CGameObjectDef::ValueInfo *)&s_genericFloatingTooltipValue,
    (const CGameObjectDef::ValueInfo *)&s_genericHighlightValue
};

static int s_cameraPropertiesList[] = {4, 33, 21};
static const CGameObjectDef::NumberValue s_cameraLockValue = {0.0f, 65535.0f, 1.0f, 0.0f};
static const CGameObjectDef::NumberValue s_cameraCameraIDValue = {0.0f, 65535.0f, 1.0f, 0.0f};
static const CGameObjectDef::NumberValue s_cameraEventIDValue = {0.0f, 65535.0f, 1.0f, 0.0f};
static const CGameObjectDef::ValueInfo *s_cameraPropertyValues[] = {
    (const CGameObjectDef::ValueInfo *)&s_cameraLockValue,
    (const CGameObjectDef::ValueInfo *)&s_cameraCameraIDValue,
    (const CGameObjectDef::ValueInfo *)&s_cameraEventIDValue
};

static int s_mapObjTransPropertiesList[] = {35, 36, 37};
static const CGameObjectDef::NumberValue s_taxiPathIdValue = {0.0f, 65535.0f, 1.0f, 0.0f};
static const CGameObjectDef::NumberValue s_speedValue = {1.0f, 60.0f, 1.0f, 1.0f};
static const CGameObjectDef::ValueInfo *s_mapObjTransPropertiesValues[] = {
    (const CGameObjectDef::ValueInfo *)&s_taxiPathIdValue,
    (const CGameObjectDef::ValueInfo *)&s_taxiPathIdValue,
    (const CGameObjectDef::ValueInfo *)&s_speedValue
};

static int s_ritualPropertiesList[] = {34, 10};
static const CGameObjectDef::NumberValue s_ritualCastersValue = {1.0f, 5.0f, 1.0f, 1.0f};
static const CGameObjectDef::NumberValue s_ritualSpellValue = {1.0f, 65536.0f, 1.0f, 1.0f};
static const CGameObjectDef::ValueInfo *s_ritualPropertyValues[] = {
    (const CGameObjectDef::ValueInfo *)&s_ritualCastersValue,
    (const CGameObjectDef::ValueInfo *)&s_ritualSpellValue
};

static const ObjectInfo s_objectInfo[CGameObjectDef::NUM_GAMEOBJECT_TYPE] = {
    {0, "door", 3, s_doorPropertiesList, s_doorPropertyValues},
    {1, "button", 3, s_buttonPropertiesList, s_buttonPropertyValues},
    {2, "questgiver", 3, s_questGiverPropertiesList, s_questGiverPropertyValues},
    {3, "chest", 8, s_chestPropertiesList, s_chestPropertyValues},
    {4, "binder", 0, 0, 0},
    {5, "generic", 2, s_genericPropertiesList, s_genericPropertiesValues},
    {6, "trap", 8, s_trapPropertiesList, s_trapPropertyValues},
    {7, "chair", 2, s_chairPropertiesList, s_chairPropertyValues},
    {8, "spellFocus", 3, s_spellFocusPropertiesList, s_spellFocusPropertyValues},
    {9, "text", 3, s_textPropertiesList, s_textPropertyValues},
    {10, "goober", 10, s_gooberPropertiesList, s_gooberPropertyValues},
    {11, "transport", 0, 0, 0},
    {12, "areaDamage", 6, s_areaDamagePropertiesList, s_areaDamagePropertyValues},
    {13, "camera", 3, s_cameraPropertiesList, s_cameraPropertyValues},
    {14, "mapobject", 0, 0, 0},
    {15, "moTransport", 3, s_mapObjTransPropertiesList, s_mapObjTransPropertiesValues},
    {16, "duelFlag", 0, 0, 0},
    {17, "fishingNode", 0, 0, 0},
    {18, "ritual", 2, s_ritualPropertiesList, s_ritualPropertyValues}
};

LPCSTR CGameObjectDef::NameFromTypeId(int typeId) {
  FATALASSERT(typeId >= 0 && typeId < NUM_GAMEOBJECT_TYPE);
  return typeId >= 0 && typeId < NUM_GAMEOBJECT_TYPE ? s_objectInfo[typeId].name : 0;
}

int CGameObjectDef::TypeIdFromName(LPCSTR string) {
  for (int typeId = 0; typeId < NUM_GAMEOBJECT_TYPE; ++typeId) {
    if (!SStrCmpI(string, s_objectInfo[typeId].name, 0x7FFFFFFF)) {
      return typeId;
    }
  }
  return -1;
}

int CGameObjectDef::GetNumProps(int typeId) {
  FATALASSERT(typeId >= 0 && typeId < NUM_GAMEOBJECT_TYPE);
  return s_objectInfo[typeId].numProperties;
}

int CGameObjectDef::GetPropId(int typeId, int propNum) {
  FATALASSERT(typeId >= 0 && typeId < NUM_GAMEOBJECT_TYPE);
  FATALASSERT(propNum >= 0 && propNum < s_objectInfo[typeId].numProperties);
  return s_objectInfo[typeId].propertyInfo[propNum];
}

int CGameObjectDef::GetPropNum(int typeId, int propId) {
  if (typeId >= NUM_GAMEOBJECT_TYPE || propId >= NUM_PROP) {
    return -1;
  }
  FATALASSERT(typeId >= 0 && typeId < NUM_GAMEOBJECT_TYPE);
  FATALASSERT(propId >= 0 && propId < NUM_PROP);

  for (int propNum = 0; propNum < s_objectInfo[typeId].numProperties; ++propNum) {
    if (s_objectInfo[typeId].propertyInfo[propNum] == propId) {
      return propNum;
    }
  }
  return -1;
}

const CGameObjectDef::ValueInfo *CGameObjectDef::GetPropValueInfo(int typeId, int propNum) {
  FATALASSERT(typeId >= 0 && typeId < NUM_GAMEOBJECT_TYPE);
  FATALASSERT(propNum >= 0 && propNum < s_objectInfo[typeId].numProperties);
  return s_objectInfo[typeId].valueInfo[propNum];
}
