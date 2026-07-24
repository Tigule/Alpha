#include <FrameScript/FrameScript.h>
#include <Frame/CSimpleRender.h>

#include "DB/DBClient/AutoCode/ChrClassesRec.h"
#include "DB/DBClient/AutoCode/ChrRacesRec.h"
#include "DB/DBClient/DBCacheInstances.h"
#include "DB/DBClient/DBClient.h"
#include "DB/WowLocale.h"
#include "Game/GameTime.h"
#include "Object/ObjectClient/Bag_C.h"
#include "Object/ObjectClient/Player_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "Ui/GameUI.h"
#include "Ui/PartyFrame.h"
#include <Os/OsTime.h>

#include <lauxlib.h>
#include <lua.h>
#include <storm.h>
#include <windows.h>

static int __fastcall Script_GetTime(lua_State *L);
static int __fastcall Script_GetGameTime(lua_State *L);
static int __fastcall Script_UnitExists(lua_State *L);
static int __fastcall Script_UnitIsUnit(lua_State *L);
static int __fastcall Script_UnitIsPlayer(lua_State *L);
static int __fastcall Script_UnitIsPartyLeader(lua_State *L);
static int __fastcall Script_UnitInParty(lua_State *L);
static int __fastcall Script_UnitReaction(lua_State *L);
static int __fastcall Script_UnitIsEnemy(lua_State *L);
static int __fastcall Script_UnitIsFriend(lua_State *L);
static int __fastcall Script_UnitCanCooperate(lua_State *L);
static int __fastcall Script_UnitIsCharmed(lua_State *L);
static int __fastcall Script_UnitIsPlusMob(lua_State *L);
static int __fastcall Script_UnitName(lua_State *L);
static int __fastcall Script_UnitXP(lua_State *L);
static int __fastcall Script_UnitXPMax(lua_State *L);
static int __fastcall Script_UnitHealth(lua_State *L);
static int __fastcall Script_UnitHealthMax(lua_State *L);
static int __fastcall Script_UnitMana(lua_State *L);
static int __fastcall Script_UnitManaMax(lua_State *L);
static int __fastcall Script_UnitPowerType(lua_State *L);
static int __fastcall Script_UnitIsDead(lua_State *L);
static int __fastcall Script_UnitIsConnected(lua_State *L);
static int __fastcall Script_UnitSex(lua_State *L);
static int __fastcall Script_UnitLevel(lua_State *L);
static int __fastcall Script_UnitMoney(lua_State *L);
static int __fastcall Script_UnitRace(lua_State *L);
static int __fastcall Script_UnitClass(lua_State *L);
static int __fastcall Script_UnitResistance(lua_State *L);
static int __fastcall Script_UnitStat(lua_State *L);
static int __fastcall Script_UnitAttackBothHands(lua_State *L);
static int __fastcall Script_UnitDamage(lua_State *L);
static int __fastcall Script_UnitAttackSpeed(lua_State *L);
static int __fastcall Script_UnitDefense(lua_State *L);
static int __fastcall Script_UnitArmor(lua_State *L);
static int __fastcall Script_UnitCharacterPoints(lua_State *L);
static int __fastcall Script_SetPortraitTexture(lua_State *L);
static int __fastcall Script_HasFullControl(lua_State *L);
static int __fastcall Script_GetComboPoints(lua_State *L);
static int __fastcall Script_IsInGuild(lua_State *L);

static char  s_unitNameArray[4][32];
static char *s_unitNames[4] = {s_unitNameArray[0], s_unitNameArray[1], s_unitNameArray[2], s_unitNameArray[3]};

CGUnit_C *__fastcall        Script_GetUnitFromName(const char *name);
unsigned __int64 __fastcall Script_GetGUIDFromName(const char *name);
void __fastcall             SetPortraitTexture(CSimpleTexture *texture, CGUnit_C *unit);
void __fastcall             SetPortraitTexture(CSimpleTexture *texture, unsigned int race, unsigned int sex, unsigned __int64 guid);

class ScriptPartyInfoAccess : public CGPartyInfo {
 public:
  using CGPartyInfo::m_leader;
  using CGPartyInfo::m_members;
};

CGUnit_C *__fastcall Script_GetUnitFromName(const char *name) {
  unsigned __int64 guid;
  CGObject_C      *object;
  CGUnit_C        *player;

  guid = ClntObjMgrGetActivePlayer();
  object = ClntObjMgrObjectPtr(guid, __FILE__, __LINE__);
  player = object && (object->GetType() & TYPE_UNIT) ? static_cast<CGUnit_C *>(object) : 0;
  if (!player || !name || !*name) {
    return 0;
  }
  if (!SStrCmpI(name, "player", 0x7FFFFFFF)) {
    return player;
  }
  if (!SStrCmpI(name, "pet", 0x7FFFFFFF)) {
    guid = player->GetUnitData()->charm;
    if (!guid) {
      guid = player->GetUnitData()->summon;
    }
  } else if (!SStrCmpI(name, "target", 0x7FFFFFFF)) {
    guid = CGGameUI::GetLockedTarget();
  } else if (!SStrCmpI(name, "npc", 0x7FFFFFFF)) {
    guid = CGGameUI::GetInteractTarget();
  } else if (!SStrCmpI(name, "mouseover", 0x7FFFFFFF)) {
    guid = CGGameUI::GetCurrentObjectTrack();
  } else if (!SStrCmpI(name, "party", 5) && name[5] >= '1' && name[5] <= '4') {
    guid = ScriptPartyInfoAccess::m_members[name[5] - '1'];
  } else {
    return 0;
  }
  object = ClntObjMgrObjectPtr(guid, __FILE__, __LINE__);
  return object && (object->GetType() & TYPE_UNIT) ? static_cast<CGUnit_C *>(object) : 0;
}

CGObject_C *__fastcall Script_GetObjectFromName(const char *name) {
  unsigned __int64 guid = ClntObjMgrGetActivePlayer();
  CGObject_C      *playerObject = ClntObjMgrObjectPtr(guid, __FILE__, __LINE__);
  CGUnit_C        *player =
      playerObject && (playerObject->GetType() & TYPE_UNIT) ? static_cast<CGUnit_C *>(playerObject) : 0;
  if (!player || !name || !*name) {
    return 0;
  }

  if (!SStrCmpI(name, "player", 0x7FFFFFFF)) {
    return playerObject;
  }
  if (!SStrCmpI(name, "pet", 0x7FFFFFFF)) {
    guid = player->GetUnitData()->charm;
    if (!guid) {
      guid = player->GetUnitData()->summon;
    }
  } else if (!SStrCmpI(name, "target", 0x7FFFFFFF)) {
    guid = CGGameUI::GetLockedTarget();
  } else if (!SStrCmpI(name, "party", 5) && name[5] >= '1' && name[5] <= '4') {
    guid = CGGameUI::GetPartyMember(name[5] - '1');
  } else if (!SStrCmpI(name, "npc", 0x7FFFFFFF)) {
    guid = CGGameUI::GetInteractTarget();
  } else {
    if (SStrCmpI(name, "mouseover", 0x7FFFFFFF)) {
      FrameScript_DisplayError("Unknown object name: %s", name);
    }
    guid = CGGameUI::GetCurrentObjectTrack();
  }

  return ClntObjMgrObjectPtr(guid, __FILE__, __LINE__);
}

