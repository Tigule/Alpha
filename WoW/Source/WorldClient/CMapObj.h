#ifndef WOW_SOURCE_WORLDCLIENT_CMAPOBJ_H
#define WOW_SOURCE_WORLDCLIENT_CMAPOBJ_H

#include "AaBsp.h"
#include "Gx/Gx.h"
#include "MapDefs.h"
#include "WorldClient/World.h"

#include "Tempest/c2ivector.h"
#include "Tempest/c2vector.h"
#include "Tempest/c33matrix.h"
#include "Tempest/c3segment.h"
#include "Tempest/c3vector.h"
#include "Tempest/c44matrix.h"
#include "Tempest/c4plane.h"
#include "Tempest/c4quaternion.h"
#include "Tempest/caabox.h"
#include "Tempest/crect.h"
#include "Tempest/cimvector.h"

#include <stpl.h>

class CAsyncObject;
class BspQuery;
class CGxTex;
class CWTriData;
class CMapObj;
class CMapObjDef;
class CMapObjGroup;
class SFile;
namespace NTempest {
  class CAaSphere;
}
struct CGxBuf;
struct CGxBufCommand;
struct HTEXTURE__;
struct SMOBatch {
  enum {
    F_RENDERED = 0xF0
  };

  unsigned char  lightMap;
  unsigned char  texture;
  short          bx;
  short          by;
  short          bz;
  short          tx;
  short          ty;
  short          tz;
  unsigned short startIndex;
  unsigned short count;
  unsigned short minIndex;
  unsigned short maxIndex;
  unsigned char  flags;
  unsigned char  pad[1];
};
struct SMODoodadDef {
  unsigned long          nameIndex;
  NTempest::C3Vector     pos;
  NTempest::C4Quaternion rot;
  float                  scale;
  NTempest::CImVector    color;
};
struct SMODoodadSet {
  char          name[20];
  unsigned long startIndex;
  unsigned long count;
  unsigned char pad[4];
};
struct SMOHeader {
  unsigned long       nTextures;
  unsigned long       nGroups;
  unsigned long       nPortals;
  unsigned long       nLights;
  unsigned long       nDoodadNames;
  unsigned long       nDoodadDefs;
  unsigned long       nDoodadSets;
  NTempest::CImVector ambColor;
  unsigned long       wmoID;
  unsigned char       pad[28];
};
struct SMOLight {
  enum LightType {
    OMNI_LGT = 0,
    SPOT_LGT = 1,
    DIRECT_LGT = 2,
    AMBIENT_LGT = 3
  };

  unsigned char       type;
  unsigned char       useAtten;
  unsigned char       pad[2];
  NTempest::CImVector color;
  NTempest::C3Vector  position;
  float               intensity;
  float               attenStart;
  float               attenEnd;
};
struct SMOLightmap;
#define LIQUID_NONE 15

struct SMOLTile {
  enum {
    SHARED_MASK = 0x80,
    SHARED_SHIFT = 7,
    FISHABLE_MASK = 0x40,
    FISHABLE_SHIFT = 6
  };

  unsigned int GetLiquid() const {
    return liquid & 0xF;
  }

  int GetShared() const {
    return (liquid & SHARED_MASK) >> SHARED_SHIFT;
  }

  __forceinline int GetFishable() const {
    return (liquid & FISHABLE_MASK) >> FISHABLE_SHIFT;
  }
  void SetLiquid(unsigned int);
  void SetShared(int);
  void SetFishable(int);

  int IsLiquid() const {
    return GetLiquid() != LIQUID_NONE;
  }

 private:
  unsigned char liquid;
};

struct SMOWVert {
  unsigned char flow1;
  unsigned char flow2;
  unsigned char flow1Pct;
  unsigned char filler;
  float         height;
};

struct SMOMVert {
  short s;
  short t;
  float height;
};

struct SMOLVert {
  union {
    SMOWVert waterVert;
    SMOMVert magmaVert;
  };
};
struct SMOPoly;
struct SMOPortal {
  unsigned short    startVertex;
  unsigned short    count;
  NTempest::C4Plane plane;
};

struct SMOPortalRef {
  unsigned short portalIndex;
  unsigned short groupIndex;
  short          side;
  unsigned short filler;
};

struct SIffChunk {
  SIffChunk() {
  }

  SIffChunk(unsigned long token, unsigned long size) : token(token), size(size) {
  }

  unsigned long token;
  unsigned long size;
};

