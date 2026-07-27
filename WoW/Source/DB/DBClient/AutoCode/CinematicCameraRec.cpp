#include "CinematicCameraRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *CinematicCameraRec::GetFilename() {
  return "DBFilesClient\\CinematicCamera.dbc";
}

CinematicCameraRec::CinematicCameraRec() {
}

CinematicCameraRec::~CinematicCameraRec() {
}

bool CinematicCameraRec::Read(SFile *f, const char *stringBuffer) {
  bool         result = true;
  unsigned int tempmodelIndices[1];

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &tempmodelIndices[0], sizeof(tempmodelIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &m_soundID, sizeof(m_soundID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_originX, sizeof(m_originX), 0, 0, 0) && result;
  result = SFile::Read(f, &m_originY, sizeof(m_originY), 0, 0, 0) && result;
  result = SFile::Read(f, &m_originZ, sizeof(m_originZ), 0, 0, 0) && result;
  result = SFile::Read(f, &m_originFacing, sizeof(m_originFacing), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading CinematicCameraRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_model = stringBuffer + tempmodelIndices[0];
  } else {
    m_model = "";
  }

  return true;
}
