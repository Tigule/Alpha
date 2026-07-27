#include "ClassTrainerFrame.h"
#include "GameUI.h"

#include "ObjectMgrClient/ObjectMgrClient.h"
#include "Object/ObjectClient/Player_C.h"
#include "DB/DBClient/AutoCode/SpellRec.h"
#include "DB/DBClient/AutoCode/SpellIconRec.h"
#include "DB/DBClient/DBCacheInstances.h"
#include "DB/DBClient/DBClient.h"
#include "Object/ItemStats.h"
#include "Object/ObjectClient/Item_C.h"
#include "DB/DBClient/AutoCode/SkillLineRec.h"
#include "DB/DBClient/AutoCode/SkillLineAbilityRec.h"
#include "SpellBookFrame.h"
#include "Net/NetClient/NetClient.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

#include <FrameScript/FrameScript.h>
#include <lauxlib.h>
#include <lua.h>

#include <stdlib.h>
#include <string.h>

static const float MAX_SHOP_DISTANCE = 5.5555553f;
static const float MAX_SHOP_DISTANCE_SQUARED = MAX_SHOP_DISTANCE * MAX_SHOP_DISTANCE;

unsigned __int64                        CGClassTrainer::m_trainer;
TRAINER_TYPE                            CGClassTrainer::m_trainerType;
int                                     CGClassTrainer::m_currentSelection;
unsigned int                            CGClassTrainer::m_numServices;
unsigned int                            CGClassTrainer::m_numSkillLines;
unsigned int                            CGClassTrainer::m_filteredServices;
int                                     CGClassTrainer::m_serviceTypeFilter;
int                                     CGClassTrainer::m_skillLineFilter;
int                                     CGClassTrainer::m_collapseFilter;
TSGrowableArray<TrainerServiceInfo *>   CGClassTrainer::m_services;
TSGrowableArray<TrainerSkillLineInfo *> CGClassTrainer::m_skillLines;
char                                    CGClassTrainer::m_greetingText[512];

void Spell_C_GetMinMaxPoints(const SpellRec *srec, int effectIndex, int *min, int *max, unsigned int level, int isPet);
int SpellParserParseText(const SpellRec *spell, char *buf, unsigned int size, int isPet);

static void TrainerItemCallback(int id, const unsigned __int64 &guid, void *arg, bool granted) {
  if (granted) {
    CGClassTrainer::RefreshList();
  }
}

static void TradeSkillItemCallback(int id, const unsigned __int64 &guid, void *arg, bool granted) {
  if (granted) {
    CGClassTrainer::RefreshList();
  }
}

void CGClassTrainer::InitializeGame() {
}

