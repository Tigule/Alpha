#include "NamesProfanityRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

LPCSTR NamesProfanityRec::GetFilename() {
  return "DBFilesClient\\NamesProfanity.dbc";
}

NamesProfanityRec::NamesProfanityRec() {
}

NamesProfanityRec::~NamesProfanityRec() {
}

bool NamesProfanityRec::Read(SFile *f, LPCSTR stringBuffer) {
  bool result = true;
  UINT tempNameIndices[1];

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &tempNameIndices[0], sizeof(tempNameIndices[0]), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading NamesProfanityRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_Name = &stringBuffer[tempNameIndices[0]];
  } else {
    m_Name = "";
  }

  return true;
}
