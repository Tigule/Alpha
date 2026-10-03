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

#include "ClassTrainerFrame.h"
#include "GameUI.h"

#include "ObjectMgrClient/ObjectMgrClient.h"
#include "Object/ObjectClient/Player_C.h"
#include "DB/DBClient/AutoCode/SpellRec.h"
#include "DB/DBClient/AutoCode/SpellIconRec.h"
#include "DB/DBClient/AutoCode/SpellRangeRec.h"
#include "DB/DBClient/AutoCode/SpellCastTimesRec.h"
#include "DB/DBClient/AutoCode/SpellDurationRec.h"
#include "DB/DBClient/AutoCode/SpellItemEnchantmentRec.h"
#include "UIUtil/Tooltip.h"
#include "DB/DBClient/DBCacheInstances.h"
#include "DB/DBClient/DBClient.h"
#include "Object/ItemStats.h"
#include "Object/ObjectClient/Item_C.h"
#include "DB/DBClient/AutoCode/SkillLineRec.h"
#include "DB/DBClient/AutoCode/SkillLineAbilityRec.h"
#include "DB/DBClient/AutoCode/ItemClassRec.h"
#include "DB/DBClient/AutoCode/ItemSubClassRec.h"
#include "DB/DBClient/AutoCode/ChrRacesRec.h"
#include "DB/DBClient/AutoCode/ChrClassesRec.h"
#include "Net/NetClient/NetClient.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

#include <FrameScript/FrameScript.h>
#include <lauxlib.h>
#include <lua.h>

#include <stdlib.h>
#include <string.h>

DWORDLONG                               CGClassTrainer::m_trainer;
TRAINER_TYPE                            CGClassTrainer::m_trainerType;
int                                     CGClassTrainer::m_currentSelection;
UINT                                    CGClassTrainer::m_numServices;
UINT                                    CGClassTrainer::m_numSkillLines;
UINT                                    CGClassTrainer::m_filteredServices;
int                                     CGClassTrainer::m_serviceTypeFilter;
int                                     CGClassTrainer::m_skillLineFilter;
int                                     CGClassTrainer::m_collapseFilter;
TSGrowableArray<TrainerServiceInfo *>   CGClassTrainer::m_services;
TSGrowableArray<TrainerSkillLineInfo *> CGClassTrainer::m_skillLines;
char                                    CGClassTrainer::m_greetingText[512];

void Spell_C_GetMinMaxPoints(const SpellRec *srec, int effectIndex, int *min, int *max, UINT level, BOOL isPet);
BOOL SpellParserParseText(const SpellRec *spell, char *buf, UINT size, BOOL isPet);
const SkillLineAbilityRec *SpellTableLookupAbility(UINT raceID, UINT classID, UINT spellID);
int  Spell_C_GetSpellLevel(int id, BOOL isPet);
int  Spell_C_GetManaCost(int id, BOOL isPet);
int  Spell_C_GetManaCostPerSecond(int id, BOOL isPet);

extern LPCSTR g_invTypeTokens[];

static const char  s_tradeSkillTypes[3][64] = {"", "TRADESKILL_SERVICE_STEP", "TRADESKILL_SERVICE_LEARN"};
static const char *KNOWN_TALENTS_TOKEN = "KNOWN_TALENTS_HEADER";

static void TrainerItemCallback(int id, const DWORDLONG &guid, LPVOID arg, bool granted) {
  if (granted) {
    CGClassTrainer::RefreshList();
  }
}

static void TradeSkillItemCallback(int id, const DWORDLONG &guid, LPVOID arg, bool granted) {
  if (granted) {
    CGClassTrainer::RefreshList();
  }
}

void CGClassTrainer::InitializeGame() {
}

void CGClassTrainer::ShutdownGame() {
  UINT i;
  for (i = 0; i < m_services.Count(); ++i) {
    DEL(m_services[i]);
  }
  m_services.SetCount(0);

  for (i = 0; i < m_skillLines.Count(); ++i) {
    DEL(m_skillLines[i]);
  }
  m_skillLines.SetCount(0);
}

void CGClassTrainer::EnterWorld() {
  m_trainer = 0;
  m_currentSelection = 0;
  m_numServices = 0;
  m_greetingText[0] = 0;
}

void CGClassTrainer::LeaveWorld() {
  SetTrainer(0, TRAINER_TYPE_GENERAL);
}

void CGClassTrainer::SetTrainer(DWORDLONG trainerGUID, TRAINER_TYPE type) {
  if (!trainerGUID) {
    FrameScript_SignalEvent(288);
    CGGameUI::ClearInteractTarget(m_trainer);
    m_trainer = 0;
    m_numSkillLines = 0;
    m_numServices = 0;
    return;
  }
  if (trainerGUID != ClntObjMgrGetActivePlayer()) {
    CGGameUI::SetInteractTarget(trainerGUID, MAX_SHOP_DISTANCE_SQUARED);
  }
  m_trainerType = type;
  m_trainer = trainerGUID;
}

void CGClassTrainer::SetSelection(UINT index) {
  if (index >= m_numServices) {
    m_currentSelection = 0;
    return;
  }
  m_currentSelection = m_services[index]->spellID;
}

int CGClassTrainer::GetSelectionIndex() {
  if (!m_currentSelection) {
    return -1;
  }
  UINT index;
  for (index = 0; index < m_numServices; ++index) {
    if (m_services[index]->spellID == m_currentSelection) {
      break;
    }
  }
  if (index == m_numServices) {
    return -1;
  }
  return index;
}

static int __cdecl QSortSkillLines(LPCVOID a, LPCVOID b) {
  FATALASSERT(a);
  FATALASSERT(b);
  TrainerSkillLineInfo *info1 = *static_cast<TrainerSkillLineInfo *const *>(a);
  TrainerSkillLineInfo *info2 = *static_cast<TrainerSkillLineInfo *const *>(b);
  if (info1->skillLine == info2->skillLine) {
    return 0;
  }
  if (info1->skillLine == -1 || info2->skillLine == -1) {
    return info2->skillLine == -1 ? 1 : -1;
  }
  if (info1->allCostPoints != info2->allCostPoints) {
    return info1->allCostPoints ? 1 : -1;
  }
  const SkillLineRec *line1 = g_skillLineDB.GetRecord(info1->skillLine);
  const SkillLineRec *line2 = g_skillLineDB.GetRecord(info2->skillLine);
  if (!line1 || !line2) {
    return 0;
  }
  return SStrCmpI(line1->m_displayName_lang[CURRENT_LANGUAGE], line2->m_displayName_lang[CURRENT_LANGUAGE], 0x7FFFFFFF);
}

static int __cdecl QSortTradeSkillTypes(LPCVOID a, LPCVOID b) {
  FATALASSERT(a);
  FATALASSERT(b);
  int line1 = (*static_cast<TrainerSkillLineInfo *const *>(a))->skillLine;
  int line2 = (*static_cast<TrainerSkillLineInfo *const *>(b))->skillLine;
  if (line1 == line2) {
    return 0;
  }
  return line1 > line2 ? 1 : -1;
}

static int __cdecl QSortServices_General(LPCVOID a, LPCVOID b) {
  FATALASSERT(a);
  FATALASSERT(b);
  TrainerServiceInfo *info1 = *static_cast<TrainerServiceInfo *const *>(a);
  TrainerServiceInfo *info2 = *static_cast<TrainerServiceInfo *const *>(b);
  if (!info1->enabled || !info2->enabled) {
    if (!info1->enabled && !info2->enabled) {
      return 0;
    }
    return info2->enabled ? 1 : -1;
  }
  int  lineRank1 = 0;
  int  lineRank2 = 0;
  UINT i;
  for (i = 0; i < CGClassTrainer::GetNumSkillLines(); ++i) {
    if (CGClassTrainer::GetSkillLine(i) == info1->skillLine) {
      lineRank1 = i;
    }
    if (CGClassTrainer::GetSkillLine(i) == info2->skillLine) {
      lineRank2 = i;
    }
  }
  if (lineRank1 == lineRank2) {
    if (info1->spellID >= 0 && info2->spellID >= 0) {
      const SpellRec *spell1 = g_spellDB.GetRecord(info1->spellID);
      const SpellRec *spell2 = g_spellDB.GetRecord(info2->spellID);
      if (spell1 && spell2) {
        if (info1->reqLevel == info2->reqLevel) {
          if (info1->reqSkillRank == info2->reqSkillRank) {
            return SStrCmp(spell1->m_name_lang[CURRENT_LANGUAGE], spell2->m_name_lang[CURRENT_LANGUAGE], 0x7FFFFFFF);
          }
          return info1->reqSkillRank > info2->reqSkillRank ? 1 : -1;
        }
        return info1->reqLevel > info2->reqLevel ? 1 : -1;
      }
      return 0;
    }
    return info1->spellID >= 0 ? 1 : -1;
  }
  return lineRank1 > lineRank2 ? 1 : -1;
}

static int __cdecl QSortServices_Tradeskill(LPCVOID a, LPCVOID b) {
  FATALASSERT(a);
  FATALASSERT(b);
  TrainerServiceInfo *info1 = *static_cast<TrainerServiceInfo *const *>(a);
  TrainerServiceInfo *info2 = *static_cast<TrainerServiceInfo *const *>(b);
  if (!info1->enabled || !info2->enabled) {
    if (!info1->enabled && !info2->enabled) {
      return 0;
    }
    return info2->enabled ? 1 : -1;
  }
  if (info1->skillLine == info2->skillLine) {
    if (info1->spellID >= 0 && info2->spellID >= 0) {
      const SpellRec *spell1 = g_spellDB.GetRecord(info1->spellID);
      const SpellRec *spell2 = g_spellDB.GetRecord(info2->spellID);
      if (spell1 && spell2) {
        if (info1->reqSkillRank == info2->reqSkillRank) {
          return SStrCmp(spell1->m_name_lang[CURRENT_LANGUAGE], spell2->m_name_lang[CURRENT_LANGUAGE], 0x7FFFFFFF);
        }
        return info1->reqSkillRank > info2->reqSkillRank ? 1 : -1;
      }
      return 0;
    }
    return info1->spellID >= 0 ? 1 : -1;
  }
  return info1->skillLine > info2->skillLine ? 1 : -1;
}

static int __cdecl QSortServices_Talent(LPCVOID a, LPCVOID b) {
  FATALASSERT(a);
  FATALASSERT(b);
  TrainerServiceInfo *info1 = *static_cast<TrainerServiceInfo *const *>(a);
  TrainerServiceInfo *info2 = *static_cast<TrainerServiceInfo *const *>(b);
  if (!info1->enabled || !info2->enabled) {
    if (!info1->enabled && !info2->enabled) {
      return 0;
    }
    return info2->enabled ? 1 : -1;
  }
  int  lineRank1 = 0;
  int  lineRank2 = 0;
  UINT i;
  for (i = 0; i < CGClassTrainer::GetNumSkillLines(); ++i) {
    if (CGClassTrainer::GetSkillLine(i) == info1->skillLine) {
      lineRank1 = i;
    }
    if (CGClassTrainer::GetSkillLine(i) == info2->skillLine) {
      lineRank2 = i;
    }
  }
  if (lineRank1 == lineRank2) {
    if (info1->spellID >= 0 && info2->spellID >= 0) {
      if (info1->usable != info2->usable) {
        return info1->usable > info2->usable ? 1 : -1;
      }
      const SpellRec *spell1 = g_spellDB.GetRecord(info1->spellID);
      const SpellRec *spell2 = g_spellDB.GetRecord(info2->spellID);
      if (spell1 && spell2) {
        return SStrCmp(spell1->m_name_lang[CURRENT_LANGUAGE], spell2->m_name_lang[CURRENT_LANGUAGE], 0x7FFFFFFF);
      }
      return 0;
    }
    return info1->spellID >= 0 ? 1 : -1;
  }
  return lineRank1 > lineRank2 ? 1 : -1;
}

