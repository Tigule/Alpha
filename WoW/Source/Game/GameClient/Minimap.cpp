#include <Base/Base.h>
#include <Gx/Gx.h>
#include <MapDefs.h>
#include <WorldClient/World.h>
#include <WowConst.h>
#include <DayNight.h>

#include "Game/GameClient/Minimap.h"

#include "Console/ConsoleVar.h"
#include "DB/DBClient/AutoCode/AreaPOIRec.h"
#include "DB/DBClient/AutoCode/MapRec.h"
#include "DB/DBClient/DBCacheInstances.h"
#include "Object/ObjectClient/Unit_C.h"
#include "Object/ObjectClient/Player_C.h"
#include "Object/Object.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "Ui/GameUI.h"
#include "Ui/PartyFrame.h"
#include "WorldClient/World.h"
#include <BLPFile/blp.h>
#include "WowSvcs/WowSvcsClient/ClientServices.h"

#include <Base/Status.h>
#include <DB/WowLocale.h>
#include <FrameScript/FrameScript.h>
#include <Services/SysMessage.h>
#include <Tempest/c2ivector.h>
#include <Tempest/c2vector.h>
#include <Tempest/c3vector.h>
#include <Tempest/c44matrix.h>
#include <Tempest/caabox.h>
#include <storm.h>
#include <malloc.h>

int APIENTRY SStrCmpI(LPCSTR string1, LPCSTR string2, DWORD maxchars = 0x7FFFFFFF);

struct MINIMAPMD5NAME : public TSHashObject<MINIMAPMD5NAME, HASHKEY_STRI> {
  char filename[40];
};

extern CVar *s_minimapZoomCVar;
extern CVar *s_minimapInsideZoomCVar;

static float                                     angle = 90.0f;
static float                                     boxHeight = 1066.6666f;
static float                                     boxWidth = 1066.6666f;
static const float                               AREA_WORLD_SIZE_X = 533.33331f;
static const float                               AREA_WORLD_SIZE_Y = 533.33331f;
static const float                               HALF_AREA_WORLD_SIZE_X = AREA_WORLD_SIZE_X * 0.5f;
static const float                               HALF_AREA_WORLD_SIZE_Y = AREA_WORLD_SIZE_Y * 0.5f;
static const float                               HALF_WORLD_SIZE_X = AREA_WORLD_SIZE_X * 64.0f * 0.5f;
static const float                               HALF_WORLD_SIZE_Y = AREA_WORLD_SIZE_Y * 64.0f * 0.5f;
static const float                               CLOSEENOUGH = 0.013888889f;
static const float                               MAX_POI_DISTANCE = 694.44446f;
static UINT                s_currentContinent = -1;
static NTempest::C3Vector  s_currentPosition(0.0f, 0.0f, 0.0f);
static NTempest::C2iVector s_currentUpperLeftArea(-1);
static NTempest::C2iVector s_currentLowerRightArea(-1);
#define NUM_ZOOMS 6

static UINT                                      s_currentZoom = 3;
static UINT                                      s_currentInsideZoom = 3;
static UINT                                      s_mapObjID;
static UINT                                      s_mapObjInstanceID;
static UINT                                      s_mapObjGroupID = -1;
static BYTE                                      s_isInside;
static NTempest::C44Matrix                       s_mapObjInvMtx;
static NTempest::CAaBox                          s_queryCenterBox;
static NTempest::C3Vector                        s_queryCenter;
static LPCSTR                                    MINIMAP_MD5_DIR = "Textures\\Minimap";
static LPCSTR                                    FILENAME_TEMPLATE = "%s\\map%d_%d.blp";
static const UINT          s_chunksPerSizeAtZoom[6] = {14, 12, 10, 8, 6, 4};
static const float         s_minimapZoomSize[6] = {150.0f, 120.0f, 90.0f, 60.0f, 40.0f, 25.0f};
static LPCSTR                                    s_mapObjTemplate = "%s_%03d_%02d_%02d.blp";
static char                                      s_mapObjDir[MAX_PATH];
static UINT                                      s_flags;
static AreaPOIRec                                s_questPOI;
static char                                      s_questPOIName[64];
static TSFixedArray<const AreaPOIRec *>          s_pointsOfInterest;
static TSFixedArray<int>                         s_POIIsVisible;
static TSFixedArray<int>                         s_visibleNoIcon;
static int                                       s_updatePOI;
static UINT                                      s_numPoints;
static TSGrowableArray<const AreaPOIRec *>       s_visiblePOI;
static int                                       s_distantPOI[3];
static UINT                                      s_numDistantPOI;
static float                                     s_POIRotation[3];
static int                                       s_updateDistantPOI;
static int                                       s_lowestVisiblePriority = 3;
static TSHashTable<MINIMAPMD5NAME, HASHKEY_STRI> s_md5NameHash;