unsigned __int64 __fastcall Script_GetGUIDFromName(const char *name) {
  CGUnit_C        *unit = Script_GetUnitFromName(name);
  unsigned __int64 guid;
  CGObject_C      *object;
  CGUnit_C        *player;

  if (unit) {
    return unit->GetGUID();
  }
  guid = ClntObjMgrGetActivePlayer();
  object = ClntObjMgrObjectPtr(guid, __FILE__, __LINE__);
  player = object && (object->GetType() & TYPE_UNIT) ? static_cast<CGUnit_C *>(object) : 0;
  if (!player || !name || !*name) {
    return 0;
  }
  if (!SStrCmpI(name, "player", 0x7FFFFFFF)) {
    return guid;
  }
  if (!SStrCmpI(name, "pet", 0x7FFFFFFF)) {
    guid = player->GetUnitData()->charm;
    return guid ? guid : player->GetUnitData()->summon;
  }
  if (!SStrCmpI(name, "target", 0x7FFFFFFF)) {
    return CGGameUI::GetLockedTarget();
  }
  if (!SStrCmpI(name, "npc", 0x7FFFFFFF)) {
    return CGGameUI::GetInteractTarget();
  }
  if (!SStrCmpI(name, "mouseover", 0x7FFFFFFF)) {
    return CGGameUI::GetCurrentObjectTrack();
  }
  if (!SStrCmpI(name, "party", 5) && name[5] >= '1' && name[5] <= '4') {
    return ScriptPartyInfoAccess::m_members[name[5] - '1'];
  }
  return 0;
}

char **__fastcall Script_GetNamesFromGUID(const unsigned __int64 &guid, int &numnames) {
  numnames = 0;
  if (!guid) {
    return 0;
  }

  CGUnit_C *player = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return 0;
  }

  if (guid == player->GetGUID()) {
    SStrCopy(s_unitNames[numnames++], "player", 32);
  }

  unsigned __int64 pet = player->GetUnitData()->charm;
  if (!pet) {
    pet = player->GetUnitData()->summon;
  }
  if (guid == pet) {
    SStrCopy(s_unitNames[numnames++], "pet", 32);
  }
  if (guid == CGGameUI::GetLockedTarget()) {
    SStrCopy(s_unitNames[numnames++], "target", 32);
  }

  for (int partyIndex = 0; partyIndex < 4; ++partyIndex) {
    if (guid == ScriptPartyInfoAccess::m_members[partyIndex]) {
      SStrPrintf(s_unitNames[numnames++], 32, "party%d", partyIndex + 1);
      break;
    }
  }

  if (guid == CGGameUI::GetInteractTarget()) {
    SStrCopy(s_unitNames[numnames++], "npc", 32);
  }
  if (guid == CGGameUI::GetCurrentObjectTrack()) {
    SStrCopy(s_unitNames[numnames++], "mouseover", 32);
  }
  return s_unitNames;
}

void __fastcall Script_SendUnitSignal(const unsigned __int64 &guid, int signal) {
  int    numnames;
  char **names = Script_GetNamesFromGUID(guid, numnames);
  for (int index = 0; index < numnames; ++index) {
    FrameScript_SignalEvent(signal, "%s", names[index]);
  }
}

static FrameScript_Method s_SystemFunctions[2] = {
    {    "GetTime",     Script_GetTime},
    {"GetGameTime", Script_GetGameTime}
};

static FrameScript_Method s_UnitFunctions[38] = {
    {         "UnitExists",          Script_UnitExists},
    {         "UnitIsUnit",          Script_UnitIsUnit},
    {       "UnitIsPlayer",        Script_UnitIsPlayer},
    {  "UnitIsPartyLeader",   Script_UnitIsPartyLeader},
    {        "UnitInParty",         Script_UnitInParty},
    {       "UnitReaction",        Script_UnitReaction},
    {        "UnitIsEnemy",         Script_UnitIsEnemy},
    {       "UnitIsFriend",        Script_UnitIsFriend},
    {   "UnitCanCooperate",    Script_UnitCanCooperate},
    {      "UnitIsCharmed",       Script_UnitIsCharmed},
    {      "UnitIsPlusMob",       Script_UnitIsPlusMob},
    {           "UnitName",            Script_UnitName},
    {             "UnitXP",              Script_UnitXP},
    {          "UnitXPMax",           Script_UnitXPMax},
    {         "UnitHealth",          Script_UnitHealth},
    {      "UnitHealthMax",       Script_UnitHealthMax},
    {           "UnitMana",            Script_UnitMana},
    {        "UnitManaMax",         Script_UnitManaMax},
    {      "UnitPowerType",       Script_UnitPowerType},
    {         "UnitIsDead",          Script_UnitIsDead},
    {    "UnitIsConnected",     Script_UnitIsConnected},
    {            "UnitSex",             Script_UnitSex},
    {          "UnitLevel",           Script_UnitLevel},
    {          "UnitMoney",           Script_UnitMoney},
    {           "UnitRace",            Script_UnitRace},
    {          "UnitClass",           Script_UnitClass},
    {     "UnitResistance",      Script_UnitResistance},
    {           "UnitStat",            Script_UnitStat},
    {"UnitAttackBothHands", Script_UnitAttackBothHands},
    {         "UnitDamage",          Script_UnitDamage},
    {    "UnitAttackSpeed",     Script_UnitAttackSpeed},
    {        "UnitDefense",         Script_UnitDefense},
    {          "UnitArmor",           Script_UnitArmor},
    {"UnitCharacterPoints", Script_UnitCharacterPoints},
    { "SetPortraitTexture",  Script_SetPortraitTexture},
    {     "HasFullControl",      Script_HasFullControl},
    {     "GetComboPoints",      Script_GetComboPoints},
    {          "IsInGuild",           Script_IsInGuild}
};

static int UnitUpdateHandler(unsigned __int64 guid, unsigned int offset, unsigned int bytes, const void* prevValue, void* param) {
  Script_SendUnitSignal(guid, offset >> 2);
  return 1;
}

static int UnitInventoryUpdate(unsigned __int64 guid, unsigned int offset, unsigned int bytes, const void* prevValue, void* param) {
  CGObject_C *object = ClntObjMgrObjectPtr(guid, __FILE__, __LINE__);
  if (object && guid == ClntObjMgrGetActivePlayer()) {
    CGBag_C         *bag = object->GetBag();
    unsigned __int64 item = bag ? bag->GetItem(offset >> 3) : 0;
    if (*static_cast<const unsigned __int64 *>(prevValue) != item) {
      CGGameUI::UnlockItem(item);
    }
  }
  Script_SendUnitSignal(guid, 183);
  return 1;
}

static int PlayerXPUpdateHandler(unsigned __int64 guid, unsigned int offset, unsigned int bytes, const void* prevValue, void* param) {
  FrameScript_SignalEvent(185);
  return 1;
}

static int __fastcall Script_GetTime(lua_State *L) {
  double currentTime = static_cast<double>(OsGetAsyncTimeMs()) * 0.001;
  lua_pushnumber(L, currentTime);
  return 1;
}

