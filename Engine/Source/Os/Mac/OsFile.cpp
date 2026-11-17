#include <Base/Base.h>

#include "Os/W32/OsFile.h"

#include <storm.h>

#include <glob.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include <CoreFoundation/CoreFoundation.h>

DWORD OsPathGetRootChars(LPCSTR path);
void  OsPathStripFilename(char *buffer);
void  OsPathStripLastDir(char *buffer);

extern "C" void APIENTRY SErrSetAppCommand(LPCSTR command);

enum {
  OS_CREATE_NEW = 1,
  OS_CREATE_ALWAYS = 2,
  OS_OPEN_EXISTING = 3,
  OS_OPEN_ALWAYS = 4,
  OS_TRUNCATE_EXISTING = 5
};

static int  s_pathsInitialized;
static char s_dataPath[0x400];
static char s_installPath[0x400];
char        g_osCommandLine[0x400];
char        g_osExeName[0x400];

void OsFileConvertSlashes(char *path) {
  char *curr;

  for (curr = path; *curr; ++curr) {
    if (*curr == '\\') {
      *curr = '/';
    }
  }

  while ((curr = SStrStr(path, "./")) != NULL) {
    while (*curr) {
      *curr = curr[2];
      ++curr;
    }
  }

  while ((curr = SStrStr(path, "//")) != NULL) {
    while (*curr) {
      *curr = curr[1];
      ++curr;
    }
  }
}

static char *build_native_path(LPCSTR path, LPCSTR mode, char *buffer, int bufferSize) {
  char converted[0x400];

  SStrCopy(converted, path, sizeof(converted));
  OsFileConvertSlashes(converted);

  if (converted[0] == '/') {
    SStrCopy(buffer, converted, bufferSize);
    return buffer;
  }

  if (mode[0] == 'r') {
    SStrPrintf(buffer, bufferSize, "%s/%s", s_dataPath, converted);
    if (!access(buffer, R_OK)) {
      return buffer;
    }
  } else if (!s_pathsInitialized) {
    SStrPrintf(buffer, bufferSize, "%s/%s", s_dataPath, converted);
    return buffer;
  }

  SStrPrintf(buffer, bufferSize, "%s/%s", s_installPath, converted);
  return buffer;
}

BOOL OsSetArgs(char *const *argv, bool resolveRealPath, bool writeDataPath) {
  LPCSTR      command = argv[0];
  CFBundleRef bundle;
  CFURLRef    bundleUrl;
  LPCSTR      home;
  char        exePath[0x400];
  UInt8       bundlePath[0x400];
  int         index;

  ASSERT(writeDataPath == true);

  bundle = CFBundleGetMainBundle();
  bundleUrl = CFBundleCopyBundleURL(bundle);
  if (bundleUrl) {
    if (CFURLGetFileSystemRepresentation(bundleUrl, 0, bundlePath, sizeof(bundlePath))) {
      DWORD chars = SStrLen((LPCSTR)bundlePath);
      if (chars > 4 && !SStrCmp((LPCSTR)bundlePath + chars - 4, ".app", 0x7FFFFFFF)) {
        command = (LPCSTR)bundlePath;
      }
    }
  }

  SErrSetAppCommand(command);
  SStrCopy(g_osExeName, command, sizeof(g_osExeName));

  home = getenv("HOME");
  if (!home) {
    home = ".";
  }

  strcpy(exePath, command);
  if (!SStrChrR(exePath, '/')) {
    LPCSTR searchPath = getenv("PATH");
    LPCSTR separator;
    int    found = 0;

    do {
      exePath[0] = 0;
      separator = SStrChr(searchPath, ':');
      if (!separator) {
        separator = searchPath + strlen(searchPath);
      }

      if (*searchPath == '~') {
        SStrCopy(exePath, home, sizeof(exePath));
        ++searchPath;
      }

      if (separator > searchPath + 1) {
        SStrPack(exePath, searchPath, sizeof(exePath));
        SStrPack(exePath, "/", sizeof(exePath));
      }

      SStrPack(exePath, "./", sizeof(exePath));
      SStrPack(exePath, command, sizeof(exePath));

      if (!access(exePath, X_OK)) {
        ++found;
      }

      searchPath = separator + 1;
    } while (*separator && !found);
  }

  s_installPath[0] = 0;
  if (resolveRealPath) {
    if (realpath(exePath, s_installPath)) {
      *SStrChrR(s_installPath, '/') = 0;
    }
  } else {
    SStrCopy(s_installPath, ".", sizeof(s_installPath));
  }

  if (s_installPath[0]) {
    if (chdir(s_installPath) < 0) {
      ASSERT(!"Couldn't change to install directory");
    }
  } else {
    chdir("/");
  }

  s_pathsInitialized = 1;

  for (index = 0; argv[index]; ++index) {
    if (index > 0) {
      SStrPack(g_osCommandLine, " ", sizeof(g_osCommandLine));
    }

    if (SStrChr(argv[index], ' ')) {
      SStrPack(g_osCommandLine, "\"", sizeof(g_osCommandLine));
      SStrPack(g_osCommandLine, argv[index], sizeof(g_osCommandLine));
      SStrPack(g_osCommandLine, "\"", sizeof(g_osCommandLine));
    } else {
      SStrPack(g_osCommandLine, argv[index], sizeof(g_osCommandLine));
    }
  }

  return TRUE;
}

