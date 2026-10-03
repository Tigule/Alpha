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

#include "QuestLog.h"

#include "Object/ObjectClient/Unit_C.h"
#include "Object/ObjectClient/Player_C.h"
#include "Object/ObjectClient/Item_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "DB/DBClient/AutoCode/AreaTableRec.h"
#include "DB/DBClient/AutoCode/QuestInfoRec.h"
#include "DB/DBClient/AutoCode/QuestSortRec.h"
#include "DB/DBClient/DBClient.h"
#include "DB/DBClient/DBCacheInstances.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

#include <Base/CDataStore.h>
#include <Console/ConsoleClient.h>
#include <FrameScript/FrameScript.h>
#include <lauxlib.h>
#include <lua.h>
#include <Os/OsTime.h>
#include <stdlib.h>
#include <string.h>

void MinimapSetQuestPOI(float x, float y, int priority, LPCSTR name);
bool QuestParserParseText(LPCSTR text, char *buf, UINT size, const DWORDLONG &target, int restoreToken);

UINT         CGQuestLog::m_numQuests;
UINT         CGQuestLog::m_numSortTypes;
int          CGQuestLog::m_selectedQuest;
int          CGQuestLog::m_abandonQuest;
QuestLogInfo CGQuestLog::m_quests[32];
int          CGQuestLog::m_sortTypes[16];
int          CGQuestLog::m_collapseFilter;
int          CGQuestLog::m_numShownQuests;
int          CGQuestLog::m_expiredQuests;
int          CGQuestLog::m_serverTimeOffset;

static int s_nextTimeUpdate;
static int s_questCallbackCount;

static void QuestQueryCounterCallback(int id, const DWORDLONG &, LPVOID, bool granted) {
  if (!granted) {
    ConsoleWrite("Invalid quest log entry", DEFAULT_COLOR);
    return;
  }
  if (s_questCallbackCount > 0) {
    --s_questCallbackCount;
  }
  if (!s_questCallbackCount) {
    CGQuestLog::Update(1);
  }
}

static void QuestQueryCallback(int id, const DWORDLONG &, LPVOID, bool granted) {
  if (!granted) {
    ConsoleWrite("Invalid quest log entry", DEFAULT_COLOR);
    return;
  }
  FrameScript_SignalEvent(285);
}

static void QuestSelectQueryCallback(int id, const DWORDLONG &, LPVOID, bool granted) {
  if (!granted) {
    ConsoleWrite("Invalid quest log entry", DEFAULT_COLOR);
    return;
  }
  CGQuestLog::UpdateSelection();
}

static void QuestFailedCallback(int id, const DWORDLONG &, LPVOID, bool granted) {
  if (!granted) {
    ConsoleWrite("Invalid quest log entry", DEFAULT_COLOR);
    return;
  }
  const QuestCache *quest = g_questDBCache.GetRecord(id, 0, 0, 0);
  if (quest) {
    CGGameUI::DisplayError(GERR_QUEST_FAILED_S, quest->m_logTitle);
  }
}

static void CreatureQueryCallback(int id, const DWORDLONG &, LPVOID, bool granted) {
  if (!granted) {
    ConsoleWrite("Invalid creature in quest", DEFAULT_COLOR);
    return;
  }
  FrameScript_SignalEvent(285);
}

static void ObjectQueryCallback(int id, const DWORDLONG &, LPVOID, bool granted) {
  if (!granted) {
    ConsoleWrite("Invalid object in quest", DEFAULT_COLOR);
    return;
  }
  FrameScript_SignalEvent(285);
}

static void ItemQueryCallback(int id, const DWORDLONG &guid, LPVOID arg, bool granted) {
  if (granted) {
    FrameScript_SignalEvent(285);
  }
}

static BOOL QuestLogUpdateHandler(DWORDLONG, UINT offset, UINT bytes, LPCVOID, LPVOID) {
  CGQuestLog::Update(1);
  return 1;
}

static BOOL OnQueryTimeResponse(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg) {
  int currentServerTime;
  msg->Get(currentServerTime);
  CGQuestLog::UpdateServerTime(currentServerTime);
  return 1;
}


void CGQuestLog::InitializeGame() {
  m_selectedQuest = 0;
  m_abandonQuest = 0;
  m_serverTimeOffset = 0;
  m_expiredQuests = 0;
  s_nextTimeUpdate = 0;
}

void CGQuestLog::ShutdownGame() {
}

void CGQuestLog::EnterWorld() {
  DWORDLONG player = ClntObjMgrGetActivePlayer();
  ClntObjMgrSetObjMirrorHandler(
      player, CGPlayer_C::OffsetOf(ID_PLAYER) + offsetof(CGPlayerData, questLog), sizeof(((CGPlayerData *)0)->questLog), QuestLogUpdateHandler, 0,
      HANDLER_PRIORITY_NORMAL
  );
  ClientServices_SetMessageHandler(SMSG_QUERY_TIME_RESPONSE, OnQueryTimeResponse, 0);

  CDataStore msg;
  msg.Put(CMSG_QUERY_TIME);
  msg.Finalize();
  ClientServices_Send(&msg);
  s_questCallbackCount = 0;
  Update(1);
}

