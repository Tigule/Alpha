#include <Base/Base.h>
#include <WowConst.h>
#include <MapDefs.h>

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
  if (granted) {
    if (!s_questCallbackCount || !--s_questCallbackCount) {
      CGQuestLog::Update(1);
    }
  }
}

static void QuestQueryCallback(int id, const DWORDLONG &, LPVOID, bool granted) {
  if (granted) {
    FrameScript_SignalEvent(285);
  } else {
    ConsoleWrite("Invalid quest log entry", DEFAULT_COLOR);
  }
}

static void QuestSelectQueryCallback(int id, const DWORDLONG &, LPVOID, bool granted) {
  if (granted) {
    CGQuestLog::UpdateSelection();
  }
}

static void QuestFailedCallback(int id, const DWORDLONG &, LPVOID, bool granted) {
  if (granted) {
    FrameScript_SignalEvent(285);
  }
}

static void CreatureQueryCallback(int id, const DWORDLONG &, LPVOID, bool granted) {
  if (granted) {
    FrameScript_SignalEvent(285);
  }
}

static void ObjectQueryCallback(int id, const DWORDLONG &, LPVOID, bool granted) {
  if (granted) {
    FrameScript_SignalEvent(285);
  }
}

static void ItemQueryCallback(int id, const DWORDLONG &guid, LPVOID, bool granted) {
  if (granted) {
    FrameScript_SignalEvent(285);
  }
}

static int QuestLogUpdateHandler(DWORDLONG, UINT offset, UINT bytes, LPCVOID, LPVOID) {
  CGQuestLog::Update(1);
  return 1;
}

static int OnQueryTimeResponse(LPVOID, NETMESSAGE msgId, DWORD eventTime, CDataStore *msg) {
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
  UINT      playerOffset = CGPlayer_C::OffsetOf(ID_PLAYER);
  ClntObjMgrSetObjMirrorHandler(player, playerOffset + 1372, 384, QuestLogUpdateHandler, 0, HANDLER_PRIORITY_NORMAL);
  ClientServices_SetMessageHandler(SMSG_QUERY_TIME_RESPONSE, OnQueryTimeResponse, 0);

  CDataStore msg;
  msg.Put(CMSG_QUERY_TIME);
  msg.Finalize();
  ClientServices_Send(&msg);
  m_expiredQuests = 0;
  Update(1);
}

void CGQuestLog::LeaveWorld() {
  DWORDLONG player = ClntObjMgrGetActivePlayer();
  UINT      playerOffset = CGPlayer_C::OffsetOf(ID_PLAYER);
  ClntObjMgrUnsetObjMirrorHandler(player, playerOffset + 1372, QuestLogUpdateHandler, 0);
  ClientServices_ClearMessageHandler(SMSG_QUERY_TIME_RESPONSE);
}

int __cdecl QSortQuestSortTypes(LPCVOID a, LPCVOID b) {
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
  if (sort1 > 0) {
    const AreaTableRec *area = g_areaTableDB.GetRecord(sort1);
    if (area) {
      SStrCopy(name1, area->m_AreaName_lang[CURRENT_LANGUAGE], sizeof(name1));
    }
  } else if (sort1 < 0) {
    const QuestSortRec *sort = g_questSortDB.GetRecord(-sort1);
    if (sort) {
      SStrCopy(name1, sort->m_SortName_lang[CURRENT_LANGUAGE], sizeof(name1));
    }
  }
  if (sort2 > 0) {
    const AreaTableRec *area = g_areaTableDB.GetRecord(sort2);
    if (area) {
      SStrCopy(name2, area->m_AreaName_lang[CURRENT_LANGUAGE], sizeof(name2));
    }
  } else if (sort2 < 0) {
    const QuestSortRec *sort = g_questSortDB.GetRecord(-sort2);
    if (sort) {
      SStrCopy(name2, sort->m_SortName_lang[CURRENT_LANGUAGE], sizeof(name2));
    }
  }
  return SStrCmpI(name1, name2, 0x7FFFFFFF);
}

