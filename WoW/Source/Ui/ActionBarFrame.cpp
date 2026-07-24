#include "ActionBarFrame.h"

#include "Base/CDataStore.h"
#include "DB/DBClient/AutoCode/SpellShapeshiftFormRec.h"
#include "DB/DBClient/AutoCode/SpellIconRec.h"
#include "DB/DBClient/AutoCode/SpellRec.h"
#include "DB/DBClient/DBClient.h"
#include "GameUI.h"
#include "SpellBookFrame.h"
#include "Object/ObjectClient/Bag_C.h"
#include "Object/ObjectClient/Item_C.h"
#include "Object/ObjectClient/Player_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "FrameScript/FrameScript.h"
#include "SoundInterface/SoundInterface.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

#include <string.h>
#include <lauxlib.h>
#include <lua.h>
#include <storm.h>

bool __fastcall Spell_C_CastSpell(int spellID, const CGItem_C *item);
int __fastcall  Spell_C_GetSpellCooldown(int spell, int isPet, unsigned int *duration, unsigned long *startTime, unsigned int *enable);
int __fastcall  Spell_C_GetItemCooldown(int itemID, unsigned int *duration, unsigned long *startTime, unsigned int *enable);
int __fastcall  Spell_C_GetModalSpell();
int __fastcall  Spell_C_GetTargettingSpell();
void __fastcall Spell_C_StopTargeting();
void __fastcall Spell_C_CancelAura(int spellID);

int          CGActionBar::m_slotActions[120];
unsigned int CGActionBar::m_bonusPage;

void __fastcall CGActionBar::InitializeGame() {
  memset(m_slotActions, 0, sizeof(m_slotActions));
}

void __fastcall CGActionBar::EnterWorld() {
  UpdateBonusBar();
}

void __fastcall CGActionBar::ShutdownGame() {
}

void __fastcall CGActionBar::UpdateBonusBar() {
  m_bonusPage = 0;

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    const SpellShapeshiftFormRec *form = g_spellShapeshiftFormDB.GetRecord(player->GetUnitData()->shapeshiftForm);
    if (form) {
      m_bonusPage = form->m_bonusActionBar;
    }
  }

  FrameScript_SignalEvent(208);
}

int __fastcall CGActionBar::IsUsableAction(int id, int &noMana) {
  noMana = 0;
  if (!HasAction(id)) {
    return 0;
  }
  if (IsItem(id)) {
    return GetCount(id) > 0;
  }
  return g_spellDB.GetRecord(GetSpell(id)) != 0;
}

int __fastcall CGActionBar::IsCurrentAction(int id) {
  int spell = GetSpell(id);
  return spell && (spell == Spell_C_GetModalSpell() || spell == Spell_C_GetTargettingSpell());
}

int __fastcall CGActionBar::IsToggledAction(int id) {
  return IsCurrentAction(id);
}

void __fastcall CGActionBar::ShowGrid() {
  FrameScript_SignalEvent(201);
}

void __fastcall CGActionBar::HideGrid() {
  FrameScript_SignalEvent(202);
}

inline void __fastcall CGActionBar::SlotChanged(int id) {
  FATALASSERT(id >= 0 && id < 120);

  CDataStore msg;
  msg.Put(CMSG_SET_ACTION_BUTTON);
  msg.Put(static_cast<unsigned char>(id));
  msg.Put(m_slotActions[id]);
  msg.Finalize();
  ClientServices_Send(&msg);
  FrameScript_SignalEvent(204, "%d", id + 1);
}

int __fastcall CGActionBar::IsAttackAction(int id) {
  SpellRec *spell = g_spellDB.GetRecord(GetSpell(id));
  return spell && spell->m_effect[0] == 78;
}

void __fastcall CGActionBar::UpdateSelection() {
  FrameScript_SignalEvent(205);
}

void __fastcall CGActionBar::UpdateItem(int entryID) {
  for (int id = 0; id < 120; ++id) {
    if (GetItem(id) == entryID) {
      FrameScript_SignalEvent(204, "%d", id + 1);
    }
  }
}

void __fastcall CGActionBar::UpdateUsable() {
  FrameScript_SignalEvent(206);
}

void __fastcall CGActionBar::UpdateCooldowns() {
  FrameScript_SignalEvent(207);
}

void __fastcall CGActionBar::SetAction(int id, int action) {
  if (static_cast<unsigned int>(id) >= 120) {
    return;
  }

  m_slotActions[id] = 0;
  if (action > 0) {
    m_slotActions[id] = action;
  } else if (action < 0) {
    CGObject_C *player = ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__);
    CGBag_C    *inventory = player ? player->GetBag() : 0;
    if (inventory && inventory->FindItemOfType(-action, 0)) {
      m_slotActions[id] = action;
    }
  }

  SlotChanged(id);
}

