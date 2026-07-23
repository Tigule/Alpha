#include "DB/DBClient/DBCacheInstances.h"
#include "DB/DBClient/AutoCode/ItemSubClassRec.h"
#include "DB/DBClient/AutoCode/SkillLineAbilityRec.h"
#include "DB/DBClient/AutoCode/SkillLineRec.h"
#include "DB/DBClient/AutoCode/SpellFocusObjectRec.h"
#include "DB/DBClient/AutoCode/SpellIconRec.h"
#include "DB/DBClient/AutoCode/SpellRec.h"
#include "DB/DBClient/DBClient.h"
#include "Object/ObjectClient/Bag_C.h"
#include "Object/ObjectClient/Item_C.h"
#include "Object/ObjectClient/Player_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"

#include <FrameScript/FrameScript.h>
#include <stpl.h>
#include <stdlib.h>
#include <lauxlib.h>
#include <lua.h>

struct TradeSkillInfo {
  int spellID;
  int category;
  int classID;
  int subClassID;
  int invSlots;
  int itemLevel;
  int numAvailable;
  int enabled;
};

struct TradeSkillSubClassInfo {
  int classID;
  int subClassID;
  int filteredCount;
  int enabled;
  int collapsed;
};

static int __cdecl QSortSkills(const void *a, const void *b);
static int __cdecl QSortSubClasses(const void *a, const void *b);

const SkillLineAbilityRec *__fastcall SpellTableLookupAbility(unsigned int raceID, unsigned int classID, unsigned int spellID);
extern const int *const               g_ITEMTYPEARRAY;

class CGTradeSkillInfo {
 public:
  static void __fastcall EnterWorld();
  static void __fastcall LeaveWorld();
  static void __fastcall ShutdownGame();
  static void __fastcall Close();
  static void __fastcall ClearItemCallbacks();
  static void __fastcall RefreshList(int resetFilters);
  static void __fastcall DecrementPendingItem() {
    if (!m_itemsPending || !--m_itemsPending) {
      RefreshList(1);
    }
  }
  static void __fastcall SetSkillLine(int id);
  static void __fastcall SetSelection(int index);
  static int __fastcall  GetSelectionIndex();
  static int __fastcall  GetSkillLine() {
    return m_skillLine;
  }
  static int __fastcall GetNumTradeSkills() {
    return m_filteredSkills;
  }
  static TradeSkillInfo *__fastcall GetTradeSkillInfo(unsigned int index) {
    return index < m_numSkills ? m_skills[index] : 0;
  }
  static unsigned int __fastcall GetNumSubClasses() {
    return m_numSubClasses;
  }
  static TradeSkillSubClassInfo *__fastcall GetSubClass(unsigned int index) {
    return index < m_numSubClasses ? m_subClasses[index] : 0;
  }
  static int __fastcall GetSubClassIndexFromSkill(unsigned int index);
  static int __fastcall IsCollpasedHeader(unsigned int index);
  static int __fastcall GetSubClassFilter() {
    return m_subClassFilter;
  }
  static int __fastcall GetInvTypeFilter() {
    return m_invTypeFilter;
  }
  static int __fastcall GetCollapseFilter() {
    return m_collapseFilter;
  }
  static int __fastcall GetAvailableSlots() {
    return m_availableSlots;
  }
  static void __fastcall SetSubClassFilter(int filter);
  static void __fastcall SetInvTypeFilter(int filter);
  static void __fastcall SetCollapseFilter(int filter);

 private:
  friend int __cdecl QSortSkills(const void *a, const void *b);
  friend int __cdecl QSortSubClasses(const void *a, const void *b);

 protected:
  static void __fastcall FilterAndSortSkills();

 private:
  static int                                       m_skillLine;
  static int                                       m_currentSelection;
  static unsigned int                              m_itemsPending;
  static unsigned int                              m_numSkills;
  static unsigned int                              m_numSubClasses;
  static unsigned int                              m_filteredSkills;
  static int                                       m_subClassFilter;
  static int                                       m_invTypeFilter;
  static int                                       m_collapseFilter;
  static TSGrowableArray<TradeSkillInfo *>         m_skills;
  static TSGrowableArray<TradeSkillSubClassInfo *> m_subClasses;
  static int                                       m_availableSlots;
};

int                                       CGTradeSkillInfo::m_skillLine;
int                                       CGTradeSkillInfo::m_currentSelection;
unsigned int                              CGTradeSkillInfo::m_itemsPending;
unsigned int                              CGTradeSkillInfo::m_numSkills;
unsigned int                              CGTradeSkillInfo::m_numSubClasses;
unsigned int                              CGTradeSkillInfo::m_filteredSkills;
int                                       CGTradeSkillInfo::m_subClassFilter;
int                                       CGTradeSkillInfo::m_invTypeFilter;
int                                       CGTradeSkillInfo::m_collapseFilter;
TSGrowableArray<TradeSkillInfo *>         CGTradeSkillInfo::m_skills;
TSGrowableArray<TradeSkillSubClassInfo *> CGTradeSkillInfo::m_subClasses;
int                                       CGTradeSkillInfo::m_availableSlots;

