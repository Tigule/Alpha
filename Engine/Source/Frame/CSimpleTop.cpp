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

static const float EVENT_PRIORITY_ABOVE_NORMAL = 1.0f;

static void PaintCursor(void *, const RECTF *, const RECTF *, float);
static void PaintScreen(void *, const RECTF *, const RECTF *, float elapsedSec);

CSimpleTop *CSimpleTop::s_instance;

CMouseEvent &CMouseEvent::operator=(const EVENT_DATA_MOUSE &rhs) {
  mode = rhs.mode;
  button = rhs.button;
  buttonState = rhs.buttonState;
  metaKeyState = rhs.metaKeyState;
  flags = rhs.flags;
  wheelDistance = rhs.wheelDistance;
  time = rhs.time;
  NDCToDDC(rhs.x, rhs.y, &x, &y);
  return *this;
}

int CFrameStrataNode::BuildBatches() {
  if (batchDirty) {
    unsigned int layer;

    for (layer = 0; layer < NUM_SIMPLEFRAME_DRAWLAYERS; ++layer) {
      CRenderBatch *batch = &batches[layer];

      if (renderList.IsLinked(batch)) {
        renderList.UnlinkNode(batch);
      }

      if (batchDirty & (1 << layer)) {
        batch->Clear();
        ITERATELIST(CSimpleFrame, frames, frame) {
          if (frame->IsVisible() && !frame->IsBeingScrolled()) {
            frame->OnFrameRender(batch, layer);
          }
        }
        batch->Finish();
      }

      if (batch->Count()) {
        renderList.LinkNode(batch, LIST_TAIL, 0);
      }
    }

    batchDirty = 0;
  }

  return batchDirty;
}

CFrameStrata::~CFrameStrata() {
  unsigned int count = levels.Count();
  unsigned int i;

  for (i = 0; i < count; ++i) {
    DELIFUSED(levels[i]);
    levels[i] = 0;
  }
}

int CFrameStrata::FrameOccluded(CSimpleFrame *thisFrame) {
  NTempest::CRect otherRect;
  NTempest::CRect thisRect;
  unsigned int    level = thisFrame->GetFrameLevel();

  while (level < topLevel) {
    ITERATELIST(CSimpleFrame, levels[level]->frames, otherFrame) {
      if (thisFrame != otherFrame && !otherFrame->IsAncestor(thisFrame)) {
        thisFrame->GetRect(&thisRect);
        otherFrame->GetRect(&otherRect);
        thisRect = thisRect.Intersect(otherRect);
        if (thisRect.NotEmpty()) {
          return 1;
        }
      }
    }

    ++level;
  }

  return 0;
}

void CFrameStrata::CheckOcclusion() {
  unsigned int i;

  for (i = 0; i < topLevel; ++i) {
    ITERATELIST(CSimpleFrame, levels[i]->frames, frame) {
      if (frame->IsToplevel()) {
        frame->SetOccluded(FrameOccluded(frame));
      }
    }
  }
}

static void PaintCursor(void *, const RECTF *, const RECTF *, float) {
  ActivityBegin(ACTIVITY_FRAMEMANAGER);

  CSimpleTop *top = CSimpleTop::GetInstance();
  ASSERT(top);
  top->DrawCursor();

  ActivityEnd(ACTIVITY_FRAMEMANAGER);
}

static void PaintScreen(void *, const RECTF *, const RECTF *, float elapsedSec) {
  ActivityBegin(ACTIVITY_FRAMEMANAGER);

  CSimpleTop *top = CSimpleTop::GetInstance();
  ASSERT(top);
  top->OnLayerUpdate(elapsedSec);
  top->OnLayerRender();

  ActivityEnd(ACTIVITY_FRAMEMANAGER);
}

CSimpleTop::CSimpleTop()
    : m_cursor(0), m_cursorVisible(1), m_mouseFocus(0), m_mouseCapture(0), m_checkFocus(1), m_mouseButtonCallback(0), m_displaySizeCallback(0) {
  float        x;
  float        y;
  unsigned int i;

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
  SIMPLEFRAMENODE *node;
  unsigned int     i;
  unsigned int     strata;

  SetCursor(0);

  ITERATELIST(SIMPLEFRAMENODE, m_destroyed, iterNode) {
    DELIFUSED(iterNode->frame);
  }

  while ((node = m_destroyed.Head()) != 0) {
    m_destroyed.DeleteNode(node);
  }

  for (i = 0; i < NUM_FRAME_STRATA; ++i) {
    DELIFUSED(m_strata[i]);
    m_strata[i] = 0;
  }

  for (strata = 0; strata < NUM_SIMPLEFRAME_DRAWLAYERS; ++strata) {
    for (i = 0; i < NUM_SIMPLE_EVENTS; ++i) {
      ASSERT(m_eventqueue[i][strata].Count() == 0);
    }
  }

  DisableEvents();
  HandleClose((HOBJECT)m_screenLayer);
  HandleClose((HOBJECT)m_cursorLayer);
  s_instance = 0;
}

