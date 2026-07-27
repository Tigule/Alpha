#include "ModelInternal.h"
#include "CollisionData.h"

#include "Base/Activity.h"
#include "Anim/WorldMatrix.h"
#include "Services/ParticleSystem2.h"
#include "Services/RibbonEmitter.h"
#include "Services/Texture.h"
#include "Tempest/cpriorityq.h"
#include "Tempest/c4quaternion.h"

#include <math.h>
#include <stddef.h>
#include <float.h>

enum SORTABLES {
  SORTOBJ_GEOSET = 0,
  SORTOBJ_EMITTER2 = 1,
  SORTOBJ_RIBBON = 2,
  SORTOBJ_CUSTOM_GEO = 3,
  SORTOBJ_CUSTOM_MODEL = 4
};

struct COpaqueLayer;

static int CompareTexLayers(COpaqueLayer *a, COpaqueLayer *b);
static CModelTexture *GetModelTextures(CModelBase *unique);
static void RestoreFog();
static void SaveFog();
static CModelTexture *GetTextureList(CModelBase *modelptr);
static void ClearUvTransforms(const CTexLayer &layerUnique, const CTexLayerShared &layerShared);
static unsigned int GetNumTexCoordLayers(const CTexLayer &layerUnique, const CTexLayerShared &layerShared);
static void ClearTransformedUVLayer(const CTexLayerShared &layerShared, unsigned int tmu);
static void RenderSingleUVMapPrep(
    CModelRenderData      *modelptr,
    CGeosetShared         *geoShared,
    const CMaterial       &uniqueMtl,
    const CMaterialShared &sharedMtl,
    unsigned int           layerId,
    int                    geosetChanged
);
static void SetUvTransforms(CModelBase *modelptr, const CTexLayer &layerUnique, const CTexLayerShared &layerShared);
static void GetTransformedUVLayer(CModelBase *modelptr, const CTexLayerShared &layerShared, unsigned int tmu);
static void LockVertsAndIndices(CGeosetShared *geoShared, const CTexLayer &uniqueLayer, const CTexLayerShared &sharedLayer);
static void LockVertices(CGeosetShared *geoShared, const CTexLayer &uniqueLayer, const CTexLayerShared &sharedLayer);
static void RenderGeosetPrep(CModelBase *modelptr, CGeoset *geoUnique, CGeosetShared *geoShared);
static void RenderGeosetSingleLayer(
    CModelRenderData *modelptr,
    CGeoset          *geoUnique,
    CGeosetShared    *geoShared,
    unsigned int      layerId,
    int               materialChanged,
    int               geosetChanged,
    CStatus          *status
);
static void RenderGeosetMultiUvMapping(CModelRenderData *modelptr, CGeosetShared *geoShared, CMaterial *uniqueMtl, CStatus *status);
static void RenderUniformUVMapLayers(
    CModelRenderData          *modelptr,
    CGeosetShared             *geoShared,
    const CMaterial           &uniqueMtl,
    const CMaterialShared     &sharedMtl,
    const NTempest::CImVector &color,
    CStatus                   *status
);
static void RenderGeosetOneUvMapping(
    CModelRenderData *modelptr,
    CGeosetShared    *geoShared,
    CMaterial        *uniqueMtl,
    unsigned int      firstLayerId,
    int               geosetChanged,
    CStatus          *status
);
static void RenderSortedGeoset(
    CModelRenderData *modelptr,
    CGeoset          *geoUnique,
    CGeosetShared    *geoShared,
    unsigned int      firstLayerId,
    int               geosetChanged,
    CStatus          *status
);
static void Project2d(CGeosetShared *geoShared, const NTempest::CImVector &color);
static int SingleUvMapping(CMaterial *uniqueMtl);
static void RenderGeosetOneUvMapping(CModelRenderData *modelptr, CGeosetShared *geoShared, CMaterial *uniqueMtl, CStatus *status);
static void RenderGeosetLayers(CModelRenderData *modelptr, CGeosetShared *geoShared, CStatus *status);
static void RenderGeoset(CModelRenderData *modelptr, CGeoset *geoUnique, CGeosetShared *geoShared, CStatus *status);
static void IModelRenderSceneOpaque(CStatus *status);
static void IModelRenderSceneTransparent(CStatus *status);
static void ModelBaseRender(CModelBase *modelptr);
static void RenderGeosetCheckVis(CModelRenderData *modelptr, CGeoset *geoUnique, CGeosetShared *geoShared, CStatus *status);
static void GeosetComplexRender(CModel *model, CGeoset *geoUnique, CGeosetShared *geoShared, unsigned int renderFlags, CStatus *status);
static void ModelComplexRender(HMODEL modelHandle, CModel *model, unsigned int renderFlags, CStatus *status);
static void ModelSimpleRender(CModel *model, unsigned int renderFlags, CStatus *status);
static int IModelTestRay(
    CModelBase                    *modelptr,
    CModelShared                  *shared,
    const NTempest::C3Vector      &rayStart,
    const NTempest::C3Vector      &rayEnd,
    float                         *distance,
    int                            testLinkedModels
);
static void SetVertexMatrixIndices(CGeosetShared *geoShared, const TSGrowableArray<unsigned int> &groupVertexCounts);
static void BuildComplexGeoset(
    const TSGrowableArray<NTempest::C3Vector> &position,
    const TSGrowableArray<NTempest::C3Vector> &normal,
    const TSGrowableArray<NTempest::C2Vector> &texCoord,
    const TSGrowableArray<unsigned short> &primitiveVertices,
    const TSGrowableArray<unsigned int> &groupVertex,
    const TSGrowableArray<unsigned int> &groupCounts,
    const TSGrowableArray<unsigned int> &matrices,
    const TSGrowableArray<CPrimitive> &primitives,
    unsigned int materialId,
    unsigned int geosetId,
    CGeosetShared *geoShared
);
static void IModelGeosetAdd(
    CModelComplex *modelptr,
    CModelShared *shared,
    const TSGrowableArray<NTempest::C3Vector> &position,
    const TSGrowableArray<NTempest::C3Vector> &normal,
    const TSGrowableArray<NTempest::C2Vector> &texCoord,
    const TSGrowableArray<unsigned short> &primitiveVertices,
    const TSGrowableArray<unsigned int> &groupVertex,
    const TSGrowableArray<unsigned int> &groupCounts,
    const TSGrowableArray<unsigned int> &matrices,
    const TSGrowableArray<CPrimitive> &primitives,
    HTEXTURE texture,
    EGxBlend blendMode,
    unsigned int disables,
    NTempest::CImVector color
);
static void IModelHandleGeosetAdd(
    CModel *model,
    const TSGrowableArray<NTempest::C3Vector> &position,
    const TSGrowableArray<NTempest::C3Vector> &normal,
    const TSGrowableArray<NTempest::C2Vector> &texCoord,
    const TSGrowableArray<unsigned short> &primitiveVertices,
    const TSGrowableArray<unsigned int> &groupVertex,
    const TSGrowableArray<unsigned int> &groupCounts,
    const TSGrowableArray<unsigned int> &matrices,
    const TSGrowableArray<CPrimitive> &primitives,
    HTEXTURE texture,
    EGxBlend blendMode,
    unsigned int disables,
    NTempest::CImVector color
);
static void CreatePlanarQuadGeometry(
    const NTempest::C3Vector &base,
    float length,
    float width,
    TSGrowableArray<NTempest::C3Vector> *positions,
    TSGrowableArray<NTempest::C3Vector> *normals,
    TSGrowableArray<NTempest::C2Vector> *texCoords,
    TSGrowableArray<unsigned short> *vertIndices,
    TSGrowableArray<CPrimitive> *primitives
);
static int CreateCylinderGeometry(
    const NTempest::C3Vector &base,
    const NTempest::C3Vector &height,
    float radius,
    TSGrowableArray<NTempest::C3Vector> *vertices,
    TSGrowableArray<NTempest::C3Vector> *normals,
    TSGrowableArray<NTempest::C2Vector> *texCoords,
    TSGrowableArray<unsigned short> *vertIndices,
    TSGrowableArray<CPrimitive> *primitives
);
static void GenerateCylinderVerts(
    const NTempest::C3Vector &base,
    const NTempest::C3Vector &height,
    float radius,
    unsigned int segments,
    TSGrowableArray<NTempest::C3Vector> *vertices,
    TSGrowableArray<NTempest::C3Vector> *normals
);

struct COpaqueLayer {
  COpaqueLayer() : model(0) {
  }

  static bool HasHigherPriority(COpaqueLayer *a, COpaqueLayer *b);

  CGeoset       *geoUnique;
  CGeosetShared *geoShared;
  HMODEL         model;
  unsigned int   flags;
  unsigned int   firstLayer;
  unsigned int   passNumber;
  CTexLayer     *layer;
};

struct CTransparentObject {
  CTransparentObject() : sortType(SORTOBJ_GEOSET) {
    geo.model = 0;
  }

  static bool HasHigherPriority(CTransparentObject *a, CTransparentObject *b);

  SORTABLES sortType;
  int       priorityPlane;
  float     sqDistFromCamera;
  union {
    struct {
      HMODEL         model;
      CGeoset       *geoUnique;
      CGeosetShared *geoShared;
    } geo;
    struct {
      HMODEL model;
      void  *object;
    } stnd;
    struct {
      void(*callback)(void *, int);
      void *param1;
      int   param2;
    } cust;
  };
};

static unsigned short                                     vertIndices[36] = {0,  1,  2,  2,  1,  3,  4,  5,  6,  6,  5,  7,  8,  9,  10, 10, 9,  11,
                                                                             12, 13, 14, 14, 13, 15, 16, 17, 18, 18, 17, 19, 20, 21, 22, 22, 21, 23};
static TSGrowableArray<COpaqueLayer>                      s_opLayerPool;
static TSGrowableArray<CTransparentObject>                s_trLayerPool;
static NTempest::C4Vector                                 s_frustumPlanes[6];
static const float                                        TWO_PI = 6.28318530717958647692f;
static const float                                        OO_TWO_PI = 0.15915494309189533577f;
static TSGrowableArray<NTempest::C34Matrix>               s_matrixPool;
static unsigned int                                       s_nextMatrix;
static unsigned int                                       s_lastFrame;
static int                                                s_verticesLocked;
static NTempest::CImVector                                s_fogColor;
static float                                              s_fogStart;
static float                                              s_fogEnd;
static float                                              s_fogDensity;
static int                                                s_fogStyle;
static MODELPROJECT2DCALLBACK                             s_Project2dCallback;
static NTempest::C3Vector                                 s_sceneCameraPos;
static NTempest::C3Vector                                 s_sceneCameraDir;
static float                                              s_sceneSharpness = -1.0f;
static const float                                        PI = 3.14159265358979323846f;
static NTempest::CPriorityQ<COpaqueLayer *, COpaqueLayer> s_opaqueScene;
static unsigned short                                     s_currAnimFrame;
static NTempest::CPriorityQ<CTransparentObject *, CTransparentObject> s_transparentScene;

static void IModelComplexAddToScene(CModel *model, unsigned int renderFlags);
static void
EnqueueSimpleObject(CModel *model, void *object, SORTABLES sortType, const NTempest::C3Vector &position, unsigned int priorityPlane);
static void AddAllGeosetsToScene(
    CModel       *modelptr,
    CModelShared *shared,
    unsigned int  renderFlags,
    CGeoset      *geosets,
    CGeosetColor *geosetColor,
    unsigned int  numGeosets,
    HMATERIAL    *materials,
    unsigned int  numMaterials
);
static void AddGeosetToScene(
    CModel        *modelptr,
    unsigned int   renderFlags,
    CGeoset       *geoUnique,
    CGeosetShared *geoShared,
    CGeosetColor  *geosetsColor,
    HMATERIAL     *materials,
    unsigned int   numMaterials
);
static int IsOpaque(CMaterial *uniqueMtl);
static void
EnqueueTransparentGeoset(CModel *model, CGeoset *geoUnique, CGeosetShared *geoShared, const NTempest::C3Vector &position, unsigned int priorityPlane);
static NTempest::C3Vector GetGeosetSortPos(CModelBase *modelUnique, CGeosetShared *geoShared);
static void AddRibbonsToScene(CModel *modelptr, CModelShared *shared);
static void IModelBaseAddToScene(CModelBase *modelptr, CModelShared *shared);
static void IModelSimpleAddToScene(CModel *model, unsigned int renderFlags);

void ModelRenderSceneLogStop();

int CTexLayer::Compare(const CModelTexture *aTextures, const CModelTexture *bTextures, const CTexLayer &a, const CTexLayer &b) {
  if (a.vertexFormat != b.vertexFormat) {
    return a.vertexFormat - b.vertexFormat;
  }

  if (a.disables != b.disables) {
    return a.disables - b.disables;
  }

  if (a.blendMode != b.blendMode) {
    return a.blendMode - b.blendMode;
  }

  for (unsigned int tmu = 0; tmu < 2; ++tmu) {
    if (a.tmuPass[tmu].combiner != b.tmuPass[tmu].combiner) {
      return a.tmuPass[tmu].combiner - b.tmuPass[tmu].combiner;
    }

    HOBJECT bTexture =
        b.tmuPass[tmu].textureId == static_cast<unsigned int>(-1) ? 0 : reinterpret_cast<HOBJECT>(bTextures[b.tmuPass[tmu].textureId].handle);
    HOBJECT aTexture =
        a.tmuPass[tmu].textureId == static_cast<unsigned int>(-1) ? 0 : reinterpret_cast<HOBJECT>(aTextures[a.tmuPass[tmu].textureId].handle);

    int result = HandleObjectCompare(aTexture, bTexture);
    if (result) {
      return result;
    }
  }

  return 0;
}

static void SaveFog() {
  GxRsGet(GxRs_FogColor, s_fogColor);
  GxRsGet(GxRs_FogStart, s_fogStart);
  GxRsGet(GxRs_FogEnd, s_fogEnd);
  GxRsGet(GxRs_FogDensity, s_fogDensity);
  GxRsGet(GxRs_FogStyle, s_fogStyle);
}

static void RestoreFog() {
  GxRsSet(GxRs_FogColor, s_fogColor);
  GxRsSet(GxRs_FogStart, s_fogStart);
  GxRsSet(GxRs_FogEnd, s_fogEnd);
  GxRsSet(GxRs_FogDensity, s_fogDensity);
  GxRsSet(GxRs_FogStyle, s_fogStyle);
}

static CModelTexture *GetModelTextures(CModelBase *unique) {
  if (unique->m_flags & 0x20) {
    return static_cast<CModelComplex *>(unique)->m_textures.Ptr();
  }

  return static_cast<CModelSimple *>(unique)->m_textures.Ptr();
}

static int CompareTexLayers(COpaqueLayer *a, COpaqueLayer *b) {
  ASSERT(a);
  ASSERT(b);

  CModelTexture *aTextures;
  CModelTexture *bTextures;
  CModelBase    *modelB;
  CModelBase    *modelA;
  IModelDerefHandle(reinterpret_cast<CModel *>(a->model), &modelA);
  IModelDerefHandle(reinterpret_cast<CModel *>(b->model), &modelB);

  aTextures = GetModelTextures(modelA);
  bTextures = GetModelTextures(modelB);
  int result = CTexLayer::Compare(aTextures, bTextures, *a->layer, *b->layer);
  if (result) {
    return result;
  }

  return (a->flags & 0xF) <= (b->flags & 0xF);
}

bool CTransparentObject::HasHigherPriority(CTransparentObject *a, CTransparentObject *b) {
  if (a->priorityPlane == b->priorityPlane) {
    return a->sqDistFromCamera >= b->sqDistFromCamera;
  }

  return a->priorityPlane <= b->priorityPlane;
}

bool COpaqueLayer::HasHigherPriority(COpaqueLayer *a, COpaqueLayer *b) {
  bool aSpecial = (a->flags & 0x3000) == 0x3000;
  bool bSpecial = (b->flags & 0x3000) == 0x3000;

  if (aSpecial != bSpecial) {
    return aSpecial;
  }
  if (a->geoShared != b->geoShared) {
    return a->geoShared < b->geoShared;
  }
  if (a->passNumber != b->passNumber) {
    return a->passNumber < b->passNumber;
  }

  return CompareTexLayers(a, b) <= 0;
}

