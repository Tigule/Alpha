#include "SpellVisualRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR SpellVisualRec::GetFilename() {
  return "DBFilesClient\\SpellVisual.dbc";
}

SpellVisualRec::SpellVisualRec() {
}

SpellVisualRec::~SpellVisualRec() {
}

bool SpellVisualRec::Read(SFile *f, LPCSTR stringBuffer) {
  int error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_precastKit) == 0);
  error |= (SFileReadTyped(f, &m_castKit) == 0);
  error |= (SFileReadTyped(f, &m_impactKit) == 0);
  error |= (SFileReadTyped(f, &m_stateKit) == 0);
  error |= (SFileReadTyped(f, &m_channelKit) == 0);
  error |= (SFileReadTyped(f, &m_hasMissile) == 0);
  error |= (SFileReadTyped(f, &m_missileModel) == 0);
  error |= (SFileReadTyped(f, &m_missilePathType) == 0);
  error |= (SFileReadTyped(f, &m_missileDestinationAttachment) == 0);
  error |= (SFileReadTyped(f, &m_missileSound) == 0);
  error |= (SFileReadTyped(f, &m_hasAreaEffect) == 0);
  error |= (SFileReadTyped(f, &m_areaModel) == 0);
  error |= (SFileReadTyped(f, &m_areaKit) == 0);
  error |= (SFileReadTyped(f, &m_animEventSoundID) == 0);
  error |= (SFileReadTyped(f, &m_weaponTrailRed) == 0);
  error |= (SFileReadTyped(f, &m_weaponTrailGreen) == 0);
  error |= (SFileReadTyped(f, &m_weaponTrailBlue) == 0);
  error |= (SFileReadTyped(f, &m_weaponTrailAlpha) == 0);
  error |= (SFileReadTyped(f, &m_weaponTrailFadeoutRate) == 0);
  error |= (SFileReadTyped(f, &m_weaponTrailDuration) == 0);

  if (error) {
    ConsoleWrite("Error reading SpellVisualRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
