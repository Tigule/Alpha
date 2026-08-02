#include "OsClipboard.h"
#include "Debugging.h"
#include "Input.h"

#include <Base/ConvertUTF.h>
#include <storm.h>
#include <windows.h>
#include <malloc.h>

static void FailureMessage(const char *title) {
  char *msgBuffer;

  FormatMessageA(0x1300, 0, GetLastError(), 0x400, reinterpret_cast<char *>(&msgBuffer), 0, 0);
  OsOutputDebugString("OsClipboard.cpp: %s failed: %s", title, msgBuffer);
  LocalFree(msgBuffer);
}

int OsClipboardGetString(char *buf, unsigned int bufSize) {
  HWND            hWnd = GetActiveWindow();
  HANDLE          globalObjectHandle;
  char           *clipboardString;
  int             wideChars;
  unsigned short *wideString;
  unsigned int    written;

  ASSERT(hWnd);

  if (!OpenClipboard(hWnd)) {
    FailureMessage("OpenClipboard");
    return 0;
  }

  globalObjectHandle = GetClipboardData(CF_TEXT);
  if (!globalObjectHandle) {
    FailureMessage("GetClipboardData");
    CloseClipboard();
    return 0;
  }

  clipboardString = static_cast<char *>(GlobalLock(globalObjectHandle));
  if (!clipboardString) {
    FailureMessage("GlobalLock");
    CloseClipboard();
    return 0;
  }

  wideChars = MultiByteToWideChar(OsInputGetCodePage(), MB_PRECOMPOSED, clipboardString, -1, 0, 0);
  wideString = static_cast<unsigned short *>(_alloca(wideChars * sizeof(unsigned short)));
  MultiByteToWideChar(OsInputGetCodePage(), MB_PRECOMPOSED, clipboardString, -1, reinterpret_cast<wchar_t *>(wideString), wideChars);

  ConvertUTF16toUTF8(buf, bufSize - 1, wideString, wideChars, &written, 0);
  buf[written] = 0;

  GlobalUnlock(globalObjectHandle);
  CloseClipboard();
  return 1;
}

char *OsClipboardGetString() {
  HWND            hWnd = GetActiveWindow();
  HANDLE          globalObjectHandle;
  char           *clipboardString;
  int             wideChars;
  unsigned short *wideString;
  char           *buffer;
  unsigned int    written;

  ASSERT(hWnd);

  if (!OpenClipboard(hWnd)) {
    FailureMessage("OpenClipboard");
    return 0;
  }

  globalObjectHandle = GetClipboardData(CF_TEXT);
  if (!globalObjectHandle) {
    FailureMessage("GetClipboardData");
    CloseClipboard();
    return 0;
  }

  clipboardString = static_cast<char *>(GlobalLock(globalObjectHandle));
  if (!clipboardString) {
    FailureMessage("GlobalLock");
    CloseClipboard();
    return 0;
  }

  wideChars = MultiByteToWideChar(OsInputGetCodePage(), MB_PRECOMPOSED, clipboardString, -1, 0, 0);
  wideString = static_cast<unsigned short *>(_alloca(wideChars * sizeof(unsigned short)));
  MultiByteToWideChar(OsInputGetCodePage(), MB_PRECOMPOSED, clipboardString, -1, reinterpret_cast<wchar_t *>(wideString), wideChars);

  buffer = static_cast<char *>(ALLOC(3 * wideChars));
  ConvertUTF16toUTF8(buffer, 3 * wideChars, wideString, wideChars, &written, 0);
  buffer[written] = 0;

  GlobalUnlock(globalObjectHandle);
  CloseClipboard();
  return buffer;
}

void OsClipboardFreeString(char *string) {
  FREEIFUSED(string);
}

int OsClipboardPutString(const char *string) {
  HWND            hWnd = GetActiveWindow();
  unsigned int    stringBytes;
  HGLOBAL         clipboardData;
  char           *globalString;
  unsigned short *wideString;

  ASSERT(hWnd);

  stringBytes = SStrLen(string) + 1;
  clipboardData = GlobalAlloc(0x2042, stringBytes);
  if (!clipboardData) {
    FailureMessage("GlobalAlloc");
    return 0;
  }

  globalString = static_cast<char *>(GlobalLock(clipboardData));
  if (!globalString) {
    FailureMessage("GlobalLock");
    CloseClipboard();
    return 0;
  }

  wideString = static_cast<unsigned short *>(_alloca(stringBytes * sizeof(unsigned short)));
  ConvertUTF8toUTF16(wideString, stringBytes, string, 0x7FFFFFFF, 0, 0);
  WideCharToMultiByte(OsInputGetCodePage(), 0, reinterpret_cast<const wchar_t *>(wideString), -1, globalString, stringBytes, 0, 0);
  GlobalUnlock(clipboardData);

  if (!OpenClipboard(hWnd)) {
    FailureMessage("OpenClipboard");
    GlobalFree(clipboardData);
    return 0;
  }

  if (!EmptyClipboard()) {
    FailureMessage("EmptyClipboard");
    CloseClipboard();
    GlobalFree(clipboardData);
    return 0;
  }

  SetClipboardData(CF_TEXT, clipboardData);
  CloseClipboard();
  return 1;
}