LPCSTR OsGetCommandLine() {
  return g_osCommandLine;
}

void OsGetExePath(char *buffer, DWORD chars) {
  SStrCopy(buffer, g_osExeName, chars);
  OsPathStripLastDir(buffer);
}

HOSFILE
OsCreateFile(LPCSTR fileName, DWORD desiredAccess, DWORD shareMode, DWORD createDisposition, DWORD flagsAndAttributes, DWORD extendedFileType) {
  LPCSTR      mode = NULL;
  struct stat stats;
  char        nativePath[0x400];
  FILE       *file;

  ASSERT(fileName);
  if (!fileName) {
    return NULL;
  }

  ASSERT(desiredAccess);
  if (!desiredAccess) {
    return NULL;
  }

  (void)shareMode;
  (void)flagsAndAttributes;
  (void)extendedFileType;

  if (createDisposition == OS_CREATE_NEW) {
    build_native_path(fileName, "r", nativePath, sizeof(nativePath));
    if (!stat(nativePath, &stats)) {
      return NULL;
    }
    createDisposition = OS_CREATE_ALWAYS;
  }

  if (createDisposition == OS_OPEN_ALWAYS) {
    build_native_path(fileName, "r", nativePath, sizeof(nativePath));
    createDisposition = stat(nativePath, &stats) ? OS_CREATE_ALWAYS : OS_OPEN_EXISTING;
  }

  if (createDisposition == OS_TRUNCATE_EXISTING) {
    build_native_path(fileName, "r", nativePath, sizeof(nativePath));
    if (stat(nativePath, &stats)) {
      return NULL;
    }
    createDisposition = OS_CREATE_ALWAYS;
  }

  ASSERT((createDisposition == OS_CREATE_ALWAYS) || (createDisposition == OS_OPEN_EXISTING));

  if (createDisposition == OS_OPEN_EXISTING) {
    if (desiredAccess & GENERIC_WRITE) {
      build_native_path(fileName, "w", nativePath, sizeof(nativePath));
      if (stat(nativePath, &stats)) {
        return NULL;
      }
      mode = "rb+";
    } else {
      mode = "rb";
    }
  } else if (createDisposition == OS_CREATE_ALWAYS) {
    mode = (desiredAccess & GENERIC_READ) ? "wb+" : "wb";
  }

  if (!mode) {
    ASSERT(mode);
    return NULL;
  }

  build_native_path(fileName, mode, nativePath, sizeof(nativePath));
  file = fopen(nativePath, mode);
  if (!file) {
    return NULL;
  }

  return (HOSFILE)file;
}

void OsCloseFile(HOSFILE fileHandle) {
  if (fileHandle) {
    fclose((FILE *)fileHandle);
  }
}

