#include <Base/Base.h>
#include <WowConst.h>

#include "SoundInterface.h"

#include "DB/DBClient/AutoCode/ZoneMusicRec.h"
#include "Event/EvtApi.h"
#include "Os/W32/OsSound.h"
#include "Os/OsTime.h"
#include "SoundInterface/ISoundInterface.h"
#include "Tempest/crandom.h"

static int                 s_flags;
static const ZoneMusicRec *s_currentMusic;
static Sound              *s_sound;
static int                 s_elapsed;
static int                 s_nextPlay = -1;
static NTempest::CRndSeed  s_rndSeed;

static int GetNextPlayTime() {
  if (s_flags & 4) {
    s_flags &= ~4;
    return OsGetAsyncTimeMs();
  }

  int minimum = s_currentMusic->m_SilenceIntervalMin[g_currentAmbience];
  int range = s_currentMusic->m_SilenceIntervalMax[g_currentAmbience] - minimum;
  if (range < 1) {
    range = 1;
  }
  UINT random = NTempest::CRandom::uint32_(s_rndSeed);
  return OsGetAsyncTimeMs() + minimum + static_cast<UINT>((static_cast<DWORDLONG>(random) * static_cast<UINT>(range)) >> 32);
}

static void PlayMusic() {
  UINT soundID = s_currentMusic->m_Sounds[g_currentAmbience];
  if (!soundID) {
    return;
  }

  SOUNDDEFINITION *definition = ISndInterfaceGetSndEntry(soundID);
  if (!definition) {
    return;
  }

  Sound::KillSound(s_sound);
  LPCSTR filename = definition->GetRandomFileName(-1);
  if (filename && *filename) {
    s_sound = Sound::Play2D(SOUNDCATEGORY_NONE, filename, 6, true);
  }
  if (s_sound) {
    if (s_sound->SetPaused(false)) {
      s_sound->SetVolume(definition->m_volume);
    } else {
      Sound::KillSound(s_sound);
    }
  }
}

static BOOL ZoneMusicIdle(LPCVOID dataPtr, LPVOID ptr) {
  const EVENT_DATA_IDLE *data = static_cast<const EVENT_DATA_IDLE *>(dataPtr);
  if (!(s_flags & 1) && s_currentMusic && (s_currentMusic->m_Sounds[0] || s_currentMusic->m_Sounds[1])) {
    if (s_sound && !s_sound->IsPlaying()) {
      s_nextPlay = GetNextPlayTime();
      Sound::KillSound(s_sound);
    }
    if (!s_sound && (s_nextPlay == -1 || data->time > static_cast<UINT>(s_nextPlay))) {
      PlayMusic();
    }
  }
  return 1;
}

void InitializeZoneMusic() {
  s_flags |= 2;
  EventRegister(EVENT_ID_IDLE, ZoneMusicIdle);
  s_elapsed = 0;
  s_nextPlay = -1;
}

void ShutdownZoneMusic() {
  EventUnregister(EVENT_ID_IDLE, ZoneMusicIdle);
  s_flags &= ~2;
  s_currentMusic = 0;
  Sound::KillSound(s_sound);
}

void SndInterfaceRegisterNewZone(UINT musicID) {
  const ZoneMusicRec *previousMusic = s_currentMusic;
  s_currentMusic = g_zoneMusicDB.GetRecord(musicID);

  if (previousMusic != s_currentMusic && s_currentMusic && s_sound) {
    s_flags |= 4;
    s_sound->Stop(4.0f);
  }
}

void SndInterfacePauseZoneMusic(int pause) {
  if (pause) {
    SndInterfaceZoneIntroStop();
    Sound::KillSound(s_sound);
    s_flags |= 1;
  } else {
    s_flags &= ~1;
  }
}

int SndInterfaceIsZoneMusicPaused() {
  return s_flags & 1;
}