void CSimpleTop::EnumerateFrames(int(*callback)(CSimpleFrame *, void *), void *param) {
  unsigned int i;

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
  CFrameStrata *strata;

  m_strata[frame->GetFrameStrata()]->OnFrameMovedOrResized(frame);

  if (!m_layout.frame) {
    m_checkFocus = 1;
    return;
  }

  strata = m_strata[frame->GetFrameStrata()];
  if (strata->levelsDirty) {
    if (!m_mouseCapture) {
      strata->CompressLevels();
    }

    strata->CheckOcclusion();
    strata->levelsDirty = 0;
  }

  if (strata->batchDirty) {
    strata->BuildBatches(!m_mouseCapture);
  }

  RaiseFrame(m_layout.frame, 0);
  m_checkFocus = 1;
}

void CSimpleTop::NotifyFrameLayerChanged(CSimpleFrame *frame, unsigned int layer) {
  m_strata[frame->GetFrameStrata()]->OnFrameLayerChanged(frame, layer);
}

void CSimpleTop::RegisterForEvent(CSimpleFrame *frame, CSimpleEventType event, unsigned int priority) {
  CSimpleSortedArray<FRAMEPRIORITY *> *queue;
  FRAMEPRIORITY                       *entry;

  ASSERT(frame->IsInitialized());
  ASSERT(frame->IsVisible());
  ASSERT(event < NUM_SIMPLE_EVENTS);

  queue = &m_eventqueue[event][frame->GetFrameStrata()];
  if (priority == static_cast<unsigned int>(-1)) {
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
  unsigned int                         count;
  unsigned int                         i;

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

int CSimpleTop::RaiseFrame(const NTempest::C2Vector &pt) {
  unsigned int i = NUM_SIMPLEFRAME_DRAWLAYERS;

  while (i) {
    CSimpleFrame *frame = m_strata[--i]->GetToplevelFrame(pt);

    if (frame) {
      RaiseFrame(frame, 0);
      return 1;
    }
  }

  return 0;
}

int CSimpleTop::RaiseFrame(CSimpleFrame *frame, int checkOcclusion) {
  CSimpleFrame *topframe = frame->GetToplevelFrame();

  if (!topframe) {
    return 0;
  }

  if (checkOcclusion) {
    CFrameStrata   *strata;
    unsigned int    level;
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

int CSimpleTop::LowerFrame(CSimpleFrame *frame) {
  return 0;
}

int CSimpleTop::StartMoveOrResizeFrame(CSimpleFrame *frame, const CMouseEvent &start, int resize) {
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

int CSimpleTop::StartMoveOrResizeFrame(const CMouseEvent &start, int resize) {
  CSimpleFrame *frame = 0;
  unsigned int  strata = NUM_SIMPLEFRAME_DRAWLAYERS;

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
  NTempest::C2Vector delta(evt.x - m_layout.last.x, evt.y - m_layout.last.y);

  if (delta.x != 0.0f || delta.y != 0.0f) {
    m_layout.frame->DragBy(delta.x, delta.y, m_layout.anchor, &m_layout.final);
  }

  m_layout.last.x += delta.x;
  m_layout.last.y += delta.y;
}

void CSimpleTop::StopMoveOrResizeFrame() {
  m_layout.frame = 0;
}

void CSimpleTop::OnLayerUpdate(float elapsedSec) {
  SIMPLEFRAMENODE *node;
  unsigned int     strata;

  ITERATELIST(SIMPLEFRAMENODE, m_destroyed, iterNode) {
    DELIFUSED(iterNode->frame);
  }

  while ((node = m_destroyed.Head()) != 0) {
    m_destroyed.DeleteNode(node);
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
  NTempest::C2Vector screenPoint(0.0f);
  unsigned int       strataIndex;

  CameraSetupScreenProjection(m_rect, screenPoint, 0.0f);

  for (strataIndex = 0; strataIndex < NUM_FRAME_STRATA; ++strataIndex) {
    CFrameStrata *strata = m_strata[strataIndex];

    if (strata->levelsDirty) {
      if (!m_mouseCapture) {
        strata->CompressLevels();
      }

      strata->CheckOcclusion();
      strata->levelsDirty = 0;
    }

    if (strata->batchDirty) {
      strata->BuildBatches(!m_mouseCapture);
    }

    strata->RenderBatches();
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

int CSimpleTop::OnChar(const EVENT_DATA_CHAR *pCharEvtData, void *param) {
  CSimpleTop  *top = static_cast<CSimpleTop *>(param);
  CCharEvent   charEvent(*pCharEvtData);
  unsigned int strata = NUM_SIMPLEFRAME_DRAWLAYERS;
  int          eaten = 0;

  charEvent.SetId(0x40060067);
  while (strata && !eaten) {
    CSimpleSortedArray<FRAMEPRIORITY *> *queue;
    FRAMEPRIORITY                      **entry;

    --strata;
    queue = &top->m_eventqueue[SIMPLE_EVENT_CHAR][strata];
    queue->IterateBegin();
    while ((entry = queue->IterateNext()) && !eaten) {
      eaten = (*entry)->frame->OnLayerChar(charEvent);
    }
  }

  return !eaten;
}

int CSimpleTop::OnIme(const EVENT_DATA_IME *pImeData, void *param) {
  CSimpleTop  *top = static_cast<CSimpleTop *>(param);
  CImeEvent    imeEvent(*pImeData);
  unsigned int strata = NUM_SIMPLEFRAME_DRAWLAYERS;
  int          eaten = 0;

  imeEvent.SetId(0x40060068);
  while (strata && !eaten) {
    CSimpleSortedArray<FRAMEPRIORITY *> *queue;
    FRAMEPRIORITY                      **entry;

    --strata;
    queue = &top->m_eventqueue[SIMPLE_EVENT_CHAR][strata];
    queue->IterateBegin();
    while ((entry = queue->IterateNext()) && !eaten) {
      eaten = (*entry)->frame->OnLayerIme(imeEvent);
    }
  }

  return !eaten;
}

int CSimpleTop::OnKeyDown(const EVENT_DATA_KEY *pKeyData, void *param) {
  CSimpleTop  *top = static_cast<CSimpleTop *>(param);
  CKeyEvent    keyEvent(*pKeyData);
  unsigned int strata = NUM_SIMPLEFRAME_DRAWLAYERS;
  int          eaten = 0;

  top->m_eventTime = keyEvent.time;
  keyEvent.SetId(0x40060064);
  while (strata && !eaten) {
    CSimpleSortedArray<FRAMEPRIORITY *> *queue;
    FRAMEPRIORITY                      **entry;

    --strata;
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

int CSimpleTop::OnKeyUp(const EVENT_DATA_KEY *pKeyData, void *param) {
  CSimpleTop   *top = static_cast<CSimpleTop *>(param);
  CSimpleFrame *frame = top->m_keydownCapture[pKeyData->key];
  int           eaten = 0;

  top->m_eventTime = pKeyData->time;
  if (frame) {
    CKeyEvent keyEvent(*pKeyData);

    keyEvent.SetId(0x40060066);
    frame->OnLayerKeyUp(keyEvent);
    return 0;
  }

  if (pKeyData->key == KEY_PRINTSCREEN) {
    eaten = !OnKeyDown(pKeyData, param);
  }

  top->m_keydownCapture[pKeyData->key] = 0;
  return !eaten;
}

int CSimpleTop::OnKeyDownRepeat(const EVENT_DATA_KEY *pKeyData, void *param) {
  CSimpleTop   *top = static_cast<CSimpleTop *>(param);
  CSimpleFrame *frame = top->m_keydownCapture[pKeyData->key];

  top->m_eventTime = pKeyData->time;
  if (frame) {
    CKeyEvent keyEvent(*pKeyData);

    keyEvent.SetId(0x40060065);
    frame->OnLayerKeyDownRepeat(keyEvent);
    return 0;
  }

  return 1;
}

int CSimpleTop::OnMouseMove(const EVENT_DATA_MOUSE *pMouseData, void *param) {
  CSimpleTop   *top = static_cast<CSimpleTop *>(param);
  CMouseEvent   mouseEvent;
  CSimpleFrame *last_focus = top->m_mouseFocus;
  CSimpleFrame *next_focus = 0;
  unsigned int  strata = NUM_SIMPLEFRAME_DRAWLAYERS;

  mouseEvent = *pMouseData;
  mouseEvent.SetId(0x400500CA);
  top->m_eventTime = pMouseData->time;
  top->m_mousePosition = *pMouseData;

  if (top->m_layout.frame) {
    CMouseEvent mouseEvent;

    mouseEvent = *pMouseData;
    top->MoveOrResizeFrame(mouseEvent);
    return 0;
  }

  while (strata && !next_focus) {
    CSimpleSortedArray<FRAMEPRIORITY *> *queue;
    FRAMEPRIORITY                      **entry;

    --strata;
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

int CSimpleTop::OnMouseMoveRelative(const EVENT_DATA_MOUSE *pMouseData, void *param) {
  CSimpleTop   *top = static_cast<CSimpleTop *>(param);
  CSimpleFrame *frame = top->m_mouseFocus;

  top->m_eventTime = pMouseData->time;
  if (frame) {
    CMouseEvent mouseEvent(*pMouseData);

    mouseEvent.SetId(0x400500CB);
    frame->OnLayerMouseMoveRelative(mouseEvent);
    return 0;
  }

  return 1;
}

int CSimpleTop::OnMouseDown(const EVENT_DATA_MOUSE *pMouseData, void *param) {
  CSimpleTop   *top = static_cast<CSimpleTop *>(param);
  CMouseEvent   mouseEvent;
  CSimpleFrame *frame;
  int           eaten = 0;

  mouseEvent = *pMouseData;
  mouseEvent.SetId(0x400500C8);
  top->m_eventTime = pMouseData->time;

  if (top->m_layout.enabled && (EventIsKeyDown(KEY_CONTROL) || EventIsKeyDown(KEY_ALT))) {
    top->StartMoveOrResizeFrame(mouseEvent, EventIsKeyDown(KEY_CONTROL));
    eaten = 1;
  } else if (top->m_mouseButtonCallback) {
    eaten = top->m_mouseButtonCallback(mouseEvent);
  }

  if (!eaten) {
    frame = top->m_mouseCapture;
    if (!frame) {
      frame = top->m_mouseFocus;
    }

    if (frame) {
      frame->Raise();
      if (frame->GetTitleRegion()) {
        NTempest::C2Vector point(mouseEvent.x, mouseEvent.y);
        CLayoutFrame      *title = reinterpret_cast<CLayoutFrame *>(frame->GetTitleRegion());

        if (title->PtInFrameRect(point)) {
          top->StartMoveOrResizeFrame(frame, mouseEvent, 0);
          eaten = 1;
        }
      }

      if (!eaten) {
        top->m_mouseCapture = frame;
        frame->OnLayerMouseDown(mouseEvent);
        eaten = 1;
      }
    }
  }

  return !eaten;
}

int CSimpleTop::OnMouseUp(const EVENT_DATA_MOUSE *pMouseData, void *param) {
  CSimpleTop   *top = static_cast<CSimpleTop *>(param);
  CSimpleFrame *frame = top->m_mouseCapture;

  top->m_eventTime = pMouseData->time;
  if (top->m_layout.frame) {
    top->StopMoveOrResizeFrame();
    return 0;
  }

  if (frame) {
    CMouseEvent mouseEvent(*pMouseData);

    mouseEvent.SetId(0x400500C9);
    frame->OnLayerMouseUp(mouseEvent);
    if (!mouseEvent.buttonState) {
      top->m_mouseCapture = 0;
    }
    return 0;
  }

  return 1;
}

int CSimpleTop::OnMouseWheel(const EVENT_DATA_MOUSE *pMouseData, void *param) {
  CSimpleTop        *top = static_cast<CSimpleTop *>(param);
  CMouseEvent        mouseEvent;
  NTempest::C2Vector pt;
  unsigned int       strata = NUM_SIMPLEFRAME_DRAWLAYERS;
  int                eaten = 0;

  mouseEvent = *pMouseData;
  mouseEvent.SetId(0x400500CD);
  NDCToDDC(top->m_mousePosition.x, top->m_mousePosition.y, &pt.x, &pt.y);

  while (strata && !eaten) {
    CSimpleSortedArray<FRAMEPRIORITY *> *queue;
    FRAMEPRIORITY                      **entry;

    --strata;
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

int CSimpleTop::OnDisplaySizeChanged(const EVENT_DATA_SIZE *pSizeData, void *param) {
  CSimpleTop *top = static_cast<CSimpleTop *>(param);

  GxuFontWindowSizeChanged();
  if (top->m_displaySizeCallback) {
    CSizeEvent sizeEvent(*pSizeData);

    sizeEvent.SetId(0x40040064);
    top->m_displaySizeCallback(sizeEvent);
  }

  return 1;
}
