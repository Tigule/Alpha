#include <Base/Base.h>

#include "Os/W32/OsGui.h"
#include "InputMac.h"

#include <storm.h>

static void *s_gxWindow;

void *OsGuiGetWindow(int inWindowType) {
  return s_gxWindow;
}

void OsGuiSetGxWindow(void *window) {
  s_gxWindow = window;
}

void OsGuiSetWindowTitle(void *inWindow, const char *inText) {
  OsMacSetWindowTitle(inWindow, inText);
}

int OsGuiMessageBox(void *inParentWindow, int inStyle, const char *inMessage, const char *inTitle) {
  return OsMacMessageBox(inMessage, inTitle);
}
