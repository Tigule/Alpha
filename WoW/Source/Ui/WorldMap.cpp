#include <Base/Base.h>
#include <Frame/CSimpleTop.h>
#include <Frame/CSimpleModel.h>
#include <WowConst.h>
#include <MapDefs.h>
#include <WorldClient/World.h>
#include "Net/NetClient/NetClient.h"
#include "Object/ObjectClient/Unit_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "SoundInterface/SoundInterface.h"
#include "UIUtil/InputControl.h"
#include "WorldFrame.h"
#include "GameUI.h"

#include "Object/ObjectClient/Player_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "Ui/GameUI.h"
#include "Ui/PartyFrame.h"
#include "WowSvcs/WowSvcsClient/ClientServices.h"

#include <Base/CDataStore.h>
#include <DB/DBClient/AutoCode/AreaPOIRec.h>
#include <DB/DBClient/AutoCode/AreaTableRec.h>
#include <DB/DBClient/AutoCode/MapRec.h>
#include <DB/DBClient/AutoCode/WorldMapAreaRec.h>
#include <DB/DBClient/AutoCode/WorldMapContinentRec.h>
#include <DB/DBClient/AutoCode/WorldSafeLocsRec.h>
#include <FrameScript/FrameScript.h>
#include <lauxlib.h>
#include <lua.h>
#include <Net/NetClient/NetClient.h>
#include <Os/OsTime.h>
#include <Tempest/c2vector.h>
#include <Tempest/crect.h>
#include <math.h>
#include <string.h>
#include <stpl.h>
#include <storm.h>

struct WorldMapContinentInfo {
  int               continentID;
  int               mapAreaID;
  TSFixedArray<int> zoneList;
  int               chunkZones[128][128];
  NTempest::CRect   hitRect;
};

struct WorldMapLandmarkInfo {
  int   entryID;
  float x;
  float y;
  BOOL  isPortLoc;
};

class CGWorldMap {
 public:
  static void InitializeGame();
  static void EnterWorld();
  static void LeaveWorld();
  static void ShutdownGame();
  static int  GetCurrentContinent() {
    return m_currentContinent;
  }
  static int GetCurrentZone() {
    return m_currentZone;
  }
  static UINT GetNumContinents() {
    return m_continents.Count();
  }
  static LPCSTR GetContinentName(UINT index);
  static UINT   GetNumZones(UINT continent) {
    return continent < m_continents.Count() ? m_continents[continent].zoneList.Count() : 0;
  }
  static LPCSTR GetZoneName(UINT continent, UINT index);
  static void   SetMapToCurrentZone();
  static void   SetMap(int continent, int zone);
  static LPCSTR GetMapFilename();
  static UINT   GetMapHeight();
  static void   ProcessClick(float x, float y);
  static int    GetMapHighlight(float x, float y);
  static void   RunNearestPortLoc(float x, float y);
  static void   GetPOIPosition(const AreaPOIRec *rec, float &x, float &y);
  static void   GetPortLocPosition(const WorldSafeLocsRec *rec, float &x, float &y);
  static void   GetPlayerPosition(DWORDLONG guid, float &x, float &y);
  static void   GetBindPosition(float &x, float &y);
  static UINT   GetNumLandmarks() {
    return m_numLandmarks;
  }
  static const WorldMapLandmarkInfo *GetLandmarkInfo(UINT index) {
    return index < m_numLandmarks ? &m_landmarks[index] : 0;
  }

 private:
  static int  GetMapAreaFromPos(float x, float y);
  static BOOL GetWorldLocFromPos(float x, float y, NTempest::C2Vector &loc, int &mapID);
  static void GetWorldPosition(const NTempest::C2Vector &pos, int mapID, float &x, float &y);

 protected:
  static int                                 m_currentContinent;
  static int                                 m_currentZone;
  static UINT                                m_numLandmarks;
  static TSFixedArray<WorldMapContinentInfo> m_continents;
  static TSFixedArray<WorldMapLandmarkInfo>  m_landmarks;
};

int                                 CGWorldMap::m_currentContinent = -1;
int                                 CGWorldMap::m_currentZone = -1;
UINT                                CGWorldMap::m_numLandmarks;
TSFixedArray<WorldMapContinentInfo> CGWorldMap::m_continents;
TSFixedArray<WorldMapLandmarkInfo>  CGWorldMap::m_landmarks;

