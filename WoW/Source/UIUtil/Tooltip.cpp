#include "Tooltip.h"

#include "DB/DBClient/AutoCode/SpellRec.h"
#include "DB/DBClient/AutoCode/SpellAuraNamesRec.h"
#include "DB/DBClient/AutoCode/SpellEffectNamesRec.h"
#include "DB/DBClient/AutoCode/SpellItemEnchantmentRec.h"
#include "DB/DBClient/DBCacheInstances.h"
#include "DB/DBClient/DBClient.h"
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
#include "Ui/QuestLog.h"
#include "Ui/QuestFrame.h"
#include "Ui/SpellBookFrame.h"
#include "Ui/TradeFrame.h"

#include <Base/Coordinate.h>
#include <Frame/CSimpleRender.h>
#include <Frame/CSimpleStatusBar.h>
#include <Frame/CSimpleTop.h>
#include <Frame/SimpleFrameRegistry.h>
#include <FrameScript/FrameScript.h>
#include <Os/OsTime.h>
#include <lauxlib.h>
#include <lua.h>
#include <storm.h>

class CGBuffDesc {
 public:
  __forceinline ~CGBuffDesc() {
  }
  int GetAuraSpell() const {
    return m_auraSpell;
  }
  unsigned char GetAuraFlags() const {
    return m_auraFlags;
  }
  int GetUntilCancelled() const {
    return m_untilCancelled;
  }

 protected:
  int          m_auraIndex;
  int          m_auraSpell;
  unsigned char m_auraFlags;
  int           m_untilCancelled;
};

class CGBuffBar {
 public:
  static const CGBuffDesc *GetBuffByIndex(int buffIndex);
  static unsigned int GetBuffTimeLeftByIndex(int buffIndex);
};

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

class CGTradeSkillInfo {
 public:
  static const TradeSkillInfo *GetTradeSkillInfo(unsigned int index) {
    return index < m_filteredSkills ? m_skills[index] : 0;
  }

 private:
  static unsigned int                      m_numSkills;
  static unsigned int                      m_filteredSkills;
  static TSGrowableArray<TradeSkillInfo *> m_skills;
};

struct CraftInfo {
  int spellID;
  int skillLine;
  int category;
};

class CGCraftInfo {
 public:
  static const CraftInfo *GetCraftInfo(unsigned int index) {
    return index < m_filteredSkills ? m_skills[index] : 0;
  }

 private:
  static unsigned int                 m_filteredSkills;
  static TSGrowableArray<CraftInfo *> m_skills;
};

int Trade_C_GetProposedEnchantment(unsigned int player, int &spellID, int &slot);

unsigned __int64 Script_GetGUIDFromName(const char *name);
CGUnit_C        *Script_GetUnitFromName(const char *name);
int SpellParserParseText(const SpellRec *spell, char *buf, unsigned int size, int isPet);
int Spell_C_GetSpellCooldown(
    int spell,
    int isPet,
    unsigned int *duration,
    unsigned long *startTime,
    unsigned int *enable
);
int Spell_C_GetItemCooldown(
    int itemID,
    unsigned int *duration,
    unsigned long *startTime,
    unsigned int *enable
);

int CGTooltip_SetPadding(lua_State *L);
int CGTooltip_IsOwned(lua_State *L);
int CGTooltip_SetOwner(lua_State *L);
int CGTooltip_ClearLines(lua_State *L);
int CGTooltip_AddLine(lua_State *L);
int CGTooltip_SetText(lua_State *L);
int CGTooltip_AppendText(lua_State *L);
int CGTooltip_FadeOut(lua_State *L);
int CGTooltip_SetHyperlink(lua_State *L);
int CGTooltip_SetAction(lua_State *L);
int CGTooltip_SetPlayerBuff(lua_State *L);
int CGTooltip_SetSpell(lua_State *L);
int CGTooltip_SetInventoryItem(lua_State *L);
int CGTooltip_SetLootItem(lua_State *L);
int CGTooltip_SetQuestItem(lua_State *L);
int CGTooltip_SetQuestLogItem(lua_State *L);
int CGTooltip_SetTrainerService(lua_State *L);
int CGTooltip_SetTradeSkillItem(lua_State *L);
int CGTooltip_SetCraftItem(lua_State *L);
int CGTooltip_SetCraftSpell(lua_State *L);
int CGTooltip_SetMerchantItem(lua_State *L);
int CGTooltip_SetTradePlayerItem(lua_State *L);
int CGTooltip_SetTradeTargetItem(lua_State *L);
int CGTooltip_SetBagItem(lua_State *L);
int CGTooltip_SetUnit(lua_State *L);
int CGTooltip_NumLines(lua_State *L);

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

TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> CGTooltip::s_scriptMethods;
unsigned int                                         CGTooltip::m_spellID;
static int s_nameOnly;
static int s_showComparison;
static int s_itemsWaiting;

static void FrameScriptGetSpellString(TOOLTIP_DETAIL detail, const char* stringLabel, int points, char* positive, unsigned int positiveSize, char* negative, unsigned int negativeSize) {
  char token[64];
  if (detail == TOOLTIP_DETAIL_GENERIC) {
    SStrPrintf(token, sizeof(token), "%s_GEN", stringLabel);
  } else if (detail == TOOLTIP_DETAIL_NORMAL) {
    SStrPrintf(token, sizeof(token), "%s", stringLabel);
  } else if (detail == TOOLTIP_DETAIL_VERBOSE) {
    SStrPrintf(token, sizeof(token), "%s_VERBOSE", stringLabel);
  }

  if (positive) {
    const char *text =
        FrameScript_GetText(token, points, GENDER_NOT_APPLICABLE);
    SStrCopy(positive, text, positiveSize);
    if (!*text) {
      SStrCopy(positive, token, positiveSize);
    }
  }
  if (negative) {
    SStrPack(token, "_NEG", sizeof(token));
    const char *text =
        FrameScript_GetText(token, points, GENDER_NOT_APPLICABLE);
    SStrCopy(negative, text, negativeSize);
    if (!*text) {
      *negative = 0;
    }
  }
}

static void FrameScriptGetEnchantString(TOOLTIP_DETAIL detail, const char* stringLabel, char* buf, unsigned int bufSize) {
  char token[64];
  if (detail == TOOLTIP_DETAIL_GENERIC) {
    SStrPrintf(token, sizeof(token), "%s_GEN", stringLabel);
  } else if (detail == TOOLTIP_DETAIL_NORMAL) {
    SStrPrintf(token, sizeof(token), "%s", stringLabel);
  } else if (detail == TOOLTIP_DETAIL_VERBOSE) {
    SStrPrintf(token, sizeof(token), "%s_VERBOSE", stringLabel);
  }
  const char *text =
      FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE);
  SStrCopy(buf, text, bufSize);
  if (!*text) {
    SStrCopy(buf, token, bufSize);
  }
}

static const SpellEffectNamesRec* GetEffectNameRec(int enumID) {
  for (int i = g_spellEffectNamesDB.GetNumRecords() - 1; i >= 0; --i) {
    const SpellEffectNamesRec *record =
        g_spellEffectNamesDB.GetRecordByIndex(i);
    if (record->m_EnumID == enumID) {
      return record;
    }
  }
  return 0;
}

