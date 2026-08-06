#include <Base/Base.h>

#include "Frame/CSimpleMessageScrollFrame.h"

#include "FrameXML/LoadXML.h"
#include "FrameXML/XMLTree.h"
#include "Gxu/IGxuFont.h"
#include "Services/TextBlock.h"

#include <malloc.h>

static const float DEFAULT_FADE_DURATION = 3.0f;
static const float DEFAULT_TIME_VISIBLE = 10.0f;

CSimpleMessageScrollFrame::CSimpleMessageScrollFrame(CSimpleFrame *parent, int maxLines) : CSimpleHyperlinkedFrame(parent) {
  ASSERT(maxLines > 0);

  m_atBottom = 1;
  m_fading = 1;
  m_numMessages = 0;
  m_maxMessages = maxLines;
  m_numDisplayed = 0;
  m_atTop = 0;
  m_textMaxSize = 0;
  m_fadeDuration = DEFAULT_FADE_DURATION;
  m_timeVisible = DEFAULT_TIME_VISIBLE;

  m_lines.SetCount(maxLines);

  m_currentScroll = 0;
  m_currentLine = -1;
}

CSimpleMessageScrollFrame::~CSimpleMessageScrollFrame() {
  m_lines.Clear();
  m_displayNodes.Clear();
  m_numDisplayed = 0;
  RefreshHyperlinks();
}

void CSimpleMessageScrollFrame::LoadXML(const XMLNode *node, CStatus *status) {
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

  value = node->GetAttributeByName("displayDuration");
  if (value && *value) {
    float timeVisible = SStrToFloat(value);
    if (timeVisible > 0.0f) {
      m_timeVisible = timeVisible;
    }
  }

  value = node->GetAttributeByName("fade");
  if (value && *value) {
    m_fading = StringToBOOL(value);
  }

  value = node->GetAttributeByName("maxLines");
  if (value && *value) {
    int maxLines = SStrToInt(value);
    if (maxLines > 0) {
      SetMaxLines(maxLines);
    }
  }
}

void CSimpleMessageScrollFrame::SetMaxLines(int maxLines) {
  ASSERT(maxLines > 0);

  m_lines.Clear();
  m_displayNodes.Clear();
  m_numMessages = 0;
  m_maxMessages = maxLines;
  m_numDisplayed = 0;
  m_atBottom = 1;
  m_atTop = 0;
  m_lines.SetCount(maxLines);
  m_currentLine = -1;
  m_currentScroll = 0;
}

void CSimpleMessageScrollFrame::SetMessageFrameInsets(float right, float left, float top, float bottom) {
  m_messageFrameInset.l = left;
  m_messageFrameInset.b = bottom;
  m_messageFrameInset.r = right;
  m_messageFrameInset.t = top;

  if (IsRectValid()) {
    OnFrameSizeChanged(m_rect);
  }
}

void CSimpleMessageScrollFrame::SetTextLength(int size) {
  m_textMaxSize = size;

  UINT count = m_displayNodes.Count();
  while (count) {
    m_displayNodes[--count].string->SetTextLength(size);
  }
}

void CSimpleMessageScrollFrame::AddMessage(LPCSTR text, const CSimpleFontStringAttributes *attrib) {
  FATALASSERT(text);

  if (m_numMessages < m_maxMessages) {
    ++m_numMessages;
  }

  if (++m_currentLine >= m_maxMessages) {
    m_currentLine -= m_maxMessages;
  }

  int nextScroll = m_currentScroll - m_numDisplayed + 1;
  if (nextScroll < 0) {
    nextScroll += m_maxMessages;
  }
  int scrollDown = m_currentLine == nextScroll;

  CSimpleMessageScrollFrameLine &line = m_lines[m_currentLine];
  FREEIFUSED(line.string);
  line.string = SStrDupA(text, __FILE__, __LINE__);
  line.isVisible = 1;
  line.timeLeft = m_timeVisible;
  line.fadeLeft = m_fadeDuration;
  line.attrib = attrib ? *attrib : m_attrib;

  if (m_atBottom) {
    ScrollMessages(m_currentLine);
  } else if (scrollDown) {
    ScrollDown();
  }
}

