#ifndef WOW_SOURCE_WORLDCLIENT_MAP_H
#define WOW_SOURCE_WORLDCLIENT_MAP_H

#include "MapDefs.h"
#include "WorldClient/World.h"
#include "WorldClient/CMapObj.h"
#include "Tempest/crange.h"
#include "Tempest/c22matrix.h"
#include "Tempest/c2ivector.h"
#include "Tempest/c3vector.h"
#include "Tempest/c4plane.h"
#include "Tempest/c4quaternion.h"
#include "Tempest/c44matrix.h"
#include "Tempest/caabox.h"
#include "Tempest/caasphere.h"
#include "Tempest/cirect.h"
#include "Tempest/cimvector.h"
#include "Tempest/crandom.h"

#include <stpl.h>

class CAsyncObject;
class CDetailDoodadInst;
class CGxTex;
class CMapArea;
class CMapChunk;
class CMapDoodadDef;
class CMapObj;
class CMapObjDef;
class CMapObjDefGroup;
class CMapObjGroup;
class CMapStaticEntity;
class CWFrustum;
class SFile;
class WMOAreaTableRec;
struct CGxBuf;
struct CGxBufCommand;
struct HMODEL__;
struct HTEXTURE__;

struct SLVert {
  union {
    SWVert waterVert;
    SOVert oceanVert;
    SMVert magmaVert;
  };
};

class CWSoundEmitter {
 public:
  DWORD              soundPointID;
  DWORD              soundNameID;
  NTempest::C3Vector pos;
  float              minDistance;
  float              maxDistance;
  float              cutoffDistance;
  DWORD              startTime;
  DWORD              endTime;
  DWORD              mode;
  DWORD              groupSilenceMin;
  DWORD              groupSilenceMax;
  DWORD              playInstancesMin;
  DWORD              playInstancesMax;
  DWORD              loopCountMin;
  DWORD              loopCountMax;
  DWORD              interSoundGapMin;
  DWORD              interSoundGapMax;
};

class CMapSoundEmitter {
 public:
  CWSoundEmitter data;
  LINKDECLEX(CMapSoundEmitter, lameAssLink);
};

struct SWFlowv {
  NTempest::CAaSphere sphere;
  NTempest::C3Vector  dir;
  float               velocity;
  float               amplitude;
  float               frequency;
};

struct WaterVert {
  NTempest::C3Vector direction;
  float              magnitude;
  float              amplitude;
  float              height;

  WaterVert() {
  }

  WaterVert(const NTempest::C3Vector &direction, float magnitude, float amplitude, float height)
      : direction(direction), magnitude(magnitude), amplitude(amplitude), height(height) {
  }
};

class WaveTrain {
 public:
  static const float PHASE_GRID_SIZE;
  static const float DEPTH_RANGE_SCALE;

  NTempest::C2Vector  pos;
  NTempest::C2Vector  halfSize;
  float               radiusSq;
  NTempest::C2iVector phaseSize;
  NTempest::C22Matrix localToWorld;
  float               speed;

  enum {
    PHASE_GRID_ARRAY_SIZE = 512
  };

  float phaseGrid[PHASE_GRID_ARRAY_SIZE];

  BOOL Phase(const NTempest::C2Vector &worldPos, float &phase);
  void Init(const NTempest::C2Vector &pPos, const NTempest::C2Vector &pSize, const float radAngle, const float pSpeed);
  void Move(float deltat);
  BOOL Contains(const NTempest::C2Vector &worldPos);
};

class CChunkLiquid {
 public:
  struct UserArg {
    CChunkLiquid *liquid;
    UINT          liquidType;
    WORD          indexCount;

    UserArg(CChunkLiquid *liquid, UINT liquidType) : liquid(liquid), liquidType(liquidType), indexCount(0) {
    }
  };

  WORD        Render0I(WORD *idxBase, UINT liquidType);
  void        RenderOcean0();
  void        RenderOcean0V(CGxVertexPNT0 *vtx);
  static void RenderOcean0Callback(CGxBufCommand &cmd, CGxBuf *gxBuf);
  void        RenderRiver0(UINT type);
  void        RenderRiver0V(CGxVertexPNT0 *vtx);
  static void RenderRiver0Callback(CGxBufCommand &cmd, CGxBuf *gxBuf);
  void        RenderMagma0(UINT type);
  void        RenderMagma0V(CGxVertexPCT0 *vtx);
  static void RenderMagma0Callback(CGxBufCommand &cmd, CGxBuf *gxBuf);

  NTempest::CRange height;
  SLVert           verts[81];
  SLTiles          tiles;
  UINT             nFlowvs;
  SWFlowv          flowvs[2];
  CMapChunk       *chunk;
  LINKDECLEX(CChunkLiquid, sceneLink);
  LINKDECLEX(CChunkLiquid, lameAssLink);

  void Render(UINT type);
  void GetAaBox(NTempest::CAaBox &aaBox);
};

class CMapBaseObj;

NODEDECL(CChunkTex) {
  DWORD pixels[4096];

  CChunkTex();
  CChunkTex(const CChunkTex &);
  ~CChunkTex();
};

NODEDECL(CChunkLayer) {
  WORD        props;
  WORD        effectId;
  HTEXTURE texId;
  BYTE       *offsAlpha;
  CChunkTex  *tex;
  CGxTex     *gxTexture;
  CMapChunk  *chunk;

  CChunkLayer();
  CChunkLayer(const CChunkLayer &);
  ~CChunkLayer();
};

class CMapCacheLight {
 public:
  CGxLight gxLight;
  float    attenStart;
  float    attenEnd;
  float    attenDenom;
  LINKDECLEX(CMapCacheLight, lameAssLink);
};

class CMapAreaLow {
 public:
  static NTempest::C3Vector s_vertexBuffer[65535];
  static WORD               s_indexBuffer[65535];
  static UINT               s_vertexBufferIndex;
  static UINT               s_indexBufferIndex;

  NTempest::CAaBox    aaBox;
  NTempest::CAaSphere aaSphere;
  NTempest::C3Vector  corner;
  NTempest::C2iVector mIndex;
  float               heights[545];
  LINKDECLEX(CMapAreaLow, sceneLink);
};

class CMapBaseObjLink {
 public:
  CMapBaseObj *owner;
  CMapBaseObj *ref;
  LINKDECLEX(CMapBaseObjLink, refLink);
  LINKDECLEX(CMapBaseObjLink, ownerLink);
};

class CMapBaseObj {
 public:
  enum {
    Type_BaseObj = 0x01,
    Type_Area = 0x02,
    Type_Chunk = 0x04,
    Type_MapObjDef = 0x08,
    Type_MapObjDefGroup = 0x10,
    Type_Entity = 0x20,
    Type_DoodadDef = 0x40,
    Type_Light = 0x80
  };

  enum {
    Flag_LightUpdate = 0x0001,
    Flag_GameObj = 0x0002,
    Flag_LoadFailed = 0x0004,
    Flag_InteriorLit = 0x0008,
    Flag_ExteriorLit = 0x0010,
    Flag_HasDoodadRefs = 0x0020,
    Flag_HasLights = 0x0040,
    Flag_Enabled = 0x0080,
    Flag_Impassable = 0x0100,
    Flag_Loaded = 0x0200,
    Flag_HasAllDoodads = 0x0400,
    Flag_NoCollision = 0x0800
  };

