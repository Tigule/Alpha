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

#include "DB/DBClient/DBCacheInstances.h"
#include "DB/DBClient/AutoCode/ChrClassesRec.h"
#include "DB/DBClient/AutoCode/ChrRacesRec.h"
#include "DB/DBClient/AutoCode/ItemClassRec.h"
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

enum TRADESKILL_CATEGORY {
  TRADESKILL_OPTIMAL = 0,
  TRADESKILL_MEDIUM = 1,
  TRADESKILL_EASY = 2,
  TRADESKILL_TRIVIAL = 3,
  NUM_TRADESKILL_CATEGORIES = 4
};

struct TradeSkillInfo {
  int                 spellID;
  TRADESKILL_CATEGORY category;
  int                 classID;
  int                 subClassID;
  int                 invSlots;
  int                 itemLevel;
  int                 numAvailable;
  int                 enabled;
};

struct TradeSkillSubClassInfo {
  int classID;
  int subClassID;
  int filteredCount;
  int enabled;
  int collapsed;
};

static int __cdecl QSortSkills(LPCVOID a, LPCVOID b);
static int __cdecl QSortSubClasses(LPCVOID a, LPCVOID b);

const SkillLineAbilityRec *SpellTableLookupAbility(UINT raceID, UINT classID, UINT spellID);
extern const int           g_ITEMTYPEARRAY[];

class CGTradeSkillInfo {
  friend int __cdecl QSortSkills(LPCVOID a, LPCVOID b);
  friend int __cdecl QSortSubClasses(LPCVOID a, LPCVOID b);

 public:
  static void EnterWorld();
  static void LeaveWorld();
  static void ShutdownGame();
  static void Close();
  static void ClearItemCallbacks();
  static void DecrementPendingItem() {
    if (!m_itemsPending || !--m_itemsPending) {
      RefreshList(1);
    }
  }
  static void SetSelection(int index);
  static int  GetSelectionIndex();
  static int  GetSkillLine() {
    return m_skillLine;
  }
  static int GetNumTradeSkills() {
    return m_filteredSkills;
  }
  static const TradeSkillInfo *GetTradeSkillInfo(UINT index) {
    return index < m_filteredSkills ? m_skills[index] : 0;
  }
  static void SetSkillLine(int id);
  static void RefreshList(int resetFilters);
  static UINT GetNumSubClasses() {
    return m_numSubClasses;
  }
  static TradeSkillSubClassInfo *GetSubClass(UINT index) {
    return index < m_numSubClasses ? m_subClasses[index] : 0;
  }
  static int  GetSubClassIndexFromSkill(UINT index);
  static BOOL IsCollpasedHeader(UINT index);
  static int  GetSubClassFilter() {
    return m_subClassFilter;
  }
  static int GetInvTypeFilter() {
    return m_invTypeFilter;
  }
  static int GetCollapseFilter() {
    return m_collapseFilter;
  }
  static int GetAvailableSlots() {
    return m_availableSlots;
  }
  static void SetSubClassFilter(int filter);
  static void SetInvTypeFilter(int filter);
  static void SetCollapseFilter(int filter);

 protected:
  static void FilterAndSortSkills();

 private:
  static int                                       m_skillLine;
  static int                                       m_currentSelection;
  static UINT                                      m_itemsPending;
  static UINT                                      m_numSkills;
  static UINT                                      m_numSubClasses;
  static UINT                                      m_filteredSkills;
  static int                                       m_subClassFilter;
  static int                                       m_invTypeFilter;
  static int                                       m_collapseFilter;
  static TSGrowableArray<TradeSkillInfo *>         m_skills;
  static TSGrowableArray<TradeSkillSubClassInfo *> m_subClasses;
  static int                                       m_availableSlots;
};

int                                       CGTradeSkillInfo::m_skillLine;
int                                       CGTradeSkillInfo::m_currentSelection;
UINT                                      CGTradeSkillInfo::m_itemsPending;
UINT                                      CGTradeSkillInfo::m_numSkills;
UINT                                      CGTradeSkillInfo::m_numSubClasses;
UINT                                      CGTradeSkillInfo::m_filteredSkills;
int                                       CGTradeSkillInfo::m_subClassFilter;
int                                       CGTradeSkillInfo::m_invTypeFilter;
int                                       CGTradeSkillInfo::m_collapseFilter;
TSGrowableArray<TradeSkillInfo *>         CGTradeSkillInfo::m_skills;
TSGrowableArray<TradeSkillSubClassInfo *> CGTradeSkillInfo::m_subClasses;
int                                       CGTradeSkillInfo::m_availableSlots;