BOOL OsFileExists(LPCSTR path) {
  DWORD attributes;

  if (!path || !path[0]) {
    return FALSE;
  }

  attributes = OsGetFileAttributes(path);
  return attributes != 0xFFFFFFFF && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

BOOL OsDirectoryExists(LPCSTR dirName) {
  DWORD attributes;

  if (!dirName || !dirName[0]) {
    return FALSE;
  }

  attributes = OsGetFileAttributes(dirName);
  return attributes != 0xFFFFFFFF && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

BOOL OsReadFile(HOSFILE fileHandle, LPVOID buffer, DWORD bytesToRead, DWORD *bytesRead) {
  size_t read;

  VALIDATE2(buffer);
  VALIDATE2(bytesRead);

  if (!fileHandle) {
    return FALSE;
  }

  read = fread(buffer, 1, bytesToRead, (FILE *)fileHandle);
  *bytesRead = (DWORD)read;
  return read != 0;
}

BOOL OsWriteFile(HOSFILE fileHandle, LPCVOID buffer, DWORD bytesToWrite, DWORD *bytesWritten) {
  size_t written;

  VALIDATE2(buffer);
  VALIDATE2(bytesWritten);

  if (!fileHandle) {
    return FALSE;
  }

  written = fwrite(buffer, 1, bytesToWrite, (FILE *)fileHandle);
  *bytesWritten = (DWORD)written;
  return written != 0;
}

DWORDLONG OsSetFilePointer(HOSFILE fileHandle, LONGLONG distanceToMove, DWORD moveMethod) {
  if (!fileHandle) {
    return (DWORDLONG)-1;
  }

  fseek((FILE *)fileHandle, (long)distanceToMove, moveMethod);
  return ftell((FILE *)fileHandle);
}

DWORDLONG OsGetFileSize(HOSFILE fileHandle) {
  struct stat stats;

  if (!fileHandle) {
    return 0;
  }

  stats.st_size = 0;
  fstat(fileno((FILE *)fileHandle), &stats);
  return stats.st_size;
}

DWORD OsGetFileAttributes(LPCSTR fileName) {
  DWORD       attributes;
  struct stat stats;
  char        nativePath[0x400];

  VALIDATE2(fileName);

  attributes = 0xFFFFFFFF;
  build_native_path(fileName, "r", nativePath, sizeof(nativePath));

  if (!stat(nativePath, &stats)) {
    attributes = 0;
    if ((stats.st_mode & S_IFMT) == S_IFDIR) {
      attributes = FILE_ATTRIBUTE_DIRECTORY;
    }
    if ((stats.st_mode & S_IFMT) == S_IFREG) {
      attributes |= FILE_ATTRIBUTE_NORMAL;
    }
  }

  return attributes;
}

BOOL OsCreateDirectory(LPCSTR pathName, int recursive) {
  struct stat stats;
  char        nativePath[0x400];

  VALIDATE2(pathName);

  build_native_path(pathName, "w", nativePath, sizeof(nativePath));

  if (!stat(nativePath, &stats) && (stats.st_mode & S_IFMT) == S_IFDIR) {
    return TRUE;
  }

  if (recursive) {
    char *cursor = nativePath;

    if (!SStrCmp(nativePath, s_dataPath, SStrLen(s_dataPath))) {
      cursor = nativePath + SStrLen(s_dataPath);
    }

    while ((cursor = SStrChr(cursor + 1, '/')) != NULL) {
      *cursor = 0;
      if ((stat(nativePath, &stats) || (stats.st_mode & S_IFMT) != S_IFDIR) && mkdir(nativePath, 0755) < 0) {
        return FALSE;
      }
      *cursor = '/';
    }
  }

  return mkdir(nativePath, 0755) == 0;
}

BOOL OsSetCurrentDirectory(LPCSTR pathName) {
  char nativePath[0x400];

  VALIDATE2(pathName);

  SStrCopy(nativePath, pathName, sizeof(nativePath));
  OsFileConvertSlashes(nativePath);

  return !nativePath[0] || chdir(nativePath) == 0;
}

BOOL OsGetCurrentDirectory(DWORD pathLen, char *pathName) {
  char *curr;

  VALIDATE2(pathName);

  if (!getcwd(pathName, pathLen)) {
    return FALSE;
  }

  for (curr = pathName; *curr && (DWORD)(curr - pathName) < pathLen - 1; ++curr) {
    if (*curr == '/') {
      *curr = curr[1] ? '\\' : 0;
    }
  }

  return TRUE;
}

static BOOL OsFileListRaw(LPCSTR pattern, int (*inCallback)(OS_FILE_DATA &, LPVOID), LPVOID inCBParam, int returnHidden) {
  glob_t       results;
  struct stat  stats;
  OS_FILE_DATA osfData;
  char         nativePath[0x400];
  int          stop = 0;
  size_t       index;

  build_native_path(pattern, "w", nativePath, sizeof(nativePath));

  if (glob(nativePath, 0, 0, &results)) {
    return FALSE;
  }

  for (index = 0; index < results.gl_pathc; ++index) {
    LPCSTR path = results.gl_pathv[index];
    LPCSTR base;

    if (stat(path, &stats)) {
      continue;
    }

    base = SStrChrR(path, '/');
    ASSERT(base != 0);

    SStrCopy(osfData.fileName, base + 1, sizeof(osfData.fileName));
    osfData.flags = 0;
    osfData.size = stats.st_size;

    if ((stats.st_mode & S_IFMT) == S_IFDIR) {
      osfData.flags = FILE_ATTRIBUTE_DIRECTORY;
    } else if (osfData.fileName[0] == '.') {
      osfData.flags = FILE_ATTRIBUTE_HIDDEN;
    }

    if ((returnHidden || (osfData.flags & FILE_ATTRIBUTE_HIDDEN) == 0) && inCallback(osfData, inCBParam)) {
      stop = 1;
      break;
    }
  }

  globfree(&results);
  return stop;
}

int OsFileList(LPCSTR inDir, LPCSTR inPattern, int (*inCallback)(OS_FILE_DATA &, LPVOID), LPVOID inCBParam, int returnHidden) {
  char pattern[0x400];

  SStrCopy(pattern, inDir, 0x7FFFFFFF);
  OsPathStripFilename(pattern);
  SStrPack(pattern, inPattern, 0x7FFFFFFF);

  return OsFileListRaw(pattern, inCallback, inCBParam, returnHidden);
}

BOOL OsDeleteFile(LPCSTR fileName) {
  char nativePath[0x400];

  VALIDATE2(fileName);

  build_native_path(fileName, "w", nativePath, sizeof(nativePath));
  return unlink(nativePath) == 0;
}
