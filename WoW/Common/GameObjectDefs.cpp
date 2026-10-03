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

static int s_doorPropertiesList[] = {1, 4, 3};
static int s_buttonPropertiesList[] = {1, 4, 3};
static int s_questGiverPropertiesList[] = {4, 5, 17};
static int s_chestPropertiesList[] = {4, 6, 7, 8, 25, 26, 13, 30};
static int s_genericPropertiesList[] = {19, 18};
static int s_trapPropertiesList[] = {4, 31, 24, 10, 9, 23, 3, 32};
static int s_chairPropertiesList[] = {11, 12};
static int s_spellFocusPropertiesList[] = {14, 24, 30};
static int s_textPropertiesList[] = {15, 16, 17};
static int s_gooberPropertiesList[] = {4, 20, 21, 3, 22, 8, 23, 15, 16, 17};
static int s_areaDamagePropertiesList[] = {4, 24, 27, 28, 29, 3};
static int s_cameraPropertiesList[] = {4, 33, 21};
static int s_mapObjTransPropertiesList[] = {35, 36, 37};
static int s_ritualPropertiesList[] = {34, 10};

static const ObjectInfo s_objectInfo[19] = {
    { 0,        "door",  3,        s_doorPropertiesList, 0},
    { 1,      "button",  3,      s_buttonPropertiesList, 0},
    { 2,  "questgiver",  3,  s_questGiverPropertiesList, 0},
    { 3,       "chest",  8,       s_chestPropertiesList, 0},
    { 4,      "binder",  0,                           0, 0},
    { 5,     "generic",  2,     s_genericPropertiesList, 0},
    { 6,        "trap",  8,        s_trapPropertiesList, 0},
    { 7,       "chair",  2,       s_chairPropertiesList, 0},
    { 8,  "spellFocus",  3,  s_spellFocusPropertiesList, 0},
    { 9,        "text",  3,        s_textPropertiesList, 0},
    {10,      "goober", 10,      s_gooberPropertiesList, 0},
    {11,   "transport",  0,                           0, 0},
    {12,  "areaDamage",  6,  s_areaDamagePropertiesList, 0},
    {13,      "camera",  3,      s_cameraPropertiesList, 0},
    {14,   "mapobject",  0,                           0, 0},
    {15, "moTransport",  3, s_mapObjTransPropertiesList, 0},
    {16,    "duelFlag",  0,                           0, 0},
    {17, "fishingNode",  0,                           0, 0},
    {18,      "ritual",  2,      s_ritualPropertiesList, 0}
};

LPCSTR CGameObjectDef::NameFromTypeId(int typeId) {
  FATALASSERT(typeId >= 0 && typeId < NUM_GAMEOBJECT_TYPE);
  return typeId >= 0 && typeId < 19 ? s_objectInfo[typeId].name : 0;
}

int CGameObjectDef::GetPropNum(int typeId, int propId) {
  if (typeId >= 19 || propId >= 38) {
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
