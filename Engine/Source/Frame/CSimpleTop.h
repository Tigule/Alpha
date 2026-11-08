#ifndef ENGINE_SOURCE_FRAME_CSIMPLETOP_H
#define ENGINE_SOURCE_FRAME_CSIMPLETOP_H

#include "Base/Coordinate.h"
#include "Event/EvtApi.h"
#include "Frame/CLayoutFrame.h"
#include "Frame/CSimpleFrame.h"
#include "Frame/CSimpleRender.h"
#include "Model/Model.h"
#include "Scrn/Scrn.h"
#include "Tempest/c2vector.h"
#include "Tempest/crect.h"

#include <stpl.h>

class CFrameStrata;
class CMouseEvent;
class CSizeEvent;
class FRAMEPRIORITY;
class CGGameUI;
class CGTooltip;
class CGWorldFrame;

template <class T>
class CSimpleSortedArray {
 public:
  CSimpleSortedArray() : m_count(0), m_maxcount(0), m_iterator(0) {
  }

  ~CSimpleSortedArray() {
    m_array.Clear();
  }

  UINT Count() {
    return m_count;
  }

  void Insert(T value) {
    UINT index;
    UINT valuePriority = value->SimpleSortedArrayValue();

    if (m_count == m_maxcount) {
      m_maxcount += 8;
      m_array.SetCount(m_maxcount);
    }

    for (index = 0; index < m_count; ++index) {
      if (valuePriority > m_array[index]->SimpleSortedArrayValue()) {
        UINT move = m_count;

        while (move > index) {
          m_array[move] = m_array[move - 1];
          --move;
        }
        m_array[index] = value;
        break;
      }
    }

    if (index == m_count) {
      m_array[index] = value;
    }
    if (index < m_iterator) {
      ++m_iterator;
    }
    ++m_count;
  }

  void Remove(UINT index) {
    UINT move;

    ASSERT(index < m_count);
    for (move = index; move < m_count - 1; ++move) {
      m_array[move] = m_array[move + 1];
    }

    if (index < m_iterator) {
      --m_iterator;
    }

    --m_count;
    if (m_maxcount - m_count > 8) {
      m_maxcount -= 8;
      m_array.SetCount(m_maxcount);
    }
  }

  void IterateBegin() {
    m_iterator = 0;
  }

  T *IterateNext() {
    if (m_iterator >= m_count) {
      return 0;
    }

    return &m_array[m_iterator++];
  }

  T &operator[](UINT index) {
    ASSERT(index < m_count);
    return m_array[index];
  }

 protected:
  TSGrowableArray<T> m_array;
  UINT               m_count;
  UINT               m_maxcount;
  UINT               m_iterator;
};

class CSimpleTop : public CLayoutFrame {
  friend class CGGameUI;
  friend class CGTooltip;
  friend class CGWorldFrame;
  friend class CSimpleButton;
  friend class CSimpleFrame;

 public:
  static CSimpleTop *GetInstance() {
    ASSERT(s_instance);
    return s_instance;
  }

  CSimpleTop();
  virtual ~CSimpleTop();
  void EnumerateFrames(BOOL (*callback)(CSimpleFrame *, LPVOID), LPVOID param);
  void ValidateDeletedFrame(CSimpleFrame *frame);
  void RegisterFrame(CSimpleFrame *frame);
  void UnregisterFrame(CSimpleFrame *frame);
  void NotifyFrameMovedOrResized(CSimpleFrame *frame);
  void NotifyFrameLayerChanged(CSimpleFrame *frame, UINT layer);
  void RegisterForEvent(CSimpleFrame *frame, CSimpleEventType event, UINT priority);
  void UnregisterForEvent(CSimpleFrame *frame, CSimpleEventType event);
  void RegisterForDelete(CSimpleFrame *frame);

  void RegisterForMouseButton(int (*callback)(const CMouseEvent &)) {
    ASSERT(!m_mouseButtonCallback);
    m_mouseButtonCallback = callback;
  }

  void UnregisterForMouseButton(int (*callback)(const CMouseEvent &)) {
    ASSERT(m_mouseButtonCallback == callback);
    m_mouseButtonCallback = 0;
  }

  void RegisterForDisplaySize(int (*callback)(const CSizeEvent &)) {
    ASSERT(!m_displaySizeCallback);
    m_displaySizeCallback = callback;
  }

  void UnregisterForDisplaySize(int (*callback)(const CSizeEvent &)) {
    ASSERT(m_displaySizeCallback == callback);
    m_displaySizeCallback = 0;
  }

  void SetCursor(HMODEL cursor);

  void HideCursor() {
    m_cursorVisible = 0;
  }

  void ShowCursor() {
    m_cursorVisible = 1;
  }

