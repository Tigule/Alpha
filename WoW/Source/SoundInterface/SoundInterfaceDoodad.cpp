#include "SoundInterface.h"
#include "ISoundInterface.h"

#include <Event/EvtApi.h>

struct LOOPEDDOODADDESC {
  LOOPEDDOODADDESC() : posInUseFlags(0), soundID(-1), sound(0), currentIndex(0) {
  }

  int  FindFreeSlot() const;
  void Update(const NTempest::C3Vector &lPos);
  int  GetClosestIndex(const NTempest::C3Vector &listener);

  NTempest::C3Vector pos[8];
  int                posInUseFlags;
  int                soundID;
  Sound             *sound;
  int                currentIndex;
};

static LOOPEDDOODADDESC s_doodadLoopedInfo[8];
static int              s_elapsed;

static LOOPEDDOODADDESC *FindFreeDoodadLoop(int soundID, int &freeSlot, int &soundIndex) {
  LOOPEDDOODADDESC *freeDoodadLoop = 0;
  int               freeIndex = 0;

  for (int index = 0; index < 8; ++index) {
    LOOPEDDOODADDESC *doodadLoop = &s_doodadLoopedInfo[index];

    if (doodadLoop->soundID == -1) {
      freeDoodadLoop = doodadLoop;
      freeIndex = index;
    }
    if (doodadLoop->soundID == static_cast<int>(soundID) && static_cast<unsigned char>(doodadLoop->posInUseFlags) != 0xFF) {
      freeSlot = doodadLoop->FindFreeSlot();
      soundIndex = index;
      return doodadLoop;
    }
  }

  if (freeDoodadLoop) {
    freeDoodadLoop->posInUseFlags = 0;
    freeSlot = 0;
    soundIndex = freeIndex;
    freeDoodadLoop->soundID = soundID;
    return freeDoodadLoop;
  }

  freeSlot = 0;
  soundIndex = 0;
  return 0;
}

int DoodadLoopHandler(const void* dataPtr, void* param) {
  s_elapsed += static_cast<int>(*static_cast<const float *>(dataPtr) * 1000.0f);
  NTempest::C3Vector listener(0.0f);
  Sound::GetListenerPosition(listener);
  for (unsigned int i = 0; i < 8; ++i) {
    s_doodadLoopedInfo[i].Update(listener);
  }
  return 1;
}

void SoundInterfaceDoodadInitialize() {
  EventRegister(EVENT_ID_IDLE, DoodadLoopHandler);
}

void SoundInterfaceDoodadDestroy() {
  EventUnregister(EVENT_ID_IDLE, DoodadLoopHandler);
  for (unsigned int i = 0; i < 8; ++i) {
    Sound::KillSound(s_doodadLoopedInfo[i].sound);
    s_doodadLoopedInfo[i].soundID = -1;
  }
}

void LOOPEDDOODADDESC::Update(const NTempest::C3Vector &listener) {
  if (!posInUseFlags) {
    soundID = -1;
  }
  if (soundID == -1) {
    Sound::KillSound(sound);
    return;
  }

  int closestIndex = GetClosestIndex(listener);
  ASSERT(closestIndex >= 0 && closestIndex < 8);

  if (sound && (sound->IsOutOfRange() || sound->IsPlaying())) {
    if (currentIndex != closestIndex) {
      sound->SetPosition(pos[closestIndex], 0);
      currentIndex = closestIndex;
    }
    return;
  }

  SOUNDDEFINITION *definition = ISndInterfaceGetSndEntry(soundID);
  if (!definition) {
    return;
  }

  const char *filename = definition->GetRandomFileName(-1);
  if (filename && *filename) {
    if (!sound) {
      sound = Sound::Play3DLooped(
          SOUNDCATEGORY_NONE,
          filename,
          definition->GetOsFlags() | 4,
          0,
          true
      );
    }
    if (sound) {
      definition->SetFrequencyAndVolume(sound, 1.0f, false);
      definition->Set3DParams(sound, &pos[closestIndex]);
      if (!sound->SetPaused(false)) {
        Sound::KillSound(sound);
      }
    }
  }
  currentIndex = closestIndex;
}

int LOOPEDDOODADDESC::GetClosestIndex(const NTempest::C3Vector &listener) {
  int   closestIndex = -1;
  float closest = 0.0f;
  for (int i = 0; i < 8; ++i) {
    if (posInUseFlags & (1 << i)) {
      float x = listener.x - pos[i].x;
      float y = listener.y - pos[i].y;
      float z = listener.z - pos[i].z;
      float distanceSquared = x * x + y * y + z * z;
      if (closestIndex == -1 || distanceSquared < closest) {
        closest = distanceSquared;
        closestIndex = i;
      }
    }
  }
  return closestIndex;
}

int LOOPEDDOODADDESC::FindFreeSlot() const {
  for (int slot = 0; slot < 8; ++slot) {
    if (!((1 << slot) & posInUseFlags)) {
      return slot;
    }
  }

  ASSERT(false);
  return 0;
}

int SndInterfaceHandleDoodadLoopStart(unsigned int soundID, const NTempest::C3Vector &pos) {
  int freeSlot;
  int soundIndex;

  if (!soundID) {
    return 0;
  }

  LOOPEDDOODADDESC *doodadLoop = FindFreeDoodadLoop(static_cast<int>(soundID), freeSlot, soundIndex);
  if (!doodadLoop) {
    return 0;
  }

  doodadLoop->posInUseFlags |= 1 << freeSlot;
  doodadLoop->pos[freeSlot] = pos;

  return (soundIndex << 16) | (freeSlot & 0xFF);
}

void SndInterfaceHandleDoodadLoopStop(unsigned int soundHandle) {
  if (soundHandle) {
    s_doodadLoopedInfo[soundHandle >> 16].posInUseFlags &= ~(1 << static_cast<unsigned char>(soundHandle));
  }
}

void SndInterfaceHandleDoodadOneShot(unsigned int soundID, const NTempest::C3Vector &position) {
  if (soundID) {
    NTempest::C3Vector adjustedPosition(position.x, position.y, position.z + 2.0f);
    SndInterfacePlaySound(soundID, adjustedPosition, -1, 1.0f);
  }
}