static int __fastcall Script_GetGameTime(lua_State *L) {
  lua_pushnumber(L, g_clientGameTime.m_hour);
  lua_pushnumber(L, g_clientGameTime.m_minute);
  return 2;
}

static void __fastcall PushBoolean(lua_State *L, int value) {
  if (value) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
}

static CGUnit_C *__fastcall GetScriptUnit(lua_State *L, int index) {
  return Script_GetUnitFromName(lua_tostring(L, index));
}

static int __fastcall Script_UnitExists(lua_State *L) {
  unsigned __int64 guid = Script_GetGUIDFromName(lua_tostring(L, 1));
  CGObject_C      *object = ClntObjMgrObjectPtr(guid, __FILE__, __LINE__);

  PushBoolean(L, object && (object->GetType() & TYPE_UNIT) || CGPartyInfo::IsMember(guid));
  return 1;
}

static int __fastcall Script_UnitIsUnit(lua_State *L) {
  if (!lua_isstring(L, 1) || !lua_isstring(L, 2)) {
    luaL_error(L, "Usage: UnitIsUnit(\"unit\", \"otherUnit\")");
  }

  PushBoolean(L, Script_GetGUIDFromName(lua_tostring(L, 1)) == Script_GetGUIDFromName(lua_tostring(L, 2)));
  return 1;
}

static int __fastcall Script_UnitIsPlayer(lua_State *L) {
  unsigned __int64 guid = Script_GetGUIDFromName(lua_tostring(L, 1));
  CGObject_C      *object = ClntObjMgrObjectPtr(guid, __FILE__, __LINE__);

  PushBoolean(L, object && (object->GetType() & TYPE_PLAYER) || CGPartyInfo::IsMember(guid));
  return 1;
}

static int __fastcall Script_UnitIsPartyLeader(lua_State *L) {
  PushBoolean(L, Script_GetGUIDFromName(lua_tostring(L, 1)) == ScriptPartyInfoAccess::m_leader);
  return 1;
}

static int __fastcall Script_UnitInParty(lua_State *L) {
  unsigned __int64 guid = Script_GetGUIDFromName(lua_tostring(L, 1));
  PushBoolean(L, CGPartyInfo::IsMember(guid));
  return 1;
}

static int __fastcall Script_UnitReaction(lua_State *L) {
  CGUnit_C *unit;
  CGUnit_C *otherUnit;

  if (!lua_isstring(L, 1) || !lua_isstring(L, 2)) {
    luaL_error(L, "Usage: UnitReaction(\"unit\", \"otherUnit\")");
  }

  unit = GetScriptUnit(L, 1);
  otherUnit = GetScriptUnit(L, 2);
  if (unit && otherUnit) {
    lua_pushnumber(L, otherUnit->UnitReaction(unit) + 1);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int __fastcall Script_UnitIsEnemy(lua_State *L) {
  CGUnit_C *unit;
  CGUnit_C *otherUnit;

  if (!lua_isstring(L, 1) || !lua_isstring(L, 2)) {
    luaL_error(L, "Usage: UnitIsEnemy(\"unit\", \"otherUnit\")");
  }

  unit = GetScriptUnit(L, 1);
  otherUnit = GetScriptUnit(L, 2);
  PushBoolean(L, unit && otherUnit && otherUnit->UnitReaction(unit) <= UNIT_REACTION_HOSTILE);
  return 1;
}

static int __fastcall Script_UnitIsFriend(lua_State *L) {
  const char      *name1;
  const char      *name2;
  unsigned __int64 unitGUID;
  unsigned __int64 targetGUID;
  CGUnit_C        *unit;
  CGUnit_C        *target;

  if (!lua_isstring(L, 1) || !lua_isstring(L, 2)) {
    luaL_error(L, "Usage: UnitIsFriend(\"unit\", \"otherUnit\")");
  }

  name1 = lua_tostring(L, 1);
  name2 = lua_tostring(L, 2);
  unitGUID = Script_GetGUIDFromName(name1);
  targetGUID = Script_GetGUIDFromName(name2);
  unit = Script_GetUnitFromName(name1);
  target = Script_GetUnitFromName(name2);
  PushBoolean(
      L, unit && target && target->UnitReaction(unit) >= UNIT_REACTION_AMIABLE || !strcmp(name1, "player") && CGPartyInfo::IsMember(targetGUID) ||
             !strcmp(name2, "player") && CGPartyInfo::IsMember(unitGUID)
  );
  return 1;
}

static int __fastcall Script_UnitCanCooperate(lua_State *L) {
  CGUnit_C *unit;
  CGUnit_C *otherUnit;

  if (!lua_isstring(L, 1) || !lua_isstring(L, 2)) {
    luaL_error(L, "Usage: UnitCanCooperate(\"unit\", \"otherUnit\")");
  }

  unit = GetScriptUnit(L, 1);
  otherUnit = GetScriptUnit(L, 2);
  PushBoolean(L, unit && otherUnit && otherUnit->CanCooperate(unit));
  return 1;
}

static int __fastcall Script_UnitIsCharmed(lua_State *L) {
  CGUnit_C *unit = GetScriptUnit(L, 1);
  PushBoolean(L, unit && unit->GetUnitData()->charmedBy != 0);
  return 1;
}

static int __fastcall Script_UnitIsPlusMob(lua_State *L) {
  CGUnit_C *unit = GetScriptUnit(L, 1);
  PushBoolean(L, unit && (unit->GetUnitData()->flags & 0x40) != 0);
  return 1;
}

static int __fastcall Script_IsInGuild(lua_State *L) {
  CGUnit_C            *player = Script_GetUnitFromName("player");
  const unsigned long *playerData = player ? player->GetStorage() : 0;
  PushBoolean(L, playerData && playerData[145] != 0);
  return 1;
}

static void __fastcall NameQueryCallback(int, const unsigned __int64 &guid, void *, bool granted) {
  if (granted) {
    CGGameUI::UnitNameUpdate(guid);
  }
}

static int __fastcall Script_UnitName(lua_State *L) {
  unsigned __int64 guid;
  CGUnit_C        *unit;
  const NameCache *name;

  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: UnitName(\"unit\")");
  }

  guid = Script_GetGUIDFromName(lua_tostring(L, 1));
  unit = Script_GetUnitFromName(lua_tostring(L, 1));
  if (unit) {
    lua_pushstring(L, unit->GetUnitName());
  } else {
    name = g_nameDBCache.GetRecord(guid, guid, NameQueryCallback, 0);
    lua_pushstring(L, name ? name->m_name : "");
  }
  return 1;
}

static int __fastcall Script_UnitXP(lua_State *L) {
  CGUnit_C            *unit;
  const unsigned long *data;

  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: UnitXP(\"unit\")");
  }
  unit = GetScriptUnit(L, 1);
  data = unit && (unit->GetType() & TYPE_PLAYER) ? unit->GetStorage() : 0;
  lua_pushnumber(L, data ? data[332] : 0);
  return 1;
}

static int __fastcall Script_UnitXPMax(lua_State *L) {
  CGUnit_C            *unit;
  const unsigned long *data;

  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: UnitXPMax(\"unit\")");
  }
  unit = GetScriptUnit(L, 1);
  data = unit && (unit->GetType() & TYPE_PLAYER) ? unit->GetStorage() : 0;
  lua_pushnumber(L, data ? data[333] : 0);
  return 1;
}