  void GetMousePosition(NTempest::C2Vector &position) {
    NDCToDDC(m_mousePosition.x, m_mousePosition.y, &position.x, &position.y);
  }

  CSimpleFrame *GetLayerUnderCursor() {
    return m_mouseFocus;
  }

  BOOL RaiseFrame(const NTempest::C2Vector &pt);
  BOOL RaiseFrame(CSimpleFrame *frame, int checkOcclusion);
  BOOL LowerFrame(CSimpleFrame *frame);

  void SetLayoutMode(int enabled) {
    m_layout.enabled = enabled;
  }

  BOOL IsLayoutEnabled() {
    return m_layout.enabled;
  }

  BOOL IsMovingOrResizing() {
    return m_layout.frame != 0;
  }

  BOOL  StartMoveOrResizeFrame(CSimpleFrame *frame, const CMouseEvent &start, int resize);
  BOOL  StartMoveOrResizeFrame(const CMouseEvent &start, int resize);
  void MoveOrResizeFrame(const CMouseEvent &evt);
  void  StopMoveOrResizeFrame();

  DWORD GetLastEventTime() {
    return m_eventTime;
  }

  void UpdateEventTime(DWORD time) {
    m_eventTime = time;
  }

  void OnLayerUpdate(float elapsedSec);
  void OnLayerRender();
  void DrawCursor();

 private:
  static CSimpleTop *s_instance;
  HLAYER__     *m_screenLayer;
  HLAYER__     *m_cursorLayer;
  HMODEL        m_cursor;
  int           m_cursorVisible;
  CSimpleFrame *m_mouseFocus;
  CSimpleFrame *m_mouseCapture;
  CSimpleFrame *m_keydownCapture[780];
  LISTDECL(SIMPLEFRAMENODE, m_frames);
  LISTDECL(SIMPLEFRAMENODE, m_destroyed);
  CFrameStrata                       *m_strata[6];

  struct frame_layout {
    int                enabled;
    CSimpleFrame      *frame;
    FRAMEPOINT         anchor;
    NTempest::C2Vector last;
    NTempest::CRect    final;
  };

  frame_layout                        m_layout;
  CSimpleSortedArray<FRAMEPRIORITY *> m_eventqueue[NUM_SIMPLE_EVENTS][5];
  DWORD                               m_eventTime;
  BOOL                                m_checkFocus;
  EVENT_DATA_MOUSE                    m_mousePosition;
  int (*m_mouseButtonCallback)(const CMouseEvent &event);
  BOOL (*m_displaySizeCallback)(const CSizeEvent &event);
  void EnableEvents();
  void DisableEvents();
  static BOOL OnChar(const EVENT_DATA_CHAR *pCharEvtData, LPVOID param);
  static BOOL OnIme(const EVENT_DATA_IME *pImeData, LPVOID param);
  static BOOL OnKeyDown(const EVENT_DATA_KEY *pKeyData, LPVOID param);
  static BOOL OnKeyUp(const EVENT_DATA_KEY *pKeyData, LPVOID param);
  static BOOL OnKeyDownRepeat(const EVENT_DATA_KEY *pKeyData, LPVOID param);
  static BOOL OnMouseMove(const EVENT_DATA_MOUSE *pMouseData, LPVOID param);
  static BOOL OnMouseMoveRelative(const EVENT_DATA_MOUSE *pMouseData, LPVOID param);
  static BOOL OnMouseDown(const EVENT_DATA_MOUSE *pMouseData, LPVOID param);
  static BOOL OnMouseUp(const EVENT_DATA_MOUSE *pMouseData, LPVOID param);
  static BOOL OnMouseWheel(const EVENT_DATA_MOUSE *pMouseData, LPVOID param);
  static BOOL OnDisplaySizeChanged(const EVENT_DATA_SIZE *pSizeData, LPVOID param);
};

inline CLayoutFrame *CSimpleFrame::GetLayoutParent() {
  if ((CLayoutFrame *)m_parent) {
    return m_parent;
  }

  return m_top;
}

inline void CSimpleFrame::LockHighlight(int lock) {
  if (lock != m_highlightLocked) {
    m_highlightLocked = lock;

    if (lock) {
      EnableDrawLayer(4);
    } else if (m_top->m_mouseFocus != this) {
      DisableDrawLayer(4);
    }
  }
}

static char charBuf[8];

