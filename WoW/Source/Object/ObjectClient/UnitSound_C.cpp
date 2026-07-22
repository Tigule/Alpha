#include "Object/ObjectClient/Unit_C.h"

#include "Client.h"
#include "DB/DBClient/AutoCode/CreatureSoundDataRec.h"
#include "DB/DBClient/AutoCode/CreatureModelDataRec.h"
#include "DB/DBClient/AutoCode/DeathThudLookupsRec.h"
#include "DB/DBClient/AutoCode/ItemSubClassRec.h"
#include "DB/DBClient/AutoCode/SoundEntriesRec.h"
#include "DB/DBClient/AutoCode/TerrainTypeRec.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "SoundInterface/SoundInterface.h"
#include "WorldClient/World.h"

struct DEATTHUDSOUNDINFO {
  unsigned int soundID;
  unsigned int soundIDWater;
};

static TSFixedArray<DEATTHUDSOUNDINFO> s_deathThudSounds[5];

static unsigned int s_unitSoundTimeouts[16] = {0, 0, 0, 0, 0, 0, 0, 10000, 0, 0, 0, 0, 0, 0, 0, 0};
static unsigned int s_unitSoundChances[16] = {70, 100, 60, 100, 100, 100, 40, 100, 100, 100, 100, 100, 100, 100, 100, 100};
static unsigned int s_unitSoundTimers[16];
static int          soundDataOffsets[16] = {4, 8, 12, 16, 24, 28, 32, 0, 36, 40, 44, 52, 20, 48, 100, 104};

static int __fastcall GetSoundID(CreatureSoundDataRec *soundData, UNITSOUNDTYPE soundType) {
  FATALASSERT(soundData);
  FATALASSERT(static_cast<unsigned int>(soundType) < 16);
  int offset = soundDataOffsets[soundType];
  return offset ? *reinterpret_cast<int *>(reinterpret_cast<unsigned char *>(soundData) + offset) : 0;
}

static int __fastcall CheckUnitPlaySound(UNITSOUNDTYPE soundType) {
  FATALASSERT(static_cast<unsigned int>(soundType) < 16);
  unsigned int random = NTempest::CRandom::uint32_(g_rndSeed);
  unsigned int value = static_cast<unsigned int>((static_cast<unsigned __int64>(101) * random) >> 32);
  return s_unitSoundChances[soundType] >= value;
}

static int __fastcall CheckUnitSoundTimer(UNITSOUNDTYPE soundType) {
  FATALASSERT(static_cast<unsigned int>(soundType) < 16);
  unsigned long currentTime = GetTickCount();
  int           canPlay = static_cast<long>(currentTime - s_unitSoundTimers[soundType]) > 0;
  s_unitSoundTimers[soundType] = currentTime + s_unitSoundTimeouts[soundType];
  return canPlay;
}