void __fastcall CGActionBar::AddAction(int action) {
  for (int id = 0; id < 120; ++id) {
    if (!m_slotActions[id]) {
      SetAction(id, action);
      return;
    }
  }
}

void __fastcall CGActionBar::RemoveAction(int id) {
  m_slotActions[id] = 0;
  SlotChanged(id);
}

void __fastcall CGActionBar::RemoveSpell(int spellID) {
  for (unsigned int id = 0; id < 120; ++id) {
    if (m_slotActions[id] == spellID) {
      RemoveAction(id);
    }
  }
}

void __fastcall CGActionBar::ReplaceSpell(int oldSpell, int newSpell) {
  for (unsigned int id = 0; id < 120; ++id) {
    if (m_slotActions[id] == oldSpell) {
      RemoveAction(id);
      SetAction(id, newSpell);
    }
  }
}

void __fastcall CGActionBar::UseAction(int id, int checkCursor) {
  ASSERT(id >= 0);
  ASSERT(id < 120);

  if (checkCursor &&
      (CGGameUI::GetCursorSpell() > 0 || CGGameUI::GetCursorItem() ||
       (CGGameUI::GetCursorVirtualItem(UICURSOR_ACTIONBAR) && CGGameUI::GetCursorVirtualItem())))
  {
    PutActionInSlot(id);
    return;
  }
  if (!HasAction(id)) {
    return;
  }
  if (IsSpell(id)) {
    int spell = GetSpell(id);
    if (IsToggledAction(id)) {
      Spell_C_CancelAura(spell);
    } else if (spell == Spell_C_GetTargettingSpell()) {
      Spell_C_StopTargeting();
    } else {
      Spell_C_CastSpell(spell, 0);
      SndInterfacePlayInterfaceSound("INTERFACESOUND_ACTIONBUTTONDOWN");
    }
  } else {
    CGObject_C *player = ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__);
    CGBag_C    *inventory = player ? player->GetBag() : 0;
    CGItem_C   *item = inventory ? inventory->FindItemOfType(GetItem(id), 0) : 0;
    if (item) {
      item->Use();
    }
  }
}

void __fastcall CGActionBar::PickupAction(int id) {
  ASSERT(id >= 0);
  ASSERT(id < 120);

  if (CGGameUI::GetCursorSpell() > 0 || CGGameUI::GetCursorItem() ||
      (CGGameUI::GetCursorVirtualItem(UICURSOR_ACTIONBAR) && CGGameUI::GetCursorVirtualItem()))
  {
    PutActionInSlot(id);
    return;
  }

  int action = m_slotActions[id];
  if (!action) {
    return;
  }

  if (action > 0) {
    CGGameUI::SetCursorSpell(action, 0);
    RemoveAction(id);
    return;
  }

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  CGItem_C *item = player && player->GetBag() ? player->GetBag()->FindItemOfType(-action, 0) : 0;
  if (item) {
    CGGameUI::SetCursorVirtualItem(-action, item->GetDisplayID(), id, UICURSOR_ACTIONBAR);
  }
  RemoveAction(id);
}

void __fastcall CGActionBar::PutActionInSlot(int id) {
  int cursorSpell = CGGameUI::GetCursorSpell();
  int cursorItem = 0;

  if (CGGameUI::GetCursorVirtualItem(UICURSOR_ACTIONBAR)) {
    cursorItem = static_cast<int>(CGGameUI::GetCursorVirtualItem());
  } else {
    CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(CGGameUI::GetCursorItem(), __FILE__, __LINE__));
    if (item) {
      cursorItem = item->GetEntryID();
    }
  }

  int oldAction = m_slotActions[id];
  if (cursorSpell > 0) {
    SpellRec *spell = g_spellDB.GetRecord(cursorSpell);
    if (!spell) {
      return;
    }
    if (spell->m_attributes & 0x40) {
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(139));
      return;
    }

    if (oldAction > 0 && oldAction == cursorSpell) {
      CGGameUI::DropCursorSpell();
      return;
    }

    if (oldAction > 0) {
      CGGameUI::SetCursorSpell(oldAction, 0);
    } else if (oldAction < 0) {
      CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
      CGItem_C *item = player && player->GetBag() ? player->GetBag()->FindItemOfType(-oldAction, 0) : 0;
      if (item) {
        CGGameUI::SetCursorVirtualItem(-oldAction, item->GetDisplayID(), id, UICURSOR_ACTIONBAR);
      }
    } else {
      CGGameUI::ClearCursor(1);
    }

    m_slotActions[id] = cursorSpell;
    SlotChanged(id);
    return;
  }

  if (cursorItem > 0) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    CGItem_C *item = player && player->GetBag() ? player->GetBag()->FindItemOfType(cursorItem, 0) : 0;
    if (!item || !item->CanBeUsed()) {
      return;
    }

    if (oldAction < 0 && -oldAction == cursorItem) {
      CGGameUI::ClearCursor(1);
      return;
    }

    if (oldAction > 0) {
      CGGameUI::SetCursorSpell(oldAction, 0);
    } else if (oldAction < 0) {
      CGItem_C *oldItem = player->GetBag()->FindItemOfType(-oldAction, 0);
      if (oldItem) {
        CGGameUI::SetCursorVirtualItem(-oldAction, oldItem->GetDisplayID(), id, UICURSOR_ACTIONBAR);
      }
    } else {
      CGGameUI::ClearCursor(1);
    }

    m_slotActions[id] = -cursorItem;
    SlotChanged(id);
  }
}

