#ifndef ENGINE_SOURCE_MODEL_MODELINTERNAL_H
#define ENGINE_SOURCE_MODEL_MODELINTERNAL_H

#include "Model/IModel.h"

#include "Anim/AnimTypes.h"
#include "Gx/Gx.h"
#include "Model/Material.h"
#include "Services/Texture.h"
#include "Tempest/c2vector.h"
#include "Tempest/c3vector.h"
#include "Tempest/c34matrix.h"
#include "Tempest/caabox.h"
#include "Tempest/caasphere.h"
#include "Tempest/cimvector.h"

#include <stpl.h>

class CAsyncObject;
class CParticleEmitter2;
class CRibbonEmitter;

enum ModelIntersectResult {
  MODEL_INTERSECT_NO_HIT = 0,
  MODEL_INTERSECT_HIT_BOUNDING_SPHERE = 1,
  MODEL_INTERSECT_HIT_COLLISION_VOLUMES = 2,
  MODEL_INTERSECT_HIT_MODEL = 3
};

UINT GetInvalidMatrixId();
void GxuLightSelectCallback(LPVOID parm, NTempest::C3Vector worldPos, const NTempest::C3Vector &cameraWorldPos, UINT maxLightsToUse);

DECLARE_DERIVED_HANDLE(HCOLLISIONDATA, HOBJECT);

enum EModelParamType {
  MPARAM_UINT = 0,
  MPARAM_HANDLE = 1,
  MPARAM_FLOAT = 2,
  MPARAM_C3VECTOR = 3,
  MPARAM_BOOL = 4,
  MPARAM_CARGB = 5,
  MPARAM_PTR = 6,
  MPARAM_BYTE = 7,
  MPARAM_NONE = 8
};

enum GROUND_TRACK {
  TRACK_YAW_ONLY = 0,
  TRACK_PITCH_YAW = 1,
  TRACK_PITCH_YAW_ROLL = 2,
  GROUND_TRACK_MASK = 3
};

enum COLLIDE_TYPE {
  COLLIDE_BOX = 0,
  COLLIDE_CYLINDER = 1,
  COLLIDE_SPHERE = 2,
  COLLIDE_PLANE = 3
};

struct CBoundsData {
  NTempest::CAaBox    extent;
  NTempest::CAaSphere sphere;
};

struct CHitTest {
  CHitTest() : type(COLLIDE_BOX), radius(0.0f) {
  }

  COLLIDE_TYPE       type;
  NTempest::C3Vector extent[2];
  float              radius;
};

struct CPrimitive {
  CPrimitive() : type(GxPrim_Triangles), vertexCount(0) {
  }

  EGxPrim type;
  UINT    vertexCount;
};

typedef TSFixedArray_<NTempest::C2Vector, 'IMod', __LINE__> CModelTexCoordArray;

struct CGeosetShared {
  CGeosetShared() : vertexShader(GxVS_PassThru), materialId(0), centroid(0.0f), radius(0.0f), selectionGroup(0), geosetId(0), flags(0) {
  }

  TSFixedArray_<NTempest::C3Vector, 'IMod', __LINE__>  position;
  TSFixedArray_<BYTE, 'IMod', __LINE__>                boneWeights;
  TSFixedArray_<NTempest::C3Vector, 'IMod', __LINE__>  normal;
  TSFixedArray_<CModelTexCoordArray, 'IMod', __LINE__> texCoord;
  TSFixedArray_<CPrimitive, 'IMod', __LINE__>          primitive;
  TSFixedArray_<WORD, 'IMod', __LINE__>                primitiveVertices;
  TSFixedArray_<UINT, 'IMod', __LINE__>                groupMatrixCounts;
  TSFixedArray_<UINT, 'IMod', __LINE__>                matrices;
  TSFixedArray_<UINT, 'IMod', __LINE__>                hwBoneIndices;
  TSFixedArray_<UINT, 'IMod', __LINE__>                hwBoneWeights;
  EGxVertexShader                                 vertexShader;
  UINT                                            materialId;
  NTempest::C3Vector                              centroid;
  float                                           radius;
  UINT                                            selectionGroup;
  UINT                                            geosetId;
  UINT                                            flags;
};

struct CGeoset {
  CGeoset() {
    flags = 0;
    weightedBones = GetInvalidMatrixId();
  }

  UINT weightedBones;
  UINT flags;
};

struct CCustomGeoset {
  NTempest::C3Vector position;
  void (*renderCallback)(HMODEL, const NTempest::C34Matrix &, LPVOID);
  LPVOID renderParam;
};

