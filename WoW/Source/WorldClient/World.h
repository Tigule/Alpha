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
struct HTEXTURE__;
struct CMapEntity;
struct SMODoodadDef;
struct SMOPoly;

struct CWFacetData {
  TSGrowableArray<NTempest::CFacet> facets;
  TSGrowableArray<unsigned __int64> gameObjects;
};

struct WorldObjCollisionHandlerData {
  HMODEL__           *model;
  NTempest::CAaBox    collideExt;
  float               scale;
  NTempest::C44Matrix matrix;
};

class Particulate {
 public:
  enum {
    MAX_PARTICLES = 4000,
    MAX_RENDER = 666
  };

  struct Particle {
    NTempest::C3Vector pos;
    float              scale;
  };

  struct Movement {
    NTempest::C3Vector dir;
    float              freq;
    float              time;
    float              amplitude;
  };

  Particulate(float particleScale, float boxSize, const char *particulateTexture);
  ~Particulate();

  void SetPercentage(float percent);
  void SetSize(float units);
  void SetScale(float s);
  void SetTexture(const char *name);
  void InitParticles(unsigned int l);
  void Show(unsigned char show) {
    this->show = show;
  }
  void                   Update();
  void                   Render();
  static void CustomRenderCallback(void *p1, int p2);

 private:
  void               InitMovement();
  NTempest::C3Vector ComputeMovement(float elapsedTime);

  static NTempest::C3Vector s_vcv[4];
  static NTempest::C2Vector s_tc[13][4];
  static unsigned int       s_tcSub[4][8];
  static const float        PTSIZE;
  Particle                  particles[MAX_PARTICLES];
  unsigned int              numParticles;
  NTempest::C3Vector        lastCamPos;
  HTEXTURE__               *texture;
  unsigned char             show;
  float                     scale;
  float                     boxSize;
  float                     percent;
  unsigned int              liquid;
  Movement                  movement;
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
    const unsigned short      *vertexIndices;
    const unsigned short      *triIndices;
    unsigned short             indexCount;
    unsigned short             triCount;
    unsigned short             minIndex;
    unsigned short             maxIndex;
    unsigned long              sourceID;

    unsigned short GetMinIndex() const {
      return minIndex;
    }

    unsigned short GetVertexCount() const {
      ASSERT(maxIndex >= minIndex);
      if (minIndex == 0xFFFF) {
        return 0;
      }
      return maxIndex - minIndex + 1;
    }

    unsigned short GetIndexCount() const {
      return indexCount;
    }

    unsigned short GetIndex(unsigned short i) const {
      ASSERT(i < indexCount);
      return vertexIndices[i];
    }

    const NTempest::C3Vector &GetVertex(unsigned short i) const {
      ASSERT(vertices);
      return vertices[i];
    }

    const NTempest::C3Vector &GetNormal(unsigned short i) const {
      ASSERT(normals);
      return normals[i];
    }
  };

  CWTriData() {
    Clear();
  }

  void Clear() {
    nBatches = 0;
    nTriIndices = 0;
    nVertexIndices = 0;
    nMatrices = 0;
  }

  unsigned int GetNumBatches() const {
    return nBatches;
  }

  const Batch &GetBatch(unsigned int b) const {
    ASSERT(b < nBatches);
    return batches[b];
  }

 private:
  friend class CMap;
  friend class CMapObjGroup;

  Batch *AllocBatch() {
    ASSERT(nBatches + 1 < MaxBatches);
    Batch *batch = &batches[nBatches++];
    memset(batch, 0, sizeof(*batch));
    batch->minIndex = 0xFFFF;
    return batch;
  }

  unsigned short *AllocVertexIndices(unsigned int count) {
    ASSERT(nVertexIndices + count < MaxVertexIndices);
    unsigned short *indices = &vertexIndices[nVertexIndices];
    nVertexIndices += count;
    return indices;
  }

  unsigned short *AllocTriIndices(unsigned int count) {
    ASSERT(nTriIndices + count < MaxTriIndices);
    unsigned short *indices = &triIndices[nTriIndices];
    nTriIndices += count;
    return indices;
  }

  NTempest::C44Matrix *AllocMatrix() {
    ASSERT(nMatrices + 1 < MaxBatches);
    return &matrices[nMatrices++];
  }

  static NTempest::C44Matrix matrices[MaxBatches];
  static unsigned short      vertexIndices[MaxVertexIndices];
  static unsigned short      triIndices[MaxTriIndices];
  static Batch               batches[MaxBatches];
  static unsigned int        nMatrices;
  static unsigned int        nVertexIndices;
  static unsigned int        nTriIndices;
  static unsigned int        nBatches;
  static NTempest::C44Matrix idMatrix;
};

void ProjectTex2d(const NTempest::CAaBox &box, NTempest::CImVector color, const NTempest::C44Matrix *basis, float fadeOffset);

class CWorld {
 public:
  struct MinimapQuad {
    unsigned int        groupNum;
    NTempest::C2iVector quad;
    NTempest::CAaBox    aaBox;
  };

  static HMODEL__ *GetModel(unsigned long doodad);
  static void SetObjectRenderCallback(unsigned long hWorldObject, void(*cb)(void *, const NTempest::C44Matrix &), void *param);
  static void SetObjectHandler(int(*handler)(void *, unsigned long, unsigned __int64, unsigned long), void *handlerParam);
  static void SetObjectCollisionHandler(int(*handler)(unsigned __int64, unsigned long, WorldObjCollisionHandlerData *));
  static void SetCameraTarget(unsigned long hWorldObject);
  static unsigned long AddObject(unsigned __int64 param64, unsigned long param32, HMODEL__ *hModel, unsigned int objFlags);
  static void RemoveObject(unsigned long hWorldObject);
  static unsigned long AddDoodad(const char *fileName, HMODEL__ *hModel, const NTempest::C44Matrix &mat, unsigned int objFlags);
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

  enum ObjFlags {
    ObjFlag_Collidable = 0x1,
    ObjFlag_NoShadow = 0x2,
    ObjFlag_AlwaysAnimate = 0x4
  };

  enum ObjStatus {
    ObjStatus_Visible = 0x1,
    ObjStatus_Audible = 0x2
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

  static const unsigned int MAX_SOUND_EXT_LEVEL;
  static const unsigned int MIN_SOUND_EXT_LEVEL;

