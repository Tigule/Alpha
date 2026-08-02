#include <Base/Base.h>
#include <WowConst.h>
#include <MapDefs.h>

#include <storm.h>
#include <Os/OsTime.h>

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

void Spell_C_CancelAura(int spellID);

class CGBuffDesc {
  friend class CGBuffBar;

 public:
  CGBuffDesc();
  __forceinline ~CGBuffDesc() {
  }
  void SetAuraIndex(int index, CGPlayer_C *player);
  int  GetAuraIndex() const {
    return m_auraIndex;
  }
  int GetAuraSpell() const {
    return m_auraSpell;
  }
  unsigned char GetAuraFlags() const {
    return m_auraFlags;
  }
  int GetUntilCancelled() const {
    return m_untilCancelled;
  }

 protected:
  int          m_auraIndex;
  int          m_auraSpell;
  unsigned char m_auraFlags;
  int           m_untilCancelled;
};

class CGBuffBar {
 public:
  static void InitializeGame();
  static void ShutdownGame();
  static void EnterWorld();
  static void LeaveWorld();
  static void UpdateBuffs();
  static void UpdateDuration(unsigned char slot, unsigned int duration);
  static const CGBuffDesc *GetBuffByFilter(int index, unsigned int filter, int &buffIndex);
  static const CGBuffDesc *GetBuffByIndex(int buffIndex);
  static unsigned int GetBuffTimeLeftByIndex(int buffIndex);

 private:
  static CGBuffDesc   m_buffs[56];
  static unsigned int m_durations[56];
};

CGBuffDesc   CGBuffBar::m_buffs[56];
unsigned int CGBuffBar::m_durations[56];

static int AuraUpdateHandler(
    unsigned __int64,
    unsigned int offset,
    unsigned int bytes,
    const void *,
    void *
) {
  CGBuffBar::UpdateBuffs();
  return 1;
}

void CGBuffBar::InitializeGame() {
  for (unsigned int i = 0; i < 56; ++i) {
    m_buffs[i].SetAuraIndex(-1, 0);
    m_durations[i] = 0;
  }
}

void CGBuffBar::ShutdownGame() {
}

void CGBuffBar::EnterWorld() {
  unsigned __int64 player = ClntObjMgrGetActivePlayer();
  unsigned int     unitOffset = CGUnit_C::OffsetOf(ID_UNIT);
  ClntObjMgrSetObjMirrorHandler(player, unitOffset + 200, 252, AuraUpdateHandler, 0, HANDLER_PRIORITY_NORMAL);
  UpdateBuffs();
}

void CGBuffBar::LeaveWorld() {
  unsigned __int64 player = ClntObjMgrGetActivePlayer();
  unsigned int     unitOffset = CGUnit_C::OffsetOf(ID_UNIT);
  ClntObjMgrUnsetObjMirrorHandler(player, unitOffset + 200, AuraUpdateHandler, 0);
}

