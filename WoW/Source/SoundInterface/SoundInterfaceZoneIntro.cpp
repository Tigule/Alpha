#include "SoundInterface.h"

#include "SoundInterface/ISoundInterface.h"

#include <windows.h>

static int    s_lastPlayTime = -1;
static Sound *s_sound;
static int    s_priority = -1;

void __fastcall SndInterfaceRegisterNewZoneIntro(int soundID, int priority) {
  if (soundID && !SndInterfaceIsZoneMusicPaused()) {
    if (!s_sound || !s_sound->IsPlaying() || priority > s_priority) {
      Sound::KillSound(s_sound);

      int currentTime = GetTickCount();
      if (s_lastPlayTime == -1 || s_lastPlayTime + 3600000 <= currentTime) {
        SOUNDDEFINITION *definition = ISndInterfaceGetSndEntry(soundID);
        if (definition) {
          const char *filename = definition->GetRandomFileName(-1);
          if (filename && *filename) {
            s_sound = Sound::Play2D(static_cast<SOUNDCATEGORIES>(6), filename, 1, true);
            if (s_sound) {
              s_priority = priority;
              s_lastPlayTime = GetTickCount();
              definition->SetFrequencyAndVolume(s_sound, 1.0f, false);
              if (!s_sound->SetPaused(false)) {
                Sound::KillSound(s_sound);
              }
            }
          }
        }
      }
    }
  } else if (s_sound) {
    s_sound->Stop(3.0f);
  }
}

void __fastcall SndInterfaceZoneIntroStop() {
  Sound::KillSound(s_sound);
}
