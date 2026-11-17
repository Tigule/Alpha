#include <Base/Base.h>

#include "Os/W32/OSSystem.h"

#include <storm.h>

#include <string.h>

#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>

extern "C" void *objc_getClass(const char *name);
extern "C" void *sel_registerName(const char *str);
extern "C" void  objc_msgSend();

static int     s_sleepInBackground = 1;
static DWORD   s_backgroundSleepMs;
static char    s_mpChecked;
static Boolean s_mpLoaded;
static void (*s_launchUrlCallback)();

void OsLaunchURLSetCallback(void (*callback)()) {
  s_launchUrlCallback = callback;
}

BOOL OsLaunchURL(LPCSTR url) {
  void *string;
  void *nsurl;
  void *workspace;

  if (!url || !*url) {
    return FALSE;
  }

  if (s_launchUrlCallback) {
    s_launchUrlCallback();
  }

  string = ((void *(*)(void *, void *, LPCSTR))objc_msgSend)(objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"), url);
  nsurl = ((void *(*)(void *, void *, void *))objc_msgSend)(objc_getClass("NSURL"), sel_registerName("URLWithString:"), string);
  workspace = ((void *(*)(void *, void *))objc_msgSend)(objc_getClass("NSWorkspace"), sel_registerName("sharedWorkspace"));
  return ((bool (*)(void *, void *, void *))objc_msgSend)(workspace, sel_registerName("openURL:"), nsurl);
}

int OsSleepInBackground() {
  return s_sleepInBackground;
}

DWORD OsGetBackgroundSleepMs() {
  return s_backgroundSleepMs;
}

UINT OsGetProcessorCount() {
  if (!s_mpChecked) {
    s_mpLoaded = MPLibraryIsLoaded();
    s_mpChecked = 1;
  }

  if (!s_mpLoaded) {
    return 1;
  }

  return MPProcessors();
}

DWORD OsGetProcessorFeatures() {
  return 0;
}

DWORD OsGetProcessorFeaturesEx(int &vendorID) {
  vendorID = 0;
  return 0;
}

void OsGetVersionString(char *string, int length) {
  SInt32 version;
  LPCSTR prefix;
  int    major;
  int    minor;
  int    bugfix;
  int    chars;

  if (Gestalt('sysv', &version)) {
    version = 0;
  }

  major = ((version >> 8) & 0xF) + 10 * ((version & 0xFFFF) >> 12);
  minor = (version >> 4) & 0xF;
  bugfix = version & 0xF;

  prefix = major >= 10 ? "X " : "";

  chars = SStrPrintf(string, length, "Mac OS %s%d.%d", prefix, major, minor);

  if (bugfix) {
    SStrPrintf(string + chars, length - chars, ".%d", bugfix);
  }
}

BOOL OsGetComputerName(char *computerName, DWORD *computerNameLen) {
  CFStringRef name = CSCopyMachineName();

  if (!name) {
    *computerName = 0;
    *computerNameLen = 0;
    return FALSE;
  }

  CFStringGetCString(name, computerName, *computerNameLen, kCFStringEncodingUTF8);
  *computerNameLen = strlen(computerName);
  CFRelease(name);

  return TRUE;
}

BOOL OsGetUserName(char *userName, DWORD *userNameLen) {
  CFStringRef name = CSCopyUserName(0);

  if (!name) {
    *userName = 0;
    *userNameLen = 0;
    return FALSE;
  }

  CFStringGetCString(name, userName, *userNameLen, kCFStringEncodingUTF8);
  *userNameLen = strlen(userName);
  CFRelease(name);

  return TRUE;
}

DWORD OsGetPhysicalMemory() {
  SInt32 ram;

  Gestalt('ram ', &ram);
  return ram;
}