UINT CSimpleMessageScrollFrame::AddMultiLine(char *text, const CSimpleFontStringAttributes *attrib) {
  if (m_messageFrameArea.r - m_messageFrameArea.l <= 0.0f) {
    return 0;
  }

  CSimpleFontString           string(0, 2, 1);
  CSimpleFontStringAttributes attributes;
  attributes = attrib ? *attrib : m_attrib;
  attributes.UpdateString(&string, 0);

  UINT *lineOffsets = static_cast<UINT *>(_alloca(m_maxMessages * sizeof(*lineOffsets)));
  UINT  lines = string.WrapText(text, m_messageFrameArea.r - m_messageFrameArea.l, lineOffsets, m_maxMessages);

  for (UINT i = 0; i < lines; ++i) {
    if (i < lines - 1) {
      char saved = text[lineOffsets[i + 1]];
      text[lineOffsets[i + 1]] = 0;
      AddMessage(text + lineOffsets[i], &attributes);
      text[lineOffsets[i + 1]] = saved;
    } else {
      AddMessage(text + lineOffsets[i], &attributes);
    }
  }

  return lines;
}

void CSimpleMessageScrollFrame::Clear() {
  m_numMessages = 0;
  m_currentLine = 0;
  m_currentScroll = 0;
  ScrollMessages(0);
  m_atTop = 0;
  m_currentLine = -1;
}

void CSimpleMessageScrollFrame::OnFrameSizeChanged(const NTempest::CRect &rect) {
  CSimpleFrame::OnFrameSizeChanged(rect);

  float scale = GetLayoutScale();
  m_messageFrameArea.l = rect.l + m_messageFrameInset.l * scale;
  m_messageFrameArea.r = rect.r - m_messageFrameInset.r * scale;
  m_messageFrameArea.t = rect.t + m_messageFrameInset.t * scale;
  m_messageFrameArea.b = rect.b - m_messageFrameInset.b * scale;

  UINT  count = m_displayNodes.Count();
  float messageWidth = (m_messageFrameArea.r - m_messageFrameArea.l) / scale;
  for (UINT i = 0; i < count; ++i) {
    m_displayNodes[i].string->SetWidth(messageWidth);
  }

  ScrollMessages(m_currentScroll);
}

void CSimpleMessageScrollFrame::OnLayerUpdate(float elapsedSec) {
  int  i;
  UINT updateHyperlinks;

  CSimpleFrame::OnLayerUpdate(elapsedSec);

  if (!m_atBottom) {
    return;
  }

  updateHyperlinks = 0;
  for (i = 0; i < m_numDisplayed; ++i) {
    CSimpleMessageScrollFrameDisplayNode &node = m_displayNodes[i];
    CSimpleMessageScrollFrameLine        *line = node.line;

    if (!line->isVisible) {
      continue;
    }

    if (line->timeLeft != 0.0f) {
      line->timeLeft -= elapsedSec;
      if (line->timeLeft >= 0.0f) {
        continue;
      }
      if (line->fadeLeft != 0.0f) {
        line->timeLeft = 0.0f;
        continue;
      }
    } else if (line->fadeLeft != 0.0f) {
      line->fadeLeft -= elapsedSec;
      if (line->fadeLeft >= 0.0f) {
        BYTE alpha = static_cast<BYTE>(line->fadeLeft / m_fadeDuration * 255.0f);
        line->attrib.SetAlpha(alpha);
        node.attrib.SetAlpha(alpha);
        node.string->SetVertexColor(node.attrib.GetColor());
        continue;
      }
    } else {
      continue;
    }

    line->isVisible = 0;
    node.string->Hide();
    updateHyperlinks = 1;
  }

  if (updateHyperlinks) {
    RefreshHyperlinks();
  }
}

