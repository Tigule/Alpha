#include "GameUI.h"
#include "QuestFrame.h"

#include "DB/DBClient/DBCacheInstances.h"
#include "DB/DBClient/DBClient.h"
#include "DB/DBClient/AutoCode/PageTextMaterialRec.h"
#include "Game/GameClient/NameCache.h"
#include "Object/ItemStats.h"
#include "Object/ObjectClient/GameObject_C.h"
#include "Object/ObjectClient/Object_C.h"
#include "Object/ObjectClient/Item_C.h"
#include "Object/ObjectClient/Player_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "Net/NetClient/NetClient.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

#include <FrameScript/FrameScript.h>
#include <Base/CDataStore.h>
#include <lauxlib.h>
#include <lua.h>
#include <storm.h>

#include <string.h>

static const float MAX_SHOP_DISTANCE = 5.5555553f;
static const float MAX_SHOP_DISTANCE_SQUARED = MAX_SHOP_DISTANCE * MAX_SHOP_DISTANCE;

bool QuestParserParseText(const char *text, char *buf, unsigned int size, const unsigned __int64 &target, int restoreToken);

static void QuestItemStatsCallback(int, const unsigned __int64 &, void *, bool granted) {
  if (granted) {
    FrameScript_SignalEvent(280);
  }
}

unsigned __int64 CGQuestInfo::m_npc;
QUEST_STATE      CGQuestInfo::m_state;
int              CGQuestInfo::m_currentQuest;
int              CGQuestInfo::m_completable;
int              CGQuestInfo::m_autoLaunched;
int              CGQuestInfo::m_lastChosenItem;
int              CGQuestInfo::m_rewardMoney;
unsigned int     CGQuestInfo::m_numQuests;
unsigned int     CGQuestInfo::m_numInProgress;
QuestInfo        CGQuestInfo::m_quests[8];
QuestInfo        CGQuestInfo::m_inProgress[8];
QuestItemInfo    CGQuestInfo::m_questItems[6];
char             CGQuestInfo::m_greetingText[256];
char             CGQuestInfo::m_questTitle[64];
char             CGQuestInfo::m_questText[1024];
char             CGQuestInfo::m_questLogText[1024];
char             CGQuestInfo::m_progressText[1024];
char             CGQuestInfo::m_rewardText[1024];
int              CGQuestInfo::m_pendingQuest;

void CGQuestInfo::EnterWorld() {
  m_npc = 0;
  m_state = QUEST_GREETING;
  m_currentQuest = 0;
  m_completable = 0;
  m_autoLaunched = 0;
  m_lastChosenItem = 0;
  memset(m_quests, 0, sizeof(m_quests));
  memset(m_inProgress, 0, sizeof(m_inProgress));
  m_numQuests = 0;
  m_numInProgress = 0;
  m_rewardMoney = 0;
  m_pendingQuest = 0;
  m_greetingText[0] = 0;
  m_questTitle[0] = 0;
  m_questText[0] = 0;
  m_questLogText[0] = 0;
  m_progressText[0] = 0;
  m_rewardText[0] = 0;
  memset(m_questItems, 0, sizeof(m_questItems));
}

void CGQuestInfo::LeaveWorld() {
  QuestGiverFinished();
}