static int __fastcall Script_UnitHealth(lua_State *L) {
  CGUnit_C *unit;

  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: UnitHealth(\"unit\")");
  }
  unit = GetScriptUnit(L, 1);
  lua_pushnumber(L, unit && !(unit->GetUnitData()->flags & 0x4000) ? unit->GetUnitData()->health : 0);
  return 1;
}

static int __fastcall Script_UnitHealthMax(lua_State *L) {
  CGUnit_C *unit;

  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: UnitHealthMax(\"unit\")");
  }
  unit = GetScriptUnit(L, 1);
  lua_pushnumber(L, unit ? unit->GetUnitData()->maxHealth : 0);
  return 1;
}

static unsigned int __fastcall PowerDisplayMod(unsigned int powerType) {
  return powerType == 1 ? 10 : 1;
}

static int __fastcall Script_UnitMana(lua_State *L) {
  CGUnit_C    *unit;
  unsigned int powerType;

  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: UnitMana(\"unit\")");
  }
  unit = GetScriptUnit(L, 1);
  powerType = unit ? unit->GetUnitData()->displayPower : 0;
  lua_pushnumber(L, unit ? static_cast<unsigned int>(unit->GetUnitData()->power[powerType]) / PowerDisplayMod(powerType) : 0);
  return 1;
}

static int __fastcall Script_UnitManaMax(lua_State *L) {
  CGUnit_C    *unit;
  unsigned int powerType;

  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: UnitManaMax(\"unit\")");
  }
  unit = GetScriptUnit(L, 1);
  powerType = unit ? unit->GetUnitData()->displayPower : 0;
  lua_pushnumber(L, unit ? static_cast<unsigned int>(unit->GetUnitData()->maxPower[powerType]) / PowerDisplayMod(powerType) : 0);
  return 1;
}

static int __fastcall Script_UnitPowerType(lua_State *L) {
  CGUnit_C *unit;

  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: UnitPowerType(\"unit\")");
  }
  unit = GetScriptUnit(L, 1);
  lua_pushnumber(L, unit ? unit->GetUnitData()->displayPower : 0);
  return 1;
}

static int __fastcall Script_UnitIsDead(lua_State *L) {
  CGUnit_C *unit;

  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: UnitIsDead(\"unit\")");
  }
  unit = GetScriptUnit(L, 1);
  PushBoolean(L, unit && (unit->GetUnitData()->health <= 0 || (unit->GetUnitData()->flags & 0x4000) != 0));
  return 1;
}

static int __fastcall Script_UnitIsConnected(lua_State *L) {
  CGUnit_C *unit;

  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: UnitIsDead(\"unit\")");
  }
  unit = GetScriptUnit(L, 1);
  PushBoolean(L, unit != 0);
  return 1;
}

static int __fastcall Script_UnitSex(lua_State *L) {
  CGUnit_C *unit;

  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: UnitSex(\"unit\")");
  }
  unit = GetScriptUnit(L, 1);
  if (unit) {
    lua_pushnumber(L, unit->GetUnitData()->sex);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int __fastcall Script_UnitLevel(lua_State *L) {
  unsigned __int64 guid;
  CGUnit_C        *unit;

  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: UnitLevel(\"unit\")");
  }
  guid = Script_GetGUIDFromName(lua_tostring(L, 1));
  unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (unit) {
    lua_pushnumber(L, unit->GetUnitData()->level);
  } else if (CGPartyInfo::IsMember(guid)) {
    CGPartyInfo::RemoteStats *stats = CGPartyInfo::GetRemoteStats(guid);
    lua_pushnumber(L, stats ? stats->level : 0);
  } else {
    lua_pushnumber(L, 0);
  }
  return 1;
}

static int __fastcall Script_UnitMoney(lua_State *L) {
  CGUnit_C *unit;

  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: UnitMoney(\"unit\")");
  }
  unit = GetScriptUnit(L, 1);
  lua_pushnumber(L, unit ? unit->GetUnitData()->coinage : 0);
  return 1;
}

static int __fastcall Script_UnitRace(lua_State *L) {
  CGUnit_C          *unit;
  const ChrRacesRec *race;

  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: UnitRace(\"unit\")");
  }
  unit = GetScriptUnit(L, 1);
  race = unit ? g_chrRacesDB.GetRecord(unit->GetUnitData()->race) : 0;
  if (race) {
    lua_pushstring(L, race->m_name_lang[CURRENT_LANGUAGE]);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int __fastcall Script_UnitClass(lua_State *L) {
  CGUnit_C            *unit;
  const ChrClassesRec *unitClass;

  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: UnitClass(\"unit\")");
  }
  unit = GetScriptUnit(L, 1);
  unitClass = unit ? g_chrClassesDB.GetRecord(unit->GetUnitData()->classId) : 0;
  if (unitClass) {
    lua_pushstring(L, unitClass->m_name_lang[CURRENT_LANGUAGE]);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int __fastcall Script_UnitResistance(lua_State *L) {
  unsigned int resistance;
  CGUnit_C    *unit;
  int          r = 0;
  int          er = 0;
  int          pos = 0;
  int          neg = 0;

  if (!lua_isstring(L, 1) || !lua_isnumber(L, 2)) {
    luaL_error(L, "Usage: UnitResistance(\"unit\", resistance)");
  }
  resistance = static_cast<unsigned int>(lua_tonumber(L, 2));
  if (resistance > 6) {
    luaL_error(L, "Invalid resistance index in UnitResistance");
  }
  unit = GetScriptUnit(L, 1);
  if (unit && resistance) {
    r = unit->GetUnitData()->resistances[resistance - 1];
    pos = unit->GetUnitData()->resistanceBuffModsPositive[resistance - 1];
    neg = unit->GetUnitData()->resistanceBuffModsNegative[resistance - 1];
    er = unit->GetUnitData()->resistanceItemMods[resistance - 1];
  }
  lua_pushnumber(L, er);
  lua_pushnumber(L, r);
  lua_pushnumber(L, pos);
  lua_pushnumber(L, neg);
  return 4;
}

static int __fastcall Script_UnitStat(lua_State *L) {
  unsigned int stat;
  CGUnit_C    *unit;
  int          base = 0;
  int          value = 0;

  if (!lua_isstring(L, 1) || !lua_isnumber(L, 2)) {
    luaL_error(L, "Usage: UnitStat(\"unit\", resistance)");
  }
  stat = static_cast<unsigned int>(lua_tonumber(L, 2)) - 1;
  if (stat >= 5) {
    luaL_error(L, "Invalid stat index in UnitStat");
  }
  unit = GetScriptUnit(L, 1);
  if (unit) {
    base = unit->GetUnitData()->baseStats[stat];
    value = unit->GetUnitData()->stats[stat];
  }
  lua_pushnumber(L, base);
  lua_pushnumber(L, value - base);
  return 2;
}

static int __fastcall Script_UnitAttackBothHands(lua_State *L) {
  CGUnit_C    *unit;
  int          base[2] = {0, 0};
  int          modifier[2] = {0, 0};
  unsigned int i;

  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: UnitAttackBothHands(\"unit\")");
  }
  unit = GetScriptUnit(L, 1);
  if (unit) {
    base[0] = unit->GetUnitData()->baseStats[0];
    base[1] = base[0];
  }
  for (i = 0; i < 2; ++i) {
    lua_pushnumber(L, base[i]);
    lua_pushnumber(L, modifier[i]);
  }
  return 4;
}

static int __fastcall Script_UnitDamage(lua_State *L) {
  CGUnit_C    *unit;
  unsigned int i;

  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: UnitDamage(\"unit\")");
  }
  unit = GetScriptUnit(L, 1);
  lua_pushnumber(L, unit ? unit->GetUnitData()->minDamage : 0);
  lua_pushnumber(L, unit ? unit->GetUnitData()->maxDamage : 0);
  for (i = 0; i < 6; ++i) {
    lua_pushnumber(L, unit ? unit->GetUnitData()->modDamageDone[i] : 0);
  }
  return 8;
}

