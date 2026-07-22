#ifndef ENGINE_SOURCE_FRAME_CSIMPLESCROLLFRAME_H
#define ENGINE_SOURCE_FRAME_CSIMPLESCROLLFRAME_H

#include "Frame/CSimpleFrame.h"
#include "Tempest/c2vector.h"

class CSimpleScrollFrame : public CSimpleFrame {
 public:
  CSimpleScrollFrame(CSimpleFrame *parent);
  virtual ~CSimpleScrollFrame();

  virtual void LoadXML(const XMLNode *node, CStatus *status);
  virtual void LoadXML_Scripts(const XMLNode *node, CStatus *status);

  void SetHorizontalScroll(float offset);
  void SetVerticalScroll(float offset);

  float GetHorizontalScroll() {
    return m_scrollOffset.x;
  }

  float GetVerticalScroll() {
    return m_scrollOffset.y;
  }

  float GetHorizontalScrollRange() {
    return m_scrollRange.x;
  }

  float GetVerticalScrollRange() {
    return m_scrollRange.y;
  }

  void UpdateScrollChildRect() {
    m_updateScrollChild = 1;
  }

  static void __fastcall RegisterScriptMethods();
  static void __fastcall UnregisterScriptMethods();

  void SetOnHorizontalScrollScript(const char *source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnHorizontalScroll", GetName());
    SetEventScript(m_onHorizontalScroll, source, description);
  }

  void SetOnVerticalScrollScript(const char *source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnVerticalScroll", GetName());
    SetEventScript(m_onVerticalScroll, source, description);
  }

  void SetOnScrollRangeChangedScript(const char *source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnScrollRangeChanged", GetName());
    SetEventScript(m_onScrollRangeChanged, source, description);
  }

 protected:
  virtual int LookupScriptMethod(lua_State *L, const char *name);

  void SetScrollChild(CSimpleFrame *frame);
  void UpdateScrollChild();

  static TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> s_scriptMethods;

  int                m_updateScrollChild;
  CSimpleFrame      *m_scrollChild;
  NTempest::C2Vector m_scrollRange;
  NTempest::C2Vector m_scrollOffset;
  int                m_onHorizontalScroll;
  int                m_onVerticalScroll;
  int                m_onScrollRangeChanged;
};

#endif
