#include "Tooltip.h"

#include "DB/DBClient/AutoCode/SpellRec.h"
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
#include "Ui/GameUI.h"
#include "Ui/QuestLog.h"
#include "Ui/SpellBookFrame.h"

#include <Base/Coordinate.h>
#include <Frame/CSimpleRender.h>
#include <Frame/CSimpleStatusBar.h>
#include <Frame/CSimpleTop.h>
#include <Frame/SimpleFrameRegistry.h>
#include <FrameScript/FrameScript.h>
#include <lauxlib.h>
#include <lua.h>
#include <storm.h>

class CGBuffDesc {
 public:
  int GetAuraSpell() const {
    return m_auraSpell;
  }
  unsigned int GetAuraFlags() const {
    return m_auraFlags;
  }
  int GetUntilCancelled() const {
    return m_untilCancelled;
  }

 protected:
  int          m_auraIndex;
  int          m_auraSpell;
  unsigned int m_auraFlags;
  int          m_untilCancelled;
};

class CGBuffBar {
 public:
  static CGBuffDesc *GetBuffByIndex(int buffIndex) {
    return buffIndex >= 0 && buffIndex < 56 && m_buffs[buffIndex].GetAuraSpell() > 0 ? &m_buffs[buffIndex] : 0;
  }
  static unsigned int GetBuffTimeLeftByIndex(int buffIndex) {
    return buffIndex >= 0 && buffIndex < 56 ? m_durations[buffIndex] : 0;
  }

 private:
  static CGBuffDesc   m_buffs[56];
  static unsigned int m_durations[56];
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
  static TradeSkillInfo *GetTradeSkillInfo(unsigned int index) {
    return index < m_numSkills ? m_skills[index] : 0;
  }

 private:
  static unsigned int                      m_numSkills;
  static TSGrowableArray<TradeSkillInfo *> m_skills;
};

struct CraftInfo {
  int spellID;
  int skillLine;
  int category;
};

class CGCraftInfo {
 public:
  static CraftInfo *GetCraftInfo(unsigned int index) {
    return index < m_filteredSkills ? m_skills[index] : 0;
  }

 private:
  static unsigned int                 m_filteredSkills;
  static TSGrowableArray<CraftInfo *> m_skills;
};

struct VendorItem {
  unsigned int m_muid;
  unsigned int m_itemType;
  unsigned int m_itemDisplayID;
  int          m_quantity;
  int          m_price;
  int          m_durability;
  int          m_stackCount;
};

class CGMerchantInfo {
 public:
  static unsigned __int64 __fastcall GetMerchant();
  static VendorItem                 *GetItem(int index) {
    return index >= 0 && index < m_itemCount ? &m_items[index] : 0;
  }

 protected:
  static unsigned __int64 m_merchant;
  static VendorItem       m_items[128];
  static int              m_itemCount;
};

class CGTradeInfo {
 public:
  static void __fastcall GetPlayerItemInfo(int index, unsigned __int64 &guid, unsigned __int64 &bag, unsigned int &slot);
  static int             GetTargetTradeItem(int index) {
    return index >= 0 && index < 8 ? m_targetItems[index] : 0;
  }
  static int GetTargetTradeItemEnachantment(int index) {
    return index >= 0 && index < 8 ? m_targetItemEnchantment[index] : 0;
  }
  static unsigned __int64 GetTargetTradeItemCreator(int index) {
    return index >= 0 && index < 8 ? m_targetItemCreator[index] : 0;
  }

 protected:
  static unsigned __int64 m_playerItems[8];
  static int              m_targetItems[8];
  static int              m_targetItemEnchantment[8];
  static unsigned __int64 m_targetItemCreator[8];
};

int __fastcall Trade_C_GetProposedEnchantment(unsigned int player, int &spellID, int &slot);

unsigned __int64 __fastcall Script_GetGUIDFromName(const char *name);

class CGQuestInfo {
 public:
  static int __fastcall GetQuestItemID(const char *type, unsigned int index);
};

