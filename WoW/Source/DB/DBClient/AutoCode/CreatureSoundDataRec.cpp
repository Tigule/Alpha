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

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_soundExertionID) ||
      !SFileReadTyped(f, &m_soundExertionCriticalID) ||
      !SFileReadTyped(f, &m_soundInjuryID) ||
      !SFileReadTyped(f, &m_soundInjuryCriticalID) ||
      !SFileReadTyped(f, &m_soundInjuryCrushingBlowID) ||
      !SFileReadTyped(f, &m_soundDeathID) ||
      !SFileReadTyped(f, &m_soundStunID) ||
      !SFileReadTyped(f, &m_soundStandID) ||
      !SFileReadTyped(f, &m_soundFootstepID) ||
      !SFileReadTyped(f, &m_soundAggroID) ||
      !SFileReadTyped(f, &m_soundWingFlapID) ||
      !SFileReadTyped(f, &m_soundWingGlideID) ||
      !SFileReadTyped(f, &m_soundAlertID) ||
      !SFile::Read(f, &m_soundFidget[0], sizeof(m_soundFidget), 0, 0, 0) ||
      !SFile::Read(f, &m_customAttack[0], sizeof(m_customAttack), 0, 0, 0) ||
      !SFileReadTyped(f, &m_NPCSoundID) ||
      !SFileReadTyped(f, &m_loopSoundID) ||
      !SFileReadTyped(f, &m_creatureImpactType) ||
      !SFileReadTyped(f, &m_soundJumpStartID) ||
      !SFileReadTyped(f, &m_soundJumpEndID)) {
    ConsoleWrite("Error reading CreatureSoundDataRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
