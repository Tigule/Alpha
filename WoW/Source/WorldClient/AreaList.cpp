#include <Base/Base.h>
#include <WowConst.h>
#include <MapDefs.h>

#include "AreaList.h"
#include "AreaListHashKey.h"
#include "World.h"

#include <Base/CDataStore.h>
#include <Console/ConsoleCommand.h>
#include <DB/DBClient/AutoCode/AreaTableRec.h>
#include <DB/DBClient/AutoCode/WMOAreaTableRec.h>
#include <Net/NetClient/NetClient.h>
#include <SoundInterface/SoundInterface.h>
#include <Ui/GameUI.h>
#include <WowSvcs/WowSvcsClient/ClientServices.h>
#include <storm.h>

static TSHashTable<AREAHASHOBJECT, AREAHASHKEY> s_areaHash;
static UINT                                     s_currentZoneID = -1;
static UINT                                     s_currentSubZoneID = -1;
static UINT                                     s_currentContinent = -1;
static int                                      s_indoors = -1;
static UINT                                     s_currentChunkID = -1;

static AREAHASHOBJECT *GetZone(UINT cont, UINT zName, UINT subZone) {
  AREAHASHKEY key(cont, zName, subZone);

  return s_areaHash.Ptr(zName, key);
}

AREAHASHOBJECT *AREAHASHOBJECT::GetParent() const {
  if (!subArea) {
    return 0;
  }

  AREAHASHKEY key(continent, area, 0);

  return s_areaHash.Ptr(area, key);
}

static void InitializeAreaMusic(AREAHASHOBJECT *c) {
  const AreaTableRec *rec = c->rec;

  c->midi = rec->m_MIDIAmbience;
  c->midiUnderwater = rec->m_MIDIAmbienceUnderwater;
  c->zoneMusic = rec->m_ZoneMusic;
  c->reverb = rec->m_SoundProviderPref;
  c->reverbUnderwater = rec->m_SoundProviderPrefUnderwater;
  c->zoneIntroID = rec->m_IntroSound;
  c->zoneIntroIDPriority = rec->m_IntroPriority;

  for (AREAHASHOBJECT *parent = c->GetParent(); parent; parent = parent->GetParent()) {
    const AreaTableRec *parentRec = parent->rec;

    if (!c->midi) {
      c->midi = parentRec->m_MIDIAmbience;
    }
    if (!c->midiUnderwater) {
      c->midiUnderwater = parentRec->m_MIDIAmbienceUnderwater;
    }
    if (!c->zoneMusic) {
      c->zoneMusic = parentRec->m_ZoneMusic;
    }
    if (!c->reverb) {
      c->reverb = parentRec->m_SoundProviderPref;
    }
    if (!c->reverbUnderwater) {
      c->reverbUnderwater = parentRec->m_SoundProviderPrefUnderwater;
    }
    if (!c->zoneIntroID) {
      c->zoneIntroID = parentRec->m_IntroSound;
    }
    if (!c->zoneIntroIDPriority) {
      c->zoneIntroIDPriority = parentRec->m_IntroPriority;
    }

    if (c->midi && c->midiUnderwater && c->zoneMusic && c->reverb && c->reverbUnderwater && c->zoneIntroID && c->zoneIntroIDPriority) {
      break;
    }
  }
}

static void InitializeMusic() {
  ITERATELIST(AREAHASHOBJECT, s_areaHash, area) {
    InitializeAreaMusic(area);
  }
}

static void LoadAreaTable() {
  UINT numEntries = g_areaTableDB.GetNumRecords();

  s_areaHash.SetTableSize(numEntries);

  for (UINT i = 0; i < numEntries; ++i) {
    const AreaTableRec *rec = g_areaTableDB.GetRecordByIndex(i);
    UINT                continentID = rec->m_ContinentID;
    UINT                areaID = rec->m_AreaNumber >> 16;
    UINT                subArea = rec->m_AreaNumber & 0xFFFF;
    AREAHASHKEY         key(continentID, areaID, subArea);

    AREAHASHOBJECT *area = s_areaHash.Ptr(areaID, key);
    if (!area) {
      area = s_areaHash.New(areaID, key, 0, 0);
      area->rec = rec;
      area->continent = continentID;
      area->area = areaID;
      area->subArea = subArea;
    }
  }

  InitializeMusic();
}

