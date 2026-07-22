#ifndef ENGINE_SOURCE_FRAME_CSIMPLEFRAME_H
#define ENGINE_SOURCE_FRAME_CSIMPLEFRAME_H

#include "Event/EvtApi.h"
#include "Frame/CLayoutFrame.h"
#include "Frame/CSimpleRender.h"
#include "FrameScript/FrameScript.h"
#include "Tempest/c2vector.h"
#include "Tempest/crect.h"

#include <stpl.h>

class CBackdropGenerator;
class CCharEvent;
class CImeEvent;
class CKeyEvent;
class CMouseEvent;
class CSimpleFrame;
class CSimpleRegion;
class CSimpleTexture;
class CSimpleTop;

struct REGIONNODE : public TSLinkedNode<REGIONNODE> {
  CSimpleRegion *region;
};

struct SIMPLEFRAMENODE : public TSLinkedNode<SIMPLEFRAMENODE> {
  CSimpleFrame *frame;
};

enum CSimpleEventType {
  SIMPLE_EVENT_CHAR = 0,
  SIMPLE_EVENT_KEY = 1,
  SIMPLE_EVENT_MOUSE = 2,
  SIMPLE_EVENT_MOUSEWHEEL = 3,
  NUM_SIMPLE_EVENTS = 4
};

enum {
  NUM_FRAME_STRATA = 6,
  NUM_SIMPLEFRAME_DRAWLAYERS = 5
};

class CSimpleTitleRegion : public CLayoutFrame {
 public:
  CSimpleTitleRegion() : m_parent(0) {
  }

  virtual CLayoutFrame *GetLayoutParent() {
    return m_parent;
  }

  CLayoutFrame *m_parent;
};

class CSimpleFrame : public FrameScript_Object, public CLayoutFrame {
  friend class CSimpleFontString;
  friend class CSimpleTexture;
  friend int __fastcall CSimpleFrame_GetParent(lua_State *L);
  friend int __fastcall CSimpleFrame_SetID(lua_State *L);
  friend int __fastcall CSimpleFrame_GetID(lua_State *L);
  friend int __fastcall CSimpleFrame_IsShown(lua_State *L);
  friend int __fastcall CSimpleFrame_RegisterForDrag(lua_State *L);
  friend int __fastcall CSimpleFrame_SetBackdropColor(lua_State *L);
  friend int __fastcall CSimpleFrame_SetBackdropBorderColor(lua_State *L);

 public:
  CSimpleFrame(CSimpleFrame *parent);
  virtual ~CSimpleFrame();

  static void __fastcall RegisterScriptMethods();
  static void __fastcall UnregisterScriptMethods();

  virtual void          DelayedDelete();
  virtual void          PreLoadXML(const XMLNode *node, CStatus *status);
  virtual void          LoadXML(const XMLNode *node, CStatus *status);
  void                  LoadXML_Layers(const XMLNode *node, CStatus *status);
  virtual void          LoadXML_Scripts(const XMLNode *node, CStatus *status);
  virtual void          PostLoadXML(const XMLNode *node, CStatus *status);
  virtual CLayoutFrame *GetLayoutParent();
  virtual const char   *GetName() const {
    return m_frameName;
  }
  virtual void SetAlpha(unsigned char alpha);
  virtual int  FrameDefPostInitialize(unsigned int createContext, void *context) {
    return 1;
  }
  virtual int  TestHitRect(const NTempest::C2Vector &pt);
  virtual void OnLayerShow();
  virtual void OnLayerHide();
  virtual void OnLayerUpdate(float elapsedSec);
  virtual int  OnLayerTrackUpdate(const CMouseEvent &evt);
  virtual void OnFrameRender(CRenderBatch *batch, unsigned int layer);
  virtual void OnFrameRender();
  virtual void OnFrameSizeChanged(const NTempest::CRect &rect);
  virtual void OnFrameSizeChanged(float w, float h);
  virtual void OnLayerCursorEnter();
  virtual void OnLayerCursorExit();
  virtual int  OnLayerIme(CImeEvent &evt) {
    return 0;
  }
  virtual int OnLayerKeyDownRepeat(CKeyEvent &evt) {
    return 0;
  }
  virtual int OnLayerChar(CCharEvent &evt);
  virtual int OnLayerKeyDown(CKeyEvent &evt);
  virtual int OnLayerKeyUp(CKeyEvent &evt);
  virtual int OnLayerMouseDown(CMouseEvent &evt);
  virtual int OnLayerMouseUp(CMouseEvent &evt);
  virtual int OnLayerMouseWheel(CMouseEvent &evt);
  virtual int OnLayerMouseMoveRelative(CMouseEvent &evt) {
    return 0;
  }
  virtual void OnDragStart(CMouseEvent &evt);
  virtual void OnDragStop(CMouseEvent &evt);
  virtual void OnReceiveDrag(CMouseEvent &evt);
  virtual void LockHighlight(int lock);

