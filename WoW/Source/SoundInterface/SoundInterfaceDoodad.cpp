#include <Base/Base.h>
#include "SoundInterface.h"
#include <Gx/Gx.h>
#include <WowConst.h>
#include <Gx/CGxDevice.h>
#include "Object/ObjectClient/Unit_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "ISoundInterface.h"

#include <Event/EvtApi.h>

struct LOOPEDDOODADDESC {
  NTempest::C3Vector pos[8];
  int                posInUseFlags;
  int                soundID;
  Sound             *sound;
  int                currentIndex;

  LOOPEDDOODADDESC() : posInUseFlags(0), soundID(-1), sound(0), currentIndex(0) {
  }

  int  FindFreeSlot() const;
  void Update(const NTempest::C3Vector &lPos);
  int  GetClosestIndex(const NTempest::C3Vector &listener);
};

static LOOPEDDOODADDESC s_doodadLoopedInfo[8];
static int              s_elapsed;

static LOOPEDDOODADDESC *FindFreeDoodadLoop(int soundID, int &freeSlot, int &soundIndex) {
  int freeIndex = -1;
  for (int index = 0; index < sizeof(s_doodadLoopedInfo) / sizeof(s_doodadLoopedInfo[0]); ++index) {
    if (s_doodadLoopedInfo[index].soundID == -1) {
      freeIndex = index;
    }
    if (s_doodadLoopedInfo[index].soundID == soundID) {
      if ((BYTE)s_doodadLoopedInfo[index].posInUseFlags != 0xFF) {
        freeSlot = s_doodadLoopedInfo[index].FindFreeSlot();
        soundIndex = index;
        return &s_doodadLoopedInfo[index];
      }
    }
  }

  if (freeIndex != -1) {
    s_doodadLoopedInfo[freeIndex].posInUseFlags = 0;
    freeSlot = 0;
    soundIndex = freeIndex;
    s_doodadLoopedInfo[freeIndex].soundID = soundID;
    return &s_doodadLoopedInfo[freeIndex];
  }

  freeSlot = 0;
  soundIndex = 0;
  return 0;
}

BOOL DoodadLoopHandler(LPCVOID dataPtr, LPVOID param) {
  s_elapsed += (int)(*(const float *)dataPtr * 1000.0f);
  NTempest::C3Vector lPos(0.0f);
  Sound::GetListenerPosition(lPos);
  for (UINT i = 0; i < sizeof(s_doodadLoopedInfo) / sizeof(s_doodadLoopedInfo[0]); ++i) {
    s_doodadLoopedInfo[i].Update(lPos);
  }
  return 1;
}

void SoundInterfaceDoodadInitialize() {
  EventRegister(EVENT_ID_IDLE, DoodadLoopHandler);
}

void SoundInterfaceDoodadDestroy() {
  EventUnregister(EVENT_ID_IDLE, DoodadLoopHandler);
  for (UINT i = 0; i < sizeof(s_doodadLoopedInfo) / sizeof(s_doodadLoopedInfo[0]); ++i) {
    Sound::KillSound(s_doodadLoopedInfo[i].sound);
    s_doodadLoopedInfo[i].soundID = -1;
  }
}

void LOOPEDDOODADDESC::Update(const NTempest::C3Vector &lPos) {
  if (!posInUseFlags) {
    soundID = -1;
  }
  if (soundID == -1) {
    Sound::KillSound(sound);
    return;
  }

  int closestIndex = GetClosestIndex(lPos);
  ASSERT(( closestIndex >= 0 ) && ( closestIndex < 8 ));

  if (!sound || (!sound->IsOutOfRange() && !sound->IsPlaying())) {
    SOUNDDEFINITION *desc = ISndInterfaceGetSndEntry(soundID);
    if (!desc) {
      return;
    }

    LPCSTR filename = desc->GetRandomFileName(-1);
    if (filename && *filename) {
      if (!sound) {
        sound = Sound::Play3DLooped(SOUNDCATEGORY_NONE, filename, 0, 0, true);
      }
      if (sound) {
        desc->SetFrequencyAndVolume(sound, 1.0f, false);
        desc->Set3DParams(sound, &pos[closestIndex]);
        if (!sound->SetPaused(false)) {
          Sound::KillSound(sound);
        }
      }
    }
  } else if (sound && currentIndex != closestIndex) {
    sound->SetPosition(pos[closestIndex], 0);
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

BOOL LOOPEDDOODADDESC::FindFreeSlot() const {
  for (int slot = 0; slot < 8; ++slot) {
    if (!((1 << slot) & posInUseFlags)) {
      return slot;
    }
  }

  ASSERT(false);
  return 0;
}

int SndInterfaceHandleDoodadLoopStart(UINT soundID, const NTempest::C3Vector &pos) {
  int freeSlot;
  int soundIndex;

  if (!soundID) {
    return 0;
  }

  LOOPEDDOODADDESC *doodadLoop = FindFreeDoodadLoop(soundID, freeSlot, soundIndex);
  if (!doodadLoop) {
    return 0;
  }

  doodadLoop->posInUseFlags |= 1 << freeSlot;
  doodadLoop->pos[freeSlot] = pos;

  return (soundIndex << 16) | (freeSlot & 0xFF);
}

void SndInterfaceHandleDoodadLoopStop(UINT soundHandle) {
  if (soundHandle) {
    int slot = soundHandle & 0xFF;
    int index = soundHandle >> 16;

    if (index >= 0 || index < 8 || slot >= 0 || slot < 8) {
      s_doodadLoopedInfo[index].posInUseFlags &= ~(1 << slot);
    }
  }
}

void SndInterfaceHandleDoodadOneShot(UINT soundID, const NTempest::C3Vector &position) {
  if (soundID) {
    SndInterfacePlaySound(soundID, NTempest::C3Vector(position.x, position.y, position.z + 2.0f), -1, 1.0f);
  }
}
