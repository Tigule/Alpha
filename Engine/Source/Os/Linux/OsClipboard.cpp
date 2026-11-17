#include <Base/Base.h>

#include <storm.h>

#include "Os/W32/OsClipboard.h"

#include <string.h>

#include <SDL.h>
#include <SDL_syswm.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>

static int s_initialized;

static BOOL ClipboardEventFilter(void *userdata, SDL_Event *event) {
  XSelectionRequestEvent *req;
  XEvent                  sevent;
  Atom                    type;
  int                     format;
  unsigned long           nbytes;
  unsigned long           overflow;
  unsigned char          *data;

  if (event->type != SDL_SYSWMEVENT) {
    return TRUE;
  }

  if (event->syswm.msg->msg.x11.event.type != SelectionRequest) {
    return TRUE;
  }

  req = &event->syswm.msg->msg.x11.event.xselectionrequest;
  sevent.xselection.type = SelectionNotify;
  sevent.xselection.display = req->display;
  sevent.xselection.selection = req->selection;
  sevent.xselection.target = None;
  sevent.xselection.property = None;
  sevent.xselection.requestor = req->requestor;
  sevent.xselection.time = req->time;

  if (XGetWindowProperty(
          req->display, DefaultRootWindow(req->display), XA_CUT_BUFFER0, 0, 0x1FFFFFFF, False, req->target, &type, &format, &nbytes, &overflow, &data
      ) == Success) {
    if (type == req->target) {
      if (type == XA_STRING && data[nbytes - 1] == 0) {
        --nbytes;
      }
      XChangeProperty(req->display, req->requestor, req->property, type, format, PropModeReplace, data, nbytes);
      sevent.xselection.property = req->property;
    }
    XFree(data);
  }

  XSendEvent(req->display, req->requestor, False, 0, &sevent);
  XSync(req->display, False);
  return TRUE;
}

static void OsClipboardInitialize() {
  SDL_SysWMinfo info;

  SDL_VERSION(&info.version);
  if (SDL_GetWindowWMInfo(SDL_GL_GetCurrentWindow(), &info) > 0 && info.subsystem == SDL_SYSWM_X11) {
    SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);
    SDL_SetEventFilter(ClipboardEventFilter, 0);
  }

  s_initialized = 1;
}

char *OsClipboardGetString() {
  SDL_SysWMinfo  info;
  Display       *display;
  Window         window;
  Window         owner;
  Atom           selection;
  Atom           type;
  int            format;
  unsigned long  nbytes;
  unsigned long  overflow;
  unsigned char *data;
  XEvent         event;
  char          *result = 0;

  SDL_VERSION(&info.version);
  if (SDL_GetWindowWMInfo(SDL_GL_GetCurrentWindow(), &info) > 0 && info.subsystem == SDL_SYSWM_X11) {
    display = info.info.x11.display;
    window = info.info.x11.window;

    owner = XGetSelectionOwner(display, XA_PRIMARY);
    if (owner == None || owner == window) {
      owner = DefaultRootWindow(display);
      selection = XA_CUT_BUFFER0;
    } else {
      int selectionResponse = 0;

      owner = window;
      selection = XInternAtom(display, "SDL_SELECTION", False);
      XConvertSelection(display, XA_PRIMARY, XA_STRING, selection, owner, CurrentTime);
      while (!selectionResponse) {
        XNextEvent(display, &event);
        if (event.type == SelectionNotify && event.xselection.requestor == owner) {
          selectionResponse = 1;
        }
      }
    }

    if (XGetWindowProperty(display, owner, selection, 0, 0x1FFFFFFF, False, XA_STRING, &type, &format, &nbytes, &overflow, &data) == Success && data) {
      if (type == XA_STRING) {
        result = (char *)SMemAlloc(nbytes + 1, __FILE__, __LINE__, 0);
        memcpy(result, data, nbytes);
        result[nbytes] = 0;
      }
      XFree(data);
    }
  } else if (SDL_HasClipboardText()) {
    char *text = SDL_GetClipboardText();
    if (text) {
      UINT length = SStrLen(text);
      result = (char *)SMemAlloc(length + 1, __FILE__, __LINE__, 0);
      memcpy(result, text, length + 1);
      SDL_free(text);
    }
  }

  if (!result) {
    result = (char *)SMemAlloc(1, __FILE__, __LINE__, 0);
    result[0] = 0;
  }

  return result;
}

void OsClipboardFreeString(char *string) {
  if (string) {
    SMemFree(string, __FILE__, __LINE__, 0);
  }
}

BOOL OsClipboardGetString(char *buf, UINT bufSize) {
  char *string = OsClipboardGetString();

  SStrCopy(buf, string, bufSize);
  OsClipboardFreeString(string);
  return TRUE;
}

BOOL OsClipboardPutString(LPCSTR string) {
  SDL_SysWMinfo info;
  Display      *display;
  Window        window;

  if (!s_initialized) {
    OsClipboardInitialize();
  }

  SDL_VERSION(&info.version);
  if (SDL_GetWindowWMInfo(SDL_GL_GetCurrentWindow(), &info) > 0 && info.subsystem == SDL_SYSWM_X11) {
    display = info.info.x11.display;
    window = info.info.x11.window;

    XChangeProperty(display, DefaultRootWindow(display), XA_CUT_BUFFER0, XA_STRING, 8, PropModeReplace, (const unsigned char *)string, SStrLen(string));
    if (XGetSelectionOwner(display, XA_PRIMARY) != window) {
      XSetSelectionOwner(display, XA_PRIMARY, window, CurrentTime);
    }
  } else {
    SDL_SetClipboardText(string);
  }

  return TRUE;
}
