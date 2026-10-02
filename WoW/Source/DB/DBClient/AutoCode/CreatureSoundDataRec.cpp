#include "CreatureSoundDataRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR CreatureSoundDataRec::GetFilename() {
  return "DBFilesClient\\CreatureSoundData.dbc";
}

CreatureSoundDataRec::CreatureSoundDataRec() {
}

CreatureSoundDataRec::~CreatureSoundDataRec() {
}

bool CreatureSoundDataRec::Read(SFile *f, LPCSTR stringBuffer) {
  int error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_soundExertionID) == 0);
  error |= (SFileReadTyped(f, &m_soundExertionCriticalID) == 0);
  error |= (SFileReadTyped(f, &m_soundInjuryID) == 0);
  error |= (SFileReadTyped(f, &m_soundInjuryCriticalID) == 0);
  error |= (SFileReadTyped(f, &m_soundInjuryCrushingBlowID) == 0);
  error |= (SFileReadTyped(f, &m_soundDeathID) == 0);
  error |= (SFileReadTyped(f, &m_soundStunID) == 0);
  error |= (SFileReadTyped(f, &m_soundStandID) == 0);
  error |= (SFileReadTyped(f, &m_soundFootstepID) == 0);
  error |= (SFileReadTyped(f, &m_soundAggroID) == 0);
  error |= (SFileReadTyped(f, &m_soundWingFlapID) == 0);
  error |= (SFileReadTyped(f, &m_soundWingGlideID) == 0);
  error |= (SFileReadTyped(f, &m_soundAlertID) == 0);
  error |= (SFileReadTyped(f, &m_soundFidget) == 0);
  error |= (SFileReadTyped(f, &m_customAttack) == 0);
  error |= (SFileReadTyped(f, &m_NPCSoundID) == 0);
  error |= (SFileReadTyped(f, &m_loopSoundID) == 0);
  error |= (SFileReadTyped(f, &m_creatureImpactType) == 0);
  error |= (SFileReadTyped(f, &m_soundJumpStartID) == 0);
  error |= (SFileReadTyped(f, &m_soundJumpEndID) == 0);

  if (error) {
    ConsoleWrite("Error reading CreatureSoundDataRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
