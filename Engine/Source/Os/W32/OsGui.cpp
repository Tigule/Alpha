#include "OsGui.h"

#include <storm.h>
#include <Tempest/cirect.h>
#include <windows.h>

static HINSTANCE                     sAppInstance;
static void                         *s_GxDevWindow;
static TSGrowableArray<COsDialog *>  sDialogs;
static TSGrowableArray<COsMenuBar *> sMenubars;
static int                           sMenuHotkeysEnabled = 1;
static HWND                          sGlobalTips;

static HWND__* sCreateTooltips(HWND__* inOwner) {
    // TODO: implement
    return 0;
}

static HWND__* sGetGlobalTips() {
    // TODO: implement
    return 0;
}

static void sEnableGlobalTips(int inVal) {
    // TODO: implement
}

static void *__fastcall sGetOsGuiPointer(HWND hwnd) {
  return GetPropA(hwnd, "OsGuiPointer");
}

static void sSetOsGuiPointer(HWND__* hwnd, void* data) {
    // TODO: implement
}

static void sRemoveOsGuiPointer(HWND__* hwnd) {
    // TODO: implement
}

static void sDoCallback(int inCB, int type, int subtype) {
    // TODO: implement
}

static void sIdleTimerProc(HWND__*, unsigned int, unsigned int, unsigned long) {
    // TODO: implement
}

static void sStartIdle() {
    // TODO: implement
}

static void sStopIdle() {
    // TODO: implement
}

static int sMenuReal2RawID(int inID) {
    // TODO: implement
    return 0;
}

static int sMenuRaw2RealID(int inID) {
    // TODO: implement
    return 0;
}

void __fastcall OsGuiMenuSelect(int menuID, int itemID) {
    // TODO: implement
}

static void sWinRectToCiRect(const tagRECT* winRect, NTempest::CiRect* outRect) {
    // TODO: implement
}

static void sCiRectToWinRect(const NTempest::CiRect* inRect, tagRECT* outRect) {
    // TODO: implement
}

void __fastcall OsGuiInitialize() {
    // TODO: implement
}

void __fastcall OsGuiDestroy() {
    // TODO: implement
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
    // TODO: implement
}

void __fastcall OsGuiEnableMenuHotkeys(int inVal) {
  sMenuHotkeysEnabled = inVal;
}

static int sKeyToVirtKey(int key) {
    // TODO: implement
    return 0;
}

static void sHotkeyToAccel(OsGuiMenuHotkey* hotkey, tagACCEL* accel) {
    // TODO: implement
}

static void sGetHotkeyText(int keyID, int modID, char* buf, int bufSize) {
    // TODO: implement
}

static int sNCodeToItemCode(int nCode, int ctrlType) {
    // TODO: implement
    return 0;
}

static int sHandleDrawItem(long lParam) {
    // TODO: implement
    return 0;
}

static void* sHandleCtlColor(unsigned int wParam, long lParam) {
    // TODO: implement
    return 0;
}

static int sDlgProc(HWND__* hdlg, unsigned int msg, unsigned int wParam, long lParam) {
    // TODO: implement
    return 0;
}

static int sDisableWindow(HWND__* hwnd, long param) {
    // TODO: implement
    return 0;
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
    // TODO: implement
    return 0;
}

static HBITMAP__* sMaskFromImageData(int inWidth, int inHeight, void* inData, HDC__* inDC) {
    // TODO: implement
    return 0;
}

static int sEditBoxProc(HWND__* hwnd, unsigned int msg, unsigned int wParam, long lParam) {
    // TODO: implement
    return 0;
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
    // TODO: implement
    return 0;
}

static void sTVGetSelectInfo(COsTreeView* inView, void* inItem, void* inParam) {
    // TODO: implement
}

static void sTVSelect(COsTreeView* inView, void* inItem, void* inParam) {
    // TODO: implement
}

void *COsTreeView::GetEditControl() {
  return reinterpret_cast<void *>(SendMessageA(static_cast<HWND>(mHandle), 0x110F, 0, 0));
}

static int sSpinButtonProc(HWND__* hwnd, unsigned int msg, unsigned int wParam, long lParam) {
    // TODO: implement
    return 0;
}

static int sConvertScrollMsg(int inWParam) {
    // TODO: implement
    return 0;
}

static int sProcessScrollMessage(void* inWindow, int inBarType, int inScrollMsg, int inInc) {
    // TODO: implement
    return 0;
}

static int sDividerProc(HWND__* hwnd, unsigned int msg, unsigned int wParam, long lParam) {
    // TODO: implement
    return 0;
}

HICON__* __fastcall sWinCursor(int inCursor) {
    // TODO: implement
    return 0;
}

void __fastcall OsGuiSetCursor(int inCursor) {
    // TODO: implement
}

void __fastcall OsGuiShowCursor(int inVal) {
    // TODO: implement
}

void __fastcall OsGuiGetCursorPosition(int* outX, int* outY) {
    // TODO: implement
}

void __fastcall OsGuiSetWindowTitle(void *inWindow, const char *inText) {
  SetWindowTextA(static_cast<HWND>(inWindow), inText);
}

void __fastcall OsGuiSetWindowIcon(void* inWindow, const char* inName) {
    // TODO: implement
}

void __fastcall OsGuiSetWindowRect(void* inWindow, const NTempest::CiRect& inRect) {
    // TODO: implement
}

void __fastcall OsGuiBringWindowToFront(void* inWindow) {
    // TODO: implement
}

void __fastcall OsGuiShowWindow(void* inWindow, int inVal) {
    // TODO: implement
}

void __fastcall OsGuiEnableWindow(void* inWindow, int inVal) {
    // TODO: implement
}

int __fastcall OsGuiWindowEnabled(void* inWindow) {
    // TODO: implement
    return 0;
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
    // TODO: implement
}

int __fastcall OsGuiWindowMaximized(void* inWindow) {
    // TODO: implement
    return 0;
}

void __fastcall OsGuiMinimizeWindow(void* inWindow, int inVal) {
    // TODO: implement
}

int __fastcall OsGuiWindowMinimized(void* inWindow) {
    // TODO: implement
    return 0;
}
void __fastcall OsGuiSetWindowRestoredRect(void* inWindow, const NTempest::CiRect& inRect) {
    // TODO: implement
}

int __fastcall OsGuiWindowIsCursorInside(void* inWindow, int inClientOnly) {
    // TODO: implement
    return 0;
}

NTempest::CiRect __fastcall OsGuiGetScreenBounds() {
    // TODO: implement
    return NTempest::CiRect();
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
    // TODO: implement
}

long __fastcall OsGuiWindowProc(void* _hWnd, unsigned int uMsg, unsigned int wParam, long lParam) {
    // TODO: implement
    return 0;
}