static int GetSkillLineFromService(int serviceSpell) {
  int             effectIndex = -1;
  const SpellRec *spell = g_spellDB.GetRecord(serviceSpell);
  if (spell) {
    BOOL petSpell = 0;
    for (int i = 0; i < 3; ++i) {
      if (spell->m_effect[i] == 36 || spell->m_effect[i] == 57) {
        effectIndex = i;
        if (spell->m_effect[i] == 57) {
          petSpell = 1;
        }
        break;
      }
    }
    CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (unit) {
      if (petSpell) {
        unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(unit->GetControlledGUID(), __FILE__, __LINE__));
      }
      if (unit) {
        if (effectIndex >= 0) {
          return unit->GetSpellSkillLine(spell->m_effectTriggerSpell[effectIndex]);
        }
        return unit->GetSpellSkillLine(spell->m_ID);
      }
    }
  }
  return 0;
}

void CGClassTrainer::AddServices(
    UINT         count,
    int         *spellID,
    UINT        *moneyCost,
    BYTE        *pointCost[2],
    BYTE        *reqLevel,
    UINT        *reqSkillLine,
    UINT        *reqSkillRank,
    UINT        *reqSkillStep,
    int         *reqAbility[3],
    BYTE        *usable,
    LPCSTR       greeting
) {
  UINT i;
  UINT j;
  UINT k;

  for (i = m_services.Count(); i < count; ++i) {
    TrainerServiceInfo *info = NEW(TrainerServiceInfo);
    m_services.Add(&info);
  }

  m_serviceTypeFilter = 3;
  if (m_trainerType == TRAINER_TYPE_TALENTS) {
    m_serviceTypeFilter = 5;
  }
  m_skillLineFilter = -1;
  m_collapseFilter = -1;
  m_numSkillLines = 0;

  if (m_trainerType == TRAINER_TYPE_TALENTS) {
    if (!m_skillLines.Count()) {
      TrainerSkillLineInfo *info = NEW(TrainerSkillLineInfo);
      m_skillLines.Add(&info);
    }
    m_skillLines[0]->skillLine = -1;
    m_skillLines[0]->ClearSkills();
    for (i = 0; i < NUM_TRAINER_SERVICE_TYPES; ++i) {
      m_skillLines[0]->numSkills[i] = 0;
    }
    m_numSkillLines = 1;
  }

  int skipped = 0;
  for (i = 0; skipped + i < count; ++i) {
    m_services[i]->spellID = spellID[skipped + i];
    m_services[i]->moneyCost = moneyCost[skipped + i];
    for (j = 0; j < 2; ++j) {
      m_services[i]->pointCost[j] = pointCost[j][skipped + i];
    }
    m_services[i]->reqLevel = reqLevel[skipped + i];
    m_services[i]->reqSkillLine = reqSkillLine[skipped + i];
    m_services[i]->reqSkillRank = reqSkillRank[skipped + i];
    m_services[i]->reqSkillStep = reqSkillStep[skipped + i];
    for (j = 0; j < 3; ++j) {
      m_services[i]->reqAbility[j] = reqAbility[j][skipped + i];
    }
    m_services[i]->usable = usable[skipped + i];

    int line;
    if (m_trainerType == TRAINER_TYPE_TRADESKILLS) {
      line = 2;
      const SpellRec *spell = g_spellDB.GetRecord(m_services[i]->spellID);
      if (spell) {
        for (j = 0; j < 3; ++j) {
          if (spell->m_effect[j] == 44) {
            line = 1;
            break;
          }
        }
      }
    } else if (m_trainerType == TRAINER_TYPE_TALENTS && m_services[i]->usable == TRAINER_SERVICE_USED) {
      line = -1;
    } else {
      line = GetSkillLineFromService(m_services[i]->spellID);
    }
    m_services[i]->skillLine = line;

    if (!line) {
      i--;
      skipped++;
      continue;
    }

    for (j = 0; j < m_numSkillLines; ++j) {
      if (m_skillLines[j]->skillLine == line) {
        ++m_skillLines[j]->numSkills[m_services[i]->usable];
        if (m_skillLines[j]->allCostPoints) {
          int hasCost = 0;
          for (k = 0; k < 2; ++k) {
            if (m_services[i]->pointCost[k]) {
              hasCost = 1;
              break;
            }
          }
          m_skillLines[j]->allCostPoints = hasCost;
        }
        break;
      }
    }

    if (j == m_numSkillLines) {
      if (m_numSkillLines >= m_skillLines.Count()) {
        TrainerSkillLineInfo *info = NEW(TrainerSkillLineInfo);
        m_skillLines.Add(&info);
      }
      m_skillLines[m_numSkillLines]->skillLine = line;
      m_skillLines[m_numSkillLines]->ClearSkills();
      ++m_skillLines[m_numSkillLines]->numSkills[m_services[i]->usable];
      m_skillLines[m_numSkillLines]->allCostPoints = 0;
      for (j = 0; j < 2; ++j) {
        if (m_services[i]->pointCost[j]) {
          m_skillLines[m_numSkillLines]->allCostPoints = 1;
          break;
        }
      }
      ++m_numSkillLines;
    }
  }

  m_numServices = count - skipped;
  qsort(
      m_skillLines.Ptr(), m_numSkillLines, sizeof(TrainerSkillLineInfo *),
      m_trainerType == TRAINER_TYPE_TRADESKILLS ? QSortTradeSkillTypes : QSortSkillLines
  );

  for (i = m_services.Count(); i < m_numServices + m_numSkillLines; ++i) {
    TrainerServiceInfo *info = NEW(TrainerServiceInfo);
    m_services.Add(&info);
  }
  for (i = 0; i < m_numSkillLines; ++i) {
    m_services[m_numServices + i]->spellID = -1;
    m_services[m_numServices + i]->skillLine = m_skillLines[i]->skillLine;
  }
  m_numServices += m_numSkillLines;
  FilterAndSortServices();
  SetSelection(0);
  if (greeting) {
    SStrCopy(m_greetingText, greeting, sizeof(m_greetingText));
  }
  FrameScript_SignalEvent(286);
}

void CGClassTrainer::RefreshList() {
  UINT i;
  UINT j;

  if (!m_trainer) {
    return;
  }

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return;
  }

  for (i = 0; i < m_numServices; ++i) {
    if (m_services[i]->spellID == -1) {
      continue;
    }
    if (m_services[i]->usable == TRAINER_SERVICE_USED && m_trainerType != TRAINER_TYPE_TALENTS) {
      continue;
    }

    const SpellRec *srec = g_spellDB.GetRecord(m_services[i]->spellID);
    if (!srec) {
      continue;
    }

    m_services[i]->usable = TRAINER_SERVICE_AVAILABLE;

    CGUnit_C *pet = 0;
    int       learnSpell = 0;
    int       numToLearn = 0;
    int       numLearned = 0;

    for (j = 0; j < 3; ++j) {
      if (srec->m_effect[j] == 36) {
        ++numToLearn;
        learnSpell = srec->m_effectTriggerSpell[j];
        BOOL known = player->IsSpellKnown(learnSpell);
        BOOL superceded = player->IsSpellSuperceded(learnSpell);
        if (m_trainerType == TRAINER_TYPE_TALENTS && superceded) {
          m_services[i]->usable = TRAINER_SERVICE_NOT_SHOWN;
          continue;
        }
        if (known || superceded) {
          ++numLearned;
          continue;
        }
      }

      if (srec->m_effect[j] == 44) {
        int min;
        int max;
        Spell_C_GetMinMaxPoints(srec, j, &min, &max, 0, 0);
        for (UINT skill = 0; skill < 64; ++skill) {
          if (player->GetMirrorSkillID(skill) == srec->m_effectMiscValue[j]) {
            if (player->GetMirrorSkillStep(skill) >= max) {
              m_services[i]->usable = TRAINER_SERVICE_USED;
            }
            break;
          }
        }
      }

      if (srec->m_effect[j] == 57) {
        ++numToLearn;
        pet = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(player->GetSummon(), __FILE__, __LINE__));
        if (!pet) {
          m_services[i]->usable = TRAINER_SERVICE_UNAVAILABLE;
          break;
        }
        int petSpell = srec->m_effectTriggerSpell[j];
        if (pet->IsSpellKnown(petSpell) || pet->IsSpellSuperceded(petSpell)) {
          ++numLearned;
          continue;
        }
        if (m_services[i]->reqLevel > 1 && pet->GetLevel() < m_services[i]->reqLevel) {
          m_services[i]->usable = TRAINER_SERVICE_UNAVAILABLE;
          break;
        }
      }
    }

    if (numToLearn > 0 && numLearned == numToLearn) {
      m_services[i]->usable = TRAINER_SERVICE_USED;
    }
    if (m_services[i]->usable) {
      continue;
    }

    if (m_services[i]->reqSkillLine) {
      int maxRank = 0;
      for (j = 0; j < 64; ++j) {
        if (player->GetMirrorSkillID(j) == m_services[i]->reqSkillLine) {
          maxRank = player->GetMirrorSkillMaxRank(j);
          if (player->GetMirrorSkillRank(j) < m_services[i]->reqSkillRank) {
            m_services[i]->usable = TRAINER_SERVICE_UNAVAILABLE;
          }
          break;
        }
      }

      const SpellRec *steprec = g_spellDB.GetRecord(m_services[i]->reqSkillStep);
      if (steprec) {
        for (j = 0; j < 3; ++j) {
          if (steprec->m_effect[j] == 44 && steprec->m_effectMiscValue[j] == m_services[i]->reqSkillLine) {
            int min;
            int max;
            Spell_C_GetMinMaxPoints(steprec, j, &min, &max, 0, 0);
            if (maxRank < 5 * max) {
              m_services[i]->usable = TRAINER_SERVICE_UNAVAILABLE;
            }
            break;
          }
        }
      }
    }

    if (m_services[i]->usable) {
      continue;
    }

    BOOL ok = 1;
    if (pet) {
      for (j = 0; j < 3; ++j) {
        if (m_services[i]->reqAbility[j] && !pet->IsSpellKnown(m_services[i]->reqAbility[j]) && !pet->IsSpellSuperceded(m_services[i]->reqAbility[j])) {
          ok = 0;
          break;
        }
      }
    } else {
      for (j = 0; j < 3; ++j) {
        if (m_services[i]->reqAbility[j] && !player->IsSpellKnown(m_services[i]->reqAbility[j]) && !player->IsSpellSuperceded(m_services[i]->reqAbility[j])) {
          ok = 0;
          break;
        }
      }
      if (!ok && m_trainerType == TRAINER_TYPE_TALENTS && learnSpell) {
        const SkillLineAbilityRec *ability = SpellTableLookupAbility(player->GetRace(), player->GetClass(), m_services[i]->reqAbility[j]);
        if (ability && ability->m_supercededBySpell == learnSpell) {
          m_services[i]->usable = TRAINER_SERVICE_NOT_SHOWN;
          continue;
        }
      }
    }

    if (!ok) {
      m_services[i]->usable = TRAINER_SERVICE_UNAVAILABLE;
      continue;
    }

    if (m_services[i]->reqLevel > 1 && player->GetLevel() < m_services[i]->reqLevel) {
      m_services[i]->usable = TRAINER_SERVICE_UNAVAILABLE;
    }
  }

  for (i = 0; i < m_numSkillLines; ++i) {
    m_skillLines[i]->ClearSkills();
  }

  for (i = 0; i < m_numServices; ++i) {
    if (m_services[i]->spellID == -1) {
      continue;
    }
    if (m_trainerType == TRAINER_TYPE_TALENTS && m_services[i]->usable == TRAINER_SERVICE_USED) {
      m_services[i]->skillLine = -1;
    }
    for (j = 0; j < m_numSkillLines; ++j) {
      if (m_skillLines[j]->skillLine == m_services[i]->skillLine) {
        ++m_skillLines[j]->numSkills[m_services[i]->usable];
        break;
      }
    }
  }

  FilterAndSortServices();
  FrameScript_SignalEvent(287);
}

