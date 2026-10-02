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
  int error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_WeaponSubClassID) == 0);
  error |= (SFileReadTyped(f, &m_ParrySoundType) == 0);
  error |= (SFileReadTyped(f, &m_impactSoundID) == 0);
  error |= (SFileReadTyped(f, &m_critImpactSoundID) == 0);

  if (error) {
    ConsoleWrite("Error reading WeaponImpactSoundsRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
