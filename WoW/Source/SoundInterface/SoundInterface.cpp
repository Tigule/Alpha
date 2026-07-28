#include "SoundInterface.h"

#include "Client.h"
#include "Console/ConsoleCommand.h"
#include "Console/ConsoleVar.h"
#include "DB/DBClient/AutoCode/FootstepTerrainLookupRec.h"
#include "DB/DBClient/AutoCode/ItemDisplayInfoRec.h"
#include "DB/DBClient/AutoCode/ItemGroupSoundsRec.h"
#include "DB/DBClient/AutoCode/MaterialRec.h"
#include "DB/DBClient/AutoCode/ResistancesRec.h"
#include "DB/DBClient/AutoCode/SpellRec.h"
#include "DB/DBClient/AutoCode/TerrainTypeSoundsRec.h"
#include "DB/DBClient/AutoCode/TerrainTypeRec.h"
#include "DB/DBClient/AutoCode/VocalUISoundsRec.h"
#include "DB/DBClient/DBClient.h"
#include "Event/EvtApi.h"
#include "FrameScript/FrameScript.h"
#include "Game/GameTime.h"
#include "Object/ObjectClient/Item_C.h"
#include "Object/ObjectClient/Object_C.h"
#include "Object/ObjectClient/Unit_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "SoundInterface/ISoundInterface.h"
#include "WorldClient/World.h"

#include "Base/Base.h"
#include "Base/CmdLine.h"
#include "Os/W32/Debugging.h"
#include "Os/W32/OsSound.h"
#include "Tempest/cmath.h"

#include <lua.h>
#include <storm.h>

static void            RegisterCVars();
static int Script_PlaySound(lua_State *L);
static int Script_PlayMusic(lua_State *L);
static bool InternalPlaySound(SOUNDCATEGORIES category, unsigned int soundID, int forceIndex);
static bool
InternalPlaySound(SOUNDCATEGORIES category, unsigned int soundID, const NTempest::C3Vector &position, int forceIndex, float volumeScaler);
static bool SoundGetParamValueInt(const char *parameter, int &value);
static bool SoundGetParamValueFloat(const char *parameter, float &value);
static bool SoundGetParamValueString(const char *parameter, const char *&value);
static void FootstepTerrainInitialize();
static float ObstructionCallback(const NTempest::C3Vector &listener, const NTempest::C3Vector &source);
static bool MusicVolumeHandler(CVar *cvar, const char *oldValue, const char *newValue, void *userArg);
static bool SoundVolumeHandler(CVar *cvar, const char *oldValue, const char *newValue, void *userArg);
static bool MasterVolumeHandler(CVar *cvar, const char *oldValue, const char *newValue, void *userArg);
static bool EnableMusicHandler(CVar *cvar, const char *oldValue, const char *newValue, void *userArg);
static bool EnableSoundHandler(CVar *cvar, const char *oldValue, const char *newValue, void *userArg);
void InitializeZoneMusic();
void ShutdownZoneMusic();
void SoundInterfaceInitializeWorldMIDI();
void SoundInterfaceShutdownWorldMIDI();
void InitializeWaterAmbiences();
void ShutdownWaterAmbiences();
void SoundInterfaceDoodadInitialize();
void SoundInterfaceDoodadDestroy();
void SndInterfaceZoneIntroInitialize();
void SndInterfaceZoneIntroDestroy();
void SndInterfaceZoneIntroIdler();
void SndInterfaceMIDIAmbienceChanged();

bool g_underWater;

static int s_elapsed;
AMBIENCE g_currentAmbience;

static int                      MIXRATE = 22050;
static const FrameScript_Method s_ScriptFunctions[2] = {
    {"PlaySound", Script_PlaySound},
    {"PlayMusic", Script_PlayMusic}
};
static HASHKEY_NONE                                s_nullHashKey;
static TSHashTable<FOOTSTEPSNDCACHE, HASHKEY_NONE> s_footstepHash;
static unsigned int                                s_footstepRequest;
static unsigned int                                s_footstepAccept;
static VOCALUISOUND                                s_vocalUISounds[66];
static VOCALUISOUNDS                               s_lastPlayedVocalUISound;
static VOCALUISOUNDTYPE                            s_currentVocalUISoundType;
static unsigned int                                s_vocalUISoundPlayCount;

IMPACTSOUNDDESC::~IMPACTSOUNDDESC() {
  for (unsigned int i = 0; i < 2; ++i) {
    materialSounds[i].Clear();
  }
}

WEAPONSOUNDS::~WEAPONSOUNDS() {
  Clear();
}

WEAPONSOUNDS::WEAPONSOUNDS() {
  Clear();
}