  static void Initialize();
  static void Destroy();
  static void LoadMap(const char *mapName, NTempest::C3Vector &position, int preLoad);
  static bool MapIsDungeon();
  static void UnloadMap();
  static void ClearCache();
  static void Preload(const NTempest::C3Vector &position);
  static void PrepareUpdate(const NTempest::C3Vector &position, const NTempest::C3Vector &target);
  static void SetUpdateTime(float elapsedSec, unsigned long pCurTimeMs);
  static void Update();
  static void ObjectGetExtents(unsigned int id, NTempest::CAaBox &extents);
  static void ObjectEnableCollision(unsigned int id, int enable);
  static bool ObjectTestConvexVolume(unsigned int id, const NTempest::C3Vector &pos);
  static void ObjectDelete(unsigned int id);
  static void SetHidden(unsigned long hWorldObject, int hidden);
  static unsigned int QueryAreaId(float x, float y);
  static int QueryShadow(const NTempest::C3Vector &pos, NTempest::CImVector &argb);
  static unsigned int SceneCamLiquidStatus();
  static int QueryObjectInside(unsigned long hWorldObject);
  static int QueryObjectVisible(unsigned long hWorldObject);
  static int QueryLiquidSounds(unsigned long hWorldObject, float radius, int *lbool, NTempest::C3Vector *ldelta);
  static int QueryMapObjZoneName(unsigned long hWorldObject, const char *&zoneName);
  static int QueryMapObjSubzoneName(unsigned long hWorldObject, const char *&subzoneName, unsigned int &subzoneId);
  static int QueryMapObjFileName(unsigned long hWorldObject, const char *&fileName);
  static int QueryMapObjFog(unsigned long hWorldObject, SMOFog::Fogs &oFogs, float &oPct);
  static bool QueryMapObjMinimap(unsigned long hWorldObject, const NTempest::CAaBox &aaBox, TSStackArray<MinimapQuad> &quads);
  static bool QueryMapObjIDs(unsigned long hWorldObject, unsigned int &wmoID, unsigned int &instanceID, unsigned int &groupID);
  static bool QueryMapObjMatrix(unsigned long hWorldObject, NTempest::C44Matrix *mtx, NTempest::C44Matrix *invMtx);
  static const char *QueryChunkName();
  static bool QueryMapObjAreaTable(unsigned long hWorldObject, const WMOAreaTableRec *&subzoneRec, const WMOAreaTableRec *&globalRec);
  static int QueryObjectLiquid(unsigned long hWorldObject, unsigned int &liquid, float &surface, NTempest::C3Vector &flowDir, int &deep);
  static int QueryGroundType(unsigned long hWorldObject, unsigned int &groundType);
  static bool QueryMountAllowed(unsigned long hWorldObject, bool &allowed);
  static int QueryLiquidStatus(const NTempest::C3Vector &point, unsigned int &liquid, float &surface, NTempest::C3Vector &waterDir);
  static int QueryLiquidFishable(const NTempest::C3Vector &point, int &fishable);
  static void UpdateObject(unsigned long hWorldObject, const NTempest::C44Matrix &mat, const NTempest::CAaBox &aaBox);
  static void ObjectUpdate(unsigned int id, NTempest::C3Vector &pos, float angle, int bSnap);
  static unsigned int ObjectCreate(
      const char *name, NTempest::C3Vector &pos, float angle, int bWait, int bSnap, unsigned __int64 param64);
  static void TickObject(unsigned long hWorldObject);
  static void WaterRipple(const NTempest::C3Vector &pos, float len, float time, float amp, float vel, float freq);
  static float GetCurTimeSec() {
    return curTimeSec;
  }
  static float GetTickTimeSec() {
    return tickTimeSec;
  }
  static unsigned int GetCurTimeMs() {
    return curTimeMs;
  }
  static unsigned int GetTickTimeMs() {
    return tickTimeMs;
  }
  static const NTempest::C3Vector &GetCamPos();
  static const NTempest::C3Vector &GetCamTarget();
  static float GetFramerate();
  static unsigned int GetPrimsRendered();
  static unsigned int GetChunksRendered();
  static unsigned int GetDoodadsRendered();
  static void GetCounts(int counts[]);
  static unsigned long GetEnables();
  static float GetFarClip();
  static float GetNearClip();
  static unsigned int GetTexMaxAnisotropyLog2();
  static void SetEnvironment();
  static void UpdateDayNight(int forceFull, const NTempest::C3Vector *position);
  static void Render();
  static void RenderAlpha();
  static void
  SelectLight(void *parm, NTempest::C3Vector worldPos, const NTempest::C3Vector &cameraWorldPos, unsigned int maxLightsToUse);
  static void SetShadowColor(NTempest::CImVector &color);
  static void SetDetailDoodadDensity(unsigned int density);
  static void SetNearClip(float nearClip);
  static void SetFarClip(float farClip);
  static void SetTexLodBias(float bias);
  static void SetTexAnisotropy(unsigned int anisotropy);
  static bool SetLodDist(float dist);
  static bool SetTextureLodDist(float dist);
  static float CalcAltitude(float x, float y, float radius);
  static bool GetFacet(const NTempest::C3Segment &seg, float &t, NTempest::C4Plane &facet, unsigned int queryFlags);
  static bool
  Intersect(const NTempest::C3Vector *a, const NTempest::C3Vector *b, float radius, NTempest::C3Vector *ip, float *dist, unsigned int queryFlags);
  static void GetFacets(const NTempest::CAaBox &aaBox, CWFacetData *facetData, unsigned int queryFlags);
  static void GetFacets(const CWFrustum &frustum, CWFacetData *facetData, unsigned int queryFlags);
  static void TriDataToFacetData(const CWTriData &triData, CWFacetData &facetData, unsigned __int64 param64);
  static bool GetTris(const NTempest::C3Segment &seg, float &t, CWTriData &triData, unsigned int queryFlags);
  static bool GetTris(const NTempest::CAaBox &aaBox, CWTriData &triData, unsigned int queryFlags);
  static void DBGShowQuery(bool show);
  static void SetSoundEmitterHandlers(void(*create)(CWSoundEmitter &), void(*destroy)(unsigned long));
  static int NDCClip(NTempest::C3Vector *p_inVerts, unsigned int p_inCount, NTempest::C3Vector **&p_outVerts, unsigned int &p_outCount);
  static bool NDCXform(const CWFrustum &frustum, NTempest::C44Matrix &xf, bool translate);

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
  friend void ShadowRender_LOD1(HMODEL__ *hModel, const NTempest::C44Matrix &basis, void *param);
  friend HTEXTURE__ *CharCustomizationLoadSkin(
      HMODEL__    *characterModel,
      const char  *skinName,
      unsigned int raceID,
      unsigned int sexID,
      unsigned int textureNumber,
      int          isNPC
  );
  friend HTEXTURE__ *
  CharCustomizationSetSkin(HMODEL__ *characterModel, unsigned int raceID, unsigned int sexID, unsigned int textureNumber, int isNPC);

  static void ModelGeoProjectCallback(const NTempest::CAaBox &worldBox, NTempest::CImVector color, const NTempest::C44Matrix &basis);
  static int ParticleProjectCallback(const NTempest::C3Segment &seg, float &z);
  static int AnimBoneProjectCallback(const NTempest::C3Segment &seg, float &z);
  static void CalcFPS();
  static void PrepareAreaOfInterest(const NTempest::C3Vector &position, const NTempest::C3Vector &target);

