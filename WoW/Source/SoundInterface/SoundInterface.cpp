#include "SoundInterface.h"

#include "Client.h"
#include "Console/ConsoleCommand.h"
#include "Console/ConsoleVar.h"
#include "DB/DBClient/AutoCode/FootstepTerrainLookupRec.h"
#include "DB/DBClient/AutoCode/ItemDisplayInfoRec.h"
#include "DB/DBClient/AutoCode/ItemGroupSoundsRec.h"
#include "DB/DBClient/AutoCode/ResistancesRec.h"
#include "DB/DBClient/AutoCode/SpellRec.h"
#include "DB/DBClient/AutoCode/TerrainTypeSoundsRec.h"
#include "DB/DBClient/AutoCode/VocalUISoundsRec.h"
#include "DB/DBClient/DBClient.h"
#include "FrameScript/FrameScript.h"
#include "Object/ObjectClient/Item_C.h"
#include "Object/ObjectClient/Object_C.h"
#include "Object/ObjectClient/Unit_C.h"
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
static int __fastcall  Script_PlaySound(lua_State *L);
static int __fastcall  Script_PlayMusic(lua_State *L);
static bool __fastcall InternalPlaySound(SOUNDCATEGORIES category, unsigned int soundID, int forceIndex);
static bool __fastcall
InternalPlaySound(SOUNDCATEGORIES category, unsigned int soundID, const NTempest::C3Vector &position, int forceIndex, float volumeScaler);
static bool __fastcall  SoundGetParamValueInt(const char *parameter, int &value);
static bool __fastcall  SoundGetParamValueFloat(const char *parameter, float &value);
static bool __fastcall  SoundGetParamValueString(const char *parameter, const char *&value);
static void __fastcall  FootstepTerrainInitialize();
static float __fastcall ObstructionCallback(const NTempest::C3Vector &listener, const NTempest::C3Vector &source);
static bool __fastcall  MusicVolumeHandler(CVar *cvar, const char *oldValue, const char *newValue, void *userArg);
static bool __fastcall  SoundVolumeHandler(CVar *cvar, const char *oldValue, const char *newValue, void *userArg);
static bool __fastcall  MasterVolumeHandler(CVar *cvar, const char *oldValue, const char *newValue, void *userArg);
static bool __fastcall  EnableMusicHandler(CVar *cvar, const char *oldValue, const char *newValue, void *userArg);
static bool __fastcall  EnableSoundHandler(CVar *cvar, const char *oldValue, const char *newValue, void *userArg);

bool g_underWater;

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