struct CMapObjHeader {
  SIffChunk    iffChunkVersion;
  unsigned long version;
  SIffChunk    iffChunkHeader;
};

struct SMOGxBatch {
  unsigned short vertStart;
  unsigned short vertCount;
  unsigned short batchStart;
  unsigned short batchCount;
};

struct SMOGroupHeader {
  SIffChunk        iffChunk;
  unsigned int     nameOffset;
  unsigned int     descriptiveNameOffset;
  unsigned int     flags;
  NTempest::CAaBox aaBox;
  unsigned int     pad0;
  unsigned short   portalStart;
  unsigned short   portalCount;
  unsigned char    fogIds[4];
  unsigned int     groupLiquid;
  SMOGxBatch       intBatch[4];
  SMOGxBatch       extBatch[4];
  int              uniqueID;
  unsigned int     pad1[2];
};

struct SMOGroupInfo {
  unsigned long    offset;
  unsigned long    size;
  unsigned long    flags;
  NTempest::CAaBox aaBox;
  unsigned long    nameIndex;
};

struct SPortalExt {
  enum {
    F_SCREEN_CULLED = 1,
    F_INTERSECT_NEAR = 2
  };

  unsigned short  flags;
  unsigned short  rLevel;
  NTempest::CRect sRect;
  unsigned int    xformTag;
  unsigned int    visitedTag;
};

struct SMOMaterial {
  enum {
    MAPID_DIFFUSE = 0,
    MAPID_ENV = 1,
    MAPID_COUNT = 2
  };

  enum {
    F_UNLIT = 1,
    F_UNFOGGED = 2,
    F_UNCULLED = 4,
    F_EXTLIGHT = 8,
    F_SIDN = 16,
    F_WINDOW = 32,
    F_CLAMP_S = 64,
    F_CLAMP_T = 128
  };

  unsigned long       version;
  unsigned long       flags;
  unsigned long       blendMode;
  unsigned long       diffuseNameIndex;
  NTempest::CImVector sidnColor;
  NTempest::CImVector frameSidnColor;
  unsigned long       envNameIndex;
  NTempest::CImVector diffColor;
  unsigned long       groundType;
  union {
    unsigned char inMemPad[8];
    HTEXTURE__   *hMaps[2];
  };
};

struct SMOLightmapTex {
  unsigned char texels[0x8000];
  union {
    unsigned char inMemPad[4];
    CGxTex       *gxTexture;
    HTEXTURE__   *hTexture;
  };
};

class CMapObjGroup {
 public:
  CMapObjGroup();
  ~CMapObjGroup();
  void         Init();
  void         InitPtrs();
  void         Clear();
  bool IsLoaded() {
    return bLoaded != 0;
  }
  bool IsLoading() {
    return asyncObject != 0;
  }
  void SetFlushTime(float time) {
    flushTime = time;
  }
  unsigned int GetFlags() {
    return flags;
  }
  unsigned int GetGroupLiquid() const {
    return groupLiquid;
  }
  unsigned int GetDoodadRefCount() {
    return doodadRefCount;
  }
  unsigned int GetDoodadRef(unsigned int index) {
    return doodadRefList[index];
  }
  unsigned int GetLightRefCount() {
    return lightRefCount;
  }
  unsigned int GetLightRef(unsigned int index) {
    return lightRefList[index];
  }
  long GetUniqueID() {
    return uniqueID;
  }
  unsigned char GetFogId(unsigned int index) {
    return fogIds[index];
  }
  SMOPoly *GetPoly(unsigned short index) {
    return &polyList[index];
  }
  bool         QueryLightmap(const NTempest::C3Vector &point, unsigned short polyIdx, NTempest::CImVector &color);
  bool         QueryLightmap(const NTempest::C3Segment &seg, NTempest::CImVector &color);
  bool QueryLiquidStatus(const NTempest::C3Vector &pos, unsigned int &liquid, float &surface, NTempest::C3Vector &dir);
  bool         QueryLiquidFishable(const NTempest::C3Vector &pos, int &fishable);
  void         QueryLiquidSounds(const NTempest::C3Vector &pos, int *lbool, NTempest::C3Vector *ldelta, float *ldsquared);
  bool         QueryMtlId(const NTempest::C3Segment &seg, unsigned int &mtlId);
  bool GetTris(CWTriData &triData, const NTempest::C3Segment &seg, float &maxT, const CMapObjDef *mapObjDef, unsigned int queryFlags);
  bool GetTris(CWTriData &triData, const NTempest::CAaBox &aaBox, const CMapObjDef *mapObjDef, unsigned int queryFlags);
  bool GetTris(CWTriData &triData, const CWFrustum &frustum, const CMapObjDef *mapObjDef, unsigned int queryFlags);

