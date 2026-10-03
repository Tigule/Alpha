#include "Model/IModel.h"

#include "Anim/AnimTypes.h"
#include "Anim/WorldMatrix.h"
#include "Base/Activity.h"
#include "Base/Base.h"
#include "Gx/Gx.h"
#include "Model/ModelInternal.h"
#include "Services/ParticleSystem2.h"
#include "Services/RibbonEmitter.h"

#include "Tempest/c3vector.h"
#include "Tempest/c34matrix.h"
#include "Tempest/cmath.h"

static TSGrowableArray<NTempest::C34Matrix> s_transforms;
static UINT                                 s_transInUse;
static TSGrowableArray<UINT>                s_layers;
static UINT                                 s_layersInUse;
static TSGrowableArray<BYTE>                s_layerAlpha;

static void ApplyWorldTransforms(
    const NTempest::C3Vector &position,
    float                     rotationAngle,
    const NTempest::C3Vector &rotationAxis,
    const NTempest::C3Vector &nonUniformScale,
    NTempest::C34Matrix      *orientation
);
static void ModelAnimateAttached(
    HMODEL                    model,
    float                     scale,
    UINT                      transform,
    int                       normalizeNorms,
    const NTempest::C3Vector &cameraWorldPos,
    const NTempest::C3Vector &cameraVector
);

void ModelAnimateLogStop();

static void BuildPrimBone(NTempest::C34Matrix *boneMatrices, UINT *matrix, UINT mtxCount, NTempest::C34Matrix *bone);
static void BuildPrimBones(CGeosetShared *geoset, NTempest::C34Matrix *boneMatrices, NTempest::C34Matrix *weightedMatrices);
static void SetGeosetMatrix(CGeoset *geoUnique, CGeosetColor *geosetColors, CGeosetShared *geoShared, NTempest::C34Matrix *boneMatrices);
static void SetUnanimatedGeosetMatrix(CGeoset *geoUnique, CGeosetColor *geosetColors, CGeosetShared *geoShared);

static void IModelAnimate(CModelBase *unique, CModelShared *shared, const NTempest::C3Vector &cameraWorldPos, const NTempest::C3Vector &cameraVector);
static void IModelProcessEvents(CModelBase *unique, CModelShared *shared);

static UINT GetTransformListIndex(UINT numAttached) {
  if (!numAttached) {
    return 0;
  }

  UINT newInUse = s_transInUse + numAttached;
  if (s_transforms.Count() < newInUse) {
    s_transforms.SetCount(newInUse);
  }

  UINT index = s_transInUse;
  s_transInUse = newInUse;
  return index;
}

static void ReleaseTransformListIndex(UINT numAttached) {
  ASSERT(s_transInUse >= numAttached);
  s_transInUse -= numAttached;
}

static NTempest::C34Matrix *GetTransformPtr(UINT index) {
  if (!s_transforms.Count()) {
    return 0;
  }

  return &s_transforms[index];
}

static UINT GetLayerIndex(UINT numLayers) {
  if (!numLayers) {
    return 0;
  }

  UINT newInUse = s_layersInUse + numLayers;
  if (s_layers.Count() < newInUse) {
    s_layers.SetCount(newInUse);
  }

  UINT index = s_layersInUse;
  s_layersInUse = newInUse;
  return index;
}

static void ReleaseLayerIndex(UINT numLayers) {
  ASSERT(s_layersInUse >= numLayers);
  s_layersInUse -= numLayers;
}

static UINT *GetLayerPtr(UINT index) {
  if (!s_layers.Count()) {
    return 0;
  }

  return &s_layers[index];
}

static void IGetLayerIDs(UINT *array, UINT numLayers) {
  if (numLayers) {
    ASSERT(array);

    do {
      array[--numLayers] = static_cast<UINT>(-1);
    } while (numLayers);
  }
}

static void BuildPrimBone(NTempest::C34Matrix *boneMatrices, UINT *matrix, UINT mtxCount, NTempest::C34Matrix *bone) {
  UINT i;

  ASSERT(mtxCount > 0);

  switch (mtxCount) {
    default: {
      *bone = boneMatrices[*matrix++];
      for (i = mtxCount - 1; i; --i) {
        *bone += boneMatrices[*matrix];
        ++matrix;
      }

      *bone /= static_cast<float>(mtxCount);
      break;
    }
    case 3: {
      float *matrix0 = reinterpret_cast<float *>(boneMatrices + matrix[0]);
      float *matrix1 = reinterpret_cast<float *>(boneMatrices + matrix[1]);
      float *matrix2 = reinterpret_cast<float *>(boneMatrices + matrix[2]);
      float *target = reinterpret_cast<float *>(bone);

      for (i = 0; i < NTempest::C34Matrix::eComponents; ++i) {
        target[i] = (matrix0[i] + matrix1[i] + matrix2[i]) * 0.33333331f;
      }
      break;
    }
    case 2: {
      float *matrix0 = reinterpret_cast<float *>(boneMatrices + matrix[0]);
      float *matrix1 = reinterpret_cast<float *>(boneMatrices + matrix[1]);
      float *target = reinterpret_cast<float *>(bone);

      for (i = 0; i < NTempest::C34Matrix::eComponents; ++i) {
        target[i] = (matrix0[i] + matrix1[i]) * 0.5f;
      }
      break;
    }
    case 1:
      *bone = boneMatrices[*matrix];
      break;
  }
}

static void BuildPrimBones(CGeosetShared *geoset, NTempest::C34Matrix *boneMatrices, NTempest::C34Matrix *weightedMatrices) {
  UINT numGroups = geoset->groupMatrixCounts.Count();
  if (!numGroups) {
    *weightedMatrices = *boneMatrices;
    return;
  }

  UINT *mtxList = geoset->matrices.Ptr();
  UINT *mtxCount = geoset->groupMatrixCounts.Ptr();

  for (; numGroups; --numGroups) {
    BuildPrimBone(boneMatrices, mtxList, *mtxCount, weightedMatrices);
    mtxList += *mtxCount;
    ++mtxCount;
    ++weightedMatrices;
  }
}

static void SetGeosetMatrix(CGeoset *geoUnique, CGeosetColor *geosetColors, CGeosetShared *geoShared, NTempest::C34Matrix *boneMatrices) {
  if (!(geoUnique->flags & 1) && geosetColors[geoShared->geosetId].animatedColor.a) {
    UINT numGroups = geoShared->groupMatrixCounts.Count();
    ASSERT(numGroups > 0);

    geoUnique->weightedBones = MatrixAlloc(numGroups);
    BuildPrimBones(geoShared, boneMatrices, MatrixDeref(geoUnique->weightedBones));
  }
}

static void SetUnanimatedGeosetMatrix(CGeoset *geoUnique, CGeosetColor *geosetColors, CGeosetShared *geoShared) {
  if (!(geoUnique->flags & 1) && geosetColors[geoShared->geosetId].animatedColor.a) {
    ASSERT(geoShared->groupMatrixCounts.Count() <= 1);

    geoUnique->weightedBones = MatrixAlloc(1);
    WorldMatrixGet(MatrixDeref(geoUnique->weightedBones));
  }
}

static void SetCollisionMatrices(CModelComplex *unique, CModelShared *shared, NTempest::C34Matrix *boneMatrices) {
  NTempest::C34Matrix *source = boneMatrices + shared->numBones - shared->hitTest.Count();
  NTempest::C34Matrix *target = unique->m_hitTestMtx.Ptr();
  UINT                 count = unique->m_hitTestMtx.Count();

  while (count) {
    *target = *source;
    ++target;
    ++source;
    --count;
  }
}

