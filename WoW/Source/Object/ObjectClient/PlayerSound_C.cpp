#include "Object/ObjectClient/Unit_C.h"
#include "Object/ObjectClient/Player_C.h"
#include "Client.h"
#include "SoundInterface/SoundInterface.h"

int __fastcall GetSoundID(CreatureSoundDataRec *soundData, UNITSOUNDTYPE soundType);
int __fastcall CheckUnitSoundTimer(UNITSOUNDTYPE soundType);

static unsigned int s_playerSoundChances[16] = {35, 100, 30, 100, 100, 100, 40, 100, 100, 100, 100, 100, 100, 100, 100, 100};

int __fastcall CheckPlayerPlaySound(UNITSOUNDTYPE soundType) {
  FATALASSERT(static_cast<unsigned int>(soundType) < 16);
  unsigned int random = NTempest::CRandom::uint32_(g_rndSeed);
  unsigned int value = static_cast<unsigned int>((static_cast<unsigned __int64>(101) * random) >> 32);
  return s_playerSoundChances[soundType] >= value;
}

void CGPlayer_C::PlayUnitSound(UNITSOUNDTYPE soundType, int alwaysPlay) const {
  if (soundType == 8 || (!alwaysPlay && !CheckPlayerPlaySound(soundType))) {
    return;
  }
  if (!CheckUnitSoundTimer(soundType)) {
    return;
  }
  int soundID = GetSoundID(m_soundData, soundType);
  if (!soundID) {
    return;
  }
  NTempest::C3Vector position;
  GetPosition(position);
  position.z += 2.0f;
  SndInterfacePlaySound(soundID, position, -1, 1.0f);
}

void __fastcall PlayerInitializeSounds() {
}

void __fastcall PlayerShutdownSounds() {
}