WEAPONSOUNDS::WEAPONSOUNDS(const WEAPONSOUNDS &rhs) {
  for (unsigned int i = 0; i < 2; ++i) {
    soundList[i] = rhs.soundList[i];
  }
}

const WEAPONSOUNDS &WEAPONSOUNDS::operator=(const WEAPONSOUNDS &rhs) {
  if (&rhs != this) {
    Clear();
    for (unsigned int i = 0; i < 2; ++i) {
      soundList[i] = rhs.soundList[i];
    }
  }
  return *this;
}

void WEAPONSOUNDS::Clear() {
  soundList[0] = 0;
  soundList[1] = 0;
}

static void DetermineWeaponTypeAndMaterial(const VirtualItemInfo *item, unsigned int *weaponType, PARRYMATERIALS *material) {
  FATALASSERT(weaponType);
  FATALASSERT(material);

  if (item) {
    FATALASSERT(item->m_classID == 2);
    *weaponType = item->m_subclassID;
    *material = CGItem_C::IsMetal(item->m_material) ? PARRYMATERIAL_METAL : PARRYMATERIAL_WOOD;
  } else {
    *material = PARRYMATERIAL_WOOD;
    *weaponType = ClientDBGetUnarmedWeapon();
  }
}

static void FootstepTerrainInitialize() {
  uint                            i;
  FOOTSTEPSNDCACHE               *node;
  const FootstepTerrainLookupRec *rec;
  uint                            numTerrains;

  s_footstepHash.Clear();
  numTerrains = g_terrainTypeSoundsDB.GetMaxID() + 1;
  ASSERT(numTerrains < sizeof(uint) * 8);

  for (i = g_footstepTerrainLookupDB.GetNumRecords(); i; --i) {
    rec = g_footstepTerrainLookupDB.GetRecordByIndex(i - 1);
    ASSERT(rec);

    node = s_footstepHash.Ptr(rec->m_CreatureFootstepID, s_nullHashKey);
    if (!node) {
      node = s_footstepHash.New(rec->m_CreatureFootstepID, s_nullHashKey, 0, 0);
      node->m_soundIDs.SetCount(numTerrains);
      node->m_splashSoundIDs.SetCount(numTerrains);

      for (uint terrain = 0; terrain < numTerrains; ++terrain) {
        node->m_soundIDs[terrain] = 0;
        node->m_splashSoundIDs[terrain] = 0;
      }
    }

    ASSERT((uint)rec->m_TerrainSoundID < numTerrains);
    node->m_soundIDs[rec->m_TerrainSoundID] = rec->m_SoundID;
    node->m_splashSoundIDs[rec->m_TerrainSoundID] = rec->m_SoundIDSplash;
  }
}

static unsigned int GetFootstepTerrain(unsigned int soundID, unsigned int terrainID, int splashing) {
  const TerrainTypeRec *terrain = g_terrainTypeDB.GetRecord(terrainID);
  if (!terrain) {
    return 0;
  }

  FOOTSTEPSNDCACHE *entry = s_footstepHash.Ptr(soundID, s_nullHashKey);
  if (!entry) {
    return 0;
  }

  unsigned int terrainSoundID = terrain->m_SoundID;
  TSGrowableArray<unsigned int> &sounds = splashing ? entry->m_splashSoundIDs : entry->m_soundIDs;
  FATALASSERT(terrainSoundID < sounds.Count());
  return sounds[terrainSoundID];
}

static float ObstructionCallback(const NTempest::C3Vector &listener, const NTempest::C3Vector &source) {
  NTempest::C3Vector ip;
  float              dist = 1.0f;
  float              squaredMag;

  squaredMag = (source - listener).SquaredMag();

  if (NTempest::CMath::fabs_(squaredMag) < 0.001f) {
    return 0.0f;
  }

  if (squaredMag > 10000.0f) {
    return 0.75f;
  }

  if (!CWorld::Intersect(&listener, &source, 0.0f, &ip, &dist, 0x110)) {
    return 0.0f;
  }

  return NTempest::CMath::sqrt_(squaredMag) * 0.01f * 0.75f;
}

static bool MusicVolumeHandler(CVar *cvar, const char *oldValue, const char *newValue, void *userArg) {
  Sound::SetMusicVolume(SStrToFloat(newValue));
  return true;
}

static bool SoundVolumeHandler(CVar *cvar, const char *oldValue, const char *newValue, void *userArg) {
  Sound::SetSoundVolume(SStrToFloat(newValue));
  return true;
}

static bool MasterVolumeHandler(CVar *cvar, const char *oldValue, const char *newValue, void *userArg) {
  Sound::SetMasterVolume(SStrToFloat(newValue));
  return true;
}

