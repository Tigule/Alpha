#ifndef ENGINE_SOURCE_FRAME_CSIMPLEEDITBOX_H
#define ENGINE_SOURCE_FRAME_CSIMPLEEDITBOX_H

#include "Frame/CSimpleFrame.h"
#include "Frame/CSimpleRender.h"
#include "Tempest/crect.h"

#include <stpl.h>

class CObserver;
class CSimpleMessageFrame;

class CSimpleEditBox : public CSimpleFrame {
 public:
  CSimpleEditBox(CSimpleFrame *parent = 0);
  virtual ~CSimpleEditBox();

  static void RegisterScriptMethods();
  static void UnregisterScriptMethods();
  static void SetKeyboardFocus(CSimpleEditBox *focus);
  static void ClearKeyboardFocus(CSimpleEditBox *focus);

  virtual void LoadXML(const XMLNode *node, CStatus *status);
  virtual void LoadXML_Scripts(const XMLNode *node, CStatus *status);
  virtual void OnLayerShow();
  virtual void OnLayerHide();
  virtual void OnLayerUpdate(float elapsedSec);
  virtual int  OnLayerTrackUpdate(const CMouseEvent &evt);
  virtual void OnFrameSizeChanged(const NTempest::CRect &rect);
  virtual int  OnLayerChar(CCharEvent &evt);
  virtual int  OnLayerIme(CImeEvent &evt);
  virtual int  OnLayerKeyDown(CKeyEvent &evt);
  virtual int  OnLayerKeyDownRepeat(CKeyEvent &evt);
  virtual int  OnLayerKeyUp(CKeyEvent &evt);
  virtual int  OnLayerMouseDown(CMouseEvent &evt);
  virtual int  OnLayerMouseUp(CMouseEvent &evt);

  void SetMultiLine(int enabled);
  void SetAutoFocus(int enabled);
  void SetEditTextInsets(float right, float left, float top, float bottom);
  void SetPassword(int enabled) {
    m_password = enabled;
    m_dirtyFlags |= DIRTY_TEXT | DIRTY_HIGHLIGHT | DIRTY_CURSOR;
  }
  void SetTextSizeLimit(int size) {
    m_textLengthMax = size;
  }
  void SetTextLetterLimit(int letters) {
    m_textLettersMax = letters;
  }
  void   SetText(LPCSTR text);
  LPCSTR GetText() {
    return m_text;
  }
  void Insert(LPCSTR utf8string, int isIME);
  void Insert(UINT utf16);
  void SetHistoryLines(int numLines);
  void AddHistoryLine(LPCSTR line);
  void HighlightText();
  void SetFont(LPCSTR fontName, float fontHeight, UINT fontFlags);

  void SetTextColor(const NTempest::CImVector &color) {
    m_string->SetVertexColor(color);
  }

  void SetCursorColor(const NTempest::CImVector &color) {
    m_cursor->SetTexture(color);
  }

  void SetHighlightColor(const NTempest::CImVector &color) {
    for (UINT i = 0; i < 3; ++i) {
      m_highlight[i]->SetTexture(color);
    }
  }

  void AddShadow(const NTempest::CImVector &color, const NTempest::C2Vector &offset) {
    m_string->AddShadow(color, offset);
  }

  void SetCursorPosition(int position) {
    if (position < 0) {
      position = 0;
    } else if (position > m_textLength) {
      position = m_textLength;
    }
    m_cursorPos = position;
    m_dirtyFlags |= DIRTY_CURSOR;
  }

  void SetCursorBlinkSpeed(float speed) {
    m_cursorBlinkSpeed = speed;
  }

  void HideCursor() {
    m_cursor->Hide();
  }

  void RegisterEnter(UINT id, CObserver *observer) {
    RegisterAction(EVENT_ENTER, id, observer);
  }

  void RegisterEscape(UINT id, CObserver *observer) {
    RegisterAction(EVENT_ESCAPE, id, observer);
  }

  void RegisterSpace(UINT id, CObserver *observer) {
    RegisterAction(EVENT_SPACE, id, observer);
  }

  void RegisterTab(UINT id, CObserver *observer) {
    RegisterAction(EVENT_TAB, id, observer);
  }

  void RegisterTextChanged(UINT id, CObserver *observer) {
    RegisterAction(EVENT_CHANGED, id, observer);
  }

  void RegisterTextSet(UINT id, CObserver *observer) {
    RegisterAction(EVENT_SET, id, observer);
  }

  void SetOnEnterPressedScript(LPCSTR source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnEnterPressed", GetName());
    SetEventScript(m_onEnterPressed, source, description);
  }

  void RunOnEnterPressedScript() {
    if (m_onEnterPressed) {
      FrameScript_Execute(m_onEnterPressed, this);
    }
  }

  void SetOnEscapePressedScript(LPCSTR source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnEscapePressed", GetName());
    SetEventScript(m_onEscapePressed, source, description);
  }

  void RunOnEscapePressedScript() {
    if (m_onEscapePressed) {
      FrameScript_Execute(m_onEscapePressed, this);
    }
  }

  void SetOnSpacePressedScript(LPCSTR source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnSpacePressed", GetName());
    SetEventScript(m_onSpacePressed, source, description);
  }

  void RunOnSpacePressedScript() {
    if (m_onSpacePressed) {
      FrameScript_Execute(m_onSpacePressed, this);
    }
  }

  void SetOnTabPressedScript(LPCSTR source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnTabPressed", GetName());
    SetEventScript(m_onTabPressed, source, description);
  }