static void SetGeosetMatrices(CModelSimple *unique, CModelShared *shared, NTempest::C34Matrix *boneMatrices) {
  ASSERT(unique);

  UINT numGeosets = unique->m_geosets.Count();
  for (UINT i = 0; i < numGeosets; ++i) {
    SetGeosetMatrix(&unique->m_geosets[i], unique->m_geosetColor.Ptr(), &shared->geosets[i], boneMatrices);
  }
}

static void SetGeosetMatrices(CModelComplex *unique, CModelShared *shared, NTempest::C34Matrix *boneMatrices) {
  ASSERT(unique);

  UINT i;
  for (i = 0; i < shared->numGeosets; ++i) {
    SetGeosetMatrix(&unique->m_geosets[i], unique->m_geosetColor.Ptr(), &shared->geosets[i], boneMatrices);
  }

  UINT numAddlGeosets = unique->m_addlGeosets.Count();
  for (i = 0; i < numAddlGeosets; ++i) {
    SetGeosetMatrix(&unique->m_geosets[i + shared->numGeosets], unique->m_geosetColor.Ptr(), &unique->m_addlGeosets[i], boneMatrices);
  }
}

static void SetUnanimatedCollisionMatrices(CModelComplex *unique) {
  NTempest::C34Matrix *matrix = unique->m_hitTestMtx.Ptr();
  UINT                 count = unique->m_hitTestMtx.Count();

  while (count) {
    WorldMatrixGet(matrix);
    ++matrix;
    --count;
  }
}

static void SetUnanimatedGeosetMatrices(CModelSimple *unique, CModelShared *shared) {
  ASSERT(unique);
  ASSERT(shared);

  UINT numGeosets = unique->m_geosets.Count();
  for (UINT i = 0; i < numGeosets; ++i) {
    SetUnanimatedGeosetMatrix(&unique->m_geosets[i], unique->m_geosetColor.Ptr(), &shared->geosets[i]);
  }
}

static void SetUnanimatedGeosetMatrices(CModelComplex *unique, CModelShared *shared) {
  ASSERT(unique);
  ASSERT(shared);

  UINT i;
  for (i = 0; i < shared->numGeosets; ++i) {
    SetUnanimatedGeosetMatrix(&unique->m_geosets[i], unique->m_geosetColor.Ptr(), &shared->geosets[i]);
  }

  UINT numAddlGeosets = unique->m_addlGeosets.Count();
  for (i = 0; i < numAddlGeosets; ++i) {
    SetUnanimatedGeosetMatrix(&unique->m_geosets[i + shared->numGeosets], unique->m_geosetColor.Ptr(), &unique->m_addlGeosets[i]);
  }
}

static void GetLayerAlpha(HMATERIAL *materials, UINT numMaterials, BYTE *layerAlpha) {
  UINT i;

  for (i = 0; i < numMaterials; ++i) {
    CMaterial *uniqueMtl = reinterpret_cast<CMaterial *>(materials[i]);
    ASSERT(uniqueMtl);

    UINT numLayers = uniqueMtl->layers.Count();
    for (UINT layer = 0; layer < numLayers; ++layer) {
      *layerAlpha++ = uniqueMtl->layers[layer].layerAlpha;
    }
  }
}

static void SetLayerAlpha(HMATERIAL *materials, UINT numMaterials, BYTE *layerAlpha) {
  UINT i;

  for (i = 0; i < numMaterials; ++i) {
    CMaterial *uniqueMtl = reinterpret_cast<CMaterial *>(materials[i]);
    ASSERT(uniqueMtl);

    UINT numLayers = uniqueMtl->layers.Count();
    for (UINT layer = 0; layer < numLayers; ++layer) {
      uniqueMtl->layers[layer].layerAlpha = *layerAlpha++;
    }
  }
}

static void UpdateEmitters(CModelComplex *unique, CModelShared *shared, const NTempest::C3Vector &cameraWorldPos) {
  float elapsed = AnimGetElapsedTime() * 0.001f;

  UINT i;
  UINT numEmitters = unique->m_emitters2.Count();
  for (i = 0; i < numEmitters; ++i) {
    NTempest::C34Matrix currentWorldMatrix = unique->m_modelToWorld;
    currentWorldMatrix.Translate(shared->positions[shared->emitter2Order[i]]);
    currentWorldMatrix.Rotate(PI * 0.5f, NTempest::C3Vector(0.0f, 0.0f, 1.0f), true);
    unique->m_emitters2[i]->SetEnabled(1, 1);
    unique->m_emitters2[i]->Update(elapsed, currentWorldMatrix, cameraWorldPos);
  }

  UINT numRibbons = unique->m_ribbons.Count();
  for (i = 0; i < numRibbons; ++i) {
    unique->m_ribbons[i]->SetEnabled(1);
    unique->m_ribbons[i]->Update(elapsed, 0);
  }
}

static void IModelAnimateBounds(HMODEL model) {
  CModelBase   *unique;
  CModelShared *shared;
  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique, &shared)) {
    IModelAnimate(unique, shared, NTempest::C3Vector(), NTempest::C3Vector());
  }
}

static void
IModelAnimate(CModelSimple *unique, CModelShared *shared, const NTempest::C3Vector &cameraWorldPos, const NTempest::C3Vector &cameraVector) {
  if (!unique->m_anim) {
    SetUnanimatedGeosetMatrices(unique, shared);
    return;
  }

  s_layerAlpha.SetCount(shared->numLayers);
  GetLayerAlpha(unique->m_materials.Ptr(), unique->m_materials.Count(), s_layerAlpha.Ptr());

  UINT bones = GetTransformListIndex(shared->numBones + shared->hitTest.Count());
  UINT numLayers = 0;
  UINT numMaterials = unique->m_materials.Count();
  for (UINT i = 0; i < numMaterials; ++i) {
    numLayers += reinterpret_cast<CMaterial *>(unique->m_materials[i])->layers.Count();
  }
  UINT layerIndex = GetLayerIndex(numLayers);
  IGetLayerIDs(GetLayerPtr(layerIndex), numLayers);

  unique->m_texBones = MatrixAlloc(shared->numTexBones);
  CAnimationData animData;
  animData.boneMtx = GetTransformPtr(bones);
  animData.numBones = shared->numBones;
  animData.textureMtx = MatrixDeref(unique->m_texBones);
  animData.numTexBones = shared->numTexBones;
  animData.positions = &shared->positions;
  animData.lights = 0;
  animData.emitters2 = 0;
  animData.ribbons = 0;
  animData.attached = 0;
  animData.numAttached = 0;
  animData.geosetColor = unique->m_geosetColor.Ptr();
  animData.cameraWorldPos = &cameraWorldPos;
  animData.cameraVector = &cameraVector;
  animData.layerAlpha = s_layerAlpha.Ptr();
  animData.layerTextureIds = GetLayerPtr(layerIndex);
  AnimAnimateModel(unique->m_anim, animData);

  UINT *layers = GetLayerPtr(layerIndex);
  if (numLayers) {
    ASSERT(layers);

    for (UINT materialIndex = 0; materialIndex < numMaterials; ++materialIndex) {
      CMaterial *material = reinterpret_cast<CMaterial *>(unique->m_materials[materialIndex]);
      for (UINT materialLayer = 0; materialLayer < material->layers.Count(); ++materialLayer) {
        if (layers[materialLayer] != static_cast<UINT>(-1)) {
          if (HandleObjectCompare(
                  unique->m_textures[material->layers[materialLayer].tmuPass[0].textureId].handle, unique->m_textures[layers[materialLayer]].handle
              ))
          {
            material->layers[materialLayer].tmuPass[0].textureId = layers[materialLayer];
          }
        }
      }
    }
  }

  SetGeosetMatrices(unique, shared, GetTransformPtr(bones));
  ReleaseLayerIndex(numLayers);
  ReleaseTransformListIndex(shared->numBones + shared->hitTest.Count());
  SetLayerAlpha(unique->m_materials.Ptr(), unique->m_materials.Count(), s_layerAlpha.Ptr());
}

