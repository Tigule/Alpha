#ifndef WOW_SOURCE_WORLDCLIENT_WORLD_H
#define WOW_SOURCE_WORLDCLIENT_WORLD_H

#include "WorldClient/Map.h"

#include <Tempest/c2vector.h>
#include <Tempest/c2ivector.h>
#include <Tempest/c3segment.h>
#include <Tempest/c3vector.h>
#include <Tempest/c4plane.h>
#include <Tempest/c4vector.h>
#include <Tempest/cimvector.h>
#include <Tempest/cirect.h>
#include <Tempest/cfacet.h>

#include <string.h>

namespace NTempest {
  class C3Vector;
  struct CFacet;
}  // namespace NTempest

class CWorldParam;
class CDetailDoodadInst;
class DNSky;
class CWFrustum;
class CGUnit_C;
class CGxPixelShader;
class CGxShaderParam;
class CGxTex;
class WMOAreaTableRec;
struct HMODEL__;
struct HTEXTURE__;
struct CMapEntity;
struct SMODoodadDef;

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
  void Show(unsigned int show) {
    this->show = show;
  }
  void                   Update();
  void                   Render();
  static void __fastcall CustomRenderCallback(void *p1, int p2);

 private:
  void               InitMovement();
  NTempest::C3Vector ComputeMovement(float elapsedTime);

  static NTempest::C3Vector s_vcv[4];
  static NTempest::C2Vector s_tc[13][4];
  static unsigned int       s_tcSub[4][8];
  static const float        PTSIZE;
  Particle                  particles[4000];
  unsigned int              numParticles;
  NTempest::C3Vector        lastCamPos;
  HTEXTURE__               *texture;
  unsigned int              show;
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

void __fastcall ProjectTex2d(NTempest::CAaBox &box, NTempest::CImVector color, NTempest::C44Matrix *basis, float fadeOffset);

class CWorld {
 public:
  typedef CWorldMinimapQuad MinimapQuad;

  static HMODEL__ *__fastcall GetModel(unsigned long doodad);
  static void __fastcall SetObjectRenderCallback(unsigned long hWorldObject, void(__fastcall *cb)(void *, const NTempest::C44Matrix &), void *param);
  static void __fastcall SetObjectHandler(int(__fastcall *handler)(void *, unsigned long, unsigned __int64, unsigned long), void *handlerParam);
  static void __fastcall SetObjectCollisionHandler(int(__fastcall *handler)(unsigned __int64, unsigned long, WorldObjCollisionHandlerData *));
  static void __fastcall SetCameraTarget(unsigned long hWorldObject);
  static unsigned long __fastcall AddObject(unsigned __int64 param64, unsigned long param32, HMODEL__ *hModel, unsigned int objFlags);
  static void __fastcall          RemoveObject(unsigned long hWorldObject);
  static unsigned long __fastcall AddDoodad(const char *fileName, HMODEL__ *hModel, const NTempest::C44Matrix &mat, unsigned int objFlags);
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

