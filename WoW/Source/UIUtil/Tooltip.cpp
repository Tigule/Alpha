#include <Base/Base.h>
#include <Frame/CSimpleTop.h>
#include <Frame/CSimpleModel.h>
#include <WowConst.h>
#include "UIUtil/Camera.h"
#include "UIUtil/InputControl.h"
#include "UIUtil/Tooltip.h"
#include <MapDefs.h>
#include <WorldClient/World.h>
#include "Ui/GameUI.h"

#include "Tooltip.h"

#include "Console/ConsoleVar.h"
#include "DB/DBClient/AutoCode/ChrClassesRec.h"
#include "DB/DBClient/AutoCode/ChrRacesRec.h"
#include "DB/DBClient/AutoCode/CreatureTypeRec.h"
#include "DB/DBClient/AutoCode/ItemClassRec.h"
#include "DB/DBClient/AutoCode/ItemSubClassRec.h"
#include "DB/DBClient/AutoCode/LanguagesRec.h"
#include "DB/DBClient/AutoCode/LockRec.h"
#include "DB/DBClient/AutoCode/LockTypeRec.h"
#include "DB/DBClient/AutoCode/SkillLineAbilityRec.h"
#include "DB/DBClient/AutoCode/SkillLineRec.h"
#include "DB/DBClient/AutoCode/SpellRec.h"
#include "DB/DBClient/AutoCode/SpellAuraNamesRec.h"
#include "DB/DBClient/AutoCode/SpellDispelTypeRec.h"
#include "DB/DBClient/AutoCode/SpellEffectNamesRec.h"
#include "DB/DBClient/AutoCode/SpellItemEnchantmentRec.h"
#include "DB/DBClient/AutoCode/SpellRadiusRec.h"
#include "DB/DBClient/AutoCode/SpellRangeRec.h"
#include "DB/DBClient/AutoCode/SpellShapeshiftFormRec.h"
#include "DB/DBClient/DBClient.h"
#include "Game/GameClient/NameCache.h"
#include "Object/ObjectClient/Corpse_C.h"
#include "Object/ObjectClient/GameObject_C.h"
#include "Object/ObjectClient/Item_C.h"
#include "Object/ObjectClient/Player_C.h"
#include "Object/ObjectClient/Unit_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "Ui/ActionBarFrame.h"
#include "Ui/ClassTrainerFrame.h"
#include "Ui/ContainerFrame.h"
#include "Ui/GameUI.h"
#include "Ui/LootFrame.h"
#include "Ui/MerchantFrame.h"
#include "Ui/PartyFrame.h"
#include "Ui/PetInfo.h"
#include "Ui/QuestLog.h"
#include "Ui/QuestFrame.h"
#include "Ui/SpellBookFrame.h"
#include "Ui/TradeFrame.h"
#include "Ui/WorldFrame.h"
#include "WorldClient/AreaList.h"
#include "DB/DBClient/DBCacheInstances.h"
#include <Base/CDataStore.h>
#include <Base/Coordinate.h>
#include <Frame/CBackdropGenerator.h>
#include <Frame/CSimpleRender.h>
#include <Frame/SimpleFrameRegistry.h>
#include <FrameScript/FrameScript.h>
#include <Os/OsTime.h>
#include <lauxlib.h>
#include <lua.h>
#include <storm.h>

class CGBuffDesc {
  friend class CGBuffBar;

 public:
  CGBuffDesc();
  __forceinline ~CGBuffDesc() {
  }
  void SetAuraIndex(int index, CGPlayer_C *player);
  int  GetAuraIndex() const {
    return m_auraIndex;
  }
  int GetAuraSpell() const {
    return m_auraSpell;
  }
  BYTE GetAuraFlags() const {
    return m_auraFlags;
  }
  int GetUntilCancelled() const {
    return m_untilCancelled;
  }

 protected:
  int  m_auraIndex;
  int  m_auraSpell;
  BYTE m_auraFlags;
  int  m_untilCancelled;
};

class CGBuffBar {
 public:
  static void              InitializeGame();
  static void              ShutdownGame();
  static void              EnterWorld();
  static void              LeaveWorld();
  static void              UpdateBuffs();
  static void              UpdateDuration(BYTE slot, UINT duration);
  static const CGBuffDesc *GetBuffByFilter(int index, UINT filter, int &buffIndex);
  static const CGBuffDesc *GetBuffByIndex(int buffIndex);
  static UINT              GetBuffTimeLeftByIndex(int buffIndex);

 private:
  static CGBuffDesc m_buffs[56];
  static UINT       m_durations[56];
};

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

struct TradeSkillSubClassInfo;

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

enum CRAFT_LEVEL_CATEGORY {
  CRAFT_NONE = 0,
  CRAFT_OPTIMAL = 1,
  CRAFT_MEDIUM = 2,
  CRAFT_EASY = 3,
  CRAFT_TRIVIAL = 4,
  NUM_CRAFT_CATEGORIES = 5
};

struct CraftInfo {
  int                  spellID;
  int                  skillLine;
  CRAFT_LEVEL_CATEGORY category;
};

struct CraftSkillLineInfo;

class CGCraftInfo {
 public:
  static void               EnterWorld();
  static void               ShutdownGame();
  static void               Close();
  static void               SetSelection(int index);
  static int                GetSelectionIndex();
  static SPELL_CAST_UI_TYPE GetCraftType() {
    return m_craftType;
  }
  static int GetNumCrafts() {
    return m_filteredSkills;
  }
  static const CraftInfo *GetCraftInfo(UINT index) {
    return index < m_numSkills ? m_skills[index] : 0;
  }
  static UINT GetNumSkillLines() {
    return m_numSkillLines;
  }
  static CraftSkillLineInfo *GetSkillLine(UINT index) {
    return index < m_numSkillLines ? m_skillLines[index] : 0;
  }
  static int  GetSkillLineIndexFromCraft(UINT index);
  static void SetCraftType(SPELL_CAST_UI_TYPE type);
  static void RefreshList();
  static BOOL IsCollpasedHeader(UINT index);
  static int  GetCollapseFilter() {
    return m_collapseFilter;
  }
  static void SetCollapseFilter(int filter);

 private:
  friend int __cdecl QSortSkills(LPCVOID a, LPCVOID b);
  friend int __cdecl QSortPetSkills(LPCVOID a, LPCVOID b);
  friend int __cdecl QSortSkillLines(LPCVOID a, LPCVOID b);

 protected:
  static void FilterAndSortSkills();

 private:
  static SPELL_CAST_UI_TYPE                    m_craftType;
  static int                                   m_currentSelection;
  static UINT                                  m_numSkills;
  static UINT                                  m_numSkillLines;
  static UINT                                  m_filteredSkills;
  static int                                   m_collapseFilter;
  static TSGrowableArray<CraftInfo *>          m_skills;
  static TSGrowableArray<CraftSkillLineInfo *> m_skillLines;
};

#include "WowSvcs/WowSvcsClient/ClientServices.h"

#include <Frame/CSimpleStatusBar.h>

extern LPCSTR g_invTypeTokens[];

inline int CGItem::GetNumPetitionSignatures() const {
  return !(m_item->m_staticFlags & ITEM_FLAG_PETITION) ? 0 : m_item->m_enchantment[0].expiration;
}

inline UINT CGGameObject::GetGameObjectFlags() const {
  return m_gameObj->m_flags;
}

inline bool CGGameObject::GetLocked() const {
  return (GetGameObjectFlags() >> 1) & 1;
}

BOOL Trade_C_GetProposedEnchantment(UINT player, int &spellID, int &slot);

DWORDLONG Script_GetGUIDFromName(LPCSTR name);
CGUnit_C *Script_GetUnitFromName(LPCSTR name);
LPCSTR    GetSpellAuraEffectToken(int effectID);
BOOL      SpellParserParseText(const SpellRec *spell, char *buf, UINT size, BOOL isPet);
int       Spell_C_GetManaCostPerSecond(int id, BOOL isPet);
int       Spell_C_GetCastTime(int id, BOOL isPet);
UINT      Spell_C_GetPowerDisplayMod(POWER_TYPE type);
int       Spell_C_GetSpellCooldown(int spell, int isPet, UINT *duration, DWORD *startTime, UINT *enable);
int       Spell_C_GetItemCooldown(int itemID, UINT *duration, DWORD *startTime, UINT *enable);
void      Spell_C_GetMinMaxPoints(const SpellRec *srec, int effectIndex, int *min, int *max, UINT level, BOOL isPet);

static int CGTooltip_SetPadding(lua_State *L);
static int CGTooltip_IsOwned(lua_State *L);
static int CGTooltip_SetOwner(lua_State *L);
static int CGTooltip_ClearLines(lua_State *L);
static int CGTooltip_AddLine(lua_State *L);
static int CGTooltip_SetText(lua_State *L);
static int CGTooltip_AppendText(lua_State *L);
static int CGTooltip_FadeOut(lua_State *L);
static int CGTooltip_SetHyperlink(lua_State *L);
static int CGTooltip_SetAction(lua_State *L);
static int CGTooltip_SetPlayerBuff(lua_State *L);
static int CGTooltip_SetSpell(lua_State *L);
static int CGTooltip_SetInventoryItem(lua_State *L);
static int CGTooltip_SetLootItem(lua_State *L);
static int CGTooltip_SetQuestItem(lua_State *L);
static int CGTooltip_SetQuestLogItem(lua_State *L);
static int CGTooltip_SetTrainerService(lua_State *L);
static int CGTooltip_SetTradeSkillItem(lua_State *L);
static int CGTooltip_SetCraftItem(lua_State *L);
static int CGTooltip_SetCraftSpell(lua_State *L);
static int CGTooltip_SetMerchantItem(lua_State *L);
static int CGTooltip_SetTradePlayerItem(lua_State *L);
static int CGTooltip_SetTradeTargetItem(lua_State *L);
static int CGTooltip_SetBagItem(lua_State *L);
static int CGTooltip_SetUnit(lua_State *L);
static int CGTooltip_NumLines(lua_State *L);

static FrameScript_Method CGTooltipMethods[26] = {
    {        "SetPadding",         CGTooltip_SetPadding},
    {           "IsOwned",            CGTooltip_IsOwned},
    {          "SetOwner",           CGTooltip_SetOwner},
    {        "ClearLines",         CGTooltip_ClearLines},
    {           "AddLine",            CGTooltip_AddLine},
    {           "SetText",            CGTooltip_SetText},
    {        "AppendText",         CGTooltip_AppendText},
    {           "FadeOut",            CGTooltip_FadeOut},
    {      "SetHyperlink",       CGTooltip_SetHyperlink},
    {         "SetAction",          CGTooltip_SetAction},
    {     "SetPlayerBuff",      CGTooltip_SetPlayerBuff},
    {          "SetSpell",           CGTooltip_SetSpell},
    {  "SetInventoryItem",   CGTooltip_SetInventoryItem},
    {       "SetLootItem",        CGTooltip_SetLootItem},
    {      "SetQuestItem",       CGTooltip_SetQuestItem},
    {   "SetQuestLogItem",    CGTooltip_SetQuestLogItem},
    { "SetTrainerService",  CGTooltip_SetTrainerService},
    { "SetTradeSkillItem",  CGTooltip_SetTradeSkillItem},
    {      "SetCraftItem",       CGTooltip_SetCraftItem},
    {     "SetCraftSpell",      CGTooltip_SetCraftSpell},
    {   "SetMerchantItem",    CGTooltip_SetMerchantItem},
    {"SetTradePlayerItem", CGTooltip_SetTradePlayerItem},
    {"SetTradeTargetItem", CGTooltip_SetTradeTargetItem},
    {        "SetBagItem",         CGTooltip_SetBagItem},
    {           "SetUnit",            CGTooltip_SetUnit},
    {          "NumLines",           CGTooltip_NumLines}
};

static const float LINE_SPACING = 0.002f;
static const float COLUMN_SPACING = 0.03f;
static const float XOFFSET = 0.008f;
static const float YOFFSET = 0.008f;
static const float STATUS_BAR_HEIGHT = 0.006f;
static const float STATUS_BAR_OFFSETY = 0.001f;
static const float TOOLTIP_FADE_TIME = 2.0f;
static const float WORDWRAP_MIN_WIDTH = 0.18f;

UINT                             CGTooltip::m_spellID;
static int                       s_nameOnly;
static int                       s_showComparison;
static int                       s_itemsWaiting;
static NTempest::CImVector       s_defaultColor(0xFFFFD200UL);
static NTempest::CImVector       s_normalColor(0xFFFFFFFFUL);
static NTempest::CImVector       s_errorColor(0xFFFF2020UL);
static NTempest::CImVector       s_inactiveColor(0xFF808080);
static NTempest::CImVector       s_friendlyColor(0xFF00FF00);
static NTempest::CImVector       s_neutralColor(0xFFFFFF00);
static NTempest::CImVector       s_hostileColor(0xFFFF0000);
static NTempest::CImVector       s_playerColor(0xFF002E5AUL);
static const NTempest::CImVector s_qualityColors[7] = {
    NTempest::CImVector(0xFF9D9D9D), NTempest::CImVector(0xFFFFC600), NTempest::CImVector(0xFF1EFF00), NTempest::CImVector(0xFF0070DD),
    NTempest::CImVector(0xFFA335EE), NTempest::CImVector(0xFFFF0000), NTempest::CImVector(0xFFF1E38A)
};
static LPCSTR s_qualityColorStrings[7] = {"|cff9d9d9d", "|cffffc600", "|cff1eff00", "|cff0070dd", "|cffa335ee", "|cffff0000", "|cfff1e38a"};
static LPCSTR s_spellEffectTokens[87] = {
    "",
    "SPELL_EFFECT_INSTAKILL",
    "SPELL_EFFECT_SCHOOL_DAMAGE",
    "SPELL_EFFECT_DUMMY",
    "SPELL_EFFECT_PORTAL_TELEPORT",
    "SPELL_EFFECT_TELEPORT_UNITS",
    "",
    "",
    "SPELL_EFFECT_MANA_DRAIN",
    "SPELL_EFFECT_HEALTH_LEECH",
    "SPELL_EFFECT_HEAL",
    "SPELL_EFFECT_BIND",
    "SPELL_EFFECT_PORTAL",
    "SPELL_EFFECT_RITUAL_BASE",
    "SPELL_EFFECT_RITUAL_SPECIALIZE",
    "SPELL_EFFECT_RITUAL_ACTIVATE_PORTAL",
    "",
    "SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL",
    "SPELL_EFFECT_RESURRECT",
    "SPELL_EFFECT_ADD_EXTRA_ATTACKS",
    "SPELL_EFFECT_DODGE",
    "SPELL_EFFECT_EVADE",
    "SPELL_EFFECT_PARRY",
    "SPELL_EFFECT_BLOCK",
    "SPELL_EFFECT_CREATE_ITEM",
    "",
    "",
    "",
    "SPELL_EFFECT_SUMMON",
    "SPELL_EFFECT_LEAP",
    "SPELL_EFFECT_ENERGIZE",
    "SPELL_EFFECT_WEAPON_PERCENT_DAMAGE",
    "SPELL_EFFECT_TRIGGER_MISSILE",
    "SPELL_EFFECT_OPEN_LOCK",
    "SPELL_EFFECT_SUMMON_MOUNT",
    "",
    "",
    "",
    "SPELL_EFFECT_DISPEL",
    "SPELL_EFFECT_LANGUAGE",
    "",
    "SPELL_EFFECT_SUMMON_WILD",
    "SPELL_EFFECT_SUMMON_GUARDIAN",
    "",
    "",
    "",
    "",
    "SPELL_EFFECT_TRADE_SKILL",
    "SPELL_EFFECT_STEALTH",
    "SPELL_EFFECT_DETECT",
    "SPELL_EFFECT_SUMMON_OBJECT",
    "SPELL_EFFECT_FORCE_CRITICAL_HIT",
    "",
    "SPELL_EFFECT_ENCHANT_ITEM",
    "SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY",
    "SPELL_EFFECT_TAMECREATURE",
    "SPELL_EFFECT_SUMMON_PET",
    "",
    "SPELL_EFFECT_WEAPON_DAMAGE",
    "",
    "",
    "",
    "",
    "SPELL_EFFECT_THREAT"
};
static LPCSTR s_itemEnchantTokens[5] = {"", "ITEM_ENCHANTMENT_PROC", "ITEM_ENCHANTMENT_DAMAGE", "ITEM_ENCHANTMENT_BUFF_EQUIPPED", "ITEM_ENCHANTMENT_ADD_ARMOR"};
static LPCSTR s_summonTypeTokens[5] = {"UNITNAME_TITLE_PET", "UNITNAME_TITLE_MINION", "UNITNAME_TITLE_CHARM", "UNITNAME_TITLE_GUARDIAN", "UNITNAME_TITLE_CREATION"};
static LPCSTR s_manaCostTemplates[4] = {"MANA_COST", "RAGE_COST", "FOCUS_COST", "ENERGY_COST"};
static LPCSTR s_allPowerTemplates[4] = {"SPELL_USE_ALL_MANA", "SPELL_USE_ALL_RAGE", "SPELL_USE_ALL_FOCUS", "SPELL_USE_ALL_ENERGY"};
static LPCSTR s_powerTokens[4] = {"MANA", "RAGE", "FOCUS", "ENERGY"};

static void FrameScriptGetSpellString(TOOLTIP_DETAIL detail, LPCSTR stringLabel, int points, char *positive, UINT positiveSize, char *negative, UINT negativeSize);
static void FrameScriptGetEnchantString(TOOLTIP_DETAIL detail, LPCSTR stringLabel, char *buf, UINT bufSize);
static const SpellEffectNamesRec *GetEffectNameRec(int enumID);
static const SpellAuraNamesRec *GetAuraNameRec(int enumID);
static BOOL HealthUpdateHandler(DWORDLONG guid, UINT, UINT, LPCVOID, LPVOID param);
static void TooltipObjectLockItemStatsCallback(int id, const DWORDLONG &, LPVOID arg, bool granted);
static void TooltipItemStatsCallback(int id, const DWORDLONG &, LPVOID arg, bool granted);
static void TooltipItemPetitionCallback(int id, const DWORDLONG &, LPVOID arg, bool granted);
static void TooltipSpellItemStatsCallback(int id, const DWORDLONG &guid, LPVOID arg, bool granted);
static void TooltipSpellCreatureStatsCallback(int id, const DWORDLONG &guid, LPVOID arg, bool granted);
static void TooltipSpellGameObjectStatsCallback(int id, const DWORDLONG &guid, LPVOID arg, bool granted);
static void TooltipItemCreatorCallback(int id, const DWORDLONG &, LPVOID arg, bool granted);
static void TooltipCorpseNameCallback(int id, const DWORDLONG &, LPVOID arg, bool granted);