static int IsOpaque(CMaterial *uniqueMtl) {
  ASSERT(uniqueMtl);

  TSGrowableArray<CTexLayer> &layers = uniqueMtl->layers;
  unsigned int                numLayers = layers.Count();
  unsigned int                layerIndex;
  for (layerIndex = 0; layerIndex < numLayers; ++layerIndex) {
    if (layers[layerIndex].layerAlpha) {
      break;
    }
  }

  if (layerIndex == numLayers) {
    return 1;
  }
  if (layers[layerIndex].blendMode >= GxBlend_Alpha) {
    return 0;
  }
  return !(layers[layerIndex].disables & 4);
}

static void
EnqueueSimpleObject(CModel *model, void *object, SORTABLES sortType, const NTempest::C3Vector &position, unsigned int priorityPlane) {
  float               sqDistFromCamera = (position - s_sceneCameraPos).SquaredMag();
  CTransparentObject *sceneObject = s_trLayerPool.New();

  sceneObject->sqDistFromCamera = sqDistFromCamera;
  sceneObject->sortType = sortType;
  sceneObject->priorityPlane = priorityPlane;
  sceneObject->stnd.object = object;
  sceneObject->stnd.model = reinterpret_cast<HMODEL>(HandleCreate(model, "HMODEL"));
}

static void EnqueueTransparentGeoset(
    CModel                   *model,
    CGeoset                  *geoUnique,
    CGeosetShared            *geoShared,
    const NTempest::C3Vector &position,
    unsigned int              priorityPlane
) {
  float               sqDistFromCamera = (position - s_sceneCameraPos).SquaredMag();
  CTransparentObject *sceneObject = s_trLayerPool.New();

  sceneObject->sqDistFromCamera = sqDistFromCamera;
  sceneObject->geo.geoShared = geoShared;
  sceneObject->priorityPlane = priorityPlane;
  sceneObject->sortType = SORTOBJ_GEOSET;
  sceneObject->geo.geoUnique = geoUnique;
  sceneObject->geo.model = reinterpret_cast<HMODEL>(HandleCreate(model, "HMODEL"));
}

static NTempest::C3Vector GetGeosetSortPos(CModelBase *modelUnique, CGeosetShared *geoShared) {
  NTempest::C3Vector sortPos = geoShared->centroid * modelUnique->m_modelToWorld;

  if (geoShared->flags & 4) {
    sortPos.x -= s_sceneCameraDir.x * geoShared->radius;
    sortPos.y -= s_sceneCameraDir.y * geoShared->radius;
    sortPos.z -= s_sceneCameraDir.z * geoShared->radius;
  } else if (geoShared->flags & 8) {
    sortPos.x += s_sceneCameraDir.x * geoShared->radius;
    sortPos.y += s_sceneCameraDir.y * geoShared->radius;
    sortPos.z += s_sceneCameraDir.z * geoShared->radius;
  }

  return sortPos;
}

static void AddGeosetToScene(
    CModel        *modelptr,
    unsigned int   renderFlags,
    CGeoset       *geoUnique,
    CGeosetShared *geoShared,
    CGeosetColor  *geosetsColor,
    HMATERIAL     *materials,
    unsigned int   numMaterials
) {
  if ((geoUnique->flags & 1) || (geoShared->vertexShader != GxVS_PassThru && !MatrixDeref(geoUnique->weightedBones)) ||
      !geosetsColor[geoShared->geosetId].animatedColor.a)
  {
    return;
  }

  if (geoShared->materialId >= numMaterials) {
    FATALERROR(("AddGeosetToScene: geoShared->materialId (%d) >= numMaterials (%d)", geoShared->materialId, numMaterials));
  }

  CMaterial *uniqueMtl = reinterpret_cast<CMaterial *>(materials[geoShared->materialId]);
  ASSERT(uniqueMtl);
  CMaterialShared *sharedMtl = reinterpret_cast<CMaterialShared *>(uniqueMtl->data);
  ASSERT(sharedMtl);

  int opaque = IsOpaque(uniqueMtl);
  if (!opaque && !(renderFlags & 3)) {
    NTempest::C3Vector position = GetGeosetSortPos(static_cast<CModelBase *>(modelptr->data), geoShared);
    EnqueueTransparentGeoset(modelptr, geoUnique, geoShared, position, sharedMtl->priorityPlane);
  }

  if (!opaque && ((renderFlags & 1) || !(renderFlags & 2))) {
    return;
  }

  unsigned int numLayers = uniqueMtl->layers.Count();
  unsigned int passNumber = 0;
  unsigned int layerIndex;
  for (layerIndex = 0; layerIndex < numLayers; ++layerIndex) {
    if (!uniqueMtl->layers[layerIndex].layerAlpha) {
      continue;
    }

    COpaqueLayer *opaqueLayer = s_opLayerPool.New();
    opaqueLayer->geoUnique = geoUnique;
    opaqueLayer->geoShared = geoShared;
    opaqueLayer->layer = &uniqueMtl->layers[layerIndex];
    opaqueLayer->flags = renderFlags;
    opaqueLayer->firstLayer = layerIndex;
    opaqueLayer->model = reinterpret_cast<HMODEL>(HandleCreate(modelptr, "HMODEL"));

    if (geoShared->vertexShader != GxVS_PassThru) {
      opaqueLayer->flags |= 0x1000;
    }
    if (renderFlags & 2) {
      return;
    }

    opaqueLayer->passNumber = passNumber++;
    if (opaqueLayer->passNumber) {
      opaqueLayer->flags |= 0x2000;
    } else {
      unsigned int j;
      for (j = layerIndex + 1; j < numLayers; ++j) {
        if (uniqueMtl->layers[j].layerAlpha) {
          opaqueLayer->flags |= 0x2000;
          break;
        }
      }
      if (opaqueLayer->flags & 0x1000) {
        return;
      }
    }
  }
}

static void AddAllGeosetsToScene(
    CModel       *modelptr,
    CModelShared *shared,
    unsigned int  renderFlags,
    CGeoset      *geosets,
    CGeosetColor *geosetColor,
    unsigned int  numGeosets,
    HMATERIAL    *materials,
    unsigned int  numMaterials
) {
  unsigned int index;

  for (index = 0; index < shared->numGeosets; ++index) {
    AddGeosetToScene(modelptr, renderFlags, &geosets[index], &shared->geosets[index], geosetColor, materials, numMaterials);
  }

  unsigned int numAddlGeosets = numGeosets - shared->numGeosets;
  for (index = 0; index < numAddlGeosets; ++index) {
    AddGeosetToScene(
        modelptr, renderFlags, &geosets[shared->numGeosets + index], &static_cast<CModelComplex *>(modelptr->data)->m_addlGeosets[index], geosetColor,
        materials, numMaterials
    );
  }
}

static void AddEmitters2ToScene(CModel *modelptr, CModelShared *shared) {
  NTempest::C3Vector center;
  unsigned int       numEmitters = static_cast<CModelComplex *>(modelptr->data)->m_emitters2.Count();
  unsigned int       index;

  for (index = 0; index < numEmitters; ++index) {
    center = shared->positions[shared->emitter2Order[index]] * modelptr->data->m_modelToWorld;
    EnqueueSimpleObject(
        modelptr, static_cast<CModelComplex *>(modelptr->data)->m_emitters2[index], SORTOBJ_EMITTER2, center,
        static_cast<CModelComplex *>(modelptr->data)->m_emitters2[index]->PriorityPlane()
    );
  }
}

static void AddRibbonsToScene(CModel *modelptr, CModelShared *shared) {
  NTempest::C3Vector center;
  unsigned int       numEmitters = static_cast<CModelComplex *>(modelptr->data)->m_ribbons.Count();
  unsigned int       index;

  for (index = 0; index < numEmitters; ++index) {
    center = shared->positions[shared->ribbonOrder[index]] * modelptr->data->m_modelToWorld;
    EnqueueSimpleObject(modelptr, static_cast<CModelComplex *>(modelptr->data)->m_ribbons[index], SORTOBJ_RIBBON, center, 0);
  }
}

static unsigned int GetNumTexCoordLayers(const CTexLayer &layerUnique, const CTexLayerShared &layerShared) {
  unsigned int numLayers = 0;

  for (unsigned int tmu = 0; tmu < 2; ++tmu) {
    numLayers +=
        layerUnique.tmuPass[tmu].textureId != static_cast<unsigned int>(-1) && layerShared.tmuPass[tmu].coordId != static_cast<unsigned int>(-1);
  }

  return numLayers;
}

static void GetTransformedUVLayer(CModelBase *modelptr, const CTexLayerShared &layerShared, unsigned int tmu) {
  EGxTextureShader textureShader = layerShared.tmuPass[tmu].textureShader;
  GxRsSet(static_cast<EGxRenderState>(GxRs_TextureShader0 + tmu), textureShader);

  if (textureShader >= GxTS_Affine) {
    NTempest::C34Matrix *texBones = MatrixDeref(modelptr->m_texBones);
    ASSERT(texBones);

    const NTempest::C34Matrix &transform = texBones[layerShared.tmuPass[tmu].transformId];
    NTempest::C44Matrix        matrix(
        transform.a0, transform.a1, transform.a2, 0.0f, transform.b0, transform.b1, transform.b2, 0.0f, transform.c0, transform.c1, transform.c2,
        0.0f, transform.d0, transform.d1, transform.d2, 1.0f
    );
    GxXformPush(static_cast<EGxXform>(GxXform_Tex0 + tmu), matrix);
  }
}

static void ClearTransformedUVLayer(const CTexLayerShared &layerShared, unsigned int tmu) {
  EGxTextureShader textureShader = layerShared.tmuPass[tmu].textureShader;
  GxRsSet(static_cast<EGxRenderState>(GxRs_TextureShader0 + tmu), textureShader);

  if (textureShader >= GxTS_Affine) {
    GxXformPop(static_cast<EGxXform>(GxXform_Tex0 + tmu));
  }
}

static void SetUvTransforms(CModelBase *modelptr, const CTexLayer &layerUnique, const CTexLayerShared &layerShared) {
  switch (GetNumTexCoordLayers(layerUnique, layerShared)) {
    case 1:
      if (layerUnique.tmuPass[1].textureId != static_cast<unsigned int>(-1) && layerShared.tmuPass[1].coordId != static_cast<unsigned int>(-1)) {
        GetTransformedUVLayer(modelptr, layerShared, 1);
      } else {
        GetTransformedUVLayer(modelptr, layerShared, 0);
      }
      break;

    case 2:
      GetTransformedUVLayer(modelptr, layerShared, 0);
      GetTransformedUVLayer(modelptr, layerShared, 1);
      break;
  }
}

static void ClearUvTransforms(const CTexLayer &layerUnique, const CTexLayerShared &layerShared) {
  switch (GetNumTexCoordLayers(layerUnique, layerShared)) {
    case 1:
      if (layerUnique.tmuPass[1].textureId != static_cast<unsigned int>(-1) && layerShared.tmuPass[1].coordId != static_cast<unsigned int>(-1)) {
        ClearTransformedUVLayer(layerShared, 1);
      } else {
        ClearTransformedUVLayer(layerShared, 0);
      }
      break;

    case 2:
      ClearTransformedUVLayer(layerShared, 0);
      ClearTransformedUVLayer(layerShared, 1);
      break;
  }
}

static void LockVertices(CGeosetShared *geoShared, const CTexLayer &uniqueLayer, const CTexLayerShared &sharedLayer) {
  const NTempest::C2Vector *texCoords[2];

  for (unsigned int tmu = 0; tmu < 2; ++tmu) {
    unsigned int coordId = sharedLayer.tmuPass[tmu].coordId;
    if (uniqueLayer.tmuPass[tmu].textureId == static_cast<unsigned int>(-1) || coordId == static_cast<unsigned int>(-1)) {
      texCoords[tmu] = 0;
    } else {
      texCoords[tmu] = geoShared->texCoord[coordId].Ptr();
    }
  }

  GxPrimLockVertexPtrs(
      geoShared->position.Count(), geoShared->position.Ptr(), sizeof(NTempest::C3Vector), geoShared->normal.Ptr(), sizeof(NTempest::C3Vector), 0, 0,
      geoShared->boneWeights.Ptr(), geoShared->boneWeights.Count() != 0, texCoords[0], sizeof(NTempest::C2Vector), texCoords[1],
      sizeof(NTempest::C2Vector)
  );
}

static void Project2d(CGeosetShared *geoShared, const NTempest::CImVector &color) {
  ASSERT(s_Project2dCallback);
  ASSERT(geoShared->position.Count() == 4);

  NTempest::C44Matrix worldMtx;
  GxXform(GxXform_World, worldMtx);

  NTempest::C3Vector xf[4];
  for (unsigned int i = 0; i < 4; ++i) {
    xf[i] = s_sceneCameraPos + geoShared->position[i] * worldMtx;
  }

  NTempest::CAaBox worldBox = NTempest::CAaBox::Bounding(xf, 4);
  worldBox.b.z -= 3.0f;
  worldBox.t.z += 3.0f;

  NTempest::CAaBox localBox = NTempest::CAaBox::Bounding(&geoShared->position[0], 4);

  float scale = worldMtx.a0 * worldMtx.a0 + worldMtx.a1 * worldMtx.a1 + worldMtx.a2 * worldMtx.a2;
  if (scale < 0.1f) {
    return;
  }

  if (!NTempest::CMath::fequal_(scale, 1.0f)) {
    scale = NTempest::CMath::sqrt_(scale);
    float ooScale = 1.0f / scale;
    worldMtx.a0 *= ooScale;
    worldMtx.a1 *= ooScale;
    worldMtx.a2 *= ooScale;
    worldMtx.b0 *= ooScale;
    worldMtx.b1 *= ooScale;
    worldMtx.b2 *= ooScale;
    worldMtx.c0 *= ooScale;
    worldMtx.c1 *= ooScale;
    worldMtx.c2 *= ooScale;
  }

  NTempest::C44Matrix textureToWorld = NTempest::C44Matrix::Rotation(PI * 0.5f, NTempest::C3Vector(0.0f, 0.0f, 1.0f), true);
  NTempest::C44Matrix worldToTexture = NTempest::C44Matrix::Rotation(PI * -0.5f, NTempest::C3Vector(0.0f, 0.0f, 1.0f), true);

  NTempest::C44Matrix mirrory;
  mirrory.b1 = -1.0f;

  NTempest::C44Matrix undoScaleMat;
  undoScaleMat.Scale(NTempest::C3Vector(worldBox.t.x - worldBox.b.x, worldBox.t.y - worldBox.b.y, 1.0f));

  NTempest::C44Matrix shadowScaleMat;
  shadowScaleMat.Scale(NTempest::C3Vector(1.0f / ((localBox.t.x - localBox.b.x) * scale), 1.0f / ((localBox.t.y - localBox.b.y) * scale), 1.0f));

  NTempest::C44Matrix basisRotMat(
      worldMtx.a0, worldMtx.b0, worldMtx.c0, 0.0f, worldMtx.a1, worldMtx.b1, worldMtx.c1, 0.0f, worldMtx.a2, worldMtx.b2, worldMtx.c2, 1.0f, 0.0f,
      0.0f, 0.0f, 1.0f
  );

  NTempest::C44Matrix basisMtx = textureToWorld * undoScaleMat * basisRotMat * shadowScaleMat * worldToTexture * mirrory;
  s_Project2dCallback(worldBox, color, basisMtx);
}

