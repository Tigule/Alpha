#include "Game/GameClient/Minimap.h"

#include "Console/ConsoleVar.h"
#include "DB/DBClient/AutoCode/AreaPOIRec.h"
#include "DB/DBClient/AutoCode/MapRec.h"
#include "DB/DBClient/DBCacheInstances.h"
#include "Object/ObjectClient/Unit_C.h"
#include "Object/ObjectClient/Player_C.h"
#include "Object/Object.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "Ui/PartyFrame.h"
#include "WorldClient/World.h"

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

struct MINIMAPMD5NAME : public TSHashObject<MINIMAPMD5NAME, HASHKEY_STRI> {
  char filename[40];
};

extern CVar *s_minimapZoomCVar;
extern CVar *s_minimapInsideZoomCVar;

static UINT                s_currentContinent = -1;
static NTempest::C3Vector  s_currentPosition(0.0f, 0.0f, -1.0f);
static NTempest::C2iVector s_currentUpperLeftArea(-1);
static NTempest::C2iVector s_currentLowerRightArea(-1);
static const UINT          s_chunksPerSizeAtZoom[6] = {14, 12, 10, 8, 6, 4};
static const float         s_minimapZoomSize[6] = {150.0f, 120.0f, 90.0f, 60.0f, 40.0f, 25.0f};
static const float                               AREA_WORLD_SIZE_X = 533.33331f;
static const float                               AREA_WORLD_SIZE_Y = 533.33331f;
static const float                               CLOSEENOUGH = 0.013888889f;
static const float                               MAX_POI_DISTANCE = 694.44446f;
static float                                     angle = 90.0f;
static float                                     boxHeight = 1066.6666f;
static float                                     boxWidth = 1066.6666f;
static const float                               HALF_AREA_WORLD_SIZE_X = AREA_WORLD_SIZE_X * 0.5f;
static const float                               HALF_AREA_WORLD_SIZE_Y = AREA_WORLD_SIZE_Y * 0.5f;
static const float                               HALF_WORLD_SIZE_X = AREA_WORLD_SIZE_X * 64.0f * 0.5f;
static const float                               HALF_WORLD_SIZE_Y = AREA_WORLD_SIZE_Y * 64.0f * 0.5f;
static UINT                                      s_currentZoom = 3;
static UINT                                      s_currentInsideZoom = 3;
static UINT                                      s_mapObjID;
static UINT                                      s_mapObjInstanceID;
static UINT                                      s_mapObjGroupID = -1;
static BYTE                                      s_isInside;
static UINT                                      s_flags;
static NTempest::C44Matrix                       s_mapObjInvMtx;
static NTempest::CAaBox                          s_queryCenterBox;
static NTempest::C3Vector                        s_queryCenter;
static LPCSTR                                    MINIMAP_MD5_DIR = "Textures\\Minimap";
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
static LPCSTR                                    FILENAME_TEMPLATE = "%s\\map%d_%d.blp";
static LPCSTR                                    s_mapObjTemplate = "%s_%03d_%02d_%02d.blp";
static char                                      s_mapObjDir[MAX_PATH];