static int MIDISetHandler(LPCSTR command, LPCSTR arguments) {
  UINT            enabled = SStrToUnsigned(arguments);
  AREAHASHOBJECT *zone = GetZone(s_currentContinent, s_currentZoneID, s_currentSubZoneID);
  if (enabled && zone) {
    SndInterfaceSetMIDIArea(zone->midi, zone->midiUnderwater);
  } else {
    SndInterfaceClearMIDI();
  }
  return 1;
}

void AreaListInitialize() {
  LoadAreaTable();
  s_currentContinent = 0;
  ConsoleCommandRegister("midiset", MIDISetHandler, DEBUG, 0);
}

void AreaListShutdown() {
  s_indoors = -1;
  ConsoleCommandUnregister("midiset");
  s_currentContinent = -1;
  s_areaHash.Clear();
}

int AreaListGetName(UINT continentID, UINT areaID, UINT subAreaID, char *buffer, UINT size, int fullName) {
  AREAHASHOBJECT *area;
  AREAHASHOBJECT *parent;
  int             badRec;

  if (!buffer || !size) {
    return 0;
  }

  buffer[0] = 0;
  area = GetZone(continentID, areaID, subAreaID);

  if (!area || !SMemIsValidPointer(area, sizeof(*area), FALSE)) {
    return 0;
  }

  badRec = !area->rec || !SMemIsValidPointer(area->rec, sizeof(*area->rec), FALSE);
  if (badRec) {
    ASSERT(!badRec);
    return 0;
  }

  SStrCopy(buffer, area->rec->m_AreaName_lang[CURRENT_LANGUAGE], size);

  if (fullName && subAreaID) {
    parent = area->GetParent();
    if (parent && SMemIsValidPointer(area, sizeof(*area), FALSE)) {
      badRec = !parent->rec || !SMemIsValidPointer(parent->rec, sizeof(*parent->rec), FALSE);
      if (badRec) {
        ASSERT(!badRec);
        return 0;
      }

      SStrPack(buffer, ", ", size);
      SStrPack(buffer, parent->rec->m_AreaName_lang[CURRENT_LANGUAGE], size);
    }
  }

  return 1;
}

static void SendZoneUpdate(AREAHASHOBJECT *hash) {
  if (hash->rec) {
    CDataStore msg;
    msg.Put(CMSG_ZONEUPDATE);
    msg.Put(hash->rec->m_ID);
    msg.Finalize();
    ClientServices_Send(&msg);
  }
}

static bool HandleIndoorZoneChange(DWORD worldObject, LPCSTR &zoneName, LPCSTR &subZoneName, bool &clearMusic) {
  const WMOAreaTableRec *globalRec = 0;
  const WMOAreaTableRec *rec = 0;
  LPCSTR                 szName = 0;
  LPCSTR                 zName = 0;
  UINT                   chunk = 0;

  CWorld::QueryMapObjZoneName(worldObject, zName);
  CWorld::QueryMapObjSubzoneName(worldObject, szName, chunk);

  if (s_indoors > 0 && chunk == s_currentChunkID) {
    return false;
  }

  s_currentChunkID = chunk;

  if (zName && *zName) {
    zoneName = zName;
  }
  if (szName && *szName) {
    subZoneName = szName;
  }
  if (!zName || !*zName) {
    zoneName = 0;
  }

  if (!CWorld::QueryMapObjAreaTable(worldObject, rec, globalRec)) {
    clearMusic = true;
    return true;
  }

  int musicID = rec && rec->m_ZoneMusic ? rec->m_ZoneMusic : globalRec ? globalRec->m_ZoneMusic : 0;
  int m = rec && rec->m_MIDIAmbience ? rec->m_MIDIAmbience : globalRec ? globalRec->m_MIDIAmbience : 0;
  int mu = rec && rec->m_MIDIAmbienceUnderwater ? rec->m_MIDIAmbienceUnderwater : globalRec ? globalRec->m_MIDIAmbienceUnderwater : 0;
  int p = rec && rec->m_SoundProviderPref ? rec->m_SoundProviderPref : globalRec ? globalRec->m_SoundProviderPref : 0;
  int pu = rec && rec->m_SoundProviderPrefUnderwater ? rec->m_SoundProviderPrefUnderwater : globalRec ? globalRec->m_SoundProviderPrefUnderwater : 0;
  int priority = rec && rec->m_IntroSound ? rec->m_IntroPriority : globalRec && globalRec->m_IntroSound ? globalRec->m_IntroPriority : 0;
  int introSound = rec && rec->m_IntroSound ? rec->m_IntroSound : globalRec ? globalRec->m_IntroSound : 0;

  SndInterfaceRegisterNewZone(musicID);
  SndInterfaceSetMIDIArea(m, mu);
  SndInterfaceSetProviderPrefs(p, pu, 2000);
  SndInterfaceRegisterNewZoneIntro(introSound, priority);

  return true;
}

