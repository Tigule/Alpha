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

  BYTE  lightMap;
  BYTE  texture;
  short bx;
  short by;
  short bz;
  short tx;
  short ty;
  short tz;
  WORD  startIndex;
  WORD  count;
  WORD  minIndex;
  WORD  maxIndex;
  BYTE  flags;
  BYTE  pad[1];
};
struct SMODoodadDef {
  DWORD                  nameIndex;
  NTempest::C3Vector     pos;
  NTempest::C4Quaternion rot;
  float                  scale;
  NTempest::CImVector    color;
};
struct SMODoodadSet {
  char  name[20];
  DWORD startIndex;
  DWORD count;
  BYTE  pad[4];
};
struct SMOHeader {
  DWORD               nTextures;
  DWORD               nGroups;
  DWORD               nPortals;
  DWORD               nLights;
  DWORD               nDoodadNames;
  DWORD               nDoodadDefs;
  DWORD               nDoodadSets;
  NTempest::CImVector ambColor;
  DWORD               wmoID;
  BYTE                pad[28];
};
struct SMOLight {
  enum LightType {
    OMNI_LGT = 0,
    SPOT_LGT = 1,
    DIRECT_LGT = 2,
    AMBIENT_LGT = 3
  };

  BYTE                type;
  BYTE                useAtten;
  BYTE                pad[2];
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

  UINT GetLiquid() const {
    return liquid & 0xF;
  }

  int GetShared() const {
    return (liquid & SHARED_MASK) >> SHARED_SHIFT;
  }

  __forceinline int GetFishable() const {
    return (liquid & FISHABLE_MASK) >> FISHABLE_SHIFT;
  }
  void SetLiquid(UINT);
  void SetShared(int);
  void SetFishable(int);

  BOOL IsLiquid() const {
    return GetLiquid() != LIQUID_NONE;
  }

 private:
  BYTE liquid;
};

struct SMOWVert {
  BYTE  flow1;
  BYTE  flow2;
  BYTE  flow1Pct;
  BYTE  filler;
  float height;
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
  WORD              startVertex;
  WORD              count;
  NTempest::C4Plane plane;
};

struct SMOPortalRef {
  WORD  portalIndex;
  WORD  groupIndex;
  short side;
  WORD  filler;
};

struct SIffChunk {
  SIffChunk() {
  }

  SIffChunk(DWORD token, DWORD size) : token(token), size(size) {
  }

  DWORD token;
  DWORD size;
};

struct CMapObjHeader {
  SIffChunk iffChunkVersion;
  DWORD     version;
  SIffChunk iffChunkHeader;
};

struct SMOGxBatch {
  WORD vertStart;
  WORD vertCount;
  WORD batchStart;
  WORD batchCount;
};

struct SMOGroupHeader {
  SIffChunk        iffChunk;
  UINT             nameOffset;
  UINT             descriptiveNameOffset;
  UINT             flags;
  NTempest::CAaBox aaBox;
  UINT             pad0;
  WORD             portalStart;
  WORD             portalCount;
  BYTE             fogIds[4];
  UINT             groupLiquid;
  SMOGxBatch       intBatch[4];
  SMOGxBatch       extBatch[4];
  int              uniqueID;
  UINT             pad1[2];
};

struct SMOGroupInfo {
  DWORD            offset;
  DWORD            size;
  DWORD            flags;
  NTempest::CAaBox aaBox;
  DWORD            nameIndex;
};

struct SPortalExt {
  enum {
    F_SCREEN_CULLED = 1,
    F_INTERSECT_NEAR = 2
  };

  WORD            flags;
  WORD            rLevel;
  NTempest::CRect sRect;
  UINT            xformTag;
  UINT            visitedTag;
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

  DWORD               version;
  DWORD               flags;
  DWORD               blendMode;
  DWORD               diffuseNameIndex;
  NTempest::CImVector sidnColor;
  NTempest::CImVector frameSidnColor;
  DWORD               envNameIndex;
  NTempest::CImVector diffColor;
  DWORD               groundType;
  union {
    BYTE        inMemPad[8];
    HTEXTURE__ *hMaps[2];
  };
};

struct SMOLightmapTex {
  BYTE texels[0x8000];
  union {
    BYTE        inMemPad[4];
    CGxTex     *gxTexture;
    HTEXTURE__ *hTexture;
  };
};