static void UpdatePointsOfInterest() {
  UINT i;
  UINT j;

  s_visiblePOI.SetCount(0);
  for (i = 0; i < s_numPoints; ++i) {
    if (fabs(s_pointsOfInterest[i]->m_x) < 0.00000023841858f && fabs(s_pointsOfInterest[i]->m_y) < 0.00000023841858f) {
      if (s_POIIsVisible[i] || s_visibleNoIcon[i]) {
        s_updatePOI = 1;
        s_POIIsVisible[i] = 0;
        s_visibleNoIcon[i] = 0;
      }
    } else {
      NTempest::C2Vector dist(s_currentPosition.x, s_currentPosition.y);
      dist.x -= s_pointsOfInterest[i]->m_x;
      dist.y -= s_pointsOfInterest[i]->m_y;
      FATALASSERT(s_currentZoom < NUM_ZOOMS);
      float minimapVisRadius = 33.333332f * (s_chunksPerSizeAtZoom[s_currentZoom] * 0.5f);
      FATALASSERT(minimapVisRadius > 0.0f);
      float totalDistance = dist.Mag();
      if (totalDistance / minimapVisRadius > 0.8f) {
        if (s_POIIsVisible[i] || s_visibleNoIcon[i]) {
          s_updatePOI = 1;
          s_POIIsVisible[i] = 0;
          s_visibleNoIcon[i] = 0;
        }
      } else if (!(s_pointsOfInterest[i]->m_flags & 0x2)) {
        if (s_POIIsVisible[i] || !s_visibleNoIcon[i]) {
          s_updatePOI = 1;
          s_POIIsVisible[i] = 0;
          s_visibleNoIcon[i] = 1;
        }
      } else {
        *s_visiblePOI.New() = s_pointsOfInterest[i];
        if (!s_POIIsVisible[i] || s_visibleNoIcon[i]) {
          s_updatePOI = 1;
          s_POIIsVisible[i] = 1;
          s_visibleNoIcon[i] = 0;
        }
      }
    }
  }

  int   closest[3] = {-1, -1, -1};
  int   largest = -1;
  int   priority[3];
  UINT  numPOI = 0;
  float distance[3] = {MAX_POI_DISTANCE, MAX_POI_DISTANCE, MAX_POI_DISTANCE};

  for (i = 0; i < s_numPoints; ++i) {
    if ((fabs(s_pointsOfInterest[i]->m_x) < 0.00000023841858f && fabs(s_pointsOfInterest[i]->m_y) < 0.00000023841858f) || s_POIIsVisible[i] ||
        s_visibleNoIcon[i])
    {
      continue;
    }

    NTempest::C2Vector dist(s_pointsOfInterest[i]->m_x - s_currentPosition.x, s_pointsOfInterest[i]->m_y - s_currentPosition.y);
    float              totalDistance = NTempest::CMath::sqrt_(dist.SquaredMag());
    if (totalDistance > MAX_POI_DISTANCE) {
      continue;
    }

    if (largest == -1) {
      distance[numPOI] = totalDistance;
      closest[numPOI] = i;
      priority[numPOI] = s_pointsOfInterest[i]->m_importance;
      ++numPOI;
    } else if (s_pointsOfInterest[i]->m_importance < priority[largest] ||
               (s_pointsOfInterest[i]->m_importance == priority[largest] && totalDistance < distance[largest]))
    {
      distance[largest] = totalDistance;
      closest[largest] = i;
      priority[largest] = s_pointsOfInterest[i]->m_importance;
    }

    if (numPOI == 3) {
      largest = 0;
      for (j = 1; j < 3; ++j) {
        if (priority[j] > priority[largest]) {
          largest = j;
        } else if (priority[j] == priority[largest] && distance[j] > distance[largest]) {
          largest = j;
        }
      }
    }
  }

  if (s_numDistantPOI != numPOI) {
    s_numDistantPOI = numPOI;
    s_updateDistantPOI = 1;
  } else {
    for (i = 0; i < numPOI; ++i) {
      if (closest[i] != s_distantPOI[i]) {
        s_updateDistantPOI = 1;
        break;
      }
    }
  }

  for (i = 0; i < numPOI; ++i) {
    s_distantPOI[i] = closest[i];
    s_POIRotation[i] = CalculateFacingTo(
        s_currentPosition, NTempest::C3Vector(s_pointsOfInterest[s_distantPOI[i]]->m_x, s_pointsOfInterest[s_distantPOI[i]]->m_y, s_currentPosition.z)
    );
  }
}