  static int ConsoleCommand_ShowDetailDoodads(const char *command, const char *arguments);
  static int ConsoleCommand_ShowMapObjBSP(const char *command, const char *arguments);
  static int ConsoleCommand_DebugBSP(const char *command, const char *arguments);
  static int ConsoleCommand_ShowTerrain(const char *command, const char *arguments);
  static int ConsoleCommand_ShowDoodads(const char *command, const char *arguments);
  static int ConsoleCommand_ShowAABoxes(const char *command, const char *arguments);
  static int ConsoleCommand_ShowCollision(const char *command, const char *arguments);
  static int ConsoleCommand_MaxLOD(const char *__formal, const char *arguments);
  static int ConsoleCommand_ShowCull(const char *command, const char *arguments);
  static int ConsoleCommand_SetShadow(const char *__formal, const char *arguments);
  static int ConsoleCommand_ShowMapObjLight(const char *command, const char *arguments);
  static int ConsoleCommand_MapObjLightMode(const char *command, const char *arguments);
  static int ConsoleCommand_ShowMapObjTex(const char *command, const char *arguments);
  static int ConsoleCommand_ShowCrappyBatches(const char *command, const char *arguments);
  static int ConsoleCommand_ShowMapObjs(const char *command, const char *arguments);
  static int ConsoleCommand_ShowPortals(const char *command, const char *arguments);
  static int ConsoleCommand_PortalVis(const char *command, const char *arguments);
  static int ConsoleCommand_WaterShow(const char *command, const char *arguments);
  static int ConsoleCommand_WaterMaxLOD(const char *__formal, const char *arguments);
  static int ConsoleCommand_WaterWaves(const char *__formal, const char *arguments);
  static int ConsoleCommand_WaterSpecular(const char *__formal, const char *arguments);
  static int ConsoleCommand_WaterRipples(const char *__formal, const char *arguments);
  static int ConsoleCommand_WaterParticulates(const char *command, const char *arguments);
  static int ConsoleCommand_Proj(const char *command, const char *arguments);
  static int ConsoleCommand_ShowTris(const char *command, const char *arguments);
  static int ConsoleCommand_ShowNormals(const char *command, const char *arguments);
  static int ConsoleCommand_DebugZones(const char *command, const char *arguments);
  static int ConsoleCommand_ShowQuery(const char *command, const char *arguments);
  static int ConsoleCommand_DetailDoodadTest(const char *command, const char *arguments);
  static int ConsoleCommand_DetailDoodadAlpha(const char *__formal, const char *arguments);
  static int ConsoleCommand_GroupOnly(const char *command, const char *arguments);
  static int ConsoleCommand_ShowShadow(const char *command, const char *arguments);
  static int ConsoleCommand_ShowLowDetail(const char *command, const char *arguments);
  static int ConsoleCommand_ShowSimpleDoodads(const char *command, const char *arguments);
  static int ConsoleCommand_EnumTextures(const char *__formal, const char *name);
  static int ConsoleCommand_EnumTextureGxCache(const char *__formal, const char *name);

  static unsigned long       enables;
  static float               curTimeSec;
  static float               tickTimeSec;
  static unsigned int        curTimeMs;
  static unsigned int        tickTimeMs;
  static unsigned long       enableLayerCnt;
  static unsigned int        maxLights;
  static float               unitDrawDist;
  static unsigned int        frameCnt;
  static unsigned int        chunkCnt;
  static unsigned int        nChunksRender;
  static unsigned int        nDoodadsRender;
  static unsigned int        nPrimsRender;
  static NTempest::CiRect    chunkRectHi;
  static NTempest::CiRect    gbChunkRect;
  static NTempest::CiRect    areaRect;
  static NTempest::C2iVector chunkAoiSize;
  static int                 prepareAll;
  static NTempest::C44Matrix idMat;
  static NTempest::C4Vector  texVect[8];
  static float               detailDoodadDist;
  static float               detailDoodadDistS;
  static unsigned int        detailDoodadDensity;
  static int                 detailDoodadTest;
  static unsigned int        detailDoodadAlphaRef;
  static float               textureLodDist;
  static float               lodDist;
  static unsigned int        lodMax;
  static unsigned int        lodMin;
  static unsigned int        pnEstimateVertex;
  static unsigned int        pnEstimateIndex;
  static unsigned int        pnt0EstimateVertex;
  static unsigned int        pnt0EstimateIndex;
  static unsigned int        pnct0EstimateVertex;
  static unsigned int        pnct0EstimateIndex;
  static float               farFog;
  static float               farClip;
  static float               nearClip;
  static NTempest::CAaBox    groupAoi;
  static NTempest::CAaBox    objectAoi;
  static float               texLodBias;
  static unsigned int        texMaxAnisotropy;
  static unsigned int        texMaxAnisotropyLog2;
  static Particulate        *particulate;
  static CGxTex             *shadowModGxTex;
  static unsigned int        shadowMipLevel;
  static unsigned int        alphaMipLevel;
  static NTempest::CImVector shadowColor;
  static unsigned int        shadowModColor[64];
  static int                 bLoadSimpleDoodads;
  static int                 bShowSimpleDoodads;
};

#include "WorldClient/Map.h"

NODEDECL(WaterRadWave) {
  int                Update(float deltat);
  void               Init(const NTempest::C3Vector &p_pos, float len, float time, float amp, float vel, float freq);
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
};

class CMapArea : public CMapBaseObj {
 public:
  CMapArea();
  ~CMapArea();

  static void Initialize();
  static void AsyncPollHandler();
  static void Destroy();
  void                   Load(SMAreaInfo *areaInfo);
  void                   Purge();
  void                   PurgeChunks();
  void                   PrepareLocalRect();
  void                   InitWater();
  void                   QueryLiquidSounds(
      const NTempest::C3Vector &worldPos,
      float                      radius,
      int                       *lbool,
      NTempest::C3Vector        *ldelta,
      float                     *ldsquared
  );

  static int ccWaterLOD;
  static int ccWaterMaxLOD;
  static int ccWaterWaves;
  static int ccWaterSpecular;
  static int ccWaterRipples;
  static int ccWaterShowTri;

  LISTDECLEX(CMapBaseObjLink, refLink, chunkLinkList);
  unsigned long                      infoIndex;
  NTempest::C2iVector                mIndex;
  NTempest::C2iVector                cOffset;
  NTempest::CiRect                   localRect;
  unsigned int                       texCount;
  SMAreaHeader                       header;
  CAsyncObject                      *asyncObject;
  TSCArray<HTEXTURE__ *, 96>         texIdTable;
  TSGrowableArray<SMDoodadDef>       doodadDefList;
  TSGrowableArray<SMMapObjDef>       mapObjDefList;
  SMChunkInfo                        chunkInfo[256];
  CMapChunk                         *chunkTable[256];

