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
    const SpellShapeshiftFormRec *form = g_spellShapeshiftFormDB.GetRecord(player->GetShapeshiftForm());
    if (form) {
      m_bonusPage = form->m_bonusActionBar;
    }
  }

  FrameScript_SignalEvent(208);
}

BOOL CGActionBar::IsUsableAction(int id, BOOL &noMana) {
  noMana = 0;

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    if (IsAttackAction(id)) {
      return !(player->GetUnitFlags() & 0x20000);
    }
    if (!IsSpell(id)) {
      if (IsItem(id)) {
        return !Spell_C_NeedsCooldownEvent(GetItem(id));
      }
      return 1;
    }
    if (IsToggledAction(id)) {
      return 1;
    }

    const SpellRec *spell = g_spellDB.GetRecord(GetSpell(id));
    if (spell && Spell_C_HaveSpellTokens(player, spell, false) && Spell_C_HaveEquippedSpellItems(player, spell, true, false) &&
        (!(spell->m_attributesEx & 0x500000) || player->GetComboPoints()) &&
        (!spell->m_shapeshiftMask || (spell->m_shapeshiftMask & (1 << (player->GetShapeshiftForm() - 1)))) &&
        (!(spell->m_attributes & 0x10000) || !player->IsShapeShifted()) &&
        (!(spell->m_attributes & 0x20000) || player->IsStealthed()) &&
        (!(spell->m_attributes & 0x10000000) || !player->IsAffectingCombat()) &&
        (!spell->m_casterAuraState || (player->GetAuraState() & (1 << (spell->m_casterAuraState - 1)))))
    {
      if (spell->m_targetAuraState && spell->m_implicitTargetA[0] == 6) {
        CGUnit_C *target = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(CGGameUI::GetLockedTarget(), __FILE__, __LINE__));
        if (!target || !(target->GetAuraState() & (1 << (spell->m_targetAuraState - 1)))) {
          return 0;
        }
      }
      if (!(spell->m_attributes & 0x2000000) || !Spell_C_NeedsCooldownEvent(spell, 0)) {
        int power = spell->m_powerType == -2 ? player->GetHealth() : player->GetPower(static_cast<POWER_TYPE>(spell->m_powerType));
        if (Spell_C_GetManaCost(spell->m_ID, 0) > power) {
          noMana = 1;
          return 0;
        }
        return 1;
      }
    }
  }
  return 0;
}

BOOL CGActionBar::IsCurrentAction(int id) {
  if (!HasAction(id)) {
    return 0;
  }

  if (IsAttackAction(id)) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    return player && player->IsInCombatMode();
  }

  if (IsItem(id)) {
    CGObject_C *item = ClntObjMgrObjectPtr(Spell_C_GetModalItem(), __FILE__, __LINE__);
    return item && item->GetEntryID() == GetItem(id);
  }

  if (Spell_C_GetModalSpell() == GetSpell(id) || Spell_C_GetTargettingSpell() == GetSpell(id)) {
    return 1;
  }

  const SpellRec *spell = g_spellDB.GetRecord(GetSpell(id));
  if (!spell) {
    return 0;
  }

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player && spell->m_effect[0] == 47) {
    if (!spell->m_effectMiscValue[0]) {
      const SkillLineAbilityRec *ability = SpellTableLookupAbility(player->GetRace(), player->GetClass(), spell->m_ID);
      if (ability && CGTradeSkillInfo::GetSkillLine() == ability->m_skillLine) {
        return 1;
      }
    } else if (CGCraftInfo::GetCraftType() == spell->m_effectMiscValue[0]) {
      return 1;
    }
  }

  for (UINT i = 0; i < 3; ++i) {
    if (spell->m_effectAura[i] == 36) {
      int form = spell->m_effectMiscValue[i];
      return form && player && player->GetShapeshiftForm() == form;
    }
  }
  return 0;
}

