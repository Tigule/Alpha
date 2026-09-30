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

  if (!SFileReadTyped(f, &m_ID) ||
      !SFile::Read(f, &m_CameraShake[0], sizeof(m_CameraShake), 0, 0, 0)) {
    ConsoleWrite("Error reading SpellEffectCameraShakesRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
