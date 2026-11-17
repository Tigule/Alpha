#include <Base/Base.h>

#include "Os/W32/Input.h"
#include "Os/OsTime.h"
#include "Gx/Gx.h"

#include <storm.h>

#include <SDL.h>

static OS_MOUSE_MODE s_mouseMode;
static char          s_textInput[SDL_TEXTINPUTEVENT_TEXT_SIZE];
static int           s_textInputPos;
static int           s_savedMouseX;
static int           s_savedMouseY;

static BOOL ConvertKey(SDL_Keycode sym, KEY *key) {
  switch (sym) {
    case SDLK_BACKSPACE:
      *key = KEY_BACKSPACE;
      return 1;
    case SDLK_TAB:
      *key = KEY_TAB;
      return 1;
    case SDLK_RETURN:
      *key = KEY_ENTER;
      return 1;
    case SDLK_PAUSE:
      *key = KEY_PAUSE;
      return 1;
    case SDLK_ESCAPE:
      *key = KEY_ESCAPE;
      return 1;
    case SDLK_SPACE:
      *key = KEY_SPACE;
      return 1;
    case SDLK_PLUS:
      *key = KEY_PLUS;
      return 1;
    case SDLK_MINUS:
      *key = KEY_MINUS;
      return 1;
    case SDLK_SLASH:
      *key = KEY_SLASH;
      return 1;
    case SDLK_LEFTBRACKET:
      *key = KEY_BRACKET_OPEN;
      return 1;
    case SDLK_RIGHTBRACKET:
      *key = KEY_BRACKET_CLOSE;
      return 1;
    case SDLK_BACKQUOTE:
      *key = KEY_TILDE;
      return 1;
    case SDLK_DELETE:
      *key = KEY_DELETE;
      return 1;
    case SDLK_KP_0:
      *key = KEY_NUMPAD0;
      return 1;
    case SDLK_KP_1:
      *key = KEY_NUMPAD1;
      return 1;
    case SDLK_KP_2:
      *key = KEY_NUMPAD2;
      return 1;
    case SDLK_KP_3:
      *key = KEY_NUMPAD3;
      return 1;
    case SDLK_KP_4:
      *key = KEY_NUMPAD4;
      return 1;
    case SDLK_KP_5:
      *key = KEY_NUMPAD5;
      return 1;
    case SDLK_KP_6:
      *key = KEY_NUMPAD6;
      return 1;
    case SDLK_KP_7:
      *key = KEY_NUMPAD7;
      return 1;
    case SDLK_KP_8:
      *key = KEY_NUMPAD8;
      return 1;
    case SDLK_KP_9:
      *key = KEY_NUMPAD9;
      return 1;
    case SDLK_KP_DIVIDE:
      *key = KEY_NUMPAD_DIVIDE;
      return 1;
    case SDLK_KP_MULTIPLY:
      *key = KEY_NUMPAD_MULTIPLY;
      return 1;
    case SDLK_KP_MINUS:
      *key = KEY_NUMPAD_MINUS;
      return 1;
    case SDLK_KP_PLUS:
      *key = KEY_NUMPAD_PLUS;
      return 1;
    case SDLK_UP:
      *key = KEY_UP;
      return 1;
    case SDLK_DOWN:
      *key = KEY_DOWN;
      return 1;
    case SDLK_RIGHT:
      *key = KEY_RIGHT;
      return 1;
    case SDLK_LEFT:
      *key = KEY_LEFT;
      return 1;
    case SDLK_INSERT:
      *key = KEY_INSERT;
      return 1;
    case SDLK_HOME:
      *key = KEY_HOME;
      return 1;
    case SDLK_END:
      *key = KEY_END;
      return 1;
    case SDLK_PAGEUP:
      *key = KEY_PAGEUP;
      return 1;
    case SDLK_PAGEDOWN:
      *key = KEY_PAGEDOWN;
      return 1;
    case SDLK_NUMLOCKCLEAR:
      *key = KEY_NUMLOCK;
      return 1;
    case SDLK_CAPSLOCK:
      *key = KEY_CAPSLOCK;
      return 1;
    case SDLK_SCROLLLOCK:
      *key = KEY_SCROLLLOCK;
      return 1;
    case SDLK_RSHIFT:
    case SDLK_LSHIFT:
      *key = KEY_SHIFT;
      return 1;
    case SDLK_RCTRL:
    case SDLK_LCTRL:
      *key = KEY_CONTROL;
      return 1;
    case SDLK_RALT:
    case SDLK_LALT:
      *key = KEY_ALT;
      return 1;
    case SDLK_PRINTSCREEN:
      *key = KEY_PRINTSCREEN;
      return 1;
  }

  if (sym >= SDLK_0 && sym <= SDLK_9) {
    *key = (KEY)sym;
    return 1;
  }
  if (sym >= SDLK_a && sym <= SDLK_z) {
    *key = (KEY)(sym - 0x20);
    return 1;
  }
  if (sym >= SDLK_F1 && sym <= SDLK_F12) {
    *key = (KEY)(KEY_F1 + (sym - SDLK_F1));
    return 1;
  }

  *key = (KEY)0;
  return 0;
}

