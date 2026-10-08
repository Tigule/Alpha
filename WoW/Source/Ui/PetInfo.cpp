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

#include "PetInfo.h"

#include "ClassTrainerFrame.h"
#include "GameUI.h"

#include "DB/DBClient/AutoCode/SpellRec.h"
#include "DB/DBClient/AutoCode/SpellIconRec.h"
#include "Object/ObjectClient/Player_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "SoundInterface/SoundInterface.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

#include "Base/CDataStore.h"
#include "FrameScript/FrameScript.h"
#include "Os/OsTime.h"

#include <string.h>
#include <storm.h>

#include <lauxlib.h>
#include <lua.h>

int Spell_C_GetSpellCooldown(int spell, BOOL isPet, UINT *duration, DWORD *startTime, UINT *enable);

static const char s_petModeTokens[3][32] = {"PASSIVE", "DEFENSIVE", "AGGRESSIVE"};
static const char s_petOrdersTokens[4][32] = {"WAIT", "FOLLOW", "ATTACK", "DISMISS"};

DWORDLONG CGPetInfo::m_pet;
UINT      CGPetInfo::m_petMode;
PetAction CGPetInfo::m_actions[10];
DWORD     CGPetInfo::m_expirationTime;

void CGPetInfo::InitializeGame() {
}

void CGPetInfo::EnterWorld() {
  FrameScript_SignalEvent(192);
  SetPet(0, 0);
}

void CGPetInfo::LeaveWorld() {
}

void CGPetInfo::ShutdownGame() {
}

void CGPetInfo::SetPet(DWORDLONG pet, DWORD expirationTime) {
  m_pet = pet;
  if (expirationTime) {
    m_expirationTime = OsGetAsyncTimeMs() + expirationTime;
  } else {
    m_expirationTime = 0;
  }

  FrameScript_SignalEvent(333);
  if (CGClassTrainer::GetTrainer() && CGClassTrainer::GetTrainerType() == TRAINER_TYPE_PET) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (player) {
      player->TalkToTrainer(CGClassTrainer::GetTrainer());
    }
  }
}

void CGPetInfo::SetPetModeAndOrders(UINT petMode) {
  m_petMode = petMode;
}

void CGPetInfo::SetPetMode(UINT mode) {
  FATALASSERT(mode < 256);
  m_petMode = mode | m_petMode & 0xFFFFFF00;
  FrameScript_SignalEvent(333);
}

void CGPetInfo::SetPetOrders(UINT orders) {
  FATALASSERT(orders < 256);
  m_petMode = orders << 8 | m_petMode & 0xFF;
  FrameScript_SignalEvent(333);
}

void CGPetInfo::ClearActions() {
  memset(m_actions, 0, sizeof(m_actions));
}

void CGPetInfo::SetAction(UINT index, PetAction &action, int save) {
  FATALASSERT(index < (sizeof(m_actions) / sizeof(m_actions[0])));

  if (action.GetAction() == m_actions[index].GetAction()) {
    return;
  }
  if (action.GetActionType() == 1) {
    const SpellRec *spell = g_spellDB.GetRecord(action.GetActionID());
    if (!spell || (spell->m_attributes & 0x40)) {
      return;
    }
  }

  int oldSlot = -1;
  for (UINT slot = 0; slot < sizeof(m_actions) / sizeof(m_actions[0]); ++slot) {
    if (m_actions[slot].GetActionTypeAndID() == action.GetActionTypeAndID() && slot != index) {
      m_actions[slot].SetAction(0);
      oldSlot = slot;
      break;
    }
  }

  if (oldSlot < 0 && (m_actions[index].GetActionType() == 6 || m_actions[index].GetActionType() == 7)) {
    return;
  }

  if (save && oldSlot < 0 && action.GetActionType() == 1) {
    action.SetAutocastEnabled(1);
  }
  if (oldSlot >= 0) {
    m_actions[oldSlot] = m_actions[index];
  }
  m_actions[index] = action;

  if (save) {
    CDataStore msg;
    msg.Put(CMSG_PET_SET_ACTION);
    msg.Put(m_pet);
    if (oldSlot >= 0) {
      msg.Put(static_cast<UINT>(oldSlot));
      msg.Put(m_actions[oldSlot].GetAction());
    }
    msg.Put(index);
    msg.Put(m_actions[index].GetAction());
    msg.Finalize();
    ClientServices_Send(&msg);
    FrameScript_SignalEvent(333);
  }
}

