#include "Object/ObjectClient/Unit_C.h"

#include "Client.h"
#include "Console/ConsoleCommand.h"
#include "Console/ConsoleVar.h"
#include "DB/DBClient/AutoCode/CreatureSoundDataRec.h"
#include "DB/DBClient/AutoCode/CreatureModelDataRec.h"
#include "DB/DBClient/AutoCode/DeathThudLookupsRec.h"
#include "DB/DBClient/AutoCode/ItemSubClassRec.h"
#include "DB/DBClient/AutoCode/NPCSoundsRec.h"
#include "DB/DBClient/AutoCode/SoundEntriesRec.h"
#include "DB/DBClient/AutoCode/TerrainTypeRec.h"
#include "DB/DBClient/AutoCode/TerrainTypeSoundsRec.h"
#include "Object/ObjectClient/Player_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "SoundInterface/SoundInterface.h"
#include "WorldClient/World.h"

#include <Os/OsTime.h>

struct DEATTHUDSOUNDINFO {
  unsigned int soundID;
  unsigned int soundIDWater;
};

static TSFixedArray<DEATTHUDSOUNDINFO> s_deathThudSounds[5];

static unsigned int s_creatureIpactSounds[4] = {0, 8, 7, 9};
static unsigned int s_unitSoundTimeouts[16] = {0, 0, 0, 0, 0, 0, 0, 10000, 0, 0, 0, 0, 0, 0, 0, 0};
static unsigned int s_unitSoundChances[16] = {70, 100, 60, 100, 100, 100, 40, 100, 100, 100, 100, 100, 100, 100, 100, 100};
static unsigned int s_unitSoundTimers[16];
static CVar        *s_footstepSoundCVar;
static int          soundDataOffsets[16] = {4, 8, 12, 16, 24, 28, 32, 0, 36, 40, 44, 52, 20, 48, 100, 104};

int GetSoundID(const CreatureSoundDataRec *soundData, UNITSOUNDTYPE soundType) {
  FATALASSERT(soundData);
  FATALASSERT(soundType < NUM_UNITSOUNDTYPES);
  int offset = soundDataOffsets[soundType];
  return offset ? *reinterpret_cast<const int *>(reinterpret_cast<const unsigned char *>(soundData) + offset) : 0;
}

int GetFidgetSoundID(const CreatureSoundDataRec* soundData, unsigned int soundType) {
  FATALASSERT(soundData);
  FATALASSERT(soundType < 4);
  return soundData->m_soundFidget[soundType];
}

static int CheckUnitPlaySound(UNITSOUNDTYPE soundType) {
  FATALASSERT(soundType < NUM_UNITSOUNDTYPES);
  unsigned int random = NTempest::CRandom::uint32_(g_rndSeed);
  unsigned int value = static_cast<unsigned int>((static_cast<unsigned __int64>(101) * random) >> 32);
  return s_unitSoundChances[soundType] >= value;
}

void GenerateDeathThudSounds() {
  unsigned int maxTerrainFootstepID = g_terrainTypeSoundsDB.GetMaxID();
  for (unsigned int i = 0; i < 5; ++i) {
    s_deathThudSounds[i].SetCount(maxTerrainFootstepID + 1);
  }

  unsigned int count = g_deathThudLookupsDB.GetNumRecords();
  while (count) {
    const DeathThudLookupsRec *rec = g_deathThudLookupsDB.GetRecordByIndex(--count);
    if (static_cast<unsigned int>(rec->m_SizeClass) < 5 && rec->m_TerrainTypeSoundID <= maxTerrainFootstepID) {
      DEATTHUDSOUNDINFO &sound = s_deathThudSounds[rec->m_SizeClass][rec->m_TerrainTypeSoundID];
      sound.soundID = rec->m_SoundEntryID;
      sound.soundIDWater = rec->m_SoundEntryIDWater;
    }
  }
}

void ClearDeathThudSounds() {
  for (unsigned int i = 0; i < 5; ++i) {
    s_deathThudSounds[i].SetCount(0);
  }
}

void UnitSoundShutdown() {
  ClearDeathThudSounds();
  SndInterfaceClearPositionCallback();
}

void UnitSoundInitialize() {
  GenerateDeathThudSounds();
  SndInterfaceSetPositionCallback();
  unsigned long currentTime = OsGetAsyncTimeMs();
  for (unsigned int i = 0; i < 16; ++i) {
    s_unitSoundTimers[i] = currentTime;
  }
  s_footstepSoundCVar = CVar::Register("FootstepSounds", 0, 0, "1", 0, DEFAULT, false, 0);
}

