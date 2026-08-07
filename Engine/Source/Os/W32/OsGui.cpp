#include <Base/Base.h>

#include "OsGui.h"
#include "Input.h"
#include "Debugging.h"

#include <storm.h>
#include <Tempest/cirect.h>
#include <windows.h>
#include <commctrl.h>
#include <malloc.h>

struct WINDOWINFO_TIGULE {
  DWORD cbSize;
  RECT  rcWindow;
  RECT  rcClient;
  DWORD dwStyle;
  DWORD dwExStyle;
  DWORD dwWindowStatus;
  UINT  cxWindowBorders;
  UINT  cyWindowBorders;
  ATOM  atomWindowType;
  WORD  wCreatorVersion;
};

extern "C" BOOL WINAPI GetWindowInfo(HWND hwnd, WINDOWINFO_TIGULE *windowInfo);

#pragma pack(push, 2)
class CBasicDlgTemplate {
 public:
  CBasicDlgTemplate(WORD width, WORD height) {
    header.style = 0x80C00004;
    header.dwExtendedStyle = 0;
    header.cdit = 0;
    header.x = 0;
    header.y = 0;
    header.cx = width;
    header.cy = height;
    noMenu = 0;
    noClass = 0;
    noTitle = 0;
  }

  DLGTEMPLATE header;
  WORD        noMenu;
  WORD        noClass;
  WORD        noTitle;
};
#pragma pack(pop)

static HINSTANCE                     sAppInstance;
static LPVOID                        s_GxDevWindow;
static TSGrowableArray<COsDialog *>  sDialogs;
static TSGrowableArray<COsMenuBar *> sMenubars;
static int                           sMenuHotkeysEnabled = 1;
static int                           sMasterTooltipsEnabled = 1;
static HWND                          sGlobalTips;
static int                           sIdleTimerID;
static LPCSTR const                  OsGuiPointerProp = "OsGuiPointer";
static const UINT                    SelectedState = 3;
static const UINT                    lvExtStyle = 0x20;

struct OsGuiCallbackInfo {
  void (*function)(const OsGuiCallbackParams &);
  LPVOID userParam;
};
static OsGuiCallbackInfo sCallbacks[2];

static const UINT ControlStyles[20] = {0x00010000, 0x00000080, 0x00000100, 0x0000010E, 0x00810080, 0x00210003, 0x00A10001,
                                       0x00010006, 0x00800001, 0x00000001, 0x00810033, 0x0000000B, 0x00000007, 0x00000020,
                                       0x00000009, 0x00000000, 0x00A1000D, 0x00001100, 0x00000000, 0x00040100};