void CGQuestInfo::SetState(unsigned __int64 guid, QUEST_STATE state, const char *text, int quest) {
  FATALASSERT(state < QUEST_STATE_NUM_TYPES);

  CGObject_C *object = ClntObjMgrObjectPtr(guid, __FILE__, __LINE__);
  if (!object || !(object->GetType() & (TYPE_ITEM | TYPE_UNIT | TYPE_GAMEOBJECT))) {
    return;
  }

  if (state == QUEST_GREETING) {
    ClearQuests();
    ClearItems();
    m_greetingText[0] = 0;
    m_questTitle[0] = 0;
    m_questText[0] = 0;
    m_questLogText[0] = 0;
    m_progressText[0] = 0;
    m_rewardText[0] = 0;
  }

  char             parsed[1024];
  unsigned __int64 player = ClntObjMgrGetActivePlayer();
  QuestParserParseText(text, parsed, sizeof(parsed), player, 0);
  if (!parsed[0]) {
    SStrCopy(parsed, " ", sizeof(parsed));
  }

  switch (state) {
    case QUEST_GREETING:
      SStrCopy(m_greetingText, parsed, sizeof(m_greetingText));
      break;
    case QUEST_OFFER:
      SStrCopy(m_questText, parsed, sizeof(m_questText));
      break;
    case QUEST_ACCEPTED:
      SStrCopy(m_progressText, parsed, sizeof(m_progressText));
      break;
    case QUEST_REWARD:
      SStrCopy(m_rewardText, parsed, sizeof(m_rewardText));
      break;
  }

  m_currentQuest = quest;
  m_state = state;
  CGGameUI::SetInteractTarget(guid, MAX_SHOP_DISTANCE_SQUARED);
  m_npc = guid;
  m_lastChosenItem = 0;
}

void CGQuestInfo::SetLogDescription(const char *desc) {
  if (desc && *desc) {
    char             parsed[1024];
    unsigned __int64 player = ClntObjMgrGetActivePlayer();
    QuestParserParseText(desc, parsed, sizeof(parsed), player, 0);
    SStrCopy(m_questLogText, parsed, sizeof(m_questLogText));
  } else {
    m_questLogText[0] = 0;
  }
}

void CGQuestInfo::AddQuest(int quest, const char *desc, int questLevel, int turnIn) {
  FATALASSERT(m_state == QUEST_GREETING);
  FATALASSERT((m_numQuests + m_numInProgress) < 8);

  QuestInfo &info = m_quests[m_numQuests];
  info.id = quest;
  info.level = questLevel;
  if (desc) {
    SStrCopy(info.name, desc, sizeof(info.name));
  }
  info.turnIn = turnIn;
  ++m_numQuests;
}

void CGQuestInfo::AddQuestInProgress(int quest, const char *desc, int questLevel) {
  FATALASSERT(m_state == QUEST_GREETING);
  FATALASSERT((m_numQuests + m_numInProgress) < 7);

  QuestInfo &info = m_inProgress[m_numInProgress];
  info.id = quest;
  info.level = questLevel;
  if (desc) {
    SStrCopy(info.name, desc, sizeof(info.name));
  }
  info.turnIn = 0;
  ++m_numInProgress;
}

void CGQuestInfo::EndQuestList() {
  FrameScript_SignalEvent(277);
}

void CGQuestInfo::AddReward(
    const char *title,
    int        *itemChoice,
    int        *choiceDisplay,
    int        *choiceAmount,
    int         numChoice,
    int        *itemReward,
    int        *itemDisplay,
    int        *itemAmount,
    int         numReward,
    int         money,
    int         autoLaunched
) {
  FATALASSERT(numChoice <= 6);
  FATALASSERT(numReward <= 6);

  memset(m_questItems, 0, sizeof(m_questItems));
  int i;
  for (i = 0; i < numChoice; ++i) {
    m_questItems[i].choiceItemID = itemChoice[i];
    m_questItems[i].choiceDisplayID = choiceDisplay[i];
    m_questItems[i].choiceAmount = choiceAmount[i];
  }
  for (; i < 6; ++i) {
    m_questItems[i].choiceAmount = 1;
  }

  for (i = 0; i < numReward; ++i) {
    m_questItems[i].rewardItemID = itemReward[i];
    m_questItems[i].rewardDisplayID = itemDisplay[i];
    m_questItems[i].rewardAmount = itemAmount[i];
  }
  for (; i < 6; ++i) {
    m_questItems[i].rewardAmount = 1;
  }

  if (title) {
    SStrCopy(m_questTitle, *title ? title : " ", sizeof(m_questTitle));
  }
  m_rewardMoney = money;
  m_autoLaunched = autoLaunched;
  FrameScript_SignalEvent(m_state == QUEST_OFFER ? 278 : 280);
}