void CGQuestLog::LeaveWorld() {
  DWORDLONG player = ClntObjMgrGetActivePlayer();
  ClntObjMgrUnsetObjMirrorHandler(player, CGPlayer_C::OffsetOf(ID_PLAYER) + offsetof(CGPlayerData, questLog), QuestLogUpdateHandler, 0);
  ClientServices_ClearMessageHandler(SMSG_QUERY_TIME_RESPONSE);
}

static int __cdecl QSortQuestSortTypes(LPCVOID a, LPCVOID b) {
  ASSERT(a);
  ASSERT(b);
  int sort1 = *static_cast<const int *>(a);
  int sort2 = *static_cast<const int *>(b);
  if (sort1 == sort2) {
    return 0;
  }
  if (!sort1) {
    return -1;
  }
  if (!sort2) {
    return 1;
  }
  char name1[256] = "";
  char name2[256] = "";
  if (sort1 < 0) {
    const QuestSortRec *sort = g_questSortDB.GetRecord(-sort1);
    if (sort) {
      SStrCopy(name1, sort->m_SortName_lang[CURRENT_LANGUAGE], sizeof(name1));
    }
  } else {
    const AreaTableRec *area = g_areaTableDB.GetRecord(sort1);
    if (area) {
      SStrCopy(name1, area->m_AreaName_lang[CURRENT_LANGUAGE], sizeof(name1));
    }
  }
  if (sort2 < 0) {
    const QuestSortRec *sort = g_questSortDB.GetRecord(-sort2);
    if (sort) {
      SStrCopy(name2, sort->m_SortName_lang[CURRENT_LANGUAGE], sizeof(name2));
    }
  } else {
    const AreaTableRec *area = g_areaTableDB.GetRecord(sort2);
    if (area) {
      SStrCopy(name2, area->m_AreaName_lang[CURRENT_LANGUAGE], sizeof(name2));
    }
  }
  return SStrCmpI(name1, name2, 0x7FFFFFFF);
}

static int __cdecl QSortQuests(LPCVOID a, LPCVOID b) {
  ASSERT(a);
  ASSERT(b);
  QuestLogInfo      info1 = *static_cast<const QuestLogInfo *>(a);
  QuestLogInfo      info2 = *static_cast<const QuestLogInfo *>(b);
  UINT              sortRank1 = 0;
  UINT              sortRank2 = 0;
  const QuestCache *quest1 = info1.isHeader ? 0 : g_questDBCache.GetRecord(info1.questID, 0, 0, 0);
  const QuestCache *quest2 = info2.isHeader ? 0 : g_questDBCache.GetRecord(info2.questID, 0, 0, 0);
  UINT              i;
  if (info1.isHeader) {
    for (i = 0; i < 16; ++i) {
      if (CGQuestLog::GetQuestSortID(i) == info1.questID) {
        sortRank1 = i;
        break;
      }
    }
  } else {
    if (!quest1) {
      return 0;
    }
    for (i = 0; i < 16; ++i) {
      if (CGQuestLog::GetQuestSortID(i) == quest1->m_questSortID) {
        sortRank1 = i;
        break;
      }
    }
  }
  if (info2.isHeader) {
    for (i = 0; i < 16; ++i) {
      if (CGQuestLog::GetQuestSortID(i) == info2.questID) {
        sortRank2 = i;
        break;
      }
    }
  } else {
    if (!quest2) {
      return 0;
    }
    for (i = 0; i < 16; ++i) {
      if (CGQuestLog::GetQuestSortID(i) == quest2->m_questSortID) {
        sortRank2 = i;
        break;
      }
    }
  }
  BOOL hidden1 = !info1.isHeader && CGQuestLog::IsSortHeaderCollapsed(sortRank1);
  BOOL hidden2 = !info2.isHeader && CGQuestLog::IsSortHeaderCollapsed(sortRank2);
  if (hidden1 || hidden2) {
    if (hidden1 && hidden2) {
      return 0;
    }
    return hidden1 ? 1 : -1;
  }
  if (sortRank1 != sortRank2) {
    return sortRank1 < sortRank2 ? -1 : 1;
  }
  if (info1.isHeader) {
    return -1;
  }
  if (info2.isHeader) {
    return 1;
  }
  if (quest1->m_questLevel != quest2->m_questLevel) {
    return quest1->m_questLevel < quest2->m_questLevel ? -1 : 1;
  }
  return SStrCmpI(quest1->m_logTitle, quest2->m_logTitle, 0x7FFFFFFF);
}