 private:
  static void FreeAsyncLoadBuffer(unsigned char *buffer);
  static void InitAsyncLoadBuffers();
  static unsigned char *AllocAsyncLoadBuffer();
  void                            Create(unsigned char *data);
  void                            LoadTextures(char *texNames, unsigned long size);
  static void AsyncCallback(void *userArg);
};

#define LIQUID_COUNT 9
#define LIQUID_TEXTURE_COUNT 30

class CMap {
 public:
  static void Initialize();
  static void CalcMem();
  static void PrepareUpdate();
  static void Update();
  static void Unload();
  static void Load(const char *fileName);
  static void LoadWdl();
  static void LoadWdt();
  static void Preload();
  static void Open();
  static void GetCounts(int counts[]);
  static CMapDoodadDef *CreateDoodadDef(SMDoodadDef &smDoodadDef, NTempest::C3Vector &pos);
  static CMapDoodadDef *CreateDoodadDef(
      const char *fileName, NTempest::C3Vector &pos, float angle, int bWait);
  static CMapDoodadDef *CreateDoodadDef(
      unsigned int         doodadRef,
      SMODoodadDef        &smoDoodadDef,
      const char          *fileName,
      unsigned int         mapObjDefId,
      NTempest::C44Matrix &mapObjDefMat
  );
  static CMapObjDef *CreateMapObjDef(SMMapObjDef &smMapObjDef, NTempest::C3Vector &pos);
  static CMapObjDef *CreateMapObjDef(
      const char *fileName, NTempest::C3Vector &pos, float angle, int bWait);
  static void
  CreateMapObjDefGroupDoodads(CMapObj *mapObj, CMapObjGroup *mapObjGroup, CMapObjDef *mapObjDef, CMapObjDefGroup *mapObjDefGroup);
  static void CreateMapObjDefLights(CMapObj *mapObj, CMapObjGroup *mapObjGroup, CMapObjDef *mapObjDef, CMapObjDefGroup *mapObjDefGroup);
  static HTEXTURE__ *LoadTexture(const char *fileName);
  static void WaterRipple(const NTempest::C3Vector &pos, float len, float time, float amp, float vel, float freq);
  static void UpdateEntity(CMapEntity *entity);
  static void LinkEntity(CMapStaticEntity *entity);
  static bool QueryGroundType(const NTempest::C3Vector &pos, unsigned int &groundType);
  static bool QueryShadow(const NTempest::C3Vector &pos);
  static bool
  QueryLiquidStatus(const NTempest::C3Vector &point, unsigned int &liquid, float &surface, NTempest::C3Vector &waterDir, int &deep);
  static void QueryLiquidSounds(
      const NTempest::C3Vector &worldPos,
      float                      radius,
      int                       *lbool,
      NTempest::C3Vector        *ldelta,
      float                     *ldsquared
  );
  static bool
  QueryLiquidStatusMapObjsExt(const NTempest::C3Vector &point, unsigned int &liquid, float &surface, NTempest::C3Vector &waterDir);
  static bool QueryLiquidFishable(const NTempest::C3Vector &point, int &fishable);
  static bool QueryLiquidFishableMapObjsExt(const NTempest::C3Vector &point, int &fishable);
  static bool
  VectorIntersectTerrain(
      const NTempest::C3Vector *p0,
      const NTempest::C3Vector *p1,
      float                    *t,
      unsigned int              queryFlags,
      CMapChunk               **chunk
  );
  static bool VectorIntersect(
      const NTempest::C3Vector *p0,
      const NTempest::C3Vector *p1,
      NTempest::C3Vector       *ip,
      float                    *dist,
      unsigned int              queryFlags
  );
  static bool VectorIntersectMapObjs(
      const NTempest::C3Vector *p0,
      const NTempest::C3Vector *p1,
      unsigned int              queryFlags,
      unsigned int              polyIgnoreFlags,
      unsigned int              groupIgnoreFlags,
      float                    *t,
      SMOPoly                 **poly,
      CMapObj                 **qMapObj
  );
  static bool VectorIntersectDoodadDefLinkList(
      LISTEX(CMapBaseObjLink, refLink) &doodadDefLinkList,
      const NTempest::C3Vector           *p0,
      const NTempest::C3Vector           *p1,
      float                              *t,
      unsigned int                        queryFlags
  );
  static bool VectorIntersectGameObjLinkList(
      LISTEX(CMapBaseObjLink, refLink) &gameObjLinkList,
      const NTempest::C3Vector           *p0,
      const NTempest::C3Vector           *p1,
      float                              *t,
      unsigned int                        queryFlags
  );
  static bool LocateViewerMapObjs(
      const NTempest::C3Vector &lCen,
      const NTempest::C3Vector &lEnd,
      float                    &maxT,
      CMapObjDef              *&hitMapObjDef,
      unsigned int             *hitGroupIDs
  );
  static bool LocateViewerMapObjs4(
      const NTempest::C3Vector &lCen,
      const NTempest::C3Vector &lEnd,
      float                    &maxT,
      CMapObjDef              *&hitMapObjDef,
      unsigned int             *hitGroupIDs
  );
  static void SetLightFuncs();
  static void GxuLightInitialize();
  static void GxuLightShutdown();
  static unsigned long GxuLightCreate();
  static void GxuLightDestroy(unsigned long lightId);
  static CGxLight *GxuLightLock(unsigned long lightId);
  static void GxuLightUnlock(unsigned long lightId);
  static void GxuLightSelect(NTempest::C3Vector worldPos, const NTempest::C3Vector &cameraWorldPos, unsigned int maxLightsToUse);
  static void SelectLight(CMapBaseObj *baseObj);
  static void SelectLight(void *parm, NTempest::C3Vector worldPos, const NTempest::C3Vector &cameraWorldPos, unsigned int maxLightsToUse);
  static int GxuLightEnable(unsigned long lightId);
  static void GxuLightEnableSet(unsigned long lightId, int enable);
  static void GxuLightSetMaxLights(unsigned int maxLightsToUse);
  static float GxuLightBucketSize();
  static void GxuLightBucketSizeSet(float bucketSize);
  static void GxuLightResetCache();
  static CMapLight *CreateLight(bool dynamic);
  static void DestroyLight(CMapLight *light);
  static void UpdateLight(CMapLight *light);
  static void UpdateLightBounds(CMapLight *light);
  static void EnableLight(CMapLight *light);
  static void DisableLight(CMapLight *light);
  static bool EnablePixelShaders() {
    return enablePixelShaders;
  }
  static bool EnableSpecular() {
    return enableSpecular;
  }
  static bool EnableSpecularTerrain() {
    return enableSpecularTerrain;
  }
  static bool EnableSpecularWater() {
    return enableSpecularWater;
  }
  static bool EnableTerrainShader() {
    return enableTerrainShader;
  }
  static void Destroy();
  static void ClearDetailDoodads();
  static float PointIntersect(float wx, float wy, float radius);
  static bool GetPlane(float wx, float wy, NTempest::C4Plane &plane);
  static void MakeAllEntityNonVisible();
  static void OceanFFT();
  static void UpdateOcean();
  static void GxBufDynLowDetailCallback(CGxBufCommand &cmd, CGxBuf *buf);
  static void CreateAreaLowDetailVertices(CMapAreaLow *areaLow, const CGxBufCommand &cmd, CGxBuf *buf);
  static void CreateAreaLowDetailIndices(CMapAreaLow *areaLow, const CGxBufCommand &cmd, CGxBuf *buf);
  static CMapBaseObjLink *AllocBaseObjLink(CMapBaseObj *owner);
  static CMapObj *AllocMapObj();
  static CMapObjGroup *AllocMapObjGroup();
  static CChunkLayer *GetLayer();
  static CChunkTex *GetTex();
  static CMapArea *AllocArea();
  static CMapChunk *AllocChunk();
  static CMapDoodadDef *AllocDoodadDef();
  static CMapEntity *AllocEntity();
  static CMapObjDefGroup *AllocMapObjDefGroup();
  static CMapObjDef *AllocMapObjDef();
  static CChunkLiquid *AllocChunkLiquid();
  static CMapSoundEmitter *AllocSoundEmitter();
  static CMapLight *AllocLight();
  static CMapCacheLight *AllocCacheLight();
  static void SnapBaseObjToSubChunk(CMapBaseObj *baseObj, NTempest::C3Vector &pos, float angle);
  static void UpdateDoodadDef(CMapDoodadDef *doodadDef, NTempest::C3Vector &pos, float angle);
  static void UpdateMapObjDef(CMapObjDef *mapObjDef, CMapObj *mapObj);
  static void UpdateMapObjDef(CMapObjDef *mapObjDef, NTempest::C3Vector &pos, float angle);
  static void CreateChunkNeighborPtrs(CMapChunk *chunk);
  static void UpdateLiquidTextures();
  static void
  UpdateMapObjDefGroupDoodads(CMapObj *mapObj, CMapObjGroup *mapObjGroup, CMapObjDef *mapObjDef, CMapObjDefGroup *mapObjDefGroup);
  static void FreeBaseObjLink(CMapBaseObjLink *link);
  static void FreeLight(CMapLight *light);
  static void FreeCacheLight(CMapCacheLight *cacheLight);
  static void FreeMapObj(CMapObj *mapObj);
  static void FreeMapObjGroup(CMapObjGroup *group);
  static void FreeArea(CMapArea *area);
  static void FreeChunk(CMapChunk *chunk);
  static void FreeLayer(CChunkLayer *layer);
  static void FreeTex(CChunkTex *tex);
  static void FreeDoodadDef(CMapDoodadDef *doodadDef);
  static void FreeEntity(CMapEntity *entity);
  static void InitializeDoodadBounds(CMapDoodadDef *doodadDef);
  static int LoadDoodadModel(CMapDoodadDef *doodadDef, int bWait);
  static void PurgeDoodadDef(CMapDoodadDef *doodadDef);
  static void FreeChunkLiquid(CChunkLiquid *&cl);
  static void FreeSoundEmitter(CMapSoundEmitter *soundEmitter);
  static void FreeMapObjDefGroup(CMapObjDefGroup *mapObjDefGroup);
  static void FreeMapObjDef(CMapObjDef *mapObjDef);
  static void PurgeMapObjDef(CMapObjDef *mapObjDef);
  static void PurgeMapObjDefGroup(CMapObjDefGroup *mapObjDefGroup);
  static void PurgeArea(CMapArea *area);
  static void PurgeChunk(CMapChunk *chunk);
  static void ReloadDoodadModels();
  static void UnloadLiquidTexture(unsigned int liquid);
  static HTEXTURE__ *GetLiquidTexture(unsigned int liquid);
  static void ProjectLights();
  static void WaterInitialize();
  static void WaterDiffTexCallback(
      EGxTexCommand cmd,
      unsigned int  w,
      unsigned int  h,
      unsigned int  d,
      unsigned int  mipLevel,
      void         *userArg,
      unsigned int &texelStrideInBytes,
      const void  *&texels
  );
  static void WaterDestroy();
  static void EnableDoodadFullAlpha(int enable);
  static unsigned int QueryAreaId(float x, float y);
  static bool GetFacet(const NTempest::C3Segment &seg, float &t, NTempest::C4Plane &facet, unsigned int queryFlags);
  static bool GetFacets(const NTempest::CAaBox &aaBox, CWFacetData *facetData, unsigned int queryFlags);
  static bool GetFacets(const CWFrustum &frustum, CWFacetData *facetData, unsigned int queryFlags);
  static bool GetTris(const NTempest::CAaBox &aaBox, CWTriData &triData, unsigned int queryFlags);
  static void TestQueryAdd(const NTempest::CFacet &facet, NTempest::CImVector color, const NTempest::C44Matrix *basis);
  static void TestQueryAdd(const CWFrustum &frustum, NTempest::CImVector color, const NTempest::C44Matrix *basis);
  static void TestQueryAdd(const NTempest::CAaBox &aaBox, NTempest::CImVector color, const NTempest::C44Matrix *basis);
  static void TestQueryRender();
  static void RenderLow();
  static void RenderAreaLow(CMapAreaLow *areaLow);
  static unsigned long GetTextureUseage();
  static SFile                             *wdtFile;
  static unsigned long                      version;
  static SMMapHeader                        header;
  static int                                bDungeon;
  static SMAreaInfo                         areaInfo[4096];
  static CMapArea                          *areaTable[4096];
  static unsigned long                      areaLowOffsets[4096];
  static CMapAreaLow                       *areaLowTable[4096];
  static LISTDECLEX(CMapBaseObjLink, refLink, areaLinkList);
  static TSGrowableArray<char>              doodadNames;
  static TSGrowableArray<unsigned int>      doodadNamesIndex;
  static TSGrowableArray<char>              mapObjNames;
  static TSGrowableArray<unsigned int>      mapObjNamesIndex;
  enum {
    Cnt_Area = 0,
    Cnt_DoodadDef = 1,
    Cnt_Chunk = 2,
    Cnt_ChunkLayer = 3,
    Cnt_ChunkTex = 4,
    Cnt_MapObjDef = 5,
    Cnt_MapObjDefGroup = 6,
    Cnt_Entity = 7,
    Cnt_Light = 8,
    Cnt_BaseObjLink = 9,
    Cnt_CacheLight = 10,
    Cnt_Num = 11
  };
  enum {
    NUM_LIQUID_TEX_FRAMES = 30,
    NUM_RIPPLES = 48
  };
  enum {
    OCEAN_DIFF_TEX = 0,
    RIVER_DIFF_TEX = 1
  };
  static const unsigned int                             SKYTEX_HEIGHT;
  static const unsigned int                             WATERTEX_HEIGHT;
  static const float                                    LIQUID_TEX_PURGE_TIME;
  static const float                                    WATER_SPEC_EXP;
  static CGxTex                                        *skyTexid;
  static CGxTex                                        *riverDiffTexid;
  static CGxTex                                        *oceanDiffTexid;
  static TSFixedArray<NTempest::CImVector>              skyTexels;
  static HTEXTURE__                                    *liquidTex[LIQUID_COUNT][LIQUID_TEXTURE_COUNT];
  static bool                                           liquidTexLoaded[LIQUID_COUNT];
  static float                                          liquidLastShown[LIQUID_COUNT];
  static const float                                    liquidTexLoopTime[LIQUID_COUNT];
  static const char                                    *liquidTexBaseName[LIQUID_COUNT];
  static bool                                           riverDiffTexUpdated;
  static bool                                           oceanDiffTexUpdated;
  static LISTDECL(WaterRadWave, waterRipplesFree);
  static LISTDECL(WaterRadWave, waterRipplesActive);
  static CGxPixelShader                                *psOcean0;
  static CGxPixelShader                                *psSpecTerrain;
  static CGxShaderParam                                *psSpecTerrain_LayerMask;
  static CGxPixelShader                                *psTerrain;
  static CGxShaderParam                                *psTerrain_LayerMask;
  static CGxPixelShader                                *psSpecUTerrain;
  static CGxShaderParam                                *psSpecUTerrain_LayerMask;
  static CGxPixelShader                                *psUTerrain;
  static CGxShaderParam                                *psUTerrain_LayerMask;
  static CGxBuf                                        *gxBufDynLowDetail;
  static int                                            bActive;
  static int                                            bPreload;
  static void                                          *oldSelectLightParm;
  static TSGrowableArray<unsigned int>                  scCollideList;
  static unsigned int                                   scCollideCnt;
  static unsigned int                                   mapGetFacetsCount;
  static int(*entityHandler)(void *handlerParam, unsigned long event, unsigned __int64 guid, unsigned long flags);
  static void *entityHandlerParam;
  static int(*entityCollisionHandler)(unsigned __int64 guid, unsigned long flags, WorldObjCollisionHandlerData *data);
  static TSGrowableArray<CGxVertexPC>                 testQueryVerts;
  static TSGrowableArray<unsigned short>              testQueryIndices;
  static unsigned int                                 cCount;
  static unsigned int                                 bspRecurseCount;
  static unsigned int                                 nChunksPrepared;
  static unsigned int                                 nGbChunksPrepared;
  static CMapLight                                   *sunLight;
  static char                                         wdtFilename[256];
  static char                                         wobFilename[256];
  static char                                         mapPath[256];
  static char                                         mapName[256];
  static LISTDECLEX(CMapBaseObjLink, refLink, doodadDefLinkList);
  static LISTDECLEX(CMapBaseObjLink, refLink, mapObjDefLinkList);
  static HASHKEY_NONE                                 nullHashKey;
  static LISTDECLEX(CMapLight, lameAssLink, lightList);
  static LISTDECLEX(CMapLight, lameAssLink, lightFreeList);
  static LISTDECLEX(CMapCacheLight, lameAssLink, cacheLightFreeList);
  static LISTDECL(CChunkLayer, chunkLayerFreeList);
  static LISTDECL(CChunkTex, chunkTexFreeList);

