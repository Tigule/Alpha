#include "Frame/CSimpleEditBox.h"

#include "Base/ConvertUTF.h"
#include "Event/CMouseEvent.h"
#include "Event/CObserver.h"
#include "Frame/CSimpleRender.h"
#include "FrameXML/LoadXML.h"
#include "FrameXML/XMLTree.h"
#include "Gxu/IGxuFont.h"

#include <limits.h>
#include <string.h>

CSimpleEditBox *CSimpleEditBox::s_currentFocus;

CSimpleEditBox::CSimpleEditBox(CSimpleFrame *parent)
    : CSimpleFrame(parent),
      m_dirtyFlags(DIRTY_NONE),
      m_textHidden(0),
      m_textLength(0),
      m_textLengthMax(0),
      m_textLettersMax(0),
      m_visiblePos(0),
      m_visibleLen(0),
      m_highlightLeft(0),
      m_highlightRight(0),
      m_highlightDrag(0),
      m_cursor(0),
      m_cursorPos(0),
      m_cursorBlinkSpeed(0.5f),
      m_blinkElapsedTime(0.0f),
      m_password(0),
      m_multiline(0),
      m_autoFocus(1),
      m_numHistory(0),
      m_curHistory(0),
      m_imeInputMode(0),
      m_clauseHighlight(0),
      m_candidatesFrame(0),
      m_candidatesHighlight(0),
      m_onEnterPressed(0),
      m_onEscapePressed(0),
      m_onSpacePressed(0),
      m_onTabPressed(0),
      m_onTextChanged(0),
      m_onTextSet(0) {
  unsigned int i;

  m_textSize = 32;
  m_text = static_cast<char *>(ALLOC(0x20));
  *m_text = 0;

  m_textInfo = static_cast<unsigned int *>(ALLOC(4 * m_textSize));
  memset(m_textInfo, 0, 4 * m_textSize);

  m_string = NEW(CSimpleFontString)(this, 2, 1);
  m_string->SetHorizontalAlignment(1);
  SetMultiLine(0);

  for (i = 0; i < 3; ++i) {
    m_highlight[i] = NEW(CSimpleTexture)(this, 2, 0);
  }

  NTempest::CImVector highlightColor(0xFF606060UL);
  for (i = 0; i < 3; ++i) {
    m_highlight[i]->SetTexture(highlightColor);
  }

  m_cursor = NEW(CSimpleTexture)(this, 3, 1);
  m_cursor->Hide();

  for (i = 0; i < 6; ++i) {
    m_actions[i].obj = 0;
  }

  EnableEvent(SIMPLE_EVENT_CHAR, UINT_MAX);
  EnableEvent(SIMPLE_EVENT_KEY, UINT_MAX);
  EnableEvent(SIMPLE_EVENT_MOUSE, UINT_MAX);
}

CSimpleEditBox::~CSimpleEditBox() {
  ClearKeyboardFocus(this);
  SetHistoryLines(0);

  DELIFUSED(m_string);
  FREEIFUSED(m_text);
  FREEIFUSED(m_textHidden);
  DELIFUSED(m_cursor);

  SetOnEnterPressedScript(0);
  SetOnEscapePressedScript(0);
  SetOnSpacePressedScript(0);
  SetOnTabPressedScript(0);
  SetOnTextChangedScript(0);
  SetOnTextSetScript(0);
}

