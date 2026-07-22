#include "ObjectMgrClient/ObjectMgrClient.h"
#include "Object/ObjectClient/Player_C.h"
#include "DB/DBClient/AutoCode/SkillLineAbilityRec.h"
#include "DB/DBClient/AutoCode/SkillLineRec.h"
#include "DB/DBClient/AutoCode/SpellFocusObjectRec.h"
#include "DB/DBClient/AutoCode/SpellIconRec.h"
#include "DB/DBClient/AutoCode/SpellRec.h"
#include "DB/DBClient/DBClient.h"
#include "DB/DBClient/DBCacheInstances.h"
#include "Object/ItemStats.h"
#include "Object/ObjectClient/Item_C.h"

#include <FrameScript/FrameScript.h>
#include <lauxlib.h>
#include <lua.h>
#include <stpl.h>
#include <stdlib.h>

enum CRAFT_LEVEL_CATEGORY {
  CRAFT_LEVEL_NONE = 0,
  CRAFT_LEVEL_OPTIMAL = 1,
  CRAFT_LEVEL_MEDIUM = 2,
  CRAFT_LEVEL_EASY = 3,
  CRAFT_LEVEL_TRIVIAL = 4
};

struct CraftInfo {
  int                  spellID;
  int                  skillLine;
  CRAFT_LEVEL_CATEGORY category;
};

struct CraftSkillLineInfo {
  int skillLine;
  int filteredCount;
  int collapsed;
};

static int __cdecl QSortSkills(const void *a, const void *b);
static int __cdecl QSortPetSkills(const void *a, const void *b);
static int __cdecl QSortSkillLines(const void *a, const void *b);

bool __fastcall Spell_C_CastSpell(int spellID, const CGItem_C *item);
bool __fastcall SpellParserParseText(const SpellRec *spell, char *buf, unsigned int size, int isPet);

class CGCraftInfo {
 public:
  static void __fastcall               EnterWorld();
  static void __fastcall               ShutdownGame();
  static void __fastcall               Close();
  static void __fastcall               SetSelection(int index);
  static int __fastcall                GetSelectionIndex();
  static SPELL_CAST_UI_TYPE __fastcall GetCraftType() {
    return m_craftType;
  }
  static int __fastcall GetNumCrafts() {
    return m_filteredSkills;
  }
  static CraftInfo *__fastcall GetCraftInfo(unsigned int index) {
    return index < m_filteredSkills ? m_skills[index] : 0;
  }
  static unsigned int __fastcall GetNumSkillLines() {
    return m_numSkillLines;
  }
  static CraftSkillLineInfo *__fastcall GetSkillLine(unsigned int index) {
    return index < m_numSkillLines ? m_skillLines[index] : 0;
  }
  static int __fastcall  GetSkillLineIndexFromCraft(unsigned int index);
  static void __fastcall SetCraftType(SPELL_CAST_UI_TYPE type);
  static void __fastcall RefreshList();
  static int __fastcall  IsCollpasedHeader(unsigned int index);
  static int __fastcall  GetCollapseFilter() {
    return m_collapseFilter;
  }
  static void __fastcall SetCollapseFilter(int filter);

 private:
  friend int __cdecl QSortSkills(const void *a, const void *b);
  friend int __cdecl QSortPetSkills(const void *a, const void *b);
  friend int __cdecl QSortSkillLines(const void *a, const void *b);

 protected:
  static void __fastcall FilterAndSortSkills();

 private:
  static SPELL_CAST_UI_TYPE                    m_craftType;
  static int                                   m_currentSelection;
  static unsigned int                          m_numSkills;
  static unsigned int                          m_numSkillLines;
  static unsigned int                          m_filteredSkills;
  static int                                   m_collapseFilter;
  static TSGrowableArray<CraftInfo *>          m_skills;
  static TSGrowableArray<CraftSkillLineInfo *> m_skillLines;
};