void CGWorldMap::InitializeGame() {
  UINT j;
  UINT i;
  UINT count = 0;
  UINT numEntries = g_worldMapAreaDB.GetNumRecords();

  for (i = 0; i < numEntries; ++i) {
    const WorldMapAreaRec *rec = g_worldMapAreaDB.GetRecordByIndex(i);
    if (rec && !rec->m_areaID) {
      ++count;
    }
  }

  m_continents.SetCount(count);
  count = 0;

  for (i = 0; i < numEntries; ++i) {
    const WorldMapAreaRec *rec = g_worldMapAreaDB.GetRecordByIndex(i);
    if (rec && !rec->m_areaID) {
      m_continents[count].continentID = rec->m_mapID;
      m_continents[count].mapAreaID = rec->m_ID;

      int zoneCount = 0;
      for (j = 0; j < numEntries; ++j) {
        const WorldMapAreaRec *areaRec = g_worldMapAreaDB.GetRecordByIndex(j);
        if (areaRec && areaRec->m_areaID && areaRec->m_mapID == rec->m_mapID) {
          ++zoneCount;
        }
      }

      m_continents[count].zoneList.SetCount(zoneCount);
      zoneCount = 0;
      for (j = 0; j < numEntries; ++j) {
        const WorldMapAreaRec *areaRec = g_worldMapAreaDB.GetRecordByIndex(j);
        if (areaRec && areaRec->m_areaID && areaRec->m_mapID == rec->m_mapID) {
          m_continents[count].zoneList[zoneCount++] = areaRec->m_ID;
        }
      }

      for (j = 0; j < static_cast<UINT>(g_worldMapContinentDB.GetNumRecords()); ++j) {
        const WorldMapContinentRec *continentRec = g_worldMapContinentDB.GetRecordByIndex(j);
        if (continentRec->m_mapID == rec->m_mapID) {
          float xOffset = continentRec->m_continentOffsetX - 0.6875f;
          float yOffset = continentRec->m_continentOffsetY - 11.125f;
          m_continents[count].hitRect.l = (continentRec->m_leftBoundary + xOffset) * 0.015968064f;
          m_continents[count].hitRect.r = (continentRec->m_rightBoundary + xOffset + 1.0f) * 0.015968064f;
          m_continents[count].hitRect.t = (continentRec->m_topBoundary + yOffset) * 0.023952097f;
          m_continents[count].hitRect.b = (continentRec->m_bottomBoundary + yOffset + 1.0f) * 0.023952097f;
          break;
        }
      }

      char   buf[MAX_PATH];
      SFile *file;
      SStrPrintf(buf, sizeof(buf), "Interface\\WorldMap\\%s.zmp", rec->m_areaName);
      if (SFile::Open(buf, &file)) {
        SFile::Read(file, m_continents[count].chunkZones, sizeof(m_continents[count].chunkZones), 0, 0, 0);
        SFile::Close(file);

        for (j = 0; j < 128; ++j) {
          for (UINT k = 0; k < 128; ++k) {
            BOOL found = 0;
            int  areaNum = m_continents[count].chunkZones[j][k] & 0xFFFF0000;
            if (areaNum) {
              for (UINT n = 0; n < numEntries; ++n) {
                const WorldMapAreaRec *areaRec = g_worldMapAreaDB.GetRecordByIndex(n);
                if (areaRec) {
                  const AreaTableRec *area = g_areaTableDB.GetRecord(areaRec->m_areaID);
                  if (area && area->m_AreaNumber == areaNum && area->m_ContinentID == rec->m_mapID) {
                    m_continents[count].chunkZones[j][k] = areaRec->m_ID;
                    found = 1;
                    break;
                  }
                }
              }
            }
            if (!found) {
              m_continents[count].chunkZones[j][k] = 0;
            }
          }
        }
        ++count;
      }
    }
  }

  m_numLandmarks = 0;
  m_landmarks.SetCount(16);
}

void CGWorldMap::EnterWorld() {
}

void CGWorldMap::LeaveWorld() {
}

void CGWorldMap::ShutdownGame() {
  for (UINT i = 0; i < m_continents.Count(); ++i) {
    m_continents[i].zoneList.Clear();
  }
  m_continents.Clear();
  m_landmarks.Clear();
}

