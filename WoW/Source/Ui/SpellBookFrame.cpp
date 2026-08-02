#include <Base/Base.h>
#include <WowConst.h>
#include <MapDefs.h>

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
#include "GameUI.h"
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
int Spell_C_GetTargettingSpell();
bool Spell_C_CastSpell(int spellID, const CGItem_C *item);
int Spell_C_GetSpellCooldown(int spell, int isPet, unsigned int *duration, unsigned long *startTime, unsigned int *enable);
void Spell_C_StopTargeting();
void Spell_C_CancelAura(int spellID);

class CGTradeSkillInfo {
  friend class CGSpellBook;

 private:
  static int m_skillLine;
};

class CGCraftInfo {
  friend class CGSpellBook;

 private:
  static SPELL_CAST_UI_TYPE m_craftType;
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
  m_knownSpellBits.SetCount(0);
  m_languageSpells.Clear();
}

void CGSpellBook::ClearSpells() {
  unsigned int i;

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

int __cdecl QSortShapeshiftForms(const void *a, const void *b) {
  FATALASSERT(a);
  FATALASSERT(b);

  const SpellRec *spellA = g_spellDB.GetRecord(*static_cast<const int *>(a));
  const SpellRec *spellB = g_spellDB.GetRecord(*static_cast<const int *>(b));
  if (!spellA || !spellB) {
    return 0;
  }

  if (spellA->m_spellLevel == spellB->m_spellLevel) {
    return SStrCmp(spellA->m_name_lang[CURRENT_LANGUAGE], spellB->m_name_lang[CURRENT_LANGUAGE], 0x7FFFFFFF);
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
    FATALASSERT(info->m_effectMiscValue[0] < static_cast<int>(m_languageSpells.Count()));
    m_languageSpells[info->m_effectMiscValue[0]] = spellID;
    CGChat::UpdateLanguages();
  }
  if (info->m_effect[0] == 33) {
    m_unlockSpells.Add(&spellID);
  }

  for (unsigned int effect = 0; effect < 3; ++effect) {
    if (info->m_effectAura[effect] != 36) {
      continue;
    }

    unsigned int unlock;
    for (unlock = 0; unlock < m_shapeshiftForms.Count(); ++unlock) {
      if (m_shapeshiftForms[unlock] == spellID) {
        break;
      }
    }
    if (unlock == m_shapeshiftForms.Count()) {
      m_shapeshiftForms.Add(&spellID);
      qsort(m_shapeshiftForms.Ptr(), m_shapeshiftForms.Count(), sizeof(int), QSortShapeshiftForms);
      FrameScript_SignalEvent(370);
    }
    break;
  }

  if (info->m_attributes & 0x80) {
    return;
  }
  if (info->m_attributes & 0x20) {
    if (learned) {
      CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(50), info->m_name_lang[CURRENT_LANGUAGE]);
    }
    return;
  }

  int ability = info->m_attributes & 0x10;
  if (learned) {
    CGTutorial::TriggerTutorial(TUTORIAL_ABILITIES);
    CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(ability ? 49 : 48), info->m_name_lang[CURRENT_LANGUAGE]);
  }

  if (info->m_castUI > 0) {
    return;
  }

  unsigned int spellSlot;
  int          sendSpellSlot;
  if (!slot || static_cast<unsigned int>(abs(slot) - 1) >= MAXIMUM_LEARNED_SPELLS) {
    sendSpellSlot = 1;
  } else {
    spellSlot = abs(slot) - 1;
    sendSpellSlot = ability ? m_knownAbilities[spellSlot] > 0 : m_knownSpells[spellSlot] > 0;
  }

  if (sendSpellSlot) {
    spellSlot = ability && info->m_effect[0] != 78 ? 1 : 0;
    while (spellSlot < MAXIMUM_LEARNED_SPELLS) {
      int spell = ability ? m_knownAbilities[spellSlot] : m_knownSpells[spellSlot];
      if (spell <= 0) {
        break;
      }
      ++spellSlot;
    }
    if (spellSlot >= MAXIMUM_LEARNED_SPELLS) {
      return;
    }
  }

  if (ability) {
    m_knownAbilities[spellSlot] = spellID;
  } else {
    m_knownSpells[spellSlot] = spellID;
    ++m_knowsSpells;
  }
  if (learned) {
    UpdateSpells();
  }
  if (sendSpellSlot) {
    SendSpellSlot(spellSlot, ability ? PLAYER_ABILITY : PLAYER_SPELL);
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

  for (unsigned int language = m_languageSpells.Count(); language;) {
    --language;
    if (m_languageSpells[language] == spellID) {
      m_languageSpells[language] = 0;
    }
  }

  for (unsigned int form = m_shapeshiftForms.Count(); form;) {
    --form;
    if (m_shapeshiftForms[form] == spellID) {
      unsigned int remaining = m_shapeshiftForms.Count() - form - 1;
      if (remaining) {
        memmove(&m_shapeshiftForms[form], &m_shapeshiftForms[form + 1], remaining * sizeof(int));
      }
      m_shapeshiftForms.SetCount(m_shapeshiftForms.Count() - 1);
      break;
    }
  }

  if (i < MAXIMUM_LEARNED_SPELLS) {
    UpdateSpells();
  }
}

