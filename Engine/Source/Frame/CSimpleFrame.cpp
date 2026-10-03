#include <Base/Base.h>

#include "Frame/CSimpleFrame.h"

#include "Base/ConvertUTF.h"
#include "Base/Status.h"
#include "Event/CMouseEvent.h"
#include "Frame/CBackdropGenerator.h"
#include "Frame/SimpleFrameRegistry.h"
#include "Frame/CSimpleTop.h"
#include "FrameXML/FrameXML.h"
#include "FrameXML/LoadXML.h"
#include "FrameXML/XMLTree.h"

#include <math.h>

static const float MIN_DRAG_DIST = 0.0080000004f;
static const float MIN_DRAG_DIST_SQ = MIN_DRAG_DIST * MIN_DRAG_DIST;
static char        charBuf[8];

CSimpleFrame::CSimpleFrame(CSimpleFrame *parent)
    : m_parent(0),
      m_tooltip(0),
      m_titleRegion(0),
      m_initialized_state(STATE_INITIALIZED),
      m_id(0),
      m_frameName(0),
      m_frameRegContext(0),
      m_flags(0),
      m_strata(2),
      m_level(0),
      m_alpha(0xFF),
      m_eventmask(0),
      m_visible(0),
      m_highlightLocked(0),
      m_lookForDrag(0),
      m_mouseDown(0),
      m_dragging(0),
      m_loading(0),
      m_onLoad(0),
      m_onSizeChanged(0),
      m_onUpdate(0),
      m_onShow(0),
      m_onHide(0),
      m_onEnter(0),
      m_onLeave(0),
      m_onMouseDown(0),
      m_onMouseUp(0),
      m_onMouseWheel(0),
      m_onDragStart(0),
      m_onDragStop(0),
      m_onReceiveDrag(0),
      m_onChar(0),
      m_onKeyDown(0),
      m_onKeyUp(0),
      m_backdrop(0),
      m_batchDirty(0) {
  m_top = CSimpleTop::GetInstance();
  m_top->RegisterFrame(this);
  SetParent(parent);

  for (UINT i = 0; i < 4; ++i) {
    m_drawenabled[i] = 1;
  }
  m_drawenabled[4] = 0;
  Show();
}

CSimpleFrame::~CSimpleFrame() {
  m_top->UnregisterFrame(this);
  m_top->ValidateDeletedFrame(this);
  m_top = 0;

  ClearFromSimpleRegistry();

  if (m_titleRegion) {
    DEL(m_titleRegion);
  }

  while (m_regions.Head()) {
    REGIONNODE *region = m_regions.Head();
    DEL(region->region);
  }

  UINT i;
  for (i = 0; i < NUM_SIMPLEFRAME_DRAWLAYERS; ++i) {
    ASSERT(m_drawlayers[i].IsEmpty());
  }

  for (CRenderBatch *node = m_renderList.Head(), *nodenext_node; reinterpret_cast<int>(node) > 0 ? ((nodenext_node = m_renderList.RawNext(node)), 1) : 0;
       node = nodenext_node) {
    m_renderList.UnlinkNode(node);
  }

  while (m_children.Head()) {
    SIMPLEFRAMENODE *frame = m_children.Head();
    DEL(frame->frame);
  }

  if (m_parent) {
    m_parent->UnparentFrame(this);
  }

  if (m_backdrop) {
    DEL(m_backdrop);
  }

  SetOnLoadScript(0);
  SetOnSizeChangedScript(0);
  SetOnUpdateScript(0);
  SetOnShowScript(0);
  SetOnHideScript(0);
  SetOnEnterScript(0);
  SetOnLeaveScript(0);
  SetOnMouseDownScript(0);
  SetOnMouseUpScript(0);
  SetOnMouseWheelScript(0);
  SetOnDragStartScript(0);
  SetOnDragStopScript(0);
  SetOnReceiveDragScript(0);

  m_initialized_state = STATE_DELETED;
}

void CSimpleFrame::DelayedDelete() {
  ASSERT(m_initialized_state != STATE_DELETED);

  if (m_initialized_state != STATE_DESTROYED) {
    ClearChildrenFromSimpleRegistry();
    m_top->RegisterForDelete(this);
    m_initialized_state = STATE_DESTROYED;
  }
}

CLayoutFrame *CSimpleFrame::GetLayoutFrameByName(LPCSTR name) {
  char newName[1024];

  if (!SStrCmpI(name, "$parent", SStrLen("$parent"))) {
    CSimpleFrame *parent;

    SStrCopy(newName, "Top", 0x7FFFFFFF);
    for (parent = m_parent; parent; parent = parent->m_parent) {
      LPCSTR parentName = parent->GetName();

      if (parentName && *parentName) {
        SStrCopy(newName, parentName, sizeof(newName));
        break;
      }
    }

    SStrPack(newName, name + SStrLen("$parent"), sizeof(newName));
  } else {
    SStrCopy(newName, name, sizeof(newName));
  }

  return CLayoutFrame::GetLayoutFrameByName(newName);
}