LPCSTR CGWorldMap::GetContinentName(UINT index) {
  if (index >= m_continents.Count()) {
    return 0;
  }
  const MapRec *rec = g_mapDB.GetRecord(m_continents[index].continentID);
  return rec ? rec->m_MapName_lang[CURRENT_LANGUAGE] : 0;
}

LPCSTR CGWorldMap::GetZoneName(UINT continent, UINT index) {
  if (continent >= m_continents.Count() || index >= m_continents[continent].zoneList.Count()) {
    return 0;
  }
  const WorldMapAreaRec *map = g_worldMapAreaDB.GetRecord(m_continents[continent].zoneList[index]);
  const AreaTableRec    *area = map ? g_areaTableDB.GetRecord(map->m_areaID) : 0;
  return area ? area->m_AreaName_lang[CURRENT_LANGUAGE] : 0;
}

void CGWorldMap::SetMapToCurrentZone() {
  UINT i;

  if (!ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__)) {
    SetMap(-1, -1);
    return;
  }

  int continent = -1;
  for (i = 0; i < m_continents.Count(); ++i) {
    if (m_continents[i].continentID == static_cast<int>(CGPlayer_C::GetNewContinentID())) {
      continent = i;
      break;
    }
  }

  if (continent == -1) {
    SetMap(-1, -1);
    return;
  }

  int zone = -1;
  for (i = 0; i < m_continents[continent].zoneList.Count(); ++i) {
    const WorldMapAreaRec *rec = g_worldMapAreaDB.GetRecord(m_continents[continent].zoneList[i]);
    if (rec && rec->m_areaID == CGGameUI::m_areaID) {
      zone = i;
      break;
    }
  }

  SetMap(continent, zone);
}

void CGWorldMap::SetMap(int continent, int zone) {
  if (static_cast<UINT>(continent) >= m_continents.Count()) {
    m_currentContinent = -1;
    m_currentZone = -1;
  } else {
    m_currentContinent = continent;
    m_currentZone = -1;
    if (static_cast<UINT>(zone) < m_continents[continent].zoneList.Count()) {
      m_currentZone = zone;
    }
  }

  m_numLandmarks = 0;

  UINT numEntries = g_areaPOIDB.GetNumRecords();
  UINT i;
  for (i = 0; i < numEntries; ++i) {
    const AreaPOIRec *rec = g_areaPOIDB.GetRecordByIndex(i);
    float             y = 0.0f;
    float             x = 0.0f;
    GetPOIPosition(rec, x, y);
    if (fabs(x) >= 2.3841858e-7f || fabs(y) >= 2.3841858e-7f) {
      if (m_landmarks.Count() <= m_numLandmarks) {
        m_landmarks.SetCount(m_landmarks.Count() * 2);
      }
      m_landmarks[m_numLandmarks].entryID = rec->m_ID;
      m_landmarks[m_numLandmarks].x = x;
      m_landmarks[m_numLandmarks].y = y;
      m_landmarks[m_numLandmarks].isPortLoc = 0;
      ++m_numLandmarks;
    }
  }

  numEntries = g_worldSafeLocsDB.GetNumRecords();
  for (i = 0; i < numEntries; ++i) {
    const WorldSafeLocsRec *rec = g_worldSafeLocsDB.GetRecordByIndex(i);
    float                   x = 0.0f;
    float                   y = 0.0f;
    GetPortLocPosition(rec, x, y);
    if (fabs(x) >= 2.3841858e-7f || fabs(y) >= 2.3841858e-7f) {
      if (m_landmarks.Count() <= m_numLandmarks) {
        m_landmarks.SetCount(m_landmarks.Count() * 2);
      }
      m_landmarks[m_numLandmarks].entryID = rec->m_ID;
      m_landmarks[m_numLandmarks].x = x;
      m_landmarks[m_numLandmarks].y = y;
      m_landmarks[m_numLandmarks].isPortLoc = 1;
      ++m_numLandmarks;
    }
  }

  FrameScript_SignalEvent(349);
}

LPCSTR CGWorldMap::GetMapFilename() {
  if (m_currentContinent == -1) {
    return 0;
  }
  int mapAreaID = m_currentZone < 0 ? m_continents[m_currentContinent].mapAreaID : m_continents[m_currentContinent].zoneList[m_currentZone];
  const WorldMapAreaRec *rec = g_worldMapAreaDB.GetRecord(mapAreaID);
  return rec ? rec->m_areaName : 0;
}