static LPCSTR s_invSlotTokens[24] = {"HEADSLOT",    "NECKSLOT",    "SHOULDERSLOT", "SHIRTSLOT",    "CHESTSLOT",         "WAISTSLOT",
                                     "LEGSSLOT",    "FEETSLOT",    "WRISTSLOT",    "HANDSSLOT",    "FINGER0SLOT",       "FINGER1SLOT",
                                     "TRINKETSLOT", "TRINKETSLOT", "BACKSLOT",     "MAINHANDSLOT", "SECONDARYHANDSLOT", "RANGEDSLOT",
                                     "TABARDSLOT",  "BAGSLOT",     "BAGSLOT",      "BAGSLOT",      "BAGSLOT",           "NONEQUIPSLOT"};

static const char s_skillCategoryStrings[4][32] = {"optimal", "medium", "easy", "trivial"};

bool Spell_C_CastSpell(int spellID, const CGItem_C *item);

extern LPCSTR g_invTypeTokens[];

static void TradeSkillItemCallback(int id, const DWORDLONG &guid, LPVOID arg, bool granted) {
  if (granted) {
    CGTradeSkillInfo::RefreshList(0);
  }
}

static void TradeSkillListItemCallback(int id, const DWORDLONG &guid, LPVOID arg, bool granted) {
  if (granted) {
    CGTradeSkillInfo::DecrementPendingItem();
  }
}

void CGTradeSkillInfo::EnterWorld() {
  m_skillLine = 0;
  m_currentSelection = 0;
  m_numSkills = 0;
  m_filteredSkills = 0;
  m_numSubClasses = 0;
}

void CGTradeSkillInfo::LeaveWorld() {
  ClearItemCallbacks();
}

void CGTradeSkillInfo::ShutdownGame() {
  m_skills.Clear();
  m_subClasses.Clear();
}

void CGTradeSkillInfo::Close() {
  m_skillLine = 0;
  FrameScript_SignalEvent(292);
}

void CGTradeSkillInfo::ClearItemCallbacks() {
  if (m_itemsPending) {
    for (UINT i = 0; i < m_numSkills; ++i) {
      g_itemDBCache.CancelCallback(m_skills[i]->spellID, TradeSkillListItemCallback, 0);
    }
    m_itemsPending = 0;
  }
}

void CGTradeSkillInfo::SetSkillLine(int id) {
  ClearItemCallbacks();
  if (id == m_skillLine) {
    m_skillLine = 0;
    FrameScript_SignalEvent(292);
  } else if (g_skillLineDB.GetRecord(id)) {
    m_skillLine = id;
    m_currentSelection = 0;
    RefreshList(1);
    if (m_numSkills > 0) {
      FrameScript_SignalEvent(290);
    }
  }
}

void CGTradeSkillInfo::SetSelection(int index) {
  if (index < m_numSkills && m_skills[index]->spellID > 0) {
    m_currentSelection = m_skills[index]->spellID;
  } else {
    m_currentSelection = 0;
  }
}

int CGTradeSkillInfo::GetSelectionIndex() {
  if (!m_currentSelection) {
    return -1;
  }
  UINT i;
  for (i = 0; i < m_numSkills; ++i) {
    if (m_skills[i]->spellID == m_currentSelection) {
      break;
    }
  }
  if (i == m_numSkills) {
    return -1;
  }
  return i;
}

static int __cdecl QSortSkills(LPCVOID a, LPCVOID b) {
  FATALASSERT(a);
  FATALASSERT(b);
  TradeSkillInfo *info1 = *(TradeSkillInfo *const *)a;
  TradeSkillInfo *info2 = *(TradeSkillInfo *const *)b;
  UINT            subClassRank1 = 0;
  UINT            subClassRank2 = 0;
  int             enabled1 = 1;
  int             enabled2 = 1;
  for (UINT i = 0; i < CGTradeSkillInfo::GetNumSubClasses(); ++i) {
    TradeSkillSubClassInfo *subClass = CGTradeSkillInfo::GetSubClass(i);
    if (subClass->classID == info1->classID && subClass->subClassID == info1->subClassID) {
      subClassRank1 = i;
      enabled1 = info1->enabled && subClass->enabled;
    }
    if (subClass->classID == info2->classID && subClass->subClassID == info2->subClassID) {
      subClassRank2 = i;
      enabled2 = info2->enabled && subClass->enabled;
    }
  }
  if (enabled1 && enabled2) {
    if (subClassRank1 != subClassRank2) {
      return subClassRank2 < subClassRank1 ? 1 : -1;
    }
    if (info1->spellID != -1 && info2->spellID != -1) {
      const SpellRec *spell1 = g_spellDB.GetRecord(info1->spellID);
      const SpellRec *spell2 = g_spellDB.GetRecord(info2->spellID);
      if (!spell1 || !spell2) {
        return 0;
      }
      if (info1->category == info2->category) {
        if (info1->itemLevel == info2->itemLevel) {
          return SStrCmpI(spell1->m_name_lang[CURRENT_LANGUAGE], spell2->m_name_lang[CURRENT_LANGUAGE], 0x7FFFFFFF);
        }
        return info1->itemLevel < info2->itemLevel ? 1 : -1;
      }
      return info1->category > info2->category ? 1 : -1;
    }
    return info2->spellID == -1 ? 1 : -1;
  }
  if (!enabled1 && !enabled2) {
    return 0;
  }
  return enabled2 ? 1 : -1;
}

