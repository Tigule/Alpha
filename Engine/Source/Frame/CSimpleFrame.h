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

static void GetScrollChildRect(CSimpleFrame *frame, NTempest::CRect &rect);

NODEDECL(REGIONNODE) {
  REGIONNODE() {
  }

  REGIONNODE(const REGIONNODE &);

  CSimpleRegion *region;
};

NODEDECL(SIMPLEFRAMENODE) {
  SIMPLEFRAMENODE() {
  }

  SIMPLEFRAMENODE(const SIMPLEFRAMENODE &);

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

  void SetParent(CLayoutFrame *parent) {
    m_parent = parent;
  }

 protected:
  CLayoutFrame *m_parent;
};

class CSimpleFrame : public FrameScript_Object, public CLayoutFrame {
  friend void GetScrollChildRect(CSimpleFrame *frame, NTempest::CRect &rect);
  friend class CSimpleFontString;
  friend class CSimpleTexture;
  friend int CSimpleFrame_GetParent(lua_State *L);
  friend int CSimpleFrame_SetID(lua_State *L);
  friend int CSimpleFrame_GetID(lua_State *L);
  friend int CSimpleFrame_IsShown(lua_State *L);
  friend int CSimpleFrame_RegisterForDrag(lua_State *L);
  friend int CSimpleFrame_SetBackdropColor(lua_State *L);
  friend int CSimpleFrame_SetBackdropBorderColor(lua_State *L);

 public:
  CSimpleFrame(CSimpleFrame *parent = 0);
  virtual ~CSimpleFrame();

  static void RegisterScriptMethods();
  static void UnregisterScriptMethods();

  virtual void          DelayedDelete();
  virtual void          PreLoadXML(const XMLNode *node, CStatus *status);
  virtual void          LoadXML(const XMLNode *node, CStatus *status);
  void                  LoadXML_Layers(const XMLNode *node, CStatus *status);
  virtual void          LoadXML_Scripts(const XMLNode *node, CStatus *status);
  virtual void          PostLoadXML(const XMLNode *node, CStatus *status);
  virtual CLayoutFrame *GetLayoutParent();
  virtual LPCSTR        GetName() const {
    return m_frameName;
  }
  virtual void SetAlpha(BYTE alpha);
  virtual BOOL FrameDefPostInitialize(UINT createContext, LPVOID context) {
    return 1;
  }
  virtual BOOL TestHitRect(const NTempest::C2Vector &pt);
  virtual void OnLayerShow();
  virtual void OnLayerHide();
  virtual void OnLayerUpdate(float elapsedSec);
  virtual BOOL OnLayerTrackUpdate(const CMouseEvent &evt);
  virtual void OnFrameRender();
  virtual void OnFrameRender(CRenderBatch *batch, UINT layer);
  virtual void OnFrameSizeChanged(const NTempest::CRect &rect);
  virtual void OnFrameSizeChanged(float w, float h);
  virtual void OnLayerCursorEnter();
  virtual void OnLayerCursorExit();
  virtual BOOL OnLayerIme(CImeEvent &evt) {
    return 0;
  }
  virtual BOOL OnLayerKeyDownRepeat(CKeyEvent &evt) {
    return 0;
  }
  virtual BOOL OnLayerChar(CCharEvent &evt);
  virtual BOOL OnLayerKeyDown(CKeyEvent &evt);
  virtual BOOL OnLayerKeyUp(CKeyEvent &evt);
  virtual BOOL OnLayerMouseDown(CMouseEvent &evt);
  virtual BOOL OnLayerMouseUp(CMouseEvent &evt);
  virtual BOOL OnLayerMouseWheel(CMouseEvent &evt);
  virtual BOOL OnLayerMouseMoveRelative(CMouseEvent &evt) {
    return 0;
  }
  virtual void OnDragStart(CMouseEvent &evt);
  virtual void OnDragStop(CMouseEvent &evt);
  virtual void OnReceiveDrag(CMouseEvent &evt);
  virtual void LockHighlight(int lock);

  BOOL Hide() {
    m_shown = 0;
    return HideThis();
  }

  BOOL Show() {
    m_shown = 1;
    return ShowThis();
  }

  int GetFrameLevel() const {
    return m_level;
  }

  void SetId(int id) {
    m_id = id;
  }

  int GetId() {
    return m_id;
  }

  CSimpleTop *GetTop() const {
    return m_top;
  }

