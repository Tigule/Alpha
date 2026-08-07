#include <WowConst.h>

#include "ISoundInterface.h"

#include "Client.h"
#include "DB/DBClient/DBClient.h"
#include "DB/DBClient/AutoCode/MaterialRec.h"
#include "DB/DBClient/AutoCode/SheatheSoundLookupsRec.h"
#include "DB/DBClient/AutoCode/SoundEntriesRec.h"
#include "DB/DBClient/AutoCode/SoundSamplePreferencesRec.h"
#include "DB/DBClient/AutoCode/WeaponImpactSoundsRec.h"
#include "DB/DBClient/AutoCode/WeaponSwingSounds2Rec.h"
#include "SoundInterface.h"

#include "Tempest/cmath.h"
#include "Tempest/crandom.h"

UINT g_sndInterfaceFlags;

TSHashTable<SHEATHSOUNDHASH, HASHKEY_NONE>        g_sheathSoundList;
TSFixedArray<IMPACTSOUNDARRAY>                    g_impactSounds;
WEAPONSOUNDS                                      g_weaponSwingSounds[3];
TSHashTable<UISOUNDLOOKUP, HASHKEY_STRI>          g_uiSoundLookups;
static HASHKEY_NONE                               s_nullHashKey;
static TSHashTable<SOUNDDEFINITION, HASHKEY_NONE> s_fileNameHash;
static UINT                                       s_numFileNameEntries;
static TSGrowableArray<REVERBINFO>                s_reverbTable;

static const UINT s_primes[272] = {
    1667U, 1669U, 1693U, 1697U, 647U,  653U,  659U,  661U,  1733U, 1741U, 1747U, 1753U, 673U,  677U,  683U,  691U,  701U,  709U,  719U,  727U,  1277U,
    1279U, 1283U, 1289U, 701U,  709U,  719U,  727U,  1583U, 1597U, 1601U, 1607U, 733U,  739U,  743U,  751U,  1481U, 1483U, 1487U, 1489U, 757U,  761U,
    769U,  773U,  1307U, 1319U, 1321U, 1327U, 983U,  991U,  997U,  1009U, 1307U, 1319U, 1321U, 1327U, 787U,  797U,  809U,  811U,  1399U, 1409U, 1423U,
    1427U, 821U,  823U,  827U,  829U,  1213U, 1217U, 1223U, 1229U, 839U,  853U,  857U,  859U,  1481U, 1483U, 1487U, 1489U, 863U,  877U,  881U,  883U,
    1559U, 1567U, 1571U, 1579U, 887U,  907U,  911U,  919U,  1583U, 1597U, 1601U, 1607U, 929U,  937U,  941U,  947U,  1583U, 1597U, 1601U, 1607U, 953U,
    967U,  971U,  977U,  1481U, 1483U, 1487U, 1489U, 983U,  991U,  997U,  1009U, 673U,  677U,  683U,  691U,  1013U, 1019U, 1021U, 1031U, 1033U, 1039U,
    1049U, 1051U, 701U,  709U,  719U,  727U,  1061U, 1063U, 1069U, 1087U, 1091U, 1093U, 1097U, 1103U, 1109U, 1117U, 1123U, 1129U, 757U,  761U,  769U,
    773U,  1151U, 1153U, 1163U, 1171U, 733U,  739U,  743U,  751U,  1181U, 1187U, 1193U, 1201U, 1213U, 1217U, 1223U, 1229U, 929U,  937U,  941U,  947U,
    1231U, 1237U, 1249U, 1259U, 1277U, 1279U, 1283U, 1289U, 983U,  991U,  997U,  1009U, 1291U, 1297U, 1301U, 1303U, 673U,  677U,  683U,  691U,  1307U,
    1319U, 1321U, 1327U, 1361U, 1367U, 1373U, 1381U, 1399U, 1409U, 1423U, 1427U, 1429U, 1433U, 1439U, 1447U, 1451U, 1453U, 1459U, 1471U, 787U,  797U,
    809U,  811U,  1481U, 1483U, 1487U, 1489U, 1493U, 1499U, 1511U, 1523U, 733U,  739U,  743U,  751U,  1531U, 1543U, 1549U, 1553U, 1559U, 1567U, 1571U,
    1579U, 733U,  739U,  743U,  751U,  1583U, 1597U, 1601U, 1607U, 1609U, 1613U, 1619U, 1621U, 887U,  907U,  911U,  919U,  1627U, 1637U, 1657U, 1663U,
    1361U, 1367U, 1373U, 1381U, 1667U, 1669U, 1693U, 1697U, 1699U, 1709U, 1721U, 1723U, 673U,  677U,  683U,  691U,  1733U, 1741U, 1747U, 1753U
};

