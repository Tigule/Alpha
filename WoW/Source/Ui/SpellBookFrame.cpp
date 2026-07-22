#include "SpellBookFrame.h"

#include <DB/DBClient/AutoCode/LanguagesRec.h>
#include <DB/DBClient/AutoCode/SpellRec.h>
#include <DB/DBClient/AutoCode/SpellIconRec.h>
#include <DB/WowLocale.h>
#include <FrameScript/FrameScript.h>

#include "ChatFrame.h"
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

int __fastcall Spell_C_GetModalSpell();
class CGItem_C;
int __fastcall  Spell_C_GetTargettingSpell();
bool __fastcall Spell_C_CastSpell(int spellID, const CGItem_C *item);
int __fastcall  Spell_C_GetSpellCooldown(int spell, int isPet, unsigned int *duration, unsigned long *startTime, unsigned int *enable);

FBitField            CGSpellBook::m_knownSpellBits;
int                  CGSpellBook::m_knownSpells[1024];
int                  CGSpellBook::m_knownAbilities[1024];
int                  CGSpellBook::m_petSpells[1024];
int                  CGSpellBook::m_duelSpell;
int                  CGSpellBook::m_stuckSpell;
TSFixedArray<int>    CGSpellBook::m_languageSpells;
TSGrowableArray<int> CGSpellBook::m_unlockSpells;
TSGrowableArray<int> CGSpellBook::m_shapeshiftForms;
int                  CGSpellBook::m_selectedSlot = -1;
UI_SPELL_TYPE        CGSpellBook::m_selectedType;
int                  CGSpellBook::m_knowsSpells;
int                  CGSpellBook::m_knowsPetSpells;

void __fastcall CGSpellBook::InitializeGame() {
  m_knownSpellBits.SetCount(g_spellDB.GetMaxID() + 1);
  m_languageSpells.SetCount(g_languagesDB.GetMaxID() + 1);
  ClearSpells();
}

void __fastcall CGSpellBook::ShutdownGame() {
  m_knownSpellBits.SetCount(0);
  m_languageSpells.Clear();
}

