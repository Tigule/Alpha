#ifndef WOW_SOURCE_WORLDCLIENT_MAP_H
#define WOW_SOURCE_WORLDCLIENT_MAP_H

#include "MapDefs.h"
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
struct HMODEL__;
struct HTEXTURE__;

namespace NTempest {

  class CRange {
   public:
    float min;
    float max;
  };

}  // namespace NTempest

struct CWorldMinimapQuad {
  unsigned int        groupNum;
  NTempest::C2iVector quad;
  NTempest::CAaBox    aaBox;

  CWorldMinimapQuad();
  ~CWorldMinimapQuad();
};

struct SWVert {
  unsigned int depth : 8;
  unsigned int flow0Pct : 8;
  unsigned int flow1Pct : 8;
  unsigned int filler : 8;
  float        height;
};

struct SOVert {
  unsigned int depth : 8;
  unsigned int foam : 8;
  unsigned int wet : 8;
  unsigned int filler : 8;
};

struct SMVert {
  unsigned short s;
  unsigned short t;
  float          height;
};

struct SLVert {
  union {
    SWVert waterVert;
    SOVert oceanVert;
    SMVert magmaVert;
  };
};

struct SLTiles {
  unsigned char flags[64];
};

class CWSoundEmitter {
 public:
  unsigned long      soundPointID;
  unsigned long      soundNameID;
  NTempest::C3Vector pos;
  float              minDistance;
  float              maxDistance;
  float              cutoffDistance;
  unsigned long      startTime;
  unsigned long      endTime;
  unsigned long      mode;
  unsigned long      groupSilenceMin;
  unsigned long      groupSilenceMax;
  unsigned long      playInstancesMin;
  unsigned long      playInstancesMax;
  unsigned long      loopCountMin;
  unsigned long      loopCountMax;
  unsigned long      interSoundGapMin;
  unsigned long      interSoundGapMax;
};

class CMapSoundEmitter : public CWSoundEmitter {
 public:
  TSLink<CMapSoundEmitter> link;
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
    UserArg(CChunkLiquid *liquid, unsigned int liquidType) : liquid(liquid), liquidType(liquidType), indexCount(0) {
    }

    CChunkLiquid  *liquid;
    unsigned int   liquidType;
    unsigned short indexCount;
  };

  unsigned short         Render0I(unsigned short *idxBase, unsigned int liquidType);
  void                   RenderOcean0();
  void                   RenderOcean0V(CGxVertexPNT0 *vtx);
  static void RenderOcean0Callback(CGxBufCommand &cmd, CGxBuf *gxBuf);
  void                   RenderRiver0(unsigned int type);
  void                   RenderRiver0V(CGxVertexPNT0 *vtx);
  static void RenderRiver0Callback(CGxBufCommand &cmd, CGxBuf *gxBuf);
  void                   RenderMagma0(unsigned int type);
  void                   RenderMagma0V(CGxVertexPCT0 *vtx);
  static void RenderMagma0Callback(CGxBufCommand &cmd, CGxBuf *gxBuf);
  void                   Render(unsigned int type);
  void                   GetAaBox(NTempest::CAaBox &aaBox);

  NTempest::CRange     height;
  SLVert               verts[81];
  SLTiles              tiles;
  unsigned int         nFlowvs;
  SWFlowv              flowvs[2];
  CMapChunk           *chunk;
  TSLink<CChunkLiquid> sceneLink;
  TSLink<CChunkLiquid> lameAssLink;
};

class CMapBaseObj;

struct CChunkTex : public TSLinkedNode<CChunkTex> {
  ~CChunkTex();

  unsigned long pixels[4096];
};

struct CChunkLayer : public TSLinkedNode<CChunkLayer> {
  ~CChunkLayer();

  unsigned short props;
  unsigned short effectId;
  HTEXTURE__    *texId;
  unsigned char *offsAlpha;
  CChunkTex     *tex;
  CGxTex        *gxTexture;
  CMapChunk     *chunk;
};

class CMapCacheLight {
 public:

  CGxLight               gxLight;
  float                  attenStart;
  float                  attenEnd;
  float                  attenDenom;
  TSLink<CMapCacheLight> lameAssLink;
};

class CMapAreaLow {
 public:

  NTempest::CAaBox    aaBox;
  NTempest::CAaSphere aaSphere;
  NTempest::C3Vector  corner;
  NTempest::C2iVector mIndex;
  float               heights[545];
  TSLink<CMapAreaLow> sceneLink;
};