static void UpdatePointsOfInterest() {
  UINT               numPOI;
  NTempest::C2Vector dist;
  float              minimapVisRadius;
  float              totalDistance;

  s_visiblePOI.SetCount(0);
  for (numPOI = 0; numPOI < s_numPoints; ++numPOI) {
    const AreaPOIRec *poi = s_pointsOfInterest[numPOI];

    if (fabs(poi->m_x) >= 0.00000023841858f || fabs(poi->m_y) >= 0.00000023841858f) {
      dist.x = s_currentPosition.x - poi->m_x;
      dist.y = s_currentPosition.y - poi->m_y;
      FATALASSERT(s_currentZoom < 6);
      minimapVisRadius = s_chunksPerSizeAtZoom[s_currentZoom] * 0.5f * 33.333332f;
      FATALASSERT(minimapVisRadius > 0.0f);
      totalDistance = sqrt(dist.x * dist.x + dist.y * dist.y);

      if (totalDistance / minimapVisRadius > 0.8f) {
        if (s_POIIsVisible[numPOI] || s_visibleNoIcon[numPOI]) {
          s_updatePOI = 1;
          s_POIIsVisible[numPOI] = 0;
          s_visibleNoIcon[numPOI] = 0;
        }
        continue;
      }

      if (!(poi->m_flags & 0x2)) {
        if (s_POIIsVisible[numPOI] || !s_visibleNoIcon[numPOI]) {
          s_updatePOI = 1;
          s_POIIsVisible[numPOI] = 0;
          s_visibleNoIcon[numPOI] = 1;
        }
        continue;
      }

      s_visiblePOI.Add(1, &poi);
      if (!s_POIIsVisible[numPOI] || s_visibleNoIcon[numPOI]) {
        s_updatePOI = 1;
        s_POIIsVisible[numPOI] = 1;
        s_visibleNoIcon[numPOI] = 0;
      }
    } else if (s_POIIsVisible[numPOI] || s_visibleNoIcon[numPOI]) {
      s_updatePOI = 1;
      s_POIIsVisible[numPOI] = 0;
      s_visibleNoIcon[numPOI] = 0;
    }
  }

  int   priority[3];
  int   closest[3] = {-1, -1, -1};
  float distance[3] = {MAX_POI_DISTANCE, MAX_POI_DISTANCE, MAX_POI_DISTANCE};
  int   largest = -1;
  UINT  numDistantPOI = 0;

  for (numPOI = 0; numPOI < s_numPoints; ++numPOI) {
    const AreaPOIRec *poi = s_pointsOfInterest[numPOI];

    if ((fabs(poi->m_x) < 0.00000023841858f && fabs(poi->m_y) < 0.00000023841858f) || s_POIIsVisible[numPOI] || s_visibleNoIcon[numPOI]) {
      continue;
    }

    dist.x = poi->m_x - s_currentPosition.x;
    dist.y = poi->m_y - s_currentPosition.y;
    totalDistance = sqrt(dist.x * dist.x + dist.y * dist.y);
    if (totalDistance > MAX_POI_DISTANCE) {
      continue;
    }

    if (largest == -1) {
      distance[numDistantPOI] = totalDistance;
      closest[numDistantPOI] = numPOI;
      priority[numDistantPOI] = poi->m_importance;
      ++numDistantPOI;
    } else if (poi->m_importance < priority[largest] || (poi->m_importance == priority[largest] && totalDistance < distance[largest])) {
      distance[largest] = totalDistance;
      closest[largest] = numPOI;
      priority[largest] = poi->m_importance;
    }

    if (numDistantPOI == 3) {
      largest = 0;
      for (UINT i = 1; i < 3; ++i) {
        if (priority[i] > priority[largest] || (priority[i] == priority[largest] && distance[i] > distance[largest])) {
          largest = i;
        }
      }
    }
  }

  if (s_numDistantPOI != numDistantPOI) {
    s_numDistantPOI = numDistantPOI;
    s_updateDistantPOI = 1;
  } else {
    for (numPOI = 0; numPOI < numDistantPOI; ++numPOI) {
      if (s_distantPOI[numPOI] != closest[numPOI]) {
        s_updateDistantPOI = 1;
        break;
      }
    }
  }

  for (numPOI = 0; numPOI < numDistantPOI; ++numPOI) {
    s_distantPOI[numPOI] = closest[numPOI];
    const AreaPOIRec  *poi = s_pointsOfInterest[closest[numPOI]];
    NTempest::C3Vector poiPosition(poi->m_x, poi->m_y, s_currentPosition.z);
    s_POIRotation[numPOI] = CalculateFacingTo(s_currentPosition, poiPosition);
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
  if (!map || !map->m_Directory || !map->m_Directory[0]) {
    return;
  }

  SStrPrintf(buffer, size, FILENAME_TEMPLATE, map->m_Directory, location.x, location.y);
  MINIMAPMD5NAME *name = s_md5NameHash.Ptr(buffer);
  if (name) {
    SStrPrintf(buffer, size, "%s\\%s", MINIMAP_MD5_DIR, name->filename);
  } else {
    buffer[0] = 0;
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
      char                fileName[MAX_PATH];
      CStatus             status;
      NTempest::C2iVector currentArea(upperLeftArea.x + s_areaCoordOffsets[i].xIncrement, upperLeftArea.y + s_areaCoordOffsets[i].yIncrement);
      BuildPathName(currentArea, fileName, sizeof(fileName));
      if (!fileName[0]) {
        continue;
      }
      if (quads[i].m_texture) {
        HandleClose(quads[i].m_texture);
        quads[i].m_texture = 0;
      }
      quads[i].m_texture = TextureCreate(fileName, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &status, 0);
      quads[i].m_areaNum = currentArea;
      quads[i].m_flags |= 2;
    }
  }
}

