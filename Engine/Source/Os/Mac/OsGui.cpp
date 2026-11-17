#include <Base/Base.h>

#include "Os/W32/OsGui.h"

#include <storm.h>

#include <CoreFoundation/CoreFoundation.h>

static LPVOID s_gxWindow;

int OsGuiMessageBox(LPVOID inParentWindow, int inStyle, LPCSTR inMessage, LPCSTR inTitle) {
  CFStringRef  message = 0;
  CFStringRef  title = 0;
  CFStringRef  defaultText = 0;
  CFStringRef  cancelText = 0;
  CFStringRef  otherText = 0;
  CFOptionFlags response = 0;
  int          itemHit;

  if (!inTitle || !inTitle[0]) {
    inTitle = inMessage;
    inMessage = 0;
  }

  if (inMessage) {
    message = CFStringCreateWithCString(kCFAllocatorDefault, inMessage, kCFStringEncodingUTF8);
  }
  if (!message) {
    message = CFSTR("");
    CFRetain(message);
  }

  if (inTitle) {
    title = CFStringCreateWithCString(kCFAllocatorDefault, inTitle, kCFStringEncodingUTF8);
  }
  if (!title) {
    title = CFSTR("");
    CFRetain(title);
  }

  switch (inStyle) {
  case 1:
    cancelText = CFSTR("Cancel");
    break;
  case 2:
    defaultText = CFSTR("Yes");
    cancelText = CFSTR("No");
    break;
  case 3:
    defaultText = CFSTR("Yes");
    cancelText = CFSTR("No");
    otherText = CFSTR("Cancel");
    break;
  }

  CFUserNotificationDisplayAlert(0, kCFUserNotificationStopAlertLevel, 0, 0, 0, title, message, defaultText, cancelText, otherText, &response);

  CFRelease(message);
  CFRelease(title);

  itemHit = (int)(response & 3) + 1;
  if (inStyle == 1 && itemHit == 2) {
    return 2;
  }
  return itemHit - 1;
}

LPVOID OsGuiGetWindow(int inWindowType) {
  return s_gxWindow;
}

void OsGuiSetGxWindow(LPVOID window) {
  s_gxWindow = window;
}

void OsGuiSetWindowTitle(LPVOID inWindow, LPCSTR inText) {
}
