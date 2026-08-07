#include "Frame/CLayoutFrame.h"

#include "Base/Status.h"
#include "Event/EvtApi.h"
#include "Frame/CFramePoint.h"
#include "Frame/CSimpleFrame.h"
#include "Frame/CSimpleRender.h"
#include "Frame/SimpleFrameRegistry.h"
#include "FrameXML/FrameXML.h"
#include "FrameXML/LoadXML.h"
#include "FrameXML/XMLTree.h"

#include <math.h>
#include <string.h>

static LISTDECLEX(CLayoutFrame, resizeLink, s_resizePendingList);

CLayoutFrame *CLayoutFrame::GetLayoutFrameByName(LPCSTR name) {
  CLayoutFrame *result = SimpleFrameRegistryGetEntry(name, 0);

  if (!result) {
    result = SimpleFontStringRegistryGetEntry(name, 0);
  }

  if (!result) {
    result = SimpleTextureRegistryGetEntry(name, 0);
  }

  return result;
}

static float SynthesizeSide(float center, float opposite, float size) {
  if (center != CFramePoint::UNDEFINED) {
    if (opposite != CFramePoint::UNDEFINED) {
      return center - opposite + center;
    }
  } else if (opposite != CFramePoint::UNDEFINED && size != 0.0f) {
    return opposite + size;
  }

  if (center != CFramePoint::UNDEFINED && size != 0.0f) {
    return center + size * 0.5f;
  }

  return CFramePoint::UNDEFINED;
}

static float SynthesizeCenter(float side1, float side2, float size) {
  if (side1 != CFramePoint::UNDEFINED) {
    if (side2 != CFramePoint::UNDEFINED) {
      return (side1 + side2) * 0.5f;
    }

    if (size != 0.0f) {
      return side1 + size * 0.5f;
    }
  } else if (side2 != CFramePoint::UNDEFINED && size != 0.0f) {
    return side2 - size * 0.5f;
  }

  return CFramePoint::UNDEFINED;
}

float CLayoutFrame::GetFirstPointX(const FRAMEPOINT *pointarray, int elements) {
  for (; elements; --elements, ++pointarray) {
    CFramePoint *point = m_points[*pointarray];

    if (point) {
      float value = point->X(m_layoutScale);

      if (value != CFramePoint::UNDEFINED) {
        return value;
      }
    }
  }

  return CFramePoint::UNDEFINED;
}

float CLayoutFrame::GetFirstPointY(const FRAMEPOINT *pointarray, int elements) {
  for (; elements; --elements, ++pointarray) {
    CFramePoint *point = m_points[*pointarray];

    if (point) {
      float value = point->Y(m_layoutScale);

      if (value != CFramePoint::UNDEFINED) {
        return value;
      }
    }
  }

  return CFramePoint::UNDEFINED;
}

float CLayoutFrame::CenterX() {
  static const FRAMEPOINT sidepoints[] = {FRAMEPOINT_TOP, FRAMEPOINT_CENTER, FRAMEPOINT_BOTTOM};

  if (m_guard.centerX) {
    return CFramePoint::UNDEFINED;
  }

  m_guard.centerX = 1;
  float result = GetFirstPointX(sidepoints, 3);

  if (result == CFramePoint::UNDEFINED) {
    result = SynthesizeCenter(Left(), Right(), GetWidth() * m_layoutScale);
  }

  m_guard.centerX = 0;
  return result;
}

float CLayoutFrame::CenterY() {
  static const FRAMEPOINT sidepoints[] = {FRAMEPOINT_LEFT, FRAMEPOINT_CENTER, FRAMEPOINT_RIGHT};

  if (m_guard.centerY) {
    return CFramePoint::UNDEFINED;
  }

  m_guard.centerY = 1;
  float result = GetFirstPointY(sidepoints, 3);

  if (result == CFramePoint::UNDEFINED) {
    result = SynthesizeCenter(Bottom(), Top(), GetHeight() * m_layoutScale);
  }

  m_guard.centerY = 0;
  return result;
}

float CLayoutFrame::Left() {
  static const FRAMEPOINT sidepoints[] = {FRAMEPOINT_TOPLEFT, FRAMEPOINT_LEFT, FRAMEPOINT_BOTTOMLEFT};

  if (m_guard.left) {
    return CFramePoint::UNDEFINED;
  }

  m_guard.left = 1;
  float result = GetFirstPointX(sidepoints, 3);

  if (result == CFramePoint::UNDEFINED) {
    result = SynthesizeSide(CenterX(), Right(), -GetWidth() * m_layoutScale);
  }

  m_guard.left = 0;
  return result;
}