void CGQuestInfo::AddItemRequest(
    const char *title,
    int        *items,
    int        *itemAmount,
    int        *itemDisplay,
    int         numItems,
    int         completed,
    int         autoLaunched
) {
  FATALASSERT(numItems <= 6);

  int i;
  for (i = 0; i < numItems; ++i) {
    m_questItems[i].requiredItemID = items[i];
    m_questItems[i].requiredDisplayID = itemDisplay[i];
    m_questItems[i].requiredAmount = itemAmount[i];
  }
  for (; i < 6; ++i) {
    m_questItems[i].requiredItemID = 0;
    m_questItems[i].requiredDisplayID = 0;
    m_questItems[i].requiredAmount = 1;
  }

  if (title) {
    SStrCopy(m_questTitle, *title ? title : " ", sizeof(m_questTitle));
  }
  m_completable = completed;
  m_autoLaunched = autoLaunched;
  FrameScript_SignalEvent(279);
}

void CGQuestInfo::QuestGiverFinished() {
  if (m_npc) {
    FrameScript_SignalEvent(281);
    CGGameUI::ClearInteractTarget(m_npc);
    m_npc = 0;
  }
}

int CGQuestInfo::IsCompletable() {
  if (!m_completable) {
    return 0;
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return 0;
  }
  unsigned int index;
  for (index = 0; index < 6 && m_questItems[index].requiredItemID; ++index) {
    if (player->GetBag()->GetItemTypeCount(m_questItems[index].requiredItemID, 0) < static_cast<unsigned int>(m_questItems[index].requiredAmount)) {
      return 0;
    }
  }
  return 1;
}

void CGQuestInfo::QueryQuest(unsigned int index) {
  if (index >= m_numQuests) {
    return;
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    if (m_quests[index].turnIn) {
      player->CompleteQuest(m_npc, m_quests[index].id);
    } else {
      player->QueryQuest(m_npc, m_quests[index].id);
    }
  }
}

void CGQuestInfo::CompleteQuest(unsigned int index) {
  if (index >= m_numInProgress) {
    return;
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->CompleteQuest(m_npc, m_inProgress[index].id);
  }
}

void CGQuestInfo::AcceptQuest() {
  if (m_state != QUEST_OFFER) {
    return;
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->AcceptQuest(m_npc, m_currentQuest);
    QuestGiverFinished();
  }
}

void CGQuestInfo::DeclineQuest() {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player && m_npc) {
    player->CancelQuest(m_npc);
  }
  QuestGiverFinished();
}

void CGQuestInfo::GiveQuestItems() {
  if (m_state != QUEST_ACCEPTED) {
    return;
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->GiveQuestItems(m_npc, m_currentQuest);
  }
}

int CGQuestInfo::GetReward(int choice) {
  if (m_state != QUEST_REWARD) {
    return 1;
  }
  unsigned int numChoices = GetNumQuestChoices();
  if (numChoices && (choice < 0 || static_cast<unsigned int>(choice) >= numChoices)) {
    return 0;
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return 1;
  }
  if (numChoices) {
    m_lastChosenItem = m_questItems[choice].choiceItemID;
  }
  player->GetQuestReward(m_npc, m_currentQuest, choice > 0 ? choice : 0);
  return 1;
}

unsigned int CGQuestInfo::GetNumQuestRewards() {
  unsigned int index;
  for (index = 0; index < 6 && m_questItems[index].rewardItemID; ++index) {
  }
  return index;
}

unsigned int CGQuestInfo::GetNumQuestChoices() {
  unsigned int index;
  for (index = 0; index < 6 && m_questItems[index].choiceItemID; ++index) {
  }
  return index;
}