void CGClassTrainer::ShutdownGame() {
  unsigned int i;
  for (i = 0; i < m_services.Count(); ++i) {
    DEL(m_services[i]);
  }
  m_services.Clear();

  for (i = 0; i < m_skillLines.Count(); ++i) {
    DEL(m_skillLines[i]);
  }
  m_skillLines.Clear();
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

void CGClassTrainer::SetTrainer(unsigned __int64 trainerGUID, TRAINER_TYPE type) {
  if (trainerGUID) {
    if (trainerGUID != ClntObjMgrGetActivePlayer()) {
      CGGameUI::SetInteractTarget(trainerGUID, MAX_SHOP_DISTANCE_SQUARED);
    }
    m_trainerType = type;
    m_trainer = trainerGUID;
  } else {
    FrameScript_SignalEvent(288);
    CGGameUI::ClearInteractTarget(m_trainer);
    m_trainer = 0;
    m_numSkillLines = 0;
    m_numServices = 0;
  }
}

void CGClassTrainer::SetSelection(unsigned int index) {
  if (index < m_numServices) {
    m_currentSelection = m_services[index]->spellID;
  } else {
    m_currentSelection = 0;
  }
}

int CGClassTrainer::GetSelectionIndex() {
  if (!m_currentSelection) {
    return -1;
  }
  unsigned int index;
  for (index = 0; index < m_numServices; ++index) {
    if (m_services[index]->spellID == m_currentSelection) {
      break;
    }
  }
  return index == m_numServices ? -1 : index;
}

static int __cdecl QSortSkillLines(const void *a, const void *b) {
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
  SkillLineRec *line1 = g_skillLineDB.GetRecord(info1->skillLine);
  SkillLineRec *line2 = g_skillLineDB.GetRecord(info2->skillLine);
  if (!line1 || !line2) {
    return 0;
  }
  return SStrCmp(line1->m_displayName_lang[CURRENT_LANGUAGE], line2->m_displayName_lang[CURRENT_LANGUAGE], 0x7FFFFFFF);
}

int __cdecl QSortTradeSkillTypes(const void *a, const void *b) {
  FATALASSERT(a);
  FATALASSERT(b);
  int line1 = (*static_cast<TrainerSkillLineInfo *const *>(a))->skillLine;
  int line2 = (*static_cast<TrainerSkillLineInfo *const *>(b))->skillLine;
  if (line1 == line2) {
    return 0;
  }
  return line1 > line2 ? 1 : -1;
}

int __cdecl QSortServices_General(const void *a, const void *b) {
  FATALASSERT(a);
  FATALASSERT(b);
  TrainerServiceInfo *info1 = *static_cast<TrainerServiceInfo *const *>(a);
  TrainerServiceInfo *info2 = *static_cast<TrainerServiceInfo *const *>(b);
  if (info1->enabled != info2->enabled) {
    return info2->enabled ? 1 : -1;
  }
  unsigned int line1 = 0;
  unsigned int line2 = 0;
  for (unsigned int index = 0; index < CGClassTrainer::m_numSkillLines; ++index) {
    if (CGClassTrainer::m_skillLines[index]->skillLine == info1->skillLine) {
      line1 = index;
    }
    if (CGClassTrainer::m_skillLines[index]->skillLine == info2->skillLine) {
      line2 = index;
    }
  }
  if (line1 != line2) {
    return line1 > line2 ? 1 : -1;
  }
  if (info1->spellID < 0 || info2->spellID < 0) {
    return info1->spellID < 0 ? -1 : 1;
  }
  if (info1->reqLevel != info2->reqLevel) {
    return info1->reqLevel > info2->reqLevel ? 1 : -1;
  }
  if (info1->reqSkillRank != info2->reqSkillRank) {
    return info1->reqSkillRank > info2->reqSkillRank ? 1 : -1;
  }
  SpellRec *spell1 = g_spellDB.GetRecord(info1->spellID);
  SpellRec *spell2 = g_spellDB.GetRecord(info2->spellID);
  return spell1 && spell2 ? SStrCmp(spell1->m_name_lang[CURRENT_LANGUAGE], spell2->m_name_lang[CURRENT_LANGUAGE], 0x7FFFFFFF) : 0;
}

int __cdecl QSortServices_Tradeskill(const void *a, const void *b) {
  FATALASSERT(a);
  FATALASSERT(b);
  TrainerServiceInfo *info1 = *static_cast<TrainerServiceInfo *const *>(a);
  TrainerServiceInfo *info2 = *static_cast<TrainerServiceInfo *const *>(b);
  if (info1->enabled != info2->enabled) {
    return info2->enabled ? 1 : -1;
  }
  if (info1->skillLine != info2->skillLine) {
    return info1->skillLine > info2->skillLine ? 1 : -1;
  }
  if (info1->spellID < 0 || info2->spellID < 0) {
    return info1->spellID < 0 ? -1 : 1;
  }
  if (info1->reqSkillRank != info2->reqSkillRank) {
    return info1->reqSkillRank > info2->reqSkillRank ? 1 : -1;
  }
  SpellRec *spell1 = g_spellDB.GetRecord(info1->spellID);
  SpellRec *spell2 = g_spellDB.GetRecord(info2->spellID);
  return spell1 && spell2 ? SStrCmp(spell1->m_name_lang[CURRENT_LANGUAGE], spell2->m_name_lang[CURRENT_LANGUAGE], 0x7FFFFFFF) : 0;
}

int __cdecl QSortServices_Talent(const void *a, const void *b) {
  FATALASSERT(a);
  FATALASSERT(b);
  TrainerServiceInfo *info1 = *static_cast<TrainerServiceInfo *const *>(a);
  TrainerServiceInfo *info2 = *static_cast<TrainerServiceInfo *const *>(b);
  if (info1->enabled != info2->enabled) {
    return info2->enabled ? 1 : -1;
  }
  if (info1->skillLine != info2->skillLine) {
    return info1->skillLine > info2->skillLine ? 1 : -1;
  }
  if (info1->spellID < 0 || info2->spellID < 0) {
    return info1->spellID < 0 ? -1 : 1;
  }
  if (info1->usable != info2->usable) {
    return info1->usable > info2->usable ? 1 : -1;
  }
  SpellRec *spell1 = g_spellDB.GetRecord(info1->spellID);
  SpellRec *spell2 = g_spellDB.GetRecord(info2->spellID);
  return spell1 && spell2 ? SStrCmp(spell1->m_name_lang[CURRENT_LANGUAGE], spell2->m_name_lang[CURRENT_LANGUAGE], 0x7FFFFFFF) : 0;
}

int GetSkillLineFromService(int serviceSpell) {
  SpellRec *spell = g_spellDB.GetRecord(serviceSpell);
  if (!spell) {
    return 0;
  }

  int effectIndex = -1;
  int petSpell = 0;
  for (int index = 0; index < 3; ++index) {
    if (spell->m_effect[index] == 36 || spell->m_effect[index] == 57) {
      effectIndex = index;
      petSpell = spell->m_effect[index] == 57;
      break;
    }
  }

  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!unit) {
    return 0;
  }
  if (petSpell) {
    const CGUnitData *unitData = unit->GetUnitData();
    unsigned __int64  pet = unitData->charm ? unitData->charm : unitData->summon;
    unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(pet, __FILE__, __LINE__));
  }
  if (!unit) {
    return 0;
  }

  return unit->GetSpellSkillLine(effectIndex < 0 ? spell->m_ID : spell->m_effectTriggerSpell[effectIndex]);
}

