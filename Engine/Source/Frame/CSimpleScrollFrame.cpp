#include "Frame/CSimpleScrollFrame.h"

#include "Base/Coordinate.h"
#include "Base/Status.h"
#include "FrameXML/FrameXML.h"
#include "FrameXML/XMLTree.h"
#include "Gx/Gx.h"
#include "Services/Camera.h"
#include "Tempest/c44matrix.h"

#include <float.h>
#include <math.h>
#include <storm.h>

CSimpleScrollFrame::CSimpleScrollFrame(CSimpleFrame *parent)
    : CSimpleFrame(parent),
      m_updateScrollChild(0),
      m_scrollChild(0),
      m_scrollRange(0.0f),
      m_scrollOffset(0.0f),
      m_onHorizontalScroll(0),
      m_onVerticalScroll(0),
      m_onScrollRangeChanged(0) {
}

CSimpleScrollFrame::~CSimpleScrollFrame() {
  SetOnHorizontalScrollScript(0);
  SetOnVerticalScrollScript(0);
  SetOnScrollRangeChangedScript(0);
}

void CSimpleScrollFrame::LoadXML(const XMLNode *node, CStatus *status) {
  CSimpleFrame::LoadXML(node, status);

  const XMLNode *scrollChildNode = node->GetChildByName("ScrollChild");
  if (scrollChildNode) {
    const XMLNode *child = scrollChildNode->GetChild();

    if (child) {
      CSimpleFrame *frame = FrameXML_CreateFrame(child, this, status);
      if (frame) {
        SetScrollChild(frame);
      }
    } else {
      status->Add(STATUS_WARNING, "Scroll frame created without scroll child");
    }
  }
}

void CSimpleScrollFrame::LoadXML_Scripts(const XMLNode *node, CStatus *status) {
  CSimpleFrame::LoadXML_Scripts(node, status);

  const XMLNode *script;
  for (script = node->GetChild(); script; script = script->GetSibling()) {
    const char *name = script->GetName();
    const char *source = script->GetBody();

    if (!SStrCmpI(name, "OnHorizontalScroll", 0x7FFFFFFF)) {
      SetOnHorizontalScrollScript(source);
    } else if (!SStrCmpI(name, "OnVerticalScroll", 0x7FFFFFFF)) {
      SetOnVerticalScrollScript(source);
    } else if (!SStrCmpI(name, "OnScrollRangeChanged", 0x7FFFFFFF)) {
      SetOnScrollRangeChangedScript(source);
    }
  }
}

void CSimpleScrollFrame::SetScrollChild(CSimpleFrame *frame) {
  if (m_scrollChild) {
    m_scrollChild->SetBeingScrolled(0);
  }

  m_scrollChild = frame;

  if (m_scrollChild) {
    m_scrollChild->SetBeingScrolled(1);
  }

  UpdateScrollChild();
  m_updateScrollChild = 1;
}

void CSimpleScrollFrame::SetHorizontalScroll(float offset) {
  m_scrollOffset.x = offset;
  UpdateScrollChild();

  if (m_onHorizontalScroll) {
    FrameScript_Execute(m_onHorizontalScroll, this, "%f", m_scrollOffset.x * 1024.0f * 1.25f);
  }
}

void CSimpleScrollFrame::SetVerticalScroll(float offset) {
  m_scrollOffset.y = offset;
  UpdateScrollChild();

  if (m_onVerticalScroll) {
    FrameScript_Execute(m_onVerticalScroll, this, "%f", m_scrollOffset.y * 1024.0f * 1.25f);
  }
}

static void GetScrollChildRect(CSimpleFrame *frame, NTempest::CRect &rect) {
  NTempest::CRect frameRect(0.0f);

  if (frame->GetRect(&frameRect)) {
    rect.l = rect.l <= frameRect.l ? rect.l : frameRect.l;
    rect.r = rect.r >= frameRect.r ? rect.r : frameRect.r;
    rect.t = rect.t <= frameRect.t ? rect.t : frameRect.t;
    rect.b = rect.b >= frameRect.b ? rect.b : frameRect.b;
  }

  REGIONNODE *regionNode;
  for (regionNode = frame->m_regions.Head(); regionNode; regionNode = regionNode->m_link.Next()) {
    CSimpleRegion *region = regionNode->region;

    if (region->IsVisible() && region->GetRect(&frameRect)) {
      rect.l = rect.l <= frameRect.l ? rect.l : frameRect.l;
      rect.r = rect.r >= frameRect.r ? rect.r : frameRect.r;
      rect.t = rect.t <= frameRect.t ? rect.t : frameRect.t;
      rect.b = rect.b >= frameRect.b ? rect.b : frameRect.b;
    }
  }

  SIMPLEFRAMENODE *frameNode;
  for (frameNode = frame->m_children.Head(); frameNode; frameNode = frameNode->m_link.Next()) {
    if (frameNode->frame && frameNode->frame->m_shown) {
      GetScrollChildRect(frameNode->frame, rect);
    }
  }
}