SPELL_CAST_UI_TYPE                    CGCraftInfo::m_craftType;
int                                   CGCraftInfo::m_currentSelection;
unsigned int                          CGCraftInfo::m_numSkills;
unsigned int                          CGCraftInfo::m_numSkillLines;
unsigned int                          CGCraftInfo::m_filteredSkills;
int                                   CGCraftInfo::m_collapseFilter;
TSGrowableArray<CraftInfo *>          CGCraftInfo::m_skills;
TSGrowableArray<CraftSkillLineInfo *> CGCraftInfo::m_skillLines;

static const char *s_craftButtonTokens[4] = {"USE", "TRAIN", "DISGUISE", "ENSCRIBE"};
static const char  s_skillCategoryStrings[5][32] = {"none", "optimal", "medium", "easy", "trivial"};

static void __fastcall CraftReagentItemCallback(int, const unsigned __int64 &, void *, bool granted) {
  if (granted) {
    CGCraftInfo::RefreshList();
  }
}

void __fastcall CGCraftInfo::EnterWorld() {
  m_craftType = SPELL_CAST_UI_NONE;
  m_currentSelection = 0;
  m_numSkills = 0;
  m_numSkillLines = 0;
  m_filteredSkills = 0;
}

void __fastcall CGCraftInfo::ShutdownGame() {
  m_skills.Clear();
  m_skillLines.Clear();
}

void __fastcall CGCraftInfo::Close() {
  m_craftType = SPELL_CAST_UI_NONE;
  FrameScript_SignalEvent(345);
}

void __fastcall CGCraftInfo::SetCraftType(SPELL_CAST_UI_TYPE type) {
  if (type == m_craftType) {
    Close();
  } else {
    m_craftType = type;
    m_currentSelection = 0;
    m_collapseFilter = -1;
    RefreshList();
    FrameScript_SignalEvent(343);
  }
}

void __fastcall CGCraftInfo::SetSelection(int index) {
  CraftInfo *info = GetCraftInfo(index);
  m_currentSelection = info && info->spellID > 0 ? info->spellID : 0;
}

int __fastcall CGCraftInfo::GetSelectionIndex() {
  if (!m_currentSelection) {
    return -1;
  }
  unsigned int index;
  for (index = 0; index < m_numSkills; ++index) {
    if (m_skills[index]->spellID == m_currentSelection) {
      break;
    }
  }
  return index == m_numSkills ? -1 : index;
}