static const SpellAuraNamesRec* GetAuraNameRec(int enumID) {
  for (int i = g_spellAuraNamesDB.GetNumRecords() - 1; i >= 0; --i) {
    const SpellAuraNamesRec *record =
        g_spellAuraNamesDB.GetRecordByIndex(i);
    if (record->m_EnumID == enumID) {
      return record;
    }
  }
  return 0;
}

static int HealthUpdateHandler(unsigned __int64 guid, unsigned int, unsigned int, const void*, void* param) {
  CSimpleStatusBar *statusBar = static_cast<CSimpleStatusBar *>(param);
  FATALASSERT(statusBar);
  CGUnit_C *unit = static_cast<CGUnit_C *>(
      ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (unit) {
    statusBar->SetMinMaxValues(
        0.0f,
        static_cast<float>(unit->GetUnitData()->maxHealth));
    statusBar->SetValue(static_cast<float>(unit->GetUnitData()->health));
  }
  return 1;
}

static void TooltipObjectLockItemStatsCallback(int id, const unsigned __int64&, void* arg, unsigned char granted) {
  if (granted) {
    CGTooltip *tooltip = static_cast<CGTooltip *>(arg);
    FATALASSERT(tooltip);
    tooltip->SetObject(tooltip->GetObjectGUID());
  }
}

static void TooltipItemStatsCallback(int id, const unsigned __int64&, void* arg, bool granted) {
  if (granted) {
    CGTooltip *tooltip = static_cast<CGTooltip *>(arg);
    FATALASSERT(tooltip);
    const unsigned __int64 noGUID = 0;
    tooltip->SetItem(
        tooltip->GetItem(),
        noGUID,
        tooltip->GetItemGUID(),
        s_nameOnly,
        s_showComparison,
        0);
  }
}

static void TooltipItemPetitionCallback(int id, const unsigned __int64&, void* arg, unsigned char granted) {
  if (granted) {
    CGTooltip *tooltip = static_cast<CGTooltip *>(arg);
    FATALASSERT(tooltip);
    const unsigned __int64 noGUID = 0;
    tooltip->SetItem(
        tooltip->GetItem(),
        noGUID,
        tooltip->GetItemGUID(),
        s_nameOnly,
        s_showComparison,
        0);
  }
}

static void TooltipSpellItemStatsCallback(int id, const unsigned __int64& guid, void* arg, unsigned char granted) {
  if (granted && arg && *static_cast<int *>(arg) == id) {
    if (!s_itemsWaiting || !--s_itemsWaiting) {
      CGTooltip *tooltip = CGGameUI::GetGameTooltip();
      FATALASSERT(tooltip);
      tooltip->SetSpell(*static_cast<int *>(arg), 0, 0, 0);
    }
  }
}

static void TooltipSpellCreatureStatsCallback(int id, const unsigned __int64& guid, void* arg, unsigned char granted) {
  if (granted && arg && *static_cast<int *>(arg) == id) {
    if (!s_itemsWaiting || !--s_itemsWaiting) {
      CGTooltip *tooltip = CGGameUI::GetGameTooltip();
      FATALASSERT(tooltip);
      tooltip->SetSpell(*static_cast<int *>(arg), 0, 0, 0);
    }
  }
}

static void TooltipSpellGameObjectStatsCallback(int id, const unsigned __int64& guid, void* arg, unsigned char granted) {
  if (granted && arg && *static_cast<int *>(arg) == id) {
    if (!s_itemsWaiting || !--s_itemsWaiting) {
      CGTooltip *tooltip = CGGameUI::GetGameTooltip();
      FATALASSERT(tooltip);
      tooltip->SetSpell(*static_cast<int *>(arg), 0, 0, 0);
    }
  }
}

static void TooltipItemCreatorCallback(int id, const unsigned __int64&, void* arg, unsigned char granted) {
  if (granted && arg) {
    CGTooltip *tooltip = CGGameUI::GetGameTooltip();
    FATALASSERT(tooltip);
    const unsigned __int64 noGUID = 0;
    tooltip->SetItem(
        id,
        noGUID,
        *static_cast<const unsigned __int64 *>(arg),
        s_nameOnly,
        s_showComparison,
        0);
  }
}

CGTooltip::CGTooltip(CSimpleFrame *parent) : CSimpleFrame(parent) {
  m_owner = 0;
  m_anchorPoint = TOOLTIP_ANCHOR_LEFT;
  m_lines = 0;
  m_linesMax = 0;
  m_reposition = 0;
  m_statusBar = 0;
  m_itemID = 0;
  m_itemGUID = 0;
  m_objectGUID = 0;
  m_corpseGUID = 0;
  m_unit = 0;
  m_debugUnit = 0;
  m_fading = 0;
  m_fadeTime = 0.0f;
  m_padding = 0.0f;
}

CGTooltip::~CGTooltip() {
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
    SStrPrintf(nameL, sizeof(nameL), "%sTextLeft%d", m_frameName, 1);
    SStrPrintf(nameR, sizeof(nameR), "%sTextRight%d", m_frameName, 1);

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
  m_statusBar = reinterpret_cast<CSimpleStatusBar *>(SimpleFrameRegistryGetEntry(buf, 0));
}

int CGTooltip::SetUnit(const unsigned __int64 &unit) {
  if (unit == m_unit) {
    return 0;
  }

  if (m_unit && m_statusBar) {
    ClntObjMgrUnsetObjMirrorHandler(
        m_unit,
        CGUnit_C::OffsetOf(ID_UNIT) + 64,
        HealthUpdateHandler,
        m_statusBar
    );
  }

  m_unit = unit;
  CGUnit_C *unitPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(unit, __FILE__, __LINE__));
  if (!unitPtr) {
    if (m_statusBar) {
      m_statusBar->Hide();
    }
    return 0;
  }

  if (m_statusBar) {
    ClntObjMgrSetObjMirrorHandler(
        m_unit,
        CGUnit_C::OffsetOf(ID_UNIT) + 64,
        4,
        HealthUpdateHandler,
        m_statusBar,
        HANDLER_PRIORITY_NORMAL
    );
    m_statusBar->SetMinMaxValues(
        0.0f,
        static_cast<float>(unitPtr->GetUnitData()->maxHealth)
    );
    m_statusBar->SetValue(static_cast<float>(unitPtr->GetUnitData()->health));
    m_statusBar->Show();
  }

  ClearLines();
  static const NTempest::CImVector color(0xFFFFFFFFUL);
  AddLine(unitPtr->GetUnitName(), color, 0);
  Show();
  return 0;
}

void CGTooltip::SetObject(const unsigned __int64 &object) {
  CGGameObject_C *gameObject =
      static_cast<CGGameObject_C *>(ClntObjMgrObjectPtr(object, __FILE__, __LINE__));
  if (!gameObject) {
    return;
  }

  m_objectGUID = object;
  ClearLines();
  static const NTempest::CImVector color(0xFFFFFFFFUL);
  AddLine(gameObject->GetName(), color, 0);
  Show();
}