float CLayoutFrame::Top() {
  static const FRAMEPOINT sidepoints[] = {FRAMEPOINT_TOPLEFT, FRAMEPOINT_TOP, FRAMEPOINT_TOPRIGHT};

  if (m_guard.top) {
    return CFramePoint::UNDEFINED;
  }

  m_guard.top = 1;
  float result = GetFirstPointY(sidepoints, 3);

  if (result == CFramePoint::UNDEFINED) {
    result = SynthesizeSide(CenterY(), Bottom(), GetHeight() * m_layoutScale);
  }

  m_guard.top = 0;
  return result;
}

float CLayoutFrame::Right() {
  static const FRAMEPOINT sidepoints[] = {FRAMEPOINT_TOPRIGHT, FRAMEPOINT_RIGHT, FRAMEPOINT_BOTTOMRIGHT};

  if (m_guard.right) {
    return CFramePoint::UNDEFINED;
  }

  m_guard.right = 1;
  float result = GetFirstPointX(sidepoints, 3);

  if (result == CFramePoint::UNDEFINED) {
    result = SynthesizeSide(CenterX(), Left(), GetWidth() * m_layoutScale);
  }

  m_guard.right = 0;
  return result;
}

float CLayoutFrame::Bottom() {
  static const FRAMEPOINT sidepoints[] = {FRAMEPOINT_BOTTOMLEFT, FRAMEPOINT_BOTTOM, FRAMEPOINT_BOTTOMRIGHT};

  if (m_guard.bottom) {
    return CFramePoint::UNDEFINED;
  }

  m_guard.bottom = 1;
  float result = GetFirstPointY(sidepoints, 3);

  if (result == CFramePoint::UNDEFINED) {
    result = SynthesizeSide(CenterY(), Top(), -GetHeight() * m_layoutScale);
  }

  m_guard.bottom = 0;
  return result;
}

void CLayoutFrame::FreePoints() {
  UINT          count = m_points.Count();
  CFramePoint **point = m_points.Ptr();

  while (count) {
    if (*point) {
      CLayoutFrame *relative = (*point)->GetRelative();

      if (relative) {
        relative->UnregisterResize(this);
      }

      DEL(*point);
      *point = 0;
    }

    ++point;
    --count;
  }
}

CLayoutFrame::CLayoutFrame() : m_flags(0), m_width(0.0f), m_height(0.0f), m_layoutScale(1.0f) {
  m_points.SetCount(FRAMEPOINT_NUMPOINTS);
  memset(m_points.Ptr(), 0, m_points.Count() * sizeof(CFramePoint *));
  memset(&m_guard, 0, sizeof(m_guard));
}

CLayoutFrame::~CLayoutFrame() {
  DestroyLayout();
}