int CheckUnitSoundTimer(UNITSOUNDTYPE soundType) {
  FATALASSERT(soundType < NUM_UNITSOUNDTYPES);
  unsigned long currentTime = OsGetAsyncTimeMs();
  int           canPlay = static_cast<long>(currentTime - s_unitSoundTimers[soundType]) > 0;
  s_unitSoundTimers[soundType] = currentTime + s_unitSoundTimeouts[soundType];
  return canPlay;
}

const ItemSubClassRec *SDBItemSubclassGetSubClassRec(unsigned int classID, unsigned int subClassID);

void CGUnit_C::HandlePlayStandSound(unsigned long code, const char *eventName) {
  if (code == 0x58444624) {
    PlayStandSound();
    return;
  }

  FATALASSERT(eventName && *eventName);
  PlayFidgetSound(eventName[3] - '1');
}

void CGUnit_C::HandleFootfallAnimEvent(const NTempest::C3Vector &position) {
  const CreatureSoundDataRec *soundData = GetSoundData();
  int                         soundID = GetSoundID(soundData, UNITSOUNDTYPE_FOOTFALL);
  if (soundID && s_footstepSoundCVar->GetInt()) {
    int splashing = IsSplashing(position);
    SndInterfacePlayFootstepSound(soundID, position, m_terrain, splashing);
  }
  PlayFoleySound();
}

void CGUnit_C::PlayFidgetSound(unsigned int fidgetNumber) {
  FATALASSERT(m_soundData);
  FATALASSERT(fidgetNumber < 4);
  int soundID = m_soundData->m_soundFidget[fidgetNumber];
  if (soundID) {
    NTempest::C3Vector position;
    GetPosition(position);
    position.z += 2.0f;
    SndInterfacePlaySound(soundID, position, -1, 1.0f);
  }
}

void CGUnit_C::PlayUnitSound(UNITSOUNDTYPE soundType, int alwaysPlay) const {
  if (soundType == 8 || (!alwaysPlay && !CheckUnitPlaySound(soundType))) {
    return;
  }
  if (!CheckUnitSoundTimer(soundType)) {
    return;
  }

  int soundID = GetSoundID(m_soundData, soundType);
  if (!soundID) {
    return;
  }

  NTempest::C3Vector position;
  GetPosition(position);
  position.z += 2.0f;
  SndInterfacePlaySound(soundID, position, -1, 1.0f);
}

void CGUnit_C::PlayParrySound(bool ignoreMainHand, const ATTACKROUNDINFO *roundInfo, const NTempest::C3Vector &position) const {
  FATALASSERT(roundInfo);

  CGUnit_C        *attacker = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(roundInfo->attacker, __FILE__, __LINE__));
  const VirtualItemInfo *attackingWeapon = attacker ? attacker->GetAttackingWeapon(static_cast<COMBATHAND>((roundInfo->flags >> 9) & 1)) : 0;
  const VirtualItemInfo *defendingItem = GetParryingItem(ignoreMainHand);
  if (defendingItem) {
    SndInterfacePlayParrySound(attackingWeapon, defendingItem, roundInfo->flags & 8, position);
  }
}

void CGUnit_C::PlayImpactSound(unsigned __int64 attacker, int criticalHit, COMBATHAND hand) const {
  CGUnit_C *attackerPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(attacker, __FILE__, __LINE__));
  if (!attackerPtr) {
    return;
  }

  NTempest::C3Vector position;
  GetPosition(position);
  SndInterfacePlayHitSound(attackerPtr->GetAttackingWeapon(hand), GetImpactType(), criticalHit, position);
}

void CGUnit_C::PlayCustomAttackSound(int sound, const NTempest::C3Vector &position) {
  SndInterfacePlaySound(sound, position, -1, 1.0f);
}

void CGUnit_C::SetCustomAttackSound(int sound, const NTempest::C3Vector &position) {
  m_customAttackSound = sound;
  m_customAttackPosition = position;
}

void CGUnit_C::PlayStandSound() const {
  if (GetGUID() != ClntObjMgrGetActivePlayer()) {
    PlayUnitSound(static_cast<UNITSOUNDTYPE>(6), 0);
  }
}

