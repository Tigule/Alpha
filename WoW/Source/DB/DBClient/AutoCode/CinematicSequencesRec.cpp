#include "CinematicSequencesRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR CinematicSequencesRec::GetFilename() {
  return "DBFilesClient\\CinematicSequences.dbc";
}

CinematicSequencesRec::CinematicSequencesRec() {
}

CinematicSequencesRec::~CinematicSequencesRec() {
}

bool CinematicSequencesRec::Read(SFile *f, LPCSTR stringBuffer) {

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_soundID) ||
      !SFile::Read(f, &m_camera[0], sizeof(m_camera), 0, 0, 0)) {
    ConsoleWrite("Error reading CinematicSequencesRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