void CGClassTrainer::FilterAndSortServices() {
  UINT i;
  UINT j;
  m_filteredServices = m_numServices;
  for (i = 0; i < m_numSkillLines; ++i) {
    BOOL hasService = 0;
    for (j = 0; j < NUM_TRAINER_SERVICE_TYPES; ++j) {
      if ((m_serviceTypeFilter & (1 << j)) && m_skillLines[i]->numSkills[j] > 0) {
        hasService = 1;
        break;
      }
    }
    m_skillLines[i]->enabled = hasService && (m_skillLineFilter & (1 << i));
    m_skillLines[i]->collapsed = !(m_collapseFilter & (1 << i));
  }

  for (i = 0; i < m_numServices; ++i) {
    if (m_services[i]->spellID >= 0 && !(m_serviceTypeFilter & (1 << m_services[i]->usable))) {
      m_services[i]->enabled = 0;
      --m_filteredServices;
    } else {
      m_services[i]->enabled = 1;
      if (m_numSkillLines) {
        UINT line = 0;
        for (j = 0; j < m_numSkillLines; ++j) {
          if (m_skillLines[j]->skillLine == m_services[i]->skillLine) {
            line = j;
            break;
          }
        }
        if (!m_skillLines[line]->enabled || (m_services[i]->spellID >= 0 && m_skillLines[line]->collapsed)) {
          m_services[i]->enabled = 0;
          --m_filteredServices;
        }
      }
    }
  }

  switch (m_trainerType) {
    case TRAINER_TYPE_TALENTS:
      qsort(m_services.Ptr(), m_numServices, sizeof(TrainerServiceInfo *), QSortServices_Talent);
      break;
    case TRAINER_TYPE_TRADESKILLS:
      qsort(m_services.Ptr(), m_numServices, sizeof(TrainerServiceInfo *), QSortServices_Tradeskill);
      break;
    default:
      qsort(m_services.Ptr(), m_numServices, sizeof(TrainerServiceInfo *), QSortServices_General);
      break;
  }
}

const TrainerServiceInfo *CGClassTrainer::GetService(UINT index) {
  if (index >= m_numServices) {
    return 0;
  }
  return m_services[index];
}

LPCSTR CGClassTrainer::GetServiceName(UINT index) {
  if (index < m_numServices) {
    if (m_services[index]->spellID >= 0) {
      const SpellRec *spell = g_spellDB.GetRecord(m_services[index]->spellID);
      if (spell) {
        return spell->m_name_lang[CURRENT_LANGUAGE];
      }
    } else if (m_trainerType == TRAINER_TYPE_TRADESKILLS) {
      return FrameScript_GetText(s_tradeSkillTypes[m_services[index]->skillLine], -1, GENDER_NOT_APPLICABLE);
    } else if (m_trainerType == TRAINER_TYPE_TALENTS && m_services[index]->skillLine == -1) {
      return FrameScript_GetText(KNOWN_TALENTS_TOKEN, -1, GENDER_NOT_APPLICABLE);
    } else {
      const SkillLineRec *line = g_skillLineDB.GetRecord(m_services[index]->skillLine);
      if (line) {
        return line->m_displayName_lang[CURRENT_LANGUAGE];
      }
    }
  }
  return 0;
}

LPCSTR CGClassTrainer::GetServiceSubtext(UINT index) {
  if (index >= m_numServices) {
    return 0;
  }
  if (m_services[index]->spellID >= 0) {
    const SpellRec *spell = g_spellDB.GetRecord(m_services[index]->spellID);
    if (spell) {
      return spell->m_nameSubtext_lang[CURRENT_LANGUAGE];
    }
    return 0;
  }
  return "";
}

LPCSTR CGClassTrainer::GetServiceType(UINT index) {
  if (index >= m_numServices) {
    return 0;
  }
  if (m_services[index]->spellID >= 0) {
    switch (m_services[index]->usable) {
      case TRAINER_SERVICE_AVAILABLE:
        return "available";
      case TRAINER_SERVICE_USED:
        return "used";
      default:
        return "unavailable";
    }
  }
  return "header";
}

int CGClassTrainer::GetSkillLineIndexFromService(UINT index) {
  if (index < m_numServices && m_services[index]->spellID < 0) {
    UINT i;
    for (i = 0; i < m_numSkillLines; ++i) {
      if (m_skillLines[i]->skillLine == m_services[index]->skillLine) {
        return i;
      }
    }
  }
  return -1;
}

BOOL CGClassTrainer::IsCollpasedHeader(UINT index) {
  if (index < m_numServices && m_services[index]->spellID < 0) {
    UINT i;
    for (i = 0; i < m_numSkillLines; ++i) {
      if (m_skillLines[i]->skillLine == m_services[index]->skillLine) {
        return !(m_collapseFilter & (1 << i));
      }
    }
  }
  return 0;
}

void CGClassTrainer::SetServiceTypeFilter(int filter) {
  m_serviceTypeFilter = filter;
  FilterAndSortServices();
  FrameScript_SignalEvent(287);
}

void CGClassTrainer::SetSkillLineFilter(int filter) {
  m_skillLineFilter = filter;
  FilterAndSortServices();
  FrameScript_SignalEvent(287);
}

void CGClassTrainer::SetCollapseFilter(int filter) {
  m_collapseFilter = filter;
  FilterAndSortServices();
  FrameScript_SignalEvent(287);
}

static int Script_OpenTrainer(lua_State *) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->TalkToTrainer(player->GetGUID());
  }
  return 0;
}

static int Script_CloseTrainer(lua_State *) {
  CGClassTrainer::SetTrainer(0, TRAINER_TYPE_GENERAL);
  return 0;
}

static int Script_GetNumTrainerServices(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGClassTrainer::GetNumServices()));
  return 1;
}

static int Script_GetTrainerServiceInfo(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetTrainerServiceInfo(index)");
    return 0;
  }
  UINT index = static_cast<UINT>(lua_tonumber(L, 1)) - 1;
  lua_pushstring(L, CGClassTrainer::GetServiceName(index));
  lua_pushstring(L, CGClassTrainer::GetServiceSubtext(index));
  lua_pushstring(L, CGClassTrainer::GetServiceType(index));
  if (CGClassTrainer::IsCollpasedHeader(index)) {
    lua_pushnil(L);
  } else {
    lua_pushnumber(L, 1.0);
  }
  return 4;
}

static int Script_SelectTrainerService(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: SelectTrainerService(index)");
    return 0;
  }
  UINT index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  CGClassTrainer::SetSelection(index);
  return 0;
}

static int Script_IsTradeskillTrainer(lua_State *L) {
  if (CGClassTrainer::GetTrainerType() == TRAINER_TYPE_TRADESKILLS) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_IsTalentTrainer(lua_State *L) {
  if (!CGClassTrainer::GetTrainer() || CGClassTrainer::GetTrainerType() == TRAINER_TYPE_TALENTS) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_GetTrainerSelectionIndex(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGClassTrainer::GetSelectionIndex() + 1));
  return 1;
}

static int Script_GetTrainerGreetingText(lua_State *L) {
  lua_pushstring(L, CGClassTrainer::GetGreetingText());
  return 1;
}

static int Script_GetTrainerServiceIcon(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetTrainerServiceIcon(index)");
    return 0;
  }
  UINT                      index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  const TrainerServiceInfo *info = CGClassTrainer::GetService(index);
  if (info) {
    const SpellRec *spell = g_spellDB.GetRecord(info->spellID);
    if (spell) {
      if (CGClassTrainer::GetTrainerType() == TRAINER_TYPE_TRADESKILLS) {
        int effectIndex = -1;
        for (int i = 0; i < 3; ++i) {
          if (spell->m_effect[i] == 36 || spell->m_effect[i] == 57) {
            effectIndex = i;
            break;
          }
        }
        if (effectIndex >= 0) {
          const SpellRec *learned = g_spellDB.GetRecord(spell->m_effectTriggerSpell[effectIndex]);
          if (learned && learned->m_effectItemType[0]) {
            const ItemStats_C *stats = g_itemDBCache.GetRecord(
                learned->m_effectItemType[0], static_cast<DWORDLONG>(learned->m_ID) | 0xB000000000000000ui64, TradeSkillItemCallback, 0
            );
            if (!stats) {
              lua_pushnil(L);
              return 1;
            }
            char   buffer[MAX_PATH];
            LPCSTR path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
            SStrPrintf(buffer, sizeof(buffer), "%s%s", path, *path ? "\\" : "");
            SStrPack(buffer, CGItem_C::GetInventoryArt(stats->m_displayInfoID), sizeof(buffer));
            lua_pushstring(L, buffer);
            return 1;
          }
        }
      }
      const SpellIconRec *icon = g_spellIconDB.GetRecord(spell->m_spellIconID);
      if (icon) {
        lua_pushstring(L, icon->m_textureFilename);
        return 1;
      }
    }
  }
  lua_pushnil(L);
  return 1;
}