 protected:
  DWORD type;

 public:
  LINKDECLEX(CMapBaseObj, lameAssLink);
  LISTDECLEX(CMapBaseObjLink, ownerLink, parentLinkList);
  NTempest::C3Vector     pos;
  float                  scale;
  NTempest::C4Quaternion rot;
  NTempest::CAaBox       aaBox;
  NTempest::CAaSphere    aaSphere;
  NTempest::C3Vector     corner;
  float                  camDist;
  WORD                   flags;
  short                  refCount;

  int TestAABox(const NTempest::C3Vector &v0, const NTempest::C3Vector &v1);

  CMapBaseObj();
  ~CMapBaseObj();

  virtual void SelectLights();

  UINT GetType() {
    return type;
  }
};

class CMapLight : public CMapBaseObj {
 private:
  static HTEXTURE s_hPointAttenTex;
  static void        ProjectLightRenderPN(CGxBufCommand &cmd, CGxBuf *buf);

 public:
  CGxLight gxLight;
  float    attenStart;
  float    attenEnd;
  float    attenDenom;
  BYTE     dynamic;

  void           SetAtten(float attenStart, float attenEnd);
  static CGxTex *GetPointAttenTex();

  CMapLight();
  ~CMapLight();

  void SetConstantAtten(float attenuation);
  void SetLinearAtten(float attenuation);
  void SetQuadraticAtten(float attenuation);
  void Project();

  static LISTDECLEX(CMapBaseObjLink, refLink, dirLightLinkList);
  static UINT  maxLights;
  static float bucketSize;
  static float halfBucketSize;

  static void CreatePointAtten();
  static void DestroyPointAtten();
};

class CMapStaticEntity : public CMapBaseObj {
 public:
  LISTDECLEX(CMapCacheLight, lameAssLink, cacheLightList);
  NTempest::CImVector ambient;
  NTempest::CImVector interiorDirColor;
  float               dirLightScale;
  HMODEL model;
  UINT                flagInside : 1;
  UINT                flagVisible : 1;
  UINT                flagCollidable : 1;
  UINT                flagHidden : 1;
  UINT                flagShadowed : 1;
  UINT                flagInLiquid : 1;
  UINT                flagDeepLiquid : 1;
  UINT                flagAlwaysAnimate : 1;
  UINT                flagCastShadow : 1;

  static const float              dirLightScaleAmount;
  static const NTempest::C3Vector interiorSunDir;

  virtual void QueryLightmap(CMapObjDef *mapObjDef, CMapObjGroup *mapObjGroup) = 0;

  void AdjustLightmap(const NTempest::CImVector &lmColor, NTempest::CImVector &dirColor, BYTE minDir, NTempest::CImVector &ambColor, BYTE maxAmbient);
  BOOL GetMapObjAndGroup(CMapObjDef *&mapObjDef, CMapObj *&mapObj, CMapObjDefGroup *&mapObjDefGroup, CMapObjGroup *&mapObjGroup);
  BOOL GetMapObjDef(CMapObjDef *&mapObjDef);

  virtual void SelectLights();

  void FindLights();
  void CreateCacheLight(CMapLight *light);
};

struct CMapEntity : public CMapStaticEntity {
  int (*handler)(LPVOID, DWORD, DWORDLONG, DWORD);
  DWORDLONG           param64;
  DWORD               param32;
  NTempest::C3Vector  oldPos;
  UINT                rFrameCount;
  NTempest::C3Vector  lqDirection;
  float               lqSurface;
  UINT                lqWhich;
  NTempest::CImVector ambientTarget;
  float               dirLightScaleTarget;
  LINKDECLEX(CMapDoodadDef, sceneLink);

  static const float ambLightScaleRate;
  static const float dirLightScaleRate;

  CMapEntity();
  ~CMapEntity();

  virtual void QueryLightmap(CMapObjDef *mapObjDef, CMapObjGroup *mapObjGroup);
  int          QueryMapObjZoneName(LPCSTR &zoneName);
  int          QueryMapObjSubzoneName(LPCSTR &subzoneName, UINT &subzoneId);
  int          QueryMapObjFileName(LPCSTR &fileName);
  int          QueryMapObjListenerId(UINT &listenerId);
  int          QueryMapObjFog(SMOFog::Fogs &oFog, float &oPct);
  void         QueryLiquidSounds(int *lbool, NTempest::C3Vector *ldelta, float *ldsquared, UINT &closestExtLevel);
  int          QueryMapGroundType(UINT &groundType);
  bool         QueryMapObjMinimap(const NTempest::CAaBox &aaBox, TSStackArray<CWorld::MinimapQuad> &quads);
  bool         QueryMapObjIDs(UINT &wmoID, UINT &instanceID, UINT &groupID);
  bool         QueryMapObjMatrix(NTempest::C44Matrix *mtx, NTempest::C44Matrix *invMtx);
  bool         QueryMapObjAreaTable(const WMOAreaTableRec *&subzoneRec, const WMOAreaTableRec *&globalRec);
  static int   QueryCameraFog(SMOFog::Fogs &oFog, float &oPct);
  void         UpdateMapObjLiquid();
  void         Tick();
};

class CMapDoodadDef : public CMapStaticEntity, public TSHashObject<CMapDoodadDef, HASHKEY_DWORD> {
 public:
  NTempest::C44Matrix lMat;
  NTempest::C44Matrix mat;
  NTempest::CAaBox    collideExt;
  LPCSTR              modelName;
  UINT                rCount;
  UINT                cCount;
  int                 doodadSoundHandle;
  LINKDECLEX(CMapDoodadDef, sceneLink);
  void (*RenderCB)(LPVOID param, const NTempest::C44Matrix &matrix);
  LPVOID renderCBParam;

  CMapDoodadDef();
  ~CMapDoodadDef();

  virtual void SelectLights();
  virtual void QueryLightmap(CMapObjDef *mapObjDef, CMapObjGroup *mapObjGroup);

  void Update(const NTempest::C44Matrix &newMat);
  void GetBounds(NTempest::CAaSphere &bounds);
  void GetBounds(NTempest::CAaBox &bounds);
  void GetCollideExt(NTempest::CAaBox &bounds);
};

class CMapObjDef : public CMapBaseObj, public TSHashObject<CMapObjDef, HASHKEY_NONE> {
 public:
  NTempest::C44Matrix mat;
  NTempest::C44Matrix invMat;
  DWORD               nameId;
  CMapObj            *mapObj;
  WORD                tDoodadRefs;
  WORD                firstDoodadRef;
  DWORD               doodadSet;
  WORD                nameSet;
  LPCSTR              zoneName;
  LISTDECLEX(CMapBaseObjLink, refLink, groupLinkList);
  TSGrowableArray<CMapLight *> lightList;
  UINT                         rCount;
  NTempest::CImVector          ambient;
  DWORDLONG                    param64;
  LINKDECLEX(CMapStaticEntity, sceneLink);

  CMapObjDef();
  ~CMapObjDef();
};

