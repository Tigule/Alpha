#include "EvtInt.h"

#include "Base/Coordinate.h"
#include "Os/OsTime.h"
#include "Os/W32/Input.h"

#include <Tempest/crect.h>

#include <string.h>

void IEvtInputSetMouseMode(EvtContext *context, MOUSEMODE mode, UINT holdButton);
static void PostMouseUp(EvtContext *context, int button, int x, int y, UINT flags, int time);
static void ResetAsyncState();

static UINT                     s_buttonState = 0;
static UINT                     s_metaKeyState = 0;
static UINT                     s_mouseHoldButton = 0;
static MOUSEMODE                s_mouseMode = MOUSE_MODE_NORMAL;
static EVENTCONFIRMCLOSEHANDLER s_confirmCloseCallback = 0;
static LPVOID                   s_confirmCloseParam = 0;
static NTempest::CRect          s_boundingRect(0.0f);

inline bool NTempest::CRect::Encloses(const NTempest::C2Vector &value) const {
  return value.x > l && value.x < r && value.y > t && value.y < b;
}

static void CheckMouseModeState() {
  if (s_mouseHoldButton && s_mouseHoldButton != (s_mouseHoldButton & s_buttonState)) {
    EventSetMouseMode(MOUSE_MODE_NORMAL, 0);
  }
}

static void UnconvertPosition(float x, float y, int *clientx, int *clienty) {
  RECT windowDim;

  OsGetDefaultWindowRect(&windowDim);

  *clientx = windowDim.left + static_cast<int>(static_cast<float>(windowDim.right - windowDim.left) * x);
  if (*clientx < windowDim.left) {
    *clientx = windowDim.left;
  }
  if (*clientx >= windowDim.right) {
    *clientx = windowDim.right - 1;
  }

  *clienty = windowDim.top + static_cast<int>(static_cast<float>(windowDim.bottom - windowDim.top) * y);
  if (*clienty < windowDim.top) {
    *clienty = windowDim.top;
  }
  if (*clienty >= windowDim.bottom) {
    *clienty = windowDim.bottom - 1;
  }

  *clienty = windowDim.bottom - *clienty - windowDim.top;
}

static void ConvertPosition(int clientx, int clienty, float *x, float *y) {
  if (s_boundingRect.r - s_boundingRect.l != 0.0f && s_boundingRect.b - s_boundingRect.t != 0.0f) {
    if (!s_boundingRect.Encloses(NTempest::C2Vector(static_cast<float>(clientx), static_cast<float>(clienty)))) {
      clientx = static_cast<int>(min(max(static_cast<float>(clientx), s_boundingRect.l + 1.0f), s_boundingRect.r - 1.0f));
      clienty = static_cast<int>(min(max(static_cast<float>(clienty), s_boundingRect.t + 1.0f), s_boundingRect.b - 1.0f));
      OsInputSetMousePosition(clientx, clienty);
    }
  }

  RECT windowDim;
  OsGetDefaultWindowRect(&windowDim);
  *x = static_cast<float>(clientx) / static_cast<float>(windowDim.right - windowDim.left);
  *y = 1.0f - static_cast<float>(clienty) / static_cast<float>(windowDim.bottom - windowDim.top);
}

static UINT GenerateMouseFlags() {
  UINT flags = 0;

  if (s_mouseMode == MOUSE_MODE_RELATIVE) {
    flags |= 0x2;
  }

  return flags;
}

static int ConfirmClose() {
  return s_confirmCloseCallback ? s_confirmCloseCallback(s_confirmCloseParam) : 1;
}

static void PostCaptureChanged(EvtContext *context, int x, int y) {
  while (s_buttonState) {
    UINT button = ((s_buttonState - 1) ^ s_buttonState) & s_buttonState;
    PostMouseUp(context, button, x, y, 0x1, OsGetAsyncTimeMs());
  }
}

static void PostChar(EvtContext *context, int ch, int repeat) {
  EVENT_DATA_CHAR data;
  data.ch = ch;
  data.metaKeyState = s_metaKeyState;
  data.repeat = repeat;
  IEvtQueueDispatch(context, EVENT_ID_CHAR, &data);
}

static void PostString(EvtContext *context, int str, int num_chars) {
  EVENT_DATA_CHAR data;
  data.metaKeyState = s_metaKeyState;
  data.repeat = 1;

  for (int index = 0; index < num_chars; ++index) {
    data.ch = reinterpret_cast<const WORD *>(str)[index];
    IEvtQueueDispatch(context, EVENT_ID_CHAR, &data);
  }
}

static void PostIme(EvtContext *context, int imeMessage, int wParam, int lParam) {
  EVENT_DATA_IME data;
  data.message = imeMessage;
  data.wParam = wParam;
  data.lParam = lParam;
  data.codepage = OsInputGetCodePage();
  IEvtQueueDispatch(context, EVENT_ID_IME, &data);
}

static void PostSize(EvtContext *context, int w, int h) {
  EVENT_DATA_SIZE data;
  data.w = w;
  data.h = h;
  IEvtQueueDispatch(context, EVENT_ID_SIZE, &data);
}

