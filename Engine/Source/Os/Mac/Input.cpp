#include <Base/Base.h>

#include "Os/W32/Input.h"
#include "Os/OsTime.h"
#include "InputMac.h"

#include <storm.h>

#include <ctype.h>

struct OSEVENT {
  OSINPUT id;
  int     param[4];
};

struct CommandEventHandler : public TSHashObject<CommandEventHandler, HASHKEY_NONE> {
  OSStatus (*m_handler)(OSMACEVENT *event, UInt32 command);
};

enum {
  kLeftMouseButtonIndex = 0,
  kRightMouseButtonIndex = 1
};

static int (*s_getDefaultWindowRectCB)(RECT *);
static bool                                           s_simulatedRightButtonClick;
static bool                                           s_buttonDown[5];
static Point                                          s_mousePos;
static int                                            s_queueHead;
static int                                            s_queueTail;
static OSEVENT                                        s_queue[0x10];
static OS_MOUSE_MODE                                  s_mouseMode;
static UInt32                                         s_modifiers;
static void                                          *s_windowHandler;
static void                                          *s_appHandler;
static TSHashTable<CommandEventHandler, HASHKEY_NONE> s_commandHandlers;
static int                                            s_AppIsActive = 1;

static const MOUSEBUTTON s_buttonConvert[6] = {
    MOUSE_BUTTON_NONE, MOUSE_BUTTON_LEFT, MOUSE_BUTTON_RIGHT, MOUSE_BUTTON_MIDDLE, MOUSE_BUTTON_XBUTTON1, MOUSE_BUTTON_XBUTTON2
};

static const KEY s_keyConvert[0x80] = {
    KEY_A,               KEY_S,        KEY_D,           KEY_F,          KEY_H,           KEY_G,               KEY_Z,               KEY_X,
    KEY_C,               KEY_V,        KEY_NONE,        KEY_B,          KEY_Q,           KEY_W,               KEY_E,               KEY_R,
    KEY_Y,               KEY_T,        KEY_1,           KEY_2,          KEY_3,           KEY_4,               KEY_6,               KEY_5,
    KEY_PLUS,            KEY_9,        KEY_7,           KEY_MINUS,      KEY_8,           KEY_0,               KEY_BRACKET_CLOSE,   KEY_O,
    KEY_U,               KEY_BRACKET_OPEN, KEY_I,       KEY_P,          KEY_ENTER,       KEY_L,               KEY_J,               KEY_APOSTROPHE,
    KEY_K,               KEY_SEMICOLON, KEY_BACKSLASH,  KEY_COMMA,      KEY_SLASH,       KEY_N,               KEY_M,               KEY_PERIOD,
    KEY_TAB,             KEY_SPACE,    KEY_TILDE,       KEY_BACKSPACE,  KEY_NONE,        KEY_ESCAPE,          KEY_NONE,            KEY_NONE,
    KEY_SHIFT,           KEY_CAPSLOCK, KEY_ALT,         KEY_CONTROL,    KEY_SHIFT,       KEY_ALT,             KEY_CONTROL,         KEY_NONE,
    KEY_NONE,            KEY_NUMPAD_DECIMAL, KEY_NONE,  KEY_NUMPAD_MULTIPLY, KEY_NONE,    KEY_NUMPAD_PLUS,     KEY_NONE,            KEY_NUMLOCK,
    KEY_NONE,            KEY_NONE,     KEY_NONE,        KEY_NUMPAD_DIVIDE, KEY_ENTER,    KEY_NONE,            KEY_NUMPAD_MINUS,    KEY_NONE,
    KEY_NONE,            KEY_NONE,     KEY_NUMPAD0,     KEY_NUMPAD1,    KEY_NUMPAD2,     KEY_NUMPAD3,         KEY_NUMPAD4,         KEY_NUMPAD5,
    KEY_NUMPAD6,         KEY_NUMPAD7,  KEY_NONE,        KEY_NUMPAD8,    KEY_NUMPAD9,     KEY_NONE,            KEY_NONE,            KEY_NONE,
    KEY_F5,              KEY_F6,       KEY_F7,          KEY_F3,         KEY_F8,          KEY_F9,              KEY_NONE,            KEY_F11,
    KEY_NONE,            KEY_PRINTSCREEN, KEY_NONE,     KEY_SCROLLLOCK, KEY_NONE,        KEY_F10,             KEY_NONE,            KEY_F12,
    KEY_NONE,            KEY_PAUSE,    KEY_INSERT,      KEY_HOME,       KEY_PAGEUP,      KEY_DELETE,          KEY_F4,              KEY_END,
    KEY_F2,              KEY_PAGEDOWN, KEY_F1,          KEY_LEFT,       KEY_RIGHT,       KEY_DOWN,            KEY_UP,              KEY_NONE
};