static bool EnableMusicHandler(CVar *cvar, const char *oldValue, const char *newValue, void *userArg) {
  int enable = SStrToInt(newValue);

  SndInterfacePauseZoneMusic(!enable);
  if (enable) {
    SndInterfaceSetGlueMusic("");
  } else {
    SndInterfaceStopGlueMusic(0.0f);
  }

  return true;
}

static bool EnableSoundHandler(CVar *cvar, const char *oldValue, const char *newValue, void *userArg) {
  CVar *masterSoundEffects = CVar::Lookup("MasterSoundEffects");

  if (masterSoundEffects && masterSoundEffects->GetInt() && SStrToInt(newValue)) {
    Sound::MuteSFX(false);
  } else {
    Sound::MuteSFX(true);
  }
  return true;
}

static void RegisterCVars() {
  CVar::Register("SoundOutputSystem", "sound output system", CVar::LATCH, "-1", 0, SOUND, false, 0);
  CVar::Register("SoundDriver", "sound driver", CVar::LATCH, "-1", 0, SOUND, false, 0);
  CVar::Register("SoundMixer", "sound mixer", CVar::LATCH, "-1", 0, SOUND, false, 0);
  CVar::Register("SoundBufferSize", "sound buffer size (milliseconds)", CVar::LATCH, "0", 0, SOUND, false, 0);
  CVar::Register("SoundMinHardwareChannels", "sound minimum hadrware channels", CVar::LATCH, "-1", 0, SOUND, false, 0);
  CVar::Register("SoundMaxHardwareChannels", "sound maximum hadrware channels", CVar::LATCH, "-1", 0, SOUND, false, 0);
  CVar::Register("SoundMixRate", "sound mix rate (Hz)", CVar::LATCH, "44100", 0, SOUND, false, 0);
  CVar::Register("SoundSoftwareChannels", "sound software channels", CVar::LATCH, "12", 0, SOUND, false, 0);
  CVar::Register("SoundInitFlags", "sound initialization flags", CVar::LATCH, "128", 0, SOUND, false, 0);
  CVar::Register("SoundMemoryCache", "sound cache memory size (MB)", CVar::LATCH, "4", 0, SOUND, false, 0);
  CVar::Register("MusicVolume", "music volume (0.0 to 1.0)", 0, "0.25", MusicVolumeHandler, SOUND, false, 0);
  CVar::Register("SoundVolume", "sound volume (0.0 to 1.0)", 0, "1.0", SoundVolumeHandler, SOUND, false, 0);
  CVar::Register("MasterVolume", "master volume (0.0 to 1.0)", 0, "1.0", MasterVolumeHandler, SOUND, false, 0);
  CVar::Register("MasterSoundEffects", "", 0, "1", 0, SOUND, false, 0);
  CVar::Register("EnableMusic", "Enables music", 0, "1", EnableMusicHandler, SOUND, false, 0);
  CVar::Register("EnableSound", "Enables sound", 0, "1", EnableSoundHandler, SOUND, false, 0);
}

static bool SoundGetParamValueInt(const char *parameter, int &value) {
  CVar *cvar = CVar::Lookup(parameter);
  if (!cvar) {
    return false;
  }

  value = cvar->GetInt();
  return true;
}

static bool SoundGetParamValueFloat(const char *parameter, float &value) {
  CVar *cvar = CVar::Lookup(parameter);
  if (!cvar) {
    return false;
  }

  value = cvar->GetFloat();
  return true;
}

static bool SoundGetParamValueString(const char *parameter, const char *&value) {
  CVar *cvar = CVar::Lookup(parameter);
  if (!cvar) {
    return false;
  }

  value = cvar->GetString();
  return true;
}

void SndInterfaceInitialize() {
  RegisterCVars();
  SoundInterfaceRegisterWorldCVars();

  if (CmdLineGetBool(static_cast<CMDOPT>(0x1A))) {
    return;
  }

  SndDebugInitialize();
  Sound::Initialize(SoundGetParamValueInt, SoundGetParamValueFloat, SoundGetParamValueString);
  ISndInterfaceInitialize();
  FootstepTerrainInitialize();
  InitializeGlueMusic();
  ProviderPrefInitialize();
  SndSetObstructionCallback(ObstructionCallback);
}

void SndInterfaceDestroy() {
  SndSetObstructionCallback(0);
  g_sndInterfaceFlags = 0;
  ProviderPrefShutdown();
  ShutdownGlueMusic();
  ISndInterfaceShutdown();
  SndDebugShutdown();
  s_footstepHash.Destroy();
  Sound::Shutdown();
  OsOutputDebugString("Footsteps: requested %u accepted %u\n", s_footstepRequest, s_footstepAccept);
}

