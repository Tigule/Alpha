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
  int error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_shakeType) == 0);
  error |= (SFileReadTyped(f, &m_direction) == 0);
  error |= (SFileReadTyped(f, &m_amplitude) == 0);
  error |= (SFileReadTyped(f, &m_frequency) == 0);
  error |= (SFileReadTyped(f, &m_duration) == 0);
  error |= (SFileReadTyped(f, &m_phase) == 0);
  error |= (SFileReadTyped(f, &m_coefficient) == 0);

  if (error) {
    ConsoleWrite("Error reading CameraShakesRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