 private:
  friend class CWorld;
  friend class CMapChunk;
  static unsigned int GetUniqueId();
  static void CreateChunk(CMapArea *area, CMapChunk *chunk, unsigned long flags);
  static void LoadDoodadNames();
  static void LoadMapObjNames();
  static void CreateMapObjDefGroups(CMapObj *mapObj, CMapObjDef *mapObjDef);
  static void UpdateMapObjDefs();
  static void UpdateMapObjDefGroups(CMapObjDef *mapObjDef, CMapObj *mapObj);
  static void UpdateChunks(CMapArea *area);
  static void PrepareAreas();
  static void PrepareMapObjDefs();
  static void PrepareDoodadDefs();
  static void QueryLightmap(CMapDoodadDef *doodadDef);
  static void PrepareMapObjDef(CMapObjDef *mapObjDef, CMapObj *mapObj);
  static void PrepareChunks();
  static void PrepareArea(int x, int y);
  static void PrepareChunk(CMapArea *area, int x, int y);
  static bool            enablePixelShaders;
  static bool            enableSpecular;
  static bool            enableSpecularTerrain;
  static bool            enableTerrainShader;
  static bool            enableSpecularWater;
  static void Purge();
  static void VectorIntersectSX(NTempest::CiRect &sRect);
  static void VectorIntersectSY(NTempest::CiRect &sRect);
  static void VectorIntersectDX(const NTempest::C3Vector &p0, const NTempest::C3Vector &p1, NTempest::CiRect &sRect);
  static void VectorIntersectDY(const NTempest::C3Vector &p0, const NTempest::C3Vector &p1, NTempest::CiRect &sRect);
  static void LodCreateTree(int x0, int y0, int x1, int y1, int level, int parent);
  static float PointIntersectSubChunk(float x, float y, int ix, int iy, const CMapChunk *chunk);
  static bool VectorIntersectSubchunk(
      const NTempest::C3Vector *p0,
      const NTempest::C3Vector *p1,
      const NTempest::C3Vector *normal,
      float                    *t
  );
  static bool VectorIntersectSubchunk(
      const NTempest::C3Vector *p0,
      const NTempest::C3Vector *p1,
      float                    *t,
      unsigned int              queryFlags
  );
  static bool VectorIntersectTri(
      const NTempest::C3Vector *p,
      const NTempest::C3Vector *v0,
      const NTempest::C3Vector *v1,
      const NTempest::C3Vector *v2,
      const NTempest::C3Vector *n
  );
  static bool VectorIntersectSubchunks(
      const NTempest::C3Vector *p0,
      const NTempest::C3Vector *p1,
      float                    *t,
      unsigned int              queryFlags,
      CMapChunk               **retChunk
  );
  static bool GetFacetTerrain(const NTempest::C3Segment &seg, float &t, NTempest::C4Plane &facet, unsigned int queryFlags);
  static bool GetFacetSubchunks(const NTempest::C3Segment &seg, float &t, NTempest::C4Plane &facet, unsigned int queryFlags);
  static bool GetFacetMapObjs(const NTempest::C3Segment &seg, float &t, NTempest::C4Plane &facet, unsigned int queryFlags);
  static bool
  GetChunkFacets(int cx, int cy, NTempest::CiRect &sRect, const NTempest::CAaBox &aaBox, CWFacetData *facetData, unsigned int queryFlags);
  static bool GetChunkFacets(int cx, int cy, const NTempest::CiRect &sRect, const CWFrustum &wFrustum, CWFacetData *facetData);
  static bool GetFacetsMapObjs(const NTempest::CAaBox &aaBox, CWFacetData *facetData, unsigned int queryFlags);
  static bool GetFacetsMapObjs(const CWFrustum &frustum, CWFacetData *facetData, unsigned int queryFlags);
  static bool GetTrisTerrain(const NTempest::CAaBox &aaBox, CWTriData &triData, unsigned int queryFlags);
  static bool GetTrisMapObjs(const NTempest::CAaBox &aaBox, CWTriData &triData, unsigned int queryFlags);
  static bool
  GetTrisChunk(int cx, int cy, NTempest::CiRect &sRect, const NTempest::CAaBox &aaBox, CWTriData &triData, unsigned int queryFlags);
  static void CreateImpassableFacets(CMapChunk *chunk, const NTempest::CAaBox &aaBox, CWFacetData *facetData, unsigned int queryFlags);
  static void LinkEntityToMapObj(CMapStaticEntity *entity, CMapObjDef *mapObjDef, CMapObjDefGroup *mapObjDefGroup);
  static void LinkEntityToChunk(CMapStaticEntity *entity, CMapChunk *chunk);
  static bool LinkIntersectMapObjs(
      const NTempest::C3Vector &lCen,
      const NTempest::C3Vector &lEnd,
      float                    &hitT,
      CMapObjDef              *&hitMapObjDef,
      CMapObjDefGroup         *&hitMapObjDefGroup
  );
  static void LinkLightToMapObjDefs(CMapLight *light);
  static void LinkLightToChunks(CMapLight *light);

