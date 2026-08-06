#include <Base/Base.h>

#include <storm.h>

DWORD OsPathGetRootChars(LPCSTR path) {
  DWORD pathChars = SStrLen(path);

  if (pathChars && path[0] == '/') {
    return 1;
  }

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

void OsPathStripFilename(char *buffer) {
  char *lastBackslash = SStrChrR(buffer, '\\');
  char *lastSlash = SStrChrR(buffer, '/');
  char *last = lastBackslash > lastSlash ? lastBackslash : lastSlash;

  if (last) {
    DWORD rootChars = OsPathGetRootChars(buffer);

    if (last >= buffer + rootChars) {
      last[1] = 0;
    } else {
      buffer[rootChars] = 0;
    }
  }
}

void OsPathStripLastDir(char *buffer) {
  DWORD pathChars = SStrLen(buffer);
  DWORD rootChars = OsPathGetRootChars(buffer);

  if (pathChars <= rootChars) {
    return;
  }

  if (buffer[pathChars - 1] == '\\' || buffer[pathChars - 1] == '/') {
    buffer[pathChars - 1] = 0;
  }

  OsPathStripFilename(buffer);
}

void OsPathGetFilename(LPCSTR path, char *buffer, UINT size) {
  LPCSTR lastBackslash;
  LPCSTR lastSlash;
  LPCSTR last;

  ASSERT(path && buffer && size);

  buffer[0] = 0;

  lastBackslash = SStrChrR(path, '\\');
  lastSlash = SStrChrR(path, '/');
  last = lastBackslash > lastSlash ? lastBackslash : lastSlash;

  if (last) {
    SStrCopy(buffer, last + 1, size);
  } else {
    SStrCopy(buffer, path, size);
  }
}

void OsBuildFontFilePath(LPCSTR fileName, char *buffer, UINT size) {
  SStrPrintf(buffer, size, "fonts\\%s", fileName);
}
