#include "Base/Base.h"
#include "Gx/Gx.h"
#include "Services/ParticleSystem2.h"
#include <WowConst.h>
#include "AaBsp.h"
#include <MapDefs.h>

#include "WorldClient/World.h"
#include "WorldClient/Map.h"
#include "WorldClient/WorldParam.h"
#include "WorldClient/DetailDoodad.h"
#include "WorldClient/CSimpleDoodad.h"
#include "DayNight.h"

#include "AreaList.h"
#include "AreaListHashKey.h"

#include <Base/CDataStore.h>
#include <Console/ConsoleCommand.h>
#include <DB/DBClient/AutoCode/AreaTableRec.h>
#include <DB/DBClient/AutoCode/WMOAreaTableRec.h>
#include <SoundInterface/SoundInterface.h>
#include <Net/NetClient/NetClient.h>
#include <Ui/GameUI.h>
#include <WowSvcs/WowSvcsClient/ClientServices.h>
#include <storm.h>

static UINT                                     s_flags;
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
  c->midi = c->rec->m_MIDIAmbience;
  c->midiUnderwater = c->rec->m_MIDIAmbienceUnderwater;
  c->zoneMusic = c->rec->m_ZoneMusic;
  c->reverb = c->rec->m_SoundProviderPref;
  c->reverbUnderwater = c->rec->m_SoundProviderPrefUnderwater;
  c->zoneIntroID = c->rec->m_IntroSound;
  c->zoneIntroIDPriority = c->rec->m_IntroPriority;
  if (c->midi && c->midiUnderwater && c->zoneMusic && c->reverb && c->reverbUnderwater) {
    return;
  }

  AREAHASHOBJECT *parent = c->GetParent();
  while (parent) {
    if (!c->midi) {
      c->midi = parent->rec->m_MIDIAmbience;
    }
    if (!c->midiUnderwater) {
      c->midiUnderwater = parent->rec->m_MIDIAmbienceUnderwater;
    }
    if (!c->zoneMusic) {
      c->zoneMusic = parent->rec->m_ZoneMusic;
    }
    if (!c->reverb) {
      c->reverb = parent->rec->m_SoundProviderPref;
    }
    if (!c->reverbUnderwater) {
      c->reverb = parent->rec->m_SoundProviderPrefUnderwater;
    }
    if (!c->zoneIntroID) {
      c->zoneIntroID = parent->rec->m_IntroSound;
    }
    if (!c->zoneIntroIDPriority) {
      c->zoneIntroIDPriority = parent->rec->m_IntroPriority;
    }

    if (c->midi && c->midiUnderwater && c->zoneMusic && c->reverb && c->reverbUnderwater) {
      break;
    }
    parent = parent->GetParent();
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
    UINT                id = rec->m_AreaNumber;
    UINT                areaID = id >> 16;
    UINT                subArea = id & 0xFFFF;
    AREAHASHKEY         key(continentID, areaID, subArea);

    AREAHASHOBJECT *area = s_areaHash.Ptr(areaID, key);
    if (!area) {
      area = s_areaHash.New(areaID, key, 0, 0);
      area->continent = continentID;
      area->area = areaID;
      area->subArea = subArea;
      area->rec = rec;
    }
  }

  InitializeMusic();
}

static BOOL MIDISetHandler(LPCSTR command, LPCSTR arguments) {
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
  s_flags = 0;
  ConsoleCommandRegister("midiset", MIDISetHandler, DEBUG, 0);
}

void AreaListShutdown() {
  s_indoors = -1;
  ConsoleCommandUnregister("midiset");
  s_currentContinent = -1;
  s_areaHash.Clear();
}

BOOL AreaListGetName(UINT continentID, UINT areaID, UINT subAreaID, char *buffer, UINT size, int fullName) {
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
  const WMOAreaTableRec *globalRec;
  const WMOAreaTableRec *rec;
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

  if (CWorld::QueryMapObjAreaTable(worldObject, rec, globalRec)) {
    int musicID = rec && rec->m_ZoneMusic ? rec->m_ZoneMusic : globalRec ? globalRec->m_ZoneMusic : 0;
    int m = rec && rec->m_MIDIAmbience ? rec->m_MIDIAmbience : globalRec ? globalRec->m_MIDIAmbience : 0;
    int mu = rec && rec->m_MIDIAmbienceUnderwater ? rec->m_MIDIAmbienceUnderwater : globalRec ? globalRec->m_MIDIAmbienceUnderwater : 0;
    int p = rec && rec->m_SoundProviderPref ? rec->m_SoundProviderPref : globalRec ? globalRec->m_SoundProviderPref : 0;
    int pu = rec && rec->m_SoundProviderPrefUnderwater ? rec->m_SoundProviderPrefUnderwater : globalRec ? globalRec->m_SoundProviderPrefUnderwater : 0;
    int introSound = rec && rec->m_IntroSound ? rec->m_IntroSound : globalRec ? globalRec->m_IntroSound : 0;
    int priority = rec && rec->m_IntroSound ? rec->m_IntroPriority : globalRec && globalRec->m_IntroSound ? globalRec->m_IntroPriority : 0;

    SndInterfaceRegisterNewZone(musicID);
    SndInterfaceSetMIDIArea(m, mu);
    SndInterfaceSetProviderPrefs(p, pu, 2000);
    SndInterfaceRegisterNewZoneIntro(introSound, priority);
  } else {
    clearMusic = true;
  }

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
  if (hash) {
    SendZoneUpdate(hash);
    SndInterfaceRegisterNewZone(hash->zoneMusic);
    SndInterfaceSetMIDIArea(hash->midi, hash->midiUnderwater);
    SndInterfaceSetProviderPrefs(hash->reverb, hash->reverbUnderwater, 2000);
    SndInterfaceRegisterNewZoneIntro(hash->zoneIntroID, hash->zoneIntroIDPriority);
  } else {
    clearMusic = true;
  }

  return true;
}

void AreaListRegisterLocation(const NTempest::C3Vector &location, UINT continent, DWORD worldObject) {
  FATALASSERT(worldObject);

  int    indoors = CWorld::QueryObjectInside(worldObject) != 0;
  LPCSTR subZoneName = 0;
  LPCSTR zoneName = 0;
  UINT   areaID = CWorld::QueryAreaId(location.x, location.y);
  UINT   zoneID = areaID >> 16;
  UINT   subZoneID = areaID & 0xFFFF;
  int    parentAreaID = 0;

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

  CGGameUI::NewZoneFeedback(parentAreaID, zoneName && *zoneName ? zoneName : 0, subZoneName && *subZoneName ? subZoneName : 0);

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