void CSimpleFrame::PreLoadXML(const XMLNode *node, CStatus *status) {
  LPCSTR frameName = node->GetAttributeByName("name");

  if (frameName && *frameName) {
    char name[1024];

    if (!SStrCmpI(frameName, "$parent", SStrLen("$parent"))) {
      CSimpleFrame *parent;

      SStrCopy(name, "Top", 0x7FFFFFFF);
      for (parent = m_parent; parent; parent = parent->m_parent) {
        LPCSTR parentName = parent->GetName();

        if (parentName && *parentName) {
          SStrCopy(name, parentName, sizeof(name));
          break;
        }
      }

      SStrPack(name, frameName + SStrLen("$parent"), sizeof(name));
    } else {
      SStrCopy(name, frameName, sizeof(name));
    }

    if (!AddToFrameRegistry(name, 0)) {
      status->Add(STATUS_WARNING, "Frame named '%s' already registered", name);
    }
  }

  m_loading = 1;
  SetDeferredResize(1);
}

void CSimpleFrame::LoadXML(const XMLNode *node, CStatus *status) {
  LPCSTR attribute;

  CLayoutFrame::LoadXML(node, status);

  attribute = node->GetAttributeByName("hidden");
  if (attribute && *attribute) {
    if (StringToBOOL(attribute)) {
      Hide();
    } else {
      Show();
    }
  }

  attribute = node->GetAttributeByName("toplevel");
  if (attribute && *attribute) {
    SetFrameFlag(0x1, StringToBOOL(attribute));
  }

  attribute = node->GetAttributeByName("movable");
  if (attribute && *attribute) {
    SetFrameFlag(0x100, StringToBOOL(attribute));
  }

  attribute = node->GetAttributeByName("resizable");
  if (attribute && *attribute) {
    SetFrameFlag(0x200, StringToBOOL(attribute));
  }

  attribute = node->GetAttributeByName("frameStrata");
  if (attribute && *attribute) {
    if (!SStrCmpI(attribute, "BACKGROUND", 0x7FFFFFFF)) {
      SetFrameStrata(0);
    } else if (!SStrCmpI(attribute, "LOW", 0x7FFFFFFF)) {
      SetFrameStrata(1);
    } else if (!SStrCmpI(attribute, "MEDIUM", 0x7FFFFFFF) || !SStrCmpI(attribute, "NORMAL", 0x7FFFFFFF)) {
      SetFrameStrata(2);
    } else if (!SStrCmpI(attribute, "HIGH", 0x7FFFFFFF)) {
      SetFrameStrata(3);
    } else if (!SStrCmpI(attribute, "DIALOG", 0x7FFFFFFF)) {
      SetFrameStrata(4);
    } else if (!SStrCmpI(attribute, "TOOLTIP", 0x7FFFFFFF)) {
      SetFrameStrata(5);
    }
  }

  attribute = node->GetAttributeByName("frameLevel");
  if (attribute && *attribute) {
    int level = SStrToInt(attribute);

    if (level > 0) {
      SetFrameLevel(level, 0);
    } else {
      status->Add(STATUS_WARNING, "Unknown frame level: %s", attribute);
    }
  }

  attribute = node->GetAttributeByName("alpha");
  if (attribute && *attribute) {
    SetAlpha(static_cast<BYTE>(__max(__min(SStrToFloat(attribute), 1.0f), 0.0f) * 255.0f));
  }

  attribute = node->GetAttributeByName("id");
  if (attribute && *attribute) {
    int id = SStrToInt(attribute);

    if (id >= 0) {
      SetId(id);
    }
  }

  attribute = node->GetAttributeByName("enableMouse");
  if (attribute && *attribute && StringToBOOL(attribute)) {
    EnableEvent(SIMPLE_EVENT_MOUSE, static_cast<UINT>(-1));
  }

  attribute = node->GetAttributeByName("enableKeyboard");
  if (attribute && *attribute && StringToBOOL(attribute)) {
    EnableEvent(SIMPLE_EVENT_KEY, static_cast<UINT>(-1));
    EnableEvent(SIMPLE_EVENT_CHAR, static_cast<UINT>(-1));
  }

  const XMLNode *child;
  for (child = node->GetChild(); child; child = child->GetSibling()) {
    if (!SStrCmpI(child->GetName(), "TitleRegion", 0x7FFFFFFF)) {
      CSimpleTitleRegion *titleRegion = NEW(CSimpleTitleRegion);

      titleRegion->SetParent(this);
      titleRegion->LoadXML(child, status);
      SetTitleRegion(titleRegion);
    } else if (!SStrCmpI(child->GetName(), "Backdrop", 0x7FFFFFFF)) {
      CBackdropGenerator *backdrop = NEW(CBackdropGenerator);

      backdrop->LoadXML(child, status);
      SetBackdrop(backdrop);
    } else if (!SStrCmpI(child->GetName(), "HitRectInsets", 0x7FFFFFFF)) {
      float l;
      float r;
      float t;
      float b;

      if (LoadXML_Insets(child, l, r, t, b, status)) {
        SetHitRectInsets(l, r, t, b);
      }
    } else if (!SStrCmpI(child->GetName(), "Layers", 0x7FFFFFFF)) {
      LoadXML_Layers(child, status);
    } else if (!SStrCmpI(child->GetName(), "Scripts", 0x7FFFFFFF)) {
      LPCSTR name = GetName();

      if (name && *name) {
        LoadXML_Scripts(child, status);
      } else {
        status->Add(STATUS_WARNING, "An unnamed frame cannot have a 'Scripts' element");
      }
    }
  }

  const XMLNode *frames = node->GetChildByName("Frames");
  if (frames) {
    for (child = frames->GetChild(); child; child = child->GetSibling()) {
      FrameXML_CreateFrame(child, this, status);
    }
  }
}

