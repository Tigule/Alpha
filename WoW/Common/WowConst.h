#pragma once

#include <storm.h>
#include "InventoryConst.h"

static const float MAX_DUEL_DISTANCE = 10.0f;
static const float MAX_DUEL_DISTANCE_SQUARED = MAX_DUEL_DISTANCE * MAX_DUEL_DISTANCE;
static const float MAX_TRADE_DISTANCE = 11.111111f;
static const float MAX_TRADE_DISTANCE_SQUARED = MAX_TRADE_DISTANCE * MAX_TRADE_DISTANCE;
static const float MAX_SHOP_DISTANCE = 5.5555553f;
static const float MAX_SHOP_DISTANCE_SQUARED = MAX_SHOP_DISTANCE * MAX_SHOP_DISTANCE;
static const float MAX_INSPECT_DISTANCE = 5.5555553f;
static const float MAX_INSPECT_DISTANCE_SQUARED = MAX_INSPECT_DISTANCE * MAX_INSPECT_DISTANCE;
static const float MAX_LOOT_DISTANCE = 5.0f;
static const float MAX_LOOT_DISTANCE_SQUARED = MAX_LOOT_DISTANCE * MAX_LOOT_DISTANCE;
static const float MAX_BIND_DISTANCE = 10.0f;
static const float MAX_BIND_DISTANCE_SQUARED = MAX_BIND_DISTANCE * MAX_BIND_DISTANCE;
static const float MAX_SITCHAIR_DISTANCE = 0.5f;
static const float MAX_SITCHAIR_DISTANCE_SQUARED = MAX_SITCHAIR_DISTANCE * MAX_SITCHAIR_DISTANCE;
static const float MAX_SITCHAIRUSE_DISTANCE = 3.0f;
static const float MAX_SITCHAIRUSE_DISTANCE_SQUARED = MAX_SITCHAIRUSE_DISTANCE * MAX_SITCHAIRUSE_DISTANCE;
static const float MAX_SHARE_DISTANCE = 100.0f;
static const float MAX_SHARE_DISTANCE_SQUARED = MAX_SHARE_DISTANCE * MAX_SHARE_DISTANCE;
static const int MILLISECONDS_PER_HEARTBEAT = 5000;
static const float SECONDS_PER_HEARTBEAT = (float)MILLISECONDS_PER_HEARTBEAT / 1000.0f;
static const float SPELL_FOCUS_DISTANCE = 50.0f;
static const float SPELL_FOCUS_DISTANCE_SQUARED = SPELL_FOCUS_DISTANCE * SPELL_FOCUS_DISTANCE;
static const float MAX_OBJ_INTEREST_RADIUS = 100.0f;
static const float MAX_OBJ_INTEREST_RADIUS_SQUARED = MAX_OBJ_INTEREST_RADIUS * MAX_OBJ_INTEREST_RADIUS;

enum LOOT_ACQUIRE {
  LOOT_ACQUIRE_FAILED = 0,
  LOOT_ACQUIRE_NORMAL = 1,
  LOOT_ACQUIRE_PICKPOCKET = 2,
  LOOT_ACQUIRE_FISHING = 3
};

enum LOOT_METHOD {
  LOOT_METHOD_FREEFORALL = 0,
  LOOT_METHOD_ROUNDROBIN = 1,
  LOOT_METHOD_MASTERLOOTER = 2,
  LOOT_METHOD_MAX = 3
};

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
