#include "SoundInterface.h"

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

static LOOPEDDOODADDESC *__fastcall FindFreeDoodadLoop(int soundID, int &freeSlot, int &soundIndex) {
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

int LOOPEDDOODADDESC::FindFreeSlot() const {
  for (int slot = 0; slot < 8; ++slot) {
    if (!((1 << slot) & posInUseFlags)) {
      return slot;
    }
  }

  ASSERT(false);
  return 0;
}

int __fastcall SndInterfaceHandleDoodadLoopStart(unsigned int soundID, const NTempest::C3Vector &pos) {
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

void __fastcall SndInterfaceHandleDoodadLoopStop(unsigned int soundHandle) {
  if (soundHandle) {
    s_doodadLoopedInfo[soundHandle >> 16].posInUseFlags &= ~(1 << static_cast<unsigned char>(soundHandle));
  }
}

void __fastcall SndInterfaceHandleDoodadOneShot(unsigned int soundID, const NTempest::C3Vector &position) {
  if (soundID) {
    NTempest::C3Vector adjustedPosition(position.x, position.y, position.z + 2.0f);
    SndInterfacePlaySound(soundID, adjustedPosition, -1, 1.0f);
  }
}