void CGQuestLog::Update(int resetFilters) {
  UINT i;
  if (!resetFilters) {
    FrameScript_SignalEvent(285);
    return;
  }
  if (s_questCallbackCount) {
    return;
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return;
  }
  for (i = 0; i < 32; ++i) {
    m_quests[i].questID = 0;
    m_quests[i].logIndex = 0;
    m_quests[i].isHeader = 0;
  }
  memset(m_sortTypes, 0, sizeof(m_sortTypes));
  int offset = m_serverTimeOffset;
  m_numQuests = 0;
  m_numSortTypes = 0;
  m_expiredQuests = 0;
  s_questCallbackCount = 0;
  m_collapseFilter = -1;
  for (i = 0; i < 16; ++i) {
    const CQuestLogData *entry = player->GetQuestLogData(i);
    int                  questID = entry->m_questID;
    if (questID > 0) {
      const QuestCache *quest = g_questDBCache.GetRecord(questID, 0, reinterpret_cast<DBCACHECALLBACKPROC>(QuestQueryCounterCallback), 0);
      if (!quest) {
        ++s_questCallbackCount;
        continue;
      }
      m_quests[m_numQuests].questID = questID;
      m_quests[m_numQuests].logIndex = i;
      if (entry->m_questFailureTime && !(entry->m_questFlags & 0x80000000) && static_cast<int>(entry->m_questFailureTime + offset - OsGetTime() - 1) < 0) {
        m_expiredQuests |= 1 << i;
      }
      ++m_numQuests;
      UINT j;
      for (j = 0; j < m_numSortTypes; ++j) {
        if (m_sortTypes[j] == quest->m_questSortID) {
          break;
        }
      }
      FATALASSERT(j < (sizeof(m_sortTypes) / sizeof(m_sortTypes[0])));
      if (j == m_numSortTypes) {
        m_sortTypes[j] = quest->m_questSortID;
        ++m_numSortTypes;
        m_quests[m_numQuests].questID = quest->m_questSortID;
        m_quests[m_numQuests].isHeader = 1;
        ++m_numQuests;
      }
    }
  }
  FilterAndSortQuests();
  FrameScript_SignalEvent(285);
  if (s_nextTimeUpdate && static_cast<int>(OsGetTime()) > s_nextTimeUpdate) {
    CDataStore msg;
    msg.Put(CMSG_QUERY_TIME);
    msg.Finalize();
    ClientServices_Send(&msg);
    s_nextTimeUpdate = 0;
  }
}

void CGQuestLog::FilterAndSortQuests() {
  qsort(m_sortTypes, m_numSortTypes, sizeof(int), QSortQuestSortTypes);
  qsort(m_quests, m_numQuests, sizeof(QuestLogInfo), QSortQuests);
  m_numShownQuests = m_numQuests;
  UINT i;
  for (i = 0; i < m_numQuests; ++i) {
    if (!m_quests[i].isHeader) {
      if (IsSortHeaderCollapsed(GetQuestSortIndex(i))) {
        --m_numShownQuests;
      }
    }
  }
}

void CGQuestLog::CollapseHeader(UINT index, int collapse) {
  if (index < m_numQuests && m_quests[index].isHeader) {
    UINT i;
    for (i = 0; i < m_numSortTypes; ++i) {
      if (m_sortTypes[i] == m_quests[index].questID) {
        break;
      }
    }
    if (i == m_numSortTypes) {
      return;
    }
    if (collapse) {
      m_collapseFilter &= ~(1 << i);
    } else {
      m_collapseFilter |= 1 << i;
    }
  } else {
    m_collapseFilter = collapse ? 0 : -1;
  }
  FilterAndSortQuests();
  FrameScript_SignalEvent(285);
}

int CGQuestLog::GetQuestSortIndex(UINT index) {
  if (index >= m_numQuests) {
    return -1;
  }
  int sortID;
  if (m_quests[index].isHeader) {
    sortID = m_quests[index].questID;
  } else {
    const QuestCache *quest = g_questDBCache.GetRecord(m_quests[index].questID, 0, 0, 0);
    if (!quest) {
      return -1;
    }
    sortID = quest->m_questSortID;
  }
  UINT i;
  for (i = 0; i < m_numSortTypes; ++i) {
    if (m_sortTypes[i] == sortID) {
      return i;
    }
  }
  return -1;
}

void CGQuestLog::UpdateServerTime(int serverTime) {
  m_serverTimeOffset = OsGetTime() - serverTime;
  if (!m_serverTimeOffset) {
    m_serverTimeOffset = 1;
  }
  s_nextTimeUpdate = OsGetTime() + 3600;
  FrameScript_SignalEvent(285);
}

void CGQuestLog::SetSelectedQuest(int index) {
  ClearQuest(m_selectedQuest);
  if (index >= 0 && index < static_cast<int>(m_numQuests) && !m_quests[index].isHeader) {
    m_selectedQuest = m_quests[index].questID;
    UpdateSelection();
  }
}

void CGQuestLog::UpdateSelection() {
  const QuestCache *quest = g_questDBCache.GetRecord(m_selectedQuest, 0, reinterpret_cast<DBCACHECALLBACKPROC>(QuestSelectQueryCallback), 0);
  if (quest) {
    MinimapSetQuestPOI(quest->m_POIx, quest->m_POIy, quest->m_POIPriority, quest->m_logTitle);
  }
}