class CMapBaseObjLink {
 public:
  CMapBaseObj            *owner;
  CMapBaseObj            *ref;
  TSLink<CMapBaseObjLink> refLink;
  TSLink<CMapBaseObjLink> ownerLink;
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

  unsigned int GetType() const {
    return type;
  }

  unsigned int                        type;
  TSLink<CMapBaseObj>                 lameAssLink;
  TSExplicitList<CMapBaseObjLink, 16> parentLinkList;
  NTempest::C3Vector                  pos;
  float                               scale;
  NTempest::C4Quaternion              rot;
  NTempest::CAaBox                    aaBox;
  NTempest::CAaSphere                 aaSphere;
  NTempest::C3Vector                  corner;
  float                               camDist;
  unsigned short                      flags;
  short                               refCount;
};

class CMapLight : public CMapBaseObj {
 public:
  CMapLight();
  ~CMapLight();

  static void CreatePointAtten();
  static void DestroyPointAtten();
  static CGxTex *GetPointAttenTex();

  static TSExplicitList<CMapBaseObjLink, 8> dirLightLinkList;
  static unsigned int                       maxLights;
  static float                              bucketSize;
  static float                              halfBucketSize;

  CGxLight     gxLight;
  float        attenStart;
  float        attenEnd;
  float        attenDenom;
  unsigned int dynamic;

 private:
  static HTEXTURE__ *s_hPointAttenTex;
};

class CMapStaticEntity : public CMapBaseObj {
 public:
  virtual void SelectLights();
  virtual void QueryLightmap(CMapObjDef *mapObjDef, CMapObjGroup *mapObjGroup) = 0;

  void AdjustLightmap(
      NTempest::CImVector &lmColor,
      NTempest::CImVector &dirColor,
      unsigned int         minDir,
      NTempest::CImVector &ambColor,
      unsigned int         maxAmbient
  );
  int  GetMapObjAndGroup(CMapObjDef *&mapObjDef, CMapObj *&mapObj, CMapObjDefGroup *&mapObjDefGroup, CMapObjGroup *&mapObjGroup);
  int  GetMapObjDef(CMapObjDef *&mapObjDef);
  void FindLights();
  void CreateCacheLight(CMapLight *light);

  TSExplicitList<CMapCacheLight, 72> cacheLightList;
  NTempest::CImVector                ambient;
  NTempest::CImVector                interiorDirColor;
  float                              dirLightScale;
  HMODEL__                          *model;
  unsigned int                       flagInside : 1;
  unsigned int                       flagVisible : 1;
  unsigned int                       flagCollidable : 1;
  unsigned int                       flagHidden : 1;
  unsigned int                       flagShadowed : 1;
  unsigned int                       flagInLiquid : 1;
  unsigned int                       flagDeepLiquid : 1;
  unsigned int                       flagAlwaysAnimate : 1;
  unsigned int                       flagCastShadow : 1;

  static const float              dirLightScaleAmount;
  static const NTempest::C3Vector interiorSunDir;
};

class CMapEntity : public CMapStaticEntity {
 public:
  static const float ambLightScaleRate;
  static const float dirLightScaleRate;

  CMapEntity();
  ~CMapEntity();

  virtual void QueryLightmap(CMapObjDef *mapObjDef, CMapObjGroup *mapObjGroup);
  void         Tick();
  void         QueryLiquidSounds(int *lbool, NTempest::C3Vector *ldelta, float *ldsquared, unsigned int &closestExtLevel);
  void         UpdateMapObjLiquid();
  int          QueryMapObjZoneName(const char *&zoneName);
  int          QueryMapObjSubzoneName(const char *&subzoneName, unsigned int &subzoneId);
  int          QueryMapObjFileName(const char *&fileName);
  int          QueryMapObjListenerId(unsigned int &listenerId);
  int          QueryMapObjFog(SMOFog::Fogs &oFog, float &oPct);
  static int   QueryCameraFog(SMOFog::Fogs &oFog, float &oPct);
  unsigned int QueryMapObjMinimap(NTempest::CAaBox &aaBox, TSStackArray<CWorldMinimapQuad> &quads);
  unsigned int QueryMapObjIDs(unsigned int &wmoID, unsigned int &instanceID, unsigned int &groupID);
  unsigned int QueryMapObjMatrix(NTempest::C44Matrix *mtx, NTempest::C44Matrix *invMtx);
  bool         QueryMapObjAreaTable(const WMOAreaTableRec *&subzoneRec, const WMOAreaTableRec *&globalRec);

