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
  DRAWLAYER_BACKGROUND = 0,
  DRAWLAYER_BACKGROUND_BORDER = 1,
  DRAWLAYER_ARTWORK = 2,
  DRAWLAYER_ARTWORK_OVERLAY = 3,
  DRAWLAYER_HIGHLIGHT = 4,
  NUM_SIMPLEFRAME_DRAWLAYERS = 5
};

enum {
  FRAME_STRATA_BACKGROUND = 0,
  FRAME_STRATA_LOW = 1,
  FRAME_STRATA_MEDIUM = 2,
  FRAME_STRATA_HIGH = 3,
  FRAME_STRATA_DIALOG = 4,
  FRAME_STRATA_TOOLTIP = 5,
  NUM_FRAME_STRATA = 6
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

  BOOL IsInitialized() {
    return m_initialized_state == STATE_INITIALIZED;
  }

  void SetFrameFlag(int flag, int on);

  void SetToplevel(int toplevel) {
    SetFrameFlag(0x1, toplevel);
  }

  BOOL IsToplevel() const {
    return (m_flags & 0x1) != 0;
  }

  void SetOccluded(int occluded) {
    SetFrameFlag(0x10, occluded);
  }

  BOOL IsOccluded() const {
    return (m_flags & 0x10) != 0;
  }

  void SetMovable(int movable) {
    SetFrameFlag(0x100, movable);
  }

  BOOL IsMovable() const {
    return (m_flags & 0x100) != 0;
  }

  void SetResizable(int resizable) {
    SetFrameFlag(0x200, resizable);
  }

  BOOL IsResizable() const {
    return (m_flags & 0x200) != 0;
  }

  void SetUserPlaced(int userPlaced) {
    SetFrameFlag(0x1000, userPlaced);
  }

  BOOL IsUserPlaced() const {
    return (m_flags & 0x1000) != 0;
  }

  void                SetBeingScrolled(int on);

  BOOL IsBeingScrolled() const {
    return (m_flags >> 13) & 1;
  }

  BOOL IsParentDrawn() const {
    return !m_parent || m_parent->m_visible;
  }

  void SetFrameStrata(int strata);

  int GetFrameStrata() const {
    return m_strata;
  }

  BOOL IsDialog() {
    return m_strata == 4;
  }

  BOOL IsTooltip() {
    return m_strata == 5;
  }

  void SetFrameLevel(int level, int shiftChildren);

  int GetFrameLevel() const {
    return m_level;
  }

  CSimpleTop *GetTop() const {
    return m_top;
  }

  void Raise();
  void Lower();

  void SetTitleRegion(CSimpleTitleRegion *titleRegion) {
    m_titleRegion = titleRegion;
  }

  CSimpleTitleRegion *GetTitleRegion() {
    return m_titleRegion;
  }

  int ScaleBy(float scaleX, float scaleY, FRAMEPOINT anchor, NTempest::CRect *rect) {
    return CLayoutFrame::ScaleBy(reinterpret_cast<CLayoutFrame *>(m_top), scaleX, scaleY, anchor, rect);
  }

  int DragBy(float deltaX, float deltaY, FRAMEPOINT anchor, NTempest::CRect *rect) {
    return CLayoutFrame::DragBy(reinterpret_cast<CLayoutFrame *>(m_top), deltaX, deltaY, anchor, rect);
  }

  void                SetBackdrop(CBackdropGenerator *backdrop);

  CBackdropGenerator *GetBackdrop() {
    return m_backdrop;
  }

  BOOL SetHighlight(LPCSTR texFile, EGxBlend blendMode);
  BOOL SetHighlight(CSimpleTexture *texture, EGxBlend blendMode);
  virtual void SetAlpha(BYTE alpha);

  BYTE GetAlpha() const {
    return m_alpha;
  }

  void EnableDrawLayer(UINT drawlayer);
  void DisableDrawLayer(UINT drawlayer);
  void RegisterRegion(CSimpleRegion *region);
  void                  UnregisterRegion(CSimpleRegion *region);
  void AddFrameRegion(CSimpleRegion *region, UINT drawlayer);
  void RemoveFrameRegion(CSimpleRegion *region, UINT drawlayer);
  void NotifyDrawLayerChanged(UINT drawlayer);
  void NotifyDrawLayersChanged();

  void SetId(int id) {
    m_id = id;
  }

  int GetId() {
    return m_id;
  }

  BOOL AddToFrameRegistry(LPCSTR frameName, UINT context);
  void ClearFromSimpleRegistry();

  virtual BOOL FrameDefPostInitialize(UINT createContext, LPVOID context) {
    return 1;
  }

  void SetParent(CSimpleFrame *parent);

  CSimpleFrame *GetParent() const {
    return m_parent;
  }