static void PostClose() {
  EventInitiateShutdown();
}

static void PostFocus(EvtContext *context, int focus) {
  EVENT_DATA_FOCUS data;
  ResetAsyncState();
  data.focus = focus;
  IEvtQueueDispatch(context, EVENT_ID_FOCUS, &data);
  CheckMouseModeState();
}

static void PostKeyDown(EvtContext *context, int key, int repeat, int time) {
  EVENT_DATA_KEY data;
  if (key <= KEY_LASTMETAKEY) {
    s_metaKeyState |= 1 << key;
  }
  data.key = static_cast<KEY>(key);
  data.metaKeyState = s_metaKeyState;
  data.repeat = repeat;
  data.time = time;
  IEvtQueueDispatch(context, EVENT_ID_KEYDOWN, &data);
}

static void PostKeyUp(EvtContext *context, int key, int repeat, int time) {
  EVENT_DATA_KEY data;
  if (key <= KEY_LASTMETAKEY) {
    s_metaKeyState &= ~(1 << key);
  }
  data.key = static_cast<KEY>(key);
  data.metaKeyState = s_metaKeyState;
  data.repeat = repeat;
  data.time = time;
  IEvtQueueDispatch(context, EVENT_ID_KEYUP, &data);
}

static void PostMouseDown(EvtContext *context, int button, int x, int y, int time) {
  EVENT_DATA_MOUSE data;
  data.button = static_cast<MOUSEBUTTON>(button);
  s_buttonState |= button;
  data.mode = s_mouseMode;
  data.buttonState = s_buttonState;
  data.metaKeyState = s_metaKeyState;
  data.flags = GenerateMouseFlags();
  ConvertPosition(x, y, &data.x, &data.y);
  data.time = time;
  IEvtQueueDispatch(context, EVENT_ID_MOUSEDOWN, &data);
}

static void PostMouseMove(EvtContext *context, int x, int y, int time) {
  EVENT_DATA_MOUSE data;
  data.mode = s_mouseMode;
  data.button = MOUSE_BUTTON_NONE;
  data.buttonState = s_buttonState;
  data.metaKeyState = s_metaKeyState;
  data.flags = GenerateMouseFlags();
  data.time = time;
  ConvertPosition(x, y, &data.x, &data.y);
  IEvtQueueDispatch(context, EVENT_ID_MOUSEMOVE, &data);
}

static void PostMouseMoveRelative(EvtContext *context, int x, int y, int time) {
  EVENT_DATA_MOUSE data;
  data.mode = s_mouseMode;
  data.button = MOUSE_BUTTON_NONE;
  data.buttonState = s_buttonState;
  data.metaKeyState = s_metaKeyState;
  data.flags = GenerateMouseFlags();
  data.x = static_cast<float>(x);
  data.y = static_cast<float>(y);
  data.time = time;
  IEvtQueueDispatch(context, EVENT_ID_MOUSEMOVE_RELATIVE, &data);
}

static void PostMouseUp(EvtContext *context, int button, int x, int y, UINT flags, int time) {
  EVENT_DATA_MOUSE data;
  s_buttonState &= ~static_cast<UINT>(button);
  data.mode = s_mouseMode;
  data.button = static_cast<MOUSEBUTTON>(button);
  data.buttonState = s_buttonState;
  data.metaKeyState = s_metaKeyState;
  data.flags = flags | GenerateMouseFlags();
  ConvertPosition(x, y, &data.x, &data.y);
  data.time = time;
  IEvtQueueDispatch(context, EVENT_ID_MOUSEUP, &data);
  CheckMouseModeState();
}

static void PostMouseModeChanged(EvtContext *context, MOUSEMODE mode) {
  EVENT_DATA_MOUSE data;
  memset(&data, 0, sizeof(data));
  s_mouseMode = mode;
  data.mode = mode;
  data.buttonState = s_buttonState;
  data.metaKeyState = s_metaKeyState;
  data.flags = GenerateMouseFlags();
  IEvtQueueDispatch(context, EVENT_ID_MOUSEMODE_CHANGED, &data);
}

static void PostMouseWheel(EvtContext *context, int distance, int x, int y, int time) {
  EVENT_DATA_MOUSE data;
  data.mode = s_mouseMode;
  data.button = MOUSE_BUTTON_NONE;
  data.buttonState = s_buttonState;
  data.metaKeyState = s_metaKeyState;
  data.wheelDistance = distance;
  data.flags = GenerateMouseFlags();
  ConvertPosition(x, y, &data.x, &data.y);
  data.time = time;
  IEvtQueueDispatch(context, EVENT_ID_MOUSEWHEEL, &data);
}

