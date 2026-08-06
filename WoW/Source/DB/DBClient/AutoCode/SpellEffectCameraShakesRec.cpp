#include "SpellEffectCameraShakesRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

LPCSTR SpellEffectCameraShakesRec::GetFilename() {
  return "DBFilesClient\\SpellEffectCameraShakes.dbc";
}

SpellEffectCameraShakesRec::SpellEffectCameraShakesRec() {
}

SpellEffectCameraShakesRec::~SpellEffectCameraShakesRec() {
}

bool SpellEffectCameraShakesRec::Read(SFile *f, LPCSTR stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_CameraShake[0], sizeof(m_CameraShake), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading SpellEffectCameraShakesRec", DEFAULT_COLOR);
  }

  return result;
}
