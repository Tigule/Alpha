#ifndef WOW_SOURCE_UI_QUESTFRAME_H
#define WOW_SOURCE_UI_QUESTFRAME_H

#include <string.h>

enum QUEST_STATE {
  QUEST_GREETING = 0,
  QUEST_OFFER = 1,
  QUEST_ACCEPTED = 2,
  QUEST_REWARD = 3,
  QUEST_STATE_NUM_TYPES = 4
};

class QuestInfo {
 public:
  int  id;
  int  level;
  char name[64];
  int  turnIn;

  void Clear();
};

class QuestItemInfo {
 public:
  int rewardItemID;
  int rewardDisplayID;
  int rewardAmount;
  int choiceItemID;
  int choiceDisplayID;
  int choiceAmount;
  int requiredItemID;
  int requiredDisplayID;
  int requiredAmount;

  void Clear();
};

class CGQuestInfo {
 public:
  static void EnterWorld();
  static void LeaveWorld();
  static void SetState(DWORDLONG guid, QUEST_STATE state, LPCSTR text, int quest);
  static void SetLogDescription(LPCSTR desc);
  static void AddQuest(int quest, LPCSTR desc, int questLevel, int turnIn);
  static void AddQuestInProgress(int quest, LPCSTR desc, int questLevel);
  static void EndQuestList();
  static void AddReward(
      LPCSTR title,
      int   *itemChoice,
      int   *choiceDisplay,
      int   *choiceAmount,
      int    numChoice,
      int   *itemReward,
      int   *itemDisplay,
      int   *itemAmount,
      int    numReward,
      int    money,
      int    autoLaunched
  );
  static void             AddItemRequest(LPCSTR title, int *items, int *itemAmount, int *itemDisplay, int numItems, int completed, int autoLaunched);
  static void             QuestGiverFinished();
  static const DWORDLONG &GetQuestGiver() {
    return m_npc;
  }
  static int GetCurrentQuest() {
    return m_currentQuest;
  }
  static int IsCompletable();
  static int GetLastChosenItem() {
    return m_lastChosenItem;
  }
  static void ClearLastChosenItem() {
    m_lastChosenItem = 0;
  }
  static LPCSTR GetTitleText() {
    return m_questTitle;
  }
  static LPCSTR GetGreetingText() {
    return m_greetingText;
  }
  static LPCSTR GetQuestText() {
    return m_questText;
  }
  static LPCSTR GetQuestLogText() {
    return m_questLogText;
  }
  static LPCSTR GetProgressText() {
    return m_progressText;
  }
  static LPCSTR GetRewardText() {
    return m_rewardText;
  }
  static void QueryQuest(UINT index);
  static void CompleteQuest(UINT index);
  static void AcceptQuest();
  static void DeclineQuest();
  static void GiveQuestItems();
  static int  GetReward(int choice);
  static int  GetRewardMoney() {
    return m_rewardMoney;
  }
  static UINT GetNumQuestRewards();
  static UINT GetNumQuestChoices();
  static UINT GetNumQuestItems();
  static int
  GetQuestItemInfo(LPCSTR type, UINT index, char *name, UINT nameSize, char *texture, UINT textureSize, UINT &amount, int &quality, int &usable);
  static int  GetQuestItemID(LPCSTR type, UINT index);
  static void ConfirmAcceptQuest(int questID, LPCSTR questTitle, const DWORDLONG &initiatedBy);
  static int  GetPendingConfirmQuest() {
    return m_pendingQuest;
  }
  static int GetNumQuests() {
    return m_numQuests;
  }
  static int GetNumInProgress() {
    return m_numInProgress;
  }
  static LPCSTR GetQuestName(UINT index) {
    return index < m_numQuests ? m_quests[index].name : 0;
  }
  static LPCSTR GetInProgressName(UINT index) {
    return index < m_numInProgress ? m_inProgress[index].name : 0;
  }
  static int GetQuestLevel(UINT index) {
    return index < m_numQuests ? m_quests[index].level : 0;
  }
  static int GetInProgressLevel(UINT index) {
    return index < m_numInProgress ? m_inProgress[index].level : 0;
  }

 private:
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

 protected:
  static DWORDLONG     m_npc;
  static QUEST_STATE   m_state;
  static int           m_currentQuest;
  static int           m_completable;
  static int           m_autoLaunched;
  static int           m_lastChosenItem;
  static int           m_rewardMoney;
  static UINT          m_numQuests;
  static UINT          m_numInProgress;
  static QuestInfo     m_quests[8];
  static QuestInfo     m_inProgress[8];
  static QuestItemInfo m_questItems[6];
  static char          m_greetingText[256];
  static char          m_questTitle[64];
  static char          m_questText[1024];
  static char          m_questLogText[1024];
  static char          m_progressText[1024];
  static char          m_rewardText[1024];
  static int           m_pendingQuest;
};

#endif