static void ModelAnimateAttached(
    HMODEL                    model,
    float                     scale,
    UINT                      transform,
    int                       normalizeNorms,
    const NTempest::C3Vector &cameraWorldPos,
    const NTempest::C3Vector &cameraVector
) {
  CModelBase   *unique;
  CModelShared *shared;
  ASSERT(model);
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique, &shared)) {
    return;
  }
  WorldMatrixPush();
  WorldMatrixLoad(*GetTransformPtr(transform));
  WorldMatrixScale(scale);
  WorldMatrixGet(&unique->m_modelToWorld);
  if (normalizeNorms)
    unique->m_flags |= 2;
  else
    unique->m_flags &= ~2;
  IModelAnimate(unique, shared, cameraWorldPos, cameraVector);
  if (unique->m_boundsModel)
    IModelAnimateBounds(unique->m_boundsModel);
  if (unique->m_collideModel)
    IModelAnimateBounds(unique->m_collideModel);
  WorldMatrixPop();
}

static void
IModelAnimate(CModelComplex *unique, CModelShared *shared, const NTempest::C3Vector &cameraWorldPos, const NTempest::C3Vector &cameraVector) {
  const UINT numAttachments = unique->m_attached.Count();
  UINT       transforms = GetTransformListIndex(numAttachments);

  if (unique->m_anim) {
    s_layerAlpha.SetCount(shared->numLayers);
    GetLayerAlpha(unique->m_materials.Ptr(), unique->m_materials.Count(), s_layerAlpha.Ptr());

    UINT bones = GetTransformListIndex(shared->numBones + shared->hitTest.Count());
    UINT numLayers = 0;
    UINT numMaterials = unique->m_materials.Count();
    for (UINT i = 0; i < numMaterials; ++i) {
      numLayers += reinterpret_cast<CMaterial *>(unique->m_materials[i])->layers.Count();
    }
    UINT layerIndex = GetLayerIndex(numLayers);
    IGetLayerIDs(GetLayerPtr(layerIndex), numLayers);

    unique->m_texBones = MatrixAlloc(shared->numTexBones);
    CAnimationData animData;
    animData.boneMtx = GetTransformPtr(bones);
    animData.numBones = shared->numBones;
    animData.textureMtx = MatrixDeref(unique->m_texBones);
    animData.numTexBones = shared->numTexBones;
    animData.positions = &shared->positions;
    animData.lights = &unique->m_lights;
    animData.emitters2 = &unique->m_emitters2;
    animData.ribbons = &unique->m_ribbons;
    animData.attached = GetTransformPtr(transforms);
    animData.numAttached = numAttachments;
    animData.geosetColor = unique->m_geosetColor.Ptr();
    animData.cameraWorldPos = &cameraWorldPos;
    animData.cameraVector = &cameraVector;
    animData.layerAlpha = s_layerAlpha.Ptr();
    animData.layerTextureIds = GetLayerPtr(layerIndex);
    AnimAnimateModel(unique->m_anim, animData);

    memset(unique->m_attachmentFlags.Ptr(), 0, unique->m_attachmentFlags.Count());

    UINT *layers = GetLayerPtr(layerIndex);
    if (numLayers) {
      ASSERT(layers);

      for (UINT materialIndex = 0; materialIndex < numMaterials; ++materialIndex) {
        CMaterial *material = reinterpret_cast<CMaterial *>(unique->m_materials[materialIndex]);
        for (UINT materialLayer = 0; materialLayer < material->layers.Count(); ++materialLayer) {
          if (layers[materialLayer] != static_cast<UINT>(-1)) {
            if (HandleObjectCompare(
                    unique->m_textures[material->layers[materialLayer].tmuPass[0].textureId].handle, unique->m_textures[layers[materialLayer]].handle
                ))
            {
              material->layers[materialLayer].tmuPass[0].textureId = layers[materialLayer];
            }
          }
        }
      }
    }

    SetGeosetMatrices(unique, shared, GetTransformPtr(bones));
    SetCollisionMatrices(unique, shared, GetTransformPtr(bones));
    ReleaseLayerIndex(numLayers);
    ReleaseTransformListIndex(shared->numBones + shared->hitTest.Count());
    SetLayerAlpha(unique->m_materials.Ptr(), unique->m_materials.Count(), s_layerAlpha.Ptr());
  } else {
    SetUnanimatedGeosetMatrices(unique, shared);
    SetUnanimatedCollisionMatrices(unique);
    UpdateEmitters(unique, shared, cameraWorldPos);
  }

  int normalizeNorms = unique->m_flags & 2;
  LISTPTR(LINKUNIQUE) attached = unique->m_attached.Ptr();
  for (UINT i = 0; i < numAttachments; ++i, ++attached) {
    int enabled = 1;
    if (unique->m_anim) {
      if (unique->m_attachmentFlags[i] & 1) {
        enabled = unique->m_attachmentFlags[i] & 2;
      } else {
        enabled = AnimIsAttachmentEnabled(unique->m_anim, i);
        unique->m_attachmentFlags[i] = (enabled ? 2 : 0) | 1;
      }
    }
    if (enabled) {
      ITERATELISTPTR(LINKUNIQUE, attached, link) {
        ModelAnimateAttached(link->child, link->scale, transforms + i, normalizeNorms, cameraWorldPos, cameraVector);
      }
    }
  }
  ReleaseTransformListIndex(numAttachments);
}

static void
IModelAnimate(CModelBase *unique, CModelShared *shared, const NTempest::C3Vector &cameraWorldPos, const NTempest::C3Vector &cameraVector) {
  ASSERT(unique);
  ASSERT(shared);
  if (unique->m_flags & 0x20) {
    IModelAnimate(static_cast<CModelComplex *>(unique), shared, cameraWorldPos, cameraVector);
  } else {
    IModelAnimate(static_cast<CModelSimple *>(unique), shared, cameraWorldPos, cameraVector);
  }
}

static void IModelProcessEvents(CModelBase *unique, CModelShared *shared) {
  FATALASSERT(unique);
  FATALASSERT(shared);

  if (unique->m_anim) {
    AnimProcessEvents(unique->m_anim, shared->positions);
  }

  if (unique->m_flags & 0x20) {
    CModelComplex *complex = static_cast<CModelComplex *>(unique);
    UINT           numAttached = complex->m_attached.Count();
    for (UINT index = 0; index < numAttached; ++index) {
      int enabled = 1;
      if (complex->m_anim) {
        if (complex->m_attachmentFlags[index] & 1) {
          enabled = complex->m_attachmentFlags[index] & 2;
        } else {
          enabled = AnimIsAttachmentEnabled(complex->m_anim, index);
          complex->m_attachmentFlags[index] = (enabled ? 2 : 0) | 1;
        }
      }

      if (enabled) {
        WorldMatrixPush();
        ITERATELIST(LINKUNIQUE, complex->m_attached[index], link) {
          CModelBase   *childModel;
          CModelShared *childShared;
          if (IModelDerefHandle(reinterpret_cast<CModel *>(link->child), &childModel, &childShared)) {
            WorldMatrixLoad(childModel->m_modelToWorld);
            IModelProcessEvents(childModel, childShared);
          }
        }
        WorldMatrixPop();
      }
    }
  }
}

