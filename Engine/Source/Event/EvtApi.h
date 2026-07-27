#pragma once

#include <storm.h>

namespace NTempest {
  class CRect;
}

DECLARE_STRICT_HANDLE(HEVENTCONTEXT);

enum EVENTID {
  EVENT_ID_CAPTURECHANGED = 0,
  EVENT_ID_CHAR = 1,
  EVENT_ID_FOCUS = 2,
  EVENT_ID_CLOSE = 3,
  EVENT_ID_DESTROY = 4,
  EVENT_ID_IDLE = 5,
  EVENT_ID_POLL = 6,
  EVENT_ID_INITIALIZE = 7,
  EVENT_ID_KEYDOWN = 8,
  EVENT_ID_KEYUP = 9,
  EVENT_ID_KEYDOWN_REPEATING = 10,
  EVENT_ID_MOUSEDOWN = 11,
  EVENT_ID_MOUSEMOVE = 12,
  EVENT_ID_MOUSEMOVE_RELATIVE = 13,
  EVENT_ID_MOUSEUP = 14,
  EVENT_ID_MOUSEMODE_CHANGED = 15,
  EVENT_ID_MOUSEWHEEL = 16,
  EVENT_ID_PAINT = 17,
  EVENT_ID_NET_DATA = 18,
  EVENT_ID_NET_CONNECT = 19,
  EVENT_ID_NET_DISCONNECT = 20,
  EVENT_ID_NET_CANTCONNECT = 21,
  EVENT_ID_NET_DESTROY = 22,
  EVENT_ID_CONSOLE_INPUT = 23,
  EVENT_ID_ENGINENET = 24,
  EVENT_ID_BATTLENET = 25,
  EVENT_ID_WOW_Q_IDLE = 26,
  EVENT_ID_IME = 27,
  EVENT_ID_SIZE = 28,
  EVENTIDS = 29
};

enum KEY {
  KEY_NONE = -1,
  KEY_SHIFT = 0x000,
  KEY_CONTROL = 0x001,
  KEY_ALT = 0x002,
  KEY_LASTMETAKEY = 0x002,
  KEY_0 = 0x030,
  KEY_1 = 0x031,
  KEY_2 = 0x032,
  KEY_3 = 0x033,
  KEY_4 = 0x034,
  KEY_5 = 0x035,
  KEY_6 = 0x036,
  KEY_7 = 0x037,
  KEY_8 = 0x038,
  KEY_9 = 0x039,
  KEY_A = 0x041,
  KEY_B = 0x042,
  KEY_C = 0x043,
  KEY_D = 0x044,
  KEY_E = 0x045,
  KEY_F = 0x046,
  KEY_G = 0x047,
  KEY_H = 0x048,
  KEY_I = 0x049,
  KEY_J = 0x04A,
  KEY_K = 0x04B,
  KEY_L = 0x04C,
  KEY_M = 0x04D,
  KEY_N = 0x04E,
  KEY_O = 0x04F,
  KEY_P = 0x050,
  KEY_Q = 0x051,
  KEY_R = 0x052,
  KEY_S = 0x053,
  KEY_T = 0x054,
  KEY_U = 0x055,
  KEY_V = 0x056,
  KEY_W = 0x057,
  KEY_X = 0x058,
  KEY_Y = 0x059,
  KEY_Z = 0x05A,
  KEY_SPACE = 0x020,
  KEY_TILDE = 0x100,
  KEY_NUMPAD0 = 0x101,
  KEY_NUMPAD1 = 0x102,
  KEY_NUMPAD2 = 0x103,
  KEY_NUMPAD3 = 0x104,
  KEY_NUMPAD4 = 0x105,
  KEY_NUMPAD5 = 0x106,
  KEY_NUMPAD6 = 0x107,
  KEY_NUMPAD7 = 0x108,
  KEY_NUMPAD8 = 0x109,
  KEY_NUMPAD9 = 0x10A,
  KEY_NUMPAD_PLUS = 0x10B,
  KEY_NUMPAD_MINUS = 0x10C,
  KEY_NUMPAD_MULTIPLY = 0x10D,
  KEY_NUMPAD_DIVIDE = 0x10E,
  KEY_NUMPAD_DECIMAL = 0x10F,
  KEY_PLUS = 0x110,
  KEY_MINUS = 0x111,
  KEY_BRACKET_OPEN = 0x112,
  KEY_BRACKET_CLOSE = 0x113,
  KEY_SLASH = 0x114,
  KEY_BACKSLASH = 0x115,
  KEY_SEMICOLON = 0x116,
  KEY_APOSTROPHE = 0x117,
  KEY_COMMA = 0x118,
  KEY_PERIOD = 0x119,
  KEY_ESCAPE = 0x200,
  KEY_ENTER = 0x201,
  KEY_BACKSPACE = 0x202,
  KEY_TAB = 0x203,
  KEY_LEFT = 0x204,
  KEY_UP = 0x205,
  KEY_RIGHT = 0x206,
  KEY_DOWN = 0x207,
  KEY_INSERT = 0x208,
  KEY_DELETE = 0x209,
  KEY_HOME = 0x20A,
  KEY_END = 0x20B,
  KEY_PAGEUP = 0x20C,
  KEY_PAGEDOWN = 0x20D,
  KEY_CAPSLOCK = 0x20E,
  KEY_NUMLOCK = 0x20F,
  KEY_SCROLLLOCK = 0x210,
  KEY_PAUSE = 0x211,
  KEY_PRINTSCREEN = 0x212,
  KEY_F1 = 0x300,
  KEY_F2 = 0x301,
  KEY_F3 = 0x302,
  KEY_F4 = 0x303,
  KEY_F5 = 0x304,
  KEY_F6 = 0x305,
  KEY_F7 = 0x306,
  KEY_F8 = 0x307,
  KEY_F9 = 0x308,
  KEY_F10 = 0x309,
  KEY_F11 = 0x30A,
  KEY_F12 = 0x30B,
  KEY_LAST = 0x30C
};

