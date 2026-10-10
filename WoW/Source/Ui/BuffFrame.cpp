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

enum {
  BUFF_FILTER_HELPFUL = 1,
  BUFF_FILTER_HARMFUL = 2,
  BUFF_FILTER_PASSIVE = 4,
  BUFF_FILTER_CANCELABLE = 16,
  BUFF_FILTER_NOT_CANCELABLE = 32,
  BUFF_FILTER_ALL = 7
};

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
  BYTE GetAuraFlags() const {
    return m_auraFlags;
  }
  int GetUntilCancelled() const {
    return m_untilCancelled;
  }

 protected:
  int  m_auraIndex;
  int  m_auraSpell;
  BYTE m_auraFlags;
  int  m_untilCancelled;
};

class CGBuffBar {
 public:
  static void              InitializeGame();
  static void              ShutdownGame();
  static void              EnterWorld();
  static void              LeaveWorld();
  static void              UpdateBuffs();
  static void              UpdateDuration(BYTE slot, UINT duration);
  static const CGBuffDesc *GetBuffByFilter(int index, UINT filter, int &buffIndex);
  static const CGBuffDesc *GetBuffByIndex(int buffIndex);
  static UINT              GetBuffTimeLeftByIndex(int buffIndex);

 private:
  static CGBuffDesc m_buffs[56];
  static UINT       m_durations[56];
};

CGBuffDesc CGBuffBar::m_buffs[56];
UINT       CGBuffBar::m_durations[56];

static BOOL AuraUpdateHandler(DWORDLONG, UINT offset, UINT bytes, LPCVOID, LPVOID) {
  CGBuffBar::UpdateBuffs();
  return 1;
}

void CGBuffBar::InitializeGame() {
  for (UINT i = 0; i < 56; ++i) {
    m_buffs[i].SetAuraIndex(-1, 0);
    m_durations[i] = 0;
  }
}

void CGBuffBar::ShutdownGame() {
}

void CGBuffBar::EnterWorld() {
  DWORDLONG player = ClntObjMgrGetActivePlayer();
  ClntObjMgrSetObjMirrorHandler(player, CGUnit_C::OffsetOf(ID_UNIT) + offsetof(CGUnitData, auras), sizeof(((CGUnitData *)0)->auras) + sizeof(((CGUnitData *)0)->auraFlags), AuraUpdateHandler, 0, HANDLER_PRIORITY_NORMAL);
  UpdateBuffs();
}

void CGBuffBar::LeaveWorld() {
  DWORDLONG player = ClntObjMgrGetActivePlayer();
  ClntObjMgrUnsetObjMirrorHandler(player, CGUnit_C::OffsetOf(ID_UNIT) + offsetof(CGUnitData, auras), AuraUpdateHandler, 0);
}

void CGBuffBar::UpdateBuffs() {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return;
  }

  int desc;
  int aura;
  for (desc = 0; desc < 56;) {
    int id = m_buffs[desc].m_auraSpell;
    if (id <= 0) {
      break;
    }
    const SpellRec *spell = g_spellDB.GetRecord(id);
    if (spell && ((spell->m_attributes & 0x80) || (spell->m_attributesEx & 0x10000000))) {
      continue;
    }

    BOOL found = 0;
    for (aura = 0; aura < 56; ++aura) {
      if (player->GetAura(aura) == id && (player->GetAuraFlags(aura) & 0xE)) {
        found = 1;
        break;
      }
    }
    if (!found) {
      memcpy(&m_buffs[desc], &m_buffs[desc + 1], sizeof(CGBuffDesc) * (55 - desc));
      m_buffs[55].SetAuraIndex(-1, 0);
    } else {
      m_buffs[desc].SetAuraIndex(aura, player);
      ++desc;
    }
  }

  for (aura = 0; aura < 56; ++aura) {
    int spellID = player->GetAura(aura);
    if (spellID > 0 && (player->GetAuraFlags(aura) & 0xE)) {
      const SpellRec *spell = g_spellDB.GetRecord(spellID);
      if (!spell || !((spell->m_attributes & 0x80) || (spell->m_attributesEx & 0x10000000))) {
        BOOL found = 0;
        int  index;
        for (index = 0; index < 56 && m_buffs[index].m_auraSpell > 0; ++index) {
          if (m_buffs[index].m_auraSpell == spellID) {
            found = 1;
            break;
          }
        }
        if (!found && index < 56) {
          m_buffs[index].SetAuraIndex(aura, player);
        }
      }
    }
  }
  FrameScript_SignalEvent(188);
}

void CGBuffBar::UpdateDuration(BYTE slot, UINT duration) {
  if (slot < 56) {
    m_durations[slot] = duration + OsGetAsyncTimeMs();
  }
}

