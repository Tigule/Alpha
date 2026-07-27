#include "WeaponImpactSoundsRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *WeaponImpactSoundsRec::GetFilename() {
  return "DBFilesClient\\WeaponImpactSounds.dbc";
}

WeaponImpactSoundsRec::WeaponImpactSoundsRec() {
}

WeaponImpactSoundsRec::~WeaponImpactSoundsRec() {
}

bool WeaponImpactSoundsRec::Read(SFile *f, const char *stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_WeaponSubClassID, sizeof(m_WeaponSubClassID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_ParrySoundType, sizeof(m_ParrySoundType), 0, 0, 0) && result;
  result = SFile::Read(f, &m_impactSoundID[0], sizeof(m_impactSoundID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_critImpactSoundID[0], sizeof(m_critImpactSoundID), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading WeaponImpactSoundsRec", DEFAULT_COLOR);
  }

  return result;
}