static void __fastcall DetermineWeaponTypeAndMaterial(VirtualItemInfo *item, unsigned int *weaponType, PARRYMATERIALS *material) {
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

static void __fastcall FootstepTerrainInitialize() {
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

static float __fastcall ObstructionCallback(const NTempest::C3Vector &listener, const NTempest::C3Vector &source) {
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

static bool __fastcall MusicVolumeHandler(CVar *cvar, const char *oldValue, const char *newValue, void *userArg) {
  Sound::SetMusicVolume(SStrToFloat(newValue));
  return true;
}

static bool __fastcall SoundVolumeHandler(CVar *cvar, const char *oldValue, const char *newValue, void *userArg) {
  Sound::SetSoundVolume(SStrToFloat(newValue));
  return true;
}

static bool __fastcall MasterVolumeHandler(CVar *cvar, const char *oldValue, const char *newValue, void *userArg) {
  Sound::SetMasterVolume(SStrToFloat(newValue));
  return true;
}

static bool __fastcall EnableMusicHandler(CVar *cvar, const char *oldValue, const char *newValue, void *userArg) {
  int enable = SStrToInt(newValue);

  SndInterfacePauseZoneMusic(!enable);
  if (enable) {
    SndInterfaceSetGlueMusic("");
  } else {
    SndInterfaceStopGlueMusic(0.0f);
  }

  return true;
}

static bool __fastcall EnableSoundHandler(CVar *cvar, const char *oldValue, const char *newValue, void *userArg) {
  CVar *masterSoundEffects = CVar::Lookup("MasterSoundEffects");

  if (masterSoundEffects && masterSoundEffects->m_intValue && SStrToInt(newValue)) {
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

static bool __fastcall SoundGetParamValueInt(const char *parameter, int &value) {
  CVar *cvar = CVar::Lookup(parameter);
  if (!cvar) {
    return false;
  }

  value = cvar->m_intValue;
  return true;
}

static bool __fastcall SoundGetParamValueFloat(const char *parameter, float &value) {
  CVar *cvar = CVar::Lookup(parameter);
  if (!cvar) {
    return false;
  }

  value = cvar->m_floatValue;
  return true;
}

static bool __fastcall SoundGetParamValueString(const char *parameter, const char *&value) {
  CVar *cvar = CVar::Lookup(parameter);
  if (!cvar) {
    return false;
  }

  value = cvar->m_stringValue;
  return true;
}

void __fastcall SndInterfaceInitialize() {
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

void __fastcall SndInterfaceDestroy() {
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

void __fastcall SoundInterfaceRegisterWorldCVars() {
  CVar::Register("EnableGroupSpeech", "voice macros", 0, "1", 0, SOUND, false, 0);
  CVar::Register("EnableErrorSpeech", "error speech", 0, "1", 0, SOUND, false, 0);
  SoundInterfaceInitializeWorldMIDICVars();
}

void __fastcall SndInterfacePlayItemSound(ITEMSOUNDTYPE soundType, const CGItem_C *itemPtr) {
  ASSERT(soundType < NUM_ITEMSOUNDS);
  ASSERT(itemPtr);

  ItemGroupSoundsRec *sounds = itemPtr->GetGroupSoundRec();
  if (sounds) {
    SndInterfacePlaySound(sounds->m_sound[soundType], -1);
  }
}

void __fastcall SndInterfacePlayItemSound(ITEMSOUNDTYPE soundType, int itemDisplayID) {
  ASSERT(soundType < NUM_ITEMSOUNDS);

  ItemDisplayInfoRec *displayInfo = g_itemDisplayInfoDB.GetRecord(itemDisplayID);
  if (displayInfo) {
    ItemGroupSoundsRec *sounds = g_itemGroupSoundsDB.GetRecord(displayInfo->m_groupSoundIndex);
    if (sounds) {
      SndInterfacePlaySound(sounds->m_sound[soundType], -1);
    }
  }
}

void __fastcall SndInterfacePlayInterfaceSound(const char *name) {
  UISOUNDLOOKUP *lookup;

  if (!name || !*name) {
    return;
  }

  lookup = g_uiSoundLookups.Ptr(name);
  if (lookup) {
    SndInterfacePlaySound(lookup->soundID, -1);
  }
}

void __fastcall
SndInterfacePlayParrySound(VirtualItemInfo *attackingWeapon, VirtualItemInfo *defendingItem, int criticalHit, NTempest::C3Vector &position) {
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

void __fastcall
SndInterfacePlayHitSound(VirtualItemInfo *attackingWeapon, unsigned int defendingItemType, int criticalHit, NTempest::C3Vector &position) {
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

void __fastcall SndInterfacePlayWeaponSwooshSound(WEAPONSWING_SOUNDTYPES soundType, int criticalHit, const NTempest::C3Vector &position, int missed) {
  if (soundType >= NUM_WEAPONSWING_SOUNDTYPES) {
    return;
  }

  NTempest::C3Vector soundPosition = position;
  soundPosition.z += 1.0f;
  unsigned int soundID = g_weaponSwingSounds[soundType].soundList[criticalHit != 0];
  SndInterfacePlaySound(soundID, soundPosition, -1, missed ? 0.5f : 1.0f);
}

void __fastcall SndInterfacePlayDeflectedSound(NTempest::C3Vector &position) {
  NTempest::C3Vector pos = position;
  pos.z += 2.0f;
  SndInterfacePlaySound(3263, pos, -1, 1.0f);
}

void __fastcall SndInterfacePlaySpellSound(int soundID, CGUnit_C *obj) {
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

void __fastcall SndInterfacePlaySpellFizzleSound(unsigned int spellID, const CGUnit_C *caster) {
  SpellRec *spellRec = g_spellDB.GetRecord(spellID);
  if (!spellRec) {
    return;
  }

  ResistancesRec *resistance = g_resistancesDB.GetRecord(spellRec->m_school);
  if (resistance) {
    SndInterfacePlaySpellSound(resistance->m_FizzleSoundID, const_cast<CGUnit_C *>(caster));
  }
}

void __fastcall SndInterfacePlayImmuneSound(NTempest::C3Vector &pos) {
  NTempest::C3Vector position = pos;
  position.z += 2.0f;
  SndInterfacePlaySound(3334, position, -1, 1.0f);
}

void __fastcall SndInterfacePlayAbsorbedSound(NTempest::C3Vector &pos) {
  NTempest::C3Vector position = pos;
  position.z += 2.0f;
  SndInterfacePlaySound(3334, position, -1, 1.0f);
}

static bool __fastcall InternalPlaySound(SOUNDCATEGORIES category, unsigned int soundID, int forceIndex) {
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

static int __fastcall Script_PlaySound(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    lua_pushfstring(L, "Usage: PlaySound(\"sound\")");
    lua_error(L);
  }

  SndInterfacePlayInterfaceSound(lua_tostring(L, 1));
  return 0;
}

static int __fastcall Script_PlayMusic(lua_State *L) {
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

void __fastcall SoundRegisterScriptFunctions() {
  for (unsigned int i = 0; i < 2; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void __fastcall SoundUnregisterScriptFunctions() {
  for (unsigned int i = 0; i < 2; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}

void __fastcall SndInterfaceSetUnderwater(bool underWater) {
  if (!(g_sndInterfaceFlags & 1) || underWater != g_underWater) {
    g_sndInterfaceFlags |= 1;
    g_underWater = underWater;
    SndInterfaceMIDIUnderwaterChanged();
    WaterAmbiencesUnderwaterChanged();
    SndInterfaceProviderPrefsUnderwaterChanged();
  }
}

bool __fastcall SndInterfacePlaySound(unsigned int soundID, int forceIndex) {
  return InternalPlaySound(SOUNDCATEGORY_NONE, soundID, forceIndex);
}

bool __fastcall SndInterfacePlaySound(unsigned int soundID, const NTempest::C3Vector &position, int forceIndex, float volumeScaler) {
  return InternalPlaySound(SOUNDCATEGORY_NONE, soundID, position, forceIndex, volumeScaler);
}

bool __fastcall SndInterfacePlaySplashSound(unsigned int soundID, const NTempest::C3Vector &position) {
  return InternalPlaySound(SOUNDCATEGORY_SPLASHES, soundID, position, -1, 1.0f);
}

Sound *__fastcall SndInterfacePlayLoopedSound(unsigned int soundID, unsigned int loopCount) {
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

Sound *__fastcall SndInterfacePlayLoopedSound(unsigned int soundID, const NTempest::C3Vector &position, unsigned int loopCount) {
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

static bool __fastcall
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

void __fastcall SndInterfaceAssociateSoundWithObject(Sound *sound, CGObject_C *objectPtr) {
  if (objectPtr) {
    NTempest::C3Vector position = objectPtr->GetPosition();
    sound->SetPosition(position, 0);
    sound->Set3DUpdateHandle(objectPtr->GetGUID());
  } else {
    sound->Set3DUpdateHandle(0);
  }
}

Sound *__fastcall SndInterfaceCreateSound(unsigned int soundID, float fadeInRate, int forceIndex, bool doNotKeepAlive) {
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

void __fastcall SndInterfaceInitializeVocalUISounds(unsigned int race, unsigned int sex) {
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
    VocalUISoundsRec *rec = g_vocalUISoundsDB.GetRecordByIndex(i - 1);
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

void __fastcall SndInterfacePlayVocalUISound(VOCALUISOUNDS soundType) {
  CVar *masterSoundEffects = CVar::Lookup("MasterSoundEffects");
  bool  soundEffectsEnabled = masterSoundEffects && masterSoundEffects->m_intValue;
  CVar *enableErrorSpeech = CVar::Lookup("EnableErrorSpeech");

  if (!enableErrorSpeech || !enableErrorSpeech->m_intValue || soundType >= 66 || !soundEffectsEnabled) {
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

void __fastcall SndSetObstructionCallback(float(__fastcall *)(const NTempest::C3Vector &, const NTempest::C3Vector &callback)) {
}