 private:
  friend class CMap;
  friend class CMapObj;

  unsigned int flags;
  NTempest::CAaBox     aaBox;
  unsigned int         portalStart;
  unsigned int         portalCount;
  unsigned char        fogIds[4];
  unsigned int         groupLiquid;
  SMOGxBatch           intBatch[4];
  SMOGxBatch           extBatch[4];
  CGxBuf              *intGxBuf[4];
  CGxBuf              *extGxBuf[4];
  CAaBsp               aaBsp;
  unsigned int         frameCount;
  unsigned int         rLevel;
  unsigned int         minimapTag;
  float                lightmapTexFlushTime;
  char                *dbgName;
  NTempest::C4Plane   *planeList;
  SMOPoly             *polyList;
  NTempest::C3Vector  *vertexList;
  NTempest::C3Vector  *normalList;
  NTempest::C2Vector  *textureVertexList;
  unsigned short      *indexList;
  SMOBatch            *batchList;
  unsigned short      *lightRefList;
  unsigned short      *doodadRefList;
  NTempest::CImVector *colorVertexList;
  NTempest::C2Vector  *lightmapVertexList;
  SMOLightmap         *lightmapList;
  SMOLightmapTex      *lightmapTexList;
  NTempest::C2iVector  liquidVerts;
  NTempest::C2iVector  liquidTiles;
  NTempest::C3Vector   liquidCorner;
  unsigned short       liquidMtlId;
  SMOLVert            *liquidVertexList;
  SMOLTile            *liquidTileList;
  unsigned int         planeCount;
  unsigned int         polyCount;
  unsigned int         vertexCount;
  unsigned int         normalCount;
  unsigned int         textureVertexCount;
  unsigned int         indexCount;
  unsigned int         batchCount;
  unsigned int         lightRefCount;
  unsigned int         doodadRefCount;
  unsigned int         colorVertexCount;
  unsigned int         lightmapVertexCount;
  unsigned int         lightmapCount;
  unsigned int         lightmapTexCount;
  long                 uniqueID;
  unsigned char       *data;
  CMapObj             *parent;
  float                flushTime;
  CAsyncObject        *asyncObject;
  unsigned char        bLoaded;

 public:
  LINKDECLEX(CMapObjGroup, lameAssLink);

 private:
  void CreateLightmapPointers(unsigned char *&pData);
  void CreateDataPointers(unsigned char *pData);
  void CreateOptionalDataPointers(unsigned char *pData);
  void Create(unsigned char *rawData);
  unsigned int SphereIntersectPoly(
      const NTempest::CAaSphere &sphere,
      const unsigned int         numVerts,
      const unsigned short      *indicies
  );
  bool PointInPoly(
      const NTempest::C3Vector *p,
      const unsigned int        numIndicies,
      const unsigned short     *indicies,
      const NTempest::C3Vector *n
  );
  void QueryMinimap(
      unsigned int                         groupID,
      const NTempest::CAaBox              &localBox,
      TSStackArray<CWorld::MinimapQuad>   &quads
  );
  void FreeData();
  void GenTexture(SMOLightmap *lightmap, const NTempest::CImVector *source, NTempest::CImVector *texture);

  static void UpdateLightmapTex(
      EGxTexCommand cmd,
      unsigned int  w,
      unsigned int  h,
      unsigned int  d,
      unsigned int  mipLevel,
      void         *userArg,
      unsigned int &texelStrideInBytes,
      const void  *&texels
  );
  static void AsyncPostloadCallback(void *userArg);
  static CGxBuf *AllocExtGxBuf(unsigned int nVerts, unsigned int nIndices);
  static void ExtGxBufFill(CGxBufCommand &cmd, CGxBuf *buf);
  static CGxBuf *AllocIntGxBuf(unsigned int nVerts, unsigned int nIndices);
  static void IntGxBufFill(CGxBufCommand &cmd, CGxBuf *buf);
  static void Destroy();
  static void FreeExtGxBuf(CGxBuf *&gxBuf);
  static void FreeIntGxBuf(CGxBuf *&gxBuf);
  void                      FreeLightmaps();
  void                      CreateLightmaps();
  void                      ExtGxBufFillVertex(CGxBufCommand &cmd, CGxBuf *buf);
  void                      IntGxBufFillVertex(CGxBufCommand &cmd, CGxBuf *buf);
  void                      GxBufFillIndex(CGxBufCommand &cmd, CGxBuf *buf);
  void                      GetTrisFromQuery(CWTriData &triData, BspQuery &q, const CMapObjDef *mapObjDef);