  static int                                       counts[Cnt_Num];
  static int                                       freeCounts[Cnt_Num];
  static LISTDECLEX(CMapBaseObjLink, ownerLink, baseObjLinkFreeList);
  static TSExplicitList<CMapObjGroup, 0x1AC>       mapObjGroupFreeList;
  static TSExplicitList<CMapObj, 0x1A4>            mapObjFreeList;
  static LISTDECLEX(CMapDoodadDef, lameAssLink, doodadDefFreeList);
  static LISTDECLEX(CMapEntity, lameAssLink, entityFreeList);
  static LISTDECLEX(CMapEntity, lameAssLink, entityList);
  static LISTDECLEX(CMapArea, lameAssLink, areaFreeList);
  static LISTDECLEX(CMapArea, lameAssLink, areaList);
  static LISTDECLEX(CMapChunk, lameAssLink, chunkFreeList);
  static LISTDECLEX(CMapChunk, lameAssLink, chunkList);
  static LISTDECLEX(CChunkLiquid, lameAssLink, chunkLiquidList);
  static LISTDECLEX(CChunkLiquid, lameAssLink, chunkLiquidFreeList);
  static LISTDECLEX(CMapSoundEmitter, lameAssLink, soundEmitterFreeList);
  static LISTDECLEX(CMapObjDefGroup, lameAssLink, mapObjDefGroupFreeList);
  static LISTDECLEX(CMapObjDefGroup, lameAssLink, mapObjDefGroupList);
  static LISTDECLEX(CMapObjDef, lameAssLink, mapObjDefFreeList);
  static TSHashTable<CMapDoodadDef, HASHKEY_DWORD> doodadDefHash;
  static TSHashTable<CMapObjDef, HASHKEY_NONE>     mapObjDefHash;
  static unsigned int                              uniqueId;
};

enum WorldCullStatus {
  WorldCull_outside = 0,
  WorldCull_inside = 1,
  WorldCull_intersect = 2,
  WorldCull_notOutside = 3,
  WorldCull_count = 4
};

class CWFrustum {
  friend class CGCamera;
  friend class CMap;
  friend class CMapObj;
  friend class CMapObjGroup;
  friend class CWorld;
  friend class CWorldScene;