UINT CGWorldMap::GetMapHeight() {
  if (m_currentContinent < 0) {
    return 0;
  }
  int mapAreaID = m_currentZone < 0 ? m_continents[m_currentContinent].mapAreaID : m_continents[m_currentContinent].zoneList[m_currentZone];
  const WorldMapAreaRec *rec = g_worldMapAreaDB.GetRecord(mapAreaID);
  if (!rec) {
    return 0;
  }
  UINT width = rec->m_rightBoundary - rec->m_leftBoundary + 1;
  return width ? 1002 * (rec->m_bottomBoundary - rec->m_topBoundary + 1) / width : 0;
}

int CGWorldMap::GetMapAreaFromPos(float x, float y) {
  if (m_currentContinent == -1) {
    return 0;
  }
  if (m_currentZone >= 0) {
    return 0;
  }

  const WorldMapAreaRec *rec = g_worldMapAreaDB.GetRecord(m_continents[m_currentContinent].mapAreaID);
  if (!rec) {
    return 0;
  }

  x = max(0.0f, min(x, 1.0f));
  y = max(0.0f, min(y, 1.0f));
  int zonex = static_cast<int>((static_cast<UINT>(rec->m_rightBoundary - rec->m_leftBoundary + 1) * x + rec->m_leftBoundary) * 2.0f);
  int zone = static_cast<int>((static_cast<UINT>(rec->m_bottomBoundary - rec->m_topBoundary + 1) * y + rec->m_topBoundary) * 2.0f);
  return m_continents[m_currentContinent].chunkZones[zone][zonex];
}

BOOL CGWorldMap::GetWorldLocFromPos(float x, float y, NTempest::C2Vector &loc, int &mapID) {
  if (m_currentContinent == -1) {
    for (UINT continent = 0; continent < m_continents.Count(); ++continent) {
      if (!m_continents[continent].hitRect.Contains(NTempest::C2Vector(x, y))) {
        continue;
      }

      UINT numContinents = g_worldMapContinentDB.GetNumRecords();
      for (UINT j = 0; j < numContinents; ++j) {
        const WorldMapContinentRec *rec = g_worldMapContinentDB.GetRecordByIndex(j);
        if (rec->m_mapID == m_continents[continent].continentID) {
          loc.x = (rec->m_continentOffsetY - (y - 0.5f) * 41.75f) * 533.33331f;
          loc.y = (rec->m_continentOffsetX - (x - 0.5f) * 62.625f) * 533.33331f;
          mapID = rec->m_mapID;
          return 1;
        }
      }
    }
    return 0;
  }

  const WorldMapAreaRec *areaRec;
  if (m_currentZone < 0) {
    areaRec = g_worldMapAreaDB.GetRecord(m_continents[m_currentContinent].mapAreaID);
  } else {
    areaRec = g_worldMapAreaDB.GetRecord(m_continents[m_currentContinent].zoneList[m_currentZone]);
  }
  if (!areaRec) {
    return 0;
  }

  x = (areaRec->m_rightBoundary - areaRec->m_leftBoundary + 1) * x + areaRec->m_leftBoundary;
  y = (areaRec->m_bottomBoundary - areaRec->m_topBoundary + 1) * y + areaRec->m_topBoundary;
  loc.x = (32.0f - y) * 533.33331f;
  loc.y = (32.0f - x) * 533.33331f;
  mapID = m_continents[m_currentContinent].continentID;
  return 1;
}

