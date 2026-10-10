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

#include "SpellBookFrame.h"

#include <DB/DBClient/AutoCode/LanguagesRec.h>
#include <DB/DBClient/AutoCode/ChrClassesRec.h>
#include <DB/DBClient/AutoCode/SkillLineAbilityRec.h>
#include <DB/DBClient/AutoCode/SpellRec.h>
#include <DB/DBClient/AutoCode/SpellIconRec.h>
#include <DB/DBClient/AutoCode/SpellShapeshiftFormRec.h>
#include <DB/WowLocale.h>
#include <FrameScript/FrameScript.h>

#include "ChatFrame.h"
#include "ActionBarFrame.h"
#include "PetInfo.h"
#include "SoundInterface/SoundInterface.h"
#include "Tutorial.h"
#include "Object/ObjectClient/Player_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

#include <Base/CDataStore.h>
#include <Net/NetClient/NetClient.h>

#include <stdlib.h>
#include <string.h>
#include <lauxlib.h>
#include <lua.h>

int Spell_C_GetModalSpell();
class CGItem_C;
int  Spell_C_GetTargettingSpell();
bool Spell_C_CastSpell(int spellID, const CGItem_C *item);
int  Spell_C_GetSpellCooldown(int spell, BOOL isPet, UINT *duration, DWORD *startTime, UINT *enable);
void Spell_C_StopTargeting();
void Spell_C_CancelAura(int spellID);

struct TradeSkillInfo;
struct TradeSkillSubClassInfo;

class CGTradeSkillInfo {
  friend int __cdecl QSortSkills(LPCVOID a, LPCVOID b);
  friend int __cdecl QSortSubClasses(LPCVOID a, LPCVOID b);

 public:
  static void EnterWorld();
  static void LeaveWorld();
  static void ShutdownGame();
  static void Close();
  static void ClearItemCallbacks();
  static void DecrementPendingItem() {
    if (!m_itemsPending || !--m_itemsPending) {
      RefreshList(1);
    }
  }
  static void SetSelection(int index);
  static int  GetSelectionIndex();
  static int  GetSkillLine() {
    return m_skillLine;
  }
  static int GetNumTradeSkills() {
    return m_filteredSkills;
  }
  static const TradeSkillInfo *GetTradeSkillInfo(UINT index) {
    return index < m_filteredSkills ? m_skills[index] : 0;
  }
  static void SetSkillLine(int id);
  static void RefreshList(int resetFilters);
  static UINT GetNumSubClasses() {
    return m_numSubClasses;
  }
  static TradeSkillSubClassInfo *GetSubClass(UINT index) {
    return index < m_numSubClasses ? m_subClasses[index] : 0;
  }
  static int  GetSubClassIndexFromSkill(UINT index);
  static BOOL IsCollpasedHeader(UINT index);
  static int  GetSubClassFilter() {
    return m_subClassFilter;
  }
  static int GetInvTypeFilter() {
    return m_invTypeFilter;
  }
  static int GetCollapseFilter() {
    return m_collapseFilter;
  }
  static int GetAvailableSlots() {
    return m_availableSlots;
  }
  static void SetSubClassFilter(int filter);
  static void SetInvTypeFilter(int filter);
  static void SetCollapseFilter(int filter);

 protected:
  static void FilterAndSortSkills();

 private:
  static int                                       m_skillLine;
  static int                                       m_currentSelection;
  static UINT                                      m_itemsPending;
  static UINT                                      m_numSkills;
  static UINT                                      m_numSubClasses;
  static UINT                                      m_filteredSkills;
  static int                                       m_subClassFilter;
  static int                                       m_invTypeFilter;
  static int                                       m_collapseFilter;
  static TSGrowableArray<TradeSkillInfo *>         m_skills;
  static TSGrowableArray<TradeSkillSubClassInfo *> m_subClasses;
  static int                                       m_availableSlots;
};

struct CraftInfo;
struct CraftSkillLineInfo;