class CMapObjDefGroup : public CMapBaseObj {
 public:
  UINT                            groupNum;
  DWORD                           doodadRefStart;
  DWORD                           nDoodadRefs;
  NTempest::CImVector             ambient;
  LPCSTR                          subzoneName;
  UINT                            level;
  int                             rDrawSharedLiquidToggle;
  LISTDECLEX(CWFrustum, sceneLink, frustumList);
  LISTDECLEX(CMapBaseObjLink, refLink, doodadDefLinkList);
  LISTDECLEX(CMapBaseObjLink, refLink, entityLinkList);
  LISTDECLEX(CMapBaseObjLink, refLink, lightLinkList);
  LINKDECLEX(CMapObjDefGroup, sceneLink);

  CMapObjDefGroup();
  ~CMapObjDefGroup();

  virtual void SelectLights();
  void         UpdateLights();
  void         Update(const NTempest::C44Matrix &newMat);
};

struct SMAreaHeader {
  DWORD offsInfo;
  DWORD offsTex;
  DWORD sizeTex;
  DWORD offsDoo;
  DWORD sizeDoo;
  DWORD offsMob;
  DWORD sizeMob;
  BYTE  pad[36];
};

struct SMDoodadDef {
  DWORD              nameId;
  DWORD              uniqueId;
  NTempest::C3Vector pos;
  NTempest::C3Vector rot;
  WORD               scale;
  WORD               flags;
};

struct SMMapObjDef {
  DWORD              nameId;
  DWORD              uniqueId;
  NTempest::C3Vector pos;
  NTempest::C3Vector rot;
  NTempest::CAaBox   extents;
  WORD               flags;
  WORD               doodadSet;
  WORD               nameSet;
  WORD               pad;
};

struct SMChunkInfo {
  enum {
    FLAG_LOADED = 1
  };

  DWORD offset;
  DWORD size;
  DWORD flags;
  union {
    BYTE  pad[4];
    DWORD asyncId;
  };
};

struct SMAreaInfo {
  enum {
    FLAG_LOADED = 1
  };

  DWORD offset;
  DWORD size;
  DWORD flags;
  union {
    BYTE  pad[4];
    DWORD asyncId;
  };
};

struct SMChunk {
  enum {
    FLAG_SHADOW = 1,
    FLAG_IMPASS = 2,
    FLAG_LQ_RIVER = 4,
    FLAG_LQ_OCEAN = 8,
    FLAG_LQ_MAGMA = 16
  };

  DWORD flags;
  DWORD indexX;
  DWORD indexY;
  float radius;
  DWORD nLayers;
  DWORD nDoodadRefs;
  DWORD offsHeight;
  DWORD offsNormal;
  DWORD offsLayer;
  DWORD offsRefs;
  DWORD offsAlpha;
  DWORD sizeAlpha;
  DWORD offsShadow;
  DWORD sizeShadow;
  DWORD areaid;
  DWORD nMapObjRefs;
  WORD  holes;
  WORD  pad0;
  WORD  predTex[8];
  BYTE  noEffectDoodad[8];
  DWORD offsSndEmitters;
  DWORD nSndEmitters;
  DWORD offsLiquid;
  BYTE  pad1[24];
};

struct SMLayer {
  DWORD textureId;
  DWORD props;
  DWORD offsAlpha;
  WORD  effectId;
  BYTE  pad[2];
};

struct SMNormal {
  char n[145][3];
  char pad[13];
};

struct SMMapHeader {
  DWORD nDoodadNames;
  DWORD offsDoodadNames;
  DWORD nMapObjNames;
  DWORD offsMapObjNames;
  BYTE  pad[112];
};

class CMapChunk : public CMapBaseObj {
 public:
  static UINT cornerVertexIndex[4];
  static UINT farCornerIndex;

  DWORD              infoIndex;
  WORD               holes;
  WORD               pad;
  UINT               lod;
  UINT               remapLod;
  CDetailDoodadInst *detailDoodadInst;
  CMapChunk         *neighbor[4];
  LINKDECLEX(CMapChunk, sceneLink);
  LISTDECLEX(CMapBaseObjLink, refLink, doodadDefLinkList);
  LISTDECLEX(CMapBaseObjLink, refLink, mapObjDefLinkList);
  LISTDECLEX(CMapBaseObjLink, refLink, entityLinkList);
  LISTDECLEX(CMapBaseObjLink, refLink, lightLinkList);
  LISTDECLEX(CMapSoundEmitter, lameAssLink, soundEmitterList);
  CChunkLiquid       *liquids[4];
  NTempest::C2iVector aIndex;
  NTempest::C2iVector sOffset;
  NTempest::C2iVector cOffset;
  float               freeTime;
  BOOL                bLoaded;
  CChunkLayer        *layerList[4];
  UINT                nLayers;
  CChunkTex          *shadowTexture;
  CGxTex             *shadowGxTexture;
  BYTE               *shadowOffs;
  DWORD               shadowSize;
  CGxBuf             *gxBuf;
  CChunkTex          *shaderTexture;
  CGxTex             *shaderGxTexture;
  CAsyncObject       *asyncObject;
  UINT                fileOffset;
  UINT                fileSize;
  NTempest::CRndSeed  rSeed;
  UINT                zoneId;
  WORD                predTex[8];
  BYTE                noEffectDoodad[8];
  NTempest::C3Vector  normalList[145];
  NTempest::C3Vector  vertexList[145];
  NTempest::C4Plane   planeList[256];
  DWORD               shadowBits[32];

 private:
  static BYTE               syncLoadBuffer[15000];
  static NTempest::C2Vector texCoordList[145];
  static NTempest::C2Vector texCoordList2[145];
  static NTempest::C2Vector rmTexCoordList[4][145];
  static NTempest::C2Vector rmTexCoordList2[4][145];
  static CGxBatch           rmGxBatchList[4][2];
  static WORD               primList[768];
  static WORD              *primPtr;
  static TSGrowableArray<CGxBuf *> gxBufFreeList;
  static CGxBuf                   *gxBufDyn;
  static TSGrowableArray<CGxTex *> gxAlphaTexFreeList;
  static TSGrowableArray<CGxTex *> gxShadowTexFreeList;
  static const float               TERRAIN_SPEC_EXP;
  static NTempest::C4Vector        psLayerMask[4];
  static void (*soundEmitterCreateHandler)(CWSoundEmitter &);
  static void (*soundEmitterDestroyHandler)(DWORD);

 public:
  CMapChunk();
  ~CMapChunk();

  void Load(SMChunkInfo *chunkInfo);
  void Create(BYTE *data);

  virtual void SelectLights();
  void         UpdateLights();
  void         Update();
  void         Render();
  void         Purge();
  void         CreateDetailDoodads();
  void         UpdateClipBuffer();

  static void SetSoundEmitterHandlers(void (*create)(CWSoundEmitter &), void (*destroy)(DWORD)) {
    soundEmitterCreateHandler = create;
    soundEmitterDestroyHandler = destroy;
  }