CGTooltip::CGTooltip(CSimpleFrame *parent) : CSimpleFrame(parent) {
  m_owner = 0;
  m_lines = 0;
  m_linesMax = 0;
  m_reposition = 0;
  m_itemID = 0;
  m_itemGUID = 0;
  m_unit = 0;
  m_debugUnit = 0;
  m_fading = 0;
  m_padding = 0.0f;
}

CGTooltip::~CGTooltip() {
  SetOwner(0, TOOLTIP_ANCHOR_LEFT, 0.0f);
}

void CGTooltip::PostLoadXML(const XMLNode *node, CStatus *status) {
  char               buf[64];
  char               nameR[32];
  char               nameL[32];
  int                pass;
  CSimpleFontString *textR;

  CSimpleFrame::PostLoadXML(node, status);

  for (pass = 0; pass < 2; ++pass) {
    if (pass == 1) {
      m_leftStrings.SetCount(m_linesMax);
      m_rightStrings.SetCount(m_linesMax);
      m_wrapLine.SetCount(m_linesMax);
    }

    m_linesMax = 0;
    SStrPrintf(nameL, sizeof(nameL), "%sTextLeft%d", m_frameName, m_linesMax + 1);
    SStrPrintf(nameR, sizeof(nameR), "%sTextRight%d", m_frameName, m_linesMax + 1);

    CSimpleFontString *textL = SimpleFontStringRegistryGetEntry(nameL, 0);
    textR = SimpleFontStringRegistryGetEntry(nameR, 0);
    while (textL && textR) {
      if (pass == 1) {
        m_leftStrings[m_linesMax] = textL;
        m_rightStrings[m_linesMax] = textR;
        m_wrapLine[m_linesMax] = 0;
      }

      ++m_linesMax;
      SStrPrintf(nameL, sizeof(nameL), "%sTextLeft%d", m_frameName, m_linesMax + 1);
      SStrPrintf(nameR, sizeof(nameR), "%sTextRight%d", m_frameName, m_linesMax + 1);
      textL = SimpleFontStringRegistryGetEntry(nameL, 0);
      textR = SimpleFontStringRegistryGetEntry(nameR, 0);
    }
  }

  SStrPrintf(buf, sizeof(buf), "%sStatusBar", m_frameName);
  m_statusBar = (CSimpleStatusBar *)SimpleFrameRegistryGetEntry(buf, 0);
}

BOOL CGTooltip::SetUnit(const DWORDLONG &unit) {
  if (unit == m_unit) {
    return 0;
  }

  if (m_unit && m_statusBar) {
    ClntObjMgrUnsetObjMirrorHandler(m_unit, CGUnit_C::OffsetOf(ID_UNIT) + offsetof(CGUnitData, health), HealthUpdateHandler, m_statusBar);
  }

  m_unit = unit;
  CGUnit_C *unitPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(m_unit, __FILE__, __LINE__));
  CGPartyInfo::RemoteStats *stats = CGPartyInfo::GetRemoteStats(unit);
  if (!unitPtr && !stats) {
    if (m_statusBar) {
      m_statusBar->Hide();
    }
    return 0;
  }

  const NameCache *nc = g_nameDBCache.GetRecord(unit, 0, 0, 0);
  if (m_statusBar) {
    ClntObjMgrSetObjMirrorHandler(m_unit, CGUnit_C::OffsetOf(ID_UNIT) + offsetof(CGUnitData, health), sizeof(((CGUnitData *)0)->health), HealthUpdateHandler, m_statusBar, HANDLER_PRIORITY_NORMAL);
    m_statusBar->SetMinMaxValues(0.0f, unitPtr ? (float)unitPtr->GetMaxHealth() : (float)stats->maxHealth);
    m_statusBar->SetValue(unitPtr ? (float)unitPtr->GetHealth() : (float)stats->health);
    m_statusBar->Show();
  }

  ClearLines();
  char buf[128];
  CVar *var = CVar::Lookup("showGUIDs");
  if (var && var->GetInt()) {
    SStrPrintf(buf, sizeof(buf), "0x%016I64X", m_unit);
    AddLine(buf, s_defaultColor, 0);
  }
  if (unitPtr) {
    AddLine(unitPtr->GetUnitName(), s_defaultColor, 0);
  } else if (nc) {
    AddLine(nc->m_name, s_defaultColor, 0);
  }

  buf[0] = 0;
  GetSummonedByString(unitPtr, buf, sizeof(buf));
  if (buf[0]) {
    AddLine(buf, s_normalColor, 0);
  }

  if ((unitPtr && unitPtr->IsA(TYPE_PLAYER)) || nc) {
    const ChrRacesRec *race = g_chrRacesDB.GetRecord(unitPtr ? unitPtr->GetRace() : nc->m_race);
    const ChrClassesRec *classRec = g_chrClassesDB.GetRecord(unitPtr ? unitPtr->GetClass() : stats ? stats->classID : 1);
    if (race && classRec) {
      SStrPrintf(buf, sizeof(buf), "%s %s", race->m_name_lang[CURRENT_LANGUAGE], classRec->m_name_lang[CURRENT_LANGUAGE]);
      AddLine(buf, s_normalColor, 0);
    }
  } else if (unitPtr) {
    LPCSTR title = unitPtr->GetUnitTitle();
    if (title && *title) {
      AddLine(title, s_normalColor, 0);
    }
  }

  BOOL dead = unitPtr ? unitPtr->GetHealth() <= 0 || unitPtr->IsFeignDeath() : stats->health <= 0;
  if (unitPtr) {
    if (dead) {
      SStrPrintf(buf, sizeof(buf), FrameScript_GetText("UNIT_LEVEL_DEAD_TEMPLATE", -1, GENDER_NOT_APPLICABLE), unitPtr->GetLevel());
      AddLine(buf, s_normalColor, 0);
    } else {
      CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
      if (player && unitPtr->UnitReaction(player) <= UNIT_REACTION_HOSTILE && player->GetLevel() <= unitPtr->GetLevel() - 15) {
        LPCSTR text = FrameScript_GetText("UNIT_LETHAL_LEVEL_TEMPLATE", -1, GENDER_NOT_APPLICABLE);
        SStrCopy(buf, text, sizeof(buf));
      } else {
        if (unitPtr->IsPlusMob()) {
          SStrPrintf(buf, sizeof(buf), FrameScript_GetText("UNIT_PLUS_LEVEL_TEMPLATE", -1, GENDER_NOT_APPLICABLE), unitPtr->GetLevel());
        } else {
          SStrPrintf(buf, sizeof(buf), FrameScript_GetText("UNIT_LEVEL_TEMPLATE", -1, GENDER_NOT_APPLICABLE), unitPtr->GetLevel());
        }
      }
      AddLine(buf, s_normalColor, 0);
    }
  } else {
    SStrPrintf(buf, sizeof(buf), FrameScript_GetText("UNIT_LEVEL_TEMPLATE", -1, GENDER_NOT_APPLICABLE), stats->level);
    AddLine(buf, s_normalColor, 0);
  }

  if (CGPartyInfo::IsMember(unit) && unit != ClntObjMgrGetActivePlayer()) {
    if (unitPtr) {
      AddLine(CGGameUI::GetZoneText(), s_normalColor, 0);
    } else if (stats && AreaListGetName(stats->mapID, HIWORD(stats->areaID), 0, buf, sizeof(buf), 0)) {
      AddLine(buf, s_normalColor, 0);
    }
  }

  int debugInfo = 0;
  var = CVar::Lookup("debugTargetInfo");
  if (var && var->GetInt()) {
    debugInfo = 1;
  }
  if (unitPtr && debugInfo) {
    const ChrClassesRec *classRec = g_chrClassesDB.GetRecord(unitPtr->GetClass());
    if (classRec) {
      AddLine(classRec->m_name_lang[CURRENT_LANGUAGE], (LPCSTR)0, 0);
    }
    SStrPrintf(buf, sizeof(buf), "Health: %d / %d, Power(%d): %d / %d", unitPtr->GetHealth(), unitPtr->GetMaxHealth(),
               unitPtr->GetDisplayPower(), unitPtr->GetPower(unitPtr->GetDisplayPower()), unitPtr->GetMaxPower(unitPtr->GetDisplayPower()));
    AddLine(buf, (LPCSTR)0, 0);
    CDataStore msg;
    msg.Put(CMSG_DEBUG_AISTATE);
    msg.Put(unit);
    msg.Finalize();
    ClientServices_Send(&msg);
    m_debugUnit = unit;
  }
  Show();
  return 0;
}

static BOOL HealthUpdateHandler(DWORDLONG guid, UINT, UINT, LPCVOID, LPVOID param) {
  CSimpleStatusBar *statusbar = (CSimpleStatusBar *)param;
  FATALASSERT(statusbar);
  CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (unit) {
    statusbar->SetMinMaxValues(0.0f, (float)unit->GetMaxHealth());
    statusbar->SetValue((float)unit->GetHealth());
  }
  return 1;
}

void CGTooltip::SetObject(const DWORDLONG &object) {
  CGGameObject_C *objectPtr = static_cast<CGGameObject_C *>(ClntObjMgrObjectPtr(object, __FILE__, __LINE__));
  if (!objectPtr) {
    return;
  }

  m_objectGUID = object;
  ClearLines();
  if (m_backdrop) {
    m_backdrop->SetVertexColor(s_playerColor);
  }
  char buf[128];
  char fmt[128];
  CVar *var = CVar::Lookup("showGUIDs");
  if (var && var->GetInt()) {
    SStrPrintf(buf, sizeof(buf), "0x%016I64X", object);
    AddLine(buf, s_defaultColor, 0);
  }
  AddLine(objectPtr->GetName(), s_defaultColor, 0);
  NTempest::CImVector textColor = s_normalColor;
  const LockRec *lock = objectPtr->GetLockRec();
  if (objectPtr->GetLocked()) {
    textColor = s_errorColor;
    if (lock && objectPtr->IsValidOpenAction(lock->m_Action[0]) && lock->m_Type[0] == 2) {
      int spellID = 0;
      int spellSkill = 0;
      int lockSkill = 0;
      objectPtr->IsLocked(&spellID, &spellSkill, &lockSkill, 0, 0);
      if (spellID) {
        if (spellSkill >= lockSkill + 100) {
          textColor.Set(1.0f, 0.5f, 0.5f, 0.5f);
        } else if (spellSkill >= lockSkill + 50) {
          textColor.Set(1.0f, 0.25f, 0.75f, 0.25f);
        } else if (spellSkill >= lockSkill + 25) {
          textColor.Set(1.0f, 1.0f, 1.0f, 0.0f);
        } else if (spellSkill >= lockSkill) {
          textColor.Set(1.0f, 1.0f, 0.5f, 0.25f);
        }
      }
    }
    AddLine(FrameScript_GetText("LOCKED", -1, GENDER_NOT_APPLICABLE), textColor, 0);
  } else if (lock && objectPtr->IsValidOpenAction(lock->m_Action[0])) {
    switch (lock->m_Type[0]) {
      case 1: {
        const ItemStats_C *item = g_itemDBCache.GetRecord(lock->m_Index[0], objectPtr->GetGUID(), TooltipObjectLockItemStatsCallback, this);
        if (!item) {
          return;
        }
        SStrPrintf(buf, sizeof(buf), FrameScript_GetText("LOCKED_WITH_ITEM", -1, GENDER_NOT_APPLICABLE), item->m_displayName[0]);
        AddLine(buf, textColor, 0);
        break;
      }
      case 2: {
        int spellID = 0;
        int spellSkill = 0;
        int lockSkill = 0;
        objectPtr->IsLocked(&spellID, &spellSkill, &lockSkill, 0, 0);
        if (spellID) {
          if (spellSkill >= lockSkill + 100) {
            textColor.Set(1.0f, 0.5f, 0.5f, 0.5f);
          } else if (spellSkill >= lockSkill + 50) {
            textColor.Set(1.0f, 0.25f, 0.75f, 0.25f);
          } else if (spellSkill >= lockSkill + 25) {
            textColor.Set(1.0f, 1.0f, 1.0f, 0.0f);
          } else if (spellSkill >= lockSkill) {
            textColor.Set(1.0f, 1.0f, 0.5f, 0.25f);
          } else {
            textColor = s_errorColor;
          }
          LPCSTR text = FrameScript_GetText("LOCKED_WITH_SPELL_KNOWN", -1, GENDER_NOT_APPLICABLE);
          SStrCopy(fmt, text, sizeof(fmt));
        } else {
          if (objectPtr->GetLocked()) {
            break;
          }
          textColor.Set(1.0f, 1.0f, 0.0f, 0.0f);
          LPCSTR text = FrameScript_GetText("LOCKED_WITH_SPELL", -1, GENDER_NOT_APPLICABLE);
          SStrCopy(fmt, text, sizeof(fmt));
        }
        const LockTypeRec *lockType = g_lockTypeDB.GetRecord(lock->m_Index[0]);
        SStrPrintf(buf, sizeof(buf), fmt, lockType ? lockType->m_name_lang[CURRENT_LANGUAGE] : "UNKNOWN");
        AddLine(buf, textColor, 0);
        break;
      }
    }
  }

  var = CVar::Lookup("debugTargetInfo");
  if (var && var->GetInt()) {
    AddLine(objectPtr->GetTypeName(), (LPCSTR)0, 0);
    AddLine(objectPtr->GetDebugStatus(), (LPCSTR)0, 0);
    CDataStore msg;
    msg.Put(CMSG_DEBUG_AISTATE);
    msg.Put(object);
    msg.Finalize();
    ClientServices_Send(&msg);
    m_debugUnit = object;
  }
  Show();
}

static void TooltipObjectLockItemStatsCallback(int id, const DWORDLONG &, LPVOID arg, bool granted) {
  if (granted) {
    CGTooltip *tooltip = (CGTooltip *)arg;
    FATALASSERT(tooltip);
    tooltip->SetObject(tooltip->GetObjectGUID());
  }
}

LPCSTR CGTooltip::GetItemQualityColorString(UINT quality) {
  return quality >= 7 ? "" : s_qualityColorStrings[quality];
}

void CGTooltip::SetCorpse(const DWORDLONG &corpseGUID) {
  CGCorpse_C *corpse = static_cast<CGCorpse_C *>(ClntObjMgrObjectPtr(corpseGUID, __FILE__, __LINE__));
  if (!corpse) {
    return;
  }

  ClearLines();

  CVar *showGUIDs = CVar::Lookup("showGUIDs");
  if (showGUIDs && showGUIDs->GetInt()) {
    char buf[128];
    SStrPrintf(buf, sizeof(buf), "0x%016I64X", corpseGUID);
    AddLine(buf, s_defaultColor, 0);
  }

  m_corpseGUID = corpseGUID;
  const NameCache *nc = g_nameDBCache.GetRecord(corpse->GetOwner(), corpse->GetOwner(), TooltipCorpseNameCallback, &m_corpseGUID);
  if (nc) {
    char   str[256];
    char   line[256];
    LPCSTR text = FrameScript_GetText("CORPSE_TOOLTIP", -1, GENDER_NOT_APPLICABLE);
    SStrCopy(str, text, sizeof(str));
    SStrPrintf(line, sizeof(line), str, nc->m_name);
    AddLine(line, s_normalColor, 0);
    Show();
  }
}

static void TooltipCorpseNameCallback(int id, const DWORDLONG &, LPVOID arg, bool granted) {
  if (granted && arg) {
    CGTooltip *tooltip = CGGameUI::GetGameTooltip();
    FATALASSERT(tooltip);
    tooltip->SetCorpse(*(const DWORDLONG *)arg);
  }
}

