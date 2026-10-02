#ifndef WOW_SOURCE_WORLDCLIENT_WORLD_H
#define WOW_SOURCE_WORLDCLIENT_WORLD_H

#include "MapDefs.h"

#include <stpl.h>
#include <Tempest/c2vector.h>
#include <Tempest/c2ivector.h>
#include <Tempest/c3segment.h>
#include <Tempest/c3vector.h>
#include <Tempest/c4plane.h>
#include <Tempest/c4vector.h>
#include <Tempest/cimvector.h>
#include <Tempest/cirect.h>
#include <Tempest/cfacet.h>
#include <Tempest/c44matrix.h>
#include <Tempest/caabox.h>

#include <string.h>

namespace NTempest {
  class C3Vector;
  struct CFacet;
}  // namespace NTempest

class CWorldParam;
class Particulate;
class CGGameObject_C;
class CDetailDoodadInst;
class DNSky;
class CWFrustum;
class CWSoundEmitter;
class CGUnit_C;
class CGxPixelShader;
class CGxShaderParam;
class CGxTex;
class WMOAreaTableRec;
struct HMODEL__;
typedef HMODEL__ *HMODEL;
struct HTEXTURE__;
typedef HTEXTURE__ *HTEXTURE;
struct CMapEntity;
struct SMODoodadDef;
struct SMOPoly;

struct CWFacetData {
  TSGrowableArray<NTempest::CFacet> facets;
  TSGrowableArray<DWORDLONG>        gameObjects;
};

struct WorldObjCollisionHandlerData {
  HMODEL model;
  NTempest::CAaBox    collideExt;
  float               scale;
  NTempest::C44Matrix matrix;
};

class CWTriData {
 public:
  enum {
    MaxTriIndices = 0x1000,
    MaxVertexIndices = 0x3000,
    MaxBatches = 0x20
  };

  struct Batch {
    const NTempest::C44Matrix *matrix;
    const NTempest::C3Vector  *vertices;
    const NTempest::C3Vector  *normals;
    const WORD                *vertexIndices;
    const WORD                *triIndices;
    WORD                       indexCount;
    WORD                       triCount;
    WORD                       minIndex;
    WORD                       maxIndex;
    DWORD                      sourceID;

    WORD GetMinIndex() const {
      return minIndex;
    }

    WORD GetVertexCount() const {
      ASSERT(maxIndex >= minIndex);
      if (minIndex == 0xFFFF) {
        return 0;
      }
      return maxIndex - minIndex + 1;
    }

    WORD GetIndexCount() const {
      return indexCount;
    }

    WORD GetIndex(WORD i) const {
      ASSERT(i < indexCount);
      return vertexIndices[i];
    }

    const NTempest::C3Vector &GetVertex(WORD i) const {
      ASSERT(vertices);
      return vertices[i];
    }

    const NTempest::C3Vector &GetNormal(WORD i) const {
      ASSERT(normals);
      return normals[i];
    }
  };

 private:
  friend class CMap;
  friend class CMapObjGroup;

  static NTempest::C44Matrix matrices[MaxBatches];
  static WORD                vertexIndices[MaxVertexIndices];
  static WORD                triIndices[MaxTriIndices];
  static Batch               batches[MaxBatches];
  static UINT                nMatrices;
  static UINT                nVertexIndices;
  static UINT                nTriIndices;
  static UINT                nBatches;
  static NTempest::C44Matrix idMatrix;

  Batch *AllocBatch() {
    ASSERT(nBatches + 1 < MaxBatches);
    Batch *batch = &batches[nBatches++];
    memset(batch, 0, sizeof(*batch));
    batch->minIndex = 0xFFFF;
    return batch;
  }

  WORD *AllocVertexIndices(UINT count) {
    ASSERT(nVertexIndices + count < MaxVertexIndices);
    WORD *indices = &vertexIndices[nVertexIndices];
    nVertexIndices += count;
    return indices;
  }

  WORD *AllocTriIndices(UINT count) {
    ASSERT(nTriIndices + count < MaxTriIndices);
    WORD *indices = &triIndices[nTriIndices];
    nTriIndices += count;
    return indices;
  }

  NTempest::C44Matrix *AllocMatrix() {
    ASSERT(nMatrices + 1 < MaxBatches);
    return &matrices[nMatrices++];
  }

 public:
  CWTriData() {
    Clear();
  }

  void Clear() {
    nBatches = 0;
    nTriIndices = 0;
    nVertexIndices = 0;
    nMatrices = 0;
  }

  UINT GetNumBatches() const {
    return nBatches;
  }