  int Hide() {
    m_shown = 0;
    return HideThis();
  }

  int Show() {
    m_shown = 1;
    return ShowThis();
  }

  int GetFrameLevel() {
    return m_level;
  }

  CSimpleTop *GetTop() {
    return m_top;
  }

  CSimpleFrame *GetToplevelFrame() {
    CSimpleFrame *frame = this;

    if (!frame->IsToplevel()) {
      frame = frame->m_parent;
      while (frame && !frame->IsToplevel()) {
        frame = frame->m_parent;
      }
    }

    return frame;
  }

  unsigned char GetAlpha() {
    return m_alpha;
  }

  int GetFrameStrata() {
    return m_strata;
  }

  CSimpleTitleRegion *GetTitleRegion() {
    return m_titleRegion;
  }

  int IsBeingScrolled() {
    return (m_flags >> 13) & 1;
  }

  int IsAncestor(CSimpleFrame *frame) const {
    CSimpleFrame *parent = m_parent;

    while (parent && parent != frame) {
      parent = parent->m_parent;
    }

    return parent != 0;
  }

  int IsInitialized() {
    return m_initialized_state == STATE_INITIALIZED;
  }

  int IsMovable() {
    return (m_flags & 0x100) != 0;
  }

  int IsOccluded() {
    return (m_flags & 0x10) != 0;
  }

  int IsParentDrawn() {
    return !m_parent || m_parent->m_visible;
  }

  int IsResizable() {
    return (m_flags & 0x200) != 0;
  }

  int IsToplevel() {
    return (m_flags & 0x1) != 0;
  }

  int IsUserPlaced() {
    return (m_flags & 0x1000) != 0;
  }

  int IsVisible() {
    return m_visible;
  }

  void AddFrameRegion(CSimpleRegion *region, unsigned int drawlayer);
  int  AddToFrameRegistry(const char *frameName, unsigned int context);
  void ClearFromSimpleRegistry();
  void DisableDrawLayer(unsigned int drawlayer);
  void DisableEvent(CSimpleEventType event);
  void EnableDrawLayer(unsigned int drawlayer);
  int  GetHitRect(NTempest::CRect &rect);
  void NotifyDrawLayersChanged();
  void NotifyDrawLayerChanged(unsigned int drawlayer);
  void OnUpdateBatch(unsigned int layer);
  void Lower();
  void Raise();
  void RegisterRegion(CSimpleRegion *region);
  void RegisterForEvents();
  void RemoveFrameRegion(CSimpleRegion *region, unsigned int drawlayer);
  void SetOnCharScript(const char *source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnChar", GetName());
    SetEventScript(m_onChar, source, description);
  }

  void SetOnDragStartScript(const char *source) {
    char description[1024];
    if (source) {
      EnableEvent(SIMPLE_EVENT_MOUSE, static_cast<unsigned int>(-1));
    }
    SStrPrintf(description, sizeof(description), "%s:OnDragStart", GetName());
    SetEventScript(m_onDragStart, source, description);
  }

  void SetOnDragStopScript(const char *source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnDragStop", GetName());
    SetEventScript(m_onDragStop, source, description);
  }

  void SetOnEnterScript(const char *source) {
    char description[1024];
    if (source) {
      EnableEvent(SIMPLE_EVENT_MOUSE, static_cast<unsigned int>(-1));
    }
    SStrPrintf(description, sizeof(description), "%s:OnEnter", GetName());
    SetEventScript(m_onEnter, source, description);
  }

  void SetOnHideScript(const char *source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnHide", GetName());
    SetEventScript(m_onHide, source, description);
  }

  void SetOnKeyDownScript(const char *source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnKeyDown", GetName());
    SetEventScript(m_onKeyDown, source, description);
  }

  void SetOnKeyUpScript(const char *source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnKeyUp", GetName());
    SetEventScript(m_onKeyUp, source, description);
  }

