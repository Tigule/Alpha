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

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_vocalUIEnum) ||
      !SFileReadTyped(f, &m_raceID) ||
      !SFile::Read(f, &m_NormalSoundID[0], sizeof(m_NormalSoundID), 0, 0, 0) ||
      !SFile::Read(f, &m_PissedSoundID[0], sizeof(m_PissedSoundID), 0, 0, 0)) {
    ConsoleWrite("Error reading VocalUISoundsRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