  static TSCArray<CGxBuf *, 512> extGxBufFreeList;
  static TSCArray<CGxBuf *, 512> intGxBufFreeList;
  static const EGxTexFormat      LIGHTMAP_FORMAT;
  static const SMOGxBatch       *sLockGxBatch;
  static unsigned int            rDrawSharedLiquidFirst;
  static unsigned int            rDrawSharedLiquidToggle;
};

class CMapObj : public TSHashObject<CMapObj, HASHKEY_NONE> {
 public:
  static void Initialize();
  static void Destroy();
  static void ClearCache(int force);
  static void Delete(CMapObj *mapObj);
  static CMapObj *Create(const char *fileName);
  static void SetGroupRenderCallback(void(*func)(const unsigned int, const void *, const int), void *userParam);

  CMapObj();
  ~CMapObj();
  void Init();
  void InitPtrs();
  void Clear();
  void ReadGroup(unsigned int index);

  unsigned int GetId() {
    return header->wmoID;
  }
  NTempest::CImVector GetAmbientColor() {
    return ambColor;
  }
  const SMOMaterial *GetMaterial(unsigned int index) {
    return &materialList[index];
  }
  bool IsLoaded() {
    return bLoaded;
  }
  bool IsLoading() {
    return asyncObject != 0;
  }
  const char *GetFileName() {
    return name;
  }
  unsigned int GetNumGroups() {
    return groupCount;
  }
  SMODoodadDef *GetDoodadDef(unsigned int index) {
    return &doodadDefList[index];
  }
  const char *GetDoodadName(unsigned int index) {
    return &doodadNameList[index];
  }
  unsigned int GetLightCount() {
    return lightCount;
  }
  SMOLight *GetLight(unsigned int index) {
    return &lightList[index];
  }
  bool         IsGroupLoaded(unsigned int index);
  bool         IsGroupLoading(unsigned int index);
  void         SetFlushTime(float time) {
    flushTime = time;
  }
  void         WaitLoad();
  void         WaitLoadGroup(unsigned int index);
  unsigned int GetWmoID() {
    return header->wmoID;
  }
  CMapObjGroup *GetGroup(unsigned int index, int force = 0);
  const SMOGroupInfo *GetGroupInfo(unsigned int index);
  char          *GetGroupName(unsigned int index);
  void          GetBounds(NTempest::CAaBox &aaBox);
  void          GetBounds(NTempest::CAaSphere &aaSphere);
  void          GetGroupBounds(NTempest::CAaBox &aaBox, unsigned int index);
  void          GetGroupBounds(NTempest::CAaSphere &aaSphere, unsigned int index);
  unsigned int  GetGroupFlags(unsigned int index);
  unsigned int  GetDoodadSet(unsigned int doodadIndex);
  const SMOFog &GetFog(unsigned int index) {
    FATALASSERT(index < fogCount);
    return fogList[index];
  }
  unsigned int GetFogCount() {
    return fogCount;
  }
  NTempest::C3Vector *GetMin() {
    return &aaBox.b;
  }
  NTempest::C3Vector *GetMax() {
    return &aaBox.t;
  }
  NTempest::CAaBox &GetAaBox() {
    return aaBox;
  }
  bool TestBounds(const NTempest::C3Vector &point);
  bool TestBounds(const NTempest::C3Vector &v0, const NTempest::C3Vector &v1);
  bool TestBounds(const NTempest::CAaBox &box);
  bool TestConvexVolume(const NTempest::C3Vector &point);
  bool VectorIntersect(
      CMapObjDef                  *mapObjDef,
      const NTempest::C3Vector    *v0,
      const NTempest::C3Vector    *v1,
      unsigned int                 queryFlags,
      unsigned int                 polyIgnoreFlags,
      unsigned int                 groupIgnoreFlags,
      float                       *dist,
      SMOPoly                    **poly
  );
  bool VectorIntersectPortals(const NTempest::C3Segment &seg, float &maxT, unsigned int *groupIDs);
  bool VectorIntersectPortal(
      const NTempest::C3Vector &v0, const NTempest::C3Vector &v1, unsigned int fromGroup, unsigned int &toGroup
  );
  bool TestGroupBounds(const NTempest::C3Vector &v0, const NTempest::C3Vector &v1, unsigned int index);
  bool TestGroupBounds(const NTempest::C3Vector &point, const unsigned int index);
  bool TestGroupBounds(const NTempest::CAaBox &box, const unsigned int index);
  bool GetTris(CWTriData &triData, const NTempest::CAaBox &aaBox, const CMapObjDef *mapObjDef, unsigned int queryFlags);
  bool GetTris(CWTriData &triData, const NTempest::C3Segment &seg, float &maxT, const CMapObjDef *mapObjDef, unsigned int queryFlags);
  bool GetTris(CWTriData &triData, const CWFrustum &frustum, const CMapObjDef *mapObjDef, unsigned int queryFlags);
  bool QueryLightmap(const NTempest::C3Segment &seg, NTempest::CImVector &color, float *t);
  bool
  QueryLiquidStatus(unsigned int ignoreGroupFlags, const NTempest::C3Vector &pos, unsigned int &liquid, float &surface, NTempest::C3Vector &dir);
  bool QueryLiquidFishable(unsigned int ignoreGroupFlags, const NTempest::C3Vector &pos, int &fishable);
  void QueryLiquidSounds(
      unsigned int              groupIdx,
      unsigned int              parentIdx,
      unsigned int              rlevel,
      unsigned int             &closestExtLevel,
      const NTempest::C3Vector &pos,
      int                      *lbool,
      NTempest::C3Vector       *ldelta,
      float                    *ldsquared
  );
  bool QueryMapObjMinimap(unsigned int groupID, const NTempest::CAaBox &localBox, TSStackArray<CWorld::MinimapQuad> &quads);

