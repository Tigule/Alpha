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

class CMouseEvent;
class CSizeEvent;

class FRAMEPRIORITY {
 public:
  CSimpleFrame *frame;
  unsigned int  priority;

  unsigned int SimpleSortedArrayValue() {
    return priority;
  }
};

class CFrameStrataNode {
 public:
  CFrameStrataNode() : batchDirty(0) {
  }

  ~CFrameStrataNode() {
    CSimpleFrame *frame;

    while ((frame = frames.Head()) != 0) {
      DEL(frame);
    }

    renderList.UnlinkAll();
  }

  int BuildBatches();

  int IsEmpty() {
    return frames.IsEmpty();
  }

  int AddFrame(CSimpleFrame *frame) {
    ASSERT(!frames.IsLinked(frame));
    frames.LinkNode(frame, LIST_TAIL, 0);

    if (!frame->IsBeingScrolled()) {
      batchDirty = -1;
    }

    if (frame->IsVisible()) {
      frame->RegisterForEvents();
    }

    return batchDirty != 0;
  }

  int DelFrame(CSimpleFrame *frame) {
    ASSERT(frames.IsLinked(frame));
    frames.UnlinkNode(frame);

    if (!frame->IsBeingScrolled()) {
      batchDirty = -1;
    }

    if (frame->IsVisible()) {
      frame->UnregisterForEvents();
    }

    return batchDirty != 0;
  }

  void OnLayerUpdate(float elapsedSec) {
    CSimpleFrame *frame;

    for (frame = frames.Head(); frame; frame = frames.Next(frame)) {
      if (frame->IsVisible()) {
        frame->OnLayerUpdate(elapsedSec);
      }
    }
  }

  void RenderBatches() {
    CRenderBatch *batch;

    for (batch = renderList.Head(); batch; batch = renderList.Next(batch)) {
      CSimpleRender::DrawBatch(batch);
    }
  }

  TSExplicitList<CSimpleFrame, 668> frames;
  CRenderBatch                      batches[5];
  unsigned int                      batchDirty;
  TSExplicitList<CRenderBatch, 44>  renderList;
};

class CFrameStrata {
 public:
  CFrameStrata() : batchDirty(0), levelsDirty(0), topLevel(0) {
  }

  ~CFrameStrata();

  int EnumerateFrames(int(*callback)(CSimpleFrame *, void *), void *param) {
    unsigned int i;

    for (i = 0; i < topLevel; ++i) {
      CSimpleFrame *frame;

      for (frame = levels[i]->frames.Head(); frame; frame = levels[i]->frames.Next(frame)) {
        if (!callback(frame, param)) {
          return 0;
        }
      }
    }

    return 1;
  }

  int  FrameOccluded(CSimpleFrame *thisFrame);
  void CheckOcclusion();

  void AddFrame(CSimpleFrame *frame) {
    int          frameLevel = frame->GetFrameLevel();
    unsigned int oldCount = levels.Count();

    if (frameLevel >= oldCount) {
      unsigned int index;

      levels.SetCount(frameLevel + 1);
      for (index = oldCount; index <= frameLevel; ++index) {
        levels[index] = NEW(CFrameStrataNode);
      }
    }

    if (frameLevel >= topLevel) {
      topLevel = frameLevel + 1;
    }

    CFrameStrataNode *node = levels[frameLevel];
    int               dirty = node->AddFrame(frame);
    levelsDirty = 1;
    batchDirty |= dirty;
  }

  void DelFrame(CSimpleFrame *frame) {
    CFrameStrataNode *node = levels[frame->GetFrameLevel()];
    int               dirty = node->DelFrame(frame);

    levelsDirty = 1;
    batchDirty |= dirty;
  }

  void OnFrameLayerChanged(CSimpleFrame *frame, unsigned int layer) {
    if (frame->IsBeingScrolled()) {
      frame->OnUpdateBatch(layer);
    } else {
      CFrameStrataNode *node = levels[frame->GetFrameLevel()];

      node->batchDirty |= 1 << layer;
      batchDirty = 1;
    }
  }

