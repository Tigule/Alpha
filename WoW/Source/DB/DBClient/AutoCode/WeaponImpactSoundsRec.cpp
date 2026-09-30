#include "WeaponImpactSoundsRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR WeaponImpactSoundsRec::GetFilename() {
  return "DBFilesClient\\WeaponImpactSounds.dbc";
}

WeaponImpactSoundsRec::WeaponImpactSoundsRec() {
}

WeaponImpactSoundsRec::~WeaponImpactSoundsRec() {
}

bool WeaponImpactSoundsRec::Read(SFile *f, LPCSTR stringBuffer) {

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_WeaponSubClassID) ||
      !SFileReadTyped(f, &m_ParrySoundType) ||
      !SFile::Read(f, &m_impactSoundID[0], sizeof(m_impactSoundID), 0, 0, 0) ||
      !SFile::Read(f, &m_critImpactSoundID[0], sizeof(m_critImpactSoundID), 0, 0, 0)) {
    ConsoleWrite("Error reading WeaponImpactSoundsRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
