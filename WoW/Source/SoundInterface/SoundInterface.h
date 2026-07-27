#ifndef WOW_SOURCE_SOUNDINTERFACE_SOUNDINTERFACE_H
#define WOW_SOURCE_SOUNDINTERFACE_SOUNDINTERFACE_H

#include "Os/W32/OsSound.h"

#include <stpl.h>

struct FOOTSTEPSNDCACHE : public TSHashObject<FOOTSTEPSNDCACHE, HASHKEY_NONE> {
  TSGrowableArray<unsigned int> m_soundIDs;
  TSGrowableArray<unsigned int> m_splashSoundIDs;
};

class CGItem_C;
class CGObject_C;
class CGPlayer_C;
class CGUnit_C;
struct VirtualItemInfo;

enum VOCALUISOUNDS {
  VOCALUISOUND_NONE = 0
};

enum VOCALUISOUNDTYPE {
  VUISOUNDTYPE_NORMAL = 0,
  VUISOUNDTYPE_PISSED = 1,
  NUM_SOUNDTYPES = 2
};

struct VOCALUISOUND {
  void Clear() {
    soundTypes[0] = 0;
    soundTypes[1] = 0;
    pissedCount = 0;
  }

  unsigned int soundTypes[NUM_SOUNDTYPES];
  unsigned int pissedCount;
};

enum PARRYMATERIALS {
  PARRYMATERIAL_WOOD = 0,
  PARRYMATERIAL_METAL = 1,
  NUM_PARRYMATERIALS = 2
};

enum ITEMSOUNDTYPE {
  ITEMSOUND_PICKUP = 0,
  ITEMSOUND_DROP = 1,
  ITEMSOUND_USE = 2,
  ITEMSOUND_CLOSE = 3,
  NUM_ITEMSOUNDS = 4
};

enum AMBIENCE {
  AMB_DAY = 0,
  AMB_NIGHT = 1,
  NUM_AMBIENCES = 2
};

extern unsigned int g_sndInterfaceFlags;
extern bool         g_underWater;
extern AMBIENCE     g_currentAmbience;

void SndInterfaceInitialize();
void SndInterfaceDestroy();
void SndInterfaceWorldInitialize();
void SndInterfaceWorldDestroy();
void SoundInterfaceRegisterWorldCVars();
void SoundInterfaceInitializeWorldMIDICVars();
void SndInterfaceMIDISetPaused(bool paused);
void SndInterfaceWaterSetPaused(bool p);
void SndInterfaceWaterUpdateVolume(float volume);
void SndInterfaceSetUnderwater(bool underwater);
void SndInterfaceMIDIUnderwaterChanged();
void WaterAmbiencesUnderwaterChanged();
void SndDebugInitialize();
void SndDebugShutdown();
void IndoorsShutdown();
void OutdoorsShutdown();
void InitializeGlueMusic();
void ShutdownGlueMusic();
void ProviderPrefInitialize();
void ProviderPrefShutdown();
void SndInterfaceSetProviderPrefs(unsigned int index, unsigned int indexUnderwater, unsigned int transitionDuration);
void SndInterfaceSetProviderPrefs(const _FSOUND_REVERB_PROPERTIES &pref, const _FSOUND_REVERB_PROPERTIES &prefUnderwater);
void SndInterfaceClearProviderPrefs(int indoors);
void SndInterfaceProviderPrefsUnderwaterChanged();
void SndSetRoomType(SNDROOMTYPE roomType);
void ISndInterfaceInitialize();
void ISndInterfaceShutdown();
void SndInterfacePauseZoneMusic(int pause);
int SndInterfaceIsZoneMusicPaused();
void SndInterfaceZoneIntroStop();
void SndDebugDungeonTransition(int indoors, unsigned int continent);
void SndInterfaceRegisterNewZone(unsigned int musicID);
void SndInterfaceSetMIDIArea(int normal, int underwater);
void SndInterfaceClearMIDI();
void SndInterfaceRegisterNewZoneIntro(int soundID, int priority);
void SndInterfacePlayInterfaceSound(const char *name);
void SndInterfacePlaySpellSound(int soundID, CGUnit_C *obj);
unsigned int SndInterfaceGetSoundVariations(unsigned int soundID);
void SndInterfacePlaySpellFizzleSound(unsigned int spellID, const CGUnit_C *caster);
void
SndInterfacePlayParrySound(const VirtualItemInfo *attackingWeapon, const VirtualItemInfo *defendingItem, int criticalHit, const NTempest::C3Vector &position);
void
SndInterfacePlayHitSound(const VirtualItemInfo *attackingWeapon, unsigned int defendingItemType, int criticalHit, const NTempest::C3Vector &position);
void SndInterfacePlaySheatheSound(
    const VirtualItemInfo *info,
    int sheathing,
    const NTempest::C3Vector &position
);
void SndInterfacePlayDeflectedSound(NTempest::C3Vector &position);
void SndInterfacePlayImmuneSound(NTempest::C3Vector &pos);
void SndInterfacePlayAbsorbedSound(NTempest::C3Vector &pos);
void SndInterfaceInitializeVocalUISounds(unsigned int race, unsigned int sex);
void SndInterfacePlayVocalUISound(VOCALUISOUNDS soundType);
void SoundInterfacePlayVocalMacro(const CGPlayer_C *player, int category);
void SndInterfacePlayItemSound(ITEMSOUNDTYPE soundType, const CGItem_C *itemPtr);
void SndInterfacePlayItemSound(ITEMSOUNDTYPE soundType, int itemDisplayID);
bool SndInterfacePlaySound(unsigned int soundID, int forceIndex);
bool SndInterfacePlaySound(unsigned int soundID, const NTempest::C3Vector &position, int forceIndex, float volumeScaler);
bool SoundInterfaceIsSoundLooping(unsigned int soundID, bool &looping);
bool SndInterfacePlaySplashSound(unsigned int soundID, const NTempest::C3Vector &position);
void SndInterfacePlayFootstepSound(
    unsigned int footstepID,
    const NTempest::C3Vector &position,
    unsigned int terrainID,
    int splashing
);
void SndInterfacePlayFoleySound(unsigned int materialID, const NTempest::C3Vector &position);
Sound *SndInterfacePlayLoopedSound(unsigned int soundID, unsigned int loopCount);
Sound *SndInterfacePlayLoopedSound(unsigned int soundID, const NTempest::C3Vector &position, unsigned int loopCount);
Sound *SndInterfaceCreateSound(unsigned int soundID, float fadeInRate, int forceIndex, bool doNotKeepAlive);
void SndInterfaceAssociateSoundWithObject(Sound *sound, CGObject_C *objectPtr);
int SndInterfaceHandleDoodadLoopStart(unsigned int soundID, const NTempest::C3Vector &pos);
void SndInterfaceHandleDoodadLoopStop(unsigned int soundHandle);
void SndInterfaceHandleDoodadOneShot(unsigned int soundID, const NTempest::C3Vector &position);
void SndInterfaceSetGlueMusic(const char *musicFile);
void SndInterfaceStopGlueMusic(float fadeTime);
void SoundRegisterScriptFunctions();
void SoundUnregisterScriptFunctions();
void SndInterfaceSetPositionCallback();
void SndInterfaceClearPositionCallback();

#endif
