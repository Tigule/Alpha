#include <WowConst.h>
#include <MapDefs.h>

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
  UINT landSound;
  UINT waterSound;

  DEATTHUDSOUNDINFO() : landSound(0), waterSound(0) {
  }
};

static TSFixedArray<DEATTHUDSOUNDINFO> s_deathThudSounds[5];

static UINT  s_creatureIpactSounds[4] = {0, 8, 7, 9};
static UINT  s_unitSoundTimeouts[NUM_UNITSOUNDTYPES] = {0, 0, 0, 0, 0, 0, 0, 10000, 0, 0, 0, 0, 0, 0, 0, 0};
static UINT  s_unitSoundChances[NUM_UNITSOUNDTYPES] = {70, 100, 60, 100, 100, 100, 40, 100, 100, 100, 100, 100, 100, 100, 100, 100};
static UINT  s_unitSoundTimers[NUM_UNITSOUNDTYPES];
static CVar *s_footstepSoundCVar;
static int   soundDataOffsets[NUM_UNITSOUNDTYPES] = {4, 8, 12, 16, 24, 28, 32, 0, 36, 40, 44, 52, 20, 48, 100, 104};

int GetSoundID(const CreatureSoundDataRec *soundData, UNITSOUNDTYPE soundType) {
  FATALASSERT(soundData);
  FATALASSERT(soundType < NUM_UNITSOUNDTYPES);
  int offset = soundDataOffsets[soundType];
  return offset ? *reinterpret_cast<const int *>(reinterpret_cast<const BYTE *>(soundData) + offset) : 0;
}

int GetFidgetSoundID(const CreatureSoundDataRec *soundData, UINT soundType) {
  FATALASSERT(soundData);
  FATALASSERT(soundType < 4);
  return soundData->m_soundFidget[soundType];
}

static int CheckUnitPlaySound(UNITSOUNDTYPE soundType) {
  FATALASSERT(soundType < NUM_UNITSOUNDTYPES);
  UINT random = NTempest::CRandom::uint32_(g_rndSeed);
  UINT value = NTempest::CMath::mulhwu_(random, 101);
  return s_unitSoundChances[soundType] >= value;
}

void GenerateDeathThudSounds() {
  int                              maxTerrainFootstepID = g_terrainTypeSoundsDB.GetMaxID();
  UINT                             count = 5;
  TSFixedArray<DEATTHUDSOUNDINFO> *deathThudSounds = s_deathThudSounds;
  while (count) {
    deathThudSounds->SetCount(maxTerrainFootstepID + 1);
    ++deathThudSounds;
    --count;
  }

  count = g_deathThudLookupsDB.GetNumRecords();
  while (count) {
    const DeathThudLookupsRec *rec = g_deathThudLookupsDB.GetRecordByIndex(--count);
    FATALASSERT(rec);
    if (static_cast<UINT>(rec->m_SizeClass) < 5 && rec->m_TerrainTypeSoundID <= maxTerrainFootstepID) {
      s_deathThudSounds[rec->m_SizeClass][rec->m_TerrainTypeSoundID].landSound = rec->m_SoundEntryID;
      s_deathThudSounds[rec->m_SizeClass][rec->m_TerrainTypeSoundID].waterSound = rec->m_SoundEntryIDWater;
    }
  }
}

void ClearDeathThudSounds() {
  for (UINT i = 0; i < 5; ++i) {
    s_deathThudSounds[i].Clear();
  }
}

void UnitSoundShutdown() {
  ClearDeathThudSounds();
  SndInterfaceClearPositionCallback();
}

void UnitSoundInitialize() {
  GenerateDeathThudSounds();
  SndInterfaceSetPositionCallback();
  DWORD currentTime = OsGetAsyncTimeMs();
  for (UINT i = 0; i < NUM_UNITSOUNDTYPES; ++i) {
    s_unitSoundTimers[i] = currentTime;
  }
  s_footstepSoundCVar = CVar::Register("FootstepSounds", 0, 0, "1", 0, DEFAULT, false, 0);
}

int CheckUnitSoundTimer(UNITSOUNDTYPE soundType) {
  FATALASSERT(soundType < NUM_UNITSOUNDTYPES);
  DWORD currentTime = OsGetAsyncTimeMs();
  int   canPlay = 0;
  if (static_cast<long>(currentTime - s_unitSoundTimers[soundType]) > 0) {
    canPlay = 1;
  }
  s_unitSoundTimers[soundType] = currentTime + s_unitSoundTimeouts[soundType];
  return canPlay;
}

const ItemSubClassRec *SDBItemSubclassGetSubClassRec(UINT classID, UINT subClassID);