static NTempest::C2iVector CoordinateToArea(const NTempest::C3Vector &position) {
  float x = (17066.666f - position.y) * 0.0018750001f;
  float y = (17066.666f - position.x) * 0.0018750001f;
  if (x < 0.0f) {
    x -= 1.0f;
  }
  if (y < 0.0f) {
    y -= 1.0f;
  }
  return NTempest::C2iVector(static_cast<int>(x), static_cast<int>(y));
}

static NTempest::C3Vector AreaToCoordinate(const NTempest::C2iVector &coords) {
  return NTempest::C3Vector(HALF_WORLD_SIZE_Y - coords.y * AREA_WORLD_SIZE_X, 17066.666f - coords.x * AREA_WORLD_SIZE_Y, 0.0f);
}

static void BuildPathName(const NTempest::C2iVector &location, char *buffer, UINT size) {
  FATALASSERT(buffer);
  FATALASSERT(size);
  buffer[0] = 0;

  const MapRec *map = g_mapDB.GetRecord(s_currentContinent);
  if (map && *map->m_Directory) {
    SStrPrintf(buffer, size, FILENAME_TEMPLATE, map->m_Directory, location.x, location.y);
    MINIMAPMD5NAME *name = s_md5NameHash.Ptr(buffer);
    if (name) {
      SStrPrintf(buffer, size, "%s\\%s", MINIMAP_MD5_DIR, name->filename);
    } else {
      SysMsgPrintf(SYSMSG_ERROR, 2, "No minimap texture: \"%s\"", buffer);
      buffer[0] = 0;
    }
  } else {
    SysMsgPrintf(SYSMSG_ERROR, 2, "NOCONTINENTNAME|%d", s_currentContinent);
  }
}

static void SetupTextureHandles(const NTempest::C2iVector &upperLeftArea, int continentChanged, QUADDATA *quads) {
  UINT i;
  static const struct {
    UINT xIncrement;
    UINT yIncrement;
  } s_areaCoordOffsets[4] = {
      {0, 0},
      {1, 0},
      {1, 1},
      {0, 1}
  };

  for (i = 0; i < 4; ++i) {
    quads[i].m_flags &= ~2u;
  }

  if (!continentChanged) {
    for (i = 0; i < 4; ++i) {
      NTempest::C2iVector currentArea(upperLeftArea.x + s_areaCoordOffsets[i].xIncrement, upperLeftArea.y + s_areaCoordOffsets[i].yIncrement);
      for (UINT n = 0; n < 4; ++n) {
        if (quads[n].m_areaNum.x == currentArea.x && quads[n].m_areaNum.y == currentArea.y) {
          if (n != i) {
            if (quads[i].m_texture) {
              HandleClose(quads[i].m_texture);
            }
            quads[i].m_texture = quads[n].m_texture;
            quads[i].m_areaNum = currentArea;
            quads[n].m_texture = 0;
          }
          quads[i].m_flags |= 2;
        }
      }
    }
  }

  for (i = 0; i < 4; ++i) {
    if (!(quads[i].m_flags & 2)) {
      NTempest::C2iVector currentArea(upperLeftArea.x + s_areaCoordOffsets[i].xIncrement, upperLeftArea.y + s_areaCoordOffsets[i].yIncrement);
      char                fileName[MAX_PATH];
      BuildPathName(currentArea, fileName, sizeof(fileName));
      if (!fileName[0]) {
        SysMsgPrintf(SYSMSG_ERROR, 4, "MINIMAPCHUNKNOTFOUND|%d|%d", currentArea.x, currentArea.y);
      } else {
        if (quads[i].m_texture) {
          HandleClose(quads[i].m_texture);
          quads[i].m_texture = 0;
        }
        CStatus status;
        quads[i].m_texture = TextureCreate(fileName, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &status, 0);
        quads[i].m_areaNum = currentArea;
        quads[i].m_flags |= 2;
      }
    }
  }
}