static void ApplyWorldTransforms(
    const NTempest::C3Vector &position,
    float                     rotationAngle,
    const NTempest::C3Vector &rotationAxis,
    float                     scale,
    NTempest::C34Matrix      *orientation
) {
  orientation->Translate(position);

  if (!(NTempest::CMath::fabs_(rotationAngle) < 0.00000023841858f)) {
    orientation->Rotate(rotationAngle, rotationAxis, true);
  }

  orientation->Scale(scale);
}

static void ApplyWorldTransforms(
    const NTempest::C3Vector &position,
    float                     rotationAngle,
    const NTempest::C3Vector &rotationAxis,
    const NTempest::C3Vector &nonUniformScale,
    NTempest::C34Matrix      *orientation
) {
  orientation->Translate(position);

  if (!(NTempest::CMath::fabs_(rotationAngle) < 0.00000023841858f)) {
    orientation->Rotate(rotationAngle, rotationAxis, true);
  }

  orientation->Scale(nonUniformScale);
}

static void IModelGetStandingBasis(
    GROUND_TRACK              trackType,
    const NTempest::C3Vector &groundNormal,
    float                     facing,
    NTempest::C3Vector       *xprime,
    NTempest::C3Vector       *yprime,
    NTempest::C3Vector       *zprime
) {
  NTempest::C2Vector xaxis;
  NTempest::CMath::sincos_(facing, xaxis.y, xaxis.x);

  if (trackType == TRACK_PITCH_YAW) {
    yprime->Set(-xaxis.y, xaxis.x, 0.0f);
    *xprime = NTempest::C3Vector::Cross(*yprime, groundNormal);
    xprime->Normalize();
    *zprime = NTempest::C3Vector::Cross(*xprime, *yprime);
  } else if (trackType == TRACK_PITCH_YAW_ROLL) {
    *zprime = groundNormal;
    yprime->Set(-xaxis.y * zprime->z, xaxis.x * zprime->z, xaxis.y * zprime->x - xaxis.x * zprime->y);
    yprime->Normalize();
    *xprime = NTempest::C3Vector::Cross(*yprime, *zprime);
  } else {
    xprime->Set(xaxis.x, xaxis.y, 0.0f);
    yprime->Set(-xaxis.y, xaxis.x, 0.0f);
    zprime->Set(0.0f, 0.0f, 1.0f);
  }
}

static void IModelGetStandingMatrix(
    GROUND_TRACK              trackType,
    const NTempest::C3Vector &position,
    const NTempest::C3Vector &groundNormal,
    float                     facing,
    float                     scale,
    NTempest::C34Matrix      *orientation
) {
  IModelGetStandingBasis(
      trackType, groundNormal, facing, reinterpret_cast<NTempest::C3Vector *>(&orientation->a0),
      reinterpret_cast<NTempest::C3Vector *>(&orientation->b0), reinterpret_cast<NTempest::C3Vector *>(&orientation->c0)
  );
  *reinterpret_cast<NTempest::C3Vector *>(&orientation->d0) = position;
  orientation->Scale(scale);
}

void ModelAnimateInitialize() {
  s_transInUse = 0;
}

void ModelAnimateDestroy() {
  ModelAnimateLogStop();
}

void ModelAnimateLogStart(LPCSTR fileName) {
}

void ModelAnimateLogStop() {
}

int ModelAnimateLogToggle(LPCSTR fileName) {
  return 0;
}

void ModelAnimateCameras(HMODEL model, const NTempest::C34Matrix &orientation) {
  CModelBase *unique;

  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) && unique->m_anim && (unique->m_flags & 0x20)) {
    CModelComplex *complex = static_cast<CModelComplex *>(unique);
    ActivityBegin(ACTIVITY_ANIMATE);
    WorldMatrixPush();
    WorldMatrixLoad(orientation);
    AnimAnimateCameras(complex->m_anim, complex->m_cameras);
    WorldMatrixPop();
    ActivityEnd(ACTIVITY_ANIMATE);
  }
}

void ModelAnimateCameras(HMODEL model, const NTempest::C3Vector &position, float rotationAngle, const NTempest::C3Vector &rotationAxis, float scale) {
  NTempest::C34Matrix orientation;

  ApplyWorldTransforms(position, rotationAngle, rotationAxis, scale, &orientation);
  ModelAnimateCameras(model, orientation);
}

void ModelProcessEvents(HMODEL model, const NTempest::C34Matrix &orientation) {
  CModelBase   *unique;
  CModelShared *shared;

  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique, &shared)) {
    ActivityBegin(ACTIVITY_ANIMATE);
    WorldMatrixPush();
    WorldMatrixLoad(orientation);
    IModelProcessEvents(unique, shared);
    WorldMatrixPop();
    ActivityEnd(ACTIVITY_ANIMATE);
  }
}

void ModelProcessEvents(HMODEL model, const NTempest::C3Vector &position, float rotationAngle, const NTempest::C3Vector &rotationAxis, float scale) {
  CModelBase   *unique;
  CModelShared *shared;

  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique, &shared)) {
    ActivityBegin(ACTIVITY_ANIMATE);
    WorldMatrixPush();
    WorldMatrixLoadIdentity();
    WorldMatrixTranslate(position);
    WorldMatrixRotate(rotationAngle, rotationAxis);
    WorldMatrixScale(scale);
    IModelProcessEvents(unique, shared);
    WorldMatrixPop();
    ActivityEnd(ACTIVITY_ANIMATE);
  }
}

void ModelAnimate(
    HMODEL                     model,
    const NTempest::C34Matrix &orientation,
    float                      scale,
    const NTempest::C3Vector  &cameraWorldPos,
    const NTempest::C3Vector  &cameraVector
) {
  CModelBase   *unique;
  CModelShared *shared;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique, &shared)) {
    return;
  }
  ActivityBegin(ACTIVITY_ANIMATE);
  if (NTempest::CMath::fnotequal4_(scale, 1.0f))
    unique->m_flags |= 2;
  else
    unique->m_flags &= ~2;
  unique->m_modelToWorld = orientation;
  WorldMatrixPush();
  WorldMatrixLoad(orientation);
  IModelAnimate(unique, shared, cameraWorldPos, cameraVector);
  if (unique->m_boundsModel)
    IModelAnimateBounds(unique->m_boundsModel);
  if (unique->m_collideModel)
    IModelAnimateBounds(unique->m_collideModel);
  WorldMatrixPop();
  ActivityEnd(ACTIVITY_ANIMATE);
}

void ModelAnimate(
    HMODEL                    model,
    const NTempest::C3Vector &position,
    float                     rotationAngle,
    const NTempest::C3Vector &rotationAxis,
    float                     scale,
    const NTempest::C3Vector &cameraWorldPos,
    const NTempest::C3Vector &cameraVector
) {
  NTempest::C34Matrix orientation;

  ApplyWorldTransforms(position - cameraWorldPos, rotationAngle, rotationAxis, scale, &orientation);
  ModelAnimate(model, orientation, scale, cameraWorldPos, cameraVector);
}

void ModelAnimate(
    HMODEL                    model,
    const NTempest::C3Vector &position,
    float                     rotationAngle,
    const NTempest::C3Vector &rotationAxis,
    const NTempest::C3Vector &nonUniformScale,
    const NTempest::C3Vector &cameraWorldPos,
    const NTempest::C3Vector &cameraVector
) {
  NTempest::C34Matrix orientation;

  ApplyWorldTransforms(position - cameraWorldPos, rotationAngle, rotationAxis, nonUniformScale, &orientation);
  float fakeScale = NTempest::CMath::fabs_(nonUniformScale.x - 1.0f) < 0.00000095367432f &&
                            NTempest::CMath::fabs_(nonUniformScale.y - 1.0f) < 0.00000095367432f &&
                            NTempest::CMath::fabs_(nonUniformScale.z - 1.0f) < 0.00000095367432f
                        ? 1.0f
                        : 1.5f;
  ModelAnimate(model, orientation, fakeScale, cameraWorldPos, cameraVector);
}

