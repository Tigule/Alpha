#ifndef WOW_SOURCE_WORLDCLIENT_MAP_H
#define WOW_SOURCE_WORLDCLIENT_MAP_H

#include "MapDefs.h"
#include "Tempest/crange.h"
#include "Gx/CGxDevice.h"
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

struct SWVert {
  BYTE  depth;
  BYTE  flow0Pct;
  BYTE  flow1Pct;
  BYTE  filler;
  float height;
};

struct SOVert {
  BYTE depth;
  BYTE foam;
  BYTE wet;
  BYTE filler;
};

struct SMVert {
  WORD  s;
  WORD  t;
  float height;
};

struct SLVert {
  union {
    SWVert waterVert;
    SOVert oceanVert;
    SMVert magmaVert;
  };
};

struct SLTiles {
 public:
  int  GetLiquid(const NTempest::C2iVector &pos, UINT &liquid, int &fishable, int &deep) const;
  void SetLiquid(const NTempest::C2iVector &pos, UINT liquid, int fishable, int deep);

 private:
  friend class CChunkLiquid;
  friend class CMap;
  friend class CMapArea;

  BYTE tiles[8][8];
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

class CChunkLiquid {
 public:
  struct UserArg {
    UserArg(CChunkLiquid *liquid, UINT liquidType) : liquid(liquid), liquidType(liquidType), indexCount(0) {
    }

    CChunkLiquid *liquid;
    UINT          liquidType;
    WORD          indexCount;
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
  void        Render(UINT type);
  void        GetAaBox(NTempest::CAaBox &aaBox);

  NTempest::CRange height;
  SLVert           verts[81];
  SLTiles          tiles;
  UINT             nFlowvs;
  SWFlowv          flowvs[2];
  CMapChunk       *chunk;
  LINKDECLEX(CChunkLiquid, sceneLink);
  LINKDECLEX(CChunkLiquid, lameAssLink);
};

class CMapBaseObj;

NODEDECL(CChunkTex) {
  CChunkTex();
  CChunkTex(const CChunkTex &);
  ~CChunkTex();

  DWORD pixels[4096];
};

NODEDECL(CChunkLayer) {
  CChunkLayer();
  CChunkLayer(const CChunkLayer &);
  ~CChunkLayer();

  WORD        props;
  WORD        effectId;
  HTEXTURE__ *texId;
  BYTE       *offsAlpha;
  CChunkTex  *tex;
  CGxTex     *gxTexture;
  CMapChunk  *chunk;
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

  CMapBaseObj();
  ~CMapBaseObj();

  virtual void SelectLights();

  int TestAABox(const NTempest::C3Vector &v0, const NTempest::C3Vector &v1);

  UINT GetType() {
    return type;
  }

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
};

class CMapLight : public CMapBaseObj {
 public:
  CMapLight();
  ~CMapLight();

  static void    CreatePointAtten();
  static void    DestroyPointAtten();
  static CGxTex *GetPointAttenTex();

  void SetAtten(float attenStart, float attenEnd);
  void SetConstantAtten(float attenuation);
  void SetLinearAtten(float attenuation);
  void SetQuadraticAtten(float attenuation);
  void Project();

  static LISTDECLEX(CMapBaseObjLink, refLink, dirLightLinkList);
  static UINT  maxLights;
  static float bucketSize;
  static float halfBucketSize;

  CGxLight gxLight;
  float    attenStart;
  float    attenEnd;
  float    attenDenom;
  BYTE     dynamic;

 private:
  static void        ProjectLightRenderPN(CGxBufCommand &cmd, CGxBuf *buf);
  static HTEXTURE__ *s_hPointAttenTex;
};

class CMapStaticEntity : public CMapBaseObj {
 public:
  virtual void SelectLights();
  virtual void QueryLightmap(CMapObjDef *mapObjDef, CMapObjGroup *mapObjGroup) = 0;

  void AdjustLightmap(const NTempest::CImVector &lmColor, NTempest::CImVector &dirColor, BYTE minDir, NTempest::CImVector &ambColor, BYTE maxAmbient);
  int  GetMapObjAndGroup(CMapObjDef *&mapObjDef, CMapObj *&mapObj, CMapObjDefGroup *&mapObjDefGroup, CMapObjGroup *&mapObjGroup);
  int  GetMapObjDef(CMapObjDef *&mapObjDef);
  void FindLights();
  void CreateCacheLight(CMapLight *light);