void CSimpleMessageScrollFrame::PageUp() {
  int count = m_numDisplayed;
  if (count) {
    while (count--) {
      if (!ScrollUp()) {
        return;
      }
    }
  }

  ScrollDown();
}

void CSimpleMessageScrollFrame::PageDown() {
  int count = m_numDisplayed;
  if (count) {
    while (count--) {
      if (!ScrollDown()) {
        return;
      }
    }
  }

  ScrollUp();
}

int CSimpleMessageScrollFrame::ScrollUp() {
  if (m_currentLine == -1) {
    return 0;
  }

  if (m_atTop) {
    RefreshMessages();
    return 0;
  }

  if (--m_currentScroll < 0) {
    m_currentScroll += m_maxMessages;
  }
  ScrollMessages(m_currentScroll);
  return 1;
}

int CSimpleMessageScrollFrame::ScrollDown() {
  if (m_currentLine == -1) {
    return 0;
  }

  if (m_atBottom) {
    RefreshMessages();
    return 0;
  }

  if (++m_currentScroll >= m_maxMessages) {
    m_currentScroll -= m_maxMessages;
  }
  ScrollMessages(m_currentScroll);
  return 1;
}

void CSimpleMessageScrollFrame::ScrollToTop() {
  if (m_currentLine == -1) {
    return;
  }

  int first = 0;
  if (m_numMessages == m_maxMessages) {
    first = m_currentLine + 1;
    if (first == m_maxMessages) {
      first = 0;
    }
  }
  ScrollMessages(first);
}

void CSimpleMessageScrollFrame::ScrollToBottom() {
  if (m_currentLine != -1) {
    if (m_atBottom) {
      RefreshMessages();
    } else {
      ScrollMessages(m_currentLine);
    }
  }
}

void CSimpleMessageScrollFrame::ScrollMessages(int start) {
  CSimpleMessageScrollFrameLine *line;
  int                            resetTimers;
  float                          sizeAvailable;
  int                            maxMessages;
  CSimpleFontString             *string;
  int                            current;
  float                          sizeNeeded;
  UINT                           added;

  if (m_currentLine == -1) {
    return;
  }

  m_numDisplayed = 0;
  m_currentScroll = start;
  resetTimers = 0;
  if (start == m_currentLine) {
    resetTimers = !m_atBottom;
    m_atBottom = 1;
  } else {
    m_atBottom = 0;
  }

  maxMessages = m_numMessages;
  sizeAvailable = (m_messageFrameArea.b - m_messageFrameArea.t) / GetLayoutScale();
  current = start;
  if (start != m_currentLine) {
    if (current >= m_currentLine) {
      current -= m_maxMessages;
    }
    maxMessages += current - m_currentLine;
  }

  UINT displayIndex = 0;
  current = start;
  while (static_cast<int>(displayIndex) < maxMessages) {
    added = displayIndex == m_displayNodes.Count();
    if (added) {
      m_displayNodes.SetCount(displayIndex + 1);
    }

    line = &m_lines[current];
    UpdateNode(&m_displayNodes[displayIndex], line, resetTimers);

    string = m_displayNodes[displayIndex].string;
    if (added) {
      string->SetFrame(this, 2, 1);
      string->SetWidth((m_messageFrameArea.r - m_messageFrameArea.l) / GetLayoutScale());
      if (displayIndex) {
        string->SetPoint(FRAMEPOINT_BOTTOMLEFT, m_displayNodes[displayIndex - 1].string, FRAMEPOINT_TOPLEFT, 0.0f, string->GetSpacing(), 1);
      } else {
        string->SetPoint(FRAMEPOINT_BOTTOMLEFT, this, FRAMEPOINT_BOTTOMLEFT, m_messageFrameInset.l, m_messageFrameInset.t, 1);
      }
      string->SetTextLength(m_textMaxSize);
    }

    sizeNeeded = string->GetStringHeight() + string->GetSpacing();
    if (sizeAvailable < sizeNeeded) {
      break;
    }

    if (line->isVisible) {
      string->Show();
    } else {
      string->Hide();
    }

    sizeAvailable -= sizeNeeded;
    if (--current < 0) {
      current += m_maxMessages;
    }
    ++m_numDisplayed;
    ++displayIndex;
  }

  m_atTop = static_cast<int>(displayIndex) == maxMessages;
  while (displayIndex < m_displayNodes.Count()) {
    m_displayNodes[displayIndex++].string->Hide();
  }

  RefreshHyperlinks();
}