static void ProcessInput(EvtContext *context, OSINPUT id, const int param[4], int *shutdown) {
  VALIDATEBEGIN;
  VALIDATE(context);
  VALIDATEENDVOID;

  switch (id) {
    case OS_INPUT_CAPTURE_CHANGED:
      PostCaptureChanged(context, param[1], param[2]);
      break;

    case OS_INPUT_CHAR:
      PostChar(context, param[0], param[1]);
      break;

    case OS_INPUT_STRING:
      PostString(context, param[0], param[1]);
      break;

    case OS_INPUT_IME:
      PostIme(context, param[0], param[1], param[2]);
      break;

    case OS_INPUT_SIZE:
      PostSize(context, param[0], param[1]);
      break;

    case OS_INPUT_CLOSE:
      if (ConfirmClose()) {
        PostClose();
        *shutdown = 1;
      }
      break;

    case OS_INPUT_FOCUS:
      PostFocus(context, param[0]);
      break;

    case OS_INPUT_KEY_DOWN:
      PostKeyDown(context, param[0], param[1], param[3]);
      break;

    case OS_INPUT_KEY_UP:
      PostKeyUp(context, param[0], param[1], param[3]);
      break;

    case OS_INPUT_MOUSE_DOWN:
      PostMouseDown(context, param[0], param[1], param[2], param[3]);
      break;

    case OS_INPUT_MOUSE_MOVE:
      PostMouseMove(context, param[1], param[2], param[3]);
      break;

    case OS_INPUT_MOUSE_MOVE_RELATIVE:
      PostMouseMoveRelative(context, param[1], param[2], param[3]);
      break;

    case OS_INPUT_MOUSE_UP:
      PostMouseUp(context, param[0], param[1], param[2], 0, param[3]);
      break;

    case OS_INPUT_MOUSE_WHEEL:
      PostMouseWheel(context, param[0], param[1], param[2], param[3]);
      break;

    case OS_INPUT_SHUTDOWN:
      if (ConfirmClose()) {
        *shutdown = 1;
      }
      break;
  }
}

static void ResetAsyncState() {
  s_buttonState = 0;
  s_metaKeyState = 0;
}

void IEvtInputDestroy() {
  OsInputDestroy();
  ResetAsyncState();
}

void IEvtInputInitialize() {
  OsInputInitialize();
}

BOOL IEvtInputProcess(EvtContext *context, int *shutdown) {
  VALIDATEBEGIN;
  VALIDATE(context);
  VALIDATEEND;

  SErrPingWatchdog();

  int     param[4];
  OSINPUT id;
  int     processed = 0;
  while (OsInputGet(&id, &param[0], &param[1], &param[2], &param[3])) {
    processed = 1;
    ProcessInput(context, id, param, shutdown);
  }

  return processed;
}

void IEvtInputSetMouseMode(EvtContext *context, MOUSEMODE mode, UINT holdButton) {
  VALIDATEBEGIN;
  VALIDATE(context);
  VALIDATEENDVOID;

  if (holdButton == (holdButton & s_buttonState)) {
    OS_MOUSE_MODE osMode;

    switch (mode) {
      case MOUSE_MODE_RELATIVE:
        osMode = OS_MOUSE_MODE_RELATIVE;
        break;

      case MOUSE_MODE_NORMAL:
        osMode = OS_MOUSE_MODE_NORMAL;
        break;

      default:
        FATALERROR(("Invalid case: %s=%u", "mode", mode));
        __assume(0);
    }

    s_mouseHoldButton = holdButton;
    if (mode != s_mouseMode) {
      OsInputSetMouseMode(osMode);
      PostMouseModeChanged(context, mode);
    }
  }
}

void IEvtInputSetConfirmCloseCallback(EVENTCONFIRMCLOSEHANDLER inFunc, LPVOID inParam) {
  s_confirmCloseCallback = inFunc;
  s_confirmCloseParam = inParam;
}

void IEvtInputGetMousePosition(float *x, float *y) {
  float localX;
  float localY;
  int   clientX;
  int   clientY;

  ASSERT(x);
  ASSERT(y);

  OsInputGetMousePosition(&clientX, &clientY);
  ConvertPosition(clientX, clientY, &localX, &localY);
  NDCToDDC(localX, localY, x, y);
}

void IEvtInputSetMousePosition(float x, float y) {
  int   clientX;
  int   clientY;
  float globalX;
  float globalY;

  DDCToNDC(x, y, &globalX, &globalY);
  UnconvertPosition(globalX, globalY, &clientX, &clientY);
  OsInputSetMousePosition(clientX, clientY);
}

void IEvtInputSetMouseBoundingRect(NTempest::CRect *rect) {
  int   r;
  int   b;
  int   l;
  int   t;
  float gR;
  float gT;
  float gL;
  float gB;

  if (!rect) {
    s_boundingRect.t = 0.0f;
    s_boundingRect.l = 0.0f;
    s_boundingRect.b = 0.0f;
    s_boundingRect.r = 0.0f;
    return;
  }

  DDCToNDC(rect->l, rect->t, &gL, &gT);
  DDCToNDC(rect->r, rect->b, &gR, &gB);
  UnconvertPosition(gL, gB, &l, &t);
  UnconvertPosition(gR, gT, &r, &b);

  s_boundingRect.t = static_cast<float>(t);
  s_boundingRect.l = static_cast<float>(l);
  s_boundingRect.b = static_cast<float>(b);
  s_boundingRect.r = static_cast<float>(r);
}
