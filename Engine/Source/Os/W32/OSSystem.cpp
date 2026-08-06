#include <Base/Base.h>

#include <windows.h>
#include <shellapi.h>
#include <float.h>
#include <stdio.h>
#include <string.h>

#include <storm.h>

#include "OSSystem.h"

enum OsType {
  OsType_Unknown = 0,
  OsType_Win95 = 1,
  OsType_Win95OSR2 = 2,
  OsType_Win98 = 3,
  OsType_Win98SE = 4,
  OsType_WinME = 5,
  OsType_WinNT4 = 6,
  OsType_Win2000 = 7,
  OsType_WinXP = 8,
  OsType_MacOS9 = 9,
  OsType_MacOSX = 10,
  OsType_Linux = 11,
  OsType_Last = 12
};

enum {
  OS_PROCESSOR_VENDOR_UNKNOWN = 0,
  OS_PROCESSOR_VENDOR_INTEL = 1,
  OS_PROCESSOR_VENDOR_AMD = 2,
  OS_PROCESSOR_VENDOR_PPC = 3
};

static const BYTE s_vendorGenuineIntel[12] = {'G', 'e', 'n', 'u', 'i', 'n', 'e', 'I', 'n', 't', 'e', 'l'};
static const BYTE s_vendorAuthenticAMD[12] = {'A', 'u', 't', 'h', 'e', 'n', 't', 'i', 'c', 'A', 'M', 'D'};
static const BYTE s_vendorCentaurHalls[12] = {'C', 'e', 'n', 't', 'a', 'u', 'r', 'H', 'a', 'u', 'l', 's'};
static const BYTE s_vendorCyrixInstead[12] = {'C', 'y', 'r', 'i', 'x', 'I', 'n', 's', 't', 'e', 'a', 'd'};

static const char s_xtoi[256] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static LPCSTR s_osNames[OsType_Last] = {"Unknown", "Win95",   "Win95OSR2", "Win98",  "Win98SE", "WinME",
                                        "WinNT4",  "Win2000", "WinXP",     "MacOS9", "MacOSX",  "Linux"};

static DWORD s_processorFeatures;
static int   s_processorVendor;

int   s_sleepInBackground = 1;
DWORD s_backgroundSleepMs;

static int __cdecl IOsGetProcessorFeatures(BYTE *const vendor, DWORD *featuresStd, DWORD *featuresExt) {
  int result = 1;

  memset(vendor, 0, 12);
  *featuresStd = 0;
  *featuresExt = 0;

  if (IsProcessorFeaturePresent(8)) {
    *featuresStd |= 0x00000010;
  }
  if (IsProcessorFeaturePresent(3)) {
    *featuresStd |= 0x00800000;
  }
  if (IsProcessorFeaturePresent(6)) {
    *featuresStd |= 0x02000000;
  }
  if (IsProcessorFeaturePresent(7)) {
    *featuresExt |= 0x80000000;
  }

  return result;
}

DWORD OsGetProcessorFeaturesEx(int &vendorID) {
  BYTE  vendor[12];
  DWORD featuresStd;
  DWORD featuresExt;
  DWORD features = 0;

  vendorID = s_processorVendor;
  if (!s_processorFeatures) {
    if (IOsGetProcessorFeatures(vendor, &featuresStd, &featuresExt)) {
      if (featuresStd & 0x00000010) {
        features |= 0x01;
      }
      if (featuresStd & 0x00800000) {
        features |= 0x02;
      }
      if (featuresStd & 0x02000000) {
        features |= 0x04;
      }
      if (featuresExt & 0x80000000) {
        features |= 0x08;
      }

      if (!memcmp(vendor, s_vendorGenuineIntel, sizeof(vendor))) {
        if (featuresStd & 0x04000000) {
          features |= 0x10;
        }
        s_processorVendor = OS_PROCESSOR_VENDOR_INTEL;
      } else if (!memcmp(vendor, s_vendorAuthenticAMD, sizeof(vendor))) {
        s_processorVendor = OS_PROCESSOR_VENDOR_AMD;
      }
    }

    s_processorFeatures = features | 0x80000000;
    vendorID = s_processorVendor;
  }

  return s_processorFeatures;
}

DWORD OsGetProcessorFeatures() {
  int manufacturer;
  return OsGetProcessorFeaturesEx(manufacturer);
}

UINT OsGetProcessorCount() {
  SYSTEM_INFO si;
  memset(&si, 0, sizeof(si));
  GetSystemInfo(&si);

  if (!si.dwNumberOfProcessors) {
    return 1;
  }

  return si.dwNumberOfProcessors;
}

void OsSleep(DWORD ms) {
  Sleep(ms);
}

int OsSleepInBackground() {
  return s_sleepInBackground;
}

void OsSetSleepInBackground(int sleepInBackground) {
  s_sleepInBackground = sleepInBackground;
}

DWORD OsGetBackgroundSleepMs() {
  return s_backgroundSleepMs;
}

void OsSetBackgroundSleepMs(DWORD sleepMs) {
  s_backgroundSleepMs = sleepMs;
}