  static void PrepareUpdate();
  void                   LocateViewer(NTempest::C44Matrix &im, TSGrowableArray<unsigned int> &inGroups);
  unsigned int           StabPortals(
      unsigned int fromGroupIndex, unsigned int groupIndex, NTempest::C3Vector &rayOrig, NTempest::C3Vector &rayDir
  );
  unsigned int           StabPortals(
      unsigned int groupIndex, const NTempest::C3Vector &start, const NTempest::C3Vector &end
  );
  void                   IntRender(NTempest::C44Matrix &mat, TSGrowableArray<unsigned int> &inGroups);
  void                   ExtRender(NTempest::C44Matrix &mat, const NTempest::CRect &rect);
  void RenderGroup(
      unsigned int groupNum,
      int rDrawSharedLiquidToggle,
      const NTempest::C44Matrix &invMat,
      const LISTEX(CWFrustum, sceneLink) &frustumList
  );

  static TSCArray<NTempest::CRect, 16> extViewList;
  static TSCArray<SPortalExt, 2048>    portalExtList;
  static unsigned int                  maxRLevel;
  static unsigned int                  DEFAULT_RLEVEL;
  static unsigned int                  MAX_SOUND_RLEVEL;
  static NTempest::C3Vector            localCamPos;
  static CMapObjDef                   *curMapObjDef;
  static int                           bIntRender;
  static unsigned int                  sMinimapTag;

 private:
  friend class CMap;
  friend class CMapObjGroup;
  friend class CMapEntity;