void CGClassTrainer::AddServices(
    unsigned int   count,
    int           *spellID,
    unsigned int  *moneyCost,
    unsigned char **const pointCost,
    unsigned char  *reqLevel,
    unsigned int  *reqSkillLine,
    unsigned int  *reqSkillRank,
    unsigned int  *reqSkillStep,
    int          **const reqAbility,
    unsigned char  *usable,
    const char    *greeting
) {
  while (m_services.Count() < count) {
    TrainerServiceInfo *info = NEW(TrainerServiceInfo);
    m_services.Add(1, &info);
  }

  m_serviceTypeFilter = m_trainerType == TRAINER_TYPE_TALENTS ? 5 : 3;
  m_skillLineFilter = -1;
  m_collapseFilter = -1;
  m_numSkillLines = 0;

  if (m_trainerType == TRAINER_TYPE_TALENTS) {
    if (!m_skillLines.Count()) {
      TrainerSkillLineInfo *info = NEW(TrainerSkillLineInfo);
      m_skillLines.Add(1, &info);
    }
    TrainerSkillLineInfo *info = m_skillLines[0];
    memset(info, 0, sizeof(*info));
    info->skillLine = -1;
    info->ClearSkills();
    m_numSkillLines = 1;
  }

  unsigned int skipped = 0;
  for (unsigned int index = 0; index + skipped < count; ++index) {
    unsigned int        source = index + skipped;
    TrainerServiceInfo *info = m_services[index];
    memset(info, 0, sizeof(*info));
    info->spellID = spellID[source];
    info->moneyCost = moneyCost[source];
    for (unsigned int point = 0; point < 2; ++point) {
      info->pointCost[point] = pointCost[point][source];
    }
    info->reqLevel = reqLevel[source];
    info->reqSkillLine = reqSkillLine[source];
    info->reqSkillRank = reqSkillRank[source];
    info->reqSkillStep = reqSkillStep[source];
    for (unsigned int ability = 0; ability < 3; ++ability) {
      info->reqAbility[ability] = reqAbility[ability][source];
    }
    info->usable = usable[source];

    if (m_trainerType == TRAINER_TYPE_TRADESKILLS) {
      info->skillLine = 2;
      SpellRec *spell = g_spellDB.GetRecord(info->spellID);
      if (spell) {
        for (unsigned int effect = 0; effect < 3; ++effect) {
          if (spell->m_effect[effect] == 44) {
            info->skillLine = 1;
            break;
          }
        }
      }
    } else if (m_trainerType == TRAINER_TYPE_TALENTS && info->usable == 2) {
      info->skillLine = -1;
    } else {
      info->skillLine = GetSkillLineFromService(info->spellID);
    }

    if (!info->skillLine) {
      ++skipped;
      --index;
      continue;
    }

    TrainerSkillLineInfo *line = 0;
    for (unsigned int lineIndex = 0; lineIndex < m_numSkillLines; ++lineIndex) {
      if (m_skillLines[lineIndex]->skillLine == info->skillLine) {
        line = m_skillLines[lineIndex];
        break;
      }
    }
    if (!line) {
      if (m_skillLines.Count() <= m_numSkillLines) {
        line = NEW(TrainerSkillLineInfo);
        m_skillLines.Add(1, &line);
      } else {
        line = m_skillLines[m_numSkillLines];
      }
      memset(line, 0, sizeof(*line));
      line->skillLine = info->skillLine;
      line->collapsed = 1;
      line->allCostPoints = 0;
      for (unsigned int point = 0; point < 2; ++point) {
        if (info->pointCost[point]) {
          line->allCostPoints = 1;
        }
      }
      ++m_numSkillLines;
    } else if (line->allCostPoints) {
      line->allCostPoints = info->pointCost[0] || info->pointCost[1];
    }
    ++line->numSkills[info->usable];
  }

  m_numServices = count - skipped;
  qsort(
      m_skillLines.Ptr(), m_numSkillLines, sizeof(TrainerSkillLineInfo *),
      m_trainerType == TRAINER_TYPE_TRADESKILLS ? QSortTradeSkillTypes : QSortSkillLines
  );

  while (m_services.Count() < m_numServices + m_numSkillLines) {
    TrainerServiceInfo *info = NEW(TrainerServiceInfo);
    m_services.Add(1, &info);
  }
  for (unsigned int lineIndex = 0; lineIndex < m_numSkillLines; ++lineIndex) {
    TrainerServiceInfo *header = m_services[m_numServices + lineIndex];
    memset(header, 0, sizeof(*header));
    header->spellID = -1;
    header->skillLine = m_skillLines[lineIndex]->skillLine;
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
  unsigned int i;
  unsigned int j;

  if (!m_trainer) {
    return;
  }

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return;
  }

  for (i = 0; i < m_numServices; ++i) {
    TrainerServiceInfo *info = m_services[i];
    if (info->spellID == -1 || (info->usable == 2 && m_trainerType != TRAINER_TYPE_TALENTS)) {
      continue;
    }

    SpellRec *srec = g_spellDB.GetRecord(info->spellID);
    if (!srec) {
      continue;
    }

    info->usable = 0;
    CGUnit_C *pet = 0;
    int       numLearned = 0;
    int       numToLearn = 0;
    int       learnSpell = 0;

    for (j = 0; j < 3; ++j) {
      if (srec->m_effect[j] == 36) {
        ++numToLearn;
        learnSpell = srec->m_effectTriggerSpell[j];
        if (m_trainerType == TRAINER_TYPE_TALENTS && player->IsSpellSuperceded(learnSpell)) {
          info->usable = 3;
        } else if (player->IsSpellKnown(learnSpell) || player->IsSpellSuperceded(learnSpell)) {
          ++numLearned;
        }
      }

      if (srec->m_effect[j] == 44) {
        int min;
        int max;
        Spell_C_GetMinMaxPoints(srec, j, &min, &max, player->GetUnitData()->level, 0);
        for (unsigned int skill = 0; skill < 64; ++skill) {
          if (player->GetMirrorSkillID(skill) == srec->m_effectMiscValue[j] &&
              player->GetMirrorSkillStep(skill) >= min) {
            info->usable = 2;
            break;
          }
        }
      }

      if (srec->m_effect[j] == 57) {
        ++numToLearn;
        const CGUnitData *unitData = player->GetUnitData();
        unsigned __int64  petGUID = unitData->charm ? unitData->charm : unitData->summon;
        pet = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(petGUID, __FILE__, __LINE__));
        if (!pet) {
          info->usable = 1;
          continue;
        }

        learnSpell = srec->m_effectTriggerSpell[j];
        if (pet->IsSpellKnown(learnSpell) || pet->IsSpellSuperceded(learnSpell)) {
          ++numLearned;
        } else if (info->reqLevel > 1 && pet->GetUnitData()->level < info->reqLevel) {
          info->usable = 1;
        }
      }
    }

    if (numToLearn > 0 && numLearned == numToLearn) {
      info->usable = 2;
    }
    if (info->usable) {
      continue;
    }

    if (info->reqSkillLine) {
      int            maxRank = 0;
      for (j = 0; j < 64; ++j) {
        if (player->GetMirrorSkillID(j) != info->reqSkillLine) {
          continue;
        }
        if (player->GetMirrorSkillRank(j) < info->reqSkillRank) {
          info->usable = 1;
        }
        maxRank = player->GetMirrorSkillMaxRank(j);
        break;
      }

      SpellRec *steprec = g_spellDB.GetRecord(info->reqSkillStep);
      if (steprec) {
        for (j = 0; j < 3; ++j) {
          if (steprec->m_effect[j] != 44 || steprec->m_effectMiscValue[j] != info->reqSkillLine) {
            continue;
          }
          int min;
          int max;
          Spell_C_GetMinMaxPoints(steprec, j, &min, &max, 0, 0);
          if (maxRank < 5 * min) {
            info->usable = 1;
          }
          break;
        }
      }
    }

    if (info->usable) {
      continue;
    }

    CGUnit_C *unit = pet ? pet : player;
    for (j = 0; j < 3; ++j) {
      if (!info->reqAbility[j] || unit->IsSpellKnown(info->reqAbility[j]) || unit->IsSpellSuperceded(info->reqAbility[j])) {
        continue;
      }

      info->usable = 1;
      if (m_trainerType == TRAINER_TYPE_TALENTS && learnSpell) {
        const SkillLineAbilityRec *ability = player->LookupAbility(info->reqAbility[j]);
        if (ability && ability->m_supercededBySpell == learnSpell) {
          info->usable = 3;
        }
      }
      break;
    }

    if (!info->usable && info->reqLevel > 1 && unit->GetUnitData()->level < info->reqLevel) {
      info->usable = 1;
    }
  }

  for (i = 0; i < m_numSkillLines; ++i) {
    m_skillLines[i]->ClearSkills();
  }

  for (i = 0; i < m_numServices; ++i) {
    TrainerServiceInfo *info = m_services[i];
    if (info->spellID == -1) {
      continue;
    }
    if (m_trainerType == TRAINER_TYPE_TALENTS && info->usable == 2) {
      info->skillLine = -1;
    }
    for (j = 0; j < m_numSkillLines; ++j) {
      if (m_skillLines[j]->skillLine == info->skillLine) {
        ++m_skillLines[j]->numSkills[info->usable];
        break;
      }
    }
  }

  FilterAndSortServices();
  FrameScript_SignalEvent(287);
}