void CGPetInfo::ToggleAutocast(UINT index) {
  PetAction action = m_actions[index];
  if (action.GetAutocastAllowed()) {
    action.SetAutocastEnabled(!action.GetAutocastEnabled());
    m_actions[index] = action;

    CDataStore msg;
    msg.Put(CMSG_PET_SET_ACTION);
    msg.Put(m_pet);
    msg.Put(index);
    msg.Put(m_actions[index].GetAction());
    msg.Finalize();
    ClientServices_Send(&msg);
    FrameScript_SignalEvent(333);
  }
}

void CGPetInfo::PutActionInSlot(PetAction &action, UINT slot) {
  for (UINT i = 0; i < 10; ++i) {
    if (m_actions[i].GetActionTypeAndID() == action.GetActionTypeAndID()) {
      action.SetAutocastAllowed(m_actions[i].GetAutocastAllowed());
      action.SetAutocastEnabled(m_actions[i].GetAutocastEnabled());
      break;
    }
  }
  SetAction(slot, action, 1);
}

LPCSTR CGPetInfo::GetModeToken(UINT id) {
  FATALASSERT(id < (sizeof(s_petModeTokens) / sizeof(s_petModeTokens[0])));
  return s_petModeTokens[id];
}

LPCSTR CGPetInfo::GetOrdersToken(UINT id) {
  FATALASSERT(id < (sizeof(s_petOrdersTokens) / sizeof(s_petOrdersTokens[0])));
  return s_petOrdersTokens[id];
}

void CGPetInfo::ShowGrid() {
  FrameScript_SignalEvent(335);
}

void CGPetInfo::HideGrid() {
  FrameScript_SignalEvent(336);
}

void CGPetInfo::UpdateCooldowns() {
  FrameScript_SignalEvent(334);
}

void CGPetInfo::SendPetAction(const PetAction &action, const DWORDLONG &target) {
  DWORDLONG actionTarget = target ? target : CGGameUI::GetLockedTarget();
  switch (action.GetActionType()) {
    case 6:
      SetPetMode(action.GetActionID());
      break;
    case 7:
      switch (action.GetActionID()) {
        case 0:
        case 1:
          SetPetOrders(action.GetActionID());
          break;
        case 2: {
          CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
          if (player) {
            CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(actionTarget, __FILE__, __LINE__));
            if (!unit) {
              CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(141));
              return;
            }
            if (!player->CanAttack(unit)) {
              CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(142));
              return;
            }
          }
          break;
        }
      }
      break;
  }

  CDataStore msg;
  msg.Put(CMSG_PET_ACTION);
  msg.Put(m_pet);
  msg.Put(action.GetAction());
  msg.Put(actionTarget);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPetInfo::PetPassiveMode() {
  PetAction action(0x06000000);
  SendPetAction(action, 0);
}

void CGPetInfo::PetDefensiveMode() {
  PetAction action(0x06000001);
  SendPetAction(action, 0);
}

void CGPetInfo::PetAggressiveMode() {
  PetAction action(0x06000002);
  SendPetAction(action, 0);
}

void CGPetInfo::PetWait() {
  PetAction action(0x07000000);
  SendPetAction(action, 0);
}

void CGPetInfo::PetFollow() {
  PetAction action(0x07000001);
  SendPetAction(action, 0);
}

void CGPetInfo::PetAttackTarget(const DWORDLONG &targetGUID) {
  PetAction action(0x07000002);
  SendPetAction(action, targetGUID);
}

