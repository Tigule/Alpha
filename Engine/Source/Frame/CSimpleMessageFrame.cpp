#include <Base/Base.h>

#include "Frame/CSimpleMessageFrame.h"

#include "FrameXML/LoadXML.h"
#include "FrameXML/XMLTree.h"

#include <math.h>

static const float DEFAULT_FADE_DURATION = 3.0f;

CSimpleMessageFrame::CSimpleMessageFrame(CSimpleFrame *parent) : CSimpleFrame(parent) {
  m_rows = 0;
  m_numVisible = 0;

  m_textMaxSize = 0;
  m_fadeDuration = DEFAULT_FADE_DURATION;
  m_insertMode = INSERT_AT_BOTTOM;
}

CSimpleMessageFrame::~CSimpleMessageFrame() {
  m_lines.Clear();
  ClearPending();
}

void CSimpleMessageFrame::LoadXML(const XMLNode *node, CStatus *status) {
  CSimpleFrame::LoadXML(node, status);

  const XMLNode *child;
  for (child = node->GetChild(); child; child = child->GetSibling()) {
    if (!SStrCmpI(child->GetName(), "FontString", 0x7FFFFFFF)) {
      CSimpleFontString *string = LoadXML_String(child, this, status);
      m_attrib = *string;
      DELIFUSED(string);
    } else if (!SStrCmpI(child->GetName(), "TextInsets", 0x7FFFFFFF)) {
      float r;
      float l;
      float t;
      float b;

      if (LoadXML_Insets(child, l, r, t, b, status)) {
        SetMessageFrameInsets(r, l, t, b);
      }
    }
  }

  LPCSTR value = node->GetAttributeByName("fadeDuration");
  if (value && *value) {
    float fadeDuration = SStrToFloat(value);
    if (fadeDuration > 0.0f) {
      m_fadeDuration = fadeDuration;
    }
  }

  value = node->GetAttributeByName("insertMode");
  if (value && *value) {
    SetInsertMode(!SStrCmpI(value, "BOTTOM", 0x7FFFFFFF) ? INSERT_AT_BOTTOM : INSERT_AT_TOP);
  }
}

void CSimpleMessageFrame::SetMessageFrameInsets(float right, float left, float top, float bottom) {
  m_messageFrameInset.l = left;
  m_messageFrameInset.b = bottom;
  m_messageFrameInset.r = right;
  m_messageFrameInset.t = top;

  if (IsRectValid()) {
    OnFrameSizeChanged(m_rect);
  }
}

void CSimpleMessageFrame::SetTextLength(int size) {
  m_textMaxSize = size;

  for (UINT i = 0; i < m_rows; ++i) {
    m_lines[i].stringNode->string->SetTextLength(size);
  }
}

void CSimpleMessageFrame::SetInsertMode(SimpleMessageFrameInsertMode mode) {
  m_insertMode = mode;

  if (IsRectValid()) {
    OnFrameSizeChanged(m_rect);
  }
}

void CSimpleMessageFrame::AddMessage(LPCSTR text, const NTempest::CImVector &color, float timeVisible, int permanent) {
  ASSERT(text);

  MessageData *message = m_pendingMessages.New();
  message->text = SStrDupA(text, __FILE__, __LINE__);
  message->color = color;
  message->timeVisible = timeVisible;
  message->permanent = permanent;
}

void CSimpleMessageFrame::Clear() {
  for (UINT i = 0; i < m_rows; ++i) {
    HideLineNode(m_lines[i].stringNode);
  }

  ClearPending();
}

void CSimpleMessageFrame::ClearPending() {
  UINT pendingCount = m_pendingMessages.Count();
  UINT i;

  for (i = 0; i < pendingCount; ++i) {
    DELIFUSED(m_pendingMessages[i].text);
    m_pendingMessages[i].text = 0;
  }

  m_pendingMessages.Clear();
}

void CSimpleMessageFrame::OnFrameSizeChanged(const NTempest::CRect &rect) {
  CSimpleFrame::OnFrameSizeChanged(rect);

  float scale = GetLayoutScale();
  m_messageFrameArea.l = rect.l + m_messageFrameInset.l * scale;
  m_messageFrameArea.r = rect.r - m_messageFrameInset.r * scale;
  m_messageFrameArea.t = rect.t + m_messageFrameInset.t * scale;
  m_messageFrameArea.b = rect.b - m_messageFrameInset.b * scale;

  float fontHeight = (m_attrib.GetFontHeight() + m_attrib.GetSpacing()) * scale;
  ASSERT(fontHeight != 0.0f);

  float              areaHeight = m_messageFrameArea.b - m_messageFrameArea.t;
  UINT               rows = static_cast<UINT>(areaHeight / fontHeight);
  static const float EPSILON = 2.38418579e-7f;
  if (fabs((rows + 1) * fontHeight - areaHeight) < EPSILON) {
    ++rows;
  }

  m_rows = rows;
  m_lines.SetCount(rows);

  float offsetX = m_messageFrameArea.l / scale;
  float offsetY = m_messageFrameArea.b / scale;
  float messageWidth = (m_messageFrameArea.r - m_messageFrameArea.l) / scale;
  UINT  index = m_insertMode == INSERT_AT_BOTTOM ? rows - 1 : 0;
  int   increment = m_insertMode == INSERT_AT_BOTTOM ? -1 : 1;

  for (; index < rows; index += increment) {
    CSimpleFontString *string = m_lines[index].stringNode->string;

    m_lines[index].offsetX = offsetX;
    m_lines[index].offsetY = offsetY;
    string->SetFrame(this, 2, m_lines[index].stringNode->isVisible);
    string->SetWidth(messageWidth);
    string->SetPoint(FRAMEPOINT_TOPLEFT, this, FRAMEPOINT_TOPLEFT, m_lines[index].offsetX, -m_lines[index].offsetY, 1);
    m_attrib.UpdateString(string, 1);
    string->SetTextLength(m_textMaxSize);
    offsetY += m_attrib.GetFontHeight() + m_attrib.GetSpacing();
  }
}