static const char *s_invSlotTokens[24] = {"HEADSLOT",    "NECKSLOT",    "SHOULDERSLOT", "SHIRTSLOT",    "CHESTSLOT",         "WAISTSLOT",
                                          "LEGSSLOT",    "FEETSLOT",    "WRISTSLOT",    "HANDSSLOT",    "FINGER0SLOT",       "FINGER1SLOT",
                                          "TRINKETSLOT", "TRINKETSLOT", "BACKSLOT",     "MAINHANDSLOT", "SECONDARYHANDSLOT", "RANGEDSLOT",
                                          "TABARDSLOT",  "BAGSLOT",     "BAGSLOT",      "BAGSLOT",      "BAGSLOT",           "NONEQUIPSLOT"};

static const char s_skillCategoryStrings[4][32] = {"optimal", "medium", "easy", "trivial"};

bool __fastcall Spell_C_CastSpell(int spellID, const CGItem_C *item);

static void __fastcall TradeSkillItemCallback(int, const unsigned __int64 &, void *, bool granted) {
  if (granted) {
    CGTradeSkillInfo::RefreshList(0);
  }
}

static void __fastcall TradeSkillListItemCallback(int, const unsigned __int64 &, void *, bool granted) {
  if (granted) {
    CGTradeSkillInfo::DecrementPendingItem();
  }
}

void __fastcall CGTradeSkillInfo::EnterWorld() {
  m_skillLine = 0;
  m_currentSelection = 0;
  m_numSkills = 0;
  m_filteredSkills = 0;
  m_numSubClasses = 0;
}

void __fastcall CGTradeSkillInfo::LeaveWorld() {
  ClearItemCallbacks();
}

void __fastcall CGTradeSkillInfo::ShutdownGame() {
  m_skills.Clear();
  m_subClasses.Clear();
}

void __fastcall CGTradeSkillInfo::Close() {
  m_skillLine = 0;
  FrameScript_SignalEvent(292);
}

void __fastcall CGTradeSkillInfo::ClearItemCallbacks() {
  if (m_itemsPending) {
    for (unsigned int i = 0; i < m_numSkills; ++i) {
      g_itemDBCache.CancelCallback(m_skills[i]->spellID, TradeSkillListItemCallback, 0);
    }
    m_itemsPending = 0;
  }
}

void __fastcall CGTradeSkillInfo::SetSkillLine(int id) {
  ClearItemCallbacks();
  if (id == m_skillLine) {
    m_skillLine = 0;
    FrameScript_SignalEvent(292);
  } else if (g_skillLineDB.GetRecord(id)) {
    m_skillLine = id;
    m_currentSelection = 0;
    RefreshList(1);
    if (m_numSkills) {
      FrameScript_SignalEvent(290);
    }
  }
}

void __fastcall CGTradeSkillInfo::SetSelection(int index) {
  if (index >= 0 && static_cast<unsigned int>(index) < m_numSkills && m_skills[index]->spellID > 0) {
    m_currentSelection = m_skills[index]->spellID;
    g_itemDBCache.GetRecord(
        m_currentSelection, static_cast<unsigned __int64>(m_currentSelection) | 0xB000000000000000ui64, TradeSkillItemCallback, 0
    );
  } else {
    m_currentSelection = 0;
  }
}

int __fastcall CGTradeSkillInfo::GetSelectionIndex() {
  if (!m_currentSelection) {
    return -1;
  }
  for (unsigned int i = 0; i < m_numSkills; ++i) {
    if (m_skills[i]->spellID == m_currentSelection) {
      return i;
    }
  }
  return -1;
}