void CGUnit_C::PlayDeathThud() const {
  NTempest::C3Vector pos;
  GetPosition(pos);

  NTempest::C3Vector flowDir(0.0f);
  unsigned int       liquid;
  int                deep;
  float              surfaceIntersect;
  int                inLiquid = CWorld::QueryObjectLiquid(GetWorldObject(), liquid, surfaceIntersect, flowDir, deep);
  if (surfaceIntersect - pos.z > 2.0f) {
    return;
  }

  unsigned int size = GetUnitSize();
  int          terrainType = m_terrain;
  if (size >= 5 || terrainType < 0 || terrainType >= static_cast<int>(s_deathThudSounds[size].Count())) {
    return;
  }

  const TerrainTypeRec *terrain = g_terrainTypeDB.GetRecord(terrainType);
  if (!terrain) {
    return;
  }

  const DEATTHUDSOUNDINFO &sound = s_deathThudSounds[size][terrain->m_SoundID];
  GetPosition(pos);
  SndInterfacePlaySound(inLiquid ? sound.soundIDWater : sound.soundID, pos, -1, 1.0f);
}

void CGUnit_C::PlaySplashSound(const NTempest::C3Vector &position) {
  SndInterfacePlaySplashSound(m_splashSoundID, position);
}

void CGUnit_C::PlayFoleySound() const {
  FATALASSERT(m_modelData);
  SndInterfacePlayFoleySound(m_modelData->m_foleyMaterialID, GetPosition());
}

const VirtualItemInfo *CGUnit_C::GetParryingItem(bool ignoreMainHand) const {
  const VirtualItemInfo *item = &m_unit->virtualItemInfo[0];
  if (!ignoreMainHand && item->m_classID == 2) {
    return item;
  }

  item = &m_unit->virtualItemInfo[1];
  if (item->m_classID != 2 && item->m_classID != 4) {
    return 0;
  }
  return item;
}

const VirtualItemInfo *CGUnit_C::GetDefendingItem() const {
  return GetParryingItem(0);
}

const VirtualItemInfo *CGUnit_C::GetAttackingWeapon(COMBATHAND hand) const {
  FATALASSERT(hand < NUMHANDS);
  const VirtualItemInfo *item = &m_unit->virtualItemInfo[hand == COMBAT_OFFHAND];
  return item->m_classID == 2 ? item : 0;
}

void CGUnit_C::PlaySpellLoopedSound(int soundID) {
  KillSpellLoopedSound();

  const SoundEntriesRec *soundRec = g_soundEntriesDB.GetRecord(soundID);
  if (!soundRec) {
    return;
  }

  m_spellLoopedSound = SndInterfaceCreateSound(soundRec->m_ID, 0.2f, -1, true);
  if (m_spellLoopedSound) {
    SndInterfaceAssociateSoundWithObject(m_spellLoopedSound, this);
  }
}

void CGUnit_C::KillSpellLoopedSound() {
  if (m_spellLoopedSound) {
    SndInterfaceAssociateSoundWithObject(m_spellLoopedSound, 0);
    m_spellLoopedSound->Stop(1.5f);
    m_spellLoopedSound = 0;
  }
}

int CGUnit_C::PlayNPCSound(NPCSOUNDS sound, unsigned int index) {
  FATALASSERT(sound < NUM_NPCSOUNDS);
  if (!m_NPCSoundsRec) {
    return 0;
  }

  NTempest::C3Vector position;
  GetPosition(position);
  position.z += 2.0f;
  return SndInterfacePlaySound(m_NPCSoundsRec->m_SoundID[sound], position, index, 1.0f);
}

bool CGUnit_C::GetWeaponSwingType(bool mainHand, WEAPONSWING_SOUNDTYPES &type) {
  unsigned int slot = !mainHand;
  int          itemDisplay = GetVirtualItemDisplayID(slot);
  if (!itemDisplay) {
    type = WEAPONSWING_LIGHT;
    return true;
  }

  const VirtualItemInfo *item = GetVirtualItem(slot, 0);
  if (!item || item->m_classID != 2) {
    return false;
  }

  const ItemSubClassRec *subClass = SDBItemSubclassGetSubClassRec(2, item->m_subclassID);
  if (subClass) {
    type = static_cast<WEAPONSWING_SOUNDTYPES>(subClass->m_WeaponSwingSize);
  }
  return subClass != 0;
}

unsigned int CGUnit_C::GetImpactType() const {
  if (m_soundData && m_soundData->m_creatureImpactType < 4) {
    return s_creatureIpactSounds[m_soundData->m_creatureImpactType];
  }
  return 0;
}