class CGCraftInfo {
 public:
  static void               EnterWorld();
  static void               ShutdownGame();
  static void               Close();
  static void               SetSelection(int index);
  static int                GetSelectionIndex();
  static SPELL_CAST_UI_TYPE GetCraftType() {
    return m_craftType;
  }
  static int GetNumCrafts() {
    return m_filteredSkills;
  }
  static const CraftInfo *GetCraftInfo(UINT index) {
    return index < m_numSkills ? m_skills[index] : 0;
  }
  static UINT GetNumSkillLines() {
    return m_numSkillLines;
  }
  static CraftSkillLineInfo *GetSkillLine(UINT index) {
    return index < m_numSkillLines ? m_skillLines[index] : 0;
  }
  static int  GetSkillLineIndexFromCraft(UINT index);
  static void SetCraftType(SPELL_CAST_UI_TYPE type);
  static void RefreshList();
  static BOOL IsCollpasedHeader(UINT index);
  static int  GetCollapseFilter() {
    return m_collapseFilter;
  }
  static void SetCollapseFilter(int filter);

 private:
  friend int __cdecl QSortSkills(LPCVOID a, LPCVOID b);
  friend int __cdecl QSortPetSkills(LPCVOID a, LPCVOID b);
  friend int __cdecl QSortSkillLines(LPCVOID a, LPCVOID b);

 protected:
  static void FilterAndSortSkills();

 private:
  static SPELL_CAST_UI_TYPE                    m_craftType;
  static int                                   m_currentSelection;
  static UINT                                  m_numSkills;
  static UINT                                  m_numSkillLines;
  static UINT                                  m_filteredSkills;
  static int                                   m_collapseFilter;
  static TSGrowableArray<CraftInfo *>          m_skills;
  static TSGrowableArray<CraftSkillLineInfo *> m_skillLines;
};

FBitField            CGSpellBook::m_knownSpellBits;
int                  CGSpellBook::m_knownSpells[MAXIMUM_LEARNED_SPELLS];
int                  CGSpellBook::m_knownAbilities[MAXIMUM_LEARNED_SPELLS];
int                  CGSpellBook::m_petSpells[MAXIMUM_LEARNED_SPELLS];
int                  CGSpellBook::m_duelSpell;
int                  CGSpellBook::m_stuckSpell;
TSFixedArray<int>    CGSpellBook::m_languageSpells;
TSGrowableArray<int> CGSpellBook::m_unlockSpells;
TSGrowableArray<int> CGSpellBook::m_shapeshiftForms;
int                  CGSpellBook::m_selectedSlot = -1;
UI_SPELL_TYPE        CGSpellBook::m_selectedType;
int                  CGSpellBook::m_knowsSpells;
int                  CGSpellBook::m_knowsPetSpells;

void CGSpellBook::InitializeGame() {
  m_knownSpellBits.SetCount(g_spellDB.GetMaxID() + 1);
  m_languageSpells.SetCount(g_languagesDB.GetMaxID() + 1);
  ClearSpells();
}

void CGSpellBook::ShutdownGame() {
  m_knownSpellBits.Clear();
  m_languageSpells.Clear();
}

void CGSpellBook::ClearSpells() {
  UINT i;

  m_knownSpellBits.ClearAll();
  memset(m_petSpells, 0, sizeof(m_petSpells));
  memset(m_knownAbilities, 0, sizeof(m_knownAbilities));
  memset(m_knownSpells, 0, sizeof(m_knownSpells));

  m_duelSpell = 0;
  m_stuckSpell = 0;
  for (i = m_languageSpells.Count(); i;) {
    m_languageSpells[--i] = 0;
  }

  m_unlockSpells.Clear();
  m_shapeshiftForms.Clear();
  m_selectedType = PLAYER_SPELL;
  m_knowsSpells = 0;
  m_knowsPetSpells = 0;
  m_selectedSlot = -1;
}

