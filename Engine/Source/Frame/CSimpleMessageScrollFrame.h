#ifndef ENGINE_SOURCE_FRAME_CSIMPLEMESSAGESCROLLFRAME_H
#define ENGINE_SOURCE_FRAME_CSIMPLEMESSAGESCROLLFRAME_H

#include "Frame/CSimpleHyperlinkedFrame.h"
#include "Frame/CSimpleRender.h"
#include "Tempest/crect.h"

#include <stpl.h>

class CSimpleHyperlinkButton;

class CSimpleFontStringRecord : public CSimpleFontString, public TRefCnt {
 public:
  CSimpleFontStringRecord(CSimpleFrame *frame, UINT drawlayer, int show) : CSimpleFontString(frame, drawlayer, show) {
  }
};

class CSimpleMessageScrollFrameLine {
 public:
  CSimpleMessageScrollFrameLine() : string(0), isVisible(0), timeLeft(0.0f), fadeLeft(0.0f) {
  }

  ~CSimpleMessageScrollFrameLine() {
    FREEIFUSED(string);
  }

  char                       *string;
  CSimpleFontStringAttributes attrib;
  int                         isVisible;
  float                       timeLeft;
  float                       fadeLeft;
};

class CSimpleMessageScrollFrameDisplayNode : public TRefCnt {
 public:
  CSimpleMessageScrollFrameDisplayNode();
  CSimpleMessageScrollFrameDisplayNode(const CSimpleMessageScrollFrameDisplayNode &rhs);
  virtual ~CSimpleMessageScrollFrameDisplayNode();

  CSimpleFontStringRecord       *string;
  CSimpleMessageScrollFrameLine *line;
  CSimpleFontStringAttributes    attrib;
};

class CSimpleMessageScrollFrame : public CSimpleHyperlinkedFrame {
 public:
  CSimpleMessageScrollFrame(CSimpleFrame *parent = 0, int maxLines = 8);
  virtual ~CSimpleMessageScrollFrame();

  virtual void LoadXML(const XMLNode *node, CStatus *status);

  void SetMaxLines(int maxLines);
  void SetMessageFrameInsets(float right, float left, float top, float bottom);
  void SetTextLength(int size);
  void SetFont(LPCSTR font, float fontHeight, int fontFlags) {
    m_attrib.SetFont(font, fontHeight, fontFlags);
  }
  void SetHorizontalAlignment(UINT alignment) {
    m_attrib.SetHorizontalAlignment(alignment);
  }
  void SetColor(const NTempest::CImVector &color) {
    m_attrib.SetColor(color);
  }
  void AddShadow(const NTempest::CImVector &color, const NTempest::C2Vector &offset) {
    m_attrib.AddShadow(color, offset);
  }
  void SetSpacing(float spacing) {
    m_attrib.SetSpacing(spacing);
  }
  const CSimpleFontStringAttributes *GetTextAttributes() const {
    return &m_attrib;
  }
  void SetFade(int fading) {
    m_fading = fading;
  }
  void SetTimeVisible(float timeVisible) {
    m_timeVisible = timeVisible;
  }
  void SetFadeDuration(float fadeDuration) {
    m_fadeDuration = fadeDuration;
  }
  void AddMessage(LPCSTR text, const CSimpleFontStringAttributes *attrib);
  UINT AddMultiLine(char *text, const CSimpleFontStringAttributes *attrib);
  void Clear();
  int  ScrollUp();
  int  ScrollDown();
  int  CanScroll() {
    return m_numMessages > m_numDisplayed;
  }
  void PageUp();
  void PageDown();
  void ScrollToTop();
  void ScrollToBottom();
  int  GetNumDisplayLines() {
    return m_numDisplayed;
  }
  int AtBottom() {
    return m_atBottom;
  }
  virtual void OnFrameSizeChanged(const NTempest::CRect &rect);
  virtual void OnLayerUpdate(float elapsedSec);

  static void RegisterScriptMethods();
  static void UnregisterScriptMethods();

 protected:
  virtual int LookupScriptMethod(lua_State *L, LPCSTR name);

  void ScrollMessages(int start);
  void UpdateNode(CSimpleMessageScrollFrameDisplayNode *node, CSimpleMessageScrollFrameLine *line, int resetTimers);
  void RefreshMessages();
  void RefreshHyperlinks();

  static TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> s_scriptMethods;

  int                                                   m_numMessages;
  int                                                   m_maxMessages;
  int                                                   m_currentLine;
  int                                                   m_currentScroll;
  int                                                   m_numDisplayed;
  int                                                   m_atTop;
  int                                                   m_atBottom;
  int                                                   m_textMaxSize;
  CSimpleFontStringAttributes                           m_attrib;
  int                                                   m_fading;
  float                                                 m_fadeDuration;
  float                                                 m_timeVisible;
  NTempest::CRect                                       m_messageFrameArea;
  NTempest::CRect                                       m_messageFrameInset;
  TSGrowableArray<CSimpleMessageScrollFrameLine>        m_lines;
  TSGrowableArray<CSimpleMessageScrollFrameDisplayNode> m_displayNodes;
  LISTDECLEX(CSimpleHyperlinkButton, m_link, m_hyperlinks);
};

#endif