  static void __fastcall         Initialize();
  static void __fastcall         Destroy();
  static void __fastcall         LoadMap(const char *mapName, NTempest::C3Vector &position, int preLoad);
  static void __fastcall         UnloadMap();
  static void __fastcall         PrepareUpdate(NTempest::C3Vector &position, NTempest::C3Vector &target);
  static void __fastcall         SetUpdateTime(float elapsedSec, unsigned long pCurTimeMs);
  static void __fastcall         Update();
  static void __fastcall         ObjectGetExtents(unsigned int id, NTempest::CAaBox &extents);
  static void __fastcall         ObjectDelete(unsigned int id);
  static void __fastcall         SetHidden(unsigned long hWorldObject, int hidden);
  static unsigned int __fastcall QueryAreaId(float x, float y);
  static unsigned int __fastcall SceneCamLiquidStatus();
  static int __fastcall          QueryObjectInside(unsigned long hWorldObject);
  static int __fastcall          QueryMapObjZoneName(unsigned long hWorldObject, const char *&zoneName);
  static int __fastcall          QueryMapObjSubzoneName(unsigned long hWorldObject, const char *&subzoneName, unsigned int &subzoneId);
  static int __fastcall          QueryMapObjFileName(unsigned long hWorldObject, const char *&fileName);
  static int __fastcall          QueryMapObjFog(unsigned long hWorldObject, SMOFog::Fogs &oFogs, float &oPct);
  static unsigned int __fastcall QueryMapObjMinimap(unsigned long hWorldObject, NTempest::CAaBox &aaBox, TSStackArray<MinimapQuad> &quads);
  static unsigned int __fastcall QueryMapObjIDs(unsigned long hWorldObject, unsigned int &wmoID, unsigned int &instanceID, unsigned int &groupID);
  static unsigned int __fastcall QueryMapObjMatrix(unsigned long hWorldObject, NTempest::C44Matrix *mtx, NTempest::C44Matrix *invMtx);
  static const char *__fastcall  QueryChunkName();
  static bool __fastcall  QueryMapObjAreaTable(unsigned long hWorldObject, const WMOAreaTableRec *&subzoneRec, const WMOAreaTableRec *&globalRec);
  static int __fastcall   QueryObjectLiquid(unsigned long hWorldObject, unsigned int &liquid, float &surface, NTempest::C3Vector &flowDir, int &deep);
  static int __fastcall   QueryLiquidStatus(NTempest::C3Vector &point, unsigned int &liquid, float &surface, NTempest::C3Vector &waterDir);
  static void __fastcall  UpdateObject(unsigned long hWorldObject, NTempest::C44Matrix &mat, NTempest::CAaBox &aaBox);
  static void __fastcall  ObjectUpdate(unsigned int id, NTempest::C3Vector &pos, float angle, int bSnap);
  static void __fastcall  TickObject(unsigned long hWorldObject);
  static void __fastcall  WaterRipple(NTempest::C3Vector &pos, float len, float time, float amp, float vel, float freq);
  static float __fastcall GetCurTimeSec() {
    return curTimeSec;
  }
  static float __fastcall GetTickTimeSec() {
    return tickTimeSec;
  }
  static unsigned int __fastcall GetCurTimeMs() {
    return curTimeMs;
  }
  static unsigned int __fastcall GetTickTimeMs() {
    return tickTimeMs;
  }
  static const NTempest::C3Vector &__fastcall GetCamPos();
  static const NTempest::C3Vector &__fastcall GetCamTarget();
  static float __fastcall                     GetFramerate();
  static void __fastcall                      GetCounts(int *const counts);
  static void __fastcall                      SetEnvironment();
  static void __fastcall                      Render();
  static void __fastcall                      RenderAlpha();
  static void __fastcall
  SelectLight(void *parm, NTempest::C3Vector worldPos, const NTempest::C3Vector &cameraWorldPos, unsigned int maxLightsToUse);
  static void __fastcall                      SetShadowColor(NTempest::CImVector &color);
  static void __fastcall                      SetDetailDoodadDensity(unsigned int density);
  static void __fastcall                      SetNearClip(float nearClip);
  static void __fastcall                      SetFarClip(float farClip);
  static void __fastcall                      SetTexLodBias(float bias);
  static void __fastcall                      SetTexAnisotropy(unsigned int anisotropy);
  static bool __fastcall                      SetLodDist(float dist);
  static bool __fastcall                      SetTextureLodDist(float dist);
  static float __fastcall                     CalcAltitude(float x, float y, float radius);
  static bool __fastcall                      GetFacet(const NTempest::C3Segment &seg, float &t, NTempest::C4Plane &facet, unsigned int queryFlags);
  static bool __fastcall
  Intersect(const NTempest::C3Vector *a, const NTempest::C3Vector *b, float radius, NTempest::C3Vector *ip, float *dist, unsigned int queryFlags);
  static void __fastcall         GetFacets(NTempest::CAaBox &aaBox, CWFacetData *facetData, unsigned int queryFlags);
  static void __fastcall         GetFacets(CWFrustum &frustum, CWFacetData *facetData, unsigned int queryFlags);
  static unsigned int __fastcall GetTris(NTempest::CAaBox &aaBox, CWTriData &triData, unsigned int queryFlags);
  static int __fastcall NDCClip(NTempest::C3Vector *p_inVerts, unsigned int p_inCount, NTempest::C3Vector **&p_outVerts, unsigned int &p_outCount);
  static unsigned int __fastcall NDCXform(const CWFrustum &frustum, NTempest::C44Matrix &xf, bool translate);

 private:
  friend class CDetailDoodadInst;
  friend class DNSky;
  friend class CWorldParam;
  friend class CWorldScene;
  friend class CMapChunk;
  friend class CMapObj;
  friend class CGUnit_C;
  friend class CMap;
  friend class CMapArea;
  friend class CMapObjGroup;
  friend void __fastcall        ShadowRender_LOD1(HMODEL__ *hModel, NTempest::C44Matrix &basis, void *param);
  friend HTEXTURE__ *__fastcall CharCustomizationLoadSkin(
      HMODEL__    *characterModel,
      const char  *skinName,
      unsigned int raceID,
      unsigned int sexID,
      unsigned int textureNumber,
      int          isNPC
  );
  friend HTEXTURE__ *__fastcall
  CharCustomizationSetSkin(HMODEL__ *characterModel, unsigned int raceID, unsigned int sexID, unsigned int textureNumber, int isNPC);

