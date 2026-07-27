#include "SpellVisualRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *SpellVisualRec::GetFilename() {
  return "DBFilesClient\\SpellVisual.dbc";
}

SpellVisualRec::SpellVisualRec() {
}

SpellVisualRec::~SpellVisualRec() {
}

bool SpellVisualRec::Read(SFile *f, const char *stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_precastKit, sizeof(m_precastKit), 0, 0, 0) && result;
  result = SFile::Read(f, &m_castKit, sizeof(m_castKit), 0, 0, 0) && result;
  result = SFile::Read(f, &m_impactKit, sizeof(m_impactKit), 0, 0, 0) && result;
  result = SFile::Read(f, &m_stateKit, sizeof(m_stateKit), 0, 0, 0) && result;
  result = SFile::Read(f, &m_channelKit, sizeof(m_channelKit), 0, 0, 0) && result;
  result = SFile::Read(f, &m_hasMissile, sizeof(m_hasMissile), 0, 0, 0) && result;
  result = SFile::Read(f, &m_missileModel, sizeof(m_missileModel), 0, 0, 0) && result;
  result = SFile::Read(f, &m_missilePathType, sizeof(m_missilePathType), 0, 0, 0) && result;
  result = SFile::Read(f, &m_missileDestinationAttachment, sizeof(m_missileDestinationAttachment), 0, 0, 0) && result;
  result = SFile::Read(f, &m_missileSound, sizeof(m_missileSound), 0, 0, 0) && result;
  result = SFile::Read(f, &m_hasAreaEffect, sizeof(m_hasAreaEffect), 0, 0, 0) && result;
  result = SFile::Read(f, &m_areaModel, sizeof(m_areaModel), 0, 0, 0) && result;
  result = SFile::Read(f, &m_areaKit, sizeof(m_areaKit), 0, 0, 0) && result;
  result = SFile::Read(f, &m_animEventSoundID, sizeof(m_animEventSoundID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_weaponTrailRed, sizeof(m_weaponTrailRed), 0, 0, 0) && result;
  result = SFile::Read(f, &m_weaponTrailGreen, sizeof(m_weaponTrailGreen), 0, 0, 0) && result;
  result = SFile::Read(f, &m_weaponTrailBlue, sizeof(m_weaponTrailBlue), 0, 0, 0) && result;
  result = SFile::Read(f, &m_weaponTrailAlpha, sizeof(m_weaponTrailAlpha), 0, 0, 0) && result;
  result = SFile::Read(f, &m_weaponTrailFadeoutRate, sizeof(m_weaponTrailFadeoutRate), 0, 0, 0) && result;
  result = SFile::Read(f, &m_weaponTrailDuration, sizeof(m_weaponTrailDuration), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading SpellVisualRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
