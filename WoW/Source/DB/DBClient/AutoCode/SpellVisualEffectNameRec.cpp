#include "SpellVisualEffectNameRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *SpellVisualEffectNameRec::GetFilename() {
  return "DBFilesClient\\SpellVisualEffectName.dbc";
}

SpellVisualEffectNameRec::SpellVisualEffectNameRec() {
}

SpellVisualEffectNameRec::~SpellVisualEffectNameRec() {
}

bool SpellVisualEffectNameRec::Read(SFile *f, const char *stringBuffer) {
  bool         result = true;
  unsigned int tempfileNameIndices[1];

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &tempfileNameIndices[0], sizeof(tempfileNameIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &m_specialID, sizeof(m_specialID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_specialAttachPoint, sizeof(m_specialAttachPoint), 0, 0, 0) && result;
  result = SFile::Read(f, &m_areaEffectSize, sizeof(m_areaEffectSize), 0, 0, 0) && result;
  result = SFile::Read(f, &m_VisualEffectNameFlags, sizeof(m_VisualEffectNameFlags), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading SpellVisualEffectNameRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_fileName = stringBuffer + tempfileNameIndices[0];
  } else {
    m_fileName = "";
  }

  return true;
}
