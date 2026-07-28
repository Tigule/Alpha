#ifndef WOW_SOURCE_SOUNDINTERFACE_SOUNDINTERFACE_H
#define WOW_SOURCE_SOUNDINTERFACE_SOUNDINTERFACE_H

#include "Os/W32/OsSound.h"

#include <stpl.h>

struct FOOTSTEPSNDCACHE : public TSHashObject<FOOTSTEPSNDCACHE, HASHKEY_NONE> {
  FOOTSTEPSNDCACHE() {
  }

  FOOTSTEPSNDCACHE(const FOOTSTEPSNDCACHE &rhs);

  ~FOOTSTEPSNDCACHE() {
  }

  TSGrowableArray<unsigned int> m_soundIDs;
  TSGrowableArray<unsigned int> m_splashSoundIDs;
};

class CGItem_C;
class CGObject_C;
class CGPlayer_C;
class CGUnit_C;
struct VirtualItemInfo;

enum VOCALUISOUNDS {
  VUI_INVENTORYFULL = 0,
  VUI_OUTOFAMMO = 1,
  VUI_NOEQUIP_LEVEL = 2,
  VUI_NOEQUIP_EVER = 3,
  VUI_BOUND_NODROP = 4,
  VUI_ITEMCOOLING = 5,
  VUI_CANTDRINKMORE = 6,
  VUI_CANTEATMORE = 7,
  VUI_CANTINVITE = 8,
  VUI_INVITEEBUSY = 9,
  VUI_TARGETTOOFAR = 10,
  VUI_INVALIDTARGET = 11,
  VUI_SPELLCOOLING = 12,
  VUI_CANTLEARN_LEVEL = 13,
  VUI_LOCKED = 14,
  VUI_NOMANA = 15,
  VUI_NOTWHILEDEAD = 16,
  VUI_CANTLOOT = 17,
  VUI_CANTCREATE = 18,
  VUI_DECLINEGROUP = 19,
  VUI_ALREADYINGROUP = 20,
  VUI_ALREADYINGUILD = 21,
  VUI_CANTAFFORDBANKSLOT = 22,
  VUI_TOOMANYBANKSLOTS = 23,
  VUI_CANTEAT_MOVING = 24,
  VUI_NOTABAG = 25,
  VUI_CANTPUTBAG = 26,
  VUI_WRONGSLOT = 27,
  VUI_AMMOONLYINBAG = 28,
  VUI_BAGFULL = 29,
  VUI_ITEMMAXCOUNT = 30,
  VUI_CANTLOOT_DIDNTKILL = 31,
  VUI_CANTLOOT_WRONGFACING = 32,
  VUI_CANTLOOT_LOCKED = 33,
  VUI_CANTLOOT_NOTSTANDING = 34,
  VUI_CANTLOOT_TOOFAR = 35,
  VUI_CANTATTACKRONGDIRECTION = 36,
  VUI_CANTATTACK_NOTSTANDING = 37,
  VUI_CANTATTACK_NOTARGET = 38,
  VUI_NOTENOUGHGOLD = 39,
  VUI_NOTENOUGHMONEY = 40,
  VUI_CANTEQUIP2H_SKILL = 41,
  VUI_CANTEQUIP_2HEQUIPPED = 42,
  VUI_CANTEQUIP2H_NOSKILL = 43,
  VUI_NOTEQUIPPABLE = 44,
  VUI_GENERICNOTARGET = 45,
  VUI_CANTCAST_OUTOFRANGE = 46,
  VUI_POTIONCOOLING = 47,
  VUI_PROFICIENCYNEEDED = 48,
  VUI_MUSTEQUIPPITEM = 49,
  VUI_ABILITYCOOLING = 50,
  VUI_CANTUSEITEM = 51,
  VUI_CHESTINUSE = 52,
  VUI_FOODCOOLING = 53,
  VUI_CANTTAXI_NOMONEY = 54,
  VUI_CANTUSELOCKED = 55,
  VUI_NOEQUIPSLOTAVAILABLE = 56,
  VUI_CANTUSETOOFAR = 57,
  VUI_CANTSWAP = 58,
  VUI_CANTTRADE_SOULBOUND = 59,
  VUI_NOTOWNER = 60,
  VUI_ITEMLOCKED = 61,
  VUI_GUILDPERMISSIONS = 62,
  VUI_NORAGE = 63,
  VUI_NOENERGY = 64,
  VUI_NOFOCUS = 65,
  NUM_VOCALUISOUNDS = 66,
  VUI_NONE = 66
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
void SndInterfacePlayDeflectedSound(const NTempest::C3Vector &position);
void SndInterfacePlayImmuneSound(const NTempest::C3Vector &pos);
void SndInterfacePlayAbsorbedSound(const NTempest::C3Vector &pos);
void SndInterfaceInitializeVocalUISounds(unsigned int race, unsigned int sex);
void SndInterfacePlayVocalUISound(VOCALUISOUNDS soundType);
void SoundInterfacePlayVocalMacro(const CGPlayer_C *player, int category);
void SndInterfacePlayItemSound(ITEMSOUNDTYPE soundType, const CGItem_C *itemPtr);
void SndInterfacePlayItemSound(ITEMSOUNDTYPE soundType, int itemDisplayID);
bool SndInterfacePlaySound(unsigned int soundID, int forceIndex);
bool SndInterfacePlaySound(unsigned int soundID, const NTempest::C3Vector &position, int forceIndex, float volumeScaler);
bool SndInterfacePlaySound(Sound *sound, float fadeInRate);
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