int __cdecl QSortQuests(LPCVOID a, LPCVOID b) {
  FATALASSERT(a);
  FATALASSERT(b);
  const QuestLogInfo info1 = *static_cast<const QuestLogInfo *>(a);
  const QuestLogInfo info2 = *static_cast<const QuestLogInfo *>(b);
  const QuestCache  *quest1 = info1.isHeader ? 0 : g_questDBCache.GetRecord(info1.questID, 0, 0, 0);
  const QuestCache  *quest2 = info2.isHeader ? 0 : g_questDBCache.GetRecord(info2.questID, 0, 0, 0);
  UINT               sortRank1 = 0;
  UINT               sortRank2 = 0;
  UINT               i;
  int                sortID1 = info1.isHeader ? info1.questID : quest1 ? quest1->m_questSortID : 0;
  int                sortID2 = info2.isHeader ? info2.questID : quest2 ? quest2->m_questSortID : 0;
  for (i = 0; i < CGQuestLog::m_numSortTypes; ++i) {
    if (CGQuestLog::m_sortTypes[i] == sortID1) {
      sortRank1 = i;
    }
    if (CGQuestLog::m_sortTypes[i] == sortID2) {
      sortRank2 = i;
    }
  }
  int hidden1 = !info1.isHeader && sortRank1 < 16 && !(CGQuestLog::m_collapseFilter & (1 << sortRank1));
  int hidden2 = !info2.isHeader && sortRank2 < 16 && !(CGQuestLog::m_collapseFilter & (1 << sortRank2));
  if (hidden1) {
    return hidden2 ? 0 : 1;
  }
  if (hidden2) {
    return -1;
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
  if (!quest1 || !quest2) {
    return 0;
  }
  if (quest1->m_questLevel != quest2->m_questLevel) {
    return quest1->m_questLevel < quest2->m_questLevel ? -1 : 1;
  }
  return SStrCmp(quest1->m_logTitle, quest2->m_logTitle, 0x7FFFFFFF);
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
      if (entry->m_questFailureTime && static_cast<int>(entry->m_questFlags) >= 0 && offset + entry->m_questFailureTime - OsGetTime() - 1 < 0) {
        m_expiredQuests |= 1 << i;
      }
      ++m_numQuests;
      UINT sortIndex;
      for (sortIndex = 0; sortIndex < m_numSortTypes; ++sortIndex) {
        if (m_sortTypes[sortIndex] == quest->m_questSortID) {
          break;
        }
      }
      FATALASSERT(sortIndex < 16);
      if (sortIndex == m_numSortTypes) {
        m_sortTypes[sortIndex] = quest->m_questSortID;
        ++m_numSortTypes;
        m_quests[m_numQuests].questID = quest->m_questSortID;
        m_quests[m_numQuests].isHeader = 1;
        ++m_numQuests;
      }
    }
  }
  FilterAndSortQuests();
  FrameScript_SignalEvent(285);
  if (s_nextTimeUpdate && OsGetTime() > s_nextTimeUpdate) {
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
      int sortIndex = GetQuestSortIndex(i);
      if (sortIndex >= 0 && sortIndex < 16 && !(m_collapseFilter & (1 << sortIndex))) {
        --m_numShownQuests;
      }
    }
  }
}

void CGQuestLog::CollapseHeader(UINT index, int collapse) {
  if (index < m_numQuests && m_quests[index].isHeader) {
    UINT sortIndex = GetQuestSortIndex(index);
    if (sortIndex != m_numSortTypes) {
      if (collapse) {
        m_collapseFilter &= ~(1 << sortIndex);
      } else {
        m_collapseFilter |= 1 << sortIndex;
      }
      FilterAndSortQuests();
      FrameScript_SignalEvent(285);
    }
  } else {
    m_collapseFilter = collapse ? 0 : -1;
    FilterAndSortQuests();
    FrameScript_SignalEvent(285);
  }
}