static const UINT ControlStylesExt[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static const UINT ControlFont[20] = {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static LPCSTR     ControlClassName[20] = {
    "BUTTON",
    "BUTTON",
    "STATIC",
    "STATIC",
    "EDIT",
    "COMBOBOX",
    "LISTBOX",
    "BUTTON",
    "msctls_progress32",
    "msctls_trackbar32",
    "SysTreeView32",
    "BUTTON",
    "STATIC",
    "msctls_updown32",
    "BUTTON",
    "SysTabControl32",
    "SysListView32",
    "ToolbarWindow32",
    "SCROLLBAR",
    "STATIC"
};

void OsGuiSetMenuCommandCallback(void (*inFunc)(const OsGuiCallbackParams &), LPVOID inParam) {
  sCallbacks[0].function = inFunc;
  sCallbacks[0].userParam = inParam;
}

void OsGuiSetIdleCallback(void (*inFunc)(const OsGuiCallbackParams &), LPVOID inParam) {
  sCallbacks[1].function = inFunc;
  sCallbacks[1].userParam = inParam;
}

typedef long (*OSWINDOWPROC)(LPVOID, UINT, UINT, long);
void              OsSetWindowProc(OSWINDOWPROC windowproc);
long              OsGuiWindowProc(LPVOID _hWnd, UINT uMsg, UINT wParam, long lParam);
void              OsGuiSetCursor(int inCursor);
void              OsGuiGetCursorPosition(int *outX, int *outY);
void              OsGuiSetWindowRect(LPVOID inWindow, const NTempest::CiRect &inRect);
void              OsGuiSetWindowIcon(LPVOID inWindow, LPCSTR inName);
HICON__          *sWinCursor(int inCursor);
static HBITMAP__ *sBitmapFromImageData(int inWidth, int inHeight, LPVOID inData, HDC__ *inDC);
static HBITMAP__ *sMaskFromImageData(int inWidth, int inHeight, LPVOID inData, HDC__ *inDC);

static HWND__ *sCreateTooltips(HWND__ *inOwner) {
  HWND tips = CreateWindowExA(
      0, TOOLTIPS_CLASSA, 0, WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, inOwner, 0,
      sAppInstance, 0
  );
  SetWindowPos(tips, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
  SendMessageA(tips, 0x418, 0, 300);
  SendMessageA(tips, TTM_ACTIVATE, sMasterTooltipsEnabled, 0);
  return tips;
}

static HWND__ *sGetGlobalTips() {
  if (!sGlobalTips) {
    sGlobalTips = sCreateTooltips(0);
  }
  return sGlobalTips;
}

static void sEnableGlobalTips(int inVal) {
  if (sGlobalTips) {
    SendMessageA(sGlobalTips, TTM_ACTIVATE, inVal, 0);
  }
}

static LPVOID sGetOsGuiPointer(HWND hwnd) {
  return GetPropA(hwnd, "OsGuiPointer");
}

static void sSetOsGuiPointer(HWND__ *hwnd, LPVOID data) {
  SetPropA(hwnd, "OsGuiPointer", data);
}

static void sRemoveOsGuiPointer(HWND__ *hwnd) {
  RemovePropA(hwnd, "OsGuiPointer");
}

static void sDoCallback(int inCB, int type, int subtype) {
  FATALASSERT(inCB >= 0 && inCB < 2);
  if (sCallbacks[inCB].function) {
    OsGuiCallbackParams params;
    params.type = type;
    params.subType = subtype;
    params.code = 0;
    params.user = sCallbacks[inCB].userParam;
    sCallbacks[inCB].function(params);
  }
}

static void CALLBACK sIdleTimerProc(HWND__ *, UINT, UINT, DWORD) {
  sDoCallback(1, 0, 0);
}

static void sStartIdle() {
  if (!sIdleTimerID) {
    sIdleTimerID = SetTimer(0, 0, 50, sIdleTimerProc);
  }
}

static void sStopIdle() {
  if (sIdleTimerID) {
    KillTimer(0, sIdleTimerID);
    sIdleTimerID = 0;
  }
}

static int sMenuReal2RawID(int inID) {
  if (inID == 1) {
    return 64188;
  }
  if (inID == 2) {
    return 64189;
  }
  return inID;
}

static int sMenuRaw2RealID(int inID) {
  if (inID == 64188) {
    return 1;
  }
  if (inID == 64189) {
    return 2;
  }
  return inID;
}

void OsGuiMenuSelect(int menuID, int itemID) {
  PostMessageA(GetActiveWindow(), WM_COMMAND, MAKEWPARAM(itemID, menuID), 0);
}

static void sWinRectToCiRect(const tagRECT *winRect, NTempest::CiRect *outRect) {
  outRect->l = winRect->left;
  outRect->r = winRect->right;
  outRect->t = winRect->top;
  outRect->b = winRect->bottom;
}

static void sCiRectToWinRect(const NTempest::CiRect *inRect, tagRECT *outRect) {
  outRect->left = inRect->l;
  outRect->right = inRect->r;
  outRect->top = inRect->t;
  outRect->bottom = inRect->b;
}

NTempest::CiRect OsGuiGetWindowRect(LPVOID inWindow, int inClientOnly) {
  WINDOWINFO_TIGULE windInfo;
  windInfo.cbSize = sizeof(windInfo);
  GetWindowInfo(static_cast<HWND>(inWindow), &windInfo);

  RECT             wRect = inClientOnly ? windInfo.rcClient : windInfo.rcWindow;
  NTempest::CiRect outRect;
  sWinRectToCiRect(&wRect, &outRect);
  return outRect;
}

NTempest::CiRect OsGuiGetWindowRestoredRect(LPVOID inWindow) {
  WINDOWPLACEMENT wp;
  wp.length = sizeof(wp);
  GetWindowPlacement(static_cast<HWND>(inWindow), &wp);

  NTempest::CiRect restRect;
  sWinRectToCiRect(&wp.rcNormalPosition, &restRect);
  return restRect;
}

NTempest::CImVector OsGuiGetColor(int inColorType) {
  NTempest::CImVector color;
  DWORD               value;

  switch (inColorType) {
    case 0:
      value = GetSysColor(COLOR_BTNFACE);
      break;
    case 1:
      value = GetSysColor(COLOR_WINDOW);
      break;
    default:
      return NTempest::CImVector(0ul);
  }

  color.r = GetRValue(value);
  color.g = GetGValue(value);
  color.b = GetBValue(value);
  color.a = 0xFF;

  return color;
}

void OsGuiInitialize() {
  OsSetWindowProc(OsGuiWindowProc);
  INITCOMMONCONTROLSEX initCtrls;
  initCtrls.dwSize = sizeof(initCtrls);
  initCtrls.dwICC = 0x2E;
  ASSERT(InitCommonControlsEx(&initCtrls));
}

void OsGuiDestroy() {
  OsSetWindowProc(0);
  if (sGlobalTips) {
    DestroyWindow(sGlobalTips);
    sGlobalTips = 0;
  }
}

void OsGuiSetApplicationInfo(LPVOID inData) {
  sAppInstance = static_cast<HINSTANCE>(inData);
}

BOOL OsGuiProcessMessage(LPVOID inMsgData) {
  MSG *message = static_cast<MSG *>(inMsgData);
  int  handled = 0;

  if (!sAppInstance) {
    return 0;
  }

  if (sMenuHotkeysEnabled) {
    UINT index = 0;

    while (index < sMenubars.Count()) {
      COsMenuBar *menuBar;
      HACCEL      accelTable;

      menuBar = sMenubars[index];
      accelTable = static_cast<HACCEL>(menuBar->GetAccelerators());

      if (accelTable) {
        HWND menuWindow = static_cast<HWND>(menuBar->GetWindow());
        HWND activeWindow = static_cast<HWND>(OsGuiGetWindow(1));
        BOOL canTranslate = menuWindow == activeWindow;

        if (!canTranslate) {
          COsDialog *dialog = static_cast<COsDialog *>(sGetOsGuiPointer(activeWindow));

          canTranslate = dialog && dialog->HasFlag(0x10) && menuWindow == dialog->GetParentWindow();
        }

        if (canTranslate && TranslateAcceleratorA(menuWindow, accelTable, message)) {
          handled = 1;
          if (message->message != WM_KEYDOWN || message->wParam != VK_DELETE) {
            return 1;
          }
          break;
        }
      }

      ++index;
    }
  }

  {
    UINT index = 0;

    while (index < sDialogs.Count()) {
      if (sDialogs[index]->ProcessMessage(message)) {
        handled = 1;
        break;
      }

      ++index;
    }
  }

  if (sGlobalTips) {
    SendMessageA(static_cast<HWND>(sGlobalTips), 0x407, 0, reinterpret_cast<LPARAM>(message));
  }

  return handled;
}

void OsGuiEnableTooltips(int inVal) {
  if (inVal != sMasterTooltipsEnabled) {
    sMasterTooltipsEnabled = inVal;
    sEnableGlobalTips(inVal);
    for (UINT i = 0; i < sDialogs.Count(); ++i) {
      sDialogs[i]->EnableTooltips(sMasterTooltipsEnabled);
    }
  }
}

void OsGuiEnableMenuHotkeys(int inVal) {
  sMenuHotkeysEnabled = inVal;
}

static int sKeyToVirtKey(int key) {
  if ((key >= '0' && key <= 'Z')) {
    return key;
  }
  if (key >= 768 && key <= 779) {
    return key - 656;
  }
  if (key >= 258 && key <= 266) {
    return key - 161;
  }
  switch (key) {
    case 0:
      return VK_SHIFT;
    case 1:
      return VK_CONTROL;
    case 2:
      return VK_MENU;
    case 32:
      return VK_SPACE;
    case 256:
      return 0xC0;
    case 512:
      return VK_ESCAPE;
    case 513:
      return VK_RETURN;
    case 514:
      return VK_BACK;
    case 515:
      return VK_TAB;
    case 516:
      return VK_LEFT;
    case 517:
      return VK_UP;
    case 518:
      return VK_RIGHT;
    case 519:
      return VK_DOWN;
    case 520:
      return VK_INSERT;
    case 521:
      return VK_DELETE;
    case 522:
      return VK_HOME;
    case 523:
      return VK_END;
    case 524:
      return VK_PRIOR;
    case 525:
      return VK_NEXT;
    case 526:
      return VK_CAPITAL;
    case 527:
      return VK_NUMLOCK;
    case 528:
      return VK_SCROLL;
    case 529:
      return VK_PAUSE;
    case 530:
      return VK_SNAPSHOT;
    default:
      ASSERT(key);
      return -1;
  }
}

static void sHotkeyToAccel(OsGuiMenuHotkey *hotkey, tagACCEL *accel) {
  accel->fVirt = FVIRTKEY;
  if (hotkey->modKeyID & 2) {
    accel->fVirt |= FCONTROL;
  }
  if (hotkey->modKeyID & 1) {
    accel->fVirt |= FSHIFT;
  }
  if (hotkey->modKeyID & 4) {
    accel->fVirt |= FALT;
  }
  accel->key = static_cast<WORD>(sKeyToVirtKey(hotkey->keyID));
}

static void sGetHotkeyText(int keyID, int modID, char *buf, int bufSize) {
  *buf = 0;
  char modText[20] = "";
  if (modID & 2) {
    SStrPack(modText, "Ctrl+", sizeof(modText));
  }
  if (modID & 1) {
    SStrPack(modText, "Shift+", sizeof(modText));
  }
  if (modID & 4) {
    SStrPack(modText, "Alt+", sizeof(modText));
  }

  char keyText[50] = "";
  if ((keyID >= '0' && keyID <= '9') || (keyID >= 'A' && keyID <= 'Z')) {
    keyText[0] = static_cast<char>(keyID);
    keyText[1] = 0;
  } else if (keyID >= 768 && keyID <= 779) {
    SStrPrintf(keyText, sizeof(keyText), "F%1d", keyID - 767);
  } else {
    LPCSTR text = "Unknown";
    switch (keyID) {
      case 32:
        text = "Space";
        break;
      case 274:
        text = "[";
        break;
      case 275:
        text = "]";
        break;
      case 512:
        text = "Esc";
        break;
      case 513:
        text = "Enter";
        break;
      case 514:
        text = "Backspace";
        break;
      case 515:
        text = "Tab";
        break;
      case 516:
        text = "Left";
        break;
      case 517:
        text = "Up";
        break;
      case 518:
        text = "Right";
        break;
      case 519:
        text = "Down";
        break;
      case 521:
        text = "Delete";
        break;
    }
    SStrCopy(keyText, text, sizeof(keyText));
  }
  if (keyText[0]) {
    SStrPrintf(buf, bufSize, "%s%s", modText, keyText);
  }
}

static int sNCodeToItemCode(int nCode, int ctrlType) {
  struct OsGuiCodeTranslation {
    int winCode;
    int ctrlType;
    int osGuiCode;
  };

  static const OsGuiCodeTranslation table[18] = {
      { 0,    0,  0},
      { 1,    0,  0},
      { 4,  768,  2},
      { 4,  512, 13},
      { 5,    1,  2},
      { 6,    1,  2},
      { 6,    2,  1},
      { 7,    0,  2},
      {10, -402,  2},
      {10,   -3,  1},
      {10, -411,  8},
      {10,   -7, 14},
      {10,   -8, 13},
      {11,    0,  0},
      {13,    4,  2},
      {14,    0,  2},
      {15, -551,  2},
      {16,   -3,  1}
  };

  for (UINT i = 0; i < sizeof(table) / sizeof(table[0]); ++i) {
    if (table[i].ctrlType == ctrlType && table[i].winCode == nCode) {
      return table[i].osGuiCode;
    }
  }
  return -1;
}

static int sHandleDrawItem(long lParam) {
  DRAWITEMSTRUCT *draw = reinterpret_cast<DRAWITEMSTRUCT *>(lParam);
  COsControl     *control = static_cast<COsControl *>(sGetOsGuiPointer(draw->hwndItem));
  if (!control) {
    return 0;
  }

  UINT state = 0;
  if (draw->itemState & ODS_DISABLED) {
    state |= 1;
  }
  if (draw->itemState & ODS_FOCUS) {
    state |= 2;
  }
  if (draw->itemState & ODS_SELECTED) {
    state |= 4;
  }

  NTempest::CiRect drawRect;
  memset(&drawRect, 0, sizeof(drawRect));
  sWinRectToCiRect(&draw->rcItem, &drawRect);
  return control->OnDraw(draw->hDC, state, drawRect);
}

static LPVOID sHandleCtlColor(UINT wParam, long lParam) {
  COsControl *control = static_cast<COsControl *>(sGetOsGuiPointer(reinterpret_cast<HWND>(lParam)));
  return control ? control->OnSetColors(reinterpret_cast<LPVOID>(wParam)) : 0;
}

static int CALLBACK sDlgProc(HWND__ *hdlg, UINT msg, UINT wParam, long lParam) {
  COsDialog *dialog = static_cast<COsDialog *>(sGetOsGuiPointer(hdlg));
  switch (msg) {
    case WM_INITDIALOG:
      return 1;
    case WM_DRAWITEM:
      return sHandleDrawItem(lParam);
    case WM_CTLCOLORBTN:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
      return reinterpret_cast<int>(sHandleCtlColor(wParam, lParam));
    case WM_NOTIFY: {
      NMHDR      *notify = reinterpret_cast<NMHDR *>(lParam);
      COsControl *control = static_cast<COsControl *>(sGetOsGuiPointer(notify->hwndFrom));
      return control ? control->OnNotify(notify->code, notify) : 0;
    }
    case WM_COMMAND:
      if (lParam) {
        COsControl *control = static_cast<COsControl *>(sGetOsGuiPointer(reinterpret_cast<HWND>(lParam)));
        return control ? control->OnCommand(wParam) : 0;
      }
      if (wParam == IDOK || wParam == IDCANCEL) {
        if (!dialog) {
          return 0;
        }
        int         accept = wParam == IDOK;
        COsControl *control = dialog->FindControl(GetFocus());
        if (control && (accept ? control->OnReturn() : control->OnEscape())) {
          return 1;
        }
        return accept ? dialog->OnAccept() : dialog->OnCancel();
      }
      if (dialog && GetMenu(hdlg)) {
        return dialog->OnEvent(-2, sMenuRaw2RealID(LOWORD(wParam)), 0);
      }
      return 0;
    case WM_HSCROLL:
    case WM_VSCROLL:
      if (lParam) {
        COsControl *control = static_cast<COsControl *>(sGetOsGuiPointer(reinterpret_cast<HWND>(lParam)));
        return control ? control->OnScroll(wParam) : 0;
      }
      return 0;
    case WM_SIZE:
      return dialog ? dialog->OnEvent(-1, 7, 0) : 0;
    case WM_ACTIVATE:
      return dialog && LOWORD(wParam) ? dialog->OnEvent(-1, 18, 0) : 0;
    case WM_CLOSE:
      return dialog ? dialog->OnEvent(-1, 6, 0) : 0;
    case WM_GETMINMAXINFO:
      if (dialog) {
        int minWidth;
        int minHeight;
        dialog->GetMinSize(&minWidth, &minHeight);
        MINMAXINFO *minMaxInfo = reinterpret_cast<MINMAXINFO *>(lParam);
        if (minWidth != -1) {
          minMaxInfo->ptMinTrackSize.x = minWidth;
        }
        if (minHeight != -1) {
          minMaxInfo->ptMinTrackSize.y = minHeight;
        }
      }
      return 0;
    case WM_CONTEXTMENU:
      if (reinterpret_cast<HWND>(wParam) == hdlg && dialog) {
        return dialog->OnContextMenu(LOWORD(lParam), HIWORD(lParam));
      } else {
        COsControl *control = static_cast<COsControl *>(sGetOsGuiPointer(reinterpret_cast<HWND>(wParam)));
        return control ? control->OnContextMenu(LOWORD(lParam), HIWORD(lParam)) : 0;
      }
    case WM_MOUSEMOVE:
      if (dialog) {
        dialog->OnMouseMove(LOWORD(lParam), HIWORD(lParam));
      }
      return 0;
    case WM_LBUTTONDOWN:
      return dialog ? dialog->OnMouseDown() : 0;
    case WM_LBUTTONUP:
      return dialog ? dialog->OnMouseUp() : 0;
    case 0x20A:
      if (dialog) {
        COsControl *control = dialog->FindControl(GetFocus());
        if (control) {
          control->OnMouseWheel(static_cast<short>(HIWORD(wParam)) / 120);
        }
      }
      return 0;
    case WM_MOUSELEAVE:
      if (dialog) {
        dialog->OnMouseLeave();
      }
      return 0;
    case WM_ENTERMENULOOP:
    case WM_ENTERSIZEMOVE:
      sStartIdle();
      return 0;
    case WM_EXITMENULOOP:
    case WM_EXITSIZEMOVE:
      sStopIdle();
      return 0;
    case WM_ENTERIDLE:
      sDoCallback(1, 0, 0);
      return 0;
  }
  return 0;
}

static int CALLBACK sDisableWindow(HWND__ *hwnd, long param) {
  TSGrowableArray<LPVOID> *windows = reinterpret_cast<TSGrowableArray<LPVOID> *>(param);
  if (reinterpret_cast<HINSTANCE>(GetWindowLongA(hwnd, GWL_HINSTANCE)) == sAppInstance && IsWindowEnabled(hwnd)) {
    EnableWindow(hwnd, FALSE);
    *windows->New() = hwnd;
  }
  return 1;
}

static int CALLBACK sEditBoxProc(HWND__ *hwnd, UINT msg, UINT wParam, long lParam);
static int CALLBACK sDividerProc(HWND__ *hwnd, UINT msg, UINT wParam, long lParam);

COsControl::COsControl(COsDialog *inDialog, int inType, short inID, UINT inFlags) : mDialog(inDialog) {
  Initialize(inDialog->GetHandle(), inType, inID, inFlags);
  inDialog->AddControl(this);
}

COsControl::COsControl(LPVOID inWindow, int inType, short inID, UINT inFlags) : mDialog(0) {
  Initialize(inWindow, inType, inID, inFlags);
}

void COsControl::Initialize(LPVOID inWindow, int inType, short inID, UINT inFlags) {
  ASSERT(inType >= 0 && inType < 20);
  ASSERT(inID == -1 || inID >= 10);

  mID = inID;
  mCallback = 0;
  mCallbackParam = 0;
  mContextMenu = 0;
  mRedrawLevel = 0;
  mType = inType;
  mFlags = inFlags;
  mContextMenuEnabled = 1;

  DWORD style = ControlStyles[inType] | WS_CHILD | WS_VISIBLE;
  DWORD styleEx = ControlStylesExt[inType];
  if (inFlags & 0x8) {
    style &= ~WS_VISIBLE;
  }
  if (inFlags & 0x1) {
    style |= WS_TABSTOP;
  }
  if (inFlags & 0x2) {
    style |= WS_HSCROLL;
  }
  if (inFlags & 0x4) {
    style |= WS_VSCROLL;
  }
  if (inFlags & 0x10) {
    style |= WS_BORDER;
  }
  if (inFlags & 0x20) {
    styleEx |= WS_EX_TRANSPARENT;
  }

  switch (inType) {
    case 2:
      if (inFlags & 0x10000) {
        style |= SS_NOTIFY;
      }
      break;
    case 4:
      if (inFlags & 0x10000) {
        style = (style & 0xFFFFFF3B) | ES_MULTILINE | ES_AUTOVSCROLL;
      }
      if (inFlags & 0x20000) {
        style |= ES_WANTRETURN;
      }
      break;
    case 6:
      if (inFlags & 0x10000) {
        style |= LBS_MULTIPLESEL | LBS_EXTENDEDSEL;
      }
      break;
    case 10:
      if (inFlags & 0x10000) {
        style &= ~TVS_HASLINES;
      }
      if (inFlags & 0x20000) {
        style |= TVS_HASBUTTONS;
      }
      if (inFlags & 0x80000) {
        style = (style & 0xFFFFFBFE) | TVS_TRACKSELECT;
      }
      break;
    case 12:
      if (inFlags & 0x10000) {
        style = (style & 0xFFFFFFF8) | SS_ETCHEDFRAME;
      }
      if (inFlags & 0x20000) {
        style = (style & 0xFFFFFFE8) | SS_ETCHEDVERT;
      }
      if (inFlags & 0x40000) {
        style = (style & 0xFFFFFFF0) | SS_ETCHEDHORZ;
      }
      break;
    case 13:
      if (inFlags & 0x10000) {
        style |= UDS_ALIGNRIGHT;
      }
      if (inFlags & 0x20000) {
        style |= UDS_WRAP;
      }
      break;
    case 14:
      if (inFlags & 0x10000) {
        style |= BS_PUSHLIKE | BS_AUTOCHECKBOX;
      }
      break;
    case 18:
      if (inFlags & 0x10000) {
        style |= SBS_VERT;
      }
      if (inFlags & 0x20000) {
        style |= SBS_SIZEGRIP;
      }
      break;
  }

  ASSERT(sAppInstance != 0);
  mHandle = CreateWindowExA(
      styleEx, ControlClassName[mType], "", style, 0, 0, 0, 0, static_cast<HWND>(inWindow), reinterpret_cast<HMENU>(static_cast<int>(mID)),
      sAppInstance, 0
  );
  ASSERT(mHandle != 0);

  SetFont(ControlFont[mType]);
  sSetOsGuiPointer(static_cast<HWND>(mHandle), this);
}

COsControl::~COsControl() {
  if (!mDialog) {
    sRemoveOsGuiPointer(static_cast<HWND>(mHandle));
    DestroyWindow(static_cast<HWND>(mHandle));
  }
  DELIFUSED(mContextMenu);
}

void COsControl::SetRedraw(int inVal) {
  int send = 0;
  if (inVal) {
    send = mRedrawLevel == 1;
    if (mRedrawLevel > 0) {
      --mRedrawLevel;
    }
  } else {
    send = mRedrawLevel == 0;
    ++mRedrawLevel;
  }
  if (send) {
    SendMessageA(static_cast<HWND>(mHandle), WM_SETREDRAW, inVal, 0);
  }
}

void COsControl::Refresh(int inErase) {
  InvalidateRect(static_cast<HWND>(mHandle), 0, inErase);
}

void COsControl::SetCallback(void (*inFunc)(const OsGuiCallbackParams &), LPVOID inParam) {
  mCallback = inFunc;
  mCallbackParam = inParam;
}

BOOL COsControl::OnEvent(int inItemID, int inNotifyCode, int inCode) {
  if (!mCallback) {
    return 0;
  }
  OsGuiCallbackParams params = {inItemID, inNotifyCode, inCode, mCallbackParam};
  mCallback(params);
  return 1;
}

void COsControl::SetFont(int inFont) {
  int stockObject;
  switch (inFont) {
    case 0:
      stockObject = ANSI_VAR_FONT;
      break;
    case 1:
      stockObject = SYSTEM_FONT;
      break;
    case 2:
      stockObject = OEM_FIXED_FONT;
      break;
    default:
      return;
  }
  HGDIOBJ font = GetStockObject(stockObject);
  if (font) {
    SendMessageA(static_cast<HWND>(mHandle), WM_SETFONT, reinterpret_cast<WPARAM>(font), 1);
  }
}

void COsControl::SetInputFocus() {
  if (mDialog->GetHandle() == OsGuiGetWindow(1)) {
    SetFocus(static_cast<HWND>(mHandle));
  }
}

void COsControl::LoseInputFocus() {
  if (HasInputFocus()) {
    SetFocus(GetParent(static_cast<HWND>(mHandle)));
  }
}

BOOL COsControl::HasInputFocus() {
  return mHandle == GetFocus();
}

void COsControl::SetText(LPCSTR inText) {
  SetWindowTextA(static_cast<HWND>(mHandle), inText);
  OnTextChange();
}

void COsControl::GetText(char *outText, int inBufSize) {
  GetWindowTextA(static_cast<HWND>(mHandle), outText, inBufSize);
}

int COsControl::GetTextLength() {
  return GetWindowTextLengthA(static_cast<HWND>(mHandle));
}

void COsControl::GetTextSize(LPCSTR inText, int *outW, int *outH) {
  HDC     dc = GetDC(static_cast<HWND>(mHandle));
  HGDIOBJ oldFont = SelectObject(dc, reinterpret_cast<HGDIOBJ>(SendMessageA(static_cast<HWND>(mHandle), WM_GETFONT, 0, 0)));
  SIZE    size;
  GetTextExtentPoint32A(dc, inText, SStrLen(inText), &size);
  SelectObject(dc, oldFont);
  ReleaseDC(static_cast<HWND>(mHandle), dc);
  *outW = size.cx;
  if (outH) {
    *outH = size.cy;
  }
}

void COsControl::GetTextSize(int *outW, int *outH) {
  char text[260];
  GetText(text, sizeof(text));
  GetTextSize(text, outW, outH);
}

void COsControl::Show(int inVal) {
  ShowWindow(static_cast<HWND>(mHandle), inVal ? SW_SHOW : SW_HIDE);
}

BOOL COsControl::IsShowing() {
  return IsWindowVisible(static_cast<HWND>(mHandle));
}

void COsControl::Enable(int inVal) {
  EnableWindow(static_cast<HWND>(mHandle), inVal);
}

BOOL COsControl::IsEnabled() {
  return IsWindowEnabled(static_cast<HWND>(mHandle));
}

void COsControl::SetPosition(int inX, int inY) {
  SetWindowPos(static_cast<HWND>(mHandle), 0, inX, inY, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
}

void COsControl::GetPosition(int *outX, int *outY, int inParentRelative) {
  RECT rect;
  GetWindowRect(static_cast<HWND>(mHandle), &rect);
  int x = rect.left;
  int y = rect.top;
  if (inParentRelative) {
    HWND parent = GetParent(static_cast<HWND>(mHandle));
    if (parent) {
      NTempest::CiRect prect = OsGuiGetWindowRect(parent, 1);
      x -= prect.l;
      y -= prect.t;
    }
  }
  *outX = x;
  *outY = y;
}

void COsControl::SetSize(int inW, int inH) {
  SetWindowPos(static_cast<HWND>(mHandle), 0, 0, 0, inW, inH, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
  OnSizeChange();
}

void COsControl::GetSize(int *outW, int *outH) {
  RECT rect;
  GetWindowRect(static_cast<HWND>(mHandle), &rect);
  *outW = rect.right - rect.left;
  *outH = rect.bottom - rect.top;
}

void COsControl::SetTooltip(LPCSTR inText) {
  HWND tips;
  HWND owner;
  if (mDialog) {
    tips = static_cast<HWND>(mDialog->GetTooltips());
    owner = static_cast<HWND>(mDialog->GetHandle());
  } else {
    tips = sGetGlobalTips();
    owner = GetParent(static_cast<HWND>(mHandle));
  }
  if (tips) {
    TOOLINFOA toolinfo;
    memset(&toolinfo, 0, sizeof(toolinfo));
    toolinfo.cbSize = sizeof(toolinfo);
    toolinfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    toolinfo.hwnd = owner;
    toolinfo.uId = reinterpret_cast<UINT_PTR>(mHandle);
    toolinfo.hinst = sAppInstance;
    toolinfo.lpszText = const_cast<char *>(inText);
    if (!SendMessageA(tips, TTM_ADDTOOLA, 0, reinterpret_cast<LPARAM>(&toolinfo))) {
      OsOutputDebugString("Warning: COsControl::SetTooltip - TTM_ADDTOOL failed\n");
    }
  }
}

COsMenu::COsMenu(BYTE inID, LPCSTR inTitle) {
  SStrCopy(mTitle, inTitle, 0x7FFFFFFF);
  mMenuHandle = CreateMenu();
  mID = inID;
}

COsMenu::COsMenu() {
  mTitle[0] = 0;
  mID = 0xFF;
  mMenuHandle = CreatePopupMenu();
}

COsMenu::~COsMenu() {
  DestroyMenu(static_cast<HMENU>(mMenuHandle));
}

void COsMenu::Clear() {
  int count = GetNumItems();
  while (count-- > 0) {
    RemoveItem(0);
  }
}

void COsMenu::RemoveItem(int inPos) {
  DeleteMenu(static_cast<HMENU>(mMenuHandle), inPos, MF_BYPOSITION);
  RemoveHotkey(inPos);
}

int COsMenu::GetNumItems() {
  return GetMenuItemCount(static_cast<HMENU>(mMenuHandle));
}

BOOL COsMenu::GetHotkey(int inPos, OsGuiMenuHotkey *outHotkey) {
  ASSERT(inPos >= 0 && inPos < static_cast<int>(mHotkeys.Count()));
  if (mHotkeys[inPos].keyID == -1) {
    return 0;
  }
  *outHotkey = mHotkeys[inPos];
  return 1;
}

void COsMenu::AddHotkey(int inPos) {
  UINT oldCount = mHotkeys.Count();
  mHotkeys.SetCount(oldCount + 1);
  for (int i = oldCount; i > inPos; --i) {
    mHotkeys[i] = mHotkeys[i - 1];
  }
  mHotkeys[inPos].keyID = -1;
}

void COsMenu::RemoveHotkey(int inPos) {
  int numKeys = mHotkeys.Count();
  for (int i = inPos; i < numKeys - 1; ++i) {
    mHotkeys[i] = mHotkeys[i + 1];
  }
  mHotkeys.SetCount(numKeys - 1);
}

void COsMenu::AppendHotkeyText(char *inText, const OsGuiMenuHotkey &inHotkey) {
  char keyText[52];
  sGetHotkeyText(inHotkey.keyID, inHotkey.modKeyID, keyText, 50);
  SStrPack(inText, "\t", 0x7FFFFFFF);
  SStrPack(inText, keyText, 0x7FFFFFFF);
}

void COsMenu::AddTextItem(int inPos, LPCSTR inText, OsGuiMenuHotkey *inHotkey) {
  char itemText[260];
  SStrCopy(itemText, inText, 0x7FFFFFFF);
  if (inHotkey) {
    AppendHotkeyText(itemText, *inHotkey);
  }

  MENUITEMINFOA menuInfo;
  menuInfo.cbSize = sizeof(menuInfo);
  menuInfo.fMask = MIIM_TYPE | MIIM_ID;
  menuInfo.fType = MFT_STRING;
  menuInfo.dwTypeData = itemText;
  menuInfo.wID = sMenuReal2RawID((mID << 8) | inPos);
  InsertMenuItemA(static_cast<HMENU>(mMenuHandle), inPos, TRUE, &menuInfo);

  AddHotkey(inPos);
  if (inHotkey) {
    mHotkeys[inPos] = *inHotkey;
  }
}

void COsMenu::AddSubMenu(int inPos, LPCSTR inTitle, COsMenu *inMenu) {
  AddTextItem(inPos, inTitle, 0);

  MENUITEMINFOA menuInfo;
  menuInfo.cbSize = sizeof(menuInfo);
  menuInfo.fMask = MIIM_SUBMENU;
  menuInfo.hSubMenu = static_cast<HMENU>(inMenu->GetMenuHandle());
  SetMenuItemInfoA(static_cast<HMENU>(mMenuHandle), inPos, TRUE, &menuInfo);
  AddHotkey(inPos);
}

void COsMenu::AddSeparator(int inPos) {
  MENUITEMINFOA menuInfo;
  menuInfo.cbSize = sizeof(menuInfo);
  menuInfo.fMask = MIIM_TYPE | MIIM_ID;
  menuInfo.fType = MFT_SEPARATOR;
  menuInfo.wID = 0xFFFF;
  InsertMenuItemA(static_cast<HMENU>(mMenuHandle), inPos, TRUE, &menuInfo);
  AddHotkey(inPos);
}

void COsMenu::EnableItem(int inPos, int inVal) {
  MENUITEMINFOA menuInfo;
  menuInfo.cbSize = sizeof(menuInfo);
  menuInfo.fMask = MIIM_STATE;
  GetMenuItemInfoA(static_cast<HMENU>(mMenuHandle), inPos, TRUE, &menuInfo);
  if (inVal) {
    menuInfo.fState &= ~(MFS_DISABLED | MFS_GRAYED);
  } else {
    menuInfo.fState |= MFS_DISABLED | MFS_GRAYED;
  }
  SetMenuItemInfoA(static_cast<HMENU>(mMenuHandle), inPos, TRUE, &menuInfo);
}

void COsMenu::CheckItem(int inPos, int inVal) {
  MENUITEMINFOA menuInfo;
  menuInfo.cbSize = sizeof(menuInfo);
  menuInfo.fMask = MIIM_STATE;
  GetMenuItemInfoA(static_cast<HMENU>(mMenuHandle), inPos, TRUE, &menuInfo);
  if (inVal) {
    menuInfo.fState |= MFS_CHECKED;
  } else {
    menuInfo.fState &= ~MFS_CHECKED;
  }
  SetMenuItemInfoA(static_cast<HMENU>(mMenuHandle), inPos, TRUE, &menuInfo);
}

void COsMenu::SetItemText(int inPos, LPCSTR inText) {
  char itemText[260];
  char oldText[260];
  SStrCopy(itemText, inText, 0x7FFFFFFF);
  OsGuiMenuHotkey hotkey;
  if (GetHotkey(inPos, &hotkey)) {
    AppendHotkeyText(itemText, hotkey);
  }

  MENUITEMINFOA menuInfo;
  menuInfo.cbSize = sizeof(menuInfo);
  menuInfo.fMask = 0x40;
  menuInfo.dwTypeData = oldText;
  menuInfo.cch = sizeof(oldText);
  GetMenuItemInfoA(static_cast<HMENU>(mMenuHandle), inPos, TRUE, &menuInfo);
  if (SStrCmp(oldText, itemText, 0x7FFFFFFF)) {
    menuInfo.dwTypeData = itemText;
    SetMenuItemInfoA(static_cast<HMENU>(mMenuHandle), inPos, TRUE, &menuInfo);
  }
}

COsMenuBar::COsMenuBar(LPVOID inWindowHandle) {
  mWindowHandle = inWindowHandle;
  mMenuBarHandle = CreateMenu();
  SetMenu(static_cast<HWND>(mWindowHandle), static_cast<HMENU>(mMenuBarHandle));
  mAccelerators = 0;
  *sMenubars.New() = this;
}

COsMenuBar::~COsMenuBar() {
  DestroyMenu(static_cast<HMENU>(mMenuBarHandle));
  if (mAccelerators) {
    DestroyAcceleratorTable(static_cast<HACCEL>(mAccelerators));
  }

  int  id = -1;
  UINT i;
  for (i = 0; i < sMenubars.Count(); ++i) {
    if (sMenubars[i] == this) {
      id = i;
      break;
    }
  }
  ASSERT(id != -1);
  sMenubars[id] = sMenubars[sMenubars.Count() - 1];
  sMenubars.SetCount(sMenubars.Count() - 1);
}

void COsMenuBar::Set(TSGrowableArray<COsMenu *> &inMenus) {
  mMenus = inMenus;
  for (UINT i = 0; i < mMenus.Count(); ++i) {
    COsMenu      *menu = mMenus[i];
    MENUITEMINFOA menuInfo;
    menuInfo.cbSize = sizeof(menuInfo);
    menuInfo.fMask = MIIM_TYPE;
    menuInfo.fType = MFT_STRING;
    menuInfo.dwTypeData = menu->GetTitle();
    InsertMenuItemA(static_cast<HMENU>(mMenuBarHandle), i, TRUE, &menuInfo);
    menuInfo.fMask = MIIM_SUBMENU;
    menuInfo.hSubMenu = static_cast<HMENU>(menu->GetMenuHandle());
    SetMenuItemInfoA(static_cast<HMENU>(mMenuBarHandle), i, TRUE, &menuInfo);
  }
  UpdateAccelerators();
  Refresh();
}

void COsMenuBar::UpdateAccelerators() {
  if (mAccelerators) {
    DestroyAcceleratorTable(static_cast<HACCEL>(mAccelerators));
    mAccelerators = 0;
  }

  TSGrowableArray<ACCEL> accels;
  for (int index = 0; index < mMenus.Count(); ++index) {
    COsMenu *menu = mMenus[index];
    for (int m = 0; m < menu->GetNumItems(); ++m) {
      OsGuiMenuHotkey hotkey;
      if (menu->GetHotkey(m, &hotkey)) {
        ACCEL accEntry;
        sHotkeyToAccel(&hotkey, &accEntry);
        accEntry.cmd = static_cast<WORD>((menu->GetID() << 8) | m);
        *accels.New() = accEntry;
      }
    }
  }

  if (accels.Count()) {
    mAccelerators = CreateAcceleratorTableA(accels.Ptr(), accels.Count());
    ASSERT(mAccelerators != 0);
  }
}

void COsMenuBar::Refresh() {
  DrawMenuBar(static_cast<HWND>(mWindowHandle));
}

COsDialog::COsDialog(LPVOID inWindowHandle, UINT inFlags) {
  mCallback = 0;
  mCallbackParam = 0;
  mTooltips = 0;
  mCancelButton = 0;
  mTrackMouse = 0;
  mTooltipsEnabled = sMasterTooltipsEnabled;
  mMinSize.x = -1;
  mMinSize.y = -1;
  mContextMenu = 0;
  mContextMenuEnabled = 1;
  mFlags = inFlags;

  CBasicDlgTemplate dlgTemplate(10, 10);
  if (inFlags & 0x2) {
    dlgTemplate.header.style = 0x00CF0000;
  }
  if (inFlags & 0x1) {
    dlgTemplate.header.style |= 0x00040000;
  }
  if (inFlags & 0x8) {
    dlgTemplate.header.style |= 0x00080000;
  }

  ASSERT(sAppInstance != 0);
  mHandle = CreateDialogIndirectParamA(sAppInstance, &dlgTemplate.header, static_cast<HWND>(inWindowHandle), sDlgProc, 0);
  sSetOsGuiPointer(static_cast<HWND>(mHandle), this);
  *sDialogs.New() = this;
  ApplyModality(1);
}

COsDialog::~COsDialog() {
  ApplyModality(0);

  UINT i;
  for (i = 0; i < mControls.Count(); ++i) {
    DeleteControl(mControls[i]);
  }
  mControls.Clear();

  DELIFUSED(mContextMenu);
  if (mTooltips) {
    DestroyWindow(static_cast<HWND>(mTooltips));
  }
  sRemoveOsGuiPointer(static_cast<HWND>(mHandle));
  DestroyWindow(static_cast<HWND>(mHandle));

  int id = -1;
  for (i = 0; i < sDialogs.Count(); ++i) {
    if (sDialogs[i] == this) {
      id = i;
      break;
    }
  }
  ASSERT(id != -1);
  sDialogs[id] = sDialogs[sDialogs.Count() - 1];
  sDialogs.SetCount(sDialogs.Count() - 1);
}

void COsDialog::ApplyModality(int inVal) {
  if (inVal) {
    FATALASSERT(mDisabledWindows.Count() == 0);
    if (mFlags & 0x4) {
      HWND parent = static_cast<HWND>(GetParentWindow());
      if (parent) {
        sDisableWindow(parent, reinterpret_cast<long>(&mDisabledWindows));
      }
    }
  } else {
    for (UINT i = 0; i < mDisabledWindows.Count(); ++i) {
      EnableWindow(static_cast<HWND>(mDisabledWindows[i]), TRUE);
    }
    mDisabledWindows.Clear();
  }
}

LPVOID COsDialog::GetTooltips() {
  if (!mTooltips) {
    mTooltips = sCreateTooltips(0);
  }
  return mTooltips;
}

void COsDialog::AddControl(COsControl *inControl) {
  *mControls.New() = inControl;
  if (GetParent(static_cast<HWND>(inControl->mHandle)) != mHandle) {
    SetParent(static_cast<HWND>(inControl->mHandle), static_cast<HWND>(mHandle));
  }
}

void COsDialog::EnableTooltips(int inVal) {
  mTooltipsEnabled = inVal;
  if (mTooltips) {
    SendMessageA(static_cast<HWND>(mTooltips), TTM_ACTIVATE, inVal, 0);
  }
}

int COsDialog::FindControl(COsControl *inControl) {
  for (UINT i = 0; i < mControls.Count(); ++i) {
    if (mControls[i] == inControl) {
      return i;
    }
  }
  return -1;
}

void COsDialog::DeleteControl(COsControl *inControl) {
  DetachControl(inControl);
  inControl->OnDestroy();
  sRemoveOsGuiPointer(static_cast<HWND>(inControl->mHandle));
  DestroyWindow(static_cast<HWND>(inControl->mHandle));
  DEL(inControl);
}

void COsDialog::DetachControl(COsControl *inControl) {
  int index = FindControl(inControl);
  if (index != -1) {
    mControls[index] = mControls[mControls.Count() - 1];
    mControls.SetCount(mControls.Count() - 1);
  }
}

LPVOID COsDialog::GetParentWindow() {
  return GetParent(static_cast<HWND>(mHandle));
}

COsControl *COsDialog::FindControl(LPVOID inHandle) {
  UINT index = 0;

  if (!mControls.Count()) {
    return 0;
  }

  while (index < mControls.Count()) {
    COsControl *control = mControls[index];
    if (control->IsHandleFromControl(inHandle)) {
      break;
    }

    ++index;
  }

  if (index >= mControls.Count()) {
    return 0;
  }

  return mControls[index];
}

int COsDialog::ProcessMessage(LPVOID inMsgData) {
  MSG        *message = static_cast<MSG *>(inMsgData);
  COsControl *control;

  if (message->wParam == VK_ESCAPE && message->message == WM_KEYDOWN) {
    control = FindControl(message->hwnd);
    if (control && control->GetType() == 4 && (GetWindowLongA(static_cast<HWND>(control->GetHandle()), GWL_STYLE) & 0x04)) {
      return OnCancel();
    }
  }

  if (message->message == WM_CHAR) {
    control = FindControl(message->hwnd);
    if (control) {
      char c = static_cast<char>(message->wParam);
      int  allowed;

      if (control->GetType() == 4) {
        allowed = static_cast<COsEditBox *>(control)->IsCharacterAllowed(c);
      } else if (control->GetType() == 10) {
        allowed = static_cast<COsTreeView *>(control)->IsCharacterAllowed(c);
      } else {
        allowed = 1;
      }

      if (!allowed) {
        OsGuiBeep();
        return 1;
      }
    }
  }

  if (message->wParam == VK_RETURN && message->message == WM_KEYDOWN) {
    control = FindControl(message->hwnd);

    if (control && (control->GetType() == 6 || control->GetType() == 10 || control->GetType() == 16) && control->OnReturn()) {
      return 1;
    }
  }

  if (IsInFront() && message->wParam == VK_TAB && message->message == WM_KEYDOWN && OsGuiIsModifierKeyDown(0) && OnControlTab()) {
    return 1;
  }

  return IsDialogMessageA(static_cast<HWND>(mHandle), message);
}

void COsDialog::CheckEvents() {
  MSG message;
  if (PeekMessageA(&message, 0, 0, 0, PM_NOREMOVE)) {
    GetMessageA(&message, 0, 0, 0);
    ProcessMessage(&message);
  }
}

void COsDialog::SetTrackMouse(int inVal) {
  mTrackMouse = inVal;
  if (inVal) {
    mMouseInside = IsMouseInside();
    mNeedNewTrack = 1;
  }
}

BOOL COsDialog::IsMouseInside() {
  POINT cursPoint;
  RECT  dlgRect;
  return GetCursorPos(&cursPoint) && GetWindowRect(static_cast<HWND>(mHandle), &dlgRect) && cursPoint.x > dlgRect.left &&
         cursPoint.x < dlgRect.right && cursPoint.y > dlgRect.top && cursPoint.y < dlgRect.bottom;
}

void COsDialog::SetCallback(void (*inFunc)(const OsGuiCallbackParams &), LPVOID inParam) {
  mCallback = inFunc;
  mCallbackParam = inParam;
}

void COsDialog::BringToFront() {
  HWND window = GetWindow(static_cast<HWND>(mHandle), 6);
  if (!window) {
    window = static_cast<HWND>(mHandle);
  }
  OsGuiBringWindowToFront(window);
}

BOOL COsDialog::IsInFront() {
  return mHandle == OsGuiGetWindow(2);
}

void COsDialog::SetInputFocus() {
  SetFocus(static_cast<HWND>(mHandle));
}

void COsDialog::Show(int inVal) {
  ShowWindow(static_cast<HWND>(mHandle), inVal ? SW_SHOW : SW_HIDE);
}

BOOL COsDialog::IsShowing() {
  return IsWindowVisible(static_cast<HWND>(mHandle));
}

BOOL COsDialog::IsEnabled() {
  return IsWindowEnabled(static_cast<HWND>(mHandle));
}

void COsDialog::SetRedraw(int inVal) {
  SendMessageA(static_cast<HWND>(mHandle), WM_SETREDRAW, inVal, 0);
}

void COsDialog::Refresh(int inErase) {
  if (IsShowing()) {
    InvalidateRect(static_cast<HWND>(mHandle), 0, inErase);
  }
}

void COsDialog::SetPosition(int inX, int inY) {
  SetWindowPos(static_cast<HWND>(mHandle), 0, inX, inY, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
}

void COsDialog::GetPosition(int *outX, int *outY, int inClient) {
  WINDOWINFO_TIGULE wInfo;
  wInfo.cbSize = sizeof(wInfo);
  GetWindowInfo(static_cast<HWND>(mHandle), &wInfo);
  const RECT &rect = inClient ? wInfo.rcClient : wInfo.rcWindow;
  *outX = rect.left;
  *outY = rect.top;
}

void COsDialog::SetSize(int inW, int inH) {
  SetWindowPos(static_cast<HWND>(mHandle), 0, 0, 0, inW, inH, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
}

void COsDialog::GetSize(int *outW, int *outH, int inClientOnly) {
  NTempest::CiRect rect = OsGuiGetWindowRect(mHandle, inClientOnly);
  *outW = rect.r - rect.l;
  *outH = rect.b - rect.t;
}

void COsDialog::SetMinSize(int inW, int inH) {
  mMinSize.x = inW;
  mMinSize.y = inH;
}

BOOL COsDialog::GetMinSize(int *outW, int *outH) {
  *outW = mMinSize.x;
  *outH = mMinSize.y;
  return mMinSize.x > -1 || mMinSize.y > -1;
}

void COsDialog::SetTitle(LPCSTR inText) {
  SetWindowTextA(static_cast<HWND>(mHandle), inText);
}

BOOL COsDialog::OnAccept() {
  return 1;
}

int COsDialog::OnCancel() {
  if (!mCancelButton) {
    return 1;
  }

  if (mCancelButton->OnEvent(mCancelButton->GetID(), 0, 0)) {
    return 1;
  }

  return OnEvent(mCancelButton->GetID(), 0, 0);
}

BOOL COsDialog::OnMouseDown() {
  for (UINT i = 0; i < mControls.Count(); ++i) {
    mControls[i]->OnMouseDown();
  }
  return 0;
}

BOOL COsDialog::OnMouseUp() {
  for (UINT i = 0; i < mControls.Count(); ++i) {
    mControls[i]->OnMouseUp();
  }
  return 0;
}

int COsDialog::OnMouseLeave() {
  if (mTrackMouse) {
    BOOL wasInside = mMouseInside;
    mNeedNewTrack = 1;
    mMouseInside = IsMouseInside();
    if (!mMouseInside && wasInside) {
      return OnEvent(-1, 4, 0);
    }
  }
  return 0;
}

BOOL COsDialog::OnMouseMove(int inX, int inY) {
  for (UINT i = 0; i < mControls.Count(); ++i) {
    mControls[i]->OnMouseMove(inX, inY);
  }

  if (!mTrackMouse) {
    return 0;
  }

  BOOL wasInside = mMouseInside;
  mMouseInside = IsMouseInside();
  if (mNeedNewTrack) {
    TRACKMOUSEEVENT trackInfo;
    trackInfo.cbSize = sizeof(trackInfo);
    trackInfo.dwFlags = TME_LEAVE;
    trackInfo.hwndTrack = static_cast<HWND>(mHandle);
    trackInfo.dwHoverTime = 0;
    ASSERT(_TrackMouseEvent(&trackInfo));
    mNeedNewTrack = 0;
  }

  if (mMouseInside && !wasInside) {
    return OnEvent(-1, 3, 0);
  }
  return 0;
}

BOOL COsDialog::OnControlTab() {
  int  controlCount = static_cast<int>(mControls.Count());
  UINT index = 0;

  if (controlCount <= 0) {
    return 0;
  }

  while (static_cast<int>(index) < controlCount) {
    if (mControls[index]->GetType() == 15) {
      break;
    }

    ++index;
  }

  if (static_cast<int>(index) >= controlCount) {
    return 0;
  }

  COsControl *control = mControls[index];
  if (!control) {
    return 0;
  }

  return static_cast<COsTabControl *>(control)->OnControlTab();
}

BOOL COsDialog::HasFlag(UINT inFlag) {
  return (mFlags & inFlag) != 0;
}

void COsDialog::SetContextMenu(COsMenu *inMenu) {
  DELIFUSED(mContextMenu);
  mContextMenu = inMenu;
  ASSERT(mContextMenu->GetID() == 0xFF);
}

int COsDialog::OnContextMenu(int inX, int inY) {
  if (!mContextMenuEnabled || !mContextMenu) {
    return 0;
  }

  int itemID =
      TrackPopupMenu(static_cast<HMENU>(mContextMenu->GetMenuHandle()), TPM_RETURNCMD | TPM_NONOTIFY, inX, inY, 0, static_cast<HWND>(mHandle), 0);
  return itemID ? OnEvent(-3, itemID | 0xFFFF0000, 0) : 0;
}

BOOL COsDialog::CanDoClipboardAction(int inAction) {
  COsControl *control = FindControl(GetFocus());
  return control ? control->CanDoClipboardAction(inAction) : 0;
}

BOOL COsDialog::DoClipboardAction(int inAction) {
  COsControl *control = FindControl(GetFocus());
  return control ? control->DoClipboardAction(inAction) : 0;
}

COsButton::COsButton(COsDialog *inDialog, short inID, UINT inFlags) : COsControl(inDialog, 0, inID, inFlags) {
}

void COsButton::SetDefaultButton() {
  if (mDialog) {
    SendMessageA(static_cast<HWND>(mDialog->GetHandle()), DM_SETDEFID, mID, 0);
  }
}

void COsButton::SetCancelButton() {
  if (mDialog) {
    mDialog->SetCancelButton(this);
  }
}

void COsButton::SetHighlight(int inVal) {
  int enabled = IsEnabled();
  Enable(0);
  SendMessageA(static_cast<HWND>(mHandle), BM_SETSTATE, inVal, 0);
  Enable(enabled);
}

COsCheckbox::COsCheckbox(COsDialog *inDialog, short inID) : COsControl(inDialog, 7, inID, 0), mSettingSize(0), mMaxWidth(0) {
  OnTextChange();
}

COsCheckbox::COsCheckbox(LPVOID inWindow, short inID) : COsControl(inWindow, 7, inID, 0), mSettingSize(0), mMaxWidth(0) {
  OnTextChange();
}

void COsCheckbox::SetMaxWidth(int inWidth) {
  mMaxWidth = inWidth;
  OnTextChange();
}

void COsCheckbox::SetValue(int inVal) {
  SendMessageA(static_cast<HWND>(mHandle), BM_SETCHECK, inVal != 0, 0);
}

BOOL COsCheckbox::GetValue() {
  return SendMessageA(static_cast<HWND>(mHandle), BM_GETCHECK, 0, 0) == BST_CHECKED;
}

void COsCheckbox::ClearValue() {
  SendMessageA(static_cast<HWND>(mHandle), BM_SETCHECK, BST_INDETERMINATE, 0);
}

BOOL COsCheckbox::HasValue() {
  return SendMessageA(static_cast<HWND>(mHandle), BM_GETCHECK, 0, 0) != BST_INDETERMINATE;
}

BOOL COsCheckbox::OnEvent(int inItemID, int inNotifyCode, int inCode) {
  if (inNotifyCode == 2 && !HasValue()) {
    SetValue(0);
  }
  return COsControl::OnEvent(inItemID, inNotifyCode, inCode);
}

void COsCheckbox::OnTextChange() {
  int textW;
  GetTextSize(&textW, 0);
  textW += 20;
  if (mMaxWidth > 0 && textW >= mMaxWidth) {
    textW = mMaxWidth;
  }

  int ctrlW;
  int ctrlH;
  GetSize(&ctrlW, &ctrlH);
  if (ctrlW != textW || ctrlH != 16) {
    mSettingSize = 1;
    SetSize(textW, 16);
    mSettingSize = 0;
  }
}

void COsCheckbox::OnSizeChange() {
  if (!mSettingSize) {
    char text[260];
    GetText(text, sizeof(text));
    OsOutputDebugString("Unnecessary COsCheckbox::SetSize for '%s'\n", text);
    OnTextChange();
  }
}

COsEditBox::COsEditBox(COsDialog *inDialog, short inID, UINT inFlags) : COsControl(inDialog, 4, inID, inFlags) {
  Initialize();
}

COsEditBox::COsEditBox(LPVOID inWindow, short inID, UINT inFlags) : COsControl(inWindow, 4, inID, inFlags) {
  Initialize();
}

void COsEditBox::Initialize() {
  mFiltersEnabled = 0;
  mFilters = 0;
  mSelSize = 0;
  SetWindowLongA(static_cast<HWND>(mHandle), GWL_WNDPROC, reinterpret_cast<LONG>(sEditBoxProc));
}

void COsEditBox::UpdateSelection() {
  int selectionSize = GetSelectionSize();
  if (selectionSize != mSelSize) {
    mSelSize = selectionSize;
    SendEvent(19, 0);
  }
}

BOOL COsEditBox::OnReturn() {
  return SendEvent(12, 0);
}

void COsEditBox::SetTextLimit(int inSize) {
  SendMessageA(static_cast<HWND>(mHandle), EM_LIMITTEXT, inSize, 0);
}

void COsEditBox::SelectAll() {
  SendMessageA(static_cast<HWND>(mHandle), EM_SETSEL, 0, -1);
}

int COsEditBox::GetSelectionSize() {
  UINT selStart;
  UINT selEnd;
  SendMessageA(static_cast<HWND>(mHandle), EM_GETSEL, reinterpret_cast<WPARAM>(&selStart), reinterpret_cast<LPARAM>(&selEnd));
  return selEnd - selStart;
}

void COsEditBox::EnableFilters(int inVal) {
  mFiltersEnabled = inVal;
}

void COsEditBox::SetFilter(UINT inFilter, int inVal) {
  if (inVal) {
    mFilters |= inFilter;
  } else {
    mFilters &= ~inFilter;
  }
}

BOOL COsEditBox::CanDoClipboardAction(int inAction) {
  switch (inAction) {
    case 0:
    case 1:
    case 3:
      return GetSelectionSize() > 0;
    case 2: {
      if (!OpenClipboard(static_cast<HWND>(mHandle))) {
        return 0;
      }
      int result = GetClipboardData(CF_TEXT) != 0;
      CloseClipboard();
      return result;
    }
    case 4:
      return 1;
    case 5:
      return SendMessageA(static_cast<HWND>(mHandle), EM_CANUNDO, 0, 0);
    default:
      return 0;
  }
}

BOOL COsEditBox::DoClipboardAction(int inAction) {
  switch (inAction) {
    case 0:
      SendMessageA(static_cast<HWND>(mHandle), WM_CUT, 0, 0);
      return 1;
    case 1:
      SendMessageA(static_cast<HWND>(mHandle), WM_COPY, 0, 0);
      return 1;
    case 2:
      SendMessageA(static_cast<HWND>(mHandle), WM_PASTE, 0, 0);
      return 1;
    case 3:
      SendMessageA(static_cast<HWND>(mHandle), WM_CLEAR, 0, 0);
      return 1;
    case 4:
      SelectAll();
      return 1;
    case 5:
      SendMessageA(static_cast<HWND>(mHandle), EM_UNDO, 0, 0);
      return 1;
    default:
      return 0;
  }
}

COsListBox::COsListBox(COsDialog *inDialog, short inID, UINT inFlags) : COsControl(inDialog, 6, inID, inFlags) {
}

COsListBox::~COsListBox() {
}

void COsListBox::SetValue(int inVal) {
  FATALASSERT((mFlags & 0x10000) == 0);
  SendMessageA(static_cast<HWND>(mHandle), LB_SETCURSEL, inVal, 0);
}

int COsListBox::GetValue() {
  FATALASSERT((mFlags & 0x10000) == 0);
  return SendMessageA(static_cast<HWND>(mHandle), LB_GETCURSEL, 0, 0);
}

void COsListBox::SelectItem(int inPos, int inVal) {
  FATALASSERT((mFlags & 0x10000) != 0);
  SendMessageA(static_cast<HWND>(mHandle), LB_SETSEL, inVal, inPos);
}

BOOL COsListBox::IsItemSelected(int inPos) {
  FATALASSERT((mFlags & 0x10000) != 0);
  return SendMessageA(static_cast<HWND>(mHandle), LB_GETSEL, inPos, 0);
}

void COsListBox::SelectAll(int inVal) {
  FATALASSERT((mFlags & 0x10000) != 0);
  int count = GetNumItems();
  for (int i = 0; i < count; ++i) {
    SelectItem(i, inVal);
  }
}

void COsListBox::ClearItems() {
  SendMessageA(static_cast<HWND>(mHandle), LB_RESETCONTENT, 0, 0);
}

int COsListBox::GetNumItems() {
  return SendMessageA(static_cast<HWND>(mHandle), LB_GETCOUNT, 0, 0);
}

void COsListBox::InsertItem(LPCSTR inText, int inPos) {
  SendMessageA(static_cast<HWND>(mHandle), LB_INSERTSTRING, inPos, reinterpret_cast<LPARAM>(inText));
}

void COsListBox::DeleteItem(int inPos) {
  if (inPos == -1) {
    inPos = GetNumItems() - 1;
  }
  SendMessageA(static_cast<HWND>(mHandle), LB_DELETESTRING, inPos, 0);
}

void COsListBox::SetItemText(int inPos, LPCSTR inText) {
  DeleteItem(inPos);
  InsertItem(inText, inPos);
}

int COsListBox::GetItemTextLength(int inPos) {
  return SendMessageA(static_cast<HWND>(mHandle), LB_GETTEXTLEN, inPos, 0);
}

void COsListBox::GetItemText(int inPos, char *inBuf, int inBufSize) {
  int textLength = GetItemTextLength(inPos);
  FATALASSERT(inBufSize > textLength);
  SendMessageA(static_cast<HWND>(mHandle), LB_GETTEXT, inPos, reinterpret_cast<LPARAM>(inBuf));
}

void COsListBox::SetItemHeight(int inHeight) {
  SendMessageA(static_cast<HWND>(mHandle), LB_SETITEMHEIGHT, 0, inHeight);
}

int COsListBox::GetItemHeight() {
  return SendMessageA(static_cast<HWND>(mHandle), LB_GETITEMHEIGHT, 0, 0);
}

int COsListBox::OnContextMenu(int inX, int inY) {
  int posX;
  int posY;
  GetPosition(&posX, &posY, 0);
  int item = SendMessageA(static_cast<HWND>(mHandle), LB_ITEMFROMPOINT, 0, MAKELPARAM(inX - posX, inY - posY));
  if (item < 0 || item >= GetNumItems()) {
    return 0;
  }

  if (mFlags & 0x10000) {
    SelectAll(0);
    SelectItem(item, 1);
  } else {
    int oldItem = GetValue();
    SetValue(item);
    if (oldItem == item) {
      return COsControl::OnContextMenu(inX, inY);
    }
  }
  SendEvent(2, 0);
  return COsControl::OnContextMenu(inX, inY);
}

BOOL COsListBox::OnReturn() {
  return SendEvent(9, 0);
}

COsListView::COsListView(COsDialog *inDialog, short inID, UINT inFlags) : COsControl(inDialog, 16, inID, inFlags) {
  SendMessageA(static_cast<HWND>(mHandle), 0x1036, 0x20, 0x20);

  HWND header = reinterpret_cast<HWND>(SendMessageA(static_cast<HWND>(mHandle), 0x101F, 0, 0));
  sSetOsGuiPointer(header, this);
  if (!(mFlags & 0x10000)) {
    SetWindowLongA(header, GWL_STYLE, GetWindowLongA(header, GWL_STYLE) & ~2);
  }

  mNumCols = 0;
}

COsListView::~COsListView() {
}

void COsListView::InsertColumn(int inPos) {
  if (inPos == -1) {
    inPos = mNumCols;
  }

  LVCOLUMNA colInfo;
  memset(&colInfo, 0, sizeof(colInfo));
  colInfo.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
  colInfo.cx = 50;
  colInfo.pszText = "";
  colInfo.iSubItem = inPos;
  SendMessageA(static_cast<HWND>(mHandle), LVM_INSERTCOLUMNA, inPos, reinterpret_cast<LPARAM>(&colInfo));
  ++mNumCols;
}

void COsListView::DeleteColumn(int inPos) {
  SendMessageA(static_cast<HWND>(mHandle), LVM_DELETECOLUMN, inPos, 0);
  --mNumCols;
}

int COsListView::GetNumColumns() {
  return mNumCols;
}

void COsListView::InsertRow(int inPos) {
  if (inPos == -1) {
    inPos = GetNumRows();
  }

  LVITEMA item;
  memset(&item, 0, sizeof(item));
  item.mask = LVIF_PARAM;
  item.iItem = inPos;
  SendMessageA(static_cast<HWND>(mHandle), LVM_INSERTITEMA, 0, reinterpret_cast<LPARAM>(&item));
}

void COsListView::DeleteRow(int inPos) {
  SendMessageA(static_cast<HWND>(mHandle), LVM_DELETEITEM, inPos, 0);
}

void COsListView::ClearRows() {
  SendMessageA(static_cast<HWND>(mHandle), LVM_DELETEALLITEMS, 0, 0);
}

int COsListView::GetNumRows() {
  return SendMessageA(static_cast<HWND>(mHandle), LVM_GETITEMCOUNT, 0, 0);
}

void COsListView::SetRowColor(int inPos, const NTempest::CImVector &inColor) {
  LVITEMA item;
  memset(&item, 0, sizeof(item));
  item.mask = LVIF_PARAM;
  item.iItem = inPos;
  item.lParam = *inColor.IV_();
  SendMessageA(static_cast<HWND>(mHandle), LVM_SETITEMA, 0, reinterpret_cast<LPARAM>(&item));
}

NTempest::CImVector COsListView::GetRowColor(int inPos) {
  LVITEMA item;
  memset(&item, 0, sizeof(item));
  item.mask = LVIF_PARAM;
  item.iItem = inPos;
  SendMessageA(static_cast<HWND>(mHandle), LVM_GETITEMA, 0, reinterpret_cast<LPARAM>(&item));
  return NTempest::CImVector(static_cast<DWORD>(item.lParam));
}

void COsListView::SetItemText(int inRow, int inCol, LPCSTR inText) {
  LVITEMA item;
  memset(&item, 0, sizeof(item));
  item.mask = LVIF_TEXT;
  item.iItem = inRow;
  item.iSubItem = inCol;
  item.pszText = const_cast<char *>(inText);
  SendMessageA(static_cast<HWND>(mHandle), LVM_SETITEMA, 0, reinterpret_cast<LPARAM>(&item));
}

void COsListView::GetItemText(int inRow, int inCol, char *inBuf, int inBufSize) {
  LVITEMA item;
  memset(&item, 0, sizeof(item));
  item.mask = LVIF_TEXT;
  item.iItem = inRow;
  item.iSubItem = inCol;
  item.pszText = inBuf;
  item.cchTextMax = inBufSize;
  SendMessageA(static_cast<HWND>(mHandle), LVM_GETITEMA, 0, reinterpret_cast<LPARAM>(&item));
}

void COsListView::SetColumnWidth(int inCol, int inWidth) {
  SendMessageA(static_cast<HWND>(mHandle), LVM_SETCOLUMNWIDTH, inCol, static_cast<WORD>(inWidth));
}

int COsListView::GetColumnWidth(int inCol) {
  return SendMessageA(static_cast<HWND>(mHandle), LVM_GETCOLUMNWIDTH, inCol, 0);
}

void COsListView::SetColumnTitle(int inCol, LPCSTR inText) {
  LVCOLUMNA colInfo;
  memset(&colInfo, 0, sizeof(colInfo));
  colInfo.mask = LVCF_TEXT;
  colInfo.pszText = const_cast<char *>(inText);
  SendMessageA(static_cast<HWND>(mHandle), LVM_SETCOLUMNA, inCol, reinterpret_cast<LPARAM>(&colInfo));
}

void COsListView::GetColumnTitle(int inCol, char *inBuf, int inBufSize) {
  LVCOLUMNA colInfo;
  memset(&colInfo, 0, sizeof(colInfo));
  colInfo.mask = LVCF_TEXT;
  colInfo.pszText = inBuf;
  colInfo.cchTextMax = inBufSize;
  SendMessageA(static_cast<HWND>(mHandle), LVM_GETCOLUMNA, inCol, reinterpret_cast<LPARAM>(&colInfo));
}

void COsListView::SetColumnJustification(int inCol, int inJustify) {
  int format;
  switch (inJustify) {
    case 0:
      format = LVCFMT_LEFT;
      break;
    case 1:
      format = LVCFMT_RIGHT;
      break;
    case 2:
      format = LVCFMT_CENTER;
      break;
    default:
      return;
  }

  LVCOLUMNA colInfo;
  memset(&colInfo, 0, sizeof(colInfo));
  colInfo.mask = LVCF_FMT;
  colInfo.fmt = format;
  SendMessageA(static_cast<HWND>(mHandle), LVM_SETCOLUMNA, inCol, reinterpret_cast<LPARAM>(&colInfo));
}

void COsListView::EnsureRowVisible(int inRow) {
  SendMessageA(static_cast<HWND>(mHandle), LVM_ENSUREVISIBLE, inRow, 0);
}

void COsListView::SetValue(int inVal) {
  if (inVal < 0 || inVal >= GetNumRows()) {
    inVal = -1;
  }

  LVITEMA item;
  memset(&item, 0, sizeof(item));
  item.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
  item.state = inVal == -1 ? 0 : LVIS_FOCUSED | LVIS_SELECTED;
  SendMessageA(static_cast<HWND>(mHandle), LVM_SETITEMSTATE, inVal, reinterpret_cast<LPARAM>(&item));
}

int COsListView::GetValue() {
  int numRows = GetNumRows();
  for (int i = 0; i < numRows; ++i) {
    if (SendMessageA(static_cast<HWND>(mHandle), LVM_GETITEMSTATE, i, LVIS_SELECTED)) {
      return i;
    }
  }
  return -1;
}

void COsListView::OnSizeChange() {
}

void COsListView::OnSelectionChange() {
  SendEvent(2, 0);
}

void COsListView::OnColumnClick(int inCol) {
  SendEvent(15, inCol);
}

int COsListView::OnNotify(int inCode, LPVOID inParam) {
  if (inCode == LVN_COLUMNCLICK) {
    OnColumnClick(static_cast<NMLISTVIEW *>(inParam)->iSubItem);
    return 1;
  }

  if (inCode == -301) {
    return SendEvent(16, 0);
  }

  if (inCode == LVN_KEYDOWN) {
    WORD key = static_cast<NMLVKEYDOWN *>(inParam)->wVKey;
    if (key == VK_RETURN) {
      return OnReturn();
    }
    if (key == VK_DELETE) {
      return SendEvent(10, 0);
    }
  } else if (inCode == LVN_ITEMCHANGED) {
    NMLISTVIEW *listInfo = static_cast<NMLISTVIEW *>(inParam);
    if ((listInfo->uChanged & LVIF_STATE) && ((listInfo->uOldState ^ listInfo->uNewState) & LVIS_SELECTED)) {
      OnSelectionChange();
      return 1;
    }
  } else if (inCode == NM_CUSTOMDRAW) {
    NMLVCUSTOMDRAW *drawInfo = static_cast<NMLVCUSTOMDRAW *>(inParam);
    if (drawInfo->nmcd.dwDrawStage == CDDS_PREPAINT) {
      if (mDialog) {
        SetWindowLongA(static_cast<HWND>(mDialog->GetHandle()), DWL_MSGRESULT, CDRF_NOTIFYITEMDRAW);
      }
      return CDRF_NOTIFYITEMDRAW;
    }

    if (drawInfo->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
      inCode = *GetRowColor(static_cast<int>(drawInfo->nmcd.dwItemSpec)).IV_();
      if (inCode & 0xFF000000) {
        drawInfo->clrText = RGB((inCode >> 16) & 0xFF, (inCode >> 8) & 0xFF, inCode & 0xFF);
      }
      inCode = NM_CUSTOMDRAW;
    }
  }

  return COsControl::OnNotify(inCode, inParam);
}

BOOL COsListView::OnReturn() {
  return SendEvent(9, 0);
}

COsToolBar::COsToolBar(COsDialog *inDialog, short inID, UINT inFlags) : COsControl(inDialog, 17, inID, inFlags) {
  InitializeToolBar();
}

COsToolBar::COsToolBar(LPVOID inWindow, short inID, UINT inFlags) : COsControl(inWindow, 17, inID, inFlags) {
  InitializeToolBar();
}

COsToolBar::~COsToolBar() {
  ImageList_Destroy(static_cast<HIMAGELIST>(mImageList));
}

void COsToolBar::InitializeToolBar() {
  SendMessageA(static_cast<HWND>(mHandle), 0x454, 0, 8);
  mImageList = ImageList_Create(16, 16, 0x21, 1, 1);
  SendMessageA(static_cast<HWND>(mHandle), 0x430, 0, reinterpret_cast<LPARAM>(mImageList));
}

void COsToolBar::SetButtonSize(int inW, int inH) {
  SendMessageA(static_cast<HWND>(mHandle), 0x41F, 0, MAKELPARAM(inW, inH));
}

void COsToolBar::GetButtonSize(int *outW, int *outH) {
  UINT size = SendMessageA(static_cast<HWND>(mHandle), 0x43A, 0, 0);
  *outW = LOWORD(size);
  *outH = HIWORD(size);
}

void COsToolBar::Clear() {
  for (int i = GetNumButtons(); i >= 0; --i) {
    RemoveButton(i);
  }
}

void COsToolBar::AddButton(int inPos) {
  if (inPos == -1) {
    inPos = GetNumButtons();
  }

  TBBUTTON buttonInfo;
  memset(&buttonInfo, 0, sizeof(buttonInfo));
  buttonInfo.iBitmap = -2;
  buttonInfo.idCommand = inPos;
  buttonInfo.fsState = TBSTATE_ENABLED;
  SendMessageA(static_cast<HWND>(mHandle), 0x415, inPos, reinterpret_cast<LPARAM>(&buttonInfo));
}

void COsToolBar::AddSeparator(int inPos) {
  if (inPos == -1) {
    inPos = GetNumButtons();
  }

  TBBUTTON buttonInfo;
  memset(&buttonInfo, 0, sizeof(buttonInfo));
  buttonInfo.iBitmap = -2;
  buttonInfo.idCommand = inPos;
  buttonInfo.fsStyle = TBSTYLE_SEP;
  buttonInfo.iString = -1;
  SendMessageA(static_cast<HWND>(mHandle), 0x415, inPos, reinterpret_cast<LPARAM>(&buttonInfo));
}

void COsToolBar::RemoveButton(int inPos) {
  SendMessageA(static_cast<HWND>(mHandle), 0x416, inPos, 0);
}

int COsToolBar::GetNumButtons() {
  return SendMessageA(static_cast<HWND>(mHandle), 0x418, 0, 0);
}

void COsToolBar::SetButtonImage(int inPos, int inWidth, int inHeight, LPVOID inData) {
  TBBUTTONINFOA buttonInfo;
  memset(&buttonInfo, 0, sizeof(buttonInfo));
  buttonInfo.cbSize = sizeof(buttonInfo);
  buttonInfo.dwMask = 0x80000001;
  SendMessageA(static_cast<HWND>(mHandle), 0x441, inPos, reinterpret_cast<LPARAM>(&buttonInfo));

  HDC     dc = GetDC(static_cast<HWND>(mHandle));
  HBITMAP bitmap = sBitmapFromImageData(inWidth, inHeight, inData, dc);
  HBITMAP mask = sMaskFromImageData(inWidth, inHeight, inData, dc);
  if (buttonInfo.iImage == -2) {
    int imageIndex = ImageList_Add(static_cast<HIMAGELIST>(mImageList), bitmap, mask);
    if (imageIndex != -1) {
      buttonInfo.iImage = imageIndex;
      SendMessageA(static_cast<HWND>(mHandle), 0x442, inPos, reinterpret_cast<LPARAM>(&buttonInfo));
    }
  } else {
    ImageList_Replace(static_cast<HIMAGELIST>(mImageList), buttonInfo.iImage, bitmap, mask);
    Refresh(1);
  }

  DeleteObject(bitmap);
  DeleteObject(mask);
  ReleaseDC(static_cast<HWND>(mHandle), dc);
}

void COsToolBar::SetButtonText(int inPos, LPCSTR inText) {
  TBBUTTONINFOA buttonInfo;
  memset(&buttonInfo, 0, sizeof(buttonInfo));
  buttonInfo.cbSize = sizeof(buttonInfo);
  buttonInfo.dwMask = 0x80000002;
  buttonInfo.pszText = const_cast<char *>(inText);
  SendMessageA(static_cast<HWND>(mHandle), 0x442, inPos, reinterpret_cast<LPARAM>(&buttonInfo));
}

void COsToolBar::GetButtonText(int inPos, char *inBuf, int inBufSize) {
  TBBUTTONINFOA buttonInfo;
  memset(&buttonInfo, 0, sizeof(buttonInfo));
  buttonInfo.cbSize = sizeof(buttonInfo);
  buttonInfo.dwMask = 2;
  buttonInfo.pszText = inBuf;
  buttonInfo.cchText = inBufSize;
  SendMessageA(static_cast<HWND>(mHandle), 0x441, inPos, reinterpret_cast<LPARAM>(&buttonInfo));
}

void COsToolBar::EnableButton(int inPos, int inVal) {
  TBBUTTONINFOA buttonInfo;
  memset(&buttonInfo, 0, sizeof(buttonInfo));
  buttonInfo.cbSize = sizeof(buttonInfo);
  buttonInfo.dwMask = 0x80000004;
  SendMessageA(static_cast<HWND>(mHandle), 0x441, inPos, reinterpret_cast<LPARAM>(&buttonInfo));
  if (inVal) {
    buttonInfo.fsState |= TBSTATE_ENABLED;
  } else {
    buttonInfo.fsState &= ~TBSTATE_ENABLED;
  }
  SendMessageA(static_cast<HWND>(mHandle), 0x442, inPos, reinterpret_cast<LPARAM>(&buttonInfo));
}

void COsToolBar::CheckButton(int inPos, int inVal) {
  TBBUTTONINFOA buttonInfo;
  memset(&buttonInfo, 0, sizeof(buttonInfo));
  buttonInfo.cbSize = sizeof(buttonInfo);
  buttonInfo.dwMask = 0x80000004;
  SendMessageA(static_cast<HWND>(mHandle), 0x441, inPos, reinterpret_cast<LPARAM>(&buttonInfo));
  if (inVal) {
    buttonInfo.fsState |= TBSTATE_CHECKED;
  } else {
    buttonInfo.fsState &= ~TBSTATE_CHECKED;
  }
  SendMessageA(static_cast<HWND>(mHandle), 0x442, inPos, reinterpret_cast<LPARAM>(&buttonInfo));
}

int COsToolBar::OnCommand(int inParam) {
  return SendEvent(0, inParam);
}

COsPopupMenu::COsPopupMenu(COsDialog *inDialog, short inID) : COsControl(inDialog, 5, inID, 0) {
  NTempest::CiRect sb = OsGuiGetScreenBounds();
  mMaxHeight = sb.Height() / 2 - 20;
  mBaseHeight = 0;
}

COsPopupMenu::~COsPopupMenu() {
}

void COsPopupMenu::SetSize(int inW, int inH) {
  mBaseHeight = inH;
  COsControl::SetSize(inW, inH);
  AdjustHeight();
}

void COsPopupMenu::SetValue(int inVal) {
  SendMessageA(static_cast<HWND>(mHandle), CB_SETCURSEL, inVal, 0);
}

int COsPopupMenu::GetValue() {
  return SendMessageA(static_cast<HWND>(mHandle), CB_GETCURSEL, 0, 0);
}

void COsPopupMenu::ClearItems() {
  SendMessageA(static_cast<HWND>(mHandle), CB_RESETCONTENT, 0, 0);
}

int COsPopupMenu::GetNumItems() {
  return SendMessageA(static_cast<HWND>(mHandle), CB_GETCOUNT, 0, 0);
}

void COsPopupMenu::InsertItem(LPCSTR inText, int inPos) {
  BOOL wasEmpty = GetNumItems() == 0;
  SendMessageA(static_cast<HWND>(mHandle), CB_INSERTSTRING, inPos, reinterpret_cast<LPARAM>(inText));
  AdjustHeight();
  if (wasEmpty) {
    SetValue(0);
  }
}

void COsPopupMenu::SetItemHeight(int inHeight) {
  SendMessageA(static_cast<HWND>(mHandle), CB_SETITEMHEIGHT, 0, inHeight);
  AdjustHeight();
}

int COsPopupMenu::GetItemHeight() {
  return SendMessageA(static_cast<HWND>(mHandle), CB_GETITEMHEIGHT, 0, 0);
}

void COsPopupMenu::AdjustHeight() {
  int height = GetItemHeight() * (GetNumItems() + 2);
  if (height >= mMaxHeight) {
    height = mMaxHeight;
  }
  int sizeX;
  int sizeY;
  GetSize(&sizeX, &sizeY);
  COsControl::SetSize(sizeX, height);
}

void COsPopupMenu::SetMaxHeight(int inHeight) {
  mMaxHeight = inHeight;
  AdjustHeight();
}

void COsPopupMenu::DeleteItem(int inPos) {
  SendMessageA(static_cast<HWND>(mHandle), CB_DELETESTRING, inPos, 0);
  AdjustHeight();
}

void COsPopupMenu::SetItemText(int inPos, LPCSTR inText) {
  DeleteItem(inPos);
  InsertItem(inText, inPos);
}

COsProgressBar::COsProgressBar(COsDialog *inDialog, short inID) : COsControl(inDialog, 8, inID, 0) {
}

void COsProgressBar::SetValue(int inVal) {
  SendMessageA(static_cast<HWND>(mHandle), PBM_SETPOS, inVal, 0);
}

int COsProgressBar::GetValue() {
  return SendMessageA(static_cast<HWND>(mHandle), PBM_GETPOS, 0, 0);
}

COsRadioButton::COsRadioButton(COsDialog *inDialog, short inID, UINT inFlags) : COsControl(inDialog, 14, inID, inFlags) {
}

void COsRadioButton::SetValue(int inVal) {
  SendMessageA(static_cast<HWND>(mHandle), BM_SETCHECK, inVal != 0, 0);
}

BOOL COsRadioButton::GetValue() {
  return SendMessageA(static_cast<HWND>(mHandle), BM_GETCHECK, 0, 0) == BST_CHECKED;
}

COsSlider::COsSlider(COsDialog *inDialog, short inID) : COsControl(inDialog, 9, inID, 0) {
}

void COsSlider::SetMinValue(int inVal) {
  SendMessageA(static_cast<HWND>(mHandle), TBM_SETRANGEMIN, 1, inVal);
}

void COsSlider::SetMaxValue(int inVal) {
  SendMessageA(static_cast<HWND>(mHandle), TBM_SETRANGEMAX, 1, inVal);
}

void COsSlider::SetValue(int inVal) {
  SendMessageA(static_cast<HWND>(mHandle), TBM_SETPOS, 1, inVal);
}

int COsSlider::GetValue() {
  return SendMessageA(static_cast<HWND>(mHandle), TBM_GETPOS, 0, 0);
}

void COsControl::SetContextMenu(COsMenu *inMenu) {
  DELIFUSED(mContextMenu);
  mContextMenu = inMenu;
  ASSERT(mContextMenu->GetID() == 0xFF);
}

int COsControl::OnContextMenu(int inX, int inY) {
  if (!mContextMenuEnabled || !mContextMenu) {
    return 0;
  }

  SendEvent(17, 0);
  int result =
      TrackPopupMenu(static_cast<HMENU>(mContextMenu->GetMenuHandle()), TPM_LEFTBUTTON | TPM_RIGHTBUTTON, inX, inY, 0, static_cast<HWND>(mHandle), 0);
  return result ? OnEvent(-3, result | (mID << 16), 0) : 0;
}

BOOL COsControl::IsHandleFromControl(LPVOID inHandle) {
  return inHandle == mHandle;
}

int COsControl::OnNotify(int inCode, LPVOID) {
  int event = sNCodeToItemCode(inCode, mType);
  return event == -1 ? 0 : SendEvent(event, 0);
}

int COsControl::OnCommand(int inParam) {
  int event = sNCodeToItemCode(HIWORD(inParam), mType);
  return event == -1 ? 0 : SendEvent(event, 0);
}

int COsControl::OnScroll(int inParam) {
  int event = sNCodeToItemCode(LOWORD(inParam), mType);
  return event == -1 ? 0 : SendEvent(event, 0);
}

BOOL COsControl::SendEvent(int inEvent, int inCode) {
  if (OnEvent(mID, inEvent, inCode)) {
    return 1;
  }

  if (mDialog) {
    return mDialog->OnEvent(mID, inEvent, inCode);
  }

  return 0;
}

static HBITMAP__ *sBitmapFromImageData(int inWidth, int inHeight, LPVOID inData, HDC__ *inDC) {
  BITMAPINFO bmInfo;
  memset(&bmInfo, 0, sizeof(bmInfo));
  bmInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmInfo.bmiHeader.biWidth = inWidth;
  bmInfo.bmiHeader.biHeight = -inHeight;
  bmInfo.bmiHeader.biPlanes = 1;
  bmInfo.bmiHeader.biBitCount = 32;
  return CreateDIBitmap(inDC, &bmInfo.bmiHeader, CBM_INIT, inData, &bmInfo, DIB_RGB_COLORS);
}

static HBITMAP__ *sMaskFromImageData(int inWidth, int inHeight, LPVOID inData, HDC__ *inDC) {
  int   rowBytes = 2 * ((inWidth - 1) / 8 + 1);
  BYTE *bits = static_cast<BYTE *>(_alloca(rowBytes * inHeight));
  memset(bits, 0, rowBytes * inHeight);
  BYTE *rgba = static_cast<BYTE *>(inData);
  for (int y = 0; y < inHeight; ++y) {
    for (int x = 0; x < inWidth; ++x) {
      if (!rgba[(y * inWidth + x) * 4 + 3]) {
        bits[y * rowBytes + x / 8] |= 1 << (7 - x % 8);
      }
    }
  }

  BITMAPINFO *info = static_cast<BITMAPINFO *>(SMemAlloc(sizeof(BITMAPINFOHEADER) + 2 * sizeof(RGBQUAD), __FILE__, __LINE__, 0));
  memset(info, 0, sizeof(BITMAPINFOHEADER) + 2 * sizeof(RGBQUAD));
  info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  info->bmiHeader.biWidth = inWidth;
  info->bmiHeader.biHeight = -inHeight;
  info->bmiHeader.biPlanes = 1;
  info->bmiHeader.biBitCount = 1;
  info->bmiColors[0].rgbBlue = info->bmiColors[0].rgbGreen = info->bmiColors[0].rgbRed = 0;
  *reinterpret_cast<DWORD *>(&info->bmiColors[1]) = 0xFFFFFFFF;
  HBITMAP bitmap = CreateDIBitmap(inDC, &info->bmiHeader, CBM_INIT, bits, info, DIB_RGB_COLORS);
  SMemFree(info, __FILE__, __LINE__, 0);
  return bitmap;
}

COsImageButton::COsImageButton(COsDialog *inDialog, short inID, UINT inFlags) : COsControl(inDialog, 1, inID, inFlags) {
}

COsImageButton::COsImageButton(LPVOID inWindow, short inID, UINT inFlags) : COsControl(inWindow, 1, inID, inFlags) {
}

COsImageButton::~COsImageButton() {
}

void COsImageButton::OnDestroy() {
  HBITMAP bitmap = reinterpret_cast<HBITMAP>(SendMessageA(static_cast<HWND>(mHandle), BM_GETIMAGE, IMAGE_BITMAP, 0));
  if (bitmap) {
    DeleteObject(bitmap);
  }
}

void COsImageButton::SetImage(int inWidth, int inHeight, LPVOID inData) {
  HDC     dc = GetDC(static_cast<HWND>(mHandle));
  HBITMAP bitmap = sBitmapFromImageData(inWidth, inHeight, inData, dc);
  HBITMAP oldBitmap =
      reinterpret_cast<HBITMAP>(SendMessageA(static_cast<HWND>(mHandle), BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(bitmap)));
  if (oldBitmap) {
    DeleteObject(oldBitmap);
  }
  ReleaseDC(static_cast<HWND>(mHandle), dc);
}

void COsImageButton::SetHighlight(int inVal) {
  int enabled = IsEnabled();
  Enable(0);
  SendMessageA(static_cast<HWND>(mHandle), BM_SETSTATE, inVal, 0);
  Enable(enabled);
}

BOOL COsImageButton::IsPushed() {
  return SendMessageA(static_cast<HWND>(mHandle), BM_GETSTATE, 0, 0) == BST_PUSHED;
}

COsTextButton::COsTextButton(COsDialog *inDialog, short inID)
    : COsControl(inDialog, 11, inID, 0), mActiveColor(0xFF000000), mPushedColor(0xFFFFFFFF), mGreyedColor(0xFF808080), mUnderline(1) {
}

BOOL COsTextButton::OnDraw(LPVOID inContext, UINT inState, NTempest::CiRect &inRect) {
  HDC                 dc = static_cast<HDC>(inContext);
  NTempest::CImVector color = mActiveColor;
  if (inState & 1) {
    color = mGreyedColor;
  } else if (inState & 4) {
    color = mPushedColor;
  }

  COLORREF winColor = RGB(color.r, color.g, color.b);
  HPEN     pen = CreatePen(PS_SOLID, 1, winColor);
  HPEN     oldPen = static_cast<HPEN>(SelectObject(dc, pen));
  COLORREF oldColor = SetTextColor(dc, winColor);

  RECT drawRect;
  sCiRectToWinRect(&inRect, &drawRect);
  HBRUSH bgBrush = CreateSolidBrush(GetBkColor(dc));
  FillRect(dc, &drawRect, bgBrush);

  char text[260];
  GetText(text, sizeof(text));
  int textLength = SStrLen(text);
  DrawTextA(dc, text, textLength, &drawRect, 0x10);

  if (mUnderline) {
    SIZE textSize;
    GetTextExtentPoint32A(dc, text, textLength, &textSize);
    int y = drawRect.top + textSize.cy;
    MoveToEx(dc, drawRect.left, y, 0);
    LineTo(dc, drawRect.left + textSize.cx, y);
  }

  SelectObject(dc, oldPen);
  SetTextColor(dc, oldColor);
  DeleteObject(pen);
  DeleteObject(bgBrush);
  return 1;
}

void COsStaticBox::ClearTransparentRects() {
  mTransRect.Clear();
  Refresh(1);
}

void COsStaticBox::AddTransparentRect(const NTempest::CiRect &inRect) {
  *mTransRect.New() = inRect;
  Refresh(1);
}

BOOL COsStaticBox::OnDraw(LPVOID inContext, UINT, NTempest::CiRect &inRect) {
  HDC  dc = static_cast<HDC>(inContext);
  RECT drawRect;
  sCiRectToWinRect(&inRect, &drawRect);
  HRGN drawRgn = CreateRectRgnIndirect(&drawRect);

  for (UINT i = 0; i < mTransRect.Count(); ++i) {
    RECT transRect;
    sCiRectToWinRect(&mTransRect[i], &transRect);
    HRGN transparentRegion = CreateRectRgnIndirect(&transRect);
    CombineRgn(drawRgn, drawRgn, transparentRegion, RGN_DIFF);
    DeleteObject(transparentRegion);
  }

  int    oldBkMode = SetBkMode(dc, TRANSPARENT);
  HBRUSH brush = CreateSolidBrush(GetBkColor(dc));
  FillRgn(dc, drawRgn, brush);
  SetBkMode(dc, oldBkMode);
  DeleteObject(brush);
  DeleteObject(drawRgn);
  return 1;
}

COsStaticText::COsStaticText(COsDialog *inDialog, short inID, UINT inFlags) : COsControl(inDialog, 2, inID, inFlags), mTextColor(0ul) {
  Initialize();
}

COsStaticText::COsStaticText(LPVOID inWindow, short inID, UINT inFlags) : COsControl(inWindow, 2, inID, inFlags), mTextColor(0ul) {
  Initialize();
}

void COsStaticText::Initialize() {
  mTextColor.a = 0;
}

void COsStaticText::SetJustification(int inJust) {
  LONG style = GetWindowLongA(static_cast<HWND>(mHandle), GWL_STYLE);
  style &= ~3;
  if (inJust == 1) {
    style |= SS_CENTER;
  } else if (inJust == 2) {
    style |= SS_RIGHT;
  }
  SetWindowLongA(static_cast<HWND>(mHandle), GWL_STYLE, style);
}

void COsStaticText::SetTextColor(const NTempest::CImVector &inColor) {
  NTempest::CImVector newColor(inColor);
  newColor.a = 0xFF;
  if (*mTextColor.IV_() != *newColor.IV_()) {
    mTextColor = newColor;
    Refresh(1);
  }
}

LPVOID COsStaticText::OnSetColors(LPVOID inContext) {
  if (!mTextColor.a) {
    return 0;
  }

  HDC dc = static_cast<HDC>(inContext);
  ::SetTextColor(dc, RGB(mTextColor.r, mTextColor.g, mTextColor.b));
  SetBkColor(dc, GetSysColor(COLOR_BTNFACE));
  return GetSysColorBrush(COLOR_BTNFACE);
}

COsStaticImage::COsStaticImage(COsDialog *inDialog, short inID) : COsControl(inDialog, 3, inID, 0) {
}

COsStaticImage::~COsStaticImage() {
}

void COsStaticImage::OnDestroy() {
  HBITMAP bitmap = reinterpret_cast<HBITMAP>(SendMessageA(static_cast<HWND>(mHandle), STM_GETIMAGE, IMAGE_BITMAP, 0));
  if (bitmap) {
    DeleteObject(bitmap);
  }
}

void COsStaticImage::SetImage(int inWidth, int inHeight, LPVOID inData) {
  HDC     dc = GetDC(static_cast<HWND>(mHandle));
  HBITMAP bitmap = sBitmapFromImageData(inWidth, inHeight, inData, dc);
  HBITMAP oldBitmap =
      reinterpret_cast<HBITMAP>(SendMessageA(static_cast<HWND>(mHandle), STM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(bitmap)));
  if (oldBitmap) {
    DeleteObject(oldBitmap);
  }
  ReleaseDC(static_cast<HWND>(mHandle), dc);
}

void COsStaticImage::ClearImage() {
  int sizeX;
  int sizeY;
  GetSize(&sizeX, &sizeY);
  HBITMAP bitmap = reinterpret_cast<HBITMAP>(SendMessageA(static_cast<HWND>(mHandle), STM_SETIMAGE, IMAGE_BITMAP, 0));
  if (bitmap) {
    DeleteObject(bitmap);
  }
  SetSize(sizeX, sizeY);
}

static int CALLBACK sEditBoxProc(HWND__ *hwnd, UINT msg, UINT wParam, long lParam) {
  COsEditBox *editBox = static_cast<COsEditBox *>(sGetOsGuiPointer(hwnd));
  if (editBox && ((msg >= WM_KEYDOWN && msg <= WM_KEYUP) || msg == WM_MOUSEMOVE || msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP ||
                  msg == WM_RBUTTONDOWN || msg == WM_RBUTTONUP))
  {
    editBox->UpdateSelection();
  }
  WNDPROC proc = reinterpret_cast<WNDPROC>(GetClassLongA(hwnd, GCL_WNDPROC));
  return CallWindowProcA(proc, hwnd, msg, wParam, lParam);
}

static BOOL sIsCharacterAllowed(char inChar, UINT inFilters) {
  signed char character = static_cast<signed char>(inChar);

  if (character >= 0 && character < 0x20) {
    return 1;
  }

  if ((inFilters & 0x01) && ((character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z'))) {
    return 1;
  }

  if ((inFilters & 0x02) && character >= '0' && character <= '9') {
    return 1;
  }

  if ((inFilters & 0x04) && (character == '+' || character == '-')) {
    return 1;
  }

  if ((inFilters & 0x08) && character == '.') {
    return 1;
  }

  if ((inFilters & 0x10) && character == ' ') {
    return 1;
  }

  if ((inFilters & 0x20) && character == '_') {
    return 1;
  }

  return (inFilters & 0x40) && character >= 0 && character < 0x7F;
}

BOOL COsTreeView::IsCharacterAllowed(char inChar) {
  if (!GetEditControl()) {
    return 1;
  }

  if (!mFiltersEnabled) {
    return 1;
  }

  return sIsCharacterAllowed(inChar, mFilters);
}

BOOL COsEditBox::IsCharacterAllowed(char inChar) {
  if (!mFiltersEnabled) {
    return 1;
  }

  return sIsCharacterAllowed(inChar, mFilters);
}

COsTabControl::COsTabControl(COsDialog *inDialog, short inID, UINT inFlags) : COsControl(inDialog, 15, inID, inFlags) {
}

void COsTabControl::SetValue(int inVal) {
  SendMessageA(static_cast<HWND>(mHandle), TCM_SETCURSEL, inVal, 0);
}

int COsTabControl::GetValue() {
  return SendMessageA(static_cast<HWND>(mHandle), TCM_GETCURSEL, 0, 0);
}

void COsTabControl::InsertItem(LPCSTR inText, int inPos) {
  TCITEMA itemInfo;
  itemInfo.mask = TCIF_TEXT;
  itemInfo.pszText = const_cast<char *>(inText);
  if (inPos == -1) {
    inPos = GetNumItems();
  }
  SendMessageA(static_cast<HWND>(mHandle), TCM_INSERTITEMA, inPos, reinterpret_cast<LPARAM>(&itemInfo));
}

int COsTabControl::GetNumItems() {
  return static_cast<int>(SendMessageA(static_cast<HWND>(mHandle), 0x1304, 0, 0));
}

BOOL COsTabControl::OnControlTab() {
  int shiftDown = OsGuiIsModifierKeyDown(1);
  int value = GetValue();
  int numItems = GetNumItems();

  if (shiftDown) {
    --value;
  } else {
    ++value;
  }

  if (value < 0) {
    value = numItems - 1;
  }

  if (value >= numItems) {
    value = 0;
  }

  SetValue(value);
  SendEvent(2, 0);
  return 1;
}

BOOL COsDialog::OnEvent(int inItemID, int inNotifyCode, int inCode) {
  if (mCallback) {
    OsGuiCallbackParams params;

    params.type = inItemID;
    params.subType = inNotifyCode;
    params.code = inCode;
    params.user = mCallbackParam;
    mCallback(params);
  }

  return 1;
}

static int CALLBACK sTreeViewProc(HWND__ *hwnd, UINT msg, UINT wParam, long lParam) {
  COsTreeView *tree = static_cast<COsTreeView *>(sGetOsGuiPointer(hwnd));
  if (tree) {
    if (msg == WM_LBUTTONDOWN && tree->OnMouseDown()) {
      return 0;
    }
    if (msg == WM_LBUTTONUP && tree->OnMouseUp()) {
      return 0;
    }
  }
  WNDPROC proc = reinterpret_cast<WNDPROC>(GetClassLongA(hwnd, GCL_WNDPROC));
  return CallWindowProcA(proc, hwnd, msg, wParam, lParam);
}

COsTreeView::COsTreeView(COsDialog *inDialog, short inID, UINT inFlags) : COsControl(inDialog, 10, inID, inFlags) {
  InitializeTreeView();
}

COsTreeView::COsTreeView(LPVOID inWindow, short inID, UINT inFlags) : COsControl(inWindow, 10, inID, inFlags) {
  InitializeTreeView();
}

COsTreeView::~COsTreeView() {
  DestroyDragImage();
  ImageList_Destroy(static_cast<HIMAGELIST>(mImages));
}

void COsTreeView::InitializeTreeView() {
  mDragImage = 0;
  mDragging = 0;
  mDragHandler = 0;
  mDragHandlerParam = 0;
  memset(&mDragInfo, 0, sizeof(mDragInfo));
  mDragInfo.treeView = this;
  mCanEditFunc = 0;
  mCanEditParam = 0;
  mExpandFunc = 0;
  mExpandParam = 0;
  mTextLimit = -1;
  mFiltersEnabled = 0;
  mFilters = 0;
  SetWindowLongA(static_cast<HWND>(mHandle), GWL_WNDPROC, reinterpret_cast<LONG>(sTreeViewProc));

  mImages = ImageList_Create(16, 16, 0x21, 1, 1);
  SendMessageA(static_cast<HWND>(mHandle), TVM_SETIMAGELIST, TVSIL_NORMAL, reinterpret_cast<LPARAM>(mImages));

  BYTE blank[1024];
  memset(blank, 0, sizeof(blank));
  SetItemImage(0, 16, 16, blank);
}

void COsTreeView::SetBackgroundColor(const NTempest::CImVector &inColor) {
  SendMessageA(static_cast<HWND>(mHandle), TVM_SETBKCOLOR, 0, RGB(inColor.r, inColor.g, inColor.b));
}

void COsTreeView::ClearItems() {
  SendMessageA(static_cast<HWND>(mHandle), TVM_DELETEITEM, 0, reinterpret_cast<LPARAM>(TVI_ROOT));
}

void COsTreeView::DeleteItem(LPVOID inItem) {
  SendMessageA(static_cast<HWND>(mHandle), TVM_DELETEITEM, 0, reinterpret_cast<LPARAM>(inItem));
}

LPVOID COsTreeView::InsertItem(LPVOID inParent, LPVOID inAfter, LPCSTR inText) {
  TVINSERTSTRUCTA insertInfo;
  memset(&insertInfo, 0, sizeof(insertInfo));
  insertInfo.hParent = static_cast<HTREEITEM>(inParent);
  if (!inAfter) {
    insertInfo.hInsertAfter = TVI_FIRST;
  } else if (inAfter == reinterpret_cast<LPVOID>(0xFFFF)) {
    insertInfo.hInsertAfter = TVI_LAST;
  } else {
    insertInfo.hInsertAfter = static_cast<HTREEITEM>(inAfter);
  }
  insertInfo.item.mask = TVIF_TEXT;
  insertInfo.item.pszText = const_cast<char *>(inText);

  LPVOID item = reinterpret_cast<LPVOID>(SendMessageA(static_cast<HWND>(mHandle), TVM_INSERTITEMA, 0, reinterpret_cast<LPARAM>(&insertInfo)));
  InitParams(item);
  return item;
}

void COsTreeView::SetItemText(LPVOID inItem, LPCSTR inText) {
  TVITEMA itemInfo;
  memset(&itemInfo, 0, sizeof(itemInfo));
  itemInfo.mask = TVIF_TEXT;
  itemInfo.hItem = static_cast<HTREEITEM>(inItem);
  itemInfo.pszText = const_cast<char *>(inText);
  SendMessageA(static_cast<HWND>(mHandle), TVM_SETITEMA, 0, reinterpret_cast<LPARAM>(&itemInfo));
}

void COsTreeView::GetItemText(LPVOID inItem, char *inBuf, int inBufSize) {
  TVITEMA itemInfo;
  memset(&itemInfo, 0, sizeof(itemInfo));
  itemInfo.mask = TVIF_TEXT;
  itemInfo.hItem = static_cast<HTREEITEM>(inItem);
  itemInfo.pszText = inBuf;
  itemInfo.cchTextMax = inBufSize;
  SendMessageA(static_cast<HWND>(mHandle), TVM_GETITEMA, 0, reinterpret_cast<LPARAM>(&itemInfo));
}

void COsTreeView::SetItemParam(LPVOID inItem, LPVOID inParam) {
  GetParams(inItem)->user = inParam;
}

LPVOID COsTreeView::GetItemParam(LPVOID inItem) {
  return GetParams(inItem)->user;
}

void COsTreeView::SetItemColor(LPVOID inItem, const NTempest::CImVector &inColor) {
  OsGuiTreeItemParams *params = GetParams(inItem);
  NTempest::CImVector  newColor(inColor);
  newColor.a = 0xFF;
  if (*params->color.IV_() != *newColor.IV_()) {
    params->color = newColor;
    RefreshItem(inItem);
  }
}

void COsTreeView::ResetItemColor(LPVOID inItem) {
  OsGuiTreeItemParams *params = GetParams(inItem);
  if (params->color.a) {
    params->color.a = 0;
    RefreshItem(inItem);
  }
}

NTempest::CImVector COsTreeView::GetItemColor(LPVOID inItem) {
  return GetParams(inItem)->color;
}

LPVOID COsTreeView::GetItemParent(LPVOID inItem) {
  return reinterpret_cast<LPVOID>(SendMessageA(static_cast<HWND>(mHandle), TVM_GETNEXTITEM, TVGN_PARENT, reinterpret_cast<LPARAM>(inItem)));
}

BOOL COsTreeView::GetItemNumChildren(LPVOID inItem) {
  int    count = 0;
  LPVOID item = reinterpret_cast<LPVOID>(SendMessageA(static_cast<HWND>(mHandle), TVM_GETNEXTITEM, TVGN_CHILD, reinterpret_cast<LPARAM>(inItem)));
  while (item) {
    ++count;
    item = reinterpret_cast<LPVOID>(SendMessageA(static_cast<HWND>(mHandle), TVM_GETNEXTITEM, TVGN_NEXT, reinterpret_cast<LPARAM>(item)));
  }
  return count;
}

LPVOID COsTreeView::GetItemChild(LPVOID inItem, int inIndex) {
  int    index = 0;
  LPVOID item = reinterpret_cast<LPVOID>(SendMessageA(static_cast<HWND>(mHandle), TVM_GETNEXTITEM, TVGN_CHILD, reinterpret_cast<LPARAM>(inItem)));
  while (item && index != inIndex) {
    item = reinterpret_cast<LPVOID>(SendMessageA(static_cast<HWND>(mHandle), TVM_GETNEXTITEM, TVGN_NEXT, reinterpret_cast<LPARAM>(item)));
    ++index;
  }
  return item;
}

void COsTreeView::EnumerateItems(LPVOID inParent, void (*inFunc)(COsTreeView *, LPVOID, LPVOID), LPVOID inParam) {
  inFunc(this, inParent, inParam);
  LPVOID item = reinterpret_cast<LPVOID>(SendMessageA(static_cast<HWND>(mHandle), TVM_GETNEXTITEM, TVGN_CHILD, reinterpret_cast<LPARAM>(inParent)));
  while (item) {
    EnumerateItems(item, inFunc, inParam);
    item = reinterpret_cast<LPVOID>(SendMessageA(static_cast<HWND>(mHandle), TVM_GETNEXTITEM, TVGN_NEXT, reinterpret_cast<LPARAM>(item)));
  }
}

void COsTreeView::EnumerateAllItems(void (*inFunc)(COsTreeView *, LPVOID, LPVOID), LPVOID inParam) {
  LPVOID root = reinterpret_cast<LPVOID>(SendMessageA(static_cast<HWND>(mHandle), TVM_GETNEXTITEM, TVGN_ROOT, 0));
  if (root) {
    EnumerateItems(root, inFunc, inParam);
  }
}

void COsTreeView::SetItemImage(LPVOID inItem, int inWidth, int inHeight, LPVOID inData) {
  FATALASSERT(inWidth == 16);
  FATALASSERT(inHeight == 16);

  HDC     dc = GetDC(static_cast<HWND>(mHandle));
  HBITMAP bitmap = sBitmapFromImageData(16, 16, inData, dc);
  HBITMAP mask = sMaskFromImageData(16, 16, inData, dc);
  int     imageCount = ImageList_GetImageCount(static_cast<HIMAGELIST>(mImages));

  if (!inItem) {
    if (!imageCount) {
      ImageList_Add(static_cast<HIMAGELIST>(mImages), bitmap, mask);
    } else {
      ImageList_Replace(static_cast<HIMAGELIST>(mImages), 0, bitmap, mask);
      Refresh(1);
    }
  } else {
    TVITEMA itemInfo;
    memset(&itemInfo, 0, sizeof(itemInfo));
    itemInfo.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    itemInfo.hItem = static_cast<HTREEITEM>(inItem);
    SendMessageA(static_cast<HWND>(mHandle), TVM_GETITEMA, 0, reinterpret_cast<LPARAM>(&itemInfo));

    if (itemInfo.iImage > 0 && itemInfo.iImage < imageCount) {
      ImageList_Replace(static_cast<HIMAGELIST>(mImages), itemInfo.iImage, bitmap, mask);
      Refresh(1);
    } else {
      int image;
      if (!mUnusedImageIDs.Count()) {
        image = ImageList_Add(static_cast<HIMAGELIST>(mImages), bitmap, mask);
      } else {
        image = mUnusedImageIDs[mUnusedImageIDs.Count() - 1];
        mUnusedImageIDs.SetCount(mUnusedImageIDs.Count() - 1);
        ImageList_Replace(static_cast<HIMAGELIST>(mImages), image, bitmap, mask);
      }

      if (image != -1) {
        itemInfo.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        itemInfo.hItem = static_cast<HTREEITEM>(inItem);
        itemInfo.iImage = image;
        itemInfo.iSelectedImage = image;
        SendMessageA(static_cast<HWND>(mHandle), TVM_SETITEMA, 0, reinterpret_cast<LPARAM>(&itemInfo));
      }
    }
  }

  DeleteObject(bitmap);
  DeleteObject(mask);
  ReleaseDC(static_cast<HWND>(mHandle), dc);
}

void COsTreeView::ExpandItem(LPVOID inItem, int inVal) {
  SendMessageA(static_cast<HWND>(mHandle), TVM_EXPAND, inVal ? TVE_EXPAND : TVE_COLLAPSE, reinterpret_cast<LPARAM>(inItem));
}

BOOL COsTreeView::IsItemExpanded(LPVOID inItem) {
  return (SendMessageA(static_cast<HWND>(mHandle), 0x1127, reinterpret_cast<WPARAM>(inItem), TVIS_EXPANDED) & TVIS_EXPANDED) != 0;
}

void COsTreeView::OnSizeChange() {
}

void COsTreeView::OnExpandedItem(LPVOID inItem) {
  if (mExpandFunc) {
    mExpandFunc(inItem, mExpandParam);
  }
}

void COsTreeView::EnsureItemVisible(LPVOID inItem) {
  SendMessageA(static_cast<HWND>(mHandle), TVM_ENSUREVISIBLE, 0, reinterpret_cast<LPARAM>(inItem));
}

void COsTreeView::EditItem(LPVOID inItem) {
  SetInputFocus();
  SendMessageA(static_cast<HWND>(mHandle), TVM_EDITLABELA, 0, reinterpret_cast<LPARAM>(inItem));
}

static void sTVGetSelectInfo(COsTreeView *inView, LPVOID inItem, LPVOID inParam) {
  struct SelectInfo {
    int    count;
    LPVOID first;
    UINT   flags;
    LPVOID previous;
    LPVOID last;
  };
  SelectInfo *info = static_cast<SelectInfo *>(inParam);
  if (inView->IsItemSelected(inItem)) {
    ++info->count;
    if (info->count == 1) {
      info->first = info->previous = info->last = inItem;
      info->flags |= 3;
    } else {
      if ((info->flags & 1) && TreeView_GetParent(static_cast<HWND>(inView->GetHandle()), static_cast<HTREEITEM>(inItem)) !=
                                   TreeView_GetParent(static_cast<HWND>(inView->GetHandle()), static_cast<HTREEITEM>(info->first)))
      {
        info->flags &= ~1U;
      }
      if ((info->flags & 2) && info->previous != info->last) {
        info->flags &= ~2U;
      }
      info->previous = info->last = inItem;
    }
  } else {
    info->last = inItem;
  }
}

static void sTVSelect(COsTreeView *inView, LPVOID inItem, LPVOID inParam) {
  inView->SelectItem(inItem, *static_cast<int *>(inParam));
}

void COsTreeView::SelectItem(LPVOID inItem, int inVal) {
  if (mFlags & 0x40000) {
    TVITEMA itemInfo;
    memset(&itemInfo, 0, sizeof(itemInfo));
    itemInfo.mask = TVIF_STATE;
    itemInfo.hItem = static_cast<HTREEITEM>(inItem);
    itemInfo.state = inVal ? TVIS_SELECTED : 0;
    itemInfo.stateMask = TVIS_SELECTED;
    SendMessageA(static_cast<HWND>(mHandle), TVM_SETITEMA, 0, reinterpret_cast<LPARAM>(&itemInfo));
    SendEvent(2, 0);
  } else {
    SendMessageA(static_cast<HWND>(mHandle), TVM_SELECTITEM, TVGN_CARET, inVal ? reinterpret_cast<LPARAM>(inItem) : 0);
  }
}

BOOL COsTreeView::IsItemSelected(LPVOID inItem) {
  if (mFlags & 0x40000) {
    return (SendMessageA(static_cast<HWND>(mHandle), 0x1127, reinterpret_cast<WPARAM>(inItem), TVIS_SELECTED) & TVIS_SELECTED) != 0;
  }
  return inItem == GetSelectedItem();
}

LPVOID COsTreeView::GetSelectedItem() {
  if (mFlags & 0x40000) {
    OsGuiTVSelectionInfo info;
    GetSelectionInfo(&info);
    return info.firstSelection;
  }

  return reinterpret_cast<LPVOID>(SendMessageA(static_cast<HWND>(mHandle), TVM_GETNEXTITEM, TVGN_CARET, 0));
}

void COsTreeView::GetSelectionInfo(OsGuiTVSelectionInfo *outInfo) {
  struct SelectInfo {
    OsGuiTVSelectionInfo info;
    LPVOID               lastSelected;
    LPVOID               lastProcessed;
  } results;
  memset(&results, 0, sizeof(results));
  EnumerateAllItems(sTVGetSelectInfo, &results);
  *outInfo = results.info;
}

void COsTreeView::SelectAll(int inVal) {
  EnumerateAllItems(sTVSelect, &inVal);
}

BOOL COsTreeView::OnMouseDown() {
  if (!(mFlags & 0x40000)) {
    return 0;
  }
  LPVOID item = FindItemUnderCursor();
  SetInputFocus();
  SendMessageA(static_cast<HWND>(mHandle), TVM_SELECTITEM, TVGN_CARET, 0);
  return item != 0;
}

BOOL COsTreeView::OnMouseUp() {
  if (mDragging) {
    OnEndDrag();
    mDragging = 0;
    return 0;
  }
  if (!(mFlags & 0x40000)) {
    return 0;
  }

  LPVOID item = FindItemUnderCursor();
  int    shift = OsGuiIsModifierKeyDown(1);
  int    ctrl = OsGuiIsModifierKeyDown(0);
  int    selected = IsItemSelected(item);
  if (shift) {
    return 0;
  }
  if (ctrl) {
    SelectItem(item, !selected);
  } else {
    SelectAll(0);
    SelectItem(item, 1);
  }
  return 1;
}

LPVOID COsTreeView::FindItemUnderCursor() {
  int cursX = 0;
  int cursY = 0;
  OsGuiGetCursorPosition(&cursX, &cursY);
  int ctrlX = 0;
  int ctrlY = 0;
  GetPosition(&ctrlX, &ctrlY, 0);

  TVHITTESTINFO htInfo;
  memset(&htInfo, 0, sizeof(htInfo));
  htInfo.pt.x = cursX - ctrlX;
  htInfo.pt.y = cursY - ctrlY;
  SendMessageA(static_cast<HWND>(mHandle), TVM_HITTEST, 0, reinterpret_cast<LPARAM>(&htInfo));
  return (htInfo.flags & 0x46) ? htInfo.hItem : 0;
}

BOOL COsTreeView::OnReturn() {
  if (GetEditControl()) {
    SendMessageA(static_cast<HWND>(mHandle), TVM_ENDEDITLABELNOW, 0, 0);
    return 1;
  }
  return SendEvent(9, 0);
}

BOOL COsTreeView::OnEscape() {
  if (GetEditControl()) {
    SendMessageA(static_cast<HWND>(mHandle), TVM_ENDEDITLABELNOW, 1, 0);
    return 1;
  }
  return 0;
}

int COsTreeView::OnNotify(int inCode, LPVOID inParam) {
  if (inCode == TVN_SELCHANGINGA) {
    if (mFlags & 0x40000) {
      return 1;
    }
    return COsControl::OnNotify(inCode, inParam);
  }

  if (inCode == TVN_KEYDOWN) {
    WORD key = static_cast<NMTVKEYDOWN *>(inParam)->wVKey;
    switch (key) {
      case VK_TAB:
        return SendEvent(11, 0);
      case VK_RETURN:
        return OnReturn();
      case VK_ESCAPE:
        return OnEscape();
      case VK_DELETE:
        return SendEvent(10, 0);
      default:
        return COsControl::OnNotify(inCode, inParam);
    }
  }

  if (inCode == TVN_BEGINLABELEDITA || inCode == TVN_ENDLABELEDITA) {
    NMTVDISPINFOA *editInfo = static_cast<NMTVDISPINFOA *>(inParam);
    int accepted = inCode == TVN_ENDLABELEDITA ? OnEndEdit(editInfo->item.hItem, editInfo->item.pszText) : OnBeginEdit(editInfo->item.hItem);
    if (accepted) {
      return COsControl::OnNotify(inCode, inParam);
    }
    if (inCode == TVN_BEGINLABELEDITA) {
      OnEscape();
      SetInputFocus();
      return 1;
    }
    return 0;
  }

  if (inCode == TVN_DELETEITEMA) {
    OnDeleteItem(static_cast<NMTREEVIEWA *>(inParam)->itemOld.hItem);
    return COsControl::OnNotify(inCode, inParam);
  }

  if (inCode == TVN_BEGINDRAGA) {
    NMTREEVIEWA *treeInfo = static_cast<NMTREEVIEWA *>(inParam);
    OnBeginDrag(treeInfo->itemNew.hItem, treeInfo->ptDrag.x, treeInfo->ptDrag.y);
    return 1;
  }

  if (inCode == TVN_ITEMEXPANDEDA) {
    OnExpandedItem(static_cast<NMTREEVIEWA *>(inParam)->itemNew.hItem);
    return 1;
  }

  if (inCode == NM_CUSTOMDRAW) {
    NMTVCUSTOMDRAW *drawInfo = static_cast<NMTVCUSTOMDRAW *>(inParam);
    if (drawInfo->nmcd.dwDrawStage == CDDS_PREPAINT) {
      if (mDialog) {
        SetWindowLongA(static_cast<HWND>(mDialog->GetHandle()), DWL_MSGRESULT, CDRF_NOTIFYITEMDRAW);
      }
      return CDRF_NOTIFYITEMDRAW;
    }
    if (drawInfo->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
      LPVOID item = reinterpret_cast<LPVOID>(drawInfo->nmcd.dwItemSpec);
      if (!IsItemSelected(item)) {
        inCode = *GetParams(item)->color.IV_();
        if (inCode & 0xFF000000) {
          drawInfo->clrText = RGB((inCode >> 16) & 0xFF, (inCode >> 8) & 0xFF, inCode & 0xFF);
        }
      }
      inCode = NM_CUSTOMDRAW;
    }
  }

  return COsControl::OnNotify(inCode, inParam);
}

BOOL COsTreeView::IsHandleFromControl(LPVOID inHandle) {
  LPVOID editControl = GetEditControl();
  return (editControl && inHandle == editControl) || mHandle == inHandle;
}

void COsTreeView::EnableDragDrop(int inVal) {
  LONG style = GetWindowLongA(static_cast<HWND>(mHandle), GWL_STYLE);
  if (inVal) {
    style &= ~TVS_DISABLEDRAGDROP;
  } else {
    style |= TVS_DISABLEDRAGDROP;
  }
  SetWindowLongA(static_cast<HWND>(mHandle), GWL_STYLE, style);
}

void COsTreeView::SetDragDropHandler(int (*inFunc)(const OsGuiTVDDInfo &, LPVOID), LPVOID inParam) {
  mDragHandler = inFunc;
  mDragHandlerParam = inParam;
}

void COsTreeView::CreateDragImage(LPVOID inItem) {
  DestroyDragImage();
  mDragImage = reinterpret_cast<LPVOID>(SendMessageA(static_cast<HWND>(mHandle), TVM_CREATEDRAGIMAGE, 0, reinterpret_cast<LPARAM>(inItem)));
}

void COsTreeView::DestroyDragImage() {
  if (mDragImage) {
    ImageList_Destroy(static_cast<HIMAGELIST>(mDragImage));
    mDragImage = 0;
  }
}

NTempest::CiRect COsTreeView::GetItemRect(LPVOID inItem) {
  NTempest::CiRect itemRect(0);
  RECT             wRect;
  wRect.left = reinterpret_cast<LONG>(inItem);
  SendMessageA(static_cast<HWND>(mHandle), TVM_GETITEMRECT, 1, reinterpret_cast<LPARAM>(&wRect));
  sWinRectToCiRect(&wRect, &itemRect);
  return itemRect;
}

void COsTreeView::RefreshItem(LPVOID inItem) {
  RECT wRect;
  wRect.left = reinterpret_cast<LONG>(inItem);
  SendMessageA(static_cast<HWND>(mHandle), TVM_GETITEMRECT, 1, reinterpret_cast<LPARAM>(&wRect));
  InvalidateRect(static_cast<HWND>(mHandle), &wRect, 0);
}

LPVOID COsTreeView::GetFirstVisibleItem() {
  return reinterpret_cast<LPVOID>(SendMessageA(static_cast<HWND>(mHandle), TVM_GETNEXTITEM, TVGN_FIRSTVISIBLE, 0));
}

void COsTreeView::SetFirstVisibleItem(LPVOID inItem) {
  SetRedraw(0);
  LPVOID lastVisible = reinterpret_cast<LPVOID>(SendMessageA(static_cast<HWND>(mHandle), TVM_GETNEXTITEM, TVGN_LASTVISIBLE, 0));
  EnsureItemVisible(lastVisible);
  EnsureItemVisible(inItem);
  SetRedraw(1);
}

int COsTreeView::RunDragHandler() {
  if (mDragHandler) {
    return mDragHandler(mDragInfo, mDragHandlerParam);
  }
  return 0;
}

void COsTreeView::OnBeginDrag(LPVOID inItem, int inX, int inY) {
  SetInputFocus();
  SelectItem(inItem, 1);
  mDragInfo.action = 0;
  mDragInfo.dragItem = inItem;
  mDragInfo.targItem = inItem;
  mDragInfo.targX = inX;
  mDragInfo.targY = inY;
  if (!RunDragHandler()) {
    return;
  }

  CreateDragImage(inItem);
  int wx;
  int mx;
  int cx;
  GetPosition(&wx, &cx, 0);
  mx = wx + inX;
  inY += cx;
  NTempest::CiRect itemRect = GetItemRect(inItem);
  int              dragX = mx - itemRect.l - wx + 16;
  int              dragY = inY - itemRect.t - cx;
  mDialog->GetPosition(&wx, &cx, 0);
  ImageList_BeginDrag(static_cast<HIMAGELIST>(mDragImage), 0, dragX, dragY);
  ImageList_DragEnter(static_cast<HWND>(mDialog->GetHandle()), mx - wx, inY - cx);
  SetCapture(static_cast<HWND>(mDialog->GetHandle()));
  mDragInfo.dragItem = inItem;
  mDragging = 1;
}

void COsTreeView::OnMouseMove(int inX, int inY) {
  if (!mDragging) {
    return;
  }

  int lx;
  int ly;
  int wx;
  int wy;
  int cx;
  int cy;
  mDialog->GetPosition(&lx, &ly, 1);
  mDialog->GetPosition(&wx, &wy, 0);
  GetPosition(&cx, &cy, 0);
  lx += inX;
  ly += inY;
  ImageList_DragMove(lx - wx, ly - wy);

  TVHITTESTINFO hitTest;
  memset(&hitTest, 0, sizeof(hitTest));
  hitTest.pt.x = lx - cx;
  hitTest.pt.y = ly - cy;
  SendMessageA(static_cast<HWND>(mHandle), TVM_HITTEST, 0, reinterpret_cast<LPARAM>(&hitTest));
  mDragInfo.targItem = hitTest.hItem;
  mDragInfo.targX = 0;
  mDragInfo.targY = 0;
  if (hitTest.hItem) {
    NTempest::CiRect itemRect = GetItemRect(hitTest.hItem);
    mDragInfo.targX = lx - itemRect.l - cx;
    mDragInfo.targY = ly - itemRect.t - cy;
  }
  mDragInfo.action = 1;
  RunDragHandler();
}

void COsTreeView::SetDropTarget(LPVOID inItem) {
  ImageList_DragShowNolock(0);
  SendMessageA(static_cast<HWND>(mHandle), TVM_SELECTITEM, TVGN_DROPHILITE, reinterpret_cast<LPARAM>(inItem));
  ImageList_DragShowNolock(1);
}

void COsTreeView::SetInsertionMark(LPVOID inItem, int inAfter) {
  ImageList_DragShowNolock(0);
  SendMessageA(static_cast<HWND>(mHandle), 0x111A, inAfter, reinterpret_cast<LPARAM>(inItem));
  ImageList_DragShowNolock(1);
}

void COsTreeView::OnEndDrag() {
  ImageList_EndDrag();
  ImageList_DragLeave(static_cast<HWND>(mDialog->GetHandle()));
  DestroyDragImage();
  ReleaseCapture();
  SetDropTarget(0);
  SetInsertionMark(0, 0);
  SelectItem(mDragInfo.dragItem, 1);
  mDragInfo.action = 2;
  RunDragHandler();
}

int COsTreeView::OnBeginEdit(LPVOID inItem) {
  int result;
  if (mCanEditFunc && !(result = mCanEditFunc(inItem, mCanEditParam))) {
    return result;
  }
  if (mTextLimit != -1) {
    HWND editControl = static_cast<HWND>(GetEditControl());
    if (editControl) {
      SendMessageA(editControl, EM_LIMITTEXT, mTextLimit, 0);
    }
  }
  return 1;
}

BOOL COsTreeView::OnEndEdit(LPVOID inItem, LPCSTR inNewText) {
  if (!inNewText || !*inNewText) {
    return 0;
  }
  SetItemText(inItem, inNewText);
  return 1;
}

void COsTreeView::SetTextLimit(int inSize) {
  mTextLimit = inSize;
}

void COsTreeView::EnableFilters(int inVal) {
  mFiltersEnabled = inVal;
}

void COsTreeView::SetFilter(UINT inFilter, int inVal) {
  if (inVal) {
    mFilters |= inFilter;
  } else {
    mFilters &= ~inFilter;
  }
}

void COsTreeView::SetCanEditFunction(int (*inFunc)(LPVOID, LPVOID), LPVOID inParam) {
  mCanEditFunc = inFunc;
  mCanEditParam = inParam;
}

void COsTreeView::SetExpandFunction(void (*inFunc)(LPVOID, LPVOID), LPVOID inParam) {
  mExpandFunc = inFunc;
  mExpandParam = inParam;
}

void COsTreeView::OnDeleteItem(LPVOID inItem) {
  TVITEMA itemInfo;
  memset(&itemInfo, 0, sizeof(itemInfo));
  itemInfo.mask = TVIF_IMAGE;
  itemInfo.hItem = static_cast<HTREEITEM>(inItem);
  SendMessageA(static_cast<HWND>(mHandle), TVM_GETITEMA, 0, reinterpret_cast<LPARAM>(&itemInfo));
  int imageCount = ImageList_GetImageCount(static_cast<HIMAGELIST>(mImages));
  if (itemInfo.iImage > 0 && itemInfo.iImage < imageCount) {
    *mUnusedImageIDs.New() = itemInfo.iImage;
  }
  GetParams(inItem)->used = 0;
}

int COsTreeView::FindUnusedParams() {
  UINT numParams = mItemParams.Count();
  for (UINT i = 0; i < numParams; ++i) {
    if (!mItemParams[i].used) {
      return i;
    }
  }
  return -1;
}

void COsTreeView::InitParams(LPVOID inItem) {
  int paramID = FindUnusedParams();
  if (paramID == -1) {
    mItemParams.New();
    paramID = mItemParams.Count() - 1;
  }
  mItemParams[paramID].used = 1;
  mItemParams[paramID].color = 0;
  mItemParams[paramID].user = 0;

  TVITEMA itemInfo;
  memset(&itemInfo, 0, sizeof(itemInfo));
  itemInfo.mask = TVIF_PARAM;
  itemInfo.hItem = static_cast<HTREEITEM>(inItem);
  itemInfo.lParam = paramID;
  SendMessageA(static_cast<HWND>(mHandle), TVM_SETITEMA, 0, reinterpret_cast<LPARAM>(&itemInfo));
}

OsGuiTreeItemParams *COsTreeView::GetParams(LPVOID inItem) {
  TVITEMA itemInfo;
  memset(&itemInfo, 0, sizeof(itemInfo));
  itemInfo.mask = TVIF_PARAM;
  itemInfo.hItem = static_cast<HTREEITEM>(inItem);
  SendMessageA(static_cast<HWND>(mHandle), TVM_GETITEMA, 0, reinterpret_cast<LPARAM>(&itemInfo));
  int paramID = itemInfo.lParam;
  FATALASSERT(paramID >= 0 && paramID < static_cast<int>(mItemParams.Count()));
  return &mItemParams[paramID];
}

LPVOID COsTreeView::GetEditControl() {
  return reinterpret_cast<LPVOID>(SendMessageA(static_cast<HWND>(mHandle), 0x110F, 0, 0));
}

static int CALLBACK sSpinButtonProc(HWND__ *hwnd, UINT msg, UINT wParam, long lParam) {
  COsSpinButton *spin = static_cast<COsSpinButton *>(sGetOsGuiPointer(hwnd));
  if (spin && msg == WM_LBUTTONUP) {
    spin->OnSpinMouseUp();
  }
  WNDPROC proc = reinterpret_cast<WNDPROC>(GetClassLongA(hwnd, GCL_WNDPROC));
  return CallWindowProcA(proc, hwnd, msg, wParam, lParam);
}

COsSpinButton::COsSpinButton(COsDialog *inDialog, short inID, UINT inFlags) : COsControl(inDialog, 13, inID, inFlags) {
  Initialize();
}

COsSpinButton::COsSpinButton(LPVOID inWindow, short inID, UINT inFlags) : COsControl(inWindow, 13, inID, inFlags) {
  Initialize();
}

void COsSpinButton::Initialize() {
  SetWindowLongA(static_cast<HWND>(mHandle), GWL_WNDPROC, reinterpret_cast<LONG>(sSpinButtonProc));
}

void COsSpinButton::OnSpinMouseUp() {
  SendEvent(5, 0);
}

void COsSpinButton::SetValueRange(int inMinVal, int inMaxVal) {
  SendMessageA(static_cast<HWND>(mHandle), UDM_SETRANGE32, inMinVal, inMaxVal);
}

void COsSpinButton::SetValue(int inVal) {
  SendMessageA(static_cast<HWND>(mHandle), 0x471, 0, inVal);
}

int COsSpinButton::GetValue() {
  return SendMessageA(static_cast<HWND>(mHandle), 0x472, 0, 0);
}

static int sConvertScrollMsg(int inWParam) {
  switch (LOWORD(inWParam)) {
    case SB_LINEUP:
      return 3;
    case SB_LINEDOWN:
      return 2;
    case SB_PAGEUP:
      return 5;
    case SB_PAGEDOWN:
      return 4;
    case SB_THUMBPOSITION:
      return 0;
    case SB_THUMBTRACK:
      return 1;
    case SB_TOP:
      return 7;
    case SB_BOTTOM:
      return 6;
  }
  return -1;
}

static int sProcessScrollMessage(LPVOID inWindow, int inBarType, int inScrollMsg, int inInc) {
  SCROLLINFO info;
  info.cbSize = sizeof(info);
  info.fMask = SIF_ALL;
  GetScrollInfo(static_cast<HWND>(inWindow), inBarType, &info);

  int oldPos = info.nPos;
  int newPos = oldPos;
  switch (inScrollMsg) {
    case 0:
    case 1:
      newPos = info.nTrackPos;
      break;
    case 2:
      newPos += inInc;
      break;
    case 3:
      newPos -= inInc;
      break;
    case 4:
      newPos += info.nPage;
      break;
    case 5:
      newPos -= info.nPage;
      break;
    case 6:
      newPos = info.nMax;
      break;
    case 7:
      newPos = info.nMin;
      break;
  }

  if (newPos > info.nMax - static_cast<int>(info.nPage) + 1) {
    newPos = info.nMax - info.nPage + 1;
  }
  if (newPos < info.nMin) {
    newPos = info.nMin;
  }
  if (newPos != oldPos) {
    SetScrollPos(static_cast<HWND>(inWindow), inBarType, newPos, TRUE);
  }
  return oldPos - newPos;
}

COsScrollBar::COsScrollBar(COsDialog *inDialog, short inID, UINT inFlags) : COsControl(inDialog, 18, inID, inFlags) {
  Initialize();
}

COsScrollBar::COsScrollBar(LPVOID inWindow, short inID, UINT inFlags) : COsControl(inWindow, 18, inID, inFlags) {
  Initialize();
}

void COsScrollBar::Initialize() {
  mRealMin = 0;
  mRealMax = 100;
  SetPageSize(1);
  UpdateRangeValues();
}

void COsScrollBar::SetRange(int inMin, int inMax) {
  mRealMin = inMin;
  mRealMax = inMax;
  UpdateRangeValues();
}

void COsScrollBar::SetPageSize(int inVal) {
  mPageSize = inVal;
  SCROLLINFO info;
  info.cbSize = sizeof(info);
  info.fMask = SIF_PAGE;
  info.nPage = inVal;
  SetScrollInfo(static_cast<HWND>(mHandle), SB_CTL, &info, TRUE);
  UpdateRangeValues();
}

void COsScrollBar::UpdateRangeValues() {
  int scrollMax = mRealMax + (mPageSize - 1 > 0 ? mPageSize - 1 : 0);
  SendMessageA(static_cast<HWND>(mHandle), SBM_SETRANGE, mRealMin, scrollMax);

  SCROLLINFO info;
  info.cbSize = sizeof(info);
  info.fMask = SIF_RANGE;
  info.nMin = mRealMin;
  info.nMax = scrollMax;
  SetScrollInfo(static_cast<HWND>(mHandle), SB_CTL, &info, TRUE);
}

void COsScrollBar::SetValue(int inVal) {
  SendMessageA(static_cast<HWND>(mHandle), SBM_SETPOS, inVal, TRUE);

  SCROLLINFO info;
  info.cbSize = sizeof(info);
  info.fMask = SIF_POS;
  info.nPos = inVal;
  SetScrollInfo(static_cast<HWND>(mHandle), SB_CTL, &info, TRUE);
}

int COsScrollBar::GetValue() {
  return SendMessageA(static_cast<HWND>(mHandle), SBM_GETPOS, 0, 0);
}

int COsScrollBar::OnScroll(int inParam) {
  int scrollMessage = sConvertScrollMsg(inParam);
  if (scrollMessage == -1 || !sProcessScrollMessage(mHandle, SB_CTL, scrollMessage, 1)) {
    return 0;
  }
  return SendEvent(2, 0);
}

BOOL COsScrollBar::OnMouseWheel(int inDelta) {
  if (!inDelta) {
    return 0;
  }

  int scrollMessage = (inDelta > 0) + 2;
  if (sProcessScrollMessage(mHandle, SB_CTL, scrollMessage, abs(inDelta))) {
    return SendEvent(2, 0);
  }
  return 1;
}

COsDivider::COsDivider(COsDialog *inDialog, short inID, UINT inFlags) : COsControl(inDialog, 19, inID, inFlags), mDragStartPos(0) {
  Initialize();
}

COsDivider::COsDivider(LPVOID inWindow, short inID, UINT inFlags) : COsControl(inWindow, 19, inID, inFlags), mDragStartPos(0) {
  Initialize();
}

COsDivider::~COsDivider() {
}

void COsDivider::Initialize() {
  mDragging = 0;
  mTracking = 0;
  mMaxPos = 100000;
  mMinPos = -100000;
  SetWindowLongA(static_cast<HWND>(mHandle), GWL_WNDPROC, reinterpret_cast<LONG>(sDividerProc));
}

void COsDivider::UpdateCursor() {
  if (mTracking) {
    OsGuiSetCursor((mFlags & 0x10000) ? 3 : 2);
  } else {
    OsGuiSetCursor(0);
  }
}

void COsDivider::OnDivMouseDown() {
  SetCapture(static_cast<HWND>(mHandle));
  mDragging = 1;
  OsGuiGetCursorPosition(&mDragStartMouseX, &mDragStartMouseY);

  NTempest::CiRect parentRect = OsGuiGetWindowRect(GetParent(static_cast<HWND>(mHandle)), 1);
  NTempest::CiRect controlRect = OsGuiGetWindowRect(mHandle, 0);
  mDragStartPos = controlRect;
  mDragStartPos.l -= parentRect.l;
  mDragStartPos.r -= parentRect.l;
  mDragStartPos.t -= parentRect.t;
  mDragStartPos.b -= parentRect.t;
  UpdateCursor();
}

void COsDivider::OnDivMouseMove(int, int) {
  if (!mTracking) {
    TRACKMOUSEEVENT track;
    track.cbSize = sizeof(track);
    track.dwFlags = TME_LEAVE;
    track.hwndTrack = static_cast<HWND>(mHandle);
    mTracking = _TrackMouseEvent(&track);
  }
  UpdateCursor();

  if (mDragging) {
    int curMouseX = 0;
    int curMouseY = 0;
    OsGuiGetCursorPosition(&curMouseX, &curMouseY);
    NTempest::CiRect newPos = mDragStartPos;

    if (mFlags & 0x10000) {
      newPos.t = mDragStartPos.t + curMouseY - mDragStartMouseY;
      if (newPos.t <= mMinPos) {
        newPos.t = mMinPos;
      }
      if (newPos.t >= mMaxPos) {
        newPos.t = mMaxPos;
      }
      newPos.b += newPos.t - mDragStartPos.t;
    } else {
      newPos.l = mDragStartPos.l + curMouseX - mDragStartMouseX;
      if (newPos.l <= mMinPos) {
        newPos.l = mMinPos;
      }
      if (newPos.l >= mMaxPos) {
        newPos.l = mMaxPos;
      }
      newPos.r += newPos.l - mDragStartPos.l;
    }

    OsGuiSetWindowRect(mHandle, newPos);
    sDoCallback(1, 0, 0);
  }
}

void COsDivider::OnDivMouseUp() {
  if (mDragging) {
    ReleaseCapture();
    mDragging = 0;
    SendEvent(2, 0);
  }
  UpdateCursor();
}

void COsDivider::OnDivMouseLeave() {
  mTracking = 0;
  UpdateCursor();
}

void COsDivider::SetPositionRange(int inMin, int inMax) {
  mMinPos = inMin;
  mMaxPos = inMax;
}

static int CALLBACK sDividerProc(HWND__ *hwnd, UINT msg, UINT wParam, long lParam) {
  COsDivider *divider = static_cast<COsDivider *>(sGetOsGuiPointer(hwnd));
  if (divider) {
    switch (msg) {
      case WM_MOUSEMOVE:
        divider->OnDivMouseMove(static_cast<short>(LOWORD(lParam)), static_cast<short>(HIWORD(lParam)));
        break;
      case WM_LBUTTONDOWN:
        divider->OnDivMouseDown();
        break;
      case WM_LBUTTONUP:
        divider->OnDivMouseUp();
        break;
      case WM_MOUSELEAVE:
        divider->OnDivMouseLeave();
        break;
    }
  }
  WNDPROC proc = reinterpret_cast<WNDPROC>(GetClassLongA(hwnd, GCL_WNDPROC));
  return CallWindowProcA(proc, hwnd, msg, wParam, lParam);
}

COsWindow::COsWindow(LPVOID inWindow) : mHandle(inWindow), mMinSize(-1, -1) {
  sSetOsGuiPointer(static_cast<HWND>(inWindow), this);
}

COsWindow::~COsWindow() {
  sRemoveOsGuiPointer(static_cast<HWND>(mHandle));
}

void COsWindow::SetMinSize(int inW, int inH) {
  mMinSize.x = inW;
  mMinSize.y = inH;
}

void COsWindow::GetMinSize(int *outW, int *outH) {
  *outW = mMinSize.x;
  *outH = mMinSize.y;
}

void COsWindow::SetCursor(int inCursor) {
  HCURSOR cursor = sWinCursor(inCursor);
  if (cursor) {
    SetClassLongA(static_cast<HWND>(mHandle), GCL_HCURSOR, reinterpret_cast<LONG>(cursor));
  }
}

void COsWindow::SetIcon(LPCSTR inName) {
  OsGuiSetWindowIcon(mHandle, inName);
}

void COsWindow::SetInputFocus() {
  SetFocus(static_cast<HWND>(mHandle));
}

void COsWindow::OnResize() {
}

HICON__ *sWinCursor(int inCursor) {
  FATALASSERT(inCursor >= 0 && inCursor < 4);
  switch (inCursor) {
    case 0:
      return static_cast<HCURSOR>(LoadCursorA(0, IDC_ARROW));
    case 1:
      return static_cast<HCURSOR>(LoadCursorA(0, IDC_WAIT));
    case 2:
      return static_cast<HCURSOR>(LoadCursorA(0, IDC_SIZEWE));
    case 3:
      return static_cast<HCURSOR>(LoadCursorA(0, IDC_SIZENS));
  }
  return 0;
}

void OsGuiSetCursor(int inCursor) {
  HCURSOR cursor = sWinCursor(inCursor);
  if (cursor) {
    SetCursor(cursor);
  }
}

void OsGuiShowCursor(int inVal) {
  ShowCursor(inVal);
}

void OsGuiGetCursorPosition(int *outX, int *outY) {
  POINT p;
  if (GetCursorPos(&p)) {
    *outX = p.x;
    *outY = p.y;
  }
}

void OsGuiSetWindowTitle(LPVOID inWindow, LPCSTR inText) {
  SetWindowTextA(static_cast<HWND>(inWindow), inText);
}

void OsGuiSetWindowIcon(LPVOID inWindow, LPCSTR inName) {
  HMODULE module = GetModuleHandleA(0);
  HICON   oldIcon = reinterpret_cast<HICON>(
      SetClassLongA(static_cast<HWND>(inWindow), GCL_HICON, reinterpret_cast<LONG>(LoadImageA(module, inName, IMAGE_ICON, 32, 32, 0)))
  );
  if (oldIcon) {
    DestroyIcon(oldIcon);
  }
  oldIcon = reinterpret_cast<HICON>(
      SetClassLongA(static_cast<HWND>(inWindow), GCL_HICONSM, reinterpret_cast<LONG>(LoadImageA(module, inName, IMAGE_ICON, 16, 16, 0)))
  );
  if (oldIcon) {
    DestroyIcon(oldIcon);
  }
}

void OsGuiSetWindowRect(LPVOID inWindow, const NTempest::CiRect &inRect) {
  SetWindowPos(static_cast<HWND>(inWindow), 0, inRect.l, inRect.t, inRect.Width(), inRect.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
}

void OsGuiBringWindowToFront(LPVOID inWindow) {
  BringWindowToTop(static_cast<HWND>(inWindow));
}

void OsGuiShowWindow(LPVOID inWindow, int inVal) {
  SetWindowPos(static_cast<HWND>(inWindow), 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | (inVal ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));
}

void OsGuiEnableWindow(LPVOID inWindow, int inVal) {
  EnableWindow(static_cast<HWND>(inWindow), inVal);
}

int OsGuiWindowEnabled(LPVOID inWindow) {
  return IsWindowEnabled(static_cast<HWND>(inWindow));
}

LPVOID OsGuiGetWindow(int inWindowType) {
  switch (inWindowType) {
    case 0:
      ASSERT(s_GxDevWindow);
      return s_GxDevWindow;

    case 1:
      return GetActiveWindow();

    case 2:
      return GetForegroundWindow();
  }

  return 0;
}

void OsGuiSetGxWindow(LPVOID window) {
  s_GxDevWindow = window;
}

void OsGuiMaximizeWindow(LPVOID inWindow, int inVal) {
  HWND wnd = static_cast<HWND>(inWindow);
  ShowWindow(wnd, IsWindowVisible(wnd) ? (inVal ? SW_MAXIMIZE : SW_RESTORE) : (inVal ? SW_SHOWMAXIMIZED : SW_HIDE));
}

BOOL OsGuiWindowMaximized(LPVOID inWindow) {
  WINDOWPLACEMENT wp;
  wp.length = sizeof(wp);
  GetWindowPlacement(static_cast<HWND>(inWindow), &wp);
  return wp.showCmd == SW_SHOWMAXIMIZED;
}

void OsGuiMinimizeWindow(LPVOID inWindow, int inVal) {
  HWND wnd = static_cast<HWND>(inWindow);
  ShowWindow(wnd, IsWindowVisible(wnd) ? (inVal ? SW_MINIMIZE : SW_RESTORE) : (inVal ? SW_MINIMIZE : SW_HIDE));
}

BOOL OsGuiWindowMinimized(LPVOID inWindow) {
  WINDOWPLACEMENT wp;
  wp.length = sizeof(wp);
  GetWindowPlacement(static_cast<HWND>(inWindow), &wp);
  return wp.showCmd == SW_SHOWMINIMIZED;
}
void OsGuiSetWindowRestoredRect(LPVOID inWindow, const NTempest::CiRect &inRect) {
  WINDOWPLACEMENT wp;
  wp.length = sizeof(wp);
  GetWindowPlacement(static_cast<HWND>(inWindow), &wp);
  RECT winRect;
  sCiRectToWinRect(&inRect, &winRect);
  wp.rcNormalPosition = winRect;
  wp.showCmd = IsWindowVisible(static_cast<HWND>(inWindow)) ? SW_SHOWNA : SW_HIDE;
  SetWindowPlacement(static_cast<HWND>(inWindow), &wp);
}

BOOL OsGuiWindowIsCursorInside(LPVOID inWindow, int inClientOnly) {
  int cx = 0;
  int cy = 0;
  OsGuiGetCursorPosition(&cx, &cy);
  POINT cursor = {cx, cy};
  if (WindowFromPoint(cursor) != static_cast<HWND>(inWindow)) {
    return 0;
  }
  if (inClientOnly) {
    NTempest::CiRect winRect = OsGuiGetWindowRect(inWindow, 1);
    return cx >= winRect.l && cx <= winRect.r && cy >= winRect.t && cy <= winRect.b;
  }
  return 1;
}

NTempest::CiRect OsGuiGetScreenBounds() {
  RECT             workArea;
  NTempest::CiRect sb;
  SystemParametersInfoA(SPI_GETWORKAREA, 0, &workArea, 0);
  sWinRectToCiRect(&workArea, &sb);
  return sb;
}

void OsGuiBeep() {
  MessageBeep(0);
}

int OsGuiMessageBox(LPVOID inParentWindow, int inStyle, LPCSTR inMessage, LPCSTR inTitle) {
  UINT messageBoxStyle = 0;
  int  result;

  switch (inStyle) {
    case 0:
      messageBoxStyle = MB_OK;
      break;
    case 1:
      messageBoxStyle = MB_OKCANCEL;
      break;
    case 2:
      messageBoxStyle = MB_YESNO;
      break;
    case 3:
      messageBoxStyle = MB_YESNOCANCEL;
      break;
  }

  result = MessageBoxA(static_cast<HWND>(inParentWindow), inMessage, inTitle ? inTitle : "", messageBoxStyle);

  if (result == IDOK || result == IDYES) {
    return 0;
  }

  if (result == IDNO) {
    return 1;
  }

  return 2;
}

BOOL OsGuiIsModifierKeyDown(int inKey) {
  int virtualKey;

  ASSERT(inKey >= 0 && inKey < 3);

  switch (inKey) {
    case 0:
      virtualKey = VK_CONTROL;
      break;

    case 1:
      virtualKey = VK_SHIFT;
      break;

    case 2:
      virtualKey = VK_MENU;
      break;

    default:
      ASSERT(0);
      return 0;
  }

  return (GetKeyState(virtualKey) & 0xF000) != 0;
}

void OsGuiGetHotkeyText(const OsGuiMenuHotkey &inHotkey, char *inBuf, int inBufSize) {
  sGetHotkeyText(inHotkey.keyID, inHotkey.modKeyID, inBuf, inBufSize);
}

long OsGuiWindowProc(LPVOID _hWnd, UINT uMsg, UINT wParam, long lParam) {
  HWND hwnd = static_cast<HWND>(_hWnd);
  switch (uMsg) {
    case WM_DRAWITEM:
      return sHandleDrawItem(lParam);
    case WM_CTLCOLORBTN:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
      return reinterpret_cast<long>(sHandleCtlColor(wParam, lParam));
    case WM_NOTIFY: {
      NMHDR      *notify = reinterpret_cast<NMHDR *>(lParam);
      COsControl *control = static_cast<COsControl *>(sGetOsGuiPointer(notify->hwndFrom));
      return control ? control->OnNotify(notify->code, notify) : 0;
    }
    case WM_COMMAND:
      if (lParam) {
        COsControl *control = static_cast<COsControl *>(sGetOsGuiPointer(reinterpret_cast<HWND>(lParam)));
        return control ? control->OnCommand(wParam) : 0;
      }
      if (HIWORD(wParam) == 0 || HIWORD(wParam) == 1) {
        int command = sMenuRaw2RealID(LOWORD(wParam));
        sDoCallback(0, (command >> 8) & 0xFF, command & 0xFF);
      }
      break;
    case WM_HSCROLL:
    case WM_VSCROLL:
      if (lParam) {
        COsControl *control = static_cast<COsControl *>(sGetOsGuiPointer(reinterpret_cast<HWND>(lParam)));
        return control ? control->OnScroll(wParam) : 0;
      }
      break;
    case WM_ENTERMENULOOP:
    case WM_ENTERSIZEMOVE:
      sStartIdle();
      return 0;
    case WM_EXITMENULOOP:
    case WM_EXITSIZEMOVE:
      sStopIdle();
      return 0;
    case WM_ENTERIDLE:
      sDoCallback(1, 0, 0);
      break;
  }

  COsWindow *window = static_cast<COsWindow *>(sGetOsGuiPointer(hwnd));
  if (window) {
    if (uMsg == WM_SIZE) {
      window->OnResize();
    } else if (uMsg == WM_GETMINMAXINFO) {
      int minWidth;
      int minHeight;
      window->GetMinSize(&minWidth, &minHeight);
      MINMAXINFO *minMaxInfo = reinterpret_cast<MINMAXINFO *>(lParam);
      if (minWidth != -1) {
        minMaxInfo->ptMinTrackSize.x = minWidth;
      }
      if (minHeight != -1) {
        minMaxInfo->ptMinTrackSize.y = minHeight;
      }
      return 0;
    }
  }
  return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}