const char *CGTooltip::GetItemQualityColorString(unsigned int quality) {
  static const char *colors[] = {"|cff9d9d9d", "|cffffc600", "|cff1eff00", "|cff0070dd", "|cffa335ee", "|cffff0000", "|cffff1e38"};

  return quality < 7 ? colors[quality] : "";
}

static void TooltipCorpseNameCallback(int id, const unsigned __int64&, void* arg, unsigned char granted) {
  if (granted && arg) {
    CGTooltip *tooltip = CGGameUI::GetGameTooltip();
    FATALASSERT(tooltip);
    tooltip->SetCorpse(*static_cast<const unsigned __int64 *>(arg));
  }
}

void CGTooltip::SetCorpse(const unsigned __int64 &corpseGUID) {
  if (!ClntObjMgrObjectPtr(corpseGUID, __FILE__, __LINE__)) {
    return;
  }
  ClearLines();
  m_corpseGUID = corpseGUID;
  AddLine(FrameScript_GetText("CORPSE", -1, GENDER_NOT_APPLICABLE), 0, 0);
}

int CGTooltip::SetItem(
    int                      itemID,
    const unsigned __int64  &refGUID,
    const unsigned __int64  &itemGUID,
    int                      nameOnly,
    int                      showComparison,
    TooltipExtendedItemInfo *info
) {
  const ItemStats_C *stats = g_itemDBCache.GetRecord(itemID, refGUID, 0, 0);
  if (!stats) {
    return 0;
  }
  ClearLines();
  m_itemID = itemID;
  m_itemGUID = itemGUID;
  unsigned int        quality = stats->m_overallQualityID;
  NTempest::CImVector color(0xFFFFFFFFUL);
  if (quality == 0)
    color.Set(0xFF9D9D9DUL);
  else if (quality == 2)
    color.Set(0xFF1EFF00UL);
  else if (quality == 3)
    color.Set(0xFF0070DDUL);
  else if (quality == 4)
    color.Set(0xFFA335EEUL);
  AddLine(stats->m_displayName[0] ? stats->m_displayName[0] : "", color, 0);
  if (!nameOnly && stats->m_description && *stats->m_description) {
    AddLine(stats->m_description, color, 1);
  }
  Show();
  return 1;
}

int CGTooltip::SetSpell(int spellID, int nameOnly, unsigned int cooldownTime, int isPet) {
  const SpellRec *spell = g_spellDB.GetRecord(spellID);
  if (!spell) {
    return 0;
  }
  ClearLines();
  static const NTempest::CImVector color(0xFFFFFFFFUL);
  AddLine(spell->m_name_lang[CURRENT_LANGUAGE] ? spell->m_name_lang[CURRENT_LANGUAGE] : "", color, 0);
  if (!nameOnly && spell->m_description_lang[CURRENT_LANGUAGE] && *spell->m_description_lang[CURRENT_LANGUAGE]) {
    char description[1024];
    SpellParserParseText(spell, description, sizeof(description), isPet);
    AddLine(description, 0, 1);
  }
  Show();
  return 1;
}

void CGTooltip::SetBuff(int spellID, unsigned char flags) {
  const SpellRec *spell = g_spellDB.GetRecord(spellID);
  if (!spell) {
    return;
  }

  ClearLines();
  AddLine(spell->m_name_lang[CURRENT_LANGUAGE], 0, 0);

  static const NTempest::CImVector normalColor(0xFFFFFFFFUL);
  static const NTempest::CImVector inactiveColor(0xFF808080UL);
  for (unsigned int effectIndex = 0; effectIndex < 3; ++effectIndex) {
    if (!spell->m_effect[effectIndex]) {
      continue;
    }

    char buf[128];
    GetSpellEffectString(buf, sizeof(buf), spell, effectIndex, 0, 0, TOOLTIP_DETAIL_GENERIC);
    if (*buf) {
      const NTempest::CImVector &color =
          flags & (1 << (3 - effectIndex)) ? normalColor : inactiveColor;
      AddLine(buf, color, 0);
    }
  }
}

void CGTooltip::SetOwner(CLayoutFrame *owner, TOOLTIP_ANCHORPOINT anchorpoint, float yoffset) {
  g_itemDBCache.CancelCallback(m_itemID, TooltipItemStatsCallback, this);
  const unsigned __int64 noUnit = 0;
  SetUnit(noUnit);

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
          SetPoint(
              FRAMEPOINT_BOTTOMRIGHT,
              CGGameUI::m_UISimpleParent,
              FRAMEPOINT_BOTTOMRIGHT,
              -0.01f,
              0.05f,
              1
          );
          break;
        case TOOLTIP_ANCHOR_CURSOR:
          SetPoint(
              FRAMEPOINT_BOTTOM,
              CGGameUI::m_UISimpleParent,
              FRAMEPOINT_BOTTOMLEFT,
              0.0f,
              0.0f,
              1
          );
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
  const unsigned __int64 noUnit = 0;
  SetUnit(noUnit);

  if (m_owner) {
    ClearAllPoints(1);
    SetPoint(FRAMEPOINT_BOTTOMLEFT, owner, FRAMEPOINT_BOTTOMLEFT, x, y + 0.007f, 1);
    ClearLines();
    m_reposition = 1;
  }

  SetAlpha(255);
  m_fading = 0;
  m_owner = owner;
}

void CGTooltip::GetSpellEffectString(
    char           *buf,
    unsigned int    bufSize,
    const SpellRec *spell,
    unsigned int    effectIndex,
    unsigned int    level,
    int             isPet,
    TOOLTIP_DETAIL  detail
) {
  if (!buf || !bufSize) {
    return;
  }
  buf[0] = 0;
  if (!spell || effectIndex >= 3 || !spell->m_effect[effectIndex]) {
    return;
  }
  int points = spell->m_effectBasePoints[effectIndex] + 1;
  SStrPrintf(buf, bufSize, "%d", points);
}

void CGTooltip::GetAuraEffectString(
    char           *buf,
    unsigned int    bufSize,
    const SpellRec *spell,
    unsigned int    effectIndex,
    unsigned int    level,
    int             isPet,
    TOOLTIP_DETAIL  detail
) {
  GetSpellEffectString(buf, bufSize, spell, effectIndex, level, isPet, detail);
}

void CGTooltip::AddLine(
    const char                *leftText,
    const char                *rightText,
    const NTempest::CImVector &leftColor,
    const NTempest::CImVector &rightColor,
    int                        wrapped
) {
  if (rightText && *rightText) {
    wrapped = 0;
  }

  if (m_lines >= m_linesMax ||
      !((leftText && *leftText) || (rightText && *rightText))) {
    return;
  }

  CSimpleFontString *left = m_leftStrings[m_lines];
  CSimpleFontString *right = m_rightStrings[m_lines];
  if (leftText && *leftText) {
    left->SetVertexColor(leftColor);
    left->SetText(leftText);
    left->Show();
  }
  if (rightText && *rightText) {
    right->SetVertexColor(rightColor);
    right->SetText(rightText);
    right->Show();
  }
  m_wrapLine[m_lines] = wrapped;
  ++m_lines;
}