void CSimpleMessageFrame::OnLayerUpdate(float elapsedSec) {
  CSimpleFrame::OnLayerUpdate(elapsedSec);

  UINT pendingCount = m_pendingMessages.Count();
  if (pendingCount) {
    NTempest::CRect rect;
    if (GetRect(&rect)) {
      for (UINT pendingIndex = 0; pendingIndex < pendingCount; ++pendingIndex) {
        MessageData &message = m_pendingMessages[pendingIndex];
        AddPendingMessage(message.text, message.color, message.timeVisible, message.permanent);
        DELIFUSED(message.text);
        message.text = 0;
      }

      m_pendingMessages.Clear();
    }
  }

  if (!m_numVisible) {
    return;
  }

  for (UINT i = 0; i < m_rows; ++i) {
    CSimpleMessageFrameLineNode *node = m_lines[i].stringNode;
    if (node->permanent) {
      continue;
    }

    if (node->timeLeft != 0.0f) {
      node->timeLeft -= elapsedSec;
      if (node->timeLeft < 0.0f) {
        if (node->fadeLeft == 0.0f) {
          HideLineNode(node);
        } else {
          node->timeLeft = 0.0f;
        }
      }
    } else if (node->fadeLeft != 0.0f) {
      node->fadeLeft -= elapsedSec;
      if (node->fadeLeft < 0.0f) {
        HideLineNode(node);
      } else {
        node->color.a = static_cast<BYTE>(node->fadeLeft / m_fadeDuration * 255.0f);
        node->string->SetVertexColor(node->color);
      }
    }
  }
}

void CSimpleMessageFrame::AddPendingMessage(LPCSTR text, const NTempest::CImVector &color, float timeVisible, int permanent) {
  ASSERT(text);
  ASSERT(m_rows > 0);

  ScrollMessages(0);
  CSimpleMessageFrameLineNode *node = m_lines[0].stringNode;
  node->color = color;
  node->string->SetText(text);
  node->string->SetVertexColor(color);
  ShowLineNode(node, timeVisible, m_fadeDuration, permanent);

  float rows = static_cast<float>(floor(node->string->GetHeight() / node->string->GetFontHeight() + 0.5f));
  if (rows > 1.0f && m_rows > static_cast<UINT>(rows)) {
    UINT start = m_insertMode == INSERT_AT_TOP;
    do {
      ScrollMessages(start);
      rows -= 1.0f;
    } while (rows > 1.0f);
  }
}

void CSimpleMessageFrame::ScrollMessages(UINT start) {
  CSimpleMessageFrameLineNode *lastNode = m_lines[m_rows - 1].stringNode;

  UINT i;
  for (i = m_rows - 1; i > start; --i) {
    m_lines[i].stringNode = m_lines[i - 1].stringNode;
    m_lines[i].stringNode->string->SetPoint(FRAMEPOINT_TOPLEFT, this, FRAMEPOINT_TOPLEFT, m_lines[i].offsetX, -m_lines[i].offsetY, 1);
  }

  m_lines[i].stringNode = lastNode;
  m_lines[i].stringNode->string->SetPoint(FRAMEPOINT_TOPLEFT, this, FRAMEPOINT_TOPLEFT, m_lines[i].offsetX, -m_lines[i].offsetY, 1);
  HideLineNode(lastNode);
}

void CSimpleMessageFrame::HideLineNode(CSimpleMessageFrameLineNode *node) {
  if (node->isVisible) {
    node->string->Hide();
    node->timeLeft = 0.0f;
    node->fadeLeft = 0.0f;
    node->isVisible = 0;
    --m_numVisible;
  }
}

void CSimpleMessageFrame::ShowLineNode(CSimpleMessageFrameLineNode *node, float timeVisible, float fadeDuration, int permanent) {
  ASSERT(!node->isVisible);

  node->string->Show();
  node->fadeLeft = fadeDuration;
  node->timeLeft = timeVisible;
  node->isVisible = 1;
  node->permanent = permanent || timeVisible == 0.0f;
  ++m_numVisible;
  ASSERT(m_numVisible <= m_rows);
}

CSimpleMessageFrameLineNode::CSimpleMessageFrameLineNode()
    : color(0ul), string(NEW(CSimpleFontString)(0, 2, 1)), timeLeft(0.0f), fadeLeft(0.0f), permanent(0), isVisible(0) {
  string->SetIgnoreNewlines(1);
}

CSimpleMessageFrameLineNode::~CSimpleMessageFrameLineNode() {
  DELIFUSED(string);
}
