#include <Base/Base.h>

#include "Os/W32/OsGui.h"
#include "InputMac.h"

#include <storm.h>

static LPVOID s_gxWindow;

LPVOID OsGuiGetWindow(int inWindowType) {
  return s_gxWindow;
}

void OsGuiSetGxWindow(LPVOID window) {
  s_gxWindow = window;
}

void OsGuiSetWindowTitle(LPVOID inWindow, LPCSTR inText) {
  OsMacSetWindowTitle(inWindow, inText);
}

int OsGuiMessageBox(LPVOID inParentWindow, int inStyle, LPCSTR inMessage, LPCSTR inTitle) {
  return OsMacMessageBox(inMessage, inTitle);
}
