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

int __fastcall Spell_C_GetSpellCooldown(int spell, int isPet, unsigned int *duration, unsigned long *startTime, unsigned int *enable);

static const char s_petModeTokens[3][32] = {"PASSIVE", "DEFENSIVE", "AGGRESSIVE"};
static const char s_petOrdersTokens[4][32] = {"WAIT", "FOLLOW", "ATTACK", "DISMISS"};

unsigned __int64 CGPetInfo::m_pet;
unsigned int     CGPetInfo::m_petMode;
PetAction        CGPetInfo::m_actions[10];
unsigned long    CGPetInfo::m_expirationTime;

void __fastcall CGPetInfo::InitializeGame() {
}

void __fastcall CGPetInfo::EnterWorld() {
  FrameScript_SignalEvent(192);
  SetPet(0, 0);
}

void __fastcall CGPetInfo::LeaveWorld() {
}

void __fastcall CGPetInfo::ShutdownGame() {
}

void __fastcall CGPetInfo::SetPet(unsigned __int64 pet, unsigned long expirationTime) {
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
      const unsigned __int64 trainer = CGClassTrainer::GetTrainer();
      player->TalkToTrainer(trainer);
    }
  }
}

void __fastcall CGPetInfo::SetPetModeAndOrders(unsigned int petMode) {
  m_petMode = petMode;
}

void __fastcall CGPetInfo::SetPetMode(unsigned int mode) {
  FATALASSERT(mode < 256);
  m_petMode = mode | m_petMode & 0xFFFFFF00;
  FrameScript_SignalEvent(333);
}

void __fastcall CGPetInfo::SetPetOrders(unsigned int orders) {
  FATALASSERT(orders < 256);
  m_petMode = orders << 8 | m_petMode & 0xFF;
  FrameScript_SignalEvent(333);
}

void __fastcall CGPetInfo::ClearActions() {
  memset(m_actions, 0, sizeof(m_actions));
}

void __fastcall CGPetInfo::SetAction(unsigned int index, PetAction &action, int save) {
  FATALASSERT(index < sizeof(m_actions) / sizeof(m_actions[0]));

  unsigned int &rawAction = action;
  unsigned int  actionType = rawAction >> 24 & 0x3F;
  if (rawAction == static_cast<const unsigned int &>(m_actions[index])) {
    return;
  }
  if (actionType == 1) {
    SpellRec *spell = g_spellDB.GetRecord(rawAction & 0xFFFF);
    if (!spell || spell->m_attributes & 0x40) {
      return;
    }
  }

  int oldSlot = -1;
  for (unsigned int slot = 0; slot < sizeof(m_actions) / sizeof(m_actions[0]); ++slot) {
    unsigned int &slotAction = m_actions[slot];
    if ((slotAction & 0x3FFFFFFF) == (rawAction & 0x3FFFFFFF) && slot != index) {
      slotAction = 0;
      oldSlot = slot;
      break;
    }
  }

  unsigned int &currentAction = m_actions[index];
  unsigned int  currentType = currentAction >> 24 & 0x3F;
  if (oldSlot < 0 && (currentType == 6 || currentType == 7)) {
    return;
  }

  if (save && oldSlot < 0 && actionType == 1) {
    rawAction |= 0x40000000;
  }
  if (oldSlot >= 0) {
    static_cast<unsigned int &>(m_actions[oldSlot]) = currentAction;
  }
  currentAction = rawAction;

  if (save) {
    CDataStore msg;
    msg.Put(static_cast<unsigned int>(CMSG_PET_SET_ACTION));
    msg.Put(m_pet);
    if (oldSlot >= 0) {
      msg.Put(oldSlot);
      msg.Put(static_cast<const unsigned int &>(m_actions[oldSlot]));
    }
    msg.Put(index);
    msg.Put(static_cast<const unsigned int &>(m_actions[index]));
    msg.Finalize();
    ClientServices_Send(&msg);
    FrameScript_SignalEvent(333);
  }
}

void __fastcall CGPetInfo::ToggleAutocast(unsigned int index) {
  FATALASSERT(index < 10);
  unsigned int &action = m_actions[index];
  if (static_cast<int>(action) < 0) {
    action ^= 0x40000000;

    CDataStore msg;
    msg.Put(static_cast<unsigned int>(CMSG_PET_SET_ACTION));
    msg.Put(m_pet);
    msg.Put(index);
    msg.Put(action);
    msg.Finalize();
    ClientServices_Send(&msg);
    FrameScript_SignalEvent(333);
  }
}