  const Batch &GetBatch(UINT b) const {
    ASSERT(b < nBatches);
    return batches[b];
  }
};

void ProjectTex2d(const NTempest::CAaBox &box, NTempest::CImVector color, const NTempest::C44Matrix *basis, float fadeOffset);

class CWorld {
 public:
  enum ObjStatus {
    ObjStatus_Visible = 0x1,
    ObjStatus_Audible = 0x2
  };

  enum ObjFlags {
    ObjFlag_Collidable = 0x1,
    ObjFlag_NoShadow = 0x2,
    ObjFlag_AlwaysAnimate = 0x4
  };

  enum WorldQueryFlags {
    WQF_doodadCollision = 0x0001,
    WQF_doodadRender = 0x0002,
    WQF_doodadMask = 0x000F,
    WQF_mapobjCollision = 0x0010,
    WQF_mapobjRender = 0x0020,
    WQF_mapobjNoCamCollide = 0x0040,
    WQF_mapobjMask = 0x00F0,
    WQF_terrain = 0x0100,
    WQF_terrainMask = 0x0F00,
    WQF_noForceLoad = 0x1000,
    WQF_noWmoDoodad = 0x2000,
    WQF_render = WQF_mapobjRender | WQF_doodadRender | WQF_terrain,
    WQF_collision = WQF_mapobjCollision | WQF_doodadCollision | WQF_terrain
  };

  enum Enables {
    Enable_Doodads = 0x00000001,
    Enable_Chunks = 0x00000002,
    Enable_Lod = 0x00000004,
    Enable_Texture = 0x00000008,
    Enable_Sky = 0x00000010,
    Enable_Culling = 0x00000020,
    Enable_Shadow = 0x00000040,
    Enable_Collision = 0x00000080,
    Enable_MapObjs = 0x00000100,
    Enable_MapObjLight = 0x00000200,
    Enable_VertexLight = 0x00000400,
    Enable_MapObjTex = 0x00000800,
    Enable_Portals = 0x00001000,
    Enable_PortalVis = 0x00002000,
    Enable_NoFullAlpha = 0x00004000,
    Enable_NoAnimation = 0x00008000,
    Enable_DebugBSP = 0x00010000,
    Enable_CrappyBatches = 0x00020000,
    Enable_ZoneBounds = 0x00040000,
    Enable_MapObjBSP = 0x00080000,
    Enable_DetailDoodads = 0x00100000,
    Enable_ShowQuery = 0x00200000,
    Enable_AABoxes = 0x00400000,
    Enable_Trilinear = 0x00800000,
    Enable_Water = 0x01000000,
    Enable_Particulates = 0x02000000,
    Enable_LowDetail = 0x04000000,
    Enable_Specular = 0x08000000,
    Enable_PixelShaders = 0x10000000,
    Enable_ShowTris = 0x20000000,
    Enable_ShowNormals = 0x40000000,
    Enable_Anisotropic = 0x80000000
  };

  static void                      Initialize();
  static void                      Destroy();
  static void                      LoadMap(LPCSTR mapName, NTempest::C3Vector &position, int preLoad);
  static void                      UnloadMap();
  static bool                      MapIsDungeon();
  static void                      ClearCache();
  static void                      Preload(const NTempest::C3Vector &position);
  static void                      PrepareUpdate(const NTempest::C3Vector &position, const NTempest::C3Vector &target);
  static void                      Update();
  static void                      Render();
  static void                      RenderAlpha();
  static void                      UpdateDayNight(int forceFull, const NTempest::C3Vector *position);
  static void                      SetEnvironment();
  static const NTempest::C3Vector &GetCamPos();
  static const NTempest::C3Vector &GetCamTarget();
  static UINT                      QueryAreaId(float x, float y);
  static int                       QueryShadow(const NTempest::C3Vector &pos, NTempest::CImVector &argb);
  static int                       QueryObjectInside(DWORD hWorldObject);
  static int                       QueryObjectVisible(DWORD hWorldObject);
  static int                       QueryMapObjZoneName(DWORD hWorldObject, LPCSTR &zoneName);
  static int                       QueryMapObjSubzoneName(DWORD hWorldObject, LPCSTR &subzoneName, UINT &subzoneId);
  static int                       QueryMapObjFileName(DWORD hWorldObject, LPCSTR &fileName);
  static bool                      QueryMapObjIDs(DWORD hWorldObject, UINT &wmoID, UINT &instanceID, UINT &groupID);
  static int                       QueryMapObjFog(DWORD hWorldObject, SMOFog::Fogs &oFogs, float &oPct);
  static int                       QueryGroundType(DWORD hWorldObject, UINT &groundType);
  static bool                      QueryMountAllowed(DWORD hWorldObject, bool &allowed);
  static bool QueryMapObjAreaTable(DWORD hWorldObject, const WMOAreaTableRec *&subzoneRec, const WMOAreaTableRec *&globalRec);