void __fastcall GenerateDeathThudSounds() {
  unsigned int maxTerrainFootstepID = g_terrainTypeDB.GetMaxID();
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

void __fastcall ClearDeathThudSounds() {
  for (unsigned int i = 0; i < 5; ++i) {
    s_deathThudSounds[i].SetCount(0);
  }
}

const ItemSubClassRec *__fastcall SDBItemSubclassGetSubClassRec(unsigned int classID, unsigned int subClassID);

void CGUnit_C::PlayDeathThud() const {
  NTempest::C3Vector pos;
  GetPosition(pos);

  NTempest::C3Vector flowDir;
  unsigned int       liquid;
  int                deep;
  float              surfaceIntersect;
  int                inLiquid = CWorld::QueryObjectLiquid(GetWorldObject(), liquid, surfaceIntersect, flowDir, deep);
  if (pos.z - surfaceIntersect > 2.0f) {
    return;
  }

  unsigned int size = GetUnitSize();
  int          terrainType = *reinterpret_cast<const int *>(reinterpret_cast<const unsigned char *>(this) + 0x4F0);
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

void CGUnit_C::PlaySpellLoopedSound(int soundID) {
  KillSpellLoopedSound();

  const SoundEntriesRec *soundRec = g_soundEntriesDB.GetRecord(soundID);
  if (!soundRec) {
    return;
  }

  Sound *&sound = *reinterpret_cast<Sound **>(reinterpret_cast<unsigned char *>(this) + 0x72C);
  sound = SndInterfaceCreateSound(soundRec->m_ID, 0.2f, -1, true);
  if (sound) {
    SndInterfaceAssociateSoundWithObject(sound, this);
  }
}

void CGUnit_C::KillSpellLoopedSound() {
  Sound *&sound = *reinterpret_cast<Sound **>(reinterpret_cast<unsigned char *>(this) + 0x72C);
  if (sound) {
    SndInterfaceAssociateSoundWithObject(sound, 0);
    sound->Stop(1.5f);
    sound = 0;
  }
}

int CGUnit_C::PlayNPCSound(NPCSOUNDS sound, unsigned int index) {
  FATALASSERT(sound < NUM_NPCSOUNDS);
  if (!m_soundData) {
    return 0;
  }

  NTempest::C3Vector position;
  GetPosition(position);
  position.z += 2.0f;
  const int *sounds = &m_soundData->m_soundExertionID;
  return SndInterfacePlaySound(sounds[sound], position, index, 1.0f);
}

void CGUnit_C::HandlePlayStandSound(unsigned long code, const char *eventName) {
  if (code == 0x58444624) {
    PlayStandSound();
    return;
  }

  FATALASSERT(eventName && *eventName);
  PlayFidgetSound(eventName[3] - '1');
}

void CGUnit_C::HandleFootfallAnimEvent(const NTempest::C3Vector &position) {
  if (m_soundData && m_soundData->m_soundFootstepID) {
    SndInterfacePlaySound(m_soundData->m_soundFootstepID, position, -1, 1.0f);
  }
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

void CGUnit_C::PlayStandSound() {
  if (GetGUID() != ClntObjMgrGetActivePlayer()) {
    PlayUnitSound(static_cast<UNITSOUNDTYPE>(6), 0);
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

bool CGUnit_C::GetWeaponSwingType(bool mainHand, WEAPONSWING_SOUNDTYPES &type) {
  unsigned int slot = !mainHand;
  void       **vtable = *reinterpret_cast<void ***>(this);

  typedef int (CGUnit_C::*GetVirtualItemDisplayFn)(unsigned int);
  GetVirtualItemDisplayFn getVirtualItemDisplay;
  memcpy(&getVirtualItemDisplay, &vtable[300 / 4], sizeof(getVirtualItemDisplay));
  if (!(this->*getVirtualItemDisplay)(slot)) {
    type = WEAPONSWING_LIGHT;
    return true;
  }

  typedef VirtualItemInfo *(CGUnit_C::*GetVirtualItemFn)(unsigned int, unsigned int);
  GetVirtualItemFn getVirtualItem;
  memcpy(&getVirtualItem, &vtable[296 / 4], sizeof(getVirtualItem));
  VirtualItemInfo *item = (this->*getVirtualItem)(slot, 0);
  if (!item || item->m_classID != 2) {
    return false;
  }

  const ItemSubClassRec *subClass = SDBItemSubclassGetSubClassRec(2, item->m_subclassID);
  if (subClass) {
    type = static_cast<WEAPONSWING_SOUNDTYPES>(subClass->m_WeaponSwingSize);
  }
  return subClass != 0;
}

VirtualItemInfo *CGUnit_C::GetParryingItem(unsigned int ignoreMainHand) {
  VirtualItemInfo *item = &m_unit->virtualItemInfo[0];
  if (!ignoreMainHand && item->m_classID == 2) {
    return item;
  }

  item = &m_unit->virtualItemInfo[1];
  if (item->m_classID != 2 && item->m_classID != 4) {
    return 0;
  }
  return item;
}

VirtualItemInfo *CGUnit_C::GetDefendingItem() {
  return GetParryingItem(0);
}

VirtualItemInfo *CGUnit_C::GetAttackingWeapon(COMBATHAND hand) {
  FATALASSERT(hand < NUMHANDS);
  VirtualItemInfo *item = &m_unit->virtualItemInfo[hand == COMBAT_OFFHAND];
  return item->m_classID == 2 ? item : 0;
}

unsigned int CGUnit_C::GetImpactType() {
  FATALASSERT(m_modelData);
  return m_modelData->m_sizeClass;
}

void CGUnit_C::PlayParrySound(unsigned int ignoreMainHand, ATTACKROUNDINFO *roundInfo, NTempest::C3Vector &position) {
  FATALASSERT(roundInfo);

  CGUnit_C        *attacker = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(roundInfo->attacker, __FILE__, __LINE__));
  VirtualItemInfo *attackingWeapon = attacker ? attacker->GetAttackingWeapon(static_cast<COMBATHAND>((roundInfo->flags >> 9) & 1)) : 0;
  VirtualItemInfo *defendingItem = GetParryingItem(ignoreMainHand);
  if (defendingItem) {
    SndInterfacePlayParrySound(attackingWeapon, defendingItem, roundInfo->flags & 8, position);
  }
}

void CGUnit_C::PlayImpactSound(unsigned __int64 attacker, int criticalHit, COMBATHAND hand) {
  CGUnit_C *attackerPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(attacker, __FILE__, __LINE__));
  if (!attackerPtr) {
    return;
  }

  NTempest::C3Vector position;
  GetPosition(position);
  SndInterfacePlayHitSound(attackerPtr->GetAttackingWeapon(hand), GetImpactType(), criticalHit, position);
}

void CGUnit_C::PlayCustomAttackSound(int sound, NTempest::C3Vector &position) {
  SndInterfacePlaySound(sound, position, -1, 1.0f);
}

void CGUnit_C::SetCustomAttackSound(int sound, const NTempest::C3Vector &position) {
  m_customAttackSound = sound;
  m_customAttackPosition = position;
}
