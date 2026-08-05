#include <Base/Base.h>

#include "Os/W32/OSSystem.h"

#include <storm.h>

#include <string.h>

#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>

static int  s_sleepInBackground;
static DWORD s_backgroundSleepMs;
static void (*s_launchUrlCallback)();

void OsLaunchURLSetCallback(void (*callback)()) {
  s_launchUrlCallback = callback;
}

int OsLaunchURL(const char *url) {
  CFStringRef urlString;
  CFURLRef    urlRef;
  OSStatus    status;

  if (s_launchUrlCallback) {
    s_launchUrlCallback();
  }

  urlString = CFStringCreateWithCString(0, url, kCFStringEncodingUTF8);
  if (!urlString) {
    return 0;
  }

  urlRef = CFURLCreateWithString(0, urlString, 0);
  CFRelease(urlString);

  if (!urlRef) {
    return 0;
  }

  status = LSOpenCFURLRef(urlRef, 0);
  CFRelease(urlRef);

  return status == noErr;
}

int OsSleepInBackground() {
  return s_sleepInBackground;
}

DWORD OsGetBackgroundSleepMs() {
  return s_backgroundSleepMs;
}

unsigned int OsGetProcessorCount() {
  return MPProcessors();
}

DWORD OsGetProcessorFeatures() {
  SInt32 features;

  if (!Gestalt('ppcf', &features) && (features & 0x30) == 0x30) {
    return 32;
  }

  return 0;
}

DWORD OsGetProcessorFeaturesEx(int &vendorID) {
  SInt32 features;

  vendorID = 3;

  if (!Gestalt('ppcf', &features) && (features & 0x30) == 0x30) {
    return 32;
  }

  return 0;
}

void OsGetVersionString(char *string, int length) {
  SInt32      version;
  const char *prefix;
  int         major;
  int         minor;
  int         bugfix;
  int         chars;

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

int OsGetComputerName(char *computerName, DWORD *computerNameLen) {
  CFStringRef name = CSCopyMachineName();

  if (!name) {
    *computerName = 0;
    *computerNameLen = 0;
    return 0;
  }

  CFStringGetCString(name, computerName, *computerNameLen, kCFStringEncodingUTF8);
  *computerNameLen = strlen(computerName);
  CFRelease(name);

  return 1;
}

int OsGetUserName(char *userName, DWORD *userNameLen) {
  CFStringRef name = CSCopyUserName(0);

  if (!name) {
    *userName = 0;
    *userNameLen = 0;
    return 0;
  }

  CFStringGetCString(name, userName, *userNameLen, kCFStringEncodingUTF8);
  *userNameLen = strlen(userName);
  CFRelease(name);

  return 1;
}

DWORD OsGetPhysicalMemory() {
  SInt32 ram;

  Gestalt('ram ', &ram);
  return ram;
}