unsigned int CGQuestInfo::GetNumQuestItems() {
  unsigned int index;
  for (index = 0; index < 6 && m_questItems[index].requiredItemID; ++index) {
  }
  return index;
}

int CGQuestInfo::GetQuestItemInfo(
    const char   *type,
    unsigned int  index,
    char         *name,
    unsigned int  nameSize,
    char         *texture,
    unsigned int  textureSize,
    unsigned int &amount,
    int          &quality,
    int          &usable
) {
  name[0] = 0;
  texture[0] = 0;
  amount = 1;
  quality = 0;
  usable = 1;
  if (index > 6) {
    return 0;
  }
  int itemID = 0;
  int displayID = 0;
  if (!SStrCmpI(type, "reward", 0x7FFFFFFF)) {
    itemID = m_questItems[index].rewardItemID;
    displayID = m_questItems[index].rewardDisplayID;
    amount = m_questItems[index].rewardAmount;
  } else if (!SStrCmpI(type, "choice", 0x7FFFFFFF)) {
    itemID = m_questItems[index].choiceItemID;
    displayID = m_questItems[index].choiceDisplayID;
    amount = m_questItems[index].choiceAmount;
  } else if (!SStrCmpI(type, "required", 0x7FFFFFFF)) {
    itemID = m_questItems[index].requiredItemID;
    displayID = m_questItems[index].requiredDisplayID;
    amount = m_questItems[index].requiredAmount;
  } else {
    return 0;
  }
  const ItemStats_C *stats = itemID ? g_itemDBCache.GetRecord(itemID, m_npc, reinterpret_cast<DBCACHECALLBACKPROC>(QuestItemStatsCallback), 0) : 0;
  if (stats) {
    SStrCopy(name, stats->m_displayName[0], nameSize);
    quality = stats->m_inventoryType ? stats->m_overallQualityID : -1;
    CGPlayer_C     *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    GAME_ERROR_TYPE reason;
    if (player && !player->CanUseItem(stats, reason)) {
      usable = 0;
    }
  }
  const char *path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
  SStrPrintf(texture, textureSize, "%s%s", path, *path ? "\\" : "");
  SStrPack(texture, CGItem_C::GetInventoryArt(displayID), textureSize);
  return 1;
}

int CGQuestInfo::GetQuestItemID(const char *type, unsigned int index) {
  if (index >= 6) {
    return 0;
  }
  if (!SStrCmpI(type, "reward", 0x7FFFFFFF)) {
    return m_questItems[index].rewardItemID;
  }
  if (!SStrCmpI(type, "choice", 0x7FFFFFFF)) {
    return m_questItems[index].choiceItemID;
  }
  if (!SStrCmpI(type, "required", 0x7FFFFFFF)) {
    return m_questItems[index].requiredItemID;
  }
  return 0;
}

void CGQuestInfo::ConfirmAcceptQuest(int questID, const char *questTitle, const unsigned __int64 &initiatedBy) {
  m_pendingQuest = questID;
  const NameCache *nc = g_nameDBCache.GetRecord(initiatedBy, initiatedBy, 0, 0);
  FATALASSERT(nc);
  FrameScript_SignalEvent(325, "%s%s", nc->m_name, questTitle);
}

static int Script_CloseQuest(lua_State *__formal) {
  CGQuestInfo::QuestGiverFinished();
  return 0;
}

static int Script_GetTitleText(lua_State *L) {
  lua_pushstring(L, CGQuestInfo::GetTitleText());
  return 1;
}

static int Script_GetGreetingText(lua_State *L) {
  lua_pushstring(L, CGQuestInfo::GetGreetingText());
  return 1;
}

static int Script_GetQuestText(lua_State *L) {
  lua_pushstring(L, CGQuestInfo::GetQuestText());
  return 1;
}

static int Script_GetObjectiveText(lua_State *L) {
  lua_pushstring(L, CGQuestInfo::GetQuestLogText());
  return 1;
}