  int (*handler)(void *, unsigned long, unsigned __int64, unsigned long);
  unsigned __int64      param64;
  unsigned long         param32;
  NTempest::C3Vector    oldPos;
  unsigned int          rFrameCount;
  NTempest::C3Vector    lqDirection;
  float                 lqSurface;
  unsigned int          lqWhich;
  NTempest::CImVector   ambientTarget;
  float                 dirLightScaleTarget;
  TSLink<CMapDoodadDef> sceneLink;
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

  NTempest::C44Matrix   lMat;
  NTempest::C44Matrix   mat;
  NTempest::CAaBox      collideExt;
  const char           *modelName;
  unsigned int          rCount;
  unsigned int          cCount;
  int                   doodadSoundHandle;
  TSLink<CMapDoodadDef> sceneLink;
  void(*RenderCB)(void *param, const NTempest::C44Matrix &matrix);
  void *renderCBParam;
};

class CMapObjDef : public CMapBaseObj, public TSHashObject<CMapObjDef, HASHKEY_NONE> {
 public:
  CMapObjDef();
  ~CMapObjDef();

  NTempest::C44Matrix                mat;
  NTempest::C44Matrix                invMat;
  unsigned int                       nameId;
  CMapObj                           *mapObj;
  unsigned short                     tDoodadRefs;
  unsigned short                     firstDoodadRef;
  unsigned int                       doodadSet;
  unsigned short                     nameSet;
  const char                        *zoneName;
  TSExplicitList<CMapBaseObjLink, 8> groupLinkList;
  TSGrowableArray<CMapLight *>       lightList;
  unsigned int                       rCount;
  NTempest::CImVector                ambient;
  unsigned __int64                   param64;
  TSLink<CMapStaticEntity>           sceneLink;
};

class CMapObjDefGroup : public CMapBaseObj {
 public:
  CMapObjDefGroup();
  ~CMapObjDefGroup();

  virtual void SelectLights();
  void         UpdateLights();
  void         Update(NTempest::C44Matrix &newMat);

  unsigned int                       groupNum;
  unsigned int                       doodadRefStart;
  unsigned int                       nDoodadRefs;
  NTempest::CImVector                ambient;
  const char                        *subzoneName;
  unsigned int                       level;
  int                                rDrawSharedLiquidToggle;
  TSExplicitList<CWFrustum, 0xF4>    frustumList;
  TSExplicitList<CMapBaseObjLink, 8> doodadDefLinkList;
  TSExplicitList<CMapBaseObjLink, 8> entityLinkList;
  TSExplicitList<CMapBaseObjLink, 8> lightLinkList;
  TSLink<CMapObjDefGroup>            sceneLink;
};

struct SMAreaHeader {
  unsigned int  offsInfo;
  unsigned int  offsTex;
  unsigned int  sizeTex;
  unsigned int  offsDoo;
  unsigned int  sizeDoo;
  unsigned int  offsMob;
  unsigned int  sizeMob;
  unsigned char pad[36];
};

struct SMDoodadDef {

  unsigned int       nameId;
  unsigned int       uniqueId;
  NTempest::C3Vector pos;
  NTempest::C3Vector rot;
  unsigned short     scale;
  unsigned short     flags;
};

struct SMMapObjDef {

  unsigned int       nameId;
  unsigned int       uniqueId;
  NTempest::C3Vector pos;
  NTempest::C3Vector rot;
  NTempest::CAaBox   extents;
  unsigned short     flags;
  unsigned short     doodadSet;
  unsigned short     nameSet;
  unsigned short     pad;
};

struct SMChunkInfo {
  unsigned int offset;
  unsigned int size;
  unsigned int flags;
  union {
    unsigned char pad[4];
    unsigned int  asyncId;
  };
};

struct SMAreaInfo {
  unsigned int offset;
  unsigned int size;
  unsigned int flags;
  union {
    unsigned char pad[4];
    unsigned int  asyncId;
  };
};

