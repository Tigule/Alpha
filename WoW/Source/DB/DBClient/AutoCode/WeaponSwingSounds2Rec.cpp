#include "WeaponSwingSounds2Rec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *WeaponSwingSounds2Rec::GetFilename() {
  return "DBFilesClient\\WeaponSwingSounds2.dbc";
}

WeaponSwingSounds2Rec::WeaponSwingSounds2Rec() {
}

WeaponSwingSounds2Rec::~WeaponSwingSounds2Rec() {
}

bool WeaponSwingSounds2Rec::Read(SFile *f, const char *stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_SwingType, sizeof(m_SwingType), 0, 0, 0) && result;
  result = SFile::Read(f, &m_Crit, sizeof(m_Crit), 0, 0, 0) && result;
  result = SFile::Read(f, &m_SoundID, sizeof(m_SoundID), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading WeaponSwingSounds2Rec", DEFAULT_COLOR);
  }

  return result;
}