static void SetupQuad(const UINT groupNum, QUADDATA &quadData, const CWorld::MinimapQuad &wmmQuad, const float localz, LPCSTR wmoName) {
  char fileName[MAX_PATH];

  quadData.m_flags |= 2;
  SStrPrintf(fileName, sizeof(fileName), s_mapObjTemplate, wmoName, wmmQuad.groupNum, wmmQuad.quad.x, wmmQuad.quad.y);
  MINIMAPMD5NAME *name = s_md5NameHash.Ptr(fileName);
  if (!name) {
    SysMsgPrintf(SYSMSG_ERROR, 2, "No minimap texture: \"%s\"", fileName);
    fileName[0] = 0;
  } else {
    SStrPrintf(fileName, sizeof(fileName), "%s\\%s", MINIMAP_MD5_DIR, name->filename);
  }

  if (quadData.m_texture) {
    HandleClose(quadData.m_texture);
  }
  if (fileName[0]) {
    CStatus status;
    quadData.m_texture = TextureCreate(fileName, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &status, 0);
  } else {
    quadData.m_texture = 0;
    quadData.m_flags &= ~2u;
  }

  quadData.m_areaNum = wmmQuad.quad;
  quadData.aaBox = wmmQuad.aaBox;
  quadData.groupNum = wmmQuad.groupNum;
  if (quadData.groupNum == groupNum) {
    quadData.sortz = 0.0f;
  } else {
    quadData.sortz = quadData.aaBox.Center().z - localz;
  }
}

static void SetupMapObj(DWORD hWorldObject, NTempest::C44Matrix &minimapMtx) {
  LPCSTR       wmoName;
  LPCSTR const kWorld = "World\\";

  CWorld::QueryMapObjMatrix(hWorldObject, &minimapMtx, &s_mapObjInvMtx);
  float scale = minimapMtx.Row0AsVec3()->SquaredMag();
  if (NTempest::CMath::fnotequal_(scale, 1.0f)) {
    scale = 1.0f / NTempest::CMath::sqrt_(scale);
    *minimapMtx.Row0AsVec3() *= scale / NTempest::CMath::sqrt_(minimapMtx.Row0AsVec3()->SquaredMag());
    *minimapMtx.Row1AsVec3() *= scale / NTempest::CMath::sqrt_(minimapMtx.Row1AsVec3()->SquaredMag());
    *minimapMtx.Row2AsVec3() *= scale / NTempest::CMath::sqrt_(minimapMtx.Row2AsVec3()->SquaredMag());
  }
  minimapMtx.d0 = 0.0f;
  minimapMtx.d1 = 0.0f;
  minimapMtx.d2 = 0.0f;
  minimapMtx.Rotate(angle * 0.017453292f, NTempest::C3Vector(0.0f, 0.0f, 1.0f), 1);

  CWorld::QueryMapObjFileName(hWorldObject, wmoName);
  FATALASSERT(SStrCmpI(wmoName, kWorld));
  wmoName += SStrLen(kWorld);
  SStrCopy(s_mapObjDir, wmoName, sizeof(s_mapObjDir));
  char *extension = SStrChrR(s_mapObjDir, '.');
  if (extension) {
    *extension = 0;
  }
}

