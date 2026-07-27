#include <storm.h>

class CGameObjectDef {
 public:
  static const char *NameFromTypeId(int typeId);
  static int GetPropNum(int typeId, int propId);
};

struct ObjectInfo {
  int         numProps;
  const int  *props;
  const void *values;
  const char *name;
  int         unused;
};

static const int s_doorPropertiesList[] = {1, 4, 3};
static const int s_buttonPropertiesList[] = {1, 4, 3};
static const int s_questGiverPropertiesList[] = {4, 5, 17};
static const int s_chestPropertiesList[] = {4, 6, 7, 8, 25, 26, 13, 30};
static const int s_genericPropertiesList[] = {19, 18};
static const int s_trapPropertiesList[] = {4, 31, 24, 10, 9, 23, 3, 32};
static const int s_chairPropertiesList[] = {11, 12};
static const int s_spellFocusPropertiesList[] = {14, 24, 30};
static const int s_textPropertiesList[] = {15, 16, 17};
static const int s_gooberPropertiesList[] = {4, 20, 21, 3, 22, 8, 23, 15, 16, 17};
static const int s_areaDamagePropertiesList[] = {4, 24, 27, 28, 29, 3};
static const int s_cameraPropertiesList[] = {4, 33, 21};
static const int s_mapObjTransPropertiesList[] = {35, 36, 37};
static const int s_ritualPropertiesList[] = {34, 10};

static ObjectInfo s_objectInfo[19] = {
    { 3,        s_doorPropertiesList, 0,        "door", 0},
    { 3,      s_buttonPropertiesList, 0,      "button", 0},
    { 3,  s_questGiverPropertiesList, 0,  "questgiver", 0},
    { 8,       s_chestPropertiesList, 0,       "chest", 0},
    { 0,                           0, 0,      "binder", 0},
    { 2,     s_genericPropertiesList, 0,     "generic", 0},
    { 8,        s_trapPropertiesList, 0,        "trap", 0},
    { 2,       s_chairPropertiesList, 0,       "chair", 0},
    { 3,  s_spellFocusPropertiesList, 0,  "spellFocus", 0},
    { 3,        s_textPropertiesList, 0,        "text", 0},
    {10,      s_gooberPropertiesList, 0,      "goober", 0},
    { 0,                           0, 0,   "transport", 0},
    { 6,  s_areaDamagePropertiesList, 0,  "areaDamage", 0},
    { 3,      s_cameraPropertiesList, 0,      "camera", 0},
    { 0,                           0, 0,   "mapobject", 0},
    { 3, s_mapObjTransPropertiesList, 0, "moTransport", 0},
    { 0,                           0, 0,    "duelFlag", 0},
    { 0,                           0, 0, "fishingNode", 0},
    { 2,      s_ritualPropertiesList, 0,      "ritual", 0}
};

const char *CGameObjectDef::NameFromTypeId(int typeId) {
  FATALASSERT(typeId >= 0 && typeId < 19);
  return typeId >= 0 && typeId < 19 ? s_objectInfo[typeId].name : 0;
}

int CGameObjectDef::GetPropNum(int typeId, int propId) {
  if (typeId >= 19 || propId >= 38) {
    return -1;
  }
  FATALASSERT(typeId >= 0);
  FATALASSERT(propId >= 0);

  for (int propNum = 0; propNum < s_objectInfo[typeId].numProps; ++propNum) {
    if (s_objectInfo[typeId].props[propNum] == propId) {
      return propNum;
    }
  }
  return -1;
}