 private:
  void         SyncLoadLayer(CChunkLayer *layer);
  void         SyncLoadShadow();
  void         SyncLoadShader();
  void         SyncLoad(SMChunk *&mChunk, SMLayer *&mLayer, BYTE *&shadowTex, BYTE *&alphaTex);
  void         FindLights();
  void         CreateVertices(float *heights);
  void         CreateVertices2(float *heights);
  void         CreateNormals(signed char *normals);
  void         CreateFacePlanes();
  void         CreateLayer(CMapArea *area, SMLayer *layer, BYTE *alphaTex);
  void         CreateShadow(BYTE *shadowTex);
  void         CreateAlphaShadow();
  void         CreateRefs(CMapArea *area, UINT *ref, UINT doodadCnt, UINT mapObjCnt);
  void         RenderLayers();
  void         RenderLayersDyn();
  void         RenderLayersColor();
  void         RenderLayersColorDyn();
  void         FillGxBufVertex(const CGxBufCommand &cmd, CGxBuf *buf);
  void         FillGxBufIndex(const CGxBufCommand &cmd, CGxBuf *buf);
  void         FillGxBufDynVertex(const CGxBufCommand &cmd, CGxBuf *buf);
  void         FillGxBufDynIndex(const CGxBufCommand &cmd, CGxBuf *buf);
  void         RemapVertices();
  void         RemapVerticesDyn();
  void         PurgeLayer(CChunkLayer *layer);
  void         CreateChunkLayerTex(CChunkLayer *layer);
  void         CreateChunkShadowTex();
  void         CreateChunkShaderTex();
  static void  FreeAsyncLoadBuffer(BYTE *buffer);
  static void  InitAsyncLoadBuffers();
  static BYTE *AllocAsyncLoadBuffer();
  static void  AsyncCallback(LPVOID userArg);
  static void  LodCreateTree(int level, int maxLevel, int neighborLOD, int holes, int cX, int cY);

 public:
  static void Initialize();
  static void Destroy();
  static void FreeLists();
  static void AsyncPollHandler();

 private:
  static CGxBuf *AllocGxBuf(UINT indexCount);
  static void    FreeGxBuf(CGxBuf *gxBuf);
  static CGxTex *AllocAlphaGxTex(LPVOID userArg, void (*userFunc)(EGxTexCommand, UINT, UINT, UINT, UINT, LPVOID, UINT &, LPCVOID &));
  static void    FreeAlphaGxTex(CGxTex *gxTex);
  static CGxTex *AllocShadowGxTex(LPVOID userArg, void (*userFunc)(EGxTexCommand, UINT, UINT, UINT, UINT, LPVOID, UINT &, LPCVOID &));
  static void    FreeShadowGxTex(CGxTex *gxTex);
  static void    GxBufDynFillCallback(CGxBufCommand &cmd, CGxBuf *buf);
  static void    GxBufFillCallback(CGxBufCommand &cmd, CGxBuf *buf);
  static void    CreateRenderLists();
  static void    UnpackAlphaBits(DWORD *pixels, const BYTE *alphaPixels);
  static void    UnpackShadowBits(DWORD *pixels, DWORD *shadowBits, const BYTE *shadow);
  static void    UnpackAlphaShadowBits(NTempest::CImVector *texels, DWORD *bits, const BYTE *const alpha[], const BYTE *shadow);
  static void
  UpdateLayerGxTexture(EGxTexCommand cmd, UINT w, UINT h, UINT d, UINT mipLevel, LPVOID userArg, UINT &texelStrideInBytes, LPCVOID &texels);
  static void
  UpdateShadowGxTexture(EGxTexCommand cmd, UINT w, UINT h, UINT d, UINT mipLevel, LPVOID userArg, UINT &texelStrideInBytes, LPCVOID &texels);
  static void
  UpdateShaderGxTexture(EGxTexCommand cmd, UINT w, UINT h, UINT d, UINT mipLevel, LPVOID userArg, UINT &texelStrideInBytes, LPCVOID &texels);
  static void
  UpdateTextureDefault(EGxTexCommand cmd, UINT w, UINT h, UINT d, UINT mipLevel, LPVOID userArg, UINT &texelStrideInBytes, LPCVOID &texels);
};

class Particulate {
 private:
  static NTempest::C3Vector s_vcv[4];
  static NTempest::C2Vector s_tc[13][4];
  static UINT               s_tcSub[4][8];
  static const float        PTSIZE;
 public:
  struct Particle {
    NTempest::C3Vector pos;
    float              scale;
  };
  enum {
    MAX_PARTICLES = 4000,
    MAX_RENDER = 666
  };
 private:
  Particle                  particles[MAX_PARTICLES];
  UINT                      numParticles;
  NTempest::C3Vector        lastCamPos;
  HTEXTURE texture;
  BYTE                      show;
  float                     scale;
  float                     boxSize;
  float                     percent;
  UINT                      liquid;
 public:
  struct Movement {
    NTempest::C3Vector dir;
    float              freq;
    float              time;
    float              amplitude;
  };
 private:
  Movement                  movement;
  NTempest::C3Vector ComputeMovement(float elapsedTime);
  void               InitMovement();

 public:
  Particulate(float particleScale, float boxSize, LPCSTR particulateTexture);
  ~Particulate();

  void SetPercentage(float percent);
  void SetSize(float units);
  void SetScale(float s);
  void SetTexture(LPCSTR name);
  void InitParticles(UINT l);
  void Show(BYTE show) {
    this->show = show;
  }
  void        Update();
  void        Render();
  static void CustomRenderCallback(LPVOID p1, int p2);
};

class CMapArea : public CMapBaseObj {
 public:
  LISTDECLEX(CMapBaseObjLink, refLink, chunkLinkList);
  DWORD                        infoIndex;
  NTempest::C2iVector          mIndex;
  NTempest::C2iVector          cOffset;
  NTempest::CiRect             localRect;
  UINT                         texCount;
  SMAreaHeader                 header;
  CAsyncObject                *asyncObject;
  TSCArray<HTEXTURE , 96>   texIdTable;
  TSGrowableArray<SMDoodadDef> doodadDefList;
  TSGrowableArray<SMMapObjDef> mapObjDefList;
  SMChunkInfo                  chunkInfo[256];
  CMapChunk                   *chunkTable[256];

  static int ccWaterShowTri;
  static int ccWaterLOD;
  static int ccWaterWaves;
  static int ccWaterSpecular;
  static int ccWaterRipples;
  static int ccWaterMaxLOD;

 private:
  void Create(BYTE *data);
  void LoadTextures(char *texNames, DWORD size);

 public:
  CMapArea();
  ~CMapArea();

  void        Load(SMAreaInfo *areaInfo);
  void        PrepareLocalRect();
  void        Purge();
  void        PurgeChunks();
  void        InitWater();
  void        QueryLiquidSounds(const NTempest::C3Vector &worldPos, float radius, int *lbool, NTempest::C3Vector *ldelta, float *ldsquared);
  static void Initialize();
  static void Destroy();
  static void AsyncPollHandler();

 private:
  static void  FreeAsyncLoadBuffer(BYTE *buffer);
  static void  InitAsyncLoadBuffers();
  static BYTE *AllocAsyncLoadBuffer();
  static void  AsyncCallback(LPVOID userArg);
};