static int __fastcall Script_UnitAttackSpeed(lua_State *L) {
  CGUnit_C *unit;

  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: UnitAttackSpeed(\"unit\")");
  }
  unit = GetScriptUnit(L, 1);
  if (unit && (unit->GetType() & TYPE_PLAYER)) {
    lua_pushnumber(L, unit->GetUnitData()->attackRoundBaseTime[0] * 0.001);
  } else {
    lua_pushnumber(L, 0.0);
  }
  lua_pushnil(L);
  return 2;
}

static int __fastcall Script_UnitDefense(lua_State *L) {
  CGUnit_C *unit;
  int       base = 0;
  int       modifier = 0;

  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: UnitDefense(\"unit\")");
  }
  unit = GetScriptUnit(L, 1);
  if (unit) {
    base = unit->GetUnitData()->level * 5;
  }
  lua_pushnumber(L, base);
  lua_pushnumber(L, modifier);
  return 2;
}

static int __fastcall Script_UnitArmor(lua_State *L) {
  CGUnit_C    *unit;
  unsigned int resistance = GetPhysicalDamageClassID();
  int          r = 0;
  int          er = 0;
  int          pos = 0;
  int          neg = 0;

  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: UnitArmor(\"unit\")");
  }
  unit = GetScriptUnit(L, 1);
  if (unit && resistance && resistance <= 6) {
    r = unit->GetUnitData()->resistances[resistance - 1];
    pos = unit->GetUnitData()->resistanceBuffModsPositive[resistance - 1];
    neg = unit->GetUnitData()->resistanceBuffModsNegative[resistance - 1];
    er = unit->GetUnitData()->resistanceItemMods[resistance - 1];
  }
  lua_pushnumber(L, er);
  lua_pushnumber(L, r);
  lua_pushnumber(L, pos);
  lua_pushnumber(L, neg);
  return 4;
}

static int __fastcall Script_UnitCharacterPoints(lua_State *L) {
  CGUnit_C            *unit;
  const unsigned long *data;

  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: UnitCharacterPoints(\"unit\")");
  }
  unit = GetScriptUnit(L, 1);
  data = unit && (unit->GetType() & TYPE_PLAYER) ? unit->GetStorage() : 0;
  lua_pushnumber(L, data ? data[439] : 0);
  lua_pushnumber(L, data ? data[440] : 0);
  return 2;
}

static void __fastcall PortraitQueryCallback(int, const unsigned __int64 &, void *, bool granted) {
  if (granted) {
    FrameScript_SignalEvent(181);
  }
}

static int __fastcall Script_SetPortraitTexture(lua_State *L) {
  CSimpleTexture  *texture = 0;
  unsigned __int64 guid;
  CGUnit_C        *unit;
  const NameCache *name;

  if (lua_type(L, 1) != LUA_TTABLE) {
    luaL_error(L, "Attempt to find 'this' in non-table object (used '.' instead of ':' ?)");
  }
  lua_rawgeti(L, 1, 0);
  texture = static_cast<CSimpleTexture *>(lua_touserdata(L, -1));
  lua_pop(L, 1);
  ASSERT(texture);
  if (!lua_isstring(L, 2)) {
    luaL_error(L, "Usage: SetPortraitTexture(texture, \"unit\")");
  }

  texture->SetTexture(0);
  guid = Script_GetGUIDFromName(lua_tostring(L, 2));
  unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (unit && (unit->GetType() & TYPE_UNIT)) {
    SetPortraitTexture(texture, unit);
  } else if ((name = g_nameDBCache.GetRecord(guid, guid, PortraitQueryCallback, 0)) != 0) {
    SetPortraitTexture(texture, name->m_race, name->m_sex, guid);
  }
  return 0;
}

static int __fastcall Script_HasFullControl(lua_State *L) {
  CGUnit_C *player = Script_GetUnitFromName("player");
  PushBoolean(L, player && !(player->GetUnitData()->flags & 0x100000) && player->GetUnitData()->health > 0);
  return 1;
}

static int __fastcall Script_GetComboPoints(lua_State *L) {
  CGUnit_C *player = Script_GetUnitFromName("player");
  if (player && player->GetUnitData()->comboTarget == CGGameUI::GetLockedTarget()) {
    lua_pushnumber(L, player->GetUnitData()->comboPoints);
  } else {
    lua_pushnumber(L, 0.0);
  }
  return 1;
}

const char *g_scriptEvents[0x177];

