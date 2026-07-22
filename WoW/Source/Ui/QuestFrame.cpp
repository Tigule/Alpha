#include "GameUI.h"

#include "DB/DBClient/DBCacheInstances.h"
#include "DB/DBClient/DBClient.h"
#include "Game/GameClient/NameCache.h"
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

enum QUEST_STATE {
  QUEST_GREETING = 0,
  QUEST_DETAIL = 1,
  QUEST_PROGRESS = 2,
  QUEST_REWARD = 3,
  QUEST_STATE_NUM_TYPES = 4
};

struct QuestInfo {
  int  quest;
  int  questLevel;
  char desc[64];
  int  turnIn;
};

struct QuestItemInfo {
  int itemReward;
  int rewardDisplay;
  int rewardAmount;
  int itemChoice;
  int choiceDisplay;
  int choiceAmount;
  int itemRequest;
  int requestDisplay;
  int requestAmount;
};

bool __fastcall QuestParserParseText(const char *text, char *buf, unsigned int size, const unsigned __int64 &target, int restoreToken);

static void __fastcall QuestItemStatsCallback(int, const unsigned __int64 &, void *, bool granted) {
  if (granted) {
    FrameScript_SignalEvent(280);
  }
}

class CGQuestInfo {
 public:
  static void __fastcall EnterWorld();
  static void __fastcall LeaveWorld();
  static void __fastcall SetState(unsigned __int64 guid, QUEST_STATE state, const char *text, int quest);
  static void __fastcall SetLogDescription(const char *desc);
  static void __fastcall AddQuest(int quest, const char *desc, int questLevel, int turnIn);
  static void __fastcall AddQuestInProgress(int quest, const char *desc, int questLevel);
  static void __fastcall EndQuestList();
  static void __fastcall AddReward(
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
  );
  static void __fastcall
  AddItemRequest(const char *title, int *items, int *itemAmount, int *itemDisplay, int numItems, int completed, int autoLaunched);
  static void __fastcall                    QuestGiverFinished();
  static const unsigned __int64 &__fastcall GetQuestGiver();
  static int __fastcall                     GetCurrentQuest() {
    return m_currentQuest;
  }
  static int __fastcall         IsCompletable();
  static int __fastcall         GetLastChosenItem();
  static void __fastcall        ClearLastChosenItem();
  static const char *__fastcall GetTitleText() {
    return m_questTitle;
  }
  static const char *__fastcall GetGreetingText() {
    return m_greetingText;
  }
  static const char *__fastcall GetQuestText() {
    return m_questText;
  }
  static const char *__fastcall GetQuestLogText() {
    return m_questLogText;
  }
  static const char *__fastcall GetProgressText() {
    return m_progressText;
  }
  static const char *__fastcall GetRewardText() {
    return m_rewardText;
  }
  static void __fastcall QueryQuest(unsigned int index);
  static void __fastcall CompleteQuest(unsigned int index);
  static void __fastcall AcceptQuest();
  static void __fastcall DeclineQuest();
  static void __fastcall GiveQuestItems();
  static int __fastcall  GetReward(int choice);
  static int __fastcall  GetRewardMoney() {
    return m_rewardMoney;
  }
  static unsigned int __fastcall GetNumQuestRewards();
  static unsigned int __fastcall GetNumQuestChoices();
  static unsigned int __fastcall GetNumQuestItems();
  static int __fastcall          GetQuestItemInfo(
      const char   *type,
      unsigned int  index,
      char         *name,
      unsigned int  nameSize,
      char         *texture,
      unsigned int  textureSize,
      unsigned int &amount,
      int          &quality,
      int          &usable
  );
  static int __fastcall  GetQuestItemID(const char *type, unsigned int index);
  static void __fastcall ConfirmAcceptQuest(int questID, const char *questTitle, const unsigned __int64 &initiatedBy);
  static int __fastcall  GetPendingConfirmQuest() {
    return m_pendingQuest;
  }
  static int __fastcall GetNumQuests() {
    return m_numQuests;
  }
  static int __fastcall GetNumInProgress() {
    return m_numInProgress;
  }
  static const char *__fastcall GetQuestName(unsigned int index) {
    return index < m_numQuests ? m_quests[index].desc : 0;
  }
  static const char *__fastcall GetInProgressName(unsigned int index) {
    return index < m_numInProgress ? m_inProgress[index].desc : 0;
  }
  static int __fastcall GetQuestLevel(unsigned int index) {
    return index < m_numQuests ? m_quests[index].questLevel : 0;
  }
  static int __fastcall GetInProgressLevel(unsigned int index) {
    return index < m_numInProgress ? m_inProgress[index].questLevel : 0;
  }

