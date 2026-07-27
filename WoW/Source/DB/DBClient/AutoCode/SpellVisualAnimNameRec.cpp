#include "SpellVisualAnimNameRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *SpellVisualAnimNameRec::GetFilename() {
  return "DBFilesClient\\SpellVisualAnimName.dbc";
}

SpellVisualAnimNameRec::SpellVisualAnimNameRec() {
}

SpellVisualAnimNameRec::~SpellVisualAnimNameRec() {
}

bool SpellVisualAnimNameRec::Read(SFile *f, const char *stringBuffer) {
  bool         result = true;
  unsigned int tempnameIndices[1];

  result = SFile::Read(f, &m_AnimID, sizeof(m_AnimID), 0, 0, 0) && result;
  result = SFile::Read(f, &tempnameIndices[0], sizeof(tempnameIndices[0]), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading SpellVisualAnimNameRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_name = stringBuffer + tempnameIndices[0];
  } else {
    m_name = "";
  }

  return true;
}
