#pragma once

#include <storm.h>

inline int SFileReadTyped(SFile *file, UINT *data) {
  int result = 0;
  result = SFile::Read(file, data, sizeof(*data), 0, 0, 0);
  return result;
}

inline int SFileReadTyped(SFile *file, int *data) {
  int result = 0;
  result = SFile::Read(file, data, sizeof(*data), 0, 0, 0);
  return result;
}

inline int SFileReadTyped(SFile *file, float *data) {
  int result = 0;
  result = SFile::Read(file, data, sizeof(*data), 0, 0, 0);
  return result;
}

template <class T>
inline int SFileReadTyped(SFile *file, T *data) {
  int result = 0;
  result = SFile::Read(file, data, sizeof(T), 0, 0, 0);
  return result;
}
