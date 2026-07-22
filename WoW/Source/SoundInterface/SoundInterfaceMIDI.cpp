#include "SoundInterface.h"

#include "Console/ConsoleCommand.h"
#include "Console/ConsoleVar.h"
#include "DB/DBClient/AutoCode/AreaMIDIAmbiencesRec.h"

static AreaMIDIAmbiencesRec *s_ambienceRecNormal;
static AreaMIDIAmbiencesRec *s_ambienceRecUnderwater;
static unsigned int          s_paused;
static float                 s_volume = 1.0f;

static bool __fastcall AmbienceVolumeHandler(CVar *cvar, const char *oldValue, const char *newValue, void *userArg) {
  s_volume = SStrToFloat(newValue);
  Sound::MIDI_SetVolume(s_volume);
  SndInterfaceWaterUpdateVolume(s_volume);

  CVar *masterSoundEffects = CVar::Lookup("MasterSoundEffects");
  CVar *enableAmbience = CVar::Lookup("EnableAmbience");
  bool  paused = !masterSoundEffects || !masterSoundEffects->m_intValue || !enableAmbience || !enableAmbience->m_intValue || s_volume == 0.0f;
  SndInterfaceWaterSetPaused(paused);
  SndInterfaceMIDISetPaused(paused);
  return true;
}

static bool __fastcall EnableAmbienceHandler(CVar *cvar, const char *oldValue, const char *newValue, void *userArg) {
  unsigned int enabled = SStrToInt(newValue);
  CVar        *masterSoundEffects = CVar::Lookup("MasterSoundEffects");
  if (masterSoundEffects && !masterSoundEffects->m_intValue) {
    enabled = 0;
  }

  SndInterfaceMIDISetPaused(!enabled);
  SndInterfaceWaterSetPaused(!enabled);
  return true;
}

void __fastcall SoundInterfaceInitializeWorldMIDICVars() {
  CVar::Register("AmbienceVolume", "ambience volume (0.0 to 1.0)", 0, "1.0", AmbienceVolumeHandler, SOUND, false, 0);
  CVar::Register("EnableAmbience", "MIDI ambience", 0, "1", EnableAmbienceHandler, SOUND, false, 0);
}

static void __fastcall StartAmbience() {
  Sound::MIDI_Stop();

  AreaMIDIAmbiencesRec *ambienceRec = g_underWater ? s_ambienceRecUnderwater : s_ambienceRecNormal;
  CVar                 *enableAmbience = CVar::Lookup("EnableAmbience");
  if (ambienceRec && enableAmbience && enableAmbience->m_intValue) {
    const char *sequence = s_paused ? ambienceRec->m_NightSequence : ambienceRec->m_DaySequence;
    Sound::MIDI_Play(sequence, ambienceRec->m_DLSFile);
    Sound::MIDI_SetVolume(s_volume);
  }
}

void __fastcall SndInterfaceSetMIDIArea(int normal, int underwater) {
  AreaMIDIAmbiencesRec *normalRec = g_areaMIDIAmbiencesDB.GetRecord(normal);
  AreaMIDIAmbiencesRec *underwaterRec = g_areaMIDIAmbiencesDB.GetRecord(underwater);

  if (s_paused) {
    if (s_paused != 1 || underwaterRec == s_ambienceRecUnderwater) {
      return;
    }
  } else if (normalRec == s_ambienceRecNormal) {
    return;
  }

  s_ambienceRecNormal = normalRec;
  s_ambienceRecUnderwater = underwaterRec;
  StartAmbience();
}

void __fastcall SndInterfaceMIDIUnderwaterChanged() {
  StartAmbience();
}

void __fastcall SndInterfaceMIDISetPaused(unsigned int paused) {
  s_paused = paused;
  if (paused) {
    Sound::MIDI_Stop();
  } else {
    StartAmbience();
  }
}