struct CModelTexture {
  CModelTexture() : handle(0), replaceableId(0) {
  }
  CModelTexture(const CModelTexture &source) {
    replaceableId = source.replaceableId;
    handle = (HTEXTURE)HandleDuplicate(source.handle);
  }
  CModelTexture &operator=(const CModelTexture &source) {
    replaceableId = source.replaceableId;
    if (handle) {
      HandleClose(handle);
    }
    handle = (HTEXTURE)HandleDuplicate(source.handle);
    return *this;
  }
  ~CModelTexture() {
    if (handle) {
      HandleClose(handle);
    }
  }

  HTEXTURE handle;
  UINT     replaceableId;
};

inline int CTmuPassUnique::Compare(const CModelTexture *aTextures, const CModelTexture *bTextures, const CTmuPassUnique &a, const CTmuPassUnique &b) {
  if (a.combiner != b.combiner) {
    return a.combiner - b.combiner;
  }
  return HandleObjectCompare(a.textureId == -1 ? 0 : aTextures[a.textureId].handle, b.textureId == -1 ? 0 : bTextures[b.textureId].handle);
}

class CModelBase {
 public:
  CModelBase(UINT flags = 0);
  ~CModelBase();

 private:
  CModelBase &operator=(const CModelBase &source);
 public:
  void (*m_PickLights)(LPVOID, NTempest::C3Vector, const NTempest::C3Vector &, UINT);
  LPVOID              m_pickLightsParm;
  UINT                m_flags;
  NTempest::C34Matrix m_modelToWorld;
  UINT                m_texBones;
  HANIM               m_anim;
  HMODEL              m_boundsModel;
  UINT                m_aaBoxCustGeoId;
  HMODEL              m_collideModel;

 protected:
  CModelBase(const CModelBase &source);
};

inline CModelBase::CModelBase(UINT flags)
    : m_PickLights(GxuLightSelectCallback), m_flags(flags), m_anim(0), m_boundsModel(0), m_aaBoxCustGeoId(-1), m_collideModel(0) {
  m_texBones = GetInvalidMatrixId();
}

class CModelSimple : public CModelBase {
 public:
  CModelSimple() : CModelBase(0) {
    m_geosets.SetCount(0);
    m_geosetColor.SetCount(0);
    m_custGeosets.SetCount(0);
    m_materials.SetCount(0);
    m_textures.SetCount(0);
  }
  CModelSimple(const CModelSimple &source);
  ~CModelSimple();

  TSCArray<CGeoset, 5>       m_geosets;
  TSCArray<CGeosetColor, 5>  m_geosetColor;
  TSCArray<CCustomGeoset, 1> m_custGeosets;
  TSCArray<HMATERIAL, 4>     m_materials;
  TSCArray<CModelTexture, 4> m_textures;

 private:
  CModelSimple &operator=(const CModelSimple &source);
  void          CopyMaterials(const CModelSimple &source);
};

struct CModelShared : public CHandleObject {
  CModelShared() : numBones(0), numTexBones(0), groundTrack(TRACK_YAW_ONLY), collision(0), numGeosets(0), numLayers(0) {
    name[0] = 0;
  }

  CModelShared(const CModelShared &source);
  ~CModelShared() {
    if (collision) {
      HandleClose((HOBJECT)collision);
    }
  }

  virtual LPCSTR GetObjectName() {
    return name;
  }

  TSFixedArray<CBoundsData>                      seqBounds;
  TSFixedArray<UINT>                             attachIdToIndex;
  TSFixedArray_<NTempest::C3Vector, 'IMod', __LINE__> positions;
  TSFixedArray<CHitTest>                         hitTest;
  TSFixedArray<CGeosetShared>                    geosets;
  TSFixedArray<UINT>                             emitter2Order;
  TSFixedArray<UINT>                             ribbonOrder;
  UINT                                           numBones;
  UINT                                           numTexBones;
  GROUND_TRACK                                   groundTrack;
  HCOLLISIONDATA                                 collision;
  char                                           name[MAX_PATH];
  CBoundsData                                    bounds;
  BYTE                                           numGeosets;
  BYTE                                           numLayers;

 private:
};

