#ifndef WOW_SOURCE_UIUTIL_TOOLTIP_H
#define WOW_SOURCE_UIUTIL_TOOLTIP_H

#include <Frame/CSimpleFrame.h>
#include <storm.h>
#include <string.h>

class CSimpleFontString;
class CSimpleStatusBar;
class CGUnit_C;
class SpellRec;
class SpellItemEnchantmentRec;

enum TOOLTIP_DETAIL {
  TOOLTIP_DETAIL_GENERIC = 0,
  TOOLTIP_DETAIL_NORMAL = 1,
  TOOLTIP_DETAIL_VERBOSE = 2
};

struct TooltipExtendedItemInfo {
  int       enchantment[5];
  UINT      enchantmentExpiration[5];
  UINT      cooldownTime;
  int       proposedEnchantment;
  DWORDLONG creator;
};

enum TOOLTIP_ANCHORPOINT {
  TOOLTIP_ANCHOR_LEFT = 0,
  TOOLTIP_ANCHOR_RIGHT = 1,
  TOOLTIP_ANCHOR_BOTTOMLEFT = 2,
  TOOLTIP_ANCHOR_BOTTOMRIGHT = 3,
  TOOLTIP_ANCHOR_FIXED = 4,
  TOOLTIP_ANCHOR_CURSOR = 5,
  TOOLTIP_ANCHOR_NONE = 6
};

class CGTooltip : public CSimpleFrame {
 public:
  static CSimpleFrame *Create(CSimpleFrame *parent);
  static LPCSTR        GetItemQualityColorString(UINT quality);
  static void GetSpellEffectString(char *buf, UINT bufSize, const SpellRec *spell, UINT effectIndex, UINT level, BOOL isPet, TOOLTIP_DETAIL detail);
  static void GetAuraEffectString(char *buf, UINT bufSize, const SpellRec *spell, UINT effectIndex, UINT level, BOOL isPet, TOOLTIP_DETAIL detail);
  static void GetItemEnchantString(char *buf, UINT bufSize, const SpellItemEnchantmentRec *enchant, UINT effectIndex, TOOLTIP_DETAIL detail);
  static void GetSpellTargetString(char *buf, UINT bufSize, const SpellRec *spell, UINT effectIndex);
  static void GetSummonedByString(const CGUnit_C *unitPtr, char *string, UINT size);
  static void RegisterScriptMethods();
  static void UnregisterScriptMethods();

  CLayoutFrame *GetOwner() {
    return m_owner;
  }
  void SetOwner(CLayoutFrame *owner, TOOLTIP_ANCHORPOINT anchorpoint, float yoffset);
  void SetOwner(CLayoutFrame *owner, float x, float y);
  void SetPosition(float x, float y);
  void ClearLines();
  void AddLine(LPCSTR leftText, LPCSTR rightText, const NTempest::CImVector &leftColor, const NTempest::CImVector &rightColor, int wrapped);
  void AddLine(LPCSTR leftText, LPCSTR rightText, int wrapped);
  void AddLine(LPCSTR text, const NTempest::CImVector &color, int wrapped);
  UINT NumLines() {
    return m_lines;
  }
  void AppendText(LPCSTR text);
  void SetTooltipPadding(float right);
  void CalculateSize();
  BOOL SetUnit(const DWORDLONG &unit);
  void SetObject(const DWORDLONG &object);
  BOOL SetItem(int itemID, const DWORDLONG &refGUID, const DWORDLONG &itemGUID, int nameOnly, int showComparison, TooltipExtendedItemInfo *info);
  BOOL SetSpell(int spellID, int nameOnly, UINT cooldownTime, BOOL isPet);
  void SetBuff(int spellID, BYTE flags);
  void SetCorpse(const DWORDLONG &corpseGUID);
  const DWORDLONG &GetObjectGUID() const {
    return m_objectGUID;
  }
  int GetItem() const {
    return m_itemID;
  }
  const DWORDLONG &GetItemGUID() const {
    return m_itemGUID;
  }
  void SetDebugUnit(const DWORDLONG &unit) {
    m_debugUnit = unit;
  }
  const DWORDLONG &GetDebugUnit() const {
    return m_debugUnit;
  }
  DWORDLONG GetUnit() {
    return m_unit;
  }
  void FadeOut();

  virtual void PostLoadXML(const XMLNode *node, CStatus *status);
  virtual void OnLayerUpdate(float elapsedSec);

 protected:
  CGTooltip(CSimpleFrame *parent);
  virtual ~CGTooltip();

  virtual BOOL LookupScriptMethod(lua_State *L, LPCSTR name);
  virtual BOOL HideThis();
  virtual BOOL ShowThis();

  static TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> s_scriptMethods;

 private:
  static UINT m_spellID;

  CLayoutFrame                     *m_owner;
  TOOLTIP_ANCHORPOINT               m_anchorPoint;
  UINT                              m_lines;
  UINT                              m_linesMax;
  BOOL                              m_reposition;
  TSFixedArray<CSimpleFontString *> m_leftStrings;
  TSFixedArray<CSimpleFontString *> m_rightStrings;
  TSFixedArray<int>                 m_wrapLine;
  CSimpleStatusBar                 *m_statusBar;
  DWORDLONG                         m_unit;
  DWORDLONG                         m_objectGUID;
  DWORDLONG                         m_debugUnit;
  DWORDLONG                         m_itemGUID;
  DWORDLONG                         m_corpseGUID;
  UINT                              m_itemID;
  BOOL                              m_fading;
  float                             m_fadeTime;
  float                             m_padding;
};

inline CSimpleFrame *CGTooltip::Create(CSimpleFrame *parent) {
  return NEW(CGTooltip)(parent);
}

#endif