class CMapObjGroup {
 public:
  CMapObjGroup();
  ~CMapObjGroup();
  void Init();
  void InitPtrs();
  void Clear();
  bool IsLoaded() {
    return bLoaded != 0;
  }
  bool IsLoading() {
    return asyncObject != 0;
  }
  void SetFlushTime(float time) {
    flushTime = time;
  }
  UINT GetFlags() {
    return flags;
  }
  UINT GetGroupLiquid() const {
    return groupLiquid;
  }
  UINT GetDoodadRefCount() {
    return doodadRefCount;
  }
  UINT GetDoodadRef(UINT index) {
    return doodadRefList[index];
  }
  UINT GetLightRefCount() {
    return lightRefCount;
  }
  UINT GetLightRef(UINT index) {
    return lightRefList[index];
  }
  long GetUniqueID() {
    return uniqueID;
  }
  BYTE GetFogId(UINT index) {
    return fogIds[index];
  }
  SMOPoly *GetPoly(WORD poly) {
    ASSERT(poly < polyCount);
    ASSERT(polyList);
    return &polyList[poly];
  }
  bool QueryLightmap(const NTempest::C3Vector &point, WORD polyIdx, NTempest::CImVector &color);
  bool QueryLightmap(const NTempest::C3Segment &seg, NTempest::CImVector &color);
  bool QueryLiquidStatus(const NTempest::C3Vector &pos, UINT &liquid, float &surface, NTempest::C3Vector &dir);
  bool QueryLiquidFishable(const NTempest::C3Vector &pos, int &fishable);
  void QueryLiquidSounds(const NTempest::C3Vector &pos, int *lbool, NTempest::C3Vector *ldelta, float *ldsquared);
  bool QueryMtlId(const NTempest::C3Segment &seg, UINT &mtlId);
  bool GetTris(CWTriData &triData, const NTempest::C3Segment &seg, float &maxT, const CMapObjDef *mapObjDef, UINT queryFlags);
  bool GetTris(CWTriData &triData, const NTempest::CAaBox &aaBox, const CMapObjDef *mapObjDef, UINT queryFlags);
  bool GetTris(CWTriData &triData, const CWFrustum &frustum, const CMapObjDef *mapObjDef, UINT queryFlags);

 private:
  friend class CMap;
  friend class CMapObj;

  UINT                 flags;
  NTempest::CAaBox     aaBox;
  UINT                 portalStart;
  UINT                 portalCount;
  BYTE                 fogIds[4];
  UINT                 groupLiquid;
  SMOGxBatch           intBatch[4];
  SMOGxBatch           extBatch[4];
  CGxBuf              *intGxBuf[4];
  CGxBuf              *extGxBuf[4];
  CAaBsp               aaBsp;
  UINT                 frameCount;
  UINT                 rLevel;
  UINT                 minimapTag;
  float                lightmapTexFlushTime;
  char                *dbgName;
  NTempest::C4Plane   *planeList;
  SMOPoly             *polyList;
  NTempest::C3Vector  *vertexList;
  NTempest::C3Vector  *normalList;
  NTempest::C2Vector  *textureVertexList;
  WORD                *indexList;
  SMOBatch            *batchList;
  WORD                *lightRefList;
  WORD                *doodadRefList;
  NTempest::CImVector *colorVertexList;
  NTempest::C2Vector  *lightmapVertexList;
  SMOLightmap         *lightmapList;
  SMOLightmapTex      *lightmapTexList;
  NTempest::C2iVector  liquidVerts;
  NTempest::C2iVector  liquidTiles;
  NTempest::C3Vector   liquidCorner;
  WORD                 liquidMtlId;
  SMOLVert            *liquidVertexList;
  SMOLTile            *liquidTileList;
  UINT                 planeCount;
  UINT                 polyCount;
  UINT                 vertexCount;
  UINT                 normalCount;
  UINT                 textureVertexCount;
  UINT                 indexCount;
  UINT                 batchCount;
  UINT                 lightRefCount;
  UINT                 doodadRefCount;
  UINT                 colorVertexCount;
  UINT                 lightmapVertexCount;
  UINT                 lightmapCount;
  UINT                 lightmapTexCount;
  long                 uniqueID;
  BYTE                *data;
  CMapObj             *parent;
  float                flushTime;
  CAsyncObject        *asyncObject;
  BYTE                 bLoaded;

 public:
  LINKDECLEX(CMapObjGroup, lameAssLink);

