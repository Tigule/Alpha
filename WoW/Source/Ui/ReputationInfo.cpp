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

void UnitCombatLogFactionChanged(int faction, int delta);

UINT CGReputationInfo::m_numFactions;
BYTE CGReputationInfo::m_factionFlags[64];
int  CGReputationInfo::m_factionBase[64];
int  CGReputationInfo::m_factionStandings[64];
int  CGReputationInfo::m_factionMap[64];
int  CGReputationInfo::m_factionSorting[64];

void CGReputationInfo::EnterWorld() {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  FATALASSERT(player);

  UINT raceID = player->GetRace();
  UINT classID = player->GetClass();
  memset(m_factionBase, 0, sizeof(m_factionBase));
  memset(m_factionMap, 0, sizeof(m_factionMap));

  int i = g_factionDB.GetNumRecords();
  while (i) {
    --i;
    const FactionRec *faction = g_factionDB.GetRecordByIndex(i);
    if (faction->m_reputationIndex >= 0 && faction->m_reputationIndex < MAX_REPUTATION_FACTIONS) {
      m_factionMap[faction->m_reputationIndex] = faction->m_ID;
      for (UINT j = 0; j < 4; ++j) {
        UINT raceMask = faction->m_reputationRaceMask[j];
        UINT classMask = faction->m_reputationClassMask[j];
        if ((raceMask || classMask) && (!raceMask || ((1 << (raceID - 1)) & raceMask)) && (!classMask || ((1 << (classID - 1)) & classMask))) {
          m_factionBase[faction->m_reputationIndex] = faction->m_reputationBase[j];
        }
      }
    }
  }
  SortFactions();
}

void CGReputationInfo::LeaveWorld() {
}

void CGReputationInfo::ShutdownGame() {
  m_numFactions = 0;
}

int CGReputationInfo::FactionToIndex(int faction) {
  const FactionRec *rec = g_factionDB.GetRecord(faction);
  FATALASSERT(rec);
  int index = rec->m_reputationIndex;
  FATALASSERT((index >= 0) && (index < MAX_REPUTATION_FACTIONS));
  return index;
}

int CGReputationInfo::IndexToFaction(int index) {
  return m_factionMap[index];
}

void CGReputationInfo::OnInitializeFactions(CDataStore *msg) {
  int  standing;
  int  numFactions;
  BYTE flags;

  memset(m_factionSorting, 0, sizeof(m_factionSorting));
  m_numFactions = 0;
  msg->Get(numFactions);
  FATALASSERT(numFactions == MAX_REPUTATION_FACTIONS);

  for (int index = 0; index < numFactions; ++index) {
    msg->Get(flags);
    SetFactionFlags(index, flags);
    msg->Get(standing);
    SetFactionStanding(index, standing);
    if (flags & 1) {
      m_factionSorting[m_numFactions++] = index;
    }
  }
}

void CGReputationInfo::OnSetFactionVisible(CDataStore *msg) {
  int factionIndex;

  msg->Get(factionIndex);
  BYTE flags = m_factionFlags[factionIndex];
  if (!(flags & 1)) {
    m_factionSorting[m_numFactions++] = factionIndex;
    SetFactionFlags(factionIndex, flags | 1);
    SortFactions();
    FrameScript_SignalEvent(355);
  }
}