void CSimpleEditBox::LoadXML(const XMLNode *node, CStatus *status) {
  CSimpleFrame::LoadXML(node, status);

  const char *value = node->GetAttributeByName("letters");
  if (value && *value) {
    m_textLettersMax = SStrToInt(value);
  }

  value = node->GetAttributeByName("blinkSpeed");
  if (value && *value) {
    float speed = SStrToFloat(value);
    m_cursorBlinkSpeed = speed > 0.0f ? speed : 0.0f;
  }

  value = node->GetAttributeByName("password");
  if (value && *value) {
    m_password = StringToBOOL(value);
  }

  value = node->GetAttributeByName("multiLine");
  if (value && *value) {
    SetMultiLine(StringToBOOL(value));
  }

  value = node->GetAttributeByName("historyLines");
  if (value && *value) {
    int lines = SStrToInt(value);
    SetHistoryLines(lines > 0 ? lines : 0);
  }

  value = node->GetAttributeByName("autoFocus");
  if (value && *value) {
    SetAutoFocus(StringToBOOL(value));
  }

  for (const XMLNode *child = node->GetChild(); child; child = child->GetSibling()) {
    const char *name = child->GetName();

    if (!SStrCmpI(name, "FontString", INT_MAX)) {
      m_string->LoadXML(child, status);
      if (m_string->m_textMaxSize) {
        m_textLengthMax = m_string->m_textMaxSize - 1;
      }
    } else if (!SStrCmpI(name, "HighlightColor", INT_MAX)) {
      NTempest::CImVector color;
      if (LoadXML_Color(child, color, status)) {
        for (unsigned int i = 0; i < 3; ++i) {
          m_highlight[i]->SetTexture(color);
        }
      }
    } else if (!SStrCmpI(name, "TextInsets", INT_MAX)) {
      float left;
      float right;
      float top;
      float bottom;

      if (LoadXML_Insets(child, left, right, top, bottom, status)) {
        SetEditTextInsets(right, left, top, bottom);
      }
    }
  }
}

void CSimpleEditBox::LoadXML_Scripts(const XMLNode *node, CStatus *status) {
  CSimpleFrame::LoadXML_Scripts(node, status);

  for (const XMLNode *script = node->GetChild(); script; script = script->GetSibling()) {
    const char *name = script->GetName();

    if (!SStrCmpI(name, "OnEnterPressed", INT_MAX)) {
      SetOnEnterPressedScript(script->GetBody());
    } else if (!SStrCmpI(name, "OnEscapePressed", INT_MAX)) {
      SetOnEscapePressedScript(script->GetBody());
    } else if (!SStrCmpI(name, "OnSpacePressed", INT_MAX)) {
      SetOnSpacePressedScript(script->GetBody());
    } else if (!SStrCmpI(name, "OnTabPressed", INT_MAX)) {
      SetOnTabPressedScript(script->GetBody());
    } else if (!SStrCmpI(name, "OnTextChanged", INT_MAX)) {
      SetOnTextChangedScript(script->GetBody());
    } else if (!SStrCmpI(name, "OnTextSet", INT_MAX)) {
      SetOnTextSetScript(script->GetBody());
    }
  }
}

void CSimpleEditBox::SetMultiLine(int enabled) {
  m_multiline = enabled;
  m_visibleLines.SetCount(2);

  if (m_multiline) {
    m_string->SetVerticalAlignment(8);
    m_string->SetIgnoreNewlines(0);
  } else {
    m_string->SetVerticalAlignment(0x10);
    m_string->SetIgnoreNewlines(1);
  }

  UpdateSizes(m_rect);
}

void CSimpleEditBox::SetAutoFocus(int enabled) {
  m_autoFocus = enabled;
}

void CSimpleEditBox::SetEditTextInsets(float right, float left, float top, float bottom) {
  m_editTextInset.l = left;
  m_editTextInset.b = top;
  m_editTextInset.r = right;
  m_editTextInset.t = bottom;
  UpdateSizes(m_rect);
}

void CSimpleEditBox::OnLayerShow() {
  CSimpleFrame::OnLayerShow();

  if (!s_currentFocus && m_autoFocus) {
    SetKeyboardFocus(this);
  }
}

void CSimpleEditBox::OnLayerHide() {
  CSimpleFrame::OnLayerHide();
  ClearKeyboardFocus(this);
}

void CSimpleEditBox::OnLayerUpdate(float elapsedSec) {
  CSimpleFrame::OnLayerUpdate(elapsedSec);

  int textChanged = m_dirtyFlags & DIRTY_TEXT;

  if (m_dirtyFlags & DIRTY_CURSOR) {
    if (m_cursorPos < m_visiblePos || m_cursorPos > m_visiblePos + m_visibleLen) {
      m_dirtyFlags |= DIRTY_TEXT;
    }
  }

  if (m_dirtyFlags & DIRTY_TEXT) {
    m_string->SetText(m_text);
    m_visiblePos = 0;
    m_visibleLen = m_textLength;
    m_visibleLines[0] = 0;
    m_visibleLines[1] = m_textLength;
    m_dirtyFlags &= ~DIRTY_TEXT;
  }

  if (m_dirtyFlags & DIRTY_CURSOR) {
    UpdateVisibleCursor();
    m_dirtyFlags &= ~DIRTY_CURSOR;
  }

  if (m_cursorBlinkSpeed != 0.0f && m_cursorPos >= m_visiblePos && m_cursorPos <= m_visiblePos + m_visibleLen) {
    m_blinkElapsedTime += elapsedSec;

    if (s_currentFocus == this && m_blinkElapsedTime > m_cursorBlinkSpeed) {
      if (m_cursor->IsVisible()) {
        m_cursor->Hide();
      } else {
        m_cursor->Show();
      }

      m_blinkElapsedTime = 0.0f;
    }
  }

  if (textChanged && m_onTextChanged) {
    FrameScript_Execute(m_onTextChanged, this);
  }
}