#define LIQUID_COUNT         9
#define LIQUID_TEXTURE_COUNT 30

class CMap {
 public:
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

  static LISTDECLEX(CMapLight, lameAssLink, lightFreeList);
  static LISTDECLEX(CMapLight, lameAssLink, lightList);
  static LISTDECLEX(CMapCacheLight, lameAssLink, cacheLightFreeList);

 private:
  friend class CWorld;
  friend class CMapChunk;

  static LISTDECLEX(CMapObjGroup, lameAssLink, mapObjGroupFreeList);
  static LISTDECLEX(CMapObj, lameAssLink, mapObjFreeList);
  static LISTDECLEX(CMapBaseObjLink, ownerLink, baseObjLinkFreeList);
  static LISTDECLEX(CMapArea, lameAssLink, areaFreeList);
  static LISTDECLEX(CMapChunk, lameAssLink, chunkFreeList);
  static LISTDECLEX(CMapDoodadDef, lameAssLink, doodadDefFreeList);
  static LISTDECLEX(CMapEntity, lameAssLink, entityFreeList);
  static LISTDECLEX(CMapObjDefGroup, lameAssLink, mapObjDefGroupFreeList);
  static LISTDECLEX(CMapObjDef, lameAssLink, mapObjDefFreeList);
  static LISTDECLEX(CChunkLiquid, lameAssLink, chunkLiquidFreeList);
  static LISTDECLEX(CMapSoundEmitter, lameAssLink, soundEmitterFreeList);
  static LISTDECLEX(CMapArea, lameAssLink, areaList);
  static LISTDECLEX(CMapChunk, lameAssLink, chunkList);
  static TSHashTable<CMapDoodadDef, HASHKEY_DWORD> doodadDefHash;
  static LISTDECLEX(CMapEntity, lameAssLink, entityList);
  static LISTDECLEX(CMapObjDefGroup, lameAssLink, mapObjDefGroupList);
  static TSHashTable<CMapObjDef, HASHKEY_NONE> mapObjDefHash;
  static LISTDECLEX(CChunkLiquid, lameAssLink, chunkLiquidList);
  static int  counts[Cnt_Num];
  static int  freeCounts[Cnt_Num];
  static UINT uniqueId;
  static UINT GetUniqueId();
  static bool enablePixelShaders;
  static bool enableSpecular;
  static bool enableSpecularTerrain;
  static bool enableTerrainShader;
  static bool enableSpecularWater;

