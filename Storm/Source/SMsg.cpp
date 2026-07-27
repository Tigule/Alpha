#include <storm.h>
#include <stpl.h>

#define REGISTERTYPE_BASE       'SMSG'
#define REGISTERTYPE_MESSAGE    (REGISTERTYPE_BASE + 0)
#define REGISTERTYPE_COMMAND    (REGISTERTYPE_BASE + 1)
#define REGISTERTYPE_SYSCOMMAND (REGISTERTYPE_BASE + 2)
#define REGISTERTYPE_KEYDOWN    (REGISTERTYPE_BASE + 3)
#define REGISTERTYPE_KEYUP      (REGISTERTYPE_BASE + 4)

struct WNDREC : TSLinkedNode<WNDREC> {
  HWND window;
};
typedef WNDREC *WNDRECPTR;

static HWND                               s_defaultwindow;
static RECT                               s_defaultwindowrect;
static TSList<WNDREC, TSGetLink<WNDREC> > s_wndlist;

static void AddWindow(HWND window) {
  WNDRECPTR entry;

  entry = s_wndlist.NewNode(LIST_TAIL, 0, 0);
  entry->window = window;
}

static WNDRECPTR FindWindowA(HWND window) {
  WNDRECPTR entry;

  entry = s_wndlist.Head();
  while ((LONG)entry > 0) {
    if (entry->window == window) {
      return entry;
    }
    entry = s_wndlist.RawNext(entry);
  }

  return NULL;
}

static void DeleteWindow(HWND window) {
  WNDRECPTR entry;

  if (window == s_defaultwindow) {
    s_defaultwindow = NULL;
    memset(&s_defaultwindowrect, 0, sizeof(s_defaultwindowrect));
  }

  entry = FindWindowA(window);
  if (!entry) {
    return;
  }

  SEvtUnregisterType(REGISTERTYPE_MESSAGE, (DWORD)window);
  SEvtUnregisterType(REGISTERTYPE_COMMAND, (DWORD)window);
  SEvtUnregisterType(REGISTERTYPE_SYSCOMMAND, (DWORD)window);
  SEvtUnregisterType(REGISTERTYPE_KEYUP, (DWORD)window);
  SEvtUnregisterType(REGISTERTYPE_KEYDOWN, (DWORD)window);

  s_wndlist.DeleteNode(entry);
}

static LRESULT CALLBACK GenericWndProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
  LONG result;
  BOOL useresult;

  useresult = FALSE;
  result = 0;
  if (SMsgDispatchMessage(window, message, (UINT)wparam, (LONG)lparam, &useresult, &result) && useresult) {
    return result;
  }

  return DefWindowProcA(window, message, wparam, lparam);
}

static BOOL InternalRegister(DWORD type, HWND window, DWORD id, void(APIENTRY *handler)(SMSGPARAMS *)) {
  FATALASSERT(handler);

  if (!FindWindowA(window)) {
    AddWindow(window);
  }

  return SEvtRegisterHandler(type, (DWORD)window, id, 0, (SEVTHANDLER)handler);
}

static BOOL InternalUnregister(DWORD type, HWND window, DWORD id, void(APIENTRY *handler)(SMSGPARAMS *)) {
  if (!FindWindowA(window)) {
    AddWindow(window);
  }

  return SEvtUnregisterHandler(type, (DWORD)window, id, (SEVTHANDLER)handler);
}

extern "C" BOOL APIENTRY SMsgBreakHandlerChain(SMSGPARAMS *params) {
  return SEvtBreakHandlerChain(params);
}

extern "C" BOOL APIENTRY SMsgDestroy() {
  while (!s_wndlist.IsEmpty()) {
    DeleteWindow(s_wndlist.Head()->window);
  }

  return TRUE;
}

extern "C" BOOL APIENTRY SMsgDispatchMessage(HWND window, UINT message, UINT wparam, LONG lparam, BOOL *useresult, LONG *result) {
  SMSGPARAMS params;

  if (useresult) {
    *useresult = FALSE;
  }
  if (result) {
    *result = 0;
  }

  params.window = window;
  params.message = message;
  params.wparam = wparam;
  params.lparam = lparam;
  if (message == WM_COMMAND) {
    params.notifycode = HIWORD(wparam);
  } else {
    params.notifycode = 0;
  }
  params.useresult = FALSE;
  params.result = 0;

dispatch:
  SEvtDispatch(REGISTERTYPE_MESSAGE, (DWORD)window, message, &params);

  switch (message) {
    case WM_COMMAND:
      SEvtDispatch(REGISTERTYPE_COMMAND, (DWORD)window, LOWORD(wparam), &params);
      break;
    case WM_SYSCOMMAND:
      SEvtDispatch(REGISTERTYPE_SYSCOMMAND, (DWORD)window, wparam, &params);
      break;
    case WM_KEYDOWN:
      SEvtDispatch(REGISTERTYPE_KEYDOWN, (DWORD)window, wparam, &params);
      break;
    case WM_KEYUP:
      SEvtDispatch(REGISTERTYPE_KEYUP, (DWORD)window, wparam, &params);
      break;
    default:
      break;
  }

  if (window && window == s_defaultwindow) {
    window = NULL;
    goto dispatch;
  }

  if (message == WM_NCDESTROY) {
    DeleteWindow(params.window);
  }

  if (useresult) {
    *useresult = params.useresult;
  }
  if (result) {
    *result = params.result;
  }

  return TRUE;
}

