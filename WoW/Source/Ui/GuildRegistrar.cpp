#include "Ui/GameUI.h"
#include "Game/ValidateName.h"
#include "DB/DBClient/DBCacheInstances.h"
#include "Object/Petition.h"
#include "Object/ObjectClient/Item_C.h"
#include "Object/ObjectClient/Player_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

#include <Base/CDataStore.h>
#include <FrameScript/FrameScript.h>
#include <Net/NetClient/NetClient.h>
#include <lauxlib.h>
#include <lua.h>
#include <string.h>

struct PetitionVendorItem {
  unsigned int m_muid;
  unsigned int m_itemID;
  unsigned int m_itemDisplayID;
  int          m_price;
  int          m_flags;
};

class CGGuildRegistrar {
 public:
  static void __fastcall             EnterWorld();
  static void __fastcall             LeaveWorld();
  static void __fastcall             SetRegistrar(unsigned __int64 registrar, PetitionVendorItem *petition);
  static void __fastcall             CloseRegistrar();
  static unsigned __int64 __fastcall GetRegistrar();
  static unsigned int __fastcall     GetGuildCharterCost();
  static void __fastcall             BuyGuildCharter(const char *guildName);

 protected:
  static unsigned __int64   m_registrar;
  static PetitionVendorItem m_petition;
};

unsigned __int64   CGGuildRegistrar::m_registrar;
PetitionVendorItem CGGuildRegistrar::m_petition;

void __fastcall CGGuildRegistrar::EnterWorld() {
  memset(&m_petition, 0, sizeof(m_petition));
}

void __fastcall CGGuildRegistrar::LeaveWorld() {
  CloseRegistrar();
}

void __fastcall CGGuildRegistrar::SetRegistrar(unsigned __int64 registrar, PetitionVendorItem *petition) {
  CGGameUI::SetInteractTarget(registrar, 0.0f);
  m_registrar = registrar;
  m_petition = *petition;
  FrameScript_SignalEvent(361);
}

unsigned __int64 __fastcall CGGuildRegistrar::GetRegistrar() {
  return m_registrar;
}

void __fastcall CGGuildRegistrar::CloseRegistrar() {
  if (m_registrar) {
    FrameScript_SignalEvent(362);
    CGGameUI::ClearInteractTarget(m_registrar);
    m_registrar = 0;
  }
}

unsigned int __fastcall CGGuildRegistrar::GetGuildCharterCost() {
  return m_petition.m_price;
}

void __fastcall CGGuildRegistrar::BuyGuildCharter(const char *guildName) {
  if (!guildName || !*guildName || !m_registrar) {
    return;
  }
  CGPetition petition;
  SStrCopy(petition.m_title, guildName, sizeof(petition.m_title));
  petition.m_muid = m_petition.m_muid;
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->BuyPetition(m_registrar, &petition);
  }
}

static int __fastcall Script_CloseGuildRegistrar(lua_State *__formal) {
  CGGuildRegistrar::CloseRegistrar();
  return 0;
}

static int __fastcall Script_GetGuildCharterCost(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGGuildRegistrar::GetGuildCharterCost()));
  return 1;
}

static int __fastcall Script_BuyGuildCharter(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    return luaL_error(L, "Usage: BuyGuildCharter(guildName)");
  }
  const char *name = lua_tostring(L, 1);
  if (ValidateCharacterName(CURRENT_LANGUAGE, name) == NAME_SUCCESS) {
    CGGuildRegistrar::BuyGuildCharter(name);
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int __fastcall Script_TurnInGuildCharter(lua_State *__formal) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return 0;
  }
  player->TurnInGuildCharter();
  return 0;
}

static int __fastcall Script_GetTabardInfo(lua_State *__formal) {
  unsigned __int64 registrar = CGGuildRegistrar::GetRegistrar();
  if (registrar) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (player) {
      player->TalkToTabardVendor(registrar);
    }
  }
  CGGuildRegistrar::CloseRegistrar();
  return 0;
}

static FrameScript_Method s_ScriptFunctions[5] = {
    {"CloseGuildRegistrar", Script_CloseGuildRegistrar},
    {"GetGuildCharterCost", Script_GetGuildCharterCost},
    {    "BuyGuildCharter",     Script_BuyGuildCharter},
    { "TurnInGuildCharter",  Script_TurnInGuildCharter},
    {      "GetTabardInfo",       Script_GetTabardInfo}
};

void __fastcall GuildRegistrarRegisterScriptFunctions() {
  for (unsigned int i = 0; i < 5; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void __fastcall GuildRegistrarUnregisterScriptFunctions() {
  for (unsigned int i = 0; i < 5; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