static void RenderUniformUVMapLayers(
    CModelRenderData          *modelptr,
    CGeosetShared             *geoShared,
    const CMaterial           &uniqueMtl,
    const CMaterialShared     &sharedMtl,
    const NTempest::CImVector &color,
    CStatus                   *status
) {
  if ((modelptr->m_model->data->m_flags & 2) && !(modelptr->m_renderFlags & 4)) {
    GxRsSet(GxRs_NormalizeNormals, 1);
  }

  unsigned int numLayers = uniqueMtl.layers.Count();
  unsigned int layersDrawn = 0;
  unsigned int tmu;
  for (unsigned int i = 0; i < numLayers; ++i) {
    if (!uniqueMtl.layers[i].layerAlpha) {
      continue;
    }

    int alphaBlended = uniqueMtl.layers[i].blendMode > GxBlend_AlphaKey;
    if (alphaBlended && (modelptr->m_renderFlags & 1)) {
      break;
    }

    const CTexLayerShared &layerShared = sharedMtl.layers[i];
    GxRsSet(GxRs_MatDiffuse, color);
    GxRsSet(GxRs_MatEmissive, uniqueMtl.emissiveColor);

    for (tmu = 0; tmu < 2; ++tmu) {
      unsigned int flags = layerShared.tmuPass[tmu].flags;
      GxRsSet(static_cast<EGxRenderState>(GxRs_TexGen0 + tmu), flags & 1 ? 6 : 0);
      if (flags & 2) {
        GxRsSet(static_cast<EGxRenderState>(GxRs_TexLodBias0 + tmu), s_sceneSharpness);
      }
    }

    const CTexLayer &layerUnique = uniqueMtl.layers[i];
    GxRsSet(GxRs_Lighting, !(modelptr->m_renderFlags & 4) && !layerUnique.disable.lighting);
    GxRsSet(GxRs_Fog, !(modelptr->m_renderFlags & 8) && !layerUnique.disable.fog);
    GxRsSet(GxRs_DepthTest, !layerUnique.disable.depthTest);
    GxRsSet(GxRs_DepthWrite, !layerUnique.disable.depthWrite);
    GxRsSet(GxRs_Culling, !layerUnique.disable.culling);
    GxRsSet(GxRs_Blend, layerUnique.blendMode);

    for (tmu = 0; tmu < 2; ++tmu) {
      unsigned int   textureId = layerUnique.tmuPass[tmu].textureId;
      EGxRenderState textureState = static_cast<EGxRenderState>(GxRs_Texture0 + tmu);
      if (textureId == static_cast<unsigned int>(-1)) {
        GxRsSet(textureState, 0);
      } else if (layerUnique.blendMode == GxBlend_Opaque && (modelptr->m_renderFlags & 2)) {
        GxRsSet(textureState, 0);
      } else {
        CGxTex *texture = TextureGetGxTex(modelptr->m_textures[textureId].handle, 1, status);
        GxRsSet(static_cast<EGxRenderState>(GxRs_TexBlend0 + tmu), layerUnique.tmuPass[tmu].combiner);
        GxRsSet(textureState, texture);
      }
    }

    int pushed = 0;
    if (layersDrawn && alphaBlended) {
      pushed = 1;
      GxRsPush();
      GxRsSet(GxRs_FogColor, NTempest::CImVector(0xFF000000ul));
    }

    if ((geoShared->flags & 0x10) && s_Project2dCallback) {
      Project2d(geoShared, color);
    } else {
      GxPrimDrawElements();
    }

    if (modelptr->m_renderFlags & 2) {
      break;
    }
    if (pushed) {
      GxRsPop();
    }
    ++layersDrawn;
  }

  if ((modelptr->m_model->data->m_flags & 2) && !(modelptr->m_renderFlags & 4)) {
    GxRsSet(GxRs_NormalizeNormals, 0);
  }
}

static void RenderGeosetOneUvMapping(CModelRenderData *modelptr, CGeosetShared *geoShared, CMaterial *uniqueMtl, CStatus *status) {
  CMaterialShared *sharedMtl = reinterpret_cast<CMaterialShared *>(uniqueMtl->data);
  ASSERT(sharedMtl);

  NTempest::CImVector color(0ul);
  unsigned int        numLayers = sharedMtl->layers.Count();
  unsigned int        layer = 0;
  unsigned char       layerAlpha;
  while (layer < numLayers) {
    layerAlpha = uniqueMtl->layers[layer].layerAlpha;
    if (layerAlpha) {
      break;
    }
    ++layer;
  }

  if (layer < numLayers) {
    const CGeosetColor &geosetColor = modelptr->m_geosetColor[geoShared->geosetId];
    color = geosetColor.animatedColor;
    color.MultiplyRGB(&geosetColor.proceduralColor);
    color.a = static_cast<unsigned char>(color.a * layerAlpha / 255);
  }

  if (!color.a) {
    return;
  }

  if ((geoShared->flags & 0x10) && s_Project2dCallback) {
    RenderUniformUVMapLayers(modelptr, geoShared, *uniqueMtl, *sharedMtl, color, status);
    return;
  }

  if (!(modelptr->m_renderFlags & 2)) {
    SetUvTransforms(modelptr->m_model->data, uniqueMtl->layers[layer], sharedMtl->layers[layer]);
  }

  LockVertices(geoShared, uniqueMtl->layers[layer], sharedMtl->layers[layer]);

  CPrimitive     *primitive = geoShared->primitive.Ptr();
  unsigned short *indices = geoShared->primitiveVertices.Ptr();
  unsigned int    numPrimitives = geoShared->primitive.Count();
  for (unsigned int primitiveId = 0; primitiveId < numPrimitives; ++primitiveId) {
    GxPrimLockIndexPtr(primitive[primitiveId].type, primitive[primitiveId].vertexCount, indices);
    RenderUniformUVMapLayers(modelptr, geoShared, *uniqueMtl, *sharedMtl, color, status);
    GxPrimUnlockIndexPtr();
    indices += primitive[primitiveId].vertexCount;
  }
  GxPrimUnlockVertexPtrs();

  if (!(modelptr->m_renderFlags & 2)) {
    ClearUvTransforms(uniqueMtl->layers[layer], sharedMtl->layers[layer]);
  }
}

static void LockVertsAndIndices(CGeosetShared *geoShared, const CTexLayer &uniqueLayer, const CTexLayerShared &sharedLayer) {
  LockVertices(geoShared, uniqueLayer, sharedLayer);

  ASSERT(geoShared->primitive.Count() == 1);
  CPrimitive *prim = geoShared->primitive.Ptr();
  ASSERT(prim->vertexCount == geoShared->primitiveVertices.Count());

  GxPrimLockIndexPtr(prim->type, prim->vertexCount, geoShared->primitiveVertices.Ptr());
}

static void RenderSingleUVMapPrep(
    CModelRenderData      *modelptr,
    CGeosetShared         *geoShared,
    const CMaterial       &uniqueMtl,
    const CMaterialShared &sharedMtl,
    unsigned int           layerId,
    int                    geosetChanged
) {
  if (!(modelptr->m_renderFlags & 2)) {
    SetUvTransforms(modelptr->m_model->data, uniqueMtl.layers[layerId], sharedMtl.layers[layerId]);
  }

  if (geosetChanged) {
    if (s_verticesLocked) {
      GxPrimUnlockIndexPtr();
      GxPrimUnlockVertexPtrs();
    }

    LockVertsAndIndices(geoShared, uniqueMtl.layers[layerId], sharedMtl.layers[layerId]);
    s_verticesLocked = 1;
  }
}

static void RenderGeosetOneUvMapping(
    CModelRenderData *modelptr,
    CGeosetShared    *geoShared,
    CMaterial        *uniqueMtl,
    unsigned int      firstLayerId,
    int               geosetChanged,
    CStatus          *status
) {
  CMaterialShared *sharedMtl = reinterpret_cast<CMaterialShared *>(uniqueMtl->data);
  ASSERT(sharedMtl);

  const CGeosetColor &geosetColor = modelptr->m_geosetColor[geoShared->geosetId];
  NTempest::CImVector color = geosetColor.animatedColor;
  color.MultiplyRGB(&geosetColor.proceduralColor);
  color.a = static_cast<unsigned char>(color.a * uniqueMtl->layers[firstLayerId].layerAlpha / 255);

  RenderSingleUVMapPrep(modelptr, geoShared, *uniqueMtl, *sharedMtl, firstLayerId, geosetChanged);
  RenderUniformUVMapLayers(modelptr, geoShared, *uniqueMtl, *sharedMtl, color, status);
  ClearUvTransforms(uniqueMtl->layers[firstLayerId], sharedMtl->layers[firstLayerId]);
}

static void RenderGeosetMultiUvMapping(CModelRenderData *modelptr, CGeosetShared *geoShared, CMaterial *uniqueMtl, CStatus *status) {
  CMaterialShared *sharedMtl = reinterpret_cast<CMaterialShared *>(uniqueMtl->data);
  ASSERT(sharedMtl);

  const CGeosetColor &geosetColor = modelptr->m_geosetColor[geoShared->geosetId];
  NTempest::CImVector color = geosetColor.animatedColor;
  color.MultiplyRGB(&geosetColor.proceduralColor);

  if (modelptr->m_model->data->m_flags & 2) {
    GxRsSet(GxRs_NormalizeNormals, 1);
  }

  unsigned int  numLayers = uniqueMtl->layers.Count();
  unsigned char geoAlpha = color.a;
  unsigned int  layersDrawn = 0;
  unsigned int  tmu;
  for (unsigned int i = 0; i < numLayers; ++i) {
    unsigned char layerAlpha = uniqueMtl->layers[i].layerAlpha;
    if (!layerAlpha) {
      continue;
    }

    color.a = static_cast<unsigned char>(geoAlpha * layerAlpha / 255);
    const CTexLayerShared &stateLayerShared = sharedMtl->layers[i];
    GxRsSet(GxRs_MatDiffuse, color);
    GxRsSet(GxRs_MatEmissive, uniqueMtl->emissiveColor);

    for (tmu = 0; tmu < 2; ++tmu) {
      unsigned int flags = stateLayerShared.tmuPass[tmu].flags;
      GxRsSet(static_cast<EGxRenderState>(GxRs_TexGen0 + tmu), flags & 1 ? 6 : 0);
      if (flags & 2) {
        GxRsSet(static_cast<EGxRenderState>(GxRs_TexLodBias0 + tmu), s_sceneSharpness);
      }
    }

    SetUvTransforms(modelptr->m_model->data, uniqueMtl->layers[i], sharedMtl->layers[i]);
    LockVertices(geoShared, uniqueMtl->layers[i], sharedMtl->layers[i]);

    const CTexLayer &stateLayerUnique = uniqueMtl->layers[i];
    GxRsSet(GxRs_Lighting, !(modelptr->m_renderFlags & 4) && !stateLayerUnique.disable.lighting);
    GxRsSet(GxRs_Fog, !(modelptr->m_renderFlags & 8) && !stateLayerUnique.disable.fog);
    GxRsSet(GxRs_DepthTest, !stateLayerUnique.disable.depthTest);
    GxRsSet(GxRs_DepthWrite, !stateLayerUnique.disable.depthWrite);
    GxRsSet(GxRs_Culling, !stateLayerUnique.disable.culling);
    GxRsSet(GxRs_Blend, stateLayerUnique.blendMode);

    for (tmu = 0; tmu < 2; ++tmu) {
      unsigned int   textureId = stateLayerUnique.tmuPass[tmu].textureId;
      EGxRenderState textureState = static_cast<EGxRenderState>(GxRs_Texture0 + tmu);
      if (textureId == static_cast<unsigned int>(-1) || (stateLayerUnique.blendMode == GxBlend_Opaque && (modelptr->m_renderFlags & 2))) {
        GxRsSet(textureState, 0);
      } else {
        CGxTex *texture = TextureGetGxTex(modelptr->m_textures[textureId].handle, 1, status);
        GxRsSet(static_cast<EGxRenderState>(GxRs_TexBlend0 + tmu), stateLayerUnique.tmuPass[tmu].combiner);
        GxRsSet(textureState, texture);
      }
    }

    int blackenFog = 0;
    if (layersDrawn && sharedMtl->layers[i].blendMode > GxBlend_AlphaKey) {
      blackenFog = 1;
      GxRsPush();
      GxRsSet(GxRs_FogColor, NTempest::CImVector(0xFF000000ul));
    }

    CPrimitive     *primitive = geoShared->primitive.Ptr();
    unsigned short *indices = geoShared->primitiveVertices.Ptr();
    unsigned int    numPrimitives = geoShared->primitive.Count();
    for (unsigned int primitiveId = 0; primitiveId < numPrimitives; ++primitiveId) {
      GxPrimDrawElements(primitive->type, primitive->vertexCount, indices);
      indices += primitive->vertexCount;
      ++primitive;
    }

    if (blackenFog) {
      GxRsPop();
    }
    ++layersDrawn;

    ClearUvTransforms(uniqueMtl->layers[i], sharedMtl->layers[i]);
    GxPrimUnlockVertexPtrs();
  }

  if (modelptr->m_model->data->m_flags & 2) {
    GxRsSet(GxRs_NormalizeNormals, 0);
  }
}

static int SingleUvMapping(CMaterial *uniqueMtl) {
  if (uniqueMtl->layers.Count() <= 1) {
    return 1;
  }

  CMaterialShared *sharedMtl = reinterpret_cast<CMaterialShared *>(uniqueMtl->data);
  ASSERT(sharedMtl);

  unsigned int  numLayers = uniqueMtl->layers.Count();
  unsigned int  layer;
  unsigned char layerAlpha;
  for (layer = 0; layer < numLayers; ++layer) {
    layerAlpha = uniqueMtl->layers[layer].layerAlpha;
    if (layerAlpha) {
      break;
    }
  }

  if (layer == numLayers) {
    return 1;
  }

  unsigned int firstCoordIds[2];
  unsigned int tmu;
  for (tmu = 0; tmu < 2; ++tmu) {
    firstCoordIds[tmu] = sharedMtl->layers[layer].tmuPass[tmu].coordId;
  }

  for (++layer; layer < numLayers; ++layer) {
    if (!uniqueMtl->layers[layer].layerAlpha) {
      continue;
    }
    if (uniqueMtl->layers[layer].layerAlpha != layerAlpha) {
      return 0;
    }

    for (tmu = 0; tmu < 2; ++tmu) {
      if (firstCoordIds[tmu] != sharedMtl->layers[layer].tmuPass[tmu].coordId) {
        return 0;
      }
    }
  }

  return 1;
}

static void RenderGeosetLayers(CModelRenderData *modelptr, CGeosetShared *geoShared, CStatus *status) {
  ASSERT(modelptr);
  ASSERT(geoShared);

  CMaterial *uniqueMtl = reinterpret_cast<CMaterial *>(modelptr->m_materials[geoShared->materialId]);
  ASSERT(uniqueMtl);

  if (SingleUvMapping(uniqueMtl) || (modelptr->m_renderFlags & 2)) {
    RenderGeosetOneUvMapping(modelptr, geoShared, uniqueMtl, status);
  } else {
    RenderGeosetMultiUvMapping(modelptr, geoShared, uniqueMtl, status);
  }
}

static void TransformBounds(const NTempest::C3Vector& position, float rotationAngle, const NTempest::C3Vector& rotationAxis, float scale, NTempest::CAaSphere* bounds) {
  ASSERT(bounds);
  WorldMatrixPush();
  WorldMatrixTranslate(position);
  WorldMatrixRotate(rotationAngle, rotationAxis);
  WorldMatrixScale(scale);
  WorldMatrixTransform(&bounds->c);
  WorldMatrixPop();
  bounds->r *= scale;
}

static void TransformBounds(const NTempest::C34Matrix& modelToWorld, float scale, NTempest::CAaSphere* bounds) {
  ASSERT(bounds);
  bounds->c *= modelToWorld;
  bounds->r *= scale;
}

static void RenderGeosetPrep(CModelBase *modelptr, CGeoset *geoUnique, CGeosetShared *geoShared) {
  ASSERT(modelptr);
  ASSERT(geoUnique);
  ASSERT(geoShared);

  if (modelptr->m_PickLights) {
    SaveFog();
    NTempest::C3Vector center = s_sceneCameraPos + geoShared->centroid * modelptr->m_modelToWorld;
    modelptr->m_PickLights(modelptr->m_pickLightsParm, center, s_sceneCameraPos, 8);
  }

  NTempest::C34Matrix *weightedBones = MatrixDeref(geoUnique->weightedBones);
  if (geoShared->vertexShader == GxVS_PassThru) {
    GxXformPush(GxXform_World);
    if (weightedBones) {
      NTempest::C44Matrix matrix(
          weightedBones->a0, weightedBones->a1, weightedBones->a2, 0.0f, weightedBones->b0, weightedBones->b1, weightedBones->b2, 0.0f,
          weightedBones->c0, weightedBones->c1, weightedBones->c2, 0.0f, weightedBones->d0, weightedBones->d1, weightedBones->d2, 1.0f
      );
      GxXformSet(GxXform_World, matrix);
    }
  } else {
    ASSERT(weightedBones);
    GxXformSetBones(geoShared->groupMatrixCounts.Count(), weightedBones);
  }

  GxVertexShaderSelect(geoShared->vertexShader);
}

