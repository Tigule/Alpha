#ifndef ENGINE_SOURCE_FRAME_CSIMPLESCROLLFRAME_H
#define ENGINE_SOURCE_FRAME_CSIMPLESCROLLFRAME_H

#include "Frame/CSimpleFrame.h"
#include "Tempest/c2vector.h"

class CSimpleScrollFrame : public CSimpleFrame {
 public:
  CSimpleScrollFrame(CSimpleFrame *parent = 0);
  virtual ~CSimpleScrollFrame();

  virtual void LoadXML(const XMLNode *node, CStatus *status);
  virtual void LoadXML_Scripts(const XMLNode *node, CStatus *status);
  virtual void OnLayerUpdate(float elapsedSec);
  virtual void OnFrameRender(CRenderBatch *batch, UINT layer);
  virtual void OnFrameSizeChanged(float w, float h);

  void SetHorizontalScroll(float offset);
  void SetVerticalScroll(float offset);

  float GetHorizontalScroll() const {
    return m_scrollOffset.x;
  }

  float GetVerticalScroll() const {
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

  static void RegisterScriptMethods();
  static void UnregisterScriptMethods();

  void SetScrollChild(CSimpleFrame *frame);

  void SetOnHorizontalScrollScript(LPCSTR source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnHorizontalScroll", GetName());
    SetEventScript(m_onHorizontalScroll, source, description);
  }

  void SetOnVerticalScrollScript(LPCSTR source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnVerticalScroll", GetName());
    SetEventScript(m_onVerticalScroll, source, description);
  }

  void SetOnScrollRangeChangedScript(LPCSTR source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnScrollRangeChanged", GetName());
    SetEventScript(m_onScrollRangeChanged, source, description);
  }

  void RunOnHorizontalScrollScript() {
    if (m_onHorizontalScroll) {
      FrameScript_Execute(m_onHorizontalScroll, this, "%f", m_scrollOffset.x * 1024.0f * 1.25f);
    }
  }

  void RunOnVerticalScrollScript() {
    if (m_onVerticalScroll) {
      FrameScript_Execute(m_onVerticalScroll, this, "%f", m_scrollOffset.y * 1024.0f * 1.25f);
    }
  }

  void RunOnScrollRangeChangedScript() {
    if (m_onScrollRangeChanged) {
      FrameScript_Execute(m_onScrollRangeChanged, this, "%f%f", m_scrollRange.x * 1024.0f * 1.25f, m_scrollRange.y * 1024.0f * 1.25f);
    }
  }

 protected:
  virtual int LookupScriptMethod(lua_State *L, LPCSTR name);

  void        UpdateScrollChildRect(float w, float h);
  void        UpdateScrollChild();
  static void RenderScrollChild(LPVOID param);

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
