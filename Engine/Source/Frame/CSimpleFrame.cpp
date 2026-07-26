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

  m_drawenabled[0] = 1;
  m_drawenabled[1] = 1;
  m_drawenabled[2] = 1;
  m_drawenabled[3] = 1;
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

  REGIONNODE *region;
  while ((region = m_regions.Head()) != 0) {
    if (region->region) {
      DEL(region->region);
    }
  }

  unsigned int layer;
  for (layer = 0; layer < NUM_SIMPLEFRAME_DRAWLAYERS; ++layer) {
    ASSERT(m_drawlayers[layer].IsEmpty());
  }

  m_renderList.UnlinkAll();

  SIMPLEFRAMENODE *frame;
  while ((frame = m_children.Head()) != 0) {
    if (frame->frame) {
      DEL(frame->frame);
    }
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

CLayoutFrame *CSimpleFrame::GetLayoutFrameByName(const char *name) {
  char newName[1024];

  if (!SStrCmpI(name, "$parent", SStrLen("$parent"))) {
    CSimpleFrame *parent;

    SStrCopy(newName, "Top", 0x7FFFFFFF);
    for (parent = m_parent; parent; parent = parent->m_parent) {
      const char *parentName = parent->GetName();

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
  const char *frameName = node->GetAttributeByName("name");

  if (frameName && *frameName) {
    char name[1024];

    if (!SStrCmpI(frameName, "$parent", SStrLen("$parent"))) {
      CSimpleFrame *parent;

      SStrCopy(name, "Top", 0x7FFFFFFF);
      for (parent = m_parent; parent; parent = parent->m_parent) {
        const char *parentName = parent->GetName();

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
  const char *attribute;

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
    SetAlpha(static_cast<unsigned char>(min(max(SStrToFloat(attribute), 0.0f), 1.0f) * 255.0f));
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
    EnableEvent(SIMPLE_EVENT_MOUSE, static_cast<unsigned int>(-1));
  }

  attribute = node->GetAttributeByName("enableKeyboard");
  if (attribute && *attribute && StringToBOOL(attribute)) {
    EnableEvent(SIMPLE_EVENT_KEY, static_cast<unsigned int>(-1));
    EnableEvent(SIMPLE_EVENT_CHAR, static_cast<unsigned int>(-1));
  }

  const XMLNode *child;
  for (child = node->GetChild(); child; child = child->GetSibling()) {
    const char *childName = child->GetName();

    if (!SStrCmpI(childName, "TitleRegion", 0x7FFFFFFF)) {
      CSimpleTitleRegion *titleRegion = NEW(CSimpleTitleRegion);

      titleRegion->m_parent = this;
      titleRegion->LoadXML(child, status);
      SetTitleRegion(titleRegion);
    } else if (!SStrCmpI(childName, "Backdrop", 0x7FFFFFFF)) {
      CBackdropGenerator *backdrop = NEW(CBackdropGenerator);

      backdrop->LoadXML(child, status);
      SetBackdrop(backdrop);
    } else if (!SStrCmpI(childName, "HitRectInsets", 0x7FFFFFFF)) {
      float left;
      float right;
      float top;
      float bottom;

      if (LoadXML_Insets(child, left, right, top, bottom, status)) {
        SetHitRectInsets(left, right, top, bottom);
      }
    } else if (!SStrCmpI(childName, "Layers", 0x7FFFFFFF)) {
      LoadXML_Layers(child, status);
    } else if (!SStrCmpI(childName, "Scripts", 0x7FFFFFFF)) {
      const char *name = GetName();

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

    unsigned int drawLayer = 2;
    const char  *level = layer->GetAttributeByName("level");
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
    const char *name = script->GetName();
    const char *source = script->GetBody();

    if (!SStrCmpI(name, "OnLoad", 0x7FFFFFFF)) {
      SetOnLoadScript(source);
    } else if (!SStrCmpI(name, "OnSizeChanged", 0x7FFFFFFF)) {
      SetOnSizeChangedScript(source);
    } else if (!SStrCmpI(name, "OnEvent", 0x7FFFFFFF)) {
      SetOnEventScript(source);
    } else if (!SStrCmpI(name, "OnUpdate", 0x7FFFFFFF)) {
      SetOnUpdateScript(source);
    } else if (!SStrCmpI(name, "OnShow", 0x7FFFFFFF)) {
      SetOnShowScript(source);
    } else if (!SStrCmpI(name, "OnHide", 0x7FFFFFFF)) {
      SetOnHideScript(source);
    } else if (!SStrCmpI(name, "OnEnter", 0x7FFFFFFF)) {
      SetOnEnterScript(source);
    } else if (!SStrCmpI(name, "OnLeave", 0x7FFFFFFF)) {
      SetOnLeaveScript(source);
    } else if (!SStrCmpI(name, "OnMouseDown", 0x7FFFFFFF)) {
      SetOnMouseDownScript(source);
    } else if (!SStrCmpI(name, "OnMouseUp", 0x7FFFFFFF)) {
      SetOnMouseUpScript(source);
    } else if (!SStrCmpI(name, "OnMouseWheel", 0x7FFFFFFF)) {
      SetOnMouseWheelScript(source);
    } else if (!SStrCmpI(name, "OnDragStart", 0x7FFFFFFF)) {
      SetOnDragStartScript(source);
    } else if (!SStrCmpI(name, "OnDragStop", 0x7FFFFFFF)) {
      SetOnDragStopScript(source);
    } else if (!SStrCmpI(name, "OnReceiveDrag", 0x7FFFFFFF)) {
      SetOnReceiveDragScript(source);
    } else if (!SStrCmpI(name, "OnChar", 0x7FFFFFFF)) {
      SetOnCharScript(source);
    } else if (!SStrCmpI(name, "OnKeyDown", 0x7FFFFFFF)) {
      SetOnKeyDownScript(source);
    } else if (!SStrCmpI(name, "OnKeyUp", 0x7FFFFFFF)) {
      SetOnKeyUpScript(source);
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
  SIMPLEFRAMENODE *node;

  SetFrameFlag(0x2000, on);

  if (on) {
    unsigned int layer;

    for (layer = 0; layer < NUM_SIMPLEFRAME_DRAWLAYERS; ++layer) {
      OnUpdateBatch(layer);
    }
  }

  for (node = m_children.Head(); node; node = node->m_link.Next()) {
    node->frame->SetBeingScrolled(on);
  }
}

void CSimpleFrame::SetFrameStrata(int strata) {
  ASSERT(strata >= 0 && strata < NUM_FRAME_STRATA);

  if (strata != m_strata) {
    SIMPLEFRAMENODE *node;

    m_top->UnregisterFrame(this);
    m_strata = strata;
    m_top->RegisterFrame(this);

    for (node = m_children.Head(); node; node = node->m_link.Next()) {
      node->frame->SetFrameStrata(strata);
    }
  }
}

void CSimpleFrame::SetFrameLevel(int level, int shiftChildren) {
  int delta;

  ASSERT(level >= 0);
  delta = level - m_level;

  if (delta) {
    SIMPLEFRAMENODE *node;

    m_top->UnregisterFrame(this);
    m_level += delta;
    m_top->RegisterFrame(this);

    if (shiftChildren) {
      for (node = m_children.Head(); node; node = node->m_link.Next()) {
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

int CSimpleFrame::SetHighlight(const char *texFile, EGxBlend blendMode) {
  CSimpleTexture *texture = NEW(CSimpleTexture)(this, 4, 1);

  if (texture->SetTexture(texFile, 0)) {
    texture->SetAllPoints(this, 1);
    texture->SetBlendMode(blendMode);
    return 1;
  }

  DEL(texture);
  return 0;
}

int CSimpleFrame::SetHighlight(CSimpleTexture *texture, EGxBlend blendMode) {
  if (!texture) {
    return 0;
  }

  texture->SetFrame(this, 4, 1);
  texture->SetBlendMode(blendMode);
  return 1;
}

void CSimpleFrame::SetAlpha(unsigned char alpha) {
  if (alpha != m_alpha) {
    REGIONNODE      *regionNode;
    SIMPLEFRAMENODE *frameNode;

    m_alpha = alpha;

    for (regionNode = m_regions.Head(); regionNode; regionNode = regionNode->m_link.Next()) {
      regionNode->region->OnGxColorChanged();
    }

    for (frameNode = m_children.Head(); frameNode; frameNode = frameNode->m_link.Next()) {
      frameNode->frame->SetAlpha(alpha);
    }
  }
}

void CSimpleFrame::EnableDrawLayer(unsigned int drawlayer) {
  ASSERT(drawlayer < NUM_SIMPLEFRAME_DRAWLAYERS);
  m_drawenabled[drawlayer] = 1;
  NotifyDrawLayerChanged(drawlayer);
}

void CSimpleFrame::DisableDrawLayer(unsigned int drawlayer) {
  ASSERT(drawlayer < NUM_SIMPLEFRAME_DRAWLAYERS);
  m_drawenabled[drawlayer] = 0;
  NotifyDrawLayerChanged(drawlayer);
}

void CSimpleFrame::RegisterRegion(CSimpleRegion *region) {
  FATALASSERT(region);

  REGIONNODE *node = m_regions.NewNode(LIST_TAIL, 0, 0);
  node->region = region;
}

void CSimpleFrame::UnregisterRegion(CSimpleRegion *region) {
  REGIONNODE *node;

  FATALASSERT(region);

  for (node = m_regions.Head(); node; node = m_regions.Next(node)) {
    if (node->region == region) {
      m_regions.DeleteNode(node);
      break;
    }
  }
}

void CSimpleFrame::AddFrameRegion(CSimpleRegion *region, unsigned int drawlayer) {
  REGIONNODE *node;

  region->SetLayoutScale(m_layoutScale, false);
  node = m_drawlayers[drawlayer].NewNode(LIST_TAIL, 0, 0);
  node->region = region;
  NotifyDrawLayerChanged(drawlayer);
}

void CSimpleFrame::RemoveFrameRegion(CSimpleRegion *region, unsigned int drawlayer) {
  REGIONNODE *node;

  for (node = m_drawlayers[drawlayer].Head(); node; node = m_drawlayers[drawlayer].Next(node)) {
    if (node->region == region) {
      NotifyDrawLayerChanged(drawlayer);
      m_drawlayers[drawlayer].DeleteNode(node);
      break;
    }
  }
}

void CSimpleFrame::NotifyDrawLayerChanged(unsigned int drawlayer) {
  if (m_top && m_visible) {
    m_top->NotifyFrameLayerChanged(this, drawlayer);
  }
}

void CSimpleFrame::NotifyDrawLayersChanged() {
  if (m_top && m_visible) {
    unsigned int drawlayer;

    for (drawlayer = 0; drawlayer < NUM_SIMPLEFRAME_DRAWLAYERS; ++drawlayer) {
      m_top->NotifyFrameLayerChanged(this, drawlayer);
    }
  }
}

int CSimpleFrame::AddToFrameRegistry(const char *frameName, unsigned int context) {
  if (m_frameName) {
    UnregisterScriptObject(m_frameName);
    SimpleFrameRegistryRemoveEntry(m_frameName, m_frameRegContext);
    FREE(m_frameName);
    m_frameName = 0;
  }

  if (!frameName || !*frameName) {
    return 0;
  }

  if (!SimpleFrameRegistryAddEntry(frameName, this, context)) {
    return 0;
  }

  m_frameName = SStrDupA(frameName, __FILE__, __LINE__);
  m_frameRegContext = context;
  RegisterScriptObject(m_frameName);
  return 1;
}

void CSimpleFrame::ClearFromSimpleRegistry() {
  AddToFrameRegistry(0, 0);
}

void CSimpleFrame::ParentFrame(CSimpleFrame *frame) {
  SIMPLEFRAMENODE *node = m_children.NewNode(LIST_TAIL, 0, 0);

  node->frame = frame;
}

void CSimpleFrame::UnparentFrame(CSimpleFrame *frame) {
  SIMPLEFRAMENODE *node;

  for (node = m_children.Head(); node; node = node->m_link.Next()) {
    if (node->frame == frame) {
      m_children.DeleteNode(node);
      break;
    }
  }
}

void CSimpleFrame::ClearChildrenFromSimpleRegistry() {
  REGIONNODE      *region;
  SIMPLEFRAMENODE *frame;

  ClearFromSimpleRegistry();

  for (region = m_regions.Head(); region; region = region->m_link.Next()) {
    ASSERT(region->region);
    region->region->ClearFromSimpleRegistry();
  }

  for (frame = m_children.Head(); frame; frame = frame->m_link.Next()) {
    ASSERT(frame->frame);
    frame->frame->ClearChildrenFromSimpleRegistry();
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
  REGIONNODE *node;

  if (enable) {
    CLayoutFrame::m_flags |= 0x2;
  } else {
    CLayoutFrame::m_flags &= ~0x2U;
  }

  for (node = m_regions.Head(); node; node = node->m_link.Next()) {
    node->region->SetDeferredResize(enable);
  }

  CLayoutFrame::SetDeferredResize(enable);
}

void CSimpleFrame::SetLayoutScale(float scale, bool force) {
  static const float EPSILON = 2.38418579e-7f;

  if (force || fabs(scale - m_layoutScale) >= EPSILON) {
    REGIONNODE      *regionNode;
    SIMPLEFRAMENODE *frameNode;

    if (!m_visible) {
      SetDeferredResize(1);
    }

    CLayoutFrame::SetLayoutScale(scale, force);

    for (regionNode = m_regions.Head(); regionNode; regionNode = regionNode->m_link.Next()) {
      regionNode->region->SetLayoutScale(scale, force);
    }

    for (frameNode = m_children.Head(); frameNode; frameNode = frameNode->m_link.Next()) {
      frameNode->frame->SetLayoutScale(scale, force);
    }
  }
}

int CSimpleFrame::HideThis() {
  if (m_visible) {
    SIMPLEFRAMENODE *node;

    if (this == m_top->m_mouseFocus) {
      OnLayerCursorExit();
    }

    UnregisterForEvents();
    NotifyDrawLayersChanged();
    m_visible = 0;
    OnLayerHide();

    for (node = m_children.Head(); node; node = node->m_link.Next()) {
      node->frame->HideThis();
    }
  }

  return 1;
}

int CSimpleFrame::ShowThis() {
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

      SIMPLEFRAMENODE *node;
      for (node = m_children.Head(); node; node = node->m_link.Next()) {
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

void CSimpleFrame::EnableEvent(CSimpleEventType event, unsigned int priority) {
  unsigned int eventbit = 1 << event;

  ASSERT(event < sizeof(m_eventmask) * 8);

  if (!(m_eventmask & eventbit)) {
    if (m_visible) {
      m_top->RegisterForEvent(this, event, priority);
    }

    m_eventmask |= eventbit;
  }
}

void CSimpleFrame::DisableEvent(CSimpleEventType event) {
  unsigned int eventbit = 1 << event;

  if (m_eventmask & eventbit) {
    if (m_visible) {
      m_top->UnregisterForEvent(this, event);
    }

    m_eventmask &= ~eventbit;
  }
}

void CSimpleFrame::RegisterForEvents() {
  unsigned int event;

  for (event = 0; event < NUM_SIMPLE_EVENTS; ++event) {
    if (m_eventmask & (1 << event)) {
      m_top->RegisterForEvent(this, static_cast<CSimpleEventType>(event), static_cast<unsigned int>(-1));
    }
  }
}

void CSimpleFrame::UnregisterForEvents() {
  unsigned int event;

  for (event = 0; event < NUM_SIMPLE_EVENTS; ++event) {
    if (m_eventmask & (1 << event)) {
      m_top->UnregisterForEvent(this, static_cast<CSimpleEventType>(event));
    }
  }
}

int CSimpleFrame::TestHitRect(const NTempest::C2Vector &pt) {
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
    rect = NTempest::CRect(
        m_hitRect.t > rect.t ? m_hitRect.t : rect.t, m_hitRect.l > rect.l ? m_hitRect.l : rect.l, m_hitRect.b < rect.b ? m_hitRect.b : rect.b,
        m_hitRect.r < rect.r ? m_hitRect.r : rect.r
    );

    return pt.x >= rect.l && pt.x <= rect.r && pt.y >= rect.t && pt.y <= rect.b;
  }

  return pt.x >= m_hitRect.l && pt.x <= m_hitRect.r && pt.y >= m_hitRect.t && pt.y <= m_hitRect.b;
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

int CSimpleFrame::GetHitRect(NTempest::CRect &rect) {
  if (!(CLayoutFrame::m_flags & 0x1)) {
    return 0;
  }

  rect = m_hitRect;
  return 1;
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

int CSimpleFrame::OnLayerTrackUpdate(const CMouseEvent &evt) {
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

void CSimpleFrame::OnFrameRender(CRenderBatch *batch, unsigned int layer) {
  ASSERT(layer < NUM_SIMPLEFRAME_DRAWLAYERS);

  if (m_drawenabled[layer]) {
    REGIONNODE *node;

    for (node = m_drawlayers[layer].Head(); node; node = m_drawlayers[layer].Next(node)) {
      node->region->Draw(batch);
    }
  }
}

void CSimpleFrame::OnFrameRender() {
  unsigned int layer;

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

      if (batch->Count()) {
        m_renderList.LinkNode(batch, LIST_TAIL, 0);
      }
    }

    m_batchDirty = 0;
  }

  for (layer = 0; layer < NUM_SIMPLEFRAME_DRAWLAYERS; ++layer) {
    CSimpleRender::DrawBatch(&m_batch[layer]);
  }

  SIMPLEFRAMENODE *node;
  for (node = m_children.Head(); node; node = m_children.Next(node)) {
    if (node->frame->IsVisible()) {
      node->frame->OnFrameRender();
    }
  }
}

void CSimpleFrame::OnUpdateBatch(unsigned int layer) {
  m_batchDirty |= 1 << layer;
}

void CSimpleFrame::OnFrameSizeChanged(const NTempest::CRect &rect) {
  static const float EPSILON = 2.38418579e-7f;

  CLayoutFrame::OnFrameSizeChanged(rect);

  if (!(fabs(rect.l - m_rect.l) < EPSILON) || !(fabs(rect.r - m_rect.r) < EPSILON) || !(fabs(rect.t - m_rect.t) < EPSILON) ||
      !(fabs(rect.b - m_rect.b) < EPSILON))
  {
    SetHitRect(rect);

    if (!(fabs((rect.r - rect.l) - (m_rect.r - m_rect.l)) < EPSILON) || !(fabs((rect.b - rect.t) - (m_rect.b - m_rect.t)) < EPSILON)) {
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
    CMouseEvent evt;

    evt.button = m_dragButton;
    evt.x = m_clickPoint.x;
    evt.y = m_clickPoint.y;
    m_dragging = 1;
    OnDragStart(evt);
  }

  RunOnLeaveScript();
}

int CSimpleFrame::OnLayerChar(CCharEvent &evt) {
  char utf8string[6];

  if (!m_visible || !m_onChar) {
    return 0;
  }

  sputu8(evt.ch, utf8string);
  RunOnCharScript(utf8string);
  return 1;
}

int CSimpleFrame::OnLayerKeyDown(CKeyEvent &evt) {
  const char *keyName;

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

int CSimpleFrame::OnLayerKeyUp(CKeyEvent &evt) {
  const char *keyName;

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

int CSimpleFrame::OnLayerMouseDown(CMouseEvent &evt) {
  if (m_lookForDrag & evt.button) {
    m_mouseDown = 1;
    m_dragging = 0;
    m_dragButton = evt.button;
    m_clickPoint.x = evt.x;
    m_clickPoint.y = evt.y;
  }

  RunOnMouseDownScript(evt.button);
  return 0;
}

int CSimpleFrame::OnLayerMouseUp(CMouseEvent &evt) {
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

int CSimpleFrame::OnLayerMouseWheel(CMouseEvent &evt) {
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
