#include "StringLookupsRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *StringLookupsRec::GetFilename() {
  return "DBFilesClient\\StringLookups.dbc";
}

StringLookupsRec::StringLookupsRec() {
}

StringLookupsRec::~StringLookupsRec() {
}

bool StringLookupsRec::Read(SFile *f, const char *stringBuffer) {
  bool         result = true;
  unsigned int tempStringIndices[1];

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &tempStringIndices[0], sizeof(tempStringIndices[0]), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading StringLookupsRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_String = stringBuffer + tempStringIndices[0];
  } else {
    m_String = "";
  }

  return true;
}