static int __cdecl QSortSkills(const void *a, const void *b) {
  FATALASSERT(a);
  FATALASSERT(b);
  TradeSkillInfo *info1 = *static_cast<TradeSkillInfo *const *>(a);
  TradeSkillInfo *info2 = *static_cast<TradeSkillInfo *const *>(b);
  unsigned int    subClassRank1 = 0;
  unsigned int    subClassRank2 = 0;
  int             enabled1 = 1;
  int             enabled2 = 1;
  unsigned int    i;
  for (i = 0; i < CGTradeSkillInfo::m_numSubClasses; ++i) {
    TradeSkillSubClassInfo *subClass = CGTradeSkillInfo::m_subClasses[i];
    if (subClass->classID == info1->classID && subClass->subClassID == info1->subClassID) {
      subClassRank1 = i;
      enabled1 = !info1->enabled || !subClass->enabled ? 0 : 1;
    }
    if (subClass->classID == info2->classID && subClass->subClassID == info2->subClassID) {
      subClassRank2 = i;
      enabled2 = info2->enabled && subClass->enabled;
    }
  }
  if (!enabled1) {
    return enabled2 ? 1 : 0;
  }
  if (!enabled2) {
    return -1;
  }
  if (subClassRank1 != subClassRank2) {
    return subClassRank1 < subClassRank2 ? -1 : 1;
  }
  if (info1->spellID == -1) {
    return info2->spellID == -1 ? 1 : -1;
  }
  if (info2->spellID == -1) {
    return 1;
  }
  const SpellRec *spell1 = g_spellDB.GetRecord(info1->spellID);
  const SpellRec *spell2 = g_spellDB.GetRecord(info2->spellID);
  if (!spell1 || !spell2) {
    return 0;
  }
  if (info1->category != info2->category) {
    return info1->category > info2->category ? 1 : -1;
  }
  if (info1->itemLevel != info2->itemLevel) {
    return info1->itemLevel < info2->itemLevel ? 1 : -1;
  }
  return SStrCmp(spell1->m_name_lang[CURRENT_LANGUAGE], spell2->m_name_lang[CURRENT_LANGUAGE], 0x7FFFFFFF);
}

static int __cdecl QSortSubClasses(const void *a, const void *b) {
  FATALASSERT(a);
  FATALASSERT(b);
  TradeSkillSubClassInfo *info1 = *static_cast<TradeSkillSubClassInfo *const *>(a);
  TradeSkillSubClassInfo *info2 = *static_cast<TradeSkillSubClassInfo *const *>(b);
  if (info1->classID != info2->classID) {
    return info1->classID > info2->classID ? 1 : -1;
  }
  const ItemSubClassRec *rec1 = 0;
  const ItemSubClassRec *rec2 = 0;
  int                    i;
  for (i = 0; i < g_itemSubClassDB.GetNumRecords(); ++i) {
    const ItemSubClassRec *rec = g_itemSubClassDB.GetRecordByIndex(i);
    if (rec->m_classID == info1->classID && rec->m_subClassID == info1->subClassID) {
      rec1 = rec;
    }
    if (rec->m_classID == info2->classID && rec->m_subClassID == info2->subClassID) {
      rec2 = rec;
    }
    if (rec1 && rec2) {
      break;
    }
  }
  if (!rec1 || !rec2) {
    return 0;
  }
  const char *name1 = rec1->m_verboseName_lang[CURRENT_LANGUAGE];
  const char *name2 = rec2->m_verboseName_lang[CURRENT_LANGUAGE];
  if (!name1 || !*name1) {
    name1 = rec1->m_displayName_lang[CURRENT_LANGUAGE];
  }
  if (!name2 || !*name2) {
    name2 = rec2->m_displayName_lang[CURRENT_LANGUAGE];
  }
  return name1 && name2 ? SStrCmp(name1, name2, 0x7FFFFFFF) : 0;
}

