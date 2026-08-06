#include "CreatureFamilyRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

LPCSTR CreatureFamilyRec::GetFilename() {
  return "DBFilesClient\\CreatureFamily.dbc";
}

CreatureFamilyRec::CreatureFamilyRec() {
}

CreatureFamilyRec::~CreatureFamilyRec() {
}

bool CreatureFamilyRec::Read(SFile *f, LPCSTR stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_minScale, sizeof(m_minScale), 0, 0, 0) && result;
  result = SFile::Read(f, &m_minScaleLevel, sizeof(m_minScaleLevel), 0, 0, 0) && result;
  result = SFile::Read(f, &m_maxScale, sizeof(m_maxScale), 0, 0, 0) && result;
  result = SFile::Read(f, &m_maxScaleLevel, sizeof(m_maxScaleLevel), 0, 0, 0) && result;
  result = SFile::Read(f, &m_skillLine[0], sizeof(m_skillLine), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading CreatureFamilyRec", DEFAULT_COLOR);
  }

  return result;
}