void CSimpleEditBox::OnFrameSizeChanged(const NTempest::CRect &rect) {
  CSimpleFrame::OnFrameSizeChanged(rect);

  if (rect.l != m_rect.l || rect.r != m_rect.r || rect.t != m_rect.t || rect.b != m_rect.b) {
    UpdateSizes(rect);
  }
}

int CSimpleEditBox::OnLayerChar(CCharEvent &evt) {
  if (!m_visible) {
    return 0;
  }

  if (!s_currentFocus && m_autoFocus) {
    SetKeyboardFocus(this);
  } else if (s_currentFocus != this) {
    return 0;
  }

  FATALASSERT(!m_imeInputMode);
  Insert(evt.ch);

  if (evt.ch == ' ' && m_onSpacePressed) {
    FrameScript_Execute(m_onSpacePressed, this);
  }

  return 1;
}

int CSimpleEditBox::OnLayerKeyDown(CKeyEvent &evt) {
  if (!m_visible) {
    return 0;
  }

  if (!s_currentFocus && m_autoFocus) {
    SetKeyboardFocus(this);
  } else if (s_currentFocus != this) {
    return 0;
  }

  switch (evt.key) {
    case KEY_BACKSPACE:
      if (m_cursorPos > 0) {
        Delete(-1);
      }
      break;
    case KEY_ENTER:
      if (m_onEnterPressed) {
        FrameScript_Execute(m_onEnterPressed, this);
      }
      break;
    case KEY_ESCAPE:
      if (m_onEscapePressed) {
        FrameScript_Execute(m_onEscapePressed, this);
      }
      break;
    default:
      break;
  }

  return 1;
}

int CSimpleEditBox::OnLayerMouseDown(CMouseEvent &evt) {
  int handled = CSimpleFrame::OnLayerMouseDown(evt);

  if (!handled) {
    SetKeyboardFocus(this);
  }

  return handled;
}

void CSimpleEditBox::UpdateSizes(const NTempest::CRect &rect) {
  if (IsRectValid()) {
    m_string->ClearAllPoints(0);
    m_string->SetWidth((rect.r - rect.l) / m_layoutScale - (m_editTextInset.r + m_editTextInset.l));

    if (m_multiline) {
      m_string->SetHeight(0.0f);
    } else {
      m_string->SetHeight((rect.b - rect.t) / m_layoutScale - (m_editTextInset.b + m_editTextInset.t));
    }

    m_string->SetPoint(FRAMEPOINT_TOPLEFT, this, FRAMEPOINT_TOPLEFT, m_editTextInset.l, -m_editTextInset.b, 1);

    float        fontHeight = m_string->m_fontHeight;
    unsigned int i;

    for (i = 0; i < 3; ++i) {
      m_highlight[i]->ClearAllPoints(0);

      if (i == 1) {
        m_highlight[i]->SetPoint(FRAMEPOINT_TOPRIGHT, m_highlight[0], FRAMEPOINT_BOTTOMRIGHT, 0.0f, 0.0f, 1);
        m_highlight[i]->SetPoint(FRAMEPOINT_BOTTOMLEFT, m_highlight[2], FRAMEPOINT_TOPLEFT, 0.0f, 0.0f, 1);
      } else {
        m_highlight[i]->SetHeight(fontHeight);
      }
    }

    if (m_clauseHighlight) {
      m_clauseHighlight->SetHeight(fontHeight);
    }

    m_cursor->ClearAllPoints(0);
    m_cursor->SetWidth(0.003f);
    m_cursor->SetHeight(fontHeight);
    m_dirtyFlags |= DIRTY_TEXT | DIRTY_HIGHLIGHT | DIRTY_CURSOR;
  }
}