void CGWorldMap::GetWorldPosition(const NTempest::C2Vector &pos, int mapID, float &x, float &y) {
  x = 0.0f;
  y = 0.0f;

  const WorldMapContinentRec *continentRec = 0;
  for (int i = 0; i < g_worldMapContinentDB.GetNumRecords(); ++i) {
    const WorldMapContinentRec *rec = g_worldMapContinentDB.GetRecordByIndex(i);
    if (rec->m_mapID == mapID) {
      continentRec = rec;
      break;
    }
  }
  if (!continentRec) {
    return;
  }

  if (m_currentContinent < 0) {
    x = continentRec->m_continentOffsetX * 0.015968064f + 0.5f - pos.y * 2.994012e-5f;
    y = continentRec->m_continentOffsetY * 0.023952097f + 0.5f - pos.x * 4.4910183e-5f;
    return;
  }

  if (static_cast<UINT>(m_currentContinent) >= m_continents.Count() || m_continents[m_currentContinent].continentID != mapID) {
    return;
  }

  const WorldMapAreaRec *areaRec;
  if (m_currentZone < 0) {
    areaRec = g_worldMapAreaDB.GetRecord(m_continents[m_currentContinent].mapAreaID);
  } else {
    areaRec = g_worldMapAreaDB.GetRecord(m_continents[m_currentContinent].zoneList[m_currentZone]);
  }
  if (!areaRec) {
    return;
  }

  const float tileSize = 533.33331f;
  const float mapOrigin = 17066.666f;
  float       left = mapOrigin - areaRec->m_leftBoundary * tileSize;
  float       right = mapOrigin - (areaRec->m_rightBoundary + 1) * tileSize;
  x = 1.0f - (pos.y - right) / (left - right);

  float top = mapOrigin - areaRec->m_topBoundary * tileSize;
  float bottom = mapOrigin - (areaRec->m_bottomBoundary + 1) * tileSize;
  y = 1.0f - (pos.x - bottom) / (top - bottom);

  if (x < 0.0f || x > 1.0f || y < 0.0f || y > 1.0f) {
    x = 0.0f;
    y = 0.0f;
  }
}

void CGWorldMap::ProcessClick(float x, float y) {
  UINT i;

  if (EventIsKeyDown(KEY_CONTROL)) {
    RunNearestPortLoc(x, y);
    return;
  }
  if (m_currentZone >= 0) {
    RunNearestPortLoc(x, y);
    return;
  }

  if (m_currentContinent == -1) {
    for (i = 0; i < m_continents.Count(); ++i) {
      const NTempest::CRect &rect = m_continents[i].hitRect;
      if (x >= rect.l && x <= rect.r && y >= rect.t && y <= rect.b) {
        SetMap(i, -1);
        return;
      }
    }
    return;
  }

  int mapArea = GetMapAreaFromPos(x, y);
  if (mapArea) {
    for (i = 0; i < GetNumZones(m_currentContinent); ++i) {
      if (m_continents[m_currentContinent].zoneList[i] == mapArea) {
        SetMap(m_currentContinent, i);
        return;
      }
    }
  }
}

int CGWorldMap::GetMapHighlight(float x, float y) {
  if (m_currentContinent == -1) {
    for (UINT i = 0; i < m_continents.Count(); ++i) {
      const NTempest::CRect &rect = m_continents[i].hitRect;
      if (x >= rect.l && x <= rect.r && y >= rect.t && y <= rect.b) {
        return m_continents[i].mapAreaID;
      }
    }
    return 0;
  }
  if (m_currentZone >= 0) {
    return 0;
  }
  return GetMapAreaFromPos(x, y);
}

void CGWorldMap::RunNearestPortLoc(float x, float y) {
  NTempest::C2Vector loc(0.0f, 0.0f);
  int                mapID;
  if (!GetWorldLocFromPos(x, y, loc, mapID)) {
    return;
  }

  const WorldSafeLocsRec *nearest = 0;
  float                   nearestDist = 0.0f;
  UINT                    numRecords = g_worldSafeLocsDB.GetNumRecords();
  for (UINT index = 0; index < numRecords; ++index) {
    const WorldSafeLocsRec *rec = g_worldSafeLocsDB.GetRecordByIndex(index);
    if (rec && rec->m_continent == mapID) {
      NTempest::C2Vector diff(rec->m_locX - loc.x, rec->m_locY - loc.y);
      float              distance = diff.x * diff.x + diff.y * diff.y;
      if (!nearest || distance < nearestDist) {
        nearest = rec;
        nearestDist = distance;
      }
    }
  }
  if (!nearest) {
    return;
  }

  DWORD eventTime = OsGetAsyncTimeMs();
  CDataStore msg;
  msg.Put(CMSG_WORLD_TELEPORT);
  msg.Put(eventTime);
  msg.Put(static_cast<BYTE>(nearest->m_continent));
  msg.Put(nearest->m_locX);
  msg.Put(nearest->m_locY);
  msg.Put(nearest->m_locZ);
  msg.Put(0);
  msg.Finalize();
  ClientServices_Send(&msg);
  FrameScript_SignalEvent(356);
}