int CGQuestLog::GetSelectionIndex() {
  UINT index;
  for (index = 0; index < m_numQuests; ++index) {
    if (!m_quests[index].isHeader && m_quests[index].questID == m_selectedQuest) {
      break;
    }
  }
  if (index == m_numQuests) {
    return -1;
  }
  return index;
}

int CGQuestLog::GetSelectedLogEntry() {
  int index = GetSelectionIndex();
  if (index >= 0) {
    return m_quests[index].logIndex;
  }
  return 0;
}

LPCSTR CGQuestLog::GetAbandonQuestName() {
  const QuestCache *quest = g_questDBCache.GetRecord(m_abandonQuest, 0, 0, 0);
  if (quest) {
    return quest->m_logTitle;
  }
  return 0;
}

void CGQuestLog::AbandonSelectedQuest() {
  if (!m_abandonQuest) {
    return;
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return;
  }
  UINT index;
  for (index = 0; index < 32; ++index) {
    if (m_quests[index].questID == m_abandonQuest) {
      player->QuestLogRemoveQuest(m_quests[index].logIndex);
      m_abandonQuest = 0;
      return;
    }
  }
}

void CGQuestLog::ClearQuest(int id) {
  if (id && id == m_selectedQuest) {
    MinimapSetQuestPOI(0.0f, 0.0f, 0, "");
  }
}

void CGQuestLog::AbandonQuest(int index) {
  if (index >= 0 && index < static_cast<int>(m_numQuests) && !m_quests[index].isHeader) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (player) {
      player->QuestLogRemoveQuest(m_quests[index].logIndex);
    }
  }
}

LPCSTR CGQuestLog::GetQuestName(int index) {
  if (index >= 0 && index < static_cast<int>(m_numQuests)) {
    if (m_quests[index].isHeader) {
      int sortID = m_quests[index].questID;
      if (sortID > 0) {
        const AreaTableRec *area = g_areaTableDB.GetRecord(sortID);
        if (area) {
          return area->m_AreaName_lang[CURRENT_LANGUAGE];
        }
      } else if (sortID < 0) {
        const QuestSortRec *sort = g_questSortDB.GetRecord(-sortID);
        if (sort) {
          return sort->m_SortName_lang[CURRENT_LANGUAGE];
        }
      } else {
        return "Missing header! (quest designers)";
      }
    } else {
      const QuestCache *quest = g_questDBCache.GetRecord(m_quests[index].questID, 0, reinterpret_cast<DBCACHECALLBACKPROC>(QuestQueryCallback), 0);
      if (quest) {
        return quest->m_logTitle;
      }
    }
  }
  return 0;
}

LPCSTR CGQuestLog::GetQuestTag(int index) {
  if (index >= 0 && index < static_cast<int>(m_numQuests) && !m_quests[index].isHeader) {
    const QuestCache *quest = g_questDBCache.GetRecord(m_quests[index].questID, 0, reinterpret_cast<DBCACHECALLBACKPROC>(QuestQueryCallback), 0);
    if (quest) {
      const QuestInfoRec *info = g_questInfoDB.GetRecord(quest->m_questInfoID);
      if (info) {
        return info->m_InfoName_lang[CURRENT_LANGUAGE];
      }
    }
  }
  return 0;
}

int CGQuestLog::GetQuestLevel(int index) {
  if (index >= 0 && index < static_cast<int>(m_numQuests) && !m_quests[index].isHeader) {
    const QuestCache *quest = g_questDBCache.GetRecord(m_quests[index].questID, 0, reinterpret_cast<DBCACHECALLBACKPROC>(QuestQueryCallback), 0);
    if (quest) {
      return quest->m_questLevel;
    }
  }
  return 0;
}

int CGQuestLog::GetQuestItemID(LPCSTR type, int index) {
  if (!type || !*type || index < 0) {
    return 0;
  }
  const QuestCache *quest = g_questDBCache.GetRecord(m_selectedQuest, 0, 0, 0);
  if (!quest) {
    return 0;
  }
  if (!SStrCmpI(type, "reward", 0x7FFFFFFF)) {
    return static_cast<UINT>(index) < 4 ? quest->m_rewardItems[index] : 0;
  }
  if (!SStrCmpI(type, "choice", 0x7FFFFFFF)) {
    return static_cast<UINT>(index) < 6 ? quest->m_rewardChoiceItems[index] : 0;
  }
  return 0;
}

BOOL CGQuestLog::IsSelectedQuestExpired() {
  int index = GetSelectionIndex();
  if (index >= 0) {
    return (m_expiredQuests & (1 << index)) != 0;
  }
  return 0;
}

BOOL CGQuestLog::IsQuestExpired(UINT index) {
  if (index < m_numQuests) {
    return (m_expiredQuests & (1 << index)) != 0;
  }
  return 0;
}

void CGQuestLog::SetQuestExpired(UINT index) {
  if (index < m_numQuests) {
    m_expiredQuests |= 1 << index;
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (player) {
      player->UpdateQuestStatusAll();
    }
  }
}

static int Script_GetNumQuestLogEntries(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGQuestLog::GetNumShownEntries()));
  return 1;
}

