#include <Base/Base.h>

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
  UINT i;

  m_textSize = 32;
  m_text = static_cast<char *>(ALLOC(0x20));
  *m_text = 0;

  m_textInfo = static_cast<UINT *>(ALLOC(4 * m_textSize));
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

  LPCSTR value = node->GetAttributeByName("letters");
  if (value && *value) {
    m_textLettersMax = SStrToInt(value);
  }

  value = node->GetAttributeByName("blinkSpeed");
  if (value && *value) {
    m_cursorBlinkSpeed = max(SStrToFloat(value), 0.0f);
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
    SetHistoryLines(max(SStrToInt(value), 0));
  }

  value = node->GetAttributeByName("autoFocus");
  if (value && *value) {
    SetAutoFocus(StringToBOOL(value));
  }

  for (const XMLNode *child = node->GetChild(); child; child = child->GetSibling()) {
    if (!SStrCmpI(child->GetName(), "FontString", INT_MAX)) {
      m_string->LoadXML(child, status);
      if (m_string->m_textMaxSize) {
        m_textLengthMax = m_string->m_textMaxSize - 1;
      }

      NTempest::CImVector color;
      m_string->GetVertexColor(color);
      m_cursor->SetTexture(color);
    } else if (!SStrCmpI(child->GetName(), "HighlightColor", INT_MAX)) {
      NTempest::CImVector color;
      if (LoadXML_Color(child, color, status)) {
        for (UINT i = 0; i < 3; ++i) {
          m_highlight[i]->SetTexture(color);
        }
      }
    } else if (!SStrCmpI(child->GetName(), "TextInsets", INT_MAX)) {
      float l;
      float r;
      float t;
      float b;

      if (LoadXML_Insets(child, l, r, t, b, status)) {
        SetEditTextInsets(r, l, t, b);
      }
    }
  }
}

