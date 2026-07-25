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
static unsigned int                             s_currentZoneID;
static unsigned int                             s_currentSubZoneID;
static unsigned int                             s_currentContinent = -1;
static int                                      s_indoors = -1;
static unsigned int                             s_currentChunkID;

static AREAHASHOBJECT *__fastcall GetZone(unsigned int cont, unsigned int zName, unsigned int subZone) {
  AREAHASHKEY key;

  key.cont = cont;
  key.area = zName;
  key.subArea = subZone;

  return s_areaHash.Ptr(zName, key);
}

AREAHASHOBJECT *AREAHASHOBJECT::GetParent() const {
  AREAHASHKEY key;

  if (!subArea) {
    return 0;
  }

  key.cont = continent;
  key.area = area;
  key.subArea = 0;

  return s_areaHash.Ptr(area, key);
}

static void __fastcall InitializeAreaMusic(AREAHASHOBJECT *c) {
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

static void __fastcall InitializeMusic() {
  for (AREAHASHOBJECT *area = s_areaHash.Head(); area; area = s_areaHash.Next(area)) {
    InitializeAreaMusic(area);
  }
}

static void __fastcall LoadAreaTable() {
  unsigned int numEntries = g_areaTableDB.GetNumRecords();

  s_areaHash.SetTableSize(numEntries);

  for (unsigned int i = 0; i < numEntries; ++i) {
    const AreaTableRec *rec = g_areaTableDB.GetRecordByIndex(i);
    unsigned int        continentID = rec->m_ContinentID;
    unsigned int        areaID = rec->m_AreaNumber >> 16;
    unsigned int        subArea = rec->m_AreaNumber & 0xFFFF;
    AREAHASHKEY         key;

    key.cont = continentID;
    key.area = areaID;
    key.subArea = subArea;

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

static int MIDISetHandler(const char* command, const char* arguments) {
  unsigned int enabled = SStrToUnsigned(arguments);
  AREAHASHOBJECT *zone = GetZone(s_currentContinent, s_currentZoneID, s_currentSubZoneID);
  if (enabled && zone) {
    SndInterfaceSetMIDIArea(zone->midi, zone->midiUnderwater);
  } else {
    SndInterfaceClearMIDI();
  }
  return 1;
}

void __fastcall AreaListInitialize() {
  LoadAreaTable();
  s_currentContinent = 0;
}

void __fastcall AreaListShutdown() {
  s_indoors = -1;
  ConsoleCommandUnregister("midiset");
  s_currentContinent = -1;
  s_areaHash.Clear();
}

int __fastcall AreaListGetName(unsigned int continentID, unsigned int areaID, unsigned int subAreaID, char *buffer, unsigned int size, int fullName) {
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

static void __fastcall SendZoneUpdate(AREAHASHOBJECT *hash) {
  if (hash->rec) {
    CDataStore msg;
    msg.Put(CMSG_ZONEUPDATE);
    msg.Put(hash->rec->m_ID);
    msg.Finalize();
    ClientServices_Send(&msg);
  }
}

static bool __fastcall HandleIndoorZoneChange(unsigned long worldObject, const char *&zoneName, const char *&subZoneName, bool &clearMusic) {
  const WMOAreaTableRec *globalRec = 0;
  const WMOAreaTableRec *rec = 0;
  const char            *szName = 0;
  const char            *zName = 0;
  unsigned int           chunk = 0;
  int                    p;
  int                    mu;
  int                    m;

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

  m = rec && rec->m_ZoneMusic ? rec->m_ZoneMusic : globalRec ? globalRec->m_ZoneMusic : 0;
  mu = rec && rec->m_MIDIAmbience ? rec->m_MIDIAmbience : globalRec ? globalRec->m_MIDIAmbience : 0;
  p = rec && rec->m_MIDIAmbienceUnderwater ? rec->m_MIDIAmbienceUnderwater : globalRec ? globalRec->m_MIDIAmbienceUnderwater : 0;
  SndInterfaceRegisterNewZone(m);
  SndInterfaceSetMIDIArea(mu, p);

  m = rec && rec->m_SoundProviderPref ? rec->m_SoundProviderPref : globalRec ? globalRec->m_SoundProviderPref : 0;
  p = rec && rec->m_SoundProviderPrefUnderwater ? rec->m_SoundProviderPrefUnderwater : globalRec ? globalRec->m_SoundProviderPrefUnderwater : 0;
  SndInterfaceSetProviderPrefs(m, p, 2000);

  m = rec && rec->m_IntroSound ? rec->m_IntroSound : globalRec ? globalRec->m_IntroSound : 0;
  p = rec && rec->m_IntroSound ? rec->m_IntroPriority : globalRec && globalRec->m_IntroSound ? globalRec->m_IntroPriority : 0;
  SndInterfaceRegisterNewZoneIntro(m, p);

  return true;
}

static bool __fastcall HandleOutdoorZoneChange(unsigned int zoneID, unsigned int subZoneID, unsigned int continent, bool &clearMusic) {
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

void __fastcall AreaListRegisterLocation(const NTempest::C3Vector &location, unsigned int continent, unsigned long worldObject) {
  FATALASSERT(worldObject);

  int          indoors = CWorld::QueryObjectInside(worldObject) != 0;
  unsigned int areaID = CWorld::QueryAreaId(location.x, location.y);
  unsigned int zoneID = areaID >> 16;
  unsigned int subZoneID = areaID & 0xFFFF;
  int          parentAreaID = 0;
  const char  *zoneName = 0;
  const char  *subZoneName = 0;

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

int __fastcall AreaListZoneHasBreathParticles(unsigned long worldObject, unsigned int continentID, const NTempest::C3Vector &position) {
  const WMOAreaTableRec *globalRec;
  const WMOAreaTableRec *rec;

  if (CWorld::QueryObjectInside(worldObject)) {
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

  unsigned int    areaID = CWorld::QueryAreaId(position.x, position.y);
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