static int Script_GetTrainerServiceSkillLine(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetTrainerServiceSkillLine(index)");
    return 0;
  }
  UINT                      index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  const TrainerServiceInfo *info = CGClassTrainer::GetService(index);
  if (info) {
    int             effectIndex = -1;
    const SpellRec *spell = g_spellDB.GetRecord(info->spellID);
    if (spell) {
      for (int i = 0; i < 3; ++i) {
        if (spell->m_effect[i] == 36 || spell->m_effect[i] == 57) {
          effectIndex = i;
          break;
        }
      }
      CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
      if (player) {
        const SkillLineRec *line = 0;
        if (effectIndex == -1) {
          const SkillLineAbilityRec *ability = SpellTableLookupAbility(player->GetRace(), player->GetClass(), spell->m_ID);
          line = ability ? g_skillLineDB.GetRecord(ability->m_skillLine) : 0;
        }
        if (!line) {
          const SpellRec *learned = g_spellDB.GetRecord(spell->m_effectTriggerSpell[effectIndex]);
          if (learned) {
            const SkillLineAbilityRec *ability = SpellTableLookupAbility(player->GetRace(), player->GetClass(), learned->m_ID);
            line = ability ? g_skillLineDB.GetRecord(ability->m_skillLine) : 0;
          }
        }
        if (line) {
          lua_pushstring(L, line->m_displayName_lang[CURRENT_LANGUAGE]);
          return 1;
        }
      }
    }
  }
  lua_pushnil(L);
  return 1;
}

static int Script_GetTrainerServiceCost(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetTrainerServiceCost(index)");
    return 0;
  }
  UINT                      index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  const TrainerServiceInfo *info = CGClassTrainer::GetService(index);
  int                       moneyCost;
  int                       costCP[2];
  if (info) {
    moneyCost = info->moneyCost;
    for (UINT i = 0; i < 2; ++i) {
      costCP[i] = info->pointCost[i];
    }
  } else {
    moneyCost = 0;
    for (UINT i = 0; i < 2; ++i) {
      costCP[i] = 0;
    }
  }
  lua_pushnumber(L, static_cast<double>(moneyCost));
  lua_pushnumber(L, static_cast<double>(costCP[0]));
  lua_pushnumber(L, static_cast<double>(costCP[1]));
  return 3;
}

static int Script_GetTrainerServiceLevelReq(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetTrainerServiceLevelReq(index)");
    return 0;
  }
  UINT                      index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  int                       level = 0;
  const TrainerServiceInfo *info = CGClassTrainer::GetService(index);
  if (info) {
    level = info->reqLevel;
  }
  lua_pushnumber(L, static_cast<double>(level));
  return 1;
}

static int Script_GetTrainerServiceSkillReq(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetTrainerServiceSkillReq(index)");
    return 0;
  }
  UINT                      index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  int                       met = 1;
  int                       rank = 0;
  CGPlayer_C               *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  const TrainerServiceInfo *info = CGClassTrainer::GetService(index);
  const SkillLineRec       *line = 0;
  if (player && info && info->reqSkillLine && info->reqSkillRank) {
    line = g_skillLineDB.GetRecord(info->reqSkillLine);
    if (line) {
      UINT i;
      for (i = 0; i < 64; ++i) {
        if (player->GetMirrorSkillID(i) == info->reqSkillLine) {
          break;
        }
      }
      if (i >= 64 || player->GetMirrorSkillRank(i) < info->reqSkillRank) {
        met = 0;
      }
      rank = info->reqSkillRank;
    }
  }
  if (line && line->m_displayName_lang[CURRENT_LANGUAGE] && *line->m_displayName_lang[CURRENT_LANGUAGE]) {
    lua_pushstring(L, line->m_displayName_lang[CURRENT_LANGUAGE]);
    lua_pushnumber(L, static_cast<double>(rank));
  } else {
    lua_pushnil(L);
    lua_pushnumber(L, 0.0);
  }
  if (met) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 3;
}

static int Script_GetTrainerServiceNumAbilityReq(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetTrainerServiceAbilityReq(index)");
    return 0;
  }
  UINT                      index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  const TrainerServiceInfo *info = CGClassTrainer::GetService(index);
  UINT                      count = 0;
  if (info) {
    for (UINT i = 0; i < 3; ++i) {
      if (info->reqAbility[i] > 0) {
        ++count;
      }
    }
  }
  lua_pushnumber(L, static_cast<double>(count));
  return 1;
}

static int Script_GetTrainerServiceAbilityReq(lua_State *L) {
  if (lua_isnumber(L, 1) && lua_isnumber(L, 2)) {
    UINT                      index = static_cast<int>(lua_tonumber(L, 1)) - 1;
    UINT                      abilityIndex = static_cast<int>(lua_tonumber(L, 2)) - 1;
    char                      ability[256] = "";
    int                       met = 1;
    CGPlayer_C               *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    const TrainerServiceInfo *service = CGClassTrainer::GetService(index);
    if (player && service && abilityIndex < 3) {
      const SpellRec *spell = g_spellDB.GetRecord(service->reqAbility[abilityIndex]);
      if (spell) {
        if (spell->m_nameSubtext_lang[CURRENT_LANGUAGE] && *spell->m_nameSubtext_lang[CURRENT_LANGUAGE]) {
          SStrPrintf(ability, sizeof(ability), "%s (%s)", spell->m_name_lang[CURRENT_LANGUAGE], spell->m_nameSubtext_lang[CURRENT_LANGUAGE]);
        } else {
          SStrCopy(ability, spell->m_name_lang[CURRENT_LANGUAGE], sizeof(ability));
        }
        const SpellRec *trainerSpell = g_spellDB.GetRecord(service->spellID);
        if (trainerSpell) {
          CGUnit_C *unit = player;
          for (UINT i = 0; i < 3; ++i) {
            if (trainerSpell->m_effect[i] == 57) {
              unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(player->GetSummon(), __FILE__, __LINE__));
              break;
            }
          }
          if (unit && !unit->IsSpellKnown(service->reqAbility[abilityIndex]) && !unit->IsSpellSuperceded(service->reqAbility[abilityIndex])) {
            met = 0;
          }
        }
      }
    }
    if (*ability) {
      lua_pushstring(L, ability);
    } else {
      lua_pushnil(L);
    }
    if (met) {
      lua_pushnumber(L, 1.0);
    } else {
      lua_pushnil(L);
    }
    return 2;
  }
  luaL_error(L, "Usage: GetTrainerServiceAbilityReq(index)");
  return 0;
}

static int Script_GetTrainerServiceStepReq(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetTrainerServiceStepReq(index)");
    return 0;
  }
  UINT                      index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  int                       met = 1;
  CGPlayer_C               *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  const TrainerServiceInfo *info = CGClassTrainer::GetService(index);
  const SpellRec           *spell = 0;
  if (player && info && info->reqSkillStep) {
    spell = g_spellDB.GetRecord(info->reqSkillStep);
    if (spell) {
      int found = 0;
      int i;
      for (i = 0; i < 3; ++i) {
        if (spell->m_effect[i] == 44) {
          break;
        }
      }
      if (i < 3) {
        for (UINT j = 0; j < 64; ++j) {
          if (player->GetMirrorSkillID(j) == spell->m_effectMiscValue[i]) {
            int min;
            int max;
            Spell_C_GetMinMaxPoints(spell, i, &min, &max, 0, 0);
            if (player->GetMirrorSkillMaxRank(j) >= max * 5) {
              found = 1;
            }
            break;
          }
        }
      }
      met = found;
    }
  }
  if (spell && spell->m_name_lang[CURRENT_LANGUAGE] && *spell->m_name_lang[CURRENT_LANGUAGE]) {
    lua_pushstring(L, spell->m_name_lang[CURRENT_LANGUAGE]);
  } else {
    lua_pushnil(L);
  }
  if (met) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 2;
}

static int Script_GetTrainerServiceDescription(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetTrainerServiceDescription(index)");
    return 0;
  }
  UINT                      index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  const TrainerServiceInfo *service = CGClassTrainer::GetService(index);
  if (service) {
    int             learnEffect = -1;
    int             skillStep = -1;
    const SpellRec *spell = g_spellDB.GetRecord(service->spellID);
    if (spell) {
      for (int i = 0; i < 3; ++i) {
        if ((spell->m_effect[i] == 36 || spell->m_effect[i] == 57) && learnEffect == -1) {
          learnEffect = i;
        } else if (spell->m_effect[i] == 44 && skillStep == -1) {
          skillStep = i;
        }
      }
      if ((CGClassTrainer::GetTrainerType() != TRAINER_TYPE_TALENTS || service->usable != 2) && spell->m_description_lang[CURRENT_LANGUAGE] &&
          *spell->m_description_lang[CURRENT_LANGUAGE])
      {
        char buf[1024];
        SpellParserParseText(spell, buf, sizeof(buf), learnEffect >= 0 && spell->m_effect[learnEffect] == 57);
        lua_pushstring(L, buf);
        return 1;
      }
      if (learnEffect >= 0) {
        const SpellRec *learned = g_spellDB.GetRecord(spell->m_effectTriggerSpell[learnEffect]);
        if (learned) {
          if (learned->m_description_lang[CURRENT_LANGUAGE] && *learned->m_description_lang[CURRENT_LANGUAGE]) {
            char buf[1024];
            SpellParserParseText(learned, buf, sizeof(buf), spell->m_effect[learnEffect] == 57);
            lua_pushstring(L, buf);
            return 1;
          }
          if ((learned->m_attributes & 0x20) && learned->m_effect[0] == 24) {
            int                itemID = learned->m_effectItemType[0];
            const ItemStats_C *stats = g_itemDBCache.GetRecord(itemID, static_cast<DWORDLONG>(learned->m_ID) | 0xB000000000000000ui64, TrainerItemCallback, 0);
            if (stats && stats->m_description && *stats->m_description) {
              lua_pushstring(L, stats->m_description);
              return 1;
            }
          }
        }
      }
      if (spell->m_description_lang[CURRENT_LANGUAGE] && *spell->m_description_lang[CURRENT_LANGUAGE]) {
        char buf[1024];
        SpellParserParseText(spell, buf, sizeof(buf), spell->m_effect[0] == 57);
        lua_pushstring(L, buf);
        return 1;
      }
    }
  }
  lua_pushnil(L);
  return 1;
}

static int Script_IsTrainerServiceSkillStep(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: IsTrainerServiceSkillStep(index)");
    return 0;
  }
  UINT                      index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  const TrainerServiceInfo *info = CGClassTrainer::GetService(index);
  if (info) {
    const SpellRec *spell = g_spellDB.GetRecord(info->spellID);
    if (spell) {
      for (int i = 0; i < 3; ++i) {
        if (spell->m_effect[i] == 44) {
          lua_pushnumber(L, 1.0);
          return 1;
        }
      }
    }
  }
  lua_pushnil(L);
  return 1;
}

static int Script_IsTrainerServiceLearnSpell(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: IsTrainerServiceLearnSpell(index)");
    return 0;
  }
  UINT                      index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  const TrainerServiceInfo *info = CGClassTrainer::GetService(index);
  if (info) {
    const SpellRec *spell = g_spellDB.GetRecord(info->spellID);
    if (spell) {
      for (int i = 0; i < 3; ++i) {
        if (spell->m_effect[i] == 36) {
          const SpellRec *learned = g_spellDB.GetRecord(spell->m_effectTriggerSpell[i]);
          if (!learned || !(learned->m_attributes & 0x20)) {
            lua_pushnumber(L, 1.0);
            lua_pushnil(L);
            return 2;
          }
        }
        if (spell->m_effect[i] == 57) {
          lua_pushnumber(L, 1.0);
          lua_pushnumber(L, 1.0);
          return 2;
        }
      }
    }
  }
  lua_pushnil(L);
  lua_pushnil(L);
  return 2;
}

