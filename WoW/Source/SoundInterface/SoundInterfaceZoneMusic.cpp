#include "SoundInterface.h"

#include "DB/DBClient/AutoCode/ZoneMusicRec.h"
#include "Os/W32/OsSound.h"

static int           s_flags;
static ZoneMusicRec *s_currentMusic;
static Sound        *s_sound;

static int GetNextPlayTime() {
    // TODO: implement
    return 0;
}

static void PlayMusic() {
    // TODO: implement
}

static int ZoneMusicIdle(const void* dataPtr, void* ptr) {
    // TODO: implement
    return 0;
}

void __fastcall InitializeZoneMusic() {
    // TODO: implement
}

void __fastcall ShutdownZoneMusic() {
    // TODO: implement
}

void __fastcall SndInterfaceRegisterNewZone(unsigned int musicID) {
  ZoneMusicRec *previousMusic = s_currentMusic;
  s_currentMusic = g_zoneMusicDB.GetRecord(musicID);

  if (previousMusic != s_currentMusic && s_currentMusic && s_sound) {
    s_flags |= 4;
    s_sound->Stop(4.0f);
  }
}

void __fastcall SndInterfacePauseZoneMusic(int pause) {
  if (pause) {
    SndInterfaceZoneIntroStop();
    Sound::KillSound(s_sound);
    s_flags |= 1;
  } else {
    s_flags &= ~1;
  }
}

int __fastcall SndInterfaceIsZoneMusicPaused() {
  return s_flags & 1;
}