  static void __fastcall ModelGeoProjectCallback(NTempest::CAaBox &worldBox, NTempest::CImVector color, NTempest::C44Matrix &basis);
  static int __fastcall  ParticleProjectCallback(const NTempest::C3Segment &seg, float &z);
  static int __fastcall  AnimBoneProjectCallback(const NTempest::C3Segment &seg, float &z);
  static void __fastcall CalcFPS();
  static void __fastcall UpdateDayNight(int forceFull, NTempest::C3Vector *position);
  static void __fastcall PrepareAreaOfInterest(NTempest::C3Vector &position, NTempest::C3Vector &target);

  static int __fastcall ConsoleCommand_ShowDetailDoodads(const char *command, const char *arguments);
  static int __fastcall ConsoleCommand_MaxLOD(const char *__formal, const char *arguments);
  static int __fastcall ConsoleCommand_ShowCull(const char *command, const char *arguments);
  static int __fastcall ConsoleCommand_SetShadow(const char *__formal, const char *arguments);
  static int __fastcall ConsoleCommand_MapObjLightMode(const char *command, const char *arguments);
  static int __fastcall ConsoleCommand_WaterShow(const char *command, const char *arguments);
  static int __fastcall ConsoleCommand_WaterMaxLOD(const char *__formal, const char *arguments);
  static int __fastcall ConsoleCommand_WaterWaves(const char *__formal, const char *arguments);
  static int __fastcall ConsoleCommand_WaterSpecular(const char *__formal, const char *arguments);
  static int __fastcall ConsoleCommand_WaterRipples(const char *__formal, const char *arguments);
  static int __fastcall ConsoleCommand_WaterParticulates(const char *command, const char *arguments);
  static int __fastcall ConsoleCommand_DetailDoodadAlpha(const char *__formal, const char *arguments);
  static int __fastcall ConsoleCommand_ShowShadow(const char *command, const char *arguments);
  static int __fastcall ConsoleCommand_ShowLowDetail(const char *command, const char *arguments);
  static int __fastcall ConsoleCommand_ShowSimpleDoodads(const char *command, const char *arguments);
  static int __fastcall ConsoleCommand_EnumTextures(const char *__formal, const char *name);
  static int __fastcall ConsoleCommand_EnumTextureGxCache(const char *__formal, const char *name);

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

struct WaterRadWave : public TSLinkedNode<WaterRadWave> {
  int                Update(float deltat);
  void               Init(NTempest::C3Vector &p_pos, float len, float time, float amp, float vel, float freq);
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

  static void __fastcall Initialize();
  static void __fastcall AsyncPollHandler();
  static void __fastcall Destroy();
  void                   Load(SMAreaInfo *areaInfo);
  void                   Purge();
  void                   PurgeChunks();
  void                   PrepareLocalRect();
  void                   InitWater();

  static int ccWaterLOD;
  static int ccWaterMaxLOD;
  static int ccWaterWaves;
  static int ccWaterSpecular;
  static int ccWaterRipples;

