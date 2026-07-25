#include "OsGui.h"
#include "Input.h"

#include <storm.h>
#include <Tempest/cirect.h>
#include <windows.h>
#include <commctrl.h>
#include <malloc.h>

static HINSTANCE                     sAppInstance;
static void                         *s_GxDevWindow;
static TSGrowableArray<COsDialog *>  sDialogs;
static TSGrowableArray<COsMenuBar *> sMenubars;
static int                           sMenuHotkeysEnabled = 1;
static int                           sMasterTooltipsEnabled;
static HWND                          sGlobalTips;
static unsigned int                  sIdleTimerID;

struct OsGuiCodeTranslation {
  int winCode;
  int ctrlType;
  int osGuiCode;
};

static const OsGuiCodeTranslation table[18] = {
    {0, 0, 0},      {1, 0, 0},     {4, 768, 2},  {4, 512, 13},
    {5, 1, 2},      {6, 1, 2},     {6, 2, 1},    {7, 0, 2},
    {10, -402, 2},  {10, -3, 1},   {10, -411, 8},{10, -7, 14},
    {10, -8, 13},   {11, 0, 0},    {13, 4, 2},   {14, 0, 2},
    {15, -551, 2},  {16, -3, 1}};

struct OsGuiCallbackInfo {
  void(__fastcall *function)(const OsGuiCallbackParams &);
  void *userParam;
};
static OsGuiCallbackInfo sCallbacks[2];

typedef long(__fastcall *OSWINDOWPROC)(void *, unsigned int, unsigned int, long);
void __fastcall OsSetWindowProc(OSWINDOWPROC windowproc);
long __fastcall OsGuiWindowProc(void *_hWnd, unsigned int uMsg, unsigned int wParam, long lParam);

