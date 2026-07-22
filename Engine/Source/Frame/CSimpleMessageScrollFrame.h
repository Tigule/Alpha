#ifndef ENGINE_SOURCE_FRAME_CSIMPLEMESSAGESCROLLFRAME_H
#define ENGINE_SOURCE_FRAME_CSIMPLEMESSAGESCROLLFRAME_H

#include "Frame/CSimpleHyperlinkedFrame.h"
#include "Frame/CSimpleRender.h"
#include "Tempest/crect.h"

#include <stpl.h>

class CSimpleHyperlinkButton;

class CSimpleFontStringRecord : public CSimpleFontString, public TRefCnt {
 public:
  CSimpleFontStringRecord(CSimpleFrame *frame, unsigned int drawlayer, int show) : CSimpleFontString(frame, drawlayer, show) {
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
  CSimpleMessageScrollFrame(CSimpleFrame *parent, int maxLines);
  virtual ~CSimpleMessageScrollFrame();

  virtual void LoadXML(const XMLNode *node, CStatus *status);

  void                               SetMaxLines(int maxLines);
  void                               SetMessageFrameInsets(float right, float left, float top, float bottom);
  void                               SetTextLength(int size);
  const CSimpleFontStringAttributes *GetTextAttributes() const {
    return &m_attrib;
  }
  void         AddMessage(const char *text, const CSimpleFontStringAttributes *attrib);
  unsigned int AddMultiLine(char *text, const CSimpleFontStringAttributes *attrib);
  void         Clear();
  int          ScrollUp();
  int          ScrollDown();
  void         PageUp();
  void         PageDown();
  void         ScrollToTop();
  void         ScrollToBottom();
  int          AtBottom() const {
    return m_atBottom;
  }
  virtual void OnFrameSizeChanged(const NTempest::CRect &rect);
  virtual void OnLayerUpdate(float elapsedSec);

  static void __fastcall RegisterScriptMethods();
  static void __fastcall UnregisterScriptMethods();

 protected:
  virtual int LookupScriptMethod(lua_State *L, const char *name);

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
  TSExplicitList<CSimpleHyperlinkButton, 760>           m_hyperlinks;
};

#endif
