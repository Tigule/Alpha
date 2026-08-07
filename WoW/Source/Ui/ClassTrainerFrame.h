#ifndef WOW_SOURCE_UI_CLASSTRAINERFRAME_H
#define WOW_SOURCE_UI_CLASSTRAINERFRAME_H

#include <stpl.h>

struct TrainerServiceInfo {
  int  spellID;
  int  skillLine;
  UINT moneyCost;
  BYTE pointCost[2];
  BYTE reqLevel;
  UINT reqSkillLine;
  UINT reqSkillRank;
  UINT reqSkillStep;
  int  reqAbility[3];
  BYTE usable;
  int  enabled;
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
  int  skillLine;
  UINT numSkills[NUM_TRAINER_SERVICE_TYPES];
  int  enabled;
  int  collapsed;
  int  allCostPoints;

  void ClearSkills() {
    memset(numSkills, 0, sizeof(numSkills));
    allCostPoints = 1;
  }
};

int __cdecl QSortServices_General(LPCVOID a, LPCVOID b);
int __cdecl QSortServices_Tradeskill(LPCVOID a, LPCVOID b);
int __cdecl QSortServices_Talent(LPCVOID a, LPCVOID b);

enum TRAINER_TYPE {
  TRAINER_TYPE_GENERAL = 0,
  TRAINER_TYPE_TALENTS = 1,
  TRAINER_TYPE_TRADESKILLS = 2,
  TRAINER_TYPE_PET = 3,
  NUM_TRAINER_TYPES = 4
};

class CGClassTrainer {
 public:
  static void InitializeGame();
  static void ShutdownGame();
  static void EnterWorld();
  static void LeaveWorld();
  static void SetTrainer(DWORDLONG trainerGUID, TRAINER_TYPE type);
  static void AddServices(
      UINT   count,
      int   *spellID,
      UINT  *moneyCost,
      BYTE **pointCost,
      BYTE  *reqLevel,
      UINT  *reqSkillLine,
      UINT  *reqSkillRank,
      UINT  *reqSkillStep,
      int  **reqAbility,
      BYTE  *usable,
      LPCSTR greeting
  );
  static void                      SetSelection(UINT index);
  static int                       GetSelectionIndex();
  static void                      RefreshList();
  static void                      FilterAndSortServices();
  static const TrainerServiceInfo *GetService(UINT index);
  static int                       GetNumServices() {
    return m_filteredServices;
  }
  static LPCSTR GetServiceName(UINT index);
  static LPCSTR GetServiceSubtext(UINT index);
  static LPCSTR GetServiceType(UINT index);
  static UINT   GetNumSkillLines() {
    return m_numSkillLines;
  }
  static int GetSkillLine(UINT index) {
    return index < m_numSkillLines ? m_skillLines[index]->skillLine : 0;
  }
  static int  GetSkillLineIndexFromService(UINT index);
  static BOOL IsCollpasedHeader(UINT index);
  static int  GetServiceTypeFilter() {
    return m_serviceTypeFilter;
  }
  static int GetSkillLineFilter() {
    return m_skillLineFilter;
  }
  static int GetCollapseFilter() {
    return m_collapseFilter;
  }
  static void   SetServiceTypeFilter(int filter);
  static void   SetSkillLineFilter(int filter);
  static void   SetCollapseFilter(int filter);
  static LPCSTR GetGreetingText() {
    return m_greetingText;
  }

  static DWORDLONG GetTrainer() {
    return m_trainer;
  }

  static TRAINER_TYPE GetTrainerType() {
    return m_trainerType;
  }

 private:
  friend int __cdecl                             QSortServices_General(LPCVOID a, LPCVOID b);
  friend int __cdecl                             QSortServices_Tradeskill(LPCVOID a, LPCVOID b);
  friend int __cdecl                             QSortServices_Talent(LPCVOID a, LPCVOID b);
  static DWORDLONG                               m_trainer;
  static TRAINER_TYPE                            m_trainerType;
  static int                                     m_currentSelection;
  static UINT                                    m_numServices;
  static UINT                                    m_numSkillLines;
  static UINT                                    m_filteredServices;
  static int                                     m_serviceTypeFilter;
  static int                                     m_skillLineFilter;
  static int                                     m_collapseFilter;
  static TSGrowableArray<TrainerServiceInfo *>   m_services;
  static TSGrowableArray<TrainerSkillLineInfo *> m_skillLines;
  static char                                    m_greetingText[512];
};

#endif