void CSimpleFrame::LoadXML_Layers(const XMLNode *node, CStatus *status) {
  const XMLNode *layer;

  for (layer = node->GetChild(); layer; layer = layer->GetSibling()) {
    if (SStrCmpI(layer->GetName(), "Layer", 0x7FFFFFFF)) {
      status->Add(STATUS_WARNING, "Unknown child node in %s element: %s", node->GetName(), layer->GetName());
      continue;
    }

    UINT   drawLayer = 2;
    LPCSTR level = layer->GetAttributeByName("level");
    if (level && *level) {
      StringToDrawLayer(level, drawLayer);
    }

    const XMLNode *regionNode;
    for (regionNode = layer->GetChild(); regionNode; regionNode = regionNode->GetSibling()) {
      if (!SStrCmpI(regionNode->GetName(), "Texture", 0x7FFFFFFF)) {
        CSimpleTexture *texture = LoadXML_Texture(regionNode, this, status);
        texture->SetFrame(this, drawLayer, texture->IsVisible());
      } else if (!SStrCmpI(regionNode->GetName(), "FontString", 0x7FFFFFFF)) {
        CSimpleFontString *fontString = LoadXML_String(regionNode, this, status);
        fontString->SetFrame(this, drawLayer, fontString->IsVisible());
      } else {
        status->Add(STATUS_WARNING, "Unknown child node in %s element: %s", layer->GetName(), regionNode->GetName());
      }
    }
  }
}

void CSimpleFrame::LoadXML_Scripts(const XMLNode *node, CStatus *status) {
  const XMLNode *script;

  for (script = node->GetChild(); script; script = script->GetSibling()) {
    if (!SStrCmpI(script->GetName(), "OnLoad", 0x7FFFFFFF)) {
      SetOnLoadScript(script->GetBody());
    } else if (!SStrCmpI(script->GetName(), "OnSizeChanged", 0x7FFFFFFF)) {
      SetOnSizeChangedScript(script->GetBody());
    } else if (!SStrCmpI(script->GetName(), "OnEvent", 0x7FFFFFFF)) {
      SetOnEventScript(script->GetBody());
    } else if (!SStrCmpI(script->GetName(), "OnUpdate", 0x7FFFFFFF)) {
      SetOnUpdateScript(script->GetBody());
    } else if (!SStrCmpI(script->GetName(), "OnShow", 0x7FFFFFFF)) {
      SetOnShowScript(script->GetBody());
    } else if (!SStrCmpI(script->GetName(), "OnHide", 0x7FFFFFFF)) {
      SetOnHideScript(script->GetBody());
    } else if (!SStrCmpI(script->GetName(), "OnEnter", 0x7FFFFFFF)) {
      SetOnEnterScript(script->GetBody());
    } else if (!SStrCmpI(script->GetName(), "OnLeave", 0x7FFFFFFF)) {
      SetOnLeaveScript(script->GetBody());
    } else if (!SStrCmpI(script->GetName(), "OnMouseDown", 0x7FFFFFFF)) {
      SetOnMouseDownScript(script->GetBody());
    } else if (!SStrCmpI(script->GetName(), "OnMouseUp", 0x7FFFFFFF)) {
      SetOnMouseUpScript(script->GetBody());
    } else if (!SStrCmpI(script->GetName(), "OnMouseWheel", 0x7FFFFFFF)) {
      SetOnMouseWheelScript(script->GetBody());
    } else if (!SStrCmpI(script->GetName(), "OnDragStart", 0x7FFFFFFF)) {
      SetOnDragStartScript(script->GetBody());
    } else if (!SStrCmpI(script->GetName(), "OnDragStop", 0x7FFFFFFF)) {
      SetOnDragStopScript(script->GetBody());
    } else if (!SStrCmpI(script->GetName(), "OnReceiveDrag", 0x7FFFFFFF)) {
      SetOnReceiveDragScript(script->GetBody());
    } else if (!SStrCmpI(script->GetName(), "OnChar", 0x7FFFFFFF)) {
      SetOnCharScript(script->GetBody());
    } else if (!SStrCmpI(script->GetName(), "OnKeyDown", 0x7FFFFFFF)) {
      SetOnKeyDownScript(script->GetBody());
    } else if (!SStrCmpI(script->GetName(), "OnKeyUp", 0x7FFFFFFF)) {
      SetOnKeyUpScript(script->GetBody());
    }
  }
}

void CSimpleFrame::PostLoadXML(const XMLNode *node, CStatus *status) {
  m_loading = 0;
  RunOnLoadScript();

  if (m_visible) {
    if (IsResizeDeferred()) {
      SetDeferredResize(0);
    }

    RunOnShowScript();
  }
}

void CSimpleFrame::SetFrameFlag(int flag, int on) {
  if (on) {
    m_flags |= flag;
  } else {
    m_flags &= ~flag;
  }
}

void CSimpleFrame::SetBeingScrolled(int on) {
  SetFrameFlag(0x2000, on);

  if (on) {
    UINT layer;

    for (layer = 0; layer < NUM_SIMPLEFRAME_DRAWLAYERS; ++layer) {
      OnUpdateBatch(layer);
    }
  }

  ITERATELIST(SIMPLEFRAMENODE, m_children, node) {
    node->frame->SetBeingScrolled(on);
  }
}