  struct MinimapQuad {
    UINT                groupNum;
    NTempest::C2iVector quad;
    NTempest::CAaBox    aaBox;
  };

  static bool QueryMapObjMinimap(DWORD hWorldObject, const NTempest::CAaBox &aaBox, TSStackArray<MinimapQuad> &quads);
  static bool QueryMapObjMatrix(DWORD hWorldObject, NTempest::C44Matrix *mtx, NTempest::C44Matrix *invMtx);
  static BOOL QueryObjectLiquid(DWORD hWorldObject, UINT &liquid, float &surface, NTempest::C3Vector &flowDir, int &deep);
  static int  QueryLiquidStatus(const NTempest::C3Vector &point, UINT &liquid, float &surface, NTempest::C3Vector &waterDir);
  static int  QueryLiquidFishable(const NTempest::C3Vector &point, int &fishable);

  static const UINT MAX_SOUND_EXT_LEVEL;
  static const UINT MIN_SOUND_EXT_LEVEL;

  static int    QueryLiquidSounds(DWORD hWorldObject, float radius, int *lbool, NTempest::C3Vector *ldelta);
  static UINT   SceneCamLiquidStatus();
  static UINT   ObjectCreate(LPCSTR name, NTempest::C3Vector &pos, float angle, BOOL bWait, BOOL bSnap, DWORDLONG param64);
  static void   ObjectUpdate(UINT id, NTempest::C3Vector &pos, float angle, BOOL bSnap);
  static void   ObjectGetExtents(UINT id, NTempest::CAaBox &extents);
  static void   ObjectEnableCollision(UINT id, int enable);
  static bool   ObjectTestConvexVolume(UINT id, const NTempest::C3Vector &pos);
  static void   ObjectDelete(UINT id);
  static void   SetObjectHandler(int (*handler)(LPVOID, DWORD, DWORDLONG, DWORD), LPVOID handlerParam);
  static void   SetObjectCollisionHandler(int (*handler)(DWORDLONG, DWORD, WorldObjCollisionHandlerData *));
  static DWORD  AddObject(DWORDLONG param64, DWORD param32, HMODEL hModel, UINT objFlags);
  static DWORD  AddDoodad(LPCSTR fileName, HMODEL hModel, const NTempest::C44Matrix &mat, UINT objFlags);
  static HMODEL GetModel(DWORD doodad);
  static void   UpdateObject(DWORD hWorldObject, const NTempest::C44Matrix &mat, const NTempest::CAaBox &aaBox);
  static void   RemoveObject(DWORD hWorldObject);
  static void   SetHidden(DWORD hWorldObject, int hidden);
  static void   TickObject(DWORD hWorldObject);
  static void   SetObjectRenderCallback(DWORD hWorldObject, void (*cb)(LPVOID, const NTempest::C44Matrix &), LPVOID param);
  static void   SetCameraTarget(DWORD hWorldObject);
  static void   SetUpdateTime(float elapsedSec, DWORD pCurTimeMs);
  static void   SelectLight(LPVOID parm, NTempest::C3Vector worldPos, const NTempest::C3Vector &cameraWorldPos, UINT maxLightsToUse);
  static float  CalcAltitude(float x, float y, float radius);
  static bool Intersect(const NTempest::C3Vector *a, const NTempest::C3Vector *b, float radius, NTempest::C3Vector *ip, float *dist, UINT queryFlags);
  static void GetFacets(const NTempest::CAaBox &aaBox, CWFacetData *facetData, UINT queryFlags);
  static void GetFacets(const CWFrustum &frustum, CWFacetData *facetData, UINT queryFlags);
  static bool GetFacet(const NTempest::C3Segment &seg, float &t, NTempest::C4Plane &facet, UINT queryFlags);
  static bool GetTris(const NTempest::CAaBox &aaBox, CWTriData &triData, UINT queryFlags);
  static bool GetTris(const NTempest::C3Segment &seg, float &t, CWTriData &triData, UINT queryFlags);
  static void TriDataToFacetData(const CWTriData &triData, CWFacetData &facetData, DWORDLONG param64);
  static void DBGShowQuery(bool show);
  static void SetSoundEmitterHandlers(void (*create)(CWSoundEmitter &), void (*destroy)(DWORD));
  static void WaterRipple(const NTempest::C3Vector &pos, float len, float time, float amp, float vel, float freq);
  static LPCSTR QueryChunkName();
  static BOOL   NDCClip(NTempest::C3Vector *p_inVerts, UINT p_inCount, NTempest::C3Vector **&p_outVerts, UINT &p_outCount);
  static bool   NDCXform(const CWFrustum &frustum, NTempest::C44Matrix &xf, bool translate);
  static void   SetShadowColor(NTempest::CImVector &color);
  static void   SetFarClip(float farClip);
  static void   SetNearClip(float nearClip);
  static void   SetDetailDoodadDensity(UINT density);
  static void   SetTexLodBias(float bias);
  static void   SetTexAnisotropy(UINT anisotropy);
  static bool   SetLodDist(float dist);
  static bool   SetTextureLodDist(float dist);