 protected:
  static void ClearQuests() {
    memset(m_quests, 0, sizeof(m_quests));
    memset(m_inProgress, 0, sizeof(m_inProgress));
    m_numQuests = 0;
    m_numInProgress = 0;
  }

  static void ClearItems() {
    memset(m_questItems, 0, sizeof(m_questItems));
    m_questTitle[0] = 0;
  }

  static unsigned __int64 m_npc;
  static QUEST_STATE      m_state;
  static int              m_currentQuest;
  static int              m_completable;
  static int              m_autoLaunched;
  static int              m_lastChosenItem;
  static int              m_rewardMoney;
  static unsigned int     m_numQuests;
  static unsigned int     m_numInProgress;
  static QuestInfo        m_quests[8];
  static QuestInfo        m_inProgress[8];
  static QuestItemInfo    m_questItems[6];
  static char             m_greetingText[256];
  static char             m_questTitle[64];
  static char             m_questText[1024];
  static char             m_questLogText[1024];
  static char             m_progressText[1024];
  static char             m_rewardText[1024];
  static int              m_pendingQuest;
};

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

void __fastcall CGQuestInfo::EnterWorld() {
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

void __fastcall CGQuestInfo::LeaveWorld() {
  QuestGiverFinished();
}

void __fastcall CGQuestInfo::SetState(unsigned __int64 guid, QUEST_STATE state, const char *text, int quest) {
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
    case QUEST_DETAIL:
      SStrCopy(m_questText, parsed, sizeof(m_questText));
      break;
    case QUEST_PROGRESS:
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

void __fastcall CGQuestInfo::SetLogDescription(const char *desc) {
  if (desc && *desc) {
    char             parsed[1024];
    unsigned __int64 player = ClntObjMgrGetActivePlayer();
    QuestParserParseText(desc, parsed, sizeof(parsed), player, 0);
    SStrCopy(m_questLogText, parsed, sizeof(m_questLogText));
  } else {
    m_questLogText[0] = 0;
  }
}

void __fastcall CGQuestInfo::AddQuest(int quest, const char *desc, int questLevel, int turnIn) {
  FATALASSERT(m_state == QUEST_GREETING);
  FATALASSERT((m_numQuests + m_numInProgress) < 8);

  QuestInfo &info = m_quests[m_numQuests];
  info.quest = quest;
  info.questLevel = questLevel;
  if (desc) {
    SStrCopy(info.desc, desc, sizeof(info.desc));
  }
  info.turnIn = turnIn;
  ++m_numQuests;
}

void __fastcall CGQuestInfo::AddQuestInProgress(int quest, const char *desc, int questLevel) {
  FATALASSERT(m_state == QUEST_GREETING);
  FATALASSERT((m_numQuests + m_numInProgress) < 7);

  QuestInfo &info = m_inProgress[m_numInProgress];
  info.quest = quest;
  info.questLevel = questLevel;
  if (desc) {
    SStrCopy(info.desc, desc, sizeof(info.desc));
  }
  info.turnIn = 0;
  ++m_numInProgress;
}

void __fastcall CGQuestInfo::EndQuestList() {
  FrameScript_SignalEvent(277);
}

void __fastcall CGQuestInfo::AddReward(
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
    m_questItems[i].itemChoice = itemChoice[i];
    m_questItems[i].choiceDisplay = choiceDisplay[i];
    m_questItems[i].choiceAmount = choiceAmount[i];
  }
  for (; i < 6; ++i) {
    m_questItems[i].choiceAmount = 1;
  }

  for (i = 0; i < numReward; ++i) {
    m_questItems[i].itemReward = itemReward[i];
    m_questItems[i].rewardDisplay = itemDisplay[i];
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
  FrameScript_SignalEvent(m_state == QUEST_DETAIL ? 278 : 280);
}

void __fastcall CGQuestInfo::AddItemRequest(
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
    m_questItems[i].itemRequest = items[i];
    m_questItems[i].requestDisplay = itemDisplay[i];
    m_questItems[i].requestAmount = itemAmount[i];
  }
  for (; i < 6; ++i) {
    m_questItems[i].itemRequest = 0;
    m_questItems[i].requestDisplay = 0;
    m_questItems[i].requestAmount = 1;
  }

  if (title) {
    SStrCopy(m_questTitle, *title ? title : " ", sizeof(m_questTitle));
  }
  m_completable = completed;
  m_autoLaunched = autoLaunched;
  FrameScript_SignalEvent(279);
}

void __fastcall CGQuestInfo::QuestGiverFinished() {
  if (m_npc) {
    FrameScript_SignalEvent(281);
    CGGameUI::ClearInteractTarget(m_npc);
    m_npc = 0;
  }
}

const unsigned __int64 &__fastcall CGQuestInfo::GetQuestGiver() {
  return m_npc;
}

int __fastcall CGQuestInfo::GetLastChosenItem() {
  return m_lastChosenItem;
}

void __fastcall CGQuestInfo::ClearLastChosenItem() {
  m_lastChosenItem = 0;
}

int __fastcall CGQuestInfo::IsCompletable() {
  if (!m_completable) {
    return 0;
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return 0;
  }
  unsigned int index;
  for (index = 0; index < 6 && m_questItems[index].itemRequest; ++index) {
    if (player->GetBag()->GetItemTypeCount(m_questItems[index].itemRequest, 0) < static_cast<unsigned int>(m_questItems[index].requestAmount)) {
      return 0;
    }
  }
  return 1;
}

void __fastcall CGQuestInfo::QueryQuest(unsigned int index) {
  if (index >= m_numQuests) {
    return;
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    if (m_quests[index].turnIn) {
      player->CompleteQuest(m_npc, m_quests[index].quest);
    } else {
      player->QueryQuest(m_npc, m_quests[index].quest);
    }
  }
}

void __fastcall CGQuestInfo::CompleteQuest(unsigned int index) {
  if (index >= m_numInProgress) {
    return;
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->CompleteQuest(m_npc, m_inProgress[index].quest);
  }
}

void __fastcall CGQuestInfo::AcceptQuest() {
  if (m_state != QUEST_DETAIL) {
    return;
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->AcceptQuest(m_npc, m_currentQuest);
    QuestGiverFinished();
  }
}

void __fastcall CGQuestInfo::DeclineQuest() {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player && m_npc) {
    player->CancelQuest(m_npc);
  }
  QuestGiverFinished();
}