void SoundInterfaceRegisterWorldCVars() {
  CVar::Register("EnableGroupSpeech", "voice macros", 0, "1", 0, SOUND, false, 0);
  CVar::Register("EnableErrorSpeech", "error speech", 0, "1", 0, SOUND, false, 0);
  SoundInterfaceInitializeWorldMIDICVars();
}

void SndInterfacePlayItemSound(ITEMSOUNDTYPE soundType, const CGItem_C *itemPtr) {
  ASSERT(soundType < NUM_ITEMSOUNDS);
  ASSERT(itemPtr);

  const ItemGroupSoundsRec *sounds = itemPtr->GetGroupSoundRec();
  if (sounds) {
    SndInterfacePlaySound(sounds->m_sound[soundType], -1);
  }
}

void SndInterfacePlayItemSound(ITEMSOUNDTYPE soundType, int itemDisplayID) {
  ASSERT(soundType < NUM_ITEMSOUNDS);

  const ItemDisplayInfoRec *displayInfo = g_itemDisplayInfoDB.GetRecord(itemDisplayID);
  if (displayInfo) {
    const ItemGroupSoundsRec *sounds = g_itemGroupSoundsDB.GetRecord(displayInfo->m_groupSoundIndex);
    if (sounds) {
      SndInterfacePlaySound(sounds->m_sound[soundType], -1);
    }
  }
}

static int WorldIdle(const void *dataPtr, void *) {
  s_elapsed += static_cast<int>(*static_cast<const float *>(dataPtr) * 1000.0f);
  if (s_elapsed >= 1000) {
    s_elapsed -= 1000;
    SndInterfaceZoneIntroIdler();

    unsigned int encodedTime;
    WowTime      valMax;
    WowTime      valMin;
    WowTime::WowEncodeTime(encodedTime, 0, 20, -1, -1, -1, -1, 0);
    WowTime::WowDecodeTime(encodedTime, &valMax);
    WowTime::WowEncodeTime(encodedTime, 30, 5, -1, -1, -1, -1, 0);
    WowTime::WowDecodeTime(encodedTime, &valMin);

    AMBIENCE ambience = g_clientGameTime.InRange(valMin, valMax) ? AMB_DAY : AMB_NIGHT;
    if (ambience != g_currentAmbience) {
      g_currentAmbience = ambience;
      SndInterfaceMIDIAmbienceChanged();
    }
  }

  return 1;
}

void SndInterfaceWorldInitialize() {
  SoundInterfaceRegisterWorldCVars();
  if (!CmdLineGetBool(static_cast<CMDOPT>(26))) {
    s_elapsed = 1000;
    InitializeZoneMusic();
    SoundInterfaceInitializeWorldMIDI();
    InitializeWaterAmbiences();
    SoundInterfaceDoodadInitialize();
    EventRegister(EVENT_ID_IDLE, WorldIdle);
    SndInterfaceZoneIntroInitialize();

    unsigned int encodedTime;
    WowTime      valMax;
    WowTime      valMin;
    g_currentAmbience = AMB_NIGHT;
    WowTime::WowEncodeTime(encodedTime, 0, 20, -1, -1, -1, -1, 0);
    WowTime::WowDecodeTime(encodedTime, &valMax);
    WowTime::WowEncodeTime(encodedTime, 30, 5, -1, -1, -1, -1, 0);
    WowTime::WowDecodeTime(encodedTime, &valMin);
    if (g_clientGameTime.InRange(valMin, valMax)) {
      g_currentAmbience = AMB_DAY;
    }
  }
}

void SndInterfaceWorldDestroy() {
  EventUnregister(EVENT_ID_IDLE, WorldIdle);
  if (!CmdLineGetBool(static_cast<CMDOPT>(26))) {
    ShutdownZoneMusic();
    ShutdownWaterAmbiences();
    SoundInterfaceShutdownWorldMIDI();
    SoundInterfaceDoodadDestroy();
    SndInterfaceZoneIntroDestroy();
  }
}