void CSimpleFrame::SetFrameStrata(int strata) {
  ASSERT(strata >= 0 && strata < NUM_FRAME_STRATA);

  if (strata != m_strata) {
    m_top->UnregisterFrame(this);
    m_strata = strata;
    m_top->RegisterFrame(this);

    ITERATELIST(SIMPLEFRAMENODE, m_children, node) {
      node->frame->SetFrameStrata(strata);
    }
  }
}

void CSimpleFrame::SetFrameLevel(int level, int shiftChildren) {
  ASSERT(level >= 0);

  if (level != m_level) {
    int delta = level - m_level;

    m_top->UnregisterFrame(this);
    m_level += delta;
    m_top->RegisterFrame(this);

    if (shiftChildren) {
      ITERATELIST(SIMPLEFRAMENODE, m_children, node) {
        CSimpleFrame *child = node->frame;

        if (child->m_strata == m_strata) {
          child->SetFrameLevel(child->m_level + delta, 1);
        }
      }
    }
  }
}

void CSimpleFrame::Raise() {
  m_top->RaiseFrame(this, 1);
}

void CSimpleFrame::Lower() {
  m_top->LowerFrame(this);
}

void CSimpleFrame::SetBackdrop(CBackdropGenerator *backdrop) {
  ASSERT(backdrop);

  if (m_backdrop) {
    DEL(m_backdrop);
  }

  backdrop->SetOutput(this);
  m_backdrop = backdrop;
}

BOOL CSimpleFrame::SetHighlight(LPCSTR texFile, EGxBlend blendMode) {
  int             okay = 1;
  CSimpleTexture *texture = NEW(CSimpleTexture)(this, 4, 1);

  if (texture->SetTexture(texFile, 0)) {
    texture->SetAllPoints(this, 1);
    texture->SetBlendMode(blendMode);
  } else {
    DEL(texture);
    okay = 0;
  }

  return okay;
}

BOOL CSimpleFrame::SetHighlight(CSimpleTexture *texture, EGxBlend blendMode) {
  if (!texture) {
    return 0;
  }

  texture->SetFrame(this, 4, 1);
  texture->SetBlendMode(blendMode);
  return 1;
}

void CSimpleFrame::SetAlpha(BYTE alpha) {
  if (m_alpha != alpha) {
    m_alpha = alpha;

    {
      ITERATELIST(REGIONNODE, m_regions, regionNode) {
        regionNode->region->OnGxColorChanged();
      }
    }

    {
      ITERATELIST(SIMPLEFRAMENODE, m_children, frameNode) {
        frameNode->frame->SetAlpha(alpha);
      }
    }
  }
}

void CSimpleFrame::EnableDrawLayer(UINT drawlayer) {
  ASSERT(drawlayer < NUM_SIMPLEFRAME_DRAWLAYERS);
  m_drawenabled[drawlayer] = 1;
  NotifyDrawLayerChanged(drawlayer);
}

void CSimpleFrame::DisableDrawLayer(UINT drawlayer) {
  ASSERT(drawlayer < NUM_SIMPLEFRAME_DRAWLAYERS);
  m_drawenabled[drawlayer] = 0;
  NotifyDrawLayerChanged(drawlayer);
}

void CSimpleFrame::RegisterRegion(CSimpleRegion *region) {
  VALIDATEBEGIN;
  VALIDATE(region);
  VALIDATEENDVOID;

  REGIONNODE *node = m_regions.NewNode(LIST_TAIL, 0, 0);
  node->region = region;
}

void CSimpleFrame::UnregisterRegion(CSimpleRegion *region) {
  VALIDATEBEGIN;
  VALIDATE(region);
  VALIDATEENDVOID;

  ITERATELIST(REGIONNODE, m_regions, node) {
    if (node->region == region) {
      ITERATE_DELETEANDBREAK;
    }
  }
}

void CSimpleFrame::AddFrameRegion(CSimpleRegion *region, UINT drawlayer) {
  REGIONNODE *node;

  region->SetLayoutScale(m_layoutScale, false);
  node = m_drawlayers[drawlayer].NewNode(LIST_TAIL, 0, 0);
  node->region = region;
  NotifyDrawLayerChanged(drawlayer);
}

void CSimpleFrame::RemoveFrameRegion(CSimpleRegion *region, UINT drawlayer) {
  ITERATELIST(REGIONNODE, m_drawlayers[drawlayer], node) {
    if (node->region == region) {
      NotifyDrawLayerChanged(drawlayer);
      ITERATE_DELETEANDBREAK;
    }
  }
}

void CSimpleFrame::NotifyDrawLayerChanged(UINT drawlayer) {
  if (m_top && m_visible) {
    m_top->NotifyFrameLayerChanged(this, drawlayer);
  }
}

void CSimpleFrame::NotifyDrawLayersChanged() {
  if (m_top && m_visible) {
    UINT drawlayer;

    for (drawlayer = 0; drawlayer < NUM_SIMPLEFRAME_DRAWLAYERS; ++drawlayer) {
      m_top->NotifyFrameLayerChanged(this, drawlayer);
    }
  }
}