static void RenderGeosetSingleLayer(
    CModelRenderData *modelptr,
    CGeoset          *geoUnique,
    CGeosetShared    *geoShared,
    unsigned int      layerId,
    int               materialChanged,
    int               geosetChanged,
    CStatus          *status
) {
  ASSERT(modelptr);
  ASSERT(geoUnique);
  ASSERT(geoShared);

  RenderGeosetPrep(modelptr->m_model->data, geoUnique, geoShared);

  CMaterial *uniqueMtl = reinterpret_cast<CMaterial *>(modelptr->m_materials[geoShared->materialId]);
  ASSERT(uniqueMtl);
  CMaterialShared *sharedMtl = reinterpret_cast<CMaterialShared *>(uniqueMtl->data);
  ASSERT(sharedMtl);

  const CGeosetColor &geosetColor = modelptr->m_geosetColor[geoShared->geosetId];
  NTempest::CImVector newColor = geosetColor.animatedColor;
  newColor.r = static_cast<unsigned char>((geosetColor.proceduralColor.r * newColor.r + 255) >> 8);
  newColor.g = static_cast<unsigned char>((geosetColor.proceduralColor.g * newColor.g + 255) >> 8);
  newColor.b = static_cast<unsigned char>((geosetColor.proceduralColor.b * newColor.b + 255) >> 8);
  newColor.a = static_cast<unsigned char>(newColor.a * uniqueMtl->layers[layerId].layerAlpha / 255);

  RenderSingleUVMapPrep(modelptr, geoShared, *uniqueMtl, *sharedMtl, layerId, geosetChanged);

  if ((modelptr->m_model->data->m_flags & 2) && !(modelptr->m_renderFlags & 4)) {
    GxRsSet(GxRs_NormalizeNormals, 1);
  }

  const CTexLayerShared &layerShared = sharedMtl->layers[layerId];
  GxRsSet(GxRs_MatDiffuse, newColor);
  GxRsSet(GxRs_MatEmissive, uniqueMtl->emissiveColor);

  for (unsigned int tmu = 0; tmu < 2; ++tmu) {
    unsigned int flags = layerShared.tmuPass[tmu].flags;
    GxRsSet(static_cast<EGxRenderState>(GxRs_TexGen0 + tmu), flags & 1 ? 6 : 0);
    if (flags & 2) {
      GxRsSet(static_cast<EGxRenderState>(GxRs_TexLodBias0 + tmu), s_sceneSharpness);
    }
  }

  if (materialChanged) {
    const CTexLayer &layerUnique = uniqueMtl->layers[layerId];
    GxRsSet(GxRs_Lighting, !(modelptr->m_renderFlags & 4) && !layerUnique.disable.lighting);
    GxRsSet(GxRs_Fog, !(modelptr->m_renderFlags & 8) && !layerUnique.disable.fog);
    GxRsSet(GxRs_DepthTest, !layerUnique.disable.depthTest);
    GxRsSet(GxRs_DepthWrite, !layerUnique.disable.depthWrite);
    GxRsSet(GxRs_Culling, !layerUnique.disable.culling);
    GxRsSet(GxRs_Blend, layerUnique.blendMode);

    for (unsigned int tmu = 0; tmu < 2; ++tmu) {
      unsigned int   textureId = layerUnique.tmuPass[tmu].textureId;
      EGxRenderState textureState = static_cast<EGxRenderState>(GxRs_Texture0 + tmu);
      if (textureId == static_cast<unsigned int>(-1) || (layerUnique.blendMode == GxBlend_Opaque && (modelptr->m_renderFlags & 2))) {
        GxRsSet(textureState, 0);
      } else {
        CGxTex *texture = TextureGetGxTex(modelptr->m_textures[textureId].handle, 1, status);
        GxRsSet(static_cast<EGxRenderState>(GxRs_TexBlend0 + tmu), layerUnique.tmuPass[tmu].combiner);
        GxRsSet(textureState, texture);
      }
    }
  }

  int pushed = 0;
  if (layerId && sharedMtl->layers[layerId].blendMode > GxBlend_AlphaKey) {
    pushed = 1;
    GxRsPush();
    GxRsSet(GxRs_FogColor, NTempest::CImVector(0xFF000000ul));
  }

  GxPrimDrawElements();
  if (pushed) {
    GxRsPop();
  }

  if ((modelptr->m_model->data->m_flags & 2) && !(modelptr->m_renderFlags & 4)) {
    GxRsSet(GxRs_NormalizeNormals, 0);
  }
  if (geoShared->vertexShader == GxVS_PassThru) {
    GxXformPop(GxXform_World);
  }
  if (modelptr->m_model->data->m_PickLights) {
    RestoreFog();
  }
  if (!(modelptr->m_renderFlags & 2)) {
    ClearUvTransforms(uniqueMtl->layers[layerId], sharedMtl->layers[layerId]);
  }
}

static void RenderSortedGeoset(
    CModelRenderData *modelptr,
    CGeoset          *geoUnique,
    CGeosetShared    *geoShared,
    unsigned int      firstLayerId,
    int               geosetChanged,
    CStatus          *status
) {
  ASSERT(modelptr);
  ASSERT(geoUnique);
  ASSERT(geoShared);

  RenderGeosetPrep(modelptr->m_model->data, geoUnique, geoShared);
  CMaterial *uniqueMtl = reinterpret_cast<CMaterial *>(modelptr->m_materials[geoShared->materialId]);
  if (SingleUvMapping(uniqueMtl)) {
    RenderGeosetOneUvMapping(modelptr, geoShared, uniqueMtl, firstLayerId, geosetChanged, status);
  } else {
    if (s_verticesLocked) {
      GxPrimUnlockIndexPtr();
      GxPrimUnlockVertexPtrs();
      s_verticesLocked = 0;
    }
    RenderGeosetMultiUvMapping(modelptr, geoShared, uniqueMtl, status);
  }

  if (geoShared->vertexShader == GxVS_PassThru) {
    GxXformPop(GxXform_World);
  }
  if (modelptr->m_model->data->m_PickLights) {
    RestoreFog();
  }
}

static void RenderGeoset(CModelRenderData *modelptr, CGeoset *geoUnique, CGeosetShared *geoShared, CStatus *status) {
  ASSERT(modelptr);
  ASSERT(geoUnique);
  ASSERT(geoShared);

  RenderGeosetPrep(modelptr->m_model->data, geoUnique, geoShared);
  RenderGeosetLayers(modelptr, geoShared, status);

  if (geoShared->vertexShader == GxVS_PassThru) {
    GxXformPop(GxXform_World);
  }
  if (modelptr->m_model->data->m_PickLights) {
    RestoreFog();
  }
}

static void RenderGeosetCheckVis(CModelRenderData *modelptr, CGeoset *geoUnique, CGeosetShared *geoShared, CStatus *status) {
  ASSERT(modelptr);
  ASSERT(geoUnique);
  ASSERT(geoShared);

  if (!(geoUnique->flags & 1) && modelptr->m_geosetColor[geoShared->geosetId].animatedColor.a) {
    RenderGeoset(modelptr, geoUnique, geoShared, status);
  }
}

static void RenderCustomGeoset(HMODEL model, CCustomGeoset *object) {
  ASSERT(object);

  CModelBase *modelptr;
  IModelDerefHandle(reinterpret_cast<CModel *>(model), &modelptr);
  HMODEL duplicate = static_cast<HMODEL>(HandleDuplicate(model));

  NTempest::C34Matrix matrix;
  matrix.Translate(object->position);
  matrix *= modelptr->m_modelToWorld;
  object->renderCallback(duplicate, matrix, object->renderParam);

  HandleClose(duplicate);
}

static void IModelGetBoundingSphere(CModelBase *modelptr, CModelShared *shared, NTempest::CAaSphere *sphere) {
  ASSERT(modelptr);
  ASSERT(shared);
  ASSERT(sphere);

  unsigned int sequence;
  if (shared->seqBounds.Count() && modelptr->m_anim && AnimNeedsSequenceBounds(modelptr->m_anim)) {
    AnimGetPrimarySequence(modelptr->m_anim, &sequence);
    *sphere = shared->seqBounds[sequence].sphere;
  } else {
    *sphere = shared->bounds.sphere;
  }
}

static void IModelGetExtents(CModelBase *modelptr, CModelShared *shared, NTempest::CAaBox *extents) {
  ASSERT(modelptr);
  ASSERT(shared);
  ASSERT(extents);

  unsigned int sequence;
  if (shared->seqBounds.Count() && modelptr->m_anim && AnimNeedsSequenceBounds(modelptr->m_anim)) {
    AnimGetPrimarySequence(modelptr->m_anim, &sequence);
    *extents = shared->seqBounds[sequence].extent;
  } else {
    *extents = shared->bounds.extent;
  }
}

static int IModelGetExtents(CModelBase *modelptr, CModelShared *shared, unsigned int seqnum, NTempest::CAaBox *extents) {
  ASSERT(modelptr);
  ASSERT(shared);
  ASSERT(extents);

  if (shared->seqBounds.Count() && modelptr->m_anim && AnimNeedsSequenceBounds(modelptr->m_anim)) {
    if (seqnum >= shared->seqBounds.Count()) {
      return 0;
    }
    *extents = shared->seqBounds[seqnum].extent;
  } else {
    *extents = shared->bounds.extent;
  }

  return 1;
}

static int GeosetTestRay(CGeoset* geoUnique, CGeosetShared* geoShared, CGeosetColor* geosetColor, const NTempest::C3Vector& rayStart, const NTempest::C3Vector& rayDirection, float* distance) {
  if ((geoUnique->flags & 1) || !geosetColor[geoShared->geosetId].animatedColor.a || (geoShared->flags & 1)) {
    return 0;
  }

  NTempest::C34Matrix *boneMatrices = MatrixDeref(geoUnique->weightedBones);
  if (!boneMatrices) {
    return 0;
  }

  int                   foundHit = 0;
  unsigned short       *indices = geoShared->primitiveVertices.Ptr();
  CPrimitive           *primitive = geoShared->primitive.Ptr();
  for (unsigned int i = 0; i < geoShared->primitive.Count(); ++i, ++primitive) {
    if (primitive->type >= GxPrim_Triangles) {
      float        currDistance;
      unsigned int primIntersected;
      if (GxuTestRayAndMesh(
              rayStart,
              rayDirection,
              boneMatrices,
              geoShared->groupMatrixCounts.Count(),
              geoShared->position.Count(),
              geoShared->position.Ptr(),
              sizeof(NTempest::C3Vector),
              geoShared->boneWeights.Count(),
              geoShared->boneWeights.Ptr(),
              geoShared->vertexShader == 1,
              primitive->type,
              primitive->vertexCount,
              indices,
              currDistance,
              primIntersected
          ))
      {
        foundHit = 1;
        if (currDistance < *distance) {
          *distance = currDistance;
        }
      }
    }
    indices += primitive->vertexCount;
  }
  return foundHit;
}

static int IModelTestRay(CModelSimple* modelptr, CModelShared* shared, const NTempest::C3Vector& rayStart, const NTempest::C3Vector& rayEnd, float* distance) {
  *distance = FLT_MAX;
  NTempest::C3Vector rayDirection = rayEnd - rayStart;
  rayDirection.Normalize();

  int foundHit = 0;
  for (unsigned int i = 0; i < shared->numGeosets; ++i) {
    foundHit |= GeosetTestRay(
        &modelptr->m_geosets[i], &shared->geosets[i], modelptr->m_geosetColor.Ptr(), rayStart, rayDirection, distance
    );
  }
  if (!foundHit) {
    *distance = INFINITY;
  }
  return foundHit;
}

static int IModelTestRay(CModelComplex* modelptr, CModelShared* shared, const NTempest::C3Vector& rayStart, const NTempest::C3Vector& rayEnd, float* distance, int testLinkedModels) {
  *distance = FLT_MAX;
  NTempest::C3Vector rayDirection = rayEnd - rayStart;
  rayDirection.Normalize();

  int foundHit = 0;
  unsigned int i;
  for (i = 0; i < shared->numGeosets; ++i) {
    foundHit |= GeosetTestRay(
        &modelptr->m_geosets[i], &shared->geosets[i], modelptr->m_geosetColor.Ptr(), rayStart, rayDirection, distance
    );
  }
  for (i = 0; i < modelptr->m_addlGeosets.Count(); ++i) {
    foundHit |= GeosetTestRay(
        &modelptr->m_geosets[i + shared->numGeosets],
        &modelptr->m_addlGeosets[i],
        modelptr->m_geosetColor.Ptr(),
        rayStart,
        rayDirection,
        distance
    );
  }

  if (testLinkedModels) {
    for (i = 0; i < modelptr->m_attached.Count(); ++i) {
      for (LINKUNIQUE *link = modelptr->m_attached[i].Head(); link; link = link->Next()) {
        CModelBase   *childptr;
        CModelShared *childShared;
        if (IModelDerefHandle(reinterpret_cast<CModel *>(link->child), &childptr, &childShared)) {
          float currDistance;
          if (IModelTestRay(childptr, childShared, rayStart, rayEnd, &currDistance, 1)) {
            foundHit = 1;
            if (currDistance < *distance) {
              *distance = currDistance;
            }
          }
        }
      }
    }
  }

  if (!foundHit) {
    *distance = INFINITY;
  }
  return foundHit;
}

static int IModelTestRay(CModelBase* modelptr, CModelShared* shared, const NTempest::C3Vector& rayStart, const NTempest::C3Vector& rayEnd, float* distance, int testLinkedModels) {
  if (modelptr->m_flags & 0x20) {
    return IModelTestRay(static_cast<CModelComplex *>(modelptr), shared, rayStart, rayEnd, distance, testLinkedModels);
  }
  return IModelTestRay(static_cast<CModelSimple *>(modelptr), shared, rayStart, rayEnd, distance);
}

static void CreateBoxGeometry(
    const NTempest::CAaBox              &bounds,
    TSGrowableArray<NTempest::C3Vector> *positions,
    TSGrowableArray<NTempest::C3Vector> *normals,
    TSGrowableArray<NTempest::C2Vector> *texCoords,
    TSGrowableArray<unsigned short>     *primVertIndices,
    EGxPrim                             *primType
) {
  unsigned int vertexOffset = positions->Count();
  positions->SetCount(vertexOffset + 24);

  unsigned int z;
  unsigned int y;
  unsigned int x;
  for (z = 0; z < 2; ++z) {
    for (y = 0; y < 2; ++y) {
      for (x = 0; x < 2; ++x) {
        unsigned int dst = vertexOffset + (z * 4 + y * 2 + x) * 3;
        positions->Ptr()[dst].x = (&bounds.b.x)[x * 3];
        positions->Ptr()[dst].y = (&bounds.b.y)[y * 3];
        positions->Ptr()[dst].z = (&bounds.b.z)[z * 3];
        positions->Ptr()[dst + 1].x = (&bounds.b.x)[x * 3];
        positions->Ptr()[dst + 1].y = (&bounds.b.y)[y * 3];
        positions->Ptr()[dst + 1].z = (&bounds.b.z)[z * 3];
        positions->Ptr()[dst + 2].x = (&bounds.b.x)[x * 3];
        positions->Ptr()[dst + 2].y = (&bounds.b.y)[y * 3];
        positions->Ptr()[dst + 2].z = (&bounds.b.z)[z * 3];
      }
    }
  }

  float dir[2] = {-1.0f, 1.0f};
  normals->SetCount(vertexOffset + 24);
  for (z = 0; z < 2; ++z) {
    for (y = 0; y < 2; ++y) {
      for (x = 0; x < 2; ++x) {
        unsigned int dst = vertexOffset + (z * 4 + y * 2 + x) * 3;
        normals->Ptr()[dst].x = dir[x];
        normals->Ptr()[dst + 1].y = dir[y];
        normals->Ptr()[dst + 2].z = dir[z];
      }
    }
  }

  texCoords->SetCount(vertexOffset + 24);
  for (z = 0; z < 2; ++z) {
    for (y = 0; y < 2; ++y) {
      for (x = 0; x < 2; ++x) {
        unsigned int dst = vertexOffset + (z * 4 + y * 2 + x) * 3;
        texCoords->Ptr()[dst].x = static_cast<float>(y);
        texCoords->Ptr()[dst].y = static_cast<float>(1 - z);
        texCoords->Ptr()[dst + 1].x = static_cast<float>(x ^ y);
        texCoords->Ptr()[dst + 1].y = static_cast<float>(1 - z);
        texCoords->Ptr()[dst + 2].x = static_cast<float>(y);
        texCoords->Ptr()[dst + 2].y = static_cast<float>(x);
      }
    }
  }

  unsigned int indexOffset = primVertIndices->Add(36, vertIndices);
  if (vertexOffset) {
    unsigned int index;
    for (index = indexOffset; index < primVertIndices->Count(); ++index) {
      primVertIndices->operator[](index) += static_cast<unsigned short>(vertexOffset);
    }
  }
  *primType = GxPrim_Triangles;
}

