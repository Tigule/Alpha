#include "SpellIconRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *__fastcall SpellIconRec::GetFilename() {
  return "DBFilesClient\\SpellIcon.dbc";
}

SpellIconRec::SpellIconRec() {
}

SpellIconRec::~SpellIconRec() {
}

bool SpellIconRec::Read(SFile *f, const char *stringBuffer) {
  bool         result = true;
  unsigned int temptextureFilenameIndices[1];

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &temptextureFilenameIndices[0], sizeof(temptextureFilenameIndices[0]), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading SpellIconRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_textureFilename = stringBuffer + temptextureFilenameIndices[0];
  } else {
    m_textureFilename = "";
  }

  return true;
}