static bool HandleOutdoorZoneChange(UINT zoneID, UINT subZoneID, UINT continent, bool &clearMusic) {
  if (!s_indoors && zoneID == s_currentZoneID && subZoneID == s_currentSubZoneID && continent == s_currentContinent) {
    return false;
  }

  s_currentZoneID = zoneID;
  s_currentSubZoneID = subZoneID;
  s_currentContinent = continent;

  AREAHASHOBJECT *hash = GetZone(continent, zoneID, subZoneID);
  if (!hash) {
    clearMusic = true;
    return true;
  }

  SendZoneUpdate(hash);
  SndInterfaceRegisterNewZone(hash->zoneMusic);
  SndInterfaceSetMIDIArea(hash->midi, hash->midiUnderwater);
  SndInterfaceSetProviderPrefs(hash->reverb, hash->reverbUnderwater, 2000);
  SndInterfaceRegisterNewZoneIntro(hash->zoneIntroID, hash->zoneIntroIDPriority);
  return true;
}

void AreaListRegisterLocation(const NTempest::C3Vector &location, UINT continent, DWORD worldObject) {
  FATALASSERT(worldObject);

  int    indoors = CWorld::QueryObjectInside(worldObject) != 0;
  UINT   areaID = CWorld::QueryAreaId(location.x, location.y);
  UINT   zoneID = areaID >> 16;
  UINT   subZoneID = areaID & 0xFFFF;
  int    parentAreaID = 0;
  LPCSTR zoneName = 0;
  LPCSTR subZoneName = 0;

  AREAHASHOBJECT *hash = GetZone(continent, zoneID, subZoneID);
  if (hash) {
    subZoneName = hash->rec->m_AreaName_lang[CURRENT_LANGUAGE];

    AREAHASHOBJECT *parent = hash->GetParent();
    if (parent) {
      zoneName = parent->rec->m_AreaName_lang[CURRENT_LANGUAGE];
      parentAreaID = parent->rec->m_ID;
    } else {
      zoneName = hash->rec->m_AreaName_lang[CURRENT_LANGUAGE];
      subZoneName = 0;
      parentAreaID = hash->rec->m_ID;
    }
  }

  if (indoors != s_indoors) {
    SndDebugDungeonTransition(indoors, continent);
  }

  bool clearMusic = false;
  bool changed;
  if (indoors) {
    changed = HandleIndoorZoneChange(worldObject, zoneName, subZoneName, clearMusic);
  } else {
    changed = HandleOutdoorZoneChange(zoneID, subZoneID, continent, clearMusic);
  }

  if (!changed) {
    return;
  }

  CGGameUI::SetMinimapZoneText(subZoneName ? subZoneName : zoneName);

  if (!zoneName || !*zoneName) {
    zoneName = 0;
  }
  if (!subZoneName || !*subZoneName) {
    subZoneName = 0;
  }
  CGGameUI::NewZoneFeedback(parentAreaID, zoneName, subZoneName);

  if (clearMusic) {
    SndInterfaceRegisterNewZone(0);
    SndInterfaceSetMIDIArea(0, 0);
    SndInterfaceClearProviderPrefs(indoors);
  }

  s_indoors = indoors;
}

int AreaListZoneHasBreathParticles(DWORD worldObject, UINT continentID, const NTempest::C3Vector &position) {
  const WMOAreaTableRec *globalRec;

  if (CWorld::QueryObjectInside(worldObject)) {
    const WMOAreaTableRec *rec;
    if (CWorld::QueryMapObjAreaTable(worldObject, rec, globalRec)) {
      if (rec && (rec->m_Flags & 2)) {
        return rec->m_Flags & 1;
      }
      if (globalRec && (globalRec->m_Flags & 1)) {
        return 1;
      }
    }

    return 0;
  }

  UINT            areaID = CWorld::QueryAreaId(position.x, position.y);
  AREAHASHOBJECT *hash = GetZone(continentID, areaID >> 16, areaID & 0xFFFF);
  if (!hash) {
    return 0;
  }

  if (hash->rec && (hash->rec->m_flags & 1)) {
    return 1;
  }

  AREAHASHOBJECT *parent = hash->GetParent();
  return parent && parent->rec && (parent->rec->m_flags & 1);
}