static const EventTypeSpec s_appWindowedEvents[] = {
    {kEventClassMouse, kEventMouseUp},
    {kEventClassMouse, kEventMouseMoved},
    {kEventClassMouse, kEventMouseDragged},
    {kEventClassCommand, kEventCommandProcess}
};

static const EventTypeSpec s_windowEvents[] = {
    {kEventClassMouse, kEventMouseDown},
    {kEventClassMouse, kEventMouseWheelMoved},
    {kEventClassKeyboard, kEventRawKeyDown},
    {kEventClassKeyboard, kEventRawKeyRepeat},
    {kEventClassKeyboard, kEventRawKeyUp},
    {kEventClassKeyboard, kEventRawKeyModifiersChanged},
    {kEventClassTextInput, kEventTextInputUnicodeForKeyEvent},
    {kEventClassWindow, kEventWindowActivated},
    {kEventClassWindow, kEventWindowDeactivated}
};

static const EventTypeSpec s_appFullscreenEvents[] = {
    {kEventClassMouse, kEventMouseUp},
    {kEventClassMouse, kEventMouseMoved},
    {kEventClassMouse, kEventMouseDragged},
    {kEventClassMouse, kEventMouseDown},
    {kEventClassMouse, kEventMouseWheelMoved},
    {kEventClassCommand, kEventCommandProcess},
    {kEventClassKeyboard, kEventRawKeyDown},
    {kEventClassKeyboard, kEventRawKeyRepeat},
    {kEventClassKeyboard, kEventRawKeyUp},
    {kEventClassKeyboard, kEventRawKeyModifiersChanged},
    {kEventClassTextInput, kEventTextInputUnicodeForKeyEvent}
};

LPVOID OsGuiGetWindow(int inWindowType);
int    OsSleepInBackground();
DWORD  OsGetBackgroundSleepMs();

static void OsQueuePut(OSINPUT id, int param0, int param1, int param2, int param3) {
  int nextHead;

  nextHead = s_queueHead == 0xF ? 0 : s_queueHead + 1;
  if (nextHead == s_queueTail) {
    s_queueTail = s_queueTail == 0xF ? 0 : s_queueTail + 1;
  }

  OSEVENT *event = &s_queue[s_queueHead];
  event->id = id;
  event->param[0] = param0;
  event->param[1] = param1;
  event->param[2] = param2;
  event->param[3] = param3;
  s_queueHead = nextHead;
}

static BOOL OsQueueGet(OSINPUT *id, int *param0, int *param1, int *param2, int *param3) {
  if (s_queueTail == s_queueHead) {
    return 0;
  }

  OSEVENT *event = &s_queue[s_queueTail];
  *id = event->id;
  *param0 = event->param[0];
  *param1 = event->param[1];
  *param2 = event->param[2];
  *param3 = event->param[3];

  if (s_queueTail == 0xF) {
    s_queueTail = 0;
  } else {
    ++s_queueTail;
  }

  return 1;
}

static void OsQueueSetParam(int index, int param) {
  int queueIndex = s_queueTail;

  while (queueIndex != s_queueHead) {
    s_queue[queueIndex].param[index] = param;
    if (queueIndex == 0xF) {
      queueIndex = 0;
    } else {
      ++queueIndex;
    }
  }
}