static int __cdecl QSortShapeshiftForms(LPCVOID a, LPCVOID b) {
  FATALASSERT(a);
  FATALASSERT(b);

  const SpellRec *spellA = g_spellDB.GetRecord(*(const int *)a);
  const SpellRec *spellB = g_spellDB.GetRecord(*(const int *)b);
  if (!spellA || !spellB) {
    return 0;
  }

  if (spellA->m_spellLevel == spellB->m_spellLevel) {
    return SStrCmpI(spellA->m_name_lang[CURRENT_LANGUAGE], spellB->m_name_lang[CURRENT_LANGUAGE], 0x7FFFFFFF);
  }

  return spellA->m_spellLevel > spellB->m_spellLevel ? 1 : -1;
}

void CGSpellBook::AddKnownSpell(int spellID, int slot, int learned) {
  const SpellRec *info = g_spellDB.GetRecord(spellID);
  if (!info) {
    return;
  }

  m_knownSpellBits.SetBit(spellID);

  if (info->m_effect[0] == 83) {
    m_duelSpell = spellID;
  }
  if (info->m_effect[0] == 84) {
    m_stuckSpell = spellID;
  }
  if (info->m_effect[0] == 39) {
    FATALASSERT(info->m_effectMiscValue[0] < int(m_languageSpells.Count()));
    m_languageSpells[info->m_effectMiscValue[0]] = spellID;
    CGChat::UpdateLanguages();
  }
  if (info->m_effect[0] == 33) {
    *m_unlockSpells.New() = spellID;
  }

  UINT i;
  for (i = 0; i < 3; ++i) {
    if (info->m_effectAura[i] == 36) {
      BOOL found = 0;
      UINT count = m_shapeshiftForms.Count();
      for (i = 0; i < count; ++i) {
        if (m_shapeshiftForms[i] == spellID) {
          found = 1;
          break;
        }
      }
      if (!found) {
        m_shapeshiftForms.SetCount(count + 1);
        m_shapeshiftForms[count] = spellID;
        qsort(m_shapeshiftForms.Ptr(), m_shapeshiftForms.Count(), sizeof(int), QSortShapeshiftForms);
        FrameScript_SignalEvent(370);
      }
      break;
    }
  }

  if (info->m_attributes & 0x80) {
    return;
  }
  if (info->m_attributes & 0x20) {
    if (learned) {
      CGGameUI::DisplayError(GERR_LEARN_RECIPE_S, info->m_name_lang[CURRENT_LANGUAGE]);
    }
    return;
  }

  int ability = info->m_attributes & 0x10;
  if (learned) {
    CGTutorial::TriggerTutorial(TUTORIAL_ABILITIES);
    CGGameUI::DisplayError(ability ? GERR_LEARN_ABILITY_S : GERR_LEARN_SPELL_S, info->m_name_lang[CURRENT_LANGUAGE]);
  }

  if (info->m_castUI > 0) {
    return;
  }

  if (slot < 0) {
    slot = -slot;
  }
  int sendSpellSlot;
  if (!slot || --slot >= MAXIMUM_LEARNED_SPELLS) {
    sendSpellSlot = 1;
  } else {
    sendSpellSlot = ability ? m_knownAbilities[slot] > 0 : m_knownSpells[slot] > 0;
  }

  if (sendSpellSlot) {
    slot = 0;
    if (ability && info->m_effect[0] != 78) {
      slot = 1;
    }
    for (; slot < MAXIMUM_LEARNED_SPELLS; ++slot) {
      if ((ability ? m_knownAbilities[slot] : m_knownSpells[slot]) <= 0) {
        break;
      }
    }
    if (slot >= MAXIMUM_LEARNED_SPELLS) {
      return;
    }
  }

  if (ability) {
    m_knownAbilities[slot] = spellID;
  } else {
    m_knownSpells[slot] = spellID;
    ++m_knowsSpells;
  }
  if (learned) {
    UpdateSpells();
  }
  if (sendSpellSlot) {
    SendSpellSlot(slot, ability ? PLAYER_ABILITY : PLAYER_SPELL);
  }
}