static int Script_IsTrainerServiceTradeSkill(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: IsTrainerServiceTradeSkill(index)");
    return 0;
  }
  UINT                      index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  const TrainerServiceInfo *info = CGClassTrainer::GetService(index);
  if (info) {
    const SpellRec *spell = g_spellDB.GetRecord(info->spellID);
    if (spell) {
      for (int i = 0; i < 3; ++i) {
        if (spell->m_effect[i] == 36) {
          const SpellRec *learned = g_spellDB.GetRecord(spell->m_effectTriggerSpell[i]);
          if (learned && (learned->m_attributes & 0x20)) {
            lua_pushnumber(L, 1.0);
            return 1;
          }
        }
      }
    }
  }
  lua_pushnil(L);
  return 1;
}

static int Script_GetTrainerServiceStepIncrease(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetTrainerServiceStepIncrease(index)");
    return 0;
  }
  UINT                      index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  const TrainerServiceInfo *info = CGClassTrainer::GetService(index);
  if (info) {
    const SpellRec *spell = g_spellDB.GetRecord(info->spellID);
    if (spell) {
      int effectIndex = -1;
      int i;
      for (i = 0; i < 3; ++i) {
        if (spell->m_effect[i] == 44) {
          effectIndex = i;
          break;
        }
      }
      if (effectIndex >= 0 && info->usable != 2) {
        int             min;
        int             max;
        int             prev = 0;
        const SpellRec *steprec = g_spellDB.GetRecord(info->reqSkillStep);
        if (steprec) {
          for (i = 0; i < 3; ++i) {
            if (steprec->m_effect[i] == 44) {
              break;
            }
          }
          if (i < 3) {
            Spell_C_GetMinMaxPoints(steprec, i, &min, &max, 0, 0);
            prev = max;
          }
        }
        Spell_C_GetMinMaxPoints(spell, effectIndex, &min, &max, 0, 0);
        int increase = max - prev;
        if (increase) {
          const SkillLineRec *line = g_skillLineDB.GetRecord(spell->m_effectMiscValue[effectIndex]);
          if (line) {
            char   temp[256];
            char   buf[256];
            LPCSTR text = FrameScript_GetText("INCREASE_POTENTIAL", -1, GENDER_NOT_APPLICABLE);
            SStrCopy(temp, text, sizeof(temp));
            SStrPrintf(buf, sizeof(buf), temp, line->m_displayName_lang[CURRENT_LANGUAGE], increase * 5);
            lua_pushstring(L, buf);
            return 1;
          }
        }
      }
    }
  }
  lua_pushnil(L);
  return 1;
}

static int Script_GetTrainerServiceSpellStats(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetTrainerServiceSpellStats(index)");
    return 0;
  }
  UINT                      index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  const TrainerServiceInfo *info = CGClassTrainer::GetService(index);
  if (info) {
    const SpellRec *spell = g_spellDB.GetRecord(info->spellID);
    if (spell) {
      int effectIndex = -1;
      for (int i = 0; i < 3; ++i) {
        if (spell->m_effect[i] == 36 || spell->m_effect[i] == 57) {
          effectIndex = i;
          break;
        }
      }
      if (effectIndex >= 0) {
        const SpellRec *learned = g_spellDB.GetRecord(spell->m_effectTriggerSpell[effectIndex]);
        if (learned) {
          if (spell->m_attributes & 0x40) {
            lua_pushnumber(L, 1.0);
            lua_pushnil(L);
            lua_pushnil(L);
            lua_pushnil(L);
            lua_pushnil(L);
            return 5;
          }
          lua_pushnil(L);
          int  level = Spell_C_GetSpellLevel(learned->m_ID, 0);
          int  manaPerSecond = Spell_C_GetManaCostPerSecond(learned->m_ID, 0);
          char temp[256];
          char buf[256];
          if (manaPerSecond > 0) {
            LPCSTR text = FrameScript_GetText("TRAINER_MANA_COST_PER_TIME", -1, GENDER_NOT_APPLICABLE);
            SStrCopy(temp, text, sizeof(temp));
            SStrPrintf(buf, sizeof(buf), temp, Spell_C_GetManaCost(learned->m_ID, 0), manaPerSecond);
          } else {
            LPCSTR text = FrameScript_GetText("TRAINER_MANA_COST", -1, GENDER_NOT_APPLICABLE);
            SStrCopy(temp, text, sizeof(temp));
            SStrPrintf(buf, sizeof(buf), temp, Spell_C_GetManaCost(learned->m_ID, 0));
          }
          lua_pushstring(L, buf);

          const SpellRangeRec *range = g_spellRangeDB.GetRecord(max(learned->m_rangeIndex, 1));
          LPCSTR               text = FrameScript_GetText("TRAINER_RANGE", -1, GENDER_NOT_APPLICABLE);
          SStrCopy(temp, text, sizeof(temp));
          SStrPrintf(buf, sizeof(buf), temp, range->m_displayNameShort_lang[CURRENT_LANGUAGE], static_cast<int>(range->m_rangeMax));
          lua_pushstring(L, buf);

          const SpellCastTimesRec *castTime = g_spellCastTimesDB.GetRecord(max(learned->m_castingTimeIndex, 1));
          int                      time = max(castTime->m_base + castTime->m_perLevel * level, castTime->m_minimum);
          BOOL                     minutes = time >= 60000;
          if (time > 0) {
            text = FrameScript_GetText(minutes ? "TRAINER_CAST_TIME_MIN" : "TRAINER_CAST_TIME_SEC", -1, GENDER_NOT_APPLICABLE);
            SStrCopy(temp, text, sizeof(temp));
            SStrPrintf(buf, sizeof(buf), temp, time / (minutes ? 60000 : 1000));
          } else {
            text = FrameScript_GetText("TRAINER_CAST_TIME_INSTANT", -1, GENDER_NOT_APPLICABLE);
            SStrCopy(buf, text, sizeof(buf));
          }
          lua_pushstring(L, buf);

          time = max(learned->m_recoveryTime, learned->m_categoryRecoveryTime);
          minutes = time >= 60000;
          if (time > 0) {
            text = FrameScript_GetText(minutes ? "TRAINER_COOLDOWN_TIME_MIN" : "TRAINER_COOLDOWN_TIME_SEC", -1, GENDER_NOT_APPLICABLE);
            SStrCopy(temp, text, sizeof(temp));
            SStrPrintf(buf, sizeof(buf), temp, time / (minutes ? 60000 : 1000));
          } else {
            text = FrameScript_GetText("TRAINER_COOLDOWN_TIME_INSTANT", -1, GENDER_NOT_APPLICABLE);
            SStrCopy(buf, text, sizeof(buf));
          }
          lua_pushstring(L, buf);
          return 5;
        }
      }
    }
  }
  lua_pushnil(L);
  lua_pushnil(L);
  lua_pushnil(L);
  lua_pushnil(L);
  lua_pushnil(L);
  return 5;
}

static int Script_GetTrainerServiceEffects(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetTrainerServiceEffects(index)");
    return 0;
  }
  UINT                      index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  const TrainerServiceInfo *info = CGClassTrainer::GetService(index);
  int                       count = 0;
  if (info) {
    const SpellRec *srec = g_spellDB.GetRecord(info->spellID);
    if (srec) {
      int effectIndex = -1;
      int i;
      for (i = 0; i < 3; ++i) {
        if (srec->m_effect[i] == 36 || srec->m_effect[i] == 57) {
          effectIndex = i;
          break;
        }
      }
      if (effectIndex >= 0) {
        const SpellRec *spell = g_spellDB.GetRecord(srec->m_effectTriggerSpell[effectIndex]);
        if (spell) {
          int                     spellLevel = Spell_C_GetSpellLevel(spell->m_ID, 0);
          int                     passive = (spell->m_attributes & 0x40) != 0;
          char                    temp[128];
          char                    durationString[64];
          const SpellDurationRec *duration = g_spellDurationDB.GetRecord(spell->m_durationIndex);
          if (duration) {
            int dur = min(duration->m_duration + duration->m_durationPerLevel * spellLevel, duration->m_maxDuration);
            if (dur > 0) {
              BOOL   minutes = dur >= 60000;
              LPCSTR text = FrameScript_GetText(minutes ? "SPELL_DURATION_MIN" : "SPELL_DURATION_SEC", -1, GENDER_NOT_APPLICABLE);
              SStrCopy(temp, text, sizeof(temp));
              SStrPrintf(durationString, sizeof(durationString), temp, dur / (minutes ? 60000 : 1000));
            } else {
              LPCSTR text = FrameScript_GetText("SPELL_DURATION_UNTIL_CANCELLED", -1, GENDER_NOT_APPLICABLE);
              SStrCopy(durationString, text, sizeof(durationString));
            }
          }

          int  first = !passive;
          int  singleTarget = 1;
          char targetStrings[3][128];
          int  j;
          for (i = 0; i < 3; ++i) {
            targetStrings[i][0] = 0;
            if (spell->m_effect[i] && !spell->m_effectAura[i]) {
              CGTooltip::GetSpellTargetString(targetStrings[i], sizeof(targetStrings[i]), spell, i);
              for (j = 0; j < i; ++j) {
                if (targetStrings[j][0] && SStrCmpI(targetStrings[i], targetStrings[j], 0x7FFFFFFF)) {
                  singleTarget = 0;
                }
              }
            }
          }
          for (i = 0; i < 3; ++i) {
            if (spell->m_effect[i] && !spell->m_effectAura[i]) {
              char buf[128];
              CGTooltip::GetSpellEffectString(buf, sizeof(buf), spell, i, spellLevel, 0, TOOLTIP_DETAIL_NORMAL);
              if (buf[0]) {
                if (first) {
                  if (spell->m_attributes & 0x404) {
                    LPCSTR text = FrameScript_GetText("SPELL_ON_NEXT_SWING", -1, GENDER_NOT_APPLICABLE);
                    SStrCopy(temp, text, sizeof(temp));
                  } else if (spell->m_attributes & 2) {
                    LPCSTR text = FrameScript_GetText("SPELL_ON_NEXT_RANGED", -1, GENDER_NOT_APPLICABLE);
                    SStrCopy(temp, text, sizeof(temp));
                  } else {
                    LPCSTR text = FrameScript_GetText("SPELL_INSTANT_EFFECT", -1, GENDER_NOT_APPLICABLE);
                    SStrCopy(temp, text, sizeof(temp));
                  }
                  lua_pushstring(L, temp);
                  ++count;
                  first = 0;
                }
                lua_pushstring(L, buf);
                ++count;
                if (spell->m_effect[i] == 53 || spell->m_effect[i] == 54) {
                  const SpellItemEnchantmentRec *enchant = g_spellItemEnchantmentDB.GetRecord(spell->m_effectMiscValue[i]);
                  if (enchant) {
                    for (UINT k = 0; k < 3; ++k) {
                      if (enchant->m_effect[k]) {
                        CGTooltip::GetItemEnchantString(buf, sizeof(buf), enchant, k, TOOLTIP_DETAIL_NORMAL);
                        if (buf[0]) {
                          lua_pushstring(L, buf);
                          ++count;
                        }
                      }
                    }
                  }
                }
                if (!singleTarget && targetStrings[i][0]) {
                  lua_pushstring(L, targetStrings[i]);
                  ++count;
                }
                if (spell->m_effect[i] == 28 || spell->m_effect[i] == 41 || spell->m_effect[i] == 42) {
                  LPCSTR text = FrameScript_GetText("SPELL_DURATION", -1, GENDER_NOT_APPLICABLE);
                  SStrCopy(temp, text, sizeof(temp));
                  SStrPrintf(buf, sizeof(buf), temp, durationString);
                  lua_pushstring(L, buf);
                  ++count;
                }
              }
            }
          }
          if (singleTarget) {
            for (i = 0; i < 3; ++i) {
              if (targetStrings[i][0]) {
                lua_pushstring(L, targetStrings[i]);
                ++count;
                break;
              }
            }
          }

          first = 1;
          singleTarget = 1;
          for (i = 0; i < 3; ++i) {
            targetStrings[i][0] = 0;
            if (spell->m_effect[i] && spell->m_effectAura[i]) {
              CGTooltip::GetSpellTargetString(targetStrings[i], sizeof(targetStrings[i]), spell, i);
              for (j = 0; j < i; ++j) {
                if (targetStrings[j][0] && SStrCmpI(targetStrings[i], targetStrings[j], 0x7FFFFFFF)) {
                  singleTarget = 0;
                }
              }
            }
          }
          for (i = 0; i < 3; ++i) {
            if (spell->m_effect[i] && spell->m_effectAura[i]) {
              char buf[128];
              CGTooltip::GetAuraEffectString(buf, sizeof(buf), spell, i, spellLevel, 0, TOOLTIP_DETAIL_NORMAL);
              if (buf[0]) {
                if (first) {
                  if (passive) {
                    LPCSTR text = FrameScript_GetText("SPELL_PASSIVE_EFFECT", -1, GENDER_NOT_APPLICABLE);
                    SStrCopy(temp, text, sizeof(temp));
                  } else {
                    SStrPrintf(temp, sizeof(temp), FrameScript_GetText("SPELL_LASTING_EFFECT", -1, GENDER_NOT_APPLICABLE), durationString);
                  }
                  lua_pushstring(L, temp);
                  ++count;
                  first = 0;
                }
                lua_pushstring(L, buf);
                ++count;
                if (!singleTarget) {
                  lua_pushstring(L, targetStrings[i]);
                  ++count;
                }
              }
            }
          }
          if (singleTarget) {
            for (i = 0; i < 3; ++i) {
              if (targetStrings[i][0]) {
                lua_pushstring(L, targetStrings[i]);
                ++count;
                break;
              }
            }
          }
        }
      }
    }
  }
  return count;
}