void CGClassTrainer::FilterAndSortServices() {
  m_filteredServices = m_numServices;
  for (unsigned int lineIndex = 0; lineIndex < m_numSkillLines; ++lineIndex) {
    TrainerSkillLineInfo *line = m_skillLines[lineIndex];
    int                   hasService = 0;
    for (unsigned int type = 0; type < NUM_TRAINER_SERVICE_TYPES; ++type) {
      if ((m_serviceTypeFilter & (1 << type)) && line->numSkills[type]) {
        hasService = 1;
        break;
      }
    }
    line->enabled = hasService && (m_skillLineFilter & (1 << lineIndex));
    line->collapsed = !(m_collapseFilter & (1 << lineIndex));
  }

  for (unsigned int index = 0; index < m_numServices; ++index) {
    TrainerServiceInfo *info = m_services[index];
    info->enabled = info->spellID < 0 || (m_serviceTypeFilter & (1 << info->usable));
    TrainerSkillLineInfo *line = 0;
    for (unsigned int lineIndex = 0; lineIndex < m_numSkillLines; ++lineIndex) {
      if (m_skillLines[lineIndex]->skillLine == info->skillLine) {
        line = m_skillLines[lineIndex];
        break;
      }
    }
    if (!info->enabled || !line || !line->enabled || (info->spellID >= 0 && !line->collapsed)) {
      info->enabled = 0;
      --m_filteredServices;
    }
  }

  int(__cdecl * compare)(const void *, const void *) = QSortServices_General;
  if (m_trainerType == TRAINER_TYPE_TALENTS) {
    compare = QSortServices_Talent;
  } else if (m_trainerType == TRAINER_TYPE_TRADESKILLS) {
    compare = QSortServices_Tradeskill;
  }
  qsort(m_services.Ptr(), m_numServices, sizeof(TrainerServiceInfo *), compare);
}

const TrainerServiceInfo *CGClassTrainer::GetService(unsigned int index) {
  return index < m_numServices ? m_services[index] : 0;
}

const char *CGClassTrainer::GetServiceName(unsigned int index) {
  const TrainerServiceInfo *service = GetService(index);
  if (!service) {
    return 0;
  }
  if (service->spellID >= 0) {
    const SpellRec *spell = g_spellDB.GetRecord(service->spellID);
    return spell ? spell->m_name_lang[CURRENT_LANGUAGE] : 0;
  }
  if (m_trainerType == TRAINER_TYPE_TRADESKILLS) {
    return FrameScript_GetText(service->skillLine == 2 ? "TRAINER_GENERAL_SKILLS" : "TRAINER_TRADESKILLS", -1, GENDER_NOT_APPLICABLE);
  }
  const SkillLineRec *line = g_skillLineDB.GetRecord(service->skillLine);
  return line ? line->m_displayName_lang[CURRENT_LANGUAGE] : 0;
}