static void SetupQuad(const UINT groupNum, QUADDATA &quadData, const CWorld::MinimapQuad &wmmQuad, const float localz, LPCSTR wmoName) {
  char    fileName[MAX_PATH];
  CStatus status;

  quadData.m_flags |= 2;
  SStrPrintf(fileName, sizeof(fileName), s_mapObjTemplate, wmoName, wmmQuad.groupNum, wmmQuad.quad.x, wmmQuad.quad.y);
  MINIMAPMD5NAME *name = s_md5NameHash.Ptr(fileName);
  if (name) {
    SStrPrintf(fileName, sizeof(fileName), "%s\\%s", MINIMAP_MD5_DIR, name->filename);
  } else {
    SysMsgPrintf(SYSMSG_ERROR, 2, "No minimap texture: \"%s\"", fileName);
    fileName[0] = 0;
  }

  if (quadData.m_texture) {
    HandleClose(quadData.m_texture);
  }
  if (fileName[0]) {
    quadData.m_texture = TextureCreate(fileName, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &status, 0);
  } else {
    quadData.m_texture = 0;
    quadData.m_flags &= ~2u;
  }

  quadData.groupNum = wmmQuad.groupNum;
  quadData.m_areaNum = wmmQuad.quad;
  quadData.aaBox = wmmQuad.aaBox;
  if (wmmQuad.groupNum == groupNum) {
    quadData.sortz = 0.0f;
  } else {
    quadData.sortz = (wmmQuad.aaBox.b.z + wmmQuad.aaBox.t.z) * 0.5f - localz;
  }
}

static void SetupMapObj(DWORD hWorldObject, NTempest::C44Matrix &minimapMtx) {
  LPCSTR wmoName;

  CWorld::QueryMapObjMatrix(hWorldObject, &minimapMtx, &s_mapObjInvMtx);
  float basisMag = minimapMtx.a0 * minimapMtx.a0 + minimapMtx.a1 * minimapMtx.a1 + minimapMtx.a2 * minimapMtx.a2;
  if (fabs(basisMag - 1.0f) >= 0.00000023841858f) {
    float basisScale = 1.0f / sqrt(basisMag);
    float rowMag = sqrt(minimapMtx.a0 * minimapMtx.a0 + minimapMtx.a1 * minimapMtx.a1 + minimapMtx.a2 * minimapMtx.a2);
    float rowScale = basisScale / rowMag;
    minimapMtx.a0 *= rowScale;
    minimapMtx.a1 *= rowScale;
    minimapMtx.a2 *= rowScale;

    rowMag = sqrt(minimapMtx.b0 * minimapMtx.b0 + minimapMtx.b1 * minimapMtx.b1 + minimapMtx.b2 * minimapMtx.b2);
    rowScale = basisScale / rowMag;
    minimapMtx.b0 *= rowScale;
    minimapMtx.b1 *= rowScale;
    minimapMtx.b2 *= rowScale;

    rowMag = sqrt(minimapMtx.c0 * minimapMtx.c0 + minimapMtx.c1 * minimapMtx.c1 + minimapMtx.c2 * minimapMtx.c2);
    rowScale = basisScale / rowMag;
    minimapMtx.c0 *= rowScale;
    minimapMtx.c1 *= rowScale;
    minimapMtx.c2 *= rowScale;
  }
  minimapMtx.d0 = 0.0f;
  minimapMtx.d1 = 0.0f;
  minimapMtx.d2 = 0.0f;
  minimapMtx.Rotate(angle * 0.017453292f, NTempest::C3Vector(0.0f, 0.0f, 1.0f), 1);

  if (!CWorld::QueryMapObjFileName(hWorldObject, wmoName)) {
    s_mapObjDir[0] = 0;
    return;
  }
  FATALASSERT(!SStrCmpI(wmoName, "World\\", SStrLen("World\\")));
  wmoName += SStrLen("World\\");
  SStrCopy(s_mapObjDir, wmoName, sizeof(s_mapObjDir));
  char *extension = SStrChrR(s_mapObjDir, '.');
  if (extension) {
    *extension = 0;
  }
}