HMODEL CreateModelBoundingBox(const NTempest::CAaBox &bounds, HTEXTURE texture, EGxBlend blendMode) {
  TSGrowableArray<NTempest::C3Vector> positions;
  TSGrowableArray<NTempest::C3Vector> normals;
  TSGrowableArray<NTempest::C2Vector> texCoords;
  TSGrowableArray<unsigned short>     primVertIndices;
  EGxPrim                             primType;
  CreateBoxGeometry(bounds, &positions, &normals, &texCoords, &primVertIndices, &primType);

  HTEXTURE solid = 0;
  if (!texture) {
    solid = TextureCreateSolid(NTempest::CImVector(0x7F00FFFF), 0);
    texture = solid;
  }
  HMODEL model = ModelCreateSimpleMesh(
      "BoundingBox", positions.Count(), positions.Ptr(), normals.Ptr(), texCoords.Ptr(), primType, primVertIndices.Ptr(), primVertIndices.Count(),
      texture, blendMode, blendMode == GxBlend_Alpha ? 0x10 : 0, NTempest::CImVector(0xFFFFFFFF), 0
  );
  if (solid) {
    HandleClose(solid);
  }
  return model;
}

static void GenerateSphereVerts(
    const NTempest::CAaSphere           &bounds,
    unsigned int                         latLongLines,
    TSGrowableArray<NTempest::C3Vector> *vertices,
    TSGrowableArray<NTempest::C3Vector> *normals
) {
  unsigned int offset = vertices->Count();
  unsigned int count = latLongLines * latLongLines + 2;
  vertices->SetCount(offset + count);
  normals->SetCount(offset + count);

  vertices->operator[](offset) = bounds.c + NTempest::C3Vector(0.0f, 0.0f, bounds.r);
  normals->operator[](offset) = NTempest::C3Vector(0.0f, 0.0f, 1.0f);

  unsigned int dst = offset + 1;
  for (unsigned int lat = 0; lat < latLongLines; ++lat) {
    float latitude = static_cast<float>(lat + 1) * PI / static_cast<float>(latLongLines + 1);
    float radius = static_cast<float>(sin(latitude));
    float latZ = static_cast<float>(cos(latitude));
    for (unsigned int lon = 0; lon < latLongLines; ++lon) {
      float              longitude = static_cast<float>(lon) * 2.0f * PI / static_cast<float>(latLongLines);
      NTempest::C3Vector normal(static_cast<float>(sin(longitude)) * radius, static_cast<float>(cos(longitude)) * radius, latZ);
      normals->operator[](dst) = normal;
      vertices->operator[](dst) = bounds.c + normal * bounds.r;
      ++dst;
    }
  }

  vertices->operator[](offset + count - 1) = bounds.c + NTempest::C3Vector(0.0f, 0.0f, -bounds.r);
  normals->operator[](offset + count - 1) = NTempest::C3Vector(0.0f, 0.0f, -1.0f);
}

static void CreateSphereGeometry(
    const NTempest::CAaSphere           &bounds,
    TSGrowableArray<NTempest::C3Vector> *vertices,
    TSGrowableArray<NTempest::C3Vector> *normals,
    TSGrowableArray<NTempest::C2Vector> *texCoords,
    TSGrowableArray<unsigned short>     *vertIndices,
    TSGrowableArray<CPrimitive>         *primitives
) {
  const unsigned int lines = 15;
  unsigned int       vertOffset = vertices->Count();
  unsigned int       indexOffset = vertIndices->Count();
  GenerateSphereVerts(bounds, lines, vertices, normals);
  texCoords->SetCount(vertices->Count());
  vertIndices->SetCount(indexOffset + 527);

  unsigned short *out = vertIndices->Ptr() + indexOffset;
  unsigned int    i;
  for (i = 0; i < lines; ++i) {
    *out++ = static_cast<unsigned short>(vertOffset + 1 + i);
    *out++ = static_cast<unsigned short>(vertOffset);
  }
  *out++ = static_cast<unsigned short>(vertOffset + 1);
  *out++ = static_cast<unsigned short>(vertOffset);
  *out++ = static_cast<unsigned short>(vertOffset);
  *out++ = static_cast<unsigned short>(vertOffset);

  for (unsigned int strip = 0; strip < 13; ++strip) {
    unsigned int first = vertOffset + 1 + strip * lines;
    unsigned int second = first + lines;
    if (!(strip & 1)) {
      for (i = 0; i < lines; ++i) {
        *out++ = static_cast<unsigned short>(first + i);
        *out++ = static_cast<unsigned short>(second + i);
      }
      *out++ = static_cast<unsigned short>(first);
      *out++ = static_cast<unsigned short>(second);
    } else {
      for (i = 0; i < lines; ++i) {
        *out++ = static_cast<unsigned short>(second + i);
        *out++ = static_cast<unsigned short>(first + i);
      }
      *out++ = static_cast<unsigned short>(second);
      *out++ = static_cast<unsigned short>(first);
    }
    *out++ = out[-1];
  }

  unsigned int penultimate = vertOffset + 1 + 13 * lines;
  unsigned int lastRing = penultimate + lines;
  for (i = 0; i < lines; ++i) {
    *out++ = static_cast<unsigned short>(penultimate + i);
    *out++ = static_cast<unsigned short>(lastRing + i);
  }
  *out++ = static_cast<unsigned short>(penultimate);
  *out++ = static_cast<unsigned short>(lastRing);

  unsigned int bottom = vertOffset + lines * lines + 1;
  for (i = 0; i < lines; ++i) {
    *out++ = static_cast<unsigned short>(lastRing + i);
    *out++ = static_cast<unsigned short>(bottom);
  }
  *out++ = static_cast<unsigned short>(lastRing);
  *out++ = static_cast<unsigned short>(bottom);

  primitives->SetCount(primitives->Count() + 1);
  CPrimitive &primitive = primitives->operator[](primitives->Count() - 1);
  primitive.type = GxPrim_TriangleStrip;
  primitive.vertexCount = 527;
}

static HMODEL CreateModelBoundingSphere(const NTempest::CAaSphere &bounds, HTEXTURE texture, EGxBlend blendMode) {
  TSGrowableArray<NTempest::C3Vector> vertices;
  TSGrowableArray<NTempest::C3Vector> normals;
  TSGrowableArray<NTempest::C2Vector> texCoords;
  TSGrowableArray<unsigned short>     vertIndices;
  TSGrowableArray<CPrimitive>         primitives;
  CreateSphereGeometry(bounds, &vertices, &normals, &texCoords, &vertIndices, &primitives);
  return ModelCreateSimpleMesh(
      "Bounding Sphere", vertices.Count(), vertices.Ptr(), normals.Ptr(), texCoords.Ptr(), primitives[0].type, vertIndices.Ptr(), vertIndices.Count(),
      texture, blendMode, 0, NTempest::CImVector(0xFFFFFFFF), 0
  );
}

HMODEL ModelCreateSolidSphere(float radius, HTEXTURE texture) {
  NTempest::CAaSphere bounds;
  bounds.c = NTempest::C3Vector(0.0f);
  bounds.r = radius;
  return CreateModelBoundingSphere(bounds, texture, GxBlend_Opaque);
}

HMODEL ModelCreateBox(const NTempest::CAaBox &bounds, HTEXTURE texture, EGxBlend blendMode) {
  return CreateModelBoundingBox(bounds, texture, blendMode);
}

static void FillInRenderData(CModel *modelptr, unsigned int renderFlags, CModelRenderData *renderData) {
  ASSERT(modelptr);
  ASSERT(modelptr->data);
  ASSERT(renderData);

  renderData->m_model = modelptr;
  renderData->m_shared = reinterpret_cast<CModelShared *>(modelptr->shared);
  renderData->m_renderFlags = renderFlags;

  if (modelptr->data->m_flags & 0x20) {
    CModelComplex *complex = static_cast<CModelComplex *>(modelptr->data);
    renderData->m_geosets = complex->m_geosets.Ptr();
    renderData->m_geosetColor = complex->m_geosetColor.Ptr();
    renderData->m_numGeosets = complex->m_geosets.Count();
    renderData->m_materials = complex->m_materials.Ptr();
    renderData->m_textures = complex->m_textures.Ptr();
  } else {
    CModelSimple *simple = static_cast<CModelSimple *>(modelptr->data);
    renderData->m_geosets = simple->m_geosets.Ptr();
    renderData->m_geosetColor = simple->m_geosetColor.Ptr();
    renderData->m_numGeosets = simple->m_geosets.Count();
    renderData->m_materials = simple->m_materials.Ptr();
    renderData->m_textures = simple->m_textures.Ptr();
  }
}

static CModelTexture *GetTextureList(CModelBase *modelptr) {
  if (modelptr->m_flags & 0x20) {
    return static_cast<CModelComplex *>(modelptr)->m_textures.Ptr();
  }

  return static_cast<CModelSimple *>(modelptr)->m_textures.Ptr();
}

static void IModelRenderSceneOpaque(CStatus *status) {
  if (!s_opLayerPool.Count()) {
    return;
  }

  unsigned int index;
  for (index = 0; index < s_opLayerPool.Count(); ++index) {
    s_opaqueScene.Enqueue(&s_opLayerPool[index]);
  }

  COpaqueLayer *lastSorted = s_opaqueScene[NTempest::CPriorityQ<COpaqueLayer *, COpaqueLayer>::eRootIndex];
  ASSERT(lastSorted);
  ASSERT(lastSorted->model);

  unsigned int rsStackOffset = GxRsStackOffset();
  GxRsPush();

  CModelRenderData renderData;
  FillInRenderData(reinterpret_cast<CModel *>(lastSorted->model), 0, &renderData);

  CTexLayer *layer = lastSorted->layer;
  GxRsSet(GxRs_Lighting, !(renderData.m_renderFlags & 4) && !layer->disable.lighting);
  GxRsSet(GxRs_Fog, !(renderData.m_renderFlags & 8) && !layer->disable.fog);
  GxRsSet(GxRs_DepthTest, !layer->disable.depthTest);
  GxRsSet(GxRs_DepthWrite, !layer->disable.depthWrite);
  GxRsSet(GxRs_Culling, !layer->disable.culling);
  GxRsSet(GxRs_Blend, layer->blendMode);

  for (unsigned int tmu = 0; tmu < 2; ++tmu) {
    unsigned int   textureId = layer->tmuPass[tmu].textureId;
    EGxRenderState textureState = static_cast<EGxRenderState>(GxRs_Texture0 + tmu);
    if (textureId == static_cast<unsigned int>(-1) || (layer->blendMode == GxBlend_Opaque && (renderData.m_renderFlags & 2))) {
      GxRsSet(textureState, 0);
    } else {
      CGxTex *texture = TextureGetGxTex(renderData.m_textures[textureId].handle, 1, status);
      GxRsSet(static_cast<EGxRenderState>(GxRs_TexBlend0 + tmu), layer->tmuPass[tmu].combiner);
      GxRsSet(textureState, texture);
    }
  }

  ASSERT(!s_verticesLocked);
  int            multiLayered = 0;
  CGeosetShared *lastGeoset = 0;
  while (s_opaqueScene.HasEntries()) {
    COpaqueLayer *sorted = s_opaqueScene.Dequeue();
    ASSERT(sorted);
    ASSERT(sorted->geoShared);
    ASSERT(sorted->model);

    int geosetChanged = sorted->geoShared != lastGeoset || sorted->geoShared->vertexShader != GxVS_PassThru;
    FillInRenderData(reinterpret_cast<CModel *>(sorted->model), sorted->flags, &renderData);

    if ((sorted->flags & 0x3000) == 0x3000) {
      RenderSortedGeoset(&renderData, sorted->geoUnique, sorted->geoShared, sorted->firstLayer, geosetChanged, status);
    } else {
      int materialChanged = 1;
      if (!multiLayered) {
        CModelTexture *priorTextures = GetTextureList(reinterpret_cast<CModel *>(lastSorted->model)->data);
        if (!CTexLayer::Compare(priorTextures, renderData.m_textures, *lastSorted->layer, *sorted->layer) &&
            !((lastSorted->flags ^ sorted->flags) & 0xF))
        {
          materialChanged = 0;
        }
      }

      RenderGeosetSingleLayer(&renderData, sorted->geoUnique, sorted->geoShared, sorted->firstLayer, materialChanged, geosetChanged, status);
    }

    multiLayered = (sorted->flags & 0x3000) == 0x3000;
    lastSorted = sorted;
    lastGeoset = sorted->geoShared;
  }

  if (s_verticesLocked) {
    GxPrimUnlockIndexPtr();
    GxPrimUnlockVertexPtrs();
    s_verticesLocked = 0;
  }

  GxRsPop();
  ASSERT(rsStackOffset == GxRsStackOffset());
}

static void IModelRenderSceneTransparent(CStatus *status) {
  if (!s_trLayerPool.Count()) {
    return;
  }

  unsigned int rsStackOffset = GxRsStackOffset();
  unsigned int index;
  for (index = 0; index < s_trLayerPool.Count(); ++index) {
    s_transparentScene.Enqueue(&s_trLayerPool[index]);
  }

  while (s_transparentScene.HasEntries()) {
    CTransparentObject *object = s_transparentScene.Dequeue();
    ASSERT(object);

    switch (object->sortType) {
      case SORTOBJ_GEOSET: {
        GxRsPush();
        CModelRenderData renderData;
        FillInRenderData(reinterpret_cast<CModel *>(object->geo.model), 0, &renderData);
        RenderGeoset(&renderData, object->geo.geoUnique, object->geo.geoShared, status);
        GxRsPop();
        break;
      }

      case SORTOBJ_EMITTER2: {
        CModelBase *modelptr;
        IModelDerefHandle(reinterpret_cast<CModel *>(object->stnd.model), &modelptr);
        if (modelptr->m_PickLights) {
          SaveFog();
          modelptr->m_PickLights(
              modelptr->m_pickLightsParm, s_sceneCameraPos,
              NTempest::C3Vector(
                  s_sceneCameraPos.x + modelptr->m_modelToWorld.d0, s_sceneCameraPos.y + modelptr->m_modelToWorld.d1,
                  s_sceneCameraPos.z + modelptr->m_modelToWorld.d2
              ),
              8
          );
        }

        static_cast<CParticleEmitter2 *>(object->stnd.object)->Render();
        if (modelptr->m_PickLights) {
          RestoreFog();
        }
        break;
      }

      case SORTOBJ_RIBBON:
        static_cast<CRibbonEmitter *>(object->stnd.object)->Render();
        break;

      case SORTOBJ_CUSTOM_GEO:
        RenderCustomGeoset(object->stnd.model, static_cast<CCustomGeoset *>(object->stnd.object));
        break;

      case SORTOBJ_CUSTOM_MODEL:
        GxRsPush();
        object->cust.callback(object->cust.param1, object->cust.param2);
        GxRsPop();
        break;
    }
  }

  ASSERT(rsStackOffset == GxRsStackOffset());
}

void ModelRenderInitialize() {
  s_nextMatrix = 0;
  s_lastFrame = 0;
}

void ModelRenderDestroy() {
  ModelRenderSceneLogStop();
}

unsigned int GetInvalidMatrixId() {
  return (s_currAnimFrame - 1) << 16;
}

unsigned int MatrixAlloc(unsigned int numMatrices) {
  unsigned int frame = GxPerfCounter(GxPerf_FrameNum);

  if (frame != s_lastFrame) {
    ++s_currAnimFrame;
    s_lastFrame = frame;
    s_nextMatrix = 0;
  }

  if (!numMatrices) {
    return GetInvalidMatrixId();
  }

  ASSERT(s_nextMatrix <= 0xFFFF);

  unsigned int nextMatrix = s_nextMatrix + numMatrices;
  if (nextMatrix > s_matrixPool.Count()) {
    s_matrixPool.SetCount(nextMatrix);
  }

  unsigned int result = s_nextMatrix | (static_cast<unsigned int>(s_currAnimFrame) << 16);
  s_nextMatrix = nextMatrix;
  return result;
}

NTempest::C34Matrix *MatrixDeref(unsigned int handle) {
  if (static_cast<unsigned short>(handle >> 16) != s_currAnimFrame) {
    return 0;
  }

  return &s_matrixPool[static_cast<unsigned short>(handle)];
}

void ModelRenderSceneLogStart(const char *fileName) {
}

void ModelRenderSceneLogStop() {
}