void __fastcall CGPetInfo::PutActionInSlot(PetAction &action, unsigned int slot) {
  unsigned int rawAction = action;
  for (unsigned int i = 0; i < 10; ++i) {
    unsigned int current = m_actions[i];
    if ((current & 0x3FFFFFFF) == (rawAction & 0x3FFFFFFF)) {
      rawAction = current & 0x80000000 | rawAction & 0x7FFFFFFF;
      rawAction = current & 0x40000000 | rawAction & 0xBFFFFFFF;
      break;
    }
  }
  static_cast<unsigned int &>(action) = rawAction;
  SetAction(slot, action, 1);
}

const char *__fastcall CGPetInfo::GetModeToken(unsigned int id) {
  FATALASSERT(id < sizeof(s_petModeTokens) / sizeof(s_petModeTokens[0]));
  return s_petModeTokens[id];
}

const char *__fastcall CGPetInfo::GetOrdersToken(unsigned int id) {
  FATALASSERT(id < sizeof(s_petOrdersTokens) / sizeof(s_petOrdersTokens[0]));
  return s_petOrdersTokens[id];
}

void __fastcall CGPetInfo::ShowGrid() {
  FrameScript_SignalEvent(335);
}

void __fastcall CGPetInfo::HideGrid() {
  FrameScript_SignalEvent(336);
}

void __fastcall CGPetInfo::UpdateCooldowns() {
  FrameScript_SignalEvent(334);
}

void __fastcall CGPetInfo::SendPetAction(PetAction &action, const unsigned __int64 &target) {
  unsigned __int64 actionTarget = target ? target : CGGameUI::GetLockedTarget();
  unsigned int     rawAction = action;
  unsigned int     actionType = rawAction >> 24 & 0x3F;
  if (actionType == 6) {
    SetPetMode(rawAction & 0xFFFF);
  } else if (actionType == 7) {
    unsigned int id = rawAction & 0xFFFF;
    if (id <= 1) {
      SetPetOrders(id);
    } else if (id == 2) {
      CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(actionTarget, __FILE__, __LINE__));
      if (!unit) {
        CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(141));
        return;
      }
      if (!unit->CanBeTargetted()) {
        CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(142));
        return;
      }
    }
  }

  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_PET_ACTION));
  msg.Put(m_pet);
  msg.Put(rawAction);
  msg.Put(actionTarget);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void __fastcall CGPetInfo::PetPassiveMode() {
  PetAction              action(0x06000000);
  const unsigned __int64 noTarget = 0;
  SendPetAction(action, noTarget);
}

void __fastcall CGPetInfo::PetDefensiveMode() {
  PetAction              action(0x06000001);
  const unsigned __int64 noTarget = 0;
  SendPetAction(action, noTarget);
}

void __fastcall CGPetInfo::PetAggressiveMode() {
  PetAction              action(0x06000002);
  const unsigned __int64 noTarget = 0;
  SendPetAction(action, noTarget);
}

void __fastcall CGPetInfo::PetWait() {
  PetAction              action(0x07000000);
  const unsigned __int64 noTarget = 0;
  SendPetAction(action, noTarget);
}

void __fastcall CGPetInfo::PetFollow() {
  PetAction              action(0x07000001);
  const unsigned __int64 noTarget = 0;
  SendPetAction(action, noTarget);
}

void __fastcall CGPetInfo::PetAttackTarget(const unsigned __int64 &targetGUID) {
  PetAction action(0x07000002);
  SendPetAction(action, targetGUID);
}

void __fastcall CGPetInfo::PetDismiss() {
  PetAction              action(0x07000003);
  const unsigned __int64 noTarget = 0;
  SendPetAction(action, noTarget);
  FrameScript_SignalEvent(368, "%d", 10000);
}

void __fastcall CGPetInfo::PetAbandon() {
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_PET_ABANDON));
  msg.Put(m_pet);
  msg.Finalize();
  ClientServices_Send(&msg);
}

void __fastcall CGPetInfo::PetRename(const char *newName) {
  CGUnit_C *pet = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(m_pet, __FILE__, __LINE__));
  if (!pet) {
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(217));
    return;
  }
  if (pet->GetUnitData()->summonedBy != ClntObjMgrGetActivePlayer()) {
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(218));
    return;
  }
  if (!(pet->GetUnitData()->flags & 0x10)) {
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
  msg.Put(static_cast<unsigned int>(CMSG_PET_RENAME));
  msg.Put(m_pet);
  msg.PutString(petName);
  msg.Finalize();
  ClientServices_Send(&msg);
}