static int __cdecl QSortSubClasses(LPCVOID a, LPCVOID b) {
  FATALASSERT(a);
  FATALASSERT(b);
  TradeSkillSubClassInfo *info1 = *(TradeSkillSubClassInfo *const *)a;
  TradeSkillSubClassInfo *info2 = *(TradeSkillSubClassInfo *const *)b;
  if (info1->classID == info2->classID) {
    const ItemSubClassRec *rec1 = 0;
    const ItemSubClassRec *rec2 = 0;
    for (UINT i = 0; i < g_itemSubClassDB.GetNumRecords(); ++i) {
      const ItemSubClassRec *rec = g_itemSubClassDB.GetRecordByIndex(i);
      if (rec->m_classID == info1->classID && rec->m_subClassID == info1->subClassID) {
        rec1 = rec;
        if (rec2) {
          break;
        }
      }
      if (rec->m_classID == info2->classID && rec->m_subClassID == info2->subClassID) {
        rec2 = rec;
        if (rec1) {
          break;
        }
      }
    }
    if (rec1 && rec2) {
      LPCSTR name1 = rec1->m_verboseName_lang[CURRENT_LANGUAGE];
      if (!name1 || !*name1) {
        name1 = rec1->m_displayName_lang[CURRENT_LANGUAGE];
      }
      LPCSTR name2 = rec2->m_verboseName_lang[CURRENT_LANGUAGE];
      if (!name2 || !*name2) {
        name2 = rec2->m_displayName_lang[CURRENT_LANGUAGE];
      }
      if (name1 && name2) {
        return SStrCmpI(name1, name2, 0x7FFFFFFF);
      }
    }
    return 0;
  }
  return info1->classID > info2->classID ? 1 : -1;
}

