#ifndef WOW_SOURCE_UI_CLASSTRAINERFRAME_H
#define WOW_SOURCE_UI_CLASSTRAINERFRAME_H

#include <stpl.h>

struct TrainerServiceInfo {
  int           spellID;
  int           skillLine;
  unsigned int  moneyCost;
  unsigned char pointCost[2];
  unsigned char reqLevel;
  unsigned char pad;
  unsigned int  reqSkillLine;
  unsigned int  reqSkillRank;
  unsigned int  reqSkillStep;
  int           reqAbility[3];
  unsigned char usable;
  unsigned char pad2[3];
  int           filtered;
};

struct TrainerSkillLineInfo {
  int          skillLine;
  unsigned int serviceTypeCount[6];
  int          filtered;
  int          expanded;
  int          hasCost;
};

int __cdecl QSortServices_General(const void *a, const void *b);
int __cdecl QSortServices_Tradeskill(const void *a, const void *b);
int __cdecl QSortServices_Talent(const void *a, const void *b);

enum TRAINER_TYPE {
  TRAINER_TYPE_CLASS = 0,
  TRAINER_TYPE_MOUNTS = 1,
  TRAINER_TYPE_TRADESKILLS = 2,
  TRAINER_TYPE_PET = 3
};

class CGClassTrainer {
 public:
  static void __fastcall InitializeGame();
  static void __fastcall ShutdownGame();
  static void __fastcall EnterWorld();
  static void __fastcall LeaveWorld();
  static void __fastcall SetTrainer(unsigned __int64 trainerGUID, TRAINER_TYPE type);
  static void __fastcall AddServices(
      unsigned int   count,
      int           *spellID,
      unsigned int  *moneyCost,
      unsigned int **pointCost,
      unsigned int  *reqLevel,
      unsigned int  *reqSkillLine,
      unsigned int  *reqSkillRank,
      unsigned int  *reqSkillStep,
      int          **reqAbility,
      unsigned int  *usable,
      const char    *greeting
  );
  static void __fastcall                SetSelection(unsigned int index);
  static int __fastcall                 GetSelectionIndex();
  static void __fastcall                RefreshList();
  static void __fastcall                FilterAndSortServices();
  static TrainerServiceInfo *__fastcall GetService(unsigned int index);
  static int __fastcall                 GetNumServices() {
    return m_filteredServices;
  }
  static const char *__fastcall  GetServiceName(unsigned int index);
  static const char *__fastcall  GetServiceSubtext(unsigned int index);
  static const char *__fastcall  GetServiceType(unsigned int index);
  static unsigned int __fastcall GetNumSkillLines() {
    return m_numSkillLines;
  }
  static int __fastcall GetSkillLine(unsigned int index) {
    return index < m_numSkillLines ? m_skillLines[index]->skillLine : 0;
  }
  static int __fastcall GetSkillLineIndexFromService(unsigned int index);
  static int __fastcall IsCollpasedHeader(unsigned int index);
  static int __fastcall GetServiceTypeFilter() {
    return m_serviceTypeFilter;
  }
  static int __fastcall GetSkillLineFilter() {
    return m_skillLineFilter;
  }
  static int __fastcall GetCollapseFilter() {
    return m_collapseFilter;
  }
  static void __fastcall        SetServiceTypeFilter(int filter);
  static void __fastcall        SetSkillLineFilter(int filter);
  static void __fastcall        SetCollapseFilter(int filter);
  static const char *__fastcall GetGreetingText() {
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