BOOL CGTooltip::SetItem(
    int                      itemID,
    const DWORDLONG         &refGUID,
    const DWORDLONG         &itemGUID,
    int                      nameOnly,
    int                      showComparison,
    TooltipExtendedItemInfo *info
) {
  if (!itemID) {
    return 0;
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return 0;
  }

  s_nameOnly = nameOnly;
  s_showComparison = showComparison;
  m_itemGUID = itemGUID;
  m_itemID = itemID;
  const ItemStats *stats = g_itemDBCache.GetRecord(itemID, refGUID, TooltipItemStatsCallback, this);
  if (!stats) {
    return 0;
  }

  ClearLines();
  int hasCooldown = 0;
  int isBag = stats->m_inventoryType == 18;

  char  buf[256];
  char  temp[256];
  char  left[256];
  char  right[256];
  int   i;
  int   usable;
  CVar *var = CVar::Lookup("showGUIDs");
  if (var && var->GetInt()) {
    SStrPrintf(buf, sizeof(buf), "0x%016I64X", refGUID);
    AddLine(buf, s_defaultColor, 0);
  }

  if (nameOnly) {
    AddLine(stats->m_displayName[0], s_normalColor, 0);
  } else if (stats->m_inventoryType) {
    AddLine(stats->m_displayName[0], s_qualityColors[stats->m_overallQualityID], 0);
  } else {
    AddLine(stats->m_displayName[0], s_defaultColor, 0);
  }

  CGItem_C *itemPtr = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(itemGUID, __FILE__, __LINE__));
  if (itemPtr && itemPtr->GetItemStaticFlag(ITEM_FLAG_PETITION)) {
    const CGPetition *petition = g_petitionCache.GetRecord(itemPtr->GetPetitionID(), itemGUID, TooltipItemPetitionCallback, this);
    if (!petition) {
      return 0;
    }
    if (petition->m_flags & 0x1) {
      SStrPrintf(buf, sizeof(buf), FrameScript_GetText("GUILD_CHARTER_TITLE", -1, GENDER_NOT_APPLICABLE), petition->m_title);
    } else {
      SStrPrintf(buf, sizeof(buf), FrameScript_GetText("PETITION_TITLE", -1, GENDER_NOT_APPLICABLE), petition->m_title);
    }
    AddLine(buf, s_normalColor, 1);

    const NameCache *nc = g_nameDBCache.GetRecord(petition->m_petitioner, petition->m_petitioner, TooltipItemCreatorCallback, &m_itemGUID);
    if (nc) {
      if (petition->m_flags & 0x1) {
        SStrPrintf(buf, sizeof(buf), FrameScript_GetText("GUILD_CHARTER_CREATOR", -1, GENDER_NOT_APPLICABLE), nc->m_name);
      } else {
        SStrPrintf(buf, sizeof(buf), FrameScript_GetText("PETITION_CREATOR", -1, GENDER_NOT_APPLICABLE), nc->m_name);
      }
      AddLine(buf, s_normalColor, 0);
    }

    int numSignatures = itemPtr->GetNumPetitionSignatures();
    if (numSignatures) {
      SStrPrintf(buf, sizeof(buf), FrameScript_GetText("PETITION_NUM_SIGNATURES", numSignatures, GENDER_NOT_APPLICABLE), numSignatures);
      AddLine(buf, s_normalColor, 0);
    }
  }

  if (stats->m_flags & ITEM_FLAG_PETITION) {
    AddLine(FrameScript_GetText("ITEM_SIGNABLE", -1, GENDER_NOT_APPLICABLE), s_friendlyColor, 0);
  }

  if (!nameOnly) {
    if (stats->m_flags & ITEM_FLAG_CONJURED) {
      LPCSTR text = FrameScript_GetText("ITEM_CONJURED", -1, GENDER_NOT_APPLICABLE);
      SStrCopy(temp, text, sizeof(temp));
      AddLine(temp, s_normalColor, 0);
    }

    buf[0] = 0;
    if (itemPtr && itemPtr->IsBound()) {
      if (stats->m_bonding != 4 && stats->m_bonding != 5) {
        LPCSTR text = FrameScript_GetText("ITEM_SOULBOUND", -1, GENDER_NOT_APPLICABLE);
        SStrCopy(buf, text, sizeof(buf));
      } else {
        LPCSTR text = FrameScript_GetText("ITEM_BIND_QUEST", -1, GENDER_NOT_APPLICABLE);
        SStrCopy(buf, text, sizeof(buf));
      }
    }
    if (stats->m_bonding && !buf[0]) {
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
      AddLine(buf, s_normalColor, 0);
    }

    if (stats->m_maxCount > 0) {
      if (stats->m_maxCount == 1) {
        LPCSTR text = FrameScript_GetText("ITEM_UNIQUE", -1, GENDER_NOT_APPLICABLE);
        SStrCopy(buf, text, sizeof(buf));
      } else {
        LPCSTR text = FrameScript_GetText("ITEM_UNIQUE_MULTIPLE", -1, GENDER_NOT_APPLICABLE);
        SStrCopy(temp, text, sizeof(temp));
        SStrPrintf(buf, sizeof(buf), temp, stats->m_maxCount);
      }
      AddLine(buf, s_normalColor, 0);
    }

    if (stats->m_startQuestID) {
      LPCSTR text = FrameScript_GetText("ITEM_STARTS_QUEST", -1, GENDER_NOT_APPLICABLE);
      SStrCopy(buf, text, sizeof(buf));
      AddLine(buf, s_normalColor, 0);
    }

    const LockRec *lock = g_lockDB.GetRecord(stats->m_lockID);
    if (lock && itemPtr && !itemPtr->IsUnlocked()) {
      NTempest::CImVector textColor = s_errorColor;
      if (lock->m_Type[0] == 2) {
        int spellID = 0;
        int spellSkill = 0;
        int lockSkill = 0;
        for (UINT spellIndex = 0; spellIndex < CGSpellBook::m_unlockSpells.Count(); ++spellIndex) {
          const SpellRec *srec = g_spellDB.GetRecord(CGSpellBook::m_unlockSpells[spellIndex]);
          FATALASSERT(srec);
          for (UINT effectIndex = 0; effectIndex < 3; ++effectIndex) {
            if (srec->m_effect[effectIndex] == 33 && srec->m_effectMiscValue[effectIndex] == lock->m_Index[0]) {
              int min;
              int max;
              Spell_C_GetMinMaxPoints(srec, effectIndex, &min, &max, 0, 0);
              spellID = CGSpellBook::m_unlockSpells[spellIndex];
              spellSkill = min;
              lockSkill = lock->m_Skill[0];
              if (spellSkill >= lockSkill) {
                break;
              }
            }
          }
        }
        if (spellID) {
          if (spellSkill >= lockSkill + 100) {
            textColor.Set(1.0f, 0.5f, 0.5f, 0.5f);
          } else if (spellSkill >= lockSkill + 50) {
            textColor.Set(1.0f, 0.25f, 0.75f, 0.25f);
          } else if (spellSkill >= lockSkill + 25) {
            textColor.Set(1.0f, 1.0f, 1.0f, 0.0f);
          } else if (spellSkill >= lockSkill) {
            textColor.Set(1.0f, 1.0f, 0.5f, 0.25f);
          }
        }
      }
      AddLine(FrameScript_GetText("LOCKED", -1, GENDER_NOT_APPLICABLE), textColor, 0);
    }
  }

  right[0] = 0;
  const ItemSubClassRec *subClass = 0;
  for (i = 0; i < g_itemSubClassDB.GetNumRecords(); ++i) {
    const ItemSubClassRec *rec = g_itemSubClassDB.GetRecordByIndex(i);
    if (rec && rec->m_classID == stats->m_class && rec->m_subClassID == stats->m_subclass) {
      subClass = rec;
      break;
    }
  }

  if (isBag) {
    if (subClass && subClass->m_displayName_lang[CURRENT_LANGUAGE] && *subClass->m_displayName_lang[CURRENT_LANGUAGE]) {
      LPCSTR text = FrameScript_GetText("CONTAINER_SLOTS", -1, GENDER_NOT_APPLICABLE);
      SStrCopy(temp, text, sizeof(temp));
      SStrPrintf(left, sizeof(left), temp, stats->m_containerSlots, subClass->m_displayName_lang[CURRENT_LANGUAGE]);
      AddLine(left, s_normalColor, 0);
    }
  } else {
    if (subClass && !(subClass->m_displayFlags & 0x1) && subClass->m_displayName_lang[CURRENT_LANGUAGE] &&
        *subClass->m_displayName_lang[CURRENT_LANGUAGE]) {
      SStrCopy(right, subClass->m_displayName_lang[CURRENT_LANGUAGE], sizeof(right));
    }

    if (stats->m_class == 6) {
      const ItemClassRec *itemClass = g_itemClassDB.GetRecord(6);
      if (itemClass && itemClass->m_className_lang[CURRENT_LANGUAGE] && *itemClass->m_className_lang[CURRENT_LANGUAGE]) {
        SStrCopy(left, itemClass->m_className_lang[CURRENT_LANGUAGE], sizeof(left));
      } else {
        left[0] = 0;
      }
    } else {
      LPCSTR text = FrameScript_GetText(g_invTypeTokens[stats->m_inventoryType], -1, GENDER_NOT_APPLICABLE);
      SStrCopy(left, text, sizeof(left));
      if (!*text) {
        left[0] = 0;
      }
    }

    int  error = 0;
    int  handError = 0;
    UINT proficiency = CGPlayer_C::GetProficiency(stats->m_class);
    if (proficiency) {
      if (!(proficiency & (1 << stats->m_subclass))) {
        if (stats->m_class == 2) {
          if (subClass->m_prerequisiteProficiency != -1) {
            if (proficiency & (1 << subClass->m_prerequisiteProficiency)) {
              handError = 1;
            } else {
              error = 1;
            }
          } else if (subClass->m_postrequisiteProficiency != -1) {
            if (proficiency & (1 << subClass->m_postrequisiteProficiency)) {
              handError = 1;
            } else {
              error = 1;
            }
          } else {
            error = 1;
          }
        } else {
          error = 1;
        }
      }
    }

    if (left[0]) {
      AddLine(left, right[0] ? right : 0, handError ? s_errorColor : s_normalColor, error ? s_errorColor : s_normalColor, 0);
    } else if (right[0]) {
      AddLine(right, error ? s_errorColor : s_normalColor, 0);
    }
  }

  if (!nameOnly) {
    if (stats->m_minDamage[0] || stats->m_maxDamage[0]) {
      if (stats->m_damageType[0]) {
        SStrPrintf(temp, sizeof(temp), "SPELL_SCHOOL%d_CAP", stats->m_damageType[0]);
        LPCSTR text = FrameScript_GetText(temp, -1, GENDER_NOT_APPLICABLE);
        SStrCopy(buf, text, sizeof(buf));
        text = FrameScript_GetText("DAMAGE_TEMPLATE_WITH_SCHOOL", -1, GENDER_NOT_APPLICABLE);
        SStrCopy(temp, text, sizeof(temp));
        SStrPrintf(left, sizeof(left), temp, stats->m_minDamage[0], stats->m_maxDamage[0], buf);
      } else {
        LPCSTR text = FrameScript_GetText("DAMAGE_TEMPLATE", -1, GENDER_NOT_APPLICABLE);
        SStrCopy(temp, text, sizeof(temp));
        SStrPrintf(left, sizeof(left), temp, stats->m_minDamage[0], stats->m_maxDamage[0]);
      }
      if (stats->m_class == 2) {
        LPCSTR text = FrameScript_GetText("SPEED", -1, GENDER_NOT_APPLICABLE);
        SStrCopy(temp, text, sizeof(temp));
        SStrPrintf(right, sizeof(right), "%s %.2f", temp, stats->m_delay * 0.001f);
      } else {
        right[0] = 0;
      }
      AddLine(left, right, s_normalColor, s_normalColor, 0);
    }

    if (stats->m_resistances[0] > 0) {
      LPCSTR text = FrameScript_GetText("ARMOR_TEMPLATE", -1, GENDER_NOT_APPLICABLE);
      SStrCopy(temp, text, sizeof(temp));
      SStrPrintf(buf, sizeof(buf), temp, stats->m_resistances[0]);
      AddLine(buf, s_normalColor, 0);
    }

    usable = 1;
    for (i = 2; i < 6; ++i) {
      if (stats->m_resistances[i] != stats->m_resistances[1]) {
        usable = 0;
        break;
      }
    }
    if (usable) {
      if (stats->m_resistances[1]) {
        LPCSTR text = FrameScript_GetText("ITEM_RESIST_ALL", -1, GENDER_NOT_APPLICABLE);
        SStrCopy(temp, text, sizeof(temp));
        SStrPrintf(buf, sizeof(buf), temp, stats->m_resistances[1] > 0 ? '+' : '-', abs(stats->m_resistances[1]));
        AddLine(buf, s_normalColor, 0);
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
          AddLine(buf, s_normalColor, 0);
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
          AddLine(buf, s_normalColor, 0);
        }
      }
    }

    if (itemPtr || info) {
      for (i = 0; i < NUM_ITEM_ENCHANTMENTS; ++i) {
        int enchantment = itemPtr ? itemPtr->GetEnchantmentID(i) : info->enchantment[i];
        if (enchantment) {
          const SpellItemEnchantmentRec *rec = g_spellItemEnchantmentDB.GetRecord(abs(enchantment));
          if (rec) {
            int timeLeft;
            if (itemPtr && (timeLeft = itemPtr->GetEnchantmentTimeLeft(i)) != 0) {
              int    minutes = timeLeft >= 60000;
              LPCSTR text = FrameScript_GetText(minutes ? "ITEM_ENCHANT_TIME_LEFT_MINUTES" : "ITEM_ENCHANT_TIME_LEFT_SECONDS", -1, GENDER_NOT_APPLICABLE);
              SStrCopy(temp, text, sizeof(temp));
              SStrPrintf(buf, sizeof(buf), temp, rec->m_name_lang[CURRENT_LANGUAGE], minutes ? (timeLeft - 1) / 60000 + 1 : timeLeft / 1000);
              AddLine(buf, enchantment > 0 ? s_friendlyColor : s_hostileColor, 0);
              hasCooldown = 1;
            } else {
              AddLine(rec->m_name_lang[CURRENT_LANGUAGE], enchantment > 0 ? s_friendlyColor : s_hostileColor, 0);
            }
          }
        }
      }

      if (info && info->proposedEnchantment) {
        const SpellRec *srec = g_spellDB.GetRecord(info->proposedEnchantment);
        if (srec) {
          LPCSTR text = FrameScript_GetText("ITEM_PROPOSED_ENCHANT", -1, GENDER_NOT_APPLICABLE);
          SStrCopy(temp, text, sizeof(temp));
          SStrPrintf(buf, sizeof(buf), temp, srec->m_name_lang[CURRENT_LANGUAGE]);
          AddLine(buf, s_friendlyColor, 0);
          AddLine(FrameScript_GetText("ITEM_ENCHANT_DISCLAIMER", -1, GENDER_NOT_APPLICABLE), s_hostileColor, 0);
        }
      }
    }
  }

  if (itemPtr && itemPtr->GetExpirationTimeLeft()) {
    int    timeLeft = itemPtr->GetExpirationTimeLeft();
    int    minutes = timeLeft >= 60000;
    LPCSTR text = FrameScript_GetText(minutes ? "ITEM_DURATION_MINUTES" : "ITEM_DURATION_SECONDS", -1, GENDER_NOT_APPLICABLE);
    SStrCopy(temp, text, sizeof(temp));
    SStrPrintf(buf, sizeof(buf), temp, minutes ? (timeLeft - 1) / 60000 + 1 : timeLeft / 1000);
    AddLine(buf, s_normalColor, 0);
    hasCooldown = 1;
  }

  buf[0] = 0;
  usable = 1;
  if (!(stats->m_allowableRace & (stats->m_allowableRace - 1)) && !(stats->m_allowableClass & (stats->m_allowableClass - 1))) {
    for (i = 0; i < g_chrRacesDB.GetNumRecords(); ++i) {
      const ChrRacesRec *race = g_chrRacesDB.GetRecordByIndex(i);
      if (race && !(race->m_flags & 0x1) && (stats->m_allowableRace & (1 << (race->m_ID - 1)))) {
        SStrPrintf(buf, sizeof(buf), "%s ", race->m_name_lang[CURRENT_LANGUAGE]);
        if (player->GetRace() != race->m_ID) {
          usable = 0;
        }
        break;
      }
    }
    for (i = 0; i < g_chrClassesDB.GetNumRecords(); ++i) {
      const ChrClassesRec *classRec = g_chrClassesDB.GetRecordByIndex(i);
      if (classRec && (stats->m_allowableClass & (1 << (classRec->m_ID - 1)))) {
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
      AddLine(string, usable ? s_normalColor : s_errorColor, 0);
    }
  } else {
    int allRaces = 1;
    int allClasses = 1;
    for (i = 0; i < g_chrRacesDB.GetNumRecords(); ++i) {
      const ChrRacesRec *race = g_chrRacesDB.GetRecordByIndex(i);
      if (race && !(race->m_flags & 0x1) && !(stats->m_allowableRace & (1 << (race->m_ID - 1)))) {
        allRaces = 0;
        break;
      }
    }
    for (i = 0; i < g_chrClassesDB.GetNumRecords(); ++i) {
      const ChrClassesRec *classRec = g_chrClassesDB.GetRecordByIndex(i);
      if (classRec && !(stats->m_allowableClass & (1 << (classRec->m_ID - 1)))) {
        allClasses = 0;
        break;
      }
    }

    if (!allRaces) {
      char races[512];
      int  first = 1;
      races[0] = 0;
      usable = 0;
      for (i = 0; i < g_chrRacesDB.GetNumRecords(); ++i) {
        const ChrRacesRec *race = g_chrRacesDB.GetRecordByIndex(i);
        if (race && !(race->m_flags & 0x1) && (stats->m_allowableRace & (1 << (race->m_ID - 1)))) {
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
        char   listBuf[512];
        LPCSTR text = FrameScript_GetText("ITEM_RACES_ALLOWED", -1, GENDER_NOT_APPLICABLE);
        SStrCopy(temp, text, sizeof(temp));
        SStrPrintf(listBuf, sizeof(listBuf), temp, races);
        AddLine(listBuf, usable ? s_normalColor : s_errorColor, 0);
      }
    }

    if (!allClasses) {
      char classes[512];
      int  first = 1;
      classes[0] = 0;
      usable = 0;
      for (i = 0; i < g_chrClassesDB.GetNumRecords(); ++i) {
        const ChrClassesRec *classRec = g_chrClassesDB.GetRecordByIndex(i);
        if (classRec && (stats->m_allowableClass & (1 << (classRec->m_ID - 1)))) {
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
        char   listBuf[512];
        LPCSTR text = FrameScript_GetText("ITEM_CLASSES_ALLOWED", -1, GENDER_NOT_APPLICABLE);
        SStrCopy(temp, text, sizeof(temp));
        SStrPrintf(listBuf, sizeof(listBuf), temp, classes);
        AddLine(listBuf, usable ? s_normalColor : s_errorColor, 0);
      }
    }
  }

  int itemLevel = stats->m_itemLevel;
  int requiredLevel = stats->m_requiredLevel;
  if (requiredLevel) {
    if (requiredLevel > 0) {
      LPCSTR text = FrameScript_GetText("ITEM_LEVEL_AND_MIN", -1, GENDER_NOT_APPLICABLE);
      SStrCopy(temp, text, sizeof(temp));
      SStrPrintf(buf, sizeof(buf), temp, itemLevel, requiredLevel);
      AddLine(buf, requiredLevel > player->GetLevel() ? s_errorColor : s_normalColor, 0);
    } else {
      LPCSTR text = FrameScript_GetText("ITEM_LEVEL", -1, GENDER_NOT_APPLICABLE);
      SStrCopy(temp, text, sizeof(temp));
      SStrPrintf(buf, sizeof(buf), temp, itemLevel);
      AddLine(buf, s_normalColor, 0);
    }
  }

  if (stats->m_requiredSkill > 0) {
    LPCSTR text = FrameScript_GetText(stats->m_requiredSkillRank ? "ITEM_MIN_SKILL" : "ITEM_REQ_SKILL", -1, GENDER_NOT_APPLICABLE);
    SStrCopy(temp, text, sizeof(temp));
    const SkillLineRec *skill = g_skillLineDB.GetRecord(stats->m_requiredSkill);
    if (stats->m_requiredSkillRank) {
      SStrPrintf(buf, sizeof(buf), temp, skill ? skill->m_displayName_lang[CURRENT_LANGUAGE] : "UNKNOWN", stats->m_requiredSkillRank);
    } else {
      SStrPrintf(buf, sizeof(buf), temp, skill ? skill->m_displayName_lang[CURRENT_LANGUAGE] : "UNKNOWN");
    }
    AddLine(buf, stats->m_requiredSkillRank > player->GetSkillRank(stats->m_requiredSkill) ? s_errorColor : s_normalColor, 0);
  }

  for (i = 0; i < NUM_ITEM_SPELLS; ++i) {
    if (stats->m_spellID[i] > 0) {
      const SpellRec *srec = g_spellDB.GetRecord(stats->m_spellID[i]);
      if (srec) {
        int charges = itemPtr ? itemPtr->GetSpellCharges(i) : stats->m_spellCharges[i];
        if (!stats->m_spellCharges[i]) {
          charges = -1;
        }
        SpellParserParseText(srec, temp, sizeof(temp), 0);
        if (temp[0]) {
          switch (stats->m_spellTrigger[i]) {
            case 1:
              SStrPrintf(buf, sizeof(buf), "%s %s", FrameScript_GetText("ITEM_SPELL_TRIGGER_ONEQUIP", -1, GENDER_NOT_APPLICABLE), temp);
              break;
            case 2:
              SStrPrintf(buf, sizeof(buf), "%s %s", FrameScript_GetText("ITEM_SPELL_TRIGGER_ONPROC", -1, GENDER_NOT_APPLICABLE), temp);
              break;
            default:
              SStrCopy(buf, temp, sizeof(buf));
              break;
          }
          AddLine(buf, s_friendlyColor, 1);
          if (charges != -1) {
            SStrPrintf(buf, sizeof(buf), FrameScript_GetText("ITEM_SPELL_CHARGES", abs(charges), GENDER_NOT_APPLICABLE), abs(charges));
            AddLine(buf, s_normalColor, 0);
          }
        }
      }
    }
  }

  if (info && info->cooldownTime > 0) {
    int    minutes = info->cooldownTime >= 60000;
    LPCSTR text = FrameScript_GetText(minutes ? "ITEM_COOLDOWN_TIME" : "ITEM_COOLDOWN_TIME_SEC", -1, GENDER_NOT_APPLICABLE);
    SStrCopy(temp, text, sizeof(temp));
    SStrPrintf(buf, sizeof(buf), temp, minutes ? (info->cooldownTime - 1) / 60000 + 1 : info->cooldownTime / 1000);
    AddLine(buf, s_normalColor, 0);
    hasCooldown = 1;
  }

  if (nameOnly) {
    Show();
    return hasCooldown;
  }

  if (stats->m_description && *stats->m_description) {
    SStrPrintf(buf, sizeof(buf), "\"%s\"", stats->m_description);
    AddLine(buf, s_defaultColor, 1);
  }

  if (itemPtr || info) {
    DWORDLONG creator = itemPtr ? itemPtr->GetCreator() : info->creator;
    if (creator) {
      const NameCache *nc = g_nameDBCache.GetRecord(creator, creator, TooltipItemCreatorCallback, &m_itemGUID);
      if (nc) {
        SStrPrintf(buf, sizeof(buf), FrameScript_GetText("ITEM_CREATED_BY", -1, GENDER_NOT_APPLICABLE), nc->m_name);
        AddLine(buf, s_normalColor, 0);
      }
    }
    if (itemPtr) {
      if (itemPtr->GetPageTextID(0)) {
        AddLine(FrameScript_GetText("ITEM_READABLE", -1, GENDER_NOT_APPLICABLE), s_friendlyColor, 0);
      }
      if (((stats->m_flags & ITEM_FLAG_HAS_LOOT) && (!stats->m_lockID || itemPtr->IsUnlocked())) ||
          ((stats->m_flags & ITEM_FLAG_IS_WRAPPER) && itemPtr->IsWrapped())) {
        AddLine(FrameScript_GetText("ITEM_OPENABLE", -1, GENDER_NOT_APPLICABLE), s_friendlyColor, 0);
      }
    }
  }

  if (itemGUID && CGMerchantInfo::GetMerchant()) {
    int price = stats->m_sellPrice;
    if (price) {
      if (itemPtr) {
        if (itemPtr->GetStackCount() > 1) {
          price *= itemPtr->GetStackCount();
        }
        if (stats->m_spellCharges[0] < -1) {
          price = (int)((float)itemPtr->GetSpellCharges(0) * price / stats->m_spellCharges[0] + 0.5f);
        }
      }
      FrameScript_SignalEvent(323, "%s%d", GetName(), price);
    } else {
      AddLine(FrameScript_GetText("ITEM_UNSELLABLE", -1, GENDER_NOT_APPLICABLE), s_normalColor, 0);
    }
  }

  Show();
  return hasCooldown;
}

static void TooltipItemStatsCallback(int id, const DWORDLONG &, LPVOID arg, bool granted) {
  if (granted) {
    CGTooltip *tooltip = (CGTooltip *)arg;
    FATALASSERT(tooltip);
    tooltip->SetItem(tooltip->GetItem(), 0, tooltip->GetItemGUID(), s_nameOnly, s_showComparison, 0);
  }
}

static void TooltipItemPetitionCallback(int id, const DWORDLONG &, LPVOID arg, bool granted) {
  if (granted) {
    CGTooltip *tooltip = (CGTooltip *)arg;
    FATALASSERT(tooltip);
    tooltip->SetItem(tooltip->GetItem(), 0, tooltip->GetItemGUID(), s_nameOnly, s_showComparison, 0);
  }
}

static void TooltipItemCreatorCallback(int id, const DWORDLONG &, LPVOID arg, bool granted) {
  if (granted && arg) {
    CGTooltip *tooltip = CGGameUI::GetGameTooltip();
    FATALASSERT(tooltip);
    tooltip->SetItem(id, 0, *(const DWORDLONG *)arg, s_nameOnly, s_showComparison, 0);
  }
}

BOOL CGTooltip::SetSpell(int spellID, int nameOnly, UINT cooldownTime, BOOL isPet) {
  if (!spellID) {
    return 0;
  }
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return 0;
  }
  const SpellRec *spell = g_spellDB.GetRecord(spellID);
  if (!spell) {
    return 0;
  }
  const SkillLineAbilityRec *ability = player->LookupAbility(spellID);
  const SkillLineRec        *skillLine = ability ? g_skillLineDB.GetRecord(ability->m_skillLine) : 0;

  m_spellID = spellID;
  s_itemsWaiting = 0;
  ClearLines();
  AddLine(spell->m_name_lang[CURRENT_LANGUAGE], nameOnly ? s_normalColor : s_defaultColor, 0);
  if (nameOnly && spell->m_nameSubtext_lang[CURRENT_LANGUAGE] && *spell->m_nameSubtext_lang[CURRENT_LANGUAGE]) {
    AddLine(spell->m_nameSubtext_lang[CURRENT_LANGUAGE], s_inactiveColor, 0);
  }

  char left[128];
  char right[128];
  char temp[128];
  UINT i;
  left[0] = 0;
  right[0] = 0;
  if (ability && skillLine && skillLine->m_categoryID != 4 && !nameOnly &&
      SStrCmpI(
          skillLine->m_displayName_lang[CURRENT_LANGUAGE],
          spell->m_name_lang[CURRENT_LANGUAGE],
          SStrLen(skillLine->m_displayName_lang[CURRENT_LANGUAGE])
      )) {
    LPCSTR text = FrameScript_GetText("SPELL_SKILL_LINE", -1, GENDER_NOT_APPLICABLE);
    SStrCopy(temp, text, sizeof(temp));
    SStrPrintf(left, sizeof(left), temp, skillLine->m_displayName_lang[CURRENT_LANGUAGE]);
  }
  if (!nameOnly) {
    int manaCost = Spell_C_GetManaCost(spellID, isPet);
    int manaPerSecond = Spell_C_GetManaCostPerSecond(spellID, isPet);
    manaCost /= Spell_C_GetPowerDisplayMod((POWER_TYPE)spell->m_powerType);
    manaPerSecond /= Spell_C_GetPowerDisplayMod((POWER_TYPE)spell->m_powerType);
    LPCSTR token = spell->m_powerType >= 0 && spell->m_powerType < 4 ? s_manaCostTemplates[spell->m_powerType] : "HEALTH_COST";
    if (manaCost > 0 || manaPerSecond > 0) {
      if (manaPerSecond > 0) {
        SStrPrintf(temp, sizeof(temp), "%s_PER_TIME", token);
        SStrPrintf(right, sizeof(right), FrameScript_GetText(temp, -1, GENDER_NOT_APPLICABLE), manaCost, manaPerSecond);
      } else {
        LPCSTR text = FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE);
        SStrCopy(temp, text, sizeof(temp));
        SStrPrintf(right, sizeof(right), temp, manaCost);
      }
    }
  }
  if (!left[0]) {
    SStrCopy(left, right, sizeof(left));
    right[0] = 0;
  }
  AddLine(left, right, s_normalColor, s_normalColor, 0);

  int passive = 0;
  int recastTime = max(spell->m_recoveryTime, spell->m_categoryRecoveryTime);
  if (!nameOnly) {
    if (spell->m_effect[0] == 47 || (spell->m_attributes & 0x40)) {
      passive = 1;
    } else if (spell->m_effect[0] != 78) {
      float time = (float)Spell_C_GetCastTime(spellID, isPet);
      int   minutes = time >= 60000.0f;
      if (time > 0.0f) {
        if (spell->m_attributes & 0x2) {
          LPCSTR text = FrameScript_GetText("SPELL_CAST_TIME_RANGED", -1, GENDER_NOT_APPLICABLE);
          SStrCopy(temp, text, sizeof(temp));
          SStrPrintf(left, sizeof(left), temp, time * 0.001f);
        } else {
          LPCSTR text = FrameScript_GetText(minutes ? "SPELL_CAST_TIME_MIN" : "SPELL_CAST_TIME_SEC", -1, GENDER_NOT_APPLICABLE);
          SStrCopy(temp, text, sizeof(temp));
          SStrPrintf(left, sizeof(left), temp, time / (minutes ? 60000.0f : 1000.0f));
        }
      } else if (spell->m_attributes & 0x404) {
        LPCSTR text = FrameScript_GetText("SPELL_ON_NEXT_SWING", -1, GENDER_NOT_APPLICABLE);
        SStrCopy(left, text, sizeof(left));
      } else if (spell->m_attributes & 0x2) {
        LPCSTR text = FrameScript_GetText("SPELL_ON_NEXT_RANGED", -1, GENDER_NOT_APPLICABLE);
        SStrCopy(left, text, sizeof(left));
      } else {
        LPCSTR text = FrameScript_GetText("SPELL_CAST_TIME_INSTANT", -1, GENDER_NOT_APPLICABLE);
        SStrCopy(left, text, sizeof(left));
      }

      if (recastTime > 4000) {
        minutes = recastTime >= 60000;
        LPCSTR text = FrameScript_GetText(minutes ? "SPELL_RECAST_TIME_MIN" : "SPELL_RECAST_TIME_SEC", -1, GENDER_NOT_APPLICABLE);
        SStrCopy(temp, text, sizeof(temp));
        SStrPrintf(right, sizeof(right), temp, recastTime / (minutes ? 60000 : 1000));
      } else {
        right[0] = 0;
      }
      AddLine(left, right, s_normalColor, s_normalColor, 0);

      if (!(spell->m_attributes & 0x404)) {
        const SpellRangeRec *range = g_spellRangeDB.GetRecord(max(spell->m_rangeIndex, 1));
        if (range->m_rangeMax > 0.0f) {
          char rangeString[32];
          if (range->m_rangeMin > 0.0f) {
            SStrPrintf(rangeString, sizeof(rangeString), "%d-%d", (int)range->m_rangeMin, (int)range->m_rangeMax);
          } else {
            SStrPrintf(rangeString, sizeof(rangeString), "%d", (int)range->m_rangeMax);
          }
          LPCSTR text = FrameScript_GetText("SPELL_RANGE", -1, GENDER_NOT_APPLICABLE);
          SStrCopy(temp, text, sizeof(temp));
          SStrPrintf(left, sizeof(left), temp, rangeString);
          AddLine(left, s_normalColor, 0);
        }
      }
    }

    char itemBuf[512];
    int  first = 1;
    for (i = 0; i < 8; ++i) {
      if (spell->m_reagent[i] > 0) {
        const ItemStats_C *item = g_itemDBCache.GetRecord(spell->m_reagent[i], spell->m_ID | 0xB000000000000000ui64, TooltipSpellItemStatsCallback, &m_spellID);
        if (!item) {
          ++s_itemsWaiting;
          continue;
        }
        if (first) {
          first = 0;
          LPCSTR text = FrameScript_GetText("SPELL_REAGENTS", -1, GENDER_NOT_APPLICABLE);
          SStrCopy(itemBuf, text, sizeof(itemBuf));
        } else {
          SStrPack(itemBuf, ", ", sizeof(itemBuf));
        }
        if (spell->m_reagentCount[i] > 1) {
          SStrPrintf(temp, sizeof(temp), "%s (%d)", item->m_displayName[0], spell->m_reagentCount[i]);
        } else {
          SStrCopy(temp, item->m_displayName[0], sizeof(temp));
        }
        SStrPack(itemBuf, temp, sizeof(itemBuf));
      }
    }
    if (!first) {
      AddLine(itemBuf, s_normalColor, 0);
    }

    first = 1;
    for (i = 0; i < 2; ++i) {
      if (spell->m_totem[i] > 0) {
        const ItemStats_C *item = g_itemDBCache.GetRecord(spell->m_totem[i], spell->m_ID | 0xB000000000000000ui64, TooltipSpellItemStatsCallback, &m_spellID);
        if (!item) {
          ++s_itemsWaiting;
          continue;
        }
        if (first) {
          first = 0;
          LPCSTR text = FrameScript_GetText("SPELL_TOTEMS", -1, GENDER_NOT_APPLICABLE);
          SStrCopy(itemBuf, text, sizeof(itemBuf));
        } else {
          SStrPack(itemBuf, ", ", sizeof(itemBuf));
        }
        SStrPack(itemBuf, item->m_displayName[0], sizeof(itemBuf));
      }
    }
    if (!first) {
      AddLine(itemBuf, s_normalColor, 0);
    }
  }

  if (!(spell->m_targets & 0x10) && spell->m_equippedItemClass >= 0 && spell->m_equippedItemSubclass) {
    char itemTypes[512];
    itemTypes[0] = 0;
    int first = 1;
    int usable = 0;
    for (int i = 0; i < g_itemSubClassDB.GetNumRecords(); ++i) {
      const ItemSubClassRec *rec = g_itemSubClassDB.GetRecordByIndex(i);
      if (rec && rec->m_classID == spell->m_equippedItemClass && (spell->m_equippedItemSubclass & (1 << rec->m_subClassID))) {
        if (!usable && player->HasEquipped(rec->m_classID, rec->m_subClassID)) {
          usable = 1;
        }
        if (!first) {
          SStrPack(itemTypes, ", ", sizeof(itemTypes));
        } else {
          first = 0;
        }
        SStrPack(
            itemTypes,
            rec->m_verboseName_lang[CURRENT_LANGUAGE] && *rec->m_verboseName_lang[CURRENT_LANGUAGE] ? rec->m_verboseName_lang[CURRENT_LANGUAGE]
                                                                                                     : rec->m_displayName_lang[CURRENT_LANGUAGE],
            sizeof(itemTypes)
        );
      }
    }
    if (itemTypes[0] && (!usable || !nameOnly)) {
      char   listBuf[512];
      LPCSTR text = FrameScript_GetText(nameOnly ? "SPELL_EQUIPPED_ITEM_NOSPACE" : "SPELL_EQUIPPED_ITEM", -1, GENDER_NOT_APPLICABLE);
      SStrCopy(temp, text, sizeof(temp));
      SStrPrintf(listBuf, sizeof(listBuf), temp, itemTypes);
      AddLine(listBuf, usable ? s_normalColor : s_errorColor, 1);
    }
  }

  if (spell->m_shapeshiftMask) {
    char shapes[512];
    UINT numEntries = g_spellShapeshiftFormDB.GetNumRecords();
    int  first = 1;
    for (i = 0; i < numEntries; ++i) {
      if (spell->m_shapeshiftMask & (1 << i)) {
        const SpellShapeshiftFormRec *rec = g_spellShapeshiftFormDB.GetRecordByIndex(i);
        if (rec && rec->m_name_lang[CURRENT_LANGUAGE] && *rec->m_name_lang[CURRENT_LANGUAGE]) {
          if (first) {
            SStrCopy(shapes, rec->m_name_lang[CURRENT_LANGUAGE], sizeof(shapes));
            first = 0;
          } else {
            SStrPack(shapes, ", ", sizeof(shapes));
            SStrPack(shapes, rec->m_name_lang[CURRENT_LANGUAGE], sizeof(shapes));
          }
        }
      }
    }
    int usable = spell->m_shapeshiftMask & (1 << (player->GetShapeshiftForm() - 1));
    if (!first && (!usable || !nameOnly)) {
      LPCSTR text = FrameScript_GetText(nameOnly ? "SPELL_REQUIRED_FORM_NOSPACE" : "SPELL_REQUIRED_FORM", -1, GENDER_NOT_APPLICABLE);
      SStrCopy(temp, text, sizeof(temp));
      SStrPrintf(left, sizeof(left), temp, shapes);
      AddLine(left, usable ? s_normalColor : s_errorColor, 0);
    }
  }

  int hasCooldown = 0;
  if (cooldownTime > 0 && recastTime >= 60000) {
    int    minutes = cooldownTime >= 60000;
    LPCSTR text = FrameScript_GetText(minutes ? "ITEM_COOLDOWN_TIME" : "ITEM_COOLDOWN_TIME_SEC", -1, GENDER_NOT_APPLICABLE);
    SStrCopy(temp, text, sizeof(temp));
    SStrPrintf(left, sizeof(left), temp, minutes ? (cooldownTime - 1) / 60000 + 1 : cooldownTime / 1000);
    AddLine(left, s_normalColor, 0);
    hasCooldown = 1;
  }

  if (nameOnly) {
    Show();
    return hasCooldown;
  }

  if (passive && (spell->m_effect[0] == 20 || spell->m_effect[0] == 23 || spell->m_effect[0] == 22)) {
    float  chance;
    LPCSTR token = 0;
    switch (spell->m_effect[0]) {
      case 23:
        chance = player->GetBlockChance();
        token = "CHANCE_TO_BLOCK";
        break;
      case 22:
        chance = player->GetParryChance();
        token = "CHANCE_TO_PARRY";
        break;
      case 20:
        chance = player->GetDodgeChance();
        token = "CHANCE_TO_DODGE";
        break;
    }
    if (token && *token) {
      SStrPrintf(left, sizeof(left), FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE), chance);
      AddLine(left, s_normalColor, 0);
      hasCooldown = 1;
    }
  }

  if (spell->m_attributesEx & 0x2) {
    LPCSTR text =
        FrameScript_GetText(spell->m_powerType >= 0 && spell->m_powerType < 4 ? s_allPowerTemplates[spell->m_powerType] : "SPELL_USE_ALL_HEALTH", -1, GENDER_NOT_APPLICABLE);
    SStrCopy(temp, text, sizeof(temp));
    AddLine(temp, (LPCSTR)0, 0);
  }

  if (spell->m_description_lang[CURRENT_LANGUAGE] && *spell->m_description_lang[CURRENT_LANGUAGE]) {
    char buf[1024];
    SpellParserParseText(spell, buf, sizeof(buf), isPet);
    AddLine(buf, (LPCSTR)0, 1);
  }

  if (s_itemsWaiting) {
    ClearLines();
  }
  Show();
  return hasCooldown;
}

