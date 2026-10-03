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

#include "Ui/GameUI.h"
#include "Ui/GuildRegistrar.h"
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

DWORDLONG          CGGuildRegistrar::m_registrar;
PetitionVendorItem CGGuildRegistrar::m_petition;

void CGGuildRegistrar::EnterWorld() {
  memset(&m_petition, 0, sizeof(m_petition));
}

void CGGuildRegistrar::LeaveWorld() {
  CloseRegistrar();
}

void CGGuildRegistrar::SetRegistrar(DWORDLONG registrar, const PetitionVendorItem *petition) {
  CGGameUI::SetInteractTarget(registrar, 0.0f);
  m_registrar = registrar;
  m_petition = *petition;
  FrameScript_SignalEvent(361);
}

void CGGuildRegistrar::CloseRegistrar() {
  if (m_registrar) {
    FrameScript_SignalEvent(362);
    CGGameUI::ClearInteractTarget(m_registrar);
    m_registrar = 0;
  }
}

UINT CGGuildRegistrar::GetGuildCharterCost() {
  return m_petition.m_price;
}

void CGGuildRegistrar::BuyGuildCharter(LPCSTR guildName) {
  if (!guildName || !*guildName || !m_registrar) {
    return;
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    if (player->GetGuildID()) {
      CGGameUI::DisplayError(GERR_ALREADY_IN_GUILD);
      return;
    }
    if (player->GetMoney() < static_cast<UINT>(m_petition.m_price)) {
      CGGameUI::DisplayError(GERR_NOT_ENOUGH_MONEY);
      return;
    }
    CGPetition petition;
    SStrCopy(petition.m_title, guildName, sizeof(petition.m_title));
    petition.m_muid = m_petition.m_muid;
    player->BuyPetition(m_registrar, &petition);
  }
}

static int Script_CloseGuildRegistrar(lua_State *) {
  CGGuildRegistrar::CloseRegistrar();
  return 0;
}

static int Script_GetGuildCharterCost(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGGuildRegistrar::GetGuildCharterCost()));
  return 1;
}

static int Script_BuyGuildCharter(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: BuyGuildCharter(guildName)");
    return 0;
  }
  LPCSTR name = lua_tostring(L, 1);
  VALIDATE_NAME_RESULT result = ValidateGuildName(CURRENT_LANGUAGE, name);
  if (result == NAME_SUCCESS) {
    CGGuildRegistrar::BuyGuildCharter(name);
    lua_pushnumber(L, 1.0);
  } else {
    switch (result) {
      case NAME_NO_NAME: CGGameUI::DisplayError(GERR_GUILD_ENTER_NAME); break;
      case NAME_TOO_SHORT: CGGameUI::DisplayError(GERR_GUILD_NAME_TOO_SHORT); break;
      case NAME_STARTS_WITH_GRAVE:
      case NAME_TWO_GRAVES:
      case NAME_INVALID_CHARACTER:
      case NAME_FAILURE: CGGameUI::DisplayError(GERR_GUILD_NAME_INVALID); break;
      case NAME_MIXED_LANGUAGES: CGGameUI::DisplayError(GERR_GUILD_NAME_MIXED_LANGUAGES); break;
      case NAME_PROFANE: CGGameUI::DisplayError(GERR_GUILD_NAME_PROFANE); break;
      case NAME_RESERVED: CGGameUI::DisplayError(GERR_GUILD_NAME_RESERVED); break;
      default: break;
    }
    lua_pushnil(L);
  }
  return 1;
}

static int Script_TurnInGuildCharter(lua_State *) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return 0;
  }
  player->TurnInGuildCharter();
  return 0;
}

static int Script_GetTabardInfo(lua_State *) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->TalkToTabardVendor(CGGuildRegistrar::GetRegistrar());
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

void GuildRegistrarRegisterScriptFunctions() {
  for (UINT i = 0; i < 5; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void GuildRegistrarUnregisterScriptFunctions() {
  for (UINT i = 0; i < 5; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