void CSimpleEditBox::UpdateTextInfo() {
  const char  *string = m_text;
  unsigned int offset = 0;
  unsigned int flags = 0;

  memset(m_textInfo, 0, sizeof(unsigned int) * m_textSize);

  while (*string) {
    unsigned int advance;
    unsigned int wide;
    QUOTEDCODE   code = GxuDetermineQuotedCode(string, advance, 0, 0, wide, m_textLength - offset);

    ASSERT(advance <= 0xFFFF);

    if (code == CODE_HYPERLINKSTART) {
      flags |= 0x80000000;
    }

    m_textInfo[offset] = flags | (static_cast<unsigned int>(code) << 16) | advance;

    offset += advance;
    string += advance;

    if (code == CODE_HYPERLINKSTOP) {
      flags &= 0x7FFFFFFF;
    }
  }
}

int CSimpleEditBox::GetNumToLen(int offset, int amount, bool checkHyperLink) {
  unsigned int *textInfo = m_textInfo;
  unsigned int *info = &textInfo[offset];
  int           length = 0;

  if (amount > 0) {
    while (amount-- > 0 && *info) {
      unsigned int value;
      unsigned int advance;
      unsigned int code;

      do {
        do {
          value = *info;
          advance = value & 0xFFFF;
          code = (value >> 16) & 0xFF;
          length += advance;
          info += advance;
        } while (!code);
      } while (code == CODE_HYPERLINKSTART || (checkHyperLink && (value & 0x80000000) && code != CODE_HYPERLINKSTOP));

      ASSERT((code == CODE_NEWLINE || code == CODE_PIPE || code == CODE_INVALIDCODE) || code == CODE_HYPERLINKSTOP);

      for (;;) {
        value = *info;
        advance = value & 0xFFFF;
        code = (value >> 16) & 0xFF;

        if (code != CODE_COLORRESTORE && code != CODE_HYPERLINKSTOP) {
          break;
        }

        length += advance;
        info += advance;
      }
    }
  } else if (amount < 0) {
    int remaining = -amount;

    while (remaining-- > 0 && info > textInfo) {
      unsigned int value;
      unsigned int advance;
      unsigned int code;

      do {
        do {
          --info;
        } while (info > textInfo && !*info);

        value = *info;
        advance = value & 0xFFFF;
        code = (value >> 16) & 0xFF;
        length += advance;
      } while (code == CODE_COLORRESTORE || code == CODE_HYPERLINKSTOP || (checkHyperLink && (value & 0x80000000) && code != CODE_HYPERLINKSTART));

      ASSERT((code == CODE_NEWLINE || code == CODE_PIPE || code == CODE_INVALIDCODE) || code == CODE_HYPERLINKSTART);

      if (info > textInfo) {
        for (;;) {
          do {
            --info;
          } while (info > textInfo && !*info);

          value = *info;
          advance = value & 0xFFFF;
          code = (value >> 16) & 0xFF;

          if (code && code != CODE_HYPERLINKSTART) {
            info += advance;
            break;
          }

          length += advance;
          if (info <= textInfo) {
            break;
          }
        }
      }
    }
  }

  return length;
}

int CSimpleEditBox::GetLenToNum(int offset, int amount) {
  unsigned int *textInfo = m_textInfo;
  unsigned int *info = &textInfo[offset];
  int           result = 0;

  if (amount > 0) {
    while (amount-- > 0) {
      if (*info) {
        unsigned int code = (*info >> 16) & 0xFF;
        if (code == CODE_NEWLINE || code == CODE_PIPE || code == CODE_INVALIDCODE) {
          ++result;
        }
      }

      ++info;
    }
  } else if (amount < 0) {
    int remaining = -amount;
    while (remaining-- > 0 && info > textInfo) {
      --info;
      if (*info) {
        unsigned int code = (*info >> 16) & 0xFF;
        if (code == CODE_NEWLINE || code == CODE_PIPE || code == CODE_INVALIDCODE) {
          ++result;
        }
      }
    }
  }

  return result;
}

int CSimpleEditBox::NextCharOffset(int offset) {
  return offset + (m_textInfo[offset] & 0xFFFF);
}