BOOL CSimpleFrame::AddToFrameRegistry(LPCSTR frameName, UINT context) {
  int okay = 0;

  if (m_frameName) {
    UnregisterScriptObject(m_frameName);
    SimpleFrameRegistryRemoveEntry(m_frameName, m_frameRegContext);
    FREE(m_frameName);
    m_frameName = 0;
  }

  if (frameName && *frameName) {
    if (SimpleFrameRegistryAddEntry(frameName, this, context)) {
      m_frameName = SStrDupA(frameName, __FILE__, __LINE__);
      m_frameRegContext = context;
      RegisterScriptObject(m_frameName);
      okay = 1;
    }
  }

  return okay;
}

void CSimpleFrame::ClearFromSimpleRegistry() {
  AddToFrameRegistry(0, 0);
}

void CSimpleFrame::ParentFrame(CSimpleFrame *frame) {
  SIMPLEFRAMENODE *node = m_children.NewNode(LIST_TAIL, 0, 0);

  node->frame = frame;
}

void CSimpleFrame::UnparentFrame(CSimpleFrame *frame) {
  ITERATELIST(SIMPLEFRAMENODE, m_children, node) {
    if (node->frame == frame) {
      ITERATE_DELETEANDBREAK;
    }
  }
}

void CSimpleFrame::ClearChildrenFromSimpleRegistry() {
  ClearFromSimpleRegistry();

  {
    ITERATELIST(REGIONNODE, m_regions, region) {
      ASSERT(region->region);
      region->region->ClearFromSimpleRegistry();
    }
  }

  {
    ITERATELIST(SIMPLEFRAMENODE, m_children, frame) {
      ASSERT(frame->frame);
      frame->frame->ClearChildrenFromSimpleRegistry();
    }
  }
}

void CSimpleFrame::SetParent(CSimpleFrame *parent) {
  if (parent == m_parent) {
    return;
  }

  int visible = m_visible;

  if (m_parent) {
    m_parent->UnparentFrame(this);
  }

  if (visible) {
    Hide();
  }

  m_parent = parent;

  if (m_parent) {
    SetFrameStrata(m_parent->GetFrameStrata());
    SetFrameLevel(m_parent->GetFrameLevel() + 1, 0);
    SetLayoutScale(m_parent->GetLayoutScale(), false);
    SetBeingScrolled(m_parent->IsBeingScrolled());
  } else {
    SetFrameStrata(2);
    SetFrameLevel(0, 0);
    SetLayoutScale(1.0f, false);
    SetBeingScrolled(0);
  }

  if (m_parent) {
    m_parent->ParentFrame(this);
  }

  if (visible && (!m_parent || m_parent->IsVisible())) {
    Show();
  }
}

void CSimpleFrame::SetDeferredResize(int enable) {
  if (enable) {
    CLayoutFrame::m_flags |= 0x2;
  } else {
    CLayoutFrame::m_flags &= ~0x2U;
  }

  ITERATELIST(REGIONNODE, m_regions, node) {
    node->region->SetDeferredResize(enable);
  }

  CLayoutFrame::SetDeferredResize(enable);
}

void CSimpleFrame::SetLayoutScale(float scale, bool force) {
  if (force || NTempest::CMath::fnotequal_(scale, m_layoutScale)) {
    if (!m_visible) {
      SetDeferredResize(1);
    }

    CLayoutFrame::SetLayoutScale(scale, force);

    {
      ITERATELIST(REGIONNODE, m_regions, regionNode) {
        regionNode->region->SetLayoutScale(scale, force);
      }
    }

    {
      ITERATELIST(SIMPLEFRAMENODE, m_children, frameNode) {
        frameNode->frame->SetLayoutScale(scale, force);
      }
    }
  }
}

BOOL CSimpleFrame::HideThis() {
  if (m_visible) {
    if (this == m_top->m_mouseFocus) {
      OnLayerCursorExit();
    }

    UnregisterForEvents();
    NotifyDrawLayersChanged();
    m_visible = 0;
    OnLayerHide();

    ITERATELIST(SIMPLEFRAMENODE, m_children, node) {
      node->frame->HideThis();
    }
  }

  return 1;
}

BOOL CSimpleFrame::ShowThis() {
  int shown = 0;

  if (m_shown && IsParentDrawn()) {
    if (!m_visible) {
      if (IsResizeDeferred()) {
        SetDeferredResize(0);
      }

      m_visible = 1;
      OnLayerShow();
      RegisterForEvents();
      NotifyDrawLayersChanged();

      ITERATELIST(SIMPLEFRAMENODE, m_children, node) {
        node->frame->ShowThis();
      }

      if (IsToplevel()) {
        Raise();
      }
    }

    shown = 1;
  }

  return shown;
}

void CSimpleFrame::EnableEvent(CSimpleEventType event, UINT priority) {
  UINT eventbit = 1 << event;

  ASSERT(event < sizeof(m_eventmask)*8);

  if (!(m_eventmask & eventbit)) {
    if (m_visible) {
      m_top->RegisterForEvent(this, event, priority);
    }

    m_eventmask |= eventbit;
  }
}

void CSimpleFrame::DisableEvent(CSimpleEventType event) {
  UINT eventbit = 1 << event;

  if (m_eventmask & eventbit) {
    if (m_visible) {
      m_top->UnregisterForEvent(this, event);
    }

    m_eventmask &= ~eventbit;
  }
}

void CSimpleFrame::RegisterForEvents() {
  UINT event;

  for (event = 0; event < NUM_SIMPLE_EVENTS; ++event) {
    if (m_eventmask & (1 << event)) {
      m_top->RegisterForEvent(this, static_cast<CSimpleEventType>(event), static_cast<UINT>(-1));
    }
  }
}