 public:
  enum {
    P_TOP = 0,
    P_BOTTOM = 1,
    P_LEFT = 2,
    P_RIGHT = 3,
    P_FAR = 4,
    P_NEAR = 5,
    NUM_PLANES = 6
  };

  enum {
    NEAR_LL = 0,
    NEAR_UL = 1,
    NEAR_UR = 2,
    NEAR_LR = 3,
    FAR_LL = 4,
    FAR_UL = 5,
    FAR_UR = 6,
    FAR_LR = 7,
    NUM_CORNERS = 8
  };

 protected:
  NTempest::C4Plane  planes[6];
  NTempest::C3Vector corners[8];

 public:
  NTempest::C3Vector lookPos;
  NTempest::C3Vector lookAt;
  NTempest::C3Vector lookUp;
  float              fovy;
  float              aspect;
  float              minz;
  float              maxz;
  LINKDECLEX(CWFrustum, sceneLink);

  CWFrustum() {
  }
  CWFrustum(const NTempest::C3Vector *c);
  CWFrustum(
      const NTempest::C3Vector &lPos,
      const NTempest::C3Vector &lAt,
      const NTempest::C3Vector &lUp,
      float                     p_fovy,
      float                     p_aspect,
      float                     p_minz,
      float                     p_maxz
  );
  CWFrustum &operator=(const CWFrustum &frustum) {
    if (this != &frustum) {
      unsigned int i;
      for (i = 0; i < NUM_PLANES; ++i) {
        planes[i] = frustum.planes[i];
      }
      for (i = 0; i < 8; ++i) {
        corners[i] = frustum.corners[i];
      }
      lookPos = frustum.lookPos;
      lookAt = frustum.lookAt;
      lookUp = frustum.lookUp;
      fovy = frustum.fovy;
      aspect = frustum.aspect;
      minz = frustum.minz;
      maxz = frustum.maxz;
    }
    return *this;
  }
  const NTempest::C3Vector *Corners() const {
    return corners;
  }
  const NTempest::C3Vector &Corner(unsigned int index) const {
    FATALASSERT(index < 8);
    return corners[index];
  }
  const NTempest::C4Plane &Plane(unsigned int index) const {
    FATALASSERT(index < 6);
    return planes[index];
  }
  void                CalcPlanesFromCorners();
  void                CalcPlanesFromCorners(const NTempest::C3Vector *c);
  void                Translate(const NTempest::C3Vector &t);
  void                Transform(const NTempest::C44Matrix &mat);
  WorldCullStatus     Cull(const NTempest::CAaBox &box) const;
  WorldCullStatus     Cull(const NTempest::CAaBox &box, NTempest::C33Matrix &basis, NTempest::C3Vector &pos);
  WorldCullStatus     Cull(const NTempest::CAaSphere &sphere) const;
  WorldCullStatus     Cull(const NTempest::C3Vector &center, float radius) const;
  WorldCullStatus     Cull(const NTempest::C3Vector &point) const;
  void                Cull(const NTempest::C3Vector &point, unsigned int &cullFlags) const;
  WorldCullStatus     Cull(const NTempest::C4Plane &plane) const;
  void                Render() const;
};

class CSortEntry {
 public:
  LISTDECLEX(CWFrustum, sceneLink, frustumList);
  LISTDECLEX(CMapChunk, sceneLink, chunkList);
  LISTDECLEX(CMapDoodadDef, sceneLink, doodadDefList);
  LISTDECLEX(CMapObjDef, sceneLink, mapObjDefList);
  LISTDECLEX(CChunkLiquid, sceneLink, liquidList[4]);
  LISTDECLEX(CMapEntity, sceneLink, entityList);
};

class CSortTable {
 public:
  void Initialize();
  void Destroy();
  void Clear();

