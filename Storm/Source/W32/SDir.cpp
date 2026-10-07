#include <storm.h>

#include <sys/stat.h>

SDIR *APIENTRY SFile::OpenDir(LPCSTR name) {
  SDIR *dir = NEW(SDIR);
  strcpy(dir->name, name);
  char *end = dir->name + strlen(dir->name) - 1;
  while (*end == '\\' && end > dir->name) {
    *end-- = 0;
  }
  struct _stat stats;
  if (!_stat(dir->name, &stats) && (stats.st_mode & _S_IFDIR)) {
    dir->handle = NULL;
    *++end = '\\';
    *++end = '*';
    *++end = 0;
    return dir;
  }

  delete dir;
  return NULL;
}

SDIRENT *APIENTRY SFile::ReadDir(SDIR *dir) {
  if (!dir->handle) {
    dir->handle = FindFirstFile(dir->name, &dir->findData);
    if (dir->handle != INVALID_HANDLE_VALUE) {
      strcpy(dir->dirent.d_name, dir->findData.cFileName);
      return &dir->dirent;
    }
  } else {
    if (FindNextFile(dir->handle, &dir->findData)) {
      strcpy(dir->dirent.d_name, dir->findData.cFileName);
      return &dir->dirent;
    }
  }
  return NULL;
}

void APIENTRY SFile::CloseDir(SDIR *dir) {
  if (!dir) {
    return;
  }

  FindClose(dir->handle);
  delete dir;
}