void CSimpleFrame::UnregisterForEvents() {
  UINT event;

  for (event = 0; event < NUM_SIMPLE_EVENTS; ++event) {
    if (m_eventmask & (1 << event)) {
      m_top->UnregisterForEvent(this, static_cast<CSimpleEventType>(event));
    }
  }
}

BOOL CSimpleFrame::TestHitRect(const NTempest::C2Vector &pt) {
  if (!(CLayoutFrame::m_flags & 0x1)) {
    return 0;
  }

  if (m_flags & 0x2000) {
    CSimpleFrame *parent = m_parent;

    while (parent && (parent->m_flags & 0x2000)) {
      parent = parent->m_parent;
    }

    if (!parent) {
      return 0;
    }

    NTempest::CRect rect = parent->m_hitRect;
    rect.Intersect(m_hitRect);

    return rect.Contains(pt);
  }

  return m_hitRect.Contains(pt);
}

void CSimpleFrame::SetHitRect(const NTempest::CRect &rect) {
  m_hitRect.l = m_hitOffset.l + rect.l;
  m_hitRect.r = rect.r - m_hitOffset.r;
  m_hitRect.t = m_hitOffset.t + rect.t;
  m_hitRect.b = rect.b - m_hitOffset.b;
}

void CSimpleFrame::SetHitRectInsets(float left, float right, float top, float bottom) {
  m_hitOffset.l = left;
  m_hitOffset.r = right;
  m_hitOffset.b = top;
  m_hitOffset.t = bottom;
}

BOOL CSimpleFrame::GetHitRect(NTempest::CRect &rect) {
  if (CLayoutFrame::m_flags & 0x1) {
    rect = m_hitRect;
    return 1;
  }

  return 0;
}

void CSimpleFrame::OnLayerShow() {
  RunOnShowScript();
}

void CSimpleFrame::OnLayerHide() {
  RunOnHideScript();
}

void CSimpleFrame::OnLayerUpdate(float elapsedSec) {
  RunOnUpdateScript(elapsedSec);
}

BOOL CSimpleFrame::OnLayerTrackUpdate(const CMouseEvent &evt) {
  NTempest::C2Vector pt(evt.x, evt.y);

  if (m_mouseDown && !m_dragging) {
    float dx = pt.x - m_clickPoint.x;
    float dy = pt.y - m_clickPoint.y;

    if (dx * dx + dy * dy >= MIN_DRAG_DIST_SQ) {
      m_dragging = 1;
      OnDragStart(const_cast<CMouseEvent &>(evt));
    }
  }

  return TestHitRect(pt);
}

void CSimpleFrame::OnFrameRender(CRenderBatch *batch, UINT layer) {
  ASSERT(layer < NUM_SIMPLEFRAME_DRAWLAYERS);

  if (m_drawenabled[layer]) {
    ITERATELIST(REGIONNODE, m_drawlayers[layer], node) {
      node->region->Draw(batch);
    }
  }
}

void CSimpleFrame::OnFrameRender() {
  UINT layer;

  if (m_batchDirty) {
    for (layer = 0; layer < NUM_SIMPLEFRAME_DRAWLAYERS; ++layer) {
      CRenderBatch *batch = &m_batch[layer];

      if (m_renderList.IsLinked(batch)) {
        m_renderList.UnlinkNode(batch);
      }

      if (m_batchDirty & (1 << layer)) {
        batch->Clear();
        OnFrameRender(batch, layer);
        batch->Finish();
      }

      if (batch->Count() > 0) {
        m_renderList.LinkNode(batch, LIST_TAIL, 0);
      }
    }

    m_batchDirty = 0;
  }

  for (layer = 0; layer < NUM_SIMPLEFRAME_DRAWLAYERS; ++layer) {
    CSimpleRender::DrawBatch(&m_batch[layer]);
  }

  ITERATELIST(SIMPLEFRAMENODE, m_children, node) {
    if (node->frame->IsVisible()) {
      node->frame->OnFrameRender();
    }
  }
}

void CSimpleFrame::OnUpdateBatch(UINT layer) {
  m_batchDirty |= 1 << layer;
}

void CSimpleFrame::OnFrameSizeChanged(const NTempest::CRect &rect) {
  CLayoutFrame::OnFrameSizeChanged(rect);

  if (NTempest::CMath::fnotequal_(rect.l, m_rect.l) || NTempest::CMath::fnotequal_(rect.r, m_rect.r) ||
      NTempest::CMath::fnotequal_(rect.t, m_rect.t) || NTempest::CMath::fnotequal_(rect.b, m_rect.b))
  {
    SetHitRect(rect);

    if (NTempest::CMath::fnotequal_(rect.r - rect.l, m_rect.r - m_rect.l) || NTempest::CMath::fnotequal_(rect.b - rect.t, m_rect.b - m_rect.t)) {
      if (m_backdrop) {
        m_backdrop->Generate(&rect);
      }

      OnFrameSizeChanged(rect.r - rect.l, rect.b - rect.t);
    }

    m_top->NotifyFrameMovedOrResized(this);
  }
}

void CSimpleFrame::OnFrameSizeChanged(float w, float h) {
  RunOnSizeChangedScript(w, h);
}

void CSimpleFrame::OnLayerCursorEnter() {
  if (m_tooltip) {
    m_tooltip->Show();
  }

  if (!m_highlightLocked) {
    EnableDrawLayer(4);
  }

  RunOnEnterScript();
}

