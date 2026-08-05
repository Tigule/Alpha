#include <Base/Base.h>

#include "Os/W32/Input.h"
#include "InputMac.h"

#include <storm.h>

#include <ApplicationServices/ApplicationServices.h>

struct OSEVENT {
  OSINPUT id;
  int     param[4];
};

static RECT          s_defaultwindowrect;
static int           s_screenIsWindow = 1;
static OSEVENT       s_queue[0x10];
static int           s_queueHead;
static int           s_queueTail;
static POINT         s_mousePos;
static OS_MOUSE_MODE s_mouseMode;

static void QueueEvent(OSINPUT id, int param0, int param1, int param2, int param3) {
  int next = (s_queueHead + 1) & 0xF;

  if (next == s_queueTail) {
    return;
  }

  s_queue[s_queueHead].id = id;
  s_queue[s_queueHead].param[0] = param0;
  s_queue[s_queueHead].param[1] = param1;
  s_queue[s_queueHead].param[2] = param2;
  s_queue[s_queueHead].param[3] = param3;

  s_queueHead = next;
}

void OsInputInitialize() {
  OsMacApplicationStart();

  s_queueHead = 0;
  s_queueTail = 0;
  s_mouseMode = OS_MOUSE_MODE_NORMAL;
}

void OsInputDestroy() {
}

int OsGetDefaultWindowRect(RECT *rect) {
  int width;
  int height;

  OsMacGetMainWindowRect(&width, &height);

  s_defaultwindowrect.left = 0;
  s_defaultwindowrect.top = 0;
  s_defaultwindowrect.right = width;
  s_defaultwindowrect.bottom = height;

  *rect = s_defaultwindowrect;
  return 1;
}

void OsInputNotifyScreenResize(int x, int y) {
  s_defaultwindowrect.right = x;
  s_defaultwindowrect.bottom = y;
}

void OsInputSetScreenIsWindow(int inVal) {
  s_screenIsWindow = inVal;
}

unsigned int OsInputGetCodePage() {
  return 0;
}

void OsInputSetMouseMode(OS_MOUSE_MODE mode) {
  if (s_mouseMode == mode) {
    return;
  }

  s_mouseMode = mode;

  CGAssociateMouseAndMouseCursorPosition(mode != OS_MOUSE_MODE_RELATIVE);
}

void OsInputGetMousePosition(int *x, int *y) {
  *x = s_mousePos.x;
  *y = s_mousePos.y;
}

void OsInputSetMousePosition(int x, int y) {
  ASSERT(s_mouseMode == OS_MOUSE_MODE_NORMAL);

  OsMacSetCursorPosition(x, y);

  s_mousePos.x = x;
  s_mousePos.y = y;
}

int OsInputGet(OSINPUT *id, int *param0, int *param1, int *param2, int *param3) {
  OSMACEVENT event;

  *id = static_cast<OSINPUT>(-1);

  while (s_queueHead == s_queueTail) {
    int width;
    int height;

    if (!OsMacPollEvent(&event)) {
      OsMacGetMainWindowRect(&width, &height);

      if (width != s_defaultwindowrect.right || height != s_defaultwindowrect.bottom) {
        s_defaultwindowrect.right = width;
        s_defaultwindowrect.bottom = height;
        QueueEvent(OS_INPUT_SIZE, width, height, 0, 0);
        continue;
      }

      return 0;
    }

    switch (event.type) {
      case OSMAC_EVENT_KEY_DOWN:
        QueueEvent(OS_INPUT_KEY_DOWN, event.param[0], 0, 0, 0);
        if (event.param[1]) {
          QueueEvent(OS_INPUT_CHAR, event.param[1], 0, 0, 0);
        }
        break;

      case OSMAC_EVENT_KEY_UP:
        QueueEvent(OS_INPUT_KEY_UP, event.param[0], 0, 0, 0);
        break;

      case OSMAC_EVENT_MOUSE_DOWN:
        s_mousePos.x = event.param[1];
        s_mousePos.y = s_defaultwindowrect.bottom - event.param[2];
        QueueEvent(OS_INPUT_MOUSE_DOWN, event.param[0], s_mousePos.x, s_mousePos.y, 0);
        break;

      case OSMAC_EVENT_MOUSE_UP:
        s_mousePos.x = event.param[1];
        s_mousePos.y = s_defaultwindowrect.bottom - event.param[2];
        QueueEvent(OS_INPUT_MOUSE_UP, event.param[0], s_mousePos.x, s_mousePos.y, 0);
        break;

      case OSMAC_EVENT_MOUSE_MOVE:
        s_mousePos.x = event.param[0];
        s_mousePos.y = s_defaultwindowrect.bottom - event.param[1];

        if (s_mouseMode == OS_MOUSE_MODE_RELATIVE) {
          QueueEvent(OS_INPUT_MOUSE_MOVE_RELATIVE, 0, event.param[2], event.param[3], 0);
        } else {
          QueueEvent(OS_INPUT_MOUSE_MOVE, 0, s_mousePos.x, s_mousePos.y, 0);
        }
        break;

      case OSMAC_EVENT_MOUSE_WHEEL:
        QueueEvent(OS_INPUT_MOUSE_WHEEL, event.param[0], s_mousePos.x, s_mousePos.y, 0);
        break;

      case OSMAC_EVENT_CLOSE:
        QueueEvent(OS_INPUT_CLOSE, 0, 0, 0, 0);
        break;
    }
  }

  *id = s_queue[s_queueTail].id;
  *param0 = s_queue[s_queueTail].param[0];
  *param1 = s_queue[s_queueTail].param[1];
  *param2 = s_queue[s_queueTail].param[2];
  *param3 = s_queue[s_queueTail].param[3];

  s_queueTail = (s_queueTail + 1) & 0xF;
  return 1;
}