static int Script_BuyTrainerService(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: BuyTrainerService(index)");
    return 0;
  }
  UINT                      index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  const TrainerServiceInfo *service = CGClassTrainer::GetService(index);
  CGPlayer_C               *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (service && player) {
    player->TrainerBuySpell(CGClassTrainer::GetTrainer(), service->spellID);
  }
  return 0;
}

static int Script_GetTrainerServiceItemStats(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetTrainerServiceItemStats(index)");
    return 0;
  }
  UINT                      index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  const TrainerServiceInfo *info = CGClassTrainer::GetService(index);
  if (info) {
    const SpellRec *spell = g_spellDB.GetRecord(info->spellID);
    if (spell) {
      for (int i = 0; i < 3; ++i) {
        if (spell->m_effect[i] == 36 || spell->m_effect[i] == 57) {
          const SpellRec *learned = g_spellDB.GetRecord(spell->m_effectTriggerSpell[i]);
          if (learned && (learned->m_attributes & 0x20) && learned->m_effect[0] == 24) {
            const ItemSubClassRec *subClass;
            char                   levelBuf[128];
            int                    usable;
            char                   temp[128];
            int                    count;
            int                    itemID = learned->m_effectItemType[0];
            const ItemStats       *stats = g_itemDBCache.GetRecord(itemID, static_cast<DWORDLONG>(learned->m_ID) | 0xB000000000000000ui64, TrainerItemCallback, 0);
            CGPlayer_C *player;
            char        buf[128];
            if (!stats) {
              return 0;
            }

            count = 0;
            if (stats->m_inventoryType) {
              SStrPrintf(temp, sizeof(temp), "ITEM_QUALITY%d_DESC", stats->m_overallQualityID);
              LPCSTR text = FrameScript_GetText(temp, -1, GENDER_NOT_APPLICABLE);
              SStrCopy(buf, text, sizeof(buf));
              if (*text) {
                lua_pushstring(L, buf);
                ++count;
              }
            }

            buf[0] = 0;
            if (stats->m_bonding) {
              switch (stats->m_bonding) {
                case 1: {
                  LPCSTR text = FrameScript_GetText("ITEM_BIND_ON_PICKUP", -1, GENDER_NOT_APPLICABLE);
                  SStrCopy(buf, text, sizeof(buf));
                  break;
                }
                case 4:
                case 5: {
                  LPCSTR text = FrameScript_GetText("ITEM_BIND_QUEST", -1, GENDER_NOT_APPLICABLE);
                  SStrCopy(buf, text, sizeof(buf));
                  break;
                }
                case 2: {
                  LPCSTR text = FrameScript_GetText("ITEM_BIND_ON_EQUIP", -1, GENDER_NOT_APPLICABLE);
                  SStrCopy(buf, text, sizeof(buf));
                  break;
                }
                case 3: {
                  LPCSTR text = FrameScript_GetText("ITEM_BIND_ON_USE", -1, GENDER_NOT_APPLICABLE);
                  SStrCopy(buf, text, sizeof(buf));
                  break;
                }
              }
            }
            if (buf[0]) {
              lua_pushstring(L, buf);
              ++count;
            }

            if (stats->m_maxCount > 0 && stats->m_class != 1) {
              if (stats->m_maxCount == 1) {
                LPCSTR text = FrameScript_GetText("ITEM_UNIQUE", -1, GENDER_NOT_APPLICABLE);
                SStrCopy(buf, text, sizeof(buf));
              } else {
                LPCSTR text = FrameScript_GetText("ITEM_UNIQUE_MULTIPLE", -1, GENDER_NOT_APPLICABLE);
                SStrCopy(temp, text, sizeof(temp));
                SStrPrintf(buf, sizeof(buf), temp, stats->m_maxCount);
              }
              lua_pushstring(L, buf);
              ++count;
            }

            if (stats->m_startQuestID) {
              LPCSTR text = FrameScript_GetText("ITEM_STARTS_QUEST", -1, GENDER_NOT_APPLICABLE);
              SStrCopy(buf, text, sizeof(buf));
              lua_pushstring(L, buf);
              ++count;
            }

            subClass = 0;
            for (i = 0; i < g_itemSubClassDB.GetNumRecords(); ++i) {
              const ItemSubClassRec *rec = g_itemSubClassDB.GetRecordByIndex(i);
              if (rec && rec->m_classID == stats->m_class && rec->m_subClassID == stats->m_subclass) {
                subClass = rec;
                break;
              }
            }

            if (stats->m_inventoryType == 18) {
              if (subClass && subClass->m_displayName_lang[CURRENT_LANGUAGE] && *subClass->m_displayName_lang[CURRENT_LANGUAGE]) {
                LPCSTR text = FrameScript_GetText("CONTAINER_SLOTS", -1, GENDER_NOT_APPLICABLE);
                SStrCopy(temp, text, sizeof(temp));
                SStrPrintf(buf, sizeof(buf), temp, stats->m_containerSlots, subClass->m_displayName_lang[CURRENT_LANGUAGE]);
                lua_pushstring(L, buf);
                ++count;
              }
            } else {
              int  canUse = 1;
              int  hasTwoHanded = 1;
              UINT proficiency = CGPlayer_C::GetProficiency(stats->m_class);
              if (proficiency && !(proficiency & (1 << stats->m_subclass))) {
                if (stats->m_class == 2 && subClass->m_prerequisiteProficiency != -1) {
                  hasTwoHanded = 0;
                  if (!(proficiency & (1 << subClass->m_prerequisiteProficiency))) {
                    canUse = 0;
                  }
                } else {
                  canUse = 0;
                }
              }

              if (stats->m_class == 6) {
                const ItemClassRec *itemClass = g_itemClassDB.GetRecord(6);
                if (itemClass && itemClass->m_className_lang[CURRENT_LANGUAGE] && *itemClass->m_className_lang[CURRENT_LANGUAGE]) {
                  SStrPrintf(
                      buf, sizeof(buf), "%s%s%s", hasTwoHanded ? "" : "|cffff2020", itemClass->m_className_lang[CURRENT_LANGUAGE],
                      hasTwoHanded ? "" : "|r"
                  );
                  lua_pushstring(L, buf);
                  ++count;
                }
              } else {
                LPCSTR text = FrameScript_GetText(g_invTypeTokens[stats->m_inventoryType], -1, GENDER_NOT_APPLICABLE);
                SStrCopy(temp, text, sizeof(temp));
                if (*text) {
                  SStrPrintf(buf, sizeof(buf), "%s%s%s", hasTwoHanded ? "" : "|cffff2020", temp, hasTwoHanded ? "" : "|r");
                  lua_pushstring(L, buf);
                  ++count;
                }
              }

              if (subClass && subClass->m_displayName_lang[CURRENT_LANGUAGE] && *subClass->m_displayName_lang[CURRENT_LANGUAGE]) {
                SStrPrintf(
                    buf, sizeof(buf), "%s%s%s", canUse ? "" : "|cffff2020", subClass->m_displayName_lang[CURRENT_LANGUAGE], canUse ? "" : "|r"
                );
                lua_pushstring(L, buf);
                ++count;
              }
            }

            if (stats->m_minDamage[0] || stats->m_maxDamage[0]) {
              char school[64];
              if (stats->m_damageType[0]) {
                SStrPrintf(temp, sizeof(temp), "SPELL_SCHOOL%d_CAP", stats->m_damageType[0]);
                LPCSTR text = FrameScript_GetText(temp, -1, GENDER_NOT_APPLICABLE);
                SStrCopy(school, text, sizeof(school));
                SStrPack(school, " ", sizeof(school));
              }
              LPCSTR text = FrameScript_GetText("DAMAGE", -1, GENDER_NOT_APPLICABLE);
              SStrCopy(temp, text, sizeof(temp));
              SStrPrintf(
                  buf, sizeof(buf), "%d - %d %s%s", stats->m_minDamage[0], stats->m_maxDamage[0], stats->m_damageType[0] ? school : "", temp
              );
              lua_pushstring(L, buf);
              ++count;
              if (stats->m_class == 2) {
                text = FrameScript_GetText("SPEED", -1, GENDER_NOT_APPLICABLE);
                SStrCopy(temp, text, sizeof(temp));
                SStrPrintf(buf, sizeof(buf), "%s %d", temp, stats->m_delay / 100);
                lua_pushstring(L, buf);
                ++count;
              }
            }

            if (stats->m_resistances[0] > 0) {
              LPCSTR text = FrameScript_GetText("ARMOR", -1, GENDER_NOT_APPLICABLE);
              SStrCopy(temp, text, sizeof(temp));
              SStrPrintf(buf, sizeof(buf), "%d %s", stats->m_resistances[0], temp);
              lua_pushstring(L, buf);
              ++count;
            }

            int same = 1;
            for (i = 2; i < 6; ++i) {
              if (stats->m_resistances[i] != stats->m_resistances[1]) {
                same = 0;
                break;
              }
            }
            if (same) {
              if (stats->m_resistances[1]) {
                LPCSTR text = FrameScript_GetText("ITEM_RESIST_ALL", -1, GENDER_NOT_APPLICABLE);
                SStrCopy(temp, text, sizeof(temp));
                SStrPrintf(buf, sizeof(buf), temp, stats->m_resistances[1] > 0 ? '+' : '-', abs(stats->m_resistances[1]));
                lua_pushstring(L, buf);
                ++count;
              }
            } else {
              for (i = 1; i < 6; ++i) {
                if (stats->m_resistances[i]) {
                  char school[32];
                  SStrPrintf(temp, sizeof(temp), "SPELL_SCHOOL%d_CAP", i);
                  LPCSTR text = FrameScript_GetText(temp, -1, GENDER_NOT_APPLICABLE);
                  SStrCopy(school, text, sizeof(school));
                  text = FrameScript_GetText("ITEM_RESIST_SINGLE", -1, GENDER_NOT_APPLICABLE);
                  SStrCopy(temp, text, sizeof(temp));
                  SStrPrintf(buf, sizeof(buf), temp, stats->m_resistances[i] > 0 ? '+' : '-', abs(stats->m_resistances[i]), school);
                  lua_pushstring(L, buf);
                  ++count;
                }
              }
            }

            for (i = 0; i < 10; ++i) {
              if (stats->m_bonusAmount[i] && stats->m_bonusStat[i] != -1) {
                temp[0] = 0;
                switch (stats->m_bonusStat[i]) {
                  case 0: {
                    LPCSTR text = FrameScript_GetText("ITEM_MOD_MANA", -1, GENDER_NOT_APPLICABLE);
                    SStrCopy(temp, text, sizeof(temp));
                    break;
                  }
                  case 1: {
                    LPCSTR text = FrameScript_GetText("ITEM_MOD_HEALTH", -1, GENDER_NOT_APPLICABLE);
                    SStrCopy(temp, text, sizeof(temp));
                    break;
                  }
                  case 3: {
                    LPCSTR text = FrameScript_GetText("ITEM_MOD_AGILITY", -1, GENDER_NOT_APPLICABLE);
                    SStrCopy(temp, text, sizeof(temp));
                    break;
                  }
                  case 4: {
                    LPCSTR text = FrameScript_GetText("ITEM_MOD_STRENGTH", -1, GENDER_NOT_APPLICABLE);
                    SStrCopy(temp, text, sizeof(temp));
                    break;
                  }
                  case 5: {
                    LPCSTR text = FrameScript_GetText("ITEM_MOD_INTELLECT", -1, GENDER_NOT_APPLICABLE);
                    SStrCopy(temp, text, sizeof(temp));
                    break;
                  }
                  case 6: {
                    LPCSTR text = FrameScript_GetText("ITEM_MOD_SPIRIT", -1, GENDER_NOT_APPLICABLE);
                    SStrCopy(temp, text, sizeof(temp));
                    break;
                  }
                  case 7: {
                    LPCSTR text = FrameScript_GetText("ITEM_MOD_STAMINA", -1, GENDER_NOT_APPLICABLE);
                    SStrCopy(temp, text, sizeof(temp));
                    break;
                  }
                }
                if (temp[0]) {
                  SStrPrintf(buf, sizeof(buf), temp, stats->m_bonusAmount[i] > 0 ? '+' : '-', abs(stats->m_bonusAmount[i]));
                  lua_pushstring(L, buf);
                  ++count;
                }
              }
            }

            for (i = 0; i < 5; ++i) {
              if (stats->m_spellID[i] > 0) {
                const SpellRec *srec = g_spellDB.GetRecord(stats->m_spellID[i]);
                if (srec) {
                  int    charges = stats->m_spellCharges[i];
                  LPCSTR text = FrameScript_GetText("ITEM_SPELL_EFFECT", -1, GENDER_NOT_APPLICABLE);
                  SStrCopy(temp, text, sizeof(temp));
                  SStrPrintf(buf, sizeof(buf), temp, srec->m_name_lang[CURRENT_LANGUAGE]);
                  if (charges) {
                    char chargeBuf[64];
                    if (stats->m_spellCharges[i] == -1) {
                      text = FrameScript_GetText("ITEM_SPELL_CHARGE_SINGLE", -1, GENDER_NOT_APPLICABLE);
                      SStrCopy(chargeBuf, text, sizeof(chargeBuf));
                    } else {
                      text = FrameScript_GetText("ITEM_SPELL_CHARGES", -1, GENDER_NOT_APPLICABLE);
                      SStrCopy(temp, text, sizeof(temp));
                      SStrPrintf(chargeBuf, sizeof(chargeBuf), temp, abs(charges));
                    }
                    SStrPack(buf, chargeBuf, sizeof(buf));
                  }
                  lua_pushstring(L, buf);
                  ++count;
                }
              }
            }

            player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
            if (!player) {
              return count;
            }

            buf[0] = 0;
            usable = 1;
            if (!(stats->m_allowableRace & (stats->m_allowableRace - 1)) && !(stats->m_allowableClass & (stats->m_allowableClass - 1))) {
              for (i = 0; i < g_chrRacesDB.GetNumRecords(); ++i) {
                const ChrRacesRec *race = g_chrRacesDB.GetRecordByIndex(i);
                if (!(race->m_flags & 0x1) && (stats->m_allowableRace & (1 << (race->m_ID - 1)))) {
                  SStrPrintf(buf, sizeof(buf), "%s ", race->m_name_lang[CURRENT_LANGUAGE]);
                  if (player->GetRace() != race->m_ID) {
                    usable = 0;
                  }
                  break;
                }
              }
              for (i = 0; i < g_chrClassesDB.GetNumRecords(); ++i) {
                const ChrClassesRec *classRec = g_chrClassesDB.GetRecordByIndex(i);
                if (stats->m_allowableClass & (1 << (classRec->m_ID - 1))) {
                  SStrPack(buf, classRec->m_name_lang[CURRENT_LANGUAGE], sizeof(buf));
                  if (player->GetClass() != classRec->m_ID) {
                    usable = 0;
                  }
                  break;
                }
              }
              if (buf[0]) {
                char   string[128];
                LPCSTR text = FrameScript_GetText("RACE_CLASS_ONLY", -1, GENDER_NOT_APPLICABLE);
                SStrCopy(temp, text, sizeof(temp));
                SStrPrintf(string, sizeof(string), temp, buf);
                SStrPrintf(buf, sizeof(buf), "%s%s%s", usable ? "" : "|cffff2020", string, usable ? "" : "|r");
                lua_pushstring(L, buf);
                ++count;
              }
            } else {
              int allRaces = 1;
              int allClasses = 1;
              for (i = 0; i < g_chrRacesDB.GetNumRecords(); ++i) {
                const ChrRacesRec *race = g_chrRacesDB.GetRecordByIndex(i);
                if (!(race->m_flags & 0x1) && !(stats->m_allowableRace & (1 << (race->m_ID - 1)))) {
                  allRaces = 0;
                  break;
                }
              }
              for (i = 0; i < g_chrClassesDB.GetNumRecords(); ++i) {
                const ChrClassesRec *classRec = g_chrClassesDB.GetRecordByIndex(i);
                if (!(stats->m_allowableClass & (1 << (classRec->m_ID - 1)))) {
                  allClasses = 0;
                  break;
                }
              }

              if (!allRaces) {
                char races[512];
                char listBuf[512];
                races[0] = 0;
                int first = 1;
                usable = 0;
                for (i = 0; i < g_chrRacesDB.GetNumRecords(); ++i) {
                  const ChrRacesRec *race = g_chrRacesDB.GetRecordByIndex(i);
                  if (!(race->m_flags & 0x1) && (stats->m_allowableRace & (1 << (race->m_ID - 1)))) {
                    if (!first) {
                      SStrPack(races, ", ", sizeof(races));
                    }
                    SStrPack(races, race->m_name_lang[CURRENT_LANGUAGE], sizeof(races));
                    first = 0;
                    if (player->GetRace() == race->m_ID) {
                      usable = 1;
                    }
                  }
                }
                if (races[0]) {
                  LPCSTR text = FrameScript_GetText("ITEM_RACES_ALLOWED", -1, GENDER_NOT_APPLICABLE);
                  SStrCopy(temp, text, sizeof(temp));
                  SStrPrintf(listBuf, sizeof(listBuf), temp, races);
                  SStrPrintf(buf, sizeof(buf), "%s%s%s", usable ? "" : "|cffff2020", listBuf, usable ? "" : "|r");
                  lua_pushstring(L, buf);
                  ++count;
                }
              }

              if (!allClasses) {
                char classes[512];
                char listBuf[512];
                classes[0] = 0;
                int first = 1;
                usable = 0;
                for (i = 0; i < g_chrClassesDB.GetNumRecords(); ++i) {
                  const ChrClassesRec *classRec = g_chrClassesDB.GetRecordByIndex(i);
                  if (stats->m_allowableClass & (1 << (classRec->m_ID - 1))) {
                    if (!first) {
                      SStrPack(classes, ", ", sizeof(classes));
                    }
                    SStrPack(classes, classRec->m_name_lang[CURRENT_LANGUAGE], sizeof(classes));
                    first = 0;
                    if (player->GetClass() == classRec->m_ID) {
                      usable = 1;
                    }
                  }
                }
                if (classes[0]) {
                  LPCSTR text = FrameScript_GetText("ITEM_CLASSES_ALLOWED", -1, GENDER_NOT_APPLICABLE);
                  SStrCopy(temp, text, sizeof(temp));
                  SStrPrintf(listBuf, sizeof(listBuf), temp, classes);
                  SStrPrintf(buf, sizeof(buf), "%s%s%s", usable ? "" : "|cffff2020", listBuf, usable ? "" : "|r");
                  lua_pushstring(L, buf);
                  ++count;
                }
              }
            }

            int requiredLevel = stats->m_requiredLevel;
            int itemLevel = stats->m_itemLevel;
            if (requiredLevel > 0) {
              LPCSTR text = FrameScript_GetText("ITEM_LEVEL", -1, GENDER_NOT_APPLICABLE);
              SStrCopy(temp, text, sizeof(temp));
              SStrPrintf(levelBuf, sizeof(levelBuf), temp, itemLevel);
              lua_pushstring(L, levelBuf);
              ++count;
              if (requiredLevel > 1) {
                text = FrameScript_GetText("ITEM_MIN_LEVEL", -1, GENDER_NOT_APPLICABLE);
                SStrCopy(temp, text, sizeof(temp));
                SStrPrintf(levelBuf, sizeof(levelBuf), temp, requiredLevel);
                int canUse = player->GetLevel() >= requiredLevel;
                SStrPrintf(buf, sizeof(buf), "%s%s%s", canUse ? "" : "|cffff2020", levelBuf, canUse ? "" : "|r");
                lua_pushstring(L, buf);
                ++count;
              }
            }

            if (stats->m_requiredSkill > 0) {
              LPCSTR text = FrameScript_GetText(stats->m_requiredSkillRank ? "ITEM_MIN_SKILL" : "ITEM_REQ_SKILL", -1, GENDER_NOT_APPLICABLE);
              SStrCopy(temp, text, sizeof(temp));
              const SkillLineRec *skill = g_skillLineDB.GetRecord(stats->m_requiredSkill);
              if (stats->m_requiredSkillRank) {
                SStrPrintf(
                    levelBuf, sizeof(levelBuf), temp, skill ? skill->m_displayName_lang[CURRENT_LANGUAGE] : "UNKNOWN", stats->m_requiredSkillRank
                );
              } else {
                SStrPrintf(levelBuf, sizeof(levelBuf), temp, skill ? skill->m_displayName_lang[CURRENT_LANGUAGE] : "UNKNOWN");
              }
              int canUse = player->GetSkillRank(stats->m_requiredSkill) >= stats->m_requiredSkillRank;
              SStrPrintf(buf, sizeof(buf), "%s%s%s", canUse ? "" : "|cffff2020", levelBuf, canUse ? "" : "|r");
              lua_pushstring(L, buf);
              ++count;
            }
            return count;
          }
          return 0;
        }
      }
    }
  }
  return 0;
}