  CSimpleFrame *GetParent() const {
    return m_parent;
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

  BYTE GetAlpha() const {
    return m_alpha;
  }

  int GetFrameStrata() const {
    return m_strata;
  }

  BOOL IsDialog() {
    return m_strata == 4;
  }

  BOOL IsTooltip() {
    return m_strata == 5;
  }

  CSimpleTitleRegion *GetTitleRegion() {
    return m_titleRegion;
  }

  BOOL IsBeingScrolled() const {
    return (m_flags >> 13) & 1;
  }

  BOOL IsAncestor(CSimpleFrame *frame) const {
    CSimpleFrame *parent = m_parent;

    while (parent && parent != frame) {
      parent = parent->m_parent;
    }

    return parent != 0;
  }

  BOOL IsInitialized() {
    return m_initialized_state == STATE_INITIALIZED;
  }

  BOOL IsMovable() const {
    return (m_flags & 0x100) != 0;
  }

  BOOL IsOccluded() const {
    return (m_flags & 0x10) != 0;
  }

  BOOL IsParentDrawn() const {
    return !m_parent || m_parent->m_visible;
  }

  BOOL IsResizable() const {
    return (m_flags & 0x200) != 0;
  }

  BOOL IsToplevel() const {
    return (m_flags & 0x1) != 0;
  }

  BOOL IsUserPlaced() const {
    return (m_flags & 0x1000) != 0;
  }

  BOOL IsVisible() const {
    return m_visible;
  }

  BOOL IsShown() const {
    return m_shown;
  }

  void RegisterForDrag(UINT buttons) {
    m_lookForDrag = buttons;
  }

  int ScaleBy(float scaleX, float scaleY, FRAMEPOINT anchor, NTempest::CRect *rect) {
    return CLayoutFrame::ScaleBy(reinterpret_cast<CLayoutFrame *>(m_top), scaleX, scaleY, anchor, rect);
  }

  int DragBy(float deltaX, float deltaY, FRAMEPOINT anchor, NTempest::CRect *rect) {
    return CLayoutFrame::DragBy(reinterpret_cast<CLayoutFrame *>(m_top), deltaX, deltaY, anchor, rect);
  }

  void AddFrameRegion(CSimpleRegion *region, UINT drawlayer);
  BOOL AddToFrameRegistry(LPCSTR frameName, UINT context);
  void ClearFromSimpleRegistry();
  void DisableDrawLayer(UINT drawlayer);
  void DisableEvent(CSimpleEventType event);
  void EnableDrawLayer(UINT drawlayer);
  BOOL GetHitRect(NTempest::CRect &rect);
  void NotifyDrawLayersChanged();
  void NotifyDrawLayerChanged(UINT drawlayer);
  void OnUpdateBatch(UINT layer);
  void Lower();
  void Raise();
  void RegisterRegion(CSimpleRegion *region);
  void RegisterForEvents();
  void RemoveFrameRegion(CSimpleRegion *region, UINT drawlayer);
  void SetOnCharScript(LPCSTR source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnChar", GetName());
    SetEventScript(m_onChar, source, description);
  }

  void SetOnDragStartScript(LPCSTR source) {
    char description[1024];
    if (source) {
      EnableEvent(SIMPLE_EVENT_MOUSE, static_cast<UINT>(-1));
    }
    SStrPrintf(description, sizeof(description), "%s:OnDragStart", GetName());
    SetEventScript(m_onDragStart, source, description);
  }

  void SetOnDragStopScript(LPCSTR source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnDragStop", GetName());
    SetEventScript(m_onDragStop, source, description);
  }

  void SetOnEnterScript(LPCSTR source) {
    char description[1024];
    if (source) {
      EnableEvent(SIMPLE_EVENT_MOUSE, static_cast<UINT>(-1));
    }
    SStrPrintf(description, sizeof(description), "%s:OnEnter", GetName());
    SetEventScript(m_onEnter, source, description);
  }

  void SetOnHideScript(LPCSTR source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnHide", GetName());
    SetEventScript(m_onHide, source, description);
  }

  void SetOnKeyDownScript(LPCSTR source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnKeyDown", GetName());
    SetEventScript(m_onKeyDown, source, description);
  }

  void SetOnKeyUpScript(LPCSTR source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnKeyUp", GetName());
    SetEventScript(m_onKeyUp, source, description);
  }

  void SetOnLeaveScript(LPCSTR source) {
    char description[1024];
    if (source) {
      EnableEvent(SIMPLE_EVENT_MOUSE, static_cast<UINT>(-1));
    }
    SStrPrintf(description, sizeof(description), "%s:OnLeave", GetName());
    SetEventScript(m_onLeave, source, description);
  }

  void SetOnLoadScript(LPCSTR source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnLoad", GetName());
    SetEventScript(m_onLoad, source, description);
  }

  void SetOnMouseDownScript(LPCSTR source) {
    char description[1024];
    if (source) {
      EnableEvent(SIMPLE_EVENT_MOUSE, static_cast<UINT>(-1));
    }
    SStrPrintf(description, sizeof(description), "%s:OnMouseDown", GetName());
    SetEventScript(m_onMouseDown, source, description);
  }

  void SetOnMouseUpScript(LPCSTR source) {
    char description[1024];
    if (source) {
      EnableEvent(SIMPLE_EVENT_MOUSE, static_cast<UINT>(-1));
    }
    SStrPrintf(description, sizeof(description), "%s:OnMouseUp", GetName());
    SetEventScript(m_onMouseUp, source, description);
  }

  void SetOnMouseWheelScript(LPCSTR source) {
    char description[1024];
    if (source) {
      EnableEvent(SIMPLE_EVENT_MOUSEWHEEL, static_cast<UINT>(-1));
    }
    SStrPrintf(description, sizeof(description), "%s:OnMouseWheel", GetName());
    SetEventScript(m_onMouseWheel, source, description);
  }

  void SetOnReceiveDragScript(LPCSTR source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnReceiveDrag", GetName());
    SetEventScript(m_onReceiveDrag, source, description);
  }

  void SetOnShowScript(LPCSTR source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnShow", GetName());
    SetEventScript(m_onShow, source, description);
  }

  void SetOnSizeChangedScript(LPCSTR source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnSizeChanged", GetName());
    SetEventScript(m_onSizeChanged, source, description);
  }

  void SetOnUpdateScript(LPCSTR source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnUpdate", GetName());
    SetEventScript(m_onUpdate, source, description);
  }

  void RunOnCharScript(LPCSTR character) {
    ASSERT(!m_loading);
    if (m_onChar) {
      FrameScript_Execute(m_onChar, this, "%s", character);
    }
  }

  void RunOnDragStartScript(MOUSEBUTTON button) {
    LPCSTR buttonName;

    switch (button) {
      case MOUSE_BUTTON_LEFT:
        buttonName = "LeftButton";
        break;
      case MOUSE_BUTTON_MIDDLE:
        buttonName = "MiddleButton";
        break;
      case MOUSE_BUTTON_RIGHT:
        buttonName = "RightButton";
        break;
      case MOUSE_BUTTON_XBUTTON1:
        buttonName = "Button4";
        break;
      case MOUSE_BUTTON_XBUTTON2:
        buttonName = "Button5";
        break;
      default:
        buttonName = "UNKNOWN";
        break;
    }

    ASSERT(!m_loading);
    if (m_onDragStart) {
      FrameScript_Execute(m_onDragStart, this, "%s", buttonName);
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

  void RunOnKeyDownScript(LPCSTR key) {
    ASSERT(!m_loading);
    if (m_onKeyDown) {
      FrameScript_Execute(m_onKeyDown, this, "%s", key);
    }
  }

  void RunOnKeyUpScript(LPCSTR key) {
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

  void RunOnMouseDownScript(MOUSEBUTTON button) {
    LPCSTR buttonName;

    switch (button) {
      case MOUSE_BUTTON_LEFT:
        buttonName = "LeftButton";
        break;
      case MOUSE_BUTTON_MIDDLE:
        buttonName = "MiddleButton";
        break;
      case MOUSE_BUTTON_RIGHT:
        buttonName = "RightButton";
        break;
      case MOUSE_BUTTON_XBUTTON1:
        buttonName = "Button4";
        break;
      case MOUSE_BUTTON_XBUTTON2:
        buttonName = "Button5";
        break;
      default:
        buttonName = "UNKNOWN";
        break;
    }

    ASSERT(!m_loading);
    if (m_onMouseDown) {
      FrameScript_Execute(m_onMouseDown, this, "%s", buttonName);
    }
  }

  void RunOnMouseUpScript(MOUSEBUTTON button) {
    LPCSTR buttonName;

    switch (button) {
      case MOUSE_BUTTON_LEFT:
        buttonName = "LeftButton";
        break;
      case MOUSE_BUTTON_MIDDLE:
        buttonName = "MiddleButton";
        break;
      case MOUSE_BUTTON_RIGHT:
        buttonName = "RightButton";
        break;
      case MOUSE_BUTTON_XBUTTON1:
        buttonName = "Button4";
        break;
      case MOUSE_BUTTON_XBUTTON2:
        buttonName = "Button5";
        break;
      default:
        buttonName = "UNKNOWN";
        break;
    }

    ASSERT(!m_loading);
    if (m_onMouseUp) {
      FrameScript_Execute(m_onMouseUp, this, "%s", buttonName);
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

  void                EnableEvent(CSimpleEventType event, UINT priority);
  void                SetBeingScrolled(int on);
  void                SetBackdrop(CBackdropGenerator *backdrop);
  CBackdropGenerator *GetBackdrop() {
    return m_backdrop;
  }
  LIST(REGIONNODE) & GetRegions() {
    return m_regions;
  }
  LIST(SIMPLEFRAMENODE) & GetChildren() {
    return m_children;
  }
  BOOL SetHighlight(LPCSTR texFile, EGxBlend blendMode);
  BOOL SetHighlight(CSimpleTexture *texture, EGxBlend blendMode);
  void SetFrameFlag(int flag, int on);
  void SetFrameLevel(int level, int shiftChildren);
  void SetFrameStrata(int strata);
  void SetOccluded(int occluded) {
    SetFrameFlag(0x10, occluded);
  }
  void SetMovable(int movable) {
    SetFrameFlag(0x100, movable);
  }
  void SetResizable(int resizable) {
    SetFrameFlag(0x200, resizable);
  }
  void SetToplevel(int toplevel) {
    SetFrameFlag(0x1, toplevel);
  }
  void SetHitRect(const NTempest::CRect &rect);
  void SetHitRectInsets(float left, float right, float top, float bottom);
  void SetParent(CSimpleFrame *parent);
  void SetTooltip(CSimpleFrame *tooltip) {
    m_tooltip = tooltip;
  }
  void SetTitleRegion(CSimpleTitleRegion *titleRegion) {
    m_titleRegion = titleRegion;
  }
  void SetUserPlaced(int userPlaced) {
    SetFrameFlag(0x1000, userPlaced);
  }
  void                  UnregisterRegion(CSimpleRegion *region);
  void                  UnregisterForEvents();
  virtual void          SetDeferredResize(int enable);
  virtual void          SetLayoutScale(float scale, bool resize);
  virtual CLayoutFrame *GetLayoutFrameByName(LPCSTR name);

 protected:
  virtual BOOL LookupScriptMethod(lua_State *L, LPCSTR name);
  virtual BOOL HideThis();
  virtual BOOL ShowThis();
  virtual void ClearChildrenFromSimpleRegistry();

  static TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> s_scriptMethods;

  void AnchorDrawRegion(CSimpleRegion *region, UINT drawlayer);
  void UnanchorDrawRegion(CSimpleRegion *region);
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
  int                 m_id;
  char               *m_frameName;
  UINT                m_frameRegContext;
  UINT                m_flags;
  int                 m_strata;
  int                 m_level;
  BYTE                m_alpha;
  UINT                m_eventmask;
  BOOL                m_shown;
  BOOL                m_visible;
  NTempest::CRect     m_hitRect;
  NTempest::CRect     m_hitOffset;
  int                 m_highlightLocked;
  UINT                m_lookForDrag;
  BOOL                m_mouseDown;
  BOOL                m_dragging;
  MOUSEBUTTON         m_dragButton;
  NTempest::C2Vector  m_clickPoint;
  BOOL                m_loading;
  int                 m_onLoad;
  int                 m_onSizeChanged;
  int                 m_onUpdate;
  int                 m_onShow;
  int                 m_onHide;
  int                 m_onEnter;
  int                 m_onLeave;
  int                 m_onMouseDown;
  int                 m_onMouseUp;
  int                 m_onMouseWheel;
  int                 m_onDragStart;
  int                 m_onDragStop;
  int                 m_onReceiveDrag;
  int                 m_onChar;
  int                 m_onKeyDown;
  int                 m_onKeyUp;
  int                 m_drawenabled[5];
  CBackdropGenerator *m_backdrop;
  LISTDECL(REGIONNODE, m_regions);
  LISTDECL(REGIONNODE, m_drawlayers[5]);
  UINT         m_batchDirty;
  CRenderBatch m_batch[5];
  LISTDECLEX(CRenderBatch, renderLink, m_renderList);
  LISTDECL(SIMPLEFRAMENODE, m_children);

 public:
  LINKDECLEX(CSimpleFrame, topLink);
  LINKDECLEX(CSimpleFrame, drawLink);
};

#endif
