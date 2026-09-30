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

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_precastKit) ||
      !SFileReadTyped(f, &m_castKit) ||
      !SFileReadTyped(f, &m_impactKit) ||
      !SFileReadTyped(f, &m_stateKit) ||
      !SFileReadTyped(f, &m_channelKit) ||
      !SFileReadTyped(f, &m_hasMissile) ||
      !SFileReadTyped(f, &m_missileModel) ||
      !SFileReadTyped(f, &m_missilePathType) ||
      !SFileReadTyped(f, &m_missileDestinationAttachment) ||
      !SFileReadTyped(f, &m_missileSound) ||
      !SFileReadTyped(f, &m_hasAreaEffect) ||
      !SFileReadTyped(f, &m_areaModel) ||
      !SFileReadTyped(f, &m_areaKit) ||
      !SFileReadTyped(f, &m_animEventSoundID) ||
      !SFile::Read(f, &m_weaponTrailRed, sizeof(m_weaponTrailRed), 0, 0, 0) ||
      !SFile::Read(f, &m_weaponTrailGreen, sizeof(m_weaponTrailGreen), 0, 0, 0) ||
      !SFile::Read(f, &m_weaponTrailBlue, sizeof(m_weaponTrailBlue), 0, 0, 0) ||
      !SFile::Read(f, &m_weaponTrailAlpha, sizeof(m_weaponTrailAlpha), 0, 0, 0) ||
      !SFile::Read(f, &m_weaponTrailFadeoutRate, sizeof(m_weaponTrailFadeoutRate), 0, 0, 0) ||
      !SFileReadTyped(f, &m_weaponTrailDuration)) {
    ConsoleWrite("Error reading SpellVisualRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