struct SMChunk {
  unsigned long  flags;
  unsigned long  indexX;
  unsigned long  indexY;
  float          radius;
  unsigned long  nLayers;
  unsigned long  nDoodadRefs;
  unsigned long  offsHeight;
  unsigned long  offsNormal;
  unsigned long  offsLayer;
  unsigned long  offsRefs;
  unsigned long  offsAlpha;
  unsigned long  sizeAlpha;
  unsigned long  offsShadow;
  unsigned long  sizeShadow;
  unsigned long  areaid;
  unsigned long  nMapObjRefs;
  unsigned short holes;
  unsigned short pad0;
  unsigned short predTex[8];
  unsigned char  noEffectDoodad[8];
  unsigned long  offsSndEmitters;
  unsigned long  nSndEmitters;
  unsigned long  offsLiquid;
  unsigned char  pad1[24];
};

struct SMLayer {
  unsigned long  textureId;
  unsigned long  props;
  unsigned long  offsAlpha;
  unsigned short effectId;
  unsigned char  pad[2];
};

struct SMNormal {
  char n[145][3];
  char pad[13];
};

struct SMMapHeader {
  unsigned long nDoodadNames;
  unsigned long offsDoodadNames;
  unsigned long nMapObjNames;
  unsigned long offsMapObjNames;
  unsigned char pad[112];
};

class CMapChunk : public CMapBaseObj {
 public:
  static unsigned int cornerVertexIndex[4];
  static unsigned int farCornerIndex;

  CMapChunk();
  ~CMapChunk();

  void Load(SMChunkInfo *chunkInfo);
  void Create(unsigned int *data);

  static void Initialize();
  static void AsyncPollHandler();
  static void Destroy();
  static void FreeLists();

  virtual void SelectLights();
  void         UpdateLights();
  void         Update();
  void         UpdateClipBuffer();
  void         SyncLoadLayer(CChunkLayer *layer);
  void         SyncLoadShadow();
  void         SyncLoadShader();
  void         FindLights();
  void         CreateVertices(float *heights);
  void         CreateNormals(int *normals);
  void         CreateFacePlanes();
  void         CreateLayer(CMapArea *area, SMLayer *layer, unsigned int *alphaTex);
  void         CreateShadow(unsigned int *shadowTex);
  void         CreateAlphaShadow();
  void         CreateRefs(CMapArea *area, unsigned int *ref, unsigned int doodadCnt, unsigned int mapObjCnt);
  void         CreateChunkShadowTex();
  void         CreateChunkLayerTex(CChunkLayer *layer);
  void         CreateChunkShaderTex();
  void         Render();
  void         CreateDetailDoodads();
  void         Purge();

  unsigned int                         infoIndex;
  unsigned short                       holes;
  unsigned short                       pad;
  unsigned int                         lod;
  unsigned int                         remapLod;
  CDetailDoodadInst                   *detailDoodadInst;
  CMapChunk                           *neighbor[4];
  TSLink<CMapChunk>                    sceneLink;
  TSExplicitList<CMapBaseObjLink, 8>   doodadDefLinkList;
  TSExplicitList<CMapBaseObjLink, 8>   mapObjDefLinkList;
  TSExplicitList<CMapBaseObjLink, 8>   entityLinkList;
  TSExplicitList<CMapBaseObjLink, 8>   lightLinkList;
  TSExplicitList<CMapSoundEmitter, 76> soundEmitterList;
  CChunkLiquid                        *liquids[4];
  NTempest::C2iVector                  aIndex;
  NTempest::C2iVector                  sOffset;
  NTempest::C2iVector                  cOffset;
  float                                freeTime;
  int                                  bLoaded;
  CChunkLayer                         *layerList[4];
  unsigned int                         nLayers;
  CChunkTex                           *shadowTexture;
  CGxTex                              *shadowGxTexture;
  unsigned int                        *shadowOffs;
  unsigned long                        shadowSize;
  CGxBuf                              *gxBuf;
  CChunkTex                           *shaderTexture;
  CGxTex                              *shaderGxTexture;
  CAsyncObject                        *asyncObject;
  unsigned int                         fileOffset;
  unsigned int                         fileSize;
  NTempest::CRndSeed                   rSeed;
  unsigned int                         zoneId;
  unsigned short                       predTex[8];
  unsigned char                        noEffectDoodad[8];
  NTempest::C3Vector                   normalList[145];
  NTempest::C3Vector                   vertexList[145];
  NTempest::C4Plane                    planeList[256];
  unsigned long                        shadowBits[32];

