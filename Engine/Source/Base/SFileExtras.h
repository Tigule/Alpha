#pragma once

#include <storm.h>

template <class T>
inline int SFileReadTyped(SFile *file, T *data) {
  int result = 0;
  result = SFile::Read(file, data, sizeof(T), 0, 0, 0);
  return result;
}