void CSimpleMessageScrollFrame::UpdateNode(CSimpleMessageScrollFrameDisplayNode *node, CSimpleMessageScrollFrameLine *line, int resetTimers) {
  if (resetTimers || !m_atBottom) {
    line->attrib.SetAlpha(0xFF);
    line->isVisible = 1;
    line->timeLeft = m_timeVisible;
    line->fadeLeft = m_fadeDuration;
  }

  if (line->string) {
    node->line = line;
    node->attrib = line->attrib;
    node->attrib.UpdateString(node->string, 0);
    node->string->SetText(line->string);
  } else {
    node->string->SetText("");
  }

  node->string->Resize(1);
}

void CSimpleMessageScrollFrame::RefreshMessages() {
  int index = m_numDisplayed;
  while (index) {
    CSimpleMessageScrollFrameDisplayNode &node = m_displayNodes[--index];
    CSimpleMessageScrollFrameLine        *line = node.line;
    line->attrib.SetAlpha(0xFF);
    line->isVisible = 1;
    line->timeLeft = m_timeVisible;
    line->fadeLeft = m_fadeDuration;
    node.string->SetVertexColor(line->attrib.GetColor());
    node.string->Show();
  }

  RefreshHyperlinks();
}

void CSimpleMessageScrollFrame::RefreshHyperlinks() {
  CSimpleHyperlinkButton               *button;
  int                                   i;
  CSimpleMessageScrollFrameDisplayNode *node;
  const GXUFONTHYPERLINKINFO           *links;

  while ((button = m_hyperlinks.Head()) != 0) {
    m_hyperlinks.UnlinkNode(button);
    ReleaseHyperlinkButton(button);
  }

  i = m_numDisplayed;
  while (i) {
    node = &m_displayNodes[--i];

    if (node->line->isVisible) {
      CGxString *gxString = node->string->m_string ? TextBlockGetStringPtr(node->string->m_string) : 0;
      UINT       linkCount = GxuFontStringHyperLinkInfo(gxString, links);

      for (UINT linkIndex = 0; linkIndex < linkCount; ++linkIndex) {
        button = CreateHyperlinkButton();
        m_hyperlinks.LinkNode(button, LIST_TAIL, 0);
        button->SetHyperlink(node->string, &links[linkIndex]);
      }
    }
  }
}

CSimpleMessageScrollFrameDisplayNode::CSimpleMessageScrollFrameDisplayNode() : string(NEW(CSimpleFontStringRecord)(0, 1, 1)), line(0), attrib() {
  string->IncrRef();
  string->SetIgnoreNewlines(1);
}

CSimpleMessageScrollFrameDisplayNode::CSimpleMessageScrollFrameDisplayNode(const CSimpleMessageScrollFrameDisplayNode &rhs)
    : TRefCnt(rhs), string(rhs.string), line(rhs.line), attrib() {
  string->IncrRef();
  attrib = rhs.attrib;
  attrib.CopyFlags(rhs.attrib);
}

CSimpleMessageScrollFrameDisplayNode::~CSimpleMessageScrollFrameDisplayNode() {
  string->DecrRef();
}