static int Script_GetQuestLogTitle(lua_State *L) {
  if (lua_isnumber(L, 1)) {
    int index = static_cast<int>(lua_tonumber(L, 1)) - 1;
    lua_pushstring(L, CGQuestLog::GetQuestName(index));
    lua_pushnumber(L, static_cast<double>(CGQuestLog::GetQuestLevel(index)));
    lua_pushstring(L, CGQuestLog::GetQuestTag(index));
    if (CGQuestLog::IsQuestHeader(index)) {
      lua_pushnumber(L, 1.0);
      if (CGQuestLog::IsSortHeaderCollapsed(CGQuestLog::GetQuestSortIndex(index))) {
        lua_pushnumber(L, 1.0);
      } else {
        lua_pushnil(L);
      }
    } else {
      lua_pushnil(L);
      lua_pushnil(L);
    }
    return 5;
  }
  luaL_error(L, "Usage: GetQuestLogTitle(index)");
  return 0;
}

static int Script_SelectQuestLogEntry(lua_State *L) {
  if (lua_isnumber(L, 1)) {
    CGQuestLog::SetSelectedQuest(static_cast<int>(lua_tonumber(L, 1)) - 1);
    return 0;
  }
  luaL_error(L, "Usage: SelectQuestLogEntry(index)");
  return 0;
}

static int Script_GetQuestLogSelection(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGQuestLog::GetSelectionIndex() + 1));
  return 1;
}

static int Script_SwapQuestLogEntries(lua_State *L) {
  if (lua_isnumber(L, 1) && lua_isnumber(L, 2)) {
    int index1 = CGQuestLog::GetQuestLogEntry(static_cast<int>(lua_tonumber(L, 1)) - 1);
    int index2 = CGQuestLog::GetQuestLogEntry(static_cast<int>(lua_tonumber(L, 2)) - 1);
    if (index1 >= 0 && index2 >= 0 && index1 != index2) {
      CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
      if (player) {
        player->QuestLogSwapQuest(index1, index2);
        return 0;
      }
    }
  } else {
    luaL_error(L, "Usage: SwapQuestLogEntries(index1, index2)");
  }
  return 0;
}

static int Script_SetAbandonQuest(lua_State *) {
  CGQuestLog::SetAbandonQuest();
  return 0;
}

static int Script_GetAbandonQuestName(lua_State *L) {
  lua_pushstring(L, CGQuestLog::GetAbandonQuestName());
  return 1;
}

static int Script_AbandonQuest(lua_State *) {
  CGQuestLog::AbandonSelectedQuest();
  return 0;
}

static int Script_GetQuestLogQuestText(lua_State *L) {
  int               questID = CGQuestLog::GetSelectedQuestID();
  const QuestCache *quest = g_questDBCache.GetRecord(questID, 0, reinterpret_cast<DBCACHECALLBACKPROC>(QuestQueryCallback), 0);
  if (quest) {
    char questText[1024];
    char logDesc[1024];
    QuestParserParseText(quest->m_questDescription, questText, sizeof(questText), ClntObjMgrGetActivePlayer(), 0);
    QuestParserParseText(quest->m_logDescription, logDesc, sizeof(logDesc), ClntObjMgrGetActivePlayer(), 0);
    lua_pushstring(L, questText);
    lua_pushstring(L, logDesc);
    return 2;
  }
  lua_pushnil(L);
  lua_pushnil(L);
  return 2;
}

static int Script_GetNumQuestLeaderBoards(lua_State *L) {
  int         count = 0;
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    int               questID = CGQuestLog::GetSelectedQuestID();
    const QuestCache *quest = g_questDBCache.GetRecord(questID, 0, reinterpret_cast<DBCACHECALLBACKPROC>(QuestQueryCallback), 0);
    if (quest) {
      if (quest->m_areaDescription && *quest->m_areaDescription) {
        ++count;
      }
      UINT i;
      for (i = 0; i < 4; ++i) {
        if (quest->m_monsterToKill[i]) {
          ++count;
        }
        if (quest->m_itemToGet[i] > 0 && quest->m_itemToGet[i] != quest->m_startItem) {
          ++count;
        }
      }
    }
  }
  lua_pushnumber(L, static_cast<double>(count));
  return 1;
}

