#ifndef WOW_SOURCE_UI_QUESTLOG_H
#define WOW_SOURCE_UI_QUESTLOG_H

struct QuestLogInfo {
  int  questID;
  int  logIndex;
  BOOL isHeader;
};

int __cdecl QSortQuestSortTypes(LPCVOID a, LPCVOID b);
int __cdecl QSortQuests(LPCVOID a, LPCVOID b);

class CGQuestLog {
 public:
  static void InitializeGame();
  static void ShutdownGame();
  static void EnterWorld();
  static void LeaveWorld();
  static void Update(int resetFilters);
  static void FilterAndSortQuests();
  static int  GetServerTimeOffset() {
    return m_serverTimeOffset;
  }
  static void SetSelectedQuest(int index);
  static void UpdateSelection();
  static int  GetSelectionIndex();
  static int  GetSelectedQuestID() {
    return m_selectedQuest;
  }
  static int  GetSelectedLogEntry();
  static void SetAbandonQuest() {
    m_abandonQuest = m_selectedQuest;
  }
  static int GetAbandonQuest() {
    return m_abandonQuest;
  }
  static LPCSTR GetAbandonQuestName();
  static void   AbandonSelectedQuest();
  static void   ClearQuest(int id);
  static void   AbandonQuest(int index);
  static UINT   GetNumEntries() {
    return m_numQuests;
  }
  static UINT GetNumShownEntries() {
    return m_numShownQuests;
  }
  static LPCSTR GetQuestName(int index);
  static LPCSTR GetQuestTag(int index);
  static int    GetQuestLevel(int index);
  static BOOL   IsQuestHeader(int index) {
    return index >= 0 && static_cast<UINT>(index) < m_numQuests ? m_quests[index].isHeader : 0;
  }
  static int GetQuestSortIndex(UINT index);
  static int GetQuestItemID(LPCSTR type, int index);
  static int GetQuestLogEntry(int index) {
    return index >= 0 && static_cast<UINT>(index) < m_numQuests ? m_quests[index].logIndex : -1;
  }
  static int GetQuestSortID(UINT index) {
    return index < m_numSortTypes ? m_sortTypes[index] : 0;
  }
  static BOOL IsSortHeaderCollapsed(UINT index) {
    return index < 16 && !(m_collapseFilter & (1 << index));
  }
  static void CollapseHeader(UINT index, int collapse);
  static BOOL IsSelectedQuestExpired();
  static BOOL IsQuestExpired(UINT index);
  static void SetQuestExpired(UINT index);
  static void UpdateServerTime(int serverTime);

 private:
  friend int __cdecl QSortQuestSortTypes(LPCVOID a, LPCVOID b);
  friend int __cdecl QSortQuests(LPCVOID a, LPCVOID b);

  static UINT         m_numQuests;
  static UINT         m_numSortTypes;
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