  void SetOnLeaveScript(const char *source) {
    char description[1024];
    if (source) {
      EnableEvent(SIMPLE_EVENT_MOUSE, static_cast<unsigned int>(-1));
    }
    SStrPrintf(description, sizeof(description), "%s:OnLeave", GetName());
    SetEventScript(m_onLeave, source, description);
  }

  void SetOnLoadScript(const char *source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnLoad", GetName());
    SetEventScript(m_onLoad, source, description);
  }

  void SetOnMouseDownScript(const char *source) {
    char description[1024];
    if (source) {
      EnableEvent(SIMPLE_EVENT_MOUSE, static_cast<unsigned int>(-1));
    }
    SStrPrintf(description, sizeof(description), "%s:OnMouseDown", GetName());
    SetEventScript(m_onMouseDown, source, description);
  }

  void SetOnMouseUpScript(const char *source) {
    char description[1024];
    if (source) {
      EnableEvent(SIMPLE_EVENT_MOUSE, static_cast<unsigned int>(-1));
    }
    SStrPrintf(description, sizeof(description), "%s:OnMouseUp", GetName());
    SetEventScript(m_onMouseUp, source, description);
  }

  void SetOnMouseWheelScript(const char *source) {
    char description[1024];
    if (source) {
      EnableEvent(SIMPLE_EVENT_MOUSEWHEEL, static_cast<unsigned int>(-1));
    }
    SStrPrintf(description, sizeof(description), "%s:OnMouseWheel", GetName());
    SetEventScript(m_onMouseWheel, source, description);
  }

  void SetOnReceiveDragScript(const char *source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnReceiveDrag", GetName());
    SetEventScript(m_onReceiveDrag, source, description);
  }

  void SetOnShowScript(const char *source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnShow", GetName());
    SetEventScript(m_onShow, source, description);
  }

  void SetOnSizeChangedScript(const char *source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnSizeChanged", GetName());
    SetEventScript(m_onSizeChanged, source, description);
  }

  void SetOnUpdateScript(const char *source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnUpdate", GetName());
    SetEventScript(m_onUpdate, source, description);
  }

  void RunOnCharScript(const char *character) {
    ASSERT(!m_loading);
    if (m_onChar) {
      FrameScript_Execute(m_onChar, this, "%s", character);
    }
  }

  void RunOnDragStartScript(const char *button) {
    ASSERT(!m_loading);
    if (m_onDragStart) {
      FrameScript_Execute(m_onDragStart, this, "%s", button);
    }
  }

  void RunOnDragStopScript() {
    ASSERT(!m_loading);
    if (m_onDragStop) {
      FrameScript_Execute(m_onDragStop, this);
    }
  }

  void RunOnEnterScript() {
    ASSERT(!m_loading);
    if (m_onEnter) {
      FrameScript_Execute(m_onEnter, this);
    }
  }

  void RunOnHideScript() {
    if (m_onHide && !m_loading) {
      FrameScript_Execute(m_onHide, this);
    }
  }

  void RunOnKeyDownScript(const char *key) {
    ASSERT(!m_loading);
    if (m_onKeyDown) {
      FrameScript_Execute(m_onKeyDown, this, "%s", key);
    }
  }

  void RunOnKeyUpScript(const char *key) {
    ASSERT(!m_loading);
    if (m_onKeyUp) {
      FrameScript_Execute(m_onKeyUp, this, "%s", key);
    }
  }

  void RunOnLeaveScript() {
    ASSERT(!m_loading);
    if (m_onLeave) {
      FrameScript_Execute(m_onLeave, this);
    }
  }

  void RunOnLoadScript() {
    if (m_onLoad) {
      FrameScript_Execute(m_onLoad, this);
    }
  }

  void RunOnMouseDownScript(const char *button) {
    ASSERT(!m_loading);
    if (m_onMouseDown) {
      FrameScript_Execute(m_onMouseDown, this, "%s", button);
    }
  }

  void RunOnMouseUpScript(const char *button) {
    ASSERT(!m_loading);
    if (m_onMouseUp) {
      FrameScript_Execute(m_onMouseUp, this, "%s", button);
    }
  }

  void RunOnMouseWheelScript(int delta) {
    ASSERT(!m_loading);
    if (m_onMouseWheel) {
      FrameScript_Execute(m_onMouseWheel, this, "%d", delta);
    }
  }

  void RunOnReceiveDragScript() {
    ASSERT(!m_loading);
    if (m_onReceiveDrag) {
      FrameScript_Execute(m_onReceiveDrag, this);
    }
  }

