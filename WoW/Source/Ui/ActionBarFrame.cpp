#include <Base/Base.h>
#include <WowConst.h>
#include <MapDefs.h>

#include "ActionBarFrame.h"

#include "Base/CDataStore.h"
#include "DB/DBClient/AutoCode/SkillLineAbilityRec.h"
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
#include "FrameXML/LoadXML.h"
#include "SoundInterface/SoundInterface.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

#include <string.h>
#include <lauxlib.h>
#include <lua.h>
#include <storm.h>

bool                       Spell_C_CastSpell(int spellID, const CGItem_C *item);
int                        Spell_C_GetManaCost(int id, BOOL isPet);
int                        Spell_C_GetSpellCooldown(int spell, BOOL isPet, UINT *duration, DWORD *startTime, UINT *enable);
int                        Spell_C_GetItemCooldown(int itemID, UINT *duration, DWORD *startTime, UINT *enable);
int                        Spell_C_GetModalSpell();
const DWORDLONG           &Spell_C_GetModalItem();
int                        Spell_C_GetTargettingSpell();
bool                       Spell_C_HaveSpellTokens(CGPlayer_C *player, const SpellRec *spell, bool report);
bool                       Spell_C_HaveEquippedSpellItems(CGPlayer_C *player, const SpellRec *spell, bool checkAmmo, bool report);
int                        Spell_C_NeedsCooldownEvent(const SpellRec *spell, BOOL isPet);
int                        Spell_C_NeedsCooldownEvent(int itemID);
void                       Spell_C_StopTargeting();
void                       Spell_C_CancelAura(int spellID);
void                       UnitEffectPreloadSpellEffects(int spellID);
const SkillLineAbilityRec *SpellTableLookupAbility(UINT raceID, UINT classID, UINT spellID);

class CGTradeSkillInfo {
 public:
  static int GetSkillLine() {
    return m_skillLine;
  }

 private:
  static int m_skillLine;
};

class CGCraftInfo {
 public:
  static SPELL_CAST_UI_TYPE GetCraftType() {
    return m_craftType;
  }

 private:
  static SPELL_CAST_UI_TYPE m_craftType;
};

int  CGActionBar::m_slotActions[NUM_ACTION_BUTTONS];
UINT CGActionBar::m_bonusPage;

void CGActionBar::InitializeGame() {
  memset(m_slotActions, 0, sizeof(m_slotActions));
}

void CGActionBar::EnterWorld() {
  UpdateBonusBar();
}

void CGActionBar::ShutdownGame() {
}

void CGActionBar::UpdateBonusBar() {
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

BOOL CGActionBar::IsUsableAction(int id, BOOL &noMana) {
  noMana = 0;

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return 0;
  }

  const CGUnitData *unitData = player->GetUnitData();
  if (IsAttackAction(id)) {
    return !(unitData->flags & 0x20000);
  }

  int action = m_slotActions[id];
  if (action < 0) {
    return !Spell_C_NeedsCooldownEvent(-action);
  }
  if (!action || IsToggledAction(id)) {
    return 1;
  }

  const SpellRec *spell = g_spellDB.GetRecord(GetSpell(id));
  if (!spell || !Spell_C_HaveSpellTokens(player, spell, false) || !Spell_C_HaveEquippedSpellItems(player, spell, true, false)) {
    return 0;
  }

  if ((spell->m_attributesEx & 0x500000) && !unitData->comboPoints) {
    return 0;
  }

  if (spell->m_shapeshiftMask && !(spell->m_shapeshiftMask & (1 << (unitData->shapeshiftForm - 1)))) {
    return 0;
  }

  if ((spell->m_attributes & 0x10000) && player->IsShapeShifted()) {
    return 0;
  }

  if ((spell->m_attributes & 0x20000) && !(unitData->flags & 0x8000)) {
    return 0;
  }
  if ((spell->m_attributes & 0x10000000) && (unitData->flags & 0x80000)) {
    return 0;
  }

  if (spell->m_casterAuraState && !(unitData->auraState & (1 << (spell->m_casterAuraState - 1)))) {
    return 0;
  }

  if (spell->m_targetAuraState && spell->m_implicitTargetA[0] == 6) {
    CGUnit_C *target = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(CGGameUI::GetLockedTarget(), __FILE__, __LINE__));
    if (!target || !(target->GetUnitData()->auraState & (1 << (spell->m_targetAuraState - 1)))) {
      return 0;
    }
  }

  if ((spell->m_attributes & 0x2000000) && Spell_C_NeedsCooldownEvent(spell, 0)) {
    return 0;
  }

  int power = spell->m_powerType == -2 ? unitData->health : unitData->power[spell->m_powerType];
  if (Spell_C_GetManaCost(spell->m_ID, 0) <= power) {
    return 1;
  }

  noMana = 1;
  return 0;
}

