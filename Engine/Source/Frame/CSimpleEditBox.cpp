#include "Frame/CSimpleEditBox.h"

#include "Base/ConvertUTF.h"
#include "Event/CMouseEvent.h"
#include "Event/CObserver.h"
#include "Frame/CSimpleMessageFrame.h"
#include "Frame/CSimpleRender.h"
#include "FrameXML/LoadXML.h"
#include "FrameXML/XMLTree.h"
#include "Gxu/IGxuFont.h"
#include "Os/W32/OsClipboard.h"
#include "Os/W32/OsIME.h"
#include "Services/TextBlock.h"

#include <limits.h>
#include <malloc.h>
#include <math.h>
#include <string.h>
#include <wctype.h>

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

      NTempest::CImVector color;
      m_string->GetVertexColor(color);
      m_cursor->SetTexture(color);
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
    if (m_cursorPos < m_visiblePos || m_cursorPos > m_visiblePos + m_visibleLen
        || (m_imeInputMode && m_clauseRight > m_visiblePos + m_visibleLen)) {
      m_dirtyFlags |= DIRTY_TEXT;
    }
  }

  if (m_dirtyFlags & DIRTY_TEXT) {
    UpdateVisibleText();
    m_dirtyFlags &= ~DIRTY_TEXT;
  }

  if (m_dirtyFlags & DIRTY_HIGHLIGHT) {
    UpdateVisibleHighlight();
    m_dirtyFlags &= ~DIRTY_HIGHLIGHT;
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

  if (textChanged) {
    DispatchAction(EVENT_CHANGED);
  }
}

int CSimpleEditBox::OnLayerTrackUpdate(const CMouseEvent &evt) {
  if (!CSimpleFrame::OnLayerTrackUpdate(evt)) {
    return 0;
  }

  int position;
  if (m_highlightDrag && ConvertCoordinateToIndex(evt.x, evt.y, position)) {
    ExtendHighlight(position - m_cursorPos);

    if (position < 0) {
      position = 0;
    } else if (position > m_textLength) {
      position = m_textLength;
    }

    m_cursorPos = position;
    m_dirtyFlags |= DIRTY_CURSOR;
  }

  return 1;
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

  if (evt.ch == ' ') {
    DispatchAction(EVENT_SPACE);
  }

  return 1;
}