void CGPetInfo::PetDismiss() {
  PetAction action(0x07000003);
  SendPetAction(action, 0);
  FrameScript_SignalEvent(368, "%d", 10000);
}

void CGPetInfo::PetAbandon() {
  CDataStore msg;
  msg.Put(CMSG_PET_ABANDON);
  msg.Put(m_pet);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void CGPetInfo::PetRename(LPCSTR newName) {
  CGUnit_C *pet = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(m_pet, __FILE__, __LINE__));
  if (!pet) {
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(217));
    return;
  }
  DWORDLONG player = ClntObjMgrGetActivePlayer();
  if (pet->GetSummonedBy() != player) {
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(218));
    return;
  }
  if (!(pet->GetUnitFlags() & 0x10)) {
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(219));
    return;
  }
  if (!newName || !*newName) {
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(220));
    return;
  }

  char petName[48];
  SStrPrintf(petName, sizeof(petName), "%s", newName);
  petName[sizeof(petName) - 1] = 0;
  CDataStore msg;
  msg.Put(CMSG_PET_RENAME);
  msg.Put(m_pet);
  msg.PutString(petName);
  msg.Finalize();
  ClientServices_Send(&msg);
}

static int Script_PetHasActionBar(lua_State *L) {
  if (CGPetInfo::GetPet())
    lua_pushnumber(L, 1.0);
  else
    lua_pushnil(L);
  return 1;
}

static int Script_GetPetActionInfo(lua_State *L) {
  if (!lua_isnumber(L, 1))
    return luaL_error(L, "Usage: GetPetActionInfo(index)");
  UINT             index = static_cast<UINT>(lua_tonumber(L, 1)) - 1;
  const PetAction *action = CGPetInfo::GetAction(index);
  if (!CGPetInfo::GetPet() || !action || !action->GetAction()) {
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnil(L);
    return 7;
  }
  switch (action->GetActionType()) {
  case 1:
  case 2:
  case 3:
  case 4:
  case 5: {
    const SpellRec     *spell = g_spellDB.GetRecord(action->GetActionID());
    const SpellIconRec *icon = spell ? g_spellIconDB.GetRecord(spell->m_spellIconID) : 0;
    if (spell)
      lua_pushstring(L, spell->m_name_lang[CURRENT_LANGUAGE]);
    else
      lua_pushnil(L);
    if (spell)
      lua_pushstring(L, spell->m_nameSubtext_lang[CURRENT_LANGUAGE]);
    else
      lua_pushnil(L);
    if (icon)
      lua_pushstring(L, icon->m_textureFilename);
    else
      lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnil(L);
    break;
  }
  case 6: {
    char buf[64];
    SStrPrintf(buf, sizeof(buf), "PET_MODE_%s", CGPetInfo::GetModeToken(action->GetActionID()));
    lua_pushstring(L, buf);
    lua_pushnil(L);
    SStrPrintf(buf, sizeof(buf), "PET_%s_TEXTURE", CGPetInfo::GetModeToken(action->GetActionID()));
    lua_pushstring(L, buf);
    lua_pushnumber(L, 1.0);
    if (CGPetInfo::GetPetMode() == action->GetActionID())
      lua_pushnumber(L, 1.0);
    else
      lua_pushnil(L);
    break;
  }
  case 7: {
    char buf[64];
    SStrPrintf(buf, sizeof(buf), "PET_ACTION_%s", CGPetInfo::GetOrdersToken(action->GetActionID()));
    lua_pushstring(L, buf);
    lua_pushnil(L);
    SStrPrintf(buf, sizeof(buf), "PET_%s_TEXTURE", CGPetInfo::GetOrdersToken(action->GetActionID()));
    lua_pushstring(L, buf);
    lua_pushnumber(L, 1.0);
    if (CGPetInfo::GetPetOrders() == action->GetActionID())
      lua_pushnumber(L, 1.0);
    else
      lua_pushnil(L);
    break;
  }
  default:
    FATALASSERT(!"Unknown pet action type");
    break;
  }
  if (action->GetAutocastAllowed())
    lua_pushnumber(L, 1.0);
  else
    lua_pushnil(L);
  if (action->GetAutocastEnabled())
    lua_pushnumber(L, 1.0);
  else
    lua_pushnil(L);
  return 7;
}

