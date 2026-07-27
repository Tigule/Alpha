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

unsigned int GetInvalidMatrixId();
void
GxuLightSelectCallback(void *parm, NTempest::C3Vector worldPos, const NTempest::C3Vector &cameraWorldPos, unsigned int maxLightsToUse);

struct HCOLLISIONDATA__;
typedef HCOLLISIONDATA__ *HCOLLISIONDATA;

DECLARE_DERIVED_HANDLE(HMODELSHARED, HOBJECT);

enum EModelLoad {
  CMODEL_UNINITIALIZED = 0,
  CMODEL_LOADED = 1,
  CMODEL_ASYNC_WAIT = 2,
  CMODEL_DUPE_WAIT = 3,
  CMODEL_DUPE_WAIT_PRSRV_ANIM = 4
};

enum EModelModQ {
  MODEL_ADD_LINK = 0,
  MODEL_APPLY_OBJECT_FACE_DIR = 1,
  MODEL_APPLY_OBJECT_LOOK_AT = 2,
  MODEL_CLEAR_ALL_LINKS = 3,
  MODEL_CLEAR_LINK = 4,
  MODEL_ENABLE_ANIM_BLENDING = 5,
  MODEL_ENABLE_FULL_ALPHA = 6,
  MODEL_FINISH_DUPLICATION = 7,
  MODEL_FORCE_CURRENT_SEQUENCE_TIME = 8,
  MODEL_FORCE_SEQUENCE_TIME = 9,
  MODEL_HIDE_BOUNDS = 10,
  MODEL_HIDE_GEOSETS = 11,
  MODEL_HIDE_GEOSETS_RANGE = 12,
  MODEL_LOCK_OBJECT_SEQUENCE = 13,
  MODEL_MARK_FOOTSTEP_SEQUENCE = 14,
  MODEL_MATCH_SEQUENCE = 15,
  MODEL_OPTIMIZE_VISIBLE_GEOSETS = 16,
  MODEL_REMOVE_LINK = 17,
  MODEL_REMOVE_OBJECT_FACE_DIR = 18,
  MODEL_REMOVE_OBJECT_LOOK_AT = 19,
  MODEL_REPLACE_TEXTURE = 20,
  MODEL_SET_EMISSIVE_COLOR = 21,
  MODEL_SET_EVENT_CALLBACK = 22,
  MODEL_SET_LIGHT_SELECT_CALLBACK = 23,
  MODEL_SET_OBJECT_TIME_SCALE = 24,
  MODEL_SET_RANDOM_SEQUENCE_FIDGET1 = 25,
  MODEL_SET_RANDOM_SEQUENCE_FIDGET2 = 26,
  MODEL_SET_SEQ_FINISHED_HANDLER1 = 27,
  MODEL_SET_SEQ_FINISHED_HANDLER2 = 28,
  MODEL_SET_SEQUENCE1 = 29,
  MODEL_SET_SEQUENCE2 = 30,
  MODEL_SET_SEQUENCE_FIDGET1 = 31,
  MODEL_SET_SEQUENCE_FIDGET2 = 32,
  MODEL_SET_TIME_SCALE = 33,
  MODEL_SET_VERTEX_ALPHA = 34,
  MODEL_SET_VERTEX_COLOR = 35,
  MODEL_SHOW_BOUNDING_SPHERE = 36,
  MODEL_SHOW_COLLISION = 37,
  MODEL_SHOW_COLLISION_AABOX = 38,
  MODEL_SHOW_MODEL = 39,
  MODEL_NUM_COMMANDS = 40,
  MODEL_COMMAND_NOT_QUEUED = MODEL_NUM_COMMANDS
};

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
  COLLIDE_TYPE       type;
  NTempest::C3Vector extent[2];
  float              radius;
};

struct CPrimitive {
  CPrimitive() : type(GxPrim_Triangles), vertexCount(0) {
  }

  EGxPrim      type;
  unsigned int vertexCount;
};

typedef TSFixedArray_<NTempest::C2Vector, 'IMod', 266> CModelTexCoordArray;