static bool InitializePrefTable(int index);
static void ReadFiles();
static void InitializeInterfaceSounds();
static void InitializeUISounds();
static void InitializeSheatheSounds();
static void InitializeUnitCombatSounds();
static void GenerateWeaponSwingCombatSounds();
static void InitializeWeaponImpactCombatSounds();
static void ParseWeaponImpactArmorField(const WeaponImpactSoundsRec *rec);
UINT        BuildSoundFilesRec(TSCArray<FILENAMEENTRY, 10> &array, const SoundEntriesRec *rec, LPCSTR directory, int *equalFreqsPtr);

LPCSTR SOUNDDEFINITION::GetRandomFileName(int index) {
  UINT targetFreq;

  if (!m_fileNames.Count() || !m_totalFrequency) {
    return 0;
  }

  if (index == -1) {
    if (m_equalFreqs) {
      if (!(m_loopCounter++ % m_fileNames.Count())) {
        m_primeStepIndex = NTempest::CMath::mulhwu_(NTempest::CRandom::uint32_(g_rndSeed), sizeof(s_primes) / sizeof(s_primes[0]));
      }

      m_lastPlayed = (m_lastPlayed + s_primes[m_primeStepIndex]) % m_fileNames.Count();
    } else {
      targetFreq = NTempest::CMath::mulhwu_(NTempest::CRandom::uint32_(g_rndSeed), m_totalFrequency);
      m_lastPlayed = 0;
      while (targetFreq >= m_fileNames[m_lastPlayed].accumulatedFreq) {
        ++m_lastPlayed;
      }
    }
  } else {
    m_lastPlayed = m_fileNames.Count() - 1;
    if (m_lastPlayed >= static_cast<UINT>(index)) {
      m_lastPlayed = index;
    }
    m_loopCounter = 0;
  }

  return m_fileNames[m_lastPlayed].fileName;
}

SOUNDDEFINITION::SOUNDDEFINITION()
    : m_volume(1.0f),
      m_pitch(1.0f),
      m_pitchVariation(0.0f),
      m_priority(0),
      m_channel(0),
      m_flags(0),
      m_minDistance(27.777779f),
      m_maxDistance(55.555557f),
      m_distanceCutoffSquared(6944.4443f),
      m_totalFrequency(0),
      m_lastPlayed(0),
      m_loopCounter(0),
      m_primeStepIndex(0),
      m_equalFreqs(0),
      m_reverbPrefIndex(-1) {
}

SOUNDDEFINITION::SOUNDDEFINITION(const SOUNDDEFINITION &rhs)
    : m_volume(rhs.m_volume),
      m_pitch(rhs.m_pitch),
      m_pitchVariation(rhs.m_pitchVariation),
      m_priority(rhs.m_priority),
      m_channel(rhs.m_channel),
      m_flags(rhs.m_flags),
      m_minDistance(rhs.m_minDistance),
      m_maxDistance(rhs.m_maxDistance),
      m_distanceCutoffSquared(rhs.m_distanceCutoffSquared),
      m_lastPlayed(rhs.m_lastPlayed),
      m_loopCounter(rhs.m_loopCounter),
      m_primeStepIndex(rhs.m_primeStepIndex),
      m_equalFreqs(rhs.m_equalFreqs) {
  m_fileNames = rhs.m_fileNames;
}