void CSimpleFrame::OnLayerCursorExit() {
  if (m_tooltip) {
    m_tooltip->Hide();
  }

  if (!m_highlightLocked) {
    DisableDrawLayer(4);
  }

  if (m_mouseDown && !m_dragging) {
    m_dragging = 1;

    CMouseEvent evt;

    evt.button = m_dragButton;
    evt.x = m_clickPoint.x;
    evt.y = m_clickPoint.y;
    OnDragStart(evt);
  }

  RunOnLeaveScript();
}

BOOL CSimpleFrame::OnLayerChar(CCharEvent &evt) {
  char utf8string[6];

  if (!m_visible || !m_onChar) {
    return 0;
  }

  sputu8(evt.ch, utf8string);
  RunOnCharScript(utf8string);
  return 1;
}

BOOL CSimpleFrame::OnLayerKeyDown(CKeyEvent &evt) {
  LPCSTR keyName;

  if (!m_visible || !m_onKeyDown) {
    return 0;
  }

  if ((evt.key >= KEY_0 && evt.key <= KEY_9) || (evt.key >= KEY_A && evt.key <= KEY_Z)) {
    SStrPrintf(charBuf, sizeof(charBuf), "%c", evt.key);
    keyName = charBuf;
  } else if (evt.key >= KEY_NUMPAD0 && evt.key <= KEY_NUMPAD9) {
    SStrPrintf(charBuf, sizeof(charBuf), "NUMPAD%d", evt.key - KEY_NUMPAD0);
    keyName = charBuf;
  } else if (evt.key >= KEY_F1 && evt.key <= KEY_F12) {
    SStrPrintf(charBuf, sizeof(charBuf), "F%d", evt.key - KEY_F1 + 1);
    keyName = charBuf;
  } else {
    switch (evt.key) {
      case KEY_SHIFT:
        keyName = "SHIFT";
        break;
      case KEY_CONTROL:
        keyName = "CTRL";
        break;
      case KEY_ALT:
        keyName = "ALT";
        break;
      case KEY_SPACE:
        keyName = "SPACE";
        break;
      case KEY_TILDE:
        keyName = "TILDE";
        break;
      case KEY_NUMPAD_PLUS:
        keyName = "NUMPADPLUS";
        break;
      case KEY_NUMPAD_MINUS:
        keyName = "NUMPADMINUS";
        break;
      case KEY_NUMPAD_MULTIPLY:
        keyName = "NUMPADMULTIPLY";
        break;
      case KEY_NUMPAD_DIVIDE:
        keyName = "NUMPADDIVIDE";
        break;
      case KEY_PLUS:
        keyName = "PLUS";
        break;
      case KEY_MINUS:
        keyName = "MINUS";
        break;
      case KEY_BRACKET_OPEN:
        keyName = "LEFTBRACKET";
        break;
      case KEY_BRACKET_CLOSE:
        keyName = "RIGHTBRACKET";
        break;
      case KEY_SLASH:
        keyName = "SLASH";
        break;
      case KEY_BACKSLASH:
        keyName = "BACKSLASH";
        break;
      case KEY_SEMICOLON:
        keyName = "SEMICOLON";
        break;
      case KEY_APOSTROPHE:
        keyName = "APOSTROPHE";
        break;
      case KEY_COMMA:
        keyName = "COMMA";
        break;
      case KEY_PERIOD:
        keyName = "PERIOD";
        break;
      case KEY_ESCAPE:
        keyName = "ESCAPE";
        break;
      case KEY_ENTER:
        keyName = "ENTER";
        break;
      case KEY_BACKSPACE:
        keyName = "BACKSPACE";
        break;
      case KEY_TAB:
        keyName = "TAB";
        break;
      case KEY_LEFT:
        keyName = "LEFT";
        break;
      case KEY_UP:
        keyName = "UP";
        break;
      case KEY_RIGHT:
        keyName = "RIGHT";
        break;
      case KEY_DOWN:
        keyName = "DOWN";
        break;
      case KEY_INSERT:
        keyName = "INSERT";
        break;
      case KEY_DELETE:
        keyName = "DELETE";
        break;
      case KEY_HOME:
        keyName = "HOME";
        break;
      case KEY_END:
        keyName = "END";
        break;
      case KEY_PAGEUP:
        keyName = "PAGEUP";
        break;
      case KEY_PAGEDOWN:
        keyName = "PAGEDOWN";
        break;
      case KEY_NUMLOCK:
        keyName = "NUMLOCK";
        break;
      case KEY_CAPSLOCK:
        keyName = "CAPSLOCK";
        break;
      case KEY_SCROLLLOCK:
        keyName = "SCROLLLOCK";
        break;
      case KEY_PAUSE:
        keyName = "PAUSE";
        break;
      case KEY_PRINTSCREEN:
        keyName = "PRINTSCREEN";
        break;
      default:
        keyName = "UNKNOWN";
        break;
    }
  }

  RunOnKeyDownScript(keyName);
  return 1;
}