void __fastcall CGQuestInfo::GiveQuestItems() {
  if (m_state != QUEST_PROGRESS) {
    return;
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->GiveQuestItems(m_npc, m_currentQuest);
  }
}

int __fastcall CGQuestInfo::GetReward(int choice) {
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
    m_lastChosenItem = m_questItems[choice].itemChoice;
  }
  player->GetQuestReward(m_npc, m_currentQuest, choice > 0 ? choice : 0);
  return 1;
}

unsigned int __fastcall CGQuestInfo::GetNumQuestRewards() {
  unsigned int index;
  for (index = 0; index < 6 && m_questItems[index].itemReward; ++index) {
  }
  return index;
}

unsigned int __fastcall CGQuestInfo::GetNumQuestChoices() {
  unsigned int index;
  for (index = 0; index < 6 && m_questItems[index].itemChoice; ++index) {
  }
  return index;
}

unsigned int __fastcall CGQuestInfo::GetNumQuestItems() {
  unsigned int index;
  for (index = 0; index < 6 && m_questItems[index].itemRequest; ++index) {
  }
  return index;
}

int __fastcall CGQuestInfo::GetQuestItemInfo(
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
  if (index >= 6) {
    return 0;
  }
  int itemID = 0;
  int displayID = 0;
  if (!SStrCmpI(type, "reward", 0x7FFFFFFF)) {
    itemID = m_questItems[index].itemReward;
    displayID = m_questItems[index].rewardDisplay;
    amount = m_questItems[index].rewardAmount;
  } else if (!SStrCmpI(type, "choice", 0x7FFFFFFF)) {
    itemID = m_questItems[index].itemChoice;
    displayID = m_questItems[index].choiceDisplay;
    amount = m_questItems[index].choiceAmount;
  } else if (!SStrCmpI(type, "required", 0x7FFFFFFF)) {
    itemID = m_questItems[index].itemRequest;
    displayID = m_questItems[index].requestDisplay;
    amount = m_questItems[index].requestAmount;
  } else {
    return 0;
  }
  const ItemStats_C *stats = itemID ? g_itemDBCache.GetRecord(itemID, m_npc, reinterpret_cast<DBCACHECALLBACKPROC>(QuestItemStatsCallback), 0) : 0;
  if (stats) {
    SStrCopy(name, stats->m_displayName[0], nameSize);
    quality = stats->m_overallQualityID;
    CGPlayer_C     *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    GAME_ERROR_TYPE reason;
    if (player && !player->CanUseItem(stats, reason)) {
      usable = 0;
    }
  }
  const char *path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
  SStrPrintf(texture, textureSize, "%s%s", path, *path ? "\\" : "");
  SStrPack(texture, CGItem_C::GetInventoryArt(displayID), textureSize);
  return itemID != 0;
}