static BOOL ConvertButton(Uint8 sdlButton, MOUSEBUTTON *button) {
  switch (sdlButton) {
    case SDL_BUTTON_LEFT:
      *button = MOUSE_BUTTON_LEFT;
      return 1;
    case SDL_BUTTON_MIDDLE:
      *button = MOUSE_BUTTON_MIDDLE;
      return 1;
    case SDL_BUTTON_RIGHT:
      *button = MOUSE_BUTTON_RIGHT;
      return 1;
    case SDL_BUTTON_X1:
      *button = MOUSE_BUTTON_XBUTTON1;
      return 1;
    case SDL_BUTTON_X2:
      *button = MOUSE_BUTTON_XBUTTON2;
      return 1;
  }

  *button = MOUSE_BUTTON_NONE;
  return 0;
}

BOOL OsInputGet(OSINPUT *id, int *param0, int *param1, int *param2, int *param3) {
  SDL_Event   event;
  KEY         key;
  MOUSEBUTTON button;
  int         chars;
  UINT        ch;
  SDL_Window *window;

  *param0 = 0;
  *id = (OSINPUT)-1;
  *param1 = 0;
  *param2 = 0;
  *param3 = OsGetAsyncTimeMsPrecise();

  for (;;) {
    if (s_textInput[s_textInputPos]) {
      ch = SUniSGetUTF8((const BYTE *)&s_textInput[s_textInputPos], &chars);
      s_textInputPos += chars;
      if (ch > 0x1F && ch <= 0xFFFF) {
        *id = OS_INPUT_CHAR;
        *param0 = ch;
        return 1;
      }
      continue;
    }

    if (!SDL_PollEvent(&event)) {
      return 0;
    }

    switch (event.type) {
      case SDL_KEYDOWN:
      case SDL_KEYUP:
        if (event.type == SDL_KEYDOWN && (event.key.keysym.mod & KMOD_ALT)) {
          window = SDL_GL_GetCurrentWindow();
          if (event.key.keysym.sym == SDLK_RETURN) {
            SDL_SetWindowFullscreen(window, (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) ? 0 : SDL_WINDOW_FULLSCREEN);
            break;
          }
          if (event.key.keysym.sym == SDLK_TAB) {
            SDL_MinimizeWindow(window);
            break;
          }
          if (event.key.keysym.sym == SDLK_F4) {
            *id = OS_INPUT_CLOSE;
            break;
          }
        }
        if (ConvertKey(event.key.keysym.sym, &key)) {
          *id = event.type == SDL_KEYDOWN ? OS_INPUT_KEY_DOWN : OS_INPUT_KEY_UP;
          *param0 = key;
          *param1 = 0;
        }
        break;

      case SDL_TEXTINPUT:
        SStrCopy(s_textInput, event.text.text, sizeof(s_textInput));
        s_textInputPos = 0;
        break;

      case SDL_MOUSEMOTION:
        switch (s_mouseMode) {
          case OS_MOUSE_MODE_NORMAL:
            *id = OS_INPUT_MOUSE_MOVE;
            *param1 = event.motion.x;
            *param2 = event.motion.y;
            break;
          case OS_MOUSE_MODE_RELATIVE:
            *id = OS_INPUT_MOUSE_MOVE_RELATIVE;
            *param1 = event.motion.xrel;
            *param2 = event.motion.yrel;
            break;
          default:
            FATALERROR(("Invalid case: %s=%u", "s_mouseMode", s_mouseMode));
            break;
        }
        break;

      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
        if (ConvertButton(event.button.button, &button)) {
          *id = event.type == SDL_MOUSEBUTTONDOWN ? OS_INPUT_MOUSE_DOWN : OS_INPUT_MOUSE_UP;
          *param0 = button;
          *param1 = event.button.x;
          *param2 = event.button.y;
        }
        break;

      case SDL_MOUSEWHEEL:
        if (event.wheel.y) {
          *id = OS_INPUT_MOUSE_WHEEL;
          *param0 = event.wheel.y > 0 ? 1 : -1;
          SDL_GetMouseState(param1, param2);
        }
        break;

      case SDL_QUIT:
        *id = OS_INPUT_CLOSE;
        break;

      case SDL_WINDOWEVENT:
        if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
          NTempest::CRect rect(0.0f, 0.0f, (float)event.window.data2, (float)event.window.data1);
          GxDevWM(GxWM_Size, (LONG_PTR)&rect, 0);
          *id = OS_INPUT_SIZE;
          *param0 = event.window.data1;
          *param1 = event.window.data2;
        }
        break;
    }

    if (*id != (OSINPUT)-1) {
      return 1;
    }
  }
}