  LISTDECLEX(CMapCacheLight, lameAssLink, cacheLightList);
  NTempest::CImVector ambient;
  NTempest::CImVector interiorDirColor;
  float               dirLightScale;
  HMODEL__           *model;
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
};

struct CMapEntity : public CMapStaticEntity {
  static const float ambLightScaleRate;
  static const float dirLightScaleRate;

  CMapEntity();
  ~CMapEntity();

  virtual void QueryLightmap(CMapObjDef *mapObjDef, CMapObjGroup *mapObjGroup);
  void         Tick();
  void         QueryLiquidSounds(int *lbool, NTempest::C3Vector *ldelta, float *ldsquared, UINT &closestExtLevel);
  void         UpdateMapObjLiquid();
  int          QueryMapObjZoneName(LPCSTR &zoneName);
  int          QueryMapObjSubzoneName(LPCSTR &subzoneName, UINT &subzoneId);
  int          QueryMapObjFileName(LPCSTR &fileName);
  int          QueryMapObjListenerId(UINT &listenerId);
  int          QueryMapGroundType(UINT &groundType);
  int          QueryMapObjFog(SMOFog::Fogs &oFog, float &oPct);
  static int   QueryCameraFog(SMOFog::Fogs &oFog, float &oPct);
  bool         QueryMapObjMinimap(const NTempest::CAaBox &aaBox, TSStackArray<CWorld::MinimapQuad> &quads);
  bool         QueryMapObjIDs(UINT &wmoID, UINT &instanceID, UINT &groupID);
  bool         QueryMapObjMatrix(NTempest::C44Matrix *mtx, NTempest::C44Matrix *invMtx);
  bool         QueryMapObjAreaTable(const WMOAreaTableRec *&subzoneRec, const WMOAreaTableRec *&globalRec);

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
};

class CMapDoodadDef : public CMapStaticEntity, public TSHashObject<CMapDoodadDef, HASHKEY_DWORD> {
 public:
  CMapDoodadDef();
  ~CMapDoodadDef();

  virtual void SelectLights();
  virtual void QueryLightmap(CMapObjDef *mapObjDef, CMapObjGroup *mapObjGroup);

  void Update(const NTempest::C44Matrix &newMat);
  void GetBounds(NTempest::CAaSphere &bounds);
  void GetBounds(NTempest::CAaBox &bounds);
  void GetCollideExt(NTempest::CAaBox &bounds);

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
};

class CMapObjDef : public CMapBaseObj, public TSHashObject<CMapObjDef, HASHKEY_NONE> {
 public:
  CMapObjDef();
  ~CMapObjDef();

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
};

class CMapObjDefGroup : public CMapBaseObj {
 public:
  CMapObjDefGroup();
  ~CMapObjDefGroup();

  virtual void SelectLights();
  void         UpdateLights();
  void         Update(const NTempest::C44Matrix &newMat);

  UINT                            groupNum;
  DWORD                           doodadRefStart;
  DWORD                           nDoodadRefs;
  NTempest::CImVector             ambient;
  LPCSTR                          subzoneName;
  UINT                            level;
  int                             rDrawSharedLiquidToggle;
  TSExplicitList<CWFrustum, 0xF4> frustumList;
  LISTDECLEX(CMapBaseObjLink, refLink, doodadDefLinkList);
  LISTDECLEX(CMapBaseObjLink, refLink, entityLinkList);
  LISTDECLEX(CMapBaseObjLink, refLink, lightLinkList);
  LINKDECLEX(CMapObjDefGroup, sceneLink);
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

  CMapChunk();
  ~CMapChunk();

  void Load(SMChunkInfo *chunkInfo);
  void Create(BYTE *data);

  static void Initialize();
  static void AsyncPollHandler();
  static void Destroy();
  static void FreeLists();
  static void SetSoundEmitterHandlers(void (*create)(CWSoundEmitter &), void (*destroy)(DWORD));