const SOUNDDEFINITION &SOUNDDEFINITION::operator=(const SOUNDDEFINITION &rhs) {
  if (this != &rhs) {
    Clear();
    m_volume = rhs.m_volume;
    m_pitch = rhs.m_pitch;
    m_pitchVariation = rhs.m_pitchVariation;
    m_priority = rhs.m_priority;
    m_channel = rhs.m_channel;
    m_flags = rhs.m_flags;
    m_minDistance = rhs.m_minDistance;
    m_maxDistance = rhs.m_maxDistance;
    m_distanceCutoffSquared = rhs.m_distanceCutoffSquared;
    m_lastPlayed = rhs.m_lastPlayed;
    m_loopCounter = rhs.m_loopCounter;
    m_primeStepIndex = rhs.m_primeStepIndex;
    m_equalFreqs = rhs.m_equalFreqs;
    m_fileNames = rhs.m_fileNames;
  }

  return *this;
}

SOUNDDEFINITION::~SOUNDDEFINITION() {
  Clear();
}

void SOUNDDEFINITION::Clear() {
  m_fileNames.SetCount(0);
}

UINT BuildSoundFilesRec(TSCArray<FILENAMEENTRY, 10> &array, const SoundEntriesRec *rec, LPCSTR directory, int *equalFreqsPtr) {
  char           buff[MAX_PATH];
  int            lastFreq = 0;
  int            equalFreqs = 1;
  UINT           i;
  UINT           totalFreq = 0;
  LPCSTR         separator;
  LPCSTR         lastSlash;
  UINT           index;
  FILENAMEENTRY *newNode;

  for (i = 0; i < 10; ++i) {
    if (!rec->m_File[i][0] || !rec->m_Freq[i]) {
      continue;
    }

    separator = "";
    if (directory && *directory) {
      lastSlash = SStrChrR(directory, '\\');
      if (!lastSlash || lastSlash[1]) {
        separator = "\\";
      }
    }

    SStrPrintf(buff, sizeof(buff), "%s%s%s", directory, separator, rec->m_File[i]);

    index = array.Count();
    array.SetCount(index + 1);
    newNode = &array[index];
    ASSERT(newNode);

    totalFreq += rec->m_Freq[i];
    newNode->SetName(buff, totalFreq);
    if (equalFreqs && i && lastFreq != rec->m_Freq[i]) {
      equalFreqs = 0;
    }
    lastFreq = rec->m_Freq[i];
  }

  if (equalFreqsPtr) {
    *equalFreqsPtr = equalFreqs;
  }
  return totalFreq;
}

static void ReadFiles() {
  UINT                   numNewEntries = 0;
  UINT                   numEntries = g_soundEntriesDB.GetNumRecords();
  UINT                   i;
  const SoundEntriesRec *rec;
  SOUNDDEFINITION       *sound;

  for (i = 0; i < numEntries; ++i) {
    rec = g_soundEntriesDB.GetRecordByIndex(i);
    if (rec && !s_fileNameHash.Ptr(rec->m_ID, s_nullHashKey)) {
      ++numNewEntries;
    }
  }

  s_fileNameHash.SetTableSize(numNewEntries + s_numFileNameEntries);

  for (i = 0; i < numEntries; ++i) {
    rec = g_soundEntriesDB.GetRecordByIndex(i);
    if (!rec || s_fileNameHash.Ptr(rec->m_ID, s_nullHashKey)) {
      continue;
    }

    sound = s_fileNameHash.New(rec->m_ID, s_nullHashKey, 0, 0);
    sound->m_flags = rec->m_flags;
    sound->m_volume = rec->m_volumeFloat;
    sound->m_pitch = rec->m_pitch;
    sound->m_pitchVariation = rec->m_pitchVariation;
    sound->m_priority = rec->m_priority;
    sound->m_channel = rec->m_channel;
    sound->m_minDistance = rec->m_minDistance;
    sound->m_maxDistance = 10000.0f;
    sound->m_distanceCutoffSquared = rec->m_distanceCutoff * rec->m_distanceCutoff;
    sound->m_fileNames.SetCount(0);
    sound->m_totalFrequency = BuildSoundFilesRec(sound->m_fileNames, rec, rec->m_DirectoryBase, &sound->m_equalFreqs);
    sound->m_reverbPrefIndex = rec->m_EAXDef;
  }

  s_numFileNameEntries += numNewEntries;
}