void CGReputationInfo::OnSetFactionStanding(CDataStore *msg) {
  int standing;
  int factionIndex;

  msg->Get(factionIndex);
  msg->Get(standing);
  int           faction = IndexToFaction(factionIndex);
  UNIT_REACTION oldReaction = GetFactionStandingReaction(faction);
  SetFactionStanding(factionIndex, standing);

  BYTE flags = m_factionFlags[factionIndex];
  if (!(flags & 1)) {
    flags |= 1;
    m_factionSorting[m_numFactions++] = factionIndex;
  }
  if (oldReaction == UNIT_REACTION_UNFRIENDLY && GetFactionStandingReaction(faction) < oldReaction) {
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

static int __cdecl QSortFactions(LPCVOID a, LPCVOID b) {
  FATALASSERT(a);
  FATALASSERT(b);
  const FactionRec *recordA = g_factionDB.GetRecord(CGReputationInfo::IndexToFaction(*static_cast<const int *>(a)));
  const FactionRec *recordB = g_factionDB.GetRecord(CGReputationInfo::IndexToFaction(*static_cast<const int *>(b)));
  if (recordA && recordB) {
    return SStrCmpI(recordA->m_name_lang[CURRENT_LANGUAGE], recordB->m_name_lang[CURRENT_LANGUAGE], 0x7FFFFFFF);
  }
  return 0;
}

void CGReputationInfo::SortFactions() {
  qsort(m_factionSorting, m_numFactions, sizeof(m_factionSorting[0]), QSortFactions);
}

int CGReputationInfo::GetFactionFromSortIndex(UINT index) {
  if (index > m_numFactions) {
    return 0;
  }

  return IndexToFaction(m_factionSorting[index]);
}

void CGReputationInfo::SetFactionFlags(int factionIndex, BYTE flags) {
  m_factionFlags[factionIndex] = flags;
}

void CGReputationInfo::SetAtWar(int faction, bool state) {
  const FactionRec *rec = g_factionDB.GetRecord(faction);
  FATALASSERT(rec);
  int index = rec->m_reputationIndex;
  FATALASSERT((index >= 0) && (index < MAX_REPUTATION_FACTIONS));
  BYTE flags = m_factionFlags[index];
  if (state) {
    flags |= 2;
  } else {
    flags &= ~2;
  }
  m_factionFlags[index] = flags;
  CDataStore msg;
  msg.Put(CMSG_SET_FACTION_ATWAR);
  msg.Put(index);
  msg.Put(static_cast<BYTE>(state != 0));
  msg.Finalize();
  ClientServices_Send(&msg);
}

bool CGReputationInfo::IsAtWar(int faction) {
  return (m_factionFlags[FactionToIndex(faction)] & 2) != 0;
}

void CGReputationInfo::SetFactionStanding(int factionIndex, int standing) {
  FATALASSERT((factionIndex >= 0) && (factionIndex < MAX_REPUTATION_FACTIONS));
  int delta = standing - m_factionStandings[factionIndex];
  m_factionStandings[factionIndex] = standing;
  UnitCombatLogFactionChanged(IndexToFaction(factionIndex), delta);
}

int CGReputationInfo::GetFactionStanding(int faction) {
  if (!ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__)) {
    return 0;
  }

  int index = FactionToIndex(faction);
  return m_factionStandings[index] + m_factionBase[index];
}

UNIT_REACTION CGReputationInfo::GetFactionStandingReaction(int faction) {
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

static int Script_GetNumFactions(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGReputationInfo::GetNumFactions()));
  return 1;
}

static int Script_GetFactionInfo(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetFactionInfo(index)");
    return 0;
  }
  int               index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  int               faction = CGReputationInfo::GetFactionFromSortIndex(index);
  const FactionRec *rec = g_factionDB.GetRecord(faction);
  if (rec) {
    lua_pushstring(L, rec->m_name_lang[CURRENT_LANGUAGE]);
    UNIT_REACTION reaction = CGReputationInfo::GetFactionStandingReaction(faction);
    lua_pushnumber(L, static_cast<double>(reaction + 1));
    static const int s_factionThreshold[8] = {-4200, -600, -300, 0, 300, 900, 2100, 3300};
    int              min = s_factionThreshold[reaction];
    int              max = s_factionThreshold[reaction + 1];
    int              value = CGReputationInfo::GetFactionStanding(faction);
    FATALASSERT((value >= min) && (value <= max));
    lua_pushnumber(L, static_cast<double>(value - min) / (max - min));
    if (CGReputationInfo::IsAtWar(faction)) {
      lua_pushnumber(L, 1.0);
    } else {
      lua_pushnil(L);
    }
    return 4;
  }
  lua_pushnil(L);
  lua_pushnumber(L, 1.0);
  lua_pushnumber(L, 0.0);
  lua_pushnil(L);
  return 4;
}

static int Script_FactionToggleAtWar(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: FactionToggleAtWar(index)");
    return 0;
  }
  int index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  int faction = CGReputationInfo::GetFactionFromSortIndex(index);
  if (faction) {
    CGReputationInfo::SetAtWar(faction, !CGReputationInfo::IsAtWar(faction));
    if (CGReputationInfo::IsAtWar(faction)) {
      lua_pushnumber(L, 1.0);
      return 1;
    }
  }
  lua_pushnil(L);
  return 1;
}

static FrameScript_Method s_ScriptFunctions[3] = {
    {    "GetNumFactions",     Script_GetNumFactions},
    {    "GetFactionInfo",     Script_GetFactionInfo},
    {"FactionToggleAtWar", Script_FactionToggleAtWar}
};

void ReputationInfoRegisterScriptFunctions() {
  for (UINT i = 0; i < 3; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void ReputationInfoUnregisterScriptFunctions() {
  for (UINT i = 0; i < 3; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