void __fastcall CGTradeSkillInfo::RefreshList(int resetFilters) {
  unsigned int i;
  unsigned int j;

  if (resetFilters) {
    m_subClassFilter = -1;
    m_invTypeFilter = -1;
    m_collapseFilter = -1;
    m_availableSlots = 0;
  }
  ClearItemCallbacks();
  m_numSkills = 0;
  if (resetFilters) {
    m_numSubClasses = 0;
  }
  if (!m_skillLine) {
    return;
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return;
  }
  TSGrowableArray<int> *spells = player->GetTradeSkills(m_skillLine);
  if (!spells) {
    return;
  }
  while (m_skills.Count() < spells->Count()) {
    TradeSkillInfo *info = NEW(TradeSkillInfo);
    m_skills.Add(1, &info);
  }
  m_numSkills = spells->Count();
  for (i = 0; i < m_numSkills; ++i) {
    int             spellID = (*spells)[i];
    TradeSkillInfo *info = m_skills[i];
    info->spellID = spellID;
    const SkillLineAbilityRec *ability = SpellTableLookupAbility(player->GetUnitData()->race, player->GetUnitData()->classId, spellID);
    info->category = 0;
    if (ability) {
      int trivialMax = ability->m_trivialSkillLineRankHigh;
      int trivialMin = ability->m_trivialSkillLineRankLow;
      if (!trivialMin) {
        trivialMin = trivialMax == 25 ? 0 : trivialMax - 25;
      }
      int midpoint = (trivialMin + trivialMax) / 2;
      int rank = player->GetSkillRank(ability->m_skillLine);
      info->category = rank < trivialMin ? 0 : rank < midpoint ? 1 : rank < trivialMax ? 2 : 3;
    }
    const SpellRec *spell = g_spellDB.GetRecord(spellID);
    if (!spell) {
      continue;
    }
    int numAvailable = -1;
    for (j = 0; j < 8 && numAvailable; ++j) {
      if (spell->m_reagent[j] && spell->m_reagentCount[j]) {
        int count = player->GetBag()->GetItemTypeCount(spell->m_reagent[j], 0) /
                    spell->m_reagentCount[j];
        if (numAvailable == -1 || numAvailable >= count) {
          numAvailable = count;
        }
      }
    }
    unsigned __int64   guid = static_cast<unsigned __int64>(spellID) | 0xB000000000000000ui64;
    const ItemStats_C *stats = g_itemDBCache.GetRecord(spell->m_effectItemType[0], guid, TradeSkillListItemCallback, 0);
    if (!stats) {
      ++m_itemsPending;
      continue;
    }
    if (m_itemsPending) {
      continue;
    }
    if (resetFilters) {
      for (j = 0; j < m_numSubClasses; ++j) {
        if (m_subClasses[j]->classID == stats->m_class && m_subClasses[j]->subClassID == stats->m_subclass) {
          break;
        }
      }
      if (j == m_numSubClasses) {
        if (m_subClasses.Count() <= j) {
          TradeSkillSubClassInfo *subClass = NEW(TradeSkillSubClassInfo);
          m_subClasses.Add(1, &subClass);
        }
        m_subClasses[j]->classID = stats->m_class;
        m_subClasses[j]->subClassID = stats->m_subclass;
        ++m_numSubClasses;
      }
    }
    info->classID = stats->m_class;
    info->subClassID = stats->m_subclass;
    info->itemLevel = stats->m_itemLevel;
    info->invSlots = g_ITEMTYPEARRAY[stats->m_inventoryType];
    if (stats->m_inventoryType == 18) {
      info->invSlots = 0x80000;
    } else if (stats->m_inventoryType == 11) {
      info->invSlots = 0x400;
    } else if (stats->m_inventoryType == 12) {
      info->invSlots = 0x1000;
    }
    if (!info->invSlots) {
      info->invSlots = 0x800000;
    }
    if (resetFilters) {
      m_availableSlots |= info->invSlots;
    }
    info->numAvailable = numAvailable > 0 ? numAvailable : 0;
  }
  if (m_itemsPending) {
    return;
  }
  while (m_skills.Count() < m_numSkills + m_numSubClasses) {
    TradeSkillInfo *info = NEW(TradeSkillInfo);
    m_skills.Add(1, &info);
  }
  for (i = 0; i < m_numSubClasses; ++i) {
    TradeSkillInfo *info = m_skills[m_numSkills + i];
    info->spellID = -1;
    info->classID = m_subClasses[i]->classID;
    info->subClassID = m_subClasses[i]->subClassID;
  }
  m_numSkills += m_numSubClasses;
  FilterAndSortSkills();
  FrameScript_SignalEvent(291);
}

void __fastcall CGTradeSkillInfo::FilterAndSortSkills() {
  unsigned int i;
  unsigned int j;

  m_filteredSkills = m_numSkills;
  for (i = 0; i < m_numSubClasses; ++i) {
    m_subClasses[i]->enabled = (m_subClassFilter & (1 << i)) != 0;
    m_subClasses[i]->collapsed = (m_collapseFilter & (1 << i)) == 0;
    m_subClasses[i]->filteredCount = 0;
  }
  for (i = 0; i < m_numSkills; ++i) {
    TradeSkillInfo *info = m_skills[i];
    if (info->spellID >= 0) {
      for (j = 0; j < m_numSubClasses; ++j) {
        TradeSkillSubClassInfo *subClass = m_subClasses[j];
        if (info->classID == subClass->classID && info->subClassID == subClass->subClassID && (m_invTypeFilter & info->invSlots)) {
          ++subClass->filteredCount;
          break;
        }
      }
    }
  }
  for (i = 0; i < m_numSkills; ++i) {
    TradeSkillInfo *info = m_skills[i];
    info->enabled = 1;
    if (info->spellID >= 0 && !(m_invTypeFilter & info->invSlots)) {
      info->enabled = 0;
      --m_filteredSkills;
      continue;
    }
    TradeSkillSubClassInfo *subClass = 0;
    for (j = 0; j < m_numSubClasses; ++j) {
      if (info->classID == m_subClasses[j]->classID && info->subClassID == m_subClasses[j]->subClassID) {
        subClass = m_subClasses[j];
        break;
      }
    }
    if (!subClass || !subClass->enabled || !subClass->filteredCount || (info->spellID >= 0 && subClass->collapsed)) {
      info->enabled = 0;
      --m_filteredSkills;
    }
  }
  qsort(m_subClasses.Ptr(), m_numSubClasses, sizeof(TradeSkillSubClassInfo *), QSortSubClasses);
  qsort(m_skills.Ptr(), m_numSkills, sizeof(TradeSkillInfo *), QSortSkills);
}