int CSimpleEditBox::PrevCharOffset(int offset) {
  do {
    --offset;
  } while (offset >= 0 && !m_textInfo[offset]);

  return offset;
}

void CSimpleEditBox::GrowText(int size) {
  if (size + 1 > m_textSize) {
    m_textSize = (size + 32) & ~31;
    m_text = static_cast<char *>(SMemReAlloc(m_text, m_textSize, __FILE__, __LINE__, 0));
    m_textInfo = static_cast<unsigned int *>(SMemReAlloc(m_textInfo, sizeof(unsigned int) * m_textSize, __FILE__, __LINE__, 0));
  }
}

void CSimpleEditBox::SetText(const char *text) {
  if (m_highlightLeft != m_highlightRight) {
    m_highlightRight = 0;
    m_highlightLeft = 0;
    m_dirtyFlags |= DIRTY_HIGHLIGHT;
  }

  m_cursorPos = m_textLength < 0 ? m_textLength : 0;
  m_dirtyFlags |= DIRTY_CURSOR;
  m_textLength = 0;
  Insert(text, 0);

  if (m_onTextSet) {
    FrameScript_Execute(m_onTextSet, this);
  }

  if (m_actions[EVENT_SET].obj) {
    CEvent event(m_actions[EVENT_SET].id, this);
    m_actions[EVENT_SET].obj->OnEvent(event);
  }
}

void CSimpleEditBox::Delete(int amount) {
  FATALASSERT(amount);

  int length = GetNumToLen(m_cursorPos, amount, 1);
  if (amount < 0) {
    DeleteSubstring(m_cursorPos - length, m_cursorPos);
  } else {
    DeleteSubstring(m_cursorPos, m_cursorPos + length);
  }
}

void CSimpleEditBox::Insert(const char *utf8string, int isIME) {
  if ((m_textInfo[m_cursorPos] & 0x80000000) && m_cursorPos > 0 && (m_textInfo[PrevCharOffset(m_cursorPos)] & 0x80000000)) {
    return;
  }

  if (m_highlightLeft != m_highlightRight) {
    DeleteHighlight();
  }

  if (!utf8string) {
    utf8string = "";
  }

  unsigned int length = SStrLen(utf8string);
  GrowText(length + m_textLength);

  char *insertion = &m_text[m_cursorPos];
  if (m_cursorPos < m_textLength) {
    memmove(insertion + length, insertion, m_textLength - m_cursorPos);
  }
  memcpy(insertion, utf8string, length);

  if (isIME) {
    m_highlightLeft = m_cursorPos;
    m_highlightRight = m_cursorPos + length;
    m_dirtyFlags |= DIRTY_HIGHLIGHT;
  }

  m_textLength += length;
  m_text[m_textLength] = 0;
  m_cursorPos += length;
  UpdateTextInfo();
  m_dirtyFlags |= DIRTY_TEXT | DIRTY_CURSOR;

  if (m_textLengthMax && m_textLength > m_textLengthMax) {
    int cursor = m_cursorPos;
    m_cursorPos = m_textLength < 0 ? 0 : m_textLength;
    m_dirtyFlags |= DIRTY_CURSOR;

    do {
      Delete(-1);
    } while (m_textLength > m_textLengthMax);

    if (cursor < 0) {
      cursor = 0;
    } else if (cursor > m_textLength) {
      cursor = m_textLength;
    }

    m_cursorPos = cursor;
    m_dirtyFlags |= DIRTY_CURSOR;
  }

  if (m_textLettersMax) {
    int textLength = m_textLength;
    if (GetLenToNum(0, textLength) > m_textLettersMax) {
      int cursor = m_cursorPos;
      m_cursorPos = textLength < 0 ? 0 : textLength;
      m_dirtyFlags |= DIRTY_CURSOR;

      while (GetLenToNum(0, m_textLength) > m_textLettersMax) {
        Delete(-1);
      }

      if (cursor < 0) {
        cursor = 0;
      } else if (cursor > m_textLength) {
        cursor = m_textLength;
      }

      m_cursorPos = cursor;
      m_dirtyFlags |= DIRTY_CURSOR;
    }
  }
}