static int __cdecl QSortSkills(const void *a, const void *b) {
  FATALASSERT(a);
  FATALASSERT(b);
  CraftInfo   *info1 = *static_cast<CraftInfo *const *>(a);
  CraftInfo   *info2 = *static_cast<CraftInfo *const *>(b);
  unsigned int skillLineRank1 = 0;
  unsigned int skillLineRank2 = 0;
  int          enabled1 = 1;
  int          enabled2 = 1;
  for (unsigned int i = 0; i < CGCraftInfo::m_numSkillLines; ++i) {
    CraftSkillLineInfo *line = CGCraftInfo::m_skillLines[i];
    if (line->skillLine == info1->skillLine && info1->spellID >= 0) {
      skillLineRank1 = i;
      enabled1 = !line->collapsed;
    }
    if (line->skillLine == info2->skillLine && info2->spellID >= 0) {
      skillLineRank2 = i;
      enabled2 = !line->collapsed;
    }
  }
  if (!enabled1) {
    return enabled2 ? 1 : 0;
  }
  if (!enabled2) {
    return -1;
  }
  if (skillLineRank1 != skillLineRank2) {
    return skillLineRank1 < skillLineRank2 ? -1 : 1;
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
  return SStrCmp(spell1->m_name_lang[CURRENT_LANGUAGE], spell2->m_name_lang[CURRENT_LANGUAGE], 0x7FFFFFFF);
}

static int __cdecl QSortPetSkills(const void *a, const void *b) {
  FATALASSERT(a);
  FATALASSERT(b);
  CraftInfo   *info1 = *static_cast<CraftInfo *const *>(a);
  CraftInfo   *info2 = *static_cast<CraftInfo *const *>(b);
  unsigned int skillLineRank1 = 0;
  unsigned int skillLineRank2 = 0;
  int          enabled1 = 1;
  int          enabled2 = 1;
  for (unsigned int i = 0; i < CGCraftInfo::m_numSkillLines; ++i) {
    CraftSkillLineInfo *line = CGCraftInfo::m_skillLines[i];
    if (line->skillLine == info1->skillLine && info1->spellID >= 0) {
      skillLineRank1 = i;
      enabled1 = !line->collapsed;
    }
    if (line->skillLine == info2->skillLine && info2->spellID >= 0) {
      skillLineRank2 = i;
      enabled2 = !line->collapsed;
    }
  }
  if (!enabled1) {
    return enabled2 ? 1 : 0;
  }
  if (!enabled2) {
    return -1;
  }
  if (skillLineRank1 != skillLineRank2) {
    return skillLineRank1 < skillLineRank2 ? -1 : 1;
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
  int result = SStrCmp(spell1->m_nameSubtext_lang[CURRENT_LANGUAGE], spell2->m_nameSubtext_lang[CURRENT_LANGUAGE], 0x7FFFFFFF);
  return result ? result : SStrCmp(spell1->m_name_lang[CURRENT_LANGUAGE], spell2->m_name_lang[CURRENT_LANGUAGE], 0x7FFFFFFF);
}

static int __cdecl QSortSkillLines(const void *a, const void *b) {
  FATALASSERT(a);
  FATALASSERT(b);
  CraftSkillLineInfo *info1 = *static_cast<CraftSkillLineInfo *const *>(a);
  CraftSkillLineInfo *info2 = *static_cast<CraftSkillLineInfo *const *>(b);
  if (info1->skillLine == info2->skillLine) {
    return 0;
  }
  const SkillLineRec *line1 = g_skillLineDB.GetRecord(info1->skillLine);
  const SkillLineRec *line2 = g_skillLineDB.GetRecord(info2->skillLine);
  return line1 && line2 ? SStrCmp(line1->m_displayName_lang[CURRENT_LANGUAGE], line2->m_displayName_lang[CURRENT_LANGUAGE], 0x7FFFFFFF) : 0;
}

void __fastcall CGCraftInfo::RefreshList() {
  unsigned int i;

  m_numSkills = 0;
  m_numSkillLines = 0;
  if (!m_craftType) {
    return;
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return;
  }
  TSGrowableArray<int> *spells = player->GetCraftSkills(m_craftType);
  if (!spells) {
    return;
  }
  while (m_skills.Count() < spells->Count()) {
    CraftInfo *info = NEW(CraftInfo);
    m_skills.Add(1, &info);
  }
  m_numSkills = spells->Count();
  for (i = 0; i < m_numSkills; ++i) {
    CraftInfo *info = m_skills[i];
    info->spellID = (*spells)[i];
    const SkillLineAbilityRec *ability = player->LookupAbility(info->spellID);
    int                        skillLine = ability ? ability->m_skillLine : 0;
    info->skillLine = skillLine;
    info->category = CRAFT_LEVEL_NONE;
    if (ability && ability->m_trivialSkillLineRankHigh) {
      int high = ability->m_trivialSkillLineRankHigh;
      int low = ability->m_trivialSkillLineRankLow;
      if (!low) {
        low = high == 25 ? 0 : high - 25;
      }
      int medium = (low + high) / 2;
      int rank = player->GetSkillRank(skillLine);
      info->category = rank < low ? CRAFT_LEVEL_OPTIMAL : rank < medium ? CRAFT_LEVEL_MEDIUM : rank < high ? CRAFT_LEVEL_EASY : CRAFT_LEVEL_TRIVIAL;
    }
    unsigned int lineIndex;
    for (lineIndex = 0; lineIndex < m_numSkillLines; ++lineIndex) {
      if (m_skillLines[lineIndex]->skillLine == skillLine) {
        break;
      }
    }
    if (lineIndex == m_numSkillLines) {
      if (m_skillLines.Count() <= lineIndex) {
        CraftSkillLineInfo *line = NEW(CraftSkillLineInfo);
        m_skillLines.Add(1, &line);
      }
      m_skillLines[lineIndex]->skillLine = skillLine;
      ++m_numSkillLines;
    }
  }
  while (m_skills.Count() < m_numSkills + m_numSkillLines) {
    CraftInfo *info = NEW(CraftInfo);
    m_skills.Add(1, &info);
  }
  for (i = 0; i < m_numSkillLines; ++i) {
    CraftInfo *info = m_skills[m_numSkills + i];
    info->spellID = -1;
    info->skillLine = m_skillLines[i]->skillLine;
  }
  if (m_numSkillLines == 1) {
    m_numSkillLines = 0;
  }
  m_numSkills += m_numSkillLines;
  FilterAndSortSkills();
  FrameScript_SignalEvent(344);
}

void __fastcall CGCraftInfo::FilterAndSortSkills() {
  unsigned int i;
  unsigned int j;

  m_filteredSkills = m_numSkills;
  for (i = 0; i < m_numSkillLines; ++i) {
    m_skillLines[i]->collapsed = !(m_collapseFilter & (1 << i));
    m_skillLines[i]->filteredCount = 0;
  }
  for (i = 0; i < m_numSkills; ++i) {
    if (m_skills[i]->spellID >= 0) {
      for (j = 0; j < m_numSkillLines; ++j) {
        if (m_skills[i]->skillLine == m_skillLines[j]->skillLine) {
          ++m_skillLines[j]->filteredCount;
        }
      }
    }
  }
  for (i = 0; i < m_numSkills; ++i) {
    if (!m_numSkillLines) {
      continue;
    }
    unsigned int lineIndex = 0;
    for (j = 0; j < m_numSkillLines; ++j) {
      if (m_skills[i]->skillLine == m_skillLines[j]->skillLine) {
        lineIndex = j;
        break;
      }
    }
    CraftSkillLineInfo *line = m_skillLines[lineIndex];
    if (!line->filteredCount || (m_skills[i]->spellID >= 0 && line->collapsed)) {
      --m_filteredSkills;
    }
  }
  qsort(m_skillLines.Ptr(), m_numSkillLines, sizeof(CraftSkillLineInfo *), QSortSkillLines);
  qsort(m_skills.Ptr(), m_numSkills, sizeof(CraftInfo *), m_craftType == SPELL_CAST_UI_PET_TRAINING ? QSortPetSkills : QSortSkills);
}

int __fastcall CGCraftInfo::GetSkillLineIndexFromCraft(unsigned int index) {
  CraftInfo *info = GetCraftInfo(index);
  if (!info || info->spellID >= 0) {
    return -1;
  }
  for (unsigned int line = 0; line < m_numSkillLines; ++line) {
    if (m_skillLines[line]->skillLine == info->skillLine) {
      return line;
    }
  }
  return -1;
}

int __fastcall CGCraftInfo::IsCollpasedHeader(unsigned int index) {
  int line = GetSkillLineIndexFromCraft(index);
  return line >= 0 && !(m_collapseFilter & (1 << line));
}

void __fastcall CGCraftInfo::SetCollapseFilter(int filter) {
  m_collapseFilter = filter;
  FilterAndSortSkills();
  FrameScript_SignalEvent(344);
}

static int __fastcall Script_CloseCraft(lua_State *__formal) {
  CGCraftInfo::Close();
  return 0;
}

static int __fastcall Script_GetCraftName(lua_State *L) {
  CGPlayer_C     *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  const SpellRec *spell = player ? g_spellDB.GetRecord(player->GetSkillIndex(CGCraftInfo::GetCraftType())) : 0;
  lua_pushstring(L, spell ? spell->m_name_lang[CURRENT_LANGUAGE] : 0);
  return 1;
}

static int __fastcall Script_GetCraftButtonToken(lua_State *L) {
  lua_pushstring(L, s_craftButtonTokens[CGCraftInfo::GetCraftType()]);
  return 1;
}

static int __fastcall Script_GetNumCrafts(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGCraftInfo::GetNumCrafts()));
  return 1;
}