int __fastcall CGTradeSkillInfo::GetSubClassIndexFromSkill(unsigned int index) {
  TradeSkillInfo *skill = GetTradeSkillInfo(index);
  if (!skill || skill->spellID >= 0) {
    return -1;
  }
  for (unsigned int i = 0; i < m_numSubClasses; ++i) {
    TradeSkillSubClassInfo *subClass = m_subClasses[i];
    if (subClass->classID == skill->classID && subClass->subClassID == skill->subClassID) {
      return i;
    }
  }
  return -1;
}

int __fastcall CGTradeSkillInfo::IsCollpasedHeader(unsigned int index) {
  int subClass = GetSubClassIndexFromSkill(index);
  return subClass >= 0 && !(m_collapseFilter & (1 << subClass));
}

void __fastcall CGTradeSkillInfo::SetSubClassFilter(int filter) {
  m_subClassFilter = filter;
  FilterAndSortSkills();
  FrameScript_SignalEvent(291);
}

void __fastcall CGTradeSkillInfo::SetInvTypeFilter(int filter) {
  m_invTypeFilter = filter;
  FilterAndSortSkills();
  FrameScript_SignalEvent(291);
}

void __fastcall CGTradeSkillInfo::SetCollapseFilter(int filter) {
  m_collapseFilter = filter;
  FilterAndSortSkills();
  FrameScript_SignalEvent(291);
}

static int __fastcall Script_CloseTradeSkill(lua_State *__formal) {
  CGTradeSkillInfo::Close();
  return 0;
}

static int __fastcall Script_GetNumTradeSkills(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGTradeSkillInfo::GetNumTradeSkills()));
  return 1;
}

static int __fastcall Script_GetTradeSkillInfo(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetTradeSkillInfo(index)");
  }
  unsigned int    index = static_cast<unsigned int>(lua_tonumber(L, 1)) - 1;
  TradeSkillInfo *info = CGTradeSkillInfo::GetTradeSkillInfo(index);
  if (info && info->spellID != -1) {
    const SpellRec *spell = g_spellDB.GetRecord(info->spellID);
    if (spell) {
      lua_pushstring(L, spell->m_name_lang[CURRENT_LANGUAGE]);
      lua_pushstring(L, s_skillCategoryStrings[info->category]);
      lua_pushnumber(L, static_cast<double>(info->numAvailable));
      lua_pushnil(L);
      return 4;
    }
  } else if (info) {
    for (int i = 0; i < g_itemSubClassDB.GetNumRecords(); ++i) {
      const ItemSubClassRec *subClass = g_itemSubClassDB.GetRecordByIndex(i);
      if (subClass->m_classID == info->classID && subClass->m_subClassID == info->subClassID) {
        const char *name = subClass->m_verboseName_lang[CURRENT_LANGUAGE];
        if (!name || !*name) {
          name = subClass->m_displayName_lang[CURRENT_LANGUAGE];
        }
        lua_pushstring(L, name);
        lua_pushstring(L, "header");
        lua_pushnumber(L, 0.0);
        CGTradeSkillInfo::IsCollpasedHeader(index) ? lua_pushnil(L) : lua_pushnumber(L, 1.0);
        return 4;
      }
    }
  }
  lua_pushnil(L);
  lua_pushnil(L);
  lua_pushnumber(L, 0.0);
  lua_pushnil(L);
  return 4;
}

static int __fastcall Script_SelectTradeSkill(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: SelectTradeSkill(index)");
  }
  CGTradeSkillInfo::SetSelection(static_cast<int>(lua_tonumber(L, 1)) - 1);
  return 0;
}

static int __fastcall Script_GetTradeSkillSelectionIndex(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGTradeSkillInfo::GetSelectionIndex() + 1));
  return 1;
}

static int __fastcall Script_GetTradeSkillIcon(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetTradeSkillIcon(index)");
  }
  TradeSkillInfo     *info = CGTradeSkillInfo::GetTradeSkillInfo(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  const SpellRec     *spell = info && info->spellID >= 0 ? g_spellDB.GetRecord(info->spellID) : 0;
  const SpellIconRec *icon = spell ? g_spellIconDB.GetRecord(spell->m_spellIconID) : 0;
  lua_pushstring(L, icon ? icon->m_textureFilename : 0);
  return 1;
}

static int __fastcall Script_GetTradeSkillLine(lua_State *L) {
  const SkillLineRec *line = g_skillLineDB.GetRecord(CGTradeSkillInfo::GetSkillLine());
  lua_pushstring(L, line ? line->m_displayName_lang[CURRENT_LANGUAGE] : "UNKNOWN");
  return 1;
}