void CGTooltip::AddLine(const char *leftText, const char *rightText, int wrapped) {
  static const NTempest::CImVector color(0xFFFFFFFFUL);
  AddLine(leftText, rightText, color, color, wrapped);
}

void CGTooltip::AddLine(const char *text, const NTempest::CImVector &color, int wrapped) {
  AddLine(text, 0, color, color, wrapped);
}

void CGTooltip::GetItemEnchantString(
    char                          *buf,
    unsigned int                   bufSize,
    const SpellItemEnchantmentRec *enchant,
    unsigned int                   effectIndex,
    TOOLTIP_DETAIL                 detail
) {
  if (!buf || !bufSize) {
    return;
  }
  buf[0] = 0;
  if (!enchant || effectIndex >= 3 || !enchant->m_effect[effectIndex]) {
    return;
  }
  if (enchant->m_name_lang[0]) {
    SStrCopy(buf, enchant->m_name_lang[0], bufSize);
  }
}

void CGTooltip::GetSpellTargetString(char *buf, unsigned int bufSize, const SpellRec *spell, unsigned int effectIndex) {
  if (!buf || !bufSize) {
    return;
  }
  buf[0] = 0;
  if (!spell || effectIndex >= 3) {
    return;
  }
  int target = spell->m_implicitTargetA[effectIndex];
  if (target) {
    SStrPrintf(buf, bufSize, "%d", target);
  }
}

void CGTooltip::GetSummonedByString(const CGUnit_C *unitPtr, char *string, unsigned int size) {
  if (!string || !size) {
    return;
  }
  string[0] = 0;
  if (unitPtr && unitPtr->GetUnitName()) {
    SStrPrintf(string, size, FrameScript_GetText("UNITNAME_SUMMONED_BY", -1, GENDER_NOT_APPLICABLE), unitPtr->GetUnitName());
  }
}

void CGTooltip::SetPosition(float x, float y) {
  if (m_owner == CGGameUI::m_UISimpleParent) {
    ClearAllPoints(1);
    SetPoint(FRAMEPOINT_BOTTOM, m_owner, FRAMEPOINT_BOTTOMLEFT, x, y, 1);
  }
}

void CGTooltip::ClearLines() {
  unsigned int i;
  for (i = 0; i < m_lines; ++i) {
    m_leftStrings[i]->SetWidth(0.0f);
    m_leftStrings[i]->Hide();
    m_rightStrings[i]->Hide();
    m_wrapLine[i] = 0;
  }
  m_lines = 0;
  FrameScript_SignalEvent(322);
}

void CGTooltip::AppendText(const char *text) {
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
  static const float COLUMN_SPACING = 0.03f;
  static const float WORDWRAP_MIN_WIDTH = 0.18f;

  float frameWidth = 0.0f;
  unsigned int i;

  for (i = 0; i < m_lines; ++i) {
    if (m_wrapLine[i]) {
      continue;
    }

    CSimpleFontString *left = m_leftStrings[i];
    CSimpleFontString *right = m_rightStrings[i];
    float lineWidth = 0.0f;
    if (left && left->IsVisible()) {
      lineWidth += left->GetWidth();
    }
    if (right && right->IsVisible()) {
      if (left && left->IsVisible()) {
        lineWidth += COLUMN_SPACING;
      }
      lineWidth += right->GetWidth();
    }
    if (lineWidth > frameWidth) {
      frameWidth = lineWidth;
    }
  }

  for (i = 0; i < m_lines; ++i) {
    if (!m_wrapLine[i]) {
      continue;
    }

    CSimpleFontString *left = m_leftStrings[i];
    if (!left) {
      continue;
    }

    float maxWidth = left->GetStringWidth();
    if (maxWidth >= WORDWRAP_MIN_WIDTH) {
      maxWidth = WORDWRAP_MIN_WIDTH;
    }

    if (maxWidth > frameWidth) {
      const char *text = left->GetText();
      unsigned int offsets[10];
      unsigned int lines = left->WrapText(text, maxWidth, offsets, 10);
      maxWidth = 0.0f;
      for (unsigned int line = 0; line < lines; ++line) {
        unsigned int end = line >= lines - 1 ? SStrLen(text) : offsets[line + 1];
        float width = left->GetTextWidth(text + offsets[line], end - offsets[line]);
        if (width > maxWidth) {
          maxWidth = width;
        }
      }
    }

    if (maxWidth > frameWidth) {
      frameWidth = maxWidth;
    }
    left->SetWidth(frameWidth);
  }

  for (i = 0; i < m_lines; ++i) {
    CSimpleFontString *right = m_rightStrings[i];
    if (right && right->IsVisible()) {
      CSimpleFontString *left = m_leftStrings[i];
      right->SetPoint(
          FRAMEPOINT_RIGHT,
          left,
          FRAMEPOINT_LEFT,
          frameWidth,
          0.0f,
          1
      );
    }
  }

  float frameHeight = 0.0f;
  for (i = 0; i < m_lines; ++i) {
    CSimpleFontString *left = m_leftStrings[i];
    if (left && left->IsVisible()) {
      if (frameHeight != 0.0f) {
        frameHeight += 0.002f;
      }
      frameHeight += left->GetHeight();
    }
  }

  float width = frameWidth + m_padding + 0.016f;
  float height = frameHeight + 0.016f;
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
      SetPoint(FRAMEPOINT_BOTTOMLEFT, m_owner, FRAMEPOINT_TOPLEFT, Left(), -height - 0.007f, 1);
    }
  } else if (Top() > 0.6f) {
    float left = Left();
    ClearAllPoints(1);
    SetPoint(
        FRAMEPOINT_TOPLEFT,
        CGGameUI::m_UISimpleParent,
        FRAMEPOINT_TOPLEFT,
        left,
        0.0f,
        1
    );
  }

  Resize(1);
}

int CGTooltip::HideThis() {
  return CSimpleFrame::HideThis();
}

int CGTooltip::ShowThis() {
  if (m_owner && m_lines) {
    CSimpleFrame::SetAlpha(255);
    m_fading = 0;
    CalculateSize();
    return CSimpleFrame::ShowThis();
  }

  m_shown = 0;
  HideThis();
  return 0;
}

void CGTooltip::FadeOut() {
  m_fading = 1;
  m_fadeTime = 1.0f;
}

void CGTooltip::OnLayerUpdate(float elapsedSec) {
  CSimpleFrame::OnLayerUpdate(elapsedSec);

  if (m_anchorPoint == TOOLTIP_ANCHOR_CURSOR) {
    NTempest::C2Vector mousePos;
    NDCToDDC(m_top->m_mousePosition.x, m_top->m_mousePosition.y, &mousePos.x, &mousePos.y);
    mousePos.x /= m_layoutScale;
    mousePos.y /= m_layoutScale;
    SetPoint(
        FRAMEPOINT_BOTTOM, CGGameUI::m_UISimpleParent ? static_cast<CLayoutFrame *>(CGGameUI::m_UISimpleParent) : 0, FRAMEPOINT_BOTTOMLEFT,
        mousePos.x, mousePos.y, 1
    );
  }

  if (m_fading) {
    m_fadeTime -= elapsedSec;
    if (m_fadeTime <= 0.0f) {
      CSimpleFrame::SetAlpha(255);
      m_shown = 0;
      HideThis();
    } else {
      float alpha = m_fadeTime < 1.0f ? m_fadeTime : 1.0f;
      CSimpleFrame::SetAlpha(static_cast<unsigned char>(alpha * 255.0f));
    }
  }
}

