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
  if (string && hyperlink) {
    m_hyperlink = static_cast<char *>(SMemReAlloc(m_hyperlink, hyperlink->linkLength + 1, __FILE__, __LINE__, 0));
    SStrCopy(m_hyperlink, hyperlink->link, hyperlink->linkLength + 1);

    NTempest::CRect extent;
    NDCToDDC(hyperlink->extent.l, hyperlink->extent.b, &extent.l, &extent.t);
    NDCToDDC(hyperlink->extent.r, hyperlink->extent.t, &extent.r, &extent.b);

    ClearAllPoints(0);
    float ooScale = 1.0f / m_layoutScale;
    SetPoint(FRAMEPOINT_TOPLEFT, string, FRAMEPOINT_TOPLEFT, extent.l * ooScale, extent.b * ooScale, 1);
    SetWidth((extent.r - extent.l) / m_layoutScale);
    SetHeight((extent.b - extent.t) / m_layoutScale);
    Show();
  } else {
    Hide();
  }
}

void CSimpleHyperlinkButton::OnLayerCursorEnter() {
  static_cast<CSimpleHyperlinkedFrame *>(m_parent)->OnHyperlinkEnter(m_hyperlink);
}

void CSimpleHyperlinkButton::OnLayerCursorExit() {
  static_cast<CSimpleHyperlinkedFrame *>(m_parent)->OnHyperlinkLeave(m_hyperlink);
}

void CSimpleHyperlinkButton::OnClick(MOUSEBUTTON button) {
  static_cast<CSimpleHyperlinkedFrame *>(m_parent)->OnHyperlinkClick(m_hyperlink, button);
}

CSimpleHyperlinkedFrame::CSimpleHyperlinkedFrame(CSimpleFrame *parent)
    : CSimpleFrame(parent), m_onHyperlinkEnter(0), m_onHyperlinkLeave(0), m_onHyperlinkClick(0) {
}

CSimpleHyperlinkedFrame::~CSimpleHyperlinkedFrame() {
  m_hyperlinkButtons.Clear();

  SetOnHyperlinkEnterScript(0);
  SetOnHyperlinkLeaveScript(0);
  SetOnHyperlinkClickScript(0);
}

void CSimpleHyperlinkedFrame::LoadXML_Scripts(const XMLNode *node, CStatus *status) {
  const XMLNode *script;

  CSimpleFrame::LoadXML_Scripts(node, status);

  for (script = node->GetChild(); script; script = script->GetSibling()) {
    LPCSTR name = script->GetName();

    if (!SStrCmpI(name, "OnHyperlinkEnter", 0x7FFFFFFF)) {
      SetOnHyperlinkEnterScript(script->GetBody());
    } else if (!SStrCmpI(name, "OnHyperlinkLeave", 0x7FFFFFFF)) {
      SetOnHyperlinkLeaveScript(script->GetBody());
    } else if (!SStrCmpI(name, "OnHyperlinkClick", 0x7FFFFFFF)) {
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
  CSimpleHyperlinkButton *button = m_hyperlinkButtons.Head();

  if (button) {
    m_hyperlinkButtons.UnlinkNode(button);
  } else {
    button = new (ALLOC(sizeof(CSimpleHyperlinkButton))) CSimpleHyperlinkButton(this);
  }

  return button;
}

void CSimpleHyperlinkedFrame::ReleaseHyperlinkButton(CSimpleHyperlinkButton *button) {
  button->Hide();
  m_hyperlinkButtons.LinkNode(button, LIST_TAIL, 0);
}