BOOL CSimpleFrame::OnLayerKeyUp(CKeyEvent &evt) {
  LPCSTR keyName;

  if (!m_visible || !m_onKeyUp) {
    return 0;
  }

  if ((evt.key >= KEY_0 && evt.key <= KEY_9) || (evt.key >= KEY_A && evt.key <= KEY_Z)) {
    SStrPrintf(charBuf, sizeof(charBuf), "%c", evt.key);
    keyName = charBuf;
  } else if (evt.key >= KEY_NUMPAD0 && evt.key <= KEY_NUMPAD9) {
    SStrPrintf(charBuf, sizeof(charBuf), "NUMPAD%d", evt.key - KEY_NUMPAD0);
    keyName = charBuf;
  } else if (evt.key >= KEY_F1 && evt.key <= KEY_F12) {
    SStrPrintf(charBuf, sizeof(charBuf), "F%d", evt.key - KEY_F1 + 1);
    keyName = charBuf;
  } else {
    switch (evt.key) {
      case KEY_SHIFT:
        keyName = "SHIFT";
        break;
      case KEY_CONTROL:
        keyName = "CTRL";
        break;
      case KEY_ALT:
        keyName = "ALT";
        break;
      case KEY_SPACE:
        keyName = "SPACE";
        break;
      case KEY_TILDE:
        keyName = "TILDE";
        break;
      case KEY_NUMPAD_PLUS:
        keyName = "NUMPADPLUS";
        break;
      case KEY_NUMPAD_MINUS:
        keyName = "NUMPADMINUS";
        break;
      case KEY_NUMPAD_MULTIPLY:
        keyName = "NUMPADMULTIPLY";
        break;
      case KEY_NUMPAD_DIVIDE:
        keyName = "NUMPADDIVIDE";
        break;
      case KEY_PLUS:
        keyName = "PLUS";
        break;
      case KEY_MINUS:
        keyName = "MINUS";
        break;
      case KEY_BRACKET_OPEN:
        keyName = "LEFTBRACKET";
        break;
      case KEY_BRACKET_CLOSE:
        keyName = "RIGHTBRACKET";
        break;
      case KEY_SLASH:
        keyName = "SLASH";
        break;
      case KEY_BACKSLASH:
        keyName = "BACKSLASH";
        break;
      case KEY_SEMICOLON:
        keyName = "SEMICOLON";
        break;
      case KEY_APOSTROPHE:
        keyName = "APOSTROPHE";
        break;
      case KEY_COMMA:
        keyName = "COMMA";
        break;
      case KEY_PERIOD:
        keyName = "PERIOD";
        break;
      case KEY_ESCAPE:
        keyName = "ESCAPE";
        break;
      case KEY_ENTER:
        keyName = "ENTER";
        break;
      case KEY_BACKSPACE:
        keyName = "BACKSPACE";
        break;
      case KEY_TAB:
        keyName = "TAB";
        break;
      case KEY_LEFT:
        keyName = "LEFT";
        break;
      case KEY_UP:
        keyName = "UP";
        break;
      case KEY_RIGHT:
        keyName = "RIGHT";
        break;
      case KEY_DOWN:
        keyName = "DOWN";
        break;
      case KEY_INSERT:
        keyName = "INSERT";
        break;
      case KEY_DELETE:
        keyName = "DELETE";
        break;
      case KEY_HOME:
        keyName = "HOME";
        break;
      case KEY_END:
        keyName = "END";
        break;
      case KEY_PAGEUP:
        keyName = "PAGEUP";
        break;
      case KEY_PAGEDOWN:
        keyName = "PAGEDOWN";
        break;
      case KEY_NUMLOCK:
        keyName = "NUMLOCK";
        break;
      case KEY_CAPSLOCK:
        keyName = "CAPSLOCK";
        break;
      case KEY_SCROLLLOCK:
        keyName = "SCROLLLOCK";
        break;
      case KEY_PAUSE:
        keyName = "PAUSE";
        break;
      case KEY_PRINTSCREEN:
        keyName = "PRINTSCREEN";
        break;
      default:
        keyName = "UNKNOWN";
        break;
    }
  }

  RunOnKeyUpScript(keyName);
  return 1;
}

BOOL CSimpleFrame::OnLayerMouseDown(CMouseEvent &evt) {
  if (m_lookForDrag & evt.button) {
    m_mouseDown = 1;
    m_dragging = 0;
    m_dragButton = evt.button;
    m_clickPoint.Set(evt.x, evt.y);
  }

  RunOnMouseDownScript(evt.button);
  return 0;
}

BOOL CSimpleFrame::OnLayerMouseUp(CMouseEvent &evt) {
  if (m_lookForDrag & evt.button) {
    int dragging = m_dragging;

    m_mouseDown = 0;
    if (dragging) {
      OnDragStop(evt);
      if (m_top->m_mouseFocus) {
        m_top->m_mouseFocus->OnReceiveDrag(evt);
      }
      m_dragging = 0;
      return 1;
    }
  }

  RunOnMouseUpScript(evt.button);
  return 0;
}

BOOL CSimpleFrame::OnLayerMouseWheel(CMouseEvent &evt) {
  if (!m_visible || !m_onMouseWheel) {
    return 0;
  }

  RunOnMouseWheelScript(evt.wheelDistance >= 0 ? 1 : -1);
  return 1;
}

void CSimpleFrame::OnDragStart(CMouseEvent &evt) {
  RunOnDragStartScript(evt.button);
}

void CSimpleFrame::OnDragStop(CMouseEvent &evt) {
  RunOnDragStopScript();
}

void CSimpleFrame::OnReceiveDrag(CMouseEvent &evt) {
  RunOnReceiveDragScript();
}
