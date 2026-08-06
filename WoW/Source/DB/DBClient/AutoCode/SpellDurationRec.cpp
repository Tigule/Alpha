#include "SpellDurationRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

LPCSTR SpellDurationRec::GetFilename() {
  return "DBFilesClient\\SpellDuration.dbc";
}

SpellDurationRec::SpellDurationRec() {
}

SpellDurationRec::~SpellDurationRec() {
}

bool SpellDurationRec::Read(SFile *f, LPCSTR stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_duration, sizeof(m_duration), 0, 0, 0) && result;
  result = SFile::Read(f, &m_durationPerLevel, sizeof(m_durationPerLevel), 0, 0, 0) && result;
  result = SFile::Read(f, &m_maxDuration, sizeof(m_maxDuration), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading SpellDurationRec", DEFAULT_COLOR);
  }

  return result;
}