void CLayoutFrame::LoadXML(const XMLNode *node, CStatus *status) {
  LPCSTR inherits = node->GetAttributeByName("inherits");

  if (inherits && *inherits) {
    const XMLNode *inheritedNode = FrameXML_FindHashNode(inherits);

    if (inheritedNode) {
      LoadXML(inheritedNode, status);
    } else {
      status->Add(STATUS_WARNING, "Couldn't find inherited node: %s", inherits);
    }
  }

  const XMLNode *sizeNode = node->GetChildByName("Size");
  if (sizeNode) {
    float w;
    float h;

    if (LoadXML_Dimensions(sizeNode, w, h, status)) {
      SetWidth(w);
      SetHeight(h);
    }
  }

  CLayoutFrame *parent = GetLayoutParent();
  ASSERT(parent);

  int    setAllPoints = 0;
  LPCSTR setAllPointsValue = node->GetAttributeByName("setAllPoints");
  if (setAllPointsValue && !SStrCmpI(setAllPointsValue, "true", 0x7FFFFFFF)) {
    setAllPoints = 1;
  }

  const XMLNode *anchors = node->GetChildByName("Anchors");
  if (anchors) {
    if (setAllPoints) {
      status->Add(STATUS_WARNING, "SETALLPOINTS set to true in frame with anchors (ignored)");
    }

    const XMLNode *anchor;
    for (anchor = anchors->GetChild(); anchor; anchor = anchor->GetSibling()) {
      CLayoutFrame *relative = parent;
      float         offsetX = 0.0f;
      float         offsetY = 0.0f;
      FRAMEPOINT    point;
      LPCSTR        pointValue = anchor->GetAttributeByName("point");

      if (!pointValue || !StringToFramePoint(pointValue, point)) {
        status->Add(STATUS_WARNING, "Invalid anchor point in frame: %s", pointValue);
        continue;
      }

      FRAMEPOINT relativePoint;
      LPCSTR     relativePointValue = anchor->GetAttributeByName("relativePoint");
      if (relativePointValue && *relativePointValue) {
        if (!StringToFramePoint(relativePointValue, relativePoint)) {
          status->Add(STATUS_WARNING, "Invalid anchor point in frame: %s", relativePointValue);
          continue;
        }
      } else {
        relativePoint = point;
      }

      LPCSTR relativeTo = anchor->GetAttributeByName("relativeTo");
      if (relativeTo && *relativeTo) {
        relative = GetLayoutFrameByName(relativeTo);
        if (!relative) {
          status->Add(STATUS_WARNING, "Couldn't find relative frame: %s", relativeTo);
          continue;
        }
      }

      const XMLNode *offset = anchor->GetChildByName("Offset");
      if (offset) {
        LoadXML_Dimensions(offset, offsetX, offsetY, status);
      }

      SetPoint(point, relative, relativePoint, offsetX, offsetY, 0);
    }

    Resize(0);
  } else if (setAllPoints) {
    SetAllPoints(parent, 1);
  }
}

BOOL CLayoutFrame::CalculateRect(NTempest::CRect *rect) {
  return (rect->l = Left()) != CFramePoint::UNDEFINED && (rect->t = Bottom()) != CFramePoint::UNDEFINED &&
         (rect->r = Right()) != CFramePoint::UNDEFINED && (rect->b = Top()) != CFramePoint::UNDEFINED;
}

void CLayoutFrame::SetPoint(FRAMEPOINT point, float x, float y, int doResize) {
  CFramePoint *oldPoint = m_points[point];

  if (oldPoint) {
    CLayoutFrame *relative = oldPoint->GetRelative();

    if (relative) {
      relative->UnregisterResize(this);
    }

    DEL(oldPoint);
  }

  m_points[point] = NEW(CFramePointAbsolute)(x, y);

  if (doResize) {
    Resize(0);
  }
}

void CLayoutFrame::SetPoint(FRAMEPOINT point, CLayoutFrame *relative, FRAMEPOINT relativePoint, float offsetX, float offsetY, int doResize) {
  FATALASSERT(relative);

  FATALASSERT(relative != this);

  CFramePoint *oldPoint = m_points[point];

  if (oldPoint) {
    CLayoutFrame *oldRelative = oldPoint->GetRelative();

    if (oldRelative) {
      oldRelative->UnregisterResize(this);
    }

    DEL(oldPoint);
  }

  m_points[point] = NEW(CFramePointRelative)(relative, relativePoint, offsetX, offsetY);
  relative->RegisterResize(this, 0xF);

  if (doResize) {
    Resize(0);
  }
}

void CLayoutFrame::SetAllPoints(CLayoutFrame *relative, int doResize) {
  FATALASSERT(relative);

  FATALASSERT(relative != this);

  FreePoints();
  m_points[FRAMEPOINT_TOPLEFT] = NEW(CFramePointRelative)(relative, FRAMEPOINT_TOPLEFT, 0.0f, 0.0f);
  m_points[FRAMEPOINT_BOTTOMRIGHT] = NEW(CFramePointRelative)(relative, FRAMEPOINT_BOTTOMRIGHT, 0.0f, 0.0f);
  relative->RegisterResize(this, 0xF);

  if (doResize) {
    Resize(0);
  }
}

void CLayoutFrame::Clear(CLayoutFrame *relative, int doResize) {
  UINT          count = m_points.Count();
  CFramePoint **point = m_points.Ptr();

  while (count) {
    if (*point && (*point)->GetRelative() == relative) {
      DEL(*point);
      *point = 0;
    }

    ++point;
    --count;
  }

  if (doResize) {
    Resize(0);
  }
}

void CLayoutFrame::ClearAllPoints(int doResize) {
  FreePoints();

  if (doResize) {
    Resize(0);
  }
}