#define GET_TOOLTIP_THIS(L, tooltip)                            \
  CGTooltip *tooltip = 0;                                       \
  if (lua_type(L, 1) == LUA_TTABLE) {                           \
    lua_rawgeti(L, 1, 0);                                       \
    tooltip = static_cast<CGTooltip *>(lua_touserdata(L, -1));  \
    lua_pop(L, 1);                                              \
  } else {                                                      \
    luaL_error(                                                 \
        L,                                                      \
        "Attempt to find 'this' in non-table object (used '.' " \
        "instead of ':' ?)"                                     \
    );                                                          \
  }                                                             \
  ASSERT(tooltip)

int CGTooltip_SetPadding(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  tooltip->SetTooltipPadding(static_cast<float>(lua_tonumber(L, 2)) * 0.0009765625f * 0.8f);
  return 0;
}

int CGTooltip_IsOwned(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  lua_pushnumber(L, 0.0);
  lua_gettable(L, 2);
  CSimpleFrame *frame =
      static_cast<CSimpleFrame *>(lua_touserdata(L, -1));
  lua_pop(L, 1);
  ASSERT(frame);
  if (tooltip->GetOwner() == frame) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

int CGTooltip_SetOwner(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  tooltip->Hide();

  lua_pushnumber(L, 0.0);
  lua_gettable(L, 2);
  CSimpleFrame *owner =
      static_cast<CSimpleFrame *>(lua_touserdata(L, -1));
  lua_pop(L, 1);
  ASSERT(owner);

  TOOLTIP_ANCHORPOINT anchor = TOOLTIP_ANCHOR_LEFT;
  if (lua_isstring(L, 3)) {
    const char *name = lua_tostring(L, 3);
    if (!SStrCmpI(name, "ANCHOR_RIGHT", 0x7FFFFFFF))
      anchor = static_cast<TOOLTIP_ANCHORPOINT>(1);
    else if (!SStrCmpI(name, "ANCHOR_BOTTOMRIGHT", 0x7FFFFFFF))
      anchor = static_cast<TOOLTIP_ANCHORPOINT>(3);
    else if (!SStrCmpI(name, "ANCHOR_BOTTOMLEFT", 0x7FFFFFFF))
      anchor = static_cast<TOOLTIP_ANCHORPOINT>(2);
    else if (!SStrCmpI(name, "ANCHOR_FIXED", 0x7FFFFFFF))
      anchor = static_cast<TOOLTIP_ANCHORPOINT>(4);
    else if (!SStrCmpI(name, "ANCHOR_CURSOR", 0x7FFFFFFF))
      anchor = TOOLTIP_ANCHOR_CURSOR;
    else if (!SStrCmpI(name, "ANCHOR_NONE", 0x7FFFFFFF))
      anchor = static_cast<TOOLTIP_ANCHORPOINT>(6);
  }
  float yOffset = lua_isnumber(L, 4) ? static_cast<float>(lua_tonumber(L, 4)) : 0.0f;
  tooltip->SetOwner(owner, anchor, yOffset);
  return 0;
}

int CGTooltip_ClearLines(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  tooltip->ClearLines();
  return 0;
}

int CGTooltip_AddLine(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  const char *leftText = 0;
  const char *rightText = 0;
  int argument = 2;

  if (lua_isstring(L, argument)) {
    leftText = lua_tostring(L, argument++);
  }
  if (lua_isstring(L, argument)) {
    rightText = lua_tostring(L, argument++);
  }

  static const NTempest::CImVector defaultColor(0xFFFFFFFFUL);
  NTempest::CImVector leftColor(defaultColor);
  NTempest::CImVector rightColor(defaultColor);
  if (lua_isnumber(L, argument)) {
    float red = static_cast<float>(lua_tonumber(L, argument));
    float green = static_cast<float>(lua_tonumber(L, argument + 1));
    float blue = static_cast<float>(lua_tonumber(L, argument + 2));
    float alpha = 1.0f;
    argument += 3;
    if (lua_isnumber(L, argument)) {
      alpha = static_cast<float>(lua_tonumber(L, argument++));
    }
    leftColor.Set(alpha, red, green, blue);
  }
  if (lua_isnumber(L, argument)) {
    float red = static_cast<float>(lua_tonumber(L, argument));
    float green = static_cast<float>(lua_tonumber(L, argument + 1));
    float blue = static_cast<float>(lua_tonumber(L, argument + 2));
    float alpha = lua_isnumber(L, argument + 3)
        ? static_cast<float>(lua_tonumber(L, argument + 3))
        : 1.0f;
    rightColor.Set(alpha, red, green, blue);
  }

  tooltip->AddLine(leftText, rightText, leftColor, rightColor, 0);
  return 0;
}

int CGTooltip_SetText(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  if (!lua_isstring(L, 2)) {
    return luaL_error(L, "Usage: SetText(\"text\" [, color])");
  }
  static const NTempest::CImVector defaultColor(0xFFFFFFFFUL);
  NTempest::CImVector color(defaultColor);
  if (lua_isnumber(L, 3)) {
    float red = static_cast<float>(lua_tonumber(L, 3));
    float green = static_cast<float>(lua_tonumber(L, 4));
    float blue = static_cast<float>(lua_tonumber(L, 5));
    float alpha =
        lua_isnumber(L, 6) ? static_cast<float>(lua_tonumber(L, 6)) : 1.0f;
    color.Set(alpha, red, green, blue);
  }
  tooltip->ClearLines();
  tooltip->AddLine(lua_tostring(L, 2), color, 0);
  tooltip->Show();
  return 0;
}

int CGTooltip_AppendText(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  if (!lua_isstring(L, 2)) {
    return luaL_error(L, "Usage: AppendText(\"text\")");
  }
  tooltip->AppendText(lua_tostring(L, 2));
  return 0;
}

int CGTooltip_FadeOut(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  tooltip->FadeOut();
  return 0;
}

int CGTooltip_SetHyperlink(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  if (!lua_isstring(L, 2)) {
    return luaL_error(L, "Usage: SetHyperlink(link)");
  }
  const char *link = lua_tostring(L, 2);
  if (SStrCmpI(link, "item", 4) || link[4] != ':') {
    return luaL_error(L, "Unknown link type");
  }
  int itemID = SStrToInt(link + 5);
  if (itemID <= 0) {
    return luaL_error(L, "Unknown link type");
  }
  unsigned __int64 none = 0;
  tooltip->SetItem(itemID, none, none, 0, 1, 0);
  return 0;
}

int CGTooltip_SetAction(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  if (!lua_isnumber(L, 2)) {
    return luaL_error(L, "Usage: SetAction(slot)");
  }

  int slot = static_cast<int>(lua_tonumber(L, 2)) - 1;
  unsigned long startTime = 0;
  unsigned int duration = 0;
  unsigned int enable = 0;
  CGActionBar::GetCooldown(slot, startTime, duration, enable);
  tooltip->ClearLines();

  static const NTempest::CImVector defaultColor(0xFFFFFFFFUL);
  if (CGActionBar::IsAttackAction(slot)) {
    char text[32];
    SStrCopy(
        text,
        FrameScript_GetText("ATTACK", -1, GENDER_NOT_APPLICABLE),
        sizeof(text)
    );
    tooltip->AddLine(text, defaultColor, 0);
    tooltip->Show();
    lua_pushnil(L);
    return 1;
  }

  if (CGActionBar::IsSpell(slot)) {
    if (!startTime || !duration) {
      tooltip->SetSpell(CGActionBar::GetSpell(slot), 1, 0, 0);
      lua_pushnil(L);
      return 1;
    }

    unsigned int cooldownTime = startTime + duration - OsGetAsyncTimeMs();
    if (tooltip->SetSpell(
            CGActionBar::GetSpell(slot),
            1,
            cooldownTime,
            0))
    {
      lua_pushnumber(L, 1.0);
      return 1;
    }
  } else if (CGActionBar::IsItem(slot)) {
    CGObject_C *player = ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__);
    CGBag_C    *bag = player ? player->GetBag() : 0;
    CGItem_C   *item = bag ? bag->FindItemOfType(CGActionBar::GetItem(slot), 0) : 0;
    if (!item) {
      char text[32];
      SStrCopy(
          text,
          FrameScript_GetText("USE_ITEM", -1, GENDER_NOT_APPLICABLE),
          sizeof(text)
      );
      tooltip->AddLine(text, defaultColor, 0);
      tooltip->Show();
      lua_pushnil(L);
      return 1;
    }

    unsigned __int64 itemGUID = item->GetGUID();
    if (!startTime || !duration) {
      tooltip->SetItem(
          item->GetEntryID(),
          itemGUID,
          itemGUID,
          0,
          0,
          0
      );
      lua_pushnil(L);
      return 1;
    }

    TooltipExtendedItemInfo info = {0};
    info.cooldownTime = startTime + duration - OsGetAsyncTimeMs();
    info.creator = item->GetCreator();
    if (tooltip->SetItem(
            item->GetEntryID(),
            itemGUID,
            itemGUID,
            0,
            0,
            &info))
    {
      lua_pushnumber(L, 1.0);
      return 1;
    }
  }

  lua_pushnil(L);
  return 1;
}