void CGBuffBar::UpdateBuffs() {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(
      ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__)
  );
  if (!player) {
    return;
  }

  unsigned int      desc = 0;
  while (desc < 56 && m_buffs[desc].m_auraSpell > 0) {
    int             id = m_buffs[desc].m_auraSpell;
    const SpellRec *spell = g_spellDB.GetRecord(id);
    if (spell && (static_cast<signed char>(spell->m_attributes) < 0 || (spell->m_attributesEx & 0x10000000))) {
      continue;
    }

    int aura;
    for (aura = 0; aura < 56; ++aura) {
      unsigned int flags = (player->GetUnitData()->auraFlags[aura / 2] >> (4 * (aura % 2))) & 0xF;
      if (player->GetUnitData()->auras[aura] == id && (flags & 0xE)) {
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
    int          spellID = player->GetUnitData()->auras[aura];
    unsigned int flags = (player->GetUnitData()->auraFlags[aura / 2] >> (4 * (aura % 2))) & 0xF;
    const SpellRec    *spell = g_spellDB.GetRecord(spellID);
    if (spellID <= 0 || !(flags & 0xE) ||
        (spell && (static_cast<signed char>(spell->m_attributes) < 0 || (spell->m_attributesEx & 0x10000000)))) {
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

void CGBuffBar::UpdateDuration(unsigned char slot, unsigned int duration) {
  if (slot < 56) {
    m_durations[slot] = duration + OsGetAsyncTimeMs();
  }
}

const CGBuffDesc *CGBuffBar::GetBuffByFilter(int index, unsigned int filter, int &buffIndex) {
  for (int i = 0; i < 56; ++i) {
    CGBuffDesc &buff = m_buffs[i];
    bool matches = buff.m_auraIndex >= 0;
    if (matches) {
      if (buff.m_auraIndex < 32) {
        matches = (filter & 1) != 0;
      } else if (buff.m_auraIndex < 40) {
        matches = (filter & 2) != 0;
      } else {
        matches = (filter & 4) != 0;
      }
    }
    if (matches && (filter & 0x10) && !(buff.m_auraFlags & 1)) {
      matches = false;
    }
    if (matches && (filter & 0x20) && (buff.m_auraFlags & 1)) {
      matches = false;
    }
    if (matches && index-- == 0) {
      buffIndex = i;
      return &buff;
    }
  }
  return 0;
}

const CGBuffDesc *CGBuffBar::GetBuffByIndex(int buffIndex) {
  ASSERT(buffIndex >= 0 && buffIndex < 56);
  return &m_buffs[buffIndex];
}

unsigned int CGBuffBar::GetBuffTimeLeftByIndex(int buffIndex) {
  const CGBuffDesc *buff = GetBuffByIndex(buffIndex);
  if (buff->m_auraIndex < 0) {
    return 0;
  }
  unsigned int now = OsGetAsyncTimeMs();
  unsigned int expiry = m_durations[buff->m_auraIndex];
  return static_cast<int>(now - expiry) < 0 ? expiry - now : 0;
}

CGBuffDesc::CGBuffDesc() : m_auraIndex(-1), m_auraSpell(0), m_untilCancelled(0) {
}

void CGBuffDesc::SetAuraIndex(int index, CGPlayer_C *player) {
  m_auraIndex = index;
  if (index == -1) {
    m_auraSpell = 0;
    return;
  }

  const CGUnitData *unitData = player->GetUnitData();
  m_auraSpell = unitData->auras[index];
  m_auraFlags = (unitData->auraFlags[index / 2] >> (4 * (index % 2))) & 0xF;
  const SpellRec *spell = g_spellDB.GetRecord(m_auraSpell);
  if (spell) {
    const SpellDurationRec *duration = g_spellDurationDB.GetRecord(spell->m_durationIndex);
    m_untilCancelled = !duration || duration->m_duration < 0;
  }
}

static int Script_GetPlayerBuff(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetPlayerBuff(index [, \"filter\"])");
  }
  unsigned int filter = 7;
  if (lua_isstring(L, 2)) {
    const char *cursor = lua_tostring(L, 2);
    char token[32];
    filter = 0;
    do {
      SStrTokenize(&cursor, token, sizeof(token), " |", 0);
      if (!*token) {
        break;
      }
      if (!SStrCmpI(token, "HELPFUL", 0x7FFFFFFF)) {
        filter |= 1;
      } else if (!SStrCmpI(token, "HARMFUL", 0x7FFFFFFF)) {
        filter |= 2;
      } else if (!SStrCmpI(token, "PASSIVE", 0x7FFFFFFF)) {
        filter |= 4;
      } else if (!SStrCmpI(token, "CANCELABLE", 0x7FFFFFFF)) {
        filter |= 0x10;
      } else if (!SStrCmpI(token, "NOT_CANCELABLE", 0x7FFFFFFF)) {
        filter |= 0x20;
      }
    } while (*cursor);
  }
  int         buffIndex = -1;
  const CGBuffDesc *buff = CGBuffBar::GetBuffByFilter(static_cast<int>(lua_tonumber(L, 1)), filter, buffIndex);
  lua_pushnumber(L, static_cast<double>(buffIndex));
  lua_pushnumber(L, buff ? static_cast<double>(buff->GetUntilCancelled()) : 0.0);
  return 2;
}

static int Script_GetPlayerBuffTexture(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetPlayerBuffTexture(buffIndex)");
  }
  const CGBuffDesc *buff = CGBuffBar::GetBuffByIndex(static_cast<int>(lua_tonumber(L, 1)));
  const SpellRec     *spell = buff ? g_spellDB.GetRecord(buff->GetAuraSpell()) : 0;
  const SpellIconRec *icon = spell ? g_spellIconDB.GetRecord(spell->m_spellIconID) : 0;
  if (icon) {
    lua_pushstring(L, icon->m_textureFilename);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_GetPlayerBuffTimeLeft(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetPlayerBuffTimeLeft(buffIndex)");
  }
  lua_pushnumber(L, static_cast<double>(CGBuffBar::GetBuffTimeLeftByIndex(static_cast<int>(lua_tonumber(L, 1)))) * 0.001);
  return 1;
}

static int Script_CancelPlayerBuff(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: CancelPlayerBuff(buffIndex)");
  }
  int buffIndex = static_cast<int>(lua_tonumber(L, 1));
  if (buffIndex >= 0) {
    const CGBuffDesc *buff = CGBuffBar::GetBuffByIndex(buffIndex);
    if (buff->GetAuraIndex() >= 0) {
      Spell_C_CancelAura(buff->GetAuraSpell());
    }
  }
  return 0;
}

static FrameScript_Method s_ScriptFunctions[4] = {
    {        "GetPlayerBuff",         Script_GetPlayerBuff},
    { "GetPlayerBuffTexture",  Script_GetPlayerBuffTexture},
    {"GetPlayerBuffTimeLeft", Script_GetPlayerBuffTimeLeft},
    {     "CancelPlayerBuff",      Script_CancelPlayerBuff}
};

void BuffBarRegisterScriptFunctions() {
  for (unsigned int i = 0; i < 4; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void BuffBarUnregisterScriptFunctions() {
  for (unsigned int i = 0; i < 4; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