void __fastcall ScriptEventsInitialize() {
  g_scriptEvents[136] = "UNIT_RESISTANCE";
  g_scriptEvents[137] = "UNIT_RESISTANCE";
  g_scriptEvents[138] = "UNIT_RESISTANCE";
  g_scriptEvents[139] = "UNIT_RESISTANCE";
  g_scriptEvents[140] = "UNIT_RESISTANCE";
  g_scriptEvents[141] = "UNIT_RESISTANCE";
  g_scriptEvents[148] = "UNIT_RESISTANCE_BUFF_MODS_POSITIVE";
  g_scriptEvents[149] = "UNIT_RESISTANCE_BUFF_MODS_POSITIVE";
  g_scriptEvents[150] = "UNIT_RESISTANCE_BUFF_MODS_POSITIVE";
  g_scriptEvents[151] = "UNIT_RESISTANCE_BUFF_MODS_POSITIVE";
  g_scriptEvents[152] = "UNIT_RESISTANCE_BUFF_MODS_POSITIVE";
  g_scriptEvents[153] = "UNIT_RESISTANCE_BUFF_MODS_POSITIVE";
  g_scriptEvents[154] = "UNIT_RESISTANCE_BUFF_MODS_NEGATIVE";
  g_scriptEvents[155] = "UNIT_RESISTANCE_BUFF_MODS_NEGATIVE";
  g_scriptEvents[156] = "UNIT_RESISTANCE_BUFF_MODS_NEGATIVE";
  g_scriptEvents[157] = "UNIT_RESISTANCE_BUFF_MODS_NEGATIVE";
  g_scriptEvents[158] = "UNIT_RESISTANCE_BUFF_MODS_NEGATIVE";
  g_scriptEvents[159] = "UNIT_RESISTANCE_BUFF_MODS_NEGATIVE";
  g_scriptEvents[160] = "UNIT_RESISTANCE_ITEM_MODS";
  g_scriptEvents[161] = "UNIT_RESISTANCE_ITEM_MODS";
  g_scriptEvents[162] = "UNIT_RESISTANCE_ITEM_MODS";
  g_scriptEvents[163] = "UNIT_RESISTANCE_ITEM_MODS";
  g_scriptEvents[164] = "UNIT_RESISTANCE_ITEM_MODS";
  g_scriptEvents[165] = "UNIT_RESISTANCE_ITEM_MODS";
  g_scriptEvents[29] = "UNIT_STATS";
  g_scriptEvents[30] = "UNIT_STATS";
  g_scriptEvents[31] = "UNIT_STATS";
  g_scriptEvents[32] = "UNIT_STATS";
  g_scriptEvents[33] = "UNIT_STATS";
  g_scriptEvents[114] = "UNIT_DAMAGE_DONE_MODS";
  g_scriptEvents[115] = "UNIT_DAMAGE_DONE_MODS";
  g_scriptEvents[116] = "UNIT_DAMAGE_DONE_MODS";
  g_scriptEvents[117] = "UNIT_DAMAGE_DONE_MODS";
  g_scriptEvents[118] = "UNIT_DAMAGE_DONE_MODS";
  g_scriptEvents[119] = "UNIT_DAMAGE_DONE_MODS";
  g_scriptEvents[16] = "UNIT_HEALTH";
  g_scriptEvents[17] = "UNIT_MANA";
  g_scriptEvents[18] = "UNIT_RAGE";
  g_scriptEvents[19] = "UNIT_FOCUS";
  g_scriptEvents[20] = "UNIT_ENERGY";
  g_scriptEvents[21] = "UNIT_MAXHEALTH";
  g_scriptEvents[22] = "UNIT_MAXMANA";
  g_scriptEvents[23] = "UNIT_MAXRAGE";
  g_scriptEvents[24] = "UNIT_MAXFOCUS";
  g_scriptEvents[25] = "UNIT_MAXENERGY";
  g_scriptEvents[28] = "UNIT_DISPLAYPOWER";
  g_scriptEvents[27] = "UNIT_FACTION";
  g_scriptEvents[26] = "UNIT_LEVEL";
  g_scriptEvents[49] = "UNIT_MONEY";
  g_scriptEvents[113] = "UNIT_AURASTATE";
  g_scriptEvents[147] = "UNIT_DAMAGE";
  g_scriptEvents[134] = "UNIT_ATTACK_SPEED";
  g_scriptEvents[135] = "UNIT_ATTACK_SPEED";
  g_scriptEvents[12] = "UNIT_COMBO_TARGET";
  g_scriptEvents[176] = "UNIT_COMBO_POINTS";
  g_scriptEvents[178] = "UNIT_COMBAT";
  g_scriptEvents[179] = "UNIT_SPELLMISS";
  g_scriptEvents[180] = "UNIT_NAME_UPDATE";
  g_scriptEvents[181] = "UNIT_PORTRAIT_UPDATE";
  g_scriptEvents[182] = "UNIT_MODEL_CHANGED";
  g_scriptEvents[183] = "UNIT_INVENTORY_CHANGED";
  g_scriptEvents[184] = "ITEM_LOCK_CHANGED";
  g_scriptEvents[185] = "PLAYER_XP_UPDATE";
  g_scriptEvents[186] = "PLAYER_REGEN_DISABLED";
  g_scriptEvents[187] = "PLAYER_REGEN_ENABLED";
  g_scriptEvents[188] = "PLAYER_AURAS_CHANGED";
  g_scriptEvents[189] = "PLAYER_ENTER_COMBAT";
  g_scriptEvents[190] = "PLAYER_LEAVE_COMBAT";
  g_scriptEvents[191] = "PLAYER_TARGET_CHANGED";
  g_scriptEvents[192] = "PLAYER_PET_CHANGED";
  g_scriptEvents[193] = "PLAYER_CONTROL_LOST";
  g_scriptEvents[194] = "PLAYER_CONTROL_GAINED";
  g_scriptEvents[195] = "PLAYER_LEVEL_UP";
  g_scriptEvents[196] = "ZONE_CHANGED";
  g_scriptEvents[197] = "MINIMAP_ZONE_CHANGED";
  g_scriptEvents[198] = "MINIMAP_UPDATE_ZOOM";
  g_scriptEvents[199] = "SCREENSHOT_SUCCEEDED";
  g_scriptEvents[200] = "SCREENSHOT_FAILED";
  g_scriptEvents[201] = "ACTIONBAR_SHOWGRID";
  g_scriptEvents[202] = "ACTIONBAR_HIDEGRID";
  g_scriptEvents[203] = "ACTIONBAR_PAGE_CHANGED";
  g_scriptEvents[204] = "ACTIONBAR_SLOT_CHANGED";
  g_scriptEvents[205] = "ACTIONBAR_UPDATE_STATE";
  g_scriptEvents[206] = "ACTIONBAR_UPDATE_USABLE";
  g_scriptEvents[207] = "ACTIONBAR_UPDATE_COOLDOWN";
  g_scriptEvents[208] = "UPDATE_BONUS_ACTIONBAR";
  g_scriptEvents[209] = "PARTY_MEMBERS_CHANGED";
  g_scriptEvents[210] = "PARTY_LEADER_CHANGED";
  g_scriptEvents[211] = "PARTY_MEMBER_ENABLE";
  g_scriptEvents[212] = "PARTY_MEMBER_DISABLE";
  g_scriptEvents[213] = "PARTY_LOOT_METHOD_CHANGED";
  g_scriptEvents[214] = "SYSMSG";
  g_scriptEvents[215] = "UI_ERROR_MESSAGE";
  g_scriptEvents[216] = "UI_INFO_MESSAGE";
  g_scriptEvents[217] = "CHAT_MSG_SAY";
  g_scriptEvents[218] = "CHAT_MSG_PARTY";
  g_scriptEvents[219] = "CHAT_MSG_GUILD";
  g_scriptEvents[220] = "CHAT_MSG_OFFICER";
  g_scriptEvents[221] = "CHAT_MSG_YELL";
  g_scriptEvents[222] = "CHAT_MSG_WHISPER";
  g_scriptEvents[223] = "CHAT_MSG_WHISPER_INFORM";
  g_scriptEvents[224] = "CHAT_MSG_EMOTE";
  g_scriptEvents[225] = "CHAT_MSG_TEXT_EMOTE";
  g_scriptEvents[226] = "CHAT_MSG_SYSTEM";
  g_scriptEvents[227] = "CHAT_MSG_MONSTER_SAY";
  g_scriptEvents[228] = "CHAT_MSG_MONSTER_YELL";
  g_scriptEvents[229] = "CHAT_MSG_MONSTER_EMOTE";
  g_scriptEvents[230] = "CHAT_MSG_CHANNEL";
  g_scriptEvents[231] = "CHAT_MSG_CHANNEL_JOIN";
  g_scriptEvents[232] = "CHAT_MSG_CHANNEL_LEAVE";
  g_scriptEvents[233] = "CHAT_MSG_CHANNEL_LIST";
  g_scriptEvents[234] = "CHAT_MSG_CHANNEL_NOTICE";
  g_scriptEvents[235] = "CHAT_MSG_CHANNEL_NOTICE_USER";
  g_scriptEvents[236] = "CHAT_MSG_AFK";
  g_scriptEvents[237] = "CHAT_MSG_DND";
  g_scriptEvents[238] = "CHAT_MSG_COMBAT_LOG";
  g_scriptEvents[239] = "CHAT_MSG_IGNORED";
  g_scriptEvents[240] = "CHAT_MSG_SKILL";
  g_scriptEvents[241] = "CHAT_MSG_LOOT";
  g_scriptEvents[242] = "LANGUAGE_LIST_CHANGED";
  g_scriptEvents[243] = "TIME_PLAYED_MSG";
  g_scriptEvents[244] = "SPELLS_CHANGED";
  g_scriptEvents[245] = "CURRENT_SPELL_CAST_CHANGED";
  g_scriptEvents[246] = "SPELL_UPDATE_COOLDOWN";
  g_scriptEvents[247] = "CHARACTER_POINTS_CHANGED";
  g_scriptEvents[248] = "SKILL_LINES_CHANGED";
  g_scriptEvents[249] = "ITEM_PUSH";
  g_scriptEvents[250] = "LOOT_OPENED";
  g_scriptEvents[251] = "LOOT_SLOT_CLEARED";
  g_scriptEvents[252] = "LOOT_CLOSED";
  g_scriptEvents[253] = "PLAYER_ENTERING_WORLD";
  g_scriptEvents[254] = "PLAYER_LEAVING_WORLD";
  g_scriptEvents[255] = "PLAYER_ALIVE";
  g_scriptEvents[256] = "PLAYER_DEAD";
  g_scriptEvents[257] = "PLAYER_CAMPING";
  g_scriptEvents[258] = "PLAYER_QUITING";
  g_scriptEvents[259] = "PLAYER_STAND";
  g_scriptEvents[260] = "PLAYER_SIT";
  g_scriptEvents[262] = "PARTY_INVITE_REQUEST";
  g_scriptEvents[263] = "PARTY_INVITE_CANCEL";
  g_scriptEvents[264] = "GUILD_INVITE_REQUEST";
  g_scriptEvents[265] = "GUILD_INVITE_CANCEL";
  g_scriptEvents[266] = "TRADE_REQUEST";
  g_scriptEvents[267] = "TRADE_REQUEST_CANCEL";
  g_scriptEvents[261] = "RESURRECT_REQUEST";
  g_scriptEvents[268] = "LOOT_BIND_CONFIRM";
  g_scriptEvents[269] = "EQUIP_BIND_CONFIRM";
  g_scriptEvents[270] = "AUTOEQUIP_BIND_CONFIRM";
  g_scriptEvents[271] = "DELETE_ITEM_CONFIRM";
  g_scriptEvents[272] = "CURSOR_UPDATE";
  g_scriptEvents[273] = "ITEM_TEXT_BEGIN";
  g_scriptEvents[274] = "ITEM_TEXT_TRANSLATION";
  g_scriptEvents[275] = "ITEM_TEXT_READY";
  g_scriptEvents[276] = "ITEM_TEXT_CLOSED";
  g_scriptEvents[277] = "QUEST_GREETING";
  g_scriptEvents[278] = "QUEST_DETAIL";
  g_scriptEvents[279] = "QUEST_PROGRESS";
  g_scriptEvents[280] = "QUEST_COMPLETE";
  g_scriptEvents[281] = "QUEST_FINISHED";
  g_scriptEvents[282] = "QUEST_ITEM_UPDATE";
  g_scriptEvents[283] = "TAXIMAP_OPENED";
  g_scriptEvents[284] = "TAXIMAP_CLOSED";
  g_scriptEvents[285] = "QUEST_LOG_UPDATE";
  g_scriptEvents[286] = "TRAINER_SHOW";
  g_scriptEvents[287] = "TRAINER_UPDATE";
  g_scriptEvents[288] = "TRAINER_CLOSED";
  g_scriptEvents[289] = "CVAR_UPDATE";
  g_scriptEvents[290] = "TRADE_SKILL_SHOW";
  g_scriptEvents[291] = "TRADE_SKILL_UPDATE";
  g_scriptEvents[292] = "TRADE_SKILL_CLOSE";
  g_scriptEvents[293] = "MERCHANT_SHOW";
  g_scriptEvents[294] = "MERCHANT_UPDATE";
  g_scriptEvents[295] = "MERCHANT_CLOSED";
  g_scriptEvents[296] = "TRADE_SHOW";
  g_scriptEvents[297] = "TRADE_CLOSED";
  g_scriptEvents[298] = "TRADE_UPDATE";
  g_scriptEvents[299] = "TRADE_ACCEPT_UPDATE";
  g_scriptEvents[300] = "TRADE_TARGET_ITEM_CHANGED";
  g_scriptEvents[301] = "TRADE_PLAYER_ITEM_CHANGED";
  g_scriptEvents[302] = "TRADE_MONEY_CHANGED";
  g_scriptEvents[303] = "PLAYER_TRADE_MONEY";
  g_scriptEvents[304] = "BAG_OPEN";
  g_scriptEvents[305] = "BAG_UPDATE";
  g_scriptEvents[306] = "BAG_CLOSED";
  g_scriptEvents[307] = "BAG_UPDATE_COOLDOWN";
  g_scriptEvents[308] = "SOULSTONE_RECEIVED";
  g_scriptEvents[309] = "LOCALPLAYER_PET_RENAMED";
  g_scriptEvents[310] = "UNIT_ATTACK";
  g_scriptEvents[311] = "UNIT_DEFENSE";
  g_scriptEvents[312] = "PET_ATTACK_START";
  g_scriptEvents[313] = "PET_ATTACK_STOP";
  g_scriptEvents[314] = "UPDATE_MOUSEOVER_UNIT";
  g_scriptEvents[315] = "SPELLCAST_START";
  g_scriptEvents[316] = "SPELLCAST_STOP";
  g_scriptEvents[317] = "SPELLCAST_FAILED";
  g_scriptEvents[318] = "SPELLCAST_INTERRUPTED";
  g_scriptEvents[319] = "SPELLCAST_DELAYED";
  g_scriptEvents[320] = "SPELLCAST_CHANNEL_START";
  g_scriptEvents[321] = "SPELLCAST_CHANNEL_UPDATE";
  g_scriptEvents[322] = "CLEAR_TOOLTIP";
  g_scriptEvents[323] = "TOOLTIP_ADD_MONEY";
  g_scriptEvents[324] = "PLAYER_GUILD_UPDATE";
  g_scriptEvents[325] = "QUEST_ACCEPT_CONFIRM";
  g_scriptEvents[326] = "PLAYERBANKSLOTS_CHANGED";
  g_scriptEvents[327] = "BANKFRAME_OPENED";
  g_scriptEvents[328] = "BANKFRAME_CLOSED";
  g_scriptEvents[329] = "PLAYERBANKBAGSLOTS_CHANGED";
  g_scriptEvents[330] = "FRIENDLIST_UPDATE";
  g_scriptEvents[331] = "IGNORELIST_UPDATE";
  g_scriptEvents[332] = "PLAYER_LOGOUT_FAILED";
  g_scriptEvents[334] = "PET_BAR_UPDATE_COOLDOWN";
  g_scriptEvents[333] = "PET_BAR_UPDATE";
  g_scriptEvents[335] = "PET_BAR_SHOWGRID";
  g_scriptEvents[336] = "PET_BAR_HIDEGRID";
  g_scriptEvents[337] = "MINIMAP_PING";
  g_scriptEvents[338] = "CHAT_MSG_COMBAT_LOG_ENEMY";
  g_scriptEvents[339] = "CHAT_MSG_COMBAT_LOG_SELF";
  g_scriptEvents[340] = "CHAT_MSG_COMBAT_LOG_PARTY";
  g_scriptEvents[341] = "CHAT_MSG_COMBAT_LOG_ERROR";
  g_scriptEvents[342] = "CHAT_MSG_COMBAT_LOG_MISC_INFO";
  g_scriptEvents[343] = "CRAFT_SHOW";
  g_scriptEvents[344] = "CRAFT_UPDATE";
  g_scriptEvents[345] = "CRAFT_CLOSE";
  g_scriptEvents[346] = "MIRROR_TIMER_START";
  g_scriptEvents[347] = "MIRROR_TIMER_PAUSE";
  g_scriptEvents[348] = "MIRROR_TIMER_STOP";
  g_scriptEvents[349] = "WORLD_MAP_UPDATE";
  g_scriptEvents[350] = "AUTOFOLLOW_BEGIN";
  g_scriptEvents[351] = "AUTOFOLLOW_END";
  g_scriptEvents[353] = "CINEMATIC_START";
  g_scriptEvents[354] = "CINEMATIC_STOP";
  g_scriptEvents[355] = "UPDATE_FACTION";
  g_scriptEvents[356] = "CLOSE_WORLD_MAP";
  g_scriptEvents[357] = "OPEN_TABARD_FRAME";
  g_scriptEvents[358] = "CLOSE_TABARD_FRAME";
  g_scriptEvents[360] = "SHOW_COMPARE_TOOLTIP";
  g_scriptEvents[359] = "TABARD_CANSAVE_CHANGED";
  g_scriptEvents[361] = "GUILD_REGISTRAR_SHOW";
  g_scriptEvents[362] = "GUILD_REGISTRAR_CLOSED";
  g_scriptEvents[363] = "DUEL_REQUESTED";
  g_scriptEvents[364] = "DUEL_OUTOFBOUNDS";
  g_scriptEvents[365] = "DUEL_INBOUNDS";
  g_scriptEvents[366] = "DUEL_FINISHED";
  g_scriptEvents[367] = "TUTORIAL_TRIGGER";
  g_scriptEvents[368] = "PET_DISMISS_START";
  g_scriptEvents[369] = "UPDATE_BINDINGS";
  g_scriptEvents[370] = "UPDATE_SHAPESHIFT_FORMS";
  g_scriptEvents[371] = "WHO_LIST_UPDATE";
  g_scriptEvents[372] = "UPDATE_LFG";
  g_scriptEvents[373] = "PETITION_SHOW";
  g_scriptEvents[374] = "PETITION_CLOSED";
}