  TSExplicitList<CMapBaseObjLink, 8> chunkLinkList;
  unsigned int                       infoIndex;
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
  static void __fastcall          FreeAsyncLoadBuffer(unsigned int *buffer);
  static void __fastcall          InitAsyncLoadBuffers();
  static unsigned int *__fastcall AllocAsyncLoadBuffer();
  void                            Create(unsigned int *data);
  void                            LoadTextures(char *texNames, unsigned long size);
  static void __fastcall          AsyncCallback(void *userArg);
};

class CMap {
 public:
  static void __fastcall           Initialize();
  static void __fastcall           PrepareUpdate();
  static void __fastcall           Update();
  static void __fastcall           Unload();
  static void __fastcall           Load(const char *fileName);
  static void __fastcall           LoadWdl();
  static void __fastcall           LoadWdt();
  static void __fastcall           Preload();
  static void __fastcall           Open();
  static void __fastcall           GetCounts(int *const counts);
  static void __fastcall           LoadDoodadNames();
  static void __fastcall           LoadMapObjNames();
  static CMapDoodadDef *__fastcall CreateDoodadDef(SMDoodadDef &smDoodadDef, NTempest::C3Vector &pos);
  static CMapDoodadDef *__fastcall CreateDoodadDef(
      unsigned int         doodadRef,
      SMODoodadDef        &smoDoodadDef,
      const char          *fileName,
      unsigned int         mapObjDefId,
      NTempest::C44Matrix &mapObjDefMat
  );
  static CMapObjDef *__fastcall CreateMapObjDef(SMMapObjDef &smMapObjDef, NTempest::C3Vector &pos);
  static void __fastcall        CreateMapObjDefGroups(CMapObj *mapObj, CMapObjDef *mapObjDef);
  static void __fastcall
  CreateMapObjDefGroupDoodads(CMapObj *mapObj, CMapObjGroup *mapObjGroup, CMapObjDef *mapObjDef, CMapObjDefGroup *mapObjDefGroup);
  static void __fastcall CreateMapObjDefLights(CMapObj *mapObj, CMapObjGroup *mapObjGroup, CMapObjDef *mapObjDef, CMapObjDefGroup *mapObjDefGroup);
  static HTEXTURE__ *__fastcall  LoadTexture(const char *fileName);
  static void __fastcall         WaterRipple(NTempest::C3Vector &pos, float len, float time, float amp, float vel, float freq);
  static void __fastcall         UpdateEntity(CMapEntity *entity);
  static void __fastcall         LinkEntityToMapObj(CMapStaticEntity *entity, CMapObjDef *mapObjDef, CMapObjDefGroup *mapObjDefGroup);
  static void __fastcall         LinkEntityToChunk(CMapStaticEntity *entity, CMapChunk *chunk);
  static void __fastcall         LinkEntity(CMapStaticEntity *entity);
  static unsigned int __fastcall LinkIntersectMapObjs(
      NTempest::C3Vector &lCen,
      NTempest::C3Vector &lEnd,
      float              &hitT,
      CMapObjDef        *&hitMapObjDef,
      CMapObjDefGroup   *&hitMapObjDefGroup
  );
  static unsigned int __fastcall QueryShadow(NTempest::C3Vector &pos);
  static unsigned int __fastcall
  QueryLiquidStatus(NTempest::C3Vector &point, unsigned int &liquid, float &surface, NTempest::C3Vector &waterDir, int &deep);
  static unsigned int __fastcall
  QueryLiquidStatusMapObjsExt(NTempest::C3Vector &point, unsigned int &liquid, float &surface, NTempest::C3Vector &waterDir);
  static unsigned int __fastcall
                         VectorIntersectTerrain(NTempest::C3Vector *p0, NTempest::C3Vector *p1, float *t, unsigned int queryFlags, CMapChunk **chunk);
  static void __fastcall SetLightFuncs();
  static void __fastcall GxuLightInitialize();
  static void __fastcall GxuLightShutdown();
  static unsigned long __fastcall GxuLightCreate();
  static void __fastcall          GxuLightDestroy(unsigned long lightId);
  static CGxLight *__fastcall     GxuLightLock(unsigned long lightId);
  static void __fastcall          GxuLightUnlock(unsigned long lightId);
  static void __fastcall          GxuLightSelect(NTempest::C3Vector worldPos, const NTempest::C3Vector &cameraWorldPos, unsigned int maxLightsToUse);
  static void __fastcall          SelectLight(CMapBaseObj *baseObj);
  static void __fastcall  SelectLight(void *parm, NTempest::C3Vector worldPos, const NTempest::C3Vector &cameraWorldPos, unsigned int maxLightsToUse);
  static int __fastcall   GxuLightEnable(unsigned long lightId);
  static void __fastcall  GxuLightEnableSet(unsigned long lightId, int enable);
  static void __fastcall  GxuLightSetMaxLights(unsigned int maxLightsToUse);
  static float __fastcall GxuLightBucketSize();
  static void __fastcall  GxuLightBucketSizeSet(float bucketSize);
  static void __fastcall  GxuLightResetCache();
  static CMapLight *__fastcall CreateLight(bool dynamic);
  static void __fastcall       DestroyLight(CMapLight *light);
  static void __fastcall       UpdateLight(CMapLight *light);
  static void __fastcall       UpdateLightBounds(CMapLight *light);
  static void __fastcall       EnableLight(CMapLight *light);
  static void __fastcall       DisableLight(CMapLight *light);
  static unsigned int          EnablePixelShaders() {
    return enablePixelShaders;
  }
  static unsigned int EnableSpecular() {
    return enableSpecular;
  }
  static unsigned int EnableSpecularTerrain() {
    return enableSpecularTerrain;
  }
  static unsigned int EnableSpecularWater() {
    return enableSpecularWater;
  }
  static unsigned int EnableTerrainShader() {
    return enableTerrainShader;
  }
  static void __fastcall              Destroy();
  static void __fastcall              ClearDetailDoodads();
  static float __fastcall             PointIntersect(float wx, float wy, float radius);
  static void __fastcall              GxBufDynLowDetailCallback(CGxBufCommand &cmd, CGxBuf *buf);
  static void __fastcall              CreateAreaLowDetailVertices(CMapAreaLow *areaLow, const CGxBufCommand &cmd, CGxBuf *buf);
  static void __fastcall              CreateAreaLowDetailIndices(CMapAreaLow *areaLow, const CGxBufCommand &cmd, CGxBuf *buf);
  static CMapBaseObjLink *__fastcall  AllocBaseObjLink(CMapBaseObj *owner);
  static CMapObj *__fastcall          AllocMapObj();
  static CMapObjGroup *__fastcall     AllocMapObjGroup();
  static CChunkLayer *__fastcall      GetLayer();
  static CChunkTex *__fastcall        GetTex();
  static CMapArea *__fastcall         AllocArea();
  static CMapChunk *__fastcall        AllocChunk();
  static CMapDoodadDef *__fastcall    AllocDoodadDef();
  static CMapEntity *__fastcall       AllocEntity();
  static CMapObjDefGroup *__fastcall  AllocMapObjDefGroup();
  static CMapObjDef *__fastcall       AllocMapObjDef();
  static CChunkLiquid *__fastcall     AllocChunkLiquid();
  static CMapSoundEmitter *__fastcall AllocSoundEmitter();
  static CMapLight *__fastcall        AllocLight();
  static CMapCacheLight *__fastcall   AllocCacheLight();
  static void __fastcall              FreeBaseObjLink(CMapBaseObjLink *link);
  static void __fastcall              FreeLight(CMapLight *light);
  static void __fastcall              FreeCacheLight(CMapCacheLight *cacheLight);
  static void __fastcall              FreeMapObj(CMapObj *mapObj);
  static void __fastcall              FreeMapObjGroup(CMapObjGroup *group);
  static void __fastcall              FreeArea(CMapArea *area);
  static void __fastcall              FreeChunk(CMapChunk *chunk);
  static void __fastcall              FreeLayer(CChunkLayer *layer);
  static void __fastcall              FreeTex(CChunkTex *tex);
  static void __fastcall              FreeDoodadDef(CMapDoodadDef *doodadDef);
  static void __fastcall              FreeEntity(CMapEntity *entity);
  static void __fastcall              InitializeDoodadBounds(CMapDoodadDef *doodadDef);
  static int __fastcall               LoadDoodadModel(CMapDoodadDef *doodadDef, int bWait);
  static void __fastcall              PurgeDoodadDef(CMapDoodadDef *doodadDef);
  static void __fastcall              FreeChunkLiquid(CChunkLiquid *&cl);
  static void __fastcall              FreeSoundEmitter(CMapSoundEmitter *soundEmitter);
  static void __fastcall              FreeMapObjDefGroup(CMapObjDefGroup *mapObjDefGroup);
  static void __fastcall              FreeMapObjDef(CMapObjDef *mapObjDef);
  static void __fastcall              PurgeMapObjDef(CMapObjDef *mapObjDef);
  static void __fastcall              PurgeMapObjDefGroup(CMapObjDefGroup *mapObjDefGroup);
  static void __fastcall              PurgeArea(CMapArea *area);
  static void __fastcall              PurgeChunk(CMapChunk *chunk);
  static void __fastcall              ReloadDoodadModels();
  static void __fastcall              UnloadLiquidTexture(unsigned int liquid);
  static HTEXTURE__ *__fastcall       GetLiquidTexture(unsigned int liquid);
  static void __fastcall              ProjectLights();
  static void __fastcall              WaterInitialize();
  static void __fastcall              WaterDiffTexCallback(
      EGxTexCommand cmd,
      unsigned int  w,
      unsigned int  h,
      unsigned int  d,
      unsigned int  mipLevel,
      void         *userArg,
      unsigned int &texelStrideInBytes,
      const void  *&texels
  );
  static void __fastcall                    WaterDestroy();
  static void __fastcall                    EnableDoodadFullAlpha(int enable);
  static unsigned int __fastcall            QueryAreaId(float x, float y);
  static bool __fastcall                    GetFacet(const NTempest::C3Segment &seg, float &t, NTempest::C4Plane &facet, unsigned int queryFlags);
  static unsigned int __fastcall            GetFacets(NTempest::CAaBox &aaBox, CWFacetData *facetData, unsigned int queryFlags);
  static unsigned int __fastcall            GetFacets(CWFrustum &frustum, CWFacetData *facetData, unsigned int queryFlags);
  static unsigned int __fastcall            GetTris(NTempest::CAaBox &aaBox, CWTriData &triData, unsigned int queryFlags);
  static void __fastcall                    TestQueryAdd(const NTempest::CFacet &facet, NTempest::CImVector color, const NTempest::C44Matrix *basis);
  static void __fastcall                    TestQueryRender();
  static void __fastcall                    RenderLow();
  static void __fastcall                    RenderAreaLow(CMapAreaLow *areaLow);
  static unsigned long __fastcall           GetTextureUseage();
  static SFile                             *wdtFile;
  static unsigned long                      version;
  static SMMapHeader                        header;
  static int                                bDungeon;
  static SMAreaInfo                         areaInfo[4096];
  static CMapArea                          *areaTable[4096];
  static unsigned long                      areaLowOffsets[4096];
  static CMapAreaLow                       *areaLowTable[4096];
  static TSExplicitList<CMapBaseObjLink, 8> areaLinkList;
  static TSGrowableArray<char>              doodadNames;
  static TSGrowableArray<unsigned int>      doodadNamesIndex;
  static TSGrowableArray<char>              mapObjNames;
  static TSGrowableArray<unsigned int>      mapObjNamesIndex;
  enum {
    LIQUID_COUNT = 9,
    NUM_LIQUID_TEX_FRAMES = 30,
    LIQUID_TEXTURE_COUNT = NUM_LIQUID_TEX_FRAMES,
    NUM_RIPPLES = 48
  };
  static const unsigned int                             SKYTEX_HEIGHT;
  static const unsigned int                             WATERTEX_HEIGHT;
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
  static TSList<WaterRadWave, TSGetLink<WaterRadWave> > waterRipplesFree;
  static TSList<WaterRadWave, TSGetLink<WaterRadWave> > waterRipplesActive;
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
  static int(__fastcall *entityHandler)(void *handlerParam, unsigned long event, unsigned __int64 guid, unsigned long flags);
  static void *entityHandlerParam;
  static int(__fastcall *entityCollisionHandler)(unsigned __int64 guid, unsigned long flags, WorldObjCollisionHandlerData *data);
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
  static TSExplicitList<CMapBaseObjLink, 8>           doodadDefLinkList;
  static TSExplicitList<CMapBaseObjLink, 8>           mapObjDefLinkList;
  static HASHKEY_NONE                                 nullHashKey;
  static TSExplicitList<CMapLight, 8>                 lightList;
  static TSExplicitList<CMapLight, 8>                 lightFreeList;
  static TSExplicitList<CMapCacheLight, 72>           cacheLightFreeList;
  static TSList<CChunkLayer, TSGetLink<CChunkLayer> > chunkLayerFreeList;
  static TSList<CChunkTex, TSGetLink<CChunkTex> >     chunkTexFreeList;