void CGTradeSkillInfo::RefreshList(int resetFilters) {
  UINT i;
  UINT j;

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
  if (m_skillLine) {
    CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
    if (player) {
      const TSGrowableArray<int> *spells = player->GetTradeSkills(m_skillLine);
      if (spells) {
        for (i = m_skills.Count(); i < spells->Count(); ++i) {
          TradeSkillInfo *info = NEW(TradeSkillInfo);
          m_skills.Add(&info);
        }
        m_numSkills = spells->Count();
        for (i = 0; i < m_numSkills; ++i) {
          int                        spellID = (*spells)[i];
          const SkillLineAbilityRec *ability = SpellTableLookupAbility(player->GetRace(), player->GetClass(), spellID);
          m_skills[i]->spellID = spellID;
          m_skills[i]->category = TRADESKILL_OPTIMAL;
          if (ability) {
            int trivialMax = ability->m_trivialSkillLineRankHigh;
            int trivialMin = ability->m_trivialSkillLineRankLow;
            if (!trivialMin) {
              trivialMin = max(trivialMax - 25u, 0);
            }
            int midpoint = (trivialMax + trivialMin) / 2;
            int rank = player->GetSkillRank(ability->m_skillLine);
            if (rank < trivialMin) {
              m_skills[i]->category = TRADESKILL_OPTIMAL;
            } else if (rank < midpoint) {
              m_skills[i]->category = TRADESKILL_MEDIUM;
            } else if (rank < trivialMax) {
              m_skills[i]->category = TRADESKILL_EASY;
            } else {
              m_skills[i]->category = TRADESKILL_TRIVIAL;
            }
          }
          int             numAvailable = -1;
          const SpellRec *spell = g_spellDB.GetRecord(spellID);
          if (spell) {
            for (j = 0; j < 8; ++j) {
              if (!numAvailable) {
                break;
              }
              if (spell->m_reagent[j] && spell->m_reagentCount[j]) {
                int count = player->CGPlayer_C::GetBag()->GetItemTypeCount(spell->m_reagent[j], 0) / spell->m_reagentCount[j];
                if (numAvailable == -1 || numAvailable >= count) {
                  numAvailable = count;
                }
              }
            }
            const ItemStats_C *stats = g_itemDBCache.GetRecord(
                spell->m_effectItemType[0], (DWORDLONG)spellID | 0xB000000000000000ui64, TradeSkillListItemCallback, 0
            );
            if (!stats) {
              ++m_itemsPending;
            }
            if (!m_itemsPending) {
              if (resetFilters) {
                for (j = 0; j < m_numSubClasses; ++j) {
                  if (m_subClasses[j]->classID == stats->m_class && m_subClasses[j]->subClassID == stats->m_subclass) {
                    break;
                  }
                }
                if (j == m_numSubClasses) {
                  if (m_numSubClasses >= m_subClasses.Count()) {
                    TradeSkillSubClassInfo *info = NEW(TradeSkillSubClassInfo);
                    m_subClasses.Add(&info);
                  }
                  m_subClasses[j]->classID = stats->m_class;
                  m_subClasses[j]->subClassID = stats->m_subclass;
                  ++m_numSubClasses;
                }
              }
              m_skills[i]->classID = stats->m_class;
              m_skills[i]->subClassID = stats->m_subclass;
              m_skills[i]->itemLevel = stats->m_itemLevel;
              m_skills[i]->invSlots = g_ITEMTYPEARRAY[stats->m_inventoryType];
              if (stats->m_inventoryType == 18) {
                m_skills[i]->invSlots = 0x80000;
              } else if (stats->m_inventoryType == 11) {
                m_skills[i]->invSlots = 0x400;
              } else if (stats->m_inventoryType == 12) {
                m_skills[i]->invSlots = 0x1000;
              }
              if (!m_skills[i]->invSlots) {
                m_skills[i]->invSlots = 0x800000;
              }
              if (resetFilters) {
                m_availableSlots |= m_skills[i]->invSlots;
              }
            }
          }
          m_skills[i]->numAvailable = numAvailable > 0 ? numAvailable : 0;
        }
        if (!m_itemsPending) {
          for (i = m_skills.Count(); i < m_numSkills + m_numSubClasses; ++i) {
            TradeSkillInfo *info = NEW(TradeSkillInfo);
            m_skills.Add(&info);
          }
          for (i = 0; i < m_numSubClasses; ++i) {
            m_skills[m_numSkills + i]->spellID = -1;
            m_skills[m_numSkills + i]->classID = m_subClasses[i]->classID;
            m_skills[m_numSkills + i]->subClassID = m_subClasses[i]->subClassID;
          }
          m_numSkills += m_numSubClasses;
          FilterAndSortSkills();
          FrameScript_SignalEvent(291);
        }
      }
    }
  }
}

void CGTradeSkillInfo::FilterAndSortSkills() {
  UINT i;
  UINT j;

  m_filteredSkills = m_numSkills;
  for (i = 0; i < m_numSubClasses; ++i) {
    m_subClasses[i]->enabled = (m_subClassFilter & (1 << i)) != 0;
    m_subClasses[i]->collapsed = (m_collapseFilter & (1 << i)) == 0;
    m_subClasses[i]->filteredCount = 0;
  }
  for (i = 0; i < m_numSkills; ++i) {
    if (m_skills[i]->spellID >= 0) {
      for (j = 0; j < m_numSubClasses; ++j) {
        if (m_skills[i]->classID == m_subClasses[j]->classID && m_skills[i]->subClassID == m_subClasses[j]->subClassID) {
          if (m_skills[i]->invSlots & m_invTypeFilter) {
            ++m_subClasses[j]->filteredCount;
          }
          break;
        }
      }
    }
  }
  for (i = 0; i < m_numSkills; ++i) {
    if (m_skills[i]->spellID >= 0 && !(m_skills[i]->invSlots & m_invTypeFilter)) {
      m_skills[i]->enabled = 0;
      --m_filteredSkills;
    } else {
      m_skills[i]->enabled = 1;
      if (m_numSubClasses) {
        UINT subClass = 0;
        for (j = 0; j < m_numSubClasses; ++j) {
          if (m_skills[i]->classID == m_subClasses[j]->classID && m_skills[i]->subClassID == m_subClasses[j]->subClassID) {
            subClass = j;
            break;
          }
        }
        if (!m_subClasses[subClass]->enabled || !m_subClasses[subClass]->filteredCount) {
          m_skills[i]->enabled = 0;
          --m_filteredSkills;
        } else if (m_skills[i]->spellID >= 0 && m_subClasses[subClass]->collapsed) {
          m_skills[i]->enabled = 0;
          --m_filteredSkills;
        }
      }
    }
  }
  qsort(m_subClasses.Ptr(), m_numSubClasses, sizeof(TradeSkillSubClassInfo *), QSortSubClasses);
  qsort(m_skills.Ptr(), m_numSkills, sizeof(TradeSkillInfo *), QSortSkills);
}

