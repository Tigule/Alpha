#include <storm.h>

#include <sys/stat.h>

SDIR *APIENTRY SFile::OpenDir(LPCSTR name) {
  SDIR        *dir;
  struct _stat stats;
  char        *end;

  dir = static_cast<SDIR *>(SMemAlloc(sizeof(SDIR), __FILE__, __LINE__, 0));
  {
    char   ch;
    LPCSTR in;
    char  *out;

    in = name;
    out = dir->name;
    do {
      ch = *in++;
      *out++ = ch;
    } while (ch);
  }
  end = dir->name + strlen(dir->name) - 1;
  if (*end == '\\') {
    do {
      if (end <= dir->name) {
        break;
      }
      *end-- = 0;
    } while (*end == '\\');
  }

  if (_stat(dir->name, &stats) != 0 || (stats.st_mode & _S_IFDIR) == 0) {
    delete dir;
    return NULL;
  }

  dir->handle = NULL;
  ++end;
  *end++ = '\\';
  *end++ = '*';
  *end = 0;
  return dir;
}

SDIRENT *APIENTRY SFile::ReadDir(SDIR *dir) {
  if (!dir->handle) {
    dir->handle = FindFirstFileA(dir->name, &dir->findData);
    if (dir->handle == INVALID_HANDLE_VALUE) {
      return NULL;
    }

    {
      char   ch;
      LPCSTR in;
      char  *out;

      in = dir->findData.cFileName;
      out = dir->dirent.d_name;
      do {
        ch = *in++;
        *out++ = ch;
      } while (ch);
    }
  } else if (!FindNextFileA(dir->handle, &dir->findData)) {
    return NULL;
  } else {
    char   ch;
    LPCSTR in;
    char  *out;

    in = dir->findData.cFileName;
    out = dir->dirent.d_name;
    do {
      ch = *in++;
      *out++ = ch;
    } while (ch);
  }

  return &dir->dirent;
}

void APIENTRY SFile::CloseDir(SDIR *dir) {
  if (!dir) {
    return;
  }

  FindClose(dir->handle);
  delete dir;
}