  CSimpleFrame *GetToplevelFrame() {
    if (IsToplevel()) {
      return this;
    }

    CSimpleFrame *frame = m_parent;
    while (frame) {
      if (frame->IsToplevel()) {
        return frame;
      }

      frame = frame->m_parent;
    }

    return 0;
  }

  BOOL IsAncestor(CSimpleFrame *frame) const {
    CSimpleFrame *parent = m_parent;

    while (parent) {
      if (parent == frame) {
        return 1;
      }

      parent = parent->m_parent;
    }

    return 0;
  }

  LIST(REGIONNODE) & GetRegions() {
    return m_regions;
  }

  LIST(SIMPLEFRAMENODE) & GetChildren() {
    return m_children;
  }

  void SetTooltip(CSimpleFrame *tooltip) {
    m_tooltip = tooltip;
  }

  virtual void          SetDeferredResize(int enable);
  virtual void          SetLayoutScale(float scale, bool resize);

  BOOL Hide() {
    m_shown = 0;
    return HideThis();
  }

  BOOL Show() {
    m_shown = 1;
    return ShowThis();
  }

  BOOL IsShown() const {
    return m_shown;
  }

  BOOL IsVisible() const {
    return m_visible;
  }

  void                EnableEvent(CSimpleEventType event, UINT priority);
  void DisableEvent(CSimpleEventType event);
  void RegisterForEvents();
  void                  UnregisterForEvents();
  virtual BOOL TestHitRect(const NTempest::C2Vector &pt);
  void SetHitRect(const NTempest::CRect &rect);
  void SetHitRectInsets(float left, float right, float top, float bottom);
  BOOL GetHitRect(NTempest::CRect &rect);
  virtual void OnLayerShow();
  virtual void OnLayerHide();
  virtual void OnLayerUpdate(float elapsedSec);
  virtual BOOL OnLayerTrackUpdate(const CMouseEvent &evt);
  virtual void OnFrameRender(CRenderBatch *batch, UINT layer);
  virtual void OnFrameRender();
  void OnUpdateBatch(UINT layer);
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

  void RegisterForDrag(UINT buttons) {
    m_lookForDrag = buttons;
  }

  void SetOnLoadScript(LPCSTR source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnLoad", GetName());
    SetEventScript(m_onLoad, source, description);
  }

  void RunOnLoadScript() {
    if (m_onLoad) {
      FrameScript_Execute(m_onLoad, this);
    }
  }

  void SetOnSizeChangedScript(LPCSTR source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnSizeChanged", GetName());
    SetEventScript(m_onSizeChanged, source, description);
  }

  void RunOnSizeChangedScript(float width, float height) {
    if (m_onSizeChanged) {
      FrameScript_Execute(m_onSizeChanged, this, "%f%f", width, height);
    }
  }

  void SetOnUpdateScript(LPCSTR source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnUpdate", GetName());
    SetEventScript(m_onUpdate, source, description);
  }

  void RunOnUpdateScript(float elapsedSec) {
    ASSERT(!m_loading);
    if (m_onUpdate) {
      FrameScript_Execute(m_onUpdate, this, "%f", elapsedSec);
    }
  }

  void SetOnShowScript(LPCSTR source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnShow", GetName());
    SetEventScript(m_onShow, source, description);
  }

  void RunOnShowScript() {
    if (m_onShow && !m_loading) {
      FrameScript_Execute(m_onShow, this);
    }
  }

  void SetOnHideScript(LPCSTR source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnHide", GetName());
    SetEventScript(m_onHide, source, description);
  }

  void RunOnHideScript() {
    if (m_onHide && !m_loading) {
      FrameScript_Execute(m_onHide, this);
    }
  }

  void SetOnEnterScript(LPCSTR source) {
    char description[1024];
    if (source) {
      EnableEvent(SIMPLE_EVENT_MOUSE, static_cast<UINT>(-1));
    }
    SStrPrintf(description, sizeof(description), "%s:OnEnter", GetName());
    SetEventScript(m_onEnter, source, description);
  }

  void RunOnEnterScript() {
    ASSERT(!m_loading);
    if (m_onEnter) {
      FrameScript_Execute(m_onEnter, this);
    }
  }

  void SetOnLeaveScript(LPCSTR source) {
    char description[1024];
    if (source) {
      EnableEvent(SIMPLE_EVENT_MOUSE, static_cast<UINT>(-1));
    }
    SStrPrintf(description, sizeof(description), "%s:OnLeave", GetName());
    SetEventScript(m_onLeave, source, description);
  }

  void RunOnLeaveScript() {
    ASSERT(!m_loading);
    if (m_onLeave) {
      FrameScript_Execute(m_onLeave, this);
    }
  }

  void SetOnMouseDownScript(LPCSTR source) {
    char description[1024];
    if (source) {
      EnableEvent(SIMPLE_EVENT_MOUSE, static_cast<UINT>(-1));
    }
    SStrPrintf(description, sizeof(description), "%s:OnMouseDown", GetName());
    SetEventScript(m_onMouseDown, source, description);
  }