int CGTradeSkillInfo::GetSubClassIndexFromSkill(UINT index) {
  if (index < m_numSkills && m_skills[index]->spellID < 0) {
    for (UINT i = 0; i < m_numSubClasses; ++i) {
      if (m_subClasses[i]->classID == m_skills[index]->classID && m_subClasses[i]->subClassID == m_skills[index]->subClassID) {
        return i;
      }
    }
  }
  return -1;
}

BOOL CGTradeSkillInfo::IsCollpasedHeader(UINT index) {
  if (index < m_numSkills && m_skills[index]->spellID < 0) {
    for (UINT i = 0; i < m_numSubClasses; ++i) {
      if (m_subClasses[i]->classID == m_skills[index]->classID && m_subClasses[i]->subClassID == m_skills[index]->subClassID) {
        return (m_collapseFilter & (1 << i)) == 0;
      }
    }
  }
  return 0;
}

void CGTradeSkillInfo::SetSubClassFilter(int filter) {
  m_subClassFilter = filter;
  FilterAndSortSkills();
  FrameScript_SignalEvent(291);
}

void CGTradeSkillInfo::SetInvTypeFilter(int filter) {
  m_invTypeFilter = filter;
  FilterAndSortSkills();
  FrameScript_SignalEvent(291);
}

void CGTradeSkillInfo::SetCollapseFilter(int filter) {
  m_collapseFilter = filter;
  FilterAndSortSkills();
  FrameScript_SignalEvent(291);
}

static int Script_CloseTradeSkill(lua_State *) {
  CGTradeSkillInfo::Close();
  return 0;
}

static int Script_GetNumTradeSkills(lua_State *L) {
  lua_pushnumber(L, CGTradeSkillInfo::GetNumTradeSkills());
  return 1;
}

static int Script_GetTradeSkillInfo(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetTradeSkillInfo(index)");
    return 0;
  }
  int                   index = (int)lua_tonumber(L, 1) - 1;
  const TradeSkillInfo *info = CGTradeSkillInfo::GetTradeSkillInfo(index);
  if (info) {
    if (info->spellID == -1) {
      for (UINT i = 0; i < g_itemSubClassDB.GetNumRecords(); ++i) {
        const ItemSubClassRec *subClass = g_itemSubClassDB.GetRecordByIndex(i);
        if (subClass->m_classID == info->classID && subClass->m_subClassID == info->subClassID) {
          LPCSTR name = subClass->m_verboseName_lang[CURRENT_LANGUAGE];
          if (!name || !*name) {
            name = subClass->m_displayName_lang[CURRENT_LANGUAGE];
          }
          lua_pushstring(L, name);
          lua_pushstring(L, "header");
          lua_pushnumber(L, 0.0);
          if (CGTradeSkillInfo::IsCollpasedHeader(index)) {
            lua_pushnil(L);
          } else {
            lua_pushnumber(L, 1.0);
          }
          return 4;
        }
      }
    } else {
      const SpellRec *spell = g_spellDB.GetRecord(info->spellID);
      if (spell) {
        lua_pushstring(L, spell->m_name_lang[CURRENT_LANGUAGE]);
        lua_pushstring(L, s_skillCategoryStrings[info->category]);
        lua_pushnumber(L, info->numAvailable);
        lua_pushnil(L);
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

static int Script_SelectTradeSkill(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: SelectTradeSkill(index)");
    return 0;
  }
  CGTradeSkillInfo::SetSelection((int)lua_tonumber(L, 1) - 1);
  return 0;
}

static int Script_GetTradeSkillSelectionIndex(lua_State *L) {
  lua_pushnumber(L, CGTradeSkillInfo::GetSelectionIndex() + 1);
  return 1;
}

static int Script_GetTradeSkillIcon(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetTradeSkillIcon(index)");
    return 0;
  }
  const TradeSkillInfo *info = CGTradeSkillInfo::GetTradeSkillInfo((int)lua_tonumber(L, 1) - 1);
  if (info && info->spellID >= 0) {
    const SpellRec *spell = g_spellDB.GetRecord(info->spellID);
    if (spell) {
      const ItemStats_C *stats =
          g_itemDBCache.GetRecord(spell->m_effectItemType[0], (DWORDLONG)info->spellID | 0xB000000000000000ui64, TradeSkillItemCallback, 0);
      if (stats) {
        char   buffer[MAX_PATH];
        LPCSTR path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
        SStrPrintf(buffer, sizeof(buffer), "%s%s", path, *path ? "\\" : "");
        SStrPack(buffer, CGItem_C::GetInventoryArt(stats->m_displayInfoID), sizeof(buffer));
        lua_pushstring(L, buffer);
        return 1;
      }
    }
  }
  lua_pushnil(L);
  return 1;
}

static int Script_GetTradeSkillLine(lua_State *L) {
  const SkillLineRec *line = g_skillLineDB.GetRecord(CGTradeSkillInfo::GetSkillLine());
  if (line) {
    lua_pushstring(L, line->m_displayName_lang[CURRENT_LANGUAGE]);
  } else {
    lua_pushstring(L, "UNKNOWN");
  }
  return 1;
}

static int Script_GetTradeSkillItemStats(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetTradeSkillItemStats(index)");
    return 0;
  }
  int                   index = (int)lua_tonumber(L, 1) - 1;
  const TradeSkillInfo *info = CGTradeSkillInfo::GetTradeSkillInfo(index);
  if (!info) {
    return 0;
  }
  const SpellRec *spell = g_spellDB.GetRecord(info->spellID);
  if (!spell) {
    return 0;
  }
  const ItemSubClassRec *subClass;
  char                   levelBuf[128];
  int                    usable;
  char                   temp[128];
  int                    count;
  int                    itemID = spell->m_effectItemType[0];
  const ItemStats       *stats = g_itemDBCache.GetRecord(itemID, (DWORDLONG)spell->m_ID | 0xB000000000000000ui64, TradeSkillItemCallback, 0);
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
  for (int i = 0; i < g_itemSubClassDB.GetNumRecords(); ++i) {
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
    int canUse = stats->m_requiredSkillRank > player->GetSkillRank(stats->m_requiredSkill);
    SStrPrintf(buf, sizeof(buf), "%s%s%s", canUse ? "" : "|cffff2020", levelBuf, canUse ? "" : "|r");
    lua_pushstring(L, buf);
    ++count;
  }
  return count;
}

