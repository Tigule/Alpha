#include "VocalUISoundsRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR VocalUISoundsRec::GetFilename() {
  return "DBFilesClient\\VocalUISounds.dbc";
}

VocalUISoundsRec::VocalUISoundsRec() {
}

VocalUISoundsRec::~VocalUISoundsRec() {
}

bool VocalUISoundsRec::Read(SFile *f, LPCSTR stringBuffer) {
  int error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_vocalUIEnum) == 0);
  error |= (SFileReadTyped(f, &m_raceID) == 0);
  error |= (SFileReadTyped(f, &m_NormalSoundID) == 0);
  error |= (SFileReadTyped(f, &m_PissedSoundID) == 0);

  if (error) {
    ConsoleWrite("Error reading VocalUISoundsRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
