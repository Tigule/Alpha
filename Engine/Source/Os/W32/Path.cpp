#include <Base/Base.h>

#include <storm.h>
#include <windows.h>

static struct {
  DWORD     m_id;
  HINSTANCE m_handle;
} s_modules[8];
static LPCSTR s_invalidFileNames[22] = {"nul",  "con",  "prn",  "aux",  "com1", "com2", "com3", "com4", "com5", "com6", "com7",
                                        "com8", "com9", "lpt1", "lpt2", "lpt3", "lpt4", "lpt5", "lpt6", "lpt7", "lpt8", "lpt9"};

void OsPathStripFilename(char *buffer);

DWORD OsPathGetRootChars(LPCSTR path) {
  DWORD pathChars = SStrLen(path);

  if (pathChars < 2) {
    return 0;
  }

  if (path[1] == ':') {
    return path[2] == '\\' ? 3 : 2;
  }

  if (path[0] == '\\' && path[1] == '\\') {
    LPCSTR rootEnd = path + 2;
    int    i;

    for (i = 0; i < 2; ++i) {
      if (rootEnd) {
        rootEnd = SStrChr(rootEnd, '\\') + 1;
      }
    }

    return rootEnd ? (DWORD)(rootEnd - path) : pathChars;
  }

  return 0;
}

LPCSTR OsGetCommandLine() {
  return GetCommandLineA();
}

void OsGetExeName(char *buffer, DWORD chars) {
  GetModuleFileNameA(0, buffer, chars);
}

void OsGetExePath(char *buffer, DWORD chars) {
  OsGetExeName(buffer, chars);
  OsPathStripFilename(buffer);
}

void OsGetStormName(char *buffer, DWORD chars) {
  GetModuleFileNameA(StormGetInstance(), buffer, chars);
}

BOOL OsGetModuleName(DWORD moduleId, char *buffer, DWORD chars) {
  buffer[0] = 0;
  for (UINT i = 0; i < 8; ++i) {
    if (s_modules[i].m_id == moduleId) {
      GetModuleFileNameA(s_modules[i].m_handle, buffer, chars);
      return 1;
    }
  }
  return 0;
}

BOOL OsSetModuleHandle(DWORD moduleId, HINSTANCE__ *moduleHandle) {
  FATALASSERT(moduleId);
  FATALASSERT(moduleHandle);

  for (UINT i = 0; i < 8; ++i) {
    if (!s_modules[i].m_handle || s_modules[i].m_id == moduleId) {
      s_modules[i].m_id = moduleId;
      s_modules[i].m_handle = moduleHandle;
      return 1;
    }
  }
  return 0;
}

void OsClearModuleHandle(DWORD moduleId) {
  for (UINT i = 0; i < 8; ++i) {
    if (s_modules[i].m_id == moduleId) {
      s_modules[i].m_id = 0;
      s_modules[i].m_handle = 0;
      return;
    }
  }
}

void OsPathStripFilename(char *buffer) {
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

void OsPathStripLastDir(char *buffer) {
  DWORD pathChars = SStrLen(buffer);
  DWORD rootChars = OsPathGetRootChars(buffer);
  if (pathChars > rootChars) {
    if (buffer[pathChars - 1] == '\\') {
      buffer[pathChars - 1] = 0;
    }
    OsPathStripFilename(buffer);
  }
}

void OsPathGetFilename(LPCSTR path, char *buffer, UINT size) {
  FATALASSERT(path && buffer && size);
  buffer[0] = 0;
  LPCSTR filename = SStrChrR(path, '\\');
  SStrCopy(buffer, filename ? filename + 1 : path, size);
}

void OsPathGetLastDirectory(LPCSTR string, char *buffer, UINT size) {
  FATALASSERT(string && buffer && size);

  char path[MAX_PATH];
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

BOOL OsPathIsRelative(LPCSTR path) {
  return OsPathGetRootChars(path) == 0;
}

BOOL OsPathHasInvalidChars(LPCSTR path) {
  LPCSTR invalidChars = "*/:><|&+^?\"";
  while (*invalidChars) {
    if (SStrChr(path, *invalidChars++)) {
      return 1;
    }
  }
  return 0;
}

BOOL OsFileNameHasInvalidChars(LPCSTR filename) {
  LPCSTR invalidChars = "\\*/:><|&+^?\"";
  while (*invalidChars) {
    if (SStrChr(filename, *invalidChars++)) {
      return 1;
    }
  }
  return 0;
}

BOOL OsFileNameIsValid(LPCSTR filename) {
  if (OsFileNameHasInvalidChars(filename)) {
    return 0;
  }

  char   filenameNoExt[MAX_PATH];
  LPCSTR extension = SStrChrR(filename, '.');
  UINT   chars = extension ? static_cast<UINT>(extension - filename + 1) : sizeof(filenameNoExt);
  if (chars >= sizeof(filenameNoExt)) {
    chars = sizeof(filenameNoExt);
  }
  SStrCopy(filenameNoExt, filename, chars);

  for (UINT i = 0; i < 22; ++i) {
    if (!SStrCmpI(s_invalidFileNames[i], filenameNoExt, 0x7FFFFFFF)) {
      return 0;
    }
  }
  return 1;
}

void OsGetSystemFontDirectory(char *buffer, UINT chars) {
  UINT charsCopied = GetWindowsDirectoryA(buffer, chars);
  FATALASSERT(charsCopied);

  if (buffer[charsCopied - 1] != '\\') {
    SStrCopy(&buffer[charsCopied], "\\", chars - ++charsCopied);
  }
  SStrCopy(&buffer[charsCopied], "fonts\\", chars - charsCopied);
}

void OsBuildFontFilePath(LPCSTR fileName, char *buffer, UINT size) {
  SStrPrintf(buffer, size, "%s\\%s", "Fonts", fileName);
}
