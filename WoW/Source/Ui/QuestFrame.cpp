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

#define MAXIMUM_NUM_QUESTS_DISPLAYED 8

bool QuestParserParseText(LPCSTR text, char *buf, UINT size, const DWORDLONG &target, int restoreToken);

static void QuestItemStatsCallback(int id, const DWORDLONG &guid, LPVOID arg, bool granted) {
  if (granted && g_itemDBCache.GetRecord(id, 0, 0, 0)) {
    FrameScript_SignalEvent(282);
  }
}

DWORDLONG     CGQuestInfo::m_npc;
QUEST_STATE   CGQuestInfo::m_state;
int           CGQuestInfo::m_currentQuest;
int           CGQuestInfo::m_completable;
int           CGQuestInfo::m_autoLaunched;
int           CGQuestInfo::m_lastChosenItem;
int           CGQuestInfo::m_rewardMoney;
UINT          CGQuestInfo::m_numQuests;
UINT          CGQuestInfo::m_numInProgress;
QuestInfo     CGQuestInfo::m_quests[8];
QuestInfo     CGQuestInfo::m_inProgress[8];
QuestItemInfo CGQuestInfo::m_questItems[6];
char          CGQuestInfo::m_greetingText[256];
char          CGQuestInfo::m_questTitle[64];
char          CGQuestInfo::m_questText[1024];
char          CGQuestInfo::m_questLogText[1024];
char          CGQuestInfo::m_progressText[1024];
char          CGQuestInfo::m_rewardText[1024];
int           CGQuestInfo::m_pendingQuest;

void CGQuestInfo::EnterWorld() {
  m_npc = 0;
  m_state = QUEST_GREETING;
  m_currentQuest = 0;
  m_completable = 0;
  m_autoLaunched = 0;
  m_lastChosenItem = 0;
  ClearQuests();
  m_greetingText[0] = 0;
  m_questTitle[0] = 0;
  m_questText[0] = 0;
  m_questLogText[0] = 0;
  m_progressText[0] = 0;
  m_rewardText[0] = 0;
  ClearItems();
}

void CGQuestInfo::LeaveWorld() {
  QuestGiverFinished();
}

