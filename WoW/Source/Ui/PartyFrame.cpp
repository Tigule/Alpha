#include <Base/Base.h>
#include <Frame/CSimpleTop.h>
#include <Frame/CSimpleModel.h>
#include <WowConst.h>
#include <MapDefs.h>
#include <WorldClient/World.h>
#include "Net/NetClient/NetClient.h"
#include "Object/ObjectClient/Unit_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "SoundInterface/SoundInterface.h"
#include "UIUtil/InputControl.h"
#include "WorldFrame.h"
#include "GameUI.h"

#include "PartyFrame.h"

#include "Object/ObjectClient/Player_C.h"

#include "GameUI.h"

#include "DB/DBClient/DBCacheInstances.h"
#include "Object/ObjectClient/Unit_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

#include "Base/CDataStore.h"
#include "FrameScript/FrameScript.h"
#include <FrameXML/LoadXML.h>
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

static BOOL OnLFGResponse(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg) {
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
  msg.Put(MSG_LOOKING_FOR_GROUP);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPartyInfo::LeaveWorld() {
  ClientServices_ClearMessageHandler(MSG_LOOKING_FOR_GROUP);
}

void CGPartyInfo::ShutdownGame() {
}

BOOL CGPartyInfo::IsMember(const DWORDLONG &guid) {
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
  if (name && *name) {
    CGUnit_C *player = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (player && !SStrCmpI(name, player->GetUnitName(), 0x7FFFFFFF)) {
      return ClntObjMgrGetActivePlayer();
    }
    for (UINT i = 0; i < 4; ++i) {
      if (m_members[i]) {
        const NameCache *nc = g_nameDBCache.GetRecord(m_members[i], 0, 0, 0);
        if (nc && !SStrCmpI(name, nc->m_name, 0x7FFFFFFF)) {
          return m_members[i];
        }
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
      UINT index;
      for (index = 0; index < 4; ++index) {
        if (m_members[index] == guid) {
          break;
        }
      }
      if (index < 4) {
        m_leaderIndex = index;
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
  for (UINT index = 0; index < 4; ++index) {
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
      m_members[i] = 0;
      FrameScript_SignalEvent(209);
      CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
      if (unit) {
        unit->UpdatePlayerNameColor();
      }
    }
  }
}

void CGPartyInfo::EnableMember(DWORDLONG guid, int enable) {
  if (guid) {
    UINT index;
    for (index = 0; index < 4; ++index) {
      if (m_members[index] == guid) {
        break;
      }
    }
    if (index != 4) {
      CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
      if (unit) {
        if (enable) {
          unit->RegisterScript();
        } else {
          unit->UnregisterScript();
        }
      }
      FrameScript_SignalEvent(enable ? 211 : 212, "%d", index + 1);
    }
  }
}

UINT CGPartyInfo::NumMembers() {
  UINT count = 0;
  for (UINT i = 0; i < 4; ++i) {
    if (m_members[i]) {
      ++count;
    }
  }
  return count;
}

void CGPartyInfo::SetLootMethod(LOOT_METHOD method, DWORDLONG master) {
  if (m_lootMethod != method) {
    m_lootMethod = method;
    switch (method) {
      case LOOT_METHOD_FREEFORALL:
        CGGameUI::DisplayError(GERR_SET_LOOT_FREEFORALL);
        break;
      case LOOT_METHOD_ROUNDROBIN:
        CGGameUI::DisplayError(GERR_SET_LOOT_ROUNDROBIN);
        break;
      case LOOT_METHOD_MASTERLOOTER:
        CGGameUI::DisplayError(GERR_SET_LOOT_MASTER);
        break;
    }
  }

  if (m_lootMaster != master) {
    if (master && method == LOOT_METHOD_MASTERLOOTER) {
      const NameCache *name = g_nameDBCache.GetRecord(master, 0, 0, 0);
      if (name) {
        CGGameUI::DisplayError(GERR_NEW_LOOT_MASTER_S, name->m_name);
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
    msg.Put(CMSG_SET_LOOKING_FOR_GROUP);
    msg.Put(looking);
    msg.Finalize();
    ClientServices_Send(&msg);
    m_lookingForGroup = looking;
  }
}

static int Script_GetNumPartyMembers(lua_State *L) {
  lua_pushnumber(L, CGPartyInfo::NumMembers());
  return 1;
}

static int Script_GetPartyMember(lua_State *L) {
  if (lua_isnumber(L, 1)) {
    UINT index = (int)lua_tonumber(L, 1) - 1;
    if (index < 4) {
      if (CGPartyInfo::GetMember(index)) {
        lua_pushnumber(L, 1.0);
      } else {
        lua_pushnil(L);
      }
      return 1;
    }
  }
  luaL_error(L, "Usage: GetPartyMember(1-4)");
  return 0;
}

static int Script_GetPartyLeaderIndex(lua_State *L) {
  lua_pushnumber(L, CGPartyInfo::GetLeaderIndex() + 1);
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
  if (!CGPartyInfo::NumMembers()) {
    lua_pushstring(L, "freeforall");
    lua_pushnil(L);
    return 2;
  }
  switch (CGPartyInfo::GetLootMethod()) {
    case LOOT_METHOD_FREEFORALL:
      lua_pushstring(L, "freeforall");
      break;
    case LOOT_METHOD_ROUNDROBIN:
      lua_pushstring(L, "roundrobin");
      break;
    case LOOT_METHOD_MASTERLOOTER:
      lua_pushstring(L, "master");
      break;
    default:
      lua_pushstring(L, "ERROR!");
      break;
  }
  DWORDLONG master = CGPartyInfo::GetMasterLooter();
  if (!master) {
    lua_pushnil(L);
    return 2;
  }
  if (master == ClntObjMgrGetActivePlayer()) {
    lua_pushnumber(L, 0.0);
    return 2;
  }
  for (int i = 0; i < 4; ++i) {
    if (master == CGPartyInfo::GetMember(i)) {
      lua_pushnumber(L, i + 1);
      return 2;
    }
  }
  lua_pushnil(L);
  return 2;
}

static int Script_SetLootMethod(lua_State *L) {
  if (!CGPartyInfo::GetMember(0)) {
    CGGameUI::DisplayError(GERR_NOT_IN_GROUP);
    return 0;
  }
  if (CGPartyInfo::GetLeader() != ClntObjMgrGetActivePlayer()) {
    CGGameUI::DisplayError(GERR_NOT_LEADER);
    return 0;
  }
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: SetLootMethod(\"method\" [,master])");
    return 0;
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
    luaL_error(L, "Invalid loot method");
    return 0;
  }
  DWORDLONG master = 0;
  if (method == LOOT_METHOD_MASTERLOOTER) {
    LPCSTR name;
    if (lua_isstring(L, 2) && (name = lua_tostring(L, 2)) != 0 && *name) {
      master = CGPartyInfo::GetMemberByName(name);
      if (!master) {
        master = CGGameUI::ClosestObjectMatch(name, TYPE_PLAYER);
        if (!master || !CGPartyInfo::IsMember(master)) {
          CGGameUI::DisplayError(GERR_TARGET_NOT_IN_GROUP_S, name);
          return 0;
        }
      }
    } else {
      CGGameUI::DisplayError(GERR_SPECIFY_MASTER_LOOTER);
      return 0;
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
  int looking = 0;
  if (lua_isnumber(L, 1)) {
    looking = lua_tonumber(L, 1);
  } else if (lua_isstring(L, 1)) {
    looking = StringToBOOL(lua_tostring(L, 1));
  }
  CGPartyInfo::SetLookingForGroup(looking);
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
