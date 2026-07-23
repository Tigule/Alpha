#include "ReputationInfo.h"

#include "DB/DBClient/AutoCode/FactionRec.h"
#include "Object/ObjectClient/Player_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "Ui/Tutorial.h"

#include <Base/CDataStore.h>
#include <FrameScript/FrameScript.h>
#include <Net/NetClient/NetClient.h>
#include "WowSvcs/WowSvcsClient/ClientServices.h"

#include <lauxlib.h>
#include <lua.h>

#include <string.h>
#include <stdlib.h>

void __fastcall UnitCombatLogFactionChanged(int faction, int delta);

unsigned int CGReputationInfo::m_numFactions;
unsigned int CGReputationInfo::m_factionFlags[64];
int          CGReputationInfo::m_factionBase[64];
int          CGReputationInfo::m_factionStandings[64];
int          CGReputationInfo::m_factionMap[64];
int          CGReputationInfo::m_factionSorting[64];

void __fastcall CGReputationInfo::EnterWorld() {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  FATALASSERT(player);

  const unsigned char *playerData = reinterpret_cast<const unsigned char *>(player->GetStorage());
  unsigned int         raceID = playerData[112];
  unsigned int         classID = playerData[113];
  memset(m_factionBase, 0, sizeof(m_factionBase));
  memset(m_factionMap, 0, sizeof(m_factionMap));

  for (int i = g_factionDB.GetNumRecords() - 1; i >= 0; --i) {
    FactionRec *faction = g_factionDB.GetRecordByIndex(i);
    if (static_cast<unsigned int>(faction->m_reputationIndex) >= 64) {
      continue;
    }

    m_factionMap[faction->m_reputationIndex] = faction->m_ID;
    for (int group = 0; group < 4; ++group) {
      unsigned int raceMask = faction->m_reputationRaceMask[group];
      unsigned int classMask = faction->m_reputationClassMask[group];
      if ((!raceMask || raceMask & (1 << (raceID - 1))) && (!classMask || classMask & (1 << (classID - 1))) && (raceMask || classMask)) {
        m_factionBase[faction->m_reputationIndex] = faction->m_reputationBase[group];
      }
    }
  }
  SortFactions();
}

void __fastcall CGReputationInfo::LeaveWorld() {
}

void __fastcall CGReputationInfo::ShutdownGame() {
  m_numFactions = 0;
}

int __fastcall CGReputationInfo::FactionToIndex(int faction) {
  FactionRec *rec = g_factionDB.GetRecord(faction);
  FATALASSERT(rec);
  FATALASSERT(rec->m_reputationIndex >= 0 && rec->m_reputationIndex < 64);
  return rec->m_reputationIndex;
}

unsigned int __fastcall CGReputationInfo::GetNumFactions() {
  return m_numFactions;
}

int __fastcall CGReputationInfo::IndexToFaction(int index) {
  return m_factionMap[index];
}

void __fastcall CGReputationInfo::OnInitializeFactions(CDataStore *msg) {
  int          standing;
  int          numFactions;
  unsigned int flags;

  memset(m_factionSorting, 0, sizeof(m_factionSorting));
  m_numFactions = 0;
  msg->Get(numFactions);
  FATALASSERT(numFactions == 64);

  for (int index = 0; index < numFactions; ++index) {
    msg->Get(reinterpret_cast<unsigned char &>(flags));
    SetFactionFlags(index, flags);
    msg->Get(standing);
    SetFactionStanding(index, standing);
    if (flags & 1) {
      m_factionSorting[m_numFactions++] = index;
    }
  }
}

void __fastcall CGReputationInfo::OnSetFactionVisible(CDataStore *msg) {
  int factionIndex;

  msg->Get(factionIndex);
  unsigned int flags = m_factionFlags[factionIndex];
  if (!(flags & 1)) {
    m_factionSorting[m_numFactions++] = factionIndex;
    SetFactionFlags(factionIndex, flags | 1);
    SortFactions();
    FrameScript_SignalEvent(355);
  }
}

void __fastcall CGReputationInfo::OnSetFactionStanding(CDataStore *msg) {
  int standing;
  int factionIndex;

  msg->Get(factionIndex);
  msg->Get(standing);
  int           faction = IndexToFaction(factionIndex);
  UNIT_REACTION oldReaction = GetFactionStandingReaction(faction);
  SetFactionStanding(factionIndex, standing);

  unsigned int flags = m_factionFlags[factionIndex];
  if (!(flags & 1)) {
    m_factionSorting[m_numFactions++] = factionIndex;
    flags |= 1;
  }
  if (oldReaction == UNIT_REACTION_UNFRIENDLY && GetFactionStandingReaction(faction) < UNIT_REACTION_UNFRIENDLY) {
    flags |= 2;
  }

  if (flags != m_factionFlags[factionIndex]) {
    SetFactionFlags(factionIndex, flags);
    SortFactions();
  }
  if (GetFactionStandingReaction(faction) != oldReaction) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (player) {
      player->UpdateQuestStatusAll();
    }
  }

  CGTutorial::TriggerTutorial(TUTORIAL_REPUTATION);
  FrameScript_SignalEvent(355);
}