void CGWorldMap::GetPlayerPosition(DWORDLONG guid, float &x, float &y) {
  x = 0.0f;
  y = 0.0f;

  NTempest::C2Vector pos(0.0f, 0.0f);
  CGObject_C        *object = ClntObjMgrObjectPtr(guid, __FILE__, __LINE__);
  if (object) {
    pos.Set(object->GetPosition().x, object->GetPosition().y);
    GetWorldPosition(pos, CGPlayer_C::GetNewContinentID(), x, y);
  } else if (CGPartyInfo::IsMember(guid)) {
    CGPartyInfo::RemoteStats *stats = CGPartyInfo::GetRemoteStats(guid);
    if (stats) {
      pos.Set(stats->pos.x, stats->pos.y);
      GetWorldPosition(pos, stats->mapID, x, y);
    }
  }
}

void CGWorldMap::GetBindPosition(float &x, float &y) {
  x = 0.0f;
  y = 0.0f;

  NTempest::C2Vector pos = (CGPlayer_C::GetBindPoint().x, CGPlayer_C::GetBindPoint().y);
  GetWorldPosition(pos, CGPlayer_C::GetNewContinentID(), x, y);
}

void CGWorldMap::GetPOIPosition(const AreaPOIRec *rec, float &x, float &y) {
  x = 0.0f;
  y = 0.0f;
  if (m_currentZone >= 0 || !rec || !(rec->m_flags & 0x4)) {
    return;
  }
  if (m_currentContinent < 0 && !(rec->m_flags & 0x10)) {
    return;
  }
  if (!(rec->m_flags & 0x8)) {
    return;
  }

  GetWorldPosition(NTempest::C2Vector(rec->m_x, rec->m_y), rec->m_continentID, x, y);
}

void CGWorldMap::GetPortLocPosition(const WorldSafeLocsRec *rec, float &x, float &y) {
  x = 0.0f;
  y = 0.0f;
  if (!rec || m_currentZone < 0) {
    return;
  }

  GetWorldPosition(NTempest::C2Vector(rec->m_locX, rec->m_locY), rec->m_continent, x, y);
}

DWORDLONG Script_GetGUIDFromName(LPCSTR name);

static int Script_GetMapContinents(lua_State *L) {
  UINT count = CGWorldMap::GetNumContinents();
  for (UINT i = 0; i < count; ++i) {
    lua_pushstring(L, CGWorldMap::GetContinentName(i));
  }
  return count;
}

static int Script_GetMapZones(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetMapZones(continentIndex)");
    return 0;
  }
  UINT continent = static_cast<UINT>(lua_tonumber(L, 1)) - 1;
  UINT count = CGWorldMap::GetNumZones(continent);
  lua_checkstack(L, count);
  for (UINT i = 0; i < count; ++i) {
    lua_pushstring(L, CGWorldMap::GetZoneName(continent, i));
  }
  return count;
}

static int Script_SetMapZoom(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: SetMapZoom(continentIndex [,zoneIndex])");
    return 0;
  }
  int continent = static_cast<int>(lua_tonumber(L, 1)) - 1;
  int zone = -1;
  if (lua_isnumber(L, 2)) {
    zone = static_cast<int>(lua_tonumber(L, 2)) - 1;
  }
  CGWorldMap::SetMap(continent, zone);
  return 0;
}

static int Script_SetMapToCurrentZone(lua_State *) {
  CGWorldMap::SetMapToCurrentZone();
  return 0;
}

static int Script_GetMapInfo(lua_State *L) {
  lua_pushstring(L, CGWorldMap::GetMapFilename());
  UINT height = CGWorldMap::GetMapHeight();
  lua_pushnumber(L, static_cast<double>(height));
  UINT low = height & 0xFF;
  height -= low;
  if (low & (low - 1)) {
    int bit = 7;
    while (!(low & (1 << bit))) {
      --bit;
    }
    low = 1 << (bit + 1);
  }
  lua_pushnumber(L, static_cast<double>(low + height));
  return 3;
}

static int Script_GetCurrentMapContinent(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGWorldMap::GetCurrentContinent() + 1));
  return 1;
}

static int Script_GetCurrentMapZone(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGWorldMap::GetCurrentZone() + 1));
  return 1;
}

static int Script_ProcessMapClick(lua_State *L) {
  if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
    luaL_error(L, "Usage: ProcessMapClick(x, y)");
    return 0;
  }
  float x = static_cast<float>(lua_tonumber(L, 1));
  float y = static_cast<float>(lua_tonumber(L, 2));
  CGWorldMap::ProcessClick(x, y);
  return 0;
}