static int Script_GetProgressText(lua_State *L) {
  lua_pushstring(L, CGQuestInfo::GetProgressText());
  return 1;
}

static int Script_GetRewardText(lua_State *L) {
  lua_pushstring(L, CGQuestInfo::GetRewardText());
  return 1;
}

static int Script_GetNumAvailableQuests(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGQuestInfo::GetNumQuests()));
  return 1;
}

static int Script_GetNumActiveQuests(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGQuestInfo::GetNumInProgress()));
  return 1;
}

static int Script_GetAvailableTitle(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetAvailableTitle(index)");
  }
  lua_pushstring(L, CGQuestInfo::GetQuestName(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1));
  return 1;
}

static int Script_GetActiveTitle(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetActiveTitle(index)");
  }
  lua_pushstring(L, CGQuestInfo::GetInProgressName(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1));
  return 1;
}

static int Script_GetAvailableLevel(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetGetAvailableLevel(index)");
  }
  lua_pushnumber(L, static_cast<double>(CGQuestInfo::GetQuestLevel(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1)));
  return 1;
}

static int Script_GetActiveLevel(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetGetActiveLevel(index)");
  }
  lua_pushnumber(L, static_cast<double>(CGQuestInfo::GetInProgressLevel(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1)));
  return 1;
}

static int Script_SelectAvailableQuest(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: SelectAvailableQuest(index)");
  }
  CGQuestInfo::QueryQuest(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  return 0;
}

static int Script_SelectActiveQuest(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: SelectActiveQuest(index)");
  }
  CGQuestInfo::CompleteQuest(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  return 0;
}

static int Script_AcceptQuest(lua_State *__formal) {
  CGQuestInfo::AcceptQuest();
  return 0;
}

static int Script_DeclineQuest(lua_State *__formal) {
  CGQuestInfo::DeclineQuest();
  return 0;
}

static int Script_IsQuestCompletable(lua_State *L) {
  if (CGQuestInfo::IsCompletable()) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_CompleteQuest(lua_State *__formal) {
  CGQuestInfo::GiveQuestItems();
  return 0;
}

static int Script_GetQuestReward(lua_State *L) {
  int choice = lua_isnumber(L, 1) ? static_cast<int>(lua_tonumber(L, 1)) - 1 : 0;
  if (!CGQuestInfo::GetReward(choice)) {
    return luaL_error(L, "Invalid reward choice in GetQuestReward");
  }
  return 0;
}

static int Script_GetRewardMoney(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGQuestInfo::GetRewardMoney()));
  return 1;
}

static int Script_GetNumQuestRewards(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGQuestInfo::GetNumQuestRewards()));
  return 1;
}

static int Script_GetNumQuestChoices(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGQuestInfo::GetNumQuestChoices()));
  return 1;
}

static int Script_GetNumQuestItems(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGQuestInfo::GetNumQuestItems()));
  return 1;
}

static int Script_GetQuestItemInfo(lua_State *L) {
  if (!lua_isstring(L, 1) || !lua_isnumber(L, 2)) {
    return luaL_error(L, "Invalid quest item in GetQuestItemInfo(\"type\", index)");
  }
  char         texture[260];
  char         name[256];
  int          quality;
  int          usable;
  unsigned int amount;
  if (!CGQuestInfo::GetQuestItemInfo(
          lua_tostring(L, 1), static_cast<unsigned int>(lua_tonumber(L, 2)) - 1, name, sizeof(name), texture, sizeof(texture), amount, quality, usable
      ))
  {
    return luaL_error(L, "Invalid quest item in GetQuestItemInfo(\"type\", index)");
  }
  lua_pushstring(L, name);
  lua_pushstring(L, texture);
  lua_pushnumber(L, static_cast<double>(amount));
  lua_pushnumber(L, static_cast<double>(quality));
  if (usable) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 5;
}