 private:
  friend class CWorld;
  friend class CMapChunk;
  static void __fastcall SnapBaseObjToSubChunk(CMapBaseObj *baseObj, NTempest::C3Vector &pos, float angle);
  static void __fastcall UpdateDoodadDef(CMapDoodadDef *doodadDef, NTempest::C3Vector &pos, float angle);
  static void __fastcall UpdateMapObjDefs();
  static void __fastcall UpdateMapObjDef(CMapObjDef *mapObjDef, NTempest::C3Vector &pos, float angle);
  static void __fastcall UpdateChunks(CMapArea *area);
  static void __fastcall UpdateLiquidTextures();
  static void __fastcall PrepareAreas();
  static void __fastcall PrepareMapObjDefs();
  static void __fastcall PrepareDoodadDefs();
  static void __fastcall QueryLightmap(CMapDoodadDef *doodadDef);
  static void __fastcall
  UpdateMapObjDefGroupDoodads(CMapObj *mapObj, CMapObjGroup *mapObjGroup, CMapObjDef *mapObjDef, CMapObjDefGroup *mapObjDefGroup);
  static void __fastcall PrepareMapObjDef(CMapObjDef *mapObjDef, CMapObj *mapObj);
  static void __fastcall PrepareChunks();
  static void __fastcall PrepareArea(int x, int y);
  static void __fastcall PrepareChunk(CMapArea *area, int x, int y);
  static void __fastcall CreateChunkNeighborPtrs(CMapChunk *chunk);
  static bool            enablePixelShaders;
  static bool            enableSpecular;
  static bool            enableSpecularTerrain;
  static bool            enableTerrainShader;
  static bool            enableSpecularWater;
  static void __fastcall Purge();
  static void __fastcall VectorIntersectSX(NTempest::CiRect &sRect);
  static void __fastcall VectorIntersectSY(NTempest::CiRect &sRect);
  static void __fastcall VectorIntersectDX(const NTempest::C3Vector &p0, const NTempest::C3Vector &p1, NTempest::CiRect &sRect);
  static void __fastcall VectorIntersectDY(const NTempest::C3Vector &p0, const NTempest::C3Vector &p1, NTempest::CiRect &sRect);
  static bool __fastcall VectorIntersectTri(
      const NTempest::C3Vector *p,
      const NTempest::C3Vector *v0,
      const NTempest::C3Vector *v1,
      const NTempest::C3Vector *v2,
      const NTempest::C3Vector *n
  );
  static bool __fastcall GetFacetTerrain(const NTempest::C3Segment &seg, float &t, NTempest::C4Plane &facet, unsigned int queryFlags);
  static bool __fastcall GetFacetSubchunks(const NTempest::C3Segment &seg, float &t, NTempest::C4Plane &facet, unsigned int queryFlags);
  static unsigned int __fastcall
  GetChunkFacets(int cx, int cy, NTempest::CiRect &sRect, NTempest::CAaBox &aaBox, CWFacetData *facetData, unsigned int queryFlags);
  static unsigned int __fastcall GetChunkFacets(int cx, int cy, NTempest::CiRect &sRect, CWFrustum &wFrustum, CWFacetData *facetData);
  static unsigned int __fastcall GetFacetsMapObjs(NTempest::CAaBox &aaBox, CWFacetData *facetData, unsigned int queryFlags);
  static unsigned int __fastcall GetFacetsMapObjs(CWFrustum &frustum, CWFacetData *facetData, unsigned int queryFlags);
  static unsigned int __fastcall GetTrisTerrain(NTempest::CAaBox &aaBox, CWTriData &triData, unsigned int queryFlags);
  static unsigned int __fastcall GetTrisMapObjs(NTempest::CAaBox &aaBox, CWTriData &triData, unsigned int queryFlags);
  static unsigned int __fastcall
                         GetTrisChunk(int cx, int cy, NTempest::CiRect &sRect, NTempest::CAaBox &aaBox, CWTriData &triData, unsigned int queryFlags);
  static void __fastcall CreateImpassableFacets(CMapChunk *chunk, NTempest::CAaBox &aaBox, CWFacetData *facetData, unsigned int queryFlags);
  static void __fastcall LinkLightToMapObjDefs(CMapLight *light);
  static void __fastcall LinkLightToChunks(CMapLight *light);