  void OnFrameMovedOrResized(CSimpleFrame *frame) {
    levelsDirty = 1;
  }

  void RaiseFrame(CSimpleFrame *frame) {
    if (frame->IsOccluded()) {
      frame->SetFrameLevel(topLevel, 1);
    }
  }

  int BuildBatches() {
    unsigned int level;

    batchDirty = 0;
    for (level = 0; level < topLevel; ++level) {
      if (levels[level]->BuildBatches()) {
        batchDirty = 1;
      }
    }

    return batchDirty;
  }

  void OnLayerWindowSizeChanged();

  CSimpleFrame *GetToplevelFrame(const NTempest::C2Vector &point) {
    unsigned int level = topLevel;

    while (level) {
      CSimpleFrame *frame;

      --level;

      for (frame = levels[level]->frames.Head(); frame; frame = levels[level]->frames.Next(frame)) {
        if (frame->IsVisible() && frame->TestHitRect(point)) {
          return frame->GetToplevelFrame();
        }
      }
    }

    return 0;
  }

  void CompressLevels() {
    unsigned int firstEmpty = static_cast<unsigned int>(-1);
    unsigned int level = 0;
    unsigned int nextLevel = 0;

    while (level < topLevel) {
      if (levels[level]->IsEmpty()) {
        if (firstEmpty == static_cast<unsigned int>(-1)) {
          firstEmpty = level;
        }

        nextLevel = ++level;
        continue;
      }

      if (firstEmpty == static_cast<unsigned int>(-1)) {
        nextLevel = ++level;
        continue;
      }

      unsigned int delta = level - firstEmpty;
      while (level < topLevel) {
        CSimpleFrame *frame = levels[level]->frames.Head();

        while (frame) {
          CSimpleFrame *next = levels[level]->frames.Next(frame);

          frame->SetFrameLevel(frame->GetFrameLevel() - delta, 0);
          frame = next;
        }

        ++level;
      }

      topLevel -= delta;
      firstEmpty = static_cast<unsigned int>(-1);
      level = nextLevel;
    }

    if (firstEmpty != static_cast<unsigned int>(-1)) {
      topLevel = firstEmpty;
    }
  }

  void OnLayerUpdate(float elapsedSec) {
    unsigned int level;

    for (level = 0; level < topLevel; ++level) {
      levels[level]->OnLayerUpdate(elapsedSec);
    }
  }

  void RenderBatches() {
    unsigned int level;

    for (level = 0; level < topLevel; ++level) {
      levels[level]->RenderBatches();
    }
  }

  int                              batchDirty;
  int                              levelsDirty;
  unsigned int                     topLevel;
  TSFixedArray<CFrameStrataNode *> levels;
};

class CSimpleTop : public CLayoutFrame {
  friend class CSimpleFrame;

 public:
  struct frame_layout {
    int                enabled;
    CSimpleFrame      *frame;
    FRAMEPOINT         anchor;
    NTempest::C2Vector last;
    NTempest::CRect    final;
  };

  CSimpleTop();
  virtual ~CSimpleTop();

  static CSimpleTop *GetInstance() {
    ASSERT(s_instance);
    return s_instance;
  }