BOOL CGActionBar::IsCurrentAction(int id) {
  int action = m_slotActions[id];
  if (!action) {
    return 0;
  }

  if (IsAttackAction(id)) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    return player && (player->GetUnitData()->flags & 0x400);
  }

  if (action < 0) {
    CGObject_C *item = ClntObjMgrObjectPtr(Spell_C_GetModalItem(), __FILE__, __LINE__);
    return item && item->GetEntryID() == -action;
  }

  int spellID = GetSpell(id);
  if (Spell_C_GetModalSpell() == spellID || Spell_C_GetTargettingSpell() == spellID) {
    return 1;
  }

  const SpellRec *spell = g_spellDB.GetRecord(spellID);
  if (!spell) {
    return 0;
  }

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player && spell->m_effect[0] == 47) {
    if (spell->m_effectMiscValue[0]) {
      if (CGCraftInfo::GetCraftType() == spell->m_effectMiscValue[0]) {
        return 1;
      }
    } else {
      const SkillLineAbilityRec *ability = SpellTableLookupAbility(player->GetUnitData()->race, player->GetUnitData()->classId, spell->m_ID);
      if (ability && CGTradeSkillInfo::GetSkillLine() == ability->m_skillLine) {
        return 1;
      }
    }
  }

  UINT effect;
  for (effect = 0; effect < 3; ++effect) {
    if (spell->m_effectAura[effect] == 36) {
      break;
    }
  }
  if (effect >= 3) {
    return 0;
  }

  int form = spell->m_effectMiscValue[effect];
  return form && player && player->GetUnitData()->shapeshiftForm == form;
}

BOOL CGActionBar::IsToggledAction(int id) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return 0;
  }

  int active = 0;
  if (m_slotActions[id] > 0) {
    const SpellRec *spell = g_spellDB.GetRecord(m_slotActions[id]);
    if (spell && spell->m_activeIconID) {
      const CGUnitData *unitData = player->GetUnitData();
      const BYTE       *auraFlags = unitData->auraFlags;
      int               aura;
      for (aura = 0; aura < 40; ++aura) {
        if (unitData->auras[aura] == spell->m_ID && ((auraFlags[aura / 2] >> (4 * (aura % 2))) & 1)) {
          active = 1;
          break;
        }
      }
    }
  }
  return active;
}

void CGActionBar::ShowGrid() {
  FrameScript_SignalEvent(201);
}

void CGActionBar::HideGrid() {
  FrameScript_SignalEvent(202);
}

void CGActionBar::SlotChanged(int id) {
  FATALASSERT(id >= 0 && id < NUM_ACTION_BUTTONS);

  CDataStore msg;
  msg.Put(CMSG_SET_ACTION_BUTTON);
  msg.Put(static_cast<BYTE>(id));
  msg.Put(m_slotActions[id]);
  msg.Finalize();
  ClientServices_Send(&msg);
  FrameScript_SignalEvent(204, "%d", id + 1);
}

BOOL CGActionBar::IsAttackAction(int id) {
  const SpellRec *spell = g_spellDB.GetRecord(GetSpell(id));
  return spell && spell->m_effect[0] == 78;
}

void CGActionBar::UpdateSelection() {
  FrameScript_SignalEvent(205);
}

void CGActionBar::UpdateItem(int entryID) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return;
  }

  for (int id = 0; id < NUM_ACTION_BUTTONS; ++id) {
    if (m_slotActions[id] < 0 && -m_slotActions[id] == entryID) {
      if (player->GetBag()->GetItemTypeCount(entryID, 0) > 0) {
        SlotChanged(id);
      } else {
        RemoveAction(id);
      }
    }
  }
}

void CGActionBar::UpdateUsable() {
  FrameScript_SignalEvent(206);
}

void CGActionBar::UpdateCooldowns() {
  FrameScript_SignalEvent(207);
}

void CGActionBar::SetAction(int id, int action) {
  if (static_cast<UINT>(id) >= NUM_ACTION_BUTTONS) {
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
      SlotChanged(id);
      return;
    }
  }

  SlotChanged(id);
}

void CGActionBar::AddAction(int action) {
  for (int id = 0; id < NUM_ACTION_BUTTONS; ++id) {
    if (!m_slotActions[id]) {
      m_slotActions[id] = action;
      SlotChanged(id);
      return;
    }
  }
}

void CGActionBar::RemoveAction(int id) {
  m_slotActions[id] = 0;
  SlotChanged(id);
}