static int __fastcall Script_PetHasActionBar(lua_State *L) {
  if (CGPetInfo::GetPet())
    lua_pushnumber(L, 1.0);
  else
    lua_pushnil(L);
  return 1;
}

static int __fastcall Script_GetPetActionInfo(lua_State *L) {
  if (!lua_isnumber(L, 1))
    return luaL_error(L, "Usage: GetPetActionInfo(index)");
  unsigned int index = static_cast<unsigned int>(lua_tonumber(L, 1)) - 1;
  PetAction   *action = CGPetInfo::GetAction(index);
  unsigned int raw = action ? static_cast<unsigned int &>(*action) : 0;
  unsigned int type = raw >> 24 & 0x3F;
  if (!CGPetInfo::GetPet() || !raw) {
    for (int i = 0; i < 7; ++i)
      lua_pushnil(L);
    return 7;
  }
  if (type >= 1 && type <= 5) {
    SpellRec     *spell = g_spellDB.GetRecord(raw & 0xFFFF);
    SpellIconRec *icon = spell ? g_spellIconDB.GetRecord(spell->m_spellIconID) : 0;
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
  } else {
    char        buf[64];
    const char *token = type == 6 ? CGPetInfo::GetModeToken(raw & 0xFFFF) : CGPetInfo::GetOrdersToken(raw & 0xFFFF);
    SStrPrintf(buf, sizeof(buf), type == 6 ? "PET_MODE_%s" : "PET_ACTION_%s", token);
    lua_pushstring(L, buf);
    lua_pushnil(L);
    SStrPrintf(buf, sizeof(buf), "PET_%s_TEXTURE", token);
    lua_pushstring(L, buf);
    lua_pushnumber(L, 1.0);
    unsigned int current = type == 6 ? CGPetInfo::GetPetMode() : CGPetInfo::GetPetOrders();
    if (current == (raw & 0xFFFF))
      lua_pushnumber(L, 1.0);
    else
      lua_pushnil(L);
  }
  if (static_cast<int>(raw) < 0)
    lua_pushnumber(L, 1.0);
  else
    lua_pushnil(L);
  if (raw & 0x40000000)
    lua_pushnumber(L, 1.0);
  else
    lua_pushnil(L);
  return 7;
}

static int __fastcall Script_GetPetActionCooldown(lua_State *L) {
  if (!lua_isnumber(L, 1))
    return luaL_error(L, "Usage: GetPetActionCooldown(index)");
  PetAction    *action = CGPetInfo::GetAction(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  unsigned int  duration = 0, enable = 0;
  unsigned long startTime = 0;
  if (action) {
    unsigned int raw = *action;
    unsigned int type = raw >> 24 & 0x3F;
    if (raw && type >= 1 && type <= 5)
      Spell_C_GetSpellCooldown(raw & 0xFFFF, 1, &duration, &startTime, &enable);
  }
  lua_pushnumber(L, startTime * 0.001);
  lua_pushnumber(L, duration * 0.001);
  lua_pushnumber(L, static_cast<double>(enable));
  return 3;
}

static int __fastcall Script_PickupPetAction(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: PickupPetAction(index)");
  }
  unsigned int index = static_cast<unsigned int>(lua_tonumber(L, 1)) - 1;
  if (index >= 10) {
    return luaL_error(L, "Invalid slot in PickupPetAction");
  }

  unsigned int cursorAction = CGGameUI::GetCursorVirtualItem(UICURSOR_PET_ACTION);
  unsigned int cursorSpell = CGGameUI::GetCursorVirtualItem(UICURSOR_PET_SPELL);
  CGGameUI::ClearCursor(1);
  if (cursorSpell) {
    CGPetInfo::PutSpellInSlot(cursorSpell, index);
    return 0;
  }
  if (cursorAction) {
    CGPetInfo::PutActionInSlot(cursorAction, index);
    return 0;
  }

  PetAction *action = CGPetInfo::GetAction(index);
  if (!action || !static_cast<unsigned int &>(*action)) {
    return 0;
  }
  unsigned int raw = *action;
  unsigned int type = raw >> 24 & 0x3F;
  if (type == 1) {
    CGGameUI::SetCursorVirtualItem(raw & 0xFFFF, 0, index, UICURSOR_PET_SPELL);
  } else if (type > 1 && type <= 7) {
    CGGameUI::SetCursorVirtualItem(raw, 0, index, UICURSOR_PET_ACTION);
  }
  return 0;
}

