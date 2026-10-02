#include "CinematicCameraRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR CinematicCameraRec::GetFilename() {
  return "DBFilesClient\\CinematicCamera.dbc";
}

CinematicCameraRec::CinematicCameraRec() {
}

CinematicCameraRec::~CinematicCameraRec() {
}

bool CinematicCameraRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempmodelIndices[1];
  int  error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &tempmodelIndices[0]) == 0);
  error |= (SFileReadTyped(f, &m_soundID) == 0);
  error |= (SFileReadTyped(f, &m_originX) == 0);
  error |= (SFileReadTyped(f, &m_originY) == 0);
  error |= (SFileReadTyped(f, &m_originZ) == 0);
  error |= (SFileReadTyped(f, &m_originFacing) == 0);

  if (error) {
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