static OSStatus HICommandQuitHandler(OSMACEVENT *event, UInt32 command) {
  if (!(command == kHICommandQuit)) {
    SErrDisplayErrorFmt(
        STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
        isprint((command >> 24) & 0xFF) && isprint((command >> 16) & 0xFF) && isprint((command >> 8) & 0xFF) && isprint(command & 0xFF)
            ? "\"%s\", %s = %ld (0x%08X, '%c%c%c%c')"
            : "\"%s\", %s = %ld (0x%08X)",
        "command == kHICommandQuit", "command", (long)command, command, (command >> 24) & 0xFF, (command >> 16) & 0xFF, (command >> 8) & 0xFF,
        command & 0xFF
    );
  }

  OsQueuePut(OS_INPUT_CLOSE, 0, 0, 0, 0);
  return noErr;
}

static OSStatus s_CommandEventHandler(OSMACEVENT *event, UInt32 kind) {
  HICommand            command;
  CommandEventHandler *handler;

  OsMacGetEventParameter(event, kEventParamDirectObject, typeHICommand, 0, sizeof(command), 0, &command);

  handler = s_commandHandlers.Ptr(command.commandID, HASHKEY_NONE());
  if (!handler) {
    return eventNotHandledErr;
  }

  return handler->m_handler(event, command.commandID);
}

static OSStatus s_KeyboardEventHandler(OSMACEVENT *event, UInt32 kind) {
  UInt32 keyCode;
  UInt32 modifiers;
  UInt32 changed;
  KEY    key;

  switch (kind) {
    case kEventRawKeyDown:
    case kEventRawKeyRepeat:
    case kEventRawKeyUp:
      OsMacGetEventParameter(event, kEventParamKeyCode, typeUInt32, 0, sizeof(keyCode), 0, &keyCode);
      key = keyCode < 0x80 ? s_keyConvert[keyCode] : KEY_NONE;
      if (key != KEY_NONE) {
        OsQueuePut(kind == kEventRawKeyUp ? OS_INPUT_KEY_UP : OS_INPUT_KEY_DOWN, key, 0, 0, 0);
      }
      break;

    case kEventRawKeyModifiersChanged:
      OsMacGetEventParameter(event, kEventParamKeyModifiers, typeUInt32, 0, sizeof(modifiers), 0, &modifiers);
      changed = s_modifiers ^ modifiers;
      if (changed & shiftKey) {
        OsQueuePut(modifiers & shiftKey ? OS_INPUT_KEY_DOWN : OS_INPUT_KEY_UP, KEY_SHIFT, 0, 0, 0);
      }
      if (changed & controlKey) {
        OsQueuePut(modifiers & controlKey ? OS_INPUT_KEY_DOWN : OS_INPUT_KEY_UP, KEY_CONTROL, 0, 0, 0);
      }
      if (changed & optionKey) {
        OsQueuePut(modifiers & optionKey ? OS_INPUT_KEY_DOWN : OS_INPUT_KEY_UP, KEY_ALT, 0, 0, 0);
      }
      s_modifiers = modifiers;
      break;
  }

  return eventNotHandledErr;
}

static OSStatus s_TextInputEventHandler(OSMACEVENT *event, UInt32 kind) {
  UniChar   textInputScratch[0x100];
  ByteCount actualSize;
  OSStatus  err;
  UInt32    i;

  if (kind == kEventTextInputUnicodeForKeyEvent) {
    err = OsMacGetEventParameter(
        event, kEventParamTextInputSendText, typeUnicodeText, 0, sizeof(textInputScratch), &actualSize, textInputScratch
    );

    if (!(err == noErr)) {
      SErrDisplayErrorFmt(
          STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
          isprint((err >> 24) & 0xFF) && isprint((err >> 16) & 0xFF) && isprint((err >> 8) & 0xFF) && isprint(err & 0xFF)
              ? "\"%s\", %s = %ld (0x%08X, '%c%c%c%c')"
              : "\"%s\", %s = %ld (0x%08X)",
          "err == noErr", "err", (long)err, err, (err >> 24) & 0xFF, (err >> 16) & 0xFF, (err >> 8) & 0xFF, err & 0xFF
      );
    }

    if (!(actualSize <= sizeof(textInputScratch))) {
      SErrDisplayErrorFmt(
          STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
          isprint((actualSize >> 24) & 0xFF) && isprint((actualSize >> 16) & 0xFF) && isprint((actualSize >> 8) & 0xFF)
                  && isprint(actualSize & 0xFF)
              ? "\"%s\", %s = %ld (0x%08X, '%c%c%c%c')"
              : "\"%s\", %s = %ld (0x%08X)",
          "actualSize <= sizeof(textInputScratch)", "actualSize", (long)actualSize, (UINT)actualSize, (UINT)(actualSize >> 24) & 0xFF,
          (UINT)(actualSize >> 16) & 0xFF, (UINT)(actualSize >> 8) & 0xFF, (UINT)actualSize & 0xFF
      );
    }

    if (err == noErr && actualSize <= sizeof(textInputScratch)) {
      for (i = 0; i < actualSize / sizeof(UniChar); ++i) {
        if (textInputScratch[i] >= 0x20) {
          OsQueuePut(OS_INPUT_CHAR, textInputScratch[i], 1, 0, 0);
        }
      }
    }
  }

  return eventNotHandledErr;
}

