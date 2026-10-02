#include "WeaponSwingSounds2Rec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR WeaponSwingSounds2Rec::GetFilename() {
  return "DBFilesClient\\WeaponSwingSounds2.dbc";
}

WeaponSwingSounds2Rec::WeaponSwingSounds2Rec() {
}

WeaponSwingSounds2Rec::~WeaponSwingSounds2Rec() {
}

bool WeaponSwingSounds2Rec::Read(SFile *f, LPCSTR stringBuffer) {
  int error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_SwingType) == 0);
  error |= (SFileReadTyped(f, &m_Crit) == 0);
  error |= (SFileReadTyped(f, &m_SoundID) == 0);

  if (error) {
    ConsoleWrite("Error reading WeaponSwingSounds2Rec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
