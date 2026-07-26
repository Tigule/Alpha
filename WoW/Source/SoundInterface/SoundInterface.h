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

extern unsigned int g_sndInterfaceFlags;
extern bool         g_underWater;

void __fastcall SndInterfaceInitialize();
void __fastcall SndInterfaceDestroy();
void __fastcall SndInterfaceWorldInitialize();
void __fastcall SndInterfaceWorldDestroy();
void __fastcall SoundInterfaceRegisterWorldCVars();
void __fastcall SoundInterfaceInitializeWorldMIDICVars();
void __fastcall SndInterfaceMIDISetPaused(unsigned int paused);
void __fastcall SndInterfaceWaterSetPaused(unsigned int p);
void __fastcall SndInterfaceWaterUpdateVolume(float volume);
void __fastcall SndInterfaceSetUnderwater(bool underwater);
void __fastcall SndInterfaceMIDIUnderwaterChanged();
void __fastcall WaterAmbiencesUnderwaterChanged();
void __fastcall SndDebugInitialize();
void __fastcall SndDebugShutdown();
void __fastcall IndoorsShutdown();
void __fastcall OutdoorsShutdown();
void __fastcall InitializeGlueMusic();
void __fastcall ShutdownGlueMusic();
void __fastcall ProviderPrefInitialize();
void __fastcall ProviderPrefShutdown();
void __fastcall SndInterfaceSetProviderPrefs(unsigned int index, unsigned int indexUnderwater, unsigned int transitionDuration);
void __fastcall SndInterfaceSetProviderPrefs(const _FSOUND_REVERB_PROPERTIES &pref, const _FSOUND_REVERB_PROPERTIES &prefUnderwater);
void __fastcall SndInterfaceClearProviderPrefs(int indoors);
void __fastcall SndInterfaceProviderPrefsUnderwaterChanged();
void __fastcall SndSetRoomType(SNDROOMTYPE roomType);
void __fastcall ISndInterfaceInitialize();
void __fastcall ISndInterfaceShutdown();
void __fastcall SndInterfacePauseZoneMusic(int pause);
int __fastcall  SndInterfaceIsZoneMusicPaused();
void __fastcall SndInterfaceZoneIntroStop();
void __fastcall SndDebugDungeonTransition(int indoors, unsigned int continent);
void __fastcall SndInterfaceRegisterNewZone(unsigned int musicID);
void __fastcall SndInterfaceSetMIDIArea(int normal, int underwater);
void __fastcall SndInterfaceClearMIDI();
void __fastcall SndInterfaceRegisterNewZoneIntro(int soundID, int priority);
void __fastcall SndInterfacePlayInterfaceSound(const char *name);
void __fastcall SndInterfacePlaySpellSound(int soundID, CGUnit_C *obj);
unsigned int __fastcall SndInterfaceGetSoundVariations(unsigned int soundID);
void __fastcall SndInterfacePlaySpellFizzleSound(unsigned int spellID, const CGUnit_C *caster);
void __fastcall
SndInterfacePlayParrySound(const VirtualItemInfo *attackingWeapon, const VirtualItemInfo *defendingItem, int criticalHit, const NTempest::C3Vector &position);
void __fastcall
SndInterfacePlayHitSound(const VirtualItemInfo *attackingWeapon, unsigned int defendingItemType, int criticalHit, const NTempest::C3Vector &position);
void __fastcall SndInterfacePlaySheatheSound(
    const VirtualItemInfo *info,
    int sheathing,
    const NTempest::C3Vector &position
);
void __fastcall   SndInterfacePlayDeflectedSound(NTempest::C3Vector &position);
void __fastcall   SndInterfacePlayImmuneSound(NTempest::C3Vector &pos);
void __fastcall   SndInterfacePlayAbsorbedSound(NTempest::C3Vector &pos);
void __fastcall   SndInterfaceInitializeVocalUISounds(unsigned int race, unsigned int sex);
void __fastcall   SndInterfacePlayVocalUISound(VOCALUISOUNDS soundType);
void __fastcall   SoundInterfacePlayVocalMacro(CGPlayer_C *player, int category);
void __fastcall   SndInterfacePlayItemSound(ITEMSOUNDTYPE soundType, const CGItem_C *itemPtr);
void __fastcall   SndInterfacePlayItemSound(ITEMSOUNDTYPE soundType, int itemDisplayID);
bool __fastcall   SndInterfacePlaySound(unsigned int soundID, int forceIndex);
bool __fastcall   SndInterfacePlaySound(unsigned int soundID, const NTempest::C3Vector &position, int forceIndex, float volumeScaler);
bool __fastcall   SoundInterfaceIsSoundLooping(unsigned int soundID, bool &looping);
bool __fastcall   SndInterfacePlaySplashSound(unsigned int soundID, const NTempest::C3Vector &position);
void __fastcall   SndInterfacePlayFoleySound(unsigned int materialID, const NTempest::C3Vector &position);
Sound *__fastcall SndInterfacePlayLoopedSound(unsigned int soundID, unsigned int loopCount);
Sound *__fastcall SndInterfacePlayLoopedSound(unsigned int soundID, const NTempest::C3Vector &position, unsigned int loopCount);
Sound *__fastcall SndInterfaceCreateSound(unsigned int soundID, float fadeInRate, int forceIndex, bool doNotKeepAlive);
void __fastcall   SndInterfaceAssociateSoundWithObject(Sound *sound, CGObject_C *objectPtr);
int __fastcall    SndInterfaceHandleDoodadLoopStart(unsigned int soundID, const NTempest::C3Vector &pos);
void __fastcall   SndInterfaceHandleDoodadLoopStop(unsigned int soundHandle);
void __fastcall   SndInterfaceHandleDoodadOneShot(unsigned int soundID, const NTempest::C3Vector &position);
void __fastcall   SndInterfaceSetGlueMusic(const char *musicFile);
void __fastcall   SndInterfaceStopGlueMusic(float fadeTime);
void __fastcall   SoundRegisterScriptFunctions();
void __fastcall   SoundUnregisterScriptFunctions();
void __fastcall   SndInterfaceSetPositionCallback();
void __fastcall   SndInterfaceClearPositionCallback();

#endif