static int Script_QuestChooseRewardError(lua_State *__formal) {
  CGGameUI::DisplayError(GERR_QUEST_MUST_CHOOSE);
  return 0;
}

static int Script_ConfirmAcceptQuest(lua_State *__formal) {
  int quest = CGQuestInfo::GetPendingConfirmQuest();
  if (quest) {
    CDataStore msg;
    msg.Put(static_cast<unsigned int>(CMSG_QUEST_CONFIRM_ACCEPT));
    msg.Put(quest);
    msg.Finalize();
    ClientServices_Send(&msg);
  }
  return 0;
}

static int Script_GetQuestBackgroundMaterial(lua_State *L) {
  CGObject_C *object = ClntObjMgrObjectPtr(CGQuestInfo::GetQuestGiver(), __FILE__, __LINE__);
  int material = 0;
  if (object) {
    if (object->GetType() & TYPE_ITEM) {
      const unsigned __int64 noGuid = 0;
      const ItemStats_C *stats = g_itemDBCache.GetRecord(object->GetEntryID(), noGuid, 0, 0);
      if (stats) {
        material = stats->m_pageMaterial;
      }
    } else if (object->GetType() & TYPE_GAMEOBJECT) {
      material = static_cast<CGGameObject_C *>(object)->GetPageTextMaterial();
    }
  }

  const PageTextMaterialRec *rec = material > 0 ? g_pageTextMaterialDB.GetRecord(material) : 0;
  lua_pushstring(L, rec ? rec->m_name : 0);
  return 1;
}

static FrameScript_Method s_ScriptFunctions[28] = {
    {                "CloseQuest",                 Script_CloseQuest},
    {              "GetTitleText",               Script_GetTitleText},
    {           "GetGreetingText",            Script_GetGreetingText},
    {              "GetQuestText",               Script_GetQuestText},
    {          "GetObjectiveText",           Script_GetObjectiveText},
    {           "GetProgressText",            Script_GetProgressText},
    {             "GetRewardText",              Script_GetRewardText},
    {     "GetNumAvailableQuests",      Script_GetNumAvailableQuests},
    {        "GetNumActiveQuests",         Script_GetNumActiveQuests},
    {         "GetAvailableTitle",          Script_GetAvailableTitle},
    {            "GetActiveTitle",             Script_GetActiveTitle},
    {         "GetAvailableLevel",          Script_GetAvailableLevel},
    {            "GetActiveLevel",             Script_GetActiveLevel},
    {      "SelectAvailableQuest",       Script_SelectAvailableQuest},
    {         "SelectActiveQuest",          Script_SelectActiveQuest},
    {               "AcceptQuest",                Script_AcceptQuest},
    {              "DeclineQuest",               Script_DeclineQuest},
    {        "IsQuestCompletable",         Script_IsQuestCompletable},
    {             "CompleteQuest",              Script_CompleteQuest},
    {            "GetQuestReward",             Script_GetQuestReward},
    {            "GetRewardMoney",             Script_GetRewardMoney},
    {        "GetNumQuestRewards",         Script_GetNumQuestRewards},
    {        "GetNumQuestChoices",         Script_GetNumQuestChoices},
    {          "GetNumQuestItems",           Script_GetNumQuestItems},
    {          "GetQuestItemInfo",           Script_GetQuestItemInfo},
    {    "QuestChooseRewardError",     Script_QuestChooseRewardError},
    {        "ConfirmAcceptQuest",         Script_ConfirmAcceptQuest},
    {"GetQuestBackgroundMaterial", Script_GetQuestBackgroundMaterial}
};

void QuestInfoRegisterScriptFunctions() {
  unsigned int index;
  for (index = 0; index < 28; ++index) {
    FrameScript_RegisterFunction(s_ScriptFunctions[index].name, s_ScriptFunctions[index].method);
  }
}

void QuestInfoUnregisterScriptFunctions() {
  unsigned int index;
  for (index = 0; index < 28; ++index) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[index].name);
  }
}