struct CGeosetShared {
  CGeosetShared()
      : vertexShader(GxVS_PassThru),
        materialId(0),
        centroid(0.0f),
        radius(0.0f),
        selectionGroup(0),
        geosetId(0),
        flags(0) {
  }
  TSFixedArray_<NTempest::C3Vector, 'IMod', 276>  position;
  TSFixedArray_<unsigned char, 'IMod', 277>       boneWeights;
  TSFixedArray_<NTempest::C3Vector, 'IMod', 278>  normal;
  TSFixedArray_<CModelTexCoordArray, 'IMod', 279> texCoord;
  TSFixedArray_<CPrimitive, 'IMod', 280>          primitive;
  TSFixedArray_<unsigned short, 'IMod', 281>      primitiveVertices;
  TSFixedArray_<unsigned int, 'IMod', 282>        groupMatrixCounts;
  TSFixedArray_<unsigned int, 'IMod', 283>        matrices;
  TSFixedArray_<unsigned int, 'IMod', 284>        hwBoneIndices;
  TSFixedArray_<unsigned int, 'IMod', 285>        hwBoneWeights;
  EGxVertexShader                                 vertexShader;
  unsigned int                                    materialId;
  NTempest::C3Vector                              centroid;
  float                                           radius;
  unsigned int                                    selectionGroup;
  unsigned int                                    geosetId;
  unsigned int                                    flags;
};

struct CGeoset {
  CGeoset() : weightedBones(GetInvalidMatrixId()), flags(0) {
  }

  unsigned int weightedBones;
  unsigned int flags;
};

struct CCustomGeoset {
  NTempest::C3Vector position;
  void(*renderCallback)(HMODEL, const NTempest::C34Matrix &, void *);
  void *renderParam;
};

struct CModelTexture {
  CModelTexture() : handle(0), replaceableId(0) {
  }
  CModelTexture(const CModelTexture &source);
  ~CModelTexture() {
    if (handle) {
      HandleClose(handle);
    }
  }
  CModelTexture &operator=(const CModelTexture &source);

  HTEXTURE     handle;
  unsigned int replaceableId;
};

class CModelBase {
 public:
  CModelBase(unsigned int flags)
      : m_PickLights(GxuLightSelectCallback),
        m_pickLightsParm(0),
        m_flags(flags),
        m_modelToWorld(),
        m_texBones(GetInvalidMatrixId()),
        m_anim(0),
        m_boundsModel(0),
        m_aaBoxCustGeoId(static_cast<unsigned int>(-1)),
        m_collideModel(0) {
  }
  ~CModelBase();

  void(*m_PickLights)(void *, NTempest::C3Vector, const NTempest::C3Vector &, unsigned int);
  void               *m_pickLightsParm;
  unsigned int        m_flags;
  NTempest::C34Matrix m_modelToWorld;
  unsigned int        m_texBones;
  HANIM               m_anim;
  HMODEL              m_boundsModel;
  unsigned int        m_aaBoxCustGeoId;
  HMODEL              m_collideModel;

 protected:
  CModelBase(const CModelBase &source);

 private:
  CModelBase &operator=(const CModelBase &source);
};

class CModelSimple : public CModelBase {
 public:
  CModelSimple() : CModelBase(0) {
    m_custGeosets.SetCount(0);
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
      HandleClose(reinterpret_cast<HOBJECT>(collision));
    }
  }

  virtual const char *GetObjectName() {
    return name;
  }

  TSFixedArray<CBoundsData>                      seqBounds;
  TSFixedArray<unsigned int>                     attachIdToIndex;
  TSFixedArray_<NTempest::C3Vector, 'IMod', 376> positions;
  TSFixedArray<CHitTest>                         hitTest;
  TSFixedArray<CGeosetShared>                    geosets;
  TSFixedArray<unsigned int>                     emitter2Order;
  TSFixedArray<unsigned int>                     ribbonOrder;
  unsigned int                                   numBones;
  unsigned int                                   numTexBones;
  GROUND_TRACK                                   groundTrack;
  HCOLLISIONDATA                                 collision;
  char                                           name[260];
  CBoundsData                                    bounds;
  unsigned char                                  numGeosets;
  unsigned char                                  numLayers;

 private:
};

void ModelEnableLights(HMODEL model, int enable);
void ModelShowBoundingSphere(HMODEL model);
void ModelShowBoundingBox(HMODEL model);
int ModelGeosetAdd(
    HMODEL                    model,
    unsigned int              numVertices,
    const NTempest::C3Vector *position,
    const NTempest::C3Vector *normal,
    const NTempest::C2Vector *texCoord,
    EGxPrim                   primitiveType,
    const unsigned short     *primitiveVertices,
    unsigned int              numPrimVertices,
    HTEXTURE                  texture,
    EGxBlend                  blendMode,
    unsigned int              disables,
    NTempest::CImVector       color,
    unsigned int              replaceableId
);
HMODEL ModelCreateSimpleMesh(
    const char               *name,
    unsigned int              numVertices,
    const NTempest::C3Vector *position,
    const NTempest::C3Vector *normal,
    const NTempest::C2Vector *texCoord,
    EGxPrim                   primitiveType,
    const unsigned short     *primitiveVertices,
    unsigned int              numPrimVertices,
    HTEXTURE                  texture,
    EGxBlend                  blendMode,
    unsigned int              disables,
    NTempest::CImVector       color,
    unsigned int              replaceableId
);
HMODEL CreateModelBoundingBox(const NTempest::CAaBox &bounds, HTEXTURE texture, EGxBlend blendMode);