  static void AsyncPostloadCallbackHeader(void *userArg);
  static void AsyncPostloadCallback(void *userArg);
  static void AsyncPostloadCallbackAll(void *userArg);
  int                    Read(const char *fileName);
  void                   CreateData();
  void                   AllocGroups();
  void                   CreateAllGroups();
  void                   ReadExtGroups();
  SIffChunk             *ReadChunkHeader(unsigned char *&pData, unsigned long expectedToken);
  SIffChunk             *ReadOptionalChunkHeader(unsigned char *&pData, unsigned long expectedToken);
  void                   CreateDataPointers();
  void                   CreateMaterials();
  void                   CreateMaterial(unsigned int materialId);
  void                   CreateGroup(CMapObjGroup *group, SMOGroupInfo *groupInfo);
  void                   ReadGroup(CMapObjGroup *group, SMOGroupInfo *groupInfo, int preLoad);
  void                   UpdateMaterials();
  void                   RenderAlways(unsigned int groupIdx);
  void                   RRenderThruPortals(unsigned int groupIdx, unsigned int parentIdx, NTempest::CRect &viewRect, unsigned int level);
  void                   RTransformPortal(SMOPortal *portal, SPortalExt *portalExt, int cpIgnore);
  bool                   CullBatch(const SMOBatch *batch);
  void                   RenderGroupLightTex(const CMapObjGroup *group, unsigned int frustumCount);
  void                   RenderGroupLightmapTex_Int(const CMapObjGroup *group, unsigned int frustumCount);
  void                   RenderGroupLightmapTex_Ext(const CMapObjGroup *group, unsigned int frustumCount);
  void                   RenderGroupLightmapTex(const CMapObjGroup *group, unsigned int frustumCount);
  void                   RenderGroupColorTex_Int(const CMapObjGroup *group, unsigned int frustumCount);
  void                   RenderGroupColorTex_Ext(const CMapObjGroup *group, unsigned int frustumCount);
  void                   RenderGroupColorTex(const CMapObjGroup *group, unsigned int frustumCount);
  void                   RenderGroupLightmap(const CMapObjGroup *group, unsigned int frustumCount);
  void                   RenderGroupTex(const CMapObjGroup *group, unsigned int frustumCount);
  void                   RenderGroup_Ext(const CMapObjGroup *group, unsigned int frustumCount);
  void                   RenderGroup_Int(const CMapObjGroup *group, unsigned int frustumCount);
  void                   RenderPortals(CMapObjGroup *group);
  void                   RenderPortals();
  void                   RenderGroupBsp(const CMapObjGroup *group, unsigned int frustumCount);
  void                   RenderGroupNormals(const CMapObjGroup *group);
  void                   RenderWaterIndices_0(const CMapObjGroup *group, unsigned short *idxBase, unsigned int vtxSub, unsigned int &idxSub);
  void                   RenderLiquid_0(const CMapObjGroup *group);
  void                   RenderInteriorWater_0(const CMapObjGroup *group, unsigned int liquid);
  void                   RenderExteriorWater_0(const CMapObjGroup *group, unsigned int liquid);
  void                   RenderMagma(const CMapObjGroup *group, unsigned int liquid);
  void QueryMapObjMinimapGroup(
      unsigned int groupID, unsigned int parentID, const NTempest::CAaBox &localBox, TSStackArray<CWorld::MinimapQuad> &quads
  );

  char                name[260];
  SMOHeader          *header;
  char               *textureNameList;
  char               *groupNameList;
  SMOGroupInfo       *groupInfoList;
  NTempest::C3Vector *portalVertexList;
  SMOPortal          *portalList;
  SMOPortalRef       *portalRefList;
  SMOLight           *lightList;
  SMODoodadSet       *doodadSetList;
  char               *doodadNameList;
  SMODoodadDef       *doodadDefList;
  SMOFog             *fogList;
  NTempest::C4Plane  *convexVolumePlanes;
  unsigned int        textureNameCount;
  unsigned int        groupNameCount;
  unsigned int        groupCount;
  unsigned int        portalVertexCount;
  unsigned int        portalCount;
  unsigned int        portalRefCount;
  unsigned int        lightCount;
  unsigned int        doodadSetCount;
  unsigned int        doodadNameCount;
  unsigned int        doodadDefCount;
  unsigned int        fogCount;
  unsigned int        volumePlaneCount;
  NTempest::CImVector ambColor;
  int                 version;
  NTempest::CAaBox    aaBox;
  SFile              *file;

 public:
  LINKDECLEX(CMapObj, lameAssLink);

 private:
  CMapObjHeader                       fileHeader;
  unsigned char                      *data;
  unsigned long                       dataBytes;
  int                                 refCount;
  float                               flushTime;
  CAsyncObject                       *asyncObject;
  unsigned char                       bLoaded;
  SMOMaterial                        *materialList;
  unsigned int                        materialCount;
  unsigned int                        nGroupsRead;
  LISTDECLEX(CMapObjGroup, lameAssLink, groupList);
  TSCArray<CMapObjGroup *, 384>       groupPtrList;

  static unsigned int gRenderCount;
  static void(*gRenderCallback)(const unsigned int, const void *, const int);
  static void                              *gRenderUserParam;
  static TSHashTable<CMapObj, HASHKEY_NONE> mapObjHash;
  static HASHKEY_NONE                       nullHashKey;
};

#endif
