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
  static void __fastcall InitializeGame();
  static void __fastcall ShutdownGame();
  static void __fastcall EnterWorld();
  static void __fastcall LeaveWorld();
  static void __fastcall Update(int resetFilters);
  static void __fastcall FilterAndSortQuests();
  static int __fastcall  GetServerTimeOffset() {
    return m_serverTimeOffset;
  }
  static void __fastcall SetSelectedQuest(int index);
  static void __fastcall UpdateSelection();
  static int __fastcall  GetSelectionIndex();
  static int __fastcall  GetSelectedQuestID() {
    return m_selectedQuest;
  }
  static int __fastcall  GetSelectedLogEntry();
  static void __fastcall SetAbandonQuest() {
    m_abandonQuest = m_selectedQuest;
  }
  static int __fastcall GetAbandonQuest() {
    return m_abandonQuest;
  }
  static const char *__fastcall  GetAbandonQuestName();
  static void __fastcall         AbandonSelectedQuest();
  static void __fastcall         ClearQuest(int id);
  static void __fastcall         AbandonQuest(int index);
  static unsigned int __fastcall GetNumEntries() {
    return m_numQuests;
  }
  static unsigned int __fastcall GetNumShownEntries() {
    return m_numShownQuests;
  }
  static const char *__fastcall GetQuestName(int index);
  static const char *__fastcall GetQuestTag(int index);
  static int __fastcall         GetQuestLevel(int index);
  static int __fastcall         IsQuestHeader(int index) {
    return index >= 0 && static_cast<unsigned int>(index) < m_numQuests ? m_quests[index].isHeader : 0;
  }
  static int __fastcall GetQuestSortIndex(unsigned int index);
  static int __fastcall GetQuestItemID(const char *type, int index);
  static int __fastcall GetQuestLogEntry(int index) {
    return index >= 0 && static_cast<unsigned int>(index) < m_numQuests ? m_quests[index].logIndex : -1;
  }
  static int __fastcall GetQuestSortID(unsigned int index) {
    return index < m_numSortTypes ? m_sortTypes[index] : 0;
  }
  static int __fastcall IsSortHeaderCollapsed(unsigned int index) {
    return index < 16 && !(m_collapseFilter & (1 << index));
  }
  static void __fastcall CollapseHeader(unsigned int index, int collapse);
  static int __fastcall  IsSelectedQuestExpired();
  static int __fastcall  IsQuestExpired(unsigned int index);
  static void __fastcall SetQuestExpired(unsigned int index);
  static void __fastcall UpdateServerTime(int serverTime);

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