void
SndInterfacePlayParrySound(const VirtualItemInfo *attackingWeapon, const VirtualItemInfo *defendingItem, int criticalHit, const NTempest::C3Vector &position) {
  if (!g_impactSounds.Count()) {
    return;
  }

  unsigned int   weaponType;
  PARRYMATERIALS attackingMaterial;
  DetermineWeaponTypeAndMaterial(attackingWeapon, &weaponType, &attackingMaterial);

  unsigned int defendingItemType;
  if (defendingItem->m_classID == 2) {
    defendingItemType = CGItem_C::IsMetal(defendingItem->m_material) ? 5 : 6;
  } else if (defendingItem->m_classID == 4) {
    defendingItemType = CGItem_C::IsMetal(defendingItem->m_material) ? 3 : 4;
  } else {
    FATALASSERT(!"parrying object is not a weapon or armor..something is wrong here!");
  }

  FATALASSERT(weaponType < ClientDBGetNumWeaponSubclasses());
  NTempest::C3Vector pos = position;
  pos.z += 2.0f;
  unsigned int soundID = g_impactSounds[weaponType].desc[defendingItemType].materialSounds[attackingMaterial].soundList[criticalHit != 0];
  SndInterfacePlaySound(soundID, pos, -1, 1.0f);
}

void
SndInterfacePlayHitSound(const VirtualItemInfo *attackingWeapon, unsigned int defendingItemType, int criticalHit, const NTempest::C3Vector &position) {
  if (!g_impactSounds.Count()) {
    return;
  }

  unsigned int   weaponType;
  PARRYMATERIALS attackingMaterial;
  DetermineWeaponTypeAndMaterial(attackingWeapon, &weaponType, &attackingMaterial);
  FATALASSERT(weaponType < ClientDBGetNumWeaponSubclasses());

  unsigned int soundID = g_impactSounds[weaponType].desc[defendingItemType].materialSounds[attackingMaterial].soundList[criticalHit != 0];
  SndInterfacePlaySound(soundID, position, -1, 1.0f);
}

void SndInterfacePlayDeflectedSound(const NTempest::C3Vector &position) {
  NTempest::C3Vector pos = position;
  pos.z += 2.0f;
  SndInterfacePlaySound(3263, pos, -1, 1.0f);
}

void SndInterfacePlayWeaponSwooshSound(WEAPONSWING_SOUNDTYPES soundType, int criticalHit, const NTempest::C3Vector &position, int missed) {
  if (soundType >= NUM_WEAPONSWINGSOUNDTYPES) {
    return;
  }

  NTempest::C3Vector soundPosition = position;
  soundPosition.z += 1.0f;
  unsigned int soundID = g_weaponSwingSounds[soundType].soundList[criticalHit != 0];
  SndInterfacePlaySound(soundID, soundPosition, -1, missed ? 0.5f : 1.0f);
}

void SndInterfacePlaySpellSound(int soundID, CGUnit_C *obj) {
  SOUNDDEFINITION *definition = ISndInterfaceGetSndEntry(soundID);
  if (!definition || !obj || !(obj->GetType() & TYPE_UNIT)) {
    return;
  }

  if (definition->m_flags & 0x200) {
    obj->PlaySpellLoopedSound(soundID);
  } else {
    NTempest::C3Vector position = obj->GetPosition();
    SndInterfacePlaySound(soundID, position, -1, 1.0f);
  }
}

void SndInterfacePlayInterfaceSound(const char *name) {
  UISOUNDLOOKUP *lookup;

  if (!name || !*name) {
    return;
  }

  lookup = g_uiSoundLookups.Ptr(name);
  if (lookup) {
    SndInterfacePlaySound(lookup->soundID, -1);
  }
}

void SndInterfaceInitializeVocalUISounds(unsigned int race, unsigned int sex) {
  unsigned int i;

  s_lastPlayedVocalUISound = static_cast<VOCALUISOUNDS>(66);
  s_currentVocalUISoundType = VUISOUNDTYPE_NORMAL;
  s_vocalUISoundPlayCount = 0;

  for (i = 0; i < 66; ++i) {
    s_vocalUISounds[i].Clear();
  }

  if (sex > 1) {
    return;
  }

  for (i = g_vocalUISoundsDB.GetNumRecords(); i; --i) {
    const VocalUISoundsRec *rec = g_vocalUISoundsDB.GetRecordByIndex(i - 1);
    FATALASSERT(rec);

    if (static_cast<unsigned int>(rec->m_vocalUIEnum) < 66 && static_cast<unsigned int>(rec->m_raceID) == race) {
      VOCALUISOUND &sound = s_vocalUISounds[rec->m_vocalUIEnum];
      sound.soundTypes[VUISOUNDTYPE_NORMAL] = rec->m_NormalSoundID[sex];
      sound.soundTypes[VUISOUNDTYPE_PISSED] = rec->m_PissedSoundID[sex];

      SOUNDDEFINITION *definition = ISndInterfaceGetSndEntry(rec->m_PissedSoundID[sex]);
      if (definition) {
        sound.pissedCount = definition->m_fileNames.Count();
      }
    }
  }
}