 private:
  friend class CDetailDoodadInst;
  friend class DNSky;
  friend class CWorldParam;
  friend class CWorldScene;
  friend class CMapChunk;
  friend class CMapObj;
  friend class CGUnit_C;
  friend class CGGameObject_C;
  friend class CMap;
  friend class CMapArea;
  friend class CMapObjGroup;
  friend void        ShadowRender_LOD1(HMODEL hModel, const NTempest::C44Matrix &basis, LPVOID param);
  friend HTEXTURE CharCustomizationLoadSkin(HMODEL characterModel, LPCSTR skinName, UINT raceID, UINT sexID, UINT textureNumber, BOOL isNPC);
  friend HTEXTURE CharCustomizationSetSkin(HMODEL characterModel, UINT raceID, UINT sexID, UINT textureNumber, BOOL isNPC);

  static void CalcFPS();
  static void PrepareAreaOfInterest(const NTempest::C3Vector &position, const NTempest::C3Vector &target);
  static void ModelGeoProjectCallback(const NTempest::CAaBox &worldBox, NTempest::CImVector color, const NTempest::C44Matrix &basis);
  static BOOL AnimBoneProjectCallback(const NTempest::C3Segment &seg, float &z);
  static BOOL ParticleProjectCallback(const NTempest::C3Segment &seg, float &z);

  static float               curTimeSec;
  static float               tickTimeSec;
  static UINT                curTimeMs;
  static UINT                tickTimeMs;
  static UINT                frameCnt;
  static UINT                chunkCnt;
  static UINT                nChunksRender;
  static UINT                nDoodadsRender;
  static UINT                nPrimsRender;
  static NTempest::CiRect    chunkRectHi;
  static NTempest::CiRect    gbChunkRect;
  static NTempest::CiRect    areaRect;
  static NTempest::C2iVector chunkAoiSize;
  static int                 prepareAll;
  static NTempest::C44Matrix idMat;
  static NTempest::C4Vector  texVect[8];
  static float               detailDoodadDist;
  static float               detailDoodadDistS;
  static UINT                detailDoodadDensity;
  static int                 detailDoodadTest;
  static UINT                detailDoodadAlphaRef;
  static float               textureLodDist;
  static float               lodDist;
  static UINT                lodMax;
  static UINT                lodMin;
  static UINT                pnEstimateVertex;
  static UINT                pnEstimateIndex;
  static UINT                pnt0EstimateVertex;
  static UINT                pnt0EstimateIndex;
  static UINT                pnct0EstimateVertex;
  static UINT                pnct0EstimateIndex;
  static float               farFog;
  static float               farClip;
  static float               nearClip;
  static float               unitDrawDist;
  static NTempest::CAaBox    groupAoi;
  static NTempest::CAaBox    objectAoi;
  static DWORD               enables;
  static DWORD               enableLayerCnt;
  static UINT                maxLights;
  static NTempest::CImVector shadowColor;
  static UINT                shadowModColor[64];
  static CGxTex             *shadowModGxTex;
  static UINT                shadowMipLevel;
  static UINT                alphaMipLevel;
  static float               texLodBias;
  static UINT                texMaxAnisotropy;
  static UINT                texMaxAnisotropyLog2;
  static Particulate        *particulate;
  static BOOL                bLoadSimpleDoodads;
  static BOOL                bShowSimpleDoodads;

