#include "CInputObserver.h"
#include "CMouseEvent.h"

static TRefCntPtr<CInputObserver> s_pInputObserver;

void InputObserverInitialize() {
}

void InputObserverDestroy() {
  s_pInputObserver = static_cast<CInputObserver *>(0);
}

CInputObserver *CInputObserver::GetInputObserver() {
  if (!s_pInputObserver) {
    s_pInputObserver = NEW(CInputObserver);
  }

  return s_pInputObserver;
}

int CInputObserver::OnChar(const EVENT_DATA_CHAR *pCharEvtData, void *param) {
  CCharEvent event(*pCharEvtData);
  event.SetId(0x40060067);
  static_cast<CInputObserver *>(param)->DispatchEvent(event);
  return 1;
}

int CInputObserver::OnKeyDown(const EVENT_DATA_KEY *pKeyData, void *param) {
  CKeyEvent event(*pKeyData);
  event.SetId(0x40060064);
  static_cast<CInputObserver *>(param)->DispatchEvent(event);
  return 1;
}

int CInputObserver::OnKeyRepeat(const EVENT_DATA_KEY *pKeyData, void *param) {
  CKeyEvent event(*pKeyData);
  event.SetId(0x40060065);
  static_cast<CInputObserver *>(param)->DispatchEvent(event);
  return 1;
}

int CInputObserver::OnKeyUp(const EVENT_DATA_KEY *pKeyData, void *param) {
  CKeyEvent event(*pKeyData);
  event.SetId(0x40060066);
  static_cast<CInputObserver *>(param)->DispatchEvent(event);
  return 1;
}

#define INPUT_OBSERVER_MOUSE_HANDLER(name, eventId)                            \
  int CInputObserver::name(const EVENT_DATA_MOUSE *data, void *param) { \
    CMouseEvent event(*data);                                                  \
    event.SetId(eventId);                                                      \
    static_cast<CInputObserver *>(param)->DispatchEvent(event);                \
    return 1;                                                                  \
  }

INPUT_OBSERVER_MOUSE_HANDLER(OnMouseDown, 0x400500C8)
INPUT_OBSERVER_MOUSE_HANDLER(OnMouseUp, 0x400500C9)
INPUT_OBSERVER_MOUSE_HANDLER(OnMouseMove, 0x400500CA)
INPUT_OBSERVER_MOUSE_HANDLER(OnMouseWheel, 0x400500CD)
INPUT_OBSERVER_MOUSE_HANDLER(OnMouseMoveRelative, 0x400500CB)
INPUT_OBSERVER_MOUSE_HANDLER(OnMouseModeChanged, 0x400500CC)

#undef INPUT_OBSERVER_MOUSE_HANDLER

int CInputObserver::OnIme(const EVENT_DATA_IME *pImeData, void *param) {
  CImeEvent event(*pImeData);
  event.SetId(0x40060068);
  static_cast<CInputObserver *>(param)->DispatchEvent(event);
  return 1;
}

int CInputObserver::OnWindowSize(const EVENT_DATA_SIZE *pSizeData, void *param) {
  CSizeEvent event(*pSizeData);
  event.SetId(0x40040064);
  static_cast<CInputObserver *>(param)->DispatchEvent(event);
  return 1;
}

int CInputObserver::OnWindowFocus(const EVENT_DATA_FOCUS *pFocusData, void *param) {
  CFocusEvent event(*pFocusData);
  event.SetId(0x40040065);
  static_cast<CInputObserver *>(param)->DispatchEvent(event);
  return 1;
}

#define INPUT_OBSERVER_REGISTRATION(name, eventId, handler, inputEventId)                  \
  void CInputObserver::Register##name(CObserver *observer) {                    \
    CInputObserver *input = GetInputObserver();                                             \
    if (!input->IsEventRegistered(eventId)) {                                               \
      EventRegisterEx(inputEventId, reinterpret_cast<EVENTHANDLER>(handler), input, EVENT_PRIORITY_NORMAL); \
    }                                                                                       \
    GetInputObserver()->RegisterEvent(eventId, eventId, observer);                          \
  }                                                                                         \
  void CInputObserver::Unregister##name(CObserver *observer) {                   \
    CInputObserver *input = GetInputObserver();                                             \
    input->UnregisterEvent(eventId, observer);                                              \
    if (!GetInputObserver()->IsEventRegistered(eventId)) {                                  \
      input = GetInputObserver();                                                           \
      EventUnregisterEx(inputEventId, reinterpret_cast<EVENTHANDLER>(handler), input, 0xFFFFFFFF); \
    }                                                                                       \
  }

INPUT_OBSERVER_REGISTRATION(KeyDown, 0x40060064, OnKeyDown, EVENT_ID_KEYDOWN)
INPUT_OBSERVER_REGISTRATION(KeyDownRepeating, 0x40060065, OnKeyRepeat, EVENT_ID_KEYDOWN_REPEATING)
INPUT_OBSERVER_REGISTRATION(KeyUp, 0x40060066, OnKeyUp, EVENT_ID_KEYUP)
INPUT_OBSERVER_REGISTRATION(Char, 0x40060067, OnChar, EVENT_ID_CHAR)
INPUT_OBSERVER_REGISTRATION(MouseDown, 0x400500C8, OnMouseDown, EVENT_ID_MOUSEDOWN)
INPUT_OBSERVER_REGISTRATION(MouseUp, 0x400500C9, OnMouseUp, EVENT_ID_MOUSEUP)
INPUT_OBSERVER_REGISTRATION(MouseMove, 0x400500CA, OnMouseMove, EVENT_ID_MOUSEMOVE)
INPUT_OBSERVER_REGISTRATION(MouseWheel, 0x400500CD, OnMouseWheel, EVENT_ID_MOUSEWHEEL)
INPUT_OBSERVER_REGISTRATION(MouseMoveRelative, 0x400500CB, OnMouseMoveRelative, EVENT_ID_MOUSEMOVE_RELATIVE)
INPUT_OBSERVER_REGISTRATION(MouseModeChanged, 0x400500CC, OnMouseModeChanged, EVENT_ID_MOUSEMODE_CHANGED)
INPUT_OBSERVER_REGISTRATION(Ime, 0x40060068, OnIme, EVENT_ID_IME)
INPUT_OBSERVER_REGISTRATION(WindowSize, 0x40040064, OnWindowSize, EVENT_ID_SIZE)
INPUT_OBSERVER_REGISTRATION(WindowFocus, 0x40040065, OnWindowFocus, EVENT_ID_FOCUS)

#undef INPUT_OBSERVER_REGISTRATION

void CInputObserver::SetMouseMode(MOUSEMODE mode, unsigned int holdButton) {
  EventSetMouseMode(mode, holdButton);
}