static OSStatus s_MouseEventHandler(OSMACEVENT *event, UInt32 kind) {
  OSStatus         result = eventNotHandledErr;
  HIPoint          location;
  LPVOID           window;
  UInt32           modifiers;
  EventMouseButton button;
  MOUSEBUTTON      mouseButton;
  int              post;
  SInt32           wheelDelta;
  HIPoint          delta;

  OsMacGetEventParameter(event, kEventParamMouseLocation, typeHIPoint, 0, sizeof(location), 0, &location);

  window = OsGuiGetWindow(0);
  if (window) {
    OsMacGlobalToLocalPoint(window, &location);
  }

  s_mousePos.h = (short)location.x;
  s_mousePos.v = (short)location.y;

  switch (kind) {
    case kEventMouseDown:
    case kEventMouseUp:
      OsMacGetEventParameter(event, kEventParamKeyModifiers, typeUInt32, 0, sizeof(modifiers), 0, &modifiers);
      OsMacGetEventParameter(event, kEventParamMouseButton, typeMouseButton, 0, sizeof(button), 0, &button);
      post = 1;

      if (button > 5 || s_buttonDown[button - 1] == (kind == kEventMouseDown)) {
        mouseButton = MOUSE_BUTTON_NONE;
      } else {
        s_buttonDown[button - 1] = kind == kEventMouseDown;

        if (button == kEventMouseButtonPrimary) {
          if (kind == kEventMouseDown && (modifiers & cmdKey)) {
            if (s_buttonDown[kRightMouseButtonIndex]) {
              post = 0;
            }
            s_simulatedRightButtonClick = true;
            button = kEventMouseButtonSecondary;
          } else if (kind != kEventMouseDown && s_simulatedRightButtonClick) {
            if (s_buttonDown[kRightMouseButtonIndex]) {
              post = 0;
            }
            s_simulatedRightButtonClick = false;
            button = kEventMouseButtonSecondary;
          }
        } else if (button == kEventMouseButtonSecondary) {
          ASSERT(! (s_simulatedRightButtonClick && ! s_buttonDown[ kLeftMouseButtonIndex ]));
          if (s_simulatedRightButtonClick) {
            post = 0;
          }
        }

        mouseButton = post && button < 6 ? s_buttonConvert[button] : MOUSE_BUTTON_NONE;
      }

      if (mouseButton) {
        OsQueuePut(kind == kEventMouseDown ? OS_INPUT_MOUSE_DOWN : OS_INPUT_MOUSE_UP, mouseButton, s_mousePos.h, s_mousePos.v, 0);
      }
      break;

    case kEventMouseWheelMoved:
      OsMacGetEventParameter(event, kEventParamMouseWheelDelta, typeSInt32, 0, sizeof(wheelDelta), 0, &wheelDelta);
      OsQueuePut(OS_INPUT_MOUSE_WHEEL, wheelDelta * 120, s_mousePos.h, s_mousePos.v, 0);
      result = noErr;
      break;

    case kEventMouseMoved:
    case kEventMouseDragged:
      if (s_mouseMode == OS_MOUSE_MODE_RELATIVE) {
        if (OsMacGetEventParameter(event, kEventParamMouseDelta, typeHIPoint, 0, sizeof(delta), 0, &delta) == noErr) {
          OsQueuePut(OS_INPUT_MOUSE_MOVE_RELATIVE, 0, (int)delta.x, (int)delta.y, 0);
        }
      } else {
        OsQueuePut(OS_INPUT_MOUSE_MOVE, 0, s_mousePos.h, s_mousePos.v, 0);
      }
      break;
  }

  return result;
}