static int __fastcall Script_GetTradeSkillItemStats(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetTradeSkillItemStats(index)");
  }
  TradeSkillInfo *info = CGTradeSkillInfo::GetTradeSkillInfo(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  const SpellRec *spell = info && info->spellID >= 0 ? g_spellDB.GetRecord(info->spellID) : 0;
  if (!spell || !spell->m_effectItemType[0]) {
    return 0;
  }
  unsigned __int64   guid = static_cast<unsigned __int64>(info->spellID) | 0xB000000000000000ui64;
  const ItemStats_C *stats = g_itemDBCache.GetRecord(spell->m_effectItemType[0], guid, TradeSkillItemCallback, 0);
  if (!stats) {
    return 0;
  }
  lua_pushstring(L, stats->m_displayName[CURRENT_LANGUAGE]);
  if (stats->m_description && *stats->m_description) {
    lua_pushstring(L, stats->m_description);
    return 2;
  }
  return 1;
}

static int __fastcall Script_GetTradeSkillItemLink(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetTradeSkillItemLink(index)");
  }
  TradeSkillInfo *info = CGTradeSkillInfo::GetTradeSkillInfo(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  const SpellRec *spell = info && info->spellID >= 0 ? g_spellDB.GetRecord(info->spellID) : 0;
  if (!spell || !spell->m_effectItemType[0]) {
    return 0;
  }
  unsigned __int64   guid = static_cast<unsigned __int64>(info->spellID) | 0xB000000000000000ui64;
  const ItemStats_C *stats = g_itemDBCache.GetRecord(spell->m_effectItemType[0], guid, TradeSkillItemCallback, 0);
  if (!stats) {
    return 0;
  }
  char link[1024];
  SStrPrintf(link, sizeof(link), "|Hitem:%d|h[%s]|h", spell->m_effectItemType[0], stats->m_displayName[CURRENT_LANGUAGE]);
  lua_pushstring(L, link);
  return 1;
}

static int __fastcall Script_GetTradeSkillNumReagents(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetTradeSkillNumReagents(index)");
  }
  TradeSkillInfo *info = CGTradeSkillInfo::GetTradeSkillInfo(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  const SpellRec *spell = info && info->spellID >= 0 ? g_spellDB.GetRecord(info->spellID) : 0;
  int             count = 0;
  if (spell) {
    for (unsigned int i = 0; i < 8; ++i) {
      if (spell->m_reagent[i]) {
        ++count;
      }
    }
  }
  lua_pushnumber(L, static_cast<double>(count));
  return 1;
}

static int __fastcall Script_GetTradeSkillReagentInfo(lua_State *L) {
  if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
    return luaL_error(L, "Usage: GetTradeSkillReagentInfo(index, reagentIndex)");
  }
  TradeSkillInfo *info = CGTradeSkillInfo::GetTradeSkillInfo(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  int             reagentIndex = static_cast<int>(lua_tonumber(L, 2));
  const SpellRec *spell = info && info->spellID >= 0 ? g_spellDB.GetRecord(info->spellID) : 0;
  unsigned int    slot = 0;
  int             count = 0;
  while (spell && slot < 8) {
    if (spell->m_reagent[slot] && ++count == reagentIndex) {
      break;
    }
    ++slot;
  }
  if (spell && slot < 8) {
    int                itemID = spell->m_reagent[slot];
    unsigned __int64   guid = static_cast<unsigned __int64>(info->spellID) | 0xB000000000000000ui64;
    const ItemStats_C *stats = g_itemDBCache.GetRecord(itemID, guid, TradeSkillItemCallback, 0);
    if (stats) {
      lua_pushstring(L, stats->m_displayName[CURRENT_LANGUAGE]);
      char        buffer[260];
      const char *path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
      SStrPrintf(buffer, sizeof(buffer), "%s%s", path, *path ? "\\" : "");
      SStrPack(buffer, CGItem_C::GetInventoryArt(stats->m_displayInfoID), sizeof(buffer));
      lua_pushstring(L, buffer);
    } else {
      lua_pushnil(L);
      lua_pushnil(L);
    }
    lua_pushnumber(L, static_cast<double>(spell->m_reagentCount[slot]));
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    lua_pushnumber(L, player ? static_cast<double>(player->GetBag()->GetItemTypeCount(itemID, 0)) : 0.0);
    return 4;
  }
  lua_pushnil(L);
  lua_pushnil(L);
  lua_pushnil(L);
  lua_pushnil(L);
  return 4;
}