int CSimpleEditBox::OnLayerIme(CImeEvent &evt) {
  if (!m_visible) {
    return 0;
  }

  if (!s_currentFocus && m_autoFocus) {
    SetKeyboardFocus(this);
  } else if (s_currentFocus != this) {
    return 0;
  }

  if (m_password) {
    return 1;
  }

  char string[512];

  switch (evt.message) {
    case 1:
      m_imeInputMode = 1;
      m_highlightDrag = 0;
      break;

    case 2:
      if (evt.lParam & 4) {
        OsIMEGetCompositionResult(string, sizeof(string));
        Insert(string, 0);
      }
      if (evt.lParam & 2) {
        OsIMEGetCompositionString(string, sizeof(string));
        Insert(string, 1);
      }
      if (evt.lParam & 1) {
        UpdateClauseInfo();
      }
      break;

    case 3:
    case 4:
      if (PopulateCandidates(evt.lParam)) {
        ShowCandidates();
      } else {
        HideCandidates();
      }
      break;

    case 5:
      HideCandidates();
      break;

    case 6:
      HideCandidates();
      m_imeInputMode = 0;
      break;
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

  int control = EventIsKeyDown(KEY_CONTROL);
  int shift = EventIsKeyDown(KEY_SHIFT);

  switch (evt.key) {
    case KEY_A:
    case static_cast<KEY>(KEY_A + 32):
      if (control) {
        HighlightText();
      }
      break;

    case KEY_B:
    case static_cast<KEY>(KEY_B + 32):
      if (control) {
        MoveBackward(shift);
      }
      break;

    case KEY_C:
    case static_cast<KEY>(KEY_C + 32):
    case KEY_X:
    case static_cast<KEY>(KEY_X + 32):
      if (control && m_highlightLeft != m_highlightRight) {
        CopyToClipboard();
        if (evt.key == KEY_X || evt.key == static_cast<KEY>(KEY_X + 32)) {
          DeleteHighlight();
        }
      }
      break;

    case KEY_D:
    case static_cast<KEY>(KEY_D + 32):
      if (control) {
        DeleteForward();
      }
      break;

    case KEY_F:
    case static_cast<KEY>(KEY_F + 32):
      if (control) {
        MoveForward(shift);
      }
      break;

    case KEY_K:
    case static_cast<KEY>(KEY_K + 32):
      if (control) {
        DeleteToEnd();
      }
      break;

    case KEY_N:
    case static_cast<KEY>(KEY_N + 32):
      if (control) {
        ForwardHistory();
      }
      break;

    case KEY_P:
    case static_cast<KEY>(KEY_P + 32):
      if (control) {
        BackwardHistory();
      }
      break;

    case KEY_U:
    case static_cast<KEY>(KEY_U + 32):
      if (control) {
        DeleteToStart();
      }
      break;

    case KEY_V:
    case static_cast<KEY>(KEY_V + 32):
      if (control) {
        PasteFromClipboard();
      }
      break;

    case KEY_W:
    case static_cast<KEY>(KEY_W + 32):
      if (control) {
        DeleteBackwardWord();
      }
      break;

    case KEY_ESCAPE:
      if (m_onEscapePressed) {
        FrameScript_Execute(m_onEscapePressed, this);
      }
      DispatchAction(EVENT_ESCAPE);
      break;

    case KEY_ENTER:
      if (m_multiline) {
        Insert("\n", 0);
      } else {
        if (m_onEnterPressed) {
          FrameScript_Execute(m_onEnterPressed, this);
        }
        DispatchAction(EVENT_ENTER);
      }
      break;

    case KEY_BACKSPACE:
      if (control) {
        DeleteBackwardWord();
      } else {
        DeleteBackward();
      }
      break;

    case KEY_TAB:
      if (m_onTabPressed) {
        FrameScript_Execute(m_onTabPressed, this);
      }
      DispatchAction(EVENT_TAB);
      break;

    case KEY_LEFT:
      if (control) {
        MoveBackwardWord(shift);
      } else {
        MoveBackward(shift);
      }
      break;

    case KEY_UP:
      if (m_multiline) {
        MoveBackwardLine(shift);
      } else {
        BackwardHistory();
      }
      break;

    case KEY_RIGHT:
      if (control) {
        MoveForwardWord(shift);
      } else {
        MoveForward(shift);
      }
      break;

    case KEY_DOWN:
      if (m_multiline) {
        MoveForwardLine(shift);
      } else {
        ForwardHistory();
      }
      break;

    case KEY_INSERT:
      if (control) {
        if (m_highlightLeft != m_highlightRight) {
          CopyToClipboard();
        }
      } else if (shift) {
        PasteFromClipboard();
      }
      break;

    case KEY_DELETE:
      if (shift) {
        if (m_highlightLeft != m_highlightRight) {
          CopyToClipboard();
          DeleteHighlight();
        }
      } else if (control) {
        DeleteForwardWord();
      } else {
        DeleteForward();
      }
      break;

    case KEY_HOME:
      MoveToStart(shift);
      break;

    case KEY_END:
      MoveToEnd(shift);
      break;

    default:
      break;
  }

  return 1;
}

int CSimpleEditBox::OnLayerKeyDownRepeat(CKeyEvent &evt) {
  if (!m_visible) {
    return 0;
  }

  if (!s_currentFocus && m_autoFocus) {
    SetKeyboardFocus(this);
  } else if (s_currentFocus != this) {
    return 0;
  }

  return OnLayerKeyDown(evt);
}

int CSimpleEditBox::OnLayerKeyUp(CKeyEvent &evt) {
  if (!m_visible) {
    return 0;
  }

  if (!s_currentFocus && m_autoFocus) {
    SetKeyboardFocus(this);
  }

  return s_currentFocus == this;
}

int CSimpleEditBox::OnLayerMouseDown(CMouseEvent &evt) {
  int handled = CSimpleFrame::OnLayerMouseDown(evt);

  if (!handled) {
    if (!m_imeInputMode) {
      int position;

      if (ConvertCoordinateToIndex(evt.x, evt.y, position)) {
        if (m_highlightLeft != m_highlightRight) {
          m_highlightLeft = 0;
          m_highlightRight = 0;
          m_dirtyFlags |= DIRTY_HIGHLIGHT;
        }

        if (position < 0) {
          position = 0;
        } else if (position > m_textLength) {
          position = m_textLength;
        }

        m_cursorPos = position;
        m_dirtyFlags |= DIRTY_CURSOR;
        StartHighlight();
        m_highlightDrag = 1;
        handled = 1;
      }
    }

    SetKeyboardFocus(this);
  }

  return handled;
}

int CSimpleEditBox::OnLayerMouseUp(CMouseEvent &evt) {
  int handled = CSimpleFrame::OnLayerMouseUp(evt);

  if (!handled && m_highlightDrag) {
    m_highlightDrag = 0;
    return 1;
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

int CSimpleEditBox::GetOffsetToLine(int offset) {
  int line = 0;
  int maxLine = m_visibleLines.Count() - 2;

  while (offset >= static_cast<int>(m_visibleLines[line + 1]) && line < maxLine) {
    ++line;
  }

  return line;
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

void CSimpleEditBox::DeleteForward() {
  if (m_highlightLeft != m_highlightRight) {
    DeleteHighlight();
  } else if (m_cursorPos < m_textLength) {
    Delete(1);
  }
}

void CSimpleEditBox::DeleteForwardWord() {
  if (m_highlightLeft != m_highlightRight) {
    DeleteHighlight();
    return;
  }

  int advance;
  while (m_cursorPos < m_textLength && iswspace(sgetu8(reinterpret_cast<const unsigned char *>(&m_text[m_cursorPos]), &advance))) {
    Delete(1);
  }
  while (m_cursorPos < m_textLength && !iswspace(sgetu8(reinterpret_cast<const unsigned char *>(&m_text[m_cursorPos]), &advance))) {
    Delete(1);
  }
}

void CSimpleEditBox::DeleteBackward() {
  if (m_highlightLeft != m_highlightRight) {
    DeleteHighlight();
  } else if (m_cursorPos > 0) {
    Delete(-1);
  }
}

void CSimpleEditBox::DeleteBackwardWord() {
  if (m_highlightLeft != m_highlightRight) {
    DeleteHighlight();
    return;
  }

  int advance;
  while (m_cursorPos > 0 && iswspace(sgetu8(reinterpret_cast<const unsigned char *>(&m_text[PrevCharOffset(m_cursorPos)]), &advance))) {
    Delete(-1);
  }
  while (m_cursorPos > 0 && !iswspace(sgetu8(reinterpret_cast<const unsigned char *>(&m_text[PrevCharOffset(m_cursorPos)]), &advance))) {
    Delete(-1);
  }
}

void CSimpleEditBox::DeleteToStart() {
  int amount = GetLenToNum(0, m_cursorPos);
  if (amount) {
    Delete(-amount);
  }
}

void CSimpleEditBox::DeleteToEnd() {
  int amount = GetLenToNum(m_cursorPos, m_textLength - m_cursorPos);
  if (amount) {
    Delete(amount);
  }
}

void CSimpleEditBox::DeleteText() {
  DeleteSubstring(0, m_textLength);
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

void CSimpleEditBox::Move(int distance, int highlight) {
  FATALASSERT(distance);

  int length = GetNumToLen(m_cursorPos, distance, 1);
  if (distance < 0) {
    length = -length;
  }

  if (highlight) {
    if (m_highlightLeft == m_highlightRight) {
      StartHighlight();
    }
    ExtendHighlight(length);
  } else if (m_highlightLeft != m_highlightRight) {
    m_highlightLeft = 0;
    m_highlightRight = 0;
    m_dirtyFlags |= DIRTY_HIGHLIGHT;
  }

  m_cursorPos += length;
  m_dirtyFlags |= DIRTY_CURSOR;
}

void CSimpleEditBox::MoveForward(int highlight) {
  if (m_cursorPos < m_textLength) {
    Move(1, highlight);
  }
}

void CSimpleEditBox::MoveForwardWord(int highlight) {
  int advance;
  while (m_cursorPos < m_textLength && iswspace(sgetu8(reinterpret_cast<const unsigned char *>(&m_text[m_cursorPos]), &advance))) {
    Move(1, highlight);
  }
  while (m_cursorPos < m_textLength && !iswspace(sgetu8(reinterpret_cast<const unsigned char *>(&m_text[m_cursorPos]), &advance))) {
    Move(1, highlight);
  }
}

void CSimpleEditBox::MoveBackward(int highlight) {
  if (m_cursorPos > 0) {
    Move(-1, highlight);
  }
}

void CSimpleEditBox::MoveBackwardWord(int highlight) {
  int advance;
  while (m_cursorPos > 0 && iswspace(sgetu8(reinterpret_cast<const unsigned char *>(&m_text[m_cursorPos]), &advance))) {
    Move(-1, highlight);
  }
  while (m_cursorPos > 0 && !iswspace(sgetu8(reinterpret_cast<const unsigned char *>(&m_text[m_cursorPos]), &advance))) {
    Move(-1, highlight);
  }
}

void CSimpleEditBox::MoveToStart(int highlight) {
  int amount = GetLenToNum(m_cursorPos, -m_cursorPos);
  if (amount) {
    Move(-amount, highlight);
  }
}

void CSimpleEditBox::MoveToEnd(int highlight) {
  int amount = GetLenToNum(m_cursorPos, m_textLength - m_cursorPos);
  if (amount) {
    Move(amount, highlight);
  }
}

void CSimpleEditBox::MoveLine(int distance, int highlight) {
  int oldLine = GetOffsetToLine(m_cursorPos);
  int newLine = oldLine + distance;
  int lastLine = m_visibleLines.Count() - 2;

  if (newLine < 0) {
    newLine = 0;
  } else if (newLine > lastLine) {
    newLine = lastLine;
  }

  if (newLine == oldLine) {
    return;
  }

  int column = GetLenToNum(m_visibleLines[oldLine], m_cursorPos - m_visibleLines[oldLine]);
  int newOffset = m_visibleLines[newLine] + GetNumToLen(m_visibleLines[newLine], column, false);

  if (m_visibleLines[newLine + 1] > m_visibleLines[newLine] && newOffset >= static_cast<int>(m_visibleLines[newLine + 1])) {
    newOffset = m_visibleLines[newLine + 1] - GetNumToLen(m_visibleLines[newLine + 1], -1, false);
  }

  int movement = newOffset - m_cursorPos;
  if (highlight) {
    if (m_highlightLeft == m_highlightRight) {
      StartHighlight();
    }
    ExtendHighlight(movement);
  } else if (m_highlightLeft != m_highlightRight) {
    m_highlightLeft = 0;
    m_highlightRight = 0;
    m_dirtyFlags |= DIRTY_HIGHLIGHT;
  }

  m_cursorPos += movement;
  m_dirtyFlags |= DIRTY_CURSOR;
}

void CSimpleEditBox::MoveForwardLine(int highlight) {
  MoveLine(1, highlight);
}

void CSimpleEditBox::MoveBackwardLine(int highlight) {
  MoveLine(-1, highlight);
}

void CSimpleEditBox::HighlightText() {
  m_highlightRight = m_textLength;
  m_highlightLeft = 0;
  m_dirtyFlags |= DIRTY_HIGHLIGHT;
}

void CSimpleEditBox::StartHighlight() {
  m_highlightLeft = m_cursorPos;
  m_highlightRight = m_cursorPos;
}

void CSimpleEditBox::ExtendHighlight(int distance) {
  if (!distance) {
    return;
  }

  int position = m_cursorPos + distance;
  if (distance >= 0 && position > m_highlightRight) {
    m_highlightRight = position;
  } else if (distance < 0 && position >= m_highlightLeft) {
    m_highlightRight = position;
  } else {
    m_highlightLeft = position;
  }
  m_dirtyFlags |= DIRTY_HIGHLIGHT;
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

void CSimpleEditBox::ForwardHistory() {
  for (int i = 1; i <= m_numHistory; ++i) {
    int index = (m_curHistory + i) % m_numHistory;

    if (m_history[index]) {
      m_curHistory = index;
      SetText(m_history[index]);
      return;
    }
  }
}

void CSimpleEditBox::BackwardHistory() {
  for (int i = 1; i <= m_numHistory; ++i) {
    int index = m_curHistory - i;

    if (index < 0) {
      index += m_numHistory;
    }

    if (m_history[index]) {
      m_curHistory = index;
      SetText(m_history[index]);
      return;
    }
  }
}

int CSimpleEditBox::ConvertCoordinateToIndex(float x, float y, int &position) {
  NTempest::CRect stringRect;
  if (!m_string->GetRect(&stringRect)) {
    return 0;
  }

  unsigned int line = 0;
  unsigned int lastLine = m_visibleLines.Count() - 2;
  float fontHeight = (m_string->m_fontHeight + m_string->m_spacing) * m_layoutScale;

  if (m_multiline) {
    float distance = stringRect.b - y;
    while (distance > fontHeight && line < lastLine) {
      distance -= fontHeight;
      ++line;
    }
  }

  float offset = x - stringRect.l;
  if (offset < 0.0f) {
    position = 0;
    return 1;
  }
  if (offset > stringRect.r) {
    offset = stringRect.r;
  }

  const char *text = m_password ? m_textHidden : m_text;
  unsigned int linePosition = m_visibleLines[line];
  unsigned int amount = m_string->GetNumCharsWithinWidth(
      text + linePosition,
      m_visibleLines[line + 1] - linePosition,
      offset / m_string->m_layoutScale);

  if (text == m_text) {
    amount = GetNumToLen(linePosition, amount, false);
  }

  position = linePosition + amount;
  return 1;
}

void CSimpleEditBox::MakeTextVisible(int position, float offset, float stringWidth) {
  ASSERT(!m_password);

  if (m_multiline) {
    ASSERT(!"FIXME: Not yet implemented");
    return;
  }

  unsigned int amount = m_string->GetNumCharsWithinWidthFromEnd(m_text, position, offset);
  m_visiblePos = position - GetNumToLen(position, -static_cast<int>(amount), false);
  if (m_visiblePos < 0) {
    m_visiblePos = 0;
  }

  amount = m_string->GetNumCharsWithinWidth(m_text + m_visiblePos, 0, stringWidth);
  m_visibleLen = GetNumToLen(m_visiblePos, amount, false);
  m_visibleLines[0] = m_visiblePos;
  m_visibleLines[1] = m_visiblePos + m_visibleLen;
}

void CSimpleEditBox::UpdateVisibleText() {
  ASSERT(m_cursorPos >= 0 && m_cursorPos <= m_textLength);

  float stringWidth = m_string->GetWidth();
  ASSERT(stringWidth > 0.0f);

  char *text;
  if (m_password) {
    unsigned int length = SStrLen(m_text);
    m_textHidden = static_cast<char *>(SMemReAlloc(m_textHidden, length + 1, __FILE__, __LINE__, 0));
    memset(m_textHidden, '*', length);
    m_textHidden[length] = 0;
    text = m_textHidden;
  } else {
    text = m_text;
  }

  if (m_multiline) {
    unsigned int lines = m_string->WrapText(
        text + m_visiblePos,
        stringWidth,
        m_visibleLines.Ptr(),
        m_visibleLines.Count());

    if (lines >= m_visibleLines.Count()) {
      m_visibleLines.SetCount(lines + 1);
      m_string->WrapText(text + m_visiblePos, stringWidth, m_visibleLines.Ptr(), m_visibleLines.Count());
    }

    if (!lines) {
      lines = 1;
      m_visibleLines[0] = 0;
    }

    if (m_visiblePos > 0) {
      for (unsigned int i = 0; i < lines; ++i) {
        m_visibleLines[i] += m_visiblePos;
      }
    }

    m_visibleLines[lines] = SStrLen(text);
    m_visibleLen = m_visibleLines[lines];
    m_visibleLines.SetCount(lines + 1);
  } else {
    unsigned int amount = m_string->GetNumCharsWithinWidth(text + m_visiblePos, 0, stringWidth);
    m_visibleLen = amount;
    if (text == m_text) {
      m_visibleLen = GetNumToLen(m_visiblePos, amount, false);
    }

    m_visibleLines[0] = m_visiblePos;
    m_visibleLines[1] = m_visiblePos + m_visibleLen;
  }

  if (!m_password) {
    if (m_imeInputMode && m_clauseRight > m_visiblePos + m_visibleLen) {
      MakeTextVisible(m_clauseRight, stringWidth, stringWidth);
    }

    if (m_cursorPos < m_visiblePos || m_cursorPos > m_visiblePos + m_visibleLen) {
      float offset = m_cursorPos >= m_visiblePos ? stringWidth * 0.75f : stringWidth * 0.25f;
      MakeTextVisible(m_cursorPos, offset, stringWidth);
      ASSERT(m_cursorPos >= m_visiblePos && m_cursorPos <= m_visiblePos + m_visibleLen);
    }
  }

  int end = m_visiblePos + m_visibleLen;
  char saved = text[end];
  text[end] = 0;
  m_string->SetText(text + m_visiblePos);
  text[end] = saved;

  if (m_multiline) {
    SetHeight(m_string->GetStringHeight() + m_editTextInset.b + m_editTextInset.t);
  }
}

void CSimpleEditBox::CopyToClipboard() {
  if (m_highlightLeft == m_highlightRight) {
    return;
  }

  if (m_password) {
    OsClipboardPutString("");
    return;
  }

  unsigned int length = m_highlightRight - m_highlightLeft;
  char *buffer = static_cast<char *>(_alloca(length + 1));
  GxuFontStripEscapeCodes(m_text + m_highlightLeft, length, 0x500, buffer, length + 1);
  OsClipboardPutString(buffer);
}

void CSimpleEditBox::PasteFromClipboard() {
  char *string = OsClipboardGetString();
  if (!string) {
    return;
  }

  const char *position = string;
  while (*position) {
    int advance;
    Insert(sgetu8(reinterpret_cast<const unsigned char *>(position), &advance));
    position += advance;
  }

  OsClipboardFreeString(string);
}

void CSimpleEditBox::SetFont(const char *fontName, float fontHeight, unsigned int fontFlags) {
  m_string->SetFont(fontName, fontHeight, fontFlags);

  if (m_candidatesFrame) {
    m_candidatesFrame->m_attrib.m_font = fontName;
    m_candidatesFrame->m_attrib.m_fontHeight = fontHeight;
    m_candidatesFrame->m_attrib.m_fontFlags = fontFlags;
    m_candidatesFrame->m_attrib.m_flags |= CSimpleFontStringAttributes::FLAG_FONT;
  }

  UpdateSizes(m_rect);
}

void CSimpleEditBox::CreateClauseHighlight() {
  if (m_clauseHighlight) {
    return;
  }

  m_clauseHighlight = NEW(CSimpleTexture)(this, 3, 1);
  m_clauseHighlight->SetTexture(NTempest::CImVector(0xFF202010));
  m_clauseHighlight->SetBlendMode(GxBlend_Add);
  m_clauseHighlight->SetHeight(m_string->m_fontHeight);
}

void CSimpleEditBox::CreateCandidatesFrame() {
  if (m_candidatesFrame) {
    return;
  }

  CreateClauseHighlight();

  float fontHeight = m_string->m_fontHeight;
  m_candidatesFrame = NEW(CSimpleMessageFrame)(this);

  const char *fontName = m_string->m_font ? TextBlockGetFontName(m_string->m_font) : 0;
  unsigned int fontFlags = m_string->m_font ? TextBlockGetFontFlags(m_string->m_font) : 0;
  m_candidatesFrame->m_attrib.m_font = fontName;
  m_candidatesFrame->m_attrib.m_fontHeight = fontHeight;
  m_candidatesFrame->m_attrib.m_fontFlags = fontFlags;
  m_candidatesFrame->m_attrib.m_flags |= CSimpleFontStringAttributes::FLAG_FONT;
  m_candidatesFrame->SetWidth(fontHeight * 10.0f);
  m_candidatesFrame->SetPoint(
      FRAMEPOINT_BOTTOMLEFT,
      m_clauseHighlight,
      FRAMEPOINT_TOPLEFT,
      0.0f,
      0.0f,
      1);
  m_candidatesFrame->SetInsertMode(CSimpleMessageFrame::INSERT_AT_TOP);

  CSimpleTexture *background = NEW(CSimpleTexture)(m_candidatesFrame, 0, 1);
  background->SetTexture(NTempest::CImVector(0xFF606060));
  background->SetAllPoints(m_candidatesFrame, 1);

  m_candidatesHighlight = NEW(CSimpleTexture)(m_candidatesFrame, 2, 1);
  m_candidatesHighlight->SetTexture(NTempest::CImVector(0xFF808080));
}

void CSimpleEditBox::ShowCandidates() {
  if (m_candidatesFrame) {
    m_candidatesFrame->Show();
  }
}

void CSimpleEditBox::HideCandidates() {
  if (m_candidatesFrame) {
    m_candidatesFrame->Hide();
  }
}

void CSimpleEditBox::UpdateLanguageIndicator() {
}

void CSimpleEditBox::UpdateClauseInfo() {
  unsigned int clauseLeft;
  unsigned int clauseRight;
  unsigned int cursorPosition;

  if (OsIMEGetClauseInfo(clauseLeft, clauseRight, cursorPosition)) {
    CreateClauseHighlight();
    m_clauseLeft = m_highlightLeft + GetNumToLen(m_highlightLeft, clauseLeft, false);
    m_clauseRight = m_highlightLeft + GetNumToLen(m_highlightLeft, clauseRight, false);
    m_cursorPos = m_highlightLeft + GetNumToLen(m_highlightLeft, cursorPosition, false);
    m_dirtyFlags |= DIRTY_HIGHLIGHT | DIRTY_CURSOR;
  }
}

int CSimpleEditBox::PopulateCandidates(unsigned long which) {
  unsigned int pageSize;
  unsigned int count;
  unsigned int selection;
  TSGrowableArray<OsIMECandidate> candidates;

  if (!OsIMEGetCandidates(which, pageSize, count, selection, candidates)) {
    return 0;
  }

  CreateCandidatesFrame();

  float fontHeight = m_string->m_fontHeight;
  m_candidatesFrame->Clear();
  m_candidatesFrame->SetHeight((pageSize + 1) * fontHeight + fontHeight * 0.1f);

  m_candidatesHighlight->ClearAllPoints(1);
  unsigned int row = selection % pageSize;
  m_candidatesHighlight->SetPoint(
      FRAMEPOINT_TOPLEFT,
      m_candidatesFrame,
      FRAMEPOINT_TOPLEFT,
      0.0f,
      -row * fontHeight,
      1);
  m_candidatesHighlight->SetPoint(
      FRAMEPOINT_BOTTOMRIGHT,
      m_candidatesFrame,
      FRAMEPOINT_TOPRIGHT,
      0.0f,
      -(row + 1) * fontHeight,
      1);

  NTempest::CImVector white(0xFFFFFFFF);
  char candidate[1024];
  SStrPrintf(candidate, sizeof(candidate), "> %d/%d", selection + 1, count);
  m_candidatesFrame->AddMessage(candidate, white, 0.0f, 0);

  for (unsigned int index = pageSize; index-- > 0;) {
    if (candidates[index].candidate[0]) {
      SStrPrintf(candidate, sizeof(candidate), "%d: %s", index + 1, candidates[index].candidate);
    } else {
      SStrCopy(candidate, " ", sizeof(candidate));
    }
    m_candidatesFrame->AddMessage(candidate, white, 0.0f, 0);
  }

  m_candidatesFrame->Resize(1);
  return 1;
}

void CSimpleEditBox::DispatchAction(int action) {
  if (m_actions[action].obj) {
    CEvent event(m_actions[action].id, this);
    m_actions[action].obj->OnEvent(event);
  }
}

void CSimpleEditBox::UpdateVisibleHighlight() {
  if (m_clauseHighlight) {
    if (m_imeInputMode) {
      UpdateHighlightArea(m_clauseHighlight, m_clauseLeft, m_clauseRight);
    } else {
      m_clauseHighlight->Hide();
    }
  }

  int firstLine = GetOffsetToLine(m_highlightLeft);
  int lastLine = GetOffsetToLine(m_highlightRight);
  int numLines = lastLine - firstLine + 1;

  if (firstLine == lastLine) {
    UpdateHighlightArea(m_highlight[0], m_highlightLeft, m_highlightRight);
    m_highlight[1]->Hide();
    m_highlight[2]->Hide();
  } else {
    UpdateHighlightArea(m_highlight[0], m_highlightLeft, m_visibleLines[firstLine + 1]);

    if (numLines == 2) {
      m_highlight[1]->Hide();
    } else {
      m_highlight[1]->Show();
    }

    UpdateHighlightArea(m_highlight[2], m_visibleLines[lastLine], m_highlightRight);
  }
}

void CSimpleEditBox::UpdateVisibleCursor() {
  if (m_cursorPos < m_visiblePos || m_cursorPos > m_visiblePos + m_visibleLen) {
    m_cursor->Hide();
    return;
  }

  unsigned int line = 0;
  unsigned int lastLine = m_visibleLines.Count() - 2;
  float fontHeight = m_string->m_spacing + m_string->m_fontHeight;
  float offsetY = 0.0f;

  while (m_cursorPos >= static_cast<int>(m_visibleLines[line + 1]) && line < lastLine) {
    ++line;
    offsetY -= fontHeight;
  }

  float offsetX;
  if (m_cursorPos == static_cast<int>(m_visibleLines[line])) {
    offsetX = 0.0f;
  } else {
    const char *text = m_password ? m_textHidden : m_text;
    offsetX = m_string->GetTextWidth(
        text + m_visibleLines[line],
        m_cursorPos - m_visibleLines[line]);
  }

  FRAMEPOINT point = m_multiline ? FRAMEPOINT_TOPLEFT : FRAMEPOINT_LEFT;
  m_cursor->SetPoint(point, m_string, point, offsetX, offsetY, 0);
  m_cursor->Resize(1);
  m_blinkElapsedTime = 0.0f;

  if (s_currentFocus == this) {
    m_cursor->Show();
  }
}

void CSimpleEditBox::UpdateHighlightArea(CSimpleRegion *area, int left, int right) {
  if (m_multiline) {
    ASSERT(!m_password);
  }

  const char *text = m_password ? m_textHidden : m_text;
  unsigned int line = 0;
  unsigned int lastLine = m_visibleLines.Count() - 2;
  float fontHeight = m_string->m_spacing + m_string->m_fontHeight;
  float offsetY = 0.0f;

  while (left >= static_cast<int>(m_visibleLines[line + 1]) && line < lastLine) {
    ++line;
    offsetY -= fontHeight;
  }

  int minPosition = left;
  if (minPosition < static_cast<int>(m_visibleLines[line])) {
    minPosition = m_visibleLines[line];
  }

  int maxPosition = right;
  if (maxPosition >= static_cast<int>(m_visibleLines[line + 1])) {
    maxPosition = m_visibleLines[line + 1];
  }

  float offsetX;
  if (minPosition == static_cast<int>(m_visibleLines[line])) {
    offsetX = 0.0f;
  } else {
    offsetX = m_string->GetTextWidth(
        text + m_visibleLines[line],
        minPosition - m_visibleLines[line]);
  }

  float width;
  if (maxPosition == static_cast<int>(m_visibleLines[line + 1])) {
    width = m_string->GetStringWidth() - offsetX;
  } else {
    width = m_string->GetTextWidth(text + minPosition, maxPosition - minPosition);
  }

  FRAMEPOINT point = m_multiline ? FRAMEPOINT_TOPLEFT : FRAMEPOINT_LEFT;
  area->SetPoint(point, m_string, point, offsetX, offsetY, 0);
  area->SetWidth(width);
  area->Resize(1);

  if (minPosition < maxPosition && fabs(width) >= 0.00000023841858f) {
    area->Show();
  } else {
    area->Hide();
  }
}

void CSimpleEditBox::SetKeyboardFocus(CSimpleEditBox *focus) {
  if (s_currentFocus) {
    s_currentFocus->m_cursor->Hide();
  }

  s_currentFocus = focus;
}

void CSimpleEditBox::ClearKeyboardFocus(CSimpleEditBox *focus) {
  if (s_currentFocus == focus) {
    s_currentFocus->m_cursor->Hide();
    s_currentFocus = 0;
  }
}