int CGQuestLog::GetQuestSortIndex(UINT index) {
  if (index >= m_numQuests) {
    return -1;
  }
  int sortID = m_quests[index].questID;
  if (!m_quests[index].isHeader) {
    const QuestCache *quest = g_questDBCache.GetRecord(sortID, 0, 0, 0);
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
  if (index >= 0 && static_cast<UINT>(index) < m_numQuests && !m_quests[index].isHeader) {
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
      return index;
    }
  }
  return -1;
}

int CGQuestLog::GetSelectedLogEntry() {
  int index = GetSelectionIndex();
  return index < 0 ? -1 : m_quests[index].logIndex;
}

LPCSTR CGQuestLog::GetAbandonQuestName() {
  const QuestCache *quest = g_questDBCache.GetRecord(m_abandonQuest, 0, 0, 0);
  return quest ? quest->m_logTitle : 0;
}

void CGQuestLog::AbandonSelectedQuest() {
  if (!m_abandonQuest) {
    return;
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    UINT index;
    for (index = 0; index < m_numQuests; ++index) {
      if (!m_quests[index].isHeader && m_quests[index].questID == m_abandonQuest) {
        player->QuestLogRemoveQuest(m_quests[index].logIndex);
        break;
      }
    }
    m_abandonQuest = 0;
  }
}

void CGQuestLog::ClearQuest(int id) {
  if (id && id == m_selectedQuest) {
    m_selectedQuest = 0;
    MinimapSetQuestPOI(0.0f, 0.0f, 0, 0);
  }
}

void CGQuestLog::AbandonQuest(int index) {
  if (index >= 0 && static_cast<UINT>(index) < m_numQuests && !m_quests[index].isHeader) {
    m_abandonQuest = m_quests[index].questID;
    AbandonSelectedQuest();
  }
}

LPCSTR CGQuestLog::GetQuestName(int index) {
  if (index < 0 || static_cast<UINT>(index) >= m_numQuests) {
    return 0;
  }
  if (m_quests[index].isHeader) {
    int sortID = m_quests[index].questID;
    if (sortID > 0) {
      const AreaTableRec *area = g_areaTableDB.GetRecord(sortID);
      return area ? area->m_AreaName_lang[CURRENT_LANGUAGE] : "Missing header! (quest designers)";
    }
    if (sortID < 0) {
      const QuestSortRec *sort = g_questSortDB.GetRecord(-sortID);
      return sort ? sort->m_SortName_lang[CURRENT_LANGUAGE] : "Missing header! (quest designers)";
    }
    return "Missing header! (quest designers)";
  }
  const QuestCache *quest = g_questDBCache.GetRecord(m_quests[index].questID, 0, reinterpret_cast<DBCACHECALLBACKPROC>(QuestQueryCallback), 0);
  return quest ? quest->m_logTitle : 0;
}

LPCSTR CGQuestLog::GetQuestTag(int index) {
  if (index < 0 || static_cast<UINT>(index) >= m_numQuests || m_quests[index].isHeader) {
    return 0;
  }
  const QuestCache   *quest = g_questDBCache.GetRecord(m_quests[index].questID, 0, reinterpret_cast<DBCACHECALLBACKPROC>(QuestQueryCallback), 0);
  const QuestInfoRec *info = quest ? g_questInfoDB.GetRecord(quest->m_questInfoID) : 0;
  return info ? info->m_InfoName_lang[CURRENT_LANGUAGE] : 0;
}