int __fastcall CGTooltip_SetPadding(lua_State *L);
int __fastcall CGTooltip_IsOwned(lua_State *L);
int __fastcall CGTooltip_SetOwner(lua_State *L);
int __fastcall CGTooltip_ClearLines(lua_State *L);
int __fastcall CGTooltip_AddLine(lua_State *L);
int __fastcall CGTooltip_SetText(lua_State *L);
int __fastcall CGTooltip_AppendText(lua_State *L);
int __fastcall CGTooltip_FadeOut(lua_State *L);
int __fastcall CGTooltip_SetHyperlink(lua_State *L);
int __fastcall CGTooltip_SetAction(lua_State *L);
int __fastcall CGTooltip_SetPlayerBuff(lua_State *L);
int __fastcall CGTooltip_SetSpell(lua_State *L);
int __fastcall CGTooltip_SetInventoryItem(lua_State *L);
int __fastcall CGTooltip_SetLootItem(lua_State *L);
int __fastcall CGTooltip_SetQuestItem(lua_State *L);
int __fastcall CGTooltip_SetQuestLogItem(lua_State *L);
int __fastcall CGTooltip_SetTrainerService(lua_State *L);
int __fastcall CGTooltip_SetTradeSkillItem(lua_State *L);
int __fastcall CGTooltip_SetCraftItem(lua_State *L);
int __fastcall CGTooltip_SetCraftSpell(lua_State *L);
int __fastcall CGTooltip_SetMerchantItem(lua_State *L);
int __fastcall CGTooltip_SetTradePlayerItem(lua_State *L);
int __fastcall CGTooltip_SetTradeTargetItem(lua_State *L);
int __fastcall CGTooltip_SetBagItem(lua_State *L);
int __fastcall CGTooltip_SetUnit(lua_State *L);
int __fastcall CGTooltip_NumLines(lua_State *L);

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

void __fastcall CGTooltip::RegisterScriptMethods() {
  FrameScript_Object::FillScriptMethodTable(CGTooltipMethods, 26, s_scriptMethods);
}