static void TooltipSpellItemStatsCallback(int id, const DWORDLONG &guid, LPVOID arg, bool granted) {
  if (granted && arg && *(UINT *)arg == (UINT)guid) {
    if (!s_itemsWaiting || !--s_itemsWaiting) {
      CGTooltip *tooltip = CGGameUI::GetGameTooltip();
      FATALASSERT(tooltip);
      tooltip->SetSpell(*(int *)arg, 0, 0, 0);
    }
  }
}

void CGTooltip::SetBuff(int spellID, BYTE flags) {
  if (!spellID) {
    return;
  }
  const SpellRec *spell = g_spellDB.GetRecord(spellID);
  if (!spell) {
    return;
  }

  ClearLines();
  AddLine(spell->m_name_lang[CURRENT_LANGUAGE], 0, 0);

  for (UINT effectIndex = 0; effectIndex < 3; ++effectIndex) {
    if (!spell->m_effect[effectIndex]) {
      continue;
    }

    char buf[128];
    GetSpellEffectString(buf, sizeof(buf), spell, effectIndex, 0, 0, TOOLTIP_DETAIL_GENERIC);
    if (*buf) {
      AddLine(buf, flags & (1 << (3 - effectIndex)) ? s_normalColor : s_inactiveColor, 0);
    }
  }
}

