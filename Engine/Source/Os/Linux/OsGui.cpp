#include <Base/Base.h>

#include "Os/W32/OsGui.h"

#include <storm.h>

#include <stdio.h>

#include <SDL.h>

void OsGuiSetWindowTitle(LPVOID inWindow, LPCSTR inText) {
  SDL_SetWindowTitle(SDL_GL_GetCurrentWindow(), inText);
}

void OsGuiSetGxWindow(LPVOID window) {
}

LPVOID OsGuiGetWindow(int inWindowType) {
  return 0;
}

int OsGuiMessageBox(LPVOID inParentWindow, int inStyle, LPCSTR inMessage, LPCSTR inTitle) {
  fprintf(stderr, "%s: %s\n", inTitle, inMessage);
  return 0;
}