extern "C" int APIENTRY SMsgDoMessageLoop(SMSGIDLEPROC idleproc, BOOL cleanuponquit) {
  MSG message;
  int idlecount;

  (void)cleanuponquit;
  idlecount = 0;

  for (;;) {
    if (!PeekMessageA(&message, NULL, 0, 0, PM_NOREMOVE)) {
      if (idleproc && idleproc(idlecount++)) {
        continue;
      }
    }

    idlecount = 0;
    if (!GetMessageA(&message, NULL, 0, 0)) {
      return (int)message.wParam;
    }

    TranslateMessage(&message);
    DispatchMessageA(&message);
  }
}

extern "C" HWND APIENTRY SMsgGetDefaultWindow() {
  return s_defaultwindow;
}

extern "C" BOOL APIENTRY SMsgGetDefaultWindowRect(LPRECT rect) {
  FATALASSERT(rect);

  if (!s_defaultwindowrect.left && !s_defaultwindowrect.top && !s_defaultwindowrect.right && !s_defaultwindowrect.bottom) {
    return GetClientRect(s_defaultwindow, rect);
  }

  *rect = s_defaultwindowrect;
  return TRUE;
}

extern "C" WNDPROC APIENTRY SMsgGetGenericWndProc(DWORD id) {
  FATALASSERT(!id);

  return GenericWndProc;
}

extern "C" BOOL APIENTRY SMsgPopRegisterState(HWND window) {
  SEvtPopState(REGISTERTYPE_COMMAND, (DWORD)window);
  SEvtPopState(REGISTERTYPE_SYSCOMMAND, (DWORD)window);
  SEvtPopState(REGISTERTYPE_KEYDOWN, (DWORD)window);
  SEvtPopState(REGISTERTYPE_KEYUP, (DWORD)window);
  SEvtPopState(REGISTERTYPE_MESSAGE, (DWORD)window);
  return TRUE;
}

extern "C" BOOL APIENTRY SMsgPushRegisterState(HWND window) {
  SEvtPushState(REGISTERTYPE_COMMAND, (DWORD)window);
  SEvtPushState(REGISTERTYPE_SYSCOMMAND, (DWORD)window);
  SEvtPushState(REGISTERTYPE_KEYDOWN, (DWORD)window);
  SEvtPushState(REGISTERTYPE_KEYUP, (DWORD)window);
  SEvtPushState(REGISTERTYPE_MESSAGE, (DWORD)window);
  return TRUE;
}

extern "C" BOOL APIENTRY SMsgRegisterCommand(HWND window, UINT id, SMSGHANDLER handler) {
  return InternalRegister(REGISTERTYPE_COMMAND, window, id, (void(APIENTRY *)(SMSGPARAMS *))handler);
}

extern "C" BOOL APIENTRY SMsgRegisterSysCommand(HWND window, UINT id, SMSGHANDLER handler) {
  return InternalRegister(REGISTERTYPE_SYSCOMMAND, window, id, (void(APIENTRY *)(SMSGPARAMS *))handler);
}

extern "C" BOOL APIENTRY SMsgRegisterKeyDown(HWND window, UINT id, SMSGHANDLER handler) {
  return InternalRegister(REGISTERTYPE_KEYDOWN, window, id, (void(APIENTRY *)(SMSGPARAMS *))handler);
}

extern "C" BOOL APIENTRY SMsgRegisterKeyUp(HWND window, UINT id, SMSGHANDLER handler) {
  return InternalRegister(REGISTERTYPE_KEYUP, window, id, (void(APIENTRY *)(SMSGPARAMS *))handler);
}

extern "C" BOOL APIENTRY SMsgRegisterMessage(HWND window, UINT id, SMSGHANDLER handler) {
  return InternalRegister(REGISTERTYPE_MESSAGE, window, id, (void(APIENTRY *)(SMSGPARAMS *))handler);
}

extern "C" BOOL APIENTRY SMsgSetDefaultWindow(HWND window) {
  s_defaultwindow = window;
  return TRUE;
}

extern "C" void APIENTRY SMsgSetDefaultWindowRect(RECT *rect) {
  FATALASSERT(rect);

  s_defaultwindowrect = *rect;
}

extern "C" BOOL APIENTRY SMsgUnregisterCommand(HWND window, UINT id, SMSGHANDLER handler) {
  return InternalUnregister(REGISTERTYPE_COMMAND, window, id, (void(APIENTRY *)(SMSGPARAMS *))handler);
}

extern "C" BOOL APIENTRY SMsgUnregisterSysCommand(HWND window, UINT id, SMSGHANDLER handler) {
  return InternalUnregister(REGISTERTYPE_SYSCOMMAND, window, id, (void(APIENTRY *)(SMSGPARAMS *))handler);
}

extern "C" BOOL APIENTRY SMsgUnregisterKeyDown(HWND window, UINT id, SMSGHANDLER handler) {
  return InternalUnregister(REGISTERTYPE_KEYDOWN, window, id, (void(APIENTRY *)(SMSGPARAMS *))handler);
}

extern "C" BOOL APIENTRY SMsgUnregisterKeyUp(HWND window, UINT id, SMSGHANDLER handler) {
  return InternalUnregister(REGISTERTYPE_KEYUP, window, id, (void(APIENTRY *)(SMSGPARAMS *))handler);
}

extern "C" BOOL APIENTRY SMsgUnregisterMessage(HWND window, UINT id, SMSGHANDLER handler) {
  return InternalUnregister(REGISTERTYPE_MESSAGE, window, id, (void(APIENTRY *)(SMSGPARAMS *))handler);
}