void CLayoutFrame::RegisterResize(CLayoutFrame *frame, UINT dependency) {
  FRAMENODE *node;

  ITERATELIST(FRAMENODE, m_resizeList, existingNode) {
    if (existingNode->frame == frame) {
      existingNode->dep |= dependency;
      return;
    }
  }

  FATALASSERT(!IsResizeDependency(frame));

  node = m_resizeList.NewNode(LIST_TAIL, 0, 0);
  node->frame = frame;
  node->dep = dependency;
}

void CLayoutFrame::UnregisterResize(const CLayoutFrame *frame) {
  ITERATELIST(FRAMENODE, m_resizeList, node) {
    if (node->frame == frame) {
      m_resizeList.DeleteNode(node);
      return;
    }
  }
}

BOOL CLayoutFrame::IsResizeDependency(CLayoutFrame *pNewDependentFrame) {
  UINT whichPoint;

  for (whichPoint = 0; whichPoint < FRAMEPOINT_NUMPOINTS; ++whichPoint) {
    CFramePoint *point = GetPoint((FRAMEPOINT)whichPoint);

    if (point) {
      CLayoutFrame *relative = point->GetRelative();

      if (relative && (relative == pNewDependentFrame || relative->IsResizeDependency(pNewDependentFrame))) {
        return 1;
      }
    }
  }

  return 0;
}

void CLayoutFrame::SetDeferredResize(int enable) {
  if (enable) {
    m_flags |= 0x2;
  } else {
    m_flags &= ~0x2U;
  }

  if (!(m_flags & 0x2) && (m_flags & 0x4)) {
    UINT whichPoint;

    for (whichPoint = 0; whichPoint < FRAMEPOINT_NUMPOINTS; ++whichPoint) {
      CFramePoint *point = GetPoint((FRAMEPOINT)whichPoint);
      if (point) {
        CLayoutFrame *relative = point->GetRelative();
        if (relative && relative->IsResizeDeferred()) {
          relative->SetDeferredResize(0);
        }
      }
    }

    Resize(0);
  }
}

void CLayoutFrame::Resize(int force) {
  if (force && OnFrameResize()) {
    if (resizeLink.IsLinked()) {
      resizeLink.Unlink();
    }

    return;
  }

  if (m_flags & 0x2) {
    ITERATELIST(FRAMENODE, m_resizeList, node) {
      if (!(node->frame->m_flags & 0x2)) {
        SetDeferredResize(0);
        return;
      }
    }

    m_flags |= 0x4;
    return;
  }

  if (!resizeLink.IsLinked()) {
    CLayoutFrame *pDependentNode = 0;

    ITERATELIST(CLayoutFrame, s_resizePendingList, frame) {
      UINT whichPoint;

      for (whichPoint = 0; whichPoint < FRAMEPOINT_NUMPOINTS; ++whichPoint) {
        CFramePoint *point = frame->GetPoint((FRAMEPOINT)whichPoint);

        if (point && point->GetRelative() == this) {
          pDependentNode = frame;
        }
      }

      if (pDependentNode) {
        break;
      }
    }

    if (pDependentNode) {
      s_resizePendingList.LinkNode(this, LIST_LINK_BEFORE, pDependentNode);
    } else {
      s_resizePendingList.LinkNode(this, LIST_TAIL, 0);
    }
  }

  m_resizeCounter = 6;
}

BOOL CLayoutFrame::IsResizePending() {
  return s_resizePendingList.IsLinked(this);
}

int CLayoutFrame::SetRect(NTempest::CRect &rect) {
  m_flags |= 0x1;
  OnFrameSizeChanged(rect);
  m_rect = rect;

  if (m_flags & 0x10) {
    CageMouseInFrame(1);
  }

  return m_flags & 0x1;
}

BOOL CLayoutFrame::GetRect(NTempest::CRect *rect) const {
  if (!(m_flags & 0x1)) {
    return 0;
  }

  *rect = m_rect;
  return 1;
}

void CLayoutFrame::SetLayoutScale(float scale, bool force) {
  static const float EPSILON = 2.38418579e-7f;

  FATALASSERT(scale);

  if (force || fabs(scale - m_layoutScale) >= EPSILON) {
    m_layoutScale = scale;
    m_rect.Set(0.0f, 0.0f, 0.0f, 0.0f);
    m_flags &= ~0x1U;
    Resize(0);
  }
}

