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
  int            enchantment[5];
  unsigned int   enchantmentExpiration[5];
  unsigned int   cooldownTime;
  int            proposedEnchantment;
  unsigned __int64 creator;
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
  static const char *GetItemQualityColorString(unsigned int quality);
  static void GetSpellEffectString(
      char           *buf,
      unsigned int    bufSize,
      const SpellRec *spell,
      unsigned int    effectIndex,
      unsigned int    level,
      int             isPet,
      TOOLTIP_DETAIL  detail
  );
  static void GetAuraEffectString(
      char           *buf,
      unsigned int    bufSize,
      const SpellRec *spell,
      unsigned int    effectIndex,
      unsigned int    level,
      int             isPet,
      TOOLTIP_DETAIL  detail
  );
  static void
  GetItemEnchantString(char *buf, unsigned int bufSize, const SpellItemEnchantmentRec *enchant, unsigned int effectIndex, TOOLTIP_DETAIL detail);
  static void GetSpellTargetString(char *buf, unsigned int bufSize, const SpellRec *spell, unsigned int effectIndex);
  static void GetSummonedByString(const CGUnit_C *unitPtr, char *string, unsigned int size);
  static void RegisterScriptMethods();
  static void UnregisterScriptMethods();

  CLayoutFrame *GetOwner() {
    return m_owner;
  }
  void SetOwner(CLayoutFrame *owner, TOOLTIP_ANCHORPOINT anchorpoint, float yoffset);
  void SetOwner(CLayoutFrame *owner, float x, float y);
  void SetPosition(float x, float y);
  void ClearLines();
  void AddLine(const char *leftText, const char *rightText, const NTempest::CImVector &leftColor, const NTempest::CImVector &rightColor, int wrapped);
  void AddLine(const char *leftText, const char *rightText, int wrapped);
  void AddLine(const char *text, const NTempest::CImVector &color, int wrapped);
  unsigned int NumLines() {
    return m_lines;
  }
  void AppendText(const char *text);
  void SetTooltipPadding(float right);
  void CalculateSize();
  int  SetUnit(const unsigned __int64 &unit);
  void SetObject(const unsigned __int64 &object);
  int  SetItem(
      int                      itemID,
      const unsigned __int64  &refGUID,
      const unsigned __int64  &itemGUID,
      int                      nameOnly,
      int                      showComparison,
      TooltipExtendedItemInfo *info
  );
  int                     SetSpell(int spellID, int nameOnly, unsigned int cooldownTime, int isPet);
  void                    SetBuff(int spellID, unsigned char flags);
  void                    SetCorpse(const unsigned __int64 &corpseGUID);
  const unsigned __int64 &GetObjectGUID() const {
    return m_objectGUID;
  }
  int GetItem() const {
    return m_itemID;
  }
  const unsigned __int64 &GetItemGUID() const {
    return m_itemGUID;
  }
  void SetDebugUnit(const unsigned __int64 &unit) {
    m_debugUnit = unit;
  }
  const unsigned __int64 &GetDebugUnit() const {
    return m_debugUnit;
  }
  unsigned __int64 GetUnit() {
    return m_unit;
  }
  void FadeOut();

  virtual void PostLoadXML(const XMLNode *node, CStatus *status);
  virtual void OnLayerUpdate(float elapsedSec);

 protected:
  CGTooltip(CSimpleFrame *parent);
  virtual ~CGTooltip();

  virtual int LookupScriptMethod(lua_State *L, const char *name);
  virtual int HideThis();
  virtual int ShowThis();

  static TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> s_scriptMethods;

 private:
  static unsigned int m_spellID;

  CLayoutFrame                     *m_owner;
  TOOLTIP_ANCHORPOINT               m_anchorPoint;
  unsigned int                      m_lines;
  unsigned int                      m_linesMax;
  int                               m_reposition;
  TSFixedArray<CSimpleFontString *> m_leftStrings;
  TSFixedArray<CSimpleFontString *> m_rightStrings;
  TSFixedArray<int>                 m_wrapLine;
  CSimpleStatusBar                 *m_statusBar;
  unsigned __int64                  m_unit;
  unsigned __int64                  m_objectGUID;
  unsigned __int64                  m_debugUnit;
  unsigned __int64                  m_itemGUID;
  unsigned __int64                  m_corpseGUID;
  unsigned int                      m_itemID;
  int                               m_fading;
  float                             m_fadeTime;
  float                             m_padding;
};

inline CSimpleFrame *CGTooltip::Create(CSimpleFrame *parent) {
  return NEW(CGTooltip)(parent);
}

#endif