 public:
  static DWORD GetEnables();
  static float GetCurTimeSec() {
    return curTimeSec;
  }
  static float GetTickTimeSec() {
    return tickTimeSec;
  }
  static UINT GetCurTimeMs() {
    return curTimeMs;
  }
  static UINT GetTickTimeMs() {
    return tickTimeMs;
  }
  static float GetFramerate();
  static UINT  GetPrimsRendered();
  static UINT  GetChunksRendered();
  static UINT  GetDoodadsRendered();
  static void  GetCounts(int counts[]);
  static float GetFarClip();
  static float GetNearClip();
  static UINT  GetTexMaxAnisotropyLog2();

 private:
  static int  ConsoleCommand_DebugBSP(LPCSTR command, LPCSTR arguments);
  static int  ConsoleCommand_ShowTerrain(LPCSTR command, LPCSTR arguments);
  static int  ConsoleCommand_ShowDoodads(LPCSTR command, LPCSTR arguments);
  static int  ConsoleCommand_ShowCollision(LPCSTR command, LPCSTR arguments);
  static int  ConsoleCommand_ShowAABoxes(LPCSTR command, LPCSTR arguments);
  static int  ConsoleCommand_ShowQuery(LPCSTR command, LPCSTR arguments);
  static int  ConsoleCommand_ShowTris(LPCSTR command, LPCSTR arguments);
  static int  ConsoleCommand_ShowNormals(LPCSTR command, LPCSTR arguments);
  static int  ConsoleCommand_ShowCrappyBatches(LPCSTR command, LPCSTR arguments);
  static int  ConsoleCommand_ShowMapObjs(LPCSTR command, LPCSTR arguments);
  static int  ConsoleCommand_ShowMapObjLight(LPCSTR command, LPCSTR arguments);
  static int  ConsoleCommand_ShowMapObjBSP(LPCSTR command, LPCSTR arguments);
  static int  ConsoleCommand_ShowMapObjTex(LPCSTR command, LPCSTR arguments);
  static int  ConsoleCommand_ShowPortals(LPCSTR command, LPCSTR arguments);
  static BOOL ConsoleCommand_ShowDetailDoodads(LPCSTR command, LPCSTR arguments);
  static BOOL ConsoleCommand_ShowCull(LPCSTR command, LPCSTR arguments);
  static BOOL ConsoleCommand_ShowSimpleDoodads(LPCSTR command, LPCSTR arguments);
  static BOOL ConsoleCommand_MaxLOD(LPCSTR, LPCSTR arguments);
  static BOOL ConsoleCommand_WaterMaxLOD(LPCSTR, LPCSTR arguments);
  static BOOL ConsoleCommand_WaterWaves(LPCSTR, LPCSTR arguments);
  static BOOL ConsoleCommand_WaterSpecular(LPCSTR, LPCSTR arguments);
  static BOOL ConsoleCommand_WaterRipples(LPCSTR, LPCSTR arguments);
  static BOOL ConsoleCommand_WaterShow(LPCSTR command, LPCSTR arguments);
  static BOOL ConsoleCommand_WaterParticulates(LPCSTR command, LPCSTR arguments);
  static int  ConsoleCommand_Proj(LPCSTR command, LPCSTR arguments);
  static BOOL ConsoleCommand_SetShadow(LPCSTR, LPCSTR arguments);
  static BOOL ConsoleCommand_MapObjLightMode(LPCSTR command, LPCSTR arguments);
  static int  ConsoleCommand_PortalVis(LPCSTR command, LPCSTR arguments);
  static int  ConsoleCommand_DebugZones(LPCSTR command, LPCSTR arguments);
  static int  ConsoleCommand_DetailDoodadTest(LPCSTR command, LPCSTR arguments);
  static BOOL ConsoleCommand_DetailDoodadAlpha(LPCSTR, LPCSTR arguments);
  static int  ConsoleCommand_GroupOnly(LPCSTR command, LPCSTR arguments);
  static BOOL ConsoleCommand_ShowShadow(LPCSTR command, LPCSTR arguments);
  static BOOL ConsoleCommand_ShowLowDetail(LPCSTR command, LPCSTR arguments);
  static BOOL ConsoleCommand_EnumTextures(LPCSTR, LPCSTR name);
  static BOOL ConsoleCommand_EnumTextureGxCache(LPCSTR, LPCSTR name);
};

NODEDECL(WaterRadWave) {
  static const float PERTURB;

  float              decay;
  float              curTime;
  float              ra;
  float              rb;
  float              ooLength;
  float              ooTimeLength;
  NTempest::C3Vector pos;
  float              length;
  float              timeLength;
  float              amplitude;
  float              velocity;
  float              frequency;

  void Init(const NTempest::C3Vector &p_pos, float len, float time, float amp, float vel, float freq);
  int  Update(float deltat);
};

#endif