 public:
  static CMapBaseObjLink  *AllocBaseObjLink(CMapBaseObj *owner);
  static CMapObj          *AllocMapObj();
  static CMapObjGroup     *AllocMapObjGroup();
  static CMapArea         *AllocArea();
  static CMapChunk        *AllocChunk();
  static CMapDoodadDef    *AllocDoodadDef();
  static CMapEntity       *AllocEntity();
  static CMapLight        *AllocLight();
  static CMapCacheLight   *AllocCacheLight();
  static CMapObjDefGroup  *AllocMapObjDefGroup();
  static CMapObjDef       *AllocMapObjDef();
  static CChunkLiquid     *AllocChunkLiquid();
  static CMapSoundEmitter *AllocSoundEmitter();
  static void              FreeBaseObjLink(CMapBaseObjLink *link);
  static void              FreeMapObj(CMapObj *mapObj);
  static void              FreeMapObjGroup(CMapObjGroup *group);
  static void              FreeArea(CMapArea *area);
  static void              FreeChunk(CMapChunk *chunk);
  static void              FreeDoodadDef(CMapDoodadDef *doodadDef);
  static void              FreeEntity(CMapEntity *entity);
  static void              FreeLight(CMapLight *light);
  static void              FreeCacheLight(CMapCacheLight *cacheLight);
  static void              FreeMapObjDefGroup(CMapObjDefGroup *mapObjDefGroup);
  static void              FreeMapObjDef(CMapObjDef *mapObjDef);
  static void              FreeChunkLiquid(CChunkLiquid *&cl);
  static void              FreeSoundEmitter(CMapSoundEmitter *soundEmitter);
  static bool              EnablePixelShaders() {
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
  static CGxPixelShader *psSpecTerrain;
  static CGxShaderParam *psSpecTerrain_LayerMask;
  static CGxPixelShader *psTerrain;
  static CGxShaderParam *psTerrain_LayerMask;
  static CGxPixelShader *psSpecUTerrain;
  static CGxShaderParam *psSpecUTerrain_LayerMask;
  static CGxPixelShader *psUTerrain;
  static CGxShaderParam *psUTerrain_LayerMask;
  static CGxBuf         *gxBufDynLowDetail;
  static LISTDECL(CChunkLayer, chunkLayerFreeList);
  static LISTDECL(CChunkTex, chunkTexFreeList);
  static CChunkLayer *GetLayer();
  static CChunkTex   *GetTex();
  static void         FreeLayer(CChunkLayer *layer);
  static void         FreeTex(CChunkTex *tex);
  static SFile       *wdtFile;
  static DWORD        version;
  static SMMapHeader  header;
  static BOOL         bActive;
  static BOOL         bPreload;
  static BOOL         bDungeon;
  static SMAreaInfo   areaInfo[4096];
  static CMapArea    *areaTable[4096];
  static DWORD        areaLowOffsets[4096];
  static CMapAreaLow *areaLowTable[4096];
  static LISTDECLEX(CMapBaseObjLink, refLink, areaLinkList);
  static LISTDECLEX(CMapBaseObjLink, refLink, doodadDefLinkList);
  static LISTDECLEX(CMapBaseObjLink, refLink, mapObjDefLinkList);
  static HASHKEY_NONE          nullHashKey;
  static TSGrowableArray<char> doodadNames;
  static TSGrowableArray<UINT> doodadNamesIndex;
  static TSGrowableArray<char> mapObjNames;
  static TSGrowableArray<UINT> mapObjNamesIndex;
  enum {
    NUM_RIPPLES = 48,
    NUM_LIQUID_TEX_FRAMES = 30
  };
  static const UINT SKYTEX_HEIGHT;
  static const UINT WATERTEX_HEIGHT;
  static LISTDECL(WaterRadWave, waterRipplesActive);
  static LISTDECL(WaterRadWave, waterRipplesFree);
  static TSFixedArray<NTempest::CImVector> skyTexels;
  static CGxTex                           *skyTexid;
  enum {
    OCEAN_DIFF_TEX = 0,
    RIVER_DIFF_TEX = 1
  };
  static CGxTex         *riverDiffTexid;
  static CGxTex         *oceanDiffTexid;
  static bool            oceanDiffTexUpdated;
  static bool            riverDiffTexUpdated;
  static const float     WATER_SPEC_EXP;
  static CGxPixelShader *psOcean0;
  static HTEXTURE liquidTex[LIQUID_COUNT][LIQUID_TEXTURE_COUNT];
  static bool            liquidTexLoaded[LIQUID_COUNT];
  static float           liquidLastShown[LIQUID_COUNT];
  static const float     liquidTexLoopTime[LIQUID_COUNT];
  static LPCSTR          liquidTexBaseName[LIQUID_COUNT];
  static const float     LIQUID_TEX_PURGE_TIME;
  static char            mapPath[256];
  static char            mapName[256];
  static char            wdtFilename[256];
  static char            wobFilename[256];
  static TSGrowableArray<UINT> scCollideList;
  static UINT                  scCollideCnt;
  static UINT                  cCount;
  static UINT                  bspRecurseCount;
  static UINT                  mapGetFacetsCount;
  static UINT                  nChunksPrepared;
  static UINT                  nGbChunksPrepared;
  static TSGrowableArray<CGxVertexPC> testQueryVerts;
  static TSGrowableArray<WORD>        testQueryIndices;
  static int (*entityHandler)(LPVOID handlerParam, DWORD event, DWORDLONG guid, DWORD flags);
  static LPVOID entityHandlerParam;
  static int (*entityCollisionHandler)(DWORDLONG guid, DWORD flags, WorldObjCollisionHandlerData *data);
  static LPVOID         oldSelectLightParm;
  static CMapLight     *sunLight;
  static void           GetCounts(int counts[]);
  static void           CalcMem();
  static DWORD          GetTextureUseage();
  static HTEXTURE LoadTexture(LPCSTR fileName);
  static void           Initialize();
  static void           Destroy();
  static void           ClearDetailDoodads();
  static void           Load(LPCSTR fileName);
  static void           Unload();
  static void           LoadWdl();
  static void           LoadWdt();
  static void           Preload();
  static void           Open();
  static void           ReloadDoodadModels();
  static void           EnableDoodadFullAlpha(int enable);
  static int            LoadDoodadModel(CMapDoodadDef *doodadDef, BOOL bWait);
  static CMapDoodadDef *CreateDoodadDef(LPCSTR fileName, NTempest::C3Vector &pos, float angle, BOOL bWait);
  static CMapDoodadDef *CreateDoodadDef(SMDoodadDef &smDoodadDef, NTempest::C3Vector &pos);
  static CMapObjDef *CreateMapObjDef(LPCSTR fileName, NTempest::C3Vector &pos, float angle, BOOL bWait);
  static CMapObjDef *CreateMapObjDef(SMMapObjDef &smMapObjDef, NTempest::C3Vector &pos);
  static void        CreateMapObjDefGroupDoodads(CMapObj *mapObj, CMapObjGroup *mapObjGroup, CMapObjDef *mapObjDef, CMapObjDefGroup *mapObjDefGroup);
  static void        UpdateMapObjDefGroupDoodads(CMapObj *mapObj, CMapObjGroup *mapObjGroup, CMapObjDef *mapObjDef, CMapObjDefGroup *mapObjDefGroup);
  static void        CreateMapObjDefLights(CMapObj *mapObj, CMapObjGroup *mapObjGroup, CMapObjDef *mapObjDef, CMapObjDefGroup *mapObjDefGroup);
  static void        PurgeMapObjDef(CMapObjDef *mapObjDef);
  static void        PurgeMapObjDefGroup(CMapObjDefGroup *mapObjDefGroup);
  static void        PurgeDoodadDef(CMapDoodadDef *doodadDef);
  static void        PurgeArea(CMapArea *area);
  static void        PurgeChunk(CMapChunk *chunk);
  static UINT        QueryAreaId(float x, float y);
  static bool        QueryGroundType(const NTempest::C3Vector &pos, UINT &groundType);
  static bool        QueryShadow(const NTempest::C3Vector &pos);
  static float       PointIntersect(float wx, float wy, float radius);
  static bool        GetPlane(float wx, float wy, NTempest::C4Plane &plane);
  static bool VectorIntersect(const NTempest::C3Vector *p0, const NTempest::C3Vector *p1, NTempest::C3Vector *ip, float *dist, UINT queryFlags);
  static bool        VectorIntersectTerrain(const NTempest::C3Vector *p0, const NTempest::C3Vector *p1, float *t, UINT queryFlags, CMapChunk **chunk);
  static bool VectorIntersectMapObjs(
      const NTempest::C3Vector *p0,
      const NTempest::C3Vector *p1,
      UINT                      queryFlags,
      UINT                      polyIgnoreFlags,
      UINT                      groupIgnoreFlags,
      float                    *t,
      SMOPoly                 **poly,
      CMapObj                 **qMapObj
  );
  static bool VectorIntersectDoodadDefLinkList(
      LISTEX(CMapBaseObjLink, refLink) & doodadDefLinkList,
      const NTempest::C3Vector *p0,
      const NTempest::C3Vector *p1,
      float                    *t,
      UINT                      queryFlags
  );
  static bool VectorIntersectGameObjLinkList(
      LISTEX(CMapBaseObjLink, refLink) & gameObjLinkList,
      const NTempest::C3Vector *p0,
      const NTempest::C3Vector *p1,
      float                    *t,
      UINT                      queryFlags
  );
  static bool GetFacet(const NTempest::C3Segment &seg, float &t, NTempest::C4Plane &facet, UINT queryFlags);
  static bool GetFacets(const NTempest::CAaBox &aaBox, CWFacetData *facetData, UINT queryFlags);
  static bool GetFacets(const CWFrustum &frustum, CWFacetData *facetData, UINT queryFlags);
  static bool GetTris(const NTempest::CAaBox &aaBox, CWTriData &triData, UINT queryFlags);
  static void TestQueryAdd(const NTempest::CFacet &facet, NTempest::CImVector color, const NTempest::C44Matrix *basis);
  static void TestQueryAdd(const NTempest::CAaBox &aaBox, NTempest::CImVector color, const NTempest::C44Matrix *basis);
  static void TestQueryAdd(const CWFrustum &frustum, NTempest::CImVector color, const NTempest::C44Matrix *basis);
  static void TestQueryRender();
  static void RenderLow();
  static void RenderAreaLow(CMapAreaLow *areaLow);
  static void GxBufDynLowDetailCallback(CGxBufCommand &cmd, CGxBuf *buf);
  static void CreateAreaLowDetailVertices(CMapAreaLow *areaLow, const CGxBufCommand &cmd, CGxBuf *buf);
  static void CreateAreaLowDetailIndices(CMapAreaLow *areaLow, const CGxBufCommand &cmd, CGxBuf *buf);
  static void PrepareUpdate();
  static void Update();
  static void CreateChunkNeighborPtrs(CMapChunk *chunk);
  static void MakeAllEntityNonVisible();
  static void UpdateEntity(CMapEntity *entity);
  static void LinkEntity(CMapStaticEntity *entity);
  static void InitializeDoodadBounds(CMapDoodadDef *doodadDef);
  static void SnapBaseObjToSubChunk(CMapBaseObj *baseObj, NTempest::C3Vector &pos, float angle);
  static void UpdateDoodadDef(CMapDoodadDef *doodadDef, NTempest::C3Vector &pos, float angle);
  static void UpdateMapObjDef(CMapObjDef *mapObjDef, NTempest::C3Vector &pos, float angle);
  static void UpdateMapObjDef(CMapObjDef *mapObjDef, CMapObj *mapObj);
  static void SetLightFuncs();
  static void GxuLightInitialize();
  static void GxuLightShutdown();
  static DWORD     GxuLightCreate();
  static void      GxuLightDestroy(DWORD lightId);
  static CGxLight *GxuLightLock(DWORD lightId);
  static void      GxuLightUnlock(DWORD lightId);
  static void      GxuLightSelect(NTempest::C3Vector worldPos, const NTempest::C3Vector &cameraWorldPos, UINT maxLightsToUse);
  static BOOL      GxuLightEnable(DWORD lightId);
  static void      GxuLightEnableSet(DWORD lightId, int enable);
  static void      GxuLightSetMaxLights(UINT maxLightsToUse);
  static float     GxuLightBucketSize();
  static void      GxuLightBucketSizeSet(float bucketSize);
  static void      GxuLightResetCache();
  static CMapLight *CreateLight(bool dynamic);
  static void       DestroyLight(CMapLight *light);
  static void       UpdateLight(CMapLight *light);
  static void       UpdateLightBounds(CMapLight *light);
  static void       EnableLight(CMapLight *light);
  static void       DisableLight(CMapLight *light);
  static void       SelectLight(CMapBaseObj *baseObj);
  static void       SelectLight(LPVOID parm, NTempest::C3Vector worldPos, const NTempest::C3Vector &cameraWorldPos, UINT maxLightsToUse);
  static bool
  LocateViewerMapObjs(const NTempest::C3Vector &lCen, const NTempest::C3Vector &lEnd, float &maxT, CMapObjDef *&hitMapObjDef, UINT hitGroupIDs[]);
  static bool
  LocateViewerMapObjs4(const NTempest::C3Vector &lCen, const NTempest::C3Vector &lEnd, float &maxT, CMapObjDef *&hitMapObjDef, UINT *hitGroupIDs);
  static bool QueryLiquidStatus(const NTempest::C3Vector &point, UINT &liquid, float &surface, NTempest::C3Vector &waterDir, int &deep);
  static bool QueryLiquidStatusMapObjsExt(const NTempest::C3Vector &point, UINT &liquid, float &surface, NTempest::C3Vector &waterDir);
  static void QueryLiquidSounds(const NTempest::C3Vector &worldPos, float radius, int *lbool, NTempest::C3Vector *ldelta, float *ldsquared);
  static bool QueryLiquidFishable(const NTempest::C3Vector &point, int &fishable);
  static bool QueryLiquidFishableMapObjsExt(const NTempest::C3Vector &point, int &fishable);
  static void WaterRipple(const NTempest::C3Vector &pos, float len, float time, float amp, float vel, float freq);
  static void WaterInitialize();
  static void WaterDestroy();
  static void UpdateOcean();
  static void UnloadLiquidTexture(UINT liquid);
  static void UpdateLiquidTextures();
  static void OceanFFT();
  static void
  WaterDiffTexCallback(EGxTexCommand cmd, UINT w, UINT h, UINT d, UINT mipLevel, LPVOID userArg, UINT &texelStrideInBytes, LPCVOID &texels);
  static HTEXTURE GetLiquidTexture(UINT liquid);
  static void        ProjectLights();

 private:
  static void  CreateChunk(CMapArea *area, CMapChunk *chunk, DWORD flags);
  static CMapDoodadDef *
  CreateDoodadDef(UINT doodadRef, SMODoodadDef &smoDoodadDef, LPCSTR fileName, UINT mapObjDefId, NTempest::C44Matrix &mapObjDefMat);
  static void  LoadDoodadNames();
  static void  LoadMapObjNames();
  static void  CreateMapObjDefGroups(CMapObj *mapObj, CMapObjDef *mapObjDef);
  static void  PrepareAreas();
  static void  PrepareMapObjDefs();
  static void  PrepareDoodadDefs();
  static void  PrepareChunks();
  static void  QueryLightmap(CMapDoodadDef *doodadDef);
  static void  PrepareMapObjDef(CMapObjDef *mapObjDef, CMapObj *mapObj);
  static void  PrepareArea(int x, int y);
  static void  PrepareChunk(CMapArea *area, int x, int y);
  static void  Purge();
  static void  UpdateMapObjDefs();
  static void  UpdateMapObjDefGroups(CMapObjDef *mapObjDef, CMapObj *mapObj);
  static void  UpdateChunks(CMapArea *area);
  static void  LinkLightToMapObjDefs(CMapLight *light);
  static void  LinkLightToChunks(CMapLight *light);
  static void  LodCreateTree(int x0, int y0, int x1, int y1, int level, int parent);
  static float PointIntersectSubChunk(float x, float y, int ix, int iy, const CMapChunk *chunk);
  static void  VectorIntersectSX(NTempest::CiRect &sRect);
  static void  VectorIntersectSY(NTempest::CiRect &sRect);
  static void  VectorIntersectDX(const NTempest::C3Vector &p0, const NTempest::C3Vector &p1, NTempest::CiRect &sRect);
  static void  VectorIntersectDY(const NTempest::C3Vector &p0, const NTempest::C3Vector &p1, NTempest::CiRect &sRect);
  static bool  VectorIntersectSubchunk(const NTempest::C3Vector *p0, const NTempest::C3Vector *p1, float *t, UINT queryFlags);
  static bool  VectorIntersectSubchunk(const NTempest::C3Vector *p0, const NTempest::C3Vector *p1, const NTempest::C3Vector *normal, float *t);
  static bool VectorIntersectSubchunks(const NTempest::C3Vector *p0, const NTempest::C3Vector *p1, float *t, UINT queryFlags, CMapChunk **retChunk);
  static bool  VectorIntersectTri(
      const NTempest::C3Vector *p,
      const NTempest::C3Vector *v0,
      const NTempest::C3Vector *v1,
      const NTempest::C3Vector *v2,
      const NTempest::C3Vector *n
  );
  static bool GetFacetMapObjs(const NTempest::C3Segment &seg, float &t, NTempest::C4Plane &facet, UINT queryFlags);
  static bool GetFacetTerrain(const NTempest::C3Segment &seg, float &t, NTempest::C4Plane &facet, UINT queryFlags);
  static bool GetFacetSubchunks(const NTempest::C3Segment &seg, float &t, NTempest::C4Plane &facet, UINT queryFlags);
  static bool GetChunkFacets(int cx, int cy, NTempest::CiRect &sRect, const NTempest::CAaBox &aaBox, CWFacetData *facetData, UINT queryFlags);
  static bool GetChunkFacets(int cx, int cy, const NTempest::CiRect &sRect, const CWFrustum &wFrustum, CWFacetData *facetData);
  static void CreateImpassableFacets(CMapChunk *chunk, const NTempest::CAaBox &aaBox, CWFacetData *facetData, UINT queryFlags);
  static bool GetFacetsMapObjs(const NTempest::CAaBox &aaBox, CWFacetData *facetData, UINT queryFlags);
  static bool GetFacetsMapObjs(const CWFrustum &frustum, CWFacetData *facetData, UINT queryFlags);
  static bool GetTrisMapObjs(const NTempest::CAaBox &aaBox, CWTriData &triData, UINT queryFlags);
  static bool GetTrisTerrain(const NTempest::CAaBox &aaBox, CWTriData &triData, UINT queryFlags);
  static bool GetTrisChunk(int cx, int cy, NTempest::CiRect &sRect, const NTempest::CAaBox &aaBox, CWTriData &triData, UINT queryFlags);
  static void LinkEntityToMapObj(CMapStaticEntity *entity, CMapObjDef *mapObjDef, CMapObjDefGroup *mapObjDefGroup);
  static void LinkEntityToChunk(CMapStaticEntity *entity, CMapChunk *chunk);
  static bool LinkIntersectMapObjs(
      const NTempest::C3Vector &lCen,
      const NTempest::C3Vector &lEnd,
      float                    &hitT,
      CMapObjDef              *&hitMapObjDef,
      CMapObjDefGroup         *&hitMapObjDefGroup
  );
};

#define LQ_LAST 4

class CSortEntry {
 public:
  LISTDECLEX(CWFrustum, sceneLink, frustumList);
  LISTDECLEX(CMapChunk, sceneLink, chunkList);
  LISTDECLEX(CMapDoodadDef, sceneLink, doodadDefList);
  LISTDECLEX(CMapObjDef, sceneLink, mapObjDefList);
  LISTDECLEX(CChunkLiquid, sceneLink, liquidList[LQ_LAST]);
  LISTDECLEX(CMapEntity, sceneLink, entityList);
};

class CSortTable {
 public:
  CSortEntry table[26];
  LISTDECLEX(CWFrustum, sceneLink, frustumList);
  LISTDECLEX(CMapAreaLow, sceneLink, visAreaLowList);
  LISTDECLEX(CMapChunk, sceneLink, visChunkList);
  LISTDECLEX(CMapDoodadDef, sceneLink, visDoodadList);
  LISTDECLEX(CMapObjDefGroup, sceneLink, visMapObjDefGroupList);
  LISTDECLEX(CMapEntity, sceneLink, visEntityList);
  LISTDECLEX(CChunkLiquid, sceneLink, visLiquidList[4]);
  LISTDECLEX(CMapEntity, sceneLink, nonVisEntityList);
  LISTDECLEX(CMapChunk, sceneLink, updateChunkList);

