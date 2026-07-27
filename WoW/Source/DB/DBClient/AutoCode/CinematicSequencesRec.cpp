#include "CinematicSequencesRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *CinematicSequencesRec::GetFilename() {
  return "DBFilesClient\\CinematicSequences.dbc";
}

CinematicSequencesRec::CinematicSequencesRec() {
}

CinematicSequencesRec::~CinematicSequencesRec() {
}

bool CinematicSequencesRec::Read(SFile *f, const char *stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_soundID, sizeof(m_soundID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_camera[0], sizeof(m_camera), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading CinematicSequencesRec", DEFAULT_COLOR);
  }

  return result;
}
