#include "SoundInterface.h"

#include "Console/ConsoleCommand.h"
#include "Console/ConsoleVar.h"
#include "DB/DBClient/AutoCode/AreaMIDIAmbiencesRec.h"

static const AreaMIDIAmbiencesRec *s_ambienceRecNormal;
static const AreaMIDIAmbiencesRec *s_ambienceRecUnderwater;
static bool                  s_paused;
static float                 s_volume = 1.0f;

static bool AmbienceVolumeHandler(CVar *cvar, const char *oldValue, const char *newValue, void *userArg) {
  s_volume = SStrToFloat(newValue);
  SndInterfaceWaterUpdateVolume(s_volume);
  Sound::MIDI_SetVolume(s_volume);

  CVar *masterSoundEffects = CVar::Lookup("MasterSoundEffects");
  CVar *enableAmbience = CVar::Lookup("EnableAmbience");
  bool  enabled = true;
  if (!masterSoundEffects || !masterSoundEffects->GetInt()) {
    enabled = false;
  }
  if (!enableAmbience || !enableAmbience->GetInt()) {
    enabled = false;
  }
  if (s_volume == 0.0f) {
    enabled = false;
  }
  SndInterfaceWaterSetPaused(!enabled);
  SndInterfaceMIDISetPaused(!enabled);
  return true;
}

static bool EnableAmbienceHandler(CVar *cvar, const char *oldValue, const char *newValue, void *userArg) {
  unsigned int enabled = SStrToInt(newValue);
  CVar        *masterSoundEffects = CVar::Lookup("MasterSoundEffects");
  if (masterSoundEffects && !masterSoundEffects->GetInt()) {
    enabled = 0;
  }

  SndInterfaceMIDISetPaused(!enabled);
  SndInterfaceWaterSetPaused(!enabled);
  return true;
}

void SoundInterfaceInitializeWorldMIDICVars() {
  CVar::Register("AmbienceVolume", "ambience volume (0.0 to 1.0)", 0, "1.0", AmbienceVolumeHandler, SOUND, false, 0);
  CVar::Register("EnableAmbience", "MIDI ambience", 0, "1", EnableAmbienceHandler, SOUND, false, 0);
}

void SoundInterfaceInitializeWorldMIDI() {
  SoundInterfaceInitializeWorldMIDICVars();
  Sound::MIDI_Initialize();
  s_ambienceRecNormal = 0;
  s_ambienceRecUnderwater = 0;
}

void SoundInterfaceShutdownWorldMIDI() {
  Sound::MIDI_Shutdown();
}

static void StartAmbience() {
  Sound::MIDI_Stop();

  const AreaMIDIAmbiencesRec *ambienceRec = g_underWater ? s_ambienceRecUnderwater : s_ambienceRecNormal;
  CVar                       *enableAmbience = CVar::Lookup("EnableAmbience");
  if (ambienceRec && enableAmbience && enableAmbience->GetInt()) {
    const char *sequence = g_currentAmbience == AMB_DAY ? ambienceRec->m_DaySequence : ambienceRec->m_NightSequence;
    Sound::MIDI_Play(sequence, ambienceRec->m_DLSFile);
    Sound::MIDI_SetVolume(s_volume);
  }
}

void SndInterfaceSetMIDIArea(int normal, int underwater) {
  const AreaMIDIAmbiencesRec *normalRec = g_areaMIDIAmbiencesDB.GetRecord(normal);
  const AreaMIDIAmbiencesRec *underwaterRec = g_areaMIDIAmbiencesDB.GetRecord(underwater);

  if (g_currentAmbience == AMB_DAY) {
    if (normalRec == s_ambienceRecNormal) {
      return;
    }
  } else if (g_currentAmbience == AMB_NIGHT) {
    if (underwaterRec == s_ambienceRecUnderwater) {
      return;
    }
  } else {
    return;
  }

  s_ambienceRecNormal = normalRec;
  s_ambienceRecUnderwater = underwaterRec;
  StartAmbience();
}

void SndInterfaceClearMIDI() {
  s_ambienceRecNormal = 0;
  s_ambienceRecUnderwater = 0;
  Sound::MIDI_Stop();
}

void SndInterfaceMIDIAmbienceChanged() {
  StartAmbience();
}

void SndInterfaceMIDIUnderwaterChanged() {
  StartAmbience();
}

void SndInterfaceMIDISetPaused(bool paused) {
  s_paused = paused;
  if (paused) {
    Sound::MIDI_Stop();
  } else {
    StartAmbience();
  }
}