void __fastcall CGTooltip::UnregisterScriptMethods() {
  FrameScript_Object::EmptyScriptMethodTable(s_scriptMethods);
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

const char *__fastcall CGTooltip::GetItemQualityColorString(unsigned int quality) {
  static const char *colors[] = {"|cff9d9d9d", "|cffffc600", "|cff1eff00", "|cff0070dd", "|cffa335ee", "|cffff0000", "|cffff1e38"};

  return quality < 7 ? colors[quality] : "";
}

void __fastcall CGTooltip::GetSpellEffectString(
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

void __fastcall CGTooltip::GetAuraEffectString(
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

void __fastcall CGTooltip::GetItemEnchantString(
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

void __fastcall CGTooltip::GetSpellTargetString(char *buf, unsigned int bufSize, const SpellRec *spell, unsigned int effectIndex) {
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

void __fastcall CGTooltip::GetSummonedByString(const CGUnit_C *unitPtr, char *string, unsigned int size) {
  if (!string || !size) {
    return;
  }
  string[0] = 0;
  if (unitPtr && unitPtr->GetUnitName()) {
    SStrPrintf(string, size, FrameScript_GetText("UNITNAME_SUMMONED_BY", -1, GENDER_NOT_APPLICABLE), unitPtr->GetUnitName());
  }
}

void CGTooltip::SetOwner(CLayoutFrame *owner, TOOLTIP_ANCHORPOINT anchorpoint, float yoffset) {
  m_owner = owner;
  m_anchorPoint = anchorpoint;
  ClearAllPoints(1);

  if (!owner || anchorpoint == TOOLTIP_ANCHOR_CURSOR || anchorpoint == static_cast<TOOLTIP_ANCHORPOINT>(6)) {
    return;
  }

  FRAMEPOINT point = FRAMEPOINT_LEFT;
  FRAMEPOINT relativePoint = FRAMEPOINT_RIGHT;
  float      xOffset = 0.0f;
  if (anchorpoint == static_cast<TOOLTIP_ANCHORPOINT>(1)) {
    point = FRAMEPOINT_RIGHT;
    relativePoint = FRAMEPOINT_LEFT;
  } else if (anchorpoint == static_cast<TOOLTIP_ANCHORPOINT>(2)) {
    point = FRAMEPOINT_TOPLEFT;
    relativePoint = FRAMEPOINT_BOTTOMLEFT;
  } else if (anchorpoint == static_cast<TOOLTIP_ANCHORPOINT>(3)) {
    point = FRAMEPOINT_TOPRIGHT;
    relativePoint = FRAMEPOINT_BOTTOMRIGHT;
  } else if (anchorpoint == static_cast<TOOLTIP_ANCHORPOINT>(4)) {
    point = FRAMEPOINT_BOTTOMLEFT;
    relativePoint = FRAMEPOINT_TOPLEFT;
  }
  SetPoint(point, owner, relativePoint, xOffset, yoffset, 1);
}

void CGTooltip::SetOwner(CLayoutFrame *owner, float x, float y) {
  m_owner = owner;
  m_anchorPoint = static_cast<TOOLTIP_ANCHORPOINT>(4);
  SetPosition(x, y);
}

void CGTooltip::SetPosition(float x, float y) {
  ClearAllPoints(1);
  SetPoint(FRAMEPOINT_BOTTOMLEFT, CGGameUI::m_UISimpleParent, FRAMEPOINT_BOTTOMLEFT, x, y, 1);
}

void CGTooltip::ClearLines() {
  for (unsigned int i = 0; i < m_linesMax; ++i) {
    if (m_leftStrings[i]) {
      m_leftStrings[i]->SetText(0);
      m_leftStrings[i]->Hide();
    }
    if (m_rightStrings[i]) {
      m_rightStrings[i]->SetText(0);
      m_rightStrings[i]->Hide();
    }
    m_wrapLine[i] = 0;
  }
  if (m_statusBar) {
    m_statusBar->Hide();
  }
  m_lines = 0;
  m_unit = 0;
  m_objectGUID = 0;
  m_itemGUID = 0;
  m_corpseGUID = 0;
  m_itemID = 0;
}

void CGTooltip::AddLine(
    const char                *leftText,
    const char                *rightText,
    const NTempest::CImVector &leftColor,
    const NTempest::CImVector &rightColor,
    int                        wrapped
) {
  if (m_lines >= m_linesMax) {
    return;
  }
  CSimpleFontString *left = m_leftStrings[m_lines];
  CSimpleFontString *right = m_rightStrings[m_lines];
  if (left) {
    left->SetText(leftText ? leftText : "");
    left->SetVertexColor(leftColor);
    left->Show();
  }
  if (right) {
    right->SetText(rightText ? rightText : "");
    right->SetVertexColor(rightColor);
    if (rightText && *rightText) {
      right->Show();
    } else {
      right->Hide();
    }
  }
  m_wrapLine[m_lines] = wrapped;
  ++m_lines;
  m_reposition = 1;
}

void CGTooltip::AddLine(const char *leftText, const char *rightText, int wrapped) {
  static const NTempest::CImVector color(0xFFFFFFFFUL);
  AddLine(leftText, rightText, color, color, wrapped);
}

void CGTooltip::AddLine(const char *text, const NTempest::CImVector &color, int wrapped) {
  AddLine(text, 0, color, color, wrapped);
}

void CGTooltip::AppendText(const char *text) {
  if (!m_lines || !text || !m_leftStrings[m_lines - 1]) {
    return;
  }
  char buf[256];
  SStrPrintf(buf, sizeof(buf), "%s%s", m_leftStrings[m_lines - 1]->GetText() ? m_leftStrings[m_lines - 1]->GetText() : "", text);
  m_leftStrings[m_lines - 1]->SetText(buf);
  m_reposition = 1;
}

void CGTooltip::SetTooltipPadding(float right) {
  m_padding = right;
}

void CGTooltip::CalculateSize() {
  float width = 0.0f;
  float height = 0.0f;
  for (unsigned int i = 0; i < m_lines; ++i) {
    float lineWidth = 0.0f;
    float lineHeight = 0.0f;
    if (m_leftStrings[i]) {
      lineWidth += m_leftStrings[i]->GetStringWidth();
      lineHeight = m_leftStrings[i]->GetStringHeight();
    }
    if (m_rightStrings[i] && m_rightStrings[i]->GetText() && *m_rightStrings[i]->GetText()) {
      lineWidth += 16.0f + m_rightStrings[i]->GetStringWidth();
      float rightHeight = m_rightStrings[i]->GetStringHeight();
      if (rightHeight > lineHeight) {
        lineHeight = rightHeight;
      }
    }
    if (lineWidth > width) {
      width = lineWidth;
    }
    height += lineHeight + 2.0f;
  }
  SetWidth(width + m_padding + 20.0f);
  SetHeight(height + 20.0f);
  m_reposition = 0;
}

int CGTooltip::SetUnit(const unsigned __int64 &unit) {
  CGUnit_C *unitPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(unit, __FILE__, __LINE__));
  if (!unitPtr) {
    return 0;
  }
  ClearLines();
  m_unit = unit;
  static const NTempest::CImVector color(0xFFFFFFFFUL);
  AddLine(unitPtr->GetUnitName(), color, 0);
  Show();
  return 1;
}

void CGTooltip::SetObject(const unsigned __int64 &object) {
  if (!ClntObjMgrObjectPtr(object, __FILE__, __LINE__)) {
    return;
  }
  ClearLines();
  m_objectGUID = object;
  char text[64];
  SStrPrintf(text, sizeof(text), "Object %08X", static_cast<unsigned int>(object));
  AddLine(text, 0, 0);
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
  AddLine(spell->m_name_lang[0] ? spell->m_name_lang[0] : "", color, 0);
  if (!nameOnly && spell->m_description_lang[0] && *spell->m_description_lang[0]) {
    AddLine(spell->m_description_lang[0], color, 1);
  }
  Show();
  return 1;
}

void CGTooltip::SetBuff(int spellID, unsigned int flags) {
  SetSpell(spellID, 0, 0, 0);
}

void CGTooltip::SetCorpse(const unsigned __int64 &corpseGUID) {
  if (!ClntObjMgrObjectPtr(corpseGUID, __FILE__, __LINE__)) {
    return;
  }
  ClearLines();
  m_corpseGUID = corpseGUID;
  AddLine(FrameScript_GetText("CORPSE", -1, GENDER_NOT_APPLICABLE), 0, 0);
}

void CGTooltip::FadeOut() {
  m_fading = 1;
  m_fadeTime = 1.0f;
}

int CGTooltip::HideThis() {
  return CSimpleFrame::HideThis();
}

int CGTooltip::ShowThis() {
  if (m_owner && m_lines) {
    CSimpleFrame::SetAlpha(255);
    m_fading = 0;
    // todo: CalculateSize subsystem
    return CSimpleFrame::ShowThis();
  }

  m_shown = 0;
  HideThis();
  return 0;
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

int __fastcall CGTooltip_SetPadding(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  tooltip->SetTooltipPadding(static_cast<float>(lua_tonumber(L, 2)) * 0.0009765625f * 0.8f);
  return 0;
}

int __fastcall CGTooltip_IsOwned(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  CSimpleFrame *frame = 0;
  if (lua_type(L, 2) == LUA_TTABLE) {
    lua_rawgeti(L, 2, 0);
    frame = static_cast<CSimpleFrame *>(lua_touserdata(L, -1));
    lua_pop(L, 1);
  }
  ASSERT(frame);
  lua_pushnumber(L, tooltip->GetOwner() == frame ? 1.0 : 0.0);
  return 1;
}

int __fastcall CGTooltip_SetOwner(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  tooltip->SetReposition(0);
  tooltip->Hide();

  CSimpleFrame *owner = 0;
  if (lua_type(L, 2) == LUA_TTABLE) {
    lua_rawgeti(L, 2, 0);
    owner = static_cast<CSimpleFrame *>(lua_touserdata(L, -1));
    lua_pop(L, 1);
  }
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

int __fastcall CGTooltip_ClearLines(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  tooltip->ClearLines();
  return 0;
}

static NTempest::CImVector TooltipColor(lua_State *L, int index, const NTempest::CImVector &fallback) {
  if (!lua_isnumber(L, index)) {
    return fallback;
  }
  float               red = static_cast<float>(lua_tonumber(L, index));
  float               green = static_cast<float>(lua_tonumber(L, index + 1));
  float               blue = static_cast<float>(lua_tonumber(L, index + 2));
  float               alpha = lua_isnumber(L, index + 3) ? static_cast<float>(lua_tonumber(L, index + 3)) : 1.0f;
  NTempest::CImVector color;
  color.Set(alpha, red, green, blue);
  return color;
}

int __fastcall CGTooltip_AddLine(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  const char                      *leftText = lua_isstring(L, 2) ? lua_tostring(L, 2) : 0;
  const char                      *rightText = lua_isstring(L, 3) ? lua_tostring(L, 3) : 0;
  static const NTempest::CImVector defaultColor(0xFFFFFFFFUL);
  int                              colorIndex = rightText ? 4 : 3;
  NTempest::CImVector              leftColor = TooltipColor(L, colorIndex, defaultColor);
  NTempest::CImVector              rightColor = TooltipColor(L, colorIndex + 4, defaultColor);
  tooltip->AddLine(leftText, rightText, leftColor, rightColor, 0);
  return 0;
}

int __fastcall CGTooltip_SetText(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  if (!lua_isstring(L, 2)) {
    return luaL_error(L, "Usage: SetText(\"text\" [, r, g, b])");
  }
  static const NTempest::CImVector defaultColor(0xFFFFFFFFUL);
  NTempest::CImVector              color = TooltipColor(L, 3, defaultColor);
  tooltip->ClearLines();
  tooltip->AddLine(lua_tostring(L, 2), color, 0);
  tooltip->SetReposition(1);
  tooltip->Show();
  return 0;
}

int __fastcall CGTooltip_AppendText(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  if (!lua_isstring(L, 2)) {
    return luaL_error(L, "Usage: AppendText(\"text\")");
  }
  tooltip->AppendText(lua_tostring(L, 2));
  return 0;
}

int __fastcall CGTooltip_FadeOut(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  tooltip->FadeOut();
  return 0;
}

int __fastcall CGTooltip_SetHyperlink(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  if (!lua_isstring(L, 2)) {
    return luaL_error(L, "Usage: SetHyperlink(\"link\")");
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

int __fastcall CGTooltip_SetAction(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  if (!lua_isnumber(L, 2)) {
    return luaL_error(L, "Usage: SetAction(slot)");
  }
  int slot = static_cast<int>(lua_tonumber(L, 2)) - 1;
  tooltip->ClearLines();
  if (static_cast<unsigned int>(slot) >= 120) {
    lua_pushnil(L);
    return 1;
  }
  int shown = 0;
  if (CGActionBar::IsSpell(slot)) {
    shown = tooltip->SetSpell(CGActionBar::GetSpell(slot), 1, 0, 0);
  } else if (CGActionBar::IsItem(slot)) {
    CGObject_C *player = ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__);
    CGBag_C    *bag = player ? player->GetBag() : 0;
    CGItem_C   *item = bag ? bag->FindItemOfType(CGActionBar::GetItem(slot), 0) : 0;
    if (item) {
      unsigned __int64 playerGUID = ClntObjMgrGetActivePlayer();
      unsigned __int64 itemGUID = item->GetGUID();
      shown = tooltip->SetItem(item->GetEntryID(), playerGUID, itemGUID, 0, 0, 0);
    }
  }
  if (shown) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

int __fastcall CGTooltip_SetPlayerBuff(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  if (!lua_isnumber(L, 2)) {
    return luaL_error(L, "Usage: SetPlayerBuff(index)");
  }
  int         index = static_cast<int>(lua_tonumber(L, 2));
  CGBuffDesc *buff = CGBuffBar::GetBuffByIndex(index);
  if (!buff) {
    return 0;
  }
  tooltip->SetBuff(buff->GetAuraSpell(), buff->GetAuraFlags());
  unsigned int duration = CGBuffBar::GetBuffTimeLeftByIndex(index);
  if (!buff->GetUntilCancelled() && duration) {
    char         time[64];
    unsigned int seconds = duration / 1000;
    SStrPrintf(time, sizeof(time), FrameScript_GetText("BUFF_TIME_REMAINING", -1, GENDER_NOT_APPLICABLE), seconds ? seconds : 1);
    tooltip->AddLine(time, 0, 0);
  }
  tooltip->SetReposition(1);
  tooltip->Show();
  return 0;
}

int __fastcall CGTooltip_SetSpell(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  if (!lua_isnumber(L, 2) || !lua_isstring(L, 3)) {
    return luaL_error(L, "Invalid spell slot");
  }
  unsigned int slot = static_cast<unsigned int>(lua_tonumber(L, 2) - 1.0);
  if (slot >= 1024) {
    return luaL_error(L, "Invalid spell slot");
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
    return luaL_error(L, "Invalid spell slot");
  }
  if (spellID > 0 && tooltip->SetSpell(spellID, 0, 0, isPet)) {
    lua_pushnumber(L, 1.0);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

int __fastcall CGTooltip_SetInventoryItem(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  if (!lua_isstring(L, 2) || !lua_isnumber(L, 3)) {
    return luaL_error(L, "Usage: SetInventoryItem(\"unit\", slot)");
  }
  unsigned int     slot = static_cast<unsigned int>(lua_tonumber(L, 3) - 1.0);
  int              nameOnly = lua_isnumber(L, 4) && lua_tonumber(L, 4) > 0.0;
  CGUnit_C        *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(Script_GetGUIDFromName(lua_tostring(L, 2)), __FILE__, __LINE__));
  CGBag_C         *bag = unit ? unit->GetBag() : 0;
  unsigned __int64 itemGUID = bag ? bag->GetItem(slot) : 0;
  CGItem_C        *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(itemGUID, __FILE__, __LINE__));
  if (item) {
    unsigned __int64 unitGUID = unit->GetGUID();
    if (tooltip->SetItem(item->GetEntryID(), unitGUID, itemGUID, nameOnly, 0, 0)) {
      lua_pushnumber(L, 1.0);
    } else {
      lua_pushnil(L);
    }
  } else {
    lua_pushnil(L);
  }
  lua_pushnil(L);
  return 2;
}

int __fastcall CGTooltip_SetLootItem(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  if (!lua_isnumber(L, 2)) {
    return luaL_error(L, "Invalid loot slot");
  }
  int itemID = CGPlayer_C::GetLootItem(static_cast<unsigned int>(lua_tonumber(L, 2) - 1.0));
  if (itemID <= 0) {
    return luaL_error(L, "Invalid loot slot");
  }
  unsigned __int64 none = 0;
  tooltip->SetItem(itemID, none, none, 0, 1, 0);
  return 0;
}

int __fastcall CGTooltip_SetQuestItem(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  if (!lua_isstring(L, 2) || !lua_isnumber(L, 3)) {
    return luaL_error(L, "Invalid quest item");
  }
  int itemID = CGQuestInfo::GetQuestItemID(lua_tostring(L, 2), static_cast<unsigned int>(lua_tonumber(L, 3) - 1.0));
  if (itemID <= 0) {
    return luaL_error(L, "Invalid quest item");
  }
  unsigned __int64 none = 0;
  tooltip->SetItem(itemID, none, none, 0, 1, 0);
  return 0;
}

int __fastcall CGTooltip_SetQuestLogItem(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  if (!lua_isstring(L, 2) || !lua_isnumber(L, 3)) {
    return luaL_error(L, "Invalid quest log item");
  }
  int itemID = CGQuestLog::GetQuestItemID(lua_tostring(L, 2), static_cast<int>(lua_tonumber(L, 3)) - 1);
  if (itemID <= 0) {
    return luaL_error(L, "Invalid quest log item");
  }
  unsigned __int64 none = 0;
  tooltip->SetItem(itemID, none, none, 0, 1, 0);
  return 0;
}

int __fastcall CGTooltip_SetTrainerService(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  if (!lua_isnumber(L, 2)) {
    return luaL_error(L, "Invalid trainer service");
  }
  unsigned int        index = static_cast<unsigned int>(lua_tonumber(L, 2) - 1.0);
  TrainerServiceInfo *service = CGClassTrainer::GetService(index);
  if (!service) {
    return luaL_error(L, "Invalid trainer service");
  }
  tooltip->SetSpell(service->spellID, 0, 0, 0);
  return 0;
}

int __fastcall CGTooltip_SetTradeSkillItem(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  if (!lua_isnumber(L, 2)) {
    return luaL_error(L, "Invalid trade skill item");
  }
  unsigned int    index = static_cast<unsigned int>(lua_tonumber(L, 2) - 1.0);
  TradeSkillInfo *info = CGTradeSkillInfo::GetTradeSkillInfo(index);
  if (!info) {
    return luaL_error(L, "Invalid trade skill item");
  }
  const SpellRec *spell = g_spellDB.GetRecord(info->spellID);
  if (!spell) {
    return luaL_error(L, "Invalid trade skill item");
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
  }
  unsigned __int64 ref = static_cast<unsigned int>(info->spellID) | 0xB000000000000000ui64;
  unsigned __int64 none = 0;
  tooltip->SetItem(itemID, ref, none, 0, 1, 0);
  return 0;
}

int __fastcall CGTooltip_SetCraftItem(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  if (!lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
    return luaL_error(L, "Invalid craft item");
  }
  unsigned int index = static_cast<unsigned int>(lua_tonumber(L, 2) - 1.0);
  CraftInfo   *info = CGCraftInfo::GetCraftInfo(index);
  if (!info) {
    return luaL_error(L, "Invalid craft item");
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
    return luaL_error(L, "Invalid craft item");
  }
  unsigned __int64 ref = static_cast<unsigned int>(info->spellID) | 0xB000000000000000ui64;
  unsigned __int64 none = 0;
  tooltip->SetItem(itemID, ref, none, 0, 1, 0);
  return 0;
}

int __fastcall CGTooltip_SetCraftSpell(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  if (!lua_isnumber(L, 2)) {
    return luaL_error(L, "Invalid craft index");
  }
  unsigned int index = static_cast<unsigned int>(lua_tonumber(L, 2) - 1.0);
  CraftInfo   *info = CGCraftInfo::GetCraftInfo(index);
  if (!info) {
    return luaL_error(L, "Invalid craft index");
  }
  tooltip->SetSpell(info->spellID, 0, 0, 0);
  return 0;
}

int __fastcall CGTooltip_SetMerchantItem(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  if (!lua_isnumber(L, 2)) {
    return luaL_error(L, "Invalid merchant item");
  }
  int              index = static_cast<int>(lua_tonumber(L, 2) - 1.0);
  VendorItem      *item = CGMerchantInfo::GetItem(index);
  unsigned __int64 merchant = CGMerchantInfo::GetMerchant();
  if (!merchant || !item || !item->m_itemType) {
    return 0;
  }
  unsigned __int64 none = 0;
  tooltip->SetItem(item->m_itemType, merchant, none, 0, 1, 0);
  return 0;
}

int __fastcall CGTooltip_SetTradePlayerItem(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  if (!lua_isnumber(L, 2)) {
    return luaL_error(L, "Invalid trade slot");
  }
  unsigned int index = static_cast<unsigned int>(lua_tonumber(L, 2) - 1.0);
  if (index >= 8) {
    return 0;
  }
  unsigned __int64 playerItemGUID;
  unsigned __int64 bagGUID;
  unsigned int     bagSlot;
  CGTradeInfo::GetPlayerItemInfo(index, playerItemGUID, bagGUID, bagSlot);
  CGItem_C *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(playerItemGUID, __FILE__, __LINE__));
  if (!item) {
    return 0;
  }
  TooltipExtendedItemInfo info;
  int                     proposedEnchantment = 0;
  int                     proposedEnchantmentSlot = 0;
  if (Trade_C_GetProposedEnchantment(0, proposedEnchantment, proposedEnchantmentSlot) && proposedEnchantmentSlot == static_cast<int>(index) &&
      static_cast<unsigned int>(proposedEnchantmentSlot) < 10)
  {
    info.enchantment[proposedEnchantmentSlot] = proposedEnchantment;
  }
  playerItemGUID = item->GetGUID();
  tooltip->SetItem(item->GetEntryID(), playerItemGUID, playerItemGUID, 0, 1, &info);
  return 0;
}

int __fastcall CGTooltip_SetTradeTargetItem(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  if (!lua_isnumber(L, 2)) {
    return luaL_error(L, "Invalid trade slot");
  }
  unsigned int index = static_cast<unsigned int>(lua_tonumber(L, 2) - 1.0);
  int          targetItem = CGTradeInfo::GetTargetTradeItem(index);
  if (targetItem <= 0) {
    return 0;
  }
  TooltipExtendedItemInfo info;
  info.enchantment[0] = CGTradeInfo::GetTargetTradeItemEnachantment(index);
  int proposedEnchantment = 0;
  int proposedEnchantmentSlot = 0;
  if (Trade_C_GetProposedEnchantment(1, proposedEnchantment, proposedEnchantmentSlot) && proposedEnchantmentSlot == static_cast<int>(index) &&
      static_cast<unsigned int>(proposedEnchantmentSlot) < 10)
  {
    info.enchantment[proposedEnchantmentSlot] = proposedEnchantment;
  }
  unsigned __int64 none = 0;
  tooltip->SetItem(targetItem, CGTradeInfo::GetTargetTradeItemCreator(index), none, 0, 1, &info);
  return 0;
}

int __fastcall CGTooltip_SetBagItem(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
    return luaL_error(L, "Invalid bag slot");
  }
  unsigned int bagIndex = static_cast<unsigned int>(lua_tonumber(L, 2) - 1.0);
  CGBag_C     *bag = 0;
  if (bagIndex == static_cast<unsigned int>(-1)) {
    bag = player->GetBag();
  } else if (bagIndex < 10) {
    unsigned __int64 bagGUID = player->GetBag()->GetItem(bagIndex);
    CGObject_C      *bagObject = ClntObjMgrObjectPtr(bagGUID, __FILE__, __LINE__);
    bag = bagObject ? bagObject->GetBag() : 0;
  }
  if (!bag) {
    return luaL_error(L, "Invalid bag slot");
  }
  int slot = static_cast<int>(lua_tonumber(L, 3)) - 1;
  if (bag == player->GetBag()) {
    slot += 23;
  }
  unsigned __int64 itemGUID = slot >= 0 ? bag->GetItem(slot) : 0;
  CGItem_C        *item = static_cast<CGItem_C *>(ClntObjMgrObjectPtr(itemGUID, __FILE__, __LINE__));
  if (item) {
    unsigned __int64 playerGUID = player->GetGUID();
    if (tooltip->SetItem(item->GetEntryID(), playerGUID, itemGUID, 0, 1, 0)) {
      lua_pushnumber(L, 1.0);
      return 1;
    }
  }
  lua_pushnil(L);
  return 1;
}

int __fastcall CGTooltip_SetUnit(lua_State *L) {
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

int __fastcall CGTooltip_NumLines(lua_State *L) {
  GET_TOOLTIP_THIS(L, tooltip);
  lua_pushnumber(L, static_cast<double>(tooltip->NumLines()));
  return 1;
}

#undef GET_TOOLTIP_THIS

int CGTooltip::LookupScriptMethod(lua_State *L, const char *name) {
  if (FrameScript_Object::LookupScriptMethod(L, name, s_scriptMethods)) {
    return 1;
  }
  return CSimpleFrame::LookupScriptMethod(L, name);
}