void CSimpleEditBox::Insert(unsigned int utf16) {
  char utf8string[5];

  if (utf16 >= 0x20 && utf16 != 0x7F) {
    if (utf16 == '|') {
      Insert("||", 0);
    } else {
      sputu8(utf16, utf8string);
      Insert(utf8string, 0);
    }
  } else if (utf16 == '\t') {
    Insert("    ", 0);
  } else if (utf16 == '\n' && m_multiline) {
    Insert("\n", 0);
  }
}

void CSimpleEditBox::DeleteSubstring(int left, int right) {
  if (m_highlightLeft != m_highlightRight) {
    m_highlightRight = 0;
    m_highlightLeft = 0;
    m_dirtyFlags |= DIRTY_HIGHLIGHT;
  }

  int start = left;
  if (m_textInfo[left] & 0x80000000) {
    while (start > 0) {
      if (((m_textInfo[start] >> 16) & 0xFF) == CODE_HYPERLINKSTART) {
        break;
      }

      start = PrevCharOffset(start);
    }

    if (!(m_textInfo[start] & 0x80000000)) {
      start = NextCharOffset(start);
    }

    int stop = start;
    while (stop < m_textLength) {
      if (((m_textInfo[stop] >> 16) & 0xFF) == CODE_HYPERLINKSTOP) {
        break;
      }

      stop = NextCharOffset(stop);
    }

    if (stop > right) {
      right = stop;
    }
  }

  int removed = right - start;
  m_textLength -= removed;
  m_cursorPos = start;

  memcpy(&m_text[start], &m_text[start + removed], m_textLength - start);
  memcpy(&m_textInfo[start], &m_textInfo[start + removed], sizeof(unsigned int) * (m_textLength - start));

  m_text[m_textLength] = 0;
  m_dirtyFlags |= DIRTY_TEXT | DIRTY_CURSOR;
}

void CSimpleEditBox::DeleteHighlight() {
  DeleteSubstring(m_highlightLeft, m_highlightRight);
}

void CSimpleEditBox::SetHistoryLines(int numLines) {
  int i;

  if (numLines < m_numHistory) {
    for (i = numLines; i < m_numHistory; ++i) {
      FREEIFUSED(m_history[i]);
    }
  }

  m_history.SetCount(numLines);

  if (numLines > m_numHistory) {
    memset(&m_history[m_numHistory], 0, sizeof(char *) * (numLines - m_numHistory));
  }

  m_numHistory = numLines;
  if (m_curHistory >= numLines) {
    m_curHistory = numLines ? numLines - 1 : 0;
  }
}

void CSimpleEditBox::AddHistoryLine(const char *line) {
  if (!m_numHistory) {
    return;
  }

  if (m_history[m_curHistory]) {
    m_history[m_curHistory] = static_cast<char *>(SMemReAlloc(m_history[m_curHistory], SStrLen(line) + 1, __FILE__, __LINE__, 0));
  } else {
    m_history[m_curHistory] = static_cast<char *>(ALLOC(SStrLen(line) + 1));
  }

  SStrCopy(m_history[m_curHistory], line, INT_MAX);
  m_curHistory = (m_curHistory + 1) % m_numHistory;
}

void CSimpleEditBox::UpdateVisibleCursor() {
  if (m_cursorPos < m_visiblePos || m_cursorPos > m_visiblePos + m_visibleLen) {
    m_cursor->Hide();
    return;
  }

  char *text = m_password && m_textHidden ? m_textHidden : m_text;
  float offset_x = 0.0f;

  if (m_cursorPos != m_visiblePos) {
    offset_x = m_string->GetTextWidth(text + m_visiblePos, m_cursorPos - m_visiblePos);
  }

  m_cursor->SetPoint(FRAMEPOINT_LEFT, m_string, FRAMEPOINT_LEFT, offset_x, 0.0f, 0);
  m_cursor->Resize(1);
  m_blinkElapsedTime = 0.0f;

  if (s_currentFocus == this) {
    m_cursor->Show();
  }
}

void __fastcall CSimpleEditBox::SetKeyboardFocus(CSimpleEditBox *focus) {
  if (s_currentFocus) {
    s_currentFocus->m_cursor->Hide();
  }

  s_currentFocus = focus;
}

void __fastcall CSimpleEditBox::ClearKeyboardFocus(CSimpleEditBox *focus) {
  if (s_currentFocus == focus) {
    s_currentFocus->m_cursor->Hide();
    s_currentFocus = 0;
  }
}
