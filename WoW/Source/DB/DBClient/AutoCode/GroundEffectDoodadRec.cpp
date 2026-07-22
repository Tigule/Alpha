#include "GroundEffectDoodadRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *__fastcall GroundEffectDoodadRec::GetFilename() {
  return "DBFilesClient\\GroundEffectDoodad.dbc";
}

GroundEffectDoodadRec::GroundEffectDoodadRec() {
}

GroundEffectDoodadRec::~GroundEffectDoodadRec() {
}

bool GroundEffectDoodadRec::Read(SFile *f, const char *stringBuffer) {
  bool         result = true;
  unsigned int tempdoodadpathIndices[1];

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_doodadIdTag, sizeof(m_doodadIdTag), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdoodadpathIndices[0], sizeof(tempdoodadpathIndices[0]), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading GroundEffectDoodadRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_doodadpath = stringBuffer + tempdoodadpathIndices[0];
  } else {
    m_doodadpath = "";
  }

  return true;
}