BOOL ModelSetSequence(HMODEL model, UINT seqIndex, UINT flags) {
  CModel *modelptr = reinterpret_cast<CModel *>(model);
  VALIDATEBEGIN;
  VALIDATE(modelptr);
  VALIDATEEND;

  CModelBase *unique;
  if (!IModelDerefHandle(modelptr, &unique)) {
    EnqueueModelCommand(modelptr, MODEL_SET_SEQUENCE1, seqIndex, flags);
    return 1;
  }

  if (!unique->m_anim) {
    SErrSetLastError(ERROR_PATH_NOT_FOUND);
    return 0;
  }

  if (!AnimSetSequence(unique->m_anim, seqIndex, flags)) {
    return 0;
  }

  if (unique->m_boundsModel) {
    if (unique->m_flags & 1) {
      ModelShowBoundingSphere(model);
    } else {
      ModelShowBoundingBox(model);
    }
  }

  return 1;
}

BOOL ModelSetSequence(HMODEL model, UINT seqIndex, UINT objectId, UINT flags) {
  CModel *modelptr = reinterpret_cast<CModel *>(model);
  VALIDATEBEGIN;
  VALIDATE(modelptr);
  VALIDATEEND;

  CModelBase *unique;
  if (!IModelDerefHandle(modelptr, &unique)) {
    EnqueueModelCommand(modelptr, MODEL_SET_SEQUENCE2, seqIndex, objectId, flags);
    return 1;
  }

  if (unique->m_anim) {
    return AnimSetSequence(unique->m_anim, seqIndex, objectId, flags);
  }

  SErrSetLastError(ERROR_PATH_NOT_FOUND);
  return 0;
}

int ModelMatchSequence(HMODEL model, UINT objectId, UINT sameAsObjectId, UINT flags) {
  CModel *modelptr = reinterpret_cast<CModel *>(model);
  VALIDATEBEGIN;
  VALIDATE(modelptr);
  VALIDATEEND;

  CModelBase *unique;
  if (!IModelDerefHandle(modelptr, &unique)) {
    EnqueueModelCommand(modelptr, MODEL_MATCH_SEQUENCE, objectId, sameAsObjectId, flags);
    return 1;
  }

  if (unique->m_anim) {
    return AnimMatchSequence(unique->m_anim, objectId, sameAsObjectId, flags);
  }

  SErrSetLastError(ERROR_PATH_NOT_FOUND);
  return 0;
}

int ModelSetRandomSequenceFidget(HMODEL model, UINT seqIndex, UINT flags) {
  CModel *modelptr = reinterpret_cast<CModel *>(model);
  VALIDATEBEGIN;
  VALIDATE(modelptr);
  VALIDATEEND;

  CModelBase *unique;
  if (!IModelDerefHandle(modelptr, &unique)) {
    EnqueueModelCommand(modelptr, MODEL_SET_RANDOM_SEQUENCE_FIDGET1, seqIndex, flags);
    return 1;
  }

  if (!unique->m_anim) {
    SErrSetLastError(ERROR_PATH_NOT_FOUND);
    return 0;
  }

  int result = AnimSetRandomSequenceFidget(unique->m_anim, seqIndex, flags);
  if (unique->m_boundsModel) {
    if (unique->m_flags & 1) {
      ModelShowBoundingSphere(model);
    } else {
      ModelShowBoundingBox(model);
    }
  }
  return result;
}

int ModelSetRandomSequenceFidget(HMODEL model, UINT seqIndex, UINT objectId, UINT flags) {
  CModel *modelptr = reinterpret_cast<CModel *>(model);
  VALIDATEBEGIN;
  VALIDATE(modelptr);
  VALIDATEEND;

  CModelBase *unique;
  if (!IModelDerefHandle(modelptr, &unique)) {
    EnqueueModelCommand(modelptr, MODEL_SET_RANDOM_SEQUENCE_FIDGET2, seqIndex, objectId, flags);
    return 1;
  }

  if (unique->m_anim) {
    return AnimSetRandomSequenceFidget(unique->m_anim, seqIndex, objectId, flags);
  }

  SErrSetLastError(ERROR_PATH_NOT_FOUND);
  return 0;
}

UINT ModelGetNumSequenceFidgets(HMODEL model, UINT seqIndex) {
  CModelBase *unique;

  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) && unique->m_anim) {
    return AnimGetNumSequenceFidgets(unique->m_anim, seqIndex);
  }
  return 0;
}

int ModelSetSequenceFidget(HMODEL model, UINT seqIndex, UINT fidgetId, UINT flags) {
  CModel *modelptr = reinterpret_cast<CModel *>(model);
  VALIDATEBEGIN;
  VALIDATE(modelptr);
  VALIDATEEND;

  CModelBase *unique;
  if (!IModelDerefHandle(modelptr, &unique)) {
    EnqueueModelCommand(modelptr, MODEL_SET_SEQUENCE_FIDGET1, seqIndex, fidgetId, flags);
    return 1;
  }

  if (!unique->m_anim) {
    SErrSetLastError(ERROR_PATH_NOT_FOUND);
    return 0;
  }

  int result = AnimSetSequenceFidget(unique->m_anim, seqIndex, fidgetId, flags);
  if (unique->m_boundsModel) {
    if (unique->m_flags & 1) {
      ModelShowBoundingSphere(model);
    } else {
      ModelShowBoundingBox(model);
    }
  }
  return result;
}

int ModelSetSequenceFidget(HMODEL model, UINT seqIndex, UINT fidgetId, UINT objectId, UINT flags) {
  CModel *modelptr = reinterpret_cast<CModel *>(model);
  VALIDATEBEGIN;
  VALIDATE(modelptr);
  VALIDATEEND;

  CModelBase *unique;
  if (!IModelDerefHandle(modelptr, &unique)) {
    EnqueueModelCommand(modelptr, MODEL_SET_SEQUENCE_FIDGET1, seqIndex, fidgetId, objectId, flags);
    return 1;
  }

  if (unique->m_anim) {
    return AnimSetSequenceFidget(unique->m_anim, seqIndex, fidgetId, objectId, flags);
  }

  SErrSetLastError(ERROR_PATH_NOT_FOUND);
  return 0;
}

UINT ModelGetNumSequences(HMODEL model) {
  CModelBase *unique;

  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) && unique->m_anim) {
    return AnimGetNumSequences(unique->m_anim);
  }
  return 0;
}

BOOL ModelGetSequenceDuration(HMODEL model, UINT seqIndex, UINT *duration) {
  VALIDATEBEGIN;
  VALIDATE(duration);
  VALIDATEEND;
  *duration = 0;

  CModelBase *unique;
  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) && unique->m_anim) {
    return AnimGetSequenceDuration(unique->m_anim, seqIndex, duration);
  }

  return 0;
}

BOOL ModelGetSequenceMoveSpeed(HMODEL model, UINT seqIndex, float *moveSpeed) {
  VALIDATEBEGIN;
  VALIDATE(moveSpeed);
  VALIDATEEND;
  *moveSpeed = 0.0f;

  CModelBase *unique;
  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) && unique->m_anim) {
    return AnimGetSequenceMoveSpeed(unique->m_anim, seqIndex, moveSpeed);
  }

  return 0;
}