enum MOUSEBUTTON {
  MOUSE_BUTTON_NONE = 0x00,
  MOUSE_BUTTON_LEFT = 0x01,
  MOUSE_BUTTON_MIDDLE = 0x02,
  MOUSE_BUTTON_RIGHT = 0x04,
  MOUSE_BUTTON_XBUTTON1 = 0x08,
  MOUSE_BUTTON_XBUTTON2 = 0x10,
  MOUSE_BUTTON_ALL = -1
};

enum MOUSEMODE {
  MOUSE_MODE_NORMAL = 0,
  MOUSE_MODE_RELATIVE = 1,
  MOUSE_MODES = 2
};

enum OSINPUT {
  OS_INPUT_CAPTURE_CHANGED = 0,
  OS_INPUT_CHAR = 1,
  OS_INPUT_STRING = 2,
  OS_INPUT_IME = 3,
  OS_INPUT_SIZE = 4,
  OS_INPUT_CLOSE = 5,
  OS_INPUT_FOCUS = 6,
  OS_INPUT_KEY_DOWN = 7,
  OS_INPUT_KEY_UP = 8,
  OS_INPUT_MOUSE_DOWN = 9,
  OS_INPUT_MOUSE_MOVE = 10,
  OS_INPUT_MOUSE_WHEEL = 11,
  OS_INPUT_MOUSE_MOVE_RELATIVE = 12,
  OS_INPUT_MOUSE_UP = 13,
  OS_INPUT_SHUTDOWN = 14
};

enum OS_MOUSE_MODE {
  OS_MOUSE_MODE_NORMAL = 0,
  OS_MOUSE_MODE_RELATIVE = 1,
  OS_MOUSE_MODES = 2
};

typedef int(*EVENTHANDLER)(const void *data, void *param);
typedef int(*EVENTGUIDHANDLER)(const void *data, unsigned __int64 guid, void *param);
typedef void(*EVENTSCANHANDLER)(EVENTID id, const void *data, void *param);
typedef int(*EVENTCONFIRMCLOSEHANDLER)(void *param);