void CSimpleScrollFrame::UpdateScrollChildRect(float w, float h) {
  static const float EPSILON = 2.38418579e-7f;

  if (m_scrollChild) {
    const NTempest::C2Vector lastRange = m_scrollRange;
    NTempest::CRect          rect;

    rect.l = FLT_MAX;
    rect.r = 0.0f;
    rect.t = FLT_MAX;
    rect.b = 0.0f;

    GetScrollChildRect(m_scrollChild, rect);

    const float inverseScale = 1.0f / m_layoutScale;
    const float horizontalRange = rect.r - rect.l - w;
    const float verticalRange = rect.b - rect.t - h;

    m_scrollRange.x = inverseScale * (horizontalRange > 0.0f ? horizontalRange : 0.0f);
    m_scrollRange.y = inverseScale * (verticalRange > 0.0f ? verticalRange : 0.0f);

    if ((fabs(m_scrollRange.x - lastRange.x) >= EPSILON ||
         fabs(m_scrollRange.y - lastRange.y) >= EPSILON) &&
        m_onScrollRangeChanged) {
      FrameScript_Execute(
          m_onScrollRangeChanged,
          this,
          "%f%f",
          m_scrollRange.x * 1024.0f * 1.25f,
          m_scrollRange.y * 1024.0f * 1.25f);
    }
  }
}

void CSimpleScrollFrame::UpdateScrollChild() {
  if (m_scrollChild) {
    m_scrollChild->ClearAllPoints(1);
    m_scrollChild->SetPoint(FRAMEPOINT_TOPLEFT, this, FRAMEPOINT_TOPLEFT, m_scrollOffset.x, m_scrollOffset.y, 1);
  }
}

void CSimpleScrollFrame::OnLayerUpdate(float elapsedSec) {
  CSimpleFrame::OnLayerUpdate(elapsedSec);

  if (m_updateScrollChild) {
    NTempest::CRect rect(0.0f);

    if (GetRect(&rect)) {
      UpdateScrollChildRect(rect.r - rect.l, rect.b - rect.t);
    }

    m_updateScrollChild = 0;
  }
}

void CSimpleScrollFrame::OnFrameRender(CRenderBatch *batch, unsigned int layer) {
  CSimpleFrame::OnFrameRender(batch, layer);

  if (layer == 4) {
    batch->QueueCallback(RenderScrollChild, this);
  }
}

void CSimpleScrollFrame::OnFrameSizeChanged(float w, float h) {
  CSimpleFrame::OnFrameSizeChanged(w, h);
  m_updateScrollChild = 1;
}

void __fastcall CSimpleScrollFrame::RenderScrollChild(void *param) {
  CSimpleScrollFrame *scrollFrame = static_cast<CSimpleScrollFrame *>(param);
  NTempest::CRect     viewRect(0.0f);

  if (scrollFrame->GetHitRect(viewRect) &&
      scrollFrame->m_scrollChild &&
      scrollFrame->m_scrollChild->IsVisible()) {
    NTempest::C44Matrix savedProjection;
    NTempest::C44Matrix savedView;
    float               minX;
    float               maxX;
    float               minY;
    float               maxY;
    float               minZ;
    float               maxZ;

    GxXformProjection(savedProjection);
    GxXformView(savedView);
    GxXformViewport(minX, maxX, minY, maxY, minZ, maxZ);

    const NTempest::C2Vector screenPoint(0.0f);
    CameraSetupScreenProjection(viewRect, screenPoint, 0.0f);

    DDCToNDC(viewRect.l, viewRect.t, &viewRect.l, &viewRect.t);
    DDCToNDC(viewRect.r, viewRect.b, &viewRect.r, &viewRect.b);
    GxXformSetViewport(viewRect.l, viewRect.r, viewRect.t, viewRect.b, 0.0f, 1.0f);

    scrollFrame->m_scrollChild->OnFrameRender();

    GxXformSetProjection(savedProjection);
    GxXformSetView(savedView);
    GxXformSetViewport(minX, maxX, minY, maxY, minZ, maxZ);
  }
}