void __fastcall CGSpellBook::ClearSpells() {
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

unsigned int __fastcall CGSpellBook::IsSpellKnown(int spellID) {
  return m_knownSpellBits.IsSet(spellID);
}

unsigned int __fastcall CGSpellBook::IsPetSpellKnown(int spellID) {
  for (unsigned int i = 0; i < 1024; ++i) {
    if (m_petSpells[i] == spellID) {
      return 1;
    }
  }
  return 0;
}

int __cdecl QSortShapeshiftForms(const void *a, const void *b) {
  FATALASSERT(a);
  FATALASSERT(b);

  SpellRec *spellA = g_spellDB.GetRecord(*static_cast<const int *>(a));
  SpellRec *spellB = g_spellDB.GetRecord(*static_cast<const int *>(b));
  if (!spellA || !spellB) {
    return 0;
  }

  if (spellA->m_spellLevel == spellB->m_spellLevel) {
    return SStrCmp(spellA->m_name_lang[CURRENT_LANGUAGE], spellB->m_name_lang[CURRENT_LANGUAGE], 0x7FFFFFFF);
  }

  return spellA->m_spellLevel > spellB->m_spellLevel ? 1 : -1;
}

void __fastcall CGSpellBook::AddKnownSpell(int spellID, int slot, int learned) {
  SpellRec *info = g_spellDB.GetRecord(spellID);
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
    if (info->m_effect[effect] != 36) {
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

  if (info->m_spellLevel > 0) {
    return;
  }

  unsigned int spellSlot;
  int          sendSpellSlot;
  if (!slot || static_cast<unsigned int>(abs(slot) - 1) >= 1024) {
    sendSpellSlot = 1;
  } else {
    spellSlot = abs(slot) - 1;
    sendSpellSlot = ability ? m_knownAbilities[spellSlot] > 0 : m_knownSpells[spellSlot] > 0;
  }

  if (sendSpellSlot) {
    spellSlot = ability && info->m_effect[0] != 78 ? 1 : 0;
    while (spellSlot < 1024) {
      int spell = ability ? m_knownAbilities[spellSlot] : m_knownSpells[spellSlot];
      if (spell <= 0) {
        break;
      }
      ++spellSlot;
    }
    if (spellSlot >= 1024) {
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

void __fastcall CGSpellBook::DelKnownSpell(int spellID) {
  int i;

  m_knownSpellBits.ClearBit(spellID);
  for (i = 0; i < 1024; ++i) {
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

  if (i < 1024) {
    UpdateSpells();
  }
}

void __fastcall CGSpellBook::ReplaceSpell(int oldSpell, int newSpell) {
  int slot = 0;
  for (unsigned int index = 0; index < 1024; ++index) {
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

void __fastcall CGSpellBook::ClearPetSpells() {
  memset(m_petSpells, 0, sizeof(m_petSpells));
  m_knowsPetSpells = 0;
}

void __fastcall CGSpellBook::AddPetSpell(int spellID) {
  SpellRec *info = g_spellDB.GetRecord(spellID);
  if (info && !(info->m_attributes & 0x80000000)) {
    for (unsigned int slot = 0; slot < 1024; ++slot) {
      if (m_petSpells[slot] <= 0) {
        m_petSpells[slot] = spellID;
        break;
      }
    }
    m_knowsPetSpells = 1;
  }
}

void __fastcall CGSpellBook::SetSpell(int slot, int spellID, UI_SPELL_TYPE type) {
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

void __fastcall CGSpellBook::SendSpellSlot(int slot, UI_SPELL_TYPE type) {
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

void __fastcall CGSpellBook::UpdateSpells() {
  FrameScript_SignalEvent(244);
}

void __fastcall CGSpellBook::UpdateCooldowns() {
  FrameScript_SignalEvent(207);
}

void __fastcall CGSpellBook::UpdateSelection() {
  int spell = Spell_C_GetModalSpell();
  if (!spell) {
    spell = Spell_C_GetTargettingSpell();
  }

  if (spell <= 0) {
    m_selectedSlot = -1;
    FrameScript_SignalEvent(245);
    return;
  }

  for (unsigned int slot = 0; slot < 1024; ++slot) {
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

void __fastcall PlaySpellDropSound(UI_SPELL_TYPE type) {
  SndInterfacePlayInterfaceSound("INTERFACESOUND_CURSORDROPOBJECT");
}

void __fastcall PlaySpellPickupSound(UI_SPELL_TYPE type) {
  SndInterfacePlayInterfaceSound("INTERFACESOUND_CURSORGRABOBJECT");
}

void __fastcall PlaySpellCastSound(UI_SPELL_TYPE type) {
  SndInterfacePlayInterfaceSound("INTERFACESOUND_ACTIONBUTTONDOWN");
}

int __fastcall CGSpellBook::GetSpell(unsigned int slot, UI_SPELL_TYPE type) {
  if (slot >= 1024) {
    return 0;
  }
  if (type == PLAYER_SPELL) {
    return m_knownSpells[slot];
  }
  if (type == PLAYER_ABILITY) {
    return m_knownAbilities[slot];
  }
  return type == PET_SPELL ? m_petSpells[slot] : 0;
}

void __fastcall CGSpellBook::PickupSpell(int slot, UI_SPELL_TYPE type) {
  int spellID = GetSpell(slot, type);
  if (!spellID) {
    CGGameUI::DropCursorSpell();
    return;
  }
  if (CGGameUI::GetCursorSpell() == spellID) {
    CGGameUI::DropCursorSpell();
    return;
  }
  CGGameUI::SetCursorSpell(spellID, type == PET_SPELL);
  PlaySpellPickupSound(type);
}

void __fastcall CGSpellBook::CastSpell(int slot, UI_SPELL_TYPE type) {
  int spellID = GetSpell(slot, type);
  if (!spellID) {
    return;
  }
  if (type == PET_SPELL) {
    PetAction        action(static_cast<unsigned int>(spellID) | 0x01000000);
    unsigned __int64 target = 0;
    CGPetInfo::SendPetAction(action, target);
  } else {
    Spell_C_CastSpell(spellID, 0);
  }
  PlaySpellCastSound(type);
}

int __fastcall CGSpellBook::IsSelectedSlot(int slot, UI_SPELL_TYPE type) {
  return slot == m_selectedSlot && type == m_selectedType;
}

int __fastcall CGSpellBook::IsToggledSpell(int slot, UI_SPELL_TYPE type) {
  int spell = GetSpell(slot, type);
  return spell && (spell == Spell_C_GetModalSpell() || spell == Spell_C_GetTargettingSpell());
}

static int __fastcall GetSlotFromLua(lua_State *L, int &slot, UI_SPELL_TYPE &type) {
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
  return slot >= 0 && slot < 1024;
}

static const char *__fastcall GetSpellbookTexture(int slot, UI_SPELL_TYPE type) {
  SpellRec     *spell = g_spellDB.GetRecord(CGSpellBook::GetSpell(slot, type));
  SpellIconRec *icon = spell ? g_spellIconDB.GetRecord(spell->m_spellIconID) : 0;
  return icon ? icon->m_textureFilename : 0;
}

static int __fastcall Script_GetSpellTexture(lua_State *L) {
  int           slot;
  UI_SPELL_TYPE type;
  if (!GetSlotFromLua(L, slot, type)) {
    return luaL_error(L, "Usage: GetSpellTexture(slot, bookType)");
  }
  const char *texture = GetSpellbookTexture(slot, type);
  texture ? lua_pushstring(L, texture) : lua_pushnil(L);
  return 1;
}

static int __fastcall Script_GetSpellName(lua_State *L) {
  int           slot;
  UI_SPELL_TYPE type;
  if (!GetSlotFromLua(L, slot, type)) {
    return luaL_error(L, "Usage: GetSpellName(slot, bookType)");
  }
  SpellRec *spell = g_spellDB.GetRecord(CGSpellBook::GetSpell(slot, type));
  if (!spell) {
    return 0;
  }
  lua_pushstring(L, spell->m_name_lang[CURRENT_LANGUAGE]);
  lua_pushstring(L, spell->m_nameSubtext_lang[CURRENT_LANGUAGE]);
  return 2;
}

static int __fastcall Script_GetSpellCooldown(lua_State *L) {
  int           slot;
  UI_SPELL_TYPE type;
  if (!GetSlotFromLua(L, slot, type)) {
    return luaL_error(L, "Usage: GetSpellCooldown(slot, bookType)");
  }
  unsigned int  duration = 0;
  unsigned long startTime = 0;
  unsigned int  enable = 0;
  int           spell = CGSpellBook::GetSpell(slot, type);
  if (spell) {
    Spell_C_GetSpellCooldown(spell, type == PET_SPELL, &duration, &startTime, &enable);
  }
  lua_pushnumber(L, static_cast<double>(startTime) * 0.001);
  lua_pushnumber(L, static_cast<double>(duration) * 0.001);
  lua_pushnumber(L, static_cast<double>(enable));
  return 3;
}

static int __fastcall Script_PickupSpell(lua_State *L) {
  int           slot;
  UI_SPELL_TYPE type;
  if (!GetSlotFromLua(L, slot, type)) {
    return luaL_error(L, "Usage: PickupSpell(slot, bookType)");
  }
  CGSpellBook::PickupSpell(slot, type);
  return 0;
}

static int __fastcall Script_CastSpell(lua_State *L) {
  int           slot;
  UI_SPELL_TYPE type;
  if (!GetSlotFromLua(L, slot, type)) {
    return luaL_error(L, "Usage: CastSpell(slot, bookType)");
  }
  CGSpellBook::CastSpell(slot, type);
  return 0;
}

static int __fastcall Script_IsCurrentCast(lua_State *L) {
  int           slot;
  UI_SPELL_TYPE type;
  if (!GetSlotFromLua(L, slot, type)) {
    return luaL_error(L, "Usage: IsCurrentCast(slot, bookType)");
  }
  CGSpellBook::IsSelectedSlot(slot, type) ? lua_pushnumber(L, 1.0) : lua_pushnil(L);
  return 1;
}

static int __fastcall Script_UpdateSpells(lua_State *__formal) {
  CGSpellBook::UpdateSpells();
  return 0;
}

static int __fastcall Script_PlayerHasSpells(lua_State *L) {
  CGSpellBook::KnowsSpells() ? lua_pushnumber(L, 1.0) : lua_pushnil(L);
  return 1;
}

static int __fastcall Script_HasPetSpells(lua_State *L) {
  if (CGSpellBook::KnowsPetSpells()) {
    lua_pushnumber(L, 1.0);
    lua_pushstring(L, "pet");
    return 2;
  }
  lua_pushnil(L);
  return 1;
}

static int __fastcall Script_IsSpellPassive(lua_State *L) {
  int           slot;
  UI_SPELL_TYPE type;
  if (!GetSlotFromLua(L, slot, type)) {
    return luaL_error(L, "Usage: IsPassiveSpell(slot, bookType)");
  }
  SpellRec *spell = g_spellDB.GetRecord(CGSpellBook::GetSpell(slot, type));
  spell && (spell->m_attributes & 0x40) ? lua_pushnumber(L, 1.0) : lua_pushnil(L);
  return 1;
}

static int __fastcall Script_GetNumShapeshiftForms(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGSpellBook::GetShapeshiftForms().Count()));
  return 1;
}

static int __fastcall Script_GetShapeshiftFormInfo(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetShapeshiftFormInfo(index)");
  }
  unsigned int          index = static_cast<unsigned int>(lua_tonumber(L, 1)) - 1;
  TSGrowableArray<int> &forms = CGSpellBook::GetShapeshiftForms();
  SpellRec             *spell = index < forms.Count() ? g_spellDB.GetRecord(forms[index]) : 0;
  SpellIconRec         *icon = spell ? g_spellIconDB.GetRecord(spell->m_spellIconID) : 0;
  icon ? lua_pushstring(L, icon->m_textureFilename) : lua_pushnil(L);
  spell ? lua_pushstring(L, spell->m_name_lang[CURRENT_LANGUAGE]) : lua_pushnil(L);

  int form = 0;
  if (spell) {
    for (unsigned int effect = 0; effect < 3; ++effect) {
      if (spell->m_effect[effect] == 36) {
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

static int __fastcall Script_CastShapeshiftForm(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: CastShapeshiftForm(index)");
  }
  unsigned int          index = static_cast<unsigned int>(lua_tonumber(L, 1)) - 1;
  TSGrowableArray<int> &forms = CGSpellBook::GetShapeshiftForms();
  if (index < forms.Count()) {
    Spell_C_CastSpell(forms[index], 0);
  }
  return 0;
}

static int __fastcall Script_GetShapeshiftFormCooldown(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetShapeshiftFormCooldown(index)");
  }
  unsigned int          index = static_cast<unsigned int>(lua_tonumber(L, 1)) - 1;
  TSGrowableArray<int> &forms = CGSpellBook::GetShapeshiftForms();
  unsigned int          duration = 0;
  unsigned long         startTime = 0;
  unsigned int          enable = 0;
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
    {           "IsPassiveSpell",            Script_IsSpellPassive},
    {    "GetNumShapeshiftForms",     Script_GetNumShapeshiftForms},
    {    "GetShapeshiftFormInfo",     Script_GetShapeshiftFormInfo},
    {       "CastShapeshiftForm",        Script_CastShapeshiftForm},
    {"GetShapeshiftFormCooldown", Script_GetShapeshiftFormCooldown}
};

void __fastcall SpellBookRegisterScriptFunctions() {
  for (unsigned int i = 0; i < 14; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void __fastcall SpellBookUnregisterScriptFunctions() {
  for (unsigned int i = 0; i < 14; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