BOOL ModelGetSequenceName(HMODEL model, UINT seqIndex, char *buffer, UINT buffLength) {
  VALIDATEBEGIN;
  VALIDATEANDBLANK(buffer);
  VALIDATE(buffLength);
  VALIDATEEND;

  CModelBase *unique;
  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) && unique->m_anim) {
    return AnimGetSequenceName(unique->m_anim, seqIndex, buffer, buffLength);
  }

  return 0;
}

BOOL ModelHasSequenceId(HMODEL model, UINT seqIndex) {
  CModelBase *unique;

  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) && unique->m_anim) {
    return AnimHasSequenceId(unique->m_anim, seqIndex);
  }

  return 0;
}

UINT ModelGetTotalKeys(HMODEL model) {
  CModelBase *unique;

  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) && unique->m_anim) {
    return AnimGetTotalKeys(unique->m_anim);
  }
  return 0;
}

void ModelSetSeqFinishedHandler(HMODEL model, ANIMSEQFINISHEDHANDLER callback, LPVOID param) {
  CModelBase *unique;

  CModel *modelptr = reinterpret_cast<CModel *>(model);
  VALIDATEBEGIN;
  VALIDATE(modelptr);
  VALIDATEENDVOID;

  if (!IModelDerefHandle(modelptr, &unique)) {
    EnqueueModelCommand(modelptr, MODEL_SET_SEQ_FINISHED_HANDLER1, callback, param);
    return;
  }

  if (unique->m_anim) {
    AnimSetSeqFinishedHandler(unique->m_anim, callback, param);
  }
}

void ModelSetSeqFinishedHandler(HMODEL model, UINT sequence, ANIMSEQFINISHEDHANDLER callback, LPVOID param) {
  CModelBase *unique;

  CModel *modelptr = reinterpret_cast<CModel *>(model);
  VALIDATEBEGIN;
  VALIDATE(modelptr);
  VALIDATEENDVOID;

  if (!IModelDerefHandle(modelptr, &unique)) {
    EnqueueModelCommand(modelptr, MODEL_SET_SEQ_FINISHED_HANDLER2, sequence, callback, param);
    return;
  }

  if (unique->m_anim) {
    AnimSetSeqFinishedHandler(unique->m_anim, sequence, callback, param);
  }
}

void ModelSetTimeScale(HMODEL model, float timeScale, int doLinkedModels) {
  CModel *modelptr = reinterpret_cast<CModel *>(model);
  VALIDATEBEGIN;
  VALIDATE(modelptr);
  VALIDATEENDVOID;

  CModelBase *unique;
  if (!IModelDerefHandle(modelptr, &unique)) {
    EnqueueModelCommand(modelptr, MODEL_SET_TIME_SCALE, timeScale, doLinkedModels);
    return;
  }

  if (unique->m_anim) {
    AnimSetTimeScale(unique->m_anim, timeScale);
  }

  if (!doLinkedModels || !(unique->m_flags & 0x20)) {
    return;
  }

  CModelComplex *complex = static_cast<CModelComplex *>(unique);
  UINT           numAttachments = complex->m_attached.Count();
  for (UINT index = 0; index < numAttachments; ++index) {
    int enabled = 1;
    if (unique->m_anim) {
      if (complex->m_attachmentFlags[index] & 1) {
        enabled = complex->m_attachmentFlags[index] & 2;
      } else {
        enabled = AnimIsAttachmentEnabled(unique->m_anim, index);
        complex->m_attachmentFlags[index] = (enabled ? 2 : 0) | 1;
      }
    }

    if (enabled) {
      LIST(LINKUNIQUE) &links = complex->m_attached[index];
      ITERATELIST(LINKUNIQUE, links, link) {
        ModelSetTimeScale(link->child, timeScale, 1);
      }
    }
  }
}

BOOL ModelSetObjectTimeScale(HMODEL model, UINT objectId, float timeScale, int doLinkedModels) {
  CModel *modelptr = reinterpret_cast<CModel *>(model);
  VALIDATEBEGIN;
  VALIDATE(modelptr);
  VALIDATEEND;

  CModelBase *unique;
  if (!IModelDerefHandle(modelptr, &unique)) {
    EnqueueModelCommand(modelptr, MODEL_SET_OBJECT_TIME_SCALE, objectId, timeScale, doLinkedModels);
    return 1;
  }

  if (unique->m_anim && !AnimSetObjectTimeScale(unique->m_anim, objectId, timeScale)) {
    return 0;
  }

  if (!doLinkedModels || !(unique->m_flags & 0x20)) {
    return 1;
  }

  CModelComplex *complex = static_cast<CModelComplex *>(unique);
  UINT           numAttachments = complex->m_attached.Count();
  for (UINT index = 0; index < numAttachments; ++index) {
    int enabled = 1;
    if (unique->m_anim) {
      if (complex->m_attachmentFlags[index] & 1) {
        enabled = complex->m_attachmentFlags[index] & 2;
      } else {
        enabled = AnimIsAttachmentEnabled(unique->m_anim, index);
        complex->m_attachmentFlags[index] = (enabled ? 2 : 0) | 1;
      }
    }

    if (enabled) {
      LIST(LINKUNIQUE) &links = complex->m_attached[index];
      ITERATELIST(LINKUNIQUE, links, link) {
        if (!ModelSetObjectTimeScale(link->child, objectId, timeScale, 1)) {
          return 0;
        }
      }
    }
  }

  return 1;
}

float ModelGetTimeScale(HMODEL model) {
  CModelBase *unique;

  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) && unique->m_anim) {
    return AnimGetTimeScale(unique->m_anim);
  }
  return 1.0f;
}

float ModelGetObjectTimeScale(HMODEL model, UINT objectId) {
  CModelBase *unique;

  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) && unique->m_anim) {
    return AnimGetObjectTimeScale(unique->m_anim, objectId);
  }
  return 1.0f;
}

BOOL ModelForceCurrentSequenceTime(HMODEL model, int timeOffset, int doLinkedModels) {
  CModel *modelptr = reinterpret_cast<CModel *>(model);
  VALIDATEBEGIN;
  VALIDATE(modelptr);
  VALIDATEEND;

  CModelBase *unique;
  if (!IModelDerefHandle(modelptr, &unique)) {
    EnqueueModelCommand(modelptr, MODEL_FORCE_CURRENT_SEQUENCE_TIME, timeOffset, doLinkedModels);
    return 1;
  }

  if (!unique->m_anim || !AnimForceCurrentSequenceTime(unique->m_anim, timeOffset)) {
    return 0;
  }

  if (doLinkedModels && (unique->m_flags & 0x20)) {
    CModelComplex *complex = static_cast<CModelComplex *>(unique);
    LISTPTR(LINKUNIQUE) attached = complex->m_attached.Ptr();
    UINT index = 0;

    while (index < complex->m_attached.Count()) {
      int enabled = 1;
      if (unique->m_anim) {
        if (complex->m_attachmentFlags[index] & 1) {
          enabled = complex->m_attachmentFlags[index] & 2;
        } else {
          enabled = AnimIsAttachmentEnabled(unique->m_anim, index);
          complex->m_attachmentFlags[index] = (enabled ? 2 : 0) | 1;
        }
      }

      if (enabled) {
        LINKUNIQUE *link = attached->Head();
        while (link) {
          LINKUNIQUE *next = attached->Next(link);
          ModelForceCurrentSequenceTime(link->child, timeOffset, 1);
          link = next;
        }
      }

      ++attached;
      ++index;
    }
  }

  return 1;
}

