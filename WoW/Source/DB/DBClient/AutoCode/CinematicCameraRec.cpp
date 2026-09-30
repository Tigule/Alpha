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

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &tempmodelIndices[0]) ||
      !SFileReadTyped(f, &m_soundID) ||
      !SFileReadTyped(f, &m_originX) ||
      !SFileReadTyped(f, &m_originY) ||
      !SFileReadTyped(f, &m_originZ) ||
      !SFileReadTyped(f, &m_originFacing)) {
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
