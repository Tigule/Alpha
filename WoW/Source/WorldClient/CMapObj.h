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
struct CGxBuf;
struct CGxBufCommand;
struct HTEXTURE__;
struct SMOBatch {
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
  unsigned char  pad;
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
struct SMOLTile {
  unsigned char flags;
};

struct SMOLVert {
  unsigned int color;
  float        height;
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
  unsigned int token;
  unsigned int size;
};

struct CMapObjHeader {
  SIffChunk    iffChunkVersion;
  unsigned int version;
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
  unsigned int     offset;
  unsigned int     size;
  unsigned int     flags;
  NTempest::CAaBox aaBox;
  unsigned int     nameIndex;
};

struct SPortalExt {
  unsigned short  flags;
  unsigned short  rLevel;
  NTempest::CRect sRect;
  unsigned int    xformTag;
  unsigned int    visitedTag;
};

struct SMOMaterial {
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
  ~CMapObjGroup();
  void         Init();
  void         InitPtrs();
  void         Clear();
  bool         QueryLightmap(const NTempest::C3Vector &point, unsigned short polyIdx, NTempest::CImVector &color);
  bool         QueryLightmap(const NTempest::C3Segment &seg, NTempest::CImVector &color);
  unsigned int QueryLiquidStatus(NTempest::C3Vector &pos, unsigned int &liquid, float &surface, NTempest::C3Vector &dir);
  void         QueryMinimap(unsigned int groupID, NTempest::CAaBox &localBox, TSStackArray<CWorld::MinimapQuad> &quads);
  bool         GetTris(CWTriData &triData, const NTempest::C3Segment &seg, float &maxT, const CMapObjDef *mapObjDef, unsigned int faceIgnoreFlags);
  unsigned int GetTris(CWTriData &triData, NTempest::CAaBox &aaBox, CMapObjDef *mapObjDef, unsigned int faceIgnoreFlags);
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
  int                  uniqueID;
  unsigned char       *data;
  CMapObj             *parent;
  float                flushTime;
  CAsyncObject        *asyncObject;
  unsigned char        bLoaded;
  TSLink<CMapObjGroup> lameAssLink;

 private:
  friend class CMapObj;

  void CreateLightmapPointers(unsigned char *&pData);
  void CreateDataPointers(unsigned char *pData);
  void CreateOptionalDataPointers(unsigned char *pData);
  void Create(unsigned char *rawData);

  static void __fastcall UpdateLightmapTex(
      EGxTexCommand cmd,
      unsigned int  w,
      unsigned int  h,
      unsigned int  d,
      unsigned int  mipLevel,
      void         *userArg,
      unsigned int &texelStrideInBytes,
      const void  *&texels
  );
  static void __fastcall    AsyncPostloadCallback(void *userArg);
  static CGxBuf *__fastcall AllocExtGxBuf(unsigned int nVerts, unsigned int nIndices);
  static void __fastcall    ExtGxBufFill(CGxBufCommand &cmd, CGxBuf *buf);
  static CGxBuf *__fastcall AllocIntGxBuf(unsigned int nVerts, unsigned int nIndices);
  static void __fastcall    IntGxBufFill(CGxBufCommand &cmd, CGxBuf *buf);
  static void __fastcall    Destroy();
  static void __fastcall    FreeExtGxBuf(CGxBuf *&gxBuf);
  static void __fastcall    FreeIntGxBuf(CGxBuf *&gxBuf);
  void                      FreeLightmaps();
  void                      CreateLightmaps();
  void                      ExtGxBufFillVertex(CGxBufCommand &cmd, CGxBuf *buf);
  void                      IntGxBufFillVertex(CGxBufCommand &cmd, CGxBuf *buf);
  void                      GxBufFillIndex(CGxBufCommand &cmd, CGxBuf *buf);
  void                      GetTrisFromQuery(CWTriData &triData, BspQuery &q, const CMapObjDef *mapObjDef);

  static TSCArray<CGxBuf *, 512> extGxBufFreeList;
  static TSCArray<CGxBuf *, 512> intGxBufFreeList;
  static SMOGxBatch             *sLockGxBatch;
  static unsigned int            rDrawSharedLiquidFirst;
  static unsigned int            rDrawSharedLiquidToggle;
};

class CMapObj : public TSHashObject<CMapObj, HASHKEY_NONE> {
 public:
  static void __fastcall     Initialize();
  static void __fastcall     Destroy();
  static void __fastcall     ClearCache(int force);
  static void __fastcall     Delete(CMapObj *mapObj);
  static CMapObj *__fastcall Create(const char *fileName);
  static void __fastcall     SetGroupRenderCallback(void(__fastcall *func)(const unsigned int, const void *, const int), void *userParam);

  CMapObj();
  ~CMapObj();
  void Init();
  void InitPtrs();
  void Clear();
  void CreateDataPointers();
  void CreateMaterials();
  void CreateData();
  int  Read(const char *fileName);
  void ReadGroup(unsigned int index);

