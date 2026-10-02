#include "SpellEffectCameraShakesRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR SpellEffectCameraShakesRec::GetFilename() {
  return "DBFilesClient\\SpellEffectCameraShakes.dbc";
}

SpellEffectCameraShakesRec::SpellEffectCameraShakesRec() {
}

SpellEffectCameraShakesRec::~SpellEffectCameraShakesRec() {
}

bool SpellEffectCameraShakesRec::Read(SFile *f, LPCSTR stringBuffer) {
  int error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_CameraShake) == 0);

  if (error) {
    ConsoleWrite("Error reading SpellEffectCameraShakesRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
