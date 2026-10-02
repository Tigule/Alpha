#ifndef ENGINE_SOURCE_FRAME_CSIMPLETOP_H
#define ENGINE_SOURCE_FRAME_CSIMPLETOP_H

#include "Base/Coordinate.h"
#include "Event/EvtApi.h"
#include "Frame/CLayoutFrame.h"
#include "Frame/CSimpleFrame.h"
#include "Frame/CSimpleRender.h"
#include "Frame/CSimpleSortedArray.h"
#include "Model/IModel.h"
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
  CSimpleSortedArray<FRAMEPRIORITY *> m_eventqueue[4][5];
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
  if (m_parent) {
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

#endif