  CSortEntry                           table[26];
  LISTDECLEX(CWFrustum, sceneLink, frustumList);
  LISTDECLEX(CMapAreaLow, sceneLink, visAreaLowList);
  LISTDECLEX(CMapChunk, sceneLink, visChunkList);
  LISTDECLEX(CMapDoodadDef, sceneLink, visDoodadList);
  LISTDECLEX(CMapObjDefGroup, sceneLink, visMapObjDefGroupList);
  LISTDECLEX(CMapEntity, sceneLink, visEntityList);
  LISTDECLEX(CChunkLiquid, sceneLink, visLiquidList[4]);
  LISTDECLEX(CMapEntity, sceneLink, nonVisEntityList);
  LISTDECLEX(CMapChunk, sceneLink, updateChunkList);
};

class CWorldScene {
  friend class CMapChunk;
  friend class CMapObj;

 public:
  static void Initialize();
  static void Destroy();
  static void PrepareRender(const NTempest::C3Vector &position, const NTempest::C3Vector &target);
  static void Update();
  static void Render();
  static void RenderAlpha();
  static void CalcFrustumCorners(NTempest::C3Vector *corners);
  static void AddDoodadDef(CMapDoodadDef *doodadDef);
  static void AddMapObjDef(CMapObjDef *mapObjDef);
  static void AddMapChunk(CMapChunk *chunk, float sortDist);
  static void AddChunkLiquid(CChunkLiquid *liquid, unsigned int type);
  static void AddMapEntity(CMapEntity *entity);
  static void ClipBufferUpdate(
      const NTempest::C3Vector *vertices,
      const int                *indicies,
      const int                 nVertices,
      const NTempest::C3Vector &corner
  );
  static void ClipPortal(NTempest::C4Vector *inList, unsigned int &inCount);
  static void FrustumPush();
  static void FrustumSet(const NTempest::CRect &sRect);
  static void FrustumSet(const NTempest::C3Vector *corners);
  static void FrustumSet(const NTempest::C3Vector *corners, const NTempest::CRect &sRect);
  static void FrustumSet(const CWFrustum &frustum);
  static CWFrustum &FrustumGet();
  static void FrustumXform(const NTempest::C44Matrix &mat);
  static int FrustumCull(const NTempest::C3Vector &center, float radius);
  static int FrustumCull(const NTempest::CAaBox &aaBox);
  static int FrustumCull(const NTempest::CAaBox &aaBox, NTempest::C33Matrix &basis, NTempest::C3Vector &pos);
  static void FrustumPop();
  static CWFrustum *AllocFrustum();
  static void FreeFrustum(CWFrustum *frustum);

  static float                         cullSmallThreshold;
  static float                         cullDistance;
  static NTempest::C3Vector            camPos;
  static NTempest::C3Vector            camTarg;
  static NTempest::C3Vector            camVec;
  static NTempest::C4Plane             camPlaneXY;
  static NTempest::C4Plane             vpPlanes[4];
  static CMapEntity                   *camTargEntity;
  static unsigned int                  camLiquid;
  static CMapObjDef                   *camMapObjDef;
  static CMapObj                      *camMapObj;
  static CMapObjGroup                 *camMapObjGroup;
  static CSortTable                    sortTable;
  static CMapObjDef                   *viewerMapObjDef;
  static TSGrowableArray<unsigned int> viewerMapObjGroups;
  static unsigned int                  bspStateBits;
  static NTempest::CAaBox              camFrustumBounds;
  static NTempest::C3Vector            camFrustumCorners[8];
  static NTempest::C44Matrix           mvp;
  static NTempest::C44Matrix           mv;
  static NTempest::C44Matrix           mp;
  static NTempest::C3Vector            vpMinPos;
  static NTempest::C3Vector            vpMaxPos;
  static NTempest::C4Vector            mvpCol3;
  static NTempest::C44Matrix           gxViewMat;
  static unsigned int                  nPrimsRendered;
  static unsigned int                  nChunksRendered;
  static unsigned int                  nDoodadsRendered;
  static unsigned int                  nObjectsRendered;
  static char                          currentChunkName[64];
  static CWFrustum                     frustumStack[16];
  static int                           frustumIndex;
  static NTempest::C4Vector            clipVertexBuffer[9];
  static float                         clipBuffer[128];
  static LISTDECLEX(CWFrustum, sceneLink, frustumFreeList);

 private:
  static void PrepareRenderLiquid();
  static void LocateViewer();
  static void AddViewerGroup2(unsigned int groupNum);
  static void LocateViewer3();
  static void LocateViewer2();
  static void CullSortTable(const NTempest::CRect &sRect);
  static void CullHorizon(const NTempest::CRect &sRect);
  static void CullEntitys(CSortEntry *sortEntry);
  static void CullDoodads(CSortEntry *sortEntry);
  static void CullDoodads(LISTEX(CMapBaseObjLink, refLink) &doodadDefLinkList);
  static void CullChunkLiquid(CSortEntry *sortEntry, unsigned int type);
  static void CullChunks(CSortEntry *sortEntry);
  static void CullMapObjDefs(CSortEntry *sortEntry, const NTempest::CRect &sRect);
  static void CullMapObjDef(CMapObjDef *mapObjDef, TSGrowableArray<unsigned int> &inGroups);
  static void CullMapObjDefGroup(const unsigned int groupNum, const void *userParam, const int rDrawSharedLiquidToggle);
  static void RenderChunks();
  static void RenderObjects();
  static void RenderDoodads();
  static void RenderHorizon();
  static void RenderMapObjDefGroups();
  static void RenderOcean();
  static void RenderWater();
  static void RenderMagma();
  static void ClipBufferClear();
  static int ClipBufferCull(const NTempest::C3Vector &center, float radius, unsigned int cullFlags);
  static int ClipBufferCull(const NTempest::CAaBox &aaBox, unsigned int cullFlags);
};

#endif