  static int                                       counts[12];
  static int                                       freeCounts[12];
  static TSExplicitList<CMapBaseObjLink, 16>       baseObjLinkFreeList;
  static TSExplicitList<CMapObjGroup, 0x1AC>       mapObjGroupFreeList;
  static TSExplicitList<CMapObj, 0x1A4>            mapObjFreeList;
  static TSExplicitList<CMapDoodadDef, 8>          doodadDefFreeList;
  static TSExplicitList<CMapEntity, 8>             entityFreeList;
  static TSExplicitList<CMapEntity, 8>             entityList;
  static TSExplicitList<CMapArea, 8>               areaFreeList;
  static TSExplicitList<CMapArea, 8>               areaList;
  static TSExplicitList<CMapChunk, 8>              chunkFreeList;
  static TSExplicitList<CMapChunk, 8>              chunkList;
  static TSExplicitList<CChunkLiquid, 816>         chunkLiquidList;
  static TSExplicitList<CChunkLiquid, 816>         chunkLiquidFreeList;
  static TSExplicitList<CMapSoundEmitter, 76>      soundEmitterFreeList;
  static TSExplicitList<CMapObjDefGroup, 8>        mapObjDefGroupFreeList;
  static TSExplicitList<CMapObjDefGroup, 8>        mapObjDefGroupList;
  static TSExplicitList<CMapObjDef, 8>             mapObjDefFreeList;
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
 public:
  NTempest::C4Plane  planes[6];
  NTempest::C3Vector corners[8];
  NTempest::C3Vector lookPos;
  NTempest::C3Vector lookAt;
  NTempest::C3Vector lookUp;
  float              fovy;
  float              aspect;
  float              minz;
  float              maxz;
  TSLink<CWFrustum>  sceneLink;