static void LoadMD5Names() {
  char   md5file[MAX_PATH];
  LPVOID buffer;
  LPCSTR readCursor;

  SStrPrintf(md5file, sizeof(md5file), "%s\\md5translate.txt", MINIMAP_MD5_DIR);
  if (!SFile::LoadFile(md5file, &buffer, 0, 1, 0)) {
    return;
  }

  readCursor = static_cast<LPCSTR>(buffer);
  char line[MAX_PATH] = "";
  do {
    SStrTokenize(&readCursor, line, sizeof(line), "\r\n", 0);
    if (!*readCursor || !line[0]) {
      break;
    }

    if (SStrCmp(line, "dir:", SStrLen("dir:"))) {
      char *space = SStrChr(line, '\t');
      if (space) {
        *space = 0;
        MINIMAPMD5NAME *name = s_md5NameHash.Ptr(line);
        if (!name) {
          name = s_md5NameHash.New(line, 0, 0);
        }
        SStrCopy(name->filename, space + 1, sizeof(name->filename));
      }
    }
  } while (line[0] && *readCursor);

  SFile::Unload(buffer);
}

int MinimapInitialize(int continentID) {
  UINT i;

  s_numPoints = 0;
  for (int pass = 0; pass < 2; ++pass) {
    if (pass > 0) {
      s_pointsOfInterest.SetCount(s_numPoints);
      s_POIIsVisible.SetCount(s_numPoints);
      s_visibleNoIcon.SetCount(s_numPoints);
      memset(s_POIIsVisible.Ptr(), 0, s_POIIsVisible.Count() * sizeof(int));
      memset(s_visibleNoIcon.Ptr(), 0, s_visibleNoIcon.Count() * sizeof(int));
    }

    s_numPoints = 0;
    for (i = g_areaPOIDB.GetNumRecords(); i--;) {
      const AreaPOIRec *rec = g_areaPOIDB.GetRecordByIndex(i);
      if (rec->m_continentID == continentID && (rec->m_flags & 0x1)) {
        if (pass > 0) {
          s_pointsOfInterest[s_numPoints] = rec;
        }
        ++s_numPoints;
      }
    }
    ++s_numPoints;
  }

  s_pointsOfInterest[s_numPoints - 1] = &s_questPOI;
  s_questPOI.m_x = 0.0f;
  s_questPOI.m_y = 0.0f;
  s_questPOI.m_z = 0.0f;
  s_questPOI.m_icon = 5;
  s_questPOI.m_importance = 1;
  s_questPOI.m_name_lang[CURRENT_LANGUAGE] = s_questPOIName;
  for (i = 0; i < 3; ++i) {
    s_distantPOI[i] = -1;
    s_POIRotation[i] = -1.0f;
  }

  s_currentZoom = s_minimapZoomCVar->GetInt();
  s_currentInsideZoom = s_minimapInsideZoomCVar->GetInt();

  FrameScript_SignalEvent(198);
  LoadMD5Names();
  return 1;
}

void MinimapShutdown() {
  s_currentContinent = -1;
  s_currentPosition = NTempest::C3Vector(3.4028235e+38f);
  s_currentLowerRightArea = NTempest::C2iVector(-1);
  s_currentUpperLeftArea = NTempest::C2iVector(-1);
  s_mapObjID = 0;
  s_mapObjInstanceID = 0;
  s_mapObjGroupID = -1;
  s_flags = 0;
  s_md5NameHash.Clear();
}

BOOL MinimapUpdatePosition(UINT continent, const NTempest::C3Vector &pos, NTempest::C2Vector *centerPoint, float *radius, QUADDATA *quads);