static int Script_UpdateMapHighlight(lua_State *L) {
  if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
    luaL_error(L, "Usage: UpdateMapHighlight(x, y)");
    return 0;
  }
  int                    mapAreaID = CGWorldMap::GetMapHighlight(static_cast<float>(lua_tonumber(L, 1)), static_cast<float>(lua_tonumber(L, 2)));
  const WorldMapAreaRec *mapArea = g_worldMapAreaDB.GetRecord(mapAreaID);
  if (!mapArea) {
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnumber(L, 0.0);
    lua_pushnumber(L, 0.0);
    lua_pushnumber(L, 0.0);
    lua_pushnumber(L, 0.0);
    lua_pushnumber(L, 0.0);
    return 7;
  }

  if (mapArea->m_areaID) {
    const AreaTableRec *area = g_areaTableDB.GetRecord(mapArea->m_areaID);
    if (area) {
      lua_pushstring(L, area->m_AreaName_lang[CURRENT_LANGUAGE]);
    } else {
      lua_pushnil(L);
    }
  } else {
    const MapRec *map = g_mapDB.GetRecord(mapArea->m_mapID);
    if (map) {
      lua_pushstring(L, map->m_MapName_lang[CURRENT_LANGUAGE]);
    } else {
      lua_pushnil(L);
    }
  }
  lua_pushstring(L, mapArea->m_areaName);

  UINT                        width = 64;
  UINT                        height = 64;
  UINT                        pixelHeight = 128;
  const WorldMapContinentRec *continent = 0;
  if (mapArea->m_areaID) {
    width = mapArea->m_rightBoundary - mapArea->m_leftBoundary + 1;
    height = mapArea->m_bottomBoundary - mapArea->m_topBoundary + 1;
    pixelHeight = (height << 7) / width;
  } else {
    for (UINT i = 0; i < g_worldMapContinentDB.GetNumRecords(); ++i) {
      const WorldMapContinentRec *record = g_worldMapContinentDB.GetRecordByIndex(i);
      if (record && record->m_mapID == mapArea->m_mapID) {
        continent = record;
        break;
      }
    }
    if (continent) {
      width = continent->m_rightBoundary - continent->m_leftBoundary + 1;
      height = continent->m_bottomBoundary - continent->m_topBoundary + 1;
      pixelHeight = (height << 8) / width;
    }
  }

  UINT textureHeight = pixelHeight & 0xFF;
  if (!textureHeight) {
    textureHeight = pixelHeight;
  } else {
    if ((textureHeight - 1) & textureHeight) {
      UINT bit = 0x80;
      while (!(textureHeight & bit)) {
        bit >>= 1;
      }
      textureHeight = bit << 1;
    }
    while (textureHeight * 8 < (pixelHeight & 0xFF)) {
      textureHeight *= 2;
    }
    if (pixelHeight > 0x100) {
      textureHeight += ((pixelHeight >> 8) + 1) << 8;
    }
  }

  if (mapArea->m_areaID) {
    lua_pushnumber(L, 1.0);
    lua_pushnumber(L, static_cast<double>(pixelHeight) / textureHeight);
    const WorldMapAreaRec *parent = 0;
    for (UINT i = 0; i < g_worldMapAreaDB.GetNumRecords(); ++i) {
      const WorldMapAreaRec *record = g_worldMapAreaDB.GetRecordByIndex(i);
      if (record && !record->m_areaID && record->m_mapID == mapArea->m_mapID) {
        parent = record;
        break;
      }
    }
    if (parent) {
      UINT parentWidth = parent->m_rightBoundary - parent->m_leftBoundary + 1;
      UINT parentHeight = parent->m_bottomBoundary - parent->m_topBoundary + 1;
      lua_pushnumber(L, static_cast<double>(width) / parentWidth);
      lua_pushnumber(L, static_cast<double>(height) / parentHeight);
      lua_pushnumber(L, static_cast<double>(mapArea->m_leftBoundary - parent->m_leftBoundary) / parentWidth);
      lua_pushnumber(L, static_cast<double>(mapArea->m_topBoundary - parent->m_topBoundary) / parentHeight);
      return 8;
    }
  } else {
    if (width <= height) {
      lua_pushnumber(L, static_cast<double>(width) / height);
      lua_pushnumber(L, 1.0);
    } else {
      lua_pushnumber(L, 1.0);
      lua_pushnumber(L, static_cast<double>(height) / width);
    }
    if (continent) {
      lua_pushnumber(L, static_cast<double>(width) * 0.015968064);
      lua_pushnumber(L, static_cast<double>(height) * 0.023952097);
      lua_pushnumber(L, (static_cast<double>(continent->m_leftBoundary) + continent->m_continentOffsetX - 0.6875) * 0.015968064);
      lua_pushnumber(L, (static_cast<double>(continent->m_topBoundary) + continent->m_continentOffsetY - 11.125) * 0.023952097);
      return 8;
    }
  }

  lua_pushnumber(L, 0.0);
  lua_pushnumber(L, 0.0);
  lua_pushnumber(L, 0.0);
  lua_pushnumber(L, 0.0);
  lua_pushnumber(L, 0.0);
  return 8;
}