 private:
  void CreateLightmapPointers(BYTE *&pData);
  void CreateDataPointers(BYTE *pData);
  void CreateOptionalDataPointers(BYTE *pData);
  void Create(BYTE *rawData);
  UINT SphereIntersectPoly(const NTempest::CAaSphere &sphere, const UINT numVerts, const WORD *indicies);
  bool PointInPoly(const NTempest::C3Vector *p, const UINT numIndicies, const WORD *indicies, const NTempest::C3Vector *n);
  void QueryMinimap(UINT groupID, const NTempest::CAaBox &localBox, TSStackArray<CWorld::MinimapQuad> &quads);
  void FreeData();
  void GenTexture(SMOLightmap *lightmap, const NTempest::CImVector *source, NTempest::CImVector *texture);

  static void UpdateLightmapTex(EGxTexCommand cmd, UINT w, UINT h, UINT d, UINT mipLevel, LPVOID userArg, UINT &texelStrideInBytes, LPCVOID &texels);
  static void AsyncPostloadCallback(LPVOID userArg);
  static CGxBuf *AllocExtGxBuf(UINT nVerts, UINT nIndices);
  static void    ExtGxBufFill(CGxBufCommand &cmd, CGxBuf *buf);
  static CGxBuf *AllocIntGxBuf(UINT nVerts, UINT nIndices);
  static void    IntGxBufFill(CGxBufCommand &cmd, CGxBuf *buf);
  static void    Destroy();
  static void    FreeExtGxBuf(CGxBuf *&gxBuf);
  static void    FreeIntGxBuf(CGxBuf *&gxBuf);
  void           FreeLightmaps();
  void           CreateLightmaps();
  void           ExtGxBufFillVertex(CGxBufCommand &cmd, CGxBuf *buf);
  void           IntGxBufFillVertex(CGxBufCommand &cmd, CGxBuf *buf);
  void           GxBufFillIndex(CGxBufCommand &cmd, CGxBuf *buf);
  void           GetTrisFromQuery(CWTriData &triData, BspQuery &q, const CMapObjDef *mapObjDef);

  static TSCArray<CGxBuf *, 512> extGxBufFreeList;
  static TSCArray<CGxBuf *, 512> intGxBufFreeList;
  static const EGxTexFormat      LIGHTMAP_FORMAT;
  static const SMOGxBatch       *sLockGxBatch;
  static UINT                    rDrawSharedLiquidFirst;
  static UINT                    rDrawSharedLiquidToggle;
};

class CMapObj : public TSHashObject<CMapObj, HASHKEY_NONE> {
 public:
  static void     Initialize();
  static void     Destroy();
  static void     ClearCache(int force);
  static void     Delete(CMapObj *mapObj);
  static CMapObj *Create(LPCSTR fileName);
  static void     SetGroupRenderCallback(void (*func)(const UINT, LPCVOID, const int), LPVOID userParam);

  CMapObj();
  ~CMapObj();
  void Init();
  void InitPtrs();
  void Clear();
  void ReadGroup(UINT index);