int CGQuestLog::GetQuestLevel(int index) {
  if (index < 0 || static_cast<UINT>(index) >= m_numQuests || m_quests[index].isHeader) {
    return 0;
  }
  const QuestCache *quest = g_questDBCache.GetRecord(m_quests[index].questID, 0, reinterpret_cast<DBCACHECALLBACKPROC>(QuestQueryCallback), 0);
  return quest ? quest->m_questLevel : 0;
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

int CGQuestLog::IsSelectedQuestExpired() {
  int index = GetSelectionIndex();
  return index >= 0 && (m_expiredQuests & (1 << index));
}

int CGQuestLog::IsQuestExpired(UINT index) {
  return index < m_numQuests && (m_expiredQuests & (1 << index));
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
  lua_pushnumber(L, static_cast<double>(CGQuestLog::GetNumEntries()));
  return 2;
}

static int Script_GetQuestLogTitle(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetQuestLogTitle(index)");
  }
  int index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  lua_pushstring(L, CGQuestLog::GetQuestName(index));
  lua_pushnumber(L, static_cast<double>(CGQuestLog::GetQuestLevel(index)));
  lua_pushstring(L, CGQuestLog::GetQuestTag(index));
  if (CGQuestLog::IsQuestHeader(index)) {
    lua_pushnumber(L, 1.0);
    int sortIndex = CGQuestLog::GetQuestSortIndex(index);
    if (sortIndex >= 0 && CGQuestLog::IsSortHeaderCollapsed(sortIndex)) {
      lua_pushnumber(L, 1.0);
    } else {
      lua_pushnil(L);
    }
    return 5;
  }
  lua_pushnil(L);
  lua_pushnil(L);
  return 5;
}

static int Script_SelectQuestLogEntry(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: SelectQuestLogEntry(index)");
  }
  CGQuestLog::SetSelectedQuest(static_cast<int>(lua_tonumber(L, 1)) - 1);
  return 0;
}

static int Script_GetQuestLogSelection(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGQuestLog::GetSelectionIndex() + 1));
  return 1;
}

static int Script_SwapQuestLogEntries(lua_State *L) {
  if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
    return luaL_error(L, "Usage: SwapQuestLogEntries(index1, index2)");
  }
  int         index1 = static_cast<int>(lua_tonumber(L, 1)) - 1;
  int         index2 = static_cast<int>(lua_tonumber(L, 2)) - 1;
  int         entry1 = CGQuestLog::GetQuestLogEntry(index1);
  int         entry2 = CGQuestLog::GetQuestLogEntry(index2);
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player && entry1 >= 0 && entry2 >= 0) {
    player->QuestLogSwapQuest(entry1, entry2);
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
  const QuestCache *quest =
      g_questDBCache.GetRecord(CGQuestLog::GetSelectedQuestID(), 0, reinterpret_cast<DBCACHECALLBACKPROC>(QuestQueryCallback), 0);
  char logDesc[1024];
  char questText[1024];
  if (quest) {
    QuestParserParseText(quest->m_questDescription, questText, sizeof(questText), ClntObjMgrGetActivePlayer(), 0);
    QuestParserParseText(quest->m_logDescription, logDesc, sizeof(logDesc), ClntObjMgrGetActivePlayer(), 0);
    lua_pushstring(L, questText);
    lua_pushstring(L, logDesc);
  } else {
    lua_pushnil(L);
    lua_pushnil(L);
  }
  return 2;
}

static int Script_GetNumQuestLeaderBoards(lua_State *L) {
  int               count = 0;
  CGPlayer_C       *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  const QuestCache *quest =
      player ? g_questDBCache.GetRecord(CGQuestLog::GetSelectedQuestID(), 0, reinterpret_cast<DBCACHECALLBACKPROC>(QuestQueryCallback), 0) : 0;
  if (quest) {
    if (quest->m_areaDescription[0]) {
      ++count;
    }
    int index;
    for (index = 0; index < 4; ++index) {
      if (quest->m_monsterToKill[index]) {
        ++count;
      }
      if (quest->m_itemToGet[index] > 0 && quest->m_itemToGet[index] != quest->m_startItem) {
        ++count;
      }
    }
  }
  lua_pushnumber(L, static_cast<double>(count));
  return 1;
}

