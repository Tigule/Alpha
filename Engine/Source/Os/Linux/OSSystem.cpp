#include <Base/Base.h>

#include "Os/W32/OSSystem.h"

#include <storm.h>

#include <cpuid.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static DWORD s_processorFeatures;
static int   s_processorVendor;

static int IOsGetProcessorFeatures(DWORD *vendor, DWORD *standard, DWORD *extended) {
  unsigned int eax;
  unsigned int ebx;
  unsigned int ecx;
  unsigned int edx;

  __cpuid(0, eax, ebx, ecx, edx);
  vendor[0] = ebx;
  vendor[1] = edx;
  vendor[2] = ecx;

  __cpuid(1, eax, ebx, ecx, edx);
  *standard = edx;

  __cpuid(0x80000000, eax, ebx, ecx, edx);
  if (eax > 0x80000000) {
    __cpuid(0x80000001, eax, ebx, ecx, edx);
    *extended = edx;
  } else {
    *extended = 0;
  }

  return 1;
}

DWORD OsGetProcessorFeaturesEx(int &vendorID) {
  DWORD vendor[3];
  DWORD standard;
  DWORD extended;

  vendorID = s_processorVendor;
  if (s_processorFeatures) {
    return s_processorFeatures;
  }

  if (IOsGetProcessorFeatures(vendor, &standard, &extended)) {
    if (standard & 0x10) {
      s_processorFeatures |= 1;
    }
    if (standard & 0x800000) {
      s_processorFeatures |= 2;
    }
    if (standard & 0x2000000) {
      s_processorFeatures |= 4;
    }
    if (extended & 0x80000000) {
      s_processorFeatures |= 8;
    }

    if (!memcmp(vendor, "GenuineIntel", 12)) {
      if (standard & 0x4000000) {
        s_processorFeatures |= 0x10;
      }
      s_processorVendor = 1;
    } else if (!memcmp(vendor, "AuthenticAMD", 12)) {
      s_processorVendor = 2;
    }
  }

  s_processorFeatures |= 0x80000000;
  vendorID = s_processorVendor;
  return s_processorFeatures;
}

DWORD OsGetProcessorFeatures() {
  int vendorID;

  return OsGetProcessorFeaturesEx(vendorID);
}

UINT OsGetProcessorCount() {
  FILE *file;
  char  line[0x2000];
  UINT  count = 0;

  file = fopen("/proc/stat", "r");
  if (file) {
    while (fgets(line, sizeof(line), file)) {
      if (!strncmp(line, "cpu", 3) && line[3] != ' ') {
        ++count;
      }
    }
    fclose(file);
  }

  return count ? count : 1;
}

void OsGetVersionString(char *string, int length) {
  SStrCopy(string, "Linux", length);
}

BOOL OsGetComputerName(char *computerName, DWORD *computerNameLen) {
  if (gethostname(computerName, *computerNameLen) < 0) {
    SStrCopy(computerName, "LinuxBox", *computerNameLen);
  }

  return 1;
}

BOOL OsGetUserName(char *userName, DWORD *userNameLen) {
  SStrCopy(userName, "UnknownUser", *userNameLen);
  *userNameLen = SStrLen(userName);
  return 1;
}

DWORD OsGetPhysicalMemory() {
  FILE *file;
  char  line[0x2000];
  DWORD memory = 0x8000000;

  file = fopen("/proc/meminfo", "r");
  if (file) {
    while (fgets(line, sizeof(line), file)) {
      if (!strncmp(line, "Mem:", 4)) {
        sscanf(line, "Mem: %u", &memory);
        break;
      }
    }
    fclose(file);
  }

  return memory;
}

static int FindInPath(LPCSTR program) {
  LPCSTR      searchPath = getenv("PATH");
  LPCSTR      separator;
  LPCSTR      home;
  char        path[0x1000];
  struct stat stats;
  int         found = 0;

  do {
    path[0] = 0;
    separator = SStrChr(searchPath, ':');
    if (!separator) {
      separator = searchPath + SStrLen(searchPath);
    }

    if (*searchPath == '~') {
      home = getenv("HOME");
      SStrCopy(path, home ? home : ".", sizeof(path));
      ++searchPath;
    }

    if (separator > searchPath + 1) {
      SStrPack(path, searchPath, separator - searchPath + 1);
      SStrPack(path, "/", sizeof(path));
    }

    SStrPack(path, "./", sizeof(path));
    SStrPack(path, program, sizeof(path));

    if (!access(path, X_OK) && !stat(path, &stats) && S_ISREG(stats.st_mode)) {
      ++found;
    }

    if (!*separator) {
      break;
    }

    searchPath = separator + 1;
  } while (!found);

  return found;
}

BOOL OsLaunchURL(LPCSTR url) {
  static const struct {
    LPCSTR program;
    LPCSTR command;
  } browsers[] = {
    { "gnome-moz-remote", "gnome-moz-remote --newwin %s &" },
    { "netscape", "netscape -remote 'openURL(%s,new-window)' || netscape %s &" },
    { "galeon", "galeon %s &" },
    { "konqueror", "konqueror %s &" },
    { "mozilla", "mozilla -chrome %s || mozilla %s &" },
    { "opera", "opera -newwindow %s &" },
  };

  LPCSTR format;
  char   command[0x4000];
  UINT   index;

  if (!url || !*url) {
    return 0;
  }

  format = getenv("BROWSER");
  for (index = 0; !format && index < sizeof(browsers) / sizeof(browsers[0]); ++index) {
    if (FindInPath(browsers[index].program)) {
      format = browsers[index].command;
    }
  }

  if (!format) {
    return 0;
  }

  SStrPrintf(command, sizeof(command), format, url, url);
  return system(command) == 0;
}