static OSStatus s_AppEventHandler(OSMACEVENT *event, void *userData) {
  OSStatus result = eventNotHandledErr;

  switch (event->eventClass) {
    case kEventClassCommand:
      result = s_CommandEventHandler(event, event->eventKind);
      break;

    case kEventClassMouse:
      result = s_MouseEventHandler(event, event->eventKind);
      break;

    case kEventClassKeyboard:
      result = s_KeyboardEventHandler(event, event->eventKind);
      break;

    case kEventClassTextInput:
      result = s_TextInputEventHandler(event, event->eventKind);
      break;

    case kEventClassApplication:
      switch (event->eventKind) {
        case kEventAppActivated:
          s_AppIsActive = 1;
          break;

        case kEventAppDeactivated:
          s_AppIsActive = 0;
          break;
      }
      result = eventNotHandledErr;
      break;

    case kEventClassWindow:
      if (event->eventKind == kEventWindowDeactivated && s_mouseMode != OS_MOUSE_MODE_NORMAL) {
        CGAssociateMouseAndMouseCursorPosition(true);
        s_mouseMode = OS_MOUSE_MODE_NORMAL;
        OsQueuePut(OS_INPUT_CAPTURE_CHANGED, 0, 0, 0, 0);
      }
      result = eventNotHandledErr;
      break;
  }

  return result;
}

BOOL OsInputGet(OSINPUT *id, int *param0, int *param1, int *param2, int *param3) {
  EventTimeout timeout;
  OSStatus     result;

  if (OsQueueGet(id, param0, param1, param2, param3)) {
    return 1;
  }

  timeout = kEventDurationNoWait;
  if (!s_AppIsActive && OsSleepInBackground()) {
    timeout = OsGetBackgroundSleepMs() * kEventDurationMillisecond;
  }

  result = OsMacReceiveNextEvent(timeout);
  if (result != noErr && !(result == eventLoopTimedOutErr || result == eventLoopQuitErr)) {
    SErrDisplayErrorFmt(
        STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
        isprint((result >> 24) & 0xFF) && isprint((result >> 16) & 0xFF) && isprint((result >> 8) & 0xFF) && isprint(result & 0xFF)
            ? "\"%s\", %s = %ld (0x%08X, '%c%c%c%c')"
            : "\"%s\", %s = %ld (0x%08X)",
        "result == eventLoopTimedOutErr || result == eventLoopQuitErr", "result", (long)result, result, (result >> 24) & 0xFF,
        (result >> 16) & 0xFF, (result >> 8) & 0xFF, result & 0xFF
    );
  }

  if (s_queueTail == s_queueHead) {
    return 0;
  }

  OsQueueSetParam(3, OsGetAsyncTimeMsPrecise());
  return OsQueueGet(id, param0, param1, param2, param3);
}

void OsInputPostEvent(OSINPUT id, int param0, int param1, int param2, int param3) {
  OsQueuePut(id, param0, param1, param2, param3);
}

void OsInputSetMouseMode(OS_MOUSE_MODE mode) {
  VALIDATE2VOID(mode < OS_MOUSE_MODES);

  if (mode == s_mouseMode) {
    return;
  }

  CGAssociateMouseAndMouseCursorPosition(mode == OS_MOUSE_MODE_NORMAL);
  s_mouseMode = mode;
}

void OsInputGetMousePosition(int *x, int *y) {
  *x = s_mousePos.h;
  *y = s_mousePos.v;
}