void CGTooltip::GetSpellEffectString(
    char           *buf,
    UINT            bufSize,
    const SpellRec *spell,
    UINT            effectIndex,
    UINT            level,
    BOOL            isPet,
    TOOLTIP_DETAIL  detail
) {
  if (!buf || !spell) {
    return;
  }
  *buf = 0;
  if (spell->m_effectAura[effectIndex]) {
    GetAuraEffectString(buf, bufSize, spell, effectIndex, level, isPet, detail);
    return;
  }
  if (detail == TOOLTIP_DETAIL_GENERIC) {
    return;
  }

  int tokenPoints = -1;
  int min;
  int max;
  Spell_C_GetMinMaxPoints(spell, effectIndex, &min, &max, level, isPet);
  if (spell->m_effect[effectIndex] == 19) {
    ++min;
    ++max;
  } else if (spell->m_effect[effectIndex] == 31) {
    min += 100;
    max += 100;
  }

  char points[32];
  if (min == max) {
    SStrPrintf(points, sizeof(points), "%d", min);
    tokenPoints = min;
  } else {
    SStrPrintf(points, sizeof(points), "%d-%d", min, max);
  }

  if (!*s_spellEffectTokens[spell->m_effect[effectIndex]]) {
    return;
  }

  char temp[64];
  FrameScriptGetSpellString(detail, s_spellEffectTokens[spell->m_effect[effectIndex]], tokenPoints, temp, sizeof(temp), 0, 0);

  switch (spell->m_effect[effectIndex]) {
    case 1:
    case 3:
    case 4:
    case 5:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 20:
    case 21:
    case 22:
    case 23:
    case 29:
    case 51:
    case 53:
    case 54:
      SStrCopy(buf, temp, bufSize);
      return;
    case 8:
    case 9:
    case 10:
    case 18:
    case 19:
    case 30:
    case 31:
    case 48:
    case 49:
    case 55:
      SStrPrintf(buf, bufSize, temp, points);
      return;
    case 17:
    case 58:
      if (!min && !max) {
        SStrPrintf(buf, bufSize, temp, "");
      }
      SStrPrintf(buf, bufSize, temp, points);
      return;
    case 32: {
      const SpellRec *triggerSpell = g_spellDB.GetRecord(spell->m_effectTriggerSpell[effectIndex]);
      if (!triggerSpell) {
        SStrPrintf(buf, bufSize, "Invalid trigger spell (%d)!  Tell Kevin", spell->m_effectTriggerSpell[effectIndex]);
        return;
      }
      SStrPrintf(buf, bufSize, temp, triggerSpell->m_name_lang[CURRENT_LANGUAGE]);
      return;
    }
    case 2: {
      char school[32];
      char token[32];
      SStrPrintf(token, sizeof(token), "SPELL_SCHOOL%d_NAME", spell->m_school);
      LPCSTR text = FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE);
      SStrCopy(school, text, sizeof(school));
      if ((spell->m_attributes & 0x402) && !(spell->m_attributes & 0x4)) {
        SStrPrintf(buf, bufSize, "%s%s_ADD", s_spellEffectTokens[spell->m_effect[effectIndex]], spell->m_school ? "" : "_NOSCHOOL");
        FrameScriptGetSpellString(detail, buf, tokenPoints, temp, sizeof(temp), 0, 0);
      } else if (!spell->m_school) {
        SStrPrintf(buf, bufSize, "%s_NOSCHOOL", s_spellEffectTokens[spell->m_effect[effectIndex]]);
        FrameScriptGetSpellString(detail, buf, tokenPoints, temp, sizeof(temp), 0, 0);
      }
      if (!spell->m_school) {
        SStrPrintf(buf, bufSize, temp, points);
        return;
      }
      SStrPrintf(buf, bufSize, temp, points, school);
      return;
    }
    case 24: {
      const ItemStats_C *item =
          g_itemDBCache.GetRecord(spell->m_effectItemType[effectIndex], spell->m_ID | 0xB000000000000000ui64, TooltipSpellItemStatsCallback, &m_spellID);
      if (!item) {
        ++s_itemsWaiting;
        return;
      }
      SStrPrintf(buf, bufSize, temp, item->m_displayName[0]);
      return;
    }
    case 28:
    case 34:
    case 41:
    case 42:
    case 56: {
      const CreatureStats_C *creature = 0;
      if (spell->m_effectMiscValue[effectIndex]) {
        creature = g_creatureDBCache.GetRecord(
            spell->m_effectMiscValue[effectIndex], spell->m_ID | 0xB000000000000000ui64, TooltipSpellCreatureStatsCallback, &m_spellID
        );
        if (!creature) {
          ++s_itemsWaiting;
          return;
        }
      }
      if ((min == 1 && max == 1) || spell->m_effect[effectIndex] == 28 || spell->m_effect[effectIndex] == 56 || spell->m_effect[effectIndex] == 34) {
        if (spell->m_effect[effectIndex] == 56 && !creature) {
          FrameScriptGetSpellString(detail, "SPELL_EFFECT_SUMMON_PET_TAMED", tokenPoints, temp, sizeof(temp), 0, 0);
        } else if (spell->m_effect[effectIndex] == 41) {
          FrameScriptGetSpellString(detail, "SPELL_EFFECT_SUMMON_WILD_SINGLE", tokenPoints, temp, sizeof(temp), 0, 0);
        } else if (spell->m_effect[effectIndex] == 42) {
          FrameScriptGetSpellString(detail, "SPELL_EFFECT_SUMMON_GUARDIAN_SINGLE", tokenPoints, temp, sizeof(temp), 0, 0);
        }
        if (creature) {
          SStrPrintf(buf, bufSize, temp, creature->m_name[0]);
        } else {
          SStrCopy(buf, temp, bufSize);
        }
      } else {
        FATALASSERT(creature);
        SStrPrintf(buf, bufSize, temp, points, creature->m_name[0]);
      }
      return;
    }
    case 33: {
      const LockTypeRec *lockType = g_lockTypeDB.GetRecord(spell->m_effectMiscValue[effectIndex]);
      if (!lockType) {
        SStrPrintf(buf, bufSize, "Invalid lock type (%d)!  Tell Kevin", spell->m_effectMiscValue[effectIndex]);
        return;
      }
      SStrPrintf(buf, bufSize, temp, lockType->m_verb_lang[CURRENT_LANGUAGE], lockType->m_resourceName_lang[CURRENT_LANGUAGE], points);
      return;
    }
    case 38: {
      const SpellDispelTypeRec *rec = g_spellDispelTypeDB.GetRecord(spell->m_effectMiscValue[effectIndex]);
      if (!rec) {
        SStrPrintf(buf, bufSize, "Invalid dispel type (%d)!  Tell Kevin", spell->m_effectMiscValue[effectIndex]);
        return;
      }
      SStrPrintf(buf, bufSize, temp, points, rec->m_name_lang[CURRENT_LANGUAGE]);
      return;
    }
    case 39: {
      const LanguagesRec *language = g_languagesDB.GetRecord(spell->m_effectMiscValue[effectIndex]);
      if (!language) {
        SStrPrintf(buf, bufSize, "Invalid language ID (%d)!  Tell Kevin", spell->m_effectMiscValue[effectIndex]);
        return;
      }
      SStrPrintf(buf, bufSize, temp, points, language->m_name_lang[CURRENT_LANGUAGE]);
      return;
    }
    case 47:
      SStrPrintf(buf, bufSize, temp, spell->m_name_lang[CURRENT_LANGUAGE]);
      return;
    case 50: {
      const GameObjectStats_C *object = g_gameObjectDBCache.GetRecord(
          spell->m_effectMiscValue[effectIndex], spell->m_ID | 0xB000000000000000ui64, TooltipSpellGameObjectStatsCallback, &m_spellID
      );
      if (!object) {
        ++s_itemsWaiting;
        return;
      }
      SStrPrintf(buf, bufSize, temp, object->m_name[0]);
      return;
    }
    default:
      SStrCopy(buf, "Missing tooltip info! Please inform Jeremy", bufSize);
      return;
  }
}

static void FrameScriptGetSpellString(    TOOLTIP_DETAIL detail,
    LPCSTR         stringLabel,
    int            points,
    char          *positive,
    UINT           positiveSize,
    char          *negative,
    UINT           negativeSize
) {
  char token[64];
  switch (detail) {
    case TOOLTIP_DETAIL_GENERIC:
      SStrPrintf(token, sizeof(token), "%s_GEN", stringLabel);
      break;
    case TOOLTIP_DETAIL_NORMAL:
      SStrPrintf(token, sizeof(token), "%s", stringLabel);
      break;
    case TOOLTIP_DETAIL_VERBOSE:
      SStrPrintf(token, sizeof(token), "%s_VERBOSE", stringLabel);
      break;
  }

  if (positive) {
    LPCSTR text = FrameScript_GetText(token, points, GENDER_NOT_APPLICABLE);
    SStrCopy(positive, text, positiveSize);
    if (!*text) {
      SStrCopy(positive, token, positiveSize);
    }
  }
  if (negative) {
    SStrPack(token, "_NEG", sizeof(token));
    LPCSTR text = FrameScript_GetText(token, points, GENDER_NOT_APPLICABLE);
    SStrCopy(negative, text, negativeSize);
    if (!*text) {
      *negative = 0;
    }
  }
}

static void TooltipSpellCreatureStatsCallback(int id, const DWORDLONG &guid, LPVOID arg, bool granted) {
  if (granted && arg && *(UINT *)arg == (UINT)guid) {
    if (!s_itemsWaiting || !--s_itemsWaiting) {
      CGTooltip *tooltip = CGGameUI::GetGameTooltip();
      FATALASSERT(tooltip);
      tooltip->SetSpell(*(int *)arg, 0, 0, 0);
    }
  }
}

static void TooltipSpellGameObjectStatsCallback(int id, const DWORDLONG &guid, LPVOID arg, bool granted) {
  if (granted && arg && *(UINT *)arg == (UINT)guid) {
    if (!s_itemsWaiting || !--s_itemsWaiting) {
      CGTooltip *tooltip = CGGameUI::GetGameTooltip();
      FATALASSERT(tooltip);
      tooltip->SetSpell(*(int *)arg, 0, 0, 0);
    }
  }
}

void CGTooltip::GetAuraEffectString(char *buf, UINT bufSize, const SpellRec *spell, UINT effectIndex, UINT level, BOOL isPet, TOOLTIP_DETAIL detail) {
  if (!buf || !spell) {
    return;
  }
  *buf = 0;

  LPCSTR effectToken;
  if (spell->m_effectAura[effectIndex] == 12 && (spell->m_auraInterruptFlags & 0x3)) {
    effectToken = "SPELL_AURA_MOD_SLEEP";
  } else {
    effectToken = GetSpellAuraEffectToken(spell->m_effectAura[effectIndex]);
  }

  char                     token[32];
  char                     school[32];
  int                      noSchool = 0;
  const SpellAuraNamesRec *auraName = GetAuraNameRec(spell->m_effectAura[effectIndex]);
  if (auraName && auraName->m_specialMiscValue == 3) {
    if (spell->m_effectMiscValue[effectIndex] < 0) {
      SStrCopy(token, "SPELL_SCHOOLMAGICAL", sizeof(token));
    } else {
      SStrPrintf(token, sizeof(token), "SPELL_SCHOOL%d_NAME", spell->m_effectMiscValue[effectIndex]);
    }
    if (!spell->m_effectMiscValue[effectIndex]) {
      noSchool = 1;
    }
  } else {
    SStrPrintf(token, sizeof(token), "SPELL_SCHOOL%d_NAME", spell->m_school);
    if (!spell->m_school) {
      noSchool = 1;
    }
  }
  LPCSTR text = FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE);
  SStrCopy(school, text, sizeof(school));

  int tokenPoints = -1;
  int min;
  int max;
  Spell_C_GetMinMaxPoints(spell, effectIndex, &min, &max, level, isPet);
  if (spell->m_effectAura[effectIndex] == 33) {
    min = 100 - min;
    max = 100 - max;
  } else if (spell->m_effectAura[effectIndex] == 9) {
    min = -min;
    max = -max;
  }
  if (min == max) {
    tokenPoints = min;
  }

  char temp[64];
  char tempNeg[64];
  FrameScriptGetSpellString(detail, effectToken, tokenPoints, temp, sizeof(temp), tempNeg, sizeof(tempNeg));

  char noSchoolToken[64];
  char tempNoSchool[64];
  char tempNegNoSchool[64];
  SStrPrintf(noSchoolToken, sizeof(noSchoolToken), "%s_NOSCHOOL", effectToken);
  FrameScriptGetSpellString(detail, noSchoolToken, tokenPoints, tempNoSchool, sizeof(tempNoSchool), tempNegNoSchool, sizeof(tempNegNoSchool));

  int neg = 0;
  if (max < 0 && tempNeg[0]) {
    SStrCopy(temp, tempNeg, sizeof(temp));
    SStrCopy(tempNoSchool, tempNegNoSchool, sizeof(tempNoSchool));
    min = -min;
    max = -max;
    neg = 1;
  }

  char points[32];
  if (min == max) {
    SStrPrintf(points, sizeof(points), "%d", min);
  } else {
    SStrPrintf(points, sizeof(points), "%d-%d", min, max);
  }

  int period = (int)(spell->m_effectAuraPeriod[effectIndex] * 0.001f);

  switch (spell->m_effectAura[effectIndex]) {
    case 5:
    case 7:
    case 9:
    case 11:
    case 12:
    case 25:
    case 26:
    case 27:
    case 60:
    case 61:
    case 62:
    case 63:
    case 64:
    case 65:
    case 66:
    case 67:
    case 68:
    case 76:
    case 78:
    case 82:
    case 84:
      SStrCopy(buf, temp, bufSize);
      return;
    case 20:
    case 21:
      SStrCopy(buf, "ERROR: Bad spell effect!  Tell Kevin", bufSize);
      return;
    case 2:
    case 6:
    case 10:
    case 16:
    case 17:
    case 18:
    case 19:
    case 28:
    case 31:
    case 32:
    case 33:
    case 34:
    case 35:
    case 46:
    case 47:
    case 48:
    case 49:
    case 50:
    case 51:
    case 52:
    case 54:
    case 55:
    case 57:
    case 58:
      if (detail != TOOLTIP_DETAIL_GENERIC) {
        SStrPrintf(buf, bufSize, temp, points);
      } else {
        SStrPrintf(buf, bufSize, temp);
      }
      return;
    case 13:
    case 14:
    case 15:
    case 69:
    case 79:
    case 81:
    case 87:
      if (detail != TOOLTIP_DETAIL_GENERIC) {
        if (noSchool) {
          SStrPrintf(buf, bufSize, tempNoSchool, points);
        } else {
          SStrPrintf(buf, bufSize, temp, points, school);
        }
      } else {
        if (noSchool) {
          SStrPrintf(buf, bufSize, tempNoSchool);
        } else {
          SStrPrintf(buf, bufSize, temp, school);
        }
      }
      return;
    case 8:
    case 24:
    case 53:
      if (detail != TOOLTIP_DETAIL_GENERIC) {
        SStrPrintf(buf, bufSize, temp, points, period);
      } else {
        if (spell->m_effectAura[effectIndex] == 24 && spell->m_powerType == 1) {
          text = FrameScript_GetText("SPELL_AURA_PERIODIC_RAGE_GEN", -1, GENDER_NOT_APPLICABLE);
          SStrCopy(temp, text, sizeof(temp));
        }
        SStrPrintf(buf, bufSize, temp, period);
      }
      return;
    case 85: {
      char newTemp[64];
      SStrPrintf(newTemp, sizeof(newTemp), "%s_%s", temp, s_powerTokens[spell->m_powerType]);
      text = FrameScript_GetText(newTemp, -1, GENDER_NOT_APPLICABLE);
      SStrCopy(temp, text, sizeof(temp));
      SStrPrintf(buf, bufSize, temp);
      return;
    }
    case 39:
    case 40:
      SStrPrintf(buf, bufSize, temp, school);
      return;
    case 3:
      if (detail != TOOLTIP_DETAIL_GENERIC) {
        if (noSchool) {
          SStrPrintf(buf, bufSize, tempNoSchool, points, period);
        } else {
          SStrPrintf(buf, bufSize, temp, points, school, period);
        }
      } else {
        if (noSchool) {
          SStrPrintf(buf, bufSize, tempNoSchool, period);
        } else {
          SStrPrintf(buf, bufSize, temp, school, period);
        }
      }
      return;
    case 37: {
      const SpellEffectNamesRec *effectRec = GetEffectNameRec(spell->m_effectMiscValue[effectIndex]);
      if (!effectRec) {
        SStrPrintf(buf, bufSize, "Invalid spell effect (%d)!  Tell Kevin", spell->m_effectMiscValue[effectIndex]);
        return;
      }
      SStrPrintf(buf, bufSize, temp, effectRec->m_name_lang[CURRENT_LANGUAGE]);
      return;
    }
    case 38: {
      const SpellAuraNamesRec *auraName = GetAuraNameRec(spell->m_effectMiscValue[effectIndex]);
      if (!auraName) {
        SStrPrintf(buf, bufSize, "Invalid spell aura (%d)!  Tell Kevin", spell->m_effectMiscValue[effectIndex]);
        return;
      }
      SStrPrintf(buf, bufSize, temp, auraName->m_name_lang[CURRENT_LANGUAGE]);
      return;
    }
    case 41: {
      const SpellDispelTypeRec *rec = g_spellDispelTypeDB.GetRecord(spell->m_effectMiscValue[effectIndex]);
      if (!rec) {
        SStrPrintf(buf, bufSize, "Invalid dispel type (%d)!  Tell Kevin", spell->m_effectMiscValue[effectIndex]);
        return;
      }
      SStrPrintf(buf, bufSize, temp, rec->m_name_lang[CURRENT_LANGUAGE]);
      return;
    }
    case 42: {
      const SpellRec *rec = g_spellDB.GetRecord(spell->m_effectTriggerSpell[effectIndex]);
      char            procTrigger[64];
      char            procFlagToken[64];
      SStrPrintf(procFlagToken, sizeof(procFlagToken), "PROC_EVENT%d_DESC", spell->m_procFlags);
      text = FrameScript_GetText(procFlagToken, -1, GENDER_NOT_APPLICABLE);
      SStrCopy(procTrigger, text, sizeof(procTrigger));
      if (*text) {
        if (spell->m_procFlags & 0xA00) {
          SStrCopy(procFlagToken, procTrigger, sizeof(procFlagToken));
          SStrPrintf(procTrigger, sizeof(procTrigger), procFlagToken, school);
        }
        SStrPrintf(buf, bufSize, temp, rec->m_name_lang[CURRENT_LANGUAGE], procTrigger);
      } else {
        SStrPrintf(buf, bufSize, temp, rec->m_name_lang[CURRENT_LANGUAGE], "Proc flag not found in global strings. Tell Jeremy.");
      }
      return;
    }
    case 43: {
      char procTrigger[64];
      char procFlagToken[32];
      SStrPrintf(procFlagToken, sizeof(procFlagToken), "PROC_EVENT%d_DESC", spell->m_procFlags);
      text = FrameScript_GetText(procFlagToken, -1, GENDER_NOT_APPLICABLE);
      SStrCopy(procTrigger, text, sizeof(procTrigger));
      if (*text) {
        if (detail != TOOLTIP_DETAIL_GENERIC) {
          if (noSchool) {
            SStrPrintf(buf, bufSize, tempNoSchool, points, procTrigger);
          } else {
            SStrPrintf(buf, bufSize, temp, points, school, procTrigger);
          }
        } else {
          if (noSchool) {
            SStrPrintf(buf, bufSize, tempNoSchool, procTrigger);
          } else {
            SStrPrintf(buf, bufSize, temp, school, procTrigger);
          }
        }
      } else {
        SStrPrintf(buf, bufSize, temp, points, school, "Proc flag not found in global strings. Tell Jeremy.");
      }
      return;
    }
    case 22:
      if (!spell->m_effectMiscValue[effectIndex]) {
        FrameScriptGetSpellString(detail, "SPELL_AURA_MOD_ARMOR", tokenPoints, temp, sizeof(temp), tempNeg, sizeof(tempNeg));
        if (detail != TOOLTIP_DETAIL_GENERIC) {
          SStrPrintf(buf, bufSize, neg ? tempNeg : temp, points);
        } else {
          SStrPrintf(buf, bufSize, neg ? tempNeg : temp);
        }
      } else {
        if (detail != TOOLTIP_DETAIL_GENERIC) {
          SStrPrintf(buf, bufSize, temp, points, school);
        } else {
          SStrPrintf(buf, bufSize, temp, school);
        }
      }
      return;
    case 23:
      SStrPrintf(buf, bufSize, temp, g_spellDB.GetRecord(spell->m_effectTriggerSpell[effectIndex])->m_name_lang[CURRENT_LANGUAGE], period);
      return;
    case 29:
    case 80: {
      char stat[32];
      if (spell->m_effectMiscValue[effectIndex] < 0) {
        SStrCopy(token, "SPELL_STATALL", sizeof(token));
      } else {
        SStrPrintf(token, sizeof(token), "SPELL_STAT%d_NAME", spell->m_effectMiscValue[effectIndex]);
      }
      text = FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE);
      SStrCopy(stat, text, sizeof(stat));
      if (detail != TOOLTIP_DETAIL_GENERIC) {
        SStrPrintf(buf, bufSize, temp, points, stat);
      } else {
        SStrPrintf(buf, bufSize, temp, stat);
      }
      return;
    }
    case 30: {
      const SkillLineRec *skillLine = g_skillLineDB.GetRecord(spell->m_effectMiscValue[effectIndex]);
      if (!skillLine) {
        SStrPrintf(buf, bufSize, "Invalid skill line (%d)!  Tell Kevin", spell->m_effectMiscValue[effectIndex]);
        return;
      }
      if (detail != TOOLTIP_DETAIL_GENERIC) {
        SStrPrintf(buf, bufSize, temp, points, skillLine->m_displayName_lang[CURRENT_LANGUAGE]);
      } else {
        SStrPrintf(buf, bufSize, temp, skillLine->m_displayName_lang[CURRENT_LANGUAGE]);
      }
      return;
    }
    case 44: {
      const CreatureTypeRec *creatureType = g_creatureTypeDB.GetRecord(spell->m_effectMiscValue[effectIndex]);
      if (!creatureType) {
        SStrPrintf(buf, bufSize, "Invalid creature type (%d)!  Tell Kevin", spell->m_effectMiscValue[effectIndex]);
        return;
      }
      SStrPrintf(buf, bufSize, temp, creatureType->m_name_lang[CURRENT_LANGUAGE]);
      return;
    }
    case 45: {
      const LockTypeRec *lockType = g_lockTypeDB.GetRecord(spell->m_effectMiscValue[effectIndex]);
      if (!lockType) {
        SStrPrintf(buf, bufSize, "Invalid resource type (%d)!  Tell Kevin", spell->m_effectMiscValue[effectIndex]);
        return;
      }
      SStrPrintf(buf, bufSize, temp, lockType->m_resourceName_lang[CURRENT_LANGUAGE]);
      return;
    }
    case 36: {
      const SpellShapeshiftFormRec *form = g_spellShapeshiftFormDB.GetRecord(spell->m_effectMiscValue[effectIndex]);
      if (!form) {
        SStrPrintf(buf, bufSize, "Invalid shapeshift form (%d)!  Tell Kevin", spell->m_effectMiscValue[effectIndex]);
        return;
      }
      SStrPrintf(buf, bufSize, temp, form->m_name_lang[CURRENT_LANGUAGE]);
      return;
    }
    case 56: {
      const CreatureStats_C *creature = g_creatureDBCache.GetRecord(
          spell->m_effectMiscValue[effectIndex], spell->m_ID | 0xB000000000000000ui64, TooltipSpellCreatureStatsCallback, &m_spellID
      );
      if (!creature) {
        ++s_itemsWaiting;
        return;
      }
      SStrPrintf(buf, bufSize, temp, creature->m_name[0]);
      return;
    }
    case 59: {
      const CreatureTypeRec *creatureType = g_creatureTypeDB.GetRecord(spell->m_effectMiscValue[effectIndex]);
      if (!creatureType) {
        SStrPrintf(buf, bufSize, "Invalid creature type (%d)!  Tell Kevin", spell->m_effectMiscValue[effectIndex]);
        return;
      }
      if (detail != TOOLTIP_DETAIL_GENERIC) {
        SStrPrintf(buf, bufSize, temp, points, creatureType->m_name_lang[CURRENT_LANGUAGE]);
      } else {
        SStrPrintf(buf, bufSize, temp, creatureType->m_name_lang[CURRENT_LANGUAGE]);
      }
      return;
    }
    case 75: {
      const LanguagesRec *language = g_languagesDB.GetRecord(spell->m_effectMiscValue[effectIndex]);
      if (!language) {
        SStrPrintf(buf, bufSize, "Invalid language ID (%d)!  Tell Kevin", spell->m_effectMiscValue[effectIndex]);
        return;
      }
      SStrPrintf(buf, bufSize, temp, language->m_name_lang[CURRENT_LANGUAGE]);
      return;
    }
    case 4:
      return;
    default:
      SStrCopy(buf, "Missing tooltip info! Please inform Jeremy", bufSize);
      return;
  }
}