BOOL CGActionBar::IsToggledAction(int id) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return 0;
  }

  int active = 0;
  if (IsSpell(id)) {
    const SpellRec *spell = g_spellDB.GetRecord(GetSpell(id));
    if (spell && spell->m_activeIconID) {
      for (int i = 0; i < 40; ++i) {
        if (player->GetAura(i) == spell->m_ID && (player->GetAuraFlags(i) & 1)) {
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

  int        action = m_slotActions[id];
  CDataStore msg;
  msg.Put(CMSG_SET_ACTION_BUTTON);
  msg.Put(static_cast<BYTE>(id));
  msg.Put(action);
  msg.Finalize();
  ClientServices_Send(&msg);
  FrameScript_SignalEvent(204, "%d", id + 1);
}

BOOL CGActionBar::IsAttackAction(int id) {
  const SpellRec *spell = g_spellDB.GetRecord(GetSpell(id));
  if (spell && spell->m_effect[0] == 78) {
    return 1;
  }
  return 0;
}

void CGActionBar::UpdateSelection() {
  FrameScript_SignalEvent(205);
}

void CGActionBar::UpdateItem(int entryID) {
  CGObject_C *player = ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__);
  if (player) {
    int count = player->GetBag()->GetItemTypeCount(entryID, 0);
    for (UINT i = 0; i < NUM_ACTION_BUTTONS; ++i) {
      if (IsItem(i) && GetItem(i) == entryID) {
        if (count <= 0) {
          RemoveAction(i);
        } else {
          SlotChanged(i);
        }
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
  if (static_cast<UINT>(id) < NUM_ACTION_BUTTONS) {
    m_slotActions[id] = 0;
    if (action < 0) {
      CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
      if (player && player->CGPlayer_C::GetBag()->FindItemOfType(-action, 0)) {
        m_slotActions[id] = action;
      }
    } else if (action > 0) {
      m_slotActions[id] = action;
    }
    SlotChanged(id);
  }
}

void CGActionBar::AddAction(int action) {
  for (UINT i = 0; i < NUM_ACTION_BUTTONS; ++i) {
    if (!m_slotActions[i]) {
      m_slotActions[i] = action;
      SlotChanged(i);
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

  if (checkCursor && (CGGameUI::GetCursorSpell() > 0 || CGGameUI::GetCursorItem() > 0 ||
                      (CGGameUI::m_cursorItemType == UICURSOR_ACTIONBAR && CGGameUI::GetCursorVirtualItem())))
  {
    PutActionInSlot(id);
    return;
  }
  if (HasAction(id)) {
    if (IsItem(id)) {
      CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
      if (player) {
        CGItem_C *item = player->CGPlayer_C::GetBag()->FindItemOfType(GetItem(id), 0);
        if (item) {
          item->Use();
        }
      }
    } else if (IsToggledAction(id)) {
      Spell_C_CancelAura(GetSpell(id));
    } else if (GetSpell(id) == Spell_C_GetTargettingSpell()) {
      Spell_C_StopTargeting();
    } else {
      Spell_C_CastSpell(GetSpell(id), 0);
      SndInterfacePlayInterfaceSound("GAMESPELLACTIVATE");
    }
  }
}

void CGActionBar::PickupAction(int id) {
  ASSERT(id >= 0);
  ASSERT(id < NUM_ACTION_BUTTONS);

  if (CGGameUI::GetCursorSpell() > 0 || CGGameUI::GetCursorItem() > 0 ||
      (CGGameUI::m_cursorItemType == UICURSOR_ACTIONBAR && CGGameUI::GetCursorVirtualItem()))
  {
    PutActionInSlot(id);
    return;
  }

  if (HasAction(id)) {
    if (IsItem(id)) {
      CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
      if (player) {
        CGItem_C *item = player->CGPlayer_C::GetBag()->FindItemOfType(GetItem(id), 0);
        if (!item) {
          RemoveAction(id);
          return;
        }
        CGGameUI::SetCursorVirtualItem(GetItem(id), item->GetDisplayID(), id, UICURSOR_ACTIONBAR);
      }
    } else if (IsSpell(id)) {
      CGGameUI::SetCursorSpell(GetSpell(id), 0);
    }
    RemoveAction(id);
  }
}

void CGActionBar::PutActionInSlot(int id) {
  int cursorSpell = CGGameUI::GetCursorSpell();
  int cursorItem = CGGameUI::m_cursorItemType == UICURSOR_ACTIONBAR ? CGGameUI::GetCursorVirtualItem() : 0;

  if (!cursorItem && CGGameUI::GetCursorItem()) {
    CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(CGGameUI::GetCursorItem(), __FILE__, __LINE__));
    if (item) {
      cursorItem = item->GetEntryID();
    }
  }

  if (cursorSpell > 0) {
    const SpellRec *spell = g_spellDB.GetRecord(cursorSpell);
    if (!spell) {
      return;
    }
    if (spell->m_attributes & 0x40) {
      CGGameUI::DisplayError(GERR_PASSIVE_ABILITY);
      return;
    }
    if (cursorSpell == GetSpell(id)) {
      CGGameUI::DropCursorSpell();
      return;
    }

    if (IsItem(id)) {
      CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
      if (player) {
        CGItem_C *item = player->CGPlayer_C::GetBag()->FindItemOfType(GetItem(id), 0);
        if (item) {
          CGGameUI::SetCursorVirtualItem(GetItem(id), item->GetDisplayID(), id, UICURSOR_ACTIONBAR);
        }
      }
    } else if (IsSpell(id)) {
      CGGameUI::SetCursorSpell(GetSpell(id), 0);
    } else {
      CGGameUI::ClearCursor(1);
    }

    m_slotActions[id] = cursorSpell;
    SlotChanged(id);
  } else if (cursorItem > 0) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (!player) {
      return;
    }
    CGItem_C *item = player->CGPlayer_C::GetBag()->FindItemOfType(cursorItem, 0);
    if (!item || !item->CanBeUsed()) {
      return;
    }
    if (cursorItem == GetItem(id)) {
      CGGameUI::ClearCursor(1);
      return;
    }

    if (IsItem(id)) {
      CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
      if (player) {
        CGItem_C *item = player->CGPlayer_C::GetBag()->FindItemOfType(GetItem(id), 0);
        if (item) {
          CGGameUI::SetCursorVirtualItem(GetItem(id), item->GetDisplayID(), id, UICURSOR_ACTIONBAR);
        }
      }
    } else if (IsSpell(id)) {
      CGGameUI::SetCursorSpell(GetSpell(id), 0);
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

  if (!(player->GetUnitFlags() & 0x200000)) {
    CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(player->GetBag()->GetItem(15), __FILE__, __LINE__));
    if (item) {
      LPCSTR path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
      SStrPrintf(buffer, sizeof(buffer), "%s%s", path, *path ? "\\" : "");
      SStrPack(buffer, item->GetInventoryArt(), sizeof(buffer));
      return buffer;
    }
  }

  return "Interface\\Buttons\\Spell-Reset";
}

LPCSTR CGActionBar::GetTexture(int id) {
  static char buffer[MAX_PATH];

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    LPCSTR path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
    SStrPrintf(buffer, sizeof(buffer), "%s%s", path, *path ? "\\" : "");

    if (HasAction(id)) {
      if (IsAttackAction(id)) {
        return GetAttackTexture();
      }
      if (IsItem(id)) {
        CGItem_C *item = player->CGPlayer_C::GetBag()->FindItemOfType(GetItem(id), 0);
        if (item) {
          SStrPack(buffer, item->GetInventoryArt(), sizeof(buffer));
          return buffer;
        }
      } else if (IsSpell(id)) {
        const SpellRec *spell = g_spellDB.GetRecord(GetSpell(id));
        if (spell) {
          const SpellIconRec *icon = g_spellIconDB.GetRecord(IsToggledAction(id) ? spell->m_activeIconID : spell->m_spellIconID);
          if (icon) {
            return icon->m_textureFilename;
          }
        }
      }
    }
  }
  return 0;
}

int CGActionBar::GetCount(int id) {
  if (IsItem(id)) {
    CGObject_C *player = ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__);
    if (player) {
      return player->GetBag()->GetItemTypeCount(GetItem(id), 0);
    }
  }
  return 0;
}

void CGActionBar::GetCooldown(int id, DWORD &startTime, UINT &duration, UINT &enable) {
  startTime = 0;
  duration = 0;
  if (IsItem(id)) {
    Spell_C_GetItemCooldown(GetItem(id), &duration, &startTime, &enable);
  } else if (IsSpell(id)) {
    Spell_C_GetSpellCooldown(GetSpell(id), 0, &duration, &startTime, &enable);
  }
}

void CGActionBar::PrecacheButtonArt(int id) {
  if (IsSpell(id)) {
    UnitEffectPreloadSpellEffects(GetSpell(id));
  } else if (IsItem(id)) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (player) {
      CGItem_C *item = player->CGPlayer_C::GetBag()->FindItemOfType(GetItem(id), 0);
      if (item) {
        UnitEffectPreloadSpellEffects(item->GetUseSpell());
      }
    }
  }
}

static int Script_GetActionTexture(lua_State *L) {
  if (lua_isnumber(L, 1)) {
    int    id = static_cast<int>(lua_tonumber(L, 1)) - 1;
    LPCSTR texture = CGActionBar::GetTexture(id);
    if (texture) {
      lua_pushstring(L, texture);
    } else {
      lua_pushnil(L);
    }
    return 1;
  }
  luaL_error(L, "Usage: GetActionTexture(slot)");
  return 0;
}

static int Script_GetActionCount(lua_State *L) {
  if (lua_isnumber(L, 1)) {
    int id = static_cast<int>(lua_tonumber(L, 1)) - 1;
    lua_pushnumber(L, static_cast<double>(CGActionBar::GetCount(id)));
    return 1;
  }
  luaL_error(L, "Usage: GetActionCount(slot)");
  return 0;
}

static int Script_GetActionCooldown(lua_State *L) {
  if (lua_isnumber(L, 1)) {
    int   id = static_cast<int>(lua_tonumber(L, 1)) - 1;
    DWORD startTime;
    UINT  duration;
    UINT  enable = 0;
    CGActionBar::GetCooldown(id, startTime, duration, enable);
    lua_pushnumber(L, static_cast<double>(startTime) * 0.001);
    lua_pushnumber(L, static_cast<double>(duration) * 0.001);
    lua_pushnumber(L, static_cast<double>(enable));
    return 3;
  }
  luaL_error(L, "Usage: GetActionCooldown(slot)");
  return 0;
}

static int Script_HasAction(lua_State *L) {
  if (lua_isnumber(L, 1)) {
    int id = static_cast<int>(lua_tonumber(L, 1)) - 1;
    if (CGActionBar::HasAction(id)) {
      lua_pushnumber(L, 1.0);
    } else {
      lua_pushnil(L);
    }
    return 1;
  }
  luaL_error(L, "Usage: HasAction(slot)");
  return 0;
}

static int Script_UseAction(lua_State *L) {
  if (lua_isnumber(L, 1)) {
    int  id = static_cast<int>(lua_tonumber(L, 1)) - 1;
    BOOL checkCursor = 1;
    if (lua_isstring(L, 2)) {
      checkCursor = StringToBOOL(lua_tostring(L, 2));
    }
    CGActionBar::UseAction(id, checkCursor);
    return 0;
  }
  luaL_error(L, "Usage: UseAction(slot, [checkCursor])");
  return 0;
}

static int Script_PickupAction(lua_State *L) {
  if (lua_isnumber(L, 1)) {
    int id = static_cast<int>(lua_tonumber(L, 1)) - 1;
    CGActionBar::PickupAction(id);
    return 0;
  }
  luaL_error(L, "Usage: PickupAction(slot)");
  return 0;
}

static int Script_PlaceAction(lua_State *L) {
  if (lua_isnumber(L, 1)) {
    int id = static_cast<int>(lua_tonumber(L, 1)) - 1;
    CGActionBar::PutActionInSlot(id);
    return 0;
  }
  luaL_error(L, "Usage: PlaceAction(slot)");
  return 0;
}

static int Script_IsAttackAction(lua_State *L) {
  if (lua_isnumber(L, 1)) {
    int id = static_cast<int>(lua_tonumber(L, 1)) - 1;
    if (CGActionBar::IsAttackAction(id)) {
      lua_pushnumber(L, 1.0);
    } else {
      lua_pushnil(L);
    }
    return 1;
  }
  luaL_error(L, "Usage: IsAttackAction(slot)");
  return 0;
}

static int Script_IsCurrentAction(lua_State *L) {
  if (lua_isnumber(L, 1)) {
    int id = static_cast<int>(lua_tonumber(L, 1)) - 1;
    if (CGActionBar::IsCurrentAction(id)) {
      lua_pushnumber(L, 1.0);
    } else {
      lua_pushnil(L);
    }
    return 1;
  }
  luaL_error(L, "Usage: IsCurrentAction(slot)");
  return 0;
}

static int Script_IsUsableAction(lua_State *L) {
  if (lua_isnumber(L, 1)) {
    int  id = static_cast<int>(lua_tonumber(L, 1)) - 1;
    BOOL noMana = 0;
    if (CGActionBar::IsUsableAction(id, noMana)) {
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
  luaL_error(L, "Usage: IsUsableAction(slot)");
  return 0;
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
    CGActionBar::PrecacheButtonArt(static_cast<int>(lua_tonumber(L, 1)) - 1);
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
