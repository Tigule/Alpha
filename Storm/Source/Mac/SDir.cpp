#include <storm.h>

#include <dirent.h>

void OsFileConvertSlashes(char *path);

SDIR *APIENTRY SFile::OpenDir(LPCSTR name) {
  char native[MAX_PATH];

  // the client addresses directories with backslashes
  SStrCopy(native, name, sizeof(native));
  OsFileConvertSlashes(native);

  return opendir(native);
}

SDIRENT *APIENTRY SFile::ReadDir(SDIR *dir) {
  return readdir(dir);
}

void APIENTRY SFile::CloseDir(SDIR *dir) {
  closedir(dir);
}