static void InitializeUISounds() {
  UINT                   numEntries = g_soundEntriesDB.GetNumRecords();
  UINT                   i;
  const SoundEntriesRec *rec;
  UISOUNDLOOKUP         *lookup;

  for (i = 0; i < numEntries; ++i) {
    rec = g_soundEntriesDB.GetRecordByIndex(i);
    if (rec->m_soundType != 2) {
      continue;
    }

    lookup = g_uiSoundLookups.Ptr(rec->m_name);
    if (!lookup) {
      lookup = g_uiSoundLookups.New(rec->m_name, 0, 0);
    }
    lookup->soundID = rec->m_ID;
  }
}

static void InitializeSheatheSounds() {
  UINT                          numMaterials = g_materialDB.GetMaxID() + 1;
  UINT                          i;
  UINT                          j;
  const SheatheSoundLookupsRec *rec;
  SHEATHSOUNDHASH              *hash;

  for (i = g_sheatheSoundLookupsDB.GetNumRecords(); i; --i) {
    rec = g_sheatheSoundLookupsDB.GetRecordByIndex(i - 1);
    ASSERT(rec);

    hash = g_sheathSoundList.Ptr(rec->m_classID, s_nullHashKey);
    if (!hash) {
      hash = g_sheathSoundList.New(rec->m_classID, s_nullHashKey, 0, 0);
      hash->materialSheathSound.SetCount(numMaterials);
      hash->materialUnsheathSound.SetCount(numMaterials);

      for (j = 0; j < numMaterials; ++j) {
        hash->materialSheathSound[j] = 0;
        hash->materialUnsheathSound[j] = 0;
      }
    }

    if (rec->m_checkMaterial) {
      if (static_cast<UINT>(rec->m_material) < numMaterials) {
        hash->materialSheathSound[rec->m_material] = rec->m_sheatheSound;
        hash->materialUnsheathSound[rec->m_material] = rec->m_unsheatheSound;
      }
    } else {
      for (j = 0; j < numMaterials; ++j) {
        hash->materialSheathSound[j] = rec->m_sheatheSound;
        hash->materialUnsheathSound[j] = rec->m_unsheatheSound;
      }
    }
  }
}

static void InitializeInterfaceSounds() {
  InitializeUISounds();
  InitializeSheatheSounds();
}

static void GenerateWeaponSwingCombatSounds() {
  UINT                         i;
  const WeaponSwingSounds2Rec *rec;

  for (i = g_weaponSwingSounds2DB.GetNumRecords(); i; --i) {
    rec = g_weaponSwingSounds2DB.GetRecordByIndex(i - 1);
    if (rec && rec->m_SwingType < 3 && rec->m_Crit < 2) {
      g_weaponSwingSounds[rec->m_SwingType].soundList[rec->m_Crit != 0] = rec->m_SoundID;
    }
  }
}

static void ParseWeaponImpactArmorField(const WeaponImpactSoundsRec *rec) {
  UINT armor;

  ASSERT(rec);
  ASSERT(rec->m_WeaponSubClassID < static_cast<int>(ClientDBGetNumWeaponSubclasses()));
  ASSERT(rec->m_ParrySoundType < 2);

  if (!g_impactSounds.Count()) {
    return;
  }

  for (armor = 0; armor < 10; ++armor) {
    ASSERT(rec->m_WeaponSubClassID < static_cast<int>(ClientDBGetNumWeaponSubclasses()));
    WEAPONSOUNDS &sounds = g_impactSounds[rec->m_WeaponSubClassID].desc[armor].materialSounds[rec->m_ParrySoundType];
    sounds.soundList[0] = rec->m_impactSoundID[armor];
    sounds.soundList[1] = rec->m_critImpactSoundID[armor];
  }
}