BOOL MinimapUpdate(
    DWORD                     hWorldObject,
    UINT                      continent,
    const NTempest::C3Vector &pos,
    NTempest::C2Vector       &centerPoint,
    float                    &radius,
    QUADDATA                 *quads,
    MinimapTexParams         &mmtp
) {
  BYTE needsWork = static_cast<BYTE>(s_flags & 1);
  mmtp.updateTexture = mmtp.asyncTexWait | needsWork;

  if (pos.x != s_currentPosition.x || pos.y != s_currentPosition.y || continent != s_currentContinent) {
    UINT groupID;
    UINT mapObjID;
    UINT instanceID;

    needsWork = 1;
    if (CWorld::QueryMapObjIDs(hWorldObject, mapObjID, instanceID, groupID)) {
      mmtp.inside = 1;
      if (mapObjID != s_mapObjID || instanceID != s_mapObjInstanceID) {
        SetupMapObj(hWorldObject, mmtp.worldRotation);
        mmtp.invMapObjMtx = s_mapObjInvMtx;
        mmtp.updateTexture = 1;
        s_mapObjID = mapObjID;
        s_mapObjInstanceID = instanceID;
      }

      if (groupID != s_mapObjGroupID) {
        mmtp.updateTexture = 1;
        s_mapObjGroupID = groupID;
      }

      if (pos.x <= s_queryCenterBox.b.x || pos.y <= s_queryCenterBox.b.y || pos.x >= s_queryCenterBox.t.x || pos.y >= s_queryCenterBox.t.y) {
        mmtp.updateTexture = 1;
      }

      if (!mmtp.updateTexture) {
        mmtp.localOffset = NTempest::C44Matrix::mul3v33m_(pos - s_queryCenter, s_mapObjInvMtx);
      }

      if (!s_isInside) {
        s_isInside = 1;
        s_flags |= 1;
        FrameScript_SignalEvent(198);
      }

      s_currentPosition = pos;
      s_currentContinent = continent;
    } else {
      if (s_isInside) {
        s_isInside = 0;
        s_flags |= 1;
        FrameScript_SignalEvent(198);
      }

      mmtp.inside = 0;
      needsWork = 1;
      s_mapObjID = s_mapObjInstanceID = s_mapObjGroupID = -1;
    }
  }

  if (!needsWork) {
    return needsWork;
  }

  if (!mmtp.inside) {
    needsWork = MinimapUpdatePosition(continent, pos, &centerPoint, &radius, quads) != 0;
    return needsWork;
  }

  mmtp.size = s_minimapZoomSize[s_currentInsideZoom];
  NTempest::C2iVector ibox(static_cast<int>(floor(pos.x / mmtp.size)), static_cast<int>(floor(pos.y / mmtp.size)));
  s_queryCenterBox.b = NTempest::C3Vector(ibox.x * mmtp.size, ibox.y * mmtp.size, pos.z - mmtp.size * 0.5f);
  s_queryCenterBox.t = NTempest::C3Vector(mmtp.size, mmtp.size, mmtp.size * 0.5f) + s_queryCenterBox.b;
  s_queryCenter = s_queryCenterBox.Center();
  mmtp.localCenter = s_queryCenter * s_mapObjInvMtx;
  NTempest::C3Vector localPos = pos * s_mapObjInvMtx;
  mmtp.localOffset = localPos - mmtp.localCenter;

  NTempest::CAaBox worldBox = s_queryCenterBox;
  TSStackArray<CWorld::MinimapQuad> wmmQuads(_alloca(1024 * sizeof(CWorld::MinimapQuad)), 1024, 0);
  worldBox.b -= NTempest::C3Vector(mmtp.size);
  worldBox.t += NTempest::C3Vector(mmtp.size);
  CWorld::QueryMapObjMinimap(hWorldObject, worldBox, wmmQuads);

  for (UINT i = 0; i < wmmQuads.Count(); ++i) {
    SetupQuad(wmmQuads[0].groupNum, quads[i], wmmQuads[i], localPos.z, s_mapObjDir);
  }
  for (; i < 1024; ++i) {
    quads[i].m_flags &= ~2u;
  }

  s_flags &= ~1u;
  return needsWork;
}

