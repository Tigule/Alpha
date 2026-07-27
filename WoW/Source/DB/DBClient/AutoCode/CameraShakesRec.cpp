#include "CameraShakesRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *CameraShakesRec::GetFilename() {
  return "DBFilesClient\\CameraShakes.dbc";
}

CameraShakesRec::CameraShakesRec() {
}

CameraShakesRec::~CameraShakesRec() {
}

bool CameraShakesRec::Read(SFile *f, const char *stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_shakeType, sizeof(m_shakeType), 0, 0, 0) && result;
  result = SFile::Read(f, &m_direction, sizeof(m_direction), 0, 0, 0) && result;
  result = SFile::Read(f, &m_amplitude, sizeof(m_amplitude), 0, 0, 0) && result;
  result = SFile::Read(f, &m_frequency, sizeof(m_frequency), 0, 0, 0) && result;
  result = SFile::Read(f, &m_duration, sizeof(m_duration), 0, 0, 0) && result;
  result = SFile::Read(f, &m_phase, sizeof(m_phase), 0, 0, 0) && result;
  result = SFile::Read(f, &m_coefficient, sizeof(m_coefficient), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading CameraShakesRec", DEFAULT_COLOR);
  }

  return result;
}
