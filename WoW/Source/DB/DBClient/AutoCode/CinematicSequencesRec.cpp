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
  int error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_soundID) == 0);
  error |= (SFileReadTyped(f, &m_camera) == 0);

  if (error) {
    ConsoleWrite("Error reading CinematicSequencesRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