struct LINKUNIQUE : public TSLinkedNode<LINKUNIQUE> {
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
  CModelComplex(const CModelSimple &source);
  CModelComplex(const CModelComplex &source);
  ~CModelComplex();

  TSGrowableArray<CGeoset>                                  m_geosets;
  TSGrowableArray<CGeosetShared>                            m_addlGeosets;
  TSGrowableArray<CGeosetColor>                             m_geosetColor;
  TSGrowableArray<CCustomGeoset>                            m_custGeosets;
  TSGrowableArray<HMATERIAL>                                m_materials;
  TSGrowableArray<CModelTexture>                            m_textures;
  TSFixedArray<unsigned long>                               m_lights;
  TSFixedArray<TSList<LINKUNIQUE, TSGetLink<LINKUNIQUE> > > m_attached;
  TSFixedArray_<unsigned char, 'MDLF', 484>                 m_attachmentFlags;
  TSFixedArray<CParticleEmitter2 *>                         m_emitters2;
  TSFixedArray<CRibbonEmitter *>                            m_ribbons;
  TSFixedArray<HCAMERA>                                     m_cameras;
  TSFixedArray<unsigned int>                                m_cameraOrder;
  TSFixedArray<NTempest::C34Matrix>                         m_hitTestMtx;

 private:
  CModelComplex &operator=(const CModelSimple &source);
  CModelComplex &operator=(const CModelComplex &source);
  void           CopyAttachments(const CModelComplex &source);
  void           CopyCameras(const CModelComplex &source);
  void           CopyLights(const CModelComplex &source);
  void           CopyEmitters(const CModelComplex &source);
  void           CopyRibbons(const CModelComplex &source);
};

class CModelModItem : public TSLinkedNode<CModelModItem> {
 public:
  EModelModQ    action;
  unsigned char paramData[16];
};

class CModel : public CHandleObject {
 public:
  CModel(EModelLoad state = CMODEL_ASYNC_WAIT) : asyncObject(0), createData(0), shared(0), state(state) {
  }
  CModel(CModel &source);
  virtual ~CModel();

  void DeleteAsyncObj();
  void FinishDuplication(CModel &source);

  union {
    CAsyncObject *asyncObject;
    CModelBase   *data;
    HMODEL        dupSource;
  };
  CModelCreate                                    *createData;
  HMODELSHARED                                     shared;
  EModelLoad                                       state;
  TSList<CModelModItem, TSGetLink<CModelModItem> > modelModQueue;

 private:
  void RemoveModelCommandsFromQueue();
};

struct CModelRenderData {
  CGeoset       *m_geosets;
  CGeosetColor  *m_geosetColor;
  unsigned int   m_numGeosets;
  HMATERIAL     *m_materials;
  CModelTexture *m_textures;
  CModel        *m_model;
  CModelShared  *m_shared;
  unsigned int   m_renderFlags;
};

void                            EnqueueModelCommand(CModel *model, EModelModQ command, ...);
HMATERIAL BuildSimpleMaterial(
    CModelTexture *modelTexture,
    unsigned int textureId,
    HTEXTURE texture,
    EGxBlend blendMode,
    unsigned int disables,
    unsigned int replaceableId
);
unsigned int MatrixAlloc(unsigned int numMatrices);
NTempest::C34Matrix *MatrixDeref(unsigned int handle);
int IModelDerefHandle(CModel *model, CModelBase **unique, CModelShared **shared);
int IModelDerefHandle(CModel *model, CModelBase **unique);
int IModelDerefHandle(CModel *model, CModelShared **shared);

void MdxReadCameras(unsigned char *data, unsigned int fileBytes, TSFixedArray<HCAMERA> *cameras);

void MdxReadLights(unsigned char *data, unsigned int fileBytes, CModelComplex *modelptr);

HCOLLISIONDATA CollisionDataCreate(unsigned char *fileData, unsigned int fileBytes);

void
MdxReadAttachments(unsigned char *data, unsigned int fileBytes, unsigned int flags, CModelComplex *modelptr, CModelShared *shared, CStatus *status);

void MdxReadRibbonEmitters(unsigned char *data, unsigned int fileBytes, CModelComplex *modelptr, CModelShared *shared);

#endif
