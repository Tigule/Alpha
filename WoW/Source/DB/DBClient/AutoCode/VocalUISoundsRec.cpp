#include "VocalUISoundsRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *__fastcall VocalUISoundsRec::GetFilename() {
  return "DBFilesClient\\VocalUISounds.dbc";
}

VocalUISoundsRec::VocalUISoundsRec() {
}

VocalUISoundsRec::~VocalUISoundsRec() {
}

bool VocalUISoundsRec::Read(SFile *f, const char *stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_vocalUIEnum, sizeof(m_vocalUIEnum), 0, 0, 0) && result;
  result = SFile::Read(f, &m_raceID, sizeof(m_raceID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_NormalSoundID[0], sizeof(m_NormalSoundID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_PissedSoundID[0], sizeof(m_PissedSoundID), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading VocalUISoundsRec", DEFAULT_COLOR);
  }

  return result;
}
