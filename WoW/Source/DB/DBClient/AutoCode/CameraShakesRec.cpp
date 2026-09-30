#include "CameraShakesRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR CameraShakesRec::GetFilename() {
  return "DBFilesClient\\CameraShakes.dbc";
}

CameraShakesRec::CameraShakesRec() {
}

CameraShakesRec::~CameraShakesRec() {
}

bool CameraShakesRec::Read(SFile *f, LPCSTR stringBuffer) {

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_shakeType) ||
      !SFileReadTyped(f, &m_direction) ||
      !SFileReadTyped(f, &m_amplitude) ||
      !SFileReadTyped(f, &m_frequency) ||
      !SFileReadTyped(f, &m_duration) ||
      !SFileReadTyped(f, &m_phase) ||
      !SFileReadTyped(f, &m_coefficient)) {
    ConsoleWrite("Error reading CameraShakesRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