void CLayoutFrame::SetWidth(float width) {
  m_width = width;
  Resize(0);
}

void CLayoutFrame::SetHeight(float height) {
  m_height = height;
  Resize(0);
}

float CLayoutFrame::GetHeight() {
  return m_height;
}

float CLayoutFrame::GetWidth() {
  return m_width;
}

BOOL CLayoutFrame::FlattenFrame(CLayoutFrame *top, float width, float height, float delta_x, float delta_y, NTempest::CRect *finalrect) {
  NTempest::CRect toprect;
  NTempest::CRect rect;

  if (!GetRect(&rect) || !top->GetRect(&toprect)) {
    return 0;
  }

  FreePoints();
  SetPoint(
      FRAMEPOINT_TOPLEFT, top, FRAMEPOINT_TOPLEFT, (rect.l + delta_x - toprect.l) / m_layoutScale, (rect.b + delta_y - toprect.b) / m_layoutScale, 0
  );

  if (width == 0.0f) {
    width = GetWidth();
  }
  SetWidth(width);

  if (height == 0.0f) {
    height = GetHeight();
  }
  SetHeight(height);
  Resize(1);

  if (finalrect) {
    finalrect->l = rect.l + delta_x;
    finalrect->r = rect.l + delta_x + width;
    finalrect->t = rect.t + delta_y;
    finalrect->b = rect.t + delta_y + height;
  }

  return 1;
}

int CLayoutFrame::ScaleBy(CLayoutFrame *top, float scale_x, float scale_y, FRAMEPOINT anchorpoint, NTempest::CRect *finalrect) {
  float newwidth;
  float delta_x = 0.0f;
  float delta_y = 0.0f;
  float oldwidth;
  float oldheight;
  float newheight;

  ASSERT(top);

  oldwidth = GetWidth();
  newwidth = oldwidth * scale_x;
  oldheight = GetHeight();
  newheight = oldheight * scale_y;

  switch (anchorpoint) {
    case FRAMEPOINT_TOPLEFT:
      delta_x = 0.0f;
      delta_y = 0.0f;
      break;
    case FRAMEPOINT_TOP:
      delta_x = (oldwidth - newwidth) * 0.5f;
      delta_y = 0.0f;
      break;
    case FRAMEPOINT_TOPRIGHT:
      delta_x = oldwidth - newwidth;
      delta_y = 0.0f;
      break;
    case FRAMEPOINT_LEFT:
      delta_x = 0.0f;
      delta_y = (newheight - oldheight) * 0.5f;
      break;
    case FRAMEPOINT_CENTER:
      delta_x = (oldwidth - newwidth) * 0.5f;
      delta_y = (newheight - oldheight) * 0.5f;
      break;
    case FRAMEPOINT_RIGHT:
      delta_x = oldwidth - newwidth;
      delta_y = (newheight - oldheight) * 0.5f;
      break;
    case FRAMEPOINT_BOTTOMLEFT:
      delta_x = 0.0f;
      delta_y = newheight - oldheight;
      break;
    case FRAMEPOINT_BOTTOM:
      delta_x = (oldwidth - newwidth) * 0.5f;
      delta_y = newheight - oldheight;
      break;
    case FRAMEPOINT_BOTTOMRIGHT:
      delta_x = oldwidth - newwidth;
      delta_y = newheight - oldheight;
      break;
    default:
      ASSERT(!"Unhandled frame point");
      break;
  }

  return FlattenFrame(top, newwidth, newheight, delta_x, delta_y, finalrect);
}