static int __fastcall Script_GetCraftInfo(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetCraftInfo(index)");
  }
  unsigned int index = static_cast<unsigned int>(lua_tonumber(L, 1)) - 1;
  CraftInfo   *info = CGCraftInfo::GetCraftInfo(index);
  if (info && info->spellID != -1) {
    const SpellRec *spell = g_spellDB.GetRecord(info->spellID);
    if (spell) {
      lua_pushstring(L, spell->m_name_lang[CURRENT_LANGUAGE]);
      lua_pushstring(L, spell->m_nameSubtext_lang[CURRENT_LANGUAGE]);
      lua_pushstring(L, s_skillCategoryStrings[info->category]);
      lua_pushnil(L);
      return 4;
    }
  } else if (info) {
    const SkillLineRec *line = g_skillLineDB.GetRecord(info->skillLine);
    if (line) {
      lua_pushstring(L, line->m_displayName_lang[CURRENT_LANGUAGE]);
      lua_pushstring(L, "header");
      if (CGCraftInfo::IsCollpasedHeader(index)) {
        lua_pushnil(L);
      } else {
        lua_pushnumber(L, 1.0);
      }
      return 3;
    }
  }
  lua_pushnil(L);
  lua_pushnil(L);
  lua_pushnil(L);
  lua_pushnil(L);
  return 4;
}