inline LPCSTR KeyToString(KEY key) {
  LPCSTR keyName;

  switch (key) {
    case KEY_SHIFT:
      keyName = "SHIFT";
      break;
    case KEY_CONTROL:
      keyName = "CTRL";
      break;
    case KEY_ALT:
      keyName = "ALT";
      break;
    case KEY_SPACE:
      keyName = "SPACE";
      break;
    case KEY_0:
    case KEY_1:
    case KEY_2:
    case KEY_3:
    case KEY_4:
    case KEY_5:
    case KEY_6:
    case KEY_7:
    case KEY_8:
    case KEY_9:
    case KEY_A:
    case KEY_B:
    case KEY_C:
    case KEY_D:
    case KEY_E:
    case KEY_F:
    case KEY_G:
    case KEY_H:
    case KEY_I:
    case KEY_J:
    case KEY_K:
    case KEY_L:
    case KEY_M:
    case KEY_N:
    case KEY_O:
    case KEY_P:
    case KEY_Q:
    case KEY_R:
    case KEY_S:
    case KEY_T:
    case KEY_U:
    case KEY_V:
    case KEY_W:
    case KEY_X:
    case KEY_Y:
    case KEY_Z:
      SStrPrintf(charBuf, sizeof(charBuf), "%c", key);
      keyName = charBuf;
      break;
    case KEY_TILDE:
      keyName = "TILDE";
      break;
    case KEY_NUMPAD0:
    case KEY_NUMPAD1:
    case KEY_NUMPAD2:
    case KEY_NUMPAD3:
    case KEY_NUMPAD4:
    case KEY_NUMPAD5:
    case KEY_NUMPAD6:
    case KEY_NUMPAD7:
    case KEY_NUMPAD8:
    case KEY_NUMPAD9:
      SStrPrintf(charBuf, sizeof(charBuf), "NUMPAD%d", key - KEY_NUMPAD0);
      keyName = charBuf;
      break;
    case KEY_NUMPAD_PLUS:
      keyName = "NUMPADPLUS";
      break;
    case KEY_NUMPAD_MINUS:
      keyName = "NUMPADMINUS";
      break;
    case KEY_NUMPAD_MULTIPLY:
      keyName = "NUMPADMULTIPLY";
      break;
    case KEY_NUMPAD_DIVIDE:
      keyName = "NUMPADDIVIDE";
      break;
    case KEY_PLUS:
      keyName = "PLUS";
      break;
    case KEY_MINUS:
      keyName = "MINUS";
      break;
    case KEY_BRACKET_OPEN:
      keyName = "LEFTBRACKET";
      break;
    case KEY_BRACKET_CLOSE:
      keyName = "RIGHTBRACKET";
      break;
    case KEY_SLASH:
      keyName = "SLASH";
      break;
    case KEY_BACKSLASH:
      keyName = "BACKSLASH";
      break;
    case KEY_SEMICOLON:
      keyName = "SEMICOLON";
      break;
    case KEY_APOSTROPHE:
      keyName = "APOSTROPHE";
      break;
    case KEY_COMMA:
      keyName = "COMMA";
      break;
    case KEY_PERIOD:
      keyName = "PERIOD";
      break;
    case KEY_ESCAPE:
      keyName = "ESCAPE";
      break;
    case KEY_ENTER:
      keyName = "ENTER";
      break;
    case KEY_BACKSPACE:
      keyName = "BACKSPACE";
      break;
    case KEY_TAB:
      keyName = "TAB";
      break;
    case KEY_LEFT:
      keyName = "LEFT";
      break;
    case KEY_UP:
      keyName = "UP";
      break;
    case KEY_RIGHT:
      keyName = "RIGHT";
      break;
    case KEY_DOWN:
      keyName = "DOWN";
      break;
    case KEY_INSERT:
      keyName = "INSERT";
      break;
    case KEY_DELETE:
      keyName = "DELETE";
      break;
    case KEY_HOME:
      keyName = "HOME";
      break;
    case KEY_END:
      keyName = "END";
      break;
    case KEY_PAGEUP:
      keyName = "PAGEUP";
      break;
    case KEY_PAGEDOWN:
      keyName = "PAGEDOWN";
      break;
    case KEY_NUMLOCK:
      keyName = "NUMLOCK";
      break;
    case KEY_CAPSLOCK:
      keyName = "CAPSLOCK";
      break;
    case KEY_SCROLLLOCK:
      keyName = "SCROLLLOCK";
      break;
    case KEY_PAUSE:
      keyName = "PAUSE";
      break;
    case KEY_PRINTSCREEN:
      keyName = "PRINTSCREEN";
      break;
    case KEY_F1:
    case KEY_F2:
    case KEY_F3:
    case KEY_F4:
    case KEY_F5:
    case KEY_F6:
    case KEY_F7:
    case KEY_F8:
    case KEY_F9:
    case KEY_F10:
    case KEY_F11:
    case KEY_F12:
      SStrPrintf(charBuf, sizeof(charBuf), "F%d", key - KEY_F1 + 1);
      keyName = charBuf;
      break;
    default:
      keyName = "UNKNOWN";
      break;
  }

  return keyName;
}

#endif