static int Script_GetQuestLogLeaderBoard(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetQuestLogLeaderBoard(index)");
  }
  int               wanted = static_cast<int>(lua_tonumber(L, 1));
  int               logEntry = CGQuestLog::GetSelectedLogEntry();
  const QuestCache *quest =
      g_questDBCache.GetRecord(CGQuestLog::GetSelectedQuestID(), 0, reinterpret_cast<DBCACHECALLBACKPROC>(QuestQueryCallback), 0);
  CGPlayer_C          *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  const CQuestLogData *logData = player && logEntry >= 0 && logEntry < 17 ? player->GetQuestLogData(logEntry) : 0;
  if (!quest || !logData) {
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnil(L);
    return 3;
  }

  int found = 0;
  if (quest->m_areaDescription[0] && ++found == wanted) {
    lua_pushstring(L, quest->m_areaDescription);
    lua_pushstring(L, "event");
    if (logData->m_questFlags & 0x40000000) {
      lua_pushnumber(L, 1.0);
    } else {
      lua_pushnil(L);
    }
    return 3;
  }

  UINT objectiveBit = 0;
  int  index;
  for (index = 0; index < 4; ++index) {
    if (quest->m_monsterToKill[index] && ++found == wanted) {
      int current = 0;
      int count;
      for (count = 0; count < quest->m_monsterToKillQuantity[index]; ++count) {
        if (logData->m_questFlags & (1 << (objectiveBit + count))) {
          ++current;
        }
      }

      char   format[256];
      char   buf[256];
      LPCSTR name = " ";
      if (quest->m_monsterToKill[index] >= 0) {
        SStrCopy(format, FrameScript_GetText("QUEST_MONSTERS_KILLED", -1, GENDER_NOT_APPLICABLE), sizeof(format));
        const CreatureStats_C *stats =
            g_creatureDBCache.GetRecord(quest->m_monsterToKill[index], 0, reinterpret_cast<DBCACHECALLBACKPROC>(CreatureQueryCallback), 0);
        if (stats) {
          name = stats->m_name[FrameScript_GetPluralIndex(quest->m_monsterToKillQuantity[index])];
        }
      } else {
        SStrCopy(format, FrameScript_GetText("QUEST_OBJECTS_FOUND", -1, GENDER_NOT_APPLICABLE), sizeof(format));
        name = quest->m_getDescription[index];
        if (!*name) {
          const GameObjectStats_C *stats = g_gameObjectDBCache.GetRecord(
              quest->m_monsterToKill[index] & 0x7FFFFFFF, 0, reinterpret_cast<DBCACHECALLBACKPROC>(ObjectQueryCallback), 0
          );
          if (stats) {
            name = stats->m_name[FrameScript_GetPluralIndex(quest->m_monsterToKillQuantity[index])];
          }
        }
      }
      SStrPrintf(buf, sizeof(buf), format, name, current, quest->m_monsterToKillQuantity[index]);
      lua_pushstring(L, buf);
      lua_pushstring(L, quest->m_monsterToKill[index] < 0 ? "object" : "monster");
      if (current >= quest->m_monsterToKillQuantity[index]) {
        lua_pushnumber(L, 1.0);
      } else {
        lua_pushnil(L);
      }
      return 3;
    }
    objectiveBit += quest->m_monsterToKillQuantity[index];
  }

  for (index = 0; index < 4; ++index) {
    if (quest->m_itemToGet[index] > 0 && quest->m_itemToGet[index] != quest->m_startItem && ++found == wanted) {
      int current = player->GetBag()->GetItemTypeCount(quest->m_itemToGet[index], 8);
      if (current > quest->m_itemToGetQuantity[index]) {
        current = quest->m_itemToGetQuantity[index];
      }
      char format[256];
      char buf[256];
      SStrCopy(format, FrameScript_GetText("QUEST_ITEMS_NEEDED", -1, GENDER_NOT_APPLICABLE), sizeof(format));
      const ItemStats_C *stats = g_itemDBCache.GetRecord(quest->m_itemToGet[index], 0, reinterpret_cast<DBCACHECALLBACKPROC>(ItemQueryCallback), 0);
      LPCSTR             name = stats ? stats->m_displayName[FrameScript_GetPluralIndex(quest->m_itemToGetQuantity[index])] : " ";
      SStrPrintf(buf, sizeof(buf), format, name, current, quest->m_itemToGetQuantity[index]);
      lua_pushstring(L, buf);
      lua_pushstring(L, "item");
      if (current >= quest->m_itemToGetQuantity[index]) {
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

static int Script_GetQuestLogTimeLeft(lua_State *L) {
  int         entry = CGQuestLog::GetSelectedLogEntry();
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player && entry >= 0 && entry < 17 && CGQuestLog::GetServerTimeOffset()) {
    const CQuestLogData *logData = player->GetQuestLogData(entry);
    if (logData->m_questFailureTime && static_cast<int>(logData->m_questFlags) >= 0) {
      int timeLeft = CGQuestLog::GetServerTimeOffset() + logData->m_questFailureTime - OsGetTime() - 1;
      if (timeLeft < 0) {
        timeLeft = 0;
      }
      lua_pushnumber(L, static_cast<double>(timeLeft));
      return 1;
    }
  }
  lua_pushnil(L);
  return 1;
}

static int Script_IsCurrentQuestFailed(lua_State *L) {
  int         entry = CGQuestLog::GetSelectedLogEntry();
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player && entry >= 0 && entry < 17 &&
      (static_cast<int>(player->GetQuestLogData(entry)->m_questFlags) < 0 || CGQuestLog::IsSelectedQuestExpired()))
  {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_GetNumQuestLogRewards(lua_State *L) {
  int               count = 0;
  const QuestCache *quest = g_questDBCache.GetRecord(CGQuestLog::GetSelectedQuestID(), 0, 0, 0);
  int               index;
  for (index = 0; quest && index < 4; ++index) {
    if (quest->m_rewardItems[index]) {
      ++count;
    }
  }
  lua_pushnumber(L, static_cast<double>(count));
  return 1;
}

static int Script_GetNumQuestLogChoices(lua_State *L) {
  int               count = 0;
  const QuestCache *quest = g_questDBCache.GetRecord(CGQuestLog::GetSelectedQuestID(), 0, 0, 0);
  int               index;
  for (index = 0; quest && index < 6; ++index) {
    if (quest->m_rewardChoiceItems[index]) {
      ++count;
    }
  }
  lua_pushnumber(L, static_cast<double>(count));
  return 1;
}

static int Script_GetQuestLogRewardInfo(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetQuestLogRewardInfo(index)");
  }
  int                index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  const QuestCache  *quest = g_questDBCache.GetRecord(CGQuestLog::GetSelectedQuestID(), 0, 0, 0);
  int                itemID = quest && index >= 0 && index < 4 ? quest->m_rewardItems[index] : 0;
  const ItemStats_C *stats = itemID ? g_itemDBCache.GetRecord(itemID, 0, reinterpret_cast<DBCACHECALLBACKPROC>(ItemQueryCallback), 0) : 0;
  if (!stats) {
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnumber(L, 1.0);
    lua_pushnumber(L, 0.0);
    lua_pushnil(L);
    return 5;
  }
  static char texture[260];
  LPCSTR      path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
  SStrPrintf(texture, sizeof(texture), "%s%s", path, *path ? "\\" : "");
  SStrPack(texture, CGItem_C::GetInventoryArt(stats->m_displayInfoID), sizeof(texture));
  lua_pushstring(L, stats->m_displayName[0]);
  lua_pushstring(L, texture);
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

static int Script_GetQuestLogChoiceInfo(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetQuestLogRewardInfo(index)");
  }
  int                index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  const QuestCache  *quest = g_questDBCache.GetRecord(CGQuestLog::GetSelectedQuestID(), 0, 0, 0);
  int                itemID = quest && index >= 0 && index < 6 ? quest->m_rewardChoiceItems[index] : 0;
  const ItemStats_C *stats = itemID ? g_itemDBCache.GetRecord(itemID, 0, reinterpret_cast<DBCACHECALLBACKPROC>(ItemQueryCallback), 0) : 0;
  if (!stats) {
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnumber(L, 1.0);
    lua_pushnumber(L, 0.0);
    lua_pushnil(L);
    return 5;
  }
  static char texture[260];
  LPCSTR      path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
  SStrPrintf(texture, sizeof(texture), "%s%s", path, *path ? "\\" : "");
  SStrPack(texture, CGItem_C::GetInventoryArt(stats->m_displayInfoID), sizeof(texture));
  lua_pushstring(L, stats->m_displayName[0]);
  lua_pushstring(L, texture);
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

static int Script_GetQuestLogRewardMoney(lua_State *L) {
  const QuestCache *quest = g_questDBCache.GetRecord(CGQuestLog::GetSelectedQuestID(), 0, 0, 0);
  lua_pushnumber(L, static_cast<double>(quest ? quest->m_rewardMoney : 0));
  return 1;
}

static int Script_GetQuestTimers(lua_State *L) {
  UINT        count = 0;
  UINT        numEntries = CGQuestLog::GetNumEntries();
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  UINT        index;
  int         offset;
  for (index = 0; player && index < numEntries; ++index) {
    if (CGQuestLog::IsQuestHeader(index)) {
      continue;
    }
    offset = CGQuestLog::GetQuestLogEntry(index);
    if (offset >= 0 && offset < 17) {
      const CQuestLogData *logData = player->GetQuestLogData(offset);
      if (logData->m_questFailureTime && static_cast<int>(logData->m_questFlags) >= 0 && !CGQuestLog::IsQuestExpired(index)) {
        int timeLeft = CGQuestLog::GetServerTimeOffset() + logData->m_questFailureTime - OsGetTime() - 1;
        if (timeLeft < 0) {
          CGQuestLog::SetQuestExpired(index);
          const QuestCache *quest = g_questDBCache.GetRecord(logData->m_questID, 0, reinterpret_cast<DBCACHECALLBACKPROC>(QuestFailedCallback), 0);
          if (quest) {
            CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(125), quest->m_logTitle);
          }
          FrameScript_SignalEvent(285);
        } else {
          lua_pushnumber(L, static_cast<double>(timeLeft));
          ++count;
        }
      }
    }
  }
  return count;
}

static int Script_GetQuestIndexForTimer(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetQuestIndexForTimer(index)");
  }
  int wanted = static_cast<int>(lua_tonumber(L, 1));
  --wanted;
  int         count = 0;
  UINT        numEntries = CGQuestLog::GetNumEntries();
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  int         index;
  int         offset;
  for (index = 0; player && index < numEntries; ++index) {
    if (CGQuestLog::IsQuestHeader(index)) {
      continue;
    }
    offset = CGQuestLog::GetQuestLogEntry(index);
    if (offset >= 0 && offset < 17) {
      const CQuestLogData *logData = player->GetQuestLogData(offset);
      if (logData->m_questFailureTime && static_cast<int>(logData->m_questFlags) >= 0 &&
          CGQuestLog::GetServerTimeOffset() + logData->m_questFailureTime - OsGetTime() - 1 >= 0)
      {
        if (count == wanted) {
          lua_pushnumber(L, static_cast<double>(index + 1));
          return 1;
        }
        ++count;
      }
    }
  }
  lua_pushnil(L);
  return 1;
}

static int Script_CollapseQuestHeader(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: CollapseQuestHeader(index)");
  }
  int index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  CGQuestLog::CollapseHeader(index < 0 ? CGQuestLog::GetNumEntries() : index, 1);
  return 0;
}

static int Script_ExpandQuestHeader(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: ExpandQuestHeader(index)");
  }
  int index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  CGQuestLog::CollapseHeader(index < 0 ? CGQuestLog::GetNumEntries() : index, 0);
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
