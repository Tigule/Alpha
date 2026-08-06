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
void  OsFileConvertSlashes(char *path);

extern "C" void APIENTRY SErrSetAppCommand(LPCSTR command);

static int  s_pathsInitialized;
static char s_dataPath[0x400];
static char s_installPath[0x400];
char        g_osCommandLine[0x400];
char        g_osExeName[0x400];

static char *BuildNativePath(LPCSTR path, LPCSTR mode, char *buffer, DWORD bufferSize) {
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

void OsFileConvertSlashes(char *path) {
  char *curr;

  for (curr = path; *curr; ++curr) {
    if (*curr == '\\') {
      *curr = '/';
    }
  }

  while ((curr = SStrStr(path, "./")) != 0) {
    while (*curr) {
      *curr = curr[2];
      ++curr;
    }
  }

  while ((curr = SStrStr(path, "//")) != 0) {
    while (*curr) {
      *curr = curr[1];
      ++curr;
    }
  }
}

int OsSetArgs(LPCSTR *argv, bool resolveRealPath, bool writeDataPath) {
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
      DWORD chars = SStrLen(reinterpret_cast<LPCSTR>(bundlePath));
      if (chars > 4 && !SStrCmp(reinterpret_cast<LPCSTR>(bundlePath) + chars - 4, ".app", 0x7FFFFFFF)) {
        command = reinterpret_cast<LPCSTR>(bundlePath);
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
      char *lastSlash;

      SStrCopy(g_osExeName, s_installPath, sizeof(g_osExeName));

      lastSlash = SStrChrR(s_installPath, '/');
      if (lastSlash) {
        *lastSlash = 0;
      }
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

  return 1;
}

LPCSTR OsGetCommandLine() {
  return g_osCommandLine;
}

void OsGetExeName(char *buffer, DWORD chars) {
  SStrCopy(buffer, g_osExeName, chars);
}

void OsGetExePath(char *buffer, DWORD chars) {
  SStrCopy(buffer, g_osExeName, chars);
  OsPathStripLastDir(buffer);
}

HOSFILE
OsCreateFile(LPCSTR fileName, DWORD desiredAccess, DWORD shareMode, DWORD createDisposition, DWORD flagsAndAttributes, DWORD extendedFileType) {
  LPCSTR      mode = 0;
  struct stat stats;
  char        nativePath[0x400];
  FILE       *file;

  ASSERT(fileName);
  if (!fileName) {
    return 0;
  }

  ASSERT(desiredAccess);
  if (!desiredAccess) {
    return 0;
  }

  (void)shareMode;
  (void)flagsAndAttributes;
  (void)extendedFileType;

  if (createDisposition == CREATE_NEW) {
    BuildNativePath(fileName, "r", nativePath, sizeof(nativePath));
    if (!stat(nativePath, &stats)) {
      return 0;
    }
    createDisposition = CREATE_ALWAYS;
  }

  if (createDisposition == OPEN_ALWAYS) {
    BuildNativePath(fileName, "r", nativePath, sizeof(nativePath));
    createDisposition = stat(nativePath, &stats) ? CREATE_ALWAYS : OPEN_EXISTING;
  }

  if (createDisposition == TRUNCATE_EXISTING) {
    BuildNativePath(fileName, "r", nativePath, sizeof(nativePath));
    if (stat(nativePath, &stats)) {
      return 0;
    }
    createDisposition = CREATE_ALWAYS;
  }

  ASSERT((createDisposition == CREATE_ALWAYS) || (createDisposition == OPEN_EXISTING));

  if (createDisposition == OPEN_EXISTING) {
    if (desiredAccess & GENERIC_WRITE) {
      BuildNativePath(fileName, "w", nativePath, sizeof(nativePath));
      if (stat(nativePath, &stats)) {
        return 0;
      }
      mode = "rb+";
    } else {
      mode = "rb";
    }
  } else if (createDisposition == CREATE_ALWAYS) {
    mode = (desiredAccess & GENERIC_READ) ? "wb+" : "wb";
  }

  if (!mode) {
    ASSERT(mode);
    return 0;
  }

  BuildNativePath(fileName, mode, nativePath, sizeof(nativePath));
  file = fopen(nativePath, mode);
  if (!file) {
    return 0;
  }

  return reinterpret_cast<HOSFILE>(file);
}

void OsCloseFile(HOSFILE fileHandle) {
  if (fileHandle) {
    fclose(reinterpret_cast<FILE *>(fileHandle));
  }
}

int OsReadFile(HOSFILE fileHandle, LPVOID buffer, DWORD bytesToRead, DWORD *bytesRead) {
  size_t read;

  FATALASSERT(buffer);
  FATALASSERT(bytesRead);

  if (!fileHandle) {
    return 0;
  }

  read = fread(buffer, 1, bytesToRead, reinterpret_cast<FILE *>(fileHandle));
  *bytesRead = static_cast<DWORD>(read);
  return read != 0;
}

int OsWriteFile(HOSFILE fileHandle, LPCVOID buffer, DWORD bytesToWrite, DWORD *bytesWritten) {
  size_t written;

  FATALASSERT(buffer);
  FATALASSERT(bytesWritten);

  if (!fileHandle) {
    return 0;
  }

  written = fwrite(buffer, 1, bytesToWrite, reinterpret_cast<FILE *>(fileHandle));
  *bytesWritten = static_cast<DWORD>(written);
  return written != 0;
}

DWORDLONG OsGetFileSize(HOSFILE fileHandle) {
  struct stat stats;

  if (!fileHandle) {
    return 0;
  }

  stats.st_size = 0;
  fstat(fileno(reinterpret_cast<FILE *>(fileHandle)), &stats);
  return stats.st_size;
}

DWORDLONG OsSetFilePointer(HOSFILE fileHandle, LONGLONG distanceToMove, DWORD moveMethod) {
  if (!fileHandle) {
    return static_cast<DWORDLONG>(-1);
  }

  fseek(reinterpret_cast<FILE *>(fileHandle), static_cast<long>(distanceToMove), moveMethod);
  return ftell(reinterpret_cast<FILE *>(fileHandle));
}

DWORD OsGetFileAttributes(LPCSTR fileName) {
  DWORD       attributes;
  struct stat stats;
  char        nativePath[0x400];

  FATALASSERT(fileName);

  attributes = 0xFFFFFFFF;
  BuildNativePath(fileName, "r", nativePath, sizeof(nativePath));

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

int OsFileExists(LPCSTR path) {
  DWORD attributes;

  if (!path || !path[0]) {
    return 0;
  }

  attributes = OsGetFileAttributes(path);
  return attributes != 0xFFFFFFFF && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

int OsDirectoryExists(LPCSTR dirName) {
  DWORD attributes;

  if (!dirName || !dirName[0]) {
    return 0;
  }

  attributes = OsGetFileAttributes(dirName);
  return attributes != 0xFFFFFFFF && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

int OsCreateDirectory(LPCSTR pathName, int recursive) {
  struct stat stats;
  char        nativePath[0x400];

  FATALASSERT(pathName);

  BuildNativePath(pathName, "w", nativePath, sizeof(nativePath));

  if (!stat(nativePath, &stats) && (stats.st_mode & S_IFMT) == S_IFDIR) {
    return 1;
  }

  if (recursive) {
    char *cursor = nativePath;

    if (!SStrCmp(nativePath, s_dataPath, SStrLen(s_dataPath))) {
      cursor = nativePath + SStrLen(s_dataPath);
    }

    while ((cursor = SStrChr(cursor + 1, '/')) != 0) {
      *cursor = 0;
      if ((stat(nativePath, &stats) || (stats.st_mode & S_IFMT) != S_IFDIR) && mkdir(nativePath, 0755) < 0) {
        return 0;
      }
      *cursor = '/';
    }
  }

  return mkdir(nativePath, 0755) == 0;
}

int OsSetCurrentDirectory(LPCSTR pathName) {
  char nativePath[0x400];

  FATALASSERT(pathName);

  SStrCopy(nativePath, pathName, sizeof(nativePath));
  OsFileConvertSlashes(nativePath);

  return !nativePath[0] || chdir(nativePath) == 0;
}

int OsGetCurrentDirectory(DWORD pathLen, char *pathName) {
  char *curr;

  FATALASSERT(pathName);

  if (!getcwd(pathName, pathLen)) {
    return 0;
  }

  for (curr = pathName; *curr && static_cast<DWORD>(curr - pathName) < pathLen - 1; ++curr) {
    if (*curr == '/') {
      *curr = curr[1] ? '\\' : 0;
    }
  }

  return 1;
}

static int ListNativePattern(LPCSTR pattern, int (*inCallback)(OS_FILE_DATA &, LPVOID), LPVOID inCBParam, int returnHidden) {
  glob_t       results;
  struct stat  stats;
  OS_FILE_DATA osfData;
  char         nativePath[0x400];
  int          stop = 0;
  size_t       index;

  BuildNativePath(pattern, "w", nativePath, sizeof(nativePath));

  if (glob(nativePath, 0, 0, &results)) {
    return 0;
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

  return ListNativePattern(pattern, inCallback, inCBParam, returnHidden);
}

int OsDeleteFile(LPCSTR fileName) {
  char nativePath[0x400];

  FATALASSERT(fileName);

  BuildNativePath(fileName, "w", nativePath, sizeof(nativePath));
  return unlink(nativePath) == 0;
}
