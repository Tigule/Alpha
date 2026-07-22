#include "Frame/CSimpleScrollFrame.h"

#include "Base/Status.h"
#include "FrameXML/FrameXML.h"
#include "FrameXML/XMLTree.h"

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

void CSimpleScrollFrame::UpdateScrollChild() {
  if (m_scrollChild) {
    m_scrollChild->ClearAllPoints(1);
    m_scrollChild->SetPoint(FRAMEPOINT_TOPLEFT, this, FRAMEPOINT_TOPLEFT, m_scrollOffset.x, m_scrollOffset.y, 1);
  }
}