  void DrawCursor();
  void EnumerateFrames(int(*callback)(CSimpleFrame *, void *), void *param);
  void MoveOrResizeFrame(const CMouseEvent &evt);
  void NotifyFrameLayerChanged(CSimpleFrame *frame, unsigned int layer);
  void NotifyFrameMovedOrResized(CSimpleFrame *frame);
  void OnLayerRender();
  void OnLayerUpdate(float elapsedSec);
  int  LowerFrame(CSimpleFrame *frame);
  int  RaiseFrame(const NTempest::C2Vector &pt);
  int  RaiseFrame(CSimpleFrame *frame, int checkOcclusion);
  void RegisterForDelete(CSimpleFrame *frame);
  void RegisterForMouseButton(int(*callback)(const CMouseEvent &)) {
    ASSERT(!m_mouseButtonCallback);
    m_mouseButtonCallback = callback;
  }
  void UnregisterForMouseButton(int(*callback)(const CMouseEvent &)) {
    ASSERT(m_mouseButtonCallback == callback);
    m_mouseButtonCallback = 0;
  }
  void RegisterForDisplaySize(int(*callback)(const CSizeEvent &)) {
    ASSERT(!m_displaySizeCallback);
    m_displaySizeCallback = callback;
  }
  void UnregisterForDisplaySize(int(*callback)(const CSizeEvent &)) {
    ASSERT(m_displaySizeCallback == callback);
    m_displaySizeCallback = 0;
  }
  void RegisterForEvent(CSimpleFrame *frame, CSimpleEventType event, unsigned int priority);
  void RegisterFrame(CSimpleFrame *frame);
  void SetLayoutMode(int enabled) {
    m_layout.enabled = enabled;
  }
  int IsLayoutEnabled() const {
    return m_layout.enabled;
  }
  int IsMovingOrResizing() const {
    return m_layout.frame != 0;
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
  int  StartMoveOrResizeFrame(CSimpleFrame *frame, const CMouseEvent &start, int resize);
  int  StartMoveOrResizeFrame(const CMouseEvent &start, int resize);
  void StopMoveOrResizeFrame();
  unsigned long GetLastEventTime() const {
    return m_eventTime;
  }
  void UpdateEventTime(unsigned long time) {
    m_eventTime = time;
  }
  void UnregisterForEvent(CSimpleFrame *frame, CSimpleEventType event);
  void UnregisterFrame(CSimpleFrame *frame);
  void ValidateDeletedFrame(CSimpleFrame *frame);

  HLAYER__                                            *m_screenLayer;
  HLAYER__                                            *m_cursorLayer;
  HMODEL                                               m_cursor;
  int                                                  m_cursorVisible;
  CSimpleFrame                                        *m_mouseFocus;
  CSimpleFrame                                        *m_mouseCapture;
  CSimpleFrame                                        *m_keydownCapture[780];
  TSList<SIMPLEFRAMENODE, TSGetLink<SIMPLEFRAMENODE> > m_frames;
  TSList<SIMPLEFRAMENODE, TSGetLink<SIMPLEFRAMENODE> > m_destroyed;
  CFrameStrata                                        *m_strata[6];
  frame_layout                                         m_layout;
  CSimpleSortedArray<FRAMEPRIORITY *>                  m_eventqueue[4][5];
  unsigned int                                         m_eventTime;
  int                                                  m_checkFocus;
  EVENT_DATA_MOUSE                                     m_mousePosition;
  int(*m_mouseButtonCallback)(const CMouseEvent &event);
  int(*m_displaySizeCallback)(const CSizeEvent &event);

 private:
  void EnableEvents();
  void DisableEvents();

  static int OnChar(const EVENT_DATA_CHAR *pCharEvtData, void *param);
  static int OnIme(const EVENT_DATA_IME *pImeData, void *param);
  static int OnKeyDown(const EVENT_DATA_KEY *pKeyData, void *param);
  static int OnKeyUp(const EVENT_DATA_KEY *pKeyData, void *param);
  static int OnKeyDownRepeat(const EVENT_DATA_KEY *pKeyData, void *param);
  static int OnMouseMove(const EVENT_DATA_MOUSE *pMouseData, void *param);
  static int OnMouseMoveRelative(const EVENT_DATA_MOUSE *pMouseData, void *param);
  static int OnMouseDown(const EVENT_DATA_MOUSE *pMouseData, void *param);
  static int OnMouseUp(const EVENT_DATA_MOUSE *pMouseData, void *param);
  static int OnMouseWheel(const EVENT_DATA_MOUSE *pMouseData, void *param);
  static int OnDisplaySizeChanged(const EVENT_DATA_SIZE *pSizeData, void *param);

  static CSimpleTop *s_instance;
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