void CGSpellBook::DelKnownSpell(int spellID) {
  int i;

  m_knownSpellBits.ClearBit(spellID);
  for (i = 0; i < MAXIMUM_LEARNED_SPELLS; ++i) {
    if (m_knownSpells[i] == spellID) {
      m_knownSpells[i] = 0;
      --m_knowsSpells;
      break;
    }
    if (m_knownAbilities[i] == spellID) {
      m_knownAbilities[i] = 0;
      break;
    }
  }

  UINT j;
  for (j = m_languageSpells.Count(); j;) {
    --j;
    if (m_languageSpells[j] == spellID) {
      m_languageSpells[j] = 0;
    }
  }

  UINT count = m_unlockSpells.Count();
  for (j = count; j;) {
    --j;
    if (m_unlockSpells[j] == spellID) {
      --count;
      if (j < count) {
        memcpy(&m_unlockSpells[j], &m_unlockSpells[j + 1], (count - j) * sizeof(int));
      }
      m_unlockSpells.SetCount(count);
      break;
    }
  }

  if (i < MAXIMUM_LEARNED_SPELLS) {
    UpdateSpells();
  }
}

void CGSpellBook::ReplaceSpell(int oldSpell, int newSpell) {
  int slot = 0;
  for (UINT index = 0; index < MAXIMUM_LEARNED_SPELLS; ++index) {
    if (m_knownSpells[index] == oldSpell) {
      slot = index + 1;
      break;
    }
    if (m_knownAbilities[index] == oldSpell) {
      slot = -1 - (int)index;
      break;
    }
  }

  DelKnownSpell(oldSpell);
  AddKnownSpell(newSpell, slot, 1);
  if (slot > 0) {
    SendSpellSlot(slot - 1, PLAYER_SPELL);
  } else if (slot < 0) {
    SendSpellSlot(-1 - slot, PLAYER_ABILITY);
  }
}

void CGSpellBook::ClearPetSpells() {
  memset(m_petSpells, 0, sizeof(m_petSpells));
  m_knowsPetSpells = 0;
}

void CGSpellBook::AddPetSpell(int spellID) {
  const SpellRec *info = g_spellDB.GetRecord(spellID);
  if (info && !(info->m_attributes & 0x80)) {
    for (UINT slot = 0; slot < MAXIMUM_LEARNED_SPELLS; ++slot) {
      if (m_petSpells[slot] <= 0) {
        m_petSpells[slot] = spellID;
        break;
      }
    }
    m_knowsPetSpells = 1;
  }
}

void CGSpellBook::SetSpell(int slot, int spellID, UI_SPELL_TYPE type) {
  switch (type) {
    case PLAYER_SPELL:
      m_knownSpells[slot] = spellID;
      break;
    case PLAYER_ABILITY:
      m_knownAbilities[slot] = spellID;
      break;
    case PET_SPELL:
      m_petSpells[slot] = spellID;
      break;
  }

  SendSpellSlot(slot, type);
  UpdateSpells();
}

void CGSpellBook::SendSpellSlot(int slot, UI_SPELL_TYPE type) {
  if (type != PET_SPELL) {
    int spellID = GetSpell(slot, type);
    if (spellID) {
      CDataStore msg;
      switch (type) {
        case PLAYER_SPELL:
          msg.Put(CMSG_NEW_SPELL_SLOT);
          msg.Put(spellID);
          msg.Put(slot + 1);
          msg.Finalize();
          ClientServices_Send(&msg);
          break;
        case PLAYER_ABILITY:
          msg.Put(CMSG_NEW_SPELL_SLOT);
          msg.Put(spellID);
          msg.Put(-1 - slot);
          msg.Finalize();
          ClientServices_Send(&msg);
          break;
      }
    }
  }
}

void CGSpellBook::UpdateSpells() {
  FrameScript_SignalEvent(244);
}

void CGSpellBook::UpdateCooldowns() {
  FrameScript_SignalEvent(246);
}

