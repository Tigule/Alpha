#ifndef ENGINE_SOURCE_OS_MAC_INPUTMAC_H
#define ENGINE_SOURCE_OS_MAC_INPUTMAC_H

enum {
  OSMAC_EVENT_NONE = 0,
  OSMAC_EVENT_KEY_DOWN = 1,
  OSMAC_EVENT_KEY_UP = 2,
  OSMAC_EVENT_MOUSE_DOWN = 3,
  OSMAC_EVENT_MOUSE_UP = 4,
  OSMAC_EVENT_MOUSE_MOVE = 5,
  OSMAC_EVENT_MOUSE_WHEEL = 6,
  OSMAC_EVENT_CLOSE = 7
};

// mirrors the engine's KEY enum so the pump can translate without storm.h
enum {
  OSMAC_KEY_0 = 0x030,
  OSMAC_KEY_A = 0x041,
  OSMAC_KEY_PLUS = 0x110,
  OSMAC_KEY_MINUS = 0x111,
  OSMAC_KEY_COMMA = 0x118,
  OSMAC_KEY_PERIOD = 0x119,
  OSMAC_KEY_ESCAPE = 0x200,
  OSMAC_KEY_ENTER = 0x201,
  OSMAC_KEY_BACKSPACE = 0x202,
  OSMAC_KEY_TAB = 0x203,
  OSMAC_KEY_LEFT = 0x204,
  OSMAC_KEY_UP = 0x205,
  OSMAC_KEY_RIGHT = 0x206,
  OSMAC_KEY_DOWN = 0x207,
  OSMAC_KEY_INSERT = 0x208,
  OSMAC_KEY_DELETE = 0x209,
  OSMAC_KEY_HOME = 0x20A,
  OSMAC_KEY_END = 0x20B,
  OSMAC_KEY_PAGEUP = 0x20C,
  OSMAC_KEY_PAGEDOWN = 0x20D,
  OSMAC_KEY_CAPSLOCK = 0x20E,
  OSMAC_KEY_F1 = 0x300
};

// mirrors the engine's MOUSEBUTTON mask
enum {
  OSMAC_MOUSE_LEFT = 0x01,
  OSMAC_MOUSE_MIDDLE = 0x02,
  OSMAC_MOUSE_RIGHT = 0x04,
  OSMAC_MOUSE_XBUTTON1 = 0x08,
  OSMAC_MOUSE_XBUTTON2 = 0x10
};

struct OSMACEVENT {
  int type;
  int param[4];
};

void OsMacApplicationStart();
int  OsMacPollEvent(OSMACEVENT *event);
void OsMacGetMainWindowRect(int *width, int *height);
int  OsMacIsWindowActive();
void OsMacSetCursorPosition(int x, int y);
int  OsMacMessageBox(const char *message, const char *title);
void OsMacSetWindowTitle(void *window, const char *text);

#endif