int CGTooltip_SetPlayerBuff(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  if (!lua_isnumber(L, 2)) {
    return luaL_error(L, "Usage: SetPlayerBuff(buffIndex)");
  }
  int         index = static_cast<int>(lua_tonumber(L, 2));
  const CGBuffDesc *buff = CGBuffBar::GetBuffByIndex(index);
  tooltip->SetBuff(buff->GetAuraSpell(), buff->GetAuraFlags());
  if (!buff->GetUntilCancelled()) {
    unsigned int duration = CGBuffBar::GetBuffTimeLeftByIndex(index);
    char format[64];
    SStrCopy(format, FrameScript_GetText(
        duration < 60000 ? "SPELL_TIME_REMAINING_SEC" : "SPELL_TIME_REMAINING_MIN",
        -1,
        GENDER_NOT_APPLICABLE), sizeof(format));
    unsigned int remaining =
        duration < 60000
            ? duration / 1000
            : static_cast<unsigned int>(
                  static_cast<double>(duration) * 0.000016666667 + 0.99000001);
    char time[32];
    SStrPrintf(time, sizeof(time), format, remaining);
    tooltip->AddLine(time, 0, 0);
  }
  tooltip->Show();
  return 0;
}

int CGTooltip_SetSpell(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  if (!lua_isnumber(L, 2) || !lua_isstring(L, 3)) {
    return luaL_error(L, "Invalid spell slot in SetSpell");
  }
  unsigned int slot = static_cast<unsigned int>(lua_tonumber(L, 2) - 1.0);
  if (slot >= 1024) {
    return luaL_error(L, "Invalid spell slot in SetSpell");
  }
  const char *type = lua_tostring(L, 3);
  int         spellID;
  int         isPet = 0;
  if (!SStrCmpI(type, "spell", 0x7FFFFFFF)) {
    spellID = CGSpellBook::GetSpell(slot, PLAYER_SPELL);
  } else if (!SStrCmpI(type, "ability", 0x7FFFFFFF)) {
    spellID = CGSpellBook::GetSpell(slot, PLAYER_ABILITY);
  } else if (!SStrCmpI(type, "pet", 0x7FFFFFFF)) {
    spellID = CGSpellBook::GetSpell(slot, PET_SPELL);
    isPet = 1;
  } else {
    spellID = CGSpellBook::GetSpell(slot, PLAYER_SPELL);
  }

  unsigned int cooldownTime = 0;
  if (spellID > 0) {
    unsigned int duration = 0;
    unsigned long startTime = 0;
    unsigned int enable = 0;
    Spell_C_GetSpellCooldown(
        spellID,
        isPet,
        &duration,
        &startTime,
        &enable
    );
    if (startTime && duration) {
      cooldownTime = startTime + duration - OsGetAsyncTimeMs();
    }
  }

  if (spellID > 0 &&
      tooltip->SetSpell(spellID, 0, cooldownTime, isPet)) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

int CGTooltip_SetInventoryItem(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  if (!lua_isstring(L, 2) || !lua_isnumber(L, 3)) {
    return luaL_error(L, "Usage: SetInventoryItem(\"unit\", slot)");
  }

  unsigned int slot =
      static_cast<unsigned int>(lua_tonumber(L, 3) - 1.0);
  if (slot > 22 && (slot < 39 || slot > 68)) {
    return luaL_error(L, "Invalid inventory slot in SetInventoryItem");
  }

  int              nameOnly = lua_isnumber(L, 4) && lua_tonumber(L, 4) > 0.0;
  CGUnit_C        *unit = Script_GetUnitFromName(lua_tostring(L, 2));
  CGBag_C         *bag = unit ? unit->GetBag() : 0;
  unsigned __int64 itemGUID = bag ? bag->GetItem(slot) : 0;
  CGItem_C        *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(itemGUID, __FILE__, __LINE__));
  if (item) {
    unsigned int duration = 0;
    unsigned long startTime = 0;
    unsigned int enable = 0;
    Spell_C_GetItemCooldown(
        item->GetEntryID(),
        &duration,
        &startTime,
        &enable
    );

    unsigned __int64 unitGUID = unit->GetGUID();
    int hasCooldown = 0;
    if (startTime && duration) {
      TooltipExtendedItemInfo info = {0};
      info.cooldownTime = startTime + duration - OsGetAsyncTimeMs();
      info.creator = item->GetCreator();
      hasCooldown = tooltip->SetItem(
          item->GetEntryID(),
          unitGUID,
          itemGUID,
          nameOnly,
          0,
          &info
      );
    } else {
      tooltip->SetItem(
          item->GetEntryID(),
          unitGUID,
          itemGUID,
          nameOnly,
          0,
          0
      );
    }

    lua_pushnumber(L, 1.0);
    if (hasCooldown) {
      lua_pushnumber(L, 1.0);
    } else {
      lua_pushnil(L);
    }
  } else {
    lua_pushnil(L);
    lua_pushnil(L);
  }
  return 2;
}

