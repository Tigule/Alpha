#ifndef ENGINE_SOURCE_FRAME_CSIMPLEMESSAGEFRAME_H
#define ENGINE_SOURCE_FRAME_CSIMPLEMESSAGEFRAME_H

#include "Base/RefCount.h"
#include "Frame/CSimpleFrame.h"
#include "Frame/CSimpleRender.h"
#include "Tempest/cimvector.h"
#include "Tempest/crect.h"

#include <stpl.h>

class CSimpleMessageFrameLineNode : public TRefCnt {
 public:
  CSimpleMessageFrameLineNode();
  virtual ~CSimpleMessageFrameLineNode();

  NTempest::CImVector color;
  CSimpleFontString  *string;
  float               timeLeft;
  float               fadeLeft;
  int                 permanent : 1;
  int                 isVisible : 1;
};

struct MessageData {
  char               *text;
  NTempest::CImVector color;
  float               timeVisible;
  int                 permanent;
};

class CSimpleMessageFrameLine {
 public:
  CSimpleMessageFrameLine() : offsetX(0.0f), offsetY(0.0f), stringNode(NEW(CSimpleMessageFrameLineNode)) {
    stringNode->IncrRef();
  }
  ~CSimpleMessageFrameLine() {
    stringNode->DecrRef();
  }

  float                        offsetX;
  float                        offsetY;
  CSimpleMessageFrameLineNode *stringNode;
};

class CSimpleMessageFrame : public CSimpleFrame {
  friend class CSimpleEditBox;

 public:
  enum SimpleMessageFrameInsertMode {
    INSERT_AT_TOP = 0,
    INSERT_AT_BOTTOM = 1
  };

  CSimpleMessageFrame(CSimpleFrame *parent);
  virtual ~CSimpleMessageFrame();

  static void __fastcall RegisterScriptMethods();
  static void __fastcall UnregisterScriptMethods();

  virtual void LoadXML(const XMLNode *node, CStatus *status);
  virtual void AddMessage(const char *text, const NTempest::CImVector &color, float timeVisible, int permanent);
  virtual void OnFrameSizeChanged(const NTempest::CRect &rect);
  virtual void OnLayerUpdate(float elapsedSec);

  void SetMessageFrameInsets(float right, float left, float top, float bottom);
  void SetTextLength(int size);
  void SetInsertMode(SimpleMessageFrameInsertMode mode);
  void SetFadeDuration(float duration) {
    m_fadeDuration = duration;
  }
  void Clear();
  void ClearPending();

 protected:
  virtual int LookupScriptMethod(lua_State *L, const char *name);

  void AddPendingMessage(const char *text, const NTempest::CImVector &color, float timeVisible, int permanent);
  void ScrollMessages(unsigned int start);
  void HideLineNode(CSimpleMessageFrameLineNode *node);
  void ShowLineNode(CSimpleMessageFrameLineNode *node, float timeVisible, float fadeDuration, int permanent);

  static TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> s_scriptMethods;

  unsigned int                             m_rows;
  unsigned int                             m_numVisible;
  NTempest::CRect                          m_messageFrameArea;
  NTempest::CRect                          m_messageFrameInset;
  int                                      m_textMaxSize;
  CSimpleFontStringAttributes              m_attrib;
  float                                    m_fadeDuration;
  SimpleMessageFrameInsertMode             m_insertMode;
  TSGrowableArray<MessageData>             m_pendingMessages;
  TSGrowableArray<CSimpleMessageFrameLine> m_lines;
};

#endif