  void Initialize();
  void Destroy();
  void Clear();
};

class CWorldScene {
  friend class CMapChunk;
  friend class CMapObj;

 public:
  static LISTDECLEX(CWFrustum, sceneLink, frustumFreeList);
  static CSortTable            sortTable;
  static NTempest::C4Vector    clipVertexBuffer[9];
  static float                 clipBuffer[128];
  static CMapEntity           *camTargEntity;
  static CMapObjDef           *viewerMapObjDef;
  static TSGrowableArray<UINT> viewerMapObjGroups;
  static UINT                  bspStateBits;
  static float                 cullSmallThreshold;
  static float                 cullDistance;
  static NTempest::CAaBox      camFrustumBounds;
  static NTempest::C3Vector    camFrustumCorners[8];
  static CWFrustum             frustumStack[16];
  static int                   frustumIndex;
  static NTempest::C3Vector    camPos;
  static NTempest::C3Vector    camTarg;
  static NTempest::C3Vector    camVec;
  static NTempest::C4Plane     camPlaneXY;
  static UINT                  camLiquid;
  static CMapObjDef           *camMapObjDef;
  static CMapObj              *camMapObj;
  static CMapObjGroup         *camMapObjGroup;
  static NTempest::C44Matrix   mvp;
  static NTempest::C44Matrix   mv;
  static NTempest::C44Matrix   mp;
  static NTempest::C3Vector    vpMinPos;
  static NTempest::C3Vector    vpMaxPos;
  static NTempest::C4Plane     vpPlanes[4];
  static NTempest::C4Vector    mvpCol3;
  static NTempest::C44Matrix   gxViewMat;
  static UINT                  nPrimsRendered;
  static UINT                  nChunksRendered;
  static UINT                  nDoodadsRendered;
  static UINT                  nObjectsRendered;
  static char                  currentChunkName[64];