BOOL ModelForceSequenceTime(HMODEL model, UINT seqIndex, int timeOffset, int doLinkedModels) {
  CModel *modelptr = reinterpret_cast<CModel *>(model);
  VALIDATEBEGIN;
  VALIDATE(modelptr);
  VALIDATEEND;

  CModelBase *unique;
  if (!IModelDerefHandle(modelptr, &unique)) {
    EnqueueModelCommand(modelptr, MODEL_FORCE_SEQUENCE_TIME, seqIndex, timeOffset, doLinkedModels);
    return 1;
  }

  if (!unique->m_anim || !AnimForceSequenceTime(unique->m_anim, seqIndex, timeOffset)) {
    return 0;
  }

  if (doLinkedModels && (unique->m_flags & 0x20)) {
    CModelComplex *complex = static_cast<CModelComplex *>(unique);
    LISTPTR(LINKUNIQUE) attached = complex->m_attached.Ptr();
    UINT index = 0;

    while (index < complex->m_attached.Count()) {
      int enabled = 1;
      if (unique->m_anim) {
        if (complex->m_attachmentFlags[index] & 1) {
          enabled = complex->m_attachmentFlags[index] & 2;
        } else {
          enabled = AnimIsAttachmentEnabled(unique->m_anim, index);
          complex->m_attachmentFlags[index] = (enabled ? 2 : 0) | 1;
        }
      }

      if (enabled) {
        LINKUNIQUE *link = attached->Head();
        while (link) {
          LINKUNIQUE *next = attached->Next(link);
          ModelForceSequenceTime(link->child, 0, timeOffset, 1);
          link = next;
        }
      }

      ++attached;
      ++index;
    }
  }

  return 1;
}

BOOL ModelAdvanceTime(HMODEL model) {
  CModelBase *unique;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    return 0;
  }

  if (unique->m_anim && !AnimAdvanceTime(unique->m_anim, GxPerfCounter(GxPerf_FrameNum))) {
    return 0;
  }

  ASSERT(((CModel *)((CHandleObject*)(model))));
  if (unique->m_flags & 0x20) {
    CModelComplex *complex = static_cast<CModelComplex *>(unique);
    LISTPTR(LINKUNIQUE) attached = complex->m_attached.Ptr();

    for (UINT index = 0; index < complex->m_attached.Count(); ++index) {
      int enabled = 1;
      if (complex->m_anim) {
        if (complex->m_attachmentFlags[index] & 1) {
          enabled = complex->m_attachmentFlags[index] & 2;
        } else {
          enabled = AnimIsAttachmentEnabled(complex->m_anim, index);
          complex->m_attachmentFlags[index] = (enabled ? 2 : 0) | 1;
        }
      }

      if (enabled) {
        LINKUNIQUE *link = attached->Head();
        while (reinterpret_cast<int>(link) > 0) {
          LINKUNIQUE *next = attached->RawNext(link);
          ModelAdvanceTime(link->child);
          link = next;
        }
      }

      ++attached;
    }
  }

  return 1;
}

BOOL ModelAdvanceTime(HMODEL model, int timeChange) {
  CModelBase *unique;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    return 0;
  }

  if (unique->m_anim && !AnimManualAdvanceTime(unique->m_anim, timeChange)) {
    return 0;
  }

  FATALASSERT(((CModel *)((CHandleObject*)(model))));
  if (unique->m_flags & 0x20) {
    CModelComplex *complex = static_cast<CModelComplex *>(unique);
    LISTPTR(LINKUNIQUE) attached = complex->m_attached.Ptr();

    for (UINT index = 0; index < complex->m_attached.Count(); ++index) {
      int enabled = 1;
      if (unique->m_anim) {
        if (complex->m_attachmentFlags[index] & 1) {
          enabled = complex->m_attachmentFlags[index] & 2;
        } else {
          enabled = AnimIsAttachmentEnabled(unique->m_anim, index);
          complex->m_attachmentFlags[index] = (enabled ? 2 : 0) | 1;
        }
      }

      if (enabled) {
        ITERATELISTPTR(LINKUNIQUE, attached, link) {
          ModelAdvanceTime(link->child, timeChange);
        }
      }
      ++attached;
    }
  }

  return 1;
}

void ModelPauseTime(HMODEL model, int pause, int doLinkedModels) {
  CModelBase    *unique;
  CModelComplex *complex;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    return;
  }

  if (unique->m_anim) {
    AnimPauseTime(unique->m_anim, pause);
  }

  if (doLinkedModels && (unique->m_flags & 0x20)) {
    complex = static_cast<CModelComplex *>(unique);
    LISTPTR(LINKUNIQUE) attached = complex->m_attached.Ptr();

    for (UINT index = 0; index < complex->m_attached.Count(); ++index) {
      int enabled = 1;
      if (unique->m_anim) {
        if (complex->m_attachmentFlags[index] & 1) {
          enabled = complex->m_attachmentFlags[index] & 2;
        } else {
          enabled = AnimIsAttachmentEnabled(unique->m_anim, index);
          complex->m_attachmentFlags[index] = (enabled ? 2 : 0) | 1;
        }
      }
      if (enabled) {
        ITERATELISTPTR(LINKUNIQUE, attached, link) {
          ModelPauseTime(link->child, pause, 0);
        }
      }
      ++attached;
    }
  }
}

void ModelResetGlobalSequenceTimes(HMODEL model, int doLinkedModels) {
  CModelBase *unique;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    return;
  }

  if (unique->m_anim) {
    AnimResetGlobalSequenceTimes(unique->m_anim);
  }

  if (doLinkedModels && (unique->m_flags & 0x20)) {
    CModelComplex *complex = static_cast<CModelComplex *>(unique);
    UINT           numAttachments = complex->m_attached.Count();
    for (UINT index = 0; index < numAttachments; ++index) {
      int enabled = 1;
      if (complex->m_anim) {
        if (complex->m_attachmentFlags[index] & 1) {
          enabled = complex->m_attachmentFlags[index] & 2;
        } else {
          enabled = AnimIsAttachmentEnabled(complex->m_anim, index);
          complex->m_attachmentFlags[index] = (enabled ? 2 : 0) | 1;
        }
      }

      if (enabled) {
        ITERATELIST(LINKUNIQUE, complex->m_attached[index], link) {
          ModelResetGlobalSequenceTimes(link->child, 0);
        }
      }
    }
  }
}

void ModelSetEventCallback(HMODEL model, void (*callback)(LPCSTR, const NTempest::C3Vector &, LPVOID), LPVOID param, int doLinkedModels) {
  CModelBase *unique;

  CModel *modelptr = reinterpret_cast<CModel *>(model);
  VALIDATEBEGIN;
  VALIDATE(modelptr);
  VALIDATEENDVOID;

  if (!IModelDerefHandle(modelptr, &unique)) {
    EnqueueModelCommand(modelptr, MODEL_SET_EVENT_CALLBACK, callback, param, doLinkedModels);
    return;
  }

  if (unique->m_anim) {
    AnimSetEventCallback(unique->m_anim, callback, param);
  }

  if (doLinkedModels && (unique->m_flags & 0x20)) {
    CModelComplex *complex = static_cast<CModelComplex *>(unique);
    LISTPTR(LINKUNIQUE) attached = complex->m_attached.Ptr();
    for (UINT numAttachments = complex->m_attached.Count(); numAttachments; --numAttachments, ++attached) {
      ITERATELISTPTR(LINKUNIQUE, attached, link) {
        ModelSetEventCallback(link->child, callback, param, 0);
      }
    }
  }
}

int ModelApplyObjectLookAt(HMODEL model, UINT objectId, const NTempest::C3Vector &target) {
  CModel *modelptr = reinterpret_cast<CModel *>(model);
  VALIDATEBEGIN;
  VALIDATE(modelptr);
  VALIDATEEND;

  CModelBase *unique;
  if (!IModelDerefHandle(modelptr, &unique)) {
    EnqueueModelCommand(modelptr, MODEL_APPLY_OBJECT_LOOK_AT, objectId, target);
    return 1;
  }

  if (!unique->m_anim) {
    return 0;
  }

  return AnimApplyObjectLookAt(unique->m_anim, objectId, target);
}

