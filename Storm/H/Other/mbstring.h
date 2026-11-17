#pragma once
#include <ctype.h>

inline int _ismbcspace(unsigned int c) {
  return c < 0x80 && isspace((int)c);
}