void CGSpellBook::UpdateSelection() {
  int spell = Spell_C_GetModalSpell();
  if (!spell) {
    spell = Spell_C_GetTargettingSpell();
  }

  if (spell <= 0) {
    m_selectedSlot = -1;
    FrameScript_SignalEvent(245);
    return;
  }

  for (UINT slot = 0; slot < MAXIMUM_LEARNED_SPELLS; ++slot) {
    if (m_knownSpells[slot] == spell) {
      m_selectedSlot = slot;
      m_selectedType = PLAYER_SPELL;
      FrameScript_SignalEvent(245);
      return;
    }
    if (m_knownAbilities[slot] == spell) {
      m_selectedSlot = slot;
      m_selectedType = PLAYER_ABILITY;
      FrameScript_SignalEvent(245);
      return;
    }
    if (m_petSpells[slot] == spell) {
      m_selectedSlot = slot;
      m_selectedType = PET_SPELL;
      FrameScript_SignalEvent(245);
      return;
    }
  }

  FrameScript_SignalEvent(245);
}

static void PlaySpellDropSound(UI_SPELL_TYPE type) {
  switch (type) {
    case PLAYER_SPELL:
    case PET_SPELL:
      SndInterfacePlayInterfaceSound("igSpellBookSpellIconDrop");
      break;
    case PLAYER_ABILITY:
      SndInterfacePlayInterfaceSound("igAbilityIconDrop");
      break;
  }
}

static void PlaySpellPickupSound(UI_SPELL_TYPE type) {
  switch (type) {
    case PLAYER_SPELL:
    case PET_SPELL:
      SndInterfacePlayInterfaceSound("igSpellBookSpellIconPickup");
      break;
    case PLAYER_ABILITY:
      SndInterfacePlayInterfaceSound("igAbilityIconPickup");
      break;
  }
}

static void PlaySpellCastSound(UI_SPELL_TYPE type) {
  switch (type) {
    case PLAYER_SPELL:
    case PET_SPELL:
      SndInterfacePlayInterfaceSound("GAMESPELLACTIVATE");
      break;
    case PLAYER_ABILITY:
      SndInterfacePlayInterfaceSound("GAMEABILITYACTIVATE");
      break;
  }
}

void CGSpellBook::PickupSpell(int slot, UI_SPELL_TYPE type) {
  ASSERT(slot >= 0);
  ASSERT(slot < MAXIMUM_LEARNED_SPELLS);

  int  spellID = GetSpell(slot, type);
  int  cursorSpell = CGGameUI::GetCursorSpell();
  BOOL cursorWasPetSpell = CGGameUI::m_cursorItemType == UICURSOR_PET_SPELL;
  CGGameUI::ClearCursor(1);

  if (spellID == cursorSpell) {
    PlaySpellDropSound(type);
    return;
  }

  if (cursorSpell > 0) {
    const SpellRec *spell = g_spellDB.GetRecord(cursorSpell);
    if (!spell) {
      return;
    }

    BOOL isAbility = (spell->m_attributes & 0x10) != 0;
    if (!((type == PET_SPELL && cursorWasPetSpell) || (type == PLAYER_SPELL && !isAbility) || (type == PLAYER_ABILITY && isAbility))) {
      return;
    }

    for (UINT i = 0; i < MAXIMUM_LEARNED_SPELLS; ++i) {
      if (GetSpell(i, type) == cursorSpell) {
        SetSpell(i, spellID, type);
        SetSpell(slot, cursorSpell, type);
        PlaySpellDropSound(type);
        return;
      }
    }
    return;
  }

  if (spellID) {
    CGGameUI::SetCursorSpell(spellID, type == PET_SPELL);
    PlaySpellPickupSound(type);
  }
}

void CGSpellBook::CastSpell(int slot, UI_SPELL_TYPE type) {
  ASSERT(slot >= 0);
  ASSERT(slot < MAXIMUM_LEARNED_SPELLS);

  int cursorSpell = CGGameUI::GetCursorSpell();
  if (cursorSpell > 0) {
    PickupSpell(slot, type);
    return;
  }
  if (!CGGameUI::m_hasControl) {
    return;
  }

  int spellID = GetSpell(slot, type);
  CGGameUI::ClearCursor(1);
  if (spellID == cursorSpell) {
    return;
  }

  if (IsToggledSpell(slot, type)) {
    Spell_C_CancelAura(spellID);
    return;
  }
  if (spellID == Spell_C_GetTargettingSpell()) {
    Spell_C_StopTargeting();
    return;
  }

  if (type == PET_SPELL) {
    CDataStore msg;
    msg.Put(CMSG_PET_ACTION);
    msg.Put(CGPetInfo::GetPet());
    msg.Put((spellID & 0xFFFF) | 0x01000000);
    msg.Put(CGGameUI::GetLockedTarget());
    msg.Finalize();
    ClientServices_Send(&msg);
  } else {
    Spell_C_CastSpell(spellID, 0);
  }
  PlaySpellCastSound(type);
}