static TRAINER_SERVICE GetServiceTypeFromString(LPCSTR string) {
  if (!string) {
    return NUM_TRAINER_SERVICE_TYPES;
  }
  if (!SStrCmpI(string, "available", 0x7FFFFFFF)) {
    return TRAINER_SERVICE_AVAILABLE;
  }
  if (!SStrCmpI(string, "unavailable", 0x7FFFFFFF)) {
    return TRAINER_SERVICE_UNAVAILABLE;
  }
  return SStrCmpI(string, "used", 0x7FFFFFFF) ? NUM_TRAINER_SERVICE_TYPES : TRAINER_SERVICE_USED;
}

static int Script_SetTrainerServiceTypeFilter(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: SetTrainerServiceTypeFilter(\"type\" [, on\\off, exclusive])");
    return 0;
  }
  LPCSTR type = lua_tostring(L, 1);
  if (!SStrCmpI(type, "all", 0x7FFFFFFF)) {
    CGClassTrainer::SetServiceTypeFilter(7);
    return 0;
  }
  TRAINER_SERVICE serviceType = GetServiceTypeFromString(type);
  if (serviceType == NUM_TRAINER_SERVICE_TYPES) {
    luaL_error(L, "Bad service type in SetTrainerServiceTypeFilter");
    return 0;
  }
  if (!lua_isnumber(L, 2)) {
    luaL_error(
        L,
        "Missing on//off parameter in "
        "SetTrainerServiceTypeFilter"
    );
    return 0;
  }
  int filter = CGClassTrainer::GetServiceTypeFilter();
  if (!static_cast<int>(lua_tonumber(L, 2))) {
    CGClassTrainer::SetServiceTypeFilter(~(1 << serviceType) & filter);
    return 0;
  }
  if (lua_isnumber(L, 3) && static_cast<int>(lua_tonumber(L, 3))) {
    CGClassTrainer::SetServiceTypeFilter(1 << serviceType);
    return 0;
  }
  CGClassTrainer::SetServiceTypeFilter((1 << serviceType) | filter);
  return 0;
}