void LoadMD5Names() {
  char   md5file[MAX_PATH];
  char   line[MAX_PATH];
  char  *space;
  LPVOID buffer;
  LPCSTR readCursor;

  SStrPrintf(md5file, sizeof(md5file), "%s\\md5translate.txt", MINIMAP_MD5_DIR);
  if (!SFile::LoadFile(md5file, &buffer, 0, 1, 0)) {
    return;
  }

  readCursor = static_cast<LPCSTR>(buffer);
  line[0] = 0;
  do {
    SStrTokenize(&readCursor, line, sizeof(line), "\r\n", 0);
    if (!*readCursor || !line[0]) {
      break;
    }

    if (SStrCmp(line, "dir:", SStrLen("dir:"))) {
      space = SStrChr(line, '\t');
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
  UINT numPoints = 0;

  for (int pass = 0; pass < 2; ++pass) {
    if (pass) {
      s_pointsOfInterest.SetCount(numPoints + 1);
      s_POIIsVisible.SetCount(numPoints + 1);
      s_visibleNoIcon.SetCount(numPoints + 1);
      memset(s_POIIsVisible.Ptr(), 0, sizeof(int) * s_POIIsVisible.Count());
      memset(s_visibleNoIcon.Ptr(), 0, sizeof(int) * s_visibleNoIcon.Count());
    }

    numPoints = 0;
    for (int index = g_areaPOIDB.GetNumRecords() - 1; index >= 0; --index) {
      const AreaPOIRec *rec = g_areaPOIDB.GetRecordByIndex(index);
      if (rec->m_continentID == continentID && (rec->m_flags & 1)) {
        if (pass) {
          s_pointsOfInterest[numPoints] = rec;
        }
        ++numPoints;
      }
    }
  }

  s_pointsOfInterest[numPoints] = &s_questPOI;
  s_numPoints = numPoints + 1;

  s_currentUpperLeftArea = NTempest::C2iVector(-1);
  s_currentLowerRightArea = NTempest::C2iVector(-1);
  s_mapObjID = 0;
  s_mapObjInstanceID = 0;
  s_mapObjGroupID = 0;
  s_currentInsideZoom = 5;
  s_currentZoom = 1;
  s_currentPosition = NTempest::C3Vector(-1.0f);

  s_questPOI.m_importance = 1;
  s_questPOI.m_icon = 5;
  s_questPOI.m_x = 0.0f;
  s_questPOI.m_y = 0.0f;
  s_questPOI.m_z = 0.0f;
  s_questPOI.m_name_lang[CURRENT_LANGUAGE] = s_questPOIName;

  s_queryCenter = NTempest::C3Vector(-1.0f);
  s_queryCenterBox = NTempest::CAaBox(-1.0f);
  s_currentZoom = s_minimapZoomCVar->GetInt();
  s_currentInsideZoom = s_minimapInsideZoomCVar->GetInt();

  FrameScript_SignalEvent(198);
  LoadMD5Names();
  return 1;
}

void MinimapShutdown() {
  s_currentContinent = -1;
  s_currentPosition = NTempest::C3Vector(0.0f, 0.0f, -1.0f);
  s_currentUpperLeftArea = NTempest::C2iVector(-1);
  s_currentLowerRightArea = NTempest::C2iVector(-1);
  s_queryCenter = NTempest::C3Vector(3.4028235e+38f);
  s_queryCenterBox = NTempest::CAaBox(-1.0f);
  s_isInside = 0;
  s_mapObjID = 0;
  s_mapObjInstanceID = 0;
  s_mapObjGroupID = 0;
  s_lowestVisiblePriority = -1;
  s_updateDistantPOI = 0;
  s_updatePOI = 0;
  s_md5NameHash.Clear();
}

static BOOL MinimapUpdatePosition(UINT continent, const NTempest::C3Vector &pos, NTempest::C2Vector *centerPoint, float *radius, QUADDATA *quads) {
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
  NTempest::C3Vector  corner(pos.x + HALF_AREA_WORLD_SIZE_X, pos.y + HALF_AREA_WORLD_SIZE_Y, 0.0f);
  NTempest::C2iVector upperLeftArea = CoordinateToArea(corner);
  corner.x = pos.x - HALF_AREA_WORLD_SIZE_X;
  corner.y = pos.y - HALF_AREA_WORLD_SIZE_Y;
  NTempest::C2iVector lowerRightArea = CoordinateToArea(corner);

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

  if ((s_flags & 1) || continentChanged || upperLeftArea.x != s_currentUpperLeftArea.x || upperLeftArea.y != s_currentUpperLeftArea.y ||
      lowerRightArea.x != s_currentLowerRightArea.x || lowerRightArea.y != s_currentLowerRightArea.y)
  {
    SetupTextureHandles(upperLeftArea, continentChanged, quads);
    s_currentUpperLeftArea = upperLeftArea;
    s_currentLowerRightArea = lowerRightArea;
  }
  s_flags &= ~1u;

  NTempest::C3Vector upperLeftCoordinate = AreaToCoordinate(upperLeftArea);
  NTempest::CRect    boxBoundary(upperLeftCoordinate.x, upperLeftCoordinate.y, upperLeftCoordinate.x - boxHeight, upperLeftCoordinate.y - boxWidth);
  FATALASSERT((pos.x - CLOSEENOUGH) < boxBoundary.t);
  FATALASSERT((pos.x + CLOSEENOUGH) > boxBoundary.b);
  FATALASSERT((pos.y - CLOSEENOUGH) < boxBoundary.l);
  FATALASSERT((pos.y + CLOSEENOUGH) > boxBoundary.r);

  NTempest::C2Vector center;
  center.x = (boxBoundary.l - pos.y) / boxHeight;
  center.y = (boxBoundary.t - pos.x) / boxWidth;
  FATALASSERT(s_currentZoom < 6);
  *radius = (s_chunksPerSizeAtZoom[s_currentZoom] >> 1) * 33.333332f / boxHeight;
  *centerPoint = center;
  return 1;
}

BOOL MinimapUpdate(
    DWORD                     hWorldObject,
    UINT                      continent,
    const NTempest::C3Vector &pos,
    NTempest::C2Vector       &centerPoint,
    float                    &radius,
    QUADDATA                 *quads,
    MinimapTexParams         &mmtp
) {
  UINT needsWork = s_flags & 1;
  mmtp.updateTexture = (s_flags & 1) | mmtp.asyncTexWait;

  if (continent == s_currentContinent && pos.x == s_currentPosition.x && pos.y == s_currentPosition.y && !(s_flags & 1)) {
    return 0;
  }

  UINT mapObjID;
  UINT instanceID;
  UINT groupID;
  UINT isInside = CWorld::QueryMapObjIDs(hWorldObject, mapObjID, instanceID, groupID);

  if (continent != s_currentContinent || pos.x != s_currentPosition.x || pos.y != s_currentPosition.y || pos.z != s_currentPosition.z) {
    needsWork = 1;
  }

  if (isInside != s_isInside) {
    s_isInside = isInside;
    s_flags |= 1;
    needsWork = 1;
    FrameScript_SignalEvent(198);
  }

  if (!isInside) {
    mmtp.inside = 0;
    s_mapObjID = -1;
    s_mapObjInstanceID = -1;
    s_mapObjGroupID = -1;
    return MinimapUpdatePosition(continent, pos, &centerPoint, &radius, quads) != 0;
  }

  s_currentContinent = continent;
  s_currentPosition = pos;

  mmtp.inside = 1;
  if (mapObjID != s_mapObjID || instanceID != s_mapObjInstanceID) {
    SetupMapObj(hWorldObject, mmtp.worldRotation);
    mmtp.invMapObjMtx = s_mapObjInvMtx;
    mmtp.updateTexture = 1;
    needsWork = 1;
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

  NTempest::C3Vector localPos = pos * s_mapObjInvMtx;
  if (!mmtp.updateTexture) {
    mmtp.localOffset = localPos - mmtp.localCenter;
  }

  mmtp.size = s_minimapZoomSize[s_currentInsideZoom];
  const float halfSize = mmtp.size * 0.5f;
  s_queryCenterBox = NTempest::CAaBox(
      NTempest::C3Vector(
          static_cast<float>(floor(pos.x / mmtp.size)) * mmtp.size, static_cast<float>(floor(pos.y / mmtp.size)) * mmtp.size, pos.z - halfSize
      ),
      NTempest::C3Vector(0.0f)
  );
  s_queryCenterBox.t = NTempest::C3Vector(s_queryCenterBox.b.x + mmtp.size, s_queryCenterBox.b.y + mmtp.size, s_queryCenterBox.b.z + halfSize);
  s_queryCenter = (s_queryCenterBox.b + s_queryCenterBox.t) * 0.5f;
  mmtp.localCenter = s_queryCenter * s_mapObjInvMtx;
  mmtp.localOffset = localPos - mmtp.localCenter;

  BYTE                              wmmStorage[sizeof(CWorld::MinimapQuad) * 1024];
  TSStackArray<CWorld::MinimapQuad> wmmQuads(wmmStorage, 1024, 0);
  NTempest::CAaBox                  queryBox = s_queryCenterBox;
  queryBox.b = queryBox.b - NTempest::C3Vector(mmtp.size);
  queryBox.t += NTempest::C3Vector(mmtp.size);
  CWorld::QueryMapObjMinimap(hWorldObject, queryBox, wmmQuads);
  UINT       count = wmmQuads.Count();
  UINT       quad;
  const UINT groupNum = count ? wmmQuads[0].groupNum : 0;
  for (quad = 0; quad < count; ++quad) {
    SetupQuad(groupNum, quads[quad], wmmQuads[quad], localPos.z, s_mapObjDir);
  }
  for (quad = count; quad < 1024; ++quad) {
    quads[quad].m_flags &= ~2u;
  }

  s_flags &= ~1u;
  return needsWork != 0;
}

void MinimapSetZoom(UINT zoomFactor) {
  char  buf[8];
  UINT &zoom = s_isInside ? s_currentInsideZoom : s_currentZoom;
  UINT  oldZoom = zoom;

  if (zoomFactor >= 5) {
    zoomFactor = 5;
  }
  zoom = zoomFactor;

  if (oldZoom != zoom) {
    s_flags |= 1;
    SStrPrintf(buf, sizeof(buf), "%d", zoom);
    CVar *zoomCVar = s_isInside ? s_minimapInsideZoomCVar : s_minimapZoomCVar;
    if (zoomCVar) {
      zoomCVar->Set(buf, true, false, false);
    }
  }
}

UINT MinimapGetZoom() {
  return s_isInside ? s_currentInsideZoom : s_currentZoom;
}

UINT MinimapGetZoomLevels() {
  return 6;
}

float MinimapGetViewRadius() {
  if (s_isInside) {
    return s_minimapZoomSize[s_currentInsideZoom];
  }

  return s_chunksPerSizeAtZoom[s_currentZoom] * 0.5f * 33.333332f;
}

const TSGrowableArray<const AreaPOIRec *> &MinimapGetPOI(int &updatePOI) {
  updatePOI = s_updatePOI;
  s_updatePOI = 0;
  return s_visiblePOI;
}

BOOL MinimapGetDistantPOI(TSGrowableArray<POIDIRECTIONDATA> &directionData) {
  UINT i;

  if (s_updateDistantPOI) {
    directionData.SetCount(s_numDistantPOI);
    s_updateDistantPOI = 0;

    for (i = 0; i < s_numDistantPOI; ++i) {
      const AreaPOIRec *poi = s_pointsOfInterest[s_distantPOI[i]];
      SStrCopy(directionData[i].POIName, poi->m_name_lang[CURRENT_LANGUAGE], sizeof(directionData[i].POIName));
      directionData[i].rotation = s_POIRotation[i];
    }

    return 1;
  }

  FATALASSERT(s_numDistantPOI == directionData.Count());
  for (i = 0; i < s_numDistantPOI; ++i) {
    directionData[i].rotation = s_POIRotation[i];
  }

  return 0;
}

float MinimapGetWorldRadius() {
  if (s_isInside) {
    return s_minimapZoomSize[s_currentInsideZoom];
  }

  FATALASSERT(s_currentZoom < 6);
  return s_chunksPerSizeAtZoom[s_currentZoom] * 0.5f * 33.333332f;
}

void MinimapSetQuestPOI(float x, float y, int priority, LPCSTR name) {
  s_questPOI.m_x = x;
  s_questPOI.m_y = y;
  s_questPOI.m_importance = priority;
  SStrCopy(s_questPOIName, name ? name : "", sizeof(s_questPOIName));
  s_updatePOI = 1;
}

void MinimapGetPartyMembers(PARTYMEMBERINFO *array) {
  if (!ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__)) {
    return;
  }

  for (UINT index = 0; index < 5; ++index) {
    NTempest::C3Vector pos;
    DWORDLONG          guid;
    NTempest::C2Vector dist;
    CGUnit_C          *unit;

    if (index == 4) {
      CGUnit_C         *activePlayer = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
      const CGUnitData *unitData = activePlayer->GetUnitData();
      guid = unitData->charm ? unitData->charm : unitData->summon;
    } else {
      guid = CGPartyInfo::GetMember(index);
    }

    unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
    if (unit) {
      unit->GetPosition(pos);
    } else if (index < 4 && guid) {
      CGPartyInfo::RemoteStats *stats = CGPartyInfo::GetRemoteStats(guid);
      if (stats && stats->mapID == static_cast<int>(CGPlayer_C::GetNewContinentID())) {
        pos = stats->pos;
      } else {
        array[index].showArrow = 0;
        array[index].showBlip = 0;
        continue;
      }
    } else {
      array[index].showArrow = 0;
      array[index].showBlip = 0;
      continue;
    }

    array[index].guid = guid;
    if (index == 4) {
      SStrCopy(array[index].name, unit->GetObjectName(), sizeof(array[index].name));
    } else {
      const NameCache *name = g_nameDBCache.GetRecord(guid, guid, 0, 0);
      if (name) {
        SStrCopy(array[index].name, name->m_name, sizeof(array[index].name));
      } else {
        array[index].name[0] = 0;
      }
    }

    float minimapVisRadius = MinimapGetViewRadius();
    FATALASSERT(minimapVisRadius > 0.0f);

    dist.x = s_currentPosition.x - pos.x;
    dist.y = s_currentPosition.y - pos.y;
    if (sqrt(dist.x * dist.x + dist.y * dist.y) / minimapVisRadius <= 0.8f) {
      array[index].showArrow = 0;
      array[index].showBlip = 1;
      array[index].position.x = pos.x;
      array[index].position.y = pos.y;
    } else {
      array[index].showArrow = 1;
      array[index].showBlip = 0;

      float x = pos.x - s_currentPosition.x;
      float y = pos.y - s_currentPosition.y;
      if (fabs(x) >= 0.00000023841858f) {
        if (fabs(y) >= 0.00000023841858f) {
          array[index].rotation = static_cast<float>(atan2(y, x));
        } else {
          array[index].rotation = pos.x >= s_currentPosition.x ? 0.0f : 3.1415927f;
        }
      } else {
        array[index].rotation = y >= 0.0f ? 0.5f * 3.1415927f : 1.5f * 3.1415927f;
      }
    }
  }
}