const char *__fastcall CGActionBar::GetAttackTexture() {
  static char buffer[260];

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return 0;
  }

  if (!(player->GetUnitData()->flags & 0x200000)) {
    CGBag_C         *inventory = player->GetBag();
    unsigned __int64 itemGUID = inventory ? inventory->GetItem(15) : 0;
    CGItem_C        *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(itemGUID, __FILE__, __LINE__));
    if (item) {
      const char *path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
      SStrPrintf(buffer, sizeof(buffer), "%s%s", path, path && *path ? "\\" : "");
      SStrPack(buffer, item->GetInventoryArt(), sizeof(buffer));
      return buffer;
    }
  }

  return "Interface\\Buttons\\Spell-Reset";
}

const char *__fastcall CGActionBar::GetTexture(int id) {
  static char buffer[260];

  CGObject_C *player = ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__);
  if (!player || !HasAction(id)) {
    return 0;
  }

  const char *path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
  SStrPrintf(buffer, sizeof(buffer), "%s%s", path, path && *path ? "\\" : "");

  if (IsAttackAction(id)) {
    return GetAttackTexture();
  }

  if (IsSpell(id)) {
    SpellRec *spell = g_spellDB.GetRecord(GetSpell(id));
    if (!spell) {
      return 0;
    }

    int           iconID = IsToggledAction(id) ? spell->m_activeIconID : spell->m_spellIconID;
    SpellIconRec *icon = g_spellIconDB.GetRecord(iconID);
    return icon ? icon->m_textureFilename : 0;
  }

  CGBag_C  *inventory = player ? player->GetBag() : 0;
  CGItem_C *item = inventory ? inventory->FindItemOfType(GetItem(id), 0) : 0;
  if (!item) {
    return 0;
  }
  SStrPack(buffer, item->GetInventoryArt(), sizeof(buffer));
  return buffer;
}

int __fastcall CGActionBar::GetCount(int id) {
  if (!IsItem(id)) {
    return 0;
  }
  CGObject_C *player = ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__);
  CGBag_C    *inventory = player ? player->GetBag() : 0;
  return inventory ? inventory->GetItemTypeCount(GetItem(id), 0) : 0;
}

void __fastcall CGActionBar::GetCooldown(int id, unsigned long &startTime, unsigned int &duration, unsigned int &enable) {
  startTime = 0;
  duration = 0;
  enable = 0;
  if (IsSpell(id)) {
    Spell_C_GetSpellCooldown(GetSpell(id), 0, &duration, &startTime, &enable);
  } else if (IsItem(id)) {
    Spell_C_GetItemCooldown(GetItem(id), &duration, &startTime, &enable);
  }
}

void __fastcall CGActionBar::PrecacheButtonArt(int id) {
  GetTexture(id);
}

static int __fastcall Script_GetActionTexture(lua_State *L) {
  if (!lua_isnumber(L, 1))
    return luaL_error(L, "Usage: GetActionTexture(slot)");
  const char *texture = CGActionBar::GetTexture(static_cast<int>(lua_tonumber(L, 1)) - 1);
  texture ? lua_pushstring(L, texture) : lua_pushnil(L);
  return 1;
}

static int __fastcall Script_GetActionCount(lua_State *L) {
  if (!lua_isnumber(L, 1))
    return luaL_error(L, "Usage: GetActionCount(slot)");
  lua_pushnumber(L, static_cast<double>(CGActionBar::GetCount(static_cast<int>(lua_tonumber(L, 1)) - 1)));
  return 1;
}