  CWFrustum();
  CWFrustum(NTempest::C3Vector *c);
  CWFrustum(NTempest::C3Vector &lPos, NTempest::C3Vector &lAt, NTempest::C3Vector &lUp, float p_fovy, float p_aspect, float p_minz, float p_maxz);
  NTempest::C3Vector *Corners();
  NTempest::C3Vector &Corner(unsigned int index);
  NTempest::C4Plane  &Plane(unsigned int index);
  void                CalcPlanesFromCorners();
  void                CalcPlanesFromCorners(NTempest::C3Vector *c);
  void                Translate(NTempest::C3Vector &t);
  void                Transform(NTempest::C44Matrix &mat);
  WorldCullStatus     Cull(NTempest::CAaBox &box);
  WorldCullStatus     Cull(NTempest::CAaBox &box, NTempest::C33Matrix &basis, NTempest::C3Vector &pos);
  WorldCullStatus     Cull(NTempest::CAaSphere &sphere);
  WorldCullStatus     Cull(NTempest::C3Vector &center, float radius);
  WorldCullStatus     Cull(NTempest::C3Vector &point);
  void                Cull(NTempest::C3Vector &point, unsigned int &cullFlags);
  WorldCullStatus     Cull(NTempest::C4Plane &plane);
};

class CSortEntry {
 public:
  TSExplicitList<CWFrustum, 244>     frustumList;
  TSExplicitList<CMapChunk, 156>     chunkList;
  TSExplicitList<CMapDoodadDef, 344> doodadDefList;
  TSExplicitList<CMapObjDef, 344>    mapObjDefList;
  TSExplicitList<CChunkLiquid, 808>  liquidList[4];
  TSExplicitList<CMapEntity, 216>    entityList;
};

class CSortTable {
 public:
  void Initialize();
  void Destroy();
  void Clear();