void SndInterfacePlayVocalUISound(VOCALUISOUNDS soundType) {
  CVar *masterSoundEffects = CVar::Lookup("MasterSoundEffects");
  bool  soundEffectsEnabled = masterSoundEffects && masterSoundEffects->GetInt();
  CVar *enableErrorSpeech = CVar::Lookup("EnableErrorSpeech");

  if (!enableErrorSpeech || !enableErrorSpeech->GetInt() || soundType >= 66 || !soundEffectsEnabled) {
    return;
  }

  if (soundType != s_lastPlayedVocalUISound) {
    s_currentVocalUISoundType = VUISOUNDTYPE_NORMAL;
    s_vocalUISoundPlayCount = 0;
  }

  s_lastPlayedVocalUISound = soundType;

  if (s_currentVocalUISoundType == VUISOUNDTYPE_PISSED) {
    if (s_vocalUISoundPlayCount < s_vocalUISounds[soundType].pissedCount &&
        !InternalPlaySound(SOUNDCATEGORY_NONE, s_vocalUISounds[soundType].soundTypes[VUISOUNDTYPE_PISSED], s_vocalUISoundPlayCount))
    {
      return;
    }

    s_currentVocalUISoundType = VUISOUNDTYPE_NORMAL;
    s_vocalUISoundPlayCount = 0;
  }

  if (!s_vocalUISounds[soundType].soundTypes[VUISOUNDTYPE_NORMAL] ||
      (InternalPlaySound(SOUNDCATEGORY_NONE, s_vocalUISounds[soundType].soundTypes[VUISOUNDTYPE_NORMAL], -1) && ++s_vocalUISoundPlayCount >= 4))
  {
    s_currentVocalUISoundType = VUISOUNDTYPE_PISSED;
    s_vocalUISoundPlayCount = 0;
  }
}

void SndInterfacePlayFootstepSound(unsigned int footstepID, const NTempest::C3Vector& position, unsigned int terrainID, int splashing) {
  ++s_footstepRequest;
  NTempest::C3Vector listenerPosition;
  Sound::GetListenerPosition(listenerPosition);
  if ((position - listenerPosition).SquaredMag() <= 20.0f * 20.0f) {
    ++s_footstepAccept;
    unsigned int soundID = GetFootstepTerrain(footstepID, terrainID, splashing);
    if (soundID) {
      NTempest::C3Vector soundPosition = position;
      soundPosition.z += 1.0f / 36.0f;
      SndInterfacePlaySound(soundID, soundPosition, -1, 1.0f);
    }
  }
}

void SndInterfacePlayFoleySound(unsigned int materialID, const NTempest::C3Vector& position) {
  const MaterialRec *material = g_materialDB.GetRecord(materialID);
  if (material && material->m_foleySoundID) {
    SndInterfacePlaySound(material->m_foleySoundID, position, -1, 1.0f);
  }
}

void SndInterfacePlaySheatheSound(const VirtualItemInfo* info, int sheathing, const NTempest::C3Vector& position) {
  if (!info) {
    return;
  }
  SHEATHSOUNDHASH *entry = g_sheathSoundList.Ptr(info->m_classID, s_nullHashKey);
  if (!entry) {
    return;
  }
  TSFixedArray<unsigned int> &sounds = sheathing ? entry->materialSheathSound : entry->materialUnsheathSound;
  if (info->m_material < sounds.Count()) {
    SndInterfacePlaySound(sounds[info->m_material], position, -1, 1.0f);
  }
}

void SndInterfacePlayImmuneSound(const NTempest::C3Vector &pos) {
  NTempest::C3Vector position = pos;
  position.z += 2.0f;
  SndInterfacePlaySound(3334, position, -1, 1.0f);
}

static bool InternalPlaySound(SOUNDCATEGORIES category, unsigned int soundID, int forceIndex) {
  SOUNDDEFINITION *definition = ISndInterfaceGetSndEntry(soundID);
  const char      *filename;
  Sound           *sound;

  if (!definition) {
    return false;
  }

  filename = definition->GetRandomFileName(forceIndex);
  if (!filename || !*filename) {
    return false;
  }

  sound = Sound::Play2D(category, filename, definition->GetOsFlags(), true);
  if (!sound) {
    return false;
  }

  definition->SetFrequencyAndVolume(sound, 1.0f, false);
  if (!sound->SetPaused(false)) {
    Sound::KillSound(sound);
    return false;
  }

  return true;
}

void SndInterfacePlayAbsorbedSound(const NTempest::C3Vector &pos) {
  NTempest::C3Vector position = pos;
  position.z += 2.0f;
  SndInterfacePlaySound(3334, position, -1, 1.0f);
}