int CLayoutFrame::DragBy(CLayoutFrame *top, float delta_x, float delta_y, FRAMEPOINT dragpoint, NTempest::CRect *finalrect) {
  ASSERT(top);

  float newwidth = GetWidth();
  float newheight = GetHeight();

  switch (dragpoint) {
    case FRAMEPOINT_TOPLEFT:
      newwidth -= delta_x;
      newheight += delta_y;
      break;

    case FRAMEPOINT_TOP:
      delta_x = 0.0f;
      newheight += delta_y;
      break;

    case FRAMEPOINT_TOPRIGHT:
      newwidth += delta_x;
      delta_x = 0.0f;
      newheight += delta_y;
      break;

    case FRAMEPOINT_LEFT:
      delta_y = 0.0f;
      newwidth -= delta_x;
      break;

    case FRAMEPOINT_CENTER:
      break;

    case FRAMEPOINT_RIGHT:
      delta_y = 0.0f;
      newwidth += delta_x;
      delta_x = 0.0f;
      break;

    case FRAMEPOINT_BOTTOMLEFT:
      newwidth -= delta_x;
      newheight -= delta_y;
      delta_y = 0.0f;
      newwidth -= delta_x;
      break;

    case FRAMEPOINT_BOTTOM:
      delta_x = 0.0f;
      newheight -= delta_y;
      delta_y = 0.0f;
      break;

    case FRAMEPOINT_BOTTOMRIGHT:
      newwidth += delta_x;
      newheight -= delta_y;
      delta_x = 0.0f;
      delta_y = 0.0f;
      break;

    default:
      ASSERT(!"Unhandled frame point");
      break;
  }

  return FlattenFrame(top, newwidth, newheight, delta_x, delta_y, finalrect);
}

BOOL CLayoutFrame::PtInFrameRect(const NTempest::C2Vector &pt) {
  return IsRectValid() && pt.x >= m_rect.l && pt.x <= m_rect.r && pt.y >= m_rect.t && pt.y <= m_rect.b;
}

void CLayoutFrame::CageMouseInFrame(int enable) {
  if ((m_flags & 0x1) && enable) {
    EventSetMouseBoundingRect(&m_rect);
  } else {
    EventSetMouseBoundingRect(0);
  }

  if (enable) {
    m_flags |= 0x10;
  } else {
    m_flags &= ~0x10U;
  }
}

void CLayoutFrame::OnFrameSizeChanged(const NTempest::CRect &rect) {
  static const float EPSILON = 2.38418579e-7f;
  float              oldHeight = m_rect.b - m_rect.t;
  float              oldWidth = m_rect.r - m_rect.l;
  UINT               dependency = 0;

  if (!(fabs(rect.b - m_rect.b) < EPSILON)) {
    dependency = 0x3;
  }
  if (!(fabs(rect.l - m_rect.l) < EPSILON)) {
    dependency |= 0x5;
  }
  if (!(fabs(rect.r - m_rect.r) < EPSILON)) {
    dependency |= 0xA;
  }
  if (!(fabs(rect.t - m_rect.t) < EPSILON)) {
    dependency |= 0xC;
  }
  if (!(fabs((rect.b - rect.t) - oldHeight) < EPSILON)) {
    dependency |= 0x20;
  }
  if (!(fabs((rect.r - rect.l) - oldWidth) < EPSILON)) {
    dependency |= 0x10;
  }

  if (oldWidth == 0.0f || oldHeight == 0.0f) {
    dependency |= 0xF;
  }

  ITERATELIST(FRAMENODE, m_resizeList, node) {
    if (node->dep & dependency) {
      node->frame->Resize(0);
    }
  }
}

BOOL CLayoutFrame::OnFrameResize() {
  NTempest::CRect rect;

  if (CalculateRect(&rect)) {
    m_flags |= 0x1;
  } else {
    m_flags &= ~0x1U;
  }

  if (!(m_flags & 0x1)) {
    return 0;
  }

  SetRect(rect);
  return 1;
}

void CLayoutFrame::DestroyLayout() {
  if (m_flags & 0x10) {
    CageMouseInFrame(0);
  }

  FreePoints();

  FRAMENODE *node;
  ITERATELIST(FRAMENODE, m_resizeList, iterNode) {
    iterNode->frame->Clear(this, 1);
  }

  while ((node = m_resizeList.Head()) != 0) {
    m_resizeList.DeleteNode(node);
  }

  RemoveFromResizeList(this);
}

UINT CLayoutFrame::ResizePending() {
  UINT          resized = 0;
  CLayoutFrame *frame = s_resizePendingList.Head();

  while (frame) {
    CLayoutFrame *next = s_resizePendingList.Next(frame);

    if (frame->OnFrameResize()) {
      ++resized;
    } else if (--frame->m_resizeCounter) {
      frame = next;
      continue;
    }

    s_resizePendingList.UnlinkNode(frame);
    frame = next;
  }

  return resized;
}

void CLayoutFrame::RemoveFromResizeList(CLayoutFrame *pFrame) {
  s_resizePendingList.UnlinkNode(pFrame);
}

void CLayoutFrame::ClearResizePendingList() {
  s_resizePendingList.UnlinkAll();
}