static int Script_GetQuestLogLeaderBoard(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetQuestLogLeaderBoard(index)");
    return 0;
  }
  int         index = static_cast<int>(lua_tonumber(L, 1));
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    int                  questID = CGQuestLog::GetSelectedQuestID();
    int                  entry = CGQuestLog::GetSelectedLogEntry();
    const QuestCache    *quest = g_questDBCache.GetRecord(questID, 0, reinterpret_cast<DBCACHECALLBACKPROC>(QuestQueryCallback), 0);
    const CQuestLogData *logData = player->GetQuestLogData(entry);
    if (quest && logData) {
      int count = 0;
      if (quest->m_areaDescription && *quest->m_areaDescription) {
        ++count;
        if (count == index) {
          lua_pushstring(L, quest->m_areaDescription);
          lua_pushstring(L, "event");
          if (logData->m_questFlags & 0x40000000) {
            lua_pushnumber(L, 1.0);
          } else {
            lua_pushnil(L);
          }
          return 3;
        }
      }
      int  bitcount = 0;
      UINT i;
      for (i = 0; i < 4 && count < index; ++i) {
        if (quest->m_monsterToKill[i]) {
          ++count;
          if (count == index) {
            int numKilled = 0;
            int bit;
            for (bit = 0; bit < quest->m_monsterToKillQuantity[i]; ++bit) {
              if (logData->m_questFlags & (1 << (bitcount + bit))) {
                ++numKilled;
              }
            }
            char temp[256];
            char buf[256];
            if (quest->m_monsterToKill[i] & 0x80000000) {
              LPCSTR text = FrameScript_GetText("QUEST_OBJECTS_FOUND", -1, GENDER_NOT_APPLICABLE);
              SStrCopy(temp, text, sizeof(temp));
              LPCSTR name = quest->m_getDescription[i];
              if (!*name) {
                const GameObjectStats *rec =
                    g_gameObjectDBCache.GetRecord(quest->m_monsterToKill[i] & 0x7FFFFFFF, 0, reinterpret_cast<DBCACHECALLBACKPROC>(ObjectQueryCallback), 0);
                UINT pluralIndex = FrameScript_GetPluralIndex(quest->m_monsterToKillQuantity[i]);
                name = rec ? rec->m_name[pluralIndex] : " ";
              }
              SStrPrintf(buf, sizeof(buf), temp, name, numKilled, quest->m_monsterToKillQuantity[i]);
            } else {
              LPCSTR text = FrameScript_GetText("QUEST_MONSTERS_KILLED", -1, GENDER_NOT_APPLICABLE);
              SStrCopy(temp, text, sizeof(temp));
              const CreatureStats_C *rec =
                  g_creatureDBCache.GetRecord(quest->m_monsterToKill[i], 0, reinterpret_cast<DBCACHECALLBACKPROC>(CreatureQueryCallback), 0);
              UINT pluralIndex = FrameScript_GetPluralIndex(quest->m_monsterToKillQuantity[i]);
              SStrPrintf(buf, sizeof(buf), temp, rec ? rec->m_name[pluralIndex] : " ", numKilled, quest->m_monsterToKillQuantity[i]);
            }
            lua_pushstring(L, buf);
            lua_pushstring(L, (quest->m_monsterToKill[i] & 0x80000000) ? "object" : "monster");
            if (numKilled >= quest->m_monsterToKillQuantity[i]) {
              lua_pushnumber(L, 1.0);
            } else {
              lua_pushnil(L);
            }
            return 3;
          }
        }
        bitcount += quest->m_monsterToKillQuantity[i];
      }
      for (i = 0; i < 4 && count < index; ++i) {
        if (quest->m_itemToGet[i] > 0 && quest->m_itemToGet[i] != quest->m_startItem) {
          ++count;
          if (count == index) {
            int  numItems = min(player->CGPlayer_C::GetBag()->GetItemTypeCount(quest->m_itemToGet[i], 8), quest->m_itemToGetQuantity[i]);
            char temp[256];
            char buf[256];
            LPCSTR text = FrameScript_GetText("QUEST_ITEMS_NEEDED", -1, GENDER_NOT_APPLICABLE);
            SStrCopy(temp, text, sizeof(temp));
            const ItemStats *stats = g_itemDBCache.GetRecord(quest->m_itemToGet[i], 0, reinterpret_cast<DBCACHECALLBACKPROC>(ItemQueryCallback), 0);
            UINT             pluralIndex = FrameScript_GetPluralIndex(quest->m_itemToGetQuantity[i]);
            SStrPrintf(buf, sizeof(buf), temp, stats ? stats->m_displayName[pluralIndex] : " ", numItems, quest->m_itemToGetQuantity[i]);
            lua_pushstring(L, buf);
            lua_pushstring(L, "item");
            if (numItems >= quest->m_itemToGetQuantity[i]) {
              lua_pushnumber(L, 1.0);
            } else {
              lua_pushnil(L);
            }
            return 3;
          }
        }
      }
    }
  }
  lua_pushnil(L);
  lua_pushnil(L);
  lua_pushnil(L);
  return 3;
}

static int Script_GetQuestLogTimeLeft(lua_State *L) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    int                  entry = CGQuestLog::GetSelectedLogEntry();
    const CQuestLogData *logData = player->GetQuestLogData(entry);
    if (logData && logData->m_questFailureTime && !(logData->m_questFlags & 0x80000000)) {
      int offset = CGQuestLog::GetServerTimeOffset();
      if (offset) {
        int timeLeft = max(0, static_cast<int>(logData->m_questFailureTime + offset - OsGetTime() - 1));
        lua_pushnumber(L, static_cast<double>(timeLeft));
        return 1;
      }
    }
  }
  lua_pushnil(L);
  return 1;
}