static void InitializeWeaponImpactCombatSounds() {
  UINT                         i;
  const WeaponImpactSoundsRec *rec;

  g_impactSounds.SetCount(ClientDBGetNumWeaponSubclasses());
  for (i = g_weaponImpactSoundsDB.GetNumRecords(); i; --i) {
    rec = g_weaponImpactSoundsDB.GetRecordByIndex(i - 1);
    ASSERT(rec);
    ParseWeaponImpactArmorField(rec);
  }
}

static void InitializeUnitCombatSounds() {
  GenerateWeaponSwingCombatSounds();
  InitializeWeaponImpactCombatSounds();
}

void ISndInterfaceInitialize() {
  ISndInterfaceShutdown();
  ReadFiles();
  InitializeInterfaceSounds();
  InitializeUnitCombatSounds();
}

void ISndInterfaceShutdown() {
  s_fileNameHash.Clear();
  s_numFileNameEntries = 0;
  g_sheathSoundList.Clear();
  g_uiSoundLookups.Clear();
  s_reverbTable.Clear();
}

SOUNDDEFINITION *ISndInterfaceGetSndEntry(UINT soundID) {
  return s_fileNameHash.Ptr(soundID, s_nullHashKey);
}

static bool InitializePrefTable(int index) {
  const SoundSamplePreferencesRec  *rec = g_soundSamplePreferencesDB.GetRecord(index);
  _FSOUND_REVERB_CHANNELPROPERTIES *prefs;

  if (!rec) {
    return 0;
  }

  prefs = &s_reverbTable[index].prefs;
  prefs->Direct = rec->m_EAX2SampleDirect;
  prefs->DirectHF = rec->m_EAX2SampleDirectHF;
  prefs->Room = rec->m_EAX2SampleRoom;
  prefs->RoomHF = rec->m_EAX2SampleRoomHF;
  prefs->Obstruction = static_cast<int>(rec->m_EAX2SampleObstruction);
  prefs->ObstructionLFRatio = rec->m_EAX2SampleObstructionLFRatio;
  prefs->Occlusion = static_cast<int>(rec->m_EAX2SampleOcclusion);
  prefs->OcclusionLFRatio = rec->m_EAX2SampleOcclusionLFRatio;
  prefs->OcclusionRoomRatio = rec->m_EAX2SampleOcclusionRoomRatio;
  prefs->OcclusionDirectRatio = rec->m_EAX3SampleOcclusionDirectRatio;
  prefs->Exclusion = static_cast<int>(rec->m_EAX3SampleExclusion);
  prefs->ExclusionLFRatio = rec->m_EAX3SampleExclusionLFRatio;
  prefs->OutsideVolumeHF = rec->m_EAX2SampleOutsideVolumeHF;
  prefs->DopplerFactor = rec->m_EAX3SampleDopplerFactor;
  prefs->RoomRolloffFactor = rec->m_EAX2SampleRoomRolloff;
  prefs->AirAbsorptionFactor = rec->m_EAX2SampleAirAbsorption;
  prefs->RolloffFactor = 0.0f;
  prefs->Flags = 0;

  s_reverbTable[index].inUse = 1;
  return 1;
}

_FSOUND_REVERB_CHANNELPROPERTIES *GetReverbType(int index) {
  int maxID = g_soundSamplePreferencesDB.GetMaxID();

  if (index < 0 || index > maxID) {
    return 0;
  }

  s_reverbTable.SetCount(maxID + 1);
  if (!s_reverbTable[index].inUse && !InitializePrefTable(index)) {
    return 0;
  }

  return &s_reverbTable[index].prefs;
}