 private:
  static void PrepareRenderLiquid();

 public:
  static void       Initialize();
  static void       Destroy();
  static void       PrepareRender(const NTempest::C3Vector &position, const NTempest::C3Vector &target);
  static void       Update();
  static void       Render();
  static void       RenderAlpha();
  static void       CalcFrustumCorners(NTempest::C3Vector corners[]);
  static void       AddDoodadDef(CMapDoodadDef *doodadDef);
  static void       AddMapObjDef(CMapObjDef *mapObjDef);
  static void       AddMapChunk(CMapChunk *chunk, float sortDist);
  static void       AddMapEntity(CMapEntity *entity);
  static void       AddChunkLiquid(CChunkLiquid *liquid, UINT type);
  static void       ClipBufferUpdate(const NTempest::C3Vector *vertices, const int *indicies, const int nVertices, const NTempest::C3Vector &corner);
  static void       ClipPortal(NTempest::C4Vector *inList, UINT &inCount);
  static void       FrustumPush();
  static void       FrustumSet(const NTempest::CRect &sRect);
  static void       FrustumSet(const NTempest::C3Vector corners[]);
  static void       FrustumSet(const NTempest::C3Vector corners[], const NTempest::CRect &sRect);
  static void       FrustumSet(const CWFrustum &frustum);
  static CWFrustum &FrustumGet();
  static void       FrustumXform(const NTempest::C44Matrix &mat);
  static BOOL       FrustumCull(const NTempest::C3Vector &center, float radius);
  static BOOL       FrustumCull(const NTempest::CAaBox &aaBox);
  static BOOL       FrustumCull(const NTempest::CAaBox &aaBox, NTempest::C33Matrix &basis, NTempest::C3Vector &pos);
  static void       FrustumPop();
  static CWFrustum *AllocFrustum();
  static void       FreeFrustum(CWFrustum *frustum);

 private:
  static void LocateViewer();
  static void AddViewerGroup2(UINT groupNum);
  static void LocateViewer2();
  static void LocateViewer3();
  static void CullSortTable(const NTempest::CRect &sRect);
  static void CullHorizon(const NTempest::CRect &sRect);
  static void CullChunks(CSortEntry *sortEntry);
  static void CullChunkLiquid(CSortEntry *sortEntry, UINT type);
  static void CullEntitys(CSortEntry *sortEntry);
  static void CullDoodads(CSortEntry *sortEntry);
  static void CullDoodads(LISTEX(CMapBaseObjLink, refLink) & doodadDefLinkList);
  static void CullMapObjDefs(CSortEntry *sortEntry, const NTempest::CRect &sRect);
  static void CullMapObjDef(CMapObjDef *mapObjDef, TSGrowableArray<UINT> &inGroups);
  static void CullMapObjDefGroup(const UINT groupNum, LPCVOID userParam, const int rDrawSharedLiquidToggle);
  static void RenderHorizon();
  static void RenderChunks();
  static void RenderMapObjDefGroups();
  static void RenderObjects();
  static void RenderDoodads();
  static void RenderOcean();
  static void RenderWater();
  static void RenderMagma();
  static void ClipBufferClear();
  static int  ClipBufferCull(const NTempest::C3Vector &center, float radius, UINT cullFlags);
  static int  ClipBufferCull(const NTempest::CAaBox &aaBox, UINT cullFlags);
};

#endif
