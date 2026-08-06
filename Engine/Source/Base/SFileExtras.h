#pragma once

#include <storm.h>

inline int SFileReadTyped(SFile *file, UINT *data) {
  return SFile::Read(file, data, sizeof(*data), 0, 0, 0);
}

inline int SFileReadTyped(SFile *file, int *data) {
  return SFile::Read(file, data, sizeof(*data), 0, 0, 0);
}

inline int SFileReadTyped(SFile *file, float *data) {
  return SFile::Read(file, data, sizeof(*data), 0, 0, 0);
}
