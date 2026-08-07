#include "ModelInternal.h"
#include "CollisionData.h"

#include "Base/Activity.h"
#include "Base/Base.h"
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

static int            CompareTexLayers(COpaqueLayer *a, COpaqueLayer *b);
static CModelTexture *GetModelTextures(CModelBase *unique);
static void           RestoreFog();
static void           SaveFog();
static CModelTexture *GetTextureList(CModelBase *modelptr);
static void           ClearUvTransforms(const CTexLayer &layerUnique, const CTexLayerShared &layerShared);
static UINT           GetNumTexCoordLayers(const CTexLayer &layerUnique, const CTexLayerShared &layerShared);
static void           ClearTransformedUVLayer(const CTexLayerShared &layerShared, UINT tmu);
static void           RenderSingleUVMapPrep(
    CModelRenderData      *modelptr,
    CGeosetShared         *geoShared,
    const CMaterial       &uniqueMtl,
    const CMaterialShared &sharedMtl,
    UINT                   layerId,
    int                    geosetChanged
);
static void SetUvTransforms(CModelBase *modelptr, const CTexLayer &layerUnique, const CTexLayerShared &layerShared);
static void GetTransformedUVLayer(CModelBase *modelptr, const CTexLayerShared &layerShared, UINT tmu);
static void LockVertsAndIndices(CGeosetShared *geoShared, const CTexLayer &uniqueLayer, const CTexLayerShared &sharedLayer);
static void LockVertices(CGeosetShared *geoShared, const CTexLayer &uniqueLayer, const CTexLayerShared &sharedLayer);
static void RenderGeosetPrep(CModelBase *modelptr, CGeoset *geoUnique, CGeosetShared *geoShared);
static void RenderGeosetSingleLayer(
    CModelRenderData *modelptr,
    CGeoset          *geoUnique,
    CGeosetShared    *geoShared,
    UINT              layerId,
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
    UINT              firstLayerId,
    int               geosetChanged,
    CStatus          *status
);
static void
RenderSortedGeoset(CModelRenderData *modelptr, CGeoset *geoUnique, CGeosetShared *geoShared, UINT firstLayerId, int geosetChanged, CStatus *status);
static void Project2d(CGeosetShared *geoShared, const NTempest::CImVector &color);
static BOOL SingleUvMapping(CMaterial *uniqueMtl);
static void RenderGeosetOneUvMapping(CModelRenderData *modelptr, CGeosetShared *geoShared, CMaterial *uniqueMtl, CStatus *status);
static void RenderGeosetLayers(CModelRenderData *modelptr, CGeosetShared *geoShared, CStatus *status);
static void RenderGeoset(CModelRenderData *modelptr, CGeoset *geoUnique, CGeosetShared *geoShared, CStatus *status);
static void IModelRenderSceneOpaque(CStatus *status);
static void IModelRenderSceneTransparent(CStatus *status);
static void ModelBaseRender(CModelBase *modelptr);
static void RenderGeosetCheckVis(CModelRenderData *modelptr, CGeoset *geoUnique, CGeosetShared *geoShared, CStatus *status);
static void GeosetComplexRender(CModel *model, CGeoset *geoUnique, CGeosetShared *geoShared, UINT renderFlags, CStatus *status);
static void ModelComplexRender(HMODEL modelHandle, CModel *model, UINT renderFlags, CStatus *status);
static void ModelSimpleRender(CModel *model, UINT renderFlags, CStatus *status);
static int  IModelTestRay(
    CModelBase               *modelptr,
    CModelShared             *shared,
    const NTempest::C3Vector &rayStart,
    const NTempest::C3Vector &rayEnd,
    float                    *distance,
    int                       testLinkedModels
);
static void SetVertexMatrixIndices(CGeosetShared *geoShared, const TSGrowableArray<UINT> &groupVertexCounts);
static void BuildComplexGeoset(
    const TSGrowableArray<NTempest::C3Vector> &position,
    const TSGrowableArray<NTempest::C3Vector> &normal,
    const TSGrowableArray<NTempest::C2Vector> &texCoord,
    const TSGrowableArray<WORD>               &primitiveVertices,
    const TSGrowableArray<UINT>               &groupVertex,
    const TSGrowableArray<UINT>               &groupCounts,
    const TSGrowableArray<UINT>               &matrices,
    const TSGrowableArray<CPrimitive>         &primitives,
    UINT                                       materialId,
    UINT                                       geosetId,
    CGeosetShared                             *geoShared
);
static void IModelGeosetAdd(
    CModelComplex                             *modelptr,
    CModelShared                              *shared,
    const TSGrowableArray<NTempest::C3Vector> &position,
    const TSGrowableArray<NTempest::C3Vector> &normal,
    const TSGrowableArray<NTempest::C2Vector> &texCoord,
    const TSGrowableArray<WORD>               &primitiveVertices,
    const TSGrowableArray<UINT>               &groupVertex,
    const TSGrowableArray<UINT>               &groupCounts,
    const TSGrowableArray<UINT>               &matrices,
    const TSGrowableArray<CPrimitive>         &primitives,
    HTEXTURE                                   texture,
    EGxBlend                                   blendMode,
    UINT                                       disables,
    NTempest::CImVector                        color
);
static void IModelHandleGeosetAdd(
    CModel                                    *model,
    const TSGrowableArray<NTempest::C3Vector> &position,
    const TSGrowableArray<NTempest::C3Vector> &normal,
    const TSGrowableArray<NTempest::C2Vector> &texCoord,
    const TSGrowableArray<WORD>               &primitiveVertices,
    const TSGrowableArray<UINT>               &groupVertex,
    const TSGrowableArray<UINT>               &groupCounts,
    const TSGrowableArray<UINT>               &matrices,
    const TSGrowableArray<CPrimitive>         &primitives,
    HTEXTURE                                   texture,
    EGxBlend                                   blendMode,
    UINT                                       disables,
    NTempest::CImVector                        color
);
static void CreatePlanarQuadGeometry(
    const NTempest::C3Vector            &base,
    float                                length,
    float                                width,
    TSGrowableArray<NTempest::C3Vector> *positions,
    TSGrowableArray<NTempest::C3Vector> *normals,
    TSGrowableArray<NTempest::C2Vector> *texCoords,
    TSGrowableArray<WORD>               *vertIndices,
    TSGrowableArray<CPrimitive>         *primitives
);
static int CreateCylinderGeometry(
    const NTempest::C3Vector            &base,
    const NTempest::C3Vector            &height,
    const float                          radius,
    TSGrowableArray<NTempest::C3Vector> *vertices,
    TSGrowableArray<NTempest::C3Vector> *normals,
    TSGrowableArray<NTempest::C2Vector> *texCoords,
    TSGrowableArray<WORD>               *vertIndices,
    TSGrowableArray<CPrimitive>         *primitives
);
static void GenerateCylinderVerts(
    const NTempest::C3Vector            &base,
    const NTempest::C3Vector            &height,
    const float                          radius,
    const UINT                           segments,
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
  UINT           flags;
  UINT           firstLayer;
  UINT           passNumber;
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
      LPVOID object;
    } stnd;
    struct {
      void (*callback)(LPVOID, int);
      LPVOID param1;
      int    param2;
    } cust;
  };
};

static WORD                                               vertIndices[36] = {12, 18, 0,  0,  18, 6,  13, 1, 16, 16, 1, 4,  2,  8,  5,  5,  8,  11,
                                                                             7,  19, 10, 10, 19, 22, 3,  9, 15, 15, 9, 21, 17, 23, 14, 14, 23, 20};
static TSGrowableArray<COpaqueLayer>                      s_opLayerPool;
static TSGrowableArray<CTransparentObject>                s_trLayerPool;
static NTempest::C4Vector                                 s_frustumPlanes[6];
static TSGrowableArray<NTempest::C34Matrix>               s_matrixPool;
static UINT                                               s_nextMatrix;
static UINT                                               s_lastFrame;
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
static NTempest::CPriorityQ<COpaqueLayer *, COpaqueLayer> s_opaqueScene;
static WORD                                               s_currAnimFrame;
static NTempest::CPriorityQ<CTransparentObject *, CTransparentObject> s_transparentScene;

