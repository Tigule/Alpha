#include <Base/Base.h>
#include <WowConst.h>
#include <MapDefs.h>

#include "PartyFrame.h"

#include "Object/ObjectClient/Player_C.h"

#include "GameUI.h"

#include "DB/DBClient/DBCacheInstances.h"
#include "Object/ObjectClient/Unit_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

#include "Base/CDataStore.h"
#include "FrameScript/FrameScript.h"
#include <lauxlib.h>
#include <lua.h>

#include <string.h>

DWORDLONG                CGPartyInfo::m_leader;
int                      CGPartyInfo::m_leaderIndex = -1;
DWORDLONG                CGPartyInfo::m_members[4];
CGPartyInfo::RemoteStats CGPartyInfo::m_remoteStats[4];
LOOT_METHOD              CGPartyInfo::m_lootMethod;
DWORDLONG                CGPartyInfo::m_lootMaster;
int                      CGPartyInfo::m_lookingForGroup;

static int OnLFGResponse(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg) {
  int looking;
  msg->Get(looking);
  CGPartyInfo::SetLookingForGroup(looking);
  FrameScript_SignalEvent(372);
  return 1;
}

void CGPartyInfo::InitializeGame() {
  for (UINT i = 0; i < 4; ++i) {
    m_members[i] = 0;
  }

  m_leader = 0;
  m_leaderIndex = -1;
  m_lookingForGroup = 0;
}

void CGPartyInfo::EnterWorld() {
  ClientServices_SetMessageHandler(MSG_LOOKING_FOR_GROUP, OnLFGResponse, 0);

  CDataStore msg;
  msg.Put(static_cast<UINT>(MSG_LOOKING_FOR_GROUP));
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPartyInfo::LeaveWorld() {
  ClientServices_ClearMessageHandler(MSG_LOOKING_FOR_GROUP);
}

void CGPartyInfo::ShutdownGame() {
}

int CGPartyInfo::IsMember(const DWORDLONG &guid) {
  if (!guid) {
    return 0;
  }

  if (guid == ClntObjMgrGetActivePlayer()) {
    return 1;
  }

  for (UINT i = 0; i < 4; ++i) {
    if (m_members[i] == guid) {
      return 1;
    }
  }

  return 0;
}

DWORDLONG CGPartyInfo::GetMemberByName(LPCSTR name) {
  for (UINT i = 0; i < 4; ++i) {
    if (m_members[i]) {
      const NameCache *entry = g_nameDBCache.GetRecord(m_members[i], m_members[i], 0, 0);
      if (entry && !SStrCmpI(entry->m_name, name, 0x7FFFFFFF)) {
        return m_members[i];
      }
    }
  }
  return 0;
}

void CGPartyInfo::SetLeader(DWORDLONG guid) {
  if (guid != m_leader) {
    m_leader = guid;
    m_leaderIndex = -1;
    if (guid) {
      for (UINT index = 0; index < 4; ++index) {
        if (m_members[index] == guid) {
          m_leaderIndex = index;
          break;
        }
      }
    }
    FrameScript_SignalEvent(210);
  }
}

void CGPartyInfo::AddMember(DWORDLONG guid, int connected) {
  if (!guid) {
    return;
  }

  UINT index;
  for (index = 0; index < 4; ++index) {
    if (!m_members[index]) {
      break;
    }
  }
  if (index != 4) {
    m_members[index] = guid;
    m_remoteStats[index].connected = connected;
    FrameScript_SignalEvent(209);

    CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
    if (unit) {
      unit->UpdatePlayerNameColor();
    }
  }
}

void CGPartyInfo::RemoveAll() {
  UINT index;
  for (index = 0; index < 4; ++index) {
    CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(m_members[index], __FILE__, __LINE__));
    if (unit) {
      unit->UnregisterScript();
    }
  }

  for (index = 0; index < 4; ++index) {
    if (m_members[index]) {
      CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(m_members[index], __FILE__, __LINE__));
      if (unit) {
        unit->UpdatePlayerNameColor();
      }
    }
    m_members[index] = 0;
  }
  m_leader = 0;
  m_leaderIndex = 0;
  FrameScript_SignalEvent(209);
}

void CGPartyInfo::RemoveActivePlayer(DWORDLONG guid) {
  for (UINT i = 0; i < 4; ++i) {
    if (m_members[i] == guid) {
      memmove(&m_members[i], &m_members[i + 1], (3 - i) * sizeof(m_members[0]));
      memmove(&m_remoteStats[i], &m_remoteStats[i + 1], (3 - i) * sizeof(m_remoteStats[0]));
      m_members[3] = 0;
      memset(&m_remoteStats[3], 0, sizeof(m_remoteStats[3]));
      FrameScript_SignalEvent(209);
      return;
    }
  }
}

void CGPartyInfo::EnableMember(DWORDLONG guid, int enable) {
  RemoteStats *stats = GetRemoteStats(guid);
  if (stats && stats->connected != enable) {
    stats->connected = enable;
    FrameScript_SignalEvent(209);
  }
}

UINT CGPartyInfo::NumMembers() {
  UINT count = 0;
  while (count < 4 && m_members[count]) {
    ++count;
  }
  return count;
}

