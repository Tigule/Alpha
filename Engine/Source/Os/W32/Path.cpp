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

const char* __fastcall OsGetCommandLine() {
    // TODO: implement
    return 0;
}

void __fastcall OsGetExeName(char *buffer, DWORD chars) {
  GetModuleFileNameA(0, buffer, chars);
}

void __fastcall OsGetExePath(char *buffer, DWORD chars) {
  OsGetExeName(buffer, chars);
  OsPathStripFilename(buffer);
}

void __fastcall OsGetStormName(char* buffer, unsigned long chars) {
    // TODO: implement
}

int __fastcall OsGetModuleName(unsigned long moduleId, char* buffer, unsigned long chars) {
    // TODO: implement
    return 0;
}

int __fastcall OsSetModuleHandle(unsigned long moduleId, HINSTANCE__* moduleHandle) {
    // TODO: implement
    return 0;
}

void __fastcall OsClearModuleHandle(unsigned long moduleId) {
    // TODO: implement
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

void __fastcall OsPathStripLastDir(char* buffer) {
    // TODO: implement
}

void __fastcall OsPathGetFilename(const char* path, char* buffer, unsigned int size) {
    // TODO: implement
}

void __fastcall OsPathGetLastDirectory(const char* string, char* buffer, unsigned int size) {
    // TODO: implement
}

int __fastcall OsPathIsRelative(const char* path) {
    // TODO: implement
    return 0;
}

int __fastcall OsPathHasInvalidChars(const char* path) {
    // TODO: implement
    return 0;
}

int __fastcall OsFileNameHasInvalidChars(const char* filename) {
    // TODO: implement
    return 0;
}

int __fastcall OsFileNameIsValid(const char* filename) {
    // TODO: implement
    return 0;
}

void __fastcall OsGetSystemFontDirectory(char* buffer, unsigned int chars) {
    // TODO: implement
}

void __fastcall OsBuildFontFilePath(const char *fileName, char *buffer, unsigned int size) {
  SStrPrintf(buffer, size, "%s\\%s", "Fonts", fileName);
}