int ModelRenderSceneLogToggle(const char *fileName) {
  return 0;
}

void ModelScenePlaceCamera(const NTempest::C3Vector &position, const NTempest::C3Vector &direction) {
  ASSERT(!s_opLayerPool.Count());
  ASSERT(!s_trLayerPool.Count());
  s_sceneCameraPos = position;
  s_sceneCameraDir = direction;
}

void ModelSceneSetSharpness(float sharpness) {
  float adjusted = sharpness - 1.0f;

  if (adjusted != adjusted || adjusted >= -1.0f) {
    if (adjusted <= 1.0f) {
      s_sceneSharpness = adjusted;
    } else {
      s_sceneSharpness = 1.0f;
    }
  } else {
    s_sceneSharpness = -1.0f;
  }
}

static void IModelBaseAddToScene(CModelBase *modelptr, CModelShared *shared) {
  ASSERT(modelptr);
  ASSERT(shared);

  if (modelptr->m_boundsModel) {
    ModelAddToScene(modelptr->m_boundsModel, 0);
  }
  if (modelptr->m_collideModel) {
    ModelAddToScene(modelptr->m_collideModel, 0);
  }
}

static void IModelComplexAddToScene(CModel *model, unsigned int renderFlags) {
  ASSERT(model);

  CModelComplex *modelptr = static_cast<CModelComplex *>(model->data);
  ASSERT(modelptr);

  unsigned int count = modelptr->m_textures.Count();
  unsigned int index;
  for (index = 0; index < count; ++index) {
    if (!TextureGetGxTex(modelptr->m_textures[index].handle, 0, 0)) {
      return;
    }
  }

  CModelShared *shared = reinterpret_cast<CModelShared *>(model->shared);
  ASSERT(shared);

  IModelBaseAddToScene(modelptr, shared);
  if (!(modelptr->m_flags & 0x10)) {
    AddAllGeosetsToScene(
        model, shared, renderFlags, modelptr->m_geosets.Ptr(), modelptr->m_geosetColor.Ptr(), modelptr->m_geosets.Count(),
        modelptr->m_materials.Ptr(), modelptr->m_materials.Count()
    );
    if (!(renderFlags & 2)) {
      AddEmitters2ToScene(model, shared);
      AddRibbonsToScene(model, shared);
    }
  }

  count = modelptr->m_custGeosets.Count();
  for (index = 0; index < count; ++index) {
    NTempest::C3Vector position = modelptr->m_custGeosets[index].position * modelptr->m_modelToWorld;
    EnqueueSimpleObject(model, &modelptr->m_custGeosets[index], SORTOBJ_CUSTOM_GEO, position, 0);
  }

  count = modelptr->m_attached.Count();
  for (index = 0; index < count; ++index) {
    int enabled = 1;

    if (modelptr->m_anim) {
      if (modelptr->m_attachmentFlags[index] & 1) {
        enabled = modelptr->m_attachmentFlags[index] & 2;
      } else {
        enabled = AnimIsAttachmentEnabled(modelptr->m_anim, index);
        modelptr->m_attachmentFlags[index] = (enabled ? 2 : 0) | 1;
      }
    }

    if (enabled) {
      TSList<LINKUNIQUE, TSGetLink<LINKUNIQUE> > &attached = modelptr->m_attached.Ptr()[index];
      LINKUNIQUE                                 *link;
      LINKUNIQUE                                 *next;

      for (link = attached.Head(); link; link = next) {
        next = attached.Next(link);
        ModelAddToScene(link->child, 0);
      }
    }
  }
}

static void IModelSimpleAddToScene(CModel *model, unsigned int renderFlags) {
  ASSERT(model);

  CModelSimple *modelptr = static_cast<CModelSimple *>(model->data);
  ASSERT(modelptr);

  unsigned int count = modelptr->m_textures.Count();
  unsigned int index;
  for (index = 0; index < count; ++index) {
    if (!TextureGetGxTex(modelptr->m_textures[index].handle, 0, 0)) {
      return;
    }
  }

  CModelShared *shared = reinterpret_cast<CModelShared *>(model->shared);
  ASSERT(shared);

  IModelBaseAddToScene(modelptr, shared);
  if (!(modelptr->m_flags & 0x10)) {
    AddAllGeosetsToScene(
        model, shared, renderFlags, modelptr->m_geosets.Ptr(), modelptr->m_geosetColor.Ptr(), modelptr->m_geosets.Count(),
        modelptr->m_materials.Ptr(), modelptr->m_materials.Count()
    );
  }

  count = modelptr->m_custGeosets.Count();
  for (index = 0; index < count; ++index) {
    NTempest::C3Vector position = modelptr->m_custGeosets[index].position * modelptr->m_modelToWorld;
    EnqueueSimpleObject(model, &modelptr->m_custGeosets[index], SORTOBJ_CUSTOM_GEO, position, 0);
  }
}

void ModelAddToScene(HMODEL model, unsigned int renderFlags) {
  CModel     *modelptr = reinterpret_cast<CModel *>(model);
  CModelBase *unique;

  FATALASSERT(modelptr);

  if (IModelDerefHandle(modelptr, &unique)) {
    ActivityBegin(ACTIVITY_MODEL);
    if (unique->m_flags & 0x20) {
      IModelComplexAddToScene(modelptr, renderFlags);
    } else {
      IModelSimpleAddToScene(modelptr, renderFlags);
    }
    ActivityEnd(ACTIVITY_MODEL);
  }
}

void ModelAddToScene(NTempest::C3Vector &position, int priorityPlane, void(*callback)(void *, int), void *param1, int param2) {
  CTransparentObject *object = s_trLayerPool.New();
  object->sortType = SORTOBJ_CUSTOM_MODEL;
  object->priorityPlane = priorityPlane;
  object->sqDistFromCamera = (position - s_sceneCameraPos).SquaredMag();
  object->cust.callback = callback;
  object->cust.param1 = param1;
  object->cust.param2 = param2;
}

void ModelRenderScene(CStatus *status) {
  ActivityBegin(ACTIVITY_MODEL);
  IModelRenderSceneOpaque(status);
  IModelRenderSceneTransparent(status);

  for (unsigned int opaqueIndex = 0; opaqueIndex < s_opLayerPool.Count(); ++opaqueIndex) {
    if (s_opLayerPool.Ptr()[opaqueIndex].model) {
      HandleClose(s_opLayerPool.Ptr()[opaqueIndex].model);
    }
    s_opLayerPool.Ptr()[opaqueIndex].model = 0;
  }
  s_opLayerPool.SetCount(0);

  for (unsigned int transparentIndex = 0; transparentIndex < s_trLayerPool.Count(); ++transparentIndex) {
    if (s_trLayerPool.Ptr()[transparentIndex].sortType < SORTOBJ_CUSTOM_MODEL && s_trLayerPool.Ptr()[transparentIndex].geo.model) {
      HandleClose(s_trLayerPool.Ptr()[transparentIndex].geo.model);
    }
  }
  s_trLayerPool.SetCount(0);
  ActivityEnd(ACTIVITY_MODEL);
}

void ModelRenderSceneOpaque(CStatus *status) {
  ActivityBegin(ACTIVITY_MODEL);
  IModelRenderSceneOpaque(status);

  for (unsigned int index = 0; index < s_opLayerPool.Count(); ++index) {
    if (s_opLayerPool.Ptr()[index].model) {
      HandleClose(s_opLayerPool.Ptr()[index].model);
    }
    s_opLayerPool.Ptr()[index].model = 0;
  }
  s_opLayerPool.SetCount(0);
  ActivityEnd(ACTIVITY_MODEL);
}

void ModelRenderSceneTransparent(CStatus *status) {
  ActivityBegin(ACTIVITY_MODEL);
  IModelRenderSceneTransparent(status);

  for (unsigned int index = 0; index < s_trLayerPool.Count(); ++index) {
    if (s_trLayerPool.Ptr()[index].sortType < SORTOBJ_CUSTOM_MODEL && s_trLayerPool.Ptr()[index].geo.model) {
      HandleClose(s_trLayerPool.Ptr()[index].geo.model);
    }
  }
  s_trLayerPool.SetCount(0);
  ActivityEnd(ACTIVITY_MODEL);
}

static void ModelBaseRender(CModelBase *modelptr) {
  if (modelptr->m_boundsModel) {
    ModelRender(modelptr->m_boundsModel, 0, 0);
  }
  if (modelptr->m_collideModel) {
    ModelRender(modelptr->m_collideModel, 0, 0);
  }
}

static void GeosetComplexRender(CModel *model, CGeoset *geoUnique, CGeosetShared *geoShared, unsigned int renderFlags, CStatus *status) {
  CModelRenderData renderData;
  FillInRenderData(model, renderFlags, &renderData);
  RenderGeosetCheckVis(&renderData, geoUnique, geoShared, status);
}

static void ModelComplexRender(HMODEL modelHandle, CModel *model, unsigned int renderFlags, CStatus *status) {
  CModelComplex *modelptr = static_cast<CModelComplex *>(model->data);
  CModelShared  *shared = reinterpret_cast<CModelShared *>(model->shared);

  unsigned int index;
  if (!(modelptr->m_flags & 0x10)) {
    for (index = 0; index < shared->numGeosets; ++index) {
      GeosetComplexRender(model, &modelptr->m_geosets[index], &shared->geosets[index], renderFlags, status);
    }

    unsigned int numAddlGeosets = modelptr->m_addlGeosets.Count();
    for (index = 0; index < numAddlGeosets; ++index) {
      GeosetComplexRender(model, &modelptr->m_geosets[index + shared->numGeosets], &modelptr->m_addlGeosets[index], renderFlags, status);
    }

    if (!(renderFlags & 2)) {
      for (index = 0; index < modelptr->m_emitters2.Count(); ++index) {
        modelptr->m_emitters2[index]->Render();
      }
      for (index = 0; index < modelptr->m_ribbons.Count(); ++index) {
        modelptr->m_ribbons[index]->Render();
      }
    }
  }

  ModelBaseRender(modelptr);

  unsigned int numCustGeos = modelptr->m_custGeosets.Count();
  for (index = 0; index < numCustGeos; ++index) {
    RenderCustomGeoset(modelHandle, &modelptr->m_custGeosets[index]);
  }

  unsigned int numAttached = modelptr->m_attached.Count();
  for (index = 0; index < numAttached; ++index) {
    int enabled = 1;
    if (modelptr->m_anim) {
      if (modelptr->m_attachmentFlags[index] & 1) {
        enabled = modelptr->m_attachmentFlags[index] & 2;
      } else {
        enabled = AnimIsAttachmentEnabled(modelptr->m_anim, index);
        modelptr->m_attachmentFlags[index] = (enabled ? 2 : 0) | 1;
      }
    }

    if (enabled) {
      ITERATELIST(LINKUNIQUE, modelptr->m_attached[index], link) {
        ModelRender(link->child, 0, 0);
      }
    }
  }
}

static void ModelSimpleRender(CModel *model, unsigned int renderFlags, CStatus *status) {
  CModelSimple *modelptr = static_cast<CModelSimple *>(model->data);
  CModelShared *shared = reinterpret_cast<CModelShared *>(model->shared);

  unsigned int numGeosets = modelptr->m_geosets.Count();
  if (!(modelptr->m_flags & 0x10)) {
    for (unsigned int index = 0; index < numGeosets; ++index) {
      CModelRenderData renderData;
      FillInRenderData(model, renderFlags, &renderData);
      RenderGeosetCheckVis(&renderData, &modelptr->m_geosets[index], &shared->geosets[index], status);
    }
  }

  ModelBaseRender(modelptr);
}

void ModelRender(HMODEL model, CStatus *status, unsigned int renderFlags) {
  FATALASSERT(model);

  CModelBase *unique;
  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    ActivityBegin(ACTIVITY_MODEL);
    GxRsPush();
    if (unique->m_flags & 0x20) {
      ModelComplexRender(model, reinterpret_cast<CModel *>(model), renderFlags, status);
    } else {
      ModelSimpleRender(reinterpret_cast<CModel *>(model), renderFlags, status);
    }
    GxRsPop();
    ActivityEnd(ACTIVITY_MODEL);
  }
}

void ModelSceneCalcFrustumPlanes() {
  NTempest::C44Matrix viewProj;
  GxXformViewProj(viewProj);
  GxuXformCalcFrustumPlanes(viewProj, s_frustumPlanes);
}

int ModelTestSphere(HMODEL model, NTempest::C34Matrix &orientation, float scale, int testLinkedModels) {
  CModelBase   *modelptr;
  CModelShared *shared;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &modelptr, &shared)) {
    return 0;
  }

  NTempest::CAaSphere bounds;
  IModelGetBoundingSphere(modelptr, shared, &bounds);
  bounds.c *= scale;
  bounds.c *= orientation;
  bounds.r *= scale;
  if (GxuTestSphereAndFrustumPlanes(bounds.c, bounds.r, s_frustumPlanes)) {
    return 1;
  }

  if (testLinkedModels && (modelptr->m_flags & 0x20)) {
    CModelComplex *complex = static_cast<CModelComplex *>(modelptr);
    unsigned int   numAttachments = complex->m_attached.Count();
    for (unsigned int i = 0; i < numAttachments; ++i) {
      for (LINKUNIQUE *link = complex->m_attached[i].Head(); link; link = link->Next()) {
        NTempest::C34Matrix childMatrix = orientation;
        if (ModelTestSphere(link->child, childMatrix, scale * link->scale, 1)) {
          return 1;
        }
      }
    }
  }
  return 0;
}

void ModelSceneGetFrustumPlanes(NTempest::C4Vector *const fp) {
  for (unsigned int i = 0; i < 6; ++i) {
    fp[i] = s_frustumPlanes[i];
  }
}

void ModelSceneSetFrustumPlanes(NTempest::C4Vector *const fp) {
  for (unsigned int i = 0; i < 6; ++i) {
    s_frustumPlanes[i] = fp[i];
  }
}

int ModelIntersectLineSegment(HMODEL__* model, float scale, const NTempest::C3Vector& a, const NTempest::C3Vector& b, float radius, float* linePos, int testLinkedModels) {
  CModelBase   *modelptr;
  CModelShared *shared;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &modelptr, &shared)) {
    return 0;
  }

  NTempest::CAaSphere bounds;
  IModelGetBoundingSphere(modelptr, shared, &bounds);
  TransformBounds(modelptr->m_modelToWorld, scale, &bounds);

  NTempest::C3Vector closest = bounds.c - a;
  NTempest::C3Vector segment = b - a;
  float              position = NTempest::C3Vector::Dot(closest, segment);
  float              divisor = NTempest::C3Vector::Dot(segment, segment);
  if (position < 0.0f) {
    position = 0.0f;
  } else if (position <= divisor) {
    position /= divisor;
    closest.x -= segment.x * position;
    closest.y -= segment.y * position;
    closest.z -= segment.z * position;
  } else {
    position = 1.0f;
    closest.x -= segment.x;
    closest.y -= segment.y;
    closest.z -= segment.z;
  }

  *linePos = position;
  if (closest.SquaredMag() <= bounds.r * bounds.r) {
    return IModelTestRay(modelptr, shared, a, b, linePos, testLinkedModels);
  }
  *linePos = INFINITY;
  return 0;
}