const char *CGClassTrainer::GetServiceSubtext(unsigned int index) {
  const TrainerServiceInfo *service = GetService(index);
  if (!service || service->spellID < 0) {
    return service ? "" : 0;
  }
  const SpellRec *spell = g_spellDB.GetRecord(service->spellID);
  return spell ? spell->m_nameSubtext_lang[CURRENT_LANGUAGE] : 0;
}

const char *CGClassTrainer::GetServiceType(unsigned int index) {
  const TrainerServiceInfo *service = GetService(index);
  if (!service) {
    return 0;
  }
  if (service->spellID < 0) {
    return "header";
  }
  if (!service->usable) {
    return "available";
  }
  return service->usable == 2 ? "used" : "unavailable";
}

int CGClassTrainer::GetSkillLineIndexFromService(unsigned int index) {
  const TrainerServiceInfo *service = GetService(index);
  if (!service || service->spellID >= 0) {
    return -1;
  }
  for (unsigned int line = 0; line < m_numSkillLines; ++line) {
    if (m_skillLines[line]->skillLine == service->skillLine) {
      return line;
    }
  }
  return -1;
}

int CGClassTrainer::IsCollpasedHeader(unsigned int index) {
  int line = GetSkillLineIndexFromService(index);
  return line >= 0 && !(m_collapseFilter & (1 << line));
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

static int Script_OpenTrainer(lua_State *__formal) {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (player) {
    player->TalkToTrainer(player->GetUnitData()->target);
  }
  return 0;
}

static int Script_CloseTrainer(lua_State *__formal) {
  CGClassTrainer::SetTrainer(0, TRAINER_TYPE_GENERAL);
  return 0;
}

static int Script_GetNumTrainerServices(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGClassTrainer::GetNumServices()));
  return 1;
}

static int Script_GetTrainerServiceInfo(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetTrainerServiceInfo(index)");
  }
  unsigned int index = static_cast<unsigned int>(lua_tonumber(L, 1)) - 1;
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
    return luaL_error(L, "Usage: SelectTrainerService(index)");
  }
  CGClassTrainer::SetSelection(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
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
    return luaL_error(L, "Usage: GetTrainerServiceIcon(index)");
  }
  const TrainerServiceInfo *service = CGClassTrainer::GetService(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  const SpellRec     *spell = service ? g_spellDB.GetRecord(service->spellID) : 0;
  if (spell && CGClassTrainer::GetTrainerType() == TRAINER_TYPE_TRADESKILLS) {
    for (unsigned int i = 0; i < 3; ++i) {
      if (spell->m_effect[i] == 36 || spell->m_effect[i] == 57) {
        const SpellRec *learned = g_spellDB.GetRecord(spell->m_effectTriggerSpell[i]);
        if (learned && learned->m_effectItemType[0]) {
          unsigned __int64   guid = static_cast<unsigned __int64>(learned->m_ID) | 0xB000000000000000ui64;
          const ItemStats_C *stats = g_itemDBCache.GetRecord(learned->m_effectItemType[0], guid, TradeSkillItemCallback, 0);
          if (stats) {
            char        buffer[260];
            const char *path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
            SStrPrintf(buffer, sizeof(buffer), "%s%s", path, *path ? "\\" : "");
            SStrPack(buffer, CGItem_C::GetInventoryArt(stats->m_displayInfoID), sizeof(buffer));
            lua_pushstring(L, buffer);
            return 1;
          }
        }
        break;
      }
    }
  }
  const SpellIconRec *icon = spell ? g_spellIconDB.GetRecord(spell->m_spellIconID) : 0;
  lua_pushstring(L, icon ? icon->m_textureFilename : 0);
  return 1;
}

static int Script_GetTrainerServiceSkillLine(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetTrainerServiceSkillLine(index)");
  }
  const TrainerServiceInfo *service = CGClassTrainer::GetService(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  const SkillLineRec *line = service ? g_skillLineDB.GetRecord(GetSkillLineFromService(service->spellID)) : 0;
  lua_pushstring(L, line ? line->m_displayName_lang[CURRENT_LANGUAGE] : 0);
  return 1;
}

static int Script_GetTrainerServiceCost(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetTrainerServiceCost(index)");
  }
  const TrainerServiceInfo *service = CGClassTrainer::GetService(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  lua_pushnumber(L, service ? static_cast<double>(service->moneyCost) : 0.0);
  lua_pushnumber(L, service ? static_cast<double>(service->pointCost[0]) : 0.0);
  lua_pushnumber(L, service ? static_cast<double>(service->pointCost[1]) : 0.0);
  return 3;
}

static int Script_GetTrainerServiceLevelReq(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetTrainerServiceLevelReq(index)");
  }
  const TrainerServiceInfo *service = CGClassTrainer::GetService(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  lua_pushnumber(L, service ? static_cast<double>(service->reqLevel) : 0.0);
  return 1;
}

static int Script_GetTrainerServiceSkillReq(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetTrainerServiceSkillReq(index)");
  }
  const TrainerServiceInfo *service = CGClassTrainer::GetService(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  CGPlayer_C         *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  const SkillLineRec *line = service ? g_skillLineDB.GetRecord(service->reqSkillLine) : 0;
  lua_pushstring(L, line ? line->m_displayName_lang[CURRENT_LANGUAGE] : 0);
  lua_pushnumber(L, service ? static_cast<double>(service->reqSkillRank) : 0.0);
  if (service && player && player->GetSkillRank(service->reqSkillLine) >= static_cast<int>(service->reqSkillRank)) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 3;
}

static int Script_GetTrainerServiceNumAbilityReq(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetTrainerServiceAbilityReq(index)");
  }
  const TrainerServiceInfo *service = CGClassTrainer::GetService(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  unsigned int        count = 0;
  if (service) {
    for (unsigned int i = 0; i < 3; ++i) {
      if (service->reqAbility[i] > 0) {
        ++count;
      }
    }
  }
  lua_pushnumber(L, static_cast<double>(count));
  return 1;
}