void OsInputSetMousePosition(int x, int y) {
  HIPoint point;

  ASSERT(s_mouseMode == OS_MOUSE_MODE_NORMAL);

  point.x = x;
  point.y = y;
  OsMacLocalToGlobalPoint(OsGuiGetWindow(0), &point);

  CGWarpMouseCursorPosition(point);
}

void SetOsGetDefaultWindowRectCB(int (*callback)(RECT *)) {
  s_getDefaultWindowRectCB = callback;
}

BOOL OsGetDefaultWindowRect(RECT *rect) {
  if (s_getDefaultWindowRectCB) {
    return s_getDefaultWindowRectCB(rect);
  }

  return 0;
}

UINT OsInputGetCodePage() {
  return 0;
}

void OsInputInitialize() {
  OsInputRegisterHICommandHandler(kHICommandQuit, HICommandQuitHandler);
}

void OsInputInstallGameWindowHandlers(void *window) {
  const EventTypeSpec *appEvents;
  UInt32               appEventCount;
  const EventTypeSpec *windowEvents;
  UInt32               windowEventCount;
  OSStatus             err;

  ASSERT(!s_appHandler);
  ASSERT(!s_windowHandler);

  if (window) {
    appEvents = s_appWindowedEvents;
    appEventCount = sizeof(s_appWindowedEvents) / sizeof(s_appWindowedEvents[0]);
    windowEvents = s_windowEvents;
    windowEventCount = sizeof(s_windowEvents) / sizeof(s_windowEvents[0]);
  } else {
    appEvents = s_appFullscreenEvents;
    appEventCount = sizeof(s_appFullscreenEvents) / sizeof(s_appFullscreenEvents[0]);
    windowEvents = 0;
    windowEventCount = 0;
  }

  if (appEvents && appEventCount) {
    err = OsMacInstallEventHandler(0, s_AppEventHandler, appEventCount, appEvents, 0, &s_appHandler);
    ASSERT(s_appHandler);
    if (!(err == noErr)) {
      SErrDisplayErrorFmt(
          STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
          isprint((err >> 24) & 0xFF) && isprint((err >> 16) & 0xFF) && isprint((err >> 8) & 0xFF) && isprint(err & 0xFF)
              ? "\"%s\", %s = %ld (0x%08X, '%c%c%c%c')"
              : "\"%s\", %s = %ld (0x%08X)",
          "err == noErr", "err", (long)err, err, (err >> 24) & 0xFF, (err >> 16) & 0xFF, (err >> 8) & 0xFF, err & 0xFF
      );
    }
  }

  if (windowEventCount) {
    err = OsMacInstallEventHandler(window, s_AppEventHandler, windowEventCount, windowEvents, 0, &s_windowHandler);
    ASSERT(s_windowHandler);
    if (!(err == noErr)) {
      SErrDisplayErrorFmt(
          STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
          isprint((err >> 24) & 0xFF) && isprint((err >> 16) & 0xFF) && isprint((err >> 8) & 0xFF) && isprint(err & 0xFF)
              ? "\"%s\", %s = %ld (0x%08X, '%c%c%c%c')"
              : "\"%s\", %s = %ld (0x%08X)",
          "err == noErr", "err", (long)err, err, (err >> 24) & 0xFF, (err >> 16) & 0xFF, (err >> 8) & 0xFF, err & 0xFF
      );
    }
  }
}

void OsInputRemoveGameWindowHandlers(void *window) {
  if (s_appHandler) {
    OsMacRemoveEventHandler(s_appHandler);
    s_appHandler = 0;
  }

  if (s_windowHandler) {
    OsMacRemoveEventHandler(s_windowHandler);
    s_windowHandler = 0;
  }
}

void OsInputRegisterHICommandHandler(UInt32 command, OSStatus (*handler)(OSMACEVENT *event, UInt32 command)) {
  CommandEventHandler *commandHandler = s_commandHandlers.New(command, HASHKEY_NONE(), 0, 0);
  commandHandler->m_handler = handler;
}

void OsInputDestroy() {
}

LRESULT OsWindowProc(LPVOID window, UINT message, WPARAM wparam, LPARAM lparam) {
  ASSERT(!"What WindowProc !?!");
  return 0;
}