static int LineSegmentIntersectBox(const NTempest::C34Matrix& boxToWorld, float boxScale, const NTempest::C3Vector& boxMin, const NTempest::C3Vector& boxMax, const NTempest::C3Vector& a, const NTempest::C3Vector& b, float* linePos) {
  ASSERT(NTempest::CMath::fnotequal_(boxScale, 0.0f));

  NTempest::C34Matrix worldToBox = boxToWorld.AffineInverse(boxScale);
  NTempest::C3Vector  boxA = a * worldToBox;
  NTempest::C3Vector  boxB = b * worldToBox;
  NTempest::C3Vector  lineSegment = boxB - boxA;
  ASSERT(NTempest::CMath::fnotequal_(lineSegment.Mag(), 0.0f));

  float t0 = 0.0f;
  float t1 = 1.0f;
  float q;

  q = boxMin.x - boxA.x;
  if (lineSegment.x > 0.0f) {
    if (q > lineSegment.x * t1) return 0;
    if (q > lineSegment.x * t0) t0 = q / lineSegment.x;
  } else if (lineSegment.x < 0.0f) {
    if (q > lineSegment.x * t0) return 0;
    if (q > lineSegment.x * t1) t1 = q / lineSegment.x;
  } else if (q > 0.0f) {
    return 0;
  }

  q = boxA.x - boxMax.x;
  if (-lineSegment.x > 0.0f) {
    if (q > -lineSegment.x * t1) return 0;
    if (q > -lineSegment.x * t0) t0 = q / -lineSegment.x;
  } else if (-lineSegment.x < 0.0f) {
    if (q > -lineSegment.x * t0) return 0;
    if (q > -lineSegment.x * t1) t1 = q / -lineSegment.x;
  } else if (q > 0.0f) {
    return 0;
  }

  q = boxMin.y - boxA.y;
  if (lineSegment.y > 0.0f) {
    if (q > lineSegment.y * t1) return 0;
    if (q > lineSegment.y * t0) t0 = q / lineSegment.y;
  } else if (lineSegment.y < 0.0f) {
    if (q > lineSegment.y * t0) return 0;
    if (q > lineSegment.y * t1) t1 = q / lineSegment.y;
  } else if (q > 0.0f) {
    return 0;
  }

  q = boxA.y - boxMax.y;
  if (-lineSegment.y > 0.0f) {
    if (q > -lineSegment.y * t1) return 0;
    if (q > -lineSegment.y * t0) t0 = q / -lineSegment.y;
  } else if (-lineSegment.y < 0.0f) {
    if (q > -lineSegment.y * t0) return 0;
    if (q > -lineSegment.y * t1) t1 = q / -lineSegment.y;
  } else if (q > 0.0f) {
    return 0;
  }

  q = boxMin.z - boxA.z;
  if (lineSegment.z > 0.0f) {
    if (q > lineSegment.z * t1) return 0;
    if (q > lineSegment.z * t0) t0 = q / lineSegment.z;
  } else if (lineSegment.z < 0.0f) {
    if (q > lineSegment.z * t0) return 0;
    if (q > lineSegment.z * t1) t1 = q / lineSegment.z;
  } else if (q > 0.0f) {
    return 0;
  }

  q = boxA.z - boxMax.z;
  if (-lineSegment.z > 0.0f) {
    if (q > -lineSegment.z * t1) return 0;
    if (q > -lineSegment.z * t0) t0 = q / -lineSegment.z;
  } else if (-lineSegment.z < 0.0f) {
    if (q > -lineSegment.z * t0) return 0;
    if (q > -lineSegment.z * t1) t1 = q / -lineSegment.z;
  } else if (q > 0.0f) {
    return 0;
  }

  if (linePos) {
    *linePos = t0;
  }
  return 1;
}

static int LineSegmentIntersectCylinder(const NTempest::C34Matrix& cylToWorld, float cylScale, const NTempest::C3Vector& cylBottom, float cylHeight, float cylRadius, const NTempest::C3Vector& a, const NTempest::C3Vector& b, float* linePos) {
  ASSERT(NTempest::CMath::fnotequal_(cylScale, 0.0f));

  NTempest::C34Matrix worldToCyl = cylToWorld.AffineInverse(cylScale);
  NTempest::C3Vector  cylA = a * worldToCyl;
  NTempest::C3Vector  cylB = b * worldToCyl;
  float               cylTop = cylBottom.z + cylHeight;
  if ((cylA.z < cylBottom.z && cylB.z < cylBottom.z) || (cylA.z > cylTop && cylB.z > cylTop)) {
    return 0;
  }

  NTempest::C2Vector closest(cylBottom.x - cylA.x, cylBottom.y - cylA.y);
  NTempest::C2Vector line(cylB.x - cylA.x, cylB.y - cylA.y);
  float              position = closest.x * line.x + closest.y * line.y;
  float              divisor = line.x * line.x + line.y * line.y;
  if (position < 0.0f) {
    position = 0.0f;
  } else if (position <= divisor) {
    position /= divisor;
    closest.x -= line.x * position;
    closest.y -= line.y * position;
  } else {
    position = 1.0f;
    closest.x -= line.x;
    closest.y -= line.y;
  }

  *linePos = position;
  return closest.SquaredMag() <= cylRadius * cylRadius;
}

static int LineSegmentIntersectSphere(const NTempest::C34Matrix& sphToWorld, float sphScale, const NTempest::C3Vector& sphCenter, float sphRadius, const NTempest::C3Vector& a, const NTempest::C3Vector& b, float* linePos) {
  NTempest::C3Vector center = sphCenter * sphToWorld;
  float              radius = sphScale * sphRadius;
  NTempest::C3Vector closest = center - a;
  NTempest::C3Vector line = b - a;
  float              position = NTempest::C3Vector::Dot(closest, line);
  float              divisor = NTempest::C3Vector::Dot(line, line);
  if (position < 0.0f) {
    position = 0.0f;
  } else if (position <= divisor) {
    position /= divisor;
    closest.x -= line.x * position;
    closest.y -= line.y * position;
    closest.z -= line.z * position;
  } else {
    position = 1.0f;
    closest.x -= line.x;
    closest.y -= line.y;
    closest.z -= line.z;
  }

  *linePos = position;
  return closest.SquaredMag() <= radius * radius;
}

static int IModelTestCollisionVolumes(CModelComplex* modelptr, CModelShared* shared, float scale, const NTempest::C3Vector& a, const NTempest::C3Vector& b, float* linePos) {
  unsigned int count = shared->hitTest.Count();
  unsigned int pivotOffset = shared->positions.Count() - count;
  int          hitVolume = 0;
  float        hitVolumeLinePos = 1.0f;
  for (unsigned int i = 0; i < count; ++i) {
    const CHitTest             &hit = shared->hitTest[i];
    const NTempest::C3Vector   &translation = shared->positions[pivotOffset + i];
    const NTempest::C34Matrix  &hitToWorld = modelptr->m_hitTestMtx[i];
    float                       currlinePos = 1.0f;
    int                         result = 0;
    switch (hit.type) {
      case COLLIDE_BOX: {
        NTempest::C3Vector boxMin = translation + hit.extent[0];
        NTempest::C3Vector boxMax = translation + hit.extent[1];
        result = LineSegmentIntersectBox(hitToWorld, scale, boxMin, boxMax, a, b, &currlinePos);
        break;
      }

      case COLLIDE_CYLINDER: {
        NTempest::C3Vector cylBottom = translation + hit.extent[0];
        result = LineSegmentIntersectCylinder(
            hitToWorld, scale, cylBottom, hit.extent[1].z - hit.extent[0].z, hit.radius, a, b, &currlinePos
        );
        break;
      }

      case COLLIDE_SPHERE: {
        NTempest::C3Vector sphCenter = translation + hit.extent[0];
        result = LineSegmentIntersectSphere(hitToWorld, scale, sphCenter, hit.radius, a, b, &currlinePos);
        break;
      }

      default:
        ASSERT(0);
        break;
    }

    if (result) {
      hitVolume = 1;
      if (currlinePos < hitVolumeLinePos) {
        hitVolumeLinePos = currlinePos;
      }
    }
  }

  if (hitVolume) {
    *linePos = hitVolumeLinePos;
  }
  return hitVolume;
}

static void CreatePlanarQuadGeometry(
    const NTempest::C3Vector &base,
    float length,
    float width,
    TSGrowableArray<NTempest::C3Vector> *positions,
    TSGrowableArray<NTempest::C3Vector> *normals,
    TSGrowableArray<NTempest::C2Vector> *texCoords,
    TSGrowableArray<unsigned short> *vertIndices,
    TSGrowableArray<CPrimitive> *primitives
) {
  unsigned int vertexOffset = positions->Count();
  unsigned int indexOffset = vertIndices->Count();

  texCoords->SetCount(texCoords->Count() + 4);
  vertIndices->SetCount(indexOffset + 6);
  (*vertIndices)[indexOffset + 0] = static_cast<unsigned short>(vertexOffset);
  (*vertIndices)[indexOffset + 1] = static_cast<unsigned short>(vertexOffset + 2);
  (*vertIndices)[indexOffset + 2] = static_cast<unsigned short>(vertexOffset + 1);
  (*vertIndices)[indexOffset + 3] = static_cast<unsigned short>(vertexOffset + 1);
  (*vertIndices)[indexOffset + 4] = static_cast<unsigned short>(vertexOffset + 2);
  (*vertIndices)[indexOffset + 5] = static_cast<unsigned short>(vertexOffset + 3);

  positions->SetCount(vertexOffset + 4);
  float halfLength = length * 0.5f;
  float halfWidth = width * 0.5f;
  (*positions)[vertexOffset + 0].Set(base.x - halfLength, base.y - halfWidth, base.z);
  (*positions)[vertexOffset + 1].Set(base.x - halfLength, base.y + halfWidth, base.z);
  (*positions)[vertexOffset + 2].Set(base.x + halfLength, base.y - halfWidth, base.z);
  (*positions)[vertexOffset + 3].Set(base.x + halfLength, base.y + halfWidth, base.z);

  unsigned int normalOffset = normals->Count();
  normals->SetCount(normalOffset + 4);
  for (unsigned int i = normalOffset; i < normals->Count(); ++i) {
    (*normals)[i].Set(0.0f, 0.0f, 1.0f);
  }

  CPrimitive *primitive = primitives->New();
  primitive->type = GxPrim_Triangles;
  primitive->vertexCount = 6;
}

static void GenerateCylinderVerts(
    const NTempest::C3Vector &base,
    const NTempest::C3Vector &height,
    float radius,
    unsigned int segments,
    TSGrowableArray<NTempest::C3Vector> *vertices,
    TSGrowableArray<NTempest::C3Vector> *normals
) {
  unsigned int offset = vertices->Count();
  unsigned int count = offset + 4 * segments + 2;
  vertices->SetCount(count);
  normals->SetCount(count);

  NTempest::C3Vector topNormal = height - base;
  topNormal.Normalize();
  NTempest::C3Vector bottomNormal = -topNormal;
  NTempest::C3Vector perp(
      topNormal.y - topNormal.z,
      topNormal.z - topNormal.x,
      topNormal.x - topNormal.y
  );
  perp.Normalize();
  perp *= radius;

  unsigned int vertex = offset;
  (*vertices)[vertex] = height;
  (*normals)[vertex++] = topNormal;

  unsigned int i;
  for (i = 0; i < segments; ++i) {
    float angle = static_cast<float>(i) * TWO_PI / static_cast<float>(segments);
    NTempest::C4Quaternion rotation(angle, topNormal);
    NTempest::C3Vector rot = rotation * perp;
    (*vertices)[vertex] = rot + height;
    (*normals)[vertex++] = topNormal;
  }

  (*vertices)[vertex] = base;
  (*normals)[vertex++] = bottomNormal;

  for (i = 0; i < segments; ++i) {
    float angle = static_cast<float>(i) * TWO_PI / static_cast<float>(segments);
    NTempest::C4Quaternion rotation(angle, topNormal);
    NTempest::C3Vector rot = rotation * perp;
    (*vertices)[vertex] = rot + base;
    (*normals)[vertex++] = bottomNormal;
  }

  for (i = 0; i < segments; ++i) {
    float angle = static_cast<float>(i) * TWO_PI / static_cast<float>(segments);
    NTempest::C4Quaternion rotation(angle, topNormal);
    NTempest::C3Vector rot = rotation * perp;
    NTempest::C3Vector normal = rot;
    normal.Normalize();
    (*vertices)[vertex] = rot + height;
    (*normals)[vertex++] = normal;
  }

  for (i = 0; i < segments; ++i) {
    float angle = static_cast<float>(i) * TWO_PI / static_cast<float>(segments);
    NTempest::C4Quaternion rotation(angle, topNormal);
    NTempest::C3Vector rot = rotation * perp;
    NTempest::C3Vector normal = rot;
    normal.Normalize();
    (*vertices)[vertex] = rot + base;
    (*normals)[vertex++] = normal;
  }
}

static int CreateCylinderGeometry(
    const NTempest::C3Vector &base,
    const NTempest::C3Vector &height,
    float radius,
    TSGrowableArray<NTempest::C3Vector> *vertices,
    TSGrowableArray<NTempest::C3Vector> *normals,
    TSGrowableArray<NTempest::C2Vector> *texCoords,
    TSGrowableArray<unsigned short> *vertIndices,
    TSGrowableArray<CPrimitive> *primitives
) {
  const unsigned int segments = 16;
  unsigned int vertexOffset = vertices->Count();
  unsigned int indexOffset = vertIndices->Count();

  GenerateCylinderVerts(base, height, radius, segments, vertices, normals);
  texCoords->SetCount(vertices->Count());
  vertIndices->SetCount(indexOffset + 192);

  unsigned short *indices = vertIndices->Ptr() + indexOffset;
  unsigned int segment;
  for (segment = 0; segment < segments; ++segment) {
    unsigned int next = (segment + 1) & 0xF;
    *indices++ = static_cast<unsigned short>(vertexOffset + 1 + next);
    *indices++ = static_cast<unsigned short>(vertexOffset + 1 + segment);
    *indices++ = static_cast<unsigned short>(vertexOffset);
  }
  for (segment = 0; segment < segments; ++segment) {
    unsigned int next = (segment + 1) & 0xF;
    *indices++ = static_cast<unsigned short>(vertexOffset + 17);
    *indices++ = static_cast<unsigned short>(vertexOffset + 18 + segment);
    *indices++ = static_cast<unsigned short>(vertexOffset + 18 + next);
  }
  for (segment = 0; segment < segments; ++segment) {
    unsigned int next = (segment + 1) & 0xF;
    *indices++ = static_cast<unsigned short>(vertexOffset + 50 + next);
    *indices++ = static_cast<unsigned short>(vertexOffset + 50 + segment);
    *indices++ = static_cast<unsigned short>(vertexOffset + 34 + segment);
    *indices++ = static_cast<unsigned short>(vertexOffset + 34 + next);
    *indices++ = static_cast<unsigned short>(vertexOffset + 50 + next);
    *indices++ = static_cast<unsigned short>(vertexOffset + 34 + segment);
  }

  CPrimitive *primitive = primitives->New();
  primitive->type = GxPrim_Triangles;
  primitive->vertexCount = 192;
  return 1;
}

static void SetVertexMatrixIndices(CGeosetShared *geoShared, const TSGrowableArray<unsigned int> &groupVertexCounts) {
  unsigned int numGroups = groupVertexCounts.Count();
  if (numGroups < 2) {
    return;
  }

  geoShared->boneWeights.SetCount(geoShared->position.Count());
  unsigned char *primBone = geoShared->boneWeights.Ptr();

  for (unsigned int i = 0; i < numGroups; ++i) {
    unsigned int vertCount = groupVertexCounts[i];
    if (vertCount) {
      memset(primBone, static_cast<unsigned char>(i), vertCount);
      primBone += vertCount;
    }
  }

  geoShared->vertexShader = GxVS_Skin;
}

static void BuildComplexGeoset(
    const TSGrowableArray<NTempest::C3Vector> &position,
    const TSGrowableArray<NTempest::C3Vector> &normal,
    const TSGrowableArray<NTempest::C2Vector> &texCoord,
    const TSGrowableArray<unsigned short> &primitiveVertices,
    const TSGrowableArray<unsigned int> &groupVertex,
    const TSGrowableArray<unsigned int> &groupCounts,
    const TSGrowableArray<unsigned int> &matrices,
    const TSGrowableArray<CPrimitive> &primitives,
    unsigned int materialId,
    unsigned int geosetId,
    CGeosetShared *geoShared
) {
  ASSERT(geoShared);

  geoShared->materialId = materialId;
  geoShared->position.Set(position.Count(), position.Ptr());
  geoShared->normal.Set(normal.Count(), normal.Ptr());
  geoShared->primitive.Set(primitives.Count(), primitives.Ptr());
  geoShared->texCoord.SetCount(1);
  geoShared->texCoord[0].Set(texCoord.Count(), texCoord.Ptr());
  geoShared->groupMatrixCounts.Set(groupCounts.Count(), groupCounts.Ptr());
  geoShared->matrices.Set(matrices.Count(), matrices.Ptr());
  geoShared->geosetId = geosetId;
  geoShared->primitiveVertices.Set(primitiveVertices.Count(), primitiveVertices.Ptr());
  SetVertexMatrixIndices(geoShared, groupVertex);
}

