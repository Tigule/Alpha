#include <storm.h>
#include <windows.h>

void __fastcall OsPathStripFilename(char *buffer);

DWORD __fastcall OsPathGetRootChars(const char *path) {
  DWORD pathChars = SStrLen(path);

  if (pathChars < 2) {
    return 0;
  }

  if (path[1] == ':') {
    return path[2] == '\\' ? 3 : 2;
  }

  if (path[0] == '\\' && path[1] == '\\') {
    const char *rootEnd = path + 2;
    int         i;

    for (i = 0; i < 2; ++i) {
      if (rootEnd) {
        rootEnd = SStrChr(rootEnd, '\\') + 1;
      }
    }

    return rootEnd ? (DWORD)(rootEnd - path) : pathChars;
  }

  return 0;
}

void __fastcall OsGetExeName(char *buffer, DWORD chars) {
  GetModuleFileNameA(0, buffer, chars);
}

void __fastcall OsGetExePath(char *buffer, DWORD chars) {
  OsGetExeName(buffer, chars);
  OsPathStripFilename(buffer);
}

void __fastcall OsPathStripFilename(char *buffer) {
  char *filename = SStrChrR(buffer, '\\');

  if (filename) {
    char *rootEnd = buffer + OsPathGetRootChars(buffer);

    if (filename < rootEnd) {
      *rootEnd = 0;
    } else {
      filename[1] = 0;
    }
  }
}

void __fastcall OsBuildFontFilePath(const char *fileName, char *buffer, unsigned int size) {
  SStrPrintf(buffer, size, "%s\\%s", "Fonts", fileName);
}