static int Script_GetPetActionCooldown(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetPetActionCooldown(index)");
    return 0;
  }
  const PetAction *action = CGPetInfo::GetAction(static_cast<int>(lua_tonumber(L, 1)) - 1);
  DWORD            startTime = 0;
  UINT             duration = 0;
  UINT             enable = 0;
  if (action && action->GetAction()) {
    int type = action->GetActionType();
    if (type > 0 && type <= 5) {
      Spell_C_GetSpellCooldown(action->GetActionID(), 1, &duration, &startTime, &enable);
    }
  }
  lua_pushnumber(L, static_cast<double>(startTime) * 0.001);
  lua_pushnumber(L, static_cast<double>(duration) * 0.001);
  lua_pushnumber(L, static_cast<double>(enable));
  return 3;
}

static int Script_PickupPetAction(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: PickupPetAction(index)");
    return 0;
  }
  UINT index = static_cast<UINT>(lua_tonumber(L, 1)) - 1;
  if (index > 10) {
    luaL_error(L, "Invalid slot in PickupPetAction");
    return 0;
  }
  if (!index) {
    return 0;
  }

  UINT cursorSpell = CGGameUI::m_cursorItemType == UICURSOR_PET_SPELL ? CGGameUI::GetCursorSpell() : 0;
  UINT cursorAction = CGGameUI::m_cursorPetAction;
  CGGameUI::ClearCursor(1);
  if (cursorSpell) {
    CGPetInfo::PutSpellInSlot(cursorSpell, index);
    return 0;
  }
  if (cursorAction) {
    CGPetInfo::PutActionInSlot(cursorAction, index);
    return 0;
  }

  const PetAction *action = CGPetInfo::GetAction(index);
  if (!action) {
    return 0;
  }
  switch (action->GetActionType()) {
    case 1:
      CGGameUI::SetCursorSpell(action->GetActionID(), 1);
      break;
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
      CGGameUI::SetCursorPetAction(*action);
      break;
  }
  return 0;
}

static int Script_TogglePetAutocast(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: TogglePetAutocast(index)");
    return 0;
  }
  UINT index = static_cast<UINT>(lua_tonumber(L, 1)) - 1;
  if (index > 10) {
    luaL_error(L, "Invalid slot in TogglePetAutocast");
    return 0;
  }
  UINT cursorSpell = CGGameUI::m_cursorItemType == UICURSOR_PET_SPELL ? CGGameUI::GetCursorSpell() : 0;
  UINT cursorAction = CGGameUI::m_cursorPetAction;
  CGGameUI::ClearCursor(1);
  if (index) {
    if (cursorSpell) {
      CGPetInfo::PutSpellInSlot(cursorSpell, index);
      return 0;
    }
    if (cursorAction) {
      CGPetInfo::PutActionInSlot(cursorAction, index);
      return 0;
    }
  }
  CGPetInfo::ToggleAutocast(index);
  return 0;
}

static int Script_CastPetAction(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: CastPetAction(index)");
    return 0;
  }
  UINT index = static_cast<UINT>(lua_tonumber(L, 1)) - 1;
  if (index > 10) {
    luaL_error(L, "Invalid slot in CastPetAction");
    return 0;
  }
  UINT cursorSpell = CGGameUI::m_cursorItemType == UICURSOR_PET_SPELL ? CGGameUI::GetCursorSpell() : 0;
  UINT cursorAction = CGGameUI::m_cursorPetAction;
  CGGameUI::ClearCursor(1);
  if (index) {
    if (cursorSpell) {
      CGPetInfo::PutSpellInSlot(cursorSpell, index);
      return 0;
    }
    if (cursorAction) {
      CGPetInfo::PutActionInSlot(cursorAction, index);
      return 0;
    }
  }
  const PetAction *action = CGPetInfo::GetAction(index);
  if (action) {
    CGPetInfo::SendPetAction(*action, 0);
    SndInterfacePlayInterfaceSound("GAMEABILITYACTIVATE");
  }
  return 0;
}