static void IModelGeosetAdd(
    CModelComplex *modelptr,
    CModelShared *shared,
    const TSGrowableArray<NTempest::C3Vector> &position,
    const TSGrowableArray<NTempest::C3Vector> &normal,
    const TSGrowableArray<NTempest::C2Vector> &texCoord,
    const TSGrowableArray<unsigned short> &primitiveVertices,
    const TSGrowableArray<unsigned int> &groupVertex,
    const TSGrowableArray<unsigned int> &groupCounts,
    const TSGrowableArray<unsigned int> &matrices,
    const TSGrowableArray<CPrimitive> &primitives,
    HTEXTURE texture,
    EGxBlend blendMode,
    unsigned int disables,
    NTempest::CImVector color
) {
  ASSERT(modelptr);
  ASSERT(shared);

  unsigned int materialId = modelptr->m_materials.Count();
  unsigned int textureId = modelptr->m_textures.Count();
  unsigned int geosetId = modelptr->m_geosets.Count();

  modelptr->m_textures.New();
  modelptr->m_materials.New();
  modelptr->m_geosets.New();
  modelptr->m_addlGeosets.SetCount(geosetId - shared->numGeosets + 1);
  modelptr->m_geosetColor.SetCount(geosetId + 1);

  HMATERIAL material = BuildSimpleMaterial(&modelptr->m_textures[textureId], textureId, texture, blendMode, disables, 0);
  ASSERT(material);
  modelptr->m_materials[materialId] = material;
  modelptr->m_geosetColor[geosetId].animatedColor = color;

  BuildComplexGeoset(
      position,
      normal,
      texCoord,
      primitiveVertices,
      groupVertex,
      groupCounts,
      matrices,
      primitives,
      materialId,
      geosetId,
      &modelptr->m_addlGeosets[geosetId - shared->numGeosets]
  );
}

static void IModelHandleGeosetAdd(
    CModel *model,
    const TSGrowableArray<NTempest::C3Vector> &position,
    const TSGrowableArray<NTempest::C3Vector> &normal,
    const TSGrowableArray<NTempest::C2Vector> &texCoord,
    const TSGrowableArray<unsigned short> &primitiveVertices,
    const TSGrowableArray<unsigned int> &groupVertex,
    const TSGrowableArray<unsigned int> &groupCounts,
    const TSGrowableArray<unsigned int> &matrices,
    const TSGrowableArray<CPrimitive> &primitives,
    HTEXTURE texture,
    EGxBlend blendMode,
    unsigned int disables,
    NTempest::CImVector color
) {
  if (model->data && (model->data->m_flags & 0x20)) {
    IModelGeosetAdd(
        static_cast<CModelComplex *>(model->data),
        reinterpret_cast<CModelShared *>(model->shared),
        position,
        normal,
        texCoord,
        primitiveVertices,
        groupVertex,
        groupCounts,
        matrices,
        primitives,
        texture,
        blendMode,
        disables,
        color
    );
  }
}

static void AddHitTestGeometryGeoset(HMODEL__* modelHandle, HTEXTURE__* tex) {
  FATALASSERT(modelHandle);
  FATALASSERT(tex);

  CModelBase   *unique;
  CModelShared *shared;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(modelHandle), &unique, &shared) || !(unique->m_flags & 0x20)) {
    return;
  }

  TSGrowableArray<NTempest::C3Vector> positions;
  TSGrowableArray<NTempest::C3Vector> normals;
  TSGrowableArray<NTempest::C2Vector> texCoords;
  TSGrowableArray<unsigned short>     primVertIndices;
  TSGrowableArray<unsigned int>       groupVertex;
  TSGrowableArray<unsigned int>       groupCounts;
  TSGrowableArray<unsigned int>       matrices;
  TSGrowableArray<CPrimitive>         primitives;

  unsigned int count = shared->hitTest.Count();
  unsigned int boneOffset = shared->numBones - count;
  unsigned int pivotOffset = shared->positions.Count() - count;
  unsigned int i;
  for (i = 0; i < count; ++i) {
    const CHitTest           &hit = shared->hitTest[i];
    const NTempest::C3Vector &pivot = shared->positions[pivotOffset + i];
    unsigned int              oldVerts = positions.Count();
    unsigned int              oldIndices = primVertIndices.Count();

    switch (hit.type) {
      case COLLIDE_BOX: {
        CPrimitive *primitive = primitives.New();
        NTempest::CAaBox bounds;
        bounds.b = pivot + hit.extent[0];
        bounds.t = pivot + hit.extent[1];
        CreateBoxGeometry(bounds, &positions, &normals, &texCoords, &primVertIndices, &primitive->type);
        primitive->vertexCount = primVertIndices.Count() - oldIndices;
        break;
      }

      case COLLIDE_CYLINDER: {
        CreateCylinderGeometry(
            hit.extent[0],
            hit.extent[1],
            hit.radius,
            &positions,
            &normals,
            &texCoords,
            &primVertIndices,
            &primitives
        );
        break;
      }

      case COLLIDE_SPHERE: {
        NTempest::CAaSphere bounds;
        bounds.c = hit.extent[0];
        bounds.r = hit.radius;
        CreateSphereGeometry(bounds, &positions, &normals, &texCoords, &primVertIndices, &primitives);
        break;
      }

      case COLLIDE_PLANE: {
        CreatePlanarQuadGeometry(
            pivot,
            hit.extent[0].x,
            hit.extent[0].y,
            &positions,
            &normals,
            &texCoords,
            &primVertIndices,
            &primitives
        );
        break;
      }

      default:
        break;
    }

    *groupVertex.New() = positions.Count() - oldVerts;
    *groupCounts.New() = 1;
    *matrices.New() = boneOffset + i;
  }

  for (i = 0; i < texCoords.Count(); ++i) {
    texCoords[i] = NTempest::C2Vector(0.0f, 0.0f);
  }

  IModelHandleGeosetAdd(
      reinterpret_cast<CModel *>(modelHandle),
      positions,
      normals,
      texCoords,
      primVertIndices,
      groupVertex,
      groupCounts,
      matrices,
      primitives,
      tex,
      GxBlend_Alpha,
      0,
      NTempest::CImVector(0xFFFFFFFF)
  );
}

int ModelHitTestSphere(HMODEL model, float scale, NTempest::C3Vector &a, NTempest::C3Vector &b, int testLinkedModels, float *linePos) {
  CModelBase   *modelptr;
  CModelShared *shared;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &modelptr, &shared)) {
    return 0;
  }

  NTempest::CAaSphere bounds;
  IModelGetBoundingSphere(modelptr, shared, &bounds);
  TransformBounds(modelptr->m_modelToWorld, scale, &bounds);

  NTempest::C3Vector lineSegment = b - a;
  float              lineLength = lineSegment.Mag();
  *linePos = 0.0f;
  if (lineLength > 0.0f) {
    float divisor = lineSegment.SquaredMag();
    float position = NTempest::C3Vector::Dot(bounds.c - a, lineSegment);
    if (position < 0.0f) {
      position = 0.0f;
    } else if (position > divisor) {
      position = divisor;
    }
    position /= divisor;
    NTempest::C3Vector closest = a + lineSegment * position;
    if ((closest - bounds.c).SquaredMag() <= bounds.r * bounds.r) {
      *linePos = position * lineLength;
      return 1;
    }
  }

  if (testLinkedModels && (modelptr->m_flags & 0x20)) {
    CModelComplex *complex = static_cast<CModelComplex *>(modelptr);
    unsigned int   numAttachments = complex->m_attached.Count();
    for (unsigned int i = 0; i < numAttachments; ++i) {
      for (LINKUNIQUE *link = complex->m_attached[i].Head(); link; link = link->Next()) {
        if (ModelHitTestSphere(link->child, scale * link->scale, a, b, 1, linePos)) {
          return 1;
        }
      }
    }
  }
  return 0;
}

int ModelHasHitTestVolumes(HMODEL model) {
  CModelShared *shared;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &shared)) {
    return 0;
  }
  FATALASSERT(shared);
  return shared->hitTest.Count() != 0;
}

int ModelHitTestVolumes(HMODEL model, float scale, NTempest::C3Vector &a, NTempest::C3Vector &b, int testLinkedModels, float *linePos) {
  CModelBase   *modelptr;
  CModelShared *shared;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &modelptr, &shared)) {
    return 0;
  }

  if (!(modelptr->m_flags & 0x20)) {
    return 0;
  }

  float lineLength = (b - a).Mag();
  float hitVolumeLinePos = 0.0f;
  if (IModelTestCollisionVolumes(static_cast<CModelComplex *>(modelptr), shared, scale, a, b, &hitVolumeLinePos)) {
    *linePos = hitVolumeLinePos * lineLength;
    return 1;
  }

  if (testLinkedModels) {
    CModelComplex *complex = static_cast<CModelComplex *>(modelptr);
    for (unsigned int i = 0; i < complex->m_attached.Count(); ++i) {
      for (LINKUNIQUE *link = complex->m_attached[i].Head(); link; link = link->Next()) {
        CModelShared *childShared;
        if (IModelDerefHandle(reinterpret_cast<CModel *>(link->child), &childShared) &&
            (childShared->hitTest.Count() ? ModelHitTestVolumes(link->child, scale, a, b, 1, linePos)
                                          : ModelHitTestGeometry(link->child, scale, a, b, 1, linePos)))
        {
          return 1;
        }
      }
    }
  }
  return 0;
}

int ModelHitTestGeometry(HMODEL model, float scale, NTempest::C3Vector &a, NTempest::C3Vector &b, int testLinkedModels, float *linePos) {
  CModelBase   *modelptr;
  CModelShared *shared;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &modelptr, &shared)) {
    return 0;
  }

  if (IModelTestRay(modelptr, shared, a, b, linePos, testLinkedModels)) {
    return 1;
  }

  if (testLinkedModels && (modelptr->m_flags & 0x20)) {
    CModelComplex *complex = static_cast<CModelComplex *>(modelptr);
    for (unsigned int i = 0; i < complex->m_attached.Count(); ++i) {
      for (LINKUNIQUE *link = complex->m_attached[i].Head(); link; link = link->Next()) {
        if (ModelHitTestGeometry(link->child, scale, a, b, 1, linePos)) {
          return 1;
        }
      }
    }
  }
  return 0;
}

ModelIntersectResult ModelIntersectLineSegmentEx(HMODEL__* model, float scale, const NTempest::C3Vector& a, const NTempest::C3Vector& b, unsigned int hitTestFlags, float* linePos, float* centerDistSq, int testLinkedModels) {
  FATALASSERT(linePos);
  FATALASSERT(centerDistSq);

  CModelBase   *modelptr;
  CModelShared *shared;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &modelptr, &shared)) {
    return MODEL_INTERSECT_NO_HIT;
  }

  NTempest::CAaSphere bounds;
  IModelGetBoundingSphere(modelptr, shared, &bounds);
  TransformBounds(modelptr->m_modelToWorld, scale, &bounds);

  *centerDistSq = INFINITY;
  float lineLength = (b - a).Mag();
  float hitLinePos = INFINITY;
  ModelIntersectResult result = MODEL_INTERSECT_NO_HIT;

  if (hitTestFlags & 0x1) {
    NTempest::C3Vector closest = bounds.c - a;
    NTempest::C3Vector segment = b - a;
    float              position = NTempest::C3Vector::Dot(closest, segment);
    float              divisor = NTempest::C3Vector::Dot(segment, segment);
    if (position < 0.0f) {
      position = 0.0f;
    } else if (position <= divisor) {
      position /= divisor;
      closest.x -= segment.x * position;
      closest.y -= segment.y * position;
      closest.z -= segment.z * position;
    } else {
      position = 1.0f;
      closest.x -= segment.x;
      closest.y -= segment.y;
      closest.z -= segment.z;
    }

    *centerDistSq = closest.SquaredMag();
    if (*centerDistSq > bounds.r * bounds.r) {
      return MODEL_INTERSECT_NO_HIT;
    }
    result = MODEL_INTERSECT_HIT_BOUNDING_SPHERE;
    hitLinePos = position * lineLength;
  }

  if ((hitTestFlags & 0x2) && (modelptr->m_flags & 0x20)) {
    float volumeLinePos = 0.0f;
    if (IModelTestCollisionVolumes(static_cast<CModelComplex *>(modelptr), shared, scale, a, b, &volumeLinePos)) {
      result = MODEL_INTERSECT_HIT_COLLISION_VOLUMES;
      hitLinePos = volumeLinePos * lineLength;
    } else if (!(hitTestFlags & 0x10)) {
      *linePos = hitLinePos;
      return result;
    }
  }

  if (!(hitTestFlags & 0x4)) {
    *linePos = hitLinePos;
    return result;
  }

  float modelHitLinePos;
  if (!IModelTestRay(modelptr, shared, a, b, &modelHitLinePos, testLinkedModels)) {
    *linePos = hitLinePos;
    return result;
  }

  if ((hitTestFlags & 0x8) && modelHitLinePos >= hitLinePos) {
    *linePos = hitLinePos;
  } else {
    *linePos = modelHitLinePos;
  }
  return MODEL_INTERSECT_HIT_MODEL;
}

void ModelShowBoundingSphere(HMODEL model) {
  FATALASSERT(model);

  CModelBase   *unique;
  CModelShared *shared;
  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique, &shared)) {
    if (unique->m_boundsModel) {
      HandleClose(unique->m_boundsModel);
    }
    NTempest::CAaSphere bounds;
    IModelGetBoundingSphere(unique, shared, &bounds);
    HTEXTURE texture = TextureCreateSolid(NTempest::CImVector(0x7F00FFFF), 0);
    if (texture) {
      unique->m_boundsModel = CreateModelBoundingSphere(bounds, texture, GxBlend_Alpha);
      unique->m_flags |= 1;
      HandleClose(texture);
    }
  } else {
    EnqueueModelCommand(reinterpret_cast<CModel *>(model), MODEL_SHOW_BOUNDING_SPHERE);
  }
}

void ModelShowBoundingBox(HMODEL model) {
  CModelBase   *unique;
  CModelShared *shared;
  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique, &shared)) {
    if (unique->m_boundsModel) {
      HandleClose(unique->m_boundsModel);
    }
    NTempest::CAaBox extents;
    IModelGetExtents(unique, shared, &extents);
    unique->m_boundsModel = CreateModelBoundingBox(extents, 0, GxBlend_Alpha);
    unique->m_flags &= ~1u;
  }
}

void ModelHideBounds(HMODEL model) {
  FATALASSERT(model);

  CModelBase *unique;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    EnqueueModelCommand(reinterpret_cast<CModel *>(model), MODEL_HIDE_BOUNDS);
    return;
  }

  if (unique->m_boundsModel) {
    HandleClose(unique->m_boundsModel);
    unique->m_boundsModel = 0;
  }
}

void ModelShowHitTestGeometry(HMODEL__* model) {
  CModelBase   *unique;
  CModelShared *shared;
  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique, &shared) &&
      !(unique->m_flags & 8) &&
      shared->hitTest.Count())
  {
    HTEXTURE texture = TextureCreateSolid(NTempest::CImVector(0x7FFF0000), 0);
    AddHitTestGeometryGeoset(model, texture);
    unique->m_flags |= 8;
    HandleClose(texture);
  }
}

void ModelHideHitTestGeometry(HMODEL__* model) {
  CModelBase *unique;
  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) && (unique->m_flags & 0x28) == 0x28) {
    CModelComplex *complex = static_cast<CModelComplex *>(unique);
    complex->m_geosets.SetCount(complex->m_geosets.Count() - 1);
    complex->m_addlGeosets.SetCount(complex->m_addlGeosets.Count() - 1);
    unique->m_flags &= ~8u;
  }
}

int ModelGetExtents(HMODEL model, NTempest::CAaBox *extents) {
  CModelShared *shared;

  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &shared)) {
    return 0;
  }

  *extents = shared->bounds.extent;
  return 1;
}

int ModelGetSeqExtents(HMODEL model, NTempest::CAaBox *extents) {
  CModelBase   *unique;
  CModelShared *shared;

  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique, &shared)) {
    return 0;
  }

  IModelGetExtents(unique, shared, extents);
  return 1;
}

int ModelGetSeqExtents(HMODEL model, unsigned int seqnum, NTempest::CAaBox *extents) {
  CModelBase   *unique;
  CModelShared *shared;

  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique, &shared)) {
    return 0;
  }

  return IModelGetExtents(unique, shared, seqnum, extents);
}

int ModelGetBounds(HMODEL model, NTempest::CAaSphere *bounds) {
  CModelBase   *unique;
  CModelShared *shared;

  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique, &shared)) {
    return 0;
  }

  IModelGetBoundingSphere(unique, shared, bounds);
  return 1;
}

void ModelSetProject2dCallback(MODELPROJECT2DCALLBACK callback) {
  s_Project2dCallback = callback;
}
