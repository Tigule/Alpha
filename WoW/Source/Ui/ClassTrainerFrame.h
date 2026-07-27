#ifndef WOW_SOURCE_UI_CLASSTRAINERFRAME_H
#define WOW_SOURCE_UI_CLASSTRAINERFRAME_H

#include <stpl.h>

struct TrainerServiceInfo {
  int           spellID;
  int           skillLine;
  unsigned int  moneyCost;
  unsigned char pointCost[2];
  unsigned char reqLevel;
  unsigned int  reqSkillLine;
  unsigned int  reqSkillRank;
  unsigned int  reqSkillStep;
  int           reqAbility[3];
  unsigned char usable;
  int           enabled;
};

enum TRAINER_SERVICE {
  TRAINER_SERVICE_AVAILABLE = 0,
  TRAINER_SERVICE_UNAVAILABLE = 1,
  TRAINER_SERVICE_USED = 2,
  TRAINER_SERVICE_NOT_SHOWN = 3,
  TRAINER_SERVICE_NEVER = 4,
  TRAINER_SERVICE_NO_PET = 5,
  NUM_TRAINER_SERVICE_TYPES = 6
};

struct TrainerSkillLineInfo {
  int          skillLine;
  unsigned int numSkills[NUM_TRAINER_SERVICE_TYPES];
  int          enabled;
  int          collapsed;
  int          allCostPoints;

  void ClearSkills() {
    memset(numSkills, 0, sizeof(numSkills));
    allCostPoints = 1;
  }
};

int __cdecl QSortServices_General(const void *a, const void *b);
int __cdecl QSortServices_Tradeskill(const void *a, const void *b);
int __cdecl QSortServices_Talent(const void *a, const void *b);

enum TRAINER_TYPE {
  TRAINER_TYPE_GENERAL = 0,
  TRAINER_TYPE_TALENTS = 1,
  TRAINER_TYPE_TRADESKILLS = 2,
  TRAINER_TYPE_PET = 3
};

class CGClassTrainer {
 public:
  static void InitializeGame();
  static void ShutdownGame();
  static void EnterWorld();
  static void LeaveWorld();
  static void SetTrainer(unsigned __int64 trainerGUID, TRAINER_TYPE type);
  static void AddServices(
      unsigned int   count,
      int           *spellID,
      unsigned int  *moneyCost,
      unsigned char **pointCost,
      unsigned char  *reqLevel,
      unsigned int  *reqSkillLine,
      unsigned int  *reqSkillRank,
      unsigned int  *reqSkillStep,
      int          **reqAbility,
      unsigned char  *usable,
      const char    *greeting
  );
  static void SetSelection(unsigned int index);
  static int GetSelectionIndex();
  static void RefreshList();
  static void FilterAndSortServices();
  static const TrainerServiceInfo *GetService(unsigned int index);
  static int GetNumServices() {
    return m_filteredServices;
  }
  static const char *GetServiceName(unsigned int index);
  static const char *GetServiceSubtext(unsigned int index);
  static const char *GetServiceType(unsigned int index);
  static unsigned int GetNumSkillLines() {
    return m_numSkillLines;
  }
  static int GetSkillLine(unsigned int index) {
    return index < m_numSkillLines ? m_skillLines[index]->skillLine : 0;
  }
  static int GetSkillLineIndexFromService(unsigned int index);
  static int IsCollpasedHeader(unsigned int index);
  static int GetServiceTypeFilter() {
    return m_serviceTypeFilter;
  }
  static int GetSkillLineFilter() {
    return m_skillLineFilter;
  }
  static int GetCollapseFilter() {
    return m_collapseFilter;
  }
  static void SetServiceTypeFilter(int filter);
  static void SetSkillLineFilter(int filter);
  static void SetCollapseFilter(int filter);
  static const char *GetGreetingText() {
    return m_greetingText;
  }

  static unsigned __int64 GetTrainer() {
    return m_trainer;
  }

  static TRAINER_TYPE GetTrainerType() {
    return m_trainerType;
  }

 private:
  friend int __cdecl                             QSortServices_General(const void *a, const void *b);
  friend int __cdecl                             QSortServices_Tradeskill(const void *a, const void *b);
  friend int __cdecl                             QSortServices_Talent(const void *a, const void *b);
  static unsigned __int64                        m_trainer;
  static TRAINER_TYPE                            m_trainerType;
  static int                                     m_currentSelection;
  static unsigned int                            m_numServices;
  static unsigned int                            m_numSkillLines;
  static unsigned int                            m_filteredServices;
  static int                                     m_serviceTypeFilter;
  static int                                     m_skillLineFilter;
  static int                                     m_collapseFilter;
  static TSGrowableArray<TrainerServiceInfo *>   m_services;
  static TSGrowableArray<TrainerSkillLineInfo *> m_skillLines;
  static char                                    m_greetingText[512];
};

#endif