static int __fastcall Script_SelectCraft(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: SelectCraft(index)");
  }
  CGCraftInfo::SetSelection(static_cast<int>(lua_tonumber(L, 1)) - 1);
  return 0;
}

static int __fastcall Script_GetCraftSelectionIndex(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGCraftInfo::GetSelectionIndex() + 1));
  return 1;
}

static int __fastcall Script_GetCraftIcon(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetTradeSkillIcon(index)");
  }
  CraftInfo          *info = CGCraftInfo::GetCraftInfo(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  const SpellRec     *spell = info && info->spellID >= 0 ? g_spellDB.GetRecord(info->spellID) : 0;
  const SpellIconRec *icon = spell ? g_spellIconDB.GetRecord(spell->m_spellIconID) : 0;
  lua_pushstring(L, icon ? icon->m_textureFilename : 0);
  return 1;
}

static int __fastcall Script_GetCraftSkillLine(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetCraftSkillLine(index)");
  }
  CraftInfo          *info = CGCraftInfo::GetCraftInfo(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  const SkillLineRec *line = info ? g_skillLineDB.GetRecord(info->skillLine) : 0;
  lua_pushstring(L, line ? line->m_displayName_lang[CURRENT_LANGUAGE] : 0);
  return 1;
}

static int __fastcall Script_GetCraftNumReagents(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetCraftNumReagents(index)");
  }
  CraftInfo      *info = CGCraftInfo::GetCraftInfo(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
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

static int __fastcall Script_GetCraftReagentInfo(lua_State *L) {
  if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
    return luaL_error(L, "Usage: GetCraftReagentInfo(index, reagentIndex)");
  }
  CraftInfo      *info = CGCraftInfo::GetCraftInfo(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
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
    const ItemStats_C *stats = g_itemDBCache.GetRecord(itemID, guid, CraftReagentItemCallback, 0);
    if (stats) {
      lua_pushstring(L, stats->m_displayName[0]);
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

static int __fastcall Script_GetCraftSpellFocus(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetTradeSkillSpellFocus(index)");
  }
  CraftInfo      *info = CGCraftInfo::GetCraftInfo(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  const SpellRec *spell = info && info->spellID >= 0 ? g_spellDB.GetRecord(info->spellID) : 0;
  unsigned int    count = 0;
  if (spell) {
    const SpellFocusObjectRec *focus = g_spellFocusObjectDB.GetRecord(spell->m_requiresSpellFocus);
    if (focus) {
      lua_pushstring(L, focus->m_name_lang[CURRENT_LANGUAGE]);
      ++count;
    }
    unsigned __int64 guid = static_cast<unsigned __int64>(spell->m_ID) | 0xB000000000000000ui64;
    for (unsigned int i = 0; i < 2; ++i) {
      if (spell->m_totem[i]) {
        const ItemStats_C *stats = g_itemDBCache.GetRecord(spell->m_totem[i], guid, CraftReagentItemCallback, 0);
        if (stats) {
          lua_pushstring(L, stats->m_displayName[0]);
          ++count;
        }
      }
    }
  }
  return count;
}

static int __fastcall Script_GetCraftDescription(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: GetCraftDescription(index)");
  }
  CraftInfo      *info = CGCraftInfo::GetCraftInfo(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  const SpellRec *spell = info && info->spellID >= 0 ? g_spellDB.GetRecord(info->spellID) : 0;
  if (spell) {
    char buf[1024];
    if (spell->m_description_lang[CURRENT_LANGUAGE] && *spell->m_description_lang[CURRENT_LANGUAGE]) {
      SpellParserParseText(spell, buf, sizeof(buf), CGCraftInfo::GetCraftType() == SPELL_CAST_UI_PET_TRAINING);
      lua_pushstring(L, buf);
      return 1;
    }
    for (unsigned int i = 0; i < 3; ++i) {
      if (spell->m_effect[i] == 36 || spell->m_effect[i] == 57) {
        const SpellRec *trigger = g_spellDB.GetRecord(spell->m_effectTriggerSpell[i]);
        if (trigger) {
          SpellParserParseText(trigger, buf, sizeof(buf), spell->m_effect[i] == 57);
          lua_pushstring(L, buf);
          return 1;
        }
      }
    }
  }
  lua_pushnil(L);
  return 1;
}

static int __fastcall Script_CollapseCraftSkillLine(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: CollapseCraftSkillLine(index)");
  }
  int index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  if (index < 0) {
    CGCraftInfo::SetCollapseFilter(0);
  } else {
    int line = CGCraftInfo::GetSkillLineIndexFromCraft(index);
    if (line < 0) {
      return luaL_error(L, "Bad skill line in CollapseCraftSkillLine");
    }
    CGCraftInfo::SetCollapseFilter(CGCraftInfo::GetCollapseFilter() & ~(1 << line));
  }
  return 0;
}

static int __fastcall Script_ExpandCraftSkillLine(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: ExpandCraftSkillLine(index)");
  }
  int index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  if (index < 0) {
    CGCraftInfo::SetCollapseFilter(-1);
  } else {
    int line = CGCraftInfo::GetSkillLineIndexFromCraft(index);
    if (line < 0) {
      return luaL_error(L, "Bad skill line in ExpandCraftSkillLine");
    }
    CGCraftInfo::SetCollapseFilter(CGCraftInfo::GetCollapseFilter() | (1 << line));
  }
  return 0;
}

static int __fastcall Script_DoCraft(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    return luaL_error(L, "Usage: DoCraft(index)");
  }
  CraftInfo *info = CGCraftInfo::GetCraftInfo(static_cast<unsigned int>(lua_tonumber(L, 1)) - 1);
  if (info) {
    Spell_C_CastSpell(info->spellID, 0);
  }
  return 0;
}