static void IModelComplexAddToScene(CModel *model, UINT renderFlags);
static void EnqueueSimpleObject(CModel *model, LPVOID object, SORTABLES sortType, const NTempest::C3Vector &position, UINT priorityPlane);
static void AddAllGeosetsToScene(
    CModel       *modelptr,
    CModelShared *shared,
    UINT          renderFlags,
    CGeoset      *geosets,
    CGeosetColor *geosetColor,
    UINT          numGeosets,
    HMATERIAL    *materials,
    UINT          numMaterials
);
static void AddGeosetToScene(
    CModel        *modelptr,
    UINT           renderFlags,
    CGeoset       *geoUnique,
    CGeosetShared *geoShared,
    CGeosetColor  *geosetsColor,
    HMATERIAL     *materials,
    UINT           numMaterials
);
static BOOL IsOpaque(CMaterial *uniqueMtl);
static void
EnqueueTransparentGeoset(CModel *model, CGeoset *geoUnique, CGeosetShared *geoShared, const NTempest::C3Vector &position, UINT priorityPlane);
static NTempest::C3Vector GetGeosetSortPos(CModelBase *modelUnique, CGeosetShared *geoShared);
static void               AddRibbonsToScene(CModel *modelptr, CModelShared *shared);
static void               IModelBaseAddToScene(CModelBase *modelptr, CModelShared *shared);
static void               IModelSimpleAddToScene(CModel *model, UINT renderFlags);

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

  for (UINT tmu = 0; tmu < 2; ++tmu) {
    if (a.tmuPass[tmu].combiner != b.tmuPass[tmu].combiner) {
      return a.tmuPass[tmu].combiner - b.tmuPass[tmu].combiner;
    }

    HOBJECT bTexture = b.tmuPass[tmu].textureId == static_cast<UINT>(-1) ? 0 : reinterpret_cast<HOBJECT>(bTextures[b.tmuPass[tmu].textureId].handle);
    HOBJECT aTexture = a.tmuPass[tmu].textureId == static_cast<UINT>(-1) ? 0 : reinterpret_cast<HOBJECT>(aTextures[a.tmuPass[tmu].textureId].handle);

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

static BOOL IsOpaque(CMaterial *uniqueMtl) {
  ASSERT(uniqueMtl);

  TSGrowableArray<CTexLayer> &layers = uniqueMtl->layers;
  UINT                        numLayers = layers.Count();
  UINT                        layerIndex;
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

static void EnqueueSimpleObject(CModel *model, LPVOID object, SORTABLES sortType, const NTempest::C3Vector &position, UINT priorityPlane) {
  float               sqDistFromCamera = (position - s_sceneCameraPos).SquaredMag();
  CTransparentObject *sceneObject = s_trLayerPool.New();

  sceneObject->sqDistFromCamera = sqDistFromCamera;
  sceneObject->sortType = sortType;
  sceneObject->priorityPlane = priorityPlane;
  sceneObject->stnd.object = object;
  sceneObject->stnd.model = reinterpret_cast<HMODEL>(HandleCreate(model, "HMODEL"));
}

static void
EnqueueTransparentGeoset(CModel *model, CGeoset *geoUnique, CGeosetShared *geoShared, const NTempest::C3Vector &position, UINT priorityPlane) {
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
    UINT           renderFlags,
    CGeoset       *geoUnique,
    CGeosetShared *geoShared,
    CGeosetColor  *geosetsColor,
    HMATERIAL     *materials,
    UINT           numMaterials
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

  UINT numLayers = uniqueMtl->layers.Count();
  UINT passNumber = 0;
  UINT layerIndex;
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
      UINT j;
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
    UINT          renderFlags,
    CGeoset      *geosets,
    CGeosetColor *geosetColor,
    UINT          numGeosets,
    HMATERIAL    *materials,
    UINT          numMaterials
) {
  UINT index;

  for (index = 0; index < shared->numGeosets; ++index) {
    AddGeosetToScene(modelptr, renderFlags, &geosets[index], &shared->geosets[index], geosetColor, materials, numMaterials);
  }

  UINT numAddlGeosets = numGeosets - shared->numGeosets;
  for (index = 0; index < numAddlGeosets; ++index) {
    AddGeosetToScene(
        modelptr, renderFlags, &geosets[shared->numGeosets + index], &static_cast<CModelComplex *>(modelptr->data)->m_addlGeosets[index], geosetColor,
        materials, numMaterials
    );
  }
}

static void AddEmitters2ToScene(CModel *modelptr, CModelShared *shared) {
  NTempest::C3Vector center;
  UINT               numEmitters = static_cast<CModelComplex *>(modelptr->data)->m_emitters2.Count();
  UINT               index;

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
  UINT               numEmitters = static_cast<CModelComplex *>(modelptr->data)->m_ribbons.Count();
  UINT               index;

  for (index = 0; index < numEmitters; ++index) {
    center = shared->positions[shared->ribbonOrder[index]] * modelptr->data->m_modelToWorld;
    EnqueueSimpleObject(modelptr, static_cast<CModelComplex *>(modelptr->data)->m_ribbons[index], SORTOBJ_RIBBON, center, 0);
  }
}

static UINT GetNumTexCoordLayers(const CTexLayer &layerUnique, const CTexLayerShared &layerShared) {
  UINT numLayers = 0;

  for (UINT tmu = 0; tmu < 2; ++tmu) {
    numLayers += layerUnique.tmuPass[tmu].textureId != static_cast<UINT>(-1) && layerShared.tmuPass[tmu].coordId != static_cast<UINT>(-1);
  }

  return numLayers;
}

static void GetTransformedUVLayer(CModelBase *modelptr, const CTexLayerShared &layerShared, UINT tmu) {
  EGxTextureShader textureShader = layerShared.tmuPass[tmu].textureShader;
  GxRsSet(static_cast<EGxRenderState>(GxRs_TextureShader0 + tmu), textureShader);

  if (textureShader >= GxTS_Affine) {
    NTempest::C34Matrix *texBones = MatrixDeref(modelptr->m_texBones);
    ASSERT(texBones);

    const NTempest::C34Matrix &transform = texBones[layerShared.tmuPass[tmu].transformId];
    GxXformPush(
        static_cast<EGxXform>(GxXform_Tex0 + tmu), NTempest::C44Matrix(
                                                       transform.a0, transform.a1, transform.a2, 0.0f, transform.b0, transform.b1, transform.b2, 0.0f,
                                                       transform.c0, transform.c1, transform.c2, 0.0f, transform.d0, transform.d1, transform.d2, 1.0f
                                                   )
    );
  }
}

static void ClearTransformedUVLayer(const CTexLayerShared &layerShared, UINT tmu) {
  EGxTextureShader textureShader = layerShared.tmuPass[tmu].textureShader;
  GxRsSet(static_cast<EGxRenderState>(GxRs_TextureShader0 + tmu), textureShader);

  if (textureShader >= GxTS_Affine) {
    GxXformPop(static_cast<EGxXform>(GxXform_Tex0 + tmu));
  }
}

static void SetUvTransforms(CModelBase *modelptr, const CTexLayer &layerUnique, const CTexLayerShared &layerShared) {
  switch (GetNumTexCoordLayers(layerUnique, layerShared)) {
    case 1:
      if (layerUnique.tmuPass[1].textureId != static_cast<UINT>(-1) && layerShared.tmuPass[1].coordId != static_cast<UINT>(-1)) {
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
      if (layerUnique.tmuPass[1].textureId != static_cast<UINT>(-1) && layerShared.tmuPass[1].coordId != static_cast<UINT>(-1)) {
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

  for (UINT tmu = 0; tmu < 2; ++tmu) {
    UINT coordId = sharedLayer.tmuPass[tmu].coordId;
    if (uniqueLayer.tmuPass[tmu].textureId == static_cast<UINT>(-1) || coordId == static_cast<UINT>(-1)) {
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
  for (UINT i = 0; i < 4; ++i) {
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

  UINT numLayers = uniqueMtl.layers.Count();
  UINT layersDrawn = 0;
  UINT tmu;
  for (UINT i = 0; i < numLayers; ++i) {
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
      UINT flags = layerShared.tmuPass[tmu].flags;
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
      UINT           textureId = layerUnique.tmuPass[tmu].textureId;
      EGxRenderState textureState = static_cast<EGxRenderState>(GxRs_Texture0 + tmu);
      if (textureId == static_cast<UINT>(-1)) {
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
  UINT                numLayers = sharedMtl->layers.Count();
  UINT                layer = 0;
  BYTE                layerAlpha;
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
    color.a = static_cast<BYTE>(color.a * layerAlpha / 255);
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

  CPrimitive *primitive = geoShared->primitive.Ptr();
  WORD       *indices = geoShared->primitiveVertices.Ptr();
  UINT        numPrimitives = geoShared->primitive.Count();
  for (UINT primitiveId = 0; primitiveId < numPrimitives; ++primitiveId) {
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
    UINT                   layerId,
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
    UINT              firstLayerId,
    int               geosetChanged,
    CStatus          *status
) {
  CMaterialShared *sharedMtl = reinterpret_cast<CMaterialShared *>(uniqueMtl->data);
  ASSERT(sharedMtl);

  const CGeosetColor &geosetColor = modelptr->m_geosetColor[geoShared->geosetId];
  NTempest::CImVector color = geosetColor.animatedColor;
  color.MultiplyRGB(&geosetColor.proceduralColor);
  color.a = static_cast<BYTE>(color.a * uniqueMtl->layers[firstLayerId].layerAlpha / 255);

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

  UINT numLayers = uniqueMtl->layers.Count();
  BYTE geoAlpha = color.a;
  UINT layersDrawn = 0;
  UINT tmu;
  for (UINT i = 0; i < numLayers; ++i) {
    BYTE layerAlpha = uniqueMtl->layers[i].layerAlpha;
    if (!layerAlpha) {
      continue;
    }

    color.a = static_cast<BYTE>(geoAlpha * layerAlpha / 255);
    const CTexLayerShared &stateLayerShared = sharedMtl->layers[i];
    GxRsSet(GxRs_MatDiffuse, color);
    GxRsSet(GxRs_MatEmissive, uniqueMtl->emissiveColor);

    for (tmu = 0; tmu < 2; ++tmu) {
      UINT flags = stateLayerShared.tmuPass[tmu].flags;
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
      UINT           textureId = stateLayerUnique.tmuPass[tmu].textureId;
      EGxRenderState textureState = static_cast<EGxRenderState>(GxRs_Texture0 + tmu);
      if (textureId == static_cast<UINT>(-1) || (stateLayerUnique.blendMode == GxBlend_Opaque && (modelptr->m_renderFlags & 2))) {
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

    CPrimitive *primitive = geoShared->primitive.Ptr();
    WORD       *indices = geoShared->primitiveVertices.Ptr();
    UINT        numPrimitives = geoShared->primitive.Count();
    for (UINT primitiveId = 0; primitiveId < numPrimitives; ++primitiveId) {
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

static BOOL SingleUvMapping(CMaterial *uniqueMtl) {
  if (uniqueMtl->layers.Count() <= 1) {
    return 1;
  }

  CMaterialShared *sharedMtl = reinterpret_cast<CMaterialShared *>(uniqueMtl->data);
  ASSERT(sharedMtl);

  UINT numLayers = uniqueMtl->layers.Count();
  UINT layer;
  BYTE layerAlpha;
  for (layer = 0; layer < numLayers; ++layer) {
    layerAlpha = uniqueMtl->layers[layer].layerAlpha;
    if (layerAlpha) {
      break;
    }
  }

  if (layer == numLayers) {
    return 1;
  }

  UINT firstCoordIds[2];
  UINT tmu;
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

static void TransformBounds(
    const NTempest::C3Vector &position,
    float                     rotationAngle,
    const NTempest::C3Vector &rotationAxis,
    float                     scale,
    NTempest::CAaSphere      *bounds
) {
  ASSERT(bounds);
  WorldMatrixPush();
  WorldMatrixTranslate(position);
  WorldMatrixRotate(rotationAngle, rotationAxis);
  WorldMatrixScale(scale);
  WorldMatrixTransform(&bounds->c);
  WorldMatrixPop();
  bounds->r *= scale;
}

static void TransformBounds(const NTempest::C34Matrix &modelToWorld, float scale, NTempest::CAaSphere *bounds) {
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
    modelptr->m_PickLights(modelptr->m_pickLightsParm, s_sceneCameraPos + geoShared->centroid * modelptr->m_modelToWorld, s_sceneCameraPos, 8);
  }

  NTempest::C34Matrix *weightedBones = MatrixDeref(geoUnique->weightedBones);
  if (geoShared->vertexShader == GxVS_PassThru) {
    GxXformPush(GxXform_World);
    if (weightedBones) {
      GxXformSet(
          GxXform_World,
          NTempest::C44Matrix(
              weightedBones->a0, weightedBones->a1, weightedBones->a2, 0.0f, weightedBones->b0, weightedBones->b1, weightedBones->b2, 0.0f,
              weightedBones->c0, weightedBones->c1, weightedBones->c2, 0.0f, weightedBones->d0, weightedBones->d1, weightedBones->d2, 1.0f
          )
      );
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
    UINT              layerId,
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
  newColor.r = static_cast<BYTE>((geosetColor.proceduralColor.r * newColor.r + 255) >> 8);
  newColor.g = static_cast<BYTE>((geosetColor.proceduralColor.g * newColor.g + 255) >> 8);
  newColor.b = static_cast<BYTE>((geosetColor.proceduralColor.b * newColor.b + 255) >> 8);
  newColor.a = static_cast<BYTE>(newColor.a * uniqueMtl->layers[layerId].layerAlpha / 255);

  RenderSingleUVMapPrep(modelptr, geoShared, *uniqueMtl, *sharedMtl, layerId, geosetChanged);

  if ((modelptr->m_model->data->m_flags & 2) && !(modelptr->m_renderFlags & 4)) {
    GxRsSet(GxRs_NormalizeNormals, 1);
  }

  const CTexLayerShared &layerShared = sharedMtl->layers[layerId];
  GxRsSet(GxRs_MatDiffuse, newColor);
  GxRsSet(GxRs_MatEmissive, uniqueMtl->emissiveColor);

  for (UINT tmu = 0; tmu < 2; ++tmu) {
    UINT flags = layerShared.tmuPass[tmu].flags;
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

    for (UINT tmu = 0; tmu < 2; ++tmu) {
      UINT           textureId = layerUnique.tmuPass[tmu].textureId;
      EGxRenderState textureState = static_cast<EGxRenderState>(GxRs_Texture0 + tmu);
      if (textureId == static_cast<UINT>(-1) || (layerUnique.blendMode == GxBlend_Opaque && (modelptr->m_renderFlags & 2))) {
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

static void
RenderSortedGeoset(CModelRenderData *modelptr, CGeoset *geoUnique, CGeosetShared *geoShared, UINT firstLayerId, int geosetChanged, CStatus *status) {
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

  UINT sequence;
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

  UINT sequence;
  if (shared->seqBounds.Count() && modelptr->m_anim && AnimNeedsSequenceBounds(modelptr->m_anim)) {
    AnimGetPrimarySequence(modelptr->m_anim, &sequence);
    *extents = shared->seqBounds[sequence].extent;
  } else {
    *extents = shared->bounds.extent;
  }
}

static BOOL IModelGetExtents(CModelBase *modelptr, CModelShared *shared, UINT seqnum, NTempest::CAaBox *extents) {
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

static BOOL GeosetTestRay(
    CGeoset                  *geoUnique,
    CGeosetShared            *geoShared,
    CGeosetColor             *geosetColor,
    const NTempest::C3Vector &rayStart,
    const NTempest::C3Vector &rayDirection,
    float                    *distance
) {
  if ((geoUnique->flags & 1) || !geosetColor[geoShared->geosetId].animatedColor.a || (geoShared->flags & 1)) {
    return 0;
  }

  NTempest::C34Matrix *boneMatrices = MatrixDeref(geoUnique->weightedBones);
  if (!boneMatrices) {
    return 0;
  }

  int         foundHit = 0;
  WORD       *indices = geoShared->primitiveVertices.Ptr();
  CPrimitive *primitive = geoShared->primitive.Ptr();
  for (UINT i = 0; i < geoShared->primitive.Count(); ++i, ++primitive) {
    if (primitive->type >= GxPrim_Triangles) {
      float currDistance;
      UINT  primIntersected;
      if (GxuTestRayAndMesh(
              rayStart, rayDirection, boneMatrices, geoShared->groupMatrixCounts.Count(), geoShared->position.Count(), geoShared->position.Ptr(),
              sizeof(NTempest::C3Vector), geoShared->boneWeights.Count(), geoShared->boneWeights.Ptr(), geoShared->vertexShader == 1, primitive->type,
              primitive->vertexCount, indices, currDistance, primIntersected
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

static int
IModelTestRay(CModelSimple *modelptr, CModelShared *shared, const NTempest::C3Vector &rayStart, const NTempest::C3Vector &rayEnd, float *distance) {
  *distance = FLT_MAX;
  NTempest::C3Vector rayDirection = rayEnd - rayStart;
  rayDirection.Normalize();

  int foundHit = 0;
  for (UINT i = 0; i < shared->numGeosets; ++i) {
    foundHit |= GeosetTestRay(&modelptr->m_geosets[i], &shared->geosets[i], modelptr->m_geosetColor.Ptr(), rayStart, rayDirection, distance);
  }
  if (!foundHit) {
    *distance = INFINITY;
  }
  return foundHit;
}

static int IModelTestRay(
    CModelComplex            *modelptr,
    CModelShared             *shared,
    const NTempest::C3Vector &rayStart,
    const NTempest::C3Vector &rayEnd,
    float                    *distance,
    int                       testLinkedModels
) {
  *distance = FLT_MAX;
  NTempest::C3Vector rayDirection = rayEnd - rayStart;
  rayDirection.Normalize();

  int  foundHit = 0;
  UINT i;
  for (i = 0; i < shared->numGeosets; ++i) {
    foundHit |= GeosetTestRay(&modelptr->m_geosets[i], &shared->geosets[i], modelptr->m_geosetColor.Ptr(), rayStart, rayDirection, distance);
  }
  for (i = 0; i < modelptr->m_addlGeosets.Count(); ++i) {
    foundHit |= GeosetTestRay(
        &modelptr->m_geosets[i + shared->numGeosets], &modelptr->m_addlGeosets[i], modelptr->m_geosetColor.Ptr(), rayStart, rayDirection, distance
    );
  }

  if (testLinkedModels) {
    for (i = 0; i < modelptr->m_attached.Count(); ++i) {
      ITERATELIST(LINKUNIQUE, modelptr->m_attached[i], link) {
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

static int IModelTestRay(
    CModelBase               *modelptr,
    CModelShared             *shared,
    const NTempest::C3Vector &rayStart,
    const NTempest::C3Vector &rayEnd,
    float                    *distance,
    int                       testLinkedModels
) {
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
    TSGrowableArray<WORD>               *primVertIndices,
    EGxPrim                             *primType
) {
  UINT vertexOffset = positions->Count();
  positions->SetCount(vertexOffset + 24);

  UINT z;
  UINT y;
  UINT x;
  for (z = 0; z < 2; ++z) {
    for (y = 0; y < 2; ++y) {
      for (x = 0; x < 2; ++x) {
        UINT dst = vertexOffset + (z * 4 + y * 2 + x) * 3;
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
        UINT dst = vertexOffset + (z * 4 + y * 2 + x) * 3;
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
        UINT dst = vertexOffset + (z * 4 + y * 2 + x) * 3;
        texCoords->Ptr()[dst].x = static_cast<float>(y);
        texCoords->Ptr()[dst].y = static_cast<float>(1 - z);
        texCoords->Ptr()[dst + 1].x = static_cast<float>(x ^ y);
        texCoords->Ptr()[dst + 1].y = static_cast<float>(1 - z);
        texCoords->Ptr()[dst + 2].x = static_cast<float>(y);
        texCoords->Ptr()[dst + 2].y = static_cast<float>(x);
      }
    }
  }

  UINT indexOffset = primVertIndices->Add(36, vertIndices);
  if (vertexOffset) {
    UINT index;
    for (index = indexOffset; index < primVertIndices->Count(); ++index) {
      primVertIndices->operator[](index) += static_cast<WORD>(vertexOffset);
    }
  }
  *primType = GxPrim_Triangles;
}

HMODEL CreateModelBoundingBox(const NTempest::CAaBox &bounds, HTEXTURE texture, EGxBlend blendMode) {
  TSGrowableArray<NTempest::C3Vector> positions;
  TSGrowableArray<NTempest::C3Vector> normals;
  TSGrowableArray<NTempest::C2Vector> texCoords;
  TSGrowableArray<WORD>               primVertIndices;
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
    UINT                                 latLongLines,
    TSGrowableArray<NTempest::C3Vector> *vertices,
    TSGrowableArray<NTempest::C3Vector> *normals
) {
  UINT offset = vertices->Count();
  UINT count = latLongLines * latLongLines + 2;
  vertices->SetCount(offset + count);
  normals->SetCount(offset + count);

  vertices->operator[](offset) = bounds.c + NTempest::C3Vector(0.0f, 0.0f, bounds.r);
  normals->operator[](offset) = NTempest::C3Vector(0.0f, 0.0f, 1.0f);

  UINT dst = offset + 1;
  for (UINT lat = 0; lat < latLongLines; ++lat) {
    float latitude = static_cast<float>(lat + 1) * PI / static_cast<float>(latLongLines + 1);
    float radius = static_cast<float>(sin(latitude));
    float latZ = static_cast<float>(cos(latitude));
    for (UINT lon = 0; lon < latLongLines; ++lon) {
      normals->operator[](dst).Set(
          static_cast<float>(sin(static_cast<float>(lon) * 2.0f * PI / static_cast<float>(latLongLines))) * radius,
          static_cast<float>(cos(static_cast<float>(lon) * 2.0f * PI / static_cast<float>(latLongLines))) * radius, latZ
      );
      vertices->operator[](dst) = bounds.c + normals->operator[](dst) * bounds.r;
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
    TSGrowableArray<WORD>               *vertIndices,
    TSGrowableArray<CPrimitive>         *primitives
) {
  const UINT lines = 15;
  UINT       vertOffset = vertices->Count();
  GenerateSphereVerts(bounds, lines, vertices, normals);
  texCoords->SetCount(vertices->Count());
  vertIndices->SetCount(vertIndices->Count() + 527);

  WORD *out = vertIndices->Ptr() + vertIndices->Count() - 527;
  UINT  i;
  for (i = 0; i < lines; ++i) {
    *out++ = static_cast<WORD>(vertOffset + 1 + i);
    *out++ = static_cast<WORD>(vertOffset);
  }
  *out++ = static_cast<WORD>(vertOffset + 1);
  *out++ = static_cast<WORD>(vertOffset);
  *out++ = static_cast<WORD>(vertOffset);
  *out++ = static_cast<WORD>(vertOffset);

  for (UINT strip = 0; strip < 13; ++strip) {
    UINT first = vertOffset + 1 + strip * lines;
    UINT second = first + lines;
    if (!(strip & 1)) {
      for (i = 0; i < lines; ++i) {
        *out++ = static_cast<WORD>(first + i);
        *out++ = static_cast<WORD>(second + i);
      }
      *out++ = static_cast<WORD>(first);
      *out++ = static_cast<WORD>(second);
    } else {
      for (i = 0; i < lines; ++i) {
        *out++ = static_cast<WORD>(second + i);
        *out++ = static_cast<WORD>(first + i);
      }
      *out++ = static_cast<WORD>(second);
      *out++ = static_cast<WORD>(first);
    }
    *out++ = out[-1];
  }

  UINT penultimate = vertOffset + 1 + 13 * lines;
  UINT lastRing = penultimate + lines;
  for (i = 0; i < lines; ++i) {
    *out++ = static_cast<WORD>(penultimate + i);
    *out++ = static_cast<WORD>(lastRing + i);
  }
  *out++ = static_cast<WORD>(penultimate);
  *out++ = static_cast<WORD>(lastRing);

  UINT bottom = vertOffset + lines * lines + 1;
  for (i = 0; i < lines; ++i) {
    *out++ = static_cast<WORD>(lastRing + i);
    *out++ = static_cast<WORD>(bottom);
  }
  *out++ = static_cast<WORD>(lastRing);
  *out++ = static_cast<WORD>(bottom);

  primitives->SetCount(primitives->Count() + 1);
  CPrimitive &primitive = primitives->operator[](primitives->Count() - 1);
  primitive.type = GxPrim_TriangleStrip;
  primitive.vertexCount = 527;
}

static HMODEL CreateModelBoundingSphere(const NTempest::CAaSphere &bounds, HTEXTURE texture, EGxBlend blendMode) {
  TSGrowableArray<NTempest::C3Vector> vertices;
  TSGrowableArray<NTempest::C3Vector> normals;
  TSGrowableArray<NTempest::C2Vector> texCoords;
  TSGrowableArray<WORD>               vertIndices;
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

static void FillInRenderData(CModel *modelptr, UINT renderFlags, CModelRenderData *renderData) {
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

  UINT index;
  for (index = 0; index < s_opLayerPool.Count(); ++index) {
    s_opaqueScene.Enqueue(&s_opLayerPool[index]);
  }

  COpaqueLayer *lastSorted = s_opaqueScene[NTempest::CPriorityQ<COpaqueLayer *, COpaqueLayer>::eRootIndex];
  ASSERT(lastSorted);
  ASSERT(lastSorted->model);

  UINT rsStackOffset = GxRsStackOffset();
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

  for (UINT tmu = 0; tmu < 2; ++tmu) {
    UINT           textureId = layer->tmuPass[tmu].textureId;
    EGxRenderState textureState = static_cast<EGxRenderState>(GxRs_Texture0 + tmu);
    if (textureId == static_cast<UINT>(-1) || (layer->blendMode == GxBlend_Opaque && (renderData.m_renderFlags & 2))) {
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

  UINT rsStackOffset = GxRsStackOffset();
  UINT index;
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

UINT GetInvalidMatrixId() {
  return (s_currAnimFrame - 1) << 16;
}

UINT MatrixAlloc(UINT numMatrices) {
  UINT frame = GxPerfCounter(GxPerf_FrameNum);

  if (frame != s_lastFrame) {
    ++s_currAnimFrame;
    s_lastFrame = frame;
    s_nextMatrix = 0;
  }

  if (!numMatrices) {
    return GetInvalidMatrixId();
  }

  ASSERT(s_nextMatrix <= 0xFFFF);

  UINT nextMatrix = s_nextMatrix + numMatrices;
  if (nextMatrix > s_matrixPool.Count()) {
    s_matrixPool.SetCount(nextMatrix);
  }

  UINT result = s_nextMatrix | (static_cast<UINT>(s_currAnimFrame) << 16);
  s_nextMatrix = nextMatrix;
  return result;
}

NTempest::C34Matrix *MatrixDeref(UINT handle) {
  if (static_cast<WORD>(handle >> 16) != s_currAnimFrame) {
    return 0;
  }

  return &s_matrixPool[static_cast<WORD>(handle)];
}

void ModelRenderSceneLogStart(LPCSTR fileName) {
}

void ModelRenderSceneLogStop() {
}

int ModelRenderSceneLogToggle(LPCSTR fileName) {
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

static void IModelComplexAddToScene(CModel *model, UINT renderFlags) {
  ASSERT(model);

  CModelComplex *modelptr = static_cast<CModelComplex *>(model->data);
  ASSERT(modelptr);

  UINT count = modelptr->m_textures.Count();
  UINT index;
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
      LIST(LINKUNIQUE) &attached = modelptr->m_attached.Ptr()[index];
      LINKUNIQUE *link;
      LINKUNIQUE *next;

      for (link = attached.Head(); link; link = next) {
        next = attached.Next(link);
        ModelAddToScene(link->child, 0);
      }
    }
  }
}

static void IModelSimpleAddToScene(CModel *model, UINT renderFlags) {
  ASSERT(model);

  CModelSimple *modelptr = static_cast<CModelSimple *>(model->data);
  ASSERT(modelptr);

  UINT count = modelptr->m_textures.Count();
  UINT index;
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

void ModelAddToScene(HMODEL model, UINT renderFlags) {
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

void ModelAddToScene(const NTempest::C3Vector &position, int priorityPlane, void (*callback)(LPVOID, int), LPVOID param1, int param2) {
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

  for (UINT opaqueIndex = 0; opaqueIndex < s_opLayerPool.Count(); ++opaqueIndex) {
    if (s_opLayerPool.Ptr()[opaqueIndex].model) {
      HandleClose(s_opLayerPool.Ptr()[opaqueIndex].model);
    }
    s_opLayerPool.Ptr()[opaqueIndex].model = 0;
  }
  s_opLayerPool.SetCount(0);

  for (UINT transparentIndex = 0; transparentIndex < s_trLayerPool.Count(); ++transparentIndex) {
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

  for (UINT index = 0; index < s_opLayerPool.Count(); ++index) {
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

  for (UINT index = 0; index < s_trLayerPool.Count(); ++index) {
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

static void GeosetComplexRender(CModel *model, CGeoset *geoUnique, CGeosetShared *geoShared, UINT renderFlags, CStatus *status) {
  CModelRenderData renderData;
  FillInRenderData(model, renderFlags, &renderData);
  RenderGeosetCheckVis(&renderData, geoUnique, geoShared, status);
}

static void ModelComplexRender(HMODEL modelHandle, CModel *model, UINT renderFlags, CStatus *status) {
  CModelComplex *modelptr = static_cast<CModelComplex *>(model->data);
  CModelShared  *shared = reinterpret_cast<CModelShared *>(model->shared);

  UINT index;
  if (!(modelptr->m_flags & 0x10)) {
    for (index = 0; index < shared->numGeosets; ++index) {
      GeosetComplexRender(model, &modelptr->m_geosets[index], &shared->geosets[index], renderFlags, status);
    }

    UINT numAddlGeosets = modelptr->m_addlGeosets.Count();
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

  UINT numCustGeos = modelptr->m_custGeosets.Count();
  for (index = 0; index < numCustGeos; ++index) {
    RenderCustomGeoset(modelHandle, &modelptr->m_custGeosets[index]);
  }

  UINT numAttached = modelptr->m_attached.Count();
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

static void ModelSimpleRender(CModel *model, UINT renderFlags, CStatus *status) {
  CModelSimple *modelptr = static_cast<CModelSimple *>(model->data);
  CModelShared *shared = reinterpret_cast<CModelShared *>(model->shared);

  UINT numGeosets = modelptr->m_geosets.Count();
  if (!(modelptr->m_flags & 0x10)) {
    for (UINT index = 0; index < numGeosets; ++index) {
      CModelRenderData renderData;
      FillInRenderData(model, renderFlags, &renderData);
      RenderGeosetCheckVis(&renderData, &modelptr->m_geosets[index], &shared->geosets[index], status);
    }
  }

  ModelBaseRender(modelptr);
}

void ModelRender(HMODEL model, CStatus *status, UINT renderFlags) {
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

BOOL ModelTestSphere(HMODEL model, const NTempest::C34Matrix &orientation, float scale, int testLinkedModels) {
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
    UINT           numAttachments = complex->m_attached.Count();
    for (UINT i = 0; i < numAttachments; ++i) {
      ITERATELIST(LINKUNIQUE, complex->m_attached[i], link) {
        NTempest::C34Matrix childMatrix = orientation;
        if (ModelTestSphere(link->child, childMatrix, scale * link->scale, 1)) {
          return 1;
        }
      }
    }
  }
  return 0;
}

void ModelSceneGetFrustumPlanes(NTempest::C4Vector *fp) {
  for (UINT i = 0; i < 6; ++i) {
    fp[i] = s_frustumPlanes[i];
  }
}

void ModelSceneSetFrustumPlanes(NTempest::C4Vector *fp) {
  for (UINT i = 0; i < 6; ++i) {
    s_frustumPlanes[i] = fp[i];
  }
}

int ModelIntersectLineSegment(
    HMODEL__                 *model,
    float                     scale,
    const NTempest::C3Vector &a,
    const NTempest::C3Vector &b,
    float                     radius,
    float                    *linePos,
    int                       testLinkedModels
) {
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

static int LineSegmentIntersectBox(
    const NTempest::C34Matrix &boxToWorld,
    float                      boxScale,
    const NTempest::C3Vector  &boxMin,
    const NTempest::C3Vector  &boxMax,
    const NTempest::C3Vector  &a,
    const NTempest::C3Vector  &b,
    float                     *linePos
) {
  ASSERT(NTempest::CMath::fnotequal_(boxScale, 0.0f));

  NTempest::C34Matrix worldToBox = boxToWorld.AffineInverse(boxScale);
  NTempest::C3Vector  ax = a * worldToBox;
  NTempest::C3Vector  bx = b * worldToBox;
  NTempest::C3Vector  lineSegment = bx - ax;
  ASSERT(NTempest::CMath::fnotequal_(lineSegment.Mag(), 0.0f));

  float t0 = 0.0f;
  float t1 = 1.0f;
  float q;

  q = boxMin.x - ax.x;
  if (lineSegment.x > 0.0f) {
    if (q > lineSegment.x * t1)
      return 0;
    if (q > lineSegment.x * t0)
      t0 = q / lineSegment.x;
  } else if (lineSegment.x < 0.0f) {
    if (q > lineSegment.x * t0)
      return 0;
    if (q > lineSegment.x * t1)
      t1 = q / lineSegment.x;
  } else if (q > 0.0f) {
    return 0;
  }

  q = ax.x - boxMax.x;
  if (-lineSegment.x > 0.0f) {
    if (q > -lineSegment.x * t1)
      return 0;
    if (q > -lineSegment.x * t0)
      t0 = q / -lineSegment.x;
  } else if (-lineSegment.x < 0.0f) {
    if (q > -lineSegment.x * t0)
      return 0;
    if (q > -lineSegment.x * t1)
      t1 = q / -lineSegment.x;
  } else if (q > 0.0f) {
    return 0;
  }

  q = boxMin.y - ax.y;
  if (lineSegment.y > 0.0f) {
    if (q > lineSegment.y * t1)
      return 0;
    if (q > lineSegment.y * t0)
      t0 = q / lineSegment.y;
  } else if (lineSegment.y < 0.0f) {
    if (q > lineSegment.y * t0)
      return 0;
    if (q > lineSegment.y * t1)
      t1 = q / lineSegment.y;
  } else if (q > 0.0f) {
    return 0;
  }

  q = ax.y - boxMax.y;
  if (-lineSegment.y > 0.0f) {
    if (q > -lineSegment.y * t1)
      return 0;
    if (q > -lineSegment.y * t0)
      t0 = q / -lineSegment.y;
  } else if (-lineSegment.y < 0.0f) {
    if (q > -lineSegment.y * t0)
      return 0;
    if (q > -lineSegment.y * t1)
      t1 = q / -lineSegment.y;
  } else if (q > 0.0f) {
    return 0;
  }

  q = boxMin.z - ax.z;
  if (lineSegment.z > 0.0f) {
    if (q > lineSegment.z * t1)
      return 0;
    if (q > lineSegment.z * t0)
      t0 = q / lineSegment.z;
  } else if (lineSegment.z < 0.0f) {
    if (q > lineSegment.z * t0)
      return 0;
    if (q > lineSegment.z * t1)
      t1 = q / lineSegment.z;
  } else if (q > 0.0f) {
    return 0;
  }

  q = ax.z - boxMax.z;
  if (-lineSegment.z > 0.0f) {
    if (q > -lineSegment.z * t1)
      return 0;
    if (q > -lineSegment.z * t0)
      t0 = q / -lineSegment.z;
  } else if (-lineSegment.z < 0.0f) {
    if (q > -lineSegment.z * t0)
      return 0;
    if (q > -lineSegment.z * t1)
      t1 = q / -lineSegment.z;
  } else if (q > 0.0f) {
    return 0;
  }

  if (linePos) {
    *linePos = t0;
  }
  return 1;
}

static BOOL LineSegmentIntersectCylinder(
    const NTempest::C34Matrix &cylToWorld,
    float                      cylScale,
    const NTempest::C3Vector  &cylBottom,
    float                      cylHeight,
    float                      cylRadius,
    const NTempest::C3Vector  &a,
    const NTempest::C3Vector  &b,
    float                     *linePos
) {
  ASSERT(NTempest::CMath::fnotequal_(cylScale, 0.0f));

  NTempest::C34Matrix worldToCyl = cylToWorld.AffineInverse(cylScale);
  NTempest::C3Vector  ax = a * worldToCyl;
  NTempest::C3Vector  bx = b * worldToCyl;
  float               cylTop = cylBottom.z + cylHeight;
  if ((ax.z < cylBottom.z && bx.z < cylBottom.z) || (ax.z > cylTop && bx.z > cylTop)) {
    return 0;
  }

  NTempest::C2Vector closest(cylBottom.x - ax.x, cylBottom.y - ax.y);
  float              position = closest.x * (bx.x - ax.x) + closest.y * (bx.y - ax.y);
  float              divisor = (bx.x - ax.x) * (bx.x - ax.x) + (bx.y - ax.y) * (bx.y - ax.y);
  if (position < 0.0f) {
    position = 0.0f;
  } else if (position <= divisor) {
    position /= divisor;
    closest.x -= (bx.x - ax.x) * position;
    closest.y -= (bx.y - ax.y) * position;
  } else {
    position = 1.0f;
    closest.x -= bx.x - ax.x;
    closest.y -= bx.y - ax.y;
  }

  *linePos = position;
  return closest.SquaredMag() <= cylRadius * cylRadius;
}

static BOOL LineSegmentIntersectSphere(
    const NTempest::C34Matrix &sphToWorld,
    float                      sphScale,
    const NTempest::C3Vector  &sphCenter,
    float                      sphRadius,
    const NTempest::C3Vector  &a,
    const NTempest::C3Vector  &b,
    float                     *linePos
) {
  NTempest::C3Vector center = sphCenter * sphToWorld;
  float              radius = sphScale * sphRadius;
  NTempest::C3Vector closest = center - a;
  float              divisor = NTempest::C3Vector::Dot(b - a, b - a);
  float              position = NTempest::C3Vector::Dot(closest, b - a);
  if (position < 0.0f) {
    position = 0.0f;
  } else if (position <= divisor) {
    position /= divisor;
    closest.x -= (b.x - a.x) * position;
    closest.y -= (b.y - a.y) * position;
    closest.z -= (b.z - a.z) * position;
  } else {
    position = 1.0f;
    closest.x -= b.x - a.x;
    closest.y -= b.y - a.y;
    closest.z -= b.z - a.z;
  }

  *linePos = position;
  return closest.SquaredMag() <= radius * radius;
}

static BOOL IModelTestCollisionVolumes(
    CModelComplex            *modelptr,
    CModelShared             *shared,
    float                     scale,
    const NTempest::C3Vector &a,
    const NTempest::C3Vector &b,
    float                    *linePos
) {
  UINT  count = shared->hitTest.Count();
  UINT  pivotOffset = shared->positions.Count() - count;
  int   hitVolume = 0;
  float hitVolumeLinePos = 1.0f;
  for (UINT i = 0; i < count; ++i) {
    const CHitTest            &hit = shared->hitTest[i];
    const NTempest::C3Vector  &translation = shared->positions[pivotOffset + i];
    const NTempest::C34Matrix &hitToWorld = modelptr->m_hitTestMtx[i];
    float                      currlinePos = 1.0f;
    int                        result = 0;
    switch (hit.type) {
      case COLLIDE_BOX: {
        NTempest::C3Vector boxMin = translation + hit.extent[0];
        NTempest::C3Vector boxMax = translation + hit.extent[1];
        result = LineSegmentIntersectBox(hitToWorld, scale, boxMin, boxMax, a, b, &currlinePos);
        break;
      }

      case COLLIDE_CYLINDER: {
        NTempest::C3Vector cylBottom = translation + hit.extent[0];
        result = LineSegmentIntersectCylinder(hitToWorld, scale, cylBottom, hit.extent[1].z - hit.extent[0].z, hit.radius, a, b, &currlinePos);
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

BOOL ModelTestSphere(
    HMODEL                    model,
    const NTempest::C3Vector &position,
    float                     rotationAngle,
    const NTempest::C3Vector &rotationAxis,
    float                     scale,
    int                       testLinkedModels
) {
  CModelBase   *modelptr;
  CModelShared *shared;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &modelptr, &shared)) {
    return 0;
  }

  NTempest::CAaSphere bounds;
  IModelGetBoundingSphere(modelptr, shared, &bounds);
  TransformBounds(position, rotationAngle, rotationAxis, scale, &bounds);
  if (GxuTestSphereAndFrustumPlanes(bounds.c, bounds.r, s_frustumPlanes)) {
    return 1;
  }

  if (testLinkedModels && (modelptr->m_flags & 0x20)) {
    CModelComplex *complex = static_cast<CModelComplex *>(modelptr);

    UINT numAttachments = complex->m_attached.Count();
    for (UINT i = 0; i < numAttachments; ++i) {
      ASSERT(complex->m_anim);
      UINT               attachmentObjId = AnimGetAttachmentObjId(complex->m_anim, i);
      NTempest::C3Vector linkPosition = shared->positions[attachmentObjId];

      ITERATELIST(LINKUNIQUE, complex->m_attached[i], link) {
        NTempest::C3Vector childPosition(position.x + linkPosition.x, position.y + linkPosition.y, position.z + linkPosition.z);
        if (ModelTestSphere(link->child, childPosition, rotationAngle, rotationAxis, scale, 1)) {
          return 1;
        }
      }
    }
  }
  return 0;
}

static void CreatePlanarQuadGeometry(
    const NTempest::C3Vector            &base,
    float                                length,
    float                                width,
    TSGrowableArray<NTempest::C3Vector> *positions,
    TSGrowableArray<NTempest::C3Vector> *normals,
    TSGrowableArray<NTempest::C2Vector> *texCoords,
    TSGrowableArray<WORD>               *vertIndices,
    TSGrowableArray<CPrimitive>         *primitives
) {
  UINT indexOffset = vertIndices->Count();

  texCoords->SetCount(texCoords->Count() + 4);
  vertIndices->SetCount(indexOffset + 6);
  (*vertIndices)[indexOffset + 0] = static_cast<WORD>(positions->Count());
  (*vertIndices)[indexOffset + 1] = static_cast<WORD>(positions->Count() + 2);
  (*vertIndices)[indexOffset + 2] = static_cast<WORD>(positions->Count() + 1);
  (*vertIndices)[indexOffset + 3] = static_cast<WORD>(positions->Count() + 1);
  (*vertIndices)[indexOffset + 4] = static_cast<WORD>(positions->Count() + 2);
  (*vertIndices)[indexOffset + 5] = static_cast<WORD>(positions->Count() + 3);

  positions->SetCount(positions->Count() + 4);
  float halfLength = length * 0.5f;
  float halfWidth = width * 0.5f;
  (*positions)[positions->Count() - 4].Set(base.x - halfLength, base.y - halfWidth, base.z);
  (*positions)[positions->Count() - 3].Set(base.x - halfLength, base.y + halfWidth, base.z);
  (*positions)[positions->Count() - 2].Set(base.x + halfLength, base.y - halfWidth, base.z);
  (*positions)[positions->Count() - 1].Set(base.x + halfLength, base.y + halfWidth, base.z);

  UINT normalOffset = normals->Count();
  normals->SetCount(normalOffset + 4);
  for (UINT i = normalOffset; i < normals->Count(); ++i) {
    (*normals)[i].Set(0.0f, 0.0f, 1.0f);
  }

  CPrimitive *primitive = primitives->New();
  primitive->type = GxPrim_Triangles;
  primitive->vertexCount = 6;
}

static void GenerateCylinderVerts(
    const NTempest::C3Vector            &base,
    const NTempest::C3Vector            &height,
    const float                          radius,
    const UINT                           segments,
    TSGrowableArray<NTempest::C3Vector> *vertices,
    TSGrowableArray<NTempest::C3Vector> *normals
) {
  UINT offset = vertices->Count();
  UINT count = offset + 4 * segments + 2;
  vertices->SetCount(count);
  normals->SetCount(count);

  NTempest::C3Vector topNormal = height - base;
  topNormal.Normalize();
  NTempest::C3Vector bottomNormal = -topNormal;
  NTempest::C3Vector perp(topNormal.y - topNormal.z, topNormal.z - topNormal.x, topNormal.x - topNormal.y);
  perp.Normalize();
  perp *= radius;

  UINT vertex = offset;
  (*vertices)[vertex] = height;
  (*normals)[vertex++] = topNormal;

  UINT                   i;
  NTempest::C4Quaternion quat;
  for (i = 0; i < segments; ++i) {
    quat = NTempest::C4Quaternion(static_cast<float>(i) * TWO_PI / static_cast<float>(segments), topNormal);
    NTempest::C3Vector rot = quat * perp;
    (*vertices)[vertex] = rot + height;
    (*normals)[vertex++] = topNormal;
  }

  (*vertices)[vertex] = base;
  (*normals)[vertex++] = bottomNormal;

  for (i = 0; i < segments; ++i) {
    quat = NTempest::C4Quaternion(static_cast<float>(i) * TWO_PI / static_cast<float>(segments), topNormal);
    NTempest::C3Vector rot = quat * perp;
    (*vertices)[vertex] = rot + base;
    (*normals)[vertex++] = bottomNormal;
  }

  for (i = 0; i < segments; ++i) {
    quat = NTempest::C4Quaternion(static_cast<float>(i) * TWO_PI / static_cast<float>(segments), topNormal);
    NTempest::C3Vector rot = quat * perp;
    NTempest::C3Vector norm = rot;
    norm.Normalize();
    (*vertices)[vertex] = rot + height;
    (*normals)[vertex++] = norm;
  }

  for (i = 0; i < segments; ++i) {
    quat = NTempest::C4Quaternion(static_cast<float>(i) * TWO_PI / static_cast<float>(segments), topNormal);
    NTempest::C3Vector rot = quat * perp;
    NTempest::C3Vector norm = rot;
    norm.Normalize();
    (*vertices)[vertex] = rot + base;
    (*normals)[vertex++] = norm;
  }
}

static int CreateCylinderGeometry(
    const NTempest::C3Vector            &base,
    const NTempest::C3Vector            &height,
    const float                          radius,
    TSGrowableArray<NTempest::C3Vector> *vertices,
    TSGrowableArray<NTempest::C3Vector> *normals,
    TSGrowableArray<NTempest::C2Vector> *texCoords,
    TSGrowableArray<WORD>               *vertIndices,
    TSGrowableArray<CPrimitive>         *primitives
) {
  const UINT segments = 16;
  UINT       vertexOffset = vertices->Count();
  UINT       indexOffset = vertIndices->Count();

  GenerateCylinderVerts(base, height, radius, segments, vertices, normals);
  texCoords->SetCount(vertices->Count());
  vertIndices->SetCount(indexOffset + 192);

  WORD *indices = vertIndices->Ptr() + indexOffset;
  UINT  segment;
  for (segment = 0; segment < segments; ++segment) {
    UINT next = (segment + 1) & 0xF;
    *indices++ = static_cast<WORD>(vertexOffset + 1 + next);
    *indices++ = static_cast<WORD>(vertexOffset + 1 + segment);
    *indices++ = static_cast<WORD>(vertexOffset);
  }
  for (segment = 0; segment < segments; ++segment) {
    UINT next = (segment + 1) & 0xF;
    *indices++ = static_cast<WORD>(vertexOffset + 17);
    *indices++ = static_cast<WORD>(vertexOffset + 18 + segment);
    *indices++ = static_cast<WORD>(vertexOffset + 18 + next);
  }
  for (segment = 0; segment < segments; ++segment) {
    UINT next = (segment + 1) & 0xF;
    *indices++ = static_cast<WORD>(vertexOffset + 50 + next);
    *indices++ = static_cast<WORD>(vertexOffset + 50 + segment);
    *indices++ = static_cast<WORD>(vertexOffset + 34 + segment);
    *indices++ = static_cast<WORD>(vertexOffset + 34 + next);
    *indices++ = static_cast<WORD>(vertexOffset + 50 + next);
    *indices++ = static_cast<WORD>(vertexOffset + 34 + segment);
  }

  CPrimitive *primitive = primitives->New();
  primitive->type = GxPrim_Triangles;
  primitive->vertexCount = 192;
  return 1;
}

static void SetVertexMatrixIndices(CGeosetShared *geoShared, const TSGrowableArray<UINT> &groupVertexCounts) {
  UINT numGroups = groupVertexCounts.Count();
  if (numGroups < 2) {
    return;
  }

  geoShared->boneWeights.SetCount(geoShared->position.Count());
  BYTE *primBone = geoShared->boneWeights.Ptr();

  for (UINT i = 0; i < numGroups; ++i) {
    UINT vertCount = groupVertexCounts[i];
    if (vertCount) {
      memset(primBone, static_cast<BYTE>(i), vertCount);
      primBone += vertCount;
    }
  }

  geoShared->vertexShader = GxVS_Skin;
}

static void BuildComplexGeoset(
    const TSGrowableArray<NTempest::C3Vector> &position,
    const TSGrowableArray<NTempest::C3Vector> &normal,
    const TSGrowableArray<NTempest::C2Vector> &texCoord,
    const TSGrowableArray<WORD>               &primitiveVertices,
    const TSGrowableArray<UINT>               &groupVertex,
    const TSGrowableArray<UINT>               &groupCounts,
    const TSGrowableArray<UINT>               &matrices,
    const TSGrowableArray<CPrimitive>         &primitives,
    UINT                                       materialId,
    UINT                                       geosetId,
    CGeosetShared                             *geoShared
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
    CModelComplex                             *modelptr,
    CModelShared                              *shared,
    const TSGrowableArray<NTempest::C3Vector> &position,
    const TSGrowableArray<NTempest::C3Vector> &normal,
    const TSGrowableArray<NTempest::C2Vector> &texCoord,
    const TSGrowableArray<WORD>               &primitiveVertices,
    const TSGrowableArray<UINT>               &groupVertex,
    const TSGrowableArray<UINT>               &groupCounts,
    const TSGrowableArray<UINT>               &matrices,
    const TSGrowableArray<CPrimitive>         &primitives,
    HTEXTURE                                   texture,
    EGxBlend                                   blendMode,
    UINT                                       disables,
    NTempest::CImVector                        color
) {
  ASSERT(modelptr);
  ASSERT(shared);

  UINT materialId = modelptr->m_materials.Count();
  UINT textureId = modelptr->m_textures.Count();
  UINT geosetId = modelptr->m_geosets.Count();

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
      position, normal, texCoord, primitiveVertices, groupVertex, groupCounts, matrices, primitives, materialId, geosetId,
      &modelptr->m_addlGeosets[geosetId - shared->numGeosets]
  );
}

static void IModelHandleGeosetAdd(
    CModel                                    *model,
    const TSGrowableArray<NTempest::C3Vector> &position,
    const TSGrowableArray<NTempest::C3Vector> &normal,
    const TSGrowableArray<NTempest::C2Vector> &texCoord,
    const TSGrowableArray<WORD>               &primitiveVertices,
    const TSGrowableArray<UINT>               &groupVertex,
    const TSGrowableArray<UINT>               &groupCounts,
    const TSGrowableArray<UINT>               &matrices,
    const TSGrowableArray<CPrimitive>         &primitives,
    HTEXTURE                                   texture,
    EGxBlend                                   blendMode,
    UINT                                       disables,
    NTempest::CImVector                        color
) {
  if (model->data && (model->data->m_flags & 0x20)) {
    IModelGeosetAdd(
        static_cast<CModelComplex *>(model->data), reinterpret_cast<CModelShared *>(model->shared), position, normal, texCoord, primitiveVertices,
        groupVertex, groupCounts, matrices, primitives, texture, blendMode, disables, color
    );
  }
}

static void AddHitTestGeometryGeoset(HMODEL__ *modelHandle, HTEXTURE__ *tex) {
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
  TSGrowableArray<WORD>               primVertIndices;
  TSGrowableArray<UINT>               groupVertex;
  TSGrowableArray<UINT>               groupCounts;
  TSGrowableArray<UINT>               matrices;
  TSGrowableArray<CPrimitive>         primitives;

  UINT count = shared->hitTest.Count();
  UINT boneOffset = shared->numBones - count;
  UINT pivotOffset = shared->positions.Count() - count;
  UINT i;
  for (i = 0; i < count; ++i) {
    const CHitTest           &hit = shared->hitTest[i];
    const NTempest::C3Vector &pivot = shared->positions[pivotOffset + i];
    UINT                      oldVerts = positions.Count();
    UINT                      oldIndices = primVertIndices.Count();

    switch (hit.type) {
      case COLLIDE_BOX: {
        CPrimitive      *primitive = primitives.New();
        NTempest::CAaBox bounds;
        bounds.b = pivot + hit.extent[0];
        bounds.t = pivot + hit.extent[1];
        CreateBoxGeometry(bounds, &positions, &normals, &texCoords, &primVertIndices, &primitive->type);
        primitive->vertexCount = primVertIndices.Count() - oldIndices;
        break;
      }

      case COLLIDE_CYLINDER: {
        CreateCylinderGeometry(hit.extent[0], hit.extent[1], hit.radius, &positions, &normals, &texCoords, &primVertIndices, &primitives);
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
        CreatePlanarQuadGeometry(pivot, hit.extent[0].x, hit.extent[0].y, &positions, &normals, &texCoords, &primVertIndices, &primitives);
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
      reinterpret_cast<CModel *>(modelHandle), positions, normals, texCoords, primVertIndices, groupVertex, groupCounts, matrices, primitives, tex,
      GxBlend_Alpha, 0, NTempest::CImVector(0xFFFFFFFF)
  );
}

BOOL ModelHitTestSphere(HMODEL model, float scale, const NTempest::C3Vector &a, const NTempest::C3Vector &b, int testLinkedModels, float *linePos) {
  CModelBase   *modelptr;
  CModelShared *shared;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &modelptr, &shared)) {
    return 0;
  }

  NTempest::CAaSphere bounds;
  IModelGetBoundingSphere(modelptr, shared, &bounds);
  TransformBounds(modelptr->m_modelToWorld, scale, &bounds);

  float lineLength = (b - a).Mag();
  *linePos = 0.0f;
  NTempest::C3Vector closest = bounds.c - a;
  NTempest::C3Vector lineSegment = b - a;
  float              position = NTempest::C3Vector::Dot(closest, lineSegment);
  if (position < 0.0f) {
    position = 0.0f;
  } else {
    float divisor = NTempest::C3Vector::Dot(lineSegment, lineSegment);
    if (position > divisor) {
      position = 1.0f;
    } else {
      position /= divisor;
    }
  }
  closest -= position * lineSegment;
  *linePos = position;
  if (NTempest::C3Vector::Dot(closest, closest) <= bounds.r * bounds.r) {
    *linePos = position * lineLength;
    return 1;
  }

  if (testLinkedModels && (modelptr->m_flags & 0x20)) {
    CModelComplex *complex = static_cast<CModelComplex *>(modelptr);
    UINT           numAttachments = complex->m_attached.Count();
    for (UINT i = 0; i < numAttachments; ++i) {
      ITERATELIST(LINKUNIQUE, complex->m_attached[i], link) {
        if (ModelHitTestSphere(link->child, scale, a, b, 1, linePos)) {
          return 1;
        }
      }
    }
  }
  return 0;
}

BOOL ModelHasHitTestVolumes(HMODEL model) {
  CModelShared *shared;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &shared)) {
    return 0;
  }
  FATALASSERT(shared);
  return shared->hitTest.Count() != 0;
}

BOOL ModelHitTestVolumes(HMODEL model, float scale, const NTempest::C3Vector &a, const NTempest::C3Vector &b, int testLinkedModels, float *linePos) {
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
    for (UINT i = 0; i < complex->m_attached.Count(); ++i) {
      ITERATELIST(LINKUNIQUE, complex->m_attached[i], link) {
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

BOOL ModelHitTestGeometry(HMODEL model, float scale, const NTempest::C3Vector &a, const NTempest::C3Vector &b, int testLinkedModels, float *linePos) {
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
    for (UINT i = 0; i < complex->m_attached.Count(); ++i) {
      ITERATELIST(LINKUNIQUE, complex->m_attached[i], link) {
        if (ModelHitTestGeometry(link->child, scale, a, b, 1, linePos)) {
          return 1;
        }
      }
    }
  }
  return 0;
}

ModelIntersectResult ModelIntersectLineSegmentEx(
    HMODEL__                 *model,
    float                     scale,
    const NTempest::C3Vector &a,
    const NTempest::C3Vector &b,
    UINT                      hitTestFlags,
    float                    *linePos,
    float                    *centerDistSq,
    int                       testLinkedModels
) {
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
  float                lineLength = (b - a).Mag();
  float                hitLinePos = INFINITY;
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

void ModelShowHitTestGeometry(HMODEL__ *model) {
  CModelBase   *unique;
  CModelShared *shared;
  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique, &shared) && !(unique->m_flags & 8) && shared->hitTest.Count()) {
    HTEXTURE texture = TextureCreateSolid(NTempest::CImVector(0x7FFF0000), 0);
    AddHitTestGeometryGeoset(model, texture);
    unique->m_flags |= 8;
    HandleClose(texture);
  }
}

void ModelHideHitTestGeometry(HMODEL__ *model) {
  CModelBase *unique;
  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) && (unique->m_flags & 0x28) == 0x28) {
    CModelComplex *complex = static_cast<CModelComplex *>(unique);
    complex->m_geosets.SetCount(complex->m_geosets.Count() - 1);
    complex->m_addlGeosets.SetCount(complex->m_addlGeosets.Count() - 1);
    unique->m_flags &= ~8u;
  }
}

BOOL ModelGetExtents(HMODEL model, NTempest::CAaBox *extents) {
  CModelShared *shared;

  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &shared)) {
    return 0;
  }

  *extents = shared->bounds.extent;
  return 1;
}

BOOL ModelGetSeqExtents(HMODEL model, NTempest::CAaBox *extents) {
  CModelBase   *unique;
  CModelShared *shared;

  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique, &shared)) {
    return 0;
  }

  IModelGetExtents(unique, shared, extents);
  return 1;
}

BOOL ModelGetSeqExtents(HMODEL model, UINT seqnum, NTempest::CAaBox *extents) {
  CModelBase   *unique;
  CModelShared *shared;

  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique, &shared)) {
    return 0;
  }

  return IModelGetExtents(unique, shared, seqnum, extents);
}

BOOL ModelGetBounds(HMODEL model, NTempest::CAaSphere *bounds) {
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