unsigned int SndInterfaceGetSoundVariations(unsigned int soundID) {
  SOUNDDEFINITION *definition = ISndInterfaceGetSndEntry(soundID);
  return definition ? definition->m_fileNames.Count() : 0;
}

static int Script_PlaySound(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    lua_pushfstring(L, "Usage: PlaySound(\"sound\")");
    lua_error(L);
  }

  SndInterfacePlayInterfaceSound(lua_tostring(L, 1));
  return 0;
}

static int Script_PlayMusic(lua_State *L) {
  Sound *sound;

  if (!lua_isstring(L, 1)) {
    lua_pushfstring(L, "Usage: PlayMusic(\"music\")");
    lua_error(L);
  }

  sound = Sound::Play2DLooped(SOUNDCATEGORY_NONE, lua_tostring(L, 1), 2, 0, true);
  if (sound && !sound->SetPaused(false)) {
    Sound::KillSound(sound);
  }

  return 0;
}

void SoundRegisterScriptFunctions() {
  for (unsigned int i = 0; i < 2; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void SoundUnregisterScriptFunctions() {
  for (unsigned int i = 0; i < 2; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}

void SndInterfaceSetUnderwater(bool underWater) {
  if (!(g_sndInterfaceFlags & 1) || underWater != g_underWater) {
    g_sndInterfaceFlags |= 1;
    g_underWater = underWater;
    SndInterfaceMIDIUnderwaterChanged();
    WaterAmbiencesUnderwaterChanged();
    SndInterfaceProviderPrefsUnderwaterChanged();
  }
}

bool SndInterfacePlaySound(unsigned int soundID, int forceIndex) {
  return InternalPlaySound(SOUNDCATEGORY_NONE, soundID, forceIndex);
}

bool SndInterfacePlaySound(unsigned int soundID, const NTempest::C3Vector &position, int forceIndex, float volumeScaler) {
  return InternalPlaySound(SOUNDCATEGORY_NONE, soundID, position, forceIndex, volumeScaler);
}

bool SoundInterfaceIsSoundLooping(unsigned int soundID, bool& looping) {
  SOUNDDEFINITION *definition = ISndInterfaceGetSndEntry(soundID);
  if (!definition) {
    return 0;
  }
  looping = (definition->m_flags & 0x200) != 0;
  return 1;
}

Sound *SndInterfacePlayLoopedSound(unsigned int soundID, unsigned int loopCount) {
  SOUNDDEFINITION *definition = ISndInterfaceGetSndEntry(soundID);
  if (!definition) {
    return 0;
  }

  const char *filename = definition->GetRandomFileName(-1);
  if (!filename || !*filename) {
    return 0;
  }

  Sound *sound = Sound::Play2DLooped(SOUNDCATEGORY_NONE, filename, definition->GetOsFlags() | 4, loopCount, true);
  if (!sound) {
    return 0;
  }

  definition->SetFrequencyAndVolume(sound, 1.0f, false);
  if (!sound->SetPaused(false)) {
    Sound::KillSound(sound);
  }
  return sound;
}

Sound *SndInterfacePlayLoopedSound(unsigned int soundID, const NTempest::C3Vector &position, unsigned int loopCount) {
  SOUNDDEFINITION *definition = ISndInterfaceGetSndEntry(soundID);
  if (!definition) {
    return 0;
  }
  const char *filename = definition->GetRandomFileName(-1);
  if (!filename || !*filename) {
    return 0;
  }
  Sound *sound = Sound::Play3DLooped(SOUNDCATEGORY_NONE, filename, definition->GetOsFlags() | 4, loopCount, true);
  if (!sound) {
    return 0;
  }
  definition->SetFrequencyAndVolume(sound, 1.0f, false);
  sound->SetPosition(position, 0);
  if (!sound->SetPaused(false)) {
    Sound::KillSound(sound);
  }
  return sound;
}

static bool
InternalPlaySound(SOUNDCATEGORIES category, unsigned int soundID, const NTempest::C3Vector &position, int forceIndex, float volumeScaler) {
  SOUNDDEFINITION *definition = ISndInterfaceGetSndEntry(soundID);
  const char      *filename;
  Sound           *sound;

  if (!definition) {
    return false;
  }

  filename = definition->GetRandomFileName(forceIndex);
  if (!filename || !*filename) {
    return false;
  }

  sound = Sound::Play3D(category, filename, definition->GetOsFlags(), true);
  if (!sound) {
    return false;
  }

  definition->SetFrequencyAndVolume(sound, volumeScaler, false);
  definition->Set3DParams(sound, &position);
  if (!sound->SetPaused(false)) {
    Sound::KillSound(sound);
  }

  return true;
}

bool SndInterfacePlaySplashSound(unsigned int soundID, const NTempest::C3Vector &position) {
  return InternalPlaySound(SOUNDCATEGORY_SPLASHES, soundID, position, -1, 1.0f);
}

void SndInterfacePlaySpellFizzleSound(unsigned int spellID, const CGUnit_C *caster) {
  const SpellRec *spellRec = g_spellDB.GetRecord(spellID);
  if (!spellRec) {
    return;
  }

  const ResistancesRec *resistance = g_resistancesDB.GetRecord(spellRec->m_school);
  if (resistance) {
    SndInterfacePlaySpellSound(resistance->m_FizzleSoundID, const_cast<CGUnit_C *>(caster));
  }
}

void SndInterfaceAssociateSoundWithObject(Sound *sound, CGObject_C *objectPtr) {
  if (objectPtr) {
    NTempest::C3Vector position = objectPtr->GetPosition();
    sound->SetPosition(position, 0);
    sound->Set3DUpdateHandle(objectPtr->GetGUID());
  } else {
    sound->Set3DUpdateHandle(0);
  }
}

Sound *SndInterfaceCreateSound(unsigned int soundID, float fadeInRate, int forceIndex, bool doNotKeepAlive) {
  static NTempest::C3Vector s_unknown;

  SOUNDDEFINITION *definition = ISndInterfaceGetSndEntry(soundID);
  if (!definition) {
    return 0;
  }

  const char *filename = definition->GetRandomFileName(forceIndex);
  if (!filename || !*filename) {
    return 0;
  }

  int flags = definition->GetOsFlags();
  if (!doNotKeepAlive) {
    flags |= 4;
  }

  Sound *sound = Sound::Play3DLooped(SOUNDCATEGORY_NONE, filename, flags, 0, true);
  if (sound) {
    sound->SetFadeIn(fadeInRate, 1.0f);
    definition->SetFrequencyAndVolume(sound, 1.0f, false);
    definition->Set3DParams(sound, 0);
    if (!sound->SetPaused(false)) {
      Sound::KillSound(sound);
    }
  }

  return sound;
}

bool SndInterfacePlaySound(Sound *sound, float fadeInRate) {
  sound->SetFadeIn(fadeInRate, 1.0f);
  return sound->SetPaused(false);
}

static unsigned char SoundPositionCallback(__int64 handle, NTempest::C3Vector& pos) {
  CGObject_C *object = ClntObjMgrObjectPtr(handle, __FILE__, __LINE__);
  if (object) {
    pos = object->GetPosition();
  }
  return object != 0;
}

void SndInterfaceSetPositionCallback() {
  Sound::m_positionUpdateCallback = SoundPositionCallback;
}

void SndInterfaceClearPositionCallback() {
  Sound::m_positionUpdateCallback = 0;
}

float SOUNDDEFINITION::GetVolume(float volumeScale, bool neverVary) const {
  float volume;

  if (!neverVary && (m_flags & 0x00000800)) {
    int value = NTempest::CMath::mulhwu_(31, NTempest::CRandom::uint32_(g_rndSeed));
    volume = m_volume + (value - 15) * 0.01f;
  } else {
    volume = m_volume;
  }

  volume *= volumeScale;
  if (volume <= 0.0f) {
    return 0.0f;
  }

  if (volume >= 1.0f) {
    return 1.0f;
  }

  return volume;
}

void SOUNDDEFINITION::SetFrequencyAndVolume(Sound *sound, float volumeScaler, bool neverVaryVolume) const {
  sound->SetVolume(GetVolume(volumeScaler, neverVaryVolume));

  if (m_flags & 0x00000400) {
    sound->SetFrequency(((static_cast<int>(NTempest::CMath::mulhwu_(31, NTempest::CRandom::uint32_(g_rndSeed))) + 85) * MIXRATE) / 100);
  }
}

void SOUNDDEFINITION::Set3DParams(Sound *sound, const NTempest::C3Vector *pos) {
  if (pos) {
    sound->SetPosition(*pos, 0);
  }

  sound->SetDistances(m_minDistance, m_maxDistance);
  sound->SetCutoffDistanceSquared(m_distanceCutoffSquared);
  sound->SetReverbProperties(GetReverbType(m_reverbPrefIndex));
}

int SOUNDDEFINITION::GetOsFlags() const {
  int flags = 0;

  if (m_flags & 0x00000020) {
    flags |= 1;
  }

  return flags;
}

void SndSetObstructionCallback(float(*)(const NTempest::C3Vector &, const NTempest::C3Vector &callback)) {
}