static int Script_GetTrainerServiceAbilityReq(lua_State *L) {
  if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
    return luaL_error(L, "Usage: GetTrainerServiceAbilityReq(index)");
  }
  const TrainerServiceInfo *service = CGClassTrainer::GetService(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  unsigned int        abilityIndex = static_cast<unsigned int>(lua_tonumber(L, 2)) - 1;
  const SpellRec     *spell = service && abilityIndex < 3 ? g_spellDB.GetRecord(service->reqAbility[abilityIndex]) : 0;
  char                ability[256];
  ability[0] = 0;
  if (spell) {
    if (spell->m_nameSubtext_lang[CURRENT_LANGUAGE] && *spell->m_nameSubtext_lang[CURRENT_LANGUAGE]) {
      SStrPrintf(ability, sizeof(ability), "%s (%s)", spell->m_name_lang[CURRENT_LANGUAGE], spell->m_nameSubtext_lang[CURRENT_LANGUAGE]);
    } else {
      SStrCopy(ability, spell->m_name_lang[CURRENT_LANGUAGE], sizeof(ability));
    }
  }
  lua_pushstring(L, *ability ? ability : 0);
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  CGUnit_C   *unit = player;
  const SpellRec *trainerSpell = service ? g_spellDB.GetRecord(service->spellID) : 0;
  for (unsigned int i = 0; trainerSpell && i < 3; ++i) {
    if (trainerSpell->m_effect[i] == 57) {
      const CGUnitData *unitData = player ? player->GetUnitData() : 0;
      unsigned __int64 pet = unitData ? unitData->summon : 0;
      unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(pet, __FILE__, __LINE__));
      break;
    }
  }
  if (!spell || !unit || unit->IsSpellKnown(spell->m_ID) || unit->IsSpellSuperceded(spell->m_ID)) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 2;
}

static int Script_GetTrainerServiceStepReq(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetTrainerServiceStepReq(index)");
  }
  int                 found = 1;
  CGPlayer_C         *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  const TrainerServiceInfo *service = CGClassTrainer::GetService(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  const SpellRec     *spell = service ? g_spellDB.GetRecord(service->reqSkillStep) : 0;
  if (player && spell) {
    found = 0;
    for (unsigned int effect = 0; effect < 3; ++effect) {
      if (spell->m_effect[effect] != 44) {
        continue;
      }
      for (unsigned int skill = 0; skill < 64; ++skill) {
        if (player->GetMirrorSkillID(skill) == spell->m_effectMiscValue[effect]) {
          int min;
          int max;
          Spell_C_GetMinMaxPoints(spell, effect, &min, &max, 0, 0);
          if (player->GetMirrorSkillMaxRank(skill) >= 5 * max) {
            found = 1;
          }
          break;
        }
      }
      break;
    }
  }
  if (spell && spell->m_name_lang[CURRENT_LANGUAGE] && *spell->m_name_lang[CURRENT_LANGUAGE]) {
    lua_pushstring(L, spell->m_name_lang[CURRENT_LANGUAGE]);
  } else {
    lua_pushnil(L);
  }
  if (found) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 2;
}

static const SpellRec *GetLearnedSpell(const TrainerServiceInfo *service, int *learnEffect) {
  const SpellRec *trainer = service ? g_spellDB.GetRecord(service->spellID) : 0;
  if (!trainer) {
    return 0;
  }
  for (int i = 0; i < 3; ++i) {
    if (trainer->m_effect[i] == 36 || trainer->m_effect[i] == 57) {
      if (learnEffect) {
        *learnEffect = i;
      }
      return g_spellDB.GetRecord(trainer->m_effectTriggerSpell[i]);
    }
  }
  return 0;
}