 private:
  void                      PurgeLayer(CChunkLayer *layer);
  static CGxBuf *AllocGxBuf(unsigned int indexCount);
  static void FreeGxBuf(CGxBuf *gxBuf);
  static CGxTex *AllocAlphaGxTex(
      void *userArg,
      void(*userFunc)(EGxTexCommand, unsigned int, unsigned int, unsigned int, unsigned int, void *, unsigned int &, const void *&)
  );
  static void FreeAlphaGxTex(CGxTex *gxTex);
  static CGxTex *AllocShadowGxTex(
      void *userArg,
      void(*userFunc)(EGxTexCommand, unsigned int, unsigned int, unsigned int, unsigned int, void *, unsigned int &, const void *&)
  );
  static void FreeShadowGxTex(CGxTex *gxTex);
  static void
  UnpackAlphaShadowBits(NTempest::CImVector *texels, unsigned long *bits, const unsigned int *const *alpha, const unsigned int *shadow);
  static void UnpackAlphaBits(unsigned long *pixels, const unsigned int *alphaPixels);
  static void UnpackShadowBits(unsigned long *pixels, unsigned long *shadowBits, const unsigned int *shadow);
  static void UpdateLayerGxTexture(
      EGxTexCommand cmd,
      unsigned int  w,
      unsigned int  h,
      unsigned int  d,
      unsigned int  mipLevel,
      void         *userArg,
      unsigned int &texelStrideInBytes,
      const void  *&texels
  );
  static void UpdateShadowGxTexture(
      EGxTexCommand cmd,
      unsigned int  w,
      unsigned int  h,
      unsigned int  d,
      unsigned int  mipLevel,
      void         *userArg,
      unsigned int &texelStrideInBytes,
      const void  *&texels
  );
  static void UpdateShaderGxTexture(
      EGxTexCommand cmd,
      unsigned int  w,
      unsigned int  h,
      unsigned int  d,
      unsigned int  mipLevel,
      void         *userArg,
      unsigned int &texelStrideInBytes,
      const void  *&texels
  );
  static void UpdateTextureDefault(
      EGxTexCommand cmd,
      unsigned int  w,
      unsigned int  h,
      unsigned int  d,
      unsigned int  mipLevel,
      void         *userArg,
      unsigned int &texelStrideInBytes,
      const void  *&texels
  );
  static void CreateRenderLists();
  static void GxBufDynFillCallback(CGxBufCommand &cmd, CGxBuf *buf);
  static void GxBufFillCallback(CGxBufCommand &cmd, CGxBuf *buf);
  static void LodCreateTree(int level, int maxLevel, int neighborLOD, int holes, int cX, int cY);
  void                            FillGxBufVertex(const CGxBufCommand &cmd, CGxBuf *buf);
  void                            FillGxBufIndex(const CGxBufCommand &cmd, CGxBuf *buf);
  void                            FillGxBufDynVertex(const CGxBufCommand &cmd, CGxBuf *buf);
  void                            FillGxBufDynIndex(const CGxBufCommand &cmd, CGxBuf *buf);
  void                            RenderLayers();
  void                            RenderLayersDyn();
  void                            RenderLayersColor();
  void                            RenderLayersColorDyn();
  static void FreeAsyncLoadBuffer(unsigned int *buffer);
  static void InitAsyncLoadBuffers();
  static unsigned int *AllocAsyncLoadBuffer();
  static void AsyncCallback(void *userArg);
  void                            SyncLoad(SMChunk *&mChunk, SMLayer *&mLayer, unsigned int *&shadowTex, unsigned int *&alphaTex);

  static unsigned int       syncLoadBuffer[15000];
  static NTempest::C2Vector texCoordList[145];
  static NTempest::C2Vector texCoordList2[145];
  static NTempest::C2Vector rmTexCoordList[4][145];
  static NTempest::C2Vector rmTexCoordList2[4][145];
  static CGxBatch           rmGxBatchList[4][2];
  static const float        TERRAIN_SPEC_EXP;
  static NTempest::C4Vector psLayerMask[4];
  static unsigned short     primList[768];
  static unsigned short    *primPtr;
  static void(*soundEmitterCreateHandler)(CWSoundEmitter &);
  static void(*soundEmitterDestroyHandler)(unsigned long);
  static CGxBuf                   *gxBufDyn;
  static TSGrowableArray<CGxBuf *> gxBufFreeList;
  static TSGrowableArray<CGxTex *> gxAlphaTexFreeList;
  static TSGrowableArray<CGxTex *> gxShadowTexFreeList;
};

#endif
