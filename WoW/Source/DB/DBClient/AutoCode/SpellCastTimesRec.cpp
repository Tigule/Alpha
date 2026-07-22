#include "SpellCastTimesRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *__fastcall SpellCastTimesRec::GetFilename() {
  return "DBFilesClient\\SpellCastTimes.dbc";
}

SpellCastTimesRec::SpellCastTimesRec() {
}

SpellCastTimesRec::~SpellCastTimesRec() {
}

bool SpellCastTimesRec::Read(SFile *f, const char *stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_base, sizeof(m_base), 0, 0, 0) && result;
  result = SFile::Read(f, &m_perLevel, sizeof(m_perLevel), 0, 0, 0) && result;
  result = SFile::Read(f, &m_minimum, sizeof(m_minimum), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading SpellCastTimesRec", DEFAULT_COLOR);
  }

  return result;
}