static int __fastcall Script_GetActionCooldown(lua_State *L) {
  if (!lua_isnumber(L, 1))
    return luaL_error(L, "Usage: GetActionCooldown(slot)");
  unsigned long startTime;
  unsigned int  duration;
  unsigned int  enable;
  CGActionBar::GetCooldown(static_cast<int>(lua_tonumber(L, 1)) - 1, startTime, duration, enable);
  lua_pushnumber(L, static_cast<double>(startTime) * 0.001);
  lua_pushnumber(L, static_cast<double>(duration) * 0.001);
  lua_pushnumber(L, static_cast<double>(enable));
  return 3;
}

static int __fastcall Script_HasAction(lua_State *L) {
  if (!lua_isnumber(L, 1))
    return luaL_error(L, "Usage: HasAction(slot)");
  CGActionBar::HasAction(static_cast<int>(lua_tonumber(L, 1)) - 1) ? lua_pushnumber(L, 1.0) : lua_pushnil(L);
  return 1;
}

static int __fastcall Script_UseAction(lua_State *L) {
  if (!lua_isnumber(L, 1))
    return luaL_error(L, "Usage: UseAction(slot)");
  CGActionBar::UseAction(static_cast<int>(lua_tonumber(L, 1)) - 1, 1);
  return 0;
}

static int __fastcall Script_PickupAction(lua_State *L) {
  if (!lua_isnumber(L, 1))
    return luaL_error(L, "Usage: PickupAction(slot)");
  CGActionBar::PickupAction(static_cast<int>(lua_tonumber(L, 1)) - 1);
  return 0;
}

static int __fastcall Script_PlaceAction(lua_State *L) {
  if (!lua_isnumber(L, 1))
    return luaL_error(L, "Usage: PlaceAction(slot)");
  CGActionBar::PutActionInSlot(static_cast<int>(lua_tonumber(L, 1)) - 1);
  return 0;
}

static int __fastcall Script_IsAttackAction(lua_State *L) {
  if (!lua_isnumber(L, 1))
    return luaL_error(L, "Usage: IsAttackAction(slot)");
  CGActionBar::IsAttackAction(static_cast<int>(lua_tonumber(L, 1)) - 1) ? lua_pushnumber(L, 1.0) : lua_pushnil(L);
  return 1;
}

static int __fastcall Script_IsCurrentAction(lua_State *L) {
  if (!lua_isnumber(L, 1))
    return luaL_error(L, "Usage: IsCurrentAction(slot)");
  CGActionBar::IsCurrentAction(static_cast<int>(lua_tonumber(L, 1)) - 1) ? lua_pushnumber(L, 1.0) : lua_pushnil(L);
  return 1;
}

static int __fastcall Script_IsUsableAction(lua_State *L) {
  if (!lua_isnumber(L, 1))
    return luaL_error(L, "Usage: IsUsableAction(slot)");
  int noMana;
  int usable = CGActionBar::IsUsableAction(static_cast<int>(lua_tonumber(L, 1)) - 1, noMana);
  usable ? lua_pushnumber(L, 1.0) : lua_pushnil(L);
  noMana ? lua_pushnumber(L, 1.0) : lua_pushnil(L);
  return 2;
}

static int __fastcall Script_GetBonusBarOffset(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGActionBar::GetBonusBarOffset()));
  return 1;
}

static int __fastcall Script_ChangeActionBarPage(lua_State *__formal) {
  FrameScript_SignalEvent(203);
  return 0;
}

static int __fastcall Script_PrecacheSpellArt(lua_State *L) {
  if (!lua_isnumber(L, 1))
    return luaL_error(L, "Usage: PrecacheSpellArt(slot)");
  CGActionBar::PrecacheButtonArt(static_cast<int>(lua_tonumber(L, 1)) - 1);
  return 0;
}

static FrameScript_Method s_ScriptFunctions[13] = {
    {   "GetActionTexture",    Script_GetActionTexture},
    {     "GetActionCount",      Script_GetActionCount},
    {  "GetActionCooldown",   Script_GetActionCooldown},
    {          "HasAction",           Script_HasAction},
    {          "UseAction",           Script_UseAction},
    {       "PickupAction",        Script_PickupAction},
    {        "PlaceAction",         Script_PlaceAction},
    {     "IsAttackAction",      Script_IsAttackAction},
    {    "IsCurrentAction",     Script_IsCurrentAction},
    {     "IsUsableAction",      Script_IsUsableAction},
    {  "GetBonusBarOffset",   Script_GetBonusBarOffset},
    {"ChangeActionBarPage", Script_ChangeActionBarPage},
    {   "PrecacheSpellArt",    Script_PrecacheSpellArt}
};

void __fastcall ActionBarRegisterScriptFunctions() {
  for (unsigned int i = 0; i < 13; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void __fastcall ActionBarUnregisterScriptFunctions() {
  for (unsigned int i = 0; i < 13; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
