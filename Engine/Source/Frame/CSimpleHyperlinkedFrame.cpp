#include <Base/Base.h>

#include "Frame/CSimpleHyperlinkedFrame.h"

#include "Base/Coordinate.h"
#include "FrameXML/XMLTree.h"
#include "Gxu/IGxuFont.h"

#include <new>

CSimpleHyperlinkButton::CSimpleHyperlinkButton(CSimpleHyperlinkedFrame *parent) : CSimpleButton(parent) {
  m_hyperlink = 0;
}

CSimpleHyperlinkButton::~CSimpleHyperlinkButton() {
  FREEIFUSED(m_hyperlink);
}

void CSimpleHyperlinkButton::SetHyperlink(CSimpleFontString *string, const GXUFONTHYPERLINKINFO *hyperlink) {
  if (!string || !hyperlink) {
    Hide();
    return;
  }

  m_hyperlink = (char *)SMemReAlloc(m_hyperlink, hyperlink->linkLength + 1, __FILE__, __LINE__, 0);
  SStrCopy(m_hyperlink, hyperlink->link, hyperlink->linkLength + 1);

  NTempest::CRect extent;

  NDCToDDC(hyperlink->extent.l, hyperlink->extent.b, &extent.l, &extent.t);
  NDCToDDC(hyperlink->extent.r, hyperlink->extent.t, &extent.r, &extent.b);
  ClearAllPoints(0);
  SetPoint(FRAMEPOINT_TOPLEFT, string, FRAMEPOINT_TOPLEFT, extent.l / m_layoutScale, extent.b / m_layoutScale, 1);
  SetWidth((extent.r - extent.l) / m_layoutScale);
  SetHeight((extent.b - extent.t) / m_layoutScale);
  Show();
}

void CSimpleHyperlinkButton::OnLayerCursorEnter() {
  ((CSimpleHyperlinkedFrame *)m_parent)->OnHyperlinkEnter(m_hyperlink);
}

void CSimpleHyperlinkButton::OnLayerCursorExit() {
  ((CSimpleHyperlinkedFrame *)m_parent)->OnHyperlinkLeave(m_hyperlink);
}

void CSimpleHyperlinkButton::OnClick(MOUSEBUTTON button) {
  ((CSimpleHyperlinkedFrame *)m_parent)->OnHyperlinkClick(m_hyperlink, button);
}

CSimpleHyperlinkedFrame::CSimpleHyperlinkedFrame(CSimpleFrame *parent)
    : CSimpleFrame(parent), m_onHyperlinkEnter(0), m_onHyperlinkLeave(0), m_onHyperlinkClick(0) {
}

CSimpleHyperlinkedFrame::~CSimpleHyperlinkedFrame() {
  ITERATELIST(CSimpleHyperlinkButton, m_hyperlinkButtons, button) {
    ITERATE_DELETE;
  }

  SetOnHyperlinkEnterScript(0);
  SetOnHyperlinkLeaveScript(0);
  SetOnHyperlinkClickScript(0);
}

void CSimpleHyperlinkedFrame::LoadXML_Scripts(const XMLNode *node, CStatus *status) {
  const XMLNode *script;

  CSimpleFrame::LoadXML_Scripts(node, status);

  for (script = node->GetChild(); script; script = script->GetSibling()) {
    if (!SStrCmpI(script->GetName(), "OnHyperlinkEnter", 0x7FFFFFFF)) {
      SetOnHyperlinkEnterScript(script->GetBody());
    } else if (!SStrCmpI(script->GetName(), "OnHyperlinkLeave", 0x7FFFFFFF)) {
      SetOnHyperlinkLeaveScript(script->GetBody());
    } else if (!SStrCmpI(script->GetName(), "OnHyperlinkClick", 0x7FFFFFFF)) {
      SetOnHyperlinkClickScript(script->GetBody());
    }
  }
}

void CSimpleHyperlinkedFrame::OnHyperlinkEnter(LPCSTR link) {
  RunOnHyperlinkEnterScript(link);
}

void CSimpleHyperlinkedFrame::OnHyperlinkLeave(LPCSTR link) {
  RunOnHyperlinkLeaveScript(link);
}

void CSimpleHyperlinkedFrame::OnHyperlinkClick(LPCSTR link, MOUSEBUTTON button) {
  RunOnHyperlinkClickScript(link, button);
}

CSimpleHyperlinkButton *CSimpleHyperlinkedFrame::CreateHyperlinkButton() {
  CSimpleHyperlinkButton *button;

  if (!m_hyperlinkButtons.Head()) {
    button = NEW(CSimpleHyperlinkButton)(this);
  } else {
    button = m_hyperlinkButtons.Head();
    m_hyperlinkButtons.UnlinkNode(button);
  }

  return button;
}

void CSimpleHyperlinkedFrame::ReleaseHyperlinkButton(CSimpleHyperlinkButton *button) {
  button->Hide();
  m_hyperlinkButtons.LinkNode(button, LIST_TAIL, 0);
}
