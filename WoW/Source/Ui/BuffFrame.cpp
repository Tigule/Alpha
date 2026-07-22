#include <storm.h>

#include "Object/ObjectClient/Unit_C.h"
#include "Object/ObjectClient/Player_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "DB/DBClient/AutoCode/SpellDurationRec.h"
#include "DB/DBClient/AutoCode/SpellIconRec.h"
#include "DB/DBClient/AutoCode/SpellRec.h"
#include "Base/CDataStore.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

#include <FrameScript/FrameScript.h>
#include <lauxlib.h>
#include <lua.h>
#include <string.h>

class CGBuffBar;

void __fastcall Spell_C_CancelAura(int spellID);

class CGBuffDesc {
  friend class CGBuffBar;

 public:
  CGBuffDesc();
  void SetAuraIndex(int index, CGPlayer_C *player);
  int  GetAuraIndex() const {
    return m_auraIndex;
  }
  int GetAuraSpell() const {
    return m_auraSpell;
  }
  unsigned int GetAuraFlags() const {
    return m_auraFlags;
  }
  int GetUntilCancelled() const {
    return m_untilCancelled;
  }

 protected:
  int          m_auraIndex;
  int          m_auraSpell;
  unsigned int m_auraFlags;
  int          m_untilCancelled;
};

class CGBuffBar {
 public:
  static void __fastcall         InitializeGame();
  static void __fastcall         ShutdownGame();
  static void __fastcall         EnterWorld();
  static void __fastcall         LeaveWorld();
  static void __fastcall         UpdateBuffs();
  static void __fastcall         UpdateDuration(unsigned char slot, unsigned int duration);
  static CGBuffDesc *__fastcall  GetBuffByFilter(int index, unsigned int filter, int &buffIndex);
  static CGBuffDesc *__fastcall  GetBuffByIndex(int buffIndex);
  static unsigned int __fastcall GetBuffTimeLeftByIndex(int buffIndex);

 private:
  static CGBuffDesc   m_buffs[56];
  static unsigned int m_durations[56];
};

CGBuffDesc   CGBuffBar::m_buffs[56];
unsigned int CGBuffBar::m_durations[56];

static int __fastcall AuraUpdateHandler(unsigned __int64, unsigned int, unsigned int, const void *, void *) {
  CGBuffBar::UpdateBuffs();
  return 1;
}

void __fastcall CGBuffBar::InitializeGame() {
  for (unsigned int i = 0; i < 56; ++i) {
    m_buffs[i].SetAuraIndex(-1, 0);
    m_durations[i] = 0;
  }
}

void __fastcall CGBuffBar::ShutdownGame() {
}

void __fastcall CGBuffBar::EnterWorld() {
  unsigned __int64 player = ClntObjMgrGetActivePlayer();
  unsigned int     unitOffset = CGUnit_C::OffsetOf(ID_UNIT);
  ClntObjMgrSetObjMirrorHandler(player, unitOffset + 200, 252, AuraUpdateHandler, 0, HANDLER_PRIORITY_NORMAL);
  UpdateBuffs();
}

void __fastcall CGBuffBar::LeaveWorld() {
  unsigned __int64 player = ClntObjMgrGetActivePlayer();
  unsigned int     unitOffset = CGUnit_C::OffsetOf(ID_UNIT);
  ClntObjMgrUnsetObjMirrorHandler(player, unitOffset + 200, AuraUpdateHandler, 0);
}

void __fastcall CGBuffBar::UpdateBuffs() {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return;
  }

  const unsigned long *storage = player->GetStorage();
  unsigned int         desc = 0;
  while (desc < 56 && m_buffs[desc].m_auraSpell > 0) {
    int       spellID = m_buffs[desc].m_auraSpell;
    SpellRec *spell = g_spellDB.GetRecord(spellID);
    if (spell && (spell->m_attributes < 0 || (spell->m_attributesEx & 0x10000000))) {
      ++desc;
      continue;
    }

    int aura;
    for (aura = 0; aura < 56; ++aura) {
      unsigned int flags = (reinterpret_cast<const unsigned char *>(storage)[424 + aura / 2] >> (4 * (aura % 2))) & 0xF;
      if (static_cast<int>(storage[50 + aura]) == spellID && (flags & 0xE)) {
        m_buffs[desc].SetAuraIndex(aura, player);
        ++desc;
        break;
      }
    }
    if (aura == 56) {
      memmove(&m_buffs[desc], &m_buffs[desc + 1], sizeof(CGBuffDesc) * (55 - desc));
      m_buffs[55].SetAuraIndex(-1, 0);
    }
  }

  for (int aura = 0; aura < 56; ++aura) {
    int          spellID = storage[50 + aura];
    unsigned int flags = (reinterpret_cast<const unsigned char *>(storage)[424 + aura / 2] >> (4 * (aura % 2))) & 0xF;
    SpellRec    *spell = g_spellDB.GetRecord(spellID);
    if (spellID <= 0 || !(flags & 0xE) || (spell && (spell->m_attributes < 0 || (spell->m_attributesEx & 0x10000000)))) {
      continue;
    }

    unsigned int index = 0;
    while (index < 56 && m_buffs[index].m_auraSpell > 0 && m_buffs[index].m_auraSpell != spellID) {
      ++index;
    }
    if (index < 56 && m_buffs[index].m_auraSpell <= 0) {
      m_buffs[index].SetAuraIndex(aura, player);
    }
  }
  FrameScript_SignalEvent(188);
}