  virtual void SelectLights();
  void         UpdateLights();
  void         Update();
  void         UpdateClipBuffer();
  void         Render();
  void         CreateDetailDoodads();
  void         Purge();

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
  int                 bLoaded;
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
  void           SyncLoadLayer(CChunkLayer *layer);
  void           SyncLoadShadow();
  void           SyncLoadShader();
  void           FindLights();
  void           CreateVertices(float *heights);
  void           CreateVertices2(float *heights);
  void           CreateNormals(signed char *normals);
  void           CreateFacePlanes();
  void           CreateLayer(CMapArea *area, SMLayer *layer, BYTE *alphaTex);
  void           CreateShadow(BYTE *shadowTex);
  void           CreateAlphaShadow();
  void           CreateRefs(CMapArea *area, UINT *ref, UINT doodadCnt, UINT mapObjCnt);
  void           CreateChunkShadowTex();
  void           CreateChunkLayerTex(CChunkLayer *layer);
  void           CreateChunkShaderTex();
  void           RemapVertices();
  void           RemapVerticesDyn();
  void           PurgeLayer(CChunkLayer *layer);
  static CGxBuf *AllocGxBuf(UINT indexCount);
  static void    FreeGxBuf(CGxBuf *gxBuf);
  static CGxTex *AllocAlphaGxTex(LPVOID userArg, void (*userFunc)(EGxTexCommand, UINT, UINT, UINT, UINT, LPVOID, UINT &, LPCVOID &));
  static void    FreeAlphaGxTex(CGxTex *gxTex);
  static CGxTex *AllocShadowGxTex(LPVOID userArg, void (*userFunc)(EGxTexCommand, UINT, UINT, UINT, UINT, LPVOID, UINT &, LPCVOID &));
  static void    FreeShadowGxTex(CGxTex *gxTex);
  static void    UnpackAlphaShadowBits(NTempest::CImVector *texels, DWORD *bits, const BYTE *const *alpha, const BYTE *shadow);
  static void    UnpackAlphaBits(DWORD *pixels, const BYTE *alphaPixels);
  static void    UnpackShadowBits(DWORD *pixels, DWORD *shadowBits, const BYTE *shadow);
  static void
  UpdateLayerGxTexture(EGxTexCommand cmd, UINT w, UINT h, UINT d, UINT mipLevel, LPVOID userArg, UINT &texelStrideInBytes, LPCVOID &texels);
  static void
  UpdateShadowGxTexture(EGxTexCommand cmd, UINT w, UINT h, UINT d, UINT mipLevel, LPVOID userArg, UINT &texelStrideInBytes, LPCVOID &texels);
  static void
  UpdateShaderGxTexture(EGxTexCommand cmd, UINT w, UINT h, UINT d, UINT mipLevel, LPVOID userArg, UINT &texelStrideInBytes, LPCVOID &texels);
  static void
  UpdateTextureDefault(EGxTexCommand cmd, UINT w, UINT h, UINT d, UINT mipLevel, LPVOID userArg, UINT &texelStrideInBytes, LPCVOID &texels);
  static void  CreateRenderLists();
  static void  GxBufDynFillCallback(CGxBufCommand &cmd, CGxBuf *buf);
  static void  GxBufFillCallback(CGxBufCommand &cmd, CGxBuf *buf);
  static void  LodCreateTree(int level, int maxLevel, int neighborLOD, int holes, int cX, int cY);
  void         FillGxBufVertex(const CGxBufCommand &cmd, CGxBuf *buf);
  void         FillGxBufIndex(const CGxBufCommand &cmd, CGxBuf *buf);
  void         FillGxBufDynVertex(const CGxBufCommand &cmd, CGxBuf *buf);
  void         FillGxBufDynIndex(const CGxBufCommand &cmd, CGxBuf *buf);
  void         RenderLayers();
  void         RenderLayersDyn();
  void         RenderLayersColor();
  void         RenderLayersColorDyn();
  static void  FreeAsyncLoadBuffer(BYTE *buffer);
  static void  InitAsyncLoadBuffers();
  static BYTE *AllocAsyncLoadBuffer();
  static void  AsyncCallback(LPVOID userArg);
  void         SyncLoad(SMChunk *&mChunk, SMLayer *&mLayer, BYTE *&shadowTex, BYTE *&alphaTex);

  static BYTE               syncLoadBuffer[15000];
  static NTempest::C2Vector texCoordList[145];
  static NTempest::C2Vector texCoordList2[145];
  static NTempest::C2Vector rmTexCoordList[4][145];
  static NTempest::C2Vector rmTexCoordList2[4][145];
  static CGxBatch           rmGxBatchList[4][2];
  static const float        TERRAIN_SPEC_EXP;
  static NTempest::C4Vector psLayerMask[4];
  static WORD               primList[768];
  static WORD              *primPtr;
  static void (*soundEmitterCreateHandler)(CWSoundEmitter &);
  static void (*soundEmitterDestroyHandler)(DWORD);
  static CGxBuf                   *gxBufDyn;
  static TSGrowableArray<CGxBuf *> gxBufFreeList;
  static TSGrowableArray<CGxTex *> gxAlphaTexFreeList;
  static TSGrowableArray<CGxTex *> gxShadowTexFreeList;
};

#endif