void __fastcall ScriptEventsRegisterFunctions() {
  unsigned int i;

  for (i = 0; i < sizeof(s_SystemFunctions) / sizeof(s_SystemFunctions[0]); ++i) {
    FrameScript_RegisterFunction(s_SystemFunctions[i].name, s_SystemFunctions[i].method);
  }

  for (i = 0; i < sizeof(s_UnitFunctions) / sizeof(s_UnitFunctions[0]); ++i) {
    FrameScript_RegisterFunction(s_UnitFunctions[i].name, s_UnitFunctions[i].method);
  }
}

void __fastcall ScriptEventsUnregisterFunctions() {
  unsigned int i;

  for (i = 0; i < sizeof(s_SystemFunctions) / sizeof(s_SystemFunctions[0]); ++i) {
    FrameScript_UnregisterFunction(s_SystemFunctions[i].name);
  }

  for (i = 0; i < sizeof(s_UnitFunctions) / sizeof(s_UnitFunctions[0]); ++i) {
    FrameScript_UnregisterFunction(s_UnitFunctions[i].name);
  }
}

void __fastcall ScriptEventsRegisterUnit(CGUnit_C* unit) {
  if (!unit) {
    return;
  }

  unsigned __int64 guid = unit->GetGUID();
  unsigned int     unitOffset = CGPlayer_C::OffsetOf(ID_UNIT);
  for (unsigned int event = 0; event < 178; ++event) {
    if (!g_scriptEvents[event]) {
      continue;
    }

    unsigned int bytes = 4;
    if (event == 0 || event == 10 || event == 134 || event == 135) {
      bytes = 8;
    } else if (event >= 29 && event <= 33) {
      bytes = 40;
    } else if ((event >= 114 && event <= 119) || (event >= 136 && event <= 141) || (event >= 148 && event <= 165)) {
      bytes = 24;
    }

    ClntObjMgrSetObjMirrorHandler(
        guid, unitOffset + 4 * event, bytes, UnitUpdateHandler, 0, HANDLER_PRIORITY_NORMAL
    );
    Script_SendUnitSignal(guid, event);
  }

  if (unit->GetType() & TYPE_PLAYER) {
    unsigned int playerOffset = CGPlayer_C::OffsetOf(ID_PLAYER);
    for (unsigned int offset = 0; offset <= 176; offset += 8) {
      ClntObjMgrSetObjMirrorHandler(
          guid, playerOffset + offset, 8, UnitInventoryUpdate, 0, HANDLER_PRIORITY_NORMAL
      );
    }
    Script_SendUnitSignal(guid, 183);

    if (guid == ClntObjMgrGetActivePlayer()) {
      ClntObjMgrSetObjMirrorHandler(
          guid, playerOffset + 592, 4, PlayerXPUpdateHandler, 0, HANDLER_PRIORITY_NORMAL
      );
      ClntObjMgrSetObjMirrorHandler(
          guid, playerOffset + 596, 4, PlayerXPUpdateHandler, 0, HANDLER_PRIORITY_NORMAL
      );
      FrameScript_SignalEvent(185);
    }
  }
}

void __fastcall ScriptEventsUnregisterUnit(CGUnit_C* unit) {
  if (!unit) {
    return;
  }

  unsigned __int64 guid = unit->GetGUID();
  unsigned int     unitOffset = CGPlayer_C::OffsetOf(ID_UNIT);
  for (unsigned int event = 0; event < 178; ++event) {
    if (g_scriptEvents[event]) {
      ClntObjMgrUnsetObjMirrorHandler(guid, unitOffset + 4 * event, UnitUpdateHandler, 0);
    }
  }

  if (unit->GetType() & TYPE_PLAYER) {
    unsigned int playerOffset = CGPlayer_C::OffsetOf(ID_PLAYER);
    for (unsigned int offset = 0; offset <= 176; offset += 8) {
      ClntObjMgrUnsetObjMirrorHandler(guid, playerOffset + offset, UnitInventoryUpdate, 0);
    }
    if (guid == ClntObjMgrGetActivePlayer()) {
      ClntObjMgrUnsetObjMirrorHandler(guid, playerOffset + 592, PlayerXPUpdateHandler, 0);
      ClntObjMgrUnsetObjMirrorHandler(guid, playerOffset + 596, PlayerXPUpdateHandler, 0);
    }
  }
}
