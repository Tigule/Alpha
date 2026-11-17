#include <storm.h>

SDIR *APIENTRY SFile::OpenDir(LPCSTR name) {
  return opendir(name);
}

SDIRENT *APIENTRY SFile::ReadDir(SDIR *dir) {
  return readdir(dir);
}

void APIENTRY SFile::CloseDir(SDIR *dir) {
  closedir(dir);
}