static int __fastcall Script_GetTradeSkillTools(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetTradeSkillTools(index)");
  }
  TradeSkillInfo *info = CGTradeSkillInfo::GetTradeSkillInfo(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  const SpellRec *spell = info && info->spellID >= 0 ? g_spellDB.GetRecord(info->spellID) : 0;
  unsigned int    count = 0;
  if (spell) {
    const SpellFocusObjectRec *focus = g_spellFocusObjectDB.GetRecord(spell->m_requiresSpellFocus);
    if (focus) {
      lua_pushstring(L, focus->m_name_lang[CURRENT_LANGUAGE]);
      ++count;
    }
    unsigned __int64 guid = static_cast<unsigned __int64>(info->spellID) | 0xB000000000000000ui64;
    for (unsigned int i = 0; i < 2; ++i) {
      if (spell->m_totem[i]) {
        const ItemStats_C *stats = g_itemDBCache.GetRecord(spell->m_totem[i], guid, TradeSkillItemCallback, 0);
        if (stats) {
          lua_pushstring(L, stats->m_displayName[CURRENT_LANGUAGE]);
          ++count;
        }
      }
    }
  }
  return count;
}

static int __fastcall Script_GetTradeSkillSubClasses(lua_State *L) {
  unsigned int count = 0;
  for (unsigned int i = 0; i < CGTradeSkillInfo::GetNumSubClasses(); ++i) {
    TradeSkillSubClassInfo *info = CGTradeSkillInfo::GetSubClass(i);
    for (int j = 0; info && j < g_itemSubClassDB.GetNumRecords(); ++j) {
      const ItemSubClassRec *rec = g_itemSubClassDB.GetRecordByIndex(j);
      if (rec->m_classID == info->classID && rec->m_subClassID == info->subClassID) {
        const char *name = rec->m_verboseName_lang[CURRENT_LANGUAGE];
        if (!name || !*name) {
          name = rec->m_displayName_lang[CURRENT_LANGUAGE];
        }
        lua_pushstring(L, name);
        ++count;
        break;
      }
    }
  }
  return count;
}

static int __fastcall Script_GetTradeSkillInvSlots(lua_State *L) {
  int available = CGTradeSkillInfo::GetAvailableSlots();
  int count = 0;
  for (unsigned int i = 0; i < 24; ++i) {
    if (available & (1 << i)) {
      lua_pushstring(L, FrameScript_GetText(s_invSlotTokens[i], -1, GENDER_NOT_APPLICABLE));
      ++count;
    }
  }
  return count;
}

static int __fastcall Script_SetTradeSkillSubClassFilter(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: SetTradeSkillSubClassFilter(index, onOff [, exclusive])");
  }
  int index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  if (index < 0) {
    CGTradeSkillInfo::SetSubClassFilter(-1);
    return 0;
  }
  if (static_cast<unsigned int>(index) >= CGTradeSkillInfo::GetNumSubClasses()) {
    return luaL_error(L, "Bad sub class in SetTradeSkillSubClassFilter");
  }
  if (!lua_isnumber(L, 2)) {
    return luaL_error(L, "Missing on/off parameter");
  }
  int filter = CGTradeSkillInfo::GetSubClassFilter();
  if (static_cast<unsigned int>(lua_tonumber(L, 2))) {
    filter = lua_isnumber(L, 3) && static_cast<unsigned int>(lua_tonumber(L, 3)) ? 1 << index : filter | (1 << index);
  } else {
    filter &= ~(1 << index);
  }
  CGTradeSkillInfo::SetSubClassFilter(filter);
  return 0;
}

static int __fastcall Script_GetTradeSkillSubClassFilter(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetTradeSkillSubClassFilter(index)");
  }
  int index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  int filter = CGTradeSkillInfo::GetSubClassFilter();
  if (index < 0) {
    for (unsigned int i = 0; i < CGTradeSkillInfo::GetNumSubClasses(); ++i) {
      if (!(filter & (1 << i))) {
        lua_pushnil(L);
        return 1;
      }
    }
    lua_pushnumber(L, 1.0);
    return 1;
  }
  if (static_cast<unsigned int>(index) >= CGTradeSkillInfo::GetNumSubClasses()) {
    return luaL_error(L, "Bad sub class in GetTradeSkillSubClassFilter");
  }
  filter & (1 << index) ? lua_pushnumber(L, 1.0) : lua_pushnil(L);
  return 1;
}

static int __fastcall Script_SetTradeSkillInvSlotFilter(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: SetTradeSkillInvSlotFilter(index, onOff [, exclusive])");
  }
  int index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  if (index < 0) {
    CGTradeSkillInfo::SetInvTypeFilter(-1);
    return 0;
  }
  int available = CGTradeSkillInfo::GetAvailableSlots();
  int slot = 0;
  int visible = 0;
  while (slot < 24) {
    if (available & (1 << slot)) {
      if (visible == index) {
        break;
      }
      ++visible;
    }
    ++slot;
  }
  if (slot >= 24) {
    return luaL_error(L, "Bad inventory slot in SetTradeSkillInvSlotFilter");
  }
  if (!lua_isnumber(L, 2)) {
    return luaL_error(L, "Missing on/off parameter");
  }
  int filter = CGTradeSkillInfo::GetInvTypeFilter();
  if (static_cast<unsigned int>(lua_tonumber(L, 2))) {
    filter = lua_isnumber(L, 3) && static_cast<unsigned int>(lua_tonumber(L, 3)) ? 1 << slot : filter | (1 << slot);
  } else {
    filter &= ~(1 << slot);
  }
  CGTradeSkillInfo::SetInvTypeFilter(filter);
  return 0;
}

