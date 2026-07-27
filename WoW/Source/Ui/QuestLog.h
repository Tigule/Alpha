#ifndef WOW_SOURCE_UI_QUESTLOG_H
#define WOW_SOURCE_UI_QUESTLOG_H

struct QuestLogInfo {
  int questID;
  int logIndex;
  int isHeader;
};

int __cdecl QSortQuestSortTypes(const void *a, const void *b);
int __cdecl QSortQuests(const void *a, const void *b);

class CGQuestLog {
 public:
  static void InitializeGame();
  static void ShutdownGame();
  static void EnterWorld();
  static void LeaveWorld();
  static void Update(int resetFilters);
  static void FilterAndSortQuests();
  static int GetServerTimeOffset() {
    return m_serverTimeOffset;
  }
  static void SetSelectedQuest(int index);
  static void UpdateSelection();
  static int GetSelectionIndex();
  static int GetSelectedQuestID() {
    return m_selectedQuest;
  }
  static int GetSelectedLogEntry();
  static void SetAbandonQuest() {
    m_abandonQuest = m_selectedQuest;
  }
  static int GetAbandonQuest() {
    return m_abandonQuest;
  }
  static const char *GetAbandonQuestName();
  static void AbandonSelectedQuest();
  static void ClearQuest(int id);
  static void AbandonQuest(int index);
  static unsigned int GetNumEntries() {
    return m_numQuests;
  }
  static unsigned int GetNumShownEntries() {
    return m_numShownQuests;
  }
  static const char *GetQuestName(int index);
  static const char *GetQuestTag(int index);
  static int GetQuestLevel(int index);
  static int IsQuestHeader(int index) {
    return index >= 0 && static_cast<unsigned int>(index) < m_numQuests ? m_quests[index].isHeader : 0;
  }
  static int GetQuestSortIndex(unsigned int index);
  static int GetQuestItemID(const char *type, int index);
  static int GetQuestLogEntry(int index) {
    return index >= 0 && static_cast<unsigned int>(index) < m_numQuests ? m_quests[index].logIndex : -1;
  }
  static int GetQuestSortID(unsigned int index) {
    return index < m_numSortTypes ? m_sortTypes[index] : 0;
  }
  static int IsSortHeaderCollapsed(unsigned int index) {
    return index < 16 && !(m_collapseFilter & (1 << index));
  }
  static void CollapseHeader(unsigned int index, int collapse);
  static int IsSelectedQuestExpired();
  static int IsQuestExpired(unsigned int index);
  static void SetQuestExpired(unsigned int index);
  static void UpdateServerTime(int serverTime);

 private:
  friend int __cdecl QSortQuestSortTypes(const void *a, const void *b);
  friend int __cdecl QSortQuests(const void *a, const void *b);

  static unsigned int m_numQuests;
  static unsigned int m_numSortTypes;
  static int          m_selectedQuest;
  static int          m_abandonQuest;
  static QuestLogInfo m_quests[32];
  static int          m_sortTypes[16];
  static int          m_collapseFilter;
  static int          m_numShownQuests;
  static int          m_expiredQuests;
  static int          m_serverTimeOffset;
};

#endif