BOOL MinimapUpdatePosition(UINT continent, const NTempest::C3Vector &pos, NTempest::C2Vector *centerPoint, float *radius, QUADDATA *quads) {
  FATALASSERT(radius);
  FATALASSERT(centerPoint);
  if (continent == s_currentContinent && pos.x == s_currentPosition.x && pos.y == s_currentPosition.y && pos.z == s_currentPosition.z &&
      !(s_flags & 1))
  {
    return 0;
  }

  int continentChanged = continent != s_currentContinent;
  s_currentPosition = pos;
  s_currentContinent = continent;
  UpdatePointsOfInterest();

  NTempest::C2iVector areaCoord = CoordinateToArea(pos);
  NTempest::C2iVector upperLeftArea = CoordinateToArea(NTempest::C3Vector(pos.x + HALF_AREA_WORLD_SIZE_X, pos.y + HALF_AREA_WORLD_SIZE_Y, 0.0f));
  NTempest::C2iVector lowerRightArea = CoordinateToArea(NTempest::C3Vector(pos.x - HALF_AREA_WORLD_SIZE_X, pos.y - HALF_AREA_WORLD_SIZE_Y, 0.0f));

  if (upperLeftArea.x == lowerRightArea.x) {
    if (lowerRightArea.x + 1 < 64) {
      ++lowerRightArea.x;
    } else {
      FATALASSERT(upperLeftArea.x);
      --upperLeftArea.x;
    }
  }
  if (upperLeftArea.y == lowerRightArea.y) {
    if (lowerRightArea.y + 1 < 64) {
      ++lowerRightArea.y;
    } else {
      FATALASSERT(upperLeftArea.y);
      --upperLeftArea.y;
    }
  }

  if ((s_flags & 1) || s_currentUpperLeftArea.x != upperLeftArea.x || s_currentUpperLeftArea.y != upperLeftArea.y ||
      s_currentLowerRightArea.x != lowerRightArea.x || s_currentLowerRightArea.y != lowerRightArea.y)
  {
    SetupTextureHandles(upperLeftArea, continentChanged, quads);
    s_currentUpperLeftArea = upperLeftArea;
    s_currentLowerRightArea = lowerRightArea;
  }
  s_flags &= ~1u;

  NTempest::C2Vector upperLeftCoordinate = AreaToCoordinate(upperLeftArea);
  NTempest::CRect    boxBoundary(upperLeftCoordinate.x, upperLeftCoordinate.y, upperLeftCoordinate.x - boxHeight, upperLeftCoordinate.y - boxWidth);
  FATALASSERT(( pos.x - CLOSEENOUGH ) < boxBoundary.t);
  FATALASSERT(( pos.x + CLOSEENOUGH ) > boxBoundary.b);
  FATALASSERT(( pos.y - CLOSEENOUGH ) < boxBoundary.l);
  FATALASSERT(( pos.y + CLOSEENOUGH ) > boxBoundary.r);

  NTempest::C2Vector center;
  center.x = (boxBoundary.l - pos.y) / boxHeight;
  center.y = (boxBoundary.t - pos.x) / boxWidth;
  FATALASSERT(s_currentZoom < (sizeof(s_chunksPerSizeAtZoom) / sizeof(s_chunksPerSizeAtZoom[0])));
  *radius = (33.333332f * (s_chunksPerSizeAtZoom[s_currentZoom] >> 1)) / boxHeight;
  *centerPoint = center;
  return 1;
}

void MinimapSetZoom(UINT zoomFactor) {
  UINT &zoom = s_isInside ? s_currentInsideZoom : s_currentZoom;
  UINT  oldZoom = zoom;

  zoom = min(zoomFactor, 5);

  if (oldZoom != zoom) {
    s_flags |= 1;
    if (s_isInside) {
      char buf[8];
      SStrPrintf(buf, sizeof(buf), "%d", s_currentInsideZoom);
      s_minimapInsideZoomCVar->Set(buf, true, false, false);
    } else if (s_minimapZoomCVar) {
      char buf[8];
      SStrPrintf(buf, sizeof(buf), "%d", s_currentZoom);
      s_minimapZoomCVar->Set(buf, true, false, false);
    }
  }
}

UINT MinimapGetZoom() {
  return s_isInside ? s_currentInsideZoom : s_currentZoom;
}

UINT MinimapGetZoomLevels() {
  return NUM_ZOOMS;
}

float MinimapGetViewRadius() {
  if (s_isInside) {
    return s_minimapZoomSize[s_currentInsideZoom];
  }

  return 33.333332f * (s_chunksPerSizeAtZoom[s_currentZoom] * 0.5f);
}