static int Script_IsCurrentQuestFailed(lua_State *L) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    int                  entry = CGQuestLog::GetSelectedLogEntry();
    const CQuestLogData *logData = player->GetQuestLogData(entry);
    if (logData && ((logData->m_questFlags & 0x80000000) || CGQuestLog::IsSelectedQuestExpired())) {
      lua_pushnumber(L, 1.0);
      return 1;
    }
  }
  lua_pushnil(L);
  return 1;
}

static int Script_GetNumQuestLogRewards(lua_State *L) {
  int               count = 0;
  int               questID = CGQuestLog::GetSelectedQuestID();
  const QuestCache *quest = g_questDBCache.GetRecord(questID, 0, 0, 0);
  if (quest) {
    UINT i;
    for (i = 0; i < 4 && quest->m_rewardItems[i]; ++i) {
    }
    if (i < 4) {
      count = i;
    }
  }
  lua_pushnumber(L, static_cast<double>(count));
  return 1;
}

static int Script_GetNumQuestLogChoices(lua_State *L) {
  int               count = 0;
  int               questID = CGQuestLog::GetSelectedQuestID();
  const QuestCache *quest = g_questDBCache.GetRecord(questID, 0, 0, 0);
  if (quest) {
    UINT i;
    for (i = 0; i < 6 && quest->m_rewardChoiceItems[i]; ++i) {
    }
    if (i < 6) {
      count = i;
    }
  }
  lua_pushnumber(L, static_cast<double>(count));
  return 1;
}

static int Script_GetQuestLogRewardInfo(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetQuestLogRewardInfo(index)");
    return 0;
  }
  int               index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  int               questID = CGQuestLog::GetSelectedQuestID();
  const QuestCache *quest = g_questDBCache.GetRecord(questID, 0, 0, 0);
  if (quest && index >= 0 && static_cast<UINT>(index) < 4) {
    const ItemStats_C *stats = g_itemDBCache.GetRecord(quest->m_rewardItems[index], 0, reinterpret_cast<DBCACHECALLBACKPROC>(ItemQueryCallback), 0);
    if (stats) {
      static char buffer[MAX_PATH];
      LPCSTR      path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
      SStrPrintf(buffer, sizeof(buffer), "%s%s", path, *path ? "\\" : "");
      SStrPack(buffer, CGItem_C::GetInventoryArt(stats->m_displayInfoID), sizeof(buffer));
      lua_pushstring(L, stats->m_displayName[0]);
      lua_pushstring(L, buffer);
      lua_pushnumber(L, static_cast<double>(quest->m_rewardAmount[index]));
      lua_pushnumber(L, static_cast<double>(stats->m_inventoryType ? stats->m_overallQualityID : -1));
      CGPlayer_C     *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
      GAME_ERROR_TYPE reason;
      if (!player || player->CanUseItem(stats, reason)) {
        lua_pushnumber(L, 1.0);
      } else {
        lua_pushnil(L);
      }
      return 5;
    }
  }
  lua_pushnil(L);
  lua_pushnil(L);
  lua_pushnumber(L, 1.0);
  lua_pushnumber(L, 0.0);
  lua_pushnil(L);
  return 5;
}

static int Script_GetQuestLogChoiceInfo(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetQuestLogRewardInfo(index)");
    return 0;
  }
  int               index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  int               questID = CGQuestLog::GetSelectedQuestID();
  const QuestCache *quest = g_questDBCache.GetRecord(questID, 0, 0, 0);
  if (quest && index >= 0 && static_cast<UINT>(index) < 6) {
    const ItemStats_C *stats = g_itemDBCache.GetRecord(quest->m_rewardChoiceItems[index], 0, reinterpret_cast<DBCACHECALLBACKPROC>(ItemQueryCallback), 0);
    if (stats) {
      static char buffer[MAX_PATH];
      LPCSTR      path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
      SStrPrintf(buffer, sizeof(buffer), "%s%s", path, *path ? "\\" : "");
      SStrPack(buffer, CGItem_C::GetInventoryArt(stats->m_displayInfoID), sizeof(buffer));
      lua_pushstring(L, stats->m_displayName[0]);
      lua_pushstring(L, buffer);
      lua_pushnumber(L, static_cast<double>(quest->m_rewardChoiceAmount[index]));
      lua_pushnumber(L, static_cast<double>(stats->m_inventoryType ? stats->m_overallQualityID : -1));
      CGPlayer_C     *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
      GAME_ERROR_TYPE reason;
      if (!player || player->CanUseItem(stats, reason)) {
        lua_pushnumber(L, 1.0);
      } else {
        lua_pushnil(L);
      }
      return 5;
    }
  }
  lua_pushnil(L);
  lua_pushnil(L);
  lua_pushnumber(L, 1.0);
  lua_pushnumber(L, 0.0);
  lua_pushnil(L);
  return 5;
}

static int Script_GetQuestLogRewardMoney(lua_State *L) {
  int               questID = CGQuestLog::GetSelectedQuestID();
  const QuestCache *quest = g_questDBCache.GetRecord(questID, 0, 0, 0);
  if (quest) {
    lua_pushnumber(L, static_cast<double>(quest->m_rewardMoney));
    return 1;
  }
  lua_pushnumber(L, 0.0);
  return 1;
}

