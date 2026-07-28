#include "SoundInterface.h"

#include "Console/ConsoleVar.h"
#include "Glue/CGlueMgr.h"
#include "Os/W32/OsSound.h"

#include <storm.h>

static const float FADEOUT_TIME = 3.0f;

static char   s_musicFile[0x104];
static Sound *s_glueMusic;

void InitializeGlueMusic() {
}

void ShutdownGlueMusic() {
  SndInterfaceStopGlueMusic(0.0f);
  s_musicFile[0] = 0;
}

void SndInterfaceSetGlueMusic(const char *musicFile) {
  if (!musicFile || !CGlueMgr::Initialized() || CGlueMgr::Suspended()) {
    SndInterfaceStopGlueMusic(FADEOUT_TIME);
    s_musicFile[0] = 0;
    return;
  }

  if (!*musicFile) {
    if (!s_glueMusic) {
      if (!s_musicFile[0]) {
        return;
      }

      s_glueMusic = Sound::Play2DLooped(SOUNDCATEGORY_NONE, s_musicFile, 2, 0, true);
      if (!s_glueMusic) {
        return;
      }
    }

    s_glueMusic->SetVolume(1.0f);
    if (!s_glueMusic->SetPaused(false)) {
      Sound::KillSound(s_glueMusic);
    }
    return;
  }

  if (s_musicFile[0] && !SStrCmp(musicFile, s_musicFile, 0x7FFFFFFF) && s_glueMusic) {
    return;
  }

  SStrCopy(s_musicFile, musicFile, sizeof(s_musicFile));
  SndInterfaceStopGlueMusic(FADEOUT_TIME);

  CVar *enableMusic = CVar::Lookup("EnableMusic");
  if (!enableMusic || !enableMusic->GetInt()) {
    return;
  }

  ASSERT(!s_glueMusic);
  s_glueMusic = Sound::Play2DLooped(SOUNDCATEGORY_NONE, musicFile, 2, 0, true);
  if (!s_glueMusic) {
    return;
  }

  s_glueMusic->SetVolume(1.0f);
  if (!s_glueMusic->SetPaused(false)) {
    Sound::KillSound(s_glueMusic);
  }
}

void SndInterfaceStopGlueMusic(float fadeTime) {
  if (s_glueMusic) {
    s_glueMusic->Stop(fadeTime);
    s_glueMusic = 0;
  }
}
