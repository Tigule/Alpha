#include "FootprintTexturesRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *__fastcall FootprintTexturesRec::GetFilename() {
  return "DBFilesClient\\FootprintTextures.dbc";
}

FootprintTexturesRec::FootprintTexturesRec() {
}

FootprintTexturesRec::~FootprintTexturesRec() {
}

bool FootprintTexturesRec::Read(SFile *f, const char *stringBuffer) {
  bool         result = true;
  unsigned int tempFootstepFilenameIndices[1];

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &tempFootstepFilenameIndices[0], sizeof(tempFootstepFilenameIndices[0]), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading FootprintTexturesRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_FootstepFilename = stringBuffer + tempFootstepFilenameIndices[0];
  } else {
    m_FootstepFilename = "";
  }

  return true;
}