void ModelEnableLights(HMODEL model, int enable);
void ModelShowBoundingSphere(HMODEL model);
void ModelShowBoundingBox(HMODEL model);
BOOL ModelGeosetAdd(
    HMODEL                    model,
    UINT                      numVertices,
    const NTempest::C3Vector *position,
    const NTempest::C3Vector *normal,
    const NTempest::C2Vector *texCoord,
    EGxPrim                   primitiveType,
    const WORD               *primitiveVertices,
    UINT                      numPrimVertices,
    HTEXTURE                  texture,
    EGxBlend                  blendMode,
    UINT                      disables,
    NTempest::CImVector       color,
    UINT                      replaceableId
);
HMODEL ModelCreateSimpleMesh(
    LPCSTR                    name,
    UINT                      numVertices,
    const NTempest::C3Vector *position,
    const NTempest::C3Vector *normal,
    const NTempest::C2Vector *texCoord,
    EGxPrim                   primitiveType,
    const WORD               *primitiveVertices,
    UINT                      numPrimVertices,
    HTEXTURE                  texture,
    EGxBlend                  blendMode,
    UINT                      disables,
    NTempest::CImVector       color,
    UINT                      replaceableId
);
HMODEL CreateModelBoundingBox(const NTempest::CAaBox &bounds, HTEXTURE texture, EGxBlend blendMode);

NODEDECL(LINKUNIQUE) {
  LINKUNIQUE() : child(0), scale(1.0f) {
  }

  ~LINKUNIQUE() {
    if (child) {
      ModelEnableLights(child, 0);
      HandleClose(child);
    }
  }

  HMODEL child;
  float  scale;

 private:
  LINKUNIQUE(const LINKUNIQUE &source);
  LINKUNIQUE &operator=(const LINKUNIQUE &source);
};

class CModelComplex : public CModelBase {
 public:
  CModelComplex() : CModelBase(0x20) {
  }
  CModelComplex(const CModelComplex &source);
  CModelComplex(const CModelSimple &source);
  ~CModelComplex();

  TSGrowableArray<CGeoset>          m_geosets;
  TSGrowableArray<CGeosetShared>    m_addlGeosets;
  TSGrowableArray<CGeosetColor>     m_geosetColor;
  TSGrowableArray<CCustomGeoset>    m_custGeosets;
  TSGrowableArray<HMATERIAL>        m_materials;
  TSGrowableArray<CModelTexture>    m_textures;
  TSFixedArray<DWORD>               m_lights;
  TSFixedArray<LIST(LINKUNIQUE)>    m_attached;
  TSFixedArray_<BYTE, 'MDLF', __LINE__>  m_attachmentFlags;
  TSFixedArray<CParticleEmitter2 *> m_emitters2;
  TSFixedArray<CRibbonEmitter *>    m_ribbons;
  TSFixedArray<HCAMERA>             m_cameras;
  TSFixedArray<UINT>                m_cameraOrder;
  TSFixedArray<NTempest::C34Matrix> m_hitTestMtx;

 private:
  CModelComplex &operator=(const CModelComplex &source);
  CModelComplex &operator=(const CModelSimple &source);
  void           CopyAttachments(const CModelComplex &source);
  void           CopyCameras(const CModelComplex &source);
  void           CopyLights(const CModelComplex &source);
  void           CopyEmitters(const CModelComplex &source);
  void           CopyRibbons(const CModelComplex &source);
};


struct CModelRenderData {
  CGeoset       *m_geosets;
  CGeosetColor  *m_geosetColor;
  UINT           m_numGeosets;
  HMATERIAL     *m_materials;
  CModelTexture *m_textures;
  CModel        *m_model;
  CModelShared  *m_shared;
  UINT           m_renderFlags;
};

void      EnqueueModelCommand(CModel *model, EModelModQ command, ...);
HMATERIAL BuildSimpleMaterial(CModelTexture *modelTexture, UINT textureId, HTEXTURE texture, EGxBlend blendMode, UINT disables, UINT replaceableId);
UINT      MatrixAlloc(UINT numMatrices);
NTempest::C34Matrix *MatrixDeref(UINT handle);
BOOL                 IModelDerefHandle(CModel *model, CModelBase **unique, CModelShared **shared);
BOOL                 IModelDerefHandle(CModel *model, CModelBase **unique);
BOOL                 IModelDerefHandle(CModel *model, CModelShared **shared);

void MdxReadCameras(BYTE *data, UINT fileBytes, TSFixedArray<HCAMERA> *cameras);

void MdxReadLights(BYTE *data, UINT fileBytes, CModelComplex *modelptr);

HCOLLISIONDATA CollisionDataCreate(BYTE *fileData, UINT fileBytes);

void MdxReadAttachments(BYTE *data, UINT fileBytes, UINT flags, CModelComplex *modelptr, CModelShared *shared, CStatus *status);

void MdxReadRibbonEmitters(BYTE *data, UINT fileBytes, CModelComplex *modelptr, CModelShared *shared);

#endif