const float EVENT_PRIORITY_NORMAL = 0.0f;

struct EVENT_DATA_CHAR {
  int          ch;
  unsigned int metaKeyState;
  unsigned int repeat;
};

struct EVENT_DATA_FOCUS {
  int focus;
};

struct EVENT_DATA_IME {
  unsigned int message;
  unsigned int wParam;
  unsigned int lParam;
  unsigned int codepage;
};

struct EVENT_DATA_KEY {
  KEY          key;
  unsigned int metaKeyState;
  unsigned int repeat;
  unsigned int time;
};

struct EVENT_DATA_MOUSE {
  MOUSEMODE    mode;
  MOUSEBUTTON  button;
  unsigned int buttonState;
  unsigned int metaKeyState;
  unsigned int flags;
  float        x;
  float        y;
  int          wheelDistance;
  unsigned int time;
};

struct EVENT_DATA_SIZE {
  int w;
  int h;
};

struct EVENT_DATA_IDLE {
  float elapsedSec;
  DWORD time;
};

struct EVENT_DATA_TIMER {
  float elapsedSec;
  DWORD currTime;
};

void EventInitialize(unsigned int threadCount, int netServer);
void EventDestroy();
void EventDoMessageLoop();
void EventInitiateShutdown();

HEVENTCONTEXT
EventCreateContextEx(int interactive, EVENTHANDLER initializeHandler, EVENTHANDLER destroyHandler, DWORD idleTime, DWORD debugFlags);
void EventCreateContext(int interactive, EVENTHANDLER initializeHandler, EVENTHANDLER destroyHandler);
int EventIsContextInteractive();
HEVENTCONTEXT EventGetCurrentContext();
void EventSetContextIdleTime(DWORD idleTime, HEVENTCONTEXT hContext);
DWORD EventGetContextIdleTime(HEVENTCONTEXT hContext);
void EventPostClose();
void EventPostCloseEx(HEVENTCONTEXT hContext);
int EventIsButtonDown(MOUSEBUTTON button);
int EventIsKeyDown(KEY key);
void EventInputGetMousePosition(float *x, float *y);
void EventInputSetMousePosition(float x, float y);
int EventQueuePost(HEVENTCONTEXT hContext, EVENTID id, const void *data, unsigned int bytes);
int EventQueueScan(EVENTSCANHANDLER scanner, void *param);
void EventSetConfirmCloseCallback(EVENTCONFIRMCLOSEHANDLER inFunc, void *inParam);
int EventInputProcess(HEVENTCONTEXT hContext);
void EventSetMouseMode(MOUSEMODE mode, unsigned int holdButton);
void EventSetMouseBoundingRect(NTempest::CRect *rect);

unsigned int EventSetTimer(float timeout, EVENTHANDLER handler, void *param);
unsigned int EventSetTimer(float timeout, EVENTGUIDHANDLER handler, unsigned __int64 param, void *param2);
unsigned int EventSetTimer(unsigned int timeout, EVENTHANDLER handler, void *param);
unsigned int EventSetTimer(unsigned int timeout, EVENTGUIDHANDLER handler, unsigned __int64 param, void *param2);
unsigned int EventSetTimerAbsolute(DWORD triggerTime, EVENTHANDLER handler, void *param);
unsigned int EventSetTimerAbsolute(DWORD triggerTime, EVENTGUIDHANDLER handler, unsigned __int64 param, void *param2);
void EventKillTimer(unsigned int timerId, EVENTHANDLER handlerFunction, const char *functionName);
float EventGetRemainingTime(unsigned int timerId);

void EventRegister(EVENTID id, EVENTHANDLER handler);
void EventRegisterEx(EVENTID id, EVENTHANDLER handler, void *param, float priority);
void EventUnregister(EVENTID id, EVENTHANDLER handler);
void EventUnregisterEx(EVENTID id, EVENTHANDLER handler, void *param, unsigned int flags);