static int Script_GetTrainerServiceDescription(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetTrainerServiceDescription(index)");
  }
  const TrainerServiceInfo *service = CGClassTrainer::GetService(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  int                 learnEffect = -1;
  const SpellRec     *learned = GetLearnedSpell(service, &learnEffect);
  const SpellRec     *trainer = service ? g_spellDB.GetRecord(service->spellID) : 0;
  const SpellRec     *description =
      learned && learned->m_description_lang[CURRENT_LANGUAGE] && *learned->m_description_lang[CURRENT_LANGUAGE] ? learned : trainer;
  if (description && description->m_description_lang[CURRENT_LANGUAGE] && *description->m_description_lang[CURRENT_LANGUAGE]) {
    char buf[1024];
    SpellParserParseText(description, buf, sizeof(buf), trainer && learnEffect >= 0 && trainer->m_effect[learnEffect] == 57);
    lua_pushstring(L, buf);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_IsTrainerServiceSkillStep(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: IsTrainerServiceSkillStep(index)");
  }
  const TrainerServiceInfo *service = CGClassTrainer::GetService(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  const SpellRec     *spell = service ? g_spellDB.GetRecord(service->spellID) : 0;
  int                 found = 0;
  for (unsigned int i = 0; spell && i < 3; ++i) {
    if (spell->m_effect[i] == 44) {
      found = 1;
      break;
    }
  }
  if (found) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_IsTrainerServiceLearnSpell(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: IsTrainerServiceLearnSpell(index)");
  }
  const TrainerServiceInfo *service = CGClassTrainer::GetService(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  const SpellRec     *trainer = service ? g_spellDB.GetRecord(service->spellID) : 0;
  int                 learn = 0;
  int                 pet = 0;
  for (unsigned int i = 0; trainer && i < 3; ++i) {
    if (trainer->m_effect[i] == 36 || trainer->m_effect[i] == 57) {
      learn = 1;
      pet = trainer->m_effect[i] == 57;
      break;
    }
  }
  if (learn) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  if (pet) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 2;
}

static int Script_IsTrainerServiceTradeSkill(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: IsTrainerServiceTradeSkill(index)");
  }
  const TrainerServiceInfo *service = CGClassTrainer::GetService(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  const SpellRec     *learned = GetLearnedSpell(service, 0);
  if (learned && (learned->m_attributes & 0x20)) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_GetTrainerServiceStepIncrease(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetTrainerServiceStepIncrease(index)");
  }
  const TrainerServiceInfo *service = CGClassTrainer::GetService(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  const SpellRec     *spell = service ? g_spellDB.GetRecord(service->spellID) : 0;
  for (unsigned int i = 0; spell && i < 3; ++i) {
    if (spell->m_effect[i] == 44) {
      int min;
      int max;
      Spell_C_GetMinMaxPoints(spell, i, &min, &max, 0, 0);
      const SkillLineRec *line = g_skillLineDB.GetRecord(spell->m_effectMiscValue[i]);
      char                buf[256];
      SStrPrintf(buf, sizeof(buf), "%s (%d)", line ? line->m_displayName_lang[CURRENT_LANGUAGE] : "", 5 * min);
      lua_pushstring(L, buf);
      return 1;
    }
  }
  lua_pushnil(L);
  return 1;
}

static int Script_GetTrainerServiceSpellStats(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetTrainerServiceSpellStats(index)");
  }
  const TrainerServiceInfo *service = CGClassTrainer::GetService(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  const SpellRec     *spell = GetLearnedSpell(service, 0);
  if (!spell) {
    for (int i = 0; i < 5; ++i) {
      lua_pushnil(L);
    }
    return 5;
  }
  if (spell->m_attributes & 0x40) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  char buf[256];
  SStrPrintf(buf, sizeof(buf), "%d", spell->m_manaCost);
  lua_pushstring(L, buf);
  SStrPrintf(buf, sizeof(buf), "%d", spell->m_rangeIndex);
  lua_pushstring(L, buf);
  SStrPrintf(buf, sizeof(buf), "%d", spell->m_castingTimeIndex);
  lua_pushstring(L, buf);
  SStrPrintf(buf, sizeof(buf), "%d", spell->m_recoveryTime > spell->m_categoryRecoveryTime ? spell->m_recoveryTime : spell->m_categoryRecoveryTime);
  lua_pushstring(L, buf);
  return 5;
}

static int Script_GetTrainerServiceEffects(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetTrainerServiceEffects(index)");
  }
  const TrainerServiceInfo *service = CGClassTrainer::GetService(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  const SpellRec     *spell = GetLearnedSpell(service, 0);
  if (!spell) {
    return 0;
  }
  int count = 0;
  if (spell->m_description_lang[CURRENT_LANGUAGE] && *spell->m_description_lang[CURRENT_LANGUAGE]) {
    char buf[1024];
    SpellParserParseText(spell, buf, sizeof(buf), 0);
    lua_pushstring(L, buf);
    ++count;
  }
  return count;
}

static int Script_BuyTrainerService(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: BuyTrainerService(index)");
  }
  const TrainerServiceInfo *service = CGClassTrainer::GetService(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  CGPlayer_C         *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (service && player) {
    player->TrainerBuySpell(CGClassTrainer::GetTrainer(), service->spellID);
  }
  return 0;
}

static int Script_GetTrainerServiceItemStats(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetTrainerServiceItemStats(index)");
  }
  const TrainerServiceInfo *service = CGClassTrainer::GetService(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  const SpellRec     *spell = GetLearnedSpell(service, 0);
  if (!spell || !(spell->m_attributes & 0x20) || spell->m_effect[0] != 24 || !spell->m_effectItemType[0]) {
    return 0;
  }
  unsigned __int64   guid = static_cast<unsigned __int64>(spell->m_ID) | 0xB000000000000000ui64;
  const ItemStats_C *stats = g_itemDBCache.GetRecord(spell->m_effectItemType[0], guid, TrainerItemCallback, 0);
  if (!stats) {
    return 0;
  }
  int count = 0;
  if (stats->m_displayName[0] && *stats->m_displayName[0]) {
    lua_pushstring(L, stats->m_displayName[0]);
    ++count;
  }
  if (stats->m_description && *stats->m_description) {
    lua_pushstring(L, stats->m_description);
    ++count;
  }
  if (stats->m_requiredLevel) {
    char buf[128];
    SStrPrintf(buf, sizeof(buf), "Requires Level %d", stats->m_requiredLevel);
    lua_pushstring(L, buf);
    ++count;
  }
  return count;
}

static TRAINER_SERVICE GetServiceTypeFromString(const char *string) {
  if (!string) {
    return NUM_TRAINER_SERVICE_TYPES;
  }
  if (!SStrCmp(string, "available", 0x7FFFFFFF)) {
    return TRAINER_SERVICE_AVAILABLE;
  }
  if (!SStrCmp(string, "unavailable", 0x7FFFFFFF)) {
    return TRAINER_SERVICE_UNAVAILABLE;
  }
  if (!SStrCmp(string, "used", 0x7FFFFFFF)) {
    return TRAINER_SERVICE_USED;
  }
  return NUM_TRAINER_SERVICE_TYPES;
}

static int Script_SetTrainerServiceTypeFilter(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    return luaL_error(L, "Usage: SetTrainerServiceTypeFilter(\"type\" [, on\\off, exclusive])");
  }
  const char *type = lua_tostring(L, 1);
  if (!SStrCmp(type, "all", 0x7FFFFFFF)) {
    CGClassTrainer::SetServiceTypeFilter(7);
    return 0;
  }
  TRAINER_SERVICE serviceType = GetServiceTypeFromString(type);
  if (serviceType == NUM_TRAINER_SERVICE_TYPES) {
    return luaL_error(L, "Bad service type in SetTrainerServiceTypeFilter");
  }
  if (!lua_isnumber(L, 2)) {
    return luaL_error(
        L,
        "Missing on//off parameter in "
        "SetTrainerServiceTypeFilter"
    );
  }
  int filter = CGClassTrainer::GetServiceTypeFilter();
  if (static_cast<unsigned int>(lua_tonumber(L, 2))) {
    filter = lua_isnumber(L, 3) && lua_tonumber(L, 3) ? 1 << serviceType : filter | (1 << serviceType);
  } else {
    filter &= ~(1 << serviceType);
  }
  CGClassTrainer::SetServiceTypeFilter(filter);
  return 0;
}

static int Script_SetTrainerSkillLineFilter(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: SetTrainerSkillLineFilter(index [, on\\off, exclusive])");
  }
  int index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  if (index < 0) {
    CGClassTrainer::SetSkillLineFilter(-1);
    return 0;
  }
  if (static_cast<unsigned int>(index) >= CGClassTrainer::GetNumSkillLines()) {
    return luaL_error(L, "Bad skill line in SetTrainerSkillLineFilter");
  }
  if (!lua_isnumber(L, 2)) {
    return luaL_error(
        L,
        "Missing on//off parameter in "
        "SetTrainerSkillLineFilter"
    );
  }
  int filter = CGClassTrainer::GetSkillLineFilter();
  if (static_cast<unsigned int>(lua_tonumber(L, 2))) {
    filter = lua_isnumber(L, 3) && lua_tonumber(L, 3) ? 1 << index : filter | (1 << index);
  } else {
    filter &= ~(1 << index);
  }
  CGClassTrainer::SetSkillLineFilter(filter);
  return 0;
}

static int Script_GetTrainerServiceTypeFilter(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    return luaL_error(L, "Usage: GetTrainerServiceTypeFilter(\"type\")");
  }
  TRAINER_SERVICE serviceType = GetServiceTypeFromString(lua_tostring(L, 1));
  if (serviceType == NUM_TRAINER_SERVICE_TYPES) {
    return luaL_error(L, "Bad service type in GetTrainerServiceTypeFilter");
  }
  if (CGClassTrainer::GetServiceTypeFilter() & (1 << serviceType)) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_GetTrainerSkillLineFilter(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetTrainerSkillLineFilter(index)");
  }
  int index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  int enabled;
  if (index < 0) {
    enabled = CGClassTrainer::GetSkillLineFilter() == static_cast<int>((1u << CGClassTrainer::GetNumSkillLines()) - 1);
  } else {
    if (static_cast<unsigned int>(index) >= CGClassTrainer::GetNumSkillLines()) {
      return luaL_error(L, "Bad skill line in GetTrainerSkillLineFilter");
    }
    enabled = (CGClassTrainer::GetSkillLineFilter() & (1 << index)) != 0;
  }
  if (enabled) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_GetTrainerSkillLines(lua_State *L) {
  unsigned int count = CGClassTrainer::GetNumSkillLines();
  for (unsigned int i = 0; i < count; ++i) {
    const SkillLineRec *line = g_skillLineDB.GetRecord(CGClassTrainer::GetSkillLine(i));
    lua_pushstring(L, line ? line->m_displayName_lang[CURRENT_LANGUAGE] : 0);
  }
  return count;
}

static int Script_CollapseTrainerSkillLine(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: CollapseTrainerSkillLine(index)");
  }
  int index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  if (index < 0) {
    CGClassTrainer::SetCollapseFilter(0);
  } else {
    int line = CGClassTrainer::GetSkillLineIndexFromService(index);
    if (line < 0) {
      return luaL_error(L, "Bad skill line in CollapseTrainerSkillLine");
    }
    CGClassTrainer::SetCollapseFilter(CGClassTrainer::GetCollapseFilter() & ~(1 << line));
  }
  return 0;
}

static int Script_ExpandTrainerSkillLine(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: ExpandTrainerSkillLine(index)");
  }
  int index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  if (index < 0) {
    CGClassTrainer::SetCollapseFilter(-1);
  } else {
    int line = CGClassTrainer::GetSkillLineIndexFromService(index);
    if (line < 0) {
      return luaL_error(L, "Bad skill line in ExpandTrainerSkillLine");
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
    {             "BuyTrainerService",              Script_BuyTrainerService},
    {    "GetTrainerServiceItemStats",     Script_GetTrainerServiceItemStats},
    {   "SetTrainerServiceTypeFilter",    Script_SetTrainerServiceTypeFilter},
    {     "SetTrainerSkillLineFilter",      Script_SetTrainerSkillLineFilter},
    {   "GetTrainerServiceTypeFilter",    Script_GetTrainerServiceTypeFilter},
    {     "GetTrainerSkillLineFilter",      Script_GetTrainerSkillLineFilter},
    {          "GetTrainerSkillLines",           Script_GetTrainerSkillLines},
    {      "CollapseTrainerSkillLine",       Script_CollapseTrainerSkillLine},
    {        "ExpandTrainerSkillLine",         Script_ExpandTrainerSkillLine}
};

void ClassTrainerRegisterScriptFunctions() {
  for (unsigned int i = 0; i < 33; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void ClassTrainerUnregisterScriptFunctions() {
  for (unsigned int i = 0; i < 33; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
