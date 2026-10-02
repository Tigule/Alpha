#include <Base/Base.h>

#include "Frame/CSimpleTop.h"

#include "Base/Activity.h"
#include "Base/Coordinate.h"
#include "Base/Handle.h"
#include "Event/CMouseEvent.h"
#include "Gxu/IGxuFont.h"
#include "Os/OsTime.h"
#include "Services/Camera.h"
#include "Tempest/c3vector.h"

#include <string.h>

class FRAMEPRIORITY {
 public:
  CSimpleFrame *frame;
  UINT          priority;

  UINT SimpleSortedArrayValue() {
    return priority;
  }
};

class CFrameStrataNode {
 public:
  CFrameStrataNode() : batchDirty(0) {
  }

  ~CFrameStrataNode() {
    while (frames.Head()) {
      DELIFUSED(frames.Head());
    }

    CRenderBatch *next;

    for (CRenderBatch *batch = renderList.Head(); (int)batch > 0; batch = next) {
      next = renderList.RawNext(batch);
      renderList.UnlinkNode(batch);
    }
  }

  BOOL IsEmpty() {
    return frames.IsEmpty();
  }

  BOOL AddFrame(CSimpleFrame *frame) {
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

  BOOL DelFrame(CSimpleFrame *frame) {
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
    ITERATELIST(CSimpleFrame, frames, frame) {
      if (frame->IsVisible()) {
        frame->OnLayerUpdate(elapsedSec);
      }
    }
  }

  BOOL BuildBatches() {
    if (batchDirty) {
      UINT layer;

      for (layer = 0; layer < NUM_SIMPLEFRAME_DRAWLAYERS; ++layer) {
        CRenderBatch *batch = &batches[layer];

        if (renderList.IsLinked(batch)) {
          renderList.UnlinkNode(batch);
        }

        if ((1 << layer) & batchDirty) {
          batch->Clear();
          ITERATELIST(CSimpleFrame, frames, frame) {
            if (frame->IsVisible() && !frame->IsBeingScrolled()) {
              frame->OnFrameRender(batch, layer);
            }
          }
          batch->Finish();
        }

        if (batch->Count() > 0) {
          renderList.LinkNode(batch, LIST_TAIL, 0);
        }
      }

      batchDirty = 0;
    }

    return batchDirty;
  }

  void RenderBatches() {
    ITERATELIST(CRenderBatch, renderList, batch) {
      CSimpleRender::DrawBatch(batch);
    }
  }

  LISTDECLEX(CSimpleFrame, topLink, frames);
  CRenderBatch batches[5];
  UINT         batchDirty;
  LISTDECLEX(CRenderBatch, renderLink, renderList);
};

class CFrameStrata {
 public:
  CFrameStrata() : batchDirty(0), levelsDirty(0), topLevel(0) {
  }

  ~CFrameStrata() {
    UINT count = levels.Count();
    UINT i;

    for (i = 0; i < count; ++i) {
      CFrameStrataNode *node = levels[i];

      DELIFUSED(node);
      levels[i] = 0;
    }
  }

  BOOL EnumerateFrames(BOOL (*callback)(CSimpleFrame *, LPVOID), LPVOID param) {
    UINT i;

    for (i = 0; i < topLevel; ++i) {
      ITERATELIST(CSimpleFrame, levels[i]->frames, frame) {
        if (!callback(frame, param)) {
          return 0;
        }
      }
    }

    return 1;
  }

  void AddFrame(CSimpleFrame *frame) {
    UINT frameLevel = frame->GetFrameLevel();

    if (frameLevel >= levels.Count()) {
      UINT index = levels.Count();

      levels.SetCount(frameLevel + 1);
      while (index <= frameLevel) {
        levels[index++] = NEW(CFrameStrataNode);
      }
    }

    if (frameLevel >= topLevel) {
      topLevel = frameLevel + 1;
    }

    batchDirty |= levels[frameLevel]->AddFrame(frame);
    levelsDirty = 1;
  }

  void DelFrame(CSimpleFrame *frame) {
    CFrameStrataNode *node = levels[frame->GetFrameLevel()];

    batchDirty |= node->DelFrame(frame);
    levelsDirty = 1;
  }

  CSimpleFrame *GetToplevelFrame(const NTempest::C2Vector &point) {
    UINT level = topLevel;

    while (level) {
      --level;

      ITERATELIST(CSimpleFrame, levels[level]->frames, frame) {
        if (frame->IsVisible() && frame->TestHitRect(point)) {
          return frame->GetToplevelFrame();
        }
      }
    }

    return 0;
  }