const CGBuffDesc *CGBuffBar::GetBuffByFilter(int index, UINT filter, int &buffIndex) {
  int count = 0;
  for (int i = 0; i < 56; ++i) {
    CGBuffDesc &buff = m_buffs[i];
    if (buff.m_auraIndex < 0) {
      continue;
    }
    if (buff.m_auraIndex < 32) {
      if (!(filter & 1)) {
        continue;
      }
    } else if (buff.m_auraIndex < 40) {
      if (!(filter & 2)) {
        continue;
      }
    } else if (!(filter & 4)) {
      continue;
    }
    int flags = buff.m_auraFlags & 1;
    if ((filter & 0x10) && !flags) {
      continue;
    }
    if ((filter & 0x20) && flags) {
      continue;
    }
    if (count == index) {
      buffIndex = i;
      return &buff;
    }
    ++count;
  }
  return 0;
}

const CGBuffDesc *CGBuffBar::GetBuffByIndex(int buffIndex) {
  ASSERT(buffIndex >= 0 && buffIndex < (sizeof(m_buffs) / sizeof(m_buffs[0])));
  return &m_buffs[buffIndex];
}

UINT CGBuffBar::GetBuffTimeLeftByIndex(int buffIndex) {
  UINT timeLeft = 0;
  int  auraIndex = GetBuffByIndex(buffIndex)->GetAuraIndex();
  if (auraIndex >= 0) {
    UINT now = OsGetAsyncTimeMs();
    timeLeft = (int)(now - m_durations[auraIndex]) >= 0 ? 0 : m_durations[auraIndex] - now;
  }
  return timeLeft;
}

CGBuffDesc::CGBuffDesc() {
  m_auraSpell = 0;
  m_auraIndex = -1;
  m_untilCancelled = 0;
}

void CGBuffDesc::SetAuraIndex(int index, CGPlayer_C *player) {
  m_auraIndex = index;
  if (index == -1) {
    m_auraSpell = 0;
    return;
  }

  m_auraSpell = player->GetAura(index);
  m_auraFlags = player->GetAuraFlags(index);
  if (m_auraSpell > 0) {
    const SpellRec *spell = g_spellDB.GetRecord(m_auraSpell);
    if (spell) {
      const SpellDurationRec *duration = g_spellDurationDB.GetRecord(spell->m_durationIndex);
      m_untilCancelled = !duration || duration->m_duration < 0;
    }
  }
}

static int Script_GetPlayerBuff(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetPlayerBuff(index [, \"filter\"])");
    return 0;
  }
  int  index = lua_tonumber(L, 1);
  UINT filter = 7;
  if (lua_isstring(L, 2)) {
    LPCSTR cursor = lua_tostring(L, 2);
    filter = 0;
    char token[32];
    do {
      SStrTokenize(&cursor, token, sizeof(token), " |", 0);
      if (*token) {
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
      }
    } while (*token && *cursor);
  }
  int               buffIndex = -1;
  const CGBuffDesc *buff = CGBuffBar::GetBuffByFilter(index, filter, buffIndex);
  lua_pushnumber(L, buffIndex);
  if (buff) {
    lua_pushnumber(L, buff->GetUntilCancelled());
  } else {
    lua_pushnumber(L, 0.0);
  }
  return 2;
}

static int Script_GetPlayerBuffTexture(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetPlayerBuffTexture(buffIndex)");
    return 0;
  }
  const CGBuffDesc *buff = CGBuffBar::GetBuffByIndex(lua_tonumber(L, 1));
  const SpellRec   *spell = g_spellDB.GetRecord(buff->GetAuraSpell());
  if (!spell) {
    lua_pushnil(L);
    return 1;
  }
  const SpellIconRec *icon = g_spellIconDB.GetRecord(spell->m_spellIconID);
  if (!icon) {
    lua_pushnil(L);
    return 1;
  }
  lua_pushstring(L, icon->m_textureFilename);
  return 1;
}

static int Script_GetPlayerBuffTimeLeft(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetPlayerBuffTimeLeft(buffIndex)");
    return 0;
  }
  lua_pushnumber(L, (double)CGBuffBar::GetBuffTimeLeftByIndex(lua_tonumber(L, 1)) * 0.001);
  return 1;
}

static int Script_CancelPlayerBuff(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: CancelPlayerBuff(buffIndex)");
    return 0;
  }
  int buffIndex = lua_tonumber(L, 1);
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
  for (UINT i = 0; i < 4; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void BuffBarUnregisterScriptFunctions() {
  for (UINT i = 0; i < 4; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