void CGActionBar::RemoveSpell(int spellID) {
  for (UINT id = 0; id < NUM_ACTION_BUTTONS; ++id) {
    if (m_slotActions[id] == spellID) {
      RemoveAction(id);
    }
  }
}

void CGActionBar::ReplaceSpell(int oldSpell, int newSpell) {
  for (UINT id = 0; id < NUM_ACTION_BUTTONS; ++id) {
    if (m_slotActions[id] == oldSpell) {
      RemoveAction(id);
      SetAction(id, newSpell);
    }
  }
}

void CGActionBar::UseAction(int id, BOOL checkCursor) {
  ASSERT(id >= 0);
  ASSERT(id < NUM_ACTION_BUTTONS);

  if (checkCursor && (CGGameUI::GetCursorSpell() > 0 || CGGameUI::GetCursorItem() ||
                      (CGGameUI::m_cursorItemType == UICURSOR_ACTIONBAR && CGGameUI::GetCursorVirtualItem())))
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

void CGActionBar::PickupAction(int id) {
  ASSERT(id >= 0);
  ASSERT(id < NUM_ACTION_BUTTONS);

  if (CGGameUI::GetCursorSpell() > 0 || CGGameUI::GetCursorItem() ||
      (CGGameUI::m_cursorItemType == UICURSOR_ACTIONBAR && CGGameUI::GetCursorVirtualItem()))
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
  CGItem_C   *item = player && player->GetBag() ? player->GetBag()->FindItemOfType(-action, 0) : 0;
  if (item) {
    CGGameUI::SetCursorVirtualItem(-action, item->GetDisplayID(), id, UICURSOR_ACTIONBAR);
  }
  RemoveAction(id);
}

void CGActionBar::PutActionInSlot(int id) {
  int cursorSpell = CGGameUI::GetCursorSpell();
  int cursorItem = 0;

  if (CGGameUI::m_cursorItemType == UICURSOR_ACTIONBAR) {
    cursorItem = static_cast<int>(CGGameUI::GetCursorVirtualItem());
  }

  if (!cursorItem && CGGameUI::GetCursorItem()) {
    CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(CGGameUI::GetCursorItem(), __FILE__, __LINE__));
    if (item) {
      cursorItem = item->GetEntryID();
    }
  }

  int oldAction = m_slotActions[id];
  if (cursorSpell > 0) {
    const SpellRec *spell = g_spellDB.GetRecord(cursorSpell);
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
      CGItem_C   *item = player && player->GetBag() ? player->GetBag()->FindItemOfType(-oldAction, 0) : 0;
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
    CGItem_C   *item = player && player->GetBag() ? player->GetBag()->FindItemOfType(cursorItem, 0) : 0;
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

LPCSTR CGActionBar::GetAttackTexture() {
  static char buffer[MAX_PATH];

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return 0;
  }

  if (!(player->GetUnitData()->flags & 0x200000)) {
    CGBag_C  *inventory = player->GetBag();
    DWORDLONG itemGUID = inventory ? inventory->GetItem(15) : 0;
    CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(itemGUID, __FILE__, __LINE__));
    if (item) {
      LPCSTR path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
      SStrPrintf(buffer, sizeof(buffer), "%s%s", path, path && *path ? "\\" : "");
      SStrPack(buffer, item->GetInventoryArt(), sizeof(buffer));
      return buffer;
    }
  }

  return "Interface\\Buttons\\Spell-Reset";
}

LPCSTR CGActionBar::GetTexture(int id) {
  static char buffer[MAX_PATH];

  CGObject_C *player = ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__);
  if (!player || !HasAction(id)) {
    return 0;
  }

  LPCSTR path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
  SStrPrintf(buffer, sizeof(buffer), "%s%s", path, path && *path ? "\\" : "");

  if (IsAttackAction(id)) {
    return GetAttackTexture();
  }

  if (IsSpell(id)) {
    const SpellRec *spell = g_spellDB.GetRecord(GetSpell(id));
    if (!spell) {
      return 0;
    }

    int                 iconID = IsToggledAction(id) ? spell->m_activeIconID : spell->m_spellIconID;
    const SpellIconRec *icon = g_spellIconDB.GetRecord(iconID);
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

int CGActionBar::GetCount(int id) {
  if (!IsItem(id)) {
    return 0;
  }
  CGObject_C *player = ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__);
  return player ? player->GetBag()->GetItemTypeCount(GetItem(id), 0) : 0;
}

void CGActionBar::GetCooldown(int id, DWORD &startTime, UINT &duration, UINT &enable) {
  startTime = 0;
  duration = 0;
  if (IsSpell(id)) {
    Spell_C_GetSpellCooldown(GetSpell(id), 0, &duration, &startTime, &enable);
  } else if (IsItem(id)) {
    Spell_C_GetItemCooldown(GetItem(id), &duration, &startTime, &enable);
  }
}