static int __cdecl QSortFactions(const void *a, const void *b) {
  FATALASSERT(a);
  FATALASSERT(b);
  int         factionA = CGReputationInfo::IndexToFaction(*static_cast<const int *>(a));
  int         factionB = CGReputationInfo::IndexToFaction(*static_cast<const int *>(b));
  FactionRec *recordA = g_factionDB.GetRecord(factionA);
  FactionRec *recordB = g_factionDB.GetRecord(factionB);
  return recordA && recordB ? SStrCmpI(recordA->m_name_lang[CURRENT_LANGUAGE], recordB->m_name_lang[CURRENT_LANGUAGE], 0x7FFFFFFF) : 0;
}

void __fastcall CGReputationInfo::SortFactions() {
  qsort(m_factionSorting, m_numFactions, sizeof(m_factionSorting[0]), QSortFactions);
}

int __fastcall CGReputationInfo::GetFactionFromSortIndex(unsigned int index) {
  return index < m_numFactions ? IndexToFaction(m_factionSorting[index]) : 0;
}

void __fastcall CGReputationInfo::SetFactionFlags(int index, unsigned char flags) {
  FATALASSERT(index >= 0 && index < 64);
  m_factionFlags[index] = flags;
}

void __fastcall CGReputationInfo::SetAtWar(int faction, unsigned int state) {
  int          index = FactionToIndex(faction);
  unsigned int flags = m_factionFlags[index];
  if (state) {
    flags |= 2;
  } else {
    flags &= ~2;
  }
  if (flags == m_factionFlags[index]) {
    return;
  }
  SetFactionFlags(index, static_cast<unsigned char>(flags));
  CDataStore msg;
  msg.Put(static_cast<unsigned int>(CMSG_SET_FACTION_ATWAR));
  msg.Put(index);
  msg.Put(static_cast<unsigned char>(state != 0));
  msg.Finalize();
  ClientServices_Send(&msg);
}

bool __fastcall CGReputationInfo::IsAtWar(int faction) {
  return (m_factionFlags[FactionToIndex(faction)] & 2) != 0;
}

void __fastcall CGReputationInfo::SetFactionStanding(int factionIndex, int standing) {
  FATALASSERT(factionIndex >= 0 && factionIndex < 64);
  m_factionStandings[factionIndex] = standing;
  int faction = IndexToFaction(factionIndex);
  UnitCombatLogFactionChanged(faction, GetFactionStanding(faction));
}

int __fastcall CGReputationInfo::GetFactionStanding(int faction) {
  if (!ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__)) {
    return 0;
  }

  int index = FactionToIndex(faction);
  return m_factionStandings[index] + m_factionBase[index];
}

UNIT_REACTION __fastcall CGReputationInfo::GetFactionStandingReaction(int faction) {
  int standing = GetFactionStanding(faction);
  if (standing >= 2100) {
    return UNIT_REACTION_REVERED;
  }
  if (standing >= 900) {
    return UNIT_REACTION_FRIENDLY;
  }
  if (standing >= 300) {
    return UNIT_REACTION_AMIABLE;
  }
  if (standing >= 0) {
    return UNIT_REACTION_NEUTRAL;
  }
  if (standing >= -300) {
    return UNIT_REACTION_UNFRIENDLY;
  }
  return standing >= -600 ? UNIT_REACTION_HOSTILE : UNIT_REACTION_HATED;
}

static int __fastcall Script_GetNumFactions(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGReputationInfo::GetNumFactions()));
  return 1;
}

static int __fastcall Script_GetFactionInfo(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetFactionInfo(index)");
  }
  int               faction = CGReputationInfo::GetFactionFromSortIndex(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  const FactionRec *rec = g_factionDB.GetRecord(faction);
  if (rec) {
    UNIT_REACTION    reaction = CGReputationInfo::GetFactionStandingReaction(faction);
    static const int threshold[8] = {-4200, -600, -300, 0, 300, 900, 2100, 3300};
    int              standing = CGReputationInfo::GetFactionStanding(faction);
    FATALASSERT(standing >= threshold[reaction] && standing <= threshold[reaction + 1]);
    lua_pushstring(L, rec->m_name_lang[CURRENT_LANGUAGE]);
    lua_pushnumber(L, static_cast<double>(reaction + 1));
    lua_pushnumber(L, static_cast<double>(standing - threshold[reaction]) / (threshold[reaction + 1] - threshold[reaction]));
    CGReputationInfo::IsAtWar(faction) ? lua_pushnumber(L, 1.0) : lua_pushnil(L);
    return 4;
  }
  lua_pushnil(L);
  lua_pushnumber(L, 1.0);
  lua_pushnumber(L, 0.0);
  lua_pushnil(L);
  return 4;
}

static int __fastcall Script_FactionToggleAtWar(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: FactionToggleAtWar(index)");
  }
  int faction = CGReputationInfo::GetFactionFromSortIndex(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  if (faction) {
    CGReputationInfo::SetAtWar(faction, !CGReputationInfo::IsAtWar(faction));
    CGReputationInfo::IsAtWar(faction) ? lua_pushnumber(L, 1.0) : lua_pushnil(L);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static FrameScript_Method s_ScriptFunctions[3] = {
    {    "GetNumFactions",     Script_GetNumFactions},
    {    "GetFactionInfo",     Script_GetFactionInfo},
    {"FactionToggleAtWar", Script_FactionToggleAtWar}
};

void __fastcall ReputationInfoRegisterScriptFunctions() {
  for (unsigned int i = 0; i < 3; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void __fastcall ReputationInfoUnregisterScriptFunctions() {
  for (unsigned int i = 0; i < 3; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