static int Script_PetPassiveMode(lua_State *) {
  CGPetInfo::PetPassiveMode();
  return 0;
}

static int Script_PetDefensiveMode(lua_State *) {
  CGPetInfo::PetDefensiveMode();
  return 0;
}

static int Script_PetAggressiveMode(lua_State *) {
  CGPetInfo::PetAggressiveMode();
  return 0;
}

static int Script_PetWait(lua_State *) {
  CGPetInfo::PetWait();
  return 0;
}

static int Script_PetFollow(lua_State *) {
  CGPetInfo::PetFollow();
  return 0;
}

static int Script_PetAttack(lua_State *) {
  CGPetInfo::PetAttackTarget(CGGameUI::GetLockedTarget());
  return 0;
}

static int Script_PetAbandon(lua_State *) {
  CGPetInfo::PetAbandon();
  return 0;
}

static int Script_PetDismiss(lua_State *) {
  CGPetInfo::PetDismiss();
  return 0;
}

static int Script_PetRename(lua_State *L) {
  CGPetInfo::PetRename(lua_tostring(L, 1));
  return 0;
}

static int Script_PetCanBeAbandoned(lua_State *L) {
  CGUnit_C *pet = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(), __FILE__, __LINE__));
  if (pet) {
    DWORDLONG player = ClntObjMgrGetActivePlayer();
    if (pet->GetSummonedBy() == player && (pet->GetUnitFlags() & 0x20)) {
      lua_pushnumber(L, 1.0);
      return 1;
    }
  }
  lua_pushnil(L);
  return 1;
}

static int Script_PetCanBeRenamed(lua_State *L) {
  CGUnit_C *pet = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(), __FILE__, __LINE__));
  if (pet) {
    DWORDLONG player = ClntObjMgrGetActivePlayer();
    if (pet->GetSummonedBy() == player && (pet->GetUnitFlags() & 0x10)) {
      lua_pushnumber(L, 1.0);
      return 1;
    }
  }
  lua_pushnil(L);
  return 1;
}

static int Script_GetPetTimeRemaining(lua_State *L) {
  DWORD expiration = CGPetInfo::GetExpirationTime();
  if (expiration) {
    lua_pushnumber(L, static_cast<double>(max(expiration - OsGetAsyncTimeMs(), 0)));
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static FrameScript_Method s_ScriptFunctions[18] = {
    {     "PetHasActionBar",      Script_PetHasActionBar},
    {    "GetPetActionInfo",     Script_GetPetActionInfo},
    {"GetPetActionCooldown", Script_GetPetActionCooldown},
    {     "PickupPetAction",      Script_PickupPetAction},
    {   "TogglePetAutocast",    Script_TogglePetAutocast},
    {       "CastPetAction",        Script_CastPetAction},
    {      "PetPassiveMode",       Script_PetPassiveMode},
    {    "PetDefensiveMode",     Script_PetDefensiveMode},
    {   "PetAggressiveMode",    Script_PetAggressiveMode},
    {             "PetWait",              Script_PetWait},
    {           "PetFollow",            Script_PetFollow},
    {           "PetAttack",            Script_PetAttack},
    {          "PetAbandon",           Script_PetAbandon},
    {          "PetDismiss",           Script_PetDismiss},
    {           "PetRename",            Script_PetRename},
    {   "PetCanBeAbandoned",    Script_PetCanBeAbandoned},
    {     "PetCanBeRenamed",      Script_PetCanBeRenamed},
    { "GetPetTimeRemaining",  Script_GetPetTimeRemaining}
};

void PetInfoRegisterScriptFunctions() {
  for (UINT i = 0; i < 18; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void PetInfoUnregisterScriptFunctions() {
  for (UINT i = 0; i < 18; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