static int Script_GetTradeSkillItemLink(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetTradeSkillItemLink(index)");
    return 0;
  }
  const TradeSkillInfo *info = CGTradeSkillInfo::GetTradeSkillInfo((int)lua_tonumber(L, 1) - 1);
  if (info && info->spellID >= 0) {
    const SpellRec *spell = g_spellDB.GetRecord(info->spellID);
    if (spell) {
      int                itemID = spell->m_effectItemType[0];
      const ItemStats_C *stats = g_itemDBCache.GetRecord(itemID, 0, 0, 0);
      if (stats) {
        char link[1024];
        SStrPrintf(link, sizeof(link), "|Hitem:%d|h[%s]|h", itemID, stats->m_displayName[0]);
        lua_pushstring(L, link);
        return 1;
      }
    }
  }
  return 0;
}

static int Script_GetTradeSkillNumReagents(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetTradeSkillNumReagents(index)");
    return 0;
  }
  int                   index = (int)lua_tonumber(L, 1) - 1;
  int                   count = 0;
  const TradeSkillInfo *info = CGTradeSkillInfo::GetTradeSkillInfo(index);
  if (info && info->spellID >= 0) {
    const SpellRec *spell = g_spellDB.GetRecord(info->spellID);
    if (spell) {
      for (UINT i = 0; i < 8; ++i) {
        if (spell->m_reagent[i]) {
          ++count;
        }
      }
    }
  }
  lua_pushnumber(L, count);
  return 1;
}

static int Script_GetTradeSkillReagentInfo(lua_State *L) {
  if (lua_isnumber(L, 1) && lua_isnumber(L, 2)) {
    int                   index = (int)lua_tonumber(L, 1) - 1;
    int                   reagentIndex = lua_tonumber(L, 2);
    const TradeSkillInfo *info = CGTradeSkillInfo::GetTradeSkillInfo(index);
    if (info && info->spellID >= 0) {
      const SpellRec *spell = g_spellDB.GetRecord(info->spellID);
      if (spell) {
        int count = 0;
        for (UINT i = 0; i < 8; ++i) {
          if (spell->m_reagent[i]) {
            ++count;
            if (count == reagentIndex) {
              int                itemID = spell->m_reagent[i];
              const ItemStats_C *stats =
                  g_itemDBCache.GetRecord(itemID, (DWORDLONG)info->spellID | 0xB000000000000000ui64, TradeSkillItemCallback, 0);
              if (stats) {
                lua_pushstring(L, stats->m_displayName[0]);
                char   buffer[MAX_PATH];
                LPCSTR path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
                SStrPrintf(buffer, sizeof(buffer), "%s%s", path, *path ? "\\" : "");
                SStrPack(buffer, CGItem_C::GetInventoryArt(stats->m_displayInfoID), sizeof(buffer));
                lua_pushstring(L, buffer);
              } else {
                lua_pushnil(L);
                lua_pushnil(L);
              }
              lua_pushnumber(L, spell->m_reagentCount[i]);
              CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
              if (player) {
                lua_pushnumber(L, player->CGPlayer_C::GetBag()->GetItemTypeCount(itemID, 0));
              } else {
                lua_pushnumber(L, 0.0);
              }
              return 4;
            }
          }
        }
      }
    }
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnil(L);
    return 4;
  }
  luaL_error(L, "Usage: GetTradeSkillReagentInfo(index, reagentIndex)");
  return 0;
}