static int __fastcall Script_TogglePetAutocast(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: TogglePetAutocast(index)");
  }
  unsigned int index = static_cast<unsigned int>(lua_tonumber(L, 1)) - 1;
  if (index >= 10) {
    return luaL_error(L, "Invalid slot in TogglePetAutocast");
  }
  unsigned int cursorAction = CGGameUI::GetCursorVirtualItem(UICURSOR_PET_ACTION);
  unsigned int cursorSpell = CGGameUI::GetCursorVirtualItem(UICURSOR_PET_SPELL);
  CGGameUI::ClearCursor(1);
  if (cursorSpell) {
    CGPetInfo::PutSpellInSlot(cursorSpell, index);
  } else if (cursorAction) {
    CGPetInfo::PutActionInSlot(cursorAction, index);
  } else {
    CGPetInfo::ToggleAutocast(index);
  }
  return 0;
}

static int __fastcall Script_CastPetAction(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: CastPetAction(index)");
  }
  unsigned int index = static_cast<unsigned int>(lua_tonumber(L, 1)) - 1;
  if (index >= 10) {
    return luaL_error(L, "Invalid slot in CastPetAction");
  }
  unsigned int cursorAction = CGGameUI::GetCursorVirtualItem(UICURSOR_PET_ACTION);
  unsigned int cursorSpell = CGGameUI::GetCursorVirtualItem(UICURSOR_PET_SPELL);
  CGGameUI::ClearCursor(1);
  if (cursorSpell) {
    CGPetInfo::PutSpellInSlot(cursorSpell, index);
  } else if (cursorAction) {
    CGPetInfo::PutActionInSlot(cursorAction, index);
  } else {
    PetAction *action = CGPetInfo::GetAction(index);
    if (action) {
      const unsigned __int64 noTarget = 0;
      CGPetInfo::SendPetAction(*action, noTarget);
      SndInterfacePlayInterfaceSound("GAMEABILITYACTIVATE");
    }
  }
  return 0;
}

static int __fastcall Script_PetPassiveMode(lua_State *__formal) {
  CGPetInfo::PetPassiveMode();
  return 0;
}

static int __fastcall Script_PetDefensiveMode(lua_State *__formal) {
  CGPetInfo::PetDefensiveMode();
  return 0;
}

static int __fastcall Script_PetAggressiveMode(lua_State *__formal) {
  CGPetInfo::PetAggressiveMode();
  return 0;
}

static int __fastcall Script_PetWait(lua_State *__formal) {
  CGPetInfo::PetWait();
  return 0;
}

static int __fastcall Script_PetFollow(lua_State *__formal) {
  CGPetInfo::PetFollow();
  return 0;
}

static int __fastcall Script_PetAttack(lua_State *__formal) {
  CGPetInfo::PetAttackTarget(CGGameUI::GetLockedTarget());
  return 0;
}

static int __fastcall Script_PetAbandon(lua_State *__formal) {
  CGPetInfo::PetAbandon();
  return 0;
}

static int __fastcall Script_PetDismiss(lua_State *__formal) {
  CGPetInfo::PetDismiss();
  return 0;
}

static int __fastcall Script_PetRename(lua_State *L) {
  CGPetInfo::PetRename(lua_tostring(L, 1));
  return 0;
}

static int __fastcall Script_PetCanBeAbandoned(lua_State *L) {
  CGUnit_C *pet = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(), __FILE__, __LINE__));
  if (pet && pet->GetUnitData()->summonedBy == ClntObjMgrGetActivePlayer() && (pet->GetUnitData()->flags & 0x20)) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int __fastcall Script_PetCanBeRenamed(lua_State *L) {
  CGUnit_C *pet = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(), __FILE__, __LINE__));
  if (pet && pet->GetUnitData()->summonedBy == ClntObjMgrGetActivePlayer() && (pet->GetUnitData()->flags & 0x10)) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int __fastcall Script_GetPetTimeRemaining(lua_State *L) {
  unsigned long expiration = CGPetInfo::GetExpirationTime();
  if (expiration) {
    unsigned long now = OsGetAsyncTimeMs();
    lua_pushnumber(L, static_cast<double>(expiration == now ? 0 : expiration - now));
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

void __fastcall PetInfoRegisterScriptFunctions() {
  for (unsigned int i = 0; i < 18; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void __fastcall PetInfoUnregisterScriptFunctions() {
  for (unsigned int i = 0; i < 18; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