int __fastcall CGQuestInfo::GetQuestItemID(const char *type, unsigned int index) {
  if (index >= 6) {
    return 0;
  }
  if (!SStrCmpI(type, "reward", 0x7FFFFFFF)) {
    return m_questItems[index].itemReward;
  }
  if (!SStrCmpI(type, "choice", 0x7FFFFFFF)) {
    return m_questItems[index].itemChoice;
  }
  if (!SStrCmpI(type, "required", 0x7FFFFFFF)) {
    return m_questItems[index].itemRequest;
  }
  return 0;
}

void __fastcall CGQuestInfo::ConfirmAcceptQuest(int questID, const char *questTitle, const unsigned __int64 &initiatedBy) {
  m_pendingQuest = questID;
  const NameCache *nc = g_nameDBCache.GetRecord(initiatedBy, initiatedBy, 0, 0);
  FATALASSERT(nc);
  FrameScript_SignalEvent(325, "%s%s", nc->m_name, questTitle);
}

static int __fastcall Script_CloseQuest(lua_State *__formal) {
  CGQuestInfo::QuestGiverFinished();
  return 0;
}

static int __fastcall Script_GetTitleText(lua_State *L) {
  lua_pushstring(L, CGQuestInfo::GetTitleText());
  return 1;
}

static int __fastcall Script_GetGreetingText(lua_State *L) {
  lua_pushstring(L, CGQuestInfo::GetGreetingText());
  return 1;
}

static int __fastcall Script_GetQuestText(lua_State *L) {
  lua_pushstring(L, CGQuestInfo::GetQuestText());
  return 1;
}

static int __fastcall Script_GetObjectiveText(lua_State *L) {
  lua_pushstring(L, CGQuestInfo::GetQuestLogText());
  return 1;
}

static int __fastcall Script_GetProgressText(lua_State *L) {
  lua_pushstring(L, CGQuestInfo::GetProgressText());
  return 1;
}

static int __fastcall Script_GetRewardText(lua_State *L) {
  lua_pushstring(L, CGQuestInfo::GetRewardText());
  return 1;
}

static int __fastcall Script_GetNumAvailableQuests(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGQuestInfo::GetNumQuests()));
  return 1;
}

static int __fastcall Script_GetNumActiveQuests(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGQuestInfo::GetNumInProgress()));
  return 1;
}

static int __fastcall Script_GetAvailableTitle(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetAvailableTitle(index)");
  }
  lua_pushstring(L, CGQuestInfo::GetQuestName(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1));
  return 1;
}