void OsInputSetMouseMode(OS_MOUSE_MODE mode) {
  VALIDATE2VOID(mode < OS_MOUSE_MODES);

  if (mode == s_mouseMode) {
    return;
  }

  if (mode == OS_MOUSE_MODE_RELATIVE) {
    SDL_GetMouseState(&s_savedMouseX, &s_savedMouseY);
    s_mouseMode = OS_MOUSE_MODE_RELATIVE;
    SDL_SetRelativeMouseMode(SDL_TRUE);
  } else if (s_mouseMode == OS_MOUSE_MODE_RELATIVE) {
    s_mouseMode = OS_MOUSE_MODE_NORMAL;
    SDL_SetRelativeMouseMode(SDL_FALSE);
    SDL_WarpMouseInWindow(SDL_GL_GetCurrentWindow(), s_savedMouseX, s_savedMouseY);
  }
}

void OsInputGetMousePosition(int *x, int *y) {
  SDL_GetMouseState(x, y);
}

void OsInputSetMousePosition(int x, int y) {
  ASSERT(s_mouseMode == OS_MOUSE_MODE_NORMAL);
  SDL_WarpMouseInWindow(SDL_GL_GetCurrentWindow(), x, y);
}

BOOL OsGetDefaultWindowRect(RECT *rect) {
  SDL_Window *window;

  VALIDATE2(rect);

  window = SDL_GL_GetCurrentWindow();
  if (!window) {
    return 0;
  }

  SDL_GetWindowSize(window, (int *)&rect->right, (int *)&rect->bottom);
  rect->top = 0;
  rect->left = 0;
  return 1;
}

UINT OsInputGetCodePage() {
  return 0;
}

void OsInputInitialize() {
  SDL_ShowCursor(SDL_DISABLE);
  SDL_StartTextInput();
}

void OsInputDestroy() {
  SDL_ShowCursor(SDL_ENABLE);
  SDL_StopTextInput();
}

LRESULT OsWindowProc(LPVOID window, UINT message, WPARAM wparam, LPARAM lparam) {
  return 0;
}