int ModelRemoveObjectLookAt(HMODEL model, UINT objectId) {
  CModel *modelptr = reinterpret_cast<CModel *>(model);
  VALIDATEBEGIN;
  VALIDATE(modelptr);
  VALIDATEEND;

  CModelBase *unique;
  if (!IModelDerefHandle(modelptr, &unique)) {
    EnqueueModelCommand(modelptr, MODEL_REMOVE_OBJECT_LOOK_AT, objectId);
    return 1;
  }

  if (!unique->m_anim) {
    return 0;
  }

  return AnimRemoveObjectLookAt(unique->m_anim, objectId);
}

BOOL ModelObjectUsingLookAt(HMODEL model, UINT objectId) {
  CModelBase *unique;

  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) && unique->m_anim) {
    return AnimObjectUsingLookAt(unique->m_anim, objectId);
  }

  return 0;
}

int ModelApplyObjectFaceDir(HMODEL model, UINT objectId, const NTempest::C3Vector &direction) {
  CModel *modelptr = reinterpret_cast<CModel *>(model);
  VALIDATEBEGIN;
  VALIDATE(modelptr);
  VALIDATEEND;

  CModelBase *unique;
  if (!IModelDerefHandle(modelptr, &unique)) {
    EnqueueModelCommand(modelptr, MODEL_APPLY_OBJECT_FACE_DIR, objectId, direction);
    return 1;
  }

  if (!unique->m_anim) {
    return 0;
  }

  return AnimApplyObjectFaceDir(unique->m_anim, objectId, direction);
}

int ModelRemoveObjectFaceDir(HMODEL model, UINT objectId) {
  CModel *modelptr = reinterpret_cast<CModel *>(model);
  VALIDATEBEGIN;
  VALIDATE(modelptr);
  VALIDATEEND;

  CModelBase *unique;
  if (!IModelDerefHandle(modelptr, &unique)) {
    EnqueueModelCommand(modelptr, MODEL_REMOVE_OBJECT_FACE_DIR, objectId);
    return 1;
  }

  if (!unique->m_anim) {
    return 0;
  }

  return AnimRemoveObjectFaceDir(unique->m_anim, objectId);
}

BOOL ModelObjectUsingFaceDir(HMODEL model, UINT objectId) {
  CModelBase *unique;

  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) && unique->m_anim) {
    return AnimObjectUsingFaceDir(unique->m_anim, objectId);
  }

  return 0;
}

int ModelMarkFootstepSequence(HMODEL model, UINT seqIndex) {
  CModel *modelptr = reinterpret_cast<CModel *>(model);
  VALIDATEBEGIN;
  VALIDATE(modelptr);
  VALIDATEEND;

  CModelBase *unique;
  if (!IModelDerefHandle(modelptr, &unique)) {
    EnqueueModelCommand(modelptr, MODEL_MARK_FOOTSTEP_SEQUENCE, seqIndex);
    return 1;
  }

  if (!unique->m_anim) {
    return 0;
  }

  return AnimMarkFootstepSequence(unique->m_anim, seqIndex);
}

int ModelLockObjectSequence(HMODEL model, UINT objectId, int set) {
  CModel *modelptr = reinterpret_cast<CModel *>(model);
  VALIDATEBEGIN;
  VALIDATE(modelptr);
  VALIDATEEND;

  CModelBase *unique;
  if (!IModelDerefHandle(modelptr, &unique)) {
    EnqueueModelCommand(modelptr, MODEL_LOCK_OBJECT_SEQUENCE, objectId, set);
    return 1;
  }

  if (!unique->m_anim) {
    return 0;
  }

  return AnimLockObjectSequence(unique->m_anim, objectId, set);
}

void ModelGetStandingMatrix(
    HMODEL                    model,
    const NTempest::C3Vector &position,
    const NTempest::C3Vector &groundNormal,
    float                     facing,
    float                     scale,
    NTempest::C34Matrix      *orientation
) {
  CModelShared *shared;
  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &shared)) {
    IModelGetStandingMatrix(shared->groundTrack, position, groundNormal, facing, scale, orientation);
  }
}

void ModelForceStandingMatrix(
    HMODEL                    model,
    const NTempest::C3Vector &position,
    const NTempest::C3Vector &groundNormal,
    float                     facing,
    float                     scale,
    int                       enumGroundTrack,
    float                     blendRatio,
    NTempest::C34Matrix      *orientation
) {
  CModelShared *shared;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &shared)) {
    return;
  }

  if (shared->groundTrack == enumGroundTrack || NTempest::CMath::fabs_(blendRatio - 1.0f) < 0.00000095367432f) {
    IModelGetStandingMatrix(static_cast<GROUND_TRACK>(enumGroundTrack), position, groundNormal, facing, scale, orientation);
  } else if (NTempest::CMath::fabs_(blendRatio) < 0.00000095367432f) {
    IModelGetStandingMatrix(shared->groundTrack, position, groundNormal, facing, scale, orientation);
  } else {
    NTempest::C33Matrix standMtx;
    NTempest::C33Matrix deadMtx;
    IModelGetStandingBasis(shared->groundTrack, groundNormal, facing, standMtx.Row0AsVec3(), standMtx.Row1AsVec3(), standMtx.Row2AsVec3());
    IModelGetStandingBasis(
        static_cast<GROUND_TRACK>(enumGroundTrack), groundNormal, facing, deadMtx.Row0AsVec3(), deadMtx.Row1AsVec3(), deadMtx.Row2AsVec3()
    );
    deadMtx = deadMtx * blendRatio;
    standMtx = standMtx * (1.0f - blendRatio);
    standMtx += deadMtx;
    *orientation = NTempest::C34Matrix(standMtx);
    orientation->d0 = position.x;
    orientation->d1 = position.y;
    orientation->d2 = position.z;
    orientation->Scale(scale);
  }
}

float ModelGetPrimarySequenceCompletion(HMODEL model) {
  CModelBase *unique;

  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) && unique->m_anim) {
    return AnimGetPrimarySequenceCompletion(unique->m_anim);
  }
  return 0.0f;
}

BOOL ModelEventEmitterHasKeysThisSeq(HMODEL model, UINT objectId) {
  CModelBase *unique;

  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) && unique->m_anim) {
    return AnimEventEmitterHasKeysThisSeq(unique->m_anim, objectId);
  }

  return 0;
}

BOOL ModelGetModelSpacePivot(HMODEL model, UINT objectId, NTempest::C3Vector *pivot) {
  CModelBase   *unique;
  CModelShared *shared;

  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique, &shared) && unique->m_anim) {
    return AnimGetObjectPosition(unique->m_anim, objectId, shared->positions, pivot);
  }

  return 0;
}

BOOL ModelGetObjectPosition(HMODEL model, UINT objectId, NTempest::C3Vector *position) {
  CModelBase   *unique;
  CModelShared *shared;

  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique, &shared) || !unique->m_anim ||
      !AnimGetObjectPosition(unique->m_anim, objectId, shared->positions, position))
  {
    return 0;
  }

  *position *= unique->m_modelToWorld;
  return 1;
}

BOOL ModelGetEventObjectPosition(HMODEL model, UINT objectId, int modelSpace, NTempest::C3Vector *position) {
  CModelBase *unique;

  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) || !unique->m_anim ||
      !AnimGetEventObjectPosition(unique->m_anim, objectId, position))
  {
    return 0;
  }

  if (!modelSpace) {
    *position *= unique->m_modelToWorld;
  }
  return 1;
}