  void RaiseFrame(CSimpleFrame *frame) {
    if (frame->IsOccluded()) {
      frame->SetFrameLevel(topLevel, 1);
    }
  }

  void OnFrameMovedOrResized(CSimpleFrame *frame) {
    levelsDirty = 1;
  }

  void OnFrameLayerChanged(CSimpleFrame *frame, UINT layer) {
    if (frame->IsBeingScrolled()) {
      frame->OnUpdateBatch(layer);
    } else {
      CFrameStrataNode *node = levels[frame->GetFrameLevel()];

      node->batchDirty |= 1 << layer;
      batchDirty = 1;
    }
  }

  void OnLayerWindowSizeChanged();

  void OnLayerUpdate(float elapsedSec) {
    UINT level;

    for (level = 0; level < topLevel; ++level) {
      levels[level]->OnLayerUpdate(elapsedSec);
    }
  }

  void CompressLevels() {
    UINT firstEmpty = static_cast<UINT>(-1);
    UINT level = 0;

    while (level < topLevel) {
      if (levels[level]->IsEmpty()) {
        if (firstEmpty == static_cast<UINT>(-1)) {
          firstEmpty = level;
        }
      } else if (firstEmpty != static_cast<UINT>(-1)) {
        UINT delta = level - firstEmpty;

        for (UINT i = level; i < topLevel; ++i) {
          CSimpleFrame *next;

          for (CSimpleFrame *frame = levels[i]->frames.Head(); (int)frame > 0; frame = next) {
            next = levels[i]->frames.RawNext(frame);
            frame->SetFrameLevel(frame->GetFrameLevel() - delta, 0);
          }
        }

        topLevel -= delta;
        firstEmpty = static_cast<UINT>(-1);
        continue;
      }

      ++level;
    }

    if (firstEmpty != static_cast<UINT>(-1)) {
      topLevel = firstEmpty;
    }
  }

  BOOL FrameOccluded(CSimpleFrame *thisFrame) {
    NTempest::CRect otherRect;
    NTempest::CRect thisRect;
    UINT            level = thisFrame->GetFrameLevel();

    while (level < topLevel) {
      ITERATELIST(CSimpleFrame, levels[level]->frames, otherFrame) {
        if (thisFrame != otherFrame && !otherFrame->IsAncestor(thisFrame)) {
          thisFrame->GetRect(&thisRect);
          otherFrame->GetRect(&otherRect);
          if (thisRect.Intersect(otherRect).NotEmpty()) {
            return 1;
          }
        }
      }

      ++level;
    }

    return 0;
  }

  void CheckOcclusion() {
    UINT i;

    for (i = 0; i < topLevel; ++i) {
      ITERATELIST(CSimpleFrame, levels[i]->frames, frame) {
        if (frame->IsToplevel()) {
          frame->SetOccluded(FrameOccluded(frame));
        }
      }
    }
  }

  BOOL BuildBatches(int compress) {
    if (levelsDirty) {
      if (compress) {
        CompressLevels();
      }

      CheckOcclusion();
      levelsDirty = 0;
    }

    if (batchDirty) {
      UINT level;

      batchDirty = 0;
      for (level = 0; level < topLevel; ++level) {
        if (levels[level]->BuildBatches()) {
          batchDirty = 1;
        }
      }
    }

    return batchDirty;
  }

  void RenderBatches() {
    UINT level;

    for (level = 0; level < topLevel; ++level) {
      levels[level]->RenderBatches();
    }
  }

