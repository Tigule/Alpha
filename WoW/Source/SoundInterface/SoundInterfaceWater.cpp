#include "SoundInterface/SoundInterface.h"

#include <Os/W32/OsSound.h>
#include <Tempest/c3vector.h>

class SoundEntriesRec;
struct CVar;

struct LIQUIDINFO {
  Sound             *m_sound;
  unsigned int       m_subTypes[3];
  SoundEntriesRec   *m_soundRecords[3];
  NTempest::C3Vector m_positionOffset[3];
  SoundEntriesRec   *m_currentRecord;
  unsigned int       m_currentPlayingSound;

  void StopSound(int immediate);
};

static LIQUIDINFO s_liquidInfo[4];
static int        s_flags;
static float      s_volume = 1.0f;
static int        s_paused;

void LIQUIDINFO::StopSound(int immediate) {
  if (m_sound) {
    m_sound->Stop(immediate ? 0.0f : 5.0f);
    m_sound = 0;
  }
}

static void __fastcall ClearAllSounds(int immediate) {
  for (unsigned int i = 0; i < 4; ++i) {
    s_liquidInfo[i].StopSound(immediate);
  }
}

static unsigned char ToggleCallback(CVar* h, const char* oldValue, const char* newValue, void* arg) {
    // TODO: implement
    return 0;
}

static void HandleWaterAmbiences() {
    // TODO: implement
}

static int WaterHandler(const void* dataPtr, void* param) {
    // TODO: implement
    return 0;
}

void __fastcall InitializeWaterAmbiences() {
    // TODO: implement
}

void __fastcall ShutdownWaterAmbiences() {
    // TODO: implement
}

void __fastcall WaterAmbiencesUnderwaterChanged() {
  if (g_underWater) {
    ClearAllSounds(1);
  } else {
    s_flags |= 2;
  }
}

void __fastcall SndInterfaceWaterSetPaused(unsigned int p) {
  if (p && p != static_cast<unsigned int>(s_paused)) {
    ClearAllSounds(1);
  }
  s_paused = p != 0;
}

void __fastcall SndInterfaceWaterUpdateVolume(float volume) {
  s_volume = volume;
}