const TSGrowableArray<const AreaPOIRec *> &MinimapGetPOI(int &updatePOI) {
  updatePOI = s_updatePOI;
  s_updatePOI = 0;
  return s_visiblePOI;
}

BOOL MinimapGetDistantPOI(TSGrowableArray<POIDIRECTIONDATA> &directionData) {
  UINT i;

  if (!s_updateDistantPOI) {
    FATALASSERT(s_numDistantPOI == directionData.Count());
    for (i = 0; i < s_numDistantPOI; ++i) {
      directionData[i].rotation = s_POIRotation[i];
    }

    return 0;
  }

  directionData.SetCount(s_numDistantPOI);
  s_updateDistantPOI = 0;

  for (i = 0; i < s_numDistantPOI; ++i) {
    POIDIRECTIONDATA &data = directionData[i];
    SStrCopy(data.POIName, s_pointsOfInterest[s_distantPOI[i]]->m_name_lang[CURRENT_LANGUAGE], sizeof(data.POIName));
    data.rotation = s_POIRotation[i];
  }

  return 1;
}

float MinimapGetWorldRadius() {
  if (s_isInside) {
    return s_minimapZoomSize[s_currentInsideZoom];
  }

  FATALASSERT(s_currentZoom < NUM_ZOOMS);
  return 33.333332f * (s_chunksPerSizeAtZoom[s_currentZoom] * 0.5f);
}

void MinimapSetQuestPOI(float x, float y, int priority, LPCSTR name) {
  s_questPOI.m_x = x;
  s_questPOI.m_y = y;
  s_questPOI.m_importance = priority;
  SStrCopy(s_questPOIName, name, sizeof(s_questPOIName));
  UpdatePointsOfInterest();
}

void MinimapGetPartyMembers(PARTYMEMBERINFO array[]) {
  if (!ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__)) {
    return;
  }

  for (UINT i = 0; i < 5; ++i) {
    NTempest::C3Vector pos(0.0f, 0.0f, 0.0f);
    DWORDLONG          guid = 0;
    NTempest::C2Vector dist(s_currentPosition.x, s_currentPosition.y);

    if (i == 4) {
      CGUnit_C *player = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
      if (player) {
        guid = player->GetControlledGUID();
      }
    } else {
      guid = CGPartyInfo::GetMember(i);
    }

    CGUnit_C *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
    if (unit) {
      dist.x -= unit->GetPosition().x;
      dist.y -= unit->GetPosition().y;
      unit->GetPosition(pos);
    } else {
      if (i < 4 && CGPartyInfo::GetMember(i)) {
        CGPartyInfo::RemoteStats *stats = CGPartyInfo::GetRemoteStats(CGPartyInfo::GetMember(i));
        if (stats && stats->mapID == CGPlayer_C::GetNewContinentID()) {
          dist.x -= stats->pos.x;
          dist.y -= stats->pos.y;
          pos = stats->pos;
        } else {
          array[i].showBlip = 0;
          array[i].showArrow = 0;
          continue;
        }
      } else {
        array[i].showBlip = 0;
        array[i].showArrow = 0;
        continue;
      }
    }

    array[i].guid = guid;
    if (i == 4) {
      SStrCopy(array[i].name, unit->GetUnitName(), sizeof(array[i].name));
    } else {
      const NameCache *name = g_nameDBCache.GetRecord(CGPartyInfo::GetMember(i), 0, 0, 0);
      if (name) {
        SStrCopy(array[i].name, name->m_name, sizeof(array[i].name));
      } else {
        array[i].name[0] = 0;
      }
    }

    float minimapVisRadius = MinimapGetViewRadius();
    FATALASSERT(minimapVisRadius > 0.0f);

    float totalDistance = dist.Mag();
    if (totalDistance / minimapVisRadius > 0.8f) {
      array[i].showBlip = 0;
      array[i].showArrow = 1;
      array[i].rotation = CalculateFacingTo(s_currentPosition, pos);
    } else {
      array[i].showBlip = 1;
      array[i].showArrow = 0;
      array[i].position = pos;
    }
  }
}