void CGUnit_C::HandlePlayStandSound(DWORD code, LPCSTR eventName) {
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

void CGUnit_C::PlayFidgetSound(UINT fidgetNumber) {
  int soundID = GetFidgetSoundID(GetSoundData(), fidgetNumber);
  if (soundID) {
    NTempest::C3Vector position = GetPosition();
    position.z += 2.0f;
    SndInterfacePlaySound(soundID, position, -1, 1.0f);
  }
}

void CGUnit_C::PlayUnitSound(UNITSOUNDTYPE soundType, int alwaysPlay) const {
  if (soundType != UNITSOUNDTYPE_FOOTFALL && (alwaysPlay || CheckUnitPlaySound(soundType)) && CheckUnitSoundTimer(soundType)) {
    int soundID = GetSoundID(GetSoundData(), soundType);
    if (soundID) {
      NTempest::C3Vector position = GetPosition();
      position.z += 2.0f;
      SndInterfacePlaySound(soundID, position, -1, 1.0f);
    }
  }
}

void CGUnit_C::PlayParrySound(bool ignoreMainHand, const ATTACKROUNDINFO *roundInfo, const NTempest::C3Vector &position) const {
  FATALASSERT(roundInfo);

  CGUnit_C              *attacker = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(roundInfo->attacker, __FILE__, __LINE__));
  COMBATHAND             attackingHand = (roundInfo->flags & 0x200) ? COMBAT_OFFHAND : COMBAT_MAINHAND;
  const VirtualItemInfo *attackingWeapon = attacker ? attacker->GetAttackingWeapon(attackingHand) : 0;
  const VirtualItemInfo *defendingItem = GetParryingItem(ignoreMainHand);
  if (defendingItem) {
    SndInterfacePlayParrySound(attackingWeapon, defendingItem, roundInfo->flags & 8, position);
  }
}

void CGUnit_C::PlayImpactSound(DWORDLONG attacker, int criticalHit, COMBATHAND hand) const {
  CGUnit_C *attackerPtr = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(attacker, __FILE__, __LINE__));
  if (!attackerPtr) {
    return;
  }

  SndInterfacePlayHitSound(attackerPtr->GetAttackingWeapon(hand), GetImpactType(), criticalHit, GetPosition());
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
    PlayUnitSound(UNITSOUNDTYPE_STAND, 0);
  }
}

void CGUnit_C::PlayDeathThud() const {
  NTempest::C3Vector flowDir(0.0f);
  NTempest::C3Vector pos = GetPosition();
  UINT               liquid;
  int                deep;
  float              surfaceIntersect;
  int                inLiquid = CWorld::QueryObjectLiquid(GetWorldObject(), liquid, surfaceIntersect, flowDir, deep);
  if (surfaceIntersect - pos.z <= 2.0f) {
    UINT size = GetUnitSize();
    if (size < 5) {
      int terrainType = m_terrain;
      if (terrainType < s_deathThudSounds[size].Count() && terrainType >= 0) {
        const TerrainTypeRec *terrain = g_terrainTypeDB.GetRecord(terrainType);
        if (terrain) {
          int soundID = inLiquid ? s_deathThudSounds[size][terrain->m_SoundID].waterSound : s_deathThudSounds[size][terrain->m_SoundID].landSound;
          SndInterfacePlaySound(soundID, GetPosition(), -1, 1.0f);
        }
      }
    }
  }
}

void CGUnit_C::PlaySplashSound(const NTempest::C3Vector &position) {
  SndInterfacePlaySplashSound(m_splashSoundID, position);
}

void CGUnit_C::PlayFoleySound() const {
  FATALASSERT(m_modelData);
  SndInterfacePlayFoleySound(m_modelData->m_foleyMaterialID, GetPosition());
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

void CGUnit_C::KillCreatureLoopSound() {
  if (m_creatureLoopSound) {
    SndInterfaceAssociateSoundWithObject(m_creatureLoopSound, 0);
    m_creatureLoopSound->Stop(1.5f);
    m_creatureLoopSound = 0;
  }
}

void CGUnit_C::InitializeLoopSound() {
  KillCreatureLoopSound();
  const CreatureSoundDataRec *soundData = GetSoundData();
  if (soundData && soundData->m_loopSoundID) {
    m_creatureLoopSound = SndInterfaceCreateSound(soundData->m_loopSoundID, 0.2f, -1, true);
    if (m_creatureLoopSound) {
      SndInterfaceAssociateSoundWithObject(m_creatureLoopSound, this);
    }
  }
}

int CGUnit_C::PlayNPCSound(NPCSOUNDS sound, UINT index) {
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
  int itemDisplay = GetVirtualItemDisplayID(!mainHand);
  if (!itemDisplay) {
    type = WEAPONSWING_LIGHT;
    return true;
  }

  const VirtualItemInfo *item = GetVirtualItem(!mainHand, 0);
  if (!item || item->m_classID != 2) {
    return false;
  }

  const ItemSubClassRec *subClass = SDBItemSubclassGetSubClassRec(2, item->m_subclassID);
  if (subClass) {
    type = static_cast<WEAPONSWING_SOUNDTYPES>(subClass->m_WeaponSwingSize);
  }
  return subClass != 0;
}

UINT CGUnit_C::GetImpactType() const {
  if (m_soundData && m_soundData->m_creatureImpactType < 4) {
    return s_creatureIpactSounds[m_soundData->m_creatureImpactType];
  }
  return 0;
}