static HWND__* sCreateTooltips(HWND__* inOwner) {
  HWND tips = CreateWindowExA(
      0, TOOLTIPS_CLASSA, 0, WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
      inOwner, 0, sAppInstance, 0
  );
  SetWindowPos(tips, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
  SendMessageA(tips, TTM_SETDELAYTIME, TTDT_AUTOPOP, 300);
  SendMessageA(tips, TTM_ACTIVATE, sMasterTooltipsEnabled, 0);
  return tips;
}

static HWND__* sGetGlobalTips() {
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

static void *__fastcall sGetOsGuiPointer(HWND hwnd) {
  return GetPropA(hwnd, "OsGuiPointer");
}

static void sSetOsGuiPointer(HWND__* hwnd, void* data) {
  SetPropA(hwnd, "OsGuiPointer", data);
}

static void sRemoveOsGuiPointer(HWND__* hwnd) {
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

static void CALLBACK sIdleTimerProc(HWND__*, unsigned int, unsigned int, unsigned long) {
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

void __fastcall OsGuiMenuSelect(int menuID, int itemID) {
  PostMessageA(GetActiveWindow(), WM_COMMAND, MAKEWPARAM(itemID, menuID), 0);
}

static void sWinRectToCiRect(const tagRECT* winRect, NTempest::CiRect* outRect) {
  outRect->t = winRect->top;
  outRect->l = winRect->left;
  outRect->b = winRect->bottom;
  outRect->r = winRect->right;
}

static void sCiRectToWinRect(const NTempest::CiRect* inRect, tagRECT* outRect) {
  outRect->top = inRect->t;
  outRect->left = inRect->l;
  outRect->bottom = inRect->b;
  outRect->right = inRect->r;
}

void __fastcall OsGuiInitialize() {
  OsSetWindowProc(OsGuiWindowProc);
  INITCOMMONCONTROLSEX initCtrls;
  initCtrls.dwSize = sizeof(initCtrls);
  initCtrls.dwICC = ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES | ICC_TAB_CLASSES;
  ASSERT(InitCommonControlsEx(&initCtrls));
}

void __fastcall OsGuiDestroy() {
  OsSetWindowProc(0);
  if (sGlobalTips) {
    DestroyWindow(sGlobalTips);
    sGlobalTips = 0;
  }
}

void __fastcall OsGuiSetApplicationInfo(void *inData) {
  sAppInstance = static_cast<HINSTANCE>(inData);
}

int __fastcall OsGuiProcessMessage(void *inMsgData) {
  MSG *message = static_cast<MSG *>(inMsgData);
  int  handled = 0;

  if (!sAppInstance) {
    return 0;
  }

  if (sMenuHotkeysEnabled) {
    unsigned int index = 0;

    while (index < sMenubars.Count()) {
      COsMenuBar *menuBar;
      HACCEL      accelTable;

      menuBar = sMenubars[index];
      accelTable = static_cast<HACCEL>(menuBar->GetAccelerators());

      if (accelTable) {
        HWND menuWindow = static_cast<HWND>(menuBar->GetWindow());
        HWND activeWindow = static_cast<HWND>(OsGuiGetWindow(1));
        int  canTranslate = menuWindow == activeWindow;

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
    unsigned int index = 0;

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

void __fastcall OsGuiEnableTooltips(int inVal) {
  if (inVal != sMasterTooltipsEnabled) {
    sMasterTooltipsEnabled = inVal;
    sEnableGlobalTips(inVal);
  }
}

void __fastcall OsGuiEnableMenuHotkeys(int inVal) {
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
    case 0: return VK_SHIFT;
    case 1: return VK_CONTROL;
    case 2: return VK_MENU;
    case 32: return VK_SPACE;
    case 256: return 0xC0;
    case 512: return VK_ESCAPE;
    case 513: return VK_RETURN;
    case 514: return VK_BACK;
    case 515: return VK_TAB;
    case 516: return VK_LEFT;
    case 517: return VK_UP;
    case 518: return VK_RIGHT;
    case 519: return VK_DOWN;
    case 520: return VK_INSERT;
    case 521: return VK_DELETE;
    case 522: return VK_HOME;
    case 523: return VK_END;
    case 524: return VK_PRIOR;
    case 525: return VK_NEXT;
    case 526: return VK_CAPITAL;
    case 527: return VK_NUMLOCK;
    case 528: return VK_SCROLL;
    case 529: return VK_PAUSE;
    case 530: return VK_SNAPSHOT;
    default:
      ASSERT(key);
      return -1;
  }
}

static void sHotkeyToAccel(OsGuiMenuHotkey* hotkey, tagACCEL* accel) {
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

static void sGetHotkeyText(int keyID, int modID, char* buf, int bufSize) {
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
    const char *text = "Unknown";
    switch (keyID) {
      case 32: text = "Space"; break;
      case 274: text = "["; break;
      case 275: text = "]"; break;
      case 512: text = "Esc"; break;
      case 513: text = "Enter"; break;
      case 514: text = "Backspace"; break;
      case 515: text = "Tab"; break;
      case 516: text = "Left"; break;
      case 517: text = "Up"; break;
      case 518: text = "Right"; break;
      case 519: text = "Down"; break;
      case 521: text = "Delete"; break;
    }
    SStrCopy(keyText, text, sizeof(keyText));
  }
  if (keyText[0]) {
    SStrPrintf(buf, bufSize, "%s%s", modText, keyText);
  }
}

static int sNCodeToItemCode(int nCode, int ctrlType) {
  for (unsigned int i = 0; i < sizeof(table) / sizeof(table[0]); ++i) {
    if (table[i].ctrlType == ctrlType && table[i].winCode == nCode) {
      return table[i].osGuiCode;
    }
  }
  return -1;
}

static int sHandleDrawItem(long lParam) {
  DRAWITEMSTRUCT *draw = reinterpret_cast<DRAWITEMSTRUCT *>(lParam);
  COsControl *control = static_cast<COsControl *>(sGetOsGuiPointer(draw->hwndItem));
  if (!control) {
    return 0;
  }

  unsigned int state = 0;
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

static void* sHandleCtlColor(unsigned int wParam, long lParam) {
  COsControl *control = static_cast<COsControl *>(
      sGetOsGuiPointer(reinterpret_cast<HWND>(lParam)));
  return control ? control->OnSetColors(reinterpret_cast<void *>(wParam)) : 0;
}

static int sDlgProc(HWND__* hdlg, unsigned int msg, unsigned int wParam, long lParam) {
  COsDialog *dialog = static_cast<COsDialog *>(sGetOsGuiPointer(hdlg));
  switch (msg) {
    case WM_DRAWITEM:
      return sHandleDrawItem(lParam);
    case WM_CTLCOLORBTN:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
      return reinterpret_cast<int>(sHandleCtlColor(wParam, lParam));
    case WM_NOTIFY: {
      NMHDR *notify = reinterpret_cast<NMHDR *>(lParam);
      COsControl *control = static_cast<COsControl *>(sGetOsGuiPointer(notify->hwndFrom));
      return control ? control->OnNotify(notify->code, notify) : 0;
    }
    case WM_COMMAND:
      if (lParam) {
        COsControl *control = static_cast<COsControl *>(sGetOsGuiPointer(reinterpret_cast<HWND>(lParam)));
        return control ? control->OnCommand(wParam) : 0;
      }
      if (dialog && LOWORD(wParam) != IDOK && LOWORD(wParam) != IDCANCEL && GetMenu(hdlg)) {
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

static int sDisableWindow(HWND__* hwnd, long param) {
  TSGrowableArray<void *> *windows = reinterpret_cast<TSGrowableArray<void *> *>(param);
  if (reinterpret_cast<HINSTANCE>(GetWindowLongA(hwnd, GWL_HINSTANCE)) == sAppInstance && IsWindowEnabled(hwnd)) {
    EnableWindow(hwnd, FALSE);
    *windows->New() = hwnd;
  }
  return 1;
}

void *COsDialog::GetParentWindow() {
  return GetParent(static_cast<HWND>(mHandle));
}

COsControl *COsDialog::FindControl(void *inHandle) {
  unsigned int index = 0;

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

int COsDialog::ProcessMessage(void *inMsgData) {
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

int COsDialog::IsInFront() {
  return mHandle == OsGuiGetWindow(2);
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

int COsDialog::OnControlTab() {
  int          controlCount = static_cast<int>(mControls.Count());
  unsigned int index = 0;

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

int COsDialog::HasFlag(unsigned int inFlag) {
  return (mFlags & inFlag) != 0;
}

int COsControl::SendEvent(int inEvent, int inCode) {
  if (OnEvent(mID, inEvent, inCode)) {
    return 1;
  }

  if (mDialog) {
    return mDialog->OnEvent(mID, inEvent, inCode);
  }

  return 0;
}

static HBITMAP__* sBitmapFromImageData(int inWidth, int inHeight, void* inData, HDC__* inDC) {
  BITMAPINFO bmInfo;
  memset(&bmInfo, 0, sizeof(bmInfo));
  bmInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmInfo.bmiHeader.biWidth = inWidth;
  bmInfo.bmiHeader.biHeight = -inHeight;
  bmInfo.bmiHeader.biPlanes = 1;
  bmInfo.bmiHeader.biBitCount = 32;
  return CreateDIBitmap(
      inDC, &bmInfo.bmiHeader, CBM_INIT, inData, &bmInfo, DIB_RGB_COLORS);
}

static HBITMAP__* sMaskFromImageData(int inWidth, int inHeight, void* inData, HDC__* inDC) {
  int rowBytes = 2 * ((inWidth - 1) / 8 + 1);
  unsigned char *bits = static_cast<unsigned char *>(_alloca(rowBytes * inHeight));
  memset(bits, 0, rowBytes * inHeight);
  unsigned char *rgba = static_cast<unsigned char *>(inData);
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
  info->bmiColors[1].rgbBlue = info->bmiColors[1].rgbGreen = info->bmiColors[1].rgbRed = 0xFF;
  HBITMAP bitmap = CreateDIBitmap(inDC, &info->bmiHeader, CBM_INIT, bits, info, DIB_RGB_COLORS);
  SMemFree(info, __FILE__, __LINE__, 0);
  return bitmap;
}

static int sEditBoxProc(HWND__* hwnd, unsigned int msg, unsigned int wParam, long lParam) {
  COsEditBox *editBox = static_cast<COsEditBox *>(sGetOsGuiPointer(hwnd));
  if (editBox &&
      ((msg >= WM_KEYDOWN && msg <= WM_KEYUP) ||
       msg == WM_MOUSEMOVE || msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP ||
       msg == WM_RBUTTONDOWN || msg == WM_RBUTTONUP)) {
    editBox->UpdateSelection();
  }
  WNDPROC proc = reinterpret_cast<WNDPROC>(GetClassLongA(hwnd, GCL_WNDPROC));
  return CallWindowProcA(proc, hwnd, msg, wParam, lParam);
}

static int __fastcall sIsCharacterAllowed(char inChar, unsigned int inFilters) {
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

int COsTreeView::IsCharacterAllowed(char inChar) {
  if (!GetEditControl()) {
    return 1;
  }

  if (!mFiltersEnabled) {
    return 1;
  }

  return sIsCharacterAllowed(inChar, mFilters);
}

int COsEditBox::IsCharacterAllowed(char inChar) {
  if (!mFiltersEnabled) {
    return 1;
  }

  return sIsCharacterAllowed(inChar, mFilters);
}

int COsTabControl::GetNumItems() {
  return static_cast<int>(SendMessageA(static_cast<HWND>(mHandle), 0x1304, 0, 0));
}

int COsTabControl::OnControlTab() {
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

int COsDialog::OnEvent(int inItemID, int inNotifyCode, int inCode) {
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

static int sTreeViewProc(HWND__* hwnd, unsigned int msg, unsigned int wParam, long lParam) {
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

static void sTVGetSelectInfo(COsTreeView* inView, void* inItem, void* inParam) {
  struct SelectInfo {
    int count;
    void *first;
    unsigned int flags;
    void *previous;
    void *last;
  };
  SelectInfo *info = static_cast<SelectInfo *>(inParam);
  TVITEMA item;
  memset(&item, 0, sizeof(item));
  item.mask = TVIF_STATE;
  item.hItem = static_cast<HTREEITEM>(inItem);
  item.stateMask = TVIS_SELECTED;
  TreeView_GetItem(static_cast<HWND>(inView->GetHandle()), &item);
  if (item.state & TVIS_SELECTED) {
    ++info->count;
    if (info->count == 1) {
      info->first = info->previous = info->last = inItem;
      info->flags |= 3;
    } else {
      if ((info->flags & 1) &&
          TreeView_GetParent(static_cast<HWND>(inView->GetHandle()), static_cast<HTREEITEM>(inItem)) !=
              TreeView_GetParent(static_cast<HWND>(inView->GetHandle()), static_cast<HTREEITEM>(info->first))) {
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

static void sTVSelect(COsTreeView* inView, void* inItem, void* inParam) {
  inView->SelectItem(inItem, *static_cast<int *>(inParam));
}

void COsTreeView::SelectItem(void *inItem, int inVal) {
  if (mFlags & 0x40000) {
    TVITEMA itemInfo;
    memset(&itemInfo, 0, sizeof(itemInfo));
    itemInfo.mask = TVIF_STATE;
    itemInfo.hItem = static_cast<HTREEITEM>(inItem);
    itemInfo.state = inVal ? TVIS_SELECTED : 0;
    itemInfo.stateMask = TVIS_SELECTED;
    SendMessageA(
        static_cast<HWND>(mHandle), TVM_SETITEMA, 0,
        reinterpret_cast<LPARAM>(&itemInfo));
    SendEvent(2, 0);
  } else {
    SendMessageA(
        static_cast<HWND>(mHandle), TVM_SELECTITEM, TVGN_CARET,
        inVal ? reinterpret_cast<LPARAM>(inItem) : 0);
  }
}

void *COsTreeView::GetEditControl() {
  return reinterpret_cast<void *>(SendMessageA(static_cast<HWND>(mHandle), 0x110F, 0, 0));
}

static int sSpinButtonProc(HWND__* hwnd, unsigned int msg, unsigned int wParam, long lParam) {
  COsSpinButton *spin = static_cast<COsSpinButton *>(sGetOsGuiPointer(hwnd));
  if (spin && msg == WM_LBUTTONUP) {
    spin->OnSpinMouseUp();
  }
  WNDPROC proc = reinterpret_cast<WNDPROC>(GetClassLongA(hwnd, GCL_WNDPROC));
  return CallWindowProcA(proc, hwnd, msg, wParam, lParam);
}

void COsSpinButton::OnSpinMouseUp() {
  SendEvent(5, 0);
}

int COsEditBox::GetSelectionSize() {
  unsigned int selStart;
  unsigned int selEnd;
  SendMessageA(
      static_cast<HWND>(mHandle),
      EM_GETSEL,
      reinterpret_cast<WPARAM>(&selStart),
      reinterpret_cast<LPARAM>(&selEnd));
  return selEnd - selStart;
}

void COsEditBox::UpdateSelection() {
  int selectionSize = GetSelectionSize();
  if (selectionSize != mSelSize) {
    mSelSize = selectionSize;
    SendEvent(19, 0);
  }
}

static int sConvertScrollMsg(int inWParam) {
  switch (LOWORD(inWParam)) {
    case SB_LINEUP: return 3;
    case SB_LINEDOWN: return 2;
    case SB_PAGEUP: return 5;
    case SB_PAGEDOWN: return 4;
    case SB_THUMBPOSITION: return 0;
    case SB_THUMBTRACK: return 1;
    case SB_TOP: return 7;
    case SB_BOTTOM: return 6;
  }
  return -1;
}

static int sProcessScrollMessage(void* inWindow, int inBarType, int inScrollMsg, int inInc) {
  SCROLLINFO info;
  info.cbSize = sizeof(info);
  info.fMask = SIF_ALL;
  GetScrollInfo(static_cast<HWND>(inWindow), inBarType, &info);

  int oldPos = info.nPos;
  int newPos = oldPos;
  switch (inScrollMsg) {
    case 0:
    case 1: newPos = info.nTrackPos; break;
    case 2: newPos += inInc; break;
    case 3: newPos -= inInc; break;
    case 4: newPos += info.nPage; break;
    case 5: newPos -= info.nPage; break;
    case 6: newPos = info.nMax; break;
    case 7: newPos = info.nMin; break;
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

static int sDividerProc(HWND__* hwnd, unsigned int msg, unsigned int wParam, long lParam) {
  WNDPROC proc = reinterpret_cast<WNDPROC>(GetClassLongA(hwnd, GCL_WNDPROC));
  return CallWindowProcA(proc, hwnd, msg, wParam, lParam);
}

HICON__* __fastcall sWinCursor(int inCursor) {
  FATALASSERT(inCursor >= 0 && inCursor < 4);
  switch (inCursor) {
    case 0: return static_cast<HCURSOR>(LoadCursorA(0, IDC_ARROW));
    case 1: return static_cast<HCURSOR>(LoadCursorA(0, IDC_WAIT));
    case 2: return static_cast<HCURSOR>(LoadCursorA(0, IDC_SIZEWE));
    case 3: return static_cast<HCURSOR>(LoadCursorA(0, IDC_SIZENS));
  }
  return 0;
}

void __fastcall OsGuiSetCursor(int inCursor) {
  HCURSOR cursor = sWinCursor(inCursor);
  if (cursor) {
    SetCursor(cursor);
  }
}

void __fastcall OsGuiShowCursor(int inVal) {
  ShowCursor(inVal);
}

void __fastcall OsGuiGetCursorPosition(int* outX, int* outY) {
  POINT p;
  if (GetCursorPos(&p)) {
    *outX = p.x;
    *outY = p.y;
  }
}

void __fastcall OsGuiSetWindowTitle(void *inWindow, const char *inText) {
  SetWindowTextA(static_cast<HWND>(inWindow), inText);
}

void __fastcall OsGuiSetWindowIcon(void* inWindow, const char* inName) {
  HMODULE module = GetModuleHandleA(0);
  HICON oldIcon = reinterpret_cast<HICON>(
      SetClassLongA(static_cast<HWND>(inWindow), GCL_HICON,
                    reinterpret_cast<LONG>(LoadImageA(module, inName, IMAGE_ICON, 32, 32, 0)))
  );
  if (oldIcon) {
    DestroyIcon(oldIcon);
  }
  oldIcon = reinterpret_cast<HICON>(
      SetClassLongA(static_cast<HWND>(inWindow), GCL_HICONSM,
                    reinterpret_cast<LONG>(LoadImageA(module, inName, IMAGE_ICON, 16, 16, 0)))
  );
  if (oldIcon) {
    DestroyIcon(oldIcon);
  }
}

void __fastcall OsGuiSetWindowRect(void* inWindow, const NTempest::CiRect& inRect) {
  SetWindowPos(
      static_cast<HWND>(inWindow), 0, inRect.l, inRect.t, inRect.Width(), inRect.Height(),
      SWP_NOZORDER | SWP_NOACTIVATE
  );
}

void __fastcall OsGuiBringWindowToFront(void* inWindow) {
  BringWindowToTop(static_cast<HWND>(inWindow));
}

void __fastcall OsGuiShowWindow(void* inWindow, int inVal) {
  SetWindowPos(
      static_cast<HWND>(inWindow), 0, 0, 0, 0, 0,
      SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | (inVal ? SWP_SHOWWINDOW : SWP_HIDEWINDOW)
  );
}

void __fastcall OsGuiEnableWindow(void* inWindow, int inVal) {
  EnableWindow(static_cast<HWND>(inWindow), inVal);
}

int __fastcall OsGuiWindowEnabled(void* inWindow) {
  return IsWindowEnabled(static_cast<HWND>(inWindow));
}

void *__fastcall OsGuiGetWindow(int inWindowType) {
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

void __fastcall OsGuiSetGxWindow(void *window) {
  s_GxDevWindow = window;
}

void __fastcall OsGuiMaximizeWindow(void* inWindow, int inVal) {
  HWND wnd = static_cast<HWND>(inWindow);
  ShowWindow(wnd, IsWindowVisible(wnd) ? (inVal ? SW_MAXIMIZE : SW_RESTORE) : (inVal ? SW_SHOWMAXIMIZED : SW_HIDE));
}

int __fastcall OsGuiWindowMaximized(void* inWindow) {
  WINDOWPLACEMENT placement;
  placement.length = sizeof(placement);
  GetWindowPlacement(static_cast<HWND>(inWindow), &placement);
  return placement.showCmd == SW_SHOWMAXIMIZED;
}

void __fastcall OsGuiMinimizeWindow(void* inWindow, int inVal) {
  HWND wnd = static_cast<HWND>(inWindow);
  ShowWindow(wnd, IsWindowVisible(wnd) ? (inVal ? SW_MINIMIZE : SW_RESTORE) : (inVal ? SW_SHOWMINIMIZED : SW_HIDE));
}

int __fastcall OsGuiWindowMinimized(void* inWindow) {
  WINDOWPLACEMENT placement;
  placement.length = sizeof(placement);
  GetWindowPlacement(static_cast<HWND>(inWindow), &placement);
  return placement.showCmd == SW_SHOWMINIMIZED;
}
void __fastcall OsGuiSetWindowRestoredRect(void* inWindow, const NTempest::CiRect& inRect) {
  WINDOWPLACEMENT placement;
  placement.length = sizeof(placement);
  GetWindowPlacement(static_cast<HWND>(inWindow), &placement);
  sCiRectToWinRect(&inRect, &placement.rcNormalPosition);
  placement.showCmd = IsWindowVisible(static_cast<HWND>(inWindow)) ? SW_SHOWNA : SW_HIDE;
  SetWindowPlacement(static_cast<HWND>(inWindow), &placement);
}

int __fastcall OsGuiWindowIsCursorInside(void* inWindow, int inClientOnly) {
  POINT p;
  GetCursorPos(&p);
  HWND wnd = static_cast<HWND>(inWindow);
  if (WindowFromPoint(p) != wnd) {
    return 0;
  }
  if (inClientOnly) {
    RECT rect;
    GetClientRect(wnd, &rect);
    POINT origin = {rect.left, rect.top};
    ClientToScreen(wnd, &origin);
    OffsetRect(&rect, origin.x, origin.y);
    return p.x >= rect.left && p.x <= rect.right && p.y >= rect.top && p.y <= rect.bottom;
  }
  return 1;
}

NTempest::CiRect __fastcall OsGuiGetScreenBounds() {
  RECT workArea;
  NTempest::CiRect result;
  SystemParametersInfoA(SPI_GETWORKAREA, 0, &workArea, 0);
  sWinRectToCiRect(&workArea, &result);
  return result;
}

void __fastcall OsGuiBeep() {
  MessageBeep(0);
}

int __fastcall OsGuiMessageBox(void *inParentWindow, int inStyle, const char *inMessage, const char *inTitle) {
  unsigned int messageBoxStyle = 0;
  int          result;

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

int __fastcall OsGuiIsModifierKeyDown(int inKey) {
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

void __fastcall OsGuiGetHotkeyText(const OsGuiMenuHotkey& inHotkey, char* inBuf, int inBufSize) {
  sGetHotkeyText(inHotkey.keyID, inHotkey.modKeyID, inBuf, inBufSize);
}

long __fastcall OsGuiWindowProc(void* _hWnd, unsigned int uMsg, unsigned int wParam, long lParam) {
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
      NMHDR *notify = reinterpret_cast<NMHDR *>(lParam);
      COsControl *control = static_cast<COsControl *>(sGetOsGuiPointer(notify->hwndFrom));
      return control ? control->OnNotify(notify->code, notify) : 0;
    }
    case WM_COMMAND:
      if (lParam) {
        COsControl *control = static_cast<COsControl *>(sGetOsGuiPointer(reinterpret_cast<HWND>(lParam)));
        return control ? control->OnCommand(wParam) : 0;
      }
      if (HIWORD(wParam) == 0 || HIWORD(wParam) == 1) {
        sDoCallback(0, sMenuRaw2RealID(LOWORD(wParam)), 0);
        return 0;
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
      return 0;
  }
  return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}