  int                              batchDirty;
  int                              levelsDirty;
  UINT                             topLevel;
  TSFixedArray<CFrameStrataNode *> levels;
};

static const float EVENT_PRIORITY_ABOVE_NORMAL = 1.0f;

static void PaintCursor(LPVOID, const RECTF *, const RECTF *, float);
static void PaintScreen(LPVOID, const RECTF *, const RECTF *, float elapsedSec);

CSimpleTop *CSimpleTop::s_instance;

static void PaintCursor(LPVOID, const RECTF *, const RECTF *, float) {
  ActivityBegin(ACTIVITY_FRAMEMANAGER);

  CSimpleTop *top = CSimpleTop::GetInstance();
  ASSERT(top);
  top->DrawCursor();

  ActivityEnd(ACTIVITY_FRAMEMANAGER);
}

static void PaintScreen(LPVOID, const RECTF *, const RECTF *, float elapsedSec) {
  ActivityBegin(ACTIVITY_FRAMEMANAGER);

  CSimpleTop *top = CSimpleTop::GetInstance();
  ASSERT(top);
  top->OnLayerUpdate(elapsedSec);
  top->OnLayerRender();

  ActivityEnd(ACTIVITY_FRAMEMANAGER);
}

CSimpleTop::CSimpleTop()
    : m_cursor(0), m_cursorVisible(1), m_mouseFocus(0), m_mouseCapture(0), m_checkFocus(1), m_mouseButtonCallback(0), m_displaySizeCallback(0) {
  float x;
  float y;
  UINT  i;

  ASSERT(!s_instance);
  s_instance = this;

  ScrnLayerCreate(0, 1.0f, 4, 0, PaintScreen, &m_screenLayer);
  ScrnLayerCreate(0, 8.0f, 5, 0, PaintCursor, &m_cursorLayer);

  EnableEvents();
  memset(m_keydownCapture, 0, sizeof(m_keydownCapture));

  NDCToDDC(1.0f, 1.0f, &m_rect.r, &m_rect.b);
  m_flags |= 1;

  memset(&m_layout, 0, sizeof(m_layout));
  for (i = 0; i < NUM_FRAME_STRATA; ++i) {
    m_strata[i] = NEW(CFrameStrata);
  }

  memset(&m_mousePosition, 0, sizeof(m_mousePosition));
  EventInputGetMousePosition(&x, &y);
  DDCToNDC(x, y, &m_mousePosition.x, &m_mousePosition.y);

  m_mousePosition.time = OsGetAsyncTimeMs();
  m_eventTime = m_mousePosition.time;
}

CSimpleTop::~CSimpleTop() {
  UINT             i;

  SetCursor(0);

  ITERATELIST(SIMPLEFRAMENODE, m_destroyed, iterNode) {
    DELIFUSED(iterNode->frame);
  }

  m_destroyed.Clear();

  for (i = 0; i < NUM_FRAME_STRATA; ++i) {
    DELIFUSED(m_strata[i]);
    m_strata[i] = 0;
  }

  UINT strata = NUM_SIMPLEFRAME_DRAWLAYERS;
  while (strata--) {
    for (i = 0; i < NUM_SIMPLE_EVENTS; ++i) {
      ASSERT(m_eventqueue[i][strata].Count() == 0);
    }
  }

  DisableEvents();
  HandleClose((HOBJECT)m_screenLayer);
  HandleClose((HOBJECT)m_cursorLayer);
  s_instance = 0;
}

void CSimpleTop::EnumerateFrames(BOOL (*callback)(CSimpleFrame *, LPVOID), LPVOID param) {
  UINT i;

  for (i = 0; i < NUM_FRAME_STRATA; ++i) {
    if (!m_strata[i]->EnumerateFrames(callback, param)) {
      break;
    }
  }
}

void CSimpleTop::ValidateDeletedFrame(CSimpleFrame *frame) {
}

void CSimpleTop::RegisterFrame(CSimpleFrame *frame) {
  m_strata[frame->GetFrameStrata()]->AddFrame(frame);
}

void CSimpleTop::UnregisterFrame(CSimpleFrame *frame) {
  m_strata[frame->GetFrameStrata()]->DelFrame(frame);
}

void CSimpleTop::NotifyFrameMovedOrResized(CSimpleFrame *frame) {
  m_strata[frame->GetFrameStrata()]->OnFrameMovedOrResized(frame);

  if (m_layout.frame) {
    m_strata[frame->GetFrameStrata()]->BuildBatches(m_mouseCapture == 0);
    RaiseFrame(m_layout.frame, 0);
  }

  m_checkFocus = 1;
}

void CSimpleTop::NotifyFrameLayerChanged(CSimpleFrame *frame, UINT layer) {
  m_strata[frame->GetFrameStrata()]->OnFrameLayerChanged(frame, layer);
}

void CSimpleTop::RegisterForEvent(CSimpleFrame *frame, CSimpleEventType event, UINT priority) {
  CSimpleSortedArray<FRAMEPRIORITY *> *queue;
  FRAMEPRIORITY                       *entry;

  ASSERT(frame->IsInitialized());
  ASSERT(frame->IsVisible());
  ASSERT(event < NUM_SIMPLE_EVENTS);

  queue = &m_eventqueue[event][frame->GetFrameStrata()];
  if (priority == static_cast<UINT>(-1)) {
    priority = frame->GetFrameLevel();
  }

  entry = NEW(FRAMEPRIORITY);
  entry->frame = frame;
  entry->priority = priority;
  queue->Insert(entry);

  if (event == SIMPLE_EVENT_MOUSE) {
    m_checkFocus = 1;
  }
}

void CSimpleTop::UnregisterForEvent(CSimpleFrame *frame, CSimpleEventType event) {
  CSimpleSortedArray<FRAMEPRIORITY *> *queue;
  FRAMEPRIORITY                       *entry = 0;
  UINT                                 count;
  UINT                                 i;

  ASSERT(event < NUM_SIMPLE_EVENTS);

  queue = &m_eventqueue[event][frame->GetFrameStrata()];
  count = queue->Count();
  for (i = 0; i < count; ++i) {
    entry = (*queue)[i];
    if (entry->frame == frame) {
      break;
    }
  }

  if (i == count) {
    return;
  }

  queue->Remove(i);
  DEL(entry);

  if (event == SIMPLE_EVENT_KEY) {
    for (i = 0; i < KEY_LAST; ++i) {
      if (m_keydownCapture[i] == frame) {
        m_keydownCapture[i] = 0;
      }
    }
  } else if (event == SIMPLE_EVENT_MOUSE) {
    if (m_mouseCapture == frame) {
      m_mouseCapture = 0;
    }

    if (m_mouseFocus == frame) {
      m_mouseFocus = 0;
      m_checkFocus = 1;
      frame->OnLayerCursorExit();
    }
  }
}

void CSimpleTop::RegisterForDelete(CSimpleFrame *frame) {
  SIMPLEFRAMENODE *node = m_destroyed.NewNode(LIST_TAIL, 0, 0);

  node->frame = frame;
}

void CSimpleTop::SetCursor(HMODEL cursor) {
  if (m_cursor) {
    HandleClose(m_cursor);
  }

  if (cursor) {
    m_cursor = (HMODEL)HandleDuplicate(cursor);
  } else {
    m_cursor = 0;
  }
}

BOOL CSimpleTop::RaiseFrame(const NTempest::C2Vector &pt) {
  UINT i = NUM_SIMPLEFRAME_DRAWLAYERS;

  while (i) {
    CSimpleFrame *frame = m_strata[--i]->GetToplevelFrame(pt);

    if (frame) {
      RaiseFrame(frame, 0);
      return 1;
    }
  }

  return 0;
}

BOOL CSimpleTop::RaiseFrame(CSimpleFrame *frame, int checkOcclusion) {
  CSimpleFrame *topframe = frame->GetToplevelFrame();

  if (!topframe) {
    return 0;
  }

  if (checkOcclusion) {
    CFrameStrata   *strata;
    UINT            level;
    NTempest::CRect frameRect;
    NTempest::CRect otherRect;
    int             occluded = 0;

    if (!topframe->IsRectValid() || topframe->IsResizePending()) {
      topframe->Resize(1);
    }

    strata = m_strata[topframe->GetFrameStrata()];
    level = topframe->GetFrameLevel();
    while (level < strata->topLevel && !occluded) {
      ITERATELIST(CSimpleFrame, strata->levels[level]->frames, other) {
        if (other != topframe && !other->IsAncestor(topframe)) {
          topframe->GetRect(&frameRect);
          other->GetRect(&otherRect);
          if (frameRect.Intersect(otherRect).NotEmpty()) {
            occluded = 1;
            break;
          }
        }
      }

      ++level;
    }

    topframe->SetOccluded(occluded);
  }

  m_strata[topframe->GetFrameStrata()]->RaiseFrame(topframe);

  return 1;
}

BOOL CSimpleTop::LowerFrame(CSimpleFrame *frame) {
  return 0;
}

BOOL CSimpleTop::StartMoveOrResizeFrame(CSimpleFrame *frame, const CMouseEvent &start, int resize) {
  if (!frame->FlattenFrame(this, 0.0f, 0.0f, 0.0f, 0.0f, &m_layout.final)) {
    return 0;
  }

  frame->SetUserPlaced(1);
  m_layout.last.x = start.x;
  m_layout.last.y = start.y;

  if (!resize) {
    m_layout.frame = frame;
    m_layout.anchor = FRAMEPOINT_CENTER;
    return 1;
  }

  NTempest::C2Vector pt(start.x - m_layout.final.l, start.y - m_layout.final.t);
  float              size_y = (m_layout.final.b - m_layout.final.t) * 0.25f;

  if (pt.x < (m_layout.final.r - m_layout.final.l) * 0.25f) {
    if (pt.y < size_y) {
      m_layout.frame = frame;
      m_layout.anchor = FRAMEPOINT_BOTTOMLEFT;
      return 1;
    }

    if (pt.y < size_y * 3.0f) {
      m_layout.frame = frame;
      m_layout.anchor = FRAMEPOINT_LEFT;
      return 1;
    }

    m_layout.frame = frame;
    m_layout.anchor = FRAMEPOINT_TOPLEFT;
    return 1;
  }

  if (pt.x < (m_layout.final.r - m_layout.final.l) * 0.75f) {
    if (pt.y < size_y) {
      m_layout.frame = frame;
      m_layout.anchor = FRAMEPOINT_BOTTOM;
      return 1;
    }

    if (pt.y < size_y * 3.0f) {
      m_layout.frame = frame;
      m_layout.anchor = FRAMEPOINT_CENTER;
      return 1;
    }

    m_layout.frame = frame;
    m_layout.anchor = FRAMEPOINT_TOP;
    return 1;
  }

  if (pt.y < size_y) {
    m_layout.frame = frame;
    m_layout.anchor = FRAMEPOINT_BOTTOMRIGHT;
    return 1;
  }

  if (pt.y < size_y * 3.0f) {
    m_layout.frame = frame;
    m_layout.anchor = FRAMEPOINT_RIGHT;
    return 1;
  }

  m_layout.frame = frame;
  m_layout.anchor = FRAMEPOINT_TOPRIGHT;
  return 1;
}

BOOL CSimpleTop::StartMoveOrResizeFrame(const CMouseEvent &start, int resize) {
  CSimpleFrame *frame = 0;
  UINT          strata = NUM_SIMPLEFRAME_DRAWLAYERS;

  while (strata && !frame) {
    NTempest::C2Vector point(start.x, start.y);

    frame = m_strata[--strata]->GetToplevelFrame(point);
  }

  if (!frame) {
    return 0;
  }

  if (resize) {
    if (!frame->IsResizable()) {
      return 0;
    }
  } else if (!frame->IsMovable()) {
    return 0;
  }

  return StartMoveOrResizeFrame(frame, start, resize);
}

void CSimpleTop::MoveOrResizeFrame(const CMouseEvent &evt) {
  NTempest::C2Vector delta = NTempest::C2Vector(evt.x, evt.y) - m_layout.last;

  if (delta.x != 0.0f || delta.y != 0.0f) {
    m_layout.frame->DragBy(delta.x, delta.y, m_layout.anchor, &m_layout.final);
  }

  m_layout.last += delta;
}

void CSimpleTop::StopMoveOrResizeFrame() {
  m_layout.frame = 0;
}

void CSimpleTop::OnLayerUpdate(float elapsedSec) {
  UINT strata;

  if (!m_destroyed.IsEmpty()) {
    ITERATELIST(SIMPLEFRAMENODE, m_destroyed, iterNode) {
      DELIFUSED(iterNode->frame);
    }

    m_destroyed.Clear();
  }

  while (CLayoutFrame::ResizePending()) {
  }

  for (strata = 0; strata < NUM_FRAME_STRATA; ++strata) {
    m_strata[strata]->OnLayerUpdate(elapsedSec);
  }

  while (CLayoutFrame::ResizePending()) {
  }

  if (m_checkFocus) {
    m_checkFocus = 0;
    OnMouseMove(&m_mousePosition, this);
  }
}

void CSimpleTop::OnLayerRender() {
  CameraSetupScreenProjection(m_rect, NTempest::C2Vector(0.0f), 0.0f);

  for (UINT strataIndex = 0; strataIndex < NUM_FRAME_STRATA; ++strataIndex) {
    m_strata[strataIndex]->BuildBatches(m_mouseCapture == 0);
    m_strata[strataIndex]->RenderBatches();
  }
}

void CSimpleTop::EnableEvents() {
  EventRegisterEx(EVENT_ID_CHAR, (EVENTHANDLER)OnChar, this, EVENT_PRIORITY_ABOVE_NORMAL);
  EventRegisterEx(EVENT_ID_IME, (EVENTHANDLER)OnIme, this, EVENT_PRIORITY_ABOVE_NORMAL);
  EventRegisterEx(EVENT_ID_KEYDOWN, (EVENTHANDLER)OnKeyDown, this, EVENT_PRIORITY_ABOVE_NORMAL);
  EventRegisterEx(EVENT_ID_KEYUP, (EVENTHANDLER)OnKeyUp, this, EVENT_PRIORITY_ABOVE_NORMAL);
  EventRegisterEx(EVENT_ID_KEYDOWN_REPEATING, (EVENTHANDLER)OnKeyDownRepeat, this, EVENT_PRIORITY_ABOVE_NORMAL);
  EventRegisterEx(EVENT_ID_MOUSEMOVE, (EVENTHANDLER)OnMouseMove, this, EVENT_PRIORITY_ABOVE_NORMAL);
  EventRegisterEx(EVENT_ID_MOUSEMOVE_RELATIVE, (EVENTHANDLER)OnMouseMoveRelative, this, EVENT_PRIORITY_ABOVE_NORMAL);
  EventRegisterEx(EVENT_ID_MOUSEDOWN, (EVENTHANDLER)OnMouseDown, this, EVENT_PRIORITY_ABOVE_NORMAL);
  EventRegisterEx(EVENT_ID_MOUSEUP, (EVENTHANDLER)OnMouseUp, this, EVENT_PRIORITY_ABOVE_NORMAL);
  EventRegisterEx(EVENT_ID_MOUSEWHEEL, (EVENTHANDLER)OnMouseWheel, this, EVENT_PRIORITY_ABOVE_NORMAL);
  EventRegisterEx(EVENT_ID_SIZE, (EVENTHANDLER)OnDisplaySizeChanged, this, EVENT_PRIORITY_ABOVE_NORMAL);
}

void CSimpleTop::DisableEvents() {
  EventUnregisterEx(EVENT_ID_CHAR, (EVENTHANDLER)OnChar, this, -1);
  EventUnregisterEx(EVENT_ID_IME, (EVENTHANDLER)OnIme, this, -1);
  EventUnregisterEx(EVENT_ID_KEYDOWN, (EVENTHANDLER)OnKeyDown, this, -1);
  EventUnregisterEx(EVENT_ID_KEYUP, (EVENTHANDLER)OnKeyUp, this, -1);
  EventUnregisterEx(EVENT_ID_KEYDOWN_REPEATING, (EVENTHANDLER)OnKeyDownRepeat, this, -1);
  EventUnregisterEx(EVENT_ID_MOUSEMOVE, (EVENTHANDLER)OnMouseMove, this, -1);
  EventUnregisterEx(EVENT_ID_MOUSEMOVE_RELATIVE, (EVENTHANDLER)OnMouseMoveRelative, this, -1);
  EventUnregisterEx(EVENT_ID_MOUSEDOWN, (EVENTHANDLER)OnMouseDown, this, -1);
  EventUnregisterEx(EVENT_ID_MOUSEUP, (EVENTHANDLER)OnMouseUp, this, -1);
  EventUnregisterEx(EVENT_ID_MOUSEWHEEL, (EVENTHANDLER)OnMouseWheel, this, -1);
  EventUnregisterEx(EVENT_ID_SIZE, (EVENTHANDLER)OnDisplaySizeChanged, this, -1);
}

void CSimpleTop::DrawCursor() {
  if (m_cursor && m_cursorVisible) {
    CameraSetupScreenProjection(m_rect, NTempest::C2Vector(0.0f), 0.0f);

    NTempest::C2Vector mousePosition(0.0f);
    NDCToDDC(m_mousePosition.x, m_mousePosition.y, &mousePosition.x, &mousePosition.y);

    ModelAnimate(
        m_cursor, NTempest::C3Vector(mousePosition.x, mousePosition.y, 0.0f), 0.0f, NTempest::C3Vector(0.0f, 0.0f, 1.0f), 1.0f,
        NTempest::C3Vector(0.0f), NTempest::C3Vector(0.0f)
    );
    ModelAddToScene(m_cursor, 0);
    ModelRenderScene(0);
  }
}

BOOL CSimpleTop::OnChar(const EVENT_DATA_CHAR *pCharEvtData, LPVOID param) {
  CSimpleTop *top = static_cast<CSimpleTop *>(param);
  int         eaten = 0;
  CCharEvent  charEvent(*pCharEvtData);

  charEvent.SetId(0x40060067);
  UINT strata = NUM_SIMPLEFRAME_DRAWLAYERS;
  while (strata-- && !eaten) {
    CSimpleSortedArray<FRAMEPRIORITY *> *queue;
    FRAMEPRIORITY                      **entry;

    queue = &top->m_eventqueue[SIMPLE_EVENT_CHAR][strata];
    queue->IterateBegin();
    while ((entry = queue->IterateNext()) && !eaten) {
      eaten = (*entry)->frame->OnLayerChar(charEvent);
    }
  }

  return !eaten;
}

BOOL CSimpleTop::OnIme(const EVENT_DATA_IME *pImeData, LPVOID param) {
  CSimpleTop *top = static_cast<CSimpleTop *>(param);
  int         eaten = 0;
  CImeEvent   imeEvent(*pImeData);

  imeEvent.SetId(0x40060068);
  UINT strata = NUM_SIMPLEFRAME_DRAWLAYERS;
  while (strata-- && !eaten) {
    CSimpleSortedArray<FRAMEPRIORITY *> *queue;
    FRAMEPRIORITY                      **entry;

    queue = &top->m_eventqueue[SIMPLE_EVENT_CHAR][strata];
    queue->IterateBegin();
    while ((entry = queue->IterateNext()) && !eaten) {
      eaten = (*entry)->frame->OnLayerIme(imeEvent);
    }
  }

  return !eaten;
}

BOOL CSimpleTop::OnKeyDown(const EVENT_DATA_KEY *pKeyData, LPVOID param) {
  CSimpleTop *top = static_cast<CSimpleTop *>(param);
  int         eaten = 0;
  CKeyEvent   keyEvent(*pKeyData);

  top->m_eventTime = keyEvent.time;
  keyEvent.SetId(0x40060064);
  UINT strata = NUM_SIMPLEFRAME_DRAWLAYERS;
  while (strata-- && !eaten) {
    CSimpleSortedArray<FRAMEPRIORITY *> *queue;
    FRAMEPRIORITY                      **entry;

    queue = &top->m_eventqueue[SIMPLE_EVENT_KEY][strata];
    queue->IterateBegin();
    while ((entry = queue->IterateNext()) && !eaten) {
      CSimpleFrame *frame = (*entry)->frame;

      eaten = frame->OnLayerKeyDown(keyEvent);
      if (eaten) {
        top->m_keydownCapture[pKeyData->key] = frame;
      }
    }
  }

  return !eaten;
}

BOOL CSimpleTop::OnKeyUp(const EVENT_DATA_KEY *pKeyData, LPVOID param) {
  CSimpleTop   *top = static_cast<CSimpleTop *>(param);
  int           eaten = 0;
  CSimpleFrame *frame = top->m_keydownCapture[pKeyData->key];

  top->m_eventTime = pKeyData->time;
  if (frame) {
    CKeyEvent keyEvent(*pKeyData);

    keyEvent.SetId(0x40060066);
    frame->OnLayerKeyUp(keyEvent);
    eaten = 1;
  } else {
    if (pKeyData->key == KEY_PRINTSCREEN) {
      eaten = !OnKeyDown(pKeyData, param);
    }

    top->m_keydownCapture[pKeyData->key] = 0;
  }

  return !eaten;
}

BOOL CSimpleTop::OnKeyDownRepeat(const EVENT_DATA_KEY *pKeyData, LPVOID param) {
  CSimpleTop   *top = static_cast<CSimpleTop *>(param);
  CSimpleFrame *frame = top->m_keydownCapture[pKeyData->key];
  int           eaten = 0;

  top->m_eventTime = pKeyData->time;
  if (frame) {
    CKeyEvent keyEvent(*pKeyData);

    keyEvent.SetId(0x40060065);
    frame->OnLayerKeyDownRepeat(keyEvent);
    eaten = 1;
  }

  return !eaten;
}

BOOL CSimpleTop::OnMouseMove(const EVENT_DATA_MOUSE *pMouseData, LPVOID param) {
  CSimpleTop  *top = static_cast<CSimpleTop *>(param);
  CMouseEvent  mouseEvent(*pMouseData);

  mouseEvent.SetId(0x400500CA);

  CSimpleFrame *last_focus = top->m_mouseFocus;
  CSimpleFrame *next_focus = 0;

  top->m_eventTime = pMouseData->time;
  top->m_mousePosition = *pMouseData;

  if (top->m_layout.frame) {
    CMouseEvent mouseEvent(*pMouseData);

    top->MoveOrResizeFrame(mouseEvent);
    return 0;
  }

  UINT strata = NUM_SIMPLEFRAME_DRAWLAYERS;
  while (strata-- && !next_focus) {
    CSimpleSortedArray<FRAMEPRIORITY *> *queue;
    FRAMEPRIORITY                      **entry;

    queue = &top->m_eventqueue[SIMPLE_EVENT_MOUSE][strata];
    queue->IterateBegin();
    while ((entry = queue->IterateNext()) != 0) {
      CSimpleFrame *frame = (*entry)->frame;

      if (frame->OnLayerTrackUpdate(mouseEvent)) {
        next_focus = frame;
        break;
      }
    }
  }

  if (next_focus != last_focus) {
    top->m_mouseFocus = next_focus;
    if (last_focus) {
      last_focus->OnLayerCursorExit();
    }
    if (next_focus) {
      next_focus->OnLayerCursorEnter();
    }
  }

  return next_focus == 0;
}

BOOL CSimpleTop::OnMouseMoveRelative(const EVENT_DATA_MOUSE *pMouseData, LPVOID param) {
  CSimpleTop   *top = static_cast<CSimpleTop *>(param);
  CSimpleFrame *frame = top->m_mouseFocus;
  int           eaten = 0;

  top->m_eventTime = pMouseData->time;
  if (frame) {
    CMouseEvent mouseEvent(*pMouseData);

    mouseEvent.SetId(0x400500CB);
    frame->OnLayerMouseMoveRelative(mouseEvent);
    eaten = 1;
  }

  return !eaten;
}

BOOL CSimpleTop::OnMouseDown(const EVENT_DATA_MOUSE *pMouseData, LPVOID param) {
  CSimpleTop *top = static_cast<CSimpleTop *>(param);
  int         eaten = 0;
  int (*callback)(const CMouseEvent &) = top->m_mouseButtonCallback;
  CMouseEvent mouseEvent(*pMouseData);

  mouseEvent.SetId(0x400500C8);
  top->m_eventTime = pMouseData->time;

  if (top->m_layout.enabled && (EventIsKeyDown(KEY_CONTROL) || EventIsKeyDown(KEY_ALT))) {
    top->StartMoveOrResizeFrame(mouseEvent, EventIsKeyDown(KEY_CONTROL));
    return 0;
  }

  if (callback && callback(mouseEvent)) {
    return 0;
  }

  CSimpleFrame *frame = top->m_mouseCapture;
  if (!frame) {
    frame = top->m_mouseFocus;
  }

  if (frame) {
    frame->Raise();
    if (frame->GetTitleRegion() && frame->GetTitleRegion()->PtInFrameRect(NTempest::C2Vector(mouseEvent.x, mouseEvent.y))) {
      top->StartMoveOrResizeFrame(frame, mouseEvent, 0);
    } else {
      top->m_mouseCapture = frame;
      frame->OnLayerMouseDown(mouseEvent);
    }

    eaten = 1;
  }

  return !eaten;
}

BOOL CSimpleTop::OnMouseUp(const EVENT_DATA_MOUSE *pMouseData, LPVOID param) {
  CSimpleTop   *top = static_cast<CSimpleTop *>(param);
  CSimpleFrame *frame = top->m_mouseCapture;
  int           eaten = 0;

  top->m_eventTime = pMouseData->time;
  if (top->m_layout.frame) {
    top->StopMoveOrResizeFrame();
    return 0;
  }

  if (frame) {
    CMouseEvent mouseEvent(*pMouseData);

    mouseEvent.SetId(0x400500C9);
    frame->OnLayerMouseUp(mouseEvent);
    eaten = 1;
    if (!mouseEvent.buttonState) {
      top->m_mouseCapture = 0;
    }
  }

  return !eaten;
}

BOOL CSimpleTop::OnMouseWheel(const EVENT_DATA_MOUSE *pMouseData, LPVOID param) {
  CSimpleTop        *top = static_cast<CSimpleTop *>(param);
  int                eaten = 0;
  CMouseEvent        mouseEvent(*pMouseData);

  mouseEvent.SetId(0x400500CD);

  NTempest::C2Vector pt;
  top->GetMousePosition(pt);

  UINT strata = NUM_SIMPLEFRAME_DRAWLAYERS;
  while (strata-- && !eaten) {
    CSimpleSortedArray<FRAMEPRIORITY *> *queue;
    FRAMEPRIORITY                      **entry;

    queue = &top->m_eventqueue[SIMPLE_EVENT_MOUSEWHEEL][strata];
    queue->IterateBegin();
    while ((entry = queue->IterateNext()) && !eaten) {
      CSimpleFrame *frame = (*entry)->frame;

      if (frame->TestHitRect(pt)) {
        eaten = frame->OnLayerMouseWheel(mouseEvent);
      }
    }
  }

  return !eaten;
}

BOOL CSimpleTop::OnDisplaySizeChanged(const EVENT_DATA_SIZE *pSizeData, LPVOID param) {
  CSimpleTop *top = static_cast<CSimpleTop *>(param);
  int (*callback)(const CSizeEvent &) = top->m_displaySizeCallback;

  GxuFontWindowSizeChanged();
  if (callback) {
    CSizeEvent sizeEvent(*pSizeData);

    sizeEvent.SetId(0x40040064);
    callback(sizeEvent);
  }

  return 1;
}