void CGPartyInfo::SetLootMethod(LOOT_METHOD method, DWORDLONG master) {
  if (m_lootMethod != method) {
    m_lootMethod = method;
    switch (method) {
      case LOOT_METHOD_FREEFORALL:
        CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(209));
        break;
      case LOOT_METHOD_ROUNDROBIN:
        CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(210));
        break;
      case LOOT_METHOD_MASTERLOOTER:
        CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(211));
        break;
    }
  }

  if (m_lootMaster != master) {
    if (master && method == LOOT_METHOD_MASTERLOOTER) {
      const NameCache *name = g_nameDBCache.GetRecord(master, 0, 0, 0);
      if (name) {
        CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(212), name->m_name);
      }
    }
    m_lootMaster = master;
  }
  FrameScript_SignalEvent(213);
}

CGPartyInfo::RemoteStats *CGPartyInfo::GetRemoteStats(DWORDLONG guid) {
  if (!guid) {
    return 0;
  }

  for (UINT index = 0; index < 4; ++index) {
    if (m_members[index] == guid) {
      return &m_remoteStats[index];
    }
  }

  return 0;
}

void CGPartyInfo::SetLookingForGroup(int looking) {
  if (m_lookingForGroup != looking) {
    CDataStore msg;
    msg.Put(static_cast<UINT>(CMSG_SET_LOOKING_FOR_GROUP));
    msg.Put(looking);
    msg.Finalize();
    ClientServices_Send(&msg);
    m_lookingForGroup = looking;
  }
}

static int Script_GetNumPartyMembers(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGPartyInfo::NumMembers()));
  return 1;
}

static int Script_GetPartyMember(lua_State *L) {
  if (!lua_isnumber(L, 1) || static_cast<UINT>(lua_tonumber(L, 1)) - 1 >= 4) {
    return luaL_error(L, "Usage: GetPartyMember(index)");
  }
  UINT index = static_cast<UINT>(lua_tonumber(L, 1)) - 1;
  if (CGPartyInfo::GetMember(index)) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_GetPartyLeaderIndex(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGPartyInfo::GetLeaderIndex() + 1));
  return 1;
}

static int Script_IsPartyLeader(lua_State *L) {
  if (CGPartyInfo::GetLeader() && CGPartyInfo::GetLeader() == ClntObjMgrGetActivePlayer()) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_LeaveParty(lua_State *L) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->LeaveGroup();
  }
  return 0;
}

static int Script_GetLootMethod(lua_State *L) {
  static LPCSTR methods[3] = {"freeforall", "roundrobin", "master"};
  LOOT_METHOD   method = CGPartyInfo::GetLootMethod();
  lua_pushstring(L, method < LOOT_METHOD_MAX ? methods[method] : "freeforall");
  if (method == LOOT_METHOD_MASTERLOOTER) {
    DWORDLONG master = CGPartyInfo::GetMasterLooter();
    int       index = -1;
    for (UINT i = 0; i < 4; ++i) {
      if (CGPartyInfo::GetMember(i) == master) {
        index = i + 1;
        break;
      }
    }
    lua_pushnumber(L, static_cast<double>(index));
  } else {
    lua_pushnil(L);
  }
  return 2;
}

static int Script_SetLootMethod(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    return luaL_error(L, "Usage: SetLootMethod(method, master)");
  }
  LPCSTR      string = lua_tostring(L, 1);
  LOOT_METHOD method;
  if (!SStrCmpI(string, "freeforall", 0x7FFFFFFF)) {
    method = LOOT_METHOD_FREEFORALL;
  } else if (!SStrCmpI(string, "roundrobin", 0x7FFFFFFF)) {
    method = LOOT_METHOD_ROUNDROBIN;
  } else if (!SStrCmpI(string, "master", 0x7FFFFFFF)) {
    method = LOOT_METHOD_MASTERLOOTER;
  } else {
    return luaL_error(L, "Invalid loot method");
  }
  DWORDLONG master = 0;
  if (method == LOOT_METHOD_MASTERLOOTER && lua_isnumber(L, 2)) {
    UINT index = static_cast<UINT>(lua_tonumber(L, 2)) - 1;
    if (index < 4) {
      master = CGPartyInfo::GetMember(index);
    }
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->SetLootMethod(method, master);
  }
  return 0;
}

static int Script_GetLookingForGroup(lua_State *L) {
  if (CGPartyInfo::IsLookingForGroup()) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_SetLookingForGroup(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: SetLookingForGroup(looking)");
  }
  CGPartyInfo::SetLookingForGroup(lua_tonumber(L, 1) != 0.0);
  return 0;
}

static FrameScript_Method s_ScriptFunctions[9] = {
    { "GetNumPartyMembers",  Script_GetNumPartyMembers},
    {     "GetPartyMember",      Script_GetPartyMember},
    {"GetPartyLeaderIndex", Script_GetPartyLeaderIndex},
    {      "IsPartyLeader",       Script_IsPartyLeader},
    {         "LeaveParty",          Script_LeaveParty},
    {      "GetLootMethod",       Script_GetLootMethod},
    {      "SetLootMethod",       Script_SetLootMethod},
    { "GetLookingForGroup",  Script_GetLookingForGroup},
    { "SetLookingForGroup",  Script_SetLookingForGroup}
};

void PartyInfoRegisterScriptFunctions() {
  for (UINT i = 0; i < 9; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void PartyInfoUnregisterScriptFunctions() {
  for (UINT i = 0; i < 9; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