  CSortEntry                           table[26];
  TSExplicitList<CWFrustum, 244>       frustumList;
  TSExplicitList<CMapAreaLow, 2240>    visAreaLowList;
  TSExplicitList<CMapChunk, 156>       visChunkList;
  TSExplicitList<CMapDoodadDef, 344>   visDoodadList;
  TSExplicitList<CMapObjDefGroup, 196> visMapObjDefGroupList;
  TSExplicitList<CMapEntity, 216>      visEntityList;
  TSExplicitList<CChunkLiquid, 808>    visLiquidList[4];
  TSExplicitList<CMapEntity, 216>      nonVisEntityList;
  TSExplicitList<CMapChunk, 156>       updateChunkList;
};

class CWorldScene {
  friend class CMapChunk;
  friend class CMapObj;

 public:
  static void __fastcall Initialize();
  static void __fastcall Destroy();
  static void __fastcall PrepareRender(NTempest::C3Vector &position, NTempest::C3Vector &target);
  static void __fastcall Update();
  static void __fastcall Render();
  static void __fastcall RenderAlpha();
  static void __fastcall AddDoodadDef(CMapDoodadDef *doodadDef);
  static void __fastcall AddMapObjDef(CMapObjDef *mapObjDef);
  static void __fastcall AddMapChunk(CMapChunk *chunk, float sortDist);
  static void __fastcall AddChunkLiquid(CChunkLiquid *liquid, unsigned int type);
  static void __fastcall AddMapEntity(CMapEntity *entity);
  static void __fastcall RenderChunks();

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

 private:
  static CWFrustum *__fastcall AllocFrustum();
  static void __fastcall       FreeFrustum(CWFrustum *frustum);
  static void __fastcall       PrepareRenderLiquid();
  static void __fastcall       CalcFrustumCorners(NTempest::C3Vector *corners);
  static void __fastcall       ClipBufferUpdate(NTempest::C3Vector *vertices, const int *indicies, int nVertices, NTempest::C3Vector &corner);
  static void __fastcall       ClipPortal(NTempest::C4Vector *inList, unsigned int &inCount);
  static void __fastcall       LocateViewer();
  static void __fastcall       AddViewerGroup2(unsigned int groupNum);
  static void __fastcall       LocateViewer3();
  static void __fastcall       LocateViewer2();
  static void __fastcall       CullSortTable(NTempest::CRect &sRect);
  static void __fastcall       CullHorizon(NTempest::CRect &sRect);
  static void __fastcall       CullEntitys(CSortEntry *sortEntry);
  static void __fastcall       CullDoodads(CSortEntry *sortEntry);
  static void __fastcall       CullDoodads(TSExplicitList<CMapBaseObjLink, 8> &doodadDefLinkList);
  static void __fastcall       CullChunkLiquid(CSortEntry *sortEntry, unsigned int type);
  static void __fastcall       CullChunks(CSortEntry *sortEntry);
  static void __fastcall       CullMapObjDefs(CSortEntry *sortEntry, NTempest::CRect &sRect);
  static void __fastcall       CullMapObjDef(CMapObjDef *mapObjDef, TSGrowableArray<unsigned int> &inGroups);
  static void __fastcall       CullMapObjDefGroup(const unsigned int groupNum, const void *userParam, const int rDrawSharedLiquidToggle);
  static void __fastcall       RenderObjects();
  static void __fastcall       RenderDoodads();
  static void __fastcall       RenderHorizon();
  static void __fastcall       RenderMapObjDefGroups();
  static void __fastcall       RenderOcean();
  static void __fastcall       RenderWater();
  static void __fastcall       RenderMagma();
  static void __fastcall       ClipBufferClear();
  static int __fastcall        ClipBufferCull(NTempest::C3Vector &center, float radius, unsigned int cullFlags);
  static int __fastcall        ClipBufferCull(NTempest::CAaBox &aaBox, unsigned int cullFlags);
  static void __fastcall       FrustumSet(NTempest::CRect &sRect);
  static void __fastcall       FrustumSet(NTempest::C3Vector *corners);
  static void __fastcall       FrustumSet(NTempest::C3Vector *corners, NTempest::CRect &sRect);
  static void __fastcall       FrustumSet(CWFrustum &frustum);
  static void __fastcall       FrustumPush();
  static void __fastcall       FrustumPop();
  static CWFrustum &__fastcall FrustumGet();
  static void __fastcall       FrustumXform(NTempest::C44Matrix &mat);
  static int __fastcall        FrustumCull(NTempest::C3Vector &center, float radius);
  static int __fastcall        FrustumCull(NTempest::CAaBox &aaBox);
  static int __fastcall        FrustumCull(NTempest::CAaBox &aaBox, NTempest::C33Matrix &basis, NTempest::C3Vector &pos);
  static CWFrustum             frustumStack[16];
  static int                   frustumIndex;
  static NTempest::C4Vector    clipVertexBuffer[9];
  static float                 clipBuffer[128];
  static TSExplicitList<CWFrustum, 0xF4> frustumFreeList;
};

#endif