  bool         IsGroupLoaded(unsigned int index);
  void         WaitLoad();
  void         WaitLoadGroup(unsigned int index);
  unsigned int GetWmoID() {
    return header->wmoID;
  }
  CMapObjGroup *GetGroup(unsigned int index, int force);
  void          GetBounds(NTempest::CAaBox &aaBox);
  void          GetBounds(NTempest::CAaSphere &aaSphere);
  void          GetGroupBounds(NTempest::CAaBox &aaBox, unsigned int index);
  void          GetGroupBounds(NTempest::CAaSphere &aaSphere, unsigned int index);
  unsigned int  GetGroupFlags(unsigned int index);
  unsigned int  GetDoodadSet(unsigned int doodadIndex);
  SMOFog       &GetFog(unsigned int index) {
    FATALASSERT(index < fogCount);
    return fogList[index];
  }
  bool TestBounds(const NTempest::CAaBox &box);
  bool TestGroupBounds(const NTempest::CAaBox &box, unsigned int index);
  bool QueryLightmap(const NTempest::C3Segment &seg, NTempest::CImVector &color, float *t);
  unsigned int
  QueryLiquidStatus(unsigned int ignoreGroupFlags, NTempest::C3Vector &pos, unsigned int &liquid, float &surface, NTempest::C3Vector &dir);
  unsigned int QueryMapObjMinimap(unsigned int groupID, NTempest::CAaBox &localBox, TSStackArray<CWorld::MinimapQuad> &quads);

  static void __fastcall PrepareUpdate();
  void                   LocateViewer(NTempest::C44Matrix &im, TSGrowableArray<unsigned int> &inGroups);
  void                   IntRender(NTempest::C44Matrix &mat, TSGrowableArray<unsigned int> &inGroups);
  void                   ExtRender(NTempest::C44Matrix &mat, NTempest::CRect &rect);
  void RenderGroup(unsigned int groupNum, int rDrawSharedLiquidToggle, NTempest::C44Matrix &invMat, TSExplicitList<CWFrustum, 244> &frustumList);

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

  static void __fastcall AsyncPostloadCallbackHeader(void *userArg);
  static void __fastcall AsyncPostloadCallback(void *userArg);
  static void __fastcall AsyncPostloadCallbackAll(void *userArg);
  void                   CreateAllGroups();
  void                   ReadExtGroups();
  SIffChunk             *ReadChunkHeader(unsigned int *&pData, unsigned long expectedToken);
  SIffChunk             *ReadOptionalChunkHeader(unsigned char *&pData, unsigned long expectedToken);
  void                   CreateMaterial(unsigned int materialId);
  void                   CreateGroup(CMapObjGroup *group, SMOGroupInfo *groupInfo);
  void                   ReadGroup(CMapObjGroup *group, SMOGroupInfo *groupInfo, int preLoad);
  void                   UpdateMaterials();
  void                   RenderAlways(unsigned int groupIdx);
  void                   RRenderThruPortals(unsigned int groupIdx, unsigned int parentIdx, NTempest::CRect &viewRect, unsigned int level);
  void                   RTransformPortal(SMOPortal *portal, SPortalExt *portalExt, int cpIgnore);
  unsigned int           CullBatch(SMOBatch *batch);
  void                   RenderGroupLightTex(CMapObjGroup *group, unsigned int frustumCount);
  void                   RenderGroupLightmapTex_Int(CMapObjGroup *group, unsigned int frustumCount);
  void                   RenderGroupLightmapTex_Ext(CMapObjGroup *group, unsigned int frustumCount);
  void                   RenderGroupLightmapTex(CMapObjGroup *group, unsigned int frustumCount);
  void                   RenderGroupColorTex_Int(CMapObjGroup *group, unsigned int frustumCount);
  void                   RenderGroupColorTex_Ext(CMapObjGroup *group, unsigned int frustumCount);
  void                   RenderGroupColorTex(CMapObjGroup *group, unsigned int frustumCount);
  void                   RenderGroupLightmap(CMapObjGroup *group, unsigned int frustumCount);
  void                   RenderGroupTex(CMapObjGroup *group, unsigned int frustumCount);
  void                   RenderGroup_Ext(CMapObjGroup *group, unsigned int frustumCount);
  void                   RenderGroup_Int(CMapObjGroup *group, unsigned int frustumCount);
  void                   RenderPortals(CMapObjGroup *group);
  void                   RenderPortals();
  void                   RenderGroupBsp(CMapObjGroup *group, unsigned int frustumCount);
  void                   RenderGroupNormals(CMapObjGroup *group);
  void                   RenderWaterIndices_0(CMapObjGroup *group, unsigned short *idxBase, unsigned int vtxSub, unsigned int &idxSub);
  void                   RenderLiquid_0(CMapObjGroup *group);
  void                   RenderInteriorWater_0(CMapObjGroup *group, unsigned int liquid);
  void                   RenderExteriorWater_0(CMapObjGroup *group, unsigned int liquid);
  void                   RenderMagma(CMapObjGroup *group, unsigned int liquid);
  void QueryMapObjMinimapGroup(unsigned int groupID, unsigned int parentID, NTempest::CAaBox &localBox, TSStackArray<CWorld::MinimapQuad> &quads);

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
  TSLink<CMapObj> lameAssLink;

 private:
  CMapObjHeader                       fileHeader;
  unsigned int                       *data;
  unsigned long                       dataBytes;
  int                                 refCount;
  float                               flushTime;
  CAsyncObject                       *asyncObject;
  unsigned int                        bLoaded;
  SMOMaterial                        *materialList;
  unsigned int                        materialCount;
  unsigned int                        nGroupsRead;
  TSExplicitList<CMapObjGroup, 0x1AC> groupList;
  TSCArray<CMapObjGroup *, 384>       groupPtrList;

  static unsigned int gRenderCount;
  static void(__fastcall *gRenderCallback)(const unsigned int, const void *, const int);
  static void                              *gRenderUserParam;
  static TSHashTable<CMapObj, HASHKEY_NONE> mapObjHash;
  static HASHKEY_NONE                       nullHashKey;
};

#endif