void CSimpleEditBox::LoadXML_Scripts(const XMLNode *node, CStatus *status) {
  CSimpleFrame::LoadXML_Scripts(node, status);

  for (const XMLNode *script = node->GetChild(); script; script = script->GetSibling()) {
    LPCSTR name = script->GetName();

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
  m_editTextInset.r = right;
  m_editTextInset.b = top;
  m_editTextInset.t = bottom;
  UpdateSizes(m_rect);
}

void CSimpleEditBox::OnLayerShow() {
  CSimpleFrame::OnLayerShow();

  if (!s_currentFocus && m_autoFocus) {
    SetKeyboardFocus(this);
  }

  OsIMEEnable(1);
}

void CSimpleEditBox::OnLayerHide() {
  CSimpleFrame::OnLayerHide();
  ClearKeyboardFocus(this);
  OsIMEEnable(0);
}

void CSimpleEditBox::OnLayerUpdate(float elapsedSec) {
  CSimpleFrame::OnLayerUpdate(elapsedSec);

  int textChanged = m_dirtyFlags & DIRTY_TEXT;

  if (m_dirtyFlags & DIRTY_CURSOR) {
    if (m_cursorPos < m_visiblePos || m_cursorPos > m_visiblePos + m_visibleLen || (m_imeInputMode && m_clauseRight > m_visiblePos + m_visibleLen)) {
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

    if (this == s_currentFocus && m_blinkElapsedTime > m_cursorBlinkSpeed) {
      if (m_cursor->IsVisible()) {
        m_cursor->Hide();
      } else {
        m_cursor->Show();
      }

      m_blinkElapsedTime = 0.0f;
    }
  }

  if (textChanged) {
    RunOnTextChangedScript();

    if (m_actions[EVENT_CHANGED].obj) {
      CEvent event;
      event.SetId(m_actions[EVENT_CHANGED].id);
      event.SetParam(this);
      m_actions[EVENT_CHANGED].obj->OnEvent(event);
    }
  }
}

BOOL CSimpleEditBox::OnLayerTrackUpdate(const CMouseEvent &evt) {
  if (CSimpleFrame::OnLayerTrackUpdate(evt)) {
    int position;
    if (m_highlightDrag && ConvertCoordinateToIndex(evt.x, evt.y, position)) {
      ExtendHighlight(position - m_cursorPos);
      SetCursorPosition(position);
    }

    return 1;
  }

  return 0;
}

void CSimpleEditBox::OnFrameSizeChanged(const NTempest::CRect &rect) {
  CSimpleFrame::OnFrameSizeChanged(rect);

  if (rect.t != m_rect.t || rect.l != m_rect.l || rect.b != m_rect.b || rect.r != m_rect.r) {
    UpdateSizes(rect);
  }
}

BOOL CSimpleEditBox::OnLayerChar(CCharEvent &evt) {
  if (!m_visible) {
    return 0;
  }

  if (!s_currentFocus && m_autoFocus) {
    SetKeyboardFocus(this);
  } else if (this != s_currentFocus) {
    return 0;
  }

  ASSERT(!m_imeInputMode);
  Insert(evt.ch);

  if (evt.ch == ' ') {
    RunOnSpacePressedScript();

    if (m_actions[EVENT_SPACE].obj) {
      CEvent event;
      event.SetId(m_actions[EVENT_SPACE].id);
      event.SetParam(this);
      m_actions[EVENT_SPACE].obj->OnEvent(event);
    }
  }

  return 1;
}

void CSimpleEditBox::CreateClauseHighlight() {
  if (m_clauseHighlight) {
    return;
  }

  m_clauseHighlight = NEW(CSimpleTexture)(this, 3, 1);
  m_clauseHighlight->SetTexture(NTempest::CImVector(0xFF202010));
  m_clauseHighlight->SetBlendMode(GxBlend_Add);
  m_clauseHighlight->SetHeight(m_string->GetFontHeight());
}

void CSimpleEditBox::CreateCandidatesFrame() {
  if (m_candidatesFrame) {
    return;
  }

  CreateClauseHighlight();

  float fontHeight = m_string->m_fontHeight;
  m_candidatesFrame = NEW(CSimpleMessageFrame)(this);

  LPCSTR fontName = m_string->m_font ? TextBlockGetFontName(m_string->m_font) : 0;
  UINT   fontFlags = m_string->m_font ? TextBlockGetFontFlags(m_string->m_font) : 0;
  m_candidatesFrame->m_attrib.m_font = fontName;
  m_candidatesFrame->m_attrib.m_fontHeight = fontHeight;
  m_candidatesFrame->m_attrib.m_fontFlags = fontFlags;
  m_candidatesFrame->m_attrib.m_flags |= CSimpleFontStringAttributes::FLAG_FONT_UPDATE;
  m_candidatesFrame->SetWidth(fontHeight * 10.0f);
  m_candidatesFrame->SetPoint(FRAMEPOINT_BOTTOMLEFT, m_clauseHighlight, FRAMEPOINT_TOPLEFT, 0.0f, 0.0f, 1);
  m_candidatesFrame->SetInsertMode(CSimpleMessageFrame::INSERT_AT_TOP);

  CSimpleTexture *background = NEW(CSimpleTexture)(m_candidatesFrame, 0, 1);
  background->SetTexture(NTempest::CImVector(0xFF606060));
  background->SetAllPoints(m_candidatesFrame, 1);

  m_candidatesHighlight = NEW(CSimpleTexture)(m_candidatesFrame, 2, 1);
  m_candidatesHighlight->SetTexture(NTempest::CImVector(0xFF808080));
}

void CSimpleEditBox::DispatchAction(int action) {
  if (m_actions[action].obj) {
    CEvent evt;
    evt.SetId(m_actions[action].id);
    evt.SetParam(this);
    m_actions[action].obj->OnEvent(evt);
  }
}

void CSimpleEditBox::UpdateLanguageIndicator() {
}

void CSimpleEditBox::UpdateClauseInfo() {
  UINT clauseLeft;
  UINT clauseRight;
  UINT cursorPos;

  if (OsIMEGetClauseInfo(clauseLeft, clauseRight, cursorPos)) {
    CreateClauseHighlight();
    m_clauseLeft = m_highlightLeft + GetNumToLen(m_highlightLeft, clauseLeft, false);
    m_clauseRight = m_highlightLeft + GetNumToLen(m_highlightLeft, clauseRight, false);
    m_cursorPos = m_highlightLeft + GetNumToLen(m_highlightLeft, cursorPos, false);
    m_dirtyFlags |= DIRTY_HIGHLIGHT | DIRTY_CURSOR;
  }
}

BOOL CSimpleEditBox::PopulateCandidates(DWORD which) {
  UINT                            pageSize;
  UINT                            count;
  UINT                            selection;
  TSGrowableArray<OsIMECandidate> candidates;

  if (!OsIMEGetCandidates(which, pageSize, count, selection, candidates)) {
    return 0;
  }

  CreateCandidatesFrame();

  float fontHeight = m_string->m_fontHeight;
  m_candidatesFrame->Clear();
  m_candidatesFrame->SetHeight((pageSize + 1) * fontHeight + fontHeight * 0.1f);

  m_candidatesHighlight->ClearAllPoints(1);
  UINT row = selection % pageSize;
  m_candidatesHighlight->SetPoint(FRAMEPOINT_TOPLEFT, m_candidatesFrame, FRAMEPOINT_TOPLEFT, 0.0f, -row * fontHeight, 1);
  m_candidatesHighlight->SetPoint(FRAMEPOINT_BOTTOMRIGHT, m_candidatesFrame, FRAMEPOINT_TOPRIGHT, 0.0f, -(row + 1) * fontHeight, 1);

  NTempest::CImVector white(0xFFFFFFFF);
  char                candidate[1024];
  SStrPrintf(candidate, sizeof(candidate), "> %d/%d", selection + 1, count);
  m_candidatesFrame->AddMessage(candidate, white, 0.0f, 0);

  for (UINT index = pageSize; index-- > 0;) {
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

BOOL CSimpleEditBox::OnLayerIme(CImeEvent &evt) {
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

BOOL CSimpleEditBox::OnLayerKeyDown(CKeyEvent &evt) {
  if (!m_visible) {
    return 0;
  }

  if (!s_currentFocus && m_autoFocus) {
    SetKeyboardFocus(this);
  } else if (this != s_currentFocus) {
    return 0;
  }

  switch (evt.key) {
    case KEY_ENTER:
      if (m_multiline) {
        Insert("\n", 0);
      } else {
        RunOnEnterPressedScript();
        DispatchAction(EVENT_ENTER);
      }
      break;

    case KEY_ESCAPE:
      RunOnEscapePressedScript();
      DispatchAction(EVENT_ESCAPE);
      break;

    case KEY_TAB:
      RunOnTabPressedScript();
      DispatchAction(EVENT_TAB);
      break;

    case KEY_HOME:
      MoveToStart(EventIsKeyDown(KEY_SHIFT));
      break;

    case KEY_END:
      MoveToEnd(EventIsKeyDown(KEY_SHIFT));
      break;

    case KEY_INSERT:
      if (EventIsKeyDown(KEY_CONTROL)) {
        if (IsHighlighted()) {
          CopyToClipboard();
        }
      } else if (EventIsKeyDown(KEY_SHIFT)) {
        PasteFromClipboard();
      }
      break;

    case KEY_DELETE:
      if (EventIsKeyDown(KEY_SHIFT)) {
        if (IsHighlighted()) {
          CopyToClipboard();
          DeleteHighlight();
        }
      } else if (EventIsKeyDown(KEY_CONTROL)) {
        DeleteForwardWord();
      } else {
        DeleteForward();
      }
      break;

    case KEY_BACKSPACE:
      if (EventIsKeyDown(KEY_CONTROL)) {
        DeleteBackwardWord();
      } else {
        DeleteBackward();
      }
      break;

    case KEY_RIGHT:
      if (EventIsKeyDown(KEY_CONTROL)) {
        MoveForwardWord(EventIsKeyDown(KEY_SHIFT));
      } else {
        MoveForward(EventIsKeyDown(KEY_SHIFT));
      }
      break;

    case KEY_LEFT:
      if (EventIsKeyDown(KEY_CONTROL)) {
        MoveBackwardWord(EventIsKeyDown(KEY_SHIFT));
      } else {
        MoveBackward(EventIsKeyDown(KEY_SHIFT));
      }
      break;

    case KEY_UP:
      if (m_multiline) {
        MoveBackwardLine(EventIsKeyDown(KEY_SHIFT));
      } else {
        BackwardHistory();
      }
      break;

    case KEY_DOWN:
      if (m_multiline) {
        MoveForwardLine(EventIsKeyDown(KEY_SHIFT));
      } else {
        ForwardHistory();
      }
      break;

    case KEY_A:
    case static_cast<KEY>(KEY_A + 32):
      if (EventIsKeyDown(KEY_CONTROL)) {
        HighlightText();
      }
      break;

    case KEY_F:
    case static_cast<KEY>(KEY_F + 32):
      if (EventIsKeyDown(KEY_CONTROL)) {
        MoveForward(EventIsKeyDown(KEY_SHIFT));
      }
      break;

    case KEY_B:
    case static_cast<KEY>(KEY_B + 32):
      if (EventIsKeyDown(KEY_CONTROL)) {
        MoveBackward(EventIsKeyDown(KEY_SHIFT));
      }
      break;

    case KEY_D:
    case static_cast<KEY>(KEY_D + 32):
      if (EventIsKeyDown(KEY_CONTROL)) {
        DeleteForward();
      }
      break;

    case KEY_W:
    case static_cast<KEY>(KEY_W + 32):
      if (EventIsKeyDown(KEY_CONTROL)) {
        DeleteBackwardWord();
      }
      break;

    case KEY_U:
    case static_cast<KEY>(KEY_U + 32):
      if (EventIsKeyDown(KEY_CONTROL)) {
        DeleteToStart();
      }
      break;

    case KEY_K:
    case static_cast<KEY>(KEY_K + 32):
      if (EventIsKeyDown(KEY_CONTROL)) {
        DeleteToEnd();
      }
      break;

    case KEY_P:
    case static_cast<KEY>(KEY_P + 32):
      if (EventIsKeyDown(KEY_CONTROL)) {
        BackwardHistory();
      }
      break;

    case KEY_N:
    case static_cast<KEY>(KEY_N + 32):
      if (EventIsKeyDown(KEY_CONTROL)) {
        ForwardHistory();
      }
      break;

    case KEY_C:
    case static_cast<KEY>(KEY_C + 32):
    case KEY_X:
    case static_cast<KEY>(KEY_X + 32):
      if (EventIsKeyDown(KEY_CONTROL) && IsHighlighted()) {
        CopyToClipboard();

        if (evt.key == KEY_X || evt.key == static_cast<KEY>(KEY_X + 32)) {
          DeleteHighlight();
        }
      }
      break;

    case KEY_V:
    case static_cast<KEY>(KEY_V + 32):
      if (EventIsKeyDown(KEY_CONTROL)) {
        PasteFromClipboard();
      }
      break;

    default:
      break;
  }

  return 1;
}

BOOL CSimpleEditBox::OnLayerKeyDownRepeat(CKeyEvent &evt) {
  if (!m_visible) {
    return 0;
  }

  if (!s_currentFocus && m_autoFocus) {
    SetKeyboardFocus(this);
  } else if (this != s_currentFocus) {
    return 0;
  }

  return OnLayerKeyDown(evt);
}

BOOL CSimpleEditBox::OnLayerKeyUp(CKeyEvent &evt) {
  if (!m_visible) {
    return 0;
  }

  if (!s_currentFocus && m_autoFocus) {
    SetKeyboardFocus(this);
  } else if (this != s_currentFocus) {
    return 0;
  }

  return 1;
}

BOOL CSimpleEditBox::OnLayerMouseDown(CMouseEvent &evt) {
  int handled = CSimpleFrame::OnLayerMouseDown(evt);

  if (!handled) {
    if (!m_imeInputMode) {
      int position;

      if (ConvertCoordinateToIndex(evt.x, evt.y, position)) {
        if (m_highlightLeft != m_highlightRight) {
          m_highlightRight = 0;
          m_highlightLeft = 0;
          m_dirtyFlags |= DIRTY_HIGHLIGHT;
        }

        SetCursorPosition(position);
        StartHighlight();
        m_highlightDrag = 1;
        handled = 1;
      }
    }

    SetKeyboardFocus(this);
  }

  return handled;
}

BOOL CSimpleEditBox::OnLayerMouseUp(CMouseEvent &evt) {
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

    float fontHeight = m_string->m_fontHeight;
    UINT  i;

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
  LPCSTR string = m_text;
  UINT   length = SStrLen(string);
  UINT   offset = 0;
  UINT   flags = 0;

  memset(m_textInfo, 0, sizeof(UINT) * m_textSize);

  while (length > 0) {
    UINT       advance;
    UINT       wide;
    QUOTEDCODE code = GxuDetermineQuotedCode(string, advance, 0, 0, wide, length);

    ASSERT(advance <= 0xFFFF);

    if (code == CODE_HYPERLINKSTART) {
      flags |= 0x80000000;
    }

    m_textInfo[offset] = (advance & 0xFFFF) | (static_cast<UINT>(code) << 16) | flags;

    offset += advance;
    string += advance;
    length -= advance;

    if (code == CODE_HYPERLINKSTOP) {
      flags &= 0x7FFFFFFF;
    }
  }
}

int CSimpleEditBox::GetNumToLen(int offset, int amount, bool checkHyperLink) {
  UINT *textInfo = m_textInfo;
  UINT *info = &textInfo[offset];
  int   length = 0;

  if (amount > 0) {
    while (amount-- > 0 && *info) {
      UINT value;
      UINT advance;
      UINT code;

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
      UINT value;
      UINT advance;
      UINT code;

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

BOOL CSimpleEditBox::GetLenToNum(int offset, int amount) {
  UINT *textInfo = m_textInfo;
  UINT *info = &textInfo[offset];
  int   result = 0;

  if (amount > 0) {
    while (amount-- > 0) {
      if (*info) {
        UINT code = (*info >> 16) & 0xFF;
        if (code == CODE_NEWLINE || code == CODE_PIPE || code == CODE_INVALIDCODE) {
          ++result;
        }
      }

      ++info;
    }
  } else {
    int remaining = -amount;
    while (remaining-- > 0 && info > textInfo) {
      --info;
      if (*info) {
        UINT code = (*info >> 16) & 0xFF;
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

BOOL CSimpleEditBox::GetOffsetToLine(int offset) {
  int line = 0;
  int maxLines = m_visibleLines.Count() - 2;

  while (offset >= m_visibleLines[line + 1] && line < maxLines) {
    ++line;
  }

  return line;
}

void CSimpleEditBox::GrowText(int size) {
  if (size + 1 > m_textSize) {
    m_textSize = (size + 32) & ~31;
    m_text = static_cast<char *>(SMemReAlloc(m_text, m_textSize, __FILE__, __LINE__, 0));
    m_textInfo = static_cast<UINT *>(SMemReAlloc(m_textInfo, sizeof(UINT) * m_textSize, __FILE__, __LINE__, 0));
  }
}

void CSimpleEditBox::SetText(LPCSTR text) {
  if (m_highlightLeft != m_highlightRight) {
    m_highlightRight = 0;
    m_highlightLeft = 0;
    m_dirtyFlags |= DIRTY_HIGHLIGHT;
  }

  SetCursorPosition(0);
  m_textLength = 0;
  Insert(text, 0);
  RunOnTextSetScript();

  if (m_actions[EVENT_SET].obj) {
    CEvent evt;
    evt.SetId(m_actions[EVENT_SET].id);
    evt.SetParam(this);
    m_actions[EVENT_SET].obj->OnEvent(evt);
  }
}

void CSimpleEditBox::Insert(LPCSTR utf8string, BOOL isIME) {
  if ((m_textInfo[m_cursorPos] & 0x80000000) && m_cursorPos > 0 && (m_textInfo[PrevCharOffset(m_cursorPos)] & 0x80000000)) {
    return;
  }

  if (m_highlightLeft != m_highlightRight) {
    DeleteHighlight();
  }

  if (!utf8string) {
    utf8string = "";
  }

  UINT length = SStrLen(utf8string);
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
    int pos = m_cursorPos;
    SetCursorPosition(m_textLength);

    while (m_textLength > m_textLengthMax) {
      Delete(-1);
    }

    SetCursorPosition(pos);
  }

  if (m_textLettersMax && GetLenToNum(0, m_textLength) > m_textLettersMax) {
    int pos = m_cursorPos;
    SetCursorPosition(m_textLength);

    while (GetLenToNum(0, m_textLength) > m_textLettersMax) {
      Delete(-1);
    }

    SetCursorPosition(pos);
  }
}

void CSimpleEditBox::Insert(UINT utf16) {
  char utf8string[5];

  if (utf16 < 0x20 || utf16 == 0x7F) {
    if (utf16 == '\t') {
      Insert("    ", 0);
      return;
    }

    if (!m_multiline || utf16 != '\n') {
      return;
    }
  }

  if (utf16 == '|') {
    Insert("||", 0);
  } else {
    sputu8(utf16, utf8string);
    Insert(utf8string, 0);
  }
}

void CSimpleEditBox::Delete(int amount) {
  VALIDATEBEGIN;
  VALIDATE(amount);
  VALIDATEENDVOID;

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
  while (m_cursorPos < m_textLength && iswspace(sgetu8(reinterpret_cast<const BYTE *>(&m_text[m_cursorPos]), &advance))) {
    Delete(1);
  }
  while (m_cursorPos < m_textLength && !iswspace(sgetu8(reinterpret_cast<const BYTE *>(&m_text[m_cursorPos]), &advance))) {
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
  while (m_cursorPos > 0 && iswspace(sgetu8(reinterpret_cast<const BYTE *>(&m_text[PrevCharOffset(m_cursorPos)]), &advance))) {
    Delete(-1);
  }
  while (m_cursorPos > 0 && !iswspace(sgetu8(reinterpret_cast<const BYTE *>(&m_text[PrevCharOffset(m_cursorPos)]), &advance))) {
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
  memcpy(&m_textInfo[start], &m_textInfo[start + removed], sizeof(UINT) * (m_textLength - start));

  m_text[m_textLength] = 0;
  m_dirtyFlags |= DIRTY_TEXT | DIRTY_CURSOR;
}

void CSimpleEditBox::Move(int distance, int highlight) {
  VALIDATEBEGIN;
  VALIDATE(distance);
  VALIDATEENDVOID;

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
    m_highlightRight = 0;
    m_highlightLeft = 0;
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
  while (m_cursorPos < m_textLength && iswspace(sgetu8(reinterpret_cast<const BYTE *>(&m_text[m_cursorPos]), &advance))) {
    Move(1, highlight);
  }
  while (m_cursorPos < m_textLength && !iswspace(sgetu8(reinterpret_cast<const BYTE *>(&m_text[m_cursorPos]), &advance))) {
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
  while (m_cursorPos > 0 && iswspace(sgetu8(reinterpret_cast<const BYTE *>(&m_text[m_cursorPos]), &advance))) {
    Move(-1, highlight);
  }
  while (m_cursorPos > 0 && !iswspace(sgetu8(reinterpret_cast<const BYTE *>(&m_text[m_cursorPos]), &advance))) {
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
  int lastLine = m_visibleLines.Count() - 2;
  int newLine = oldLine + distance;

  if (newLine > lastLine) {
    newLine = lastLine;
  } else if (newLine < 0) {
    newLine = 0;
  }

  if (newLine == oldLine) {
    return;
  }

  int column = GetLenToNum(m_visibleLines[oldLine], m_cursorPos - m_visibleLines[oldLine]);
  int newOffset = GetNumToLen(m_visibleLines[newLine], column, false);
  newOffset += m_visibleLines[newLine];

  if (m_visibleLines[newLine + 1] > m_visibleLines[newLine] && newOffset >= m_visibleLines[newLine + 1]) {
    newOffset = m_visibleLines[newLine + 1] - GetNumToLen(m_visibleLines[newLine + 1], -1, false);
  }

  int movement = newOffset - m_cursorPos;
  if (highlight) {
    if (m_highlightLeft == m_highlightRight) {
      StartHighlight();
    }
    ExtendHighlight(movement);
  } else if (m_highlightLeft != m_highlightRight) {
    m_highlightRight = 0;
    m_highlightLeft = 0;
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
  m_highlightRight = m_cursorPos;
  m_highlightLeft = m_cursorPos;
}

void CSimpleEditBox::ExtendHighlight(int distance) {
  if (!distance) {
    return;
  }

  int position = m_cursorPos + distance;
  if (distance < 0 && position >= m_highlightLeft) {
    m_highlightRight = position;
  } else if (distance >= 0 && position > m_highlightRight) {
    m_highlightRight = position;
  } else {
    m_highlightLeft = position;
  }
  m_dirtyFlags |= DIRTY_HIGHLIGHT;
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

void CSimpleEditBox::AddHistoryLine(LPCSTR line) {
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

BOOL CSimpleEditBox::ConvertCoordinateToIndex(float x, float y, int &position) {
  NTempest::CRect stringRect;
  if (!m_string->GetRect(&stringRect)) {
    return 0;
  }

  UINT  line = 0;
  float fontHeight = (m_string->GetSpacing() + m_string->GetFontHeight()) * m_layoutScale;
  UINT  lastLine = m_visibleLines.Count() - 2;

  if (m_multiline) {
    float distance = (stringRect.b - stringRect.t) - (y - stringRect.t);
    while (distance > fontHeight && line < lastLine) {
      distance -= fontHeight;
      ++line;
    }
  }

  x -= stringRect.l;
  if (x < 0.0f) {
    position = 0;
    return 1;
  }

  if (x > stringRect.r) {
    x = stringRect.r;
  }

  char *text = m_password ? m_textHidden : m_text;
  int   offset = m_string->GetNumCharsWithinWidth(text + m_visibleLines[line], m_visibleLines[line + 1] - m_visibleLines[line], x / m_string->GetLayoutScale());

  if (text == m_text) {
    offset = GetNumToLen(m_visibleLines[line], offset, false);
  }

  position = m_visibleLines[line] + offset;
  return 1;
}

void CSimpleEditBox::MakeTextVisible(int position, float offset, float stringWidth) {
  ASSERT(!m_password);

  if (m_multiline) {
    ASSERT(!"FIXME: Not yet implemented");
    return;
  }

  UINT amount = m_string->GetNumCharsWithinWidthFromEnd(m_text, position, offset);
  m_visiblePos = position - GetNumToLen(position, -static_cast<int>(amount), false);
  if (m_visiblePos < 0) {
    m_visiblePos = 0;
  }

  m_visibleLen = m_string->GetNumCharsWithinWidth(m_text + m_visiblePos, 0, stringWidth);
  m_visibleLen = GetNumToLen(m_visiblePos, m_visibleLen, false);
  m_visibleLines[0] = m_visiblePos;
  m_visibleLines[1] = m_visiblePos + m_visibleLen;
}

void CSimpleEditBox::UpdateVisibleText() {
  ASSERT(m_cursorPos >= 0 && m_cursorPos <= m_textLength);

  float stringWidth = m_string->GetWidth();
  ASSERT(stringWidth > 0.0f);

  char *text;
  if (m_password) {
    UINT length = SStrLen(m_text);
    m_textHidden = static_cast<char *>(SMemReAlloc(m_textHidden, length + 1, __FILE__, __LINE__, 0));
    memset(m_textHidden, '*', length);
    m_textHidden[length] = 0;
    text = m_textHidden;
  } else {
    text = m_text;
  }

  if (m_multiline) {
    UINT lines = m_string->WrapText(text + m_visiblePos, stringWidth, m_visibleLines.Ptr(), m_visibleLines.Count());

    if (lines >= m_visibleLines.Count()) {
      m_visibleLines.SetCount(lines + 1);
      m_string->WrapText(text + m_visiblePos, stringWidth, m_visibleLines.Ptr(), m_visibleLines.Count());
    }

    if (!lines) {
      lines = 1;
      m_visibleLines[0] = 0;
    }

    if (m_visiblePos > 0) {
      for (UINT i = 0; i < lines; ++i) {
        m_visibleLines[i] += m_visiblePos;
      }
    }

    m_visibleLines[lines] = SStrLen(text);
    m_visibleLen = m_visibleLines[lines];
    m_visibleLines.SetCount(lines + 1);
  } else {
    UINT amount = m_string->GetNumCharsWithinWidth(text + m_visiblePos, 0, stringWidth);
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

  int  end = m_visiblePos + m_visibleLen;
  char saved = text[end];
  text[end] = 0;
  m_string->SetText(text + m_visiblePos);
  text[end] = saved;

  if (m_multiline) {
    SetHeight(m_string->GetStringHeight() + m_editTextInset.b + m_editTextInset.t);
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
  int maxLine = GetOffsetToLine(m_highlightRight);
  int numLines = maxLine - firstLine + 1;

  if (numLines == 1) {
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

    UpdateHighlightArea(m_highlight[2], m_visibleLines[maxLine], m_highlightRight);
  }
}

void CSimpleEditBox::UpdateVisibleCursor() {
  int cursorPos = m_cursorPos;

  if (cursorPos >= m_visiblePos && cursorPos <= m_visiblePos + m_visibleLen) {
    float fontHeight = m_string->GetSpacing() + m_string->GetFontHeight();
    float offset_y = 0.0f;
    UINT  line = 0;
    UINT  maxLines = m_visibleLines.Count() - 2;

    while (cursorPos >= m_visibleLines[line + 1] && line < maxLines) {
      ++line;
      offset_y -= fontHeight;
    }

    float offset_x;
    if (cursorPos == m_visibleLines[line]) {
      offset_x = 0.0f;
    } else {
      char *text = m_password ? m_textHidden : m_text;
      offset_x = m_string->GetTextWidth(text + m_visibleLines[line], m_cursorPos - m_visibleLines[line]);
    }

    FRAMEPOINT point = m_multiline ? FRAMEPOINT_TOPLEFT : FRAMEPOINT_LEFT;
    m_cursor->SetPoint(point, m_string, point, offset_x, offset_y, 0);
    m_cursor->Resize(1);
    m_blinkElapsedTime = 0.0f;

    if (this == s_currentFocus) {
      m_cursor->Show();
    }
  } else {
    m_cursor->Hide();
  }
}

void CSimpleEditBox::UpdateHighlightArea(CSimpleRegion *area, int left, int right) {
  ASSERT(!m_multiline || !m_password);

  char *text = m_password ? m_textHidden : m_text;
  float fontHeight = m_string->GetSpacing() + m_string->GetFontHeight();
  float offset_y = 0.0f;
  UINT  line = 0;
  UINT  maxLines = m_visibleLines.Count() - 2;

  while (left >= m_visibleLines[line + 1] && line < maxLines) {
    ++line;
    offset_y -= fontHeight;
  }

  int minPos = max(m_visibleLines[line], left);
  int maxPos = min(right, m_visibleLines[line + 1]);

  float offset_x;
  if (minPos == m_visibleLines[line]) {
    offset_x = 0.0f;
  } else {
    offset_x = m_string->GetTextWidth(text + m_visibleLines[line], minPos - m_visibleLines[line]);
  }

  float width;
  if (maxPos == m_visibleLines[line + 1]) {
    width = m_string->GetStringWidth() - offset_x;
  } else {
    width = m_string->GetTextWidth(text + minPos, maxPos - minPos);
  }

  FRAMEPOINT point = m_multiline ? FRAMEPOINT_TOPLEFT : FRAMEPOINT_LEFT;
  area->SetPoint(point, m_string, point, offset_x, offset_y, 0);
  area->SetWidth(width);
  area->Resize(1);

  if (minPos >= maxPos || NTempest::CMath::fabs_(width) < 0.00000023841858f) {
    area->Hide();
  } else {
    area->Show();
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

  UINT  length = m_highlightRight - m_highlightLeft;
  char *buffer = static_cast<char *>(_alloca(length + 1));
  GxuFontStripEscapeCodes(m_text + m_highlightLeft, length, 0x500, buffer, length + 1);
  OsClipboardPutString(buffer);
}

void CSimpleEditBox::PasteFromClipboard() {
  char *string = OsClipboardGetString();
  if (!string) {
    return;
  }

  int advance;
  for (LPCSTR position = string; *position;) {
    UINT character = sgetu8(reinterpret_cast<const BYTE *>(position), &advance);
    Insert(character);
    position += advance;
  }

  OsClipboardFreeString(string);
}

void CSimpleEditBox::SetFont(LPCSTR font, float fontHeight, UINT fontFlags) {
  m_string->SetFont(font, fontHeight, fontFlags);

  if (m_candidatesFrame) {
    m_candidatesFrame->SetFont(font, fontHeight, fontFlags);
  }

  UpdateSizes(m_rect);
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