  void RunOnShowScript() {
    if (m_onShow && !m_loading) {
      FrameScript_Execute(m_onShow, this);
    }
  }

  void RunOnSizeChangedScript(float width, float height) {
    if (m_onSizeChanged) {
      FrameScript_Execute(m_onSizeChanged, this, "%f%f", width, height);
    }
  }

  void RunOnUpdateScript(float elapsedSec) {
    ASSERT(!m_loading);
    if (m_onUpdate) {
      FrameScript_Execute(m_onUpdate, this, "%f", elapsedSec);
    }
  }

  void EnableEvent(CSimpleEventType event, unsigned int priority);
  void SetBeingScrolled(int on);
  void SetBackdrop(CBackdropGenerator *backdrop);
  int  SetHighlight(const char *texFile, EGxBlend blendMode);
  int  SetHighlight(CSimpleTexture *texture, EGxBlend blendMode);
  void SetFrameFlag(int flag, int on);
  void SetFrameLevel(int level, int shiftChildren);
  void SetFrameStrata(int strata);
  void SetOccluded(int occluded) {
    SetFrameFlag(0x10, occluded);
  }
  void SetHitRect(const NTempest::CRect &rect);
  void SetHitRectInsets(float left, float right, float top, float bottom);
  void SetParent(CSimpleFrame *parent);
  void SetUserPlaced(int userPlaced) {
    SetFrameFlag(0x1000, userPlaced);
  }
  void                  UnregisterRegion(CSimpleRegion *region);
  void                  UnregisterForEvents();
  virtual void          SetDeferredResize(int enable);
  virtual void          SetLayoutScale(float scale, bool resize);
  virtual CLayoutFrame *GetLayoutFrameByName(const char *name);

 protected:
  virtual int  LookupScriptMethod(lua_State *L, const char *name);
  virtual int  HideThis();
  virtual int  ShowThis();
  virtual void ClearChildrenFromSimpleRegistry();

  static TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> s_scriptMethods;

  void ParentFrame(CSimpleFrame *frame);
  void UnparentFrame(CSimpleFrame *frame);

  CSimpleTop         *m_top;
  CSimpleFrame       *m_parent;
  CSimpleFrame       *m_tooltip;
  CSimpleTitleRegion *m_titleRegion;
  enum {
    STATE_UNKNOWN = 0,
    STATE_INITIALIZED = 1,
    STATE_DESTROYED = 2,
    STATE_DELETED = 3
  } m_initialized_state;
  int                                                  m_id;
  char                                                *m_frameName;
  unsigned int                                         m_frameRegContext;
  unsigned int                                         m_flags;
  int                                                  m_strata;
  int                                                  m_level;
  unsigned char                                        m_alpha;
  unsigned int                                         m_eventmask;
  int                                                  m_shown;
  int                                                  m_visible;
  NTempest::CRect                                      m_hitRect;
  NTempest::CRect                                      m_hitOffset;
  int                                                  m_highlightLocked;
  unsigned int                                         m_lookForDrag;
  int                                                  m_mouseDown;
  int                                                  m_dragging;
  MOUSEBUTTON                                          m_dragButton;
  NTempest::C2Vector                                   m_clickPoint;
  int                                                  m_loading;
  int                                                  m_onLoad;
  int                                                  m_onSizeChanged;
  int                                                  m_onUpdate;
  int                                                  m_onShow;
  int                                                  m_onHide;
  int                                                  m_onEnter;
  int                                                  m_onLeave;
  int                                                  m_onMouseDown;
  int                                                  m_onMouseUp;
  int                                                  m_onMouseWheel;
  int                                                  m_onDragStart;
  int                                                  m_onDragStop;
  int                                                  m_onReceiveDrag;
  int                                                  m_onChar;
  int                                                  m_onKeyDown;
  int                                                  m_onKeyUp;
  int                                                  m_drawenabled[5];
  CBackdropGenerator                                  *m_backdrop;
  TSList<REGIONNODE, TSGetLink<REGIONNODE> >           m_regions;
  TSList<REGIONNODE, TSGetLink<REGIONNODE> >           m_drawlayers[5];
  unsigned int                                         m_batchDirty;
  CRenderBatch                                         m_batch[5];
  TSExplicitList<CRenderBatch, 44>                     m_renderList;
  TSList<SIMPLEFRAMENODE, TSGetLink<SIMPLEFRAMENODE> > m_children;

 public:
  TSLink<CSimpleFrame> topLink;
  TSLink<CSimpleFrame> drawLink;
};

#endif