void __fastcall CGBuffBar::UpdateDuration(unsigned char slot, unsigned int duration) {
  if (slot < 56) {
    m_durations[slot] = duration + GetTickCount();
  }
}

inline CGBuffDesc *__fastcall CGBuffBar::GetBuffByFilter(int index, unsigned int filter, int &buffIndex) {
  for (int i = 0; i < 56; ++i) {
    CGBuffDesc &buff = m_buffs[i];
    if (buff.m_auraSpell <= 0) {
      break;
    }
    bool matches = true;
    if ((filter & 1) && !(buff.m_auraFlags & 2)) {
      matches = false;
    }
    if ((filter & 2) && !(buff.m_auraFlags & 4)) {
      matches = false;
    }
    if ((filter & 4) && buff.m_untilCancelled) {
      matches = false;
    }
    if (matches && !index--) {
      buffIndex = i;
      return &buff;
    }
  }
  buffIndex = -1;
  return 0;
}

inline CGBuffDesc *__fastcall CGBuffBar::GetBuffByIndex(int buffIndex) {
  return buffIndex >= 0 && buffIndex < 56 && m_buffs[buffIndex].m_auraSpell > 0 ? &m_buffs[buffIndex] : 0;
}

unsigned int __fastcall CGBuffBar::GetBuffTimeLeftByIndex(int buffIndex) {
  if (buffIndex < 0 || buffIndex >= 56 || !m_durations[buffIndex]) {
    return 0;
  }
  unsigned int now = GetTickCount();
  return m_durations[buffIndex] > now ? m_durations[buffIndex] - now : 0;
}

CGBuffDesc::CGBuffDesc() : m_auraIndex(-1), m_auraSpell(0), m_auraFlags(0), m_untilCancelled(0) {
}

void CGBuffDesc::SetAuraIndex(int index, CGPlayer_C *player) {
  m_auraIndex = index;
  if (index == -1) {
    m_auraSpell = 0;
    return;
  }

  const unsigned long *storage = player->GetStorage();
  m_auraSpell = storage[50 + index];
  m_auraFlags = (reinterpret_cast<const unsigned char *>(storage)[424 + index / 2] >> (4 * (index % 2))) & 0xF;
  SpellRec *spell = g_spellDB.GetRecord(m_auraSpell);
  if (spell) {
    SpellDurationRec *duration = g_spellDurationDB.GetRecord(spell->m_durationIndex);
    m_untilCancelled = !duration || duration->m_duration < 0;
  }
}

static int __fastcall Script_GetPlayerBuff(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetPlayerBuff(index, filter)");
  }
  unsigned int filter = 0;
  if (lua_isstring(L, 2)) {
    const char *cursor = lua_tostring(L, 2);
    if (strstr(cursor, "HELPFUL"))
      filter |= 1;
    if (strstr(cursor, "HARMFUL"))
      filter |= 2;
    if (strstr(cursor, "CANCELABLE"))
      filter |= 4;
  }
  int         buffIndex;
  CGBuffDesc *buff = CGBuffBar::GetBuffByFilter(static_cast<int>(lua_tonumber(L, 1)), filter, buffIndex);
  if (!buff) {
    lua_pushnumber(L, -1.0);
    return 1;
  }
  lua_pushnumber(L, static_cast<double>(buffIndex));
  lua_pushnumber(L, static_cast<double>(buff->GetAuraIndex()));
  lua_pushnumber(L, static_cast<double>(buff->GetAuraSpell()));
  return 3;
}

static int __fastcall Script_GetPlayerBuffTexture(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetPlayerBuffTexture(index)");
  }
  CGBuffDesc   *buff = CGBuffBar::GetBuffByIndex(static_cast<int>(lua_tonumber(L, 1)));
  SpellRec     *spell = buff ? g_spellDB.GetRecord(buff->GetAuraSpell()) : 0;
  SpellIconRec *icon = spell ? g_spellIconDB.GetRecord(spell->m_spellIconID) : 0;
  icon ? lua_pushstring(L, icon->m_textureFilename) : lua_pushnil(L);
  return 1;
}

static int __fastcall Script_GetPlayerBuffTimeLeft(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetPlayerBuffTimeLeft(index)");
  }
  lua_pushnumber(L, static_cast<double>(CGBuffBar::GetBuffTimeLeftByIndex(static_cast<int>(lua_tonumber(L, 1)))) * 0.001);
  return 1;
}

static int __fastcall Script_CancelPlayerBuff(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: CancelPlayerBuff(index)");
  }
  CGBuffDesc *buff = CGBuffBar::GetBuffByIndex(static_cast<int>(lua_tonumber(L, 1)));
  if (buff && !buff->GetUntilCancelled()) {
    Spell_C_CancelAura(buff->GetAuraSpell());
  }
  return 0;
}

static FrameScript_Method s_ScriptFunctions[4] = {
    {        "GetPlayerBuff",         Script_GetPlayerBuff},
    { "GetPlayerBuffTexture",  Script_GetPlayerBuffTexture},
    {"GetPlayerBuffTimeLeft", Script_GetPlayerBuffTimeLeft},
    {     "CancelPlayerBuff",      Script_CancelPlayerBuff}
};

void __fastcall BuffBarRegisterScriptFunctions() {
  for (unsigned int i = 0; i < 4; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void __fastcall BuffBarUnregisterScriptFunctions() {
  for (unsigned int i = 0; i < 4; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