  void RunOnMouseDownScript(MOUSEBUTTON button) {
    ASSERT(!m_loading);
    if (m_onMouseDown) {
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

      FrameScript_Execute(m_onMouseDown, this, "%s", buttonName);
    }
  }

  void SetOnMouseUpScript(LPCSTR source) {
    char description[1024];
    if (source) {
      EnableEvent(SIMPLE_EVENT_MOUSE, static_cast<UINT>(-1));
    }
    SStrPrintf(description, sizeof(description), "%s:OnMouseUp", GetName());
    SetEventScript(m_onMouseUp, source, description);
  }

  void RunOnMouseUpScript(MOUSEBUTTON button) {
    ASSERT(!m_loading);
    if (m_onMouseUp) {
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

      FrameScript_Execute(m_onMouseUp, this, "%s", buttonName);
    }
  }

  void SetOnMouseWheelScript(LPCSTR source) {
    char description[1024];
    if (source) {
      EnableEvent(SIMPLE_EVENT_MOUSEWHEEL, static_cast<UINT>(-1));
    }
    SStrPrintf(description, sizeof(description), "%s:OnMouseWheel", GetName());
    SetEventScript(m_onMouseWheel, source, description);
  }

  void RunOnMouseWheelScript(int delta) {
    ASSERT(!m_loading);
    if (m_onMouseWheel) {
      FrameScript_Execute(m_onMouseWheel, this, "%d", delta);
    }
  }

  void SetOnDragStartScript(LPCSTR source) {
    char description[1024];
    if (source) {
      EnableEvent(SIMPLE_EVENT_MOUSE, static_cast<UINT>(-1));
    }
    SStrPrintf(description, sizeof(description), "%s:OnDragStart", GetName());
    SetEventScript(m_onDragStart, source, description);
  }

  void RunOnDragStartScript(MOUSEBUTTON button) {
    ASSERT(!m_loading);
    if (m_onDragStart) {
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

      FrameScript_Execute(m_onDragStart, this, "%s", buttonName);
    }
  }

  void SetOnDragStopScript(LPCSTR source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnDragStop", GetName());
    SetEventScript(m_onDragStop, source, description);
  }

  void RunOnDragStopScript() {
    ASSERT(!m_loading);
    if (m_onDragStop) {
      FrameScript_Execute(m_onDragStop, this);
    }
  }

  void SetOnReceiveDragScript(LPCSTR source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnReceiveDrag", GetName());
    SetEventScript(m_onReceiveDrag, source, description);
  }

  void RunOnReceiveDragScript() {
    ASSERT(!m_loading);
    if (m_onReceiveDrag) {
      FrameScript_Execute(m_onReceiveDrag, this);
    }
  }

  void SetOnCharScript(LPCSTR source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnChar", GetName());
    SetEventScript(m_onChar, source, description);
  }

  void RunOnCharScript(LPCSTR character) {
    ASSERT(!m_loading);
    if (m_onChar) {
      FrameScript_Execute(m_onChar, this, "%s", character);
    }
  }

  void SetOnKeyDownScript(LPCSTR source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnKeyDown", GetName());
    SetEventScript(m_onKeyDown, source, description);
  }

  void RunOnKeyDownScript(LPCSTR key) {
    ASSERT(!m_loading);
    if (m_onKeyDown) {
      FrameScript_Execute(m_onKeyDown, this, "%s", key);
    }
  }

  void SetOnKeyUpScript(LPCSTR source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnKeyUp", GetName());
    SetEventScript(m_onKeyUp, source, description);
  }

  void RunOnKeyUpScript(LPCSTR key) {
    ASSERT(!m_loading);
    if (m_onKeyUp) {
      FrameScript_Execute(m_onKeyUp, this, "%s", key);
    }
  }

  virtual CLayoutFrame *GetLayoutFrameByName(LPCSTR name);
  static void RegisterScriptMethods();
  static void UnregisterScriptMethods();

 protected:
  virtual BOOL LookupScriptMethod(lua_State *L, LPCSTR name);
  static TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> s_scriptMethods;
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
  virtual BOOL HideThis();
  virtual BOOL ShowThis();
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
  void AnchorDrawRegion(CSimpleRegion *region, UINT drawlayer);
  void UnanchorDrawRegion(CSimpleRegion *region);
  LISTDECL(SIMPLEFRAMENODE, m_children);
  void ParentFrame(CSimpleFrame *frame);
  void UnparentFrame(CSimpleFrame *frame);
  virtual void ClearChildrenFromSimpleRegistry();

 public:
  LINKDECLEX(CSimpleFrame, topLink);
  LINKDECLEX(CSimpleFrame, drawLink);
};

#endif