static int __fastcall Script_GetTradeSkillInvSlotFilter(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetTradeSkillInvSlotFilter(index)");
  }
  int index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  int available = CGTradeSkillInfo::GetAvailableSlots();
  int filter = CGTradeSkillInfo::GetInvTypeFilter();
  if (index < 0) {
    lua_pushnumber(L, (filter & available) == available ? 1.0 : 0.0);
    return 1;
  }
  int slot = 0;
  int visible = 0;
  while (slot < 24) {
    if (available & (1 << slot)) {
      if (visible == index) {
        break;
      }
      ++visible;
    }
    ++slot;
  }
  if (slot >= 24) {
    return luaL_error(L, "Bad inventory slot in GetTradeSkillInvSlotFilter");
  }
  filter & (1 << slot) ? lua_pushnumber(L, 1.0) : lua_pushnil(L);
  return 1;
}

static int __fastcall Script_CollapseTradeSkillSubClass(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: CollapseTradeSkillSubClass(index)");
  }
  int index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  if (index < 0) {
    CGTradeSkillInfo::SetCollapseFilter(0);
  } else {
    int subClass = CGTradeSkillInfo::GetSubClassIndexFromSkill(index);
    if (subClass < 0) {
      return luaL_error(L, "Bad sub class in CollapseTradeSkillSubClass");
    }
    CGTradeSkillInfo::SetCollapseFilter(CGTradeSkillInfo::GetCollapseFilter() & ~(1 << subClass));
  }
  return 0;
}

static int __fastcall Script_ExpandTradeSkillSubClass(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: ExpandTradeSkillSubClass(index)");
  }
  int index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  if (index < 0) {
    CGTradeSkillInfo::SetCollapseFilter(-1);
  } else {
    int subClass = CGTradeSkillInfo::GetSubClassIndexFromSkill(index);
    if (subClass < 0) {
      return luaL_error(L, "Bad skill line in ExpandTradeSkillSubClass");
    }
    CGTradeSkillInfo::SetCollapseFilter(CGTradeSkillInfo::GetCollapseFilter() | (1 << subClass));
  }
  return 0;
}

static int __fastcall Script_DoTradeSkill(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: DoTradeSkill(index)");
  }
  TradeSkillInfo *info = CGTradeSkillInfo::GetTradeSkillInfo(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  if (info) {
    Spell_C_CastSpell(info->spellID, 0);
  }
  return 0;
}

static FrameScript_Method s_ScriptFunctions[21] = {
    {            "CloseTradeSkill",             Script_CloseTradeSkill},
    {          "GetNumTradeSkills",           Script_GetNumTradeSkills},
    {          "GetTradeSkillInfo",           Script_GetTradeSkillInfo},
    {           "SelectTradeSkill",            Script_SelectTradeSkill},
    {"GetTradeSkillSelectionIndex", Script_GetTradeSkillSelectionIndex},
    {          "GetTradeSkillIcon",           Script_GetTradeSkillIcon},
    {          "GetTradeSkillLine",           Script_GetTradeSkillLine},
    {     "GetTradeSkillItemStats",      Script_GetTradeSkillItemStats},
    {      "GetTradeSkillItemLink",       Script_GetTradeSkillItemLink},
    {   "GetTradeSkillNumReagents",    Script_GetTradeSkillNumReagents},
    {   "GetTradeSkillReagentInfo",    Script_GetTradeSkillReagentInfo},
    {         "GetTradeSkillTools",          Script_GetTradeSkillTools},
    {    "GetTradeSkillSubClasses",     Script_GetTradeSkillSubClasses},
    {      "GetTradeSkillInvSlots",       Script_GetTradeSkillInvSlots},
    {"SetTradeSkillSubClassFilter", Script_SetTradeSkillSubClassFilter},
    {"GetTradeSkillSubClassFilter", Script_GetTradeSkillSubClassFilter},
    { "SetTradeSkillInvSlotFilter",  Script_SetTradeSkillInvSlotFilter},
    { "GetTradeSkillInvSlotFilter",  Script_GetTradeSkillInvSlotFilter},
    { "CollapseTradeSkillSubClass",  Script_CollapseTradeSkillSubClass},
    {   "ExpandTradeSkillSubClass",    Script_ExpandTradeSkillSubClass},
    {               "DoTradeSkill",                Script_DoTradeSkill}
};

void __fastcall TradeSkillRegisterScriptFunctions() {
  for (unsigned int i = 0; i < 21; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void __fastcall TradeSkillUnregisterScriptFunctions() {
  for (unsigned int i = 0; i < 21; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