int CGTooltip_SetLootItem(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  if (!lua_isnumber(L, 2)) {
    return luaL_error(L, "Invalid loot slot in SetInventoryItem");
  }
  int itemID = CGLootInfo::GetLootItem(
      static_cast<unsigned int>(lua_tonumber(L, 2) - 1.0)
  );
  if (itemID <= 0) {
    return luaL_error(L, "Invalid loot slot in SetInventoryItem");
  }
  unsigned __int64 none = 0;
  tooltip->SetItem(itemID, CGLootInfo::GetObject(), none, 0, 1, 0);
  return 0;
}

int CGTooltip_SetQuestItem(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  if (!lua_isstring(L, 2) || !lua_isnumber(L, 3)) {
    return luaL_error(
        L,
        "Invalid quest item in SetQuestItem(\"type\", index)"
    );
  }
  int itemID = CGQuestInfo::GetQuestItemID(lua_tostring(L, 2), static_cast<unsigned int>(lua_tonumber(L, 3) - 1.0));
  if (itemID <= 0) {
    return luaL_error(
        L,
        "Invalid quest item in SetQuestItem(\"type\", index)"
    );
  }
  unsigned __int64 none = 0;
  tooltip->SetItem(
      itemID,
      CGQuestInfo::GetQuestGiver(),
      none,
      0,
      1,
      0
  );
  return 0;
}

int CGTooltip_SetQuestLogItem(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  if (!lua_isstring(L, 2) || !lua_isnumber(L, 3)) {
    return luaL_error(
        L,
        "Invalid quest item in SetQuestLogItem(\"type\", index)"
    );
  }
  int itemID = CGQuestLog::GetQuestItemID(lua_tostring(L, 2), static_cast<int>(lua_tonumber(L, 3)) - 1);
  if (itemID <= 0) {
    return luaL_error(
        L,
        "Invalid quest item in SetQuestLogItem(\"type\", index)"
    );
  }
  unsigned __int64 none = 0;
  tooltip->SetItem(itemID, none, none, 0, 1, 0);
  return 0;
}

int CGTooltip_SetTrainerService(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  if (!lua_isnumber(L, 2)) {
    return luaL_error(
        L,
        "Invalid trainer service in SetTrainerService(index)"
    );
  }
  unsigned int        index = static_cast<unsigned int>(lua_tonumber(L, 2) - 1.0);
  const TrainerServiceInfo *service = CGClassTrainer::GetService(index);
  const SpellRec *spell = service ? g_spellDB.GetRecord(service->spellID) : 0;
  if (!service || !spell) {
    return luaL_error(
        L,
        "Invalid trainer service in SetTrainerService(index)"
    );
  }

  int triggerSpellID = 0;
  unsigned int effect;
  for (effect = 0; effect < 3; ++effect) {
    if ((spell->m_effect[effect] == 36 || spell->m_effect[effect] == 57) &&
        spell->m_effectTriggerSpell[effect])
    {
      triggerSpellID = spell->m_effectTriggerSpell[effect];
      break;
    }
  }

  const SpellRec *triggerSpell =
      triggerSpellID ? g_spellDB.GetRecord(triggerSpellID) : 0;
  if (triggerSpell) {
    if ((triggerSpell->m_attributes & 0x20) &&
        triggerSpell->m_effect[0] == 24)
    {
      unsigned __int64 refGUID =
          static_cast<unsigned int>(triggerSpellID) |
          0xB000000000000000ui64;
      const unsigned __int64 noGUID = 0;
      tooltip->SetItem(
          triggerSpell->m_effectItemType[effect],
          refGUID,
          noGUID,
          0,
          1,
          0
      );
    } else {
      tooltip->SetSpell(triggerSpellID, 0, 0, 0);
    }
  } else {
    tooltip->SetSpell(service->spellID, 0, 0, 0);
  }
  return 0;
}

int CGTooltip_SetTradeSkillItem(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  if (!lua_isnumber(L, 2)) {
    return luaL_error(
        L,
        "Invalid trade skill item in SetTradeSkillItem(index [,reagent])"
    );
  }
  unsigned int    index = static_cast<unsigned int>(lua_tonumber(L, 2) - 1.0);
  const TradeSkillInfo *info = CGTradeSkillInfo::GetTradeSkillInfo(index);
  if (!info) {
    return luaL_error(
        L,
        "Invalid trade skill item in SetTradeSkillItem(index [,reagent])"
    );
  }
  const SpellRec *spell = g_spellDB.GetRecord(info->spellID);
  if (!spell) {
    return luaL_error(
        L,
        "Invalid trade skill item in SetTradeSkillItem(index [,reagent])"
    );
  }
  int itemID = spell->m_effectItemType[0];
  if (lua_isnumber(L, 3)) {
    unsigned int reagent = static_cast<unsigned int>(lua_tonumber(L, 3));
    unsigned int found = 0;
    itemID = 0;
    for (unsigned int i = 0; i < 8; ++i) {
      if (spell->m_reagent[i] && ++found == reagent) {
        itemID = spell->m_reagent[i];
        break;
      }
    }
    if (!itemID) {
      return luaL_error(
          L,
          "Invalid trade skill item in SetTradeSkillItem(index [,reagent])"
      );
    }
  }
  unsigned __int64 ref = static_cast<unsigned int>(info->spellID) | 0xB000000000000000ui64;
  unsigned __int64 none = 0;
  tooltip->SetItem(itemID, ref, none, 0, 1, 0);
  return 0;
}

int CGTooltip_SetCraftItem(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  if (!lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
    return luaL_error(
        L,
        "Invalid craft item in SetCraftItem(index, reagent)"
    );
  }
  unsigned int index = static_cast<unsigned int>(lua_tonumber(L, 2) - 1.0);
  const CraftInfo *info = CGCraftInfo::GetCraftInfo(index);
  if (!info) {
    return luaL_error(
        L,
        "Invalid craft item in SetCraftItem(index, reagent)"
    );
  }
  const SpellRec *spell = g_spellDB.GetRecord(info->spellID);
  unsigned int    reagent = static_cast<unsigned int>(lua_tonumber(L, 3));
  int             itemID = 0;
  unsigned int    found = 0;
  if (spell) {
    for (unsigned int i = 0; i < 8; ++i) {
      if (spell->m_reagent[i] && ++found == reagent) {
        itemID = spell->m_reagent[i];
        break;
      }
    }
  }
  if (!itemID) {
    return luaL_error(
        L,
        "Invalid craft item in SetCraftItem(index, reagent)"
    );
  }
  unsigned __int64 ref = static_cast<unsigned int>(info->spellID) | 0xB000000000000000ui64;
  unsigned __int64 none = 0;
  tooltip->SetItem(itemID, ref, none, 0, 1, 0);
  return 0;
}