void CGQuestInfo::SetState(DWORDLONG guid, QUEST_STATE state, LPCSTR text, int quest) {
  FATALASSERT(state < QUEST_STATE_NUM_TYPES);

  CGObject_C *object = ClntObjMgrObjectPtr(guid, __FILE__, __LINE__);
  if (!object || !(object->IsA(ID_UNIT) || object->IsA(ID_GAMEOBJECT) || object->IsA(ID_ITEM))) {
    return;
  }

  if (state == QUEST_GREETING) {
    ClearQuests();
    m_greetingText[0] = 0;
    m_questTitle[0] = 0;
    m_questText[0] = 0;
    m_questLogText[0] = 0;
    m_progressText[0] = 0;
    m_rewardText[0] = 0;
  }

  char parsed[1024];
  QuestParserParseText(text, parsed, sizeof(parsed), ClntObjMgrGetActivePlayer(), 0);
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

void CGQuestInfo::SetLogDescription(LPCSTR desc) {
  if (desc && *desc) {
    char parsed[1024];
    QuestParserParseText(desc, parsed, sizeof(parsed), ClntObjMgrGetActivePlayer(), 0);
    SStrCopy(m_questLogText, parsed, sizeof(m_questLogText));
  } else {
    m_questLogText[0] = 0;
  }
}

void CGQuestInfo::AddQuest(int quest, LPCSTR desc, int questLevel, int turnIn) {
  FATALASSERT(m_state == QUEST_GREETING);
  FATALASSERT((m_numQuests + m_numInProgress) < (MAXIMUM_NUM_QUESTS_DISPLAYED));

  m_quests[m_numQuests].id = quest;
  m_quests[m_numQuests].level = questLevel;
  if (desc) {
    SStrCopy(m_quests[m_numQuests].name, desc, sizeof(m_quests[m_numQuests].name));
  }
  m_quests[m_numQuests].turnIn = turnIn;
  ++m_numQuests;
}

void CGQuestInfo::AddQuestInProgress(int quest, LPCSTR desc, int questLevel) {
  FATALASSERT(m_state == QUEST_GREETING);
  FATALASSERT((m_numQuests + m_numInProgress) < (MAXIMUM_NUM_QUESTS_DISPLAYED - 1));

  m_inProgress[m_numInProgress].id = quest;
  m_inProgress[m_numInProgress].level = questLevel;
  if (desc) {
    SStrCopy(m_inProgress[m_numInProgress].name, desc, sizeof(m_inProgress[m_numInProgress].name));
  }
  m_inProgress[m_numInProgress].turnIn = 0;
  ++m_numInProgress;
}

void CGQuestInfo::EndQuestList() {
  FrameScript_SignalEvent(277);
}

void CGQuestInfo::AddReward(
    LPCSTR title,
    int    itemChoice[],
    int    choiceDisplay[],
    int    choiceAmount[],
    int    numChoice,
    int    itemReward[],
    int    itemDisplay[],
    int    itemAmount[],
    int    numReward,
    int    money,
    int    autoLaunched
) {
  FATALASSERT(numChoice <= 6);
  FATALASSERT(numReward <= 6);

  ClearItems();
  int i;
  for (i = 0; i < numChoice; ++i) {
    m_questItems[i].choiceItemID = itemChoice[i];
    m_questItems[i].choiceDisplayID = choiceDisplay[i];
    m_questItems[i].choiceAmount = choiceAmount[i];
  }
  for (; i < 6; ++i) {
    m_questItems[i].choiceItemID = 0;
    m_questItems[i].choiceDisplayID = 0;
    m_questItems[i].choiceAmount = 1;
  }

  for (i = 0; i < numReward; ++i) {
    m_questItems[i].rewardItemID = itemReward[i];
    m_questItems[i].rewardDisplayID = itemDisplay[i];
    m_questItems[i].rewardAmount = itemAmount[i];
  }
  for (; i < 6; ++i) {
    m_questItems[i].rewardItemID = 0;
    m_questItems[i].rewardDisplayID = 0;
    m_questItems[i].rewardAmount = 1;
  }

  if (title) {
    if (*title) {
      SStrCopy(m_questTitle, title, sizeof(m_questTitle));
    } else {
      SStrCopy(m_questTitle, " ", sizeof(m_questTitle));
    }
  }
  m_rewardMoney = money;
  m_autoLaunched = autoLaunched;
  if (m_state == QUEST_OFFER) {
    FrameScript_SignalEvent(278);
  } else {
    FrameScript_SignalEvent(280);
  }
}

void CGQuestInfo::AddItemRequest(LPCSTR title, int items[], int itemAmount[], int itemDisplay[], int numItems, int completed, int autoLaunched) {
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
    if (*title) {
      SStrCopy(m_questTitle, title, sizeof(m_questTitle));
    } else {
      SStrCopy(m_questTitle, " ", sizeof(m_questTitle));
    }
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

BOOL CGQuestInfo::IsCompletable() {
  if (!m_completable) {
    return 0;
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return 0;
  }
  UINT index;
  for (index = 0; index < 6 && m_questItems[index].requiredItemID; ++index) {
    if (player->CGPlayer_C::GetBag()->GetItemTypeCount(m_questItems[index].requiredItemID, 8) < m_questItems[index].requiredAmount) {
      return 0;
    }
  }
  return 1;
}

void CGQuestInfo::QueryQuest(UINT index) {
  if (index > m_numQuests) {
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

void CGQuestInfo::CompleteQuest(UINT index) {
  if (index > m_numInProgress) {
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
  CGObject_C *object = ClntObjMgrObjectPtr(m_npc, __FILE__, __LINE__);
  if ((object && object->IsA(ID_ITEM)) || m_autoLaunched) {
    QuestGiverFinished();
  } else {
    CDataStore hello;
    hello.Put(CMSG_QUESTGIVER_HELLO);
    hello.Put(m_npc);
    hello.Finalize();
    ClientServices_Send(&hello);
  }
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

BOOL CGQuestInfo::GetReward(int choice) {
  if (m_state == QUEST_REWARD) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (player) {
      int numChoices = GetNumQuestChoices();
      if (numChoices > 0) {
        if (choice >= numChoices || choice < 0) {
          return 0;
        }
        m_lastChosenItem = m_questItems[choice].choiceItemID;
      }
      player->GetQuestReward(m_npc, m_currentQuest, choice > 0 ? choice : 0);
    }
  }
  return 1;
}

UINT CGQuestInfo::GetNumQuestRewards() {
  UINT index;
  for (index = 0; index < 6 && m_questItems[index].rewardItemID; ++index) {
  }
  return index;
}

UINT CGQuestInfo::GetNumQuestChoices() {
  UINT index;
  for (index = 0; index < 6 && m_questItems[index].choiceItemID; ++index) {
  }
  return index;
}

UINT CGQuestInfo::GetNumQuestItems() {
  UINT index;
  for (index = 0; index < 6 && m_questItems[index].requiredDisplayID; ++index) {
  }
  return index;
}

int CGQuestInfo::GetQuestItemInfo(LPCSTR type, UINT index, char name[], UINT nameSize, char texture[], UINT textureSize, UINT &amount, int &quality, int &usable) {
  static char buffer[MAX_PATH];

  name[0] = 0;
  texture[0] = 0;
  amount = 1;
  quality = 0;
  usable = 1;
  if (index > 6) {
    return 0;
  }
  int itemID;
  int displayID;
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
  const ItemStats_C *stats = g_itemDBCache.GetRecord(itemID, m_npc, reinterpret_cast<DBCACHECALLBACKPROC>(QuestItemStatsCallback), 0);
  if (stats) {
    SStrCopy(name, stats->m_displayName[0], nameSize);
    quality = stats->m_inventoryType ? stats->m_overallQualityID : -1;
  }
  LPCSTR path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
  SStrPrintf(buffer, sizeof(buffer), "%s%s", path, *path ? "\\" : "");
  SStrPack(buffer, CGItem_C::GetInventoryArt(displayID), sizeof(buffer));
  SStrCopy(texture, buffer, textureSize);
  CGPlayer_C     *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  GAME_ERROR_TYPE reason;
  if (player && stats && !player->CanUseItem(stats, reason)) {
    usable = 0;
  }
  return 1;
}

int CGQuestInfo::GetQuestItemID(LPCSTR type, UINT index) {
  if (index > 6) {
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

void CGQuestInfo::ConfirmAcceptQuest(int questID, LPCSTR questTitle, const DWORDLONG &initiatedBy) {
  m_pendingQuest = questID;
  const NameCache *nc = g_nameDBCache.GetRecord(initiatedBy, 0, 0, 0);
  if (!nc) {
    FATALASSERT(nc);
    return;
  }
  FrameScript_SignalEvent(325, "%s%s", nc->m_name, questTitle);
}

static int Script_CloseQuest(lua_State *) {
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
  if (lua_isnumber(L, 1)) {
    lua_pushstring(L, CGQuestInfo::GetQuestName(static_cast<int>(lua_tonumber(L, 1)) - 1));
    return 1;
  }
  luaL_error(L, "Usage: GetAvailableTitle(index)");
  return 0;
}

static int Script_GetActiveTitle(lua_State *L) {
  if (lua_isnumber(L, 1)) {
    lua_pushstring(L, CGQuestInfo::GetInProgressName(static_cast<int>(lua_tonumber(L, 1)) - 1));
    return 1;
  }
  luaL_error(L, "Usage: GetActiveTitle(index)");
  return 0;
}

static int Script_GetAvailableLevel(lua_State *L) {
  if (lua_isnumber(L, 1)) {
    lua_pushnumber(L, static_cast<double>(CGQuestInfo::GetQuestLevel(static_cast<int>(lua_tonumber(L, 1)) - 1)));
    return 1;
  }
  luaL_error(L, "Usage: GetGetAvailableLevel(index)");
  return 0;
}

static int Script_GetActiveLevel(lua_State *L) {
  if (lua_isnumber(L, 1)) {
    lua_pushnumber(L, static_cast<double>(CGQuestInfo::GetInProgressLevel(static_cast<int>(lua_tonumber(L, 1)) - 1)));
    return 1;
  }
  luaL_error(L, "Usage: GetGetActiveLevel(index)");
  return 0;
}

static int Script_SelectAvailableQuest(lua_State *L) {
  if (lua_isnumber(L, 1)) {
    CGQuestInfo::QueryQuest(static_cast<int>(lua_tonumber(L, 1)) - 1);
    return 0;
  }
  luaL_error(L, "Usage: SelectAvailableQuest(index)");
  return 0;
}

static int Script_SelectActiveQuest(lua_State *L) {
  if (lua_isnumber(L, 1)) {
    CGQuestInfo::CompleteQuest(static_cast<int>(lua_tonumber(L, 1)) - 1);
    return 0;
  }
  luaL_error(L, "Usage: SelectActiveQuest(index)");
  return 0;
}

static int Script_AcceptQuest(lua_State *) {
  CGQuestInfo::AcceptQuest();
  return 0;
}

static int Script_DeclineQuest(lua_State *) {
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

static int Script_CompleteQuest(lua_State *) {
  CGQuestInfo::GiveQuestItems();
  return 0;
}

static int Script_GetQuestReward(lua_State *L) {
  int choice = -1;
  if (lua_isnumber(L, 1)) {
    choice = static_cast<int>(lua_tonumber(L, 1)) - 1;
  }
  if (!CGQuestInfo::GetReward(choice)) {
    luaL_error(L, "Invalid reward choice in GetQuestReward([choice])");
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
  if (lua_isstring(L, 1) && lua_isnumber(L, 2)) {
    char texture[MAX_PATH];
    char name[256];
    int  usable;
    UINT amount;
    int  quality;
    if (CGQuestInfo::GetQuestItemInfo(
            lua_tostring(L, 1), static_cast<int>(lua_tonumber(L, 2)) - 1, name, sizeof(name), texture, sizeof(texture), amount, quality, usable
        ))
    {
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
  }
  luaL_error(L, "Invalid quest item in GetQuestItemInfo(\"type\", index)");
  return 0;
}

static int Script_QuestChooseRewardError(lua_State *) {
  CGGameUI::DisplayError(GERR_QUEST_MUST_CHOOSE);
  return 0;
}

static int Script_ConfirmAcceptQuest(lua_State *) {
  CDataStore msg;
  msg.Put(CMSG_QUEST_CONFIRM_ACCEPT);
  msg.Put(CGQuestInfo::GetPendingConfirmQuest());
  msg.Finalize();
  ClientServices_Send(&msg);
  return 0;
}

static int Script_GetQuestBackgroundMaterial(lua_State *L) {
  CGObject_C *object = ClntObjMgrObjectPtr(CGQuestInfo::GetQuestGiver(), __FILE__, __LINE__);
  if (object) {
    int material = 0;
    if (object->IsA(ID_ITEM)) {
      const ItemStats_C *stats = g_itemDBCache.GetRecord(object->GetEntryID(), 0, 0, 0);
      if (stats) {
        material = stats->m_pageMaterial;
      }
    } else if (object->IsA(ID_GAMEOBJECT)) {
      material = static_cast<CGGameObject_C *>(object)->GetPageTextMaterial();
    }
    if (material > 0) {
      const PageTextMaterialRec *rec = g_pageTextMaterialDB.GetRecord(material);
      if (rec) {
        lua_pushstring(L, rec->m_name);
        return 1;
      }
    }
  }
  lua_pushnil(L);
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
  UINT index;
  for (index = 0; index < 28; ++index) {
    FrameScript_RegisterFunction(s_ScriptFunctions[index].name, s_ScriptFunctions[index].method);
  }
}

void QuestInfoUnregisterScriptFunctions() {
  UINT index;
  for (index = 0; index < 28; ++index) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[index].name);
  }
}