void CGActionBar::PrecacheButtonArt(int id) {
  int action = m_slotActions[id];
  if (action > 0) {
    UnitEffectPreloadSpellEffects(action);
  } else if (action < 0) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    CGItem_C   *item = player ? player->GetBag()->FindItemOfType(-action, 0) : 0;
    if (item) {
      UnitEffectPreloadSpellEffects(item->GetUseSpell());
    }
  }
}

static int Script_GetActionTexture(lua_State *L) {
  if (!lua_isnumber(L, 1))
    return luaL_error(L, "Usage: GetActionTexture(slot)");
  LPCSTR texture = CGActionBar::GetTexture(static_cast<int>(lua_tonumber(L, 1)) - 1);
  if (texture) {
    lua_pushstring(L, texture);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_GetActionCount(lua_State *L) {
  if (!lua_isnumber(L, 1))
    return luaL_error(L, "Usage: GetActionCount(slot)");
  lua_pushnumber(L, static_cast<double>(CGActionBar::GetCount(static_cast<int>(lua_tonumber(L, 1)) - 1)));
  return 1;
}

static int Script_GetActionCooldown(lua_State *L) {
  if (!lua_isnumber(L, 1))
    return luaL_error(L, "Usage: GetActionCooldown(slot)");
  DWORD startTime;
  UINT  duration;
  UINT  enable;
  CGActionBar::GetCooldown(static_cast<int>(lua_tonumber(L, 1)) - 1, startTime, duration, enable);
  lua_pushnumber(L, static_cast<double>(startTime) * 0.001);
  lua_pushnumber(L, static_cast<double>(duration) * 0.001);
  lua_pushnumber(L, static_cast<double>(enable));
  return 3;
}

static int Script_HasAction(lua_State *L) {
  if (!lua_isnumber(L, 1))
    return luaL_error(L, "Usage: HasAction(slot)");
  if (CGActionBar::HasAction(static_cast<int>(lua_tonumber(L, 1)) - 1)) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_UseAction(lua_State *L) {
  if (!lua_isnumber(L, 1))
    return luaL_error(L, "Usage: UseAction(slot)");
  BOOL checkCursor = 0;
  if (lua_isstring(L, 2)) {
    checkCursor = StringToBOOL(lua_tostring(L, 2));
  }
  CGActionBar::UseAction(static_cast<int>(lua_tonumber(L, 1)) - 1, checkCursor);
  return 0;
}

static int Script_PickupAction(lua_State *L) {
  if (!lua_isnumber(L, 1))
    return luaL_error(L, "Usage: PickupAction(slot)");
  CGActionBar::PickupAction(static_cast<int>(lua_tonumber(L, 1)) - 1);
  return 0;
}

static int Script_PlaceAction(lua_State *L) {
  if (!lua_isnumber(L, 1))
    return luaL_error(L, "Usage: PlaceAction(slot)");
  CGActionBar::PutActionInSlot(static_cast<int>(lua_tonumber(L, 1)) - 1);
  return 0;
}

static int Script_IsAttackAction(lua_State *L) {
  if (!lua_isnumber(L, 1))
    return luaL_error(L, "Usage: IsAttackAction(slot)");
  if (CGActionBar::IsAttackAction(static_cast<int>(lua_tonumber(L, 1)) - 1)) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_IsCurrentAction(lua_State *L) {
  if (!lua_isnumber(L, 1))
    return luaL_error(L, "Usage: IsCurrentAction(slot)");
  if (CGActionBar::IsCurrentAction(static_cast<int>(lua_tonumber(L, 1)) - 1)) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_IsUsableAction(lua_State *L) {
  if (!lua_isnumber(L, 1))
    return luaL_error(L, "Usage: IsUsableAction(slot)");
  BOOL noMana;
  int  usable = CGActionBar::IsUsableAction(static_cast<int>(lua_tonumber(L, 1)) - 1, noMana);
  if (usable) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  if (noMana) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 2;
}

static int Script_GetBonusBarOffset(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGActionBar::GetBonusBarOffset()));
  return 1;
}

static int Script_ChangeActionBarPage(lua_State *) {
  FrameScript_SignalEvent(203);
  return 0;
}

static int Script_PrecacheSpellArt(lua_State *L) {
  if (lua_isnumber(L, 1)) {
    CGActionBar::PrecacheButtonArt(static_cast<int>(lua_tonumber(L, 1)));
  }
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

void ActionBarRegisterScriptFunctions() {
  for (UINT i = 0; i < 13; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void ActionBarUnregisterScriptFunctions() {
  for (UINT i = 0; i < 13; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
