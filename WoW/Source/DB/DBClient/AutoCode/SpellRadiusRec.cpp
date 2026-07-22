#include "SpellRadiusRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *__fastcall SpellRadiusRec::GetFilename() {
  return "DBFilesClient\\SpellRadius.dbc";
}

SpellRadiusRec::SpellRadiusRec() {
}

SpellRadiusRec::~SpellRadiusRec() {
}

bool SpellRadiusRec::Read(SFile *f, const char *stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_radius, sizeof(m_radius), 0, 0, 0) && result;
  result = SFile::Read(f, &m_radiusPerLevel, sizeof(m_radiusPerLevel), 0, 0, 0) && result;
  result = SFile::Read(f, &m_radiusMax, sizeof(m_radiusMax), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading SpellRadiusRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