static int Script_GetPlayerMapPosition(lua_State *L) {
  if (!lua_isstring(L, 1)) {
    luaL_error(L, "Usage: GetPlayerMapPosition(\"player\")");
    return 0;
  }
  LPCSTR unit = lua_tostring(L, 1);
  float  x;
  float  y;
  CGWorldMap::GetPlayerPosition(Script_GetGUIDFromName(unit), x, y);
  lua_pushnumber(L, x);
  lua_pushnumber(L, y);
  return 2;
}

static int Script_GetBoundMapPosition(lua_State *L) {
  float x;
  float y;
  CGWorldMap::GetBindPosition(x, y);
  lua_pushnumber(L, x);
  lua_pushnumber(L, y);
  return 2;
}

static int Script_GetNumMapLandmarks(lua_State *L) {
  lua_pushnumber(L, static_cast<double>(CGWorldMap::GetNumLandmarks()));
  return 1;
}

static int Script_GetMapLandmarkInfo(lua_State *L) {
  if (!lua_isnumber(L, 1)) {
    luaL_error(L, "Usage: GetMapLandmarkInfo(index)");
    return 0;
  }
  UINT                        index = static_cast<int>(lua_tonumber(L, 1)) - 1;
  const WorldMapLandmarkInfo *info = CGWorldMap::GetLandmarkInfo(index);
  if (info) {
    if (info->isPortLoc) {
      const WorldSafeLocsRec *rec = g_worldSafeLocsDB.GetRecord(info->entryID);
      if (rec) {
        lua_pushstring(L, rec->m_AreaName_lang[CURRENT_LANGUAGE]);
        lua_pushnumber(L, 6.0);
        lua_pushnumber(L, info->x);
        lua_pushnumber(L, info->y);
        return 4;
      }
    } else {
      const AreaPOIRec *rec = g_areaPOIDB.GetRecord(info->entryID);
      if (rec) {
        lua_pushstring(L, rec->m_name_lang[CURRENT_LANGUAGE]);
        lua_pushnumber(L, rec->m_icon);
        lua_pushnumber(L, info->x);
        lua_pushnumber(L, info->y);
        return 4;
      }
    }
  }
  lua_pushnil(L);
  lua_pushnumber(L, 0.0);
  lua_pushnumber(L, 0.0);
  lua_pushnumber(L, 0.0);
  return 4;
}

static FrameScript_Method s_ScriptFunctions[13] = {
    {      "GetMapContinents",       Script_GetMapContinents},
    {           "GetMapZones",            Script_GetMapZones},
    {            "SetMapZoom",             Script_SetMapZoom},
    {   "SetMapToCurrentZone",    Script_SetMapToCurrentZone},
    {            "GetMapInfo",             Script_GetMapInfo},
    {"GetCurrentMapContinent", Script_GetCurrentMapContinent},
    {     "GetCurrentMapZone",      Script_GetCurrentMapZone},
    {       "ProcessMapClick",        Script_ProcessMapClick},
    {    "UpdateMapHighlight",     Script_UpdateMapHighlight},
    {  "GetPlayerMapPosition",   Script_GetPlayerMapPosition},
    {   "GetBoundMapPosition",    Script_GetBoundMapPosition},
    {    "GetNumMapLandmarks",     Script_GetNumMapLandmarks},
    {    "GetMapLandmarkInfo",     Script_GetMapLandmarkInfo}
};

void WorldMapRegisterScriptFunctions() {
  for (UINT i = 0; i < 13; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void WorldMapUnregisterScriptFunctions() {
  for (UINT i = 0; i < 13; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}