void OsPause() {
}

OsType OsGetVersion() {
  OSVERSIONINFOEX osvi;
  OsType          retVal = OsType_Unknown;

  memset(&osvi, 0, sizeof(osvi));
  osvi.dwOSVersionInfoSize = sizeof(osvi);
  if (!GetVersionEx(reinterpret_cast<OSVERSIONINFO *>(&osvi))) {
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!GetVersionEx(reinterpret_cast<OSVERSIONINFO *>(&osvi))) {
      ASSERT(retVal != OsType_Unknown);
      return retVal;
    }
  }

  switch (osvi.dwPlatformId) {
    case VER_PLATFORM_WIN32_WINDOWS:
      if (osvi.dwMajorVersion == 4) {
        switch (osvi.dwMinorVersion) {
          case 0:
            retVal = OsType_Win95;
            if (osvi.szCSDVersion[1] == 'C' || osvi.szCSDVersion[1] == 'B') {
              retVal = OsType_Win95OSR2;
            }
            break;

          case 10:
            retVal = OsType_Win98;
            if (osvi.szCSDVersion[1] == 'A') {
              retVal = OsType_Win98SE;
            }
            break;

          case 90:
            retVal = OsType_WinME;
            break;
        }
      }
      break;

    case VER_PLATFORM_WIN32_NT:
      if (osvi.dwMajorVersion == 4) {
        retVal = OsType_WinNT4;
      } else if (osvi.dwMajorVersion == 5) {
        if (osvi.dwMinorVersion == 0) {
          retVal = OsType_Win2000;
        } else if (osvi.dwMinorVersion == 1) {
          retVal = OsType_WinXP;
        }
      }
      break;
  }

  ASSERT(retVal != OsType_Unknown);
  return retVal;
}

void OsGetVersionString(char *string, int length) {
  LPCSTR osName = s_osNames[OsGetVersion()];
  if (!osName) {
    osName = s_osNames[OsType_Unknown];
  }

  SStrCopy(string, osName, length);
}

int OsGetComputerName(char *computerName, DWORD *computerNameLen) {
  return GetComputerName(computerName, computerNameLen);
}

int OsGetUserName(char *userName, DWORD *userNameLen) {
  return GetUserName(userName, userNameLen);
}

DWORD OsGetPhysicalMemory() {
  MEMORYSTATUS mem;
  GlobalMemoryStatus(&mem);
  return mem.dwTotalPhys;
}

void OsSystemObjectCreate(LPCSTR inName) {
  CreateEventA(NULL, TRUE, FALSE, inName);
}

int OsSystemObjectExists(LPCSTR inName) {
  HANDLE handle;
  DWORD  error;

  handle = CreateEventA(NULL, TRUE, FALSE, inName);
  if (!handle) {
    return 0;
  }

  error = GetLastError();
  CloseHandle(handle);
  return error == ERROR_ALREADY_EXISTS ? 1 : 0;
}

int OsLaunchURL(LPCSTR url) {
  HWND      activeWindow;
  char      fixedURL[1024];
  char      browserFilename[256];
  char     *fixedURLPos;
  LPCSTR    urlPos;
  char      urlChar;
  HINSTANCE launchResult;

  if (!url || !*url) {
    return 0;
  }

  activeWindow = GetActiveWindow();
  if (!activeWindow) {
    return 0;
  }

  FATALASSERT(SStrLen(url) < (sizeof(fixedURL) / sizeof(fixedURL[0])));

  urlChar = *url;
  fixedURLPos = fixedURL;
  urlPos = url + 1;
  while (urlChar) {
    if (urlChar == '%') {
      char high = *urlPos++;
      if (!high) {
        break;
      }

      char low = *urlPos++;
      if (!low) {
        break;
      }

      *fixedURLPos = (s_xtoi[high] << 4) | s_xtoi[low];
    } else {
      *fixedURLPos = urlChar;
    }

    urlChar = *urlPos++;
    ++fixedURLPos;
  }

  *fixedURLPos = 0;
  launchResult = ShellExecuteA(activeWindow, "open", fixedURL, 0, 0, SW_SHOWNORMAL);
  if (reinterpret_cast<DWORD>(launchResult) > 32) {
    return 1;
  }

  FILE *file = fopen("8BLZ2112.HTM", "wb");
  fclose(file);

  launchResult = FindExecutableA("8BLZ2112.HTM", 0, browserFilename);
  if (reinterpret_cast<DWORD>(launchResult) > 32) {
    launchResult = ShellExecuteA(activeWindow, "open", browserFilename, fixedURL, 0, SW_SHOWNORMAL);
  }

  DeleteFileA("8BLZ2112.HTM");
  return reinterpret_cast<DWORD>(launchResult) > 32;
}

void OsClearFP(int errCheck) {
  _clearfp();
  _control87(0x9001F, 0xFFFFF);
}

int OsGetCurrentThreadPriority() {
  return GetThreadPriority(GetCurrentThread());
}

void OsSetCurrentThreadPriority(int priority) {
  SetThreadPriority(GetCurrentThread(), priority);
}