  UINT GetId() {
    return header->wmoID;
  }
  NTempest::CImVector GetAmbientColor() {
    return ambColor;
  }
  const SMOMaterial *GetMaterial(UINT mtlId) {
    ASSERT(mtlId < materialCount);
    ASSERT(materialList != 0);
    return &materialList[mtlId];
  }
  bool IsLoaded() {
    return bLoaded;
  }
  bool IsLoading() {
    return asyncObject != 0;
  }
  LPCSTR GetFileName() {
    return name;
  }
  UINT GetNumGroups() {
    return groupCount;
  }
  SMODoodadDef *GetDoodadDef(UINT index) {
    return &doodadDefList[index];
  }
  LPCSTR GetDoodadName(UINT index) {
    return &doodadNameList[index];
  }
  UINT GetLightCount() {
    return lightCount;
  }
  SMOLight *GetLight(UINT index) {
    return &lightList[index];
  }
  bool IsGroupLoaded(UINT index);
  bool IsGroupLoading(UINT index);
  void SetFlushTime(float time) {
    flushTime = time;
  }
  void WaitLoad();
  void WaitLoadGroup(UINT index);
  UINT GetWmoID() {
    return header->wmoID;
  }
  CMapObjGroup       *GetGroup(UINT index, int force = 0);
  const SMOGroupInfo *GetGroupInfo(UINT index);
  char               *GetGroupName(UINT index);
  void                GetBounds(NTempest::CAaBox &aaBox);
  void                GetBounds(NTempest::CAaSphere &aaSphere);
  void                GetGroupBounds(NTempest::CAaBox &aaBox, UINT index);
  void                GetGroupBounds(NTempest::CAaSphere &aaSphere, UINT index);
  UINT                GetGroupFlags(UINT index);
  UINT                GetDoodadSet(UINT doodadIndex);
  const SMOFog       &GetFog(UINT index) {
    FATALASSERT(index < fogCount);
    return fogList[index];
  }
  UINT GetFogCount() {
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
      CMapObjDef               *mapObjDef,
      const NTempest::C3Vector *v0,
      const NTempest::C3Vector *v1,
      UINT                      queryFlags,
      UINT                      polyIgnoreFlags,
      UINT                      groupIgnoreFlags,
      float                    *dist,
      SMOPoly                 **poly
  );
  bool VectorIntersectPortals(const NTempest::C3Segment &seg, float &maxT, UINT *groupIDs);
  bool VectorIntersectPortal(const NTempest::C3Vector &v0, const NTempest::C3Vector &v1, UINT fromGroup, UINT &toGroup);
  bool TestGroupBounds(const NTempest::C3Vector &v0, const NTempest::C3Vector &v1, UINT index);
  bool TestGroupBounds(const NTempest::C3Vector &point, const UINT index);
  bool TestGroupBounds(const NTempest::CAaBox &box, const UINT index);
  bool GetTris(CWTriData &triData, const NTempest::CAaBox &aaBox, const CMapObjDef *mapObjDef, UINT queryFlags);
  bool GetTris(CWTriData &triData, const NTempest::C3Segment &seg, float &maxT, const CMapObjDef *mapObjDef, UINT queryFlags);
  bool GetTris(CWTriData &triData, const CWFrustum &frustum, const CMapObjDef *mapObjDef, UINT queryFlags);
  bool QueryLightmap(const NTempest::C3Segment &seg, NTempest::CImVector &color, float *t);
  bool QueryLiquidStatus(UINT ignoreGroupFlags, const NTempest::C3Vector &pos, UINT &liquid, float &surface, NTempest::C3Vector &dir);
  bool QueryLiquidFishable(UINT ignoreGroupFlags, const NTempest::C3Vector &pos, int &fishable);
  void QueryLiquidSounds(
      UINT                      groupIdx,
      UINT                      parentIdx,
      UINT                      rlevel,
      UINT                     &closestExtLevel,
      const NTempest::C3Vector &pos,
      int                      *lbool,
      NTempest::C3Vector       *ldelta,
      float                    *ldsquared
  );
  bool QueryMapObjMinimap(UINT groupID, const NTempest::CAaBox &localBox, TSStackArray<CWorld::MinimapQuad> &quads);

  static void PrepareUpdate();
  void        LocateViewer(NTempest::C44Matrix &im, TSGrowableArray<UINT> &inGroups);
  UINT        StabPortals(UINT fromGroupIndex, UINT groupIndex, NTempest::C3Vector &rayOrig, NTempest::C3Vector &rayDir);
  UINT        StabPortals(UINT groupIndex, const NTempest::C3Vector &start, const NTempest::C3Vector &end);
  void        IntRender(NTempest::C44Matrix &mat, TSGrowableArray<UINT> &inGroups);
  void        ExtRender(NTempest::C44Matrix &mat, const NTempest::CRect &rect);
  void RenderGroup(UINT groupNum, int rDrawSharedLiquidToggle, const NTempest::C44Matrix &invMat, const LISTEX(CWFrustum, sceneLink) & frustumList);

  static TSCArray<NTempest::CRect, 16> extViewList;
  static TSCArray<SPortalExt, 2048>    portalExtList;
  static UINT                          maxRLevel;
  static UINT                          DEFAULT_RLEVEL;
  static UINT                          MAX_SOUND_RLEVEL;
  static NTempest::C3Vector            localCamPos;
  static CMapObjDef                   *curMapObjDef;
  static BOOL                          bIntRender;
  static UINT                          sMinimapTag;

 private:
  friend class CMap;
  friend class CMapObjGroup;
  friend class CMapEntity;

