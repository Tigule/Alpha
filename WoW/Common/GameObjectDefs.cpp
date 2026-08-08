#include <WowConst.h>

#include <storm.h>

class CGameObjectDef {
 public:
  enum {
    TYPE_DOOR = 0,
    TYPE_BUTTON = 1,
    TYPE_QUESTGIVER = 2,
    TYPE_CHEST = 3,
    TYPE_BINDER = 4,
    TYPE_GENERIC = 5,
    TYPE_TRAP = 6,
    TYPE_CHAIR = 7,
    TYPE_SPELL_FOCUS = 8,
    TYPE_TEXT = 9,
    TYPE_GOOBER = 10,
    TYPE_TRANSPORT = 11,
    TYPE_AREADAMAGE = 12,
    TYPE_CAMERA = 13,
    TYPE_MAP_OBJECT = 14,
    TYPE_MO_TRANSPORT = 15,
    TYPE_DUEL_ARBITER = 16,
    TYPE_FISHINGNODE = 17,
    TYPE_RITUAL = 18,
    NUM_GAMEOBJECT_TYPE = 19
  };

  enum {
    PROP_TYPE = 0,
    PROP_STARTOPEN = 1,
    PROP_STARTDESTROYED = 2,
    PROP_AUTOCLOSE = 3,
    PROP_LOCK = 4,
    PROP_QUESTLISTID = 5,
    PROP_CHESTLOOT = 6,
    PROP_CHESTLOOTTIME = 7,
    PROP_CONSUMABLE = 8,
    PROP_CHARGES = 9,
    PROP_SPELL = 10,
    PROP_CHAIRSLOTS = 11,
    PROP_CHAIRHEIGHT = 12,
    PROP_CHESTLOOTEDEVENT = 13,
    PROP_SPELLFOCUSTYPE = 14,
    PROP_TEXTID = 15,
    PROP_TEXTLANGUAGE = 16,
    PROP_TEXTMATERIAL = 17,
    PROP_HIGHLIGHT = 18,
    PROP_FLOATINGTOOLTIP = 19,
    PROP_QUESTID = 20,
    PROP_EVENTID = 21,
    PROP_CUSTOMANIM = 22,
    PROP_COOLDOWN = 23,
    PROP_RADIUS = 24,
    PROP_CHESTMINRESTOCK = 25,
    PROP_CHESTMAXRESTOCK = 26,
    PROP_DAMAGE_MIN = 27,
    PROP_DAMAGE_MAX = 28,
    PROP_DAMAGE_SCHOOL = 29,
    PROP_LINKED_TRAP = 30,
    PROP_TRAP_LEVEL = 31,
    PROP_STARTDELAY = 32,
    PROP_CAMERAID = 33,
    PROP_CASTERS = 34,
    PROP_TAXIPATHID1 = 35,
    PROP_TAXIPATHID2 = 36,
    PROP_MOVESPEED = 37,
    NUM_PROP = 38
  };

  enum {
    VALUE_TYPE_TYPE = 0,
    VALUE_TYPE_BOOL = 1,
    VALUE_TYPE_INT = 2,
    VALUE_TYPE_FLOAT = 3,
    VALUE_TYPE_NAME = 4,
    VALUE_TYPE_LOCK = 5,
    VALUE_TYPE_QUESTGIVER = 6,
    VALUE_TYPE_LOOTTABLE = 7,
    VALUE_TYPE_SPELL = 8,
    VALUE_TYPE_PAGETEXT = 9,
    VALUE_TYPE_PAGEMATERIAL = 10,
    VALUE_TYPE_TRAP = 11,
    VALUE_TYPE_CAMERA = 12,
    NUM_VALUE_TYPE = 13
  };

  enum {
    BASE_TYPE_NUMBER = 0,
    BASE_TYPE_ENUM = 1,
    BASE_TYPE_STRING = 2,
    NUM_BASE_TYPE = 3
  };

  enum {
    OWNER_TYPE_TERRAIN = 0,
    OWNER_TYPE_MAPOBJ = 1,
    OWNER_TYPE_SPAWNER = 2,
    NUM_OWNER_TYPE = 3
  };

  struct EnumValue {
    int     count;
    LPCSTR *list;
    int     defaultIndex;
  };

  struct NumberValue {
    float min;
    float max;
    float step;
    float defaultValue;
  };

  struct StringValue {
    LPCSTR defaultValue;
  };

  union ValueInfo {
    EnumValue   e;
    NumberValue n;
    StringValue s;
  };

  static LPCSTR           NameFromTypeId(int typeId);
  static int              TypeIdFromName(LPCSTR string);
  static int              GetNumProps(int typeId);
  static int              GetPropId(int typeId, int propNum);
  static int              GetPropNum(int typeId, int propId);
  static const ValueInfo *GetPropValueInfo(int typeId, int propNum);
  static LPCSTR           NameFromPropId(int propId);
  static int              PropIdFromName(LPCSTR string);
  static int              GetPropValueType(int propId);
  static int              GetPropValueBaseType(int propId);
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
  FATALASSERT(typeId >= 0 && typeId < 19);
  return typeId >= 0 && typeId < 19 ? s_objectInfo[typeId].name : 0;
}

int CGameObjectDef::GetPropNum(int typeId, int propId) {
  if (typeId >= 19 || propId >= 38) {
    return -1;
  }
  FATALASSERT(typeId >= 0);
  FATALASSERT(propId >= 0);

  for (int propNum = 0; propNum < s_objectInfo[typeId].numProperties; ++propNum) {
    if (s_objectInfo[typeId].propertyInfo[propNum] == propId) {
      return propNum;
    }
  }
  return -1;
}
