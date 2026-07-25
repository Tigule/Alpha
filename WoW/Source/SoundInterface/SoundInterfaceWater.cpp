#include "SoundInterface/SoundInterface.h"
#include "SoundInterface/ISoundInterface.h"

#include "Console/ConsoleVar.h"
#include "Console/ConsoleCommand.h"
#include "DB/DBClient/AutoCode/SoundEntriesRec.h"
#include "DB/DBClient/AutoCode/SoundWaterTypeRec.h"
#include "Event/EvtApi.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "Object/ObjectClient/Object_C.h"
#include "WorldClient/World.h"

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
  int  Update(const NTempest::C3Vector &listenerPos);
  void UpdateVolume();
  void Tick();
  void StartSound(unsigned int subType, const NTempest::C3Vector &listenerPos);
};

static LIQUIDINFO s_liquidInfo[4];
static int        s_flags;
static float      s_volume = 1.0f;
static int        s_paused;
static int        s_elapsed;
static CVar      *s_cvar;
static const float PANNING_DIST_SQUARED = 81.0f;

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

static bool __fastcall ToggleCallback(CVar* h, const char* oldValue, const char* newValue, void* arg) {
  if (!SStrToInt(newValue)) {
    ClearAllSounds(0);
  }
  return 1;
}

static void HandleWaterAmbiences() {
  if (!(s_flags & 1) || !s_cvar || !s_cvar->GetInt() || g_underWater) {
    return;
  }

  CGObject_C *player =
      ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__);
  if (!player) {
    return;
  }

  int                 liquidResults[9];
  NTempest::C3Vector  distanceResults[9];
  memset(distanceResults, 0, sizeof(distanceResults));
  CWorld::QueryLiquidSounds(player->GetWorldObject(), 9.0f, liquidResults, distanceResults);

  for (unsigned int liquidType = 0; liquidType < 4; ++liquidType) {
    memset(s_liquidInfo[liquidType].m_subTypes, 0, sizeof(s_liquidInfo[liquidType].m_subTypes));
  }
  for (unsigned int i = 0; i < 9; ++i) {
    if (liquidResults[i]) {
      unsigned int subType = (i >> 2) & 3;
      FATALASSERT(subType < 3);
      ++s_liquidInfo[i & 3].m_subTypes[subType];
      s_liquidInfo[i & 3].m_positionOffset[subType] = distanceResults[i];
      s_liquidInfo[i & 3].m_positionOffset[subType].z = 0.0f;
    }
  }

  NTempest::C3Vector listenerPos(0.0f);
  Sound::GetListenerPosition(listenerPos);
  unsigned int playing = 0;
  for (unsigned int liquidInfoIndex = 0; liquidInfoIndex < 4; ++liquidInfoIndex) {
    if (playing < 2 && s_liquidInfo[liquidInfoIndex].Update(listenerPos)) {
      ++playing;
    } else if (playing >= 2) {
      s_liquidInfo[liquidInfoIndex].StopSound(0);
    }
  }
  s_flags &= ~2;
}

static int __fastcall WaterHandler(const void* dataPtr, void* param) {
  HandleWaterAmbiences();
  return 1;
}

void LIQUIDINFO::StartSound(unsigned int subType, const NTempest::C3Vector &listenerPos) {
  FATALASSERT(subType < 3);
  NTempest::C3Vector position = listenerPos + m_positionOffset[subType];
  if (m_sound) {
    if (m_sound->IsPlaying()) {
      m_sound->SetPosition(position, 0);
    }
    return;
  }
  if (s_paused > 0) {
    return;
  }

  m_currentRecord = m_soundRecords[subType];
  if (!m_currentRecord) {
    return;
  }
  SOUNDDEFINITION *definition = ISndInterfaceGetSndEntry(m_currentRecord->GetID());
  if (!definition) {
    return;
  }
  const char *filename = definition->GetRandomFileName(-1);
  if (!filename || !*filename) {
    return;
  }

  bool startPaused = !(s_flags & 2);
  m_sound = Sound::Play3DLooped(static_cast<SOUNDCATEGORIES>(4), filename, 0, 0, startPaused);
  if (m_sound) {
    definition->SetFrequencyAndVolume(m_sound, 1.0f, false);
    definition->Set3DParams(m_sound, &position);
    if (startPaused) {
      m_sound->SetFadeIn(5.0f, 1.0f);
      if (!m_sound->SetPaused(false)) {
        Sound::KillSound(m_sound);
      }
    }
    m_currentPlayingSound = subType;
  }
}

int LIQUIDINFO::Update(const NTempest::C3Vector &listenerPos) {
  unsigned int subType;
  for (subType = 0; subType < 3 && !m_subTypes[subType]; ++subType) {
  }
  if (subType >= 3) {
    StopSound(0);
    return 0;
  }
  if (m_sound && m_currentPlayingSound != subType) {
    StopSound(0);
  }
  StartSound(subType, listenerPos);
  Tick();
  return 1;
}

void LIQUIDINFO::UpdateVolume() {
  if (m_sound && m_currentRecord) {
    SOUNDDEFINITION *definition = ISndInterfaceGetSndEntry(m_currentRecord->GetID());
    if (definition) {
      definition->SetFrequencyAndVolume(m_sound, 1.0f, false);
      definition->Set3DParams(m_sound, 0);
    }
  }
}

void LIQUIDINFO::Tick() {
  if (m_sound && m_currentRecord) {
    FATALASSERT(m_currentPlayingSound < 3);
    float distanceSquared = m_positionOffset[m_currentPlayingSound].SquaredMag();
    m_sound->SetPanning(1.0f - distanceSquared / PANNING_DIST_SQUARED);
  }
}

void __fastcall InitializeWaterAmbiences() {
  for (int i = g_soundWaterTypeDB.GetNumRecords() - 1; i >= 0; --i) {
    SoundWaterTypeRec *record = g_soundWaterTypeDB.GetRecordByIndex(i);
    if (record && record->m_soundType < 4) {
      unsigned int soundSubtype = (record->m_soundSubtype >> 2) & 3;
      FATALASSERT(soundSubtype < 3);
      s_liquidInfo[record->m_soundType].m_soundRecords[soundSubtype] =
          g_soundEntriesDB.GetRecord(record->m_SoundID);
    }
  }
  s_flags |= 1;
  s_cvar = CVar::Register("MapWaterSounds", "", 0, "1", ToggleCallback, DEFAULT, false, 0);
  EventRegister(EVENT_ID_IDLE, WaterHandler);
  s_elapsed = 0;
}

void __fastcall ShutdownWaterAmbiences() {
  s_paused = -1;
  ClearAllSounds(1);
  s_flags = 0;
  EventUnregister(EVENT_ID_IDLE, WaterHandler);
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
    for (unsigned int i = 0; i < 4; ++i) {
      if (s_liquidInfo[i].m_sound) {
        Sound::KillSound(s_liquidInfo[i].m_sound);
        s_liquidInfo[i].m_sound = 0;
      }
    }
  }
  s_paused = p != 0;
}

void __fastcall SndInterfaceWaterUpdateVolume(float volume) {
  s_volume = volume;
  for (unsigned int i = 0; i < 4; ++i) {
    s_liquidInfo[i].UpdateVolume();
  }
}
