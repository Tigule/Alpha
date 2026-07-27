#include "CreatureSoundDataRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *CreatureSoundDataRec::GetFilename() {
  return "DBFilesClient\\CreatureSoundData.dbc";
}

CreatureSoundDataRec::CreatureSoundDataRec() {
}

CreatureSoundDataRec::~CreatureSoundDataRec() {
}

bool CreatureSoundDataRec::Read(SFile *f, const char *stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_soundExertionID, sizeof(m_soundExertionID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_soundExertionCriticalID, sizeof(m_soundExertionCriticalID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_soundInjuryID, sizeof(m_soundInjuryID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_soundInjuryCriticalID, sizeof(m_soundInjuryCriticalID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_soundInjuryCrushingBlowID, sizeof(m_soundInjuryCrushingBlowID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_soundDeathID, sizeof(m_soundDeathID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_soundStunID, sizeof(m_soundStunID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_soundStandID, sizeof(m_soundStandID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_soundFootstepID, sizeof(m_soundFootstepID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_soundAggroID, sizeof(m_soundAggroID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_soundWingFlapID, sizeof(m_soundWingFlapID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_soundWingGlideID, sizeof(m_soundWingGlideID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_soundAlertID, sizeof(m_soundAlertID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_soundFidget[0], sizeof(m_soundFidget), 0, 0, 0) && result;
  result = SFile::Read(f, &m_customAttack[0], sizeof(m_customAttack), 0, 0, 0) && result;
  result = SFile::Read(f, &m_NPCSoundID, sizeof(m_NPCSoundID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_loopSoundID, sizeof(m_loopSoundID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_creatureImpactType, sizeof(m_creatureImpactType), 0, 0, 0) && result;
  result = SFile::Read(f, &m_soundJumpStartID, sizeof(m_soundJumpStartID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_soundJumpEndID, sizeof(m_soundJumpEndID), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading CreatureSoundDataRec", DEFAULT_COLOR);
  }

  return result;
}