static const SpellEffectNamesRec *GetEffectNameRec(int enumID) {
  int i = g_spellEffectNamesDB.GetNumRecords();
  while (i) {
    const SpellEffectNamesRec *record = g_spellEffectNamesDB.GetRecordByIndex(--i);
    if (record->m_EnumID == enumID) {
      return record;
    }
  }
  return 0;
}

static const SpellAuraNamesRec *GetAuraNameRec(int enumID) {
  int i = g_spellAuraNamesDB.GetNumRecords();
  while (i) {
    const SpellAuraNamesRec *record = g_spellAuraNamesDB.GetRecordByIndex(--i);
    if (record->m_EnumID == enumID) {
      return record;
    }
  }
  return 0;
}

void CGTooltip::GetItemEnchantString(char *buf, UINT bufSize, const SpellItemEnchantmentRec *enchant, UINT effectIndex, TOOLTIP_DETAIL detail) {
  if (!buf || !enchant) {
    return;
  }
  *buf = 0;
  if (detail == TOOLTIP_DETAIL_GENERIC) {
    return;
  }

  char points[32];
  if (enchant->m_effectPointsMin[effectIndex] == enchant->m_effectPointsMax[effectIndex]) {
    SStrPrintf(points, sizeof(points), "%d", enchant->m_effectPointsMin[effectIndex]);
  } else {
    SStrPrintf(points, sizeof(points), "%d-%d", enchant->m_effectPointsMin[effectIndex], enchant->m_effectPointsMax[effectIndex]);
  }

  if (!*s_itemEnchantTokens[enchant->m_effect[effectIndex]]) {
    return;
  }

  char temp[64];
  FrameScriptGetEnchantString(detail, s_itemEnchantTokens[enchant->m_effect[effectIndex]], temp, sizeof(temp));

  switch (enchant->m_effect[effectIndex]) {
    case 1:
    case 3: {
      const SpellRec *spell = g_spellDB.GetRecord(enchant->m_effectArg[effectIndex]);
      if (!spell) {
        SStrPrintf(buf, bufSize, "Invalid enchantment spell (%d)!  Tell Kevin", enchant->m_effectArg[effectIndex]);
        return;
      }
      SStrPrintf(buf, bufSize, temp, spell->m_name_lang[CURRENT_LANGUAGE]);
      return;
    }
    case 2: {
      char school[32];
      char token[32];
      SStrPrintf(token, sizeof(token), "SPELL_SCHOOL%d_NAME", enchant->m_effectArg[effectIndex]);
      LPCSTR text = FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE);
      SStrCopy(school, text, sizeof(school));
      if (!enchant->m_effectArg[effectIndex]) {
        SStrPrintf(buf, sizeof(buf), "%s_NOSCHOOL", s_itemEnchantTokens[enchant->m_effect[effectIndex]]);
        FrameScriptGetEnchantString(detail, buf, temp, sizeof(temp));
        SStrPrintf(buf, bufSize, temp, points);
        return;
      }
      SStrPrintf(buf, bufSize, temp, points, school);
      return;
    }
    case 4:
      if (enchant->m_effectArg[effectIndex]) {
        char school[32];
        char token[32];
        FrameScriptGetEnchantString(detail, "ITEM_ENCHANTMENT_ADD_RESISTANCE", temp, sizeof(temp));
        SStrPrintf(token, sizeof(token), "SPELL_SCHOOL%d_NAME", enchant->m_effectArg[effectIndex]);
        LPCSTR text = FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE);
        SStrCopy(school, text, sizeof(school));
        SStrPrintf(buf, bufSize, temp, school, points);
        return;
      }
      SStrPrintf(buf, bufSize, temp, points);
      return;
    default:
      SStrCopy(buf, "Missing tooltip info! Please inform Jeremy", bufSize);
      return;
  }
}

static void FrameScriptGetEnchantString(TOOLTIP_DETAIL detail, LPCSTR stringLabel, char *buf, UINT bufSize) {
  char token[64];
  switch (detail) {
    case TOOLTIP_DETAIL_GENERIC:
      SStrPrintf(token, sizeof(token), "%s_GEN", stringLabel);
      break;
    case TOOLTIP_DETAIL_NORMAL:
      SStrPrintf(token, sizeof(token), "%s", stringLabel);
      break;
    case TOOLTIP_DETAIL_VERBOSE:
      SStrPrintf(token, sizeof(token), "%s_VERBOSE", stringLabel);
      break;
  }
  LPCSTR text = FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE);
  SStrCopy(buf, text, bufSize);
  if (!*text) {
    SStrCopy(buf, token, bufSize);
  }
}

void CGTooltip::GetSpellTargetString(char *buf, UINT bufSize, const SpellRec *spell, UINT effectIndex) {
  if (!buf || !spell) {
    return;
  }

  int                   type = 16;
  int                   centerOnCaster = 0;
  int                   cone = 0;
  int                   radius = 0;
  UINT                  i;
  const SpellRadiusRec *rec = g_spellRadiusDB.GetRecord(spell->m_effectRadiusIndex[effectIndex]);
  if (rec) {
    radius = (int)rec->m_radius;
  }

  for (i = 0; i < 2; ++i) {
    switch (i ? spell->m_implicitTargetB[effectIndex] : spell->m_implicitTargetA[effectIndex]) {
      case 1:
        type = 0;
      case 22:
        centerOnCaster = 1;
        break;
      case 5:
        type = 8;
        break;
      case 4:
      case 35:
        type = 4;
        break;
      case 3:
      case 21:
        type = 2;
        break;
      case 2:
      case 6:
        type = 3;
        break;
      case 7:
        centerOnCaster = 1;
      case 8:
        type = 12;
        break;
      case 13:
        type = 9;
        break;
      case 14:
        type = 10;
        break;
      case 24:
        cone = 1;
      case 15:
        centerOnCaster = 1;
      case 16:
        type = 13;
        break;
      case 20:
        type = radius ? 14 : 11;
        centerOnCaster = 1;
        break;
      case 23:
      case 26:
        type = 7;
        break;
      case 25:
        type = 1;
        break;
      case 27:
        type = 15;
        break;
    }
  }

  if (spell->m_effect[effectIndex] == 35) {
    type = radius ? 14 : 11;
  }

  if (type == 16) {
    if (spell->m_targets & 0x1) {
      type = 0;
    } else if (spell->m_targets & 0x2) {
      type = 1;
    } else if (spell->m_targets & 0x8) {
      type = 4;
    } else if (spell->m_targets & 0x10) {
      type = 5;
    } else if (spell->m_targets & 0x20) {
      type = 6;
    } else if (spell->m_targets & 0x40) {
      type = 6;
    } else if (spell->m_targets & 0x80) {
      type = 3;
    } else if (spell->m_targets & 0x100) {
      type = 2;
    } else if (spell->m_targets & 0x800) {
      type = 7;
    } else {
      *buf = 0;
      return;
    }
  }

  if (!type) {
    const SpellRangeRec *range = g_spellRangeDB.GetRecord(max(spell->m_rangeIndex, 1));
    if (range && NTempest::CMath::fequal_(range->m_rangeMax, 0.0f)) {
      *buf = 0;
      return;
    }
  }

  char noun[512];
  char temp[64];
  if (spell->m_targetCreatureType > 0) {
    char token[64];
    SStrPrintf(token, sizeof(token), "SPELL_TARGET_CREATURE_TYPE%s%d_DESC", spell->m_targets & 0x400 ? "_DEAD" : "", type);

    char typeList[512];
    int  first = 1;
    UINT numEntries = g_creatureTypeDB.GetNumRecords();
    for (i = 0; i < numEntries; ++i) {
      if (spell->m_targetCreatureType & (1 << i)) {
        const CreatureTypeRec *rec = g_creatureTypeDB.GetRecordByIndex(i);
        if (first) {
          SStrCopy(typeList, rec ? rec->m_name_lang[CURRENT_LANGUAGE] : "ERROR", sizeof(typeList));
          first = 0;
        } else {
          SStrPack(typeList, ", ", sizeof(typeList));
          SStrPack(typeList, rec ? rec->m_name_lang[CURRENT_LANGUAGE] : "ERROR", sizeof(typeList));
        }
      }
    }

    if (first) {
      SStrCopy(noun, "<invalid creature type>", sizeof(noun));
    } else {
      LPCSTR text = FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE);
      SStrCopy(temp, text, sizeof(temp));
      if (*text) {
        SStrPrintf(noun, sizeof(noun), temp, typeList);
      } else {
        SStrCopy(noun, "ERROR", sizeof(noun));
      }
    }
  } else {
    SStrPrintf(temp, sizeof(temp), "SPELL_TARGET_TYPE%s%d_DESC", spell->m_targets & 0x400 ? "_DEAD" : "", type);
    LPCSTR text = FrameScript_GetText(temp, -1, GENDER_NOT_APPLICABLE);
    SStrCopy(noun, text, sizeof(noun));
    if (!*text) {
      SStrCopy(noun, "ERROR", sizeof(noun));
    }
  }

  if (spell->m_effectChainTargets[effectIndex] > 0) {
    LPCSTR text = FrameScript_GetText("SPELL_TARGET_CHAIN_TEMPLATE", -1, GENDER_NOT_APPLICABLE);
    SStrCopy(temp, text, sizeof(temp));
    SStrPrintf(buf, bufSize, temp, noun, spell->m_effectChainTargets[effectIndex]);
    return;
  }

  if (!radius) {
    LPCSTR text = FrameScript_GetText("SPELL_TARGET_TEMPLATE", -1, GENDER_NOT_APPLICABLE);
    SStrCopy(temp, text, sizeof(temp));
    SStrPrintf(buf, bufSize, temp, noun);
    return;
  }

  char   center[64];
  LPCSTR text = FrameScript_GetText(centerOnCaster ? "SPELL_TARGET_CENTER_CASTER" : "SPELL_TARGET_CENTER_LOC", -1, GENDER_NOT_APPLICABLE);
  SStrCopy(center, text, sizeof(center));
  text = FrameScript_GetText(cone ? "SPELL_TARGET_CONE_TEMPLATE" : "SPELL_TARGET_MULTIPLE_TEMPLATE", -1, GENDER_NOT_APPLICABLE);
  SStrCopy(temp, text, sizeof(temp));
  SStrPrintf(buf, bufSize, temp, noun, radius, center);
}

void CGTooltip::GetSummonedByString(const CGUnit_C *unitPtr, char *string, UINT size) {
  if (!unitPtr || !string) {
    return;
  }
  DWORDLONG ownerGUID = 0;
  int summonType = 1;
  if (unitPtr->GetCharmedBy()) {
    ownerGUID = unitPtr->GetCharmedBy();
    summonType = 2;
  } else {
    if (unitPtr->GetCreatedBy()) {
      ownerGUID = unitPtr->GetCreatedBy();
      const SpellRec *spell = g_spellDB.GetRecord(unitPtr->GetCreatedBySpell());
      if (spell) {
        switch (spell->m_effect[0]) {
          case 28:
          case 73:
            summonType = 1;
            break;
          case 42:
            summonType = 3;
            break;
          case 50:
          case 74:
            summonType = 4;
            break;
          case 56:
            summonType = 0;
            break;
          default:
            return;
        }
      }
    }
  }
  if (ownerGUID) {
    const CGUnit_C *owner = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(ownerGUID, __FILE__, __LINE__));
    if (owner) {
      SStrPrintf(string, size, FrameScript_GetText(s_summonTypeTokens[summonType], -1, GENDER_NOT_APPLICABLE), owner->GetUnitName());
    }
  }
}