static int __fastcall Script_GetActiveTitle(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetActiveTitle(index)");
  }
  lua_pushstring(L, CGQuestInfo::GetInProgressName(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1));
  return 1;
}

static int __fastcall Script_GetAvailableLevel(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetGetAvailableLevel(index)");
  }
  lua_pushnumber(L, static_cast<double>(CGQuestInfo::GetQuestLevel(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1)));
  return 1;
}

static int __fastcall Script_GetActiveLevel(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetGetActiveLevel(index)");
  }
  lua_pushnumber(L, static_cast<double>(CGQuestInfo::GetInProgressLevel(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1)));
  return 1;
}

static int __fastcall Script_SelectAvailableQuest(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: SelectAvailableQuest(index)");
  }
  CGQuestInfo::QueryQuest(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  return 0;
}

static int __fastcall Script_SelectActiveQuest(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: SelectActiveQuest(index)");
  }
  CGQuestInfo::CompleteQuest(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  return 0;
}

static int __fastcall Script_AcceptQuest(lua_State *__formal) {
  CGQuestInfo::AcceptQuest();
  return 0;
}

static int __fastcall Script_DeclineQuest(lua_State *__formal) {
  CGQuestInfo::DeclineQuest();
  return 0;
}

static int __fastcall Script_IsQuestCompletable(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGQuestInfo::IsCompletable()));
  return 1;
}

static int __fastcall Script_CompleteQuest(lua_State *__formal) {
  CGQuestInfo::GiveQuestItems();
  return 0;
}

static int __fastcall Script_GetQuestReward(lua_State *L) {
  int choice = lua_isnumber(L, 1) ? static_cast<int>(lua_tonumber(L, 1)) - 1 : 0;
  if (!CGQuestInfo::GetReward(choice)) {
    return luaL_error(L, "Invalid reward choice in GetQuestReward");
  }
  return 0;
}

static int __fastcall Script_GetRewardMoney(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGQuestInfo::GetRewardMoney()));
  return 1;
}

static int __fastcall Script_GetNumQuestRewards(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGQuestInfo::GetNumQuestRewards()));
  return 1;
}

static int __fastcall Script_GetNumQuestChoices(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGQuestInfo::GetNumQuestChoices()));
  return 1;
}

static int __fastcall Script_GetNumQuestItems(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGQuestInfo::GetNumQuestItems()));
  return 1;
}

static int __fastcall Script_GetQuestItemInfo(lua_State *L) {
  if (!lua_isstring(L, 1) || !lua_isnumber(L, 2)) {
    return luaL_error(L, "Invalid quest item in GetQuestItemInfo");
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
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnil(L);
    return 5;
  }
  lua_pushstring(L, name);
  lua_pushstring(L, texture);
  lua_pushnumber(L, static_cast<double>(amount));
  lua_pushnumber(L, static_cast<double>(quality));
  lua_pushnumber(L, static_cast<double>(usable));
  return 5;
}

static int __fastcall Script_QuestChooseRewardError(lua_State *__formal) {
  CGGameUI::DisplayError(static_cast<GAME_ERROR_TYPE>(129));
  return 0;
}

static int __fastcall Script_ConfirmAcceptQuest(lua_State *__formal) {
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

static int __fastcall Script_GetQuestBackgroundMaterial(lua_State *L) {
  const CGObject_C *object = ClntObjMgrObjectPtr(CGQuestInfo::GetQuestGiver(), __FILE__, __LINE__);
  if (object && object->IsA(TYPE_GAMEOBJECT)) {
    lua_pushstring(L, "Stone");
  } else if (object && object->IsA(TYPE_ITEM)) {
    lua_pushstring(L, "Marble");
  } else {
    lua_pushstring(L, "Parchment");
  }
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

void __fastcall QuestInfoRegisterScriptFunctions() {
  unsigned int index;
  for (index = 0; index < 28; ++index) {
    FrameScript_RegisterFunction(s_ScriptFunctions[index].name, s_ScriptFunctions[index].method);
  }
}

void __fastcall QuestInfoUnregisterScriptFunctions() {
  unsigned int index;
  for (index = 0; index < 28; ++index) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[index].name);
  }
}
