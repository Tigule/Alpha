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

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_SwingType) ||
      !SFileReadTyped(f, &m_Crit) ||
      !SFileReadTyped(f, &m_SoundID)) {
    ConsoleWrite("Error reading WeaponSwingSounds2Rec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