static int Script_GetTradeSkillTools(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetTradeSkillTools(index)");
    return 0;
  }
  UINT                  count = 0;
  const TradeSkillInfo *info = CGTradeSkillInfo::GetTradeSkillInfo((int)lua_tonumber(L, 1) - 1);
  if (info && info->spellID >= 0) {
    const SpellRec *spell = g_spellDB.GetRecord(info->spellID);
    if (spell) {
      if (spell->m_requiresSpellFocus) {
        const SpellFocusObjectRec *focus = g_spellFocusObjectDB.GetRecord(spell->m_requiresSpellFocus);
        if (focus) {
          lua_pushstring(L, focus->m_name_lang[CURRENT_LANGUAGE]);
          ++count;
        }
      }
      for (UINT i = 0; i < 2; ++i) {
        if (spell->m_totem[i]) {
          const ItemStats_C *stats =
              g_itemDBCache.GetRecord(spell->m_totem[i], (DWORDLONG)info->spellID | 0xB000000000000000ui64, TradeSkillItemCallback, 0);
          if (stats) {
            lua_pushstring(L, stats->m_displayName[0]);
            ++count;
          }
        }
      }
    }
  }
  return count;
}

static int Script_GetTradeSkillSubClasses(lua_State *L) {
  UINT count = CGTradeSkillInfo::GetNumSubClasses();
  for (UINT i = 0; i < count; ++i) {
    TradeSkillSubClassInfo *info = CGTradeSkillInfo::GetSubClass(i);
    for (UINT j = 0; j < g_itemSubClassDB.GetNumRecords(); ++j) {
      const ItemSubClassRec *rec = g_itemSubClassDB.GetRecordByIndex(j);
      if (rec->m_classID == info->classID && rec->m_subClassID == info->subClassID) {
        LPCSTR name = rec->m_verboseName_lang[CURRENT_LANGUAGE];
        if (!name || !*name) {
          name = rec->m_displayName_lang[CURRENT_LANGUAGE];
        }
        lua_pushstring(L, name);
        break;
      }
    }
  }
  return count;
}

static int Script_GetTradeSkillInvSlots(lua_State *L) {
  int available = CGTradeSkillInfo::GetAvailableSlots();
  int count = 0;
  for (UINT i = 0; i < 24; ++i) {
    if ((1 << i) & available) {
      lua_pushstring(L, FrameScript_GetText(s_invSlotTokens[i], -1, GENDER_NOT_APPLICABLE));
      ++count;
    }
  }
  return count;
}

static int Script_SetTradeSkillSubClassFilter(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: SetTradeSkillSubClassFilter(index [, on\\off, exclusive])");
    return 0;
  }
  int index = (int)lua_tonumber(L, 1) - 1;
  if (index < 0) {
    CGTradeSkillInfo::SetSubClassFilter(-1);
    return 0;
  }
  if (index >= (int)CGTradeSkillInfo::GetNumSubClasses()) {
    luaL_error(L, "Bad sub class in SetTradeSkillSubClassFilter");
    return 0;
  }
  if (!lua_isnumber(L, 2)) {
    luaL_error(L, "Missing on//off parameter in SetTradeSkillSubClassFilter");
    return 0;
  }
  int filter = CGTradeSkillInfo::GetSubClassFilter();
  if (!(int)lua_tonumber(L, 2)) {
    CGTradeSkillInfo::SetSubClassFilter(filter & ~(1 << index));
    return 0;
  }
  if (lua_isnumber(L, 3) && (int)lua_tonumber(L, 3)) {
    CGTradeSkillInfo::SetSubClassFilter(1 << index);
    return 0;
  }
  CGTradeSkillInfo::SetSubClassFilter(filter | (1 << index));
  return 0;
}