  static void AsyncPostloadCallbackHeader(LPVOID userArg);
  static void AsyncPostloadCallback(LPVOID userArg);
  static void AsyncPostloadCallbackAll(LPVOID userArg);
  BOOL        Read(LPCSTR fileName);
  void        CreateData();
  void        AllocGroups();
  void        CreateAllGroups();
  void        ReadExtGroups();
  SIffChunk  *ReadChunkHeader(BYTE *&pData, DWORD expectedToken);
  SIffChunk  *ReadOptionalChunkHeader(BYTE *&pData, DWORD expectedToken);
  void        CreateDataPointers();
  void        CreateMaterials();
  void        CreateMaterial(UINT materialId);
  void        CreateGroup(CMapObjGroup *group, SMOGroupInfo *groupInfo);
  void        ReadGroup(CMapObjGroup *group, SMOGroupInfo *groupInfo, int preLoad);
  void        UpdateMaterials();
  void        RenderAlways(UINT groupIdx);
  void        RRenderThruPortals(UINT groupIdx, UINT parentIdx, NTempest::CRect &viewRect, UINT level);
  void        RTransformPortal(SMOPortal *portal, SPortalExt *portalExt, int cpIgnore);
  bool        CullBatch(const SMOBatch *batch);
  void        RenderGroupLightTex(const CMapObjGroup *group, UINT frustumCount);
  void        RenderGroupLightmapTex_Int(const CMapObjGroup *group, UINT frustumCount);
  void        RenderGroupLightmapTex_Ext(const CMapObjGroup *group, UINT frustumCount);
  void        RenderGroupLightmapTex(const CMapObjGroup *group, UINT frustumCount);
  void        RenderGroupColorTex_Int(const CMapObjGroup *group, UINT frustumCount);
  void        RenderGroupColorTex_Ext(const CMapObjGroup *group, UINT frustumCount);
  void        RenderGroupColorTex(const CMapObjGroup *group, UINT frustumCount);
  void        RenderGroupLightmap(const CMapObjGroup *group, UINT frustumCount);
  void        RenderGroupTex(const CMapObjGroup *group, UINT frustumCount);
  void        RenderGroup_Ext(const CMapObjGroup *group, UINT frustumCount);
  void        RenderGroup_Int(const CMapObjGroup *group, UINT frustumCount);
  void        RenderPortals(CMapObjGroup *group);
  void        RenderPortals();
  void        RenderGroupBsp(const CMapObjGroup *group, UINT frustumCount);
  void        RenderGroupNormals(const CMapObjGroup *group);
  void        RenderWaterIndices_0(const CMapObjGroup *group, WORD *idxBase, UINT vtxSub, UINT &idxSub);
  void        RenderLiquid_0(const CMapObjGroup *group);
  void        RenderInteriorWater_0(const CMapObjGroup *group, UINT liquid);
  void        RenderExteriorWater_0(const CMapObjGroup *group, UINT liquid);
  void        RenderMagma(const CMapObjGroup *group, UINT liquid);
  void        QueryMapObjMinimapGroup(UINT groupID, UINT parentID, const NTempest::CAaBox &localBox, TSStackArray<CWorld::MinimapQuad> &quads);

  char                name[MAX_PATH];
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
  UINT                textureNameCount;
  UINT                groupNameCount;
  UINT                groupCount;
  UINT                portalVertexCount;
  UINT                portalCount;
  UINT                portalRefCount;
  UINT                lightCount;
  UINT                doodadSetCount;
  UINT                doodadNameCount;
  UINT                doodadDefCount;
  UINT                fogCount;
  UINT                volumePlaneCount;
  NTempest::CImVector ambColor;
  int                 version;
  NTempest::CAaBox    aaBox;
  SFile              *file;

 public:
  LINKDECLEX(CMapObj, lameAssLink);

 private:
  CMapObjHeader fileHeader;
  BYTE         *data;
  DWORD         dataBytes;
  int           refCount;
  float         flushTime;
  CAsyncObject *asyncObject;
  BYTE          bLoaded;
  SMOMaterial  *materialList;
  UINT          materialCount;
  UINT          nGroupsRead;
  LISTDECLEX(CMapObjGroup, lameAssLink, groupList);
  TSCArray<CMapObjGroup *, 384> groupPtrList;

  static UINT gRenderCount;
  static void (*gRenderCallback)(const UINT, LPCVOID, const int);
  static LPVOID                             gRenderUserParam;
  static TSHashTable<CMapObj, HASHKEY_NONE> mapObjHash;
  static HASHKEY_NONE                       nullHashKey;
};

#endif