BOOL CGSpellBook::IsSelectedSlot(int slot, UI_SPELL_TYPE type) {
  ASSERT(slot >= 0);
  ASSERT(slot < MAXIMUM_LEARNED_SPELLS);

  if (m_selectedSlot == slot && m_selectedType == type) {
    return 1;
  }

  const SpellRec *spell = g_spellDB.GetRecord(GetSpell(slot, type));
  if (!spell) {
    return 0;
  }

  CGUnit_C *player = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player && spell->m_effect[0] == 47) {
    if (!spell->m_effectMiscValue[0]) {
      const SkillLineAbilityRec *ability = player->LookupAbility(spell->m_ID);
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
      if (form && player && player->GetShapeshiftForm() == form) {
        return 1;
      }
      return 0;
    }
  }
  return 0;
}

BOOL CGSpellBook::IsToggledSpell(int slot, UI_SPELL_TYPE type) {
  ASSERT(slot >= 0);
  ASSERT(slot < MAXIMUM_LEARNED_SPELLS);

  CGUnit_C *player = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return 0;
  }

  int             active = 0;
  const SpellRec *spell = g_spellDB.GetRecord(GetSpell(slot, type));
  if (spell && spell->m_activeIconID) {
    for (int i = 0; i < 40; ++i) {
      if (player->GetAura(i) == spell->m_ID && (player->GetAuraFlags(i) & 1)) {
        active = 1;
        break;
      }
    }
  }
  return active;
}

static BOOL GetSlotFromLua(lua_State *L, int &slot, UI_SPELL_TYPE &type) {
  if (lua_isnumber(L, 1) && lua_isstring(L, 2)) {
    int value = lua_tonumber(L, 1) - 1.0;
    if (value >= 0 && value < MAXIMUM_LEARNED_SPELLS) {
      slot = value;
      LPCSTR string = lua_tostring(L, 2);
      if (!SStrCmpI("spell", string, 0x7FFFFFFF)) {
        type = PLAYER_SPELL;
        return 1;
      }
      if (!SStrCmpI("ability", string, 0x7FFFFFFF)) {
        type = PLAYER_ABILITY;
        return 1;
      }
      if (!SStrCmpI("pet", string, 0x7FFFFFFF)) {
        type = PET_SPELL;
        return 1;
      }
    }
  }
  return 0;
}

static LPCSTR GetSpellbookTexture(int slot, UI_SPELL_TYPE type) {
  const SpellRec *spell = g_spellDB.GetRecord(CGSpellBook::GetSpell(slot, type));
  if (spell) {
    if (spell->m_effect[0] == 78) {
      return CGActionBar::GetAttackTexture();
    }
    const SpellIconRec *icon = g_spellIconDB.GetRecord(spell->m_spellIconID);
    if (icon) {
      return icon->m_textureFilename;
    }
  }
  return 0;
}