static int Script_GetQuestTimers(lua_State *L) {
  UINT        count = 0;
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    UINT numEntries = CGQuestLog::GetNumEntries();
    int  offset = CGQuestLog::GetServerTimeOffset();
    UINT i;
    for (i = 0; i < numEntries; ++i) {
      int                  entry = CGQuestLog::GetQuestLogEntry(i);
      const CQuestLogData *logData = player->GetQuestLogData(entry);
      if (logData && logData->m_questFailureTime && !(logData->m_questFlags & 0x80000000) && !CGQuestLog::IsQuestExpired(i)) {
        int timeLeft = logData->m_questFailureTime + offset - OsGetTime() - 1;
        if (timeLeft >= 0) {
          lua_pushnumber(L, static_cast<double>(timeLeft));
          ++count;
        } else {
          CGQuestLog::SetQuestExpired(i);
          const QuestCache *quest = g_questDBCache.GetRecord(logData->m_questID, 0, reinterpret_cast<DBCACHECALLBACKPROC>(QuestFailedCallback), 0);
          if (quest) {
            CGGameUI::DisplayError(GERR_QUEST_FAILED_S, quest->m_logTitle);
          }
          FrameScript_SignalEvent(285);
        }
      }
    }
  }
  return count;
}

static int Script_GetQuestIndexForTimer(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetQuestIndexForTimer(index)");
    return 0;
  }
  int         index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  int         count = 0;
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    UINT numEntries = CGQuestLog::GetNumEntries();
    int  offset = CGQuestLog::GetServerTimeOffset();
    UINT i;
    for (i = 0; i < numEntries; ++i) {
      int                  entry = CGQuestLog::GetQuestLogEntry(i);
      const CQuestLogData *logData = player->GetQuestLogData(entry);
      if (logData && logData->m_questFailureTime && !(logData->m_questFlags & 0x80000000)) {
        if (static_cast<int>(offset - OsGetTime() + logData->m_questFailureTime - 1) >= 0) {
          if (count == index) {
            lua_pushnumber(L, static_cast<double>(i + 1));
            return 1;
          }
          ++count;
        }
      }
    }
  }
  lua_pushnil(L);
  return 1;
}

static int Script_CollapseQuestHeader(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: CollapseQuestHeader(index)");
    return 0;
  }
  int index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  CGQuestLog::CollapseHeader(index, 1);
  return 0;
}

static int Script_ExpandQuestHeader(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: ExpandQuestHeader(index)");
    return 0;
  }
  int index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  CGQuestLog::CollapseHeader(index, 0);
  return 0;
}

static FrameScript_Method s_ScriptFunctions[22] = {
    {  "GetNumQuestLogEntries",   Script_GetNumQuestLogEntries},
    {       "GetQuestLogTitle",        Script_GetQuestLogTitle},
    {    "SelectQuestLogEntry",     Script_SelectQuestLogEntry},
    {   "GetQuestLogSelection",    Script_GetQuestLogSelection},
    {    "SwapQuestLogEntries",     Script_SwapQuestLogEntries},
    {        "SetAbandonQuest",         Script_SetAbandonQuest},
    {    "GetAbandonQuestName",     Script_GetAbandonQuestName},
    {           "AbandonQuest",            Script_AbandonQuest},
    {   "GetQuestLogQuestText",    Script_GetQuestLogQuestText},
    {"GetNumQuestLeaderBoards", Script_GetNumQuestLeaderBoards},
    { "GetQuestLogLeaderBoard",  Script_GetQuestLogLeaderBoard},
    {    "GetQuestLogTimeLeft",     Script_GetQuestLogTimeLeft},
    {   "IsCurrentQuestFailed",    Script_IsCurrentQuestFailed},
    {  "GetNumQuestLogRewards",   Script_GetNumQuestLogRewards},
    {  "GetNumQuestLogChoices",   Script_GetNumQuestLogChoices},
    {  "GetQuestLogRewardInfo",   Script_GetQuestLogRewardInfo},
    {  "GetQuestLogChoiceInfo",   Script_GetQuestLogChoiceInfo},
    { "GetQuestLogRewardMoney",  Script_GetQuestLogRewardMoney},
    {         "GetQuestTimers",          Script_GetQuestTimers},
    {  "GetQuestIndexForTimer",   Script_GetQuestIndexForTimer},
    {    "CollapseQuestHeader",     Script_CollapseQuestHeader},
    {      "ExpandQuestHeader",       Script_ExpandQuestHeader}
};

void QuestLogRegisterScriptFunctions() {
  UINT index;
  for (index = 0; index < 22; ++index) {
    FrameScript_RegisterFunction(s_ScriptFunctions[index].name, s_ScriptFunctions[index].method);
  }
}

void QuestLogUnregisterScriptFunctions() {
  UINT index;
  for (index = 0; index < 22; ++index) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[index].name);
  }
}