static int Script_SetTrainerSkillLineFilter(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: SetTrainerSkillLineFilter(index [, on\\off, exclusive])");
    return 0;
  }
  int index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  if (index < 0) {
    CGClassTrainer::SetSkillLineFilter(-1);
    return 0;
  }
  if (index >= static_cast<int>(CGClassTrainer::GetNumSkillLines())) {
    luaL_error(L, "Bad skill line in SetTrainerSkillLineFilter");
    return 0;
  }
  if (!lua_isnumber(L, 2)) {
    luaL_error(
        L,
        "Missing on//off parameter in "
        "SetTrainerSkillLineFilter"
    );
    return 0;
  }
  int filter = CGClassTrainer::GetSkillLineFilter();
  if (!static_cast<int>(lua_tonumber(L, 2))) {
    CGClassTrainer::SetSkillLineFilter(~(1 << index) & filter);
    return 0;
  }
  if (lua_isnumber(L, 3) && static_cast<int>(lua_tonumber(L, 3))) {
    CGClassTrainer::SetSkillLineFilter(1 << index);
    return 0;
  }
  CGClassTrainer::SetSkillLineFilter((1 << index) | filter);
  return 0;
}

static int Script_GetTrainerServiceTypeFilter(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: GetTrainerServiceTypeFilter(\"type\")");
    return 0;
  }
  TRAINER_SERVICE serviceType = GetServiceTypeFromString(lua_tostring(L, 1));
  if (serviceType == NUM_TRAINER_SERVICE_TYPES) {
    luaL_error(L, "Bad service type in GetTrainerServiceTypeFilter");
    return 0;
  }
  if ((1 << serviceType) & CGClassTrainer::GetServiceTypeFilter()) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_GetTrainerSkillLineFilter(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetTrainerSkillLineFilter(index)");
    return 0;
  }
  int filter = CGClassTrainer::GetSkillLineFilter();
  int index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  if (index < 0) {
    for (UINT i = 0; i < CGClassTrainer::GetNumSkillLines(); ++i) {
      if (!(filter & (1 << i))) {
        lua_pushnil(L);
        return 1;
      }
    }
    lua_pushnumber(L, 1.0);
    return 1;
  }
  if (index >= static_cast<int>(CGClassTrainer::GetNumSkillLines())) {
    luaL_error(L, "Bad skill line in GetTrainerSkillLineFilter");
    return 0;
  }
  if (filter & (1 << index)) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_GetTrainerSkillLines(lua_State *L) {
  UINT count = CGClassTrainer::GetNumSkillLines();
  for (UINT i = 0; i < count; ++i) {
    const SkillLineRec *line = g_skillLineDB.GetRecord(CGClassTrainer::GetSkillLine(i));
    lua_pushstring(L, line ? line->m_displayName_lang[CURRENT_LANGUAGE] : 0);
  }
  return count;
}

static int Script_CollapseTrainerSkillLine(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: CollapseTrainerSkillLine(index)");
    return 0;
  }
  int index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  if (index < 0) {
    CGClassTrainer::SetCollapseFilter(0);
  } else {
    int line = CGClassTrainer::GetSkillLineIndexFromService(index);
    if (line < 0) {
      luaL_error(L, "Bad skill line in CollapseTrainerSkillLine");
      return 0;
    }
    CGClassTrainer::SetCollapseFilter(CGClassTrainer::GetCollapseFilter() & ~(1 << line));
  }
  return 0;
}

static int Script_ExpandTrainerSkillLine(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: ExpandTrainerSkillLine(index)");
    return 0;
  }
  int index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  if (index < 0) {
    CGClassTrainer::SetCollapseFilter(-1);
  } else {
    int line = CGClassTrainer::GetSkillLineIndexFromService(index);
    if (line < 0) {
      luaL_error(L, "Bad skill line in ExpandTrainerSkillLine");
      return 0;
    }
    CGClassTrainer::SetCollapseFilter(CGClassTrainer::GetCollapseFilter() | (1 << line));
  }
  return 0;
}

static FrameScript_Method s_ScriptFunctions[33] = {
    {                   "OpenTrainer",                    Script_OpenTrainer},
    {                  "CloseTrainer",                   Script_CloseTrainer},
    {         "GetNumTrainerServices",          Script_GetNumTrainerServices},
    {         "GetTrainerServiceInfo",          Script_GetTrainerServiceInfo},
    {          "SelectTrainerService",           Script_SelectTrainerService},
    {           "IsTradeskillTrainer",            Script_IsTradeskillTrainer},
    {               "IsTalentTrainer",                Script_IsTalentTrainer},
    {      "GetTrainerSelectionIndex",       Script_GetTrainerSelectionIndex},
    {        "GetTrainerGreetingText",         Script_GetTrainerGreetingText},
    {         "GetTrainerServiceIcon",          Script_GetTrainerServiceIcon},
    {    "GetTrainerServiceSkillLine",     Script_GetTrainerServiceSkillLine},
    {         "GetTrainerServiceCost",          Script_GetTrainerServiceCost},
    {     "GetTrainerServiceLevelReq",      Script_GetTrainerServiceLevelReq},
    {     "GetTrainerServiceSkillReq",      Script_GetTrainerServiceSkillReq},
    {"GetTrainerServiceNumAbilityReq", Script_GetTrainerServiceNumAbilityReq},
    {   "GetTrainerServiceAbilityReq",    Script_GetTrainerServiceAbilityReq},
    {      "GetTrainerServiceStepReq",       Script_GetTrainerServiceStepReq},
    {  "GetTrainerServiceDescription",   Script_GetTrainerServiceDescription},
    {     "IsTrainerServiceSkillStep",      Script_IsTrainerServiceSkillStep},
    {    "IsTrainerServiceLearnSpell",     Script_IsTrainerServiceLearnSpell},
    {    "IsTrainerServiceTradeSkill",     Script_IsTrainerServiceTradeSkill},
    { "GetTrainerServiceStepIncrease",  Script_GetTrainerServiceStepIncrease},
    {   "GetTrainerServiceSpellStats",    Script_GetTrainerServiceSpellStats},
    {      "GetTrainerServiceEffects",       Script_GetTrainerServiceEffects},
    {    "GetTrainerServiceItemStats",     Script_GetTrainerServiceItemStats},
    {             "BuyTrainerService",              Script_BuyTrainerService},
    {   "SetTrainerServiceTypeFilter",    Script_SetTrainerServiceTypeFilter},
    {     "SetTrainerSkillLineFilter",      Script_SetTrainerSkillLineFilter},
    {   "GetTrainerServiceTypeFilter",    Script_GetTrainerServiceTypeFilter},
    {     "GetTrainerSkillLineFilter",      Script_GetTrainerSkillLineFilter},
    {          "GetTrainerSkillLines",           Script_GetTrainerSkillLines},
    {      "CollapseTrainerSkillLine",       Script_CollapseTrainerSkillLine},
    {        "ExpandTrainerSkillLine",         Script_ExpandTrainerSkillLine}
};

void ClassTrainerRegisterScriptFunctions() {
  for (UINT i = 0; i < 33; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void ClassTrainerUnregisterScriptFunctions() {
  for (UINT i = 0; i < 33; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
