#include <storm.h>
#include <windows.h>

struct OsModule {
  unsigned long m_id;
  HINSTANCE     m_handle;
};

static OsModule s_modules[8];
static const char *s_invalidFileNames[22] = {
    "nul",  "con",  "prn",  "aux",  "com1", "com2", "com3", "com4", "com5", "com6", "com7",
    "com8", "com9", "lpt1", "lpt2", "lpt3", "lpt4", "lpt5", "lpt6", "lpt7", "lpt8", "lpt9"
};

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
  return GetCommandLineA();
}

void __fastcall OsGetExeName(char *buffer, DWORD chars) {
  GetModuleFileNameA(0, buffer, chars);
}

void __fastcall OsGetExePath(char *buffer, DWORD chars) {
  OsGetExeName(buffer, chars);
  OsPathStripFilename(buffer);
}

void __fastcall OsGetStormName(char* buffer, unsigned long chars) {
  GetModuleFileNameA(StormGetInstance(), buffer, chars);
}

int __fastcall OsGetModuleName(unsigned long moduleId, char* buffer, unsigned long chars) {
  buffer[0] = 0;
  for (unsigned int i = 0; i < 8; ++i) {
    if (s_modules[i].m_id == moduleId) {
      GetModuleFileNameA(s_modules[i].m_handle, buffer, chars);
      return 1;
    }
  }
  return 0;
}

int __fastcall OsSetModuleHandle(unsigned long moduleId, HINSTANCE__* moduleHandle) {
  FATALASSERT(moduleId);
  FATALASSERT(moduleHandle);

  for (unsigned int i = 0; i < 8; ++i) {
    if (!s_modules[i].m_handle || s_modules[i].m_id == moduleId) {
      s_modules[i].m_id = moduleId;
      s_modules[i].m_handle = moduleHandle;
      return 1;
    }
  }
  return 0;
}

void __fastcall OsClearModuleHandle(unsigned long moduleId) {
  for (unsigned int i = 0; i < 8; ++i) {
    if (s_modules[i].m_id == moduleId) {
      s_modules[i].m_id = 0;
      s_modules[i].m_handle = 0;
      return;
    }
  }
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
  unsigned long pathChars = SStrLen(buffer);
  unsigned long rootChars = OsPathGetRootChars(buffer);
  if (pathChars > rootChars) {
    if (buffer[pathChars - 1] == '\\') {
      buffer[pathChars - 1] = 0;
    }
    OsPathStripFilename(buffer);
  }
}

void __fastcall OsPathGetFilename(const char* path, char* buffer, unsigned int size) {
  FATALASSERT(path && buffer && size);
  buffer[0] = 0;
  const char *filename = SStrChrR(path, '\\');
  SStrCopy(buffer, filename ? filename + 1 : path, size);
}

void __fastcall OsPathGetLastDirectory(const char* string, char* buffer, unsigned int size) {
  FATALASSERT(string && buffer && size);

  char path[260];
  SStrCopy(path, string, sizeof(path));
  OsPathStripFilename(path);

  char *directory = SStrChrR(path, '\\');
  if (directory) {
    if (!directory[1]) {
      *directory = 0;
      directory = SStrChrR(path, '\\');
    }
    SStrCopy(buffer, directory + 1, size);
  } else {
    SStrCopy(buffer, path, size);
  }
}

int __fastcall OsPathIsRelative(const char* path) {
  return OsPathGetRootChars(path) == 0;
}

int __fastcall OsPathHasInvalidChars(const char* path) {
  const char *invalidChars = "*/:><|&+^?\"";
  while (*invalidChars) {
    if (SStrChr(path, *invalidChars++)) {
      return 1;
    }
  }
  return 0;
}

int __fastcall OsFileNameHasInvalidChars(const char* filename) {
  const char *invalidChars = "\\*/:><|&+^?\"";
  while (*invalidChars) {
    if (SStrChr(filename, *invalidChars++)) {
      return 1;
    }
  }
  return 0;
}

int __fastcall OsFileNameIsValid(const char* filename) {
  if (OsFileNameHasInvalidChars(filename)) {
    return 0;
  }

  char filenameNoExt[260];
  const char *extension = SStrChrR(filename, '.');
  unsigned int chars = extension ? static_cast<unsigned int>(extension - filename + 1) : sizeof(filenameNoExt);
  if (chars >= sizeof(filenameNoExt)) {
    chars = sizeof(filenameNoExt);
  }
  SStrCopy(filenameNoExt, filename, chars);

  for (unsigned int i = 0; i < 22; ++i) {
    if (!SStrCmpI(s_invalidFileNames[i], filenameNoExt, 0x7FFFFFFF)) {
      return 0;
    }
  }
  return 1;
}

void __fastcall OsGetSystemFontDirectory(char* buffer, unsigned int chars) {
  unsigned int charsCopied = GetWindowsDirectoryA(buffer, chars);
  FATALASSERT(charsCopied);

  if (buffer[charsCopied - 1] != '\\') {
    SStrCopy(&buffer[charsCopied], "\\", chars - ++charsCopied);
  }
  SStrCopy(&buffer[charsCopied], "fonts\\", chars - charsCopied);
}

void __fastcall OsBuildFontFilePath(const char *fileName, char *buffer, unsigned int size) {
  SStrPrintf(buffer, size, "%s\\%s", "Fonts", fileName);
}