void CGTooltip::SetOwner(CLayoutFrame *owner, TOOLTIP_ANCHORPOINT anchorpoint, float yoffset) {
  g_itemDBCache.CancelCallback(m_itemID, TooltipItemStatsCallback, this);
  SetUnit(0);

  if (owner) {
    m_anchorPoint = anchorpoint;
    if (anchorpoint != TOOLTIP_ANCHOR_NONE) {
      ClearAllPoints(1);
      switch (anchorpoint) {
        case TOOLTIP_ANCHOR_LEFT:
          SetPoint(FRAMEPOINT_BOTTOMRIGHT, owner, FRAMEPOINT_TOPLEFT, 0.0f, yoffset, 1);
          break;
        case TOOLTIP_ANCHOR_RIGHT:
          SetPoint(FRAMEPOINT_BOTTOMLEFT, owner, FRAMEPOINT_TOPRIGHT, 0.0f, yoffset, 1);
          break;
        case TOOLTIP_ANCHOR_BOTTOMLEFT:
          SetPoint(FRAMEPOINT_TOPRIGHT, owner, FRAMEPOINT_BOTTOMLEFT, 0.0f, yoffset, 1);
          break;
        case TOOLTIP_ANCHOR_BOTTOMRIGHT:
          SetPoint(FRAMEPOINT_TOPLEFT, owner, FRAMEPOINT_BOTTOMRIGHT, 0.0f, yoffset, 1);
          break;
        case TOOLTIP_ANCHOR_FIXED:
          SetPoint(FRAMEPOINT_BOTTOMRIGHT, CGGameUI::m_UISimpleParent, FRAMEPOINT_BOTTOMRIGHT, -0.01f, 0.05f, 1);
          break;
        case TOOLTIP_ANCHOR_CURSOR:
          SetPoint(FRAMEPOINT_BOTTOM, CGGameUI::m_UISimpleParent, FRAMEPOINT_BOTTOMLEFT, 0.0f, 0.0f, 1);
          break;
        default:
          FATALASSERT(!"Unknown tooltip anchor point");
          break;
      }
    }
    ClearLines();
    m_reposition = 0;
  }

  SetAlpha(255);
  m_fading = 0;
  m_owner = owner;
}

void CGTooltip::SetOwner(CLayoutFrame *owner, float x, float y) {
  g_itemDBCache.CancelCallback(m_itemID, TooltipItemStatsCallback, m_owner);
  SetUnit(0);

  if (m_owner) {
    ClearAllPoints(1);
    SetPoint(FRAMEPOINT_BOTTOMLEFT, owner, FRAMEPOINT_BOTTOMLEFT, x, STATUS_BAR_OFFSETY + STATUS_BAR_HEIGHT + y, 1);
    ClearLines();
    m_reposition = 1;
  }

  SetAlpha(255);
  m_fading = 0;
  m_owner = owner;
}

void CGTooltip::SetPosition(float x, float y) {
  if (m_owner == CGGameUI::m_UISimpleParent) {
    ClearAllPoints(1);
    SetPoint(FRAMEPOINT_BOTTOM, m_owner, FRAMEPOINT_BOTTOMLEFT, x, y, 1);
  }
}

void CGTooltip::ClearLines() {
  if (m_lines) {
    for (UINT i = 0; i < m_lines; ++i) {
      m_leftStrings[i]->SetWidth(0.0f);
      m_leftStrings[i]->Hide();
      m_rightStrings[i]->Hide();
      m_wrapLine[i] = 0;
    }
    m_lines = 0;
  }
  FrameScript_SignalEvent(322);
}

void CGTooltip::AddLine(LPCSTR leftText, LPCSTR rightText, const NTempest::CImVector &leftColor, const NTempest::CImVector &rightColor, int wrapped) {
  if (rightText && *rightText) {
    wrapped = 0;
  }

  if (m_lines == m_linesMax || !((leftText && *leftText) || (rightText && *rightText))) {
    return;
  }

  if (leftText && *leftText) {
    m_leftStrings[m_lines]->SetVertexColor(leftColor);
    m_leftStrings[m_lines]->SetText(leftText);
    m_leftStrings[m_lines]->Show();
  }
  if (rightText && *rightText) {
    m_rightStrings[m_lines]->SetVertexColor(rightColor);
    m_rightStrings[m_lines]->SetText(rightText);
    m_rightStrings[m_lines]->Show();
  }
  m_wrapLine[m_lines] = wrapped;
  ++m_lines;
}

void CGTooltip::AddLine(LPCSTR leftText, LPCSTR rightText, int wrapped) {
  AddLine(leftText, rightText, s_defaultColor, s_defaultColor, wrapped);
}

void CGTooltip::AddLine(LPCSTR text, const NTempest::CImVector &color, int wrapped) {
  AddLine(text, 0, color, color, wrapped);
}

void CGTooltip::AppendText(LPCSTR text) {
  if (!m_lines) {
    return;
  }
  char buf[256];
  SStrPrintf(buf, sizeof(buf), "%s%s", m_leftStrings[0]->GetText(), text);
  m_leftStrings[0]->SetText(buf);
  CalculateSize();
}

void CGTooltip::SetTooltipPadding(float right) {
  m_padding = right;
}

void CGTooltip::CalculateSize() {
  float frameWidth = 0.0f;
  UINT  i;

  for (i = 0; i < m_lines; ++i) {
    if (!m_wrapLine[i]) {
      float width = 0.0f;
      if (m_leftStrings[i]->IsVisible() && m_rightStrings[i]->IsVisible()) {
        width = COLUMN_SPACING;
      }
      if (m_leftStrings[i]->IsVisible()) {
        width += m_leftStrings[i]->GetWidth();
      }
      if (m_rightStrings[i]->IsVisible()) {
        width += m_rightStrings[i]->GetWidth();
      }
      if (frameWidth < width) {
        frameWidth = width;
      }
    }
  }

  for (i = 0; i < m_lines; ++i) {
    if (m_wrapLine[i]) {
      float width = m_leftStrings[i]->GetStringWidth();
      width = min(width, WORDWRAP_MIN_WIDTH);
      if (width > frameWidth) {
        LPCSTR text = m_leftStrings[i]->GetText();
        UINT   offsets[10];
        UINT   lines = m_leftStrings[i]->WrapText(text, width, offsets, 10);
        width = 0.0f;
        for (UINT j = 0; j < lines; ++j) {
          float textWidth = m_leftStrings[i]->GetTextWidth(text + offsets[j], (j < lines - 1 ? offsets[j + 1] : SStrLen(text)) - offsets[j]);
          if (textWidth > width) {
            width = textWidth;
          }
        }
      }
      frameWidth = max(frameWidth, width);
      m_leftStrings[i]->SetWidth(frameWidth);
    }
  }

  for (i = 0; i < m_lines; ++i) {
    if (m_rightStrings[i]->IsVisible()) {
      m_rightStrings[i]->SetPoint(FRAMEPOINT_RIGHT, m_leftStrings[i], FRAMEPOINT_LEFT, frameWidth, 0.0f, 1);
    }
  }

  float frameHeight = 0.0f;
  for (i = 0; i < m_lines; ++i) {
    if (m_leftStrings[i]->IsVisible()) {
      if (frameHeight != 0.0f) {
        frameHeight += LINE_SPACING;
      }
      frameHeight += m_leftStrings[i]->GetHeight();
    }
  }

  float width = XOFFSET * 2.0f + m_padding + frameWidth;
  float height = YOFFSET * 2.0f + frameHeight;
  SetWidth(width);
  SetHeight(height);

  if (m_reposition) {
    if (Left() < 0.0f) {
      SetPoint(FRAMEPOINT_BOTTOMLEFT, m_owner, FRAMEPOINT_BOTTOMLEFT, 0.0f, Bottom(), 1);
    } else if (Right() > 0.8f) {
      SetPoint(FRAMEPOINT_BOTTOMLEFT, m_owner, FRAMEPOINT_BOTTOMRIGHT, -width, Bottom(), 1);
    }

    if (Bottom() < 0.0f) {
      SetPoint(FRAMEPOINT_BOTTOMLEFT, m_owner, FRAMEPOINT_BOTTOMLEFT, Left(), 0.0f, 1);
    } else if (Top() > 0.6f) {
      SetPoint(FRAMEPOINT_BOTTOMLEFT, m_owner, FRAMEPOINT_TOPLEFT, Left(), -height - STATUS_BAR_HEIGHT - STATUS_BAR_OFFSETY, 1);
    }
  } else if (Top() > 0.6f) {
    float left = Left();
    ClearAllPoints(1);
    SetPoint(FRAMEPOINT_TOPLEFT, CGGameUI::m_UISimpleParent, FRAMEPOINT_TOPLEFT, left, 0.0f, 1);
  }

  Resize(1);
}

BOOL CGTooltip::HideThis() {
  SetOwner(0, TOOLTIP_ANCHOR_LEFT, 0.0f);
  return CSimpleFrame::HideThis();
}

BOOL CGTooltip::ShowThis() {
  if (m_owner && m_lines) {
    SetAlpha(255);
    m_fading = 0;
    CalculateSize();
    return CSimpleFrame::ShowThis();
  }

  m_shown = 0;
  HideThis();
  return 0;
}

void CGTooltip::FadeOut() {
  if (m_anchorPoint == TOOLTIP_ANCHOR_CURSOR) {
    HideThis();
    m_fading = 0;
  } else {
    m_fading = 1;
    m_fadeTime = TOOLTIP_FADE_TIME;
  }
}

void CGTooltip::OnLayerUpdate(float elapsedSec) {
  CSimpleFrame::OnLayerUpdate(elapsedSec);

  if (m_anchorPoint == TOOLTIP_ANCHOR_CURSOR) {
    NTempest::C2Vector mousePos;
    NDCToDDC(m_top->m_mousePosition.x, m_top->m_mousePosition.y, &mousePos.x, &mousePos.y);
    mousePos /= m_layoutScale;
    SetPoint(
        FRAMEPOINT_BOTTOM, CGGameUI::m_UISimpleParent ? (CLayoutFrame *)CGGameUI::m_UISimpleParent : 0, FRAMEPOINT_BOTTOMLEFT,
        mousePos.x, mousePos.y, 1
    );
  }

  if (m_fading) {
    elapsedSec = m_fadeTime - elapsedSec;
    m_fadeTime = elapsedSec;
    if (elapsedSec <= 0.0f) {
      SetAlpha(255);
      Hide();
    } else {
      SetAlpha((BYTE)((elapsedSec >= TOOLTIP_FADE_TIME * 0.5f ? 1.0f : elapsedSec / (TOOLTIP_FADE_TIME * 0.5f)) * 255.0f));
    }
  }
}

static int CGTooltip_SetPadding(lua_State *L) {
  CGTooltip *tooltip = (CGTooltip *)FrameScript_GetObjectThis(L);
  tooltip->SetTooltipPadding((float)(0.8f * (lua_tonumber(L, 2) * 0.0009765625f)));
  return 0;
}