static int Script_GetSpellTexture(lua_State *L) {
  int           slot = 0;
  UI_SPELL_TYPE type;
  if (!GetSlotFromLua(L, slot, type)) {
    luaL_error(L, "Invalid spell slot in GetSpellTexture");
    return 0;
  }
  LPCSTR texture = GetSpellbookTexture(slot, type);
  if (texture && *texture) {
    lua_pushstring(L, texture);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_GetSpellName(lua_State *L) {
  int           slot = 0;
  UI_SPELL_TYPE type;
  if (!GetSlotFromLua(L, slot, type)) {
    luaL_error(L, "Invalid spell slot in GetSpellName");
    return 0;
  }
  const SpellRec *spell = g_spellDB.GetRecord(CGSpellBook::GetSpell(slot, type));
  if (spell) {
    lua_pushstring(L, spell->m_name_lang[CURRENT_LANGUAGE]);
    lua_pushstring(L, spell->m_nameSubtext_lang[CURRENT_LANGUAGE]);
  } else {
    lua_pushnil(L);
    lua_pushnil(L);
  }
  return 2;
}

static int Script_GetSpellCooldown(lua_State *L) {
  int           slot = 0;
  UI_SPELL_TYPE type;
  if (!GetSlotFromLua(L, slot, type)) {
    luaL_error(L, "Invalid spell slot in GetSpellCooldown");
    return 0;
  }
  int   spell = CGSpellBook::GetSpell(slot, type);
  DWORD startTime = 0;
  UINT  duration = 0;
  UINT  enable = 0;
  Spell_C_GetSpellCooldown(spell, type == PET_SPELL, &duration, &startTime, &enable);
  lua_pushnumber(L, (double)startTime * 0.001);
  lua_pushnumber(L, (double)duration * 0.001);
  lua_pushnumber(L, enable);
  return 3;
}

static int Script_PickupSpell(lua_State *L) {
  int           slot = 0;
  UI_SPELL_TYPE type;
  if (!GetSlotFromLua(L, slot, type)) {
    luaL_error(L, "Invalid spell slot in PickupSpell");
    return 0;
  }
  CGSpellBook::PickupSpell(slot, type);
  return 0;
}

static int Script_CastSpell(lua_State *L) {
  int           slot = 0;
  UI_SPELL_TYPE type;
  if (!GetSlotFromLua(L, slot, type)) {
    luaL_error(L, "Invalid spell slot in CastSpell");
    return 0;
  }
  CGSpellBook::CastSpell(slot, type);
  return 0;
}

static int Script_IsCurrentCast(lua_State *L) {
  int           slot = 0;
  UI_SPELL_TYPE type;
  if (!GetSlotFromLua(L, slot, type)) {
    luaL_error(L, "Invalid spell slot in IsCurrentCast");
    return 0;
  }
  if (CGSpellBook::IsSelectedSlot(slot, type)) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_UpdateSpells(lua_State *) {
  CGSpellBook::UpdateSpells();
  return 0;
}

static int Script_PlayerHasSpells(lua_State *L) {
  if (CGSpellBook::KnowsSpells()) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_HasPetSpells(lua_State *L) {
  if (CGSpellBook::KnowsPetSpells()) {
    lua_pushnumber(L, 1.0);
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (player) {
      lua_pushstring(L, g_chrClassesDB.GetRecord(player->GetClass())->m_petNameToken);
      return 2;
    }
    lua_pushstring(L, "PET");
    return 2;
  }
  lua_pushnil(L);
  lua_pushnil(L);
  return 2;
}

static int Script_IsSpellPassive(lua_State *L) {
  int           slot = 0;
  UI_SPELL_TYPE type;
  if (!GetSlotFromLua(L, slot, type)) {
    luaL_error(L, "Invalid spell slot in IsSpellPassive");
    return 0;
  }
  const SpellRec *spell = g_spellDB.GetRecord(CGSpellBook::GetSpell(slot, type));
  if (spell && (spell->m_attributes & 0x40)) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_GetNumShapeshiftForms(lua_State *L) {
  lua_pushnumber(L, CGSpellBook::GetShapeshiftForms().Count());
  return 1;
}

static int Script_GetShapeshiftFormInfo(lua_State *L) {
  if (lua_tonumber(L, 1) == 0.0) {
    luaL_error(L, "Usage: GetShapeshiftFormInfo(index)");
    return 0;
  }
  UINT                 index = (int)lua_tonumber(L, 1) - 1;
  TSGrowableArray<int> forms = CGSpellBook::GetShapeshiftForms();
  if (index < forms.Count()) {
    const SpellRec *spell = g_spellDB.GetRecord(forms[index]);
    if (spell) {
      const SpellIconRec *icon = g_spellIconDB.GetRecord(spell->m_spellIconID);
      if (icon) {
        lua_pushstring(L, icon->m_textureFilename);
      } else {
        lua_pushnil(L);
      }
      lua_pushstring(L, spell->m_name_lang[CURRENT_LANGUAGE]);

      int form = 0;
      for (UINT i = 0; i < 3; ++i) {
        if (spell->m_effectAura[i] == 36) {
          form = spell->m_effectMiscValue[i];
          break;
        }
      }

      CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
      if (player && form && player->GetShapeshiftForm() == form) {
        lua_pushnumber(L, 1.0);
      } else {
        lua_pushnil(L);
      }
      return 3;
    }
  }
  lua_pushnil(L);
  lua_pushnil(L);
  lua_pushnil(L);
  return 3;
}

static int Script_CastShapeshiftForm(lua_State *L) {
  if (lua_tonumber(L, 1) == 0.0) {
    luaL_error(L, "Usage: CastShapeshiftForm(index)");
    return 0;
  }
  UINT                 index = (int)lua_tonumber(L, 1) - 1;
  TSGrowableArray<int> forms = CGSpellBook::GetShapeshiftForms();
  if (index < forms.Count()) {
    const SpellRec *spell = g_spellDB.GetRecord(forms[index]);
    if (spell) {
      int form = 0;
      for (UINT i = 0; i < 3; ++i) {
        if (spell->m_effectAura[i] == 36) {
          form = spell->m_effectMiscValue[i];
          break;
        }
      }

      CGPlayer_C                   *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
      const SpellShapeshiftFormRec *formRec = g_spellShapeshiftFormDB.GetRecord(form);
      if (player && formRec && player->GetShapeshiftForm() == form && (formRec->m_flags & 1)) {
        return 0;
      }
      Spell_C_CastSpell(forms[index], 0);
      if (spell->m_attributes & 0x10) {
        SndInterfacePlayInterfaceSound("GAMEABILITYACTIVATE");
      } else {
        SndInterfacePlayInterfaceSound("GAMESPELLACTIVATE");
      }
    }
  }
  return 0;
}

static int Script_GetShapeshiftFormCooldown(lua_State *L) {
  if (lua_tonumber(L, 1) == 0.0) {
    luaL_error(L, "Usage: GetShapeshiftFormCooldown(index)");
    return 0;
  }
  UINT                 index = (int)lua_tonumber(L, 1) - 1;
  TSGrowableArray<int> forms = CGSpellBook::GetShapeshiftForms();
  if (index < forms.Count()) {
    DWORD startTime = 0;
    UINT  duration = 0;
    UINT  enable = 0;
    Spell_C_GetSpellCooldown(forms[index], 0, &duration, &startTime, &enable);
    lua_pushnumber(L, (double)startTime * 0.001);
    lua_pushnumber(L, (double)duration * 0.001);
    lua_pushnumber(L, enable);
    return 3;
  }
  lua_pushnumber(L, 0.0);
  lua_pushnumber(L, 0.0);
  lua_pushnumber(L, 1.0);
  return 3;
}

static FrameScript_Method s_ScriptFunctions[14] = {
    {          "GetSpellTexture",           Script_GetSpellTexture},
    {             "GetSpellName",              Script_GetSpellName},
    {         "GetSpellCooldown",          Script_GetSpellCooldown},
    {              "PickupSpell",               Script_PickupSpell},
    {                "CastSpell",                 Script_CastSpell},
    {            "IsCurrentCast",             Script_IsCurrentCast},
    {             "UpdateSpells",              Script_UpdateSpells},
    {          "PlayerHasSpells",           Script_PlayerHasSpells},
    {             "HasPetSpells",              Script_HasPetSpells},
    {           "IsSpellPassive",            Script_IsSpellPassive},
    {    "GetNumShapeshiftForms",     Script_GetNumShapeshiftForms},
    {    "GetShapeshiftFormInfo",     Script_GetShapeshiftFormInfo},
    {       "CastShapeshiftForm",        Script_CastShapeshiftForm},
    {"GetShapeshiftFormCooldown", Script_GetShapeshiftFormCooldown}
};

void SpellBookRegisterScriptFunctions() {
  for (UINT i = 0; i < 14; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void SpellBookUnregisterScriptFunctions() {
  for (UINT i = 0; i < 14; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