int CGTooltip_SetCraftSpell(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  if (!lua_isnumber(L, 2)) {
    return luaL_error(L, "Invalid craft in SetCraftSpell(index)");
  }
  unsigned int index = static_cast<unsigned int>(lua_tonumber(L, 2) - 1.0);
  const CraftInfo *info = CGCraftInfo::GetCraftInfo(index);
  if (!info) {
    return luaL_error(L, "Invalid craft in SetCraftSpell(index)");
  }
  tooltip->SetSpell(info->spellID, 0, 0, 0);
  return 0;
}

int CGTooltip_SetMerchantItem(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  if (!lua_isnumber(L, 2)) {
    return luaL_error(L, "Invalid merchant slot in SetMerchantItem");
  }
  int              index = static_cast<int>(lua_tonumber(L, 2) - 1.0);
  const VendorItem *item = CGMerchantInfo::GetItem(index);
  unsigned __int64 merchant = CGMerchantInfo::GetMerchant();
  if (!merchant || !item || !item->m_itemType) {
    return 0;
  }
  unsigned __int64 none = 0;
  tooltip->SetItem(item->m_itemType, merchant, none, 0, 1, 0);
  return 0;
}

int CGTooltip_SetTradePlayerItem(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  if (!lua_isnumber(L, 2)) {
    return luaL_error(L, "Invalid trade slot in SetTradePlayerItem");
  }
  unsigned int index = static_cast<unsigned int>(lua_tonumber(L, 2) - 1.0);
  if (index >= 8) {
    return 0;
  }
  unsigned __int64 playerItemGUID;
  unsigned __int64 bagGUID;
  unsigned char    bagSlot;
  CGTradeInfo::GetPlayerItemInfo(index, playerItemGUID, bagGUID, bagSlot);
  CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(playerItemGUID, __FILE__, __LINE__));
  if (!item) {
    return 0;
  }
  TooltipExtendedItemInfo info = {0};
  int                     proposedEnchantment = 0;
  int                     proposedEnchantmentSlot = 0;
  info.creator = item->GetCreator();
  if (Trade_C_GetProposedEnchantment(
          1,
          proposedEnchantment,
          proposedEnchantmentSlot) &&
      proposedEnchantmentSlot == static_cast<int>(index))
  {
    info.proposedEnchantment = proposedEnchantment;
  }
  playerItemGUID = item->GetGUID();
  tooltip->SetItem(item->GetEntryID(), playerItemGUID, playerItemGUID, 0, 1, &info);
  return 0;
}

int CGTooltip_SetTradeTargetItem(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  if (!lua_isnumber(L, 2)) {
    return luaL_error(L, "Invalid trade slot in SetTradeTargetItem");
  }
  unsigned int index = static_cast<unsigned int>(lua_tonumber(L, 2) - 1.0);
  int          targetItem = CGTradeInfo::GetTargetTradeItem(index);
  if (targetItem <= 0) {
    return 0;
  }
  TooltipExtendedItemInfo info = {0};
  info.enchantment[0] = CGTradeInfo::GetTargetTradeItemEnachantment(index);
  info.creator = CGTradeInfo::GetTargetTradeItemCreator(index);
  int proposedEnchantment = 0;
  int proposedEnchantmentSlot = 0;
  if (Trade_C_GetProposedEnchantment(
          0,
          proposedEnchantment,
          proposedEnchantmentSlot) &&
      proposedEnchantmentSlot == static_cast<int>(index))
  {
    info.proposedEnchantment = proposedEnchantment;
  }
  unsigned __int64 none = 0;
  tooltip->SetItem(
      targetItem,
      CGTradeInfo::GetTradePartner(),
      none,
      0,
      1,
      &info
  );
  return 0;
}

int CGTooltip_SetBagItem(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
    return luaL_error(L, "Invalid bag slot in SetBagItem");
  }
  unsigned int bagIndex = static_cast<unsigned int>(lua_tonumber(L, 2) - 1.0);
  CGBag_C     *bag = 0;
  if (bagIndex == static_cast<unsigned int>(-1)) {
    bag = player->GetBag();
  } else if (bagIndex < 10) {
    unsigned __int64 bagGUID = CGContainerInfo::GetContainer(bagIndex);
    CGObject_C      *bagObject = ClntObjMgrObjectPtr(bagGUID, __FILE__, __LINE__);
    bag = bagObject ? bagObject->GetBag() : 0;
  }
  if (!bag) {
    return luaL_error(L, "Invalid bag slot in SetBagItem");
  }
  int slot = static_cast<int>(lua_tonumber(L, 3)) - 1;
  if (bag == player->GetBag()) {
    slot += 23;
  }
  unsigned __int64 itemGUID = slot >= 0 ? bag->GetItem(slot) : 0;
  CGItem_C        *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(itemGUID, __FILE__, __LINE__));
  if (item) {
    unsigned int duration = 0;
    unsigned long startTime = 0;
    unsigned int enable = 0;
    Spell_C_GetItemCooldown(
        item->GetEntryID(),
        &duration,
        &startTime,
        &enable
    );

    int result;
    if (startTime && duration) {
      TooltipExtendedItemInfo info = {0};
      info.cooldownTime = startTime + duration - OsGetAsyncTimeMs();
      info.creator = item->GetCreator();
      result = tooltip->SetItem(
          item->GetEntryID(),
          itemGUID,
          itemGUID,
          0,
          1,
          &info
      );
    } else {
      result = tooltip->SetItem(
          item->GetEntryID(),
          itemGUID,
          itemGUID,
          0,
          1,
          0
      );
    }
    if (result) {
      lua_pushnumber(L, 1.0);
      return 1;
    }
  }
  lua_pushnil(L);
  return 1;
}

int CGTooltip_SetUnit(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  if (!lua_isstring(L, 2)) {
    return luaL_error(L, "Usage: SetUnit(\"unit\")");
  }
  unsigned __int64 target = Script_GetGUIDFromName(lua_tostring(L, 2));
  if (target && tooltip->SetUnit(target)) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

int CGTooltip_NumLines(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  lua_pushnumber(L, static_cast<double>(tooltip->NumLines()));
  return 1;
}

void CGTooltip::RegisterScriptMethods() {
  FrameScript_Object::FillScriptMethodTable(CGTooltipMethods, 26, s_scriptMethods);
}

void CGTooltip::UnregisterScriptMethods() {
  FrameScript_Object::EmptyScriptMethodTable(s_scriptMethods);
}

#undef GET_TOOLTIP_THIS

int CGTooltip::LookupScriptMethod(lua_State *L, const char *name) {
  if (FrameScript_Object::LookupScriptMethod(L, name, s_scriptMethods)) {
    return 1;
  }
  return CSimpleFrame::LookupScriptMethod(L, name);
}