void CGSpellBook::ReplaceSpell(int oldSpell, int newSpell) {
  int slot = 0;
  for (unsigned int index = 0; index < MAXIMUM_LEARNED_SPELLS; ++index) {
    if (m_knownSpells[index] == oldSpell) {
      slot = index + 1;
      break;
    }
    if (m_knownAbilities[index] == oldSpell) {
      slot = -1 - static_cast<int>(index);
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
  if (info && !(info->m_attributes & 0x80000000)) {
    for (unsigned int slot = 0; slot < MAXIMUM_LEARNED_SPELLS; ++slot) {
      if (m_petSpells[slot] <= 0) {
        m_petSpells[slot] = spellID;
        break;
      }
    }
    m_knowsPetSpells = 1;
  }
}

void CGSpellBook::SetSpell(int slot, int spellID, UI_SPELL_TYPE type) {
  if (type == PLAYER_SPELL) {
    m_knownSpells[slot] = spellID;
  } else if (type == PLAYER_ABILITY) {
    m_knownAbilities[slot] = spellID;
  } else if (type == PET_SPELL) {
    m_petSpells[slot] = spellID;
  }

  SendSpellSlot(slot, type);
  UpdateSpells();
}

void CGSpellBook::SendSpellSlot(int slot, UI_SPELL_TYPE type) {
  if (type == PET_SPELL) {
    return;
  }

  int spellID;
  if (type == PLAYER_SPELL) {
    spellID = m_knownSpells[slot];
  } else if (type == PLAYER_ABILITY) {
    spellID = m_knownAbilities[slot];
  } else {
    return;
  }

  if (spellID) {
    CDataStore msg;
    msg.Put(CMSG_NEW_SPELL_SLOT);
    msg.Put(spellID);
    msg.Put(type == PLAYER_ABILITY ? -1 - slot : slot + 1);
    msg.Finalize();
    ClientServices_Send(&msg);
  }
}

void CGSpellBook::UpdateSpells() {
  FrameScript_SignalEvent(244);
}

void CGSpellBook::UpdateCooldowns() {
  FrameScript_SignalEvent(207);
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

  for (unsigned int slot = 0; slot < MAXIMUM_LEARNED_SPELLS; ++slot) {
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

void PlaySpellDropSound(UI_SPELL_TYPE type) {
  SndInterfacePlayInterfaceSound("INTERFACESOUND_CURSORDROPOBJECT");
}

void PlaySpellPickupSound(UI_SPELL_TYPE type) {
  SndInterfacePlayInterfaceSound("INTERFACESOUND_CURSORGRABOBJECT");
}

void PlaySpellCastSound(UI_SPELL_TYPE type) {
  SndInterfacePlayInterfaceSound("INTERFACESOUND_ACTIONBUTTONDOWN");
}

void CGSpellBook::PickupSpell(int slot, UI_SPELL_TYPE type) {
  ASSERT(slot >= 0);
  ASSERT(slot < MAXIMUM_LEARNED_SPELLS);

  int spellID = GetSpell(slot, type);
  int cursorSpell = CGGameUI::GetCursorSpell();
  int cursorWasPetSpell = CGGameUI::m_cursorItemType == UICURSOR_PET_SPELL;
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

    int isAbility = (spell->m_attributes & 0x10) != 0;
    if ((type == PET_SPELL && !cursorWasPetSpell) ||
        (type == PLAYER_ABILITY && !isAbility) ||
        (type == PLAYER_SPELL && isAbility))
    {
      return;
    }

    for (int cursorSlot = 0; cursorSlot < MAXIMUM_LEARNED_SPELLS; ++cursorSlot) {
      if (GetSpell(cursorSlot, type) == cursorSpell) {
        SetSpell(cursorSlot, spellID, type);
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

int CGSpellBook::IsSelectedSlot(int slot, UI_SPELL_TYPE type) {
  ASSERT(slot >= 0);
  ASSERT(slot < MAXIMUM_LEARNED_SPELLS);

  if (slot == m_selectedSlot && type == m_selectedType) {
    return 1;
  }

  const SpellRec *spell = g_spellDB.GetRecord(GetSpell(slot, type));
  if (!spell) {
    return 0;
  }

  CGUnit_C *player = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player && spell->m_effect[0] == 47) {
    if (spell->m_effectMiscValue[0]) {
      if (CGCraftInfo::m_craftType == spell->m_effectMiscValue[0]) {
        return 1;
      }
    } else {
      const SkillLineAbilityRec *ability = player->LookupAbility(spell->m_ID);
      if (ability && CGTradeSkillInfo::m_skillLine == ability->m_skillLine) {
        return 1;
      }
    }
  }

  unsigned int effect;
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

int CGSpellBook::IsToggledSpell(int slot, UI_SPELL_TYPE type) {
  ASSERT(slot >= 0);
  ASSERT(slot < MAXIMUM_LEARNED_SPELLS);

  CGUnit_C *player = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return 0;
  }

  int active = 0;
  const SpellRec *spell = g_spellDB.GetRecord(GetSpell(slot, type));
  if (!spell || !spell->m_activeIconID) {
    return 0;
  }

  const CGUnitData      *unitData = player->GetUnitData();
  const unsigned char   *auraFlags = unitData->auraFlags;
  for (unsigned int aura = 0; aura < 40; ++aura) {
    if (unitData->auras[aura] == spell->m_ID && ((auraFlags[aura / 2] >> (4 * (aura % 2))) & 1)) {
      active = 1;
      break;
    }
  }
  return active;
}

static int GetSlotFromLua(lua_State *L, int &slot, UI_SPELL_TYPE &type) {
  if (!lua_isnumber(L, 1) || !lua_isstring(L, 2)) {
    return 0;
  }
  slot = static_cast<int>(lua_tonumber(L, 1)) - 1;
  const char *bookType = lua_tostring(L, 2);
  if (!SStrCmpI(bookType, "spell", 0x7FFFFFFF)) {
    type = PLAYER_SPELL;
  } else if (!SStrCmpI(bookType, "ability", 0x7FFFFFFF)) {
    type = PLAYER_ABILITY;
  } else if (!SStrCmpI(bookType, "pet", 0x7FFFFFFF)) {
    type = PET_SPELL;
  } else {
    return 0;
  }
  return slot >= 0 && slot < MAXIMUM_LEARNED_SPELLS;
}

static const char *GetSpellbookTexture(int slot, UI_SPELL_TYPE type) {
  const SpellRec *spell = g_spellDB.GetRecord(CGSpellBook::GetSpell(slot, type));
  if (spell && spell->m_effect[0] == 78) {
    return CGActionBar::GetAttackTexture();
  }
  const SpellIconRec *icon = spell ? g_spellIconDB.GetRecord(spell->m_spellIconID) : 0;
  return icon ? icon->m_textureFilename : 0;
}

static int Script_GetSpellTexture(lua_State *L) {
  int           slot;
  UI_SPELL_TYPE type;
  if (!GetSlotFromLua(L, slot, type)) {
    return luaL_error(L, "Invalid spell slot in GetSpellTexture");
  }
  const char *texture = GetSpellbookTexture(slot, type);
  if (texture && *texture) {
    lua_pushstring(L, texture);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_GetSpellName(lua_State *L) {
  int           slot;
  UI_SPELL_TYPE type;
  if (!GetSlotFromLua(L, slot, type)) {
    return luaL_error(L, "Invalid spell slot in GetSpellName");
  }
  const SpellRec *spell = g_spellDB.GetRecord(CGSpellBook::GetSpell(slot, type));
  if (!spell) {
    lua_pushnil(L);
    lua_pushnil(L);
    return 2;
  }
  lua_pushstring(L, spell->m_name_lang[CURRENT_LANGUAGE]);
  lua_pushstring(L, spell->m_nameSubtext_lang[CURRENT_LANGUAGE]);
  return 2;
}

static int Script_GetSpellCooldown(lua_State *L) {
  int           slot;
  UI_SPELL_TYPE type;
  if (!GetSlotFromLua(L, slot, type)) {
    return luaL_error(L, "Invalid spell slot in GetSpellCooldown");
  }
  unsigned int  duration = 0;
  unsigned long startTime = 0;
  unsigned int  enable = 0;
  int           spell = CGSpellBook::GetSpell(slot, type);
  Spell_C_GetSpellCooldown(spell, type == PET_SPELL, &duration, &startTime, &enable);
  lua_pushnumber(L, static_cast<double>(startTime) * 0.001);
  lua_pushnumber(L, static_cast<double>(duration) * 0.001);
  lua_pushnumber(L, static_cast<double>(enable));
  return 3;
}

static int Script_PickupSpell(lua_State *L) {
  int           slot;
  UI_SPELL_TYPE type;
  if (!GetSlotFromLua(L, slot, type)) {
    return luaL_error(L, "Invalid spell slot in PickupSpell");
  }
  CGSpellBook::PickupSpell(slot, type);
  return 0;
}

static int Script_CastSpell(lua_State *L) {
  int           slot;
  UI_SPELL_TYPE type;
  if (!GetSlotFromLua(L, slot, type)) {
    return luaL_error(L, "Invalid spell slot in CastSpell");
  }
  CGSpellBook::CastSpell(slot, type);
  return 0;
}

static int Script_IsCurrentCast(lua_State *L) {
  int           slot;
  UI_SPELL_TYPE type;
  if (!GetSlotFromLua(L, slot, type)) {
    return luaL_error(L, "Invalid spell slot in IsCurrentCast");
  }
  if (CGSpellBook::IsSelectedSlot(slot, type)) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_UpdateSpells(lua_State *__formal) {
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
    CGPlayer_C *player = static_cast<CGPlayer_C *>(
        ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    const ChrClassesRec *classRec =
        player ? g_chrClassesDB.GetRecord(player->GetUnitData()->classId) : 0;
    lua_pushstring(L, classRec ? classRec->m_petNameToken : "PET");
    return 2;
  }
  lua_pushnil(L);
  lua_pushnil(L);
  return 2;
}

static int Script_IsSpellPassive(lua_State *L) {
  int           slot;
  UI_SPELL_TYPE type;
  if (!GetSlotFromLua(L, slot, type)) {
    return luaL_error(L, "Invalid spell slot in IsSpellPassive");
  }
  int spellID = CGSpellBook::GetSpell(slot, type);
  const SpellRec *spell = spellID >= 0 ? g_spellDB.GetRecord(spellID) : 0;
  if (spell && (spell->m_attributes & 0x40)) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_GetNumShapeshiftForms(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGSpellBook::GetShapeshiftForms().Count()));
  return 1;
}

static int Script_GetShapeshiftFormInfo(lua_State *L) {
  if (lua_tonumber(L, 1) == 0.0) {
    return luaL_error(L, "Usage: GetShapeshiftFormInfo(index)");
  }
  unsigned int          index = static_cast<unsigned int>(lua_tonumber(L, 1)) - 1;
  TSGrowableArray<int> forms = CGSpellBook::GetShapeshiftForms();
  const SpellRec            *spell = index < forms.Count() ? g_spellDB.GetRecord(forms[index]) : 0;
  const SpellIconRec         *icon = spell ? g_spellIconDB.GetRecord(spell->m_spellIconID) : 0;
  if (icon) {
    lua_pushstring(L, icon->m_textureFilename);
  } else {
    lua_pushnil(L);
  }
  if (spell) {
    lua_pushstring(L, spell->m_name_lang[CURRENT_LANGUAGE]);
  } else {
    lua_pushnil(L);
  }

  int form = 0;
  if (spell) {
    for (unsigned int effect = 0; effect < 3; ++effect) {
      if (spell->m_effectAura[effect] == 36) {
        form = spell->m_effectMiscValue[effect];
        break;
      }
    }
  }

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player && form && player->GetUnitData()->shapeshiftForm == form) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 3;
}

static int Script_CastShapeshiftForm(lua_State *L) {
  if (lua_tonumber(L, 1) == 0.0) {
    return luaL_error(L, "Usage: CastShapeshiftForm(index)");
  }
  unsigned int          index = static_cast<unsigned int>(lua_tonumber(L, 1)) - 1;
  TSGrowableArray<int> forms = CGSpellBook::GetShapeshiftForms();
  if (index < forms.Count()) {
    const SpellRec *spell = g_spellDB.GetRecord(forms[index]);
    if (spell) {
      int form = 0;
      for (unsigned int effect = 0; effect < 3; ++effect) {
        if (spell->m_effectAura[effect] == 36) {
          form = spell->m_effectMiscValue[effect];
          break;
        }
      }

      CGPlayer_C *player = static_cast<CGPlayer_C *>(
          ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
      const SpellShapeshiftFormRec *formRec = g_spellShapeshiftFormDB.GetRecord(form);
      if (!player || !formRec || player->GetUnitData()->shapeshiftForm != form ||
          !(formRec->m_flags & 1)) {
        Spell_C_CastSpell(forms[index], 0);
        SndInterfacePlayInterfaceSound(
            spell->m_attributes & 0x10 ? "GAMEABILITYACTIVATE" : "GAMESPELLACTIVATE");
      }
    }
  }
  return 0;
}

static int Script_GetShapeshiftFormCooldown(lua_State *L) {
  if (lua_tonumber(L, 1) == 0.0) {
    return luaL_error(L, "Usage: GetShapeshiftFormCooldown(index)");
  }
  unsigned int          index = static_cast<unsigned int>(lua_tonumber(L, 1)) - 1;
  TSGrowableArray<int> forms = CGSpellBook::GetShapeshiftForms();
  unsigned int         duration = 0;
  unsigned long         startTime = 0;
  unsigned int          enable = 1;
  if (index < forms.Count()) {
    Spell_C_GetSpellCooldown(forms[index], 0, &duration, &startTime, &enable);
  }
  lua_pushnumber(L, static_cast<double>(startTime) * 0.001);
  lua_pushnumber(L, static_cast<double>(duration) * 0.001);
  lua_pushnumber(L, static_cast<double>(enable));
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
  for (unsigned int i = 0; i < 14; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void SpellBookUnregisterScriptFunctions() {
  for (unsigned int i = 0; i < 14; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