static int CGTooltip_IsOwned(lua_State *L) {
  CGTooltip *tooltip = (CGTooltip *)FrameScript_GetObjectThis(L);
  lua_pushnumber(L, 0.0);
  lua_gettable(L, 2);
  CSimpleFrame *frame = (CSimpleFrame *)lua_touserdata(L, -1);
  lua_pop(L, 1);
  ASSERT(frame);
  if (tooltip->GetOwner() == frame) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int CGTooltip_SetOwner(lua_State *L) {
  CGTooltip *tooltip = (CGTooltip *)FrameScript_GetObjectThis(L);
  tooltip->Hide();

  lua_pushnumber(L, 0.0);
  lua_gettable(L, 2);
  CSimpleFrame *owner = (CSimpleFrame *)lua_touserdata(L, -1);
  lua_pop(L, 1);
  ASSERT(owner);

  TOOLTIP_ANCHORPOINT anchorpoint = TOOLTIP_ANCHOR_LEFT;
  if (lua_isstring(L, 3)) {
    LPCSTR name = lua_tostring(L, 3);
    if (!SStrCmpI(name, "ANCHOR_LEFT", 0x7FFFFFFF)) {
      anchorpoint = TOOLTIP_ANCHOR_LEFT;
    } else if (!SStrCmpI(name, "ANCHOR_RIGHT", 0x7FFFFFFF)) {
      anchorpoint = TOOLTIP_ANCHOR_RIGHT;
    } else if (!SStrCmpI(name, "ANCHOR_BOTTOMRIGHT", 0x7FFFFFFF)) {
      anchorpoint = TOOLTIP_ANCHOR_BOTTOMRIGHT;
    } else if (!SStrCmpI(name, "ANCHOR_BOTTOMLEFT", 0x7FFFFFFF)) {
      anchorpoint = TOOLTIP_ANCHOR_BOTTOMLEFT;
    } else if (!SStrCmpI(name, "ANCHOR_FIXED", 0x7FFFFFFF)) {
      anchorpoint = TOOLTIP_ANCHOR_FIXED;
    } else if (!SStrCmpI(name, "ANCHOR_CURSOR", 0x7FFFFFFF)) {
      anchorpoint = TOOLTIP_ANCHOR_CURSOR;
    } else if (!SStrCmpI(name, "ANCHOR_NONE", 0x7FFFFFFF)) {
      anchorpoint = TOOLTIP_ANCHOR_NONE;
    }
  }

  float yoffset = 0.0f;
  if (lua_isnumber(L, 4)) {
    yoffset = (float)lua_tonumber(L, 4);
  }
  tooltip->SetOwner(owner, anchorpoint, yoffset);
  return 0;
}

static int CGTooltip_ClearLines(lua_State *L) {
  CGTooltip *tooltip = (CGTooltip *)FrameScript_GetObjectThis(L);
  tooltip->ClearLines();
  return 0;
}

static int CGTooltip_AddLine(lua_State *L) {
  CGTooltip *tooltip = (CGTooltip *)FrameScript_GetObjectThis(L);
  LPCSTR              leftText = 0;
  LPCSTR              rightText = 0;
  NTempest::CImVector leftColor = s_defaultColor;
  NTempest::CImVector rightColor = s_defaultColor;
  int                 argument = 2;

  if (lua_isstring(L, argument)) {
    leftText = lua_tostring(L, argument++);
  }
  if (lua_isstring(L, argument)) {
    rightText = lua_tostring(L, argument++);
  }

  if (lua_isnumber(L, argument)) {
    float r = (float)lua_tonumber(L, argument++);
    float g = (float)lua_tonumber(L, argument++);
    float b = (float)lua_tonumber(L, argument++);
    float a = 1.0f;
    if (lua_isnumber(L, argument)) {
      a = (float)lua_tonumber(L, argument++);
    }
    leftColor.Set(a, r, g, b);
  }
  if (lua_isnumber(L, argument)) {
    float r = (float)lua_tonumber(L, argument++);
    float g = (float)lua_tonumber(L, argument++);
    float b = (float)lua_tonumber(L, argument++);
    float a = 1.0f;
    if (lua_isnumber(L, argument)) {
      a = (float)lua_tonumber(L, argument++);
    }
    rightColor.Set(a, r, g, b);
  }

  tooltip->AddLine(leftText, rightText, leftColor, rightColor, 0);
  return 0;
}

static int CGTooltip_SetText(lua_State *L) {
  CGTooltip *tooltip = (CGTooltip *)FrameScript_GetObjectThis(L);
  if (lua_isstring(L, 2)) {
    LPCSTR              tooltipText = lua_tostring(L, 2);
    NTempest::CImVector tooltipColor = s_defaultColor;
    if (lua_isnumber(L, 3)) {
      float r = (float)lua_tonumber(L, 3);
      float g = (float)lua_tonumber(L, 4);
      float b = (float)lua_tonumber(L, 5);
      float a = 1.0f;
      if (lua_isnumber(L, 6)) {
        a = (float)lua_tonumber(L, 6);
      }
      tooltipColor.Set(a, r, g, b);
    }
    tooltip->ClearLines();
    tooltip->AddLine(tooltipText, tooltipColor, 0);
    tooltip->Show();
    return 0;
  }
  luaL_error(L, "Usage: SetText(\"text\" [, color])");
  return 0;
}

static int CGTooltip_AppendText(lua_State *L) {
  CGTooltip *tooltip = (CGTooltip *)FrameScript_GetObjectThis(L);
  if (lua_isstring(L, 2)) {
    tooltip->AppendText(lua_tostring(L, 2));
    return 0;
  }
  luaL_error(L, "Usage: AppendText(\"text\")");
  return 0;
}

static int CGTooltip_FadeOut(lua_State *L) {
  CGTooltip *tooltip = (CGTooltip *)FrameScript_GetObjectThis(L);
  tooltip->FadeOut();
  return 0;
}

static int CGTooltip_SetHyperlink(lua_State *L) {
  CGTooltip *tooltip = (CGTooltip *)FrameScript_GetObjectThis(L);
  if (lua_isstring(L, 2)) {
    LPCSTR link = lua_tostring(L, 2);
    int    itemID;
    if (!SStrCmpI(link, "item:", 5) && (itemID = SStrToInt(link + 5)) > 0) {
      tooltip->SetItem(itemID, 0, 0, 0, 1, 0);
      return 0;
    }
    luaL_error(L, "Unknown link type");
    return 0;
  }
  luaL_error(L, "Usage: SetHyperlink(link)");
  return 0;
}

static int CGTooltip_SetAction(lua_State *L) {
  CGTooltip *tooltip = (CGTooltip *)FrameScript_GetObjectThis(L);
  int        result = 0;
  if (lua_isnumber(L, 2)) {
    int   slot = (int)lua_tonumber(L, 2) - 1;
    DWORD startTime;
    UINT  duration;
    UINT  enable;
    CGActionBar::GetCooldown(slot, startTime, duration, enable);
    tooltip->ClearLines();

    char buf[32];
    if (CGActionBar::IsAttackAction(slot)) {
      LPCSTR text = FrameScript_GetText("ATTACK", -1, GENDER_NOT_APPLICABLE);
      SStrCopy(buf, text, sizeof(buf));
      tooltip->AddLine(buf, NTempest::CImVector(0xFFFFFFFFUL), 0);
      tooltip->Show();
    } else if (CGActionBar::IsItem(slot)) {
      CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
      CGItem_C   *item = player ? player->Inventory()->FindItemOfType(CGActionBar::GetItem(slot), 0) : 0;
      if (item) {
        if (startTime > 0 && duration > 0) {
          TooltipExtendedItemInfo info;
          info.enchantment[0] = 0;
          info.enchantmentExpiration[0] = 0;
          info.enchantment[1] = 0;
          info.enchantmentExpiration[1] = 0;
          info.cooldownTime = startTime + duration - OsGetAsyncTimeMs();
          info.proposedEnchantment = 0;
          info.creator = item->GetCreator();
          result = tooltip->SetItem(item->GetEntryID(), item->GetGUID(), item->GetGUID(), 0, 0, &info);
        } else {
          tooltip->SetItem(item->GetEntryID(), item->GetGUID(), item->GetGUID(), 0, 0, 0);
        }
      } else {
        LPCSTR text = FrameScript_GetText("USE_ITEM", -1, GENDER_NOT_APPLICABLE);
        SStrCopy(buf, text, sizeof(buf));
        tooltip->AddLine(buf, NTempest::CImVector(0xFFFFFFFFUL), 0);
        tooltip->Show();
      }
    } else if (CGActionBar::IsSpell(slot)) {
      if (startTime > 0 && duration > 0) {
        result = tooltip->SetSpell(CGActionBar::GetSpell(slot), 1, startTime + duration - OsGetAsyncTimeMs(), 0);
      } else {
        tooltip->SetSpell(CGActionBar::GetSpell(slot), 1, 0, 0);
      }
    }
  } else {
    luaL_error(L, "Usage: SetAction(slot)");
  }

  if (result) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int CGTooltip_SetPlayerBuff(lua_State *L) {
  CGTooltip *tooltip = (CGTooltip *)FrameScript_GetObjectThis(L);
  if (lua_isnumber(L, 2)) {
    int               index = (int)lua_tonumber(L, 2);
    const CGBuffDesc *buff = CGBuffBar::GetBuffByIndex(index);
    tooltip->SetBuff(buff->GetAuraSpell(), buff->GetAuraFlags());
    if (!buff->GetUntilCancelled()) {
      UINT   duration = CGBuffBar::GetBuffTimeLeftByIndex(index);
      BOOL   minutes = duration >= 60000;
      char   temp[64];
      LPCSTR text = FrameScript_GetText(minutes ? "SPELL_TIME_REMAINING_MIN" : "SPELL_TIME_REMAINING_SEC", -1, GENDER_NOT_APPLICABLE);
      SStrCopy(temp, text, sizeof(temp));
      if (minutes) {
        duration = (UINT)((float)duration * (1.0f / 60000.0f) + 0.99f);
      } else {
        duration /= 1000;
      }
      char buf[32];
      SStrPrintf(buf, sizeof(buf), temp, max(duration, 0));
      tooltip->AddLine(buf, 0, 0);
    }
    tooltip->Show();
    return 0;
  }
  luaL_error(L, "Usage: SetPlayerBuff(buffIndex)");
  return 0;
}

static int CGTooltip_SetSpell(lua_State *L) {
  CGTooltip *tooltip = (CGTooltip *)FrameScript_GetObjectThis(L);
  int slot;
  if (lua_isnumber(L, 2) && lua_isstring(L, 3) && (slot = (int)(lua_tonumber(L, 2) - 1.0)) >= 0 && slot < 1024U) {
    UI_SPELL_TYPE type = PLAYER_SPELL;
    LPCSTR        name = lua_tostring(L, 3);
    if (SStrCmpI("spell", name, 0x7FFFFFFF)) {
      if (!SStrCmpI("ability", name, 0x7FFFFFFF)) {
        type = PLAYER_ABILITY;
      } else if (!SStrCmpI("pet", name, 0x7FFFFFFF)) {
        type = PET_SPELL;
      }
    }

    int spellID = CGSpellBook::GetSpell(slot, type);
    if (spellID > 0) {
      DWORD startTime = 0;
      UINT  duration = 0;
      UINT  enable = 0;
      BOOL  isPet = type == PET_SPELL;
      Spell_C_GetSpellCooldown(spellID, isPet, &duration, &startTime, &enable);
      if (startTime > 0 && duration > 0 ? tooltip->SetSpell(spellID, 0, duration + startTime - OsGetAsyncTimeMs(), isPet)
                                        : tooltip->SetSpell(spellID, 0, 0, isPet))
      {
        lua_pushnumber(L, 1.0);
        return 1;
      }
    }
    lua_pushnil(L);
    return 1;
  }
  luaL_error(L, "Invalid spell slot in SetSpell");
  return 0;
}

static int CGTooltip_SetInventoryItem(lua_State *L) {
  CGTooltip *tooltip = (CGTooltip *)FrameScript_GetObjectThis(L);
  int hasCooldown = 0;
  if (!lua_isstring(L, 2)) {
    luaL_error(L, "Usage: SetInventoryItem(unit, slot [, nameOnly])");
    return 0;
  }

  int slot;
  if (!lua_isnumber(L, 3) ||
      !(((slot = (int)(lua_tonumber(L, 3) - 1.0)) >= 0 && slot <= 22) || (slot >= 39 && slot <= 62) || (slot >= 63 && slot <= 68)))
  {
    luaL_error(L, "Invalid inventory slot in SetInventoryItem");
    return 0;
  }

  int nameOnly = 0;
  if (lua_isnumber(L, 4) && lua_tonumber(L, 4) > 0.0) {
    nameOnly = 1;
  }
  CGUnit_C *unit = Script_GetUnitFromName(lua_tostring(L, 2));
  if (unit && unit->IsA(TYPE_PLAYER)) {
    CGItem_C *item =
        static_cast<CGItem_C *>(ClntObjMgrObjectPtr(((CGPlayer_C *)unit)->Inventory()->GetItem(slot), __FILE__, __LINE__));
    if (item) {
      DWORD startTime = 0;
      UINT  duration = 0;
      Spell_C_GetItemCooldown(item->GetEntryID(), &duration, &startTime, 0);

      if (duration > 0 && startTime > 0) {
        TooltipExtendedItemInfo info;
        info.enchantment[0] = 0;
        info.enchantmentExpiration[0] = 0;
        info.enchantment[1] = 0;
        info.enchantmentExpiration[1] = 0;
        info.cooldownTime = startTime + duration - OsGetAsyncTimeMs();
        info.proposedEnchantment = 0;
        info.creator = item->GetCreator();
        hasCooldown = tooltip->SetItem(item->GetEntryID(), unit->GetGUID(), item->GetGUID(), nameOnly, 0, &info);
      } else {
        tooltip->SetItem(item->GetEntryID(), unit->GetGUID(), item->GetGUID(), nameOnly, 0, 0);
      }

      lua_pushnumber(L, 1.0);
      if (hasCooldown) {
        lua_pushnumber(L, 1.0);
      } else {
        lua_pushnil(L);
      }
      return 2;
    }
  }
  lua_pushnil(L);
  lua_pushnil(L);
  return 2;
}

static int CGTooltip_SetLootItem(lua_State *L) {
  CGTooltip *tooltip = (CGTooltip *)FrameScript_GetObjectThis(L);
  if (lua_isnumber(L, 2)) {
    int itemID = CGLootInfo::GetLootItem((UINT)(lua_tonumber(L, 2) - 1.0));
    if (itemID > 0) {
      tooltip->SetItem(itemID, CGLootInfo::GetObject(), 0, 0, 1, 0);
      return 0;
    }
  }
  luaL_error(L, "Invalid loot slot in SetInventoryItem");
  return 0;
}

static int CGTooltip_SetQuestItem(lua_State *L) {
  CGTooltip *tooltip = (CGTooltip *)FrameScript_GetObjectThis(L);
  int itemID;
  if (!lua_isstring(L, 2) || !lua_isnumber(L, 3) ||
      (itemID = CGQuestInfo::GetQuestItemID(lua_tostring(L, 2), (int)lua_tonumber(L, 3) - 1)) <= 0)
  {
    luaL_error(L, "Invalid quest item in SetQuestItem(\"type\", index)");
    return 0;
  }
  tooltip->SetItem(itemID, CGQuestInfo::GetQuestGiver(), 0, 0, 1, 0);
  return 0;
}

static int CGTooltip_SetQuestLogItem(lua_State *L) {
  CGTooltip *tooltip = (CGTooltip *)FrameScript_GetObjectThis(L);
  int itemID;
  if (!lua_isstring(L, 2) || !lua_isnumber(L, 3) ||
      (itemID = CGQuestLog::GetQuestItemID(lua_tostring(L, 2), (int)lua_tonumber(L, 3) - 1)) <= 0)
  {
    luaL_error(L, "Invalid quest item in SetQuestLogItem(\"type\", index)");
    return 0;
  }
  tooltip->SetItem(itemID, 0, 0, 0, 1, 0);
  return 0;
}

static int CGTooltip_SetTrainerService(lua_State *L) {
  CGTooltip *tooltip = (CGTooltip *)FrameScript_GetObjectThis(L);
  if (lua_isnumber(L, 2)) {
    const TrainerServiceInfo *service = CGClassTrainer::GetService((int)lua_tonumber(L, 2) - 1);
    if (service) {
      const SpellRec *spell = g_spellDB.GetRecord(service->spellID);
      if (spell) {
        for (int i = 0; i < 3; ++i) {
          if (spell->m_effect[i] == 36 || spell->m_effect[i] == 57) {
            const SpellRec *triggerSpell = g_spellDB.GetRecord(spell->m_effectTriggerSpell[i]);
            if (triggerSpell) {
              if ((triggerSpell->m_attributes & 0x20) && triggerSpell->m_effect[0] == 24) {
                tooltip->SetItem(triggerSpell->m_effectItemType[i], triggerSpell->m_ID | 0xB000000000000000ui64, 0, 0, 1, 0);
                return 0;
              }
              tooltip->SetSpell(triggerSpell->m_ID, 0, 0, 0);
              return 0;
            }
          }
        }
        tooltip->SetSpell(spell->m_ID, 0, 0, 0);
        return 0;
      }
    }
  }
  luaL_error(L, "Invalid trainer service in SetTrainerService(index)");
  return 0;
}

static int CGTooltip_SetTradeSkillItem(lua_State *L) {
  CGTooltip *tooltip = (CGTooltip *)FrameScript_GetObjectThis(L);
  if (lua_isnumber(L, 2)) {
    const TradeSkillInfo *info = CGTradeSkillInfo::GetTradeSkillInfo((int)lua_tonumber(L, 2) - 1);
    if (info) {
      const SpellRec *spell = g_spellDB.GetRecord(info->spellID);
      if (spell) {
        if (!lua_isnumber(L, 3)) {
          tooltip->SetItem(spell->m_effectItemType[0], info->spellID | 0xB000000000000000ui64, 0, 0, 1, 0);
          return 0;
        }
        int reagent = (int)lua_tonumber(L, 3);
        int found = 0;
        for (UINT i = 0; i < 8; ++i) {
          if (spell->m_reagent[i] && ++found == reagent) {
            tooltip->SetItem(spell->m_reagent[i], info->spellID | 0xB000000000000000ui64, 0, 0, 1, 0);
            return 0;
          }
        }
      }
    }
  }
  luaL_error(L, "Invalid trade skill item in SetTradeSkillItem(index [,reagent])");
  return 0;
}

static int CGTooltip_SetCraftItem(lua_State *L) {
  CGTooltip *tooltip = (CGTooltip *)FrameScript_GetObjectThis(L);
  if (lua_isnumber(L, 2) && lua_isnumber(L, 3)) {
    const CraftInfo *info = CGCraftInfo::GetCraftInfo((int)lua_tonumber(L, 2) - 1);
    if (info) {
      const SpellRec *spell = g_spellDB.GetRecord(info->spellID);
      if (spell) {
        int reagent = (int)lua_tonumber(L, 3);
        int found = 0;
        for (UINT i = 0; i < 8; ++i) {
          if (spell->m_reagent[i] && ++found == reagent) {
            tooltip->SetItem(spell->m_reagent[i], info->spellID | 0xB000000000000000ui64, 0, 0, 1, 0);
            return 0;
          }
        }
      }
    }
  }
  luaL_error(L, "Invalid craft item in SetCraftItem(index, reagent)");
  return 0;
}

static int CGTooltip_SetCraftSpell(lua_State *L) {
  CGTooltip *tooltip = (CGTooltip *)FrameScript_GetObjectThis(L);
  if (lua_isnumber(L, 2)) {
    const CraftInfo *info = CGCraftInfo::GetCraftInfo((int)lua_tonumber(L, 2) - 1);
    if (info) {
      tooltip->SetSpell(info->spellID, 0, 0, 0);
      return 0;
    }
  }
  luaL_error(L, "Invalid craft in SetCraftSpell(index)");
  return 0;
}

static int CGTooltip_SetMerchantItem(lua_State *L) {
  CGTooltip *tooltip = (CGTooltip *)FrameScript_GetObjectThis(L);
  if (lua_isnumber(L, 2)) {
    const VendorItem *item = CGMerchantInfo::GetItem((int)(lua_tonumber(L, 2) - 1.0));
    if (item && item->m_itemType > 0) {
      tooltip->SetItem(item->m_itemType, CGMerchantInfo::GetMerchant(), 0, 0, 1, 0);
    }
    return 0;
  }
  luaL_error(L, "Invalid merchant slot in SetMerchantItem");
  return 0;
}

static int CGTooltip_SetTradePlayerItem(lua_State *L) {
  CGTooltip *tooltip = (CGTooltip *)FrameScript_GetObjectThis(L);
  if (lua_isnumber(L, 2)) {
    int       index = (int)(lua_tonumber(L, 2) - 1.0);
    DWORDLONG playerItemGUID;
    DWORDLONG bagGUID;
    BYTE      bagSlot;
    CGTradeInfo::GetPlayerItemInfo(index, playerItemGUID, bagGUID, bagSlot);
    CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(playerItemGUID, __FILE__, __LINE__));
    if (item) {
      TooltipExtendedItemInfo info;
      info.enchantment[0] = 0;
      info.enchantmentExpiration[0] = 0;
      info.enchantment[1] = 0;
      info.enchantmentExpiration[1] = 0;
      info.cooldownTime = 0;
      info.proposedEnchantment = 0;
      info.creator = item->GetCreator();
      int proposedEnchantment;
      int proposedEnchantmentSlot;
      Trade_C_GetProposedEnchantment(1, proposedEnchantment, proposedEnchantmentSlot);
      if (proposedEnchantmentSlot == index) {
        info.proposedEnchantment = proposedEnchantment;
      }
      tooltip->SetItem(item->GetEntryID(), item->GetGUID(), item->GetGUID(), 0, 1, &info);
    }
    return 0;
  }
  luaL_error(L, "Invalid trade slot in SetTradePlayerItem");
  return 0;
}

static int CGTooltip_SetTradeTargetItem(lua_State *L) {
  CGTooltip *tooltip = (CGTooltip *)FrameScript_GetObjectThis(L);
  if (lua_isnumber(L, 2)) {
    int index = (int)(lua_tonumber(L, 2) - 1.0);
    int itemID = CGTradeInfo::GetTargetTradeItem(index);
    if (itemID > 0) {
      TooltipExtendedItemInfo info;
      info.enchantment[0] = CGTradeInfo::GetTargetTradeItemEnachantment(index);
      info.enchantmentExpiration[0] = 0;
      info.enchantment[1] = 0;
      info.enchantmentExpiration[1] = 0;
      info.cooldownTime = 0;
      info.proposedEnchantment = 0;
      info.creator = CGTradeInfo::GetTargetTradeItemCreator(index);
      int proposedEnchantment;
      int proposedEnchantmentSlot;
      Trade_C_GetProposedEnchantment(0, proposedEnchantment, proposedEnchantmentSlot);
      if (proposedEnchantmentSlot == index) {
        info.proposedEnchantment = proposedEnchantment;
      }
      tooltip->SetItem(itemID, CGTradeInfo::GetTradePartner(), 0, 0, 1, &info);
    }
    return 0;
  }
  luaL_error(L, "Invalid trade slot in SetTradeTargetItem");
  return 0;
}

static int CGTooltip_SetBagItem(lua_State *L) {
  CGTooltip *tooltip = (CGTooltip *)FrameScript_GetObjectThis(L);
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  CGBag_C *bag = 0;
  if (player && lua_isnumber(L, 2) && lua_isnumber(L, 3)) {
    int bagIndex = (int)lua_tonumber(L, 2) - 1;
    if (bagIndex == -1) {
      bag = player->Inventory();
    } else if (bagIndex >= 0 && bagIndex < 10) {
      CGObject_C *bagObject = ClntObjMgrObjectPtr(CGContainerInfo::GetContainer(bagIndex), __FILE__, __LINE__);
      if (bagObject) {
        bag = bagObject->GetBag();
      }
    }
  }
  if (!bag) {
    luaL_error(L, "Invalid bag slot in SetBagItem");
    return 0;
  }

  int slot = (int)lua_tonumber(L, 3) - 1;
  if (bag == player->Inventory()) {
    slot += 23;
  }
  if (slot >= 0 && slot < (int)bag->NumSlots()) {
    CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(bag->GetItem(slot), __FILE__, __LINE__));
    if (item) {
      UINT  duration = 0;
      DWORD startTime = 0;
      Spell_C_GetItemCooldown(item->GetEntryID(), &duration, &startTime, 0);

      int result;
      if (duration > 0 && startTime > 0) {
        TooltipExtendedItemInfo info;
        info.enchantment[0] = 0;
        info.enchantmentExpiration[0] = 0;
        info.enchantment[1] = 0;
        info.enchantmentExpiration[1] = 0;
        info.cooldownTime = startTime + duration - OsGetAsyncTimeMs();
        info.proposedEnchantment = 0;
        info.creator = item->GetCreator();
        result = tooltip->SetItem(item->GetEntryID(), item->GetGUID(), item->GetGUID(), 0, 1, &info);
      } else {
        result = tooltip->SetItem(item->GetEntryID(), item->GetGUID(), item->GetGUID(), 0, 1, 0);
      }
      if (result) {
        lua_pushnumber(L, 1.0);
        return 1;
      }
    }
  }
  lua_pushnil(L);
  return 1;
}

static int CGTooltip_SetUnit(lua_State *L) {
  CGTooltip *tooltip = (CGTooltip *)FrameScript_GetObjectThis(L);
  if (!lua_isstring(L, 2)) {
    luaL_error(L, "Usage: SetUnit(\"unit\")");
    return 0;
  }
  DWORDLONG target = Script_GetGUIDFromName(lua_tostring(L, 2));
  if (target && tooltip->SetUnit(target)) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int CGTooltip_NumLines(lua_State *L) {
  CGTooltip *tooltip = (CGTooltip *)FrameScript_GetObjectThis(L);
  lua_pushnumber(L, (double)tooltip->NumLines());
  return 1;
}

TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> CGTooltip::s_scriptMethods;

void CGTooltip::RegisterScriptMethods() {
  FrameScript_Object::FillScriptMethodTable(CGTooltipMethods, 26, s_scriptMethods);
}

void CGTooltip::UnregisterScriptMethods() {
  FrameScript_Object::EmptyScriptMethodTable(s_scriptMethods);
}

BOOL CGTooltip::LookupScriptMethod(lua_State *L, LPCSTR name) {
  if (FrameScript_Object::LookupScriptMethod(L, name, s_scriptMethods)) {
    return 1;
  }
  return CSimpleFrame::LookupScriptMethod(L, name);
}

inline const ItemEnchantment *CGItem::GetEnchantment(int index) const {
  if (m_item->m_staticFlags & ITEM_FLAG_PETITION) {
    static ItemEnchantment empty;
    return &empty;
  }
  return &m_item->m_enchantment[index];
}