static int Script_GetTradeSkillSubClassFilter(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetTradeSkillSubClassFilter(index)");
    return 0;
  }
  int filter = CGTradeSkillInfo::GetSubClassFilter();
  int index = (int)lua_tonumber(L, 1) - 1;
  if (index < 0) {
    for (UINT i = 0; i < CGTradeSkillInfo::GetNumSubClasses(); ++i) {
      if (!((1 << i) & filter)) {
        lua_pushnil(L);
        return 1;
      }
    }
    lua_pushnumber(L, 1.0);
    return 1;
  }
  if (index >= (int)CGTradeSkillInfo::GetNumSubClasses()) {
    luaL_error(L, "Bad sub class in GetTradeSkillSubClassFilter");
    return 0;
  }
  if ((1 << index) & filter) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_SetTradeSkillInvSlotFilter(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: SetTradeSkillInvSlotFilter(index [, on\\off, exclusive])");
    return 0;
  }
  int index = (int)lua_tonumber(L, 1) - 1;
  if (index < 0) {
    CGTradeSkillInfo::SetInvTypeFilter(-1);
    return 0;
  }
  int available = CGTradeSkillInfo::GetAvailableSlots();
  int slot = -1;
  int count = 0;
  for (UINT i = 0; i < 24; ++i) {
    if ((1 << i) & available) {
      if (count == index) {
        slot = i;
        break;
      }
      ++count;
    }
  }
  if (slot == -1) {
    luaL_error(L, "Bad inv slot in SetTradeSkillInvSlotFilter");
    return 0;
  }
  if (!lua_isnumber(L, 2)) {
    luaL_error(L, "Missing on//off parameter in SetTradeSkillInvSlotFilter");
    return 0;
  }
  int filter = CGTradeSkillInfo::GetInvTypeFilter();
  if (!(int)lua_tonumber(L, 2)) {
    CGTradeSkillInfo::SetInvTypeFilter(filter & ~(1 << slot));
    return 0;
  }
  if (lua_isnumber(L, 3) && (int)lua_tonumber(L, 3)) {
    CGTradeSkillInfo::SetInvTypeFilter(1 << slot);
    return 0;
  }
  CGTradeSkillInfo::SetInvTypeFilter(filter | (1 << slot));
  return 0;
}

static int Script_GetTradeSkillInvSlotFilter(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetTradeSkillInvSlotFilter(index)");
    return 0;
  }
  int filter = CGTradeSkillInfo::GetInvTypeFilter();
  int index = (int)lua_tonumber(L, 1) - 1;
  if (index < 0) {
    if ((CGTradeSkillInfo::GetAvailableSlots() & CGTradeSkillInfo::GetInvTypeFilter()) == CGTradeSkillInfo::GetAvailableSlots()) {
      lua_pushnumber(L, 1.0);
    } else {
      lua_pushnil(L);
    }
    return 1;
  }
  int available = CGTradeSkillInfo::GetAvailableSlots();
  int slot = -1;
  int count = 0;
  for (UINT i = 0; i < 24; ++i) {
    if ((1 << i) & available) {
      if (count == index) {
        slot = i;
        break;
      }
      ++count;
    }
  }
  if (slot == -1) {
    luaL_error(L, "Bad inv type in GetTradeSkillInvSlotFilter");
    return 0;
  }
  if ((1 << slot) & filter) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Script_CollapseTradeSkillSubClass(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: CollapseTradeSkillSubClass(index)");
    return 0;
  }
  int index = (int)lua_tonumber(L, 1) - 1;
  if (index < 0) {
    CGTradeSkillInfo::SetCollapseFilter(0);
  } else {
    int subClass = CGTradeSkillInfo::GetSubClassIndexFromSkill(index);
    if (subClass < 0) {
      luaL_error(L, "Bad sub class in CollapseTradeSkillSubClass");
      return 0;
    }
    CGTradeSkillInfo::SetCollapseFilter(CGTradeSkillInfo::GetCollapseFilter() & ~(1 << subClass));
  }
  return 0;
}

static int Script_ExpandTradeSkillSubClass(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: ExpandTradeSkillSubClass(index)");
    return 0;
  }
  int index = (int)lua_tonumber(L, 1) - 1;
  if (index < 0) {
    CGTradeSkillInfo::SetCollapseFilter(-1);
  } else {
    int subClass = CGTradeSkillInfo::GetSubClassIndexFromSkill(index);
    if (subClass < 0) {
      luaL_error(L, "Bad skill line in ExpandTradeSkillSubClass");
      return 0;
    }
    CGTradeSkillInfo::SetCollapseFilter(CGTradeSkillInfo::GetCollapseFilter() | (1 << subClass));
  }
  return 0;
}

static int Script_DoTradeSkill(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: DoTradeSkill(index)");
    return 0;
  }
  const TradeSkillInfo *info = CGTradeSkillInfo::GetTradeSkillInfo((UINT)lua_tonumber(L, 1) - 1);
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

void TradeSkillRegisterScriptFunctions() {
  for (UINT i = 0; i < 21; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void TradeSkillUnregisterScriptFunctions() {
  for (UINT i = 0; i < 21; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