  void RunOnTabPressedScript() {
    if (m_onTabPressed) {
      FrameScript_Execute(m_onTabPressed, this);
    }
  }

  void SetOnTextChangedScript(LPCSTR source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnTextChanged", GetName());
    SetEventScript(m_onTextChanged, source, description);
  }

  void RunOnTextChangedScript() {
    if (m_onTextChanged) {
      FrameScript_Execute(m_onTextChanged, this);
    }
  }

  void SetOnTextSetScript(LPCSTR source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnTextSet", GetName());
    SetEventScript(m_onTextSet, source, description);
  }

  void RunOnTextSetScript() {
    if (m_onTextSet) {
      FrameScript_Execute(m_onTextSet, this);
    }
  }

 protected:
  virtual int LookupScriptMethod(lua_State *L, LPCSTR name);

  void UpdateSizes(const NTempest::CRect &rect);
  void UpdateTextInfo();
  void UpdateVisibleCursor();
  int  GetNumToLen(int offset, int amount, bool checkHyperLink);
  int  GetLenToNum(int offset, int amount);
  int  NextCharOffset(int offset);
  int  PrevCharOffset(int offset);
  int  GetOffsetToLine(int offset);
  void GrowText(int size);
  void Delete(int amount);
  void DeleteForward();
  void DeleteForwardWord();
  void DeleteBackward();
  void DeleteBackwardWord();
  void DeleteToStart();
  void DeleteToEnd();
  void DeleteText();
  void DeleteSubstring(int left, int right);
  void Move(int distance, int highlight);
  void MoveForward(int highlight);
  void MoveForwardWord(int highlight);
  void MoveBackward(int highlight);
  void MoveBackwardWord(int highlight);
  void MoveToStart(int highlight);
  void MoveToEnd(int highlight);
  void MoveLine(int distance, int highlight);
  void MoveForwardLine(int highlight);
  void MoveBackwardLine(int highlight);
  int  IsHighlighted() {
    return m_highlightLeft != m_highlightRight;
  }
  void StartHighlight();
  void ExtendHighlight(int distance);
  void ClearHighlight() {
    if (m_highlightLeft != m_highlightRight) {
      m_highlightLeft = 0;
      m_highlightRight = 0;
      m_dirtyFlags |= DIRTY_HIGHLIGHT;
    }
  }
  void DeleteHighlight();
  void ForwardHistory();
  void BackwardHistory();
  int  ConvertCoordinateToIndex(float x, float y, int &index);
  void MakeTextVisible(int position, float extentLeft, float extentRight);
  void UpdateVisibleText();
  void UpdateVisibleHighlight();
  void UpdateHighlightArea(CSimpleRegion *region, int left, int right);
  void CopyToClipboard();
  void PasteFromClipboard();
  void ShowCandidates();
  void HideCandidates();
  void CreateClauseHighlight();
  void CreateCandidatesFrame();
  void UpdateLanguageIndicator();
  void UpdateClauseInfo();
  int  PopulateCandidates(DWORD selection);
  void DispatchAction(int action);
  void RegisterAction(int action, UINT id, CObserver *observer) {
    m_actions[action].id = id;
    m_actions[action].obj = observer;
  }

  static TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> s_scriptMethods;
  static CSimpleEditBox                                      *s_currentFocus;

  enum {
    DIRTY_NONE = 0,
    DIRTY_TEXT = 1,
    DIRTY_HIGHLIGHT = 2,
    DIRTY_CURSOR = 4
  };

  enum {
    EVENT_ENTER = 0,
    EVENT_ESCAPE = 1,
    EVENT_SPACE = 2,
    EVENT_TAB = 3,
    EVENT_CHANGED = 4,
    EVENT_SET = 5,
    NUM_EDITBOX_ACTIONS = 6
  };

  UINT                  m_dirtyFlags;
  CSimpleFontString    *m_string;
  char                 *m_text;
  UINT                 *m_textInfo;
  char                 *m_textHidden;
  int                   m_textLength;
  int                   m_textLengthMax;
  int                   m_textLettersMax;
  int                   m_textSize;
  int                   m_visiblePos;
  int                   m_visibleLen;
  CSimpleTexture       *m_highlight[3];
  int                   m_highlightLeft;
  int                   m_highlightRight;
  int                   m_highlightDrag;
  CSimpleTexture       *m_cursor;
  int                   m_cursorPos;
  float                 m_cursorBlinkSpeed;
  float                 m_blinkElapsedTime;
  int                   m_password;
  int                   m_multiline;
  TSGrowableArray<UINT> m_visibleLines;
  int                   m_autoFocus;
  int                   m_numHistory;
  int                   m_curHistory;
  TSFixedArray<char *>  m_history;
  struct {
    UINT       id;
    CObserver *obj;
  } m_actions[NUM_EDITBOX_ACTIONS];
  int                  m_imeInputMode;
  CSimpleTexture      *m_clauseHighlight;
  int                  m_clauseLeft;
  int                  m_clauseRight;
  CSimpleMessageFrame *m_candidatesFrame;
  CSimpleTexture      *m_candidatesHighlight;
  NTempest::CRect      m_editTextInset;
  int                  m_onEnterPressed;
  int                  m_onEscapePressed;
  int                  m_onSpacePressed;
  int                  m_onTabPressed;
  int                  m_onTextChanged;
  int                  m_onTextSet;
};

#endif