static FrameScript_Method s_ScriptFunctions[16] = {
    {            "CloseCraft",             Script_CloseCraft},
    {          "GetCraftName",           Script_GetCraftName},
    {   "GetCraftButtonToken",    Script_GetCraftButtonToken},
    {          "GetNumCrafts",           Script_GetNumCrafts},
    {          "GetCraftInfo",           Script_GetCraftInfo},
    {           "SelectCraft",            Script_SelectCraft},
    {"GetCraftSelectionIndex", Script_GetCraftSelectionIndex},
    {          "GetCraftIcon",           Script_GetCraftIcon},
    {     "GetCraftSkillLine",      Script_GetCraftSkillLine},
    {   "GetCraftNumReagents",    Script_GetCraftNumReagents},
    {   "GetCraftReagentInfo",    Script_GetCraftReagentInfo},
    {    "GetCraftSpellFocus",     Script_GetCraftSpellFocus},
    {   "GetCraftDescription",    Script_GetCraftDescription},
    {"CollapseCraftSkillLine", Script_CollapseCraftSkillLine},
    {  "ExpandCraftSkillLine",   Script_ExpandCraftSkillLine},
    {               "DoCraft",                Script_DoCraft}
};

void __fastcall CraftInfoRegisterScriptFunctions() {
  for (unsigned int i = 0; i < 16; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void __fastcall CraftInfoUnregisterScriptFunctions() {
  for (unsigned int i = 0; i < 16; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
