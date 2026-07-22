#include "Model/IModel.h"

#include "Anim/AnimTypes.h"
#include "Anim/WorldMatrix.h"
#include "Base/Activity.h"
#include "Gx/Gx.h"
#include "Model/ModelInternal.h"
#include "Services/ParticleSystem2.h"
#include "Services/RibbonEmitter.h"

#include "Tempest/c3vector.h"
#include "Tempest/c34matrix.h"
#include "Tempest/cmath.h"

static TSGrowableArray<NTempest::C34Matrix> s_transforms;
static unsigned int                         s_transInUse;
static TSGrowableArray<unsigned int>        s_layers;
static unsigned int                         s_layersInUse;
const float                                 PI = 3.14159265358979323846f;
const float                                 TWO_PI = 6.28318530717958647692f;
const float                                 OO_TWO_PI = 0.15915494309189533577f;
static TSGrowableArray<unsigned char>       s_layerAlpha;

static void __fastcall ApplyWorldTransforms(
    const NTempest::C3Vector &position,
    float                     rotationAngle,
    const NTempest::C3Vector &rotationAxis,
    const NTempest::C3Vector &nonUniformScale,
    NTempest::C34Matrix      *orientation
);
static void __fastcall ModelAnimateAttached(
    HMODEL                    model,
    float                     scale,
    unsigned int              transform,
    int                       normalizeNorms,
    const NTempest::C3Vector &cameraWorldPos,
    const NTempest::C3Vector &cameraVector
);

void __fastcall ModelAnimateLogStop();

static void __fastcall BuildPrimBone(NTempest::C34Matrix *boneMatrices, unsigned int *matrix, unsigned int mtxCount, NTempest::C34Matrix *bone);
static void __fastcall BuildPrimBones(CGeosetShared *geoset, NTempest::C34Matrix *boneMatrices, NTempest::C34Matrix *weightedMatrices);
static void __fastcall SetGeosetMatrix(CGeoset *geoUnique, CGeosetColor *geosetColors, CGeosetShared *geoShared, NTempest::C34Matrix *boneMatrices);
static void __fastcall SetUnanimatedGeosetMatrix(CGeoset *geoUnique, CGeosetColor *geosetColors, CGeosetShared *geoShared);

static void __fastcall
IModelAnimate(CModelBase *unique, CModelShared *shared, const NTempest::C3Vector &cameraWorldPos, const NTempest::C3Vector &cameraVector);
static void __fastcall IModelProcessEvents(CModelBase *unique, CModelShared *shared);

static unsigned int __fastcall GetTransformListIndex(unsigned int numAttached) {
  if (!numAttached) {
    return 0;
  }

  unsigned int newInUse = s_transInUse + numAttached;
  if (s_transforms.Count() < newInUse) {
    s_transforms.SetCount(newInUse);
  }

  unsigned int index = s_transInUse;
  s_transInUse = newInUse;
  return index;
}

static void __fastcall ReleaseTransformListIndex(unsigned int numAttached) {
  ASSERT(s_transInUse >= numAttached);
  s_transInUse -= numAttached;
}

static NTempest::C34Matrix *__fastcall GetTransformPtr(unsigned int index) {
  if (!s_transforms.Count()) {
    return 0;
  }

  return &s_transforms[index];
}

static unsigned int __fastcall GetLayerIndex(unsigned int numLayers) {
  if (!numLayers) {
    return 0;
  }

  unsigned int newInUse = s_layersInUse + numLayers;
  if (s_layers.Count() < newInUse) {
    s_layers.SetCount(newInUse);
  }

  unsigned int index = s_layersInUse;
  s_layersInUse = newInUse;
  return index;
}

static void __fastcall ReleaseLayerIndex(unsigned int numLayers) {
  ASSERT(s_layersInUse >= numLayers);
  s_layersInUse -= numLayers;
}

static unsigned int *__fastcall GetLayerPtr(unsigned int index) {
  if (!s_layers.Count()) {
    return 0;
  }

  return &s_layers[index];
}

static void __fastcall IGetLayerIDs(unsigned int *array, unsigned int numLayers) {
  if (numLayers) {
    ASSERT(array);

    do {
      array[--numLayers] = static_cast<unsigned int>(-1);
    } while (numLayers);
  }
}

static void __fastcall IModelGetStandingBasis(
    GROUND_TRACK              trackType,
    const NTempest::C3Vector &groundNormal,
    float                     facing,
    NTempest::C3Vector       *xprime,
    NTempest::C3Vector       *yprime,
    NTempest::C3Vector       *zprime
) {
  float sinFacing;
  float cosFacing;
  NTempest::CMath::sincos_(facing, sinFacing, cosFacing);

  if (trackType == TRACK_PITCH_YAW) {
    yprime->Set(-sinFacing, cosFacing, 0.0f);
    *xprime = NTempest::C3Vector::Cross(*yprime, groundNormal);
    xprime->Normalize();
    *zprime = NTempest::C3Vector::Cross(*xprime, *yprime);
  } else if (trackType == TRACK_PITCH_YAW_ROLL) {
    *zprime = groundNormal;
    yprime->Set(-sinFacing * zprime->z, cosFacing * zprime->z, sinFacing * zprime->x - cosFacing * zprime->y);
    yprime->Normalize();
    *xprime = NTempest::C3Vector::Cross(*yprime, *zprime);
  } else {
    xprime->Set(cosFacing, sinFacing, 0.0f);
    yprime->Set(-sinFacing, cosFacing, 0.0f);
    zprime->Set(0.0f, 0.0f, 1.0f);
  }
}

static void __fastcall IModelGetStandingMatrix(
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
  orientation->d0 = position.x;
  orientation->d1 = position.y;
  orientation->d2 = position.z;
  orientation->Scale(scale);
}

static void __fastcall BuildPrimBone(NTempest::C34Matrix *boneMatrices, unsigned int *matrix, unsigned int mtxCount, NTempest::C34Matrix *bone) {
  unsigned int i;

  ASSERT(mtxCount > 0);

  if (mtxCount == 1) {
    unsigned int *source = reinterpret_cast<unsigned int *>(boneMatrices + matrix[0]);
    unsigned int *target = reinterpret_cast<unsigned int *>(bone);

    for (i = 0; i < NTempest::C34Matrix::eComponents; ++i) {
      target[i] = source[i];
    }
  } else if (mtxCount == 2) {
    float *matrix0 = reinterpret_cast<float *>(boneMatrices + matrix[0]);
    float *matrix1 = reinterpret_cast<float *>(boneMatrices + matrix[1]);
    float *target = reinterpret_cast<float *>(bone);

    for (i = 0; i < NTempest::C34Matrix::eComponents; ++i) {
      target[i] = (matrix0[i] + matrix1[i]) * 0.5f;
    }
  } else if (mtxCount == 3) {
    float *matrix0 = reinterpret_cast<float *>(boneMatrices + matrix[0]);
    float *matrix1 = reinterpret_cast<float *>(boneMatrices + matrix[1]);
    float *matrix2 = reinterpret_cast<float *>(boneMatrices + matrix[2]);
    float *target = reinterpret_cast<float *>(bone);

    for (i = 0; i < NTempest::C34Matrix::eComponents; ++i) {
      target[i] = (matrix0[i] + matrix1[i] + matrix2[i]) * 0.33333331f;
    }
  } else {
    *bone = boneMatrices[*matrix++];
    i = mtxCount - 1;

    while (i) {
      *bone += boneMatrices[*matrix++];
      --i;
    }

    *bone /= static_cast<float>(mtxCount);
  }
}

static void __fastcall BuildPrimBones(CGeosetShared *geoset, NTempest::C34Matrix *boneMatrices, NTempest::C34Matrix *weightedMatrices) {
  unsigned int numGroups = geoset->groupMatrixCounts.Count();

  if (!numGroups) {
    *weightedMatrices = *boneMatrices;
    return;
  }

  unsigned int *mtxCount = geoset->groupMatrixCounts.Ptr();
  unsigned int *mtxList = geoset->matrices.Ptr();

  do {
    BuildPrimBone(boneMatrices, mtxList, *mtxCount, weightedMatrices);
    mtxList += *mtxCount++;
    ++weightedMatrices;
  } while (--numGroups);
}

static void __fastcall SetGeosetMatrix(CGeoset *geoUnique, CGeosetColor *geosetColors, CGeosetShared *geoShared, NTempest::C34Matrix *boneMatrices) {
  if (!(geoUnique->flags & 1) && geosetColors[geoShared->geosetId].proceduralColor.a) {
    unsigned int numGroups = geoShared->groupMatrixCounts.Count();
    ASSERT(numGroups > 0);

    geoUnique->weightedBones = MatrixAlloc(numGroups);
    BuildPrimBones(geoShared, boneMatrices, MatrixDeref(geoUnique->weightedBones));
  }
}

static void __fastcall SetGeosetMatrices(CModelSimple *unique, CModelShared *shared, NTempest::C34Matrix *boneMatrices) {
  ASSERT(unique);

  unsigned int numGeosets = unique->m_geosets.Count();
  for (unsigned int i = 0; i < numGeosets; ++i) {
    SetGeosetMatrix(&unique->m_geosets[i], unique->m_geosetColor.Ptr(), &shared->geosets[i], boneMatrices);
  }
}

static void __fastcall SetGeosetMatrices(CModelComplex *unique, CModelShared *shared, NTempest::C34Matrix *boneMatrices) {
  ASSERT(unique);

  unsigned int i;
  for (i = 0; i < shared->numGeosets; ++i) {
    SetGeosetMatrix(&unique->m_geosets[i], unique->m_geosetColor.Ptr(), &shared->geosets[i], boneMatrices);
  }

  unsigned int numAddlGeosets = unique->m_addlGeosets.Count();
  for (i = 0; i < numAddlGeosets; ++i) {
    SetGeosetMatrix(&unique->m_geosets[i + shared->numGeosets], unique->m_geosetColor.Ptr(), &unique->m_addlGeosets[i], boneMatrices);
  }
}

static void __fastcall SetUnanimatedGeosetMatrix(CGeoset *geoUnique, CGeosetColor *geosetColors, CGeosetShared *geoShared) {
  if (!(geoUnique->flags & 1) && geosetColors[geoShared->geosetId].proceduralColor.a) {
    ASSERT(geoShared->groupMatrixCounts.Count() <= 1);

    geoUnique->weightedBones = MatrixAlloc(1);
    WorldMatrixGet(MatrixDeref(geoUnique->weightedBones));
  }
}

static void __fastcall SetUnanimatedGeosetMatrices(CModelSimple *unique, CModelShared *shared) {
  ASSERT(unique);
  ASSERT(shared);

  unsigned int numGeosets = unique->m_geosets.Count();
  for (unsigned int i = 0; i < numGeosets; ++i) {
    SetUnanimatedGeosetMatrix(&unique->m_geosets[i], unique->m_geosetColor.Ptr(), &shared->geosets[i]);
  }
}

static void __fastcall SetUnanimatedGeosetMatrices(CModelComplex *unique, CModelShared *shared) {
  ASSERT(unique);
  ASSERT(shared);

  unsigned int i;
  for (i = 0; i < shared->numGeosets; ++i) {
    SetUnanimatedGeosetMatrix(&unique->m_geosets[i], unique->m_geosetColor.Ptr(), &shared->geosets[i]);
  }

  unsigned int numAddlGeosets = unique->m_addlGeosets.Count();
  for (i = 0; i < numAddlGeosets; ++i) {
    SetUnanimatedGeosetMatrix(&unique->m_geosets[i + shared->numGeosets], unique->m_geosetColor.Ptr(), &unique->m_addlGeosets[i]);
  }
}

static void __fastcall SetCollisionMatrices(CModelComplex *unique, CModelShared *shared, NTempest::C34Matrix *boneMatrices) {
  NTempest::C34Matrix *source = boneMatrices + shared->numBones - shared->hitTest.Count();
  NTempest::C34Matrix *target = unique->m_hitTestMtx.Ptr();
  unsigned int         count = unique->m_hitTestMtx.Count();

  while (count) {
    *target = *source;
    ++target;
    ++source;
    --count;
  }
}

static void __fastcall SetUnanimatedCollisionMatrices(CModelComplex *unique) {
  NTempest::C34Matrix *matrix = unique->m_hitTestMtx.Ptr();
  unsigned int         count = unique->m_hitTestMtx.Count();

  while (count) {
    WorldMatrixGet(matrix);
    ++matrix;
    --count;
  }
}

static void __fastcall GetLayerAlpha(HMATERIAL *materials, unsigned int numMaterials, unsigned char *layerAlpha) {
  for (unsigned int i = 0; i < numMaterials; ++i) {
    CMaterial *uniqueMtl = reinterpret_cast<CMaterial *>(materials[i]);
    ASSERT(uniqueMtl);

    unsigned int numLayers = uniqueMtl->layers.Count();
    for (unsigned int layer = 0; layer < numLayers; ++layer) {
      *layerAlpha++ = uniqueMtl->layers[layer].layerAlpha;
    }
  }
}

static void __fastcall SetLayerAlpha(HMATERIAL *materials, unsigned int numMaterials, unsigned char *layerAlpha) {
  for (unsigned int i = 0; i < numMaterials; ++i) {
    CMaterial *uniqueMtl = reinterpret_cast<CMaterial *>(materials[i]);
    ASSERT(uniqueMtl);

    unsigned int numLayers = uniqueMtl->layers.Count();
    for (unsigned int layer = 0; layer < numLayers; ++layer) {
      uniqueMtl->layers[layer].layerAlpha = *layerAlpha++;
    }
  }
}

static void __fastcall
IModelAnimate(CModelSimple *unique, CModelShared *shared, const NTempest::C3Vector &cameraWorldPos, const NTempest::C3Vector &cameraVector) {
  if (!unique->m_anim) {
    SetUnanimatedGeosetMatrices(unique, shared);
    return;
  }

  unsigned int numMaterials = unique->m_materials.Count();
  s_layerAlpha.SetCount(shared->numLayers);
  GetLayerAlpha(unique->m_materials.Ptr(), numMaterials, s_layerAlpha.Ptr());

  unsigned int bones = GetTransformListIndex(shared->numBones);
  unsigned int numLayers = 0;
  for (unsigned int i = 0; i < numMaterials; ++i) {
    CMaterial *material = reinterpret_cast<CMaterial *>(unique->m_materials[i]);
    numLayers += material->layers.Count();
  }
  unsigned int  layerIndex = GetLayerIndex(numLayers);
  unsigned int *layers = GetLayerPtr(layerIndex);
  IGetLayerIDs(layers, numLayers);

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
  animData.layerTextureIds = layers;
  AnimAnimateModel(unique->m_anim, animData);

  for (unsigned int materialIndex = 0; materialIndex < numMaterials; ++materialIndex) {
    CMaterial *material = reinterpret_cast<CMaterial *>(unique->m_materials[materialIndex]);
    for (unsigned int materialLayer = 0; materialLayer < material->layers.Count(); ++materialLayer) {
      unsigned int animatedId = layers[materialLayer];
      if (animatedId != static_cast<unsigned int>(-1)) {
        CTexLayer   &layer = material->layers[materialLayer];
        unsigned int originalId = layer.tmuPass[0].textureId;
        if (HandleObjectCompare(
                reinterpret_cast<HOBJECT>(unique->m_textures[animatedId].handle), reinterpret_cast<HOBJECT>(unique->m_textures[originalId].handle)
            ))
        {
          layer.tmuPass[0].textureId = animatedId;
        }
      }
    }
  }
  SetGeosetMatrices(unique, shared, animData.boneMtx);
  ReleaseLayerIndex(numLayers);
  ReleaseTransformListIndex(shared->numBones);
  SetLayerAlpha(unique->m_materials.Ptr(), numMaterials, s_layerAlpha.Ptr());
}

static void __fastcall UpdateEmitters(CModelComplex *unique, CModelShared *shared, const NTempest::C3Vector &cameraWorldPos) {
  float elapsed = AnimGetElapsedTime() * 0.001f;

  unsigned int i;
  unsigned int numEmitters = unique->m_emitters2.Count();
  for (i = 0; i < numEmitters; ++i) {
    NTempest::C34Matrix currentWorldMatrix = unique->m_modelToWorld;
    currentWorldMatrix.Translate(shared->positions[shared->emitter2Order[i]]);
    currentWorldMatrix.Rotate(PI * 0.5f, NTempest::C3Vector(0.0f, 0.0f, 1.0f), true);
    unique->m_emitters2[i]->SetEnabled(1, 1);
    unique->m_emitters2[i]->Update(elapsed, currentWorldMatrix, cameraWorldPos);
  }

  unsigned int numRibbons = unique->m_ribbons.Count();
  for (i = 0; i < numRibbons; ++i) {
    unique->m_ribbons[i]->SetEnabled(1);
    unique->m_ribbons[i]->Update(elapsed, 0);
  }
}

static void __fastcall
IModelAnimate(CModelComplex *unique, CModelShared *shared, const NTempest::C3Vector &cameraWorldPos, const NTempest::C3Vector &cameraVector) {
  const unsigned int numAttachments = unique->m_attached.Count();
  unsigned int       transforms = GetTransformListIndex(numAttachments);

  if (unique->m_anim) {
    unsigned int numMaterials = unique->m_materials.Count();
    s_layerAlpha.SetCount(shared->numLayers);
    GetLayerAlpha(unique->m_materials.Ptr(), numMaterials, s_layerAlpha.Ptr());
    unsigned int bones = GetTransformListIndex(shared->numBones);
    unsigned int numLayers = 0;
    for (unsigned int i = 0; i < numMaterials; ++i) {
      CMaterial *material = reinterpret_cast<CMaterial *>(unique->m_materials[i]);
      numLayers += material->layers.Count();
    }
    unsigned int  layerIndex = GetLayerIndex(numLayers);
    unsigned int *layers = GetLayerPtr(layerIndex);
    IGetLayerIDs(layers, numLayers);
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
    animData.layerTextureIds = layers;
    AnimAnimateModel(unique->m_anim, animData);

    if (unique->m_attachmentFlags.Count()) {
      memset(unique->m_attachmentFlags.Ptr(), 0, unique->m_attachmentFlags.Count());
    }
    for (unsigned int materialIndex = 0; materialIndex < numMaterials; ++materialIndex) {
      CMaterial *material = reinterpret_cast<CMaterial *>(unique->m_materials[materialIndex]);
      for (unsigned int materialLayer = 0; materialLayer < material->layers.Count(); ++materialLayer) {
        unsigned int animatedId = layers[materialLayer];
        if (animatedId != static_cast<unsigned int>(-1)) {
          CTexLayer   &layer = material->layers[materialLayer];
          unsigned int originalId = layer.tmuPass[0].textureId;
          if (HandleObjectCompare(
                  reinterpret_cast<HOBJECT>(unique->m_textures[animatedId].handle), reinterpret_cast<HOBJECT>(unique->m_textures[originalId].handle)
              ))
          {
            layer.tmuPass[0].textureId = animatedId;
          }
        }
      }
    }
    SetGeosetMatrices(unique, shared, animData.boneMtx);
    SetCollisionMatrices(unique, shared, animData.boneMtx);
    ReleaseLayerIndex(numLayers);
    ReleaseTransformListIndex(shared->numBones);
    SetLayerAlpha(unique->m_materials.Ptr(), numMaterials, s_layerAlpha.Ptr());
  } else {
    SetUnanimatedGeosetMatrices(unique, shared);
    SetUnanimatedCollisionMatrices(unique);
    UpdateEmitters(unique, shared, cameraWorldPos);
  }

  int normalizeNorms = unique->m_flags & 2;
  for (unsigned int attachmentIndex = 0; attachmentIndex < numAttachments; ++attachmentIndex) {
    int enabled = 1;
    if (unique->m_anim) {
      if (unique->m_attachmentFlags[attachmentIndex] & 1) {
        enabled = unique->m_attachmentFlags[attachmentIndex] & 2;
      } else {
        enabled = AnimIsAttachmentEnabled(unique->m_anim, attachmentIndex);
        unique->m_attachmentFlags[attachmentIndex] = (enabled ? 2 : 0) | 1;
      }
    }
    if (!enabled)
      continue;
    TSList<LINKUNIQUE, TSGetLink<LINKUNIQUE> > &list = unique->m_attached[attachmentIndex];
    for (LINKUNIQUE *link = list.Head(); link; link = list.Next(link)) {
      ModelAnimateAttached(link->child, link->scale, transforms + attachmentIndex, normalizeNorms, cameraWorldPos, cameraVector);
    }
  }
  ReleaseTransformListIndex(numAttachments);
}

static void __fastcall
IModelAnimate(CModelBase *unique, CModelShared *shared, const NTempest::C3Vector &cameraWorldPos, const NTempest::C3Vector &cameraVector) {
  ASSERT(unique);
  ASSERT(shared);
  if (unique->m_flags & 0x20) {
    IModelAnimate(static_cast<CModelComplex *>(unique), shared, cameraWorldPos, cameraVector);
  } else {
    IModelAnimate(static_cast<CModelSimple *>(unique), shared, cameraWorldPos, cameraVector);
  }
}

static void __fastcall IModelAnimateBounds(HMODEL model) {
  CModelBase   *unique;
  CModelShared *shared;
  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique, &shared)) {
    IModelAnimate(unique, shared, NTempest::C3Vector(), NTempest::C3Vector());
  }
}

static void __fastcall ApplyWorldTransforms(
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

static void __fastcall ApplyWorldTransforms(
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

static void __fastcall ModelAnimateAttached(
    HMODEL                    model,
    float                     scale,
    unsigned int              transform,
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

static void __fastcall IModelProcessEvents(CModelBase *unique, CModelShared *shared) {
  FATALASSERT(unique);
  FATALASSERT(shared);

  if (unique->m_anim) {
    AnimProcessEvents(unique->m_anim, shared->positions);
  }

  if (!(unique->m_flags & 0x20)) {
    return;
  }

  CModelComplex *complex = static_cast<CModelComplex *>(unique);
  unsigned int   numAttached = complex->m_attached.Count();
  for (unsigned int index = 0; index < numAttached; ++index) {
    int enabled = !unique->m_anim || AnimIsAttachmentEnabled(unique->m_anim, index);
    if (!enabled) {
      continue;
    }

    TSList<LINKUNIQUE, TSGetLink<LINKUNIQUE> > &attached = complex->m_attached[index];
    for (LINKUNIQUE *link = attached.Head(); link; link = attached.Next(link)) {
      CModelBase   *childModel;
      CModelShared *childShared;
      if (IModelDerefHandle(reinterpret_cast<CModel *>(link->child), &childModel, &childShared)) {
        WorldMatrixPush();
        WorldMatrixLoad(childModel->m_modelToWorld);
        IModelProcessEvents(childModel, childShared);
        WorldMatrixPop();
      }
    }
  }
}

void __fastcall ModelAnimateInitialize() {
  s_transInUse = 0;
}

void __fastcall ModelAnimateDestroy() {
  ModelAnimateLogStop();
}

void __fastcall ModelAnimateLogStart(const char *fileName) {
}

void __fastcall ModelAnimateLogStop() {
}

int __fastcall ModelAnimateLogToggle(const char *fileName) {
  return 0;
}

void __fastcall ModelAnimateCameras(HMODEL model, const NTempest::C34Matrix &orientation) {
  CModelBase *unique;

  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) && unique->m_anim && (unique->m_flags & 0x20)) {
    ActivityBegin(ACTIVITY_ANIMATE);
    WorldMatrixPush();
    WorldMatrixLoad(orientation);
    CModelComplex *complex = static_cast<CModelComplex *>(unique);
    AnimAnimateCameras(unique->m_anim, complex->m_cameras);
    WorldMatrixPop();
    ActivityEnd(ACTIVITY_ANIMATE);
  }
}

void __fastcall
ModelAnimateCameras(HMODEL model, const NTempest::C3Vector &position, float rotationAngle, const NTempest::C3Vector &rotationAxis, float scale) {
  NTempest::C34Matrix orientation;

  ApplyWorldTransforms(position, rotationAngle, rotationAxis, scale, &orientation);
  ModelAnimateCameras(model, orientation);
}

void __fastcall ModelProcessEvents(HMODEL model, const NTempest::C34Matrix &orientation) {
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

void __fastcall
ModelProcessEvents(HMODEL model, const NTempest::C3Vector &position, float rotationAngle, const NTempest::C3Vector &rotationAxis, float scale) {
  NTempest::C34Matrix orientation;

  ApplyWorldTransforms(position, rotationAngle, rotationAxis, scale, &orientation);
  ModelProcessEvents(model, orientation);
}

void __fastcall ModelAnimate(
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
  if (NTempest::CMath::fabs_(scale - 1.0f) < 0.00000095367432f)
    unique->m_flags &= ~2;
  else
    unique->m_flags |= 2;
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

void __fastcall ModelAnimate(
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

void __fastcall ModelAnimate(
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

int __fastcall ModelSetSequence(HMODEL model, unsigned int seqIndex, unsigned int flags) {
  FATALASSERT(model);

  CModelBase *unique;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    EnqueueModelCommand(reinterpret_cast<CModel *>(model), MODEL_SET_SEQUENCE1, seqIndex, flags);
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

int __fastcall ModelSetSequence(HMODEL model, unsigned int seqIndex, unsigned int objectId, unsigned int flags) {
  FATALASSERT(model);

  CModelBase *unique;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    EnqueueModelCommand(reinterpret_cast<CModel *>(model), MODEL_SET_SEQUENCE2, seqIndex, objectId, flags);
    return 1;
  }

  if (!unique->m_anim) {
    SErrSetLastError(ERROR_PATH_NOT_FOUND);
    return 0;
  }

  return AnimSetSequence(unique->m_anim, seqIndex, objectId, flags);
}

int __fastcall ModelMatchSequence(HMODEL model, unsigned int objectId, unsigned int sameAsObjectId, unsigned int flags) {
  FATALASSERT(model);

  CModelBase *unique;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    EnqueueModelCommand(reinterpret_cast<CModel *>(model), MODEL_MATCH_SEQUENCE, objectId, sameAsObjectId, flags);
    return 1;
  }

  if (!unique->m_anim) {
    SErrSetLastError(ERROR_PATH_NOT_FOUND);
    return 0;
  }

  return AnimMatchSequence(unique->m_anim, objectId, sameAsObjectId, flags);
}

int __fastcall ModelSetRandomSequenceFidget(HMODEL model, unsigned int seqIndex, unsigned int flags) {
  FATALASSERT(model);

  CModelBase *unique;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    EnqueueModelCommand(reinterpret_cast<CModel *>(model), MODEL_SET_RANDOM_SEQUENCE_FIDGET1, seqIndex, flags);
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

int __fastcall ModelSetRandomSequenceFidget(HMODEL model, unsigned int seqIndex, unsigned int objectId, unsigned int flags) {
  FATALASSERT(model);

  CModelBase *unique;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    EnqueueModelCommand(reinterpret_cast<CModel *>(model), MODEL_SET_RANDOM_SEQUENCE_FIDGET2, seqIndex, objectId, flags);
    return 1;
  }

  if (!unique->m_anim) {
    SErrSetLastError(ERROR_PATH_NOT_FOUND);
    return 0;
  }

  return AnimSetRandomSequenceFidget(unique->m_anim, seqIndex, objectId, flags);
}

unsigned int __fastcall ModelGetNumSequenceFidgets(HMODEL model, unsigned int seqIndex) {
  CModelBase *unique;

  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) && unique->m_anim) {
    return AnimGetNumSequenceFidgets(unique->m_anim, seqIndex);
  }
  return 0;
}

int __fastcall ModelSetSequenceFidget(HMODEL model, unsigned int seqIndex, unsigned int fidgetId, unsigned int flags) {
  FATALASSERT(model);

  CModelBase *unique;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    EnqueueModelCommand(reinterpret_cast<CModel *>(model), MODEL_SET_SEQUENCE_FIDGET1, seqIndex, fidgetId, flags);
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

int __fastcall ModelSetSequenceFidget(HMODEL model, unsigned int seqIndex, unsigned int fidgetId, unsigned int objectId, unsigned int flags) {
  FATALASSERT(model);

  CModelBase *unique;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    EnqueueModelCommand(reinterpret_cast<CModel *>(model), MODEL_SET_SEQUENCE_FIDGET1, seqIndex, fidgetId, objectId, flags);
    return 1;
  }

  if (!unique->m_anim) {
    SErrSetLastError(ERROR_PATH_NOT_FOUND);
    return 0;
  }

  return AnimSetSequenceFidget(unique->m_anim, seqIndex, fidgetId, objectId, flags);
}

unsigned int __fastcall ModelGetNumSequences(HMODEL model) {
  CModelBase *unique;

  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) && unique->m_anim) {
    return AnimGetNumSequences(unique->m_anim);
  }
  return 0;
}

int __fastcall ModelGetSequenceDuration(HMODEL model, unsigned int seqIndex, unsigned int *duration) {
  FATALASSERT(duration);
  *duration = 0;

  CModelBase *unique;
  return IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) && unique->m_anim &&
         AnimGetSequenceDuration(unique->m_anim, seqIndex, duration);
}

int __fastcall ModelGetSequenceMoveSpeed(HMODEL model, unsigned int seqIndex, float *moveSpeed) {
  FATALASSERT(moveSpeed);
  *moveSpeed = 0.0f;

  CModelBase *unique;
  return IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) && unique->m_anim &&
         AnimGetSequenceMoveSpeed(unique->m_anim, seqIndex, moveSpeed);
}

int __fastcall ModelGetSequenceName(HMODEL model, unsigned int seqIndex, char *buffer, unsigned int buffLength) {
  FATALASSERT(buffer);
  if (buffLength) {
    buffer[0] = 0;
  }

  CModelBase *unique;
  return IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) && unique->m_anim &&
         AnimGetSequenceName(unique->m_anim, seqIndex, buffer, buffLength);
}

int __fastcall ModelHasSequenceId(HMODEL model, unsigned int seqIndex) {
  CModelBase *unique;

  return IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) && unique->m_anim && AnimHasSequenceId(unique->m_anim, seqIndex);
}

unsigned int __fastcall ModelGetTotalKeys(HMODEL model) {
  CModelBase *unique;

  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) && unique->m_anim) {
    return AnimGetTotalKeys(unique->m_anim);
  }
  return 0;
}

void __fastcall ModelSetSeqFinishedHandler(HMODEL model, ANIMSEQFINISHEDHANDLER callback, void *param) {
  CModelBase *unique;

  FATALASSERT(model);

  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    EnqueueModelCommand(reinterpret_cast<CModel *>(model), MODEL_SET_SEQ_FINISHED_HANDLER1, callback, param);
    return;
  }

  if (unique->m_anim) {
    AnimSetSeqFinishedHandler(unique->m_anim, callback, param);
  }
}

void __fastcall ModelSetSeqFinishedHandler(HMODEL model, unsigned int sequence, ANIMSEQFINISHEDHANDLER callback, void *param) {
  CModelBase *unique;

  FATALASSERT(model);

  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    EnqueueModelCommand(reinterpret_cast<CModel *>(model), MODEL_SET_SEQ_FINISHED_HANDLER2, sequence, callback, param);
    return;
  }

  if (unique->m_anim) {
    AnimSetSeqFinishedHandler(unique->m_anim, sequence, callback, param);
  }
}

void __fastcall ModelSetTimeScale(HMODEL model, float timeScale, int doLinkedModels) {
  FATALASSERT(model);

  CModelBase *unique;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    EnqueueModelCommand(reinterpret_cast<CModel *>(model), MODEL_SET_TIME_SCALE, timeScale, doLinkedModels);
    return;
  }

  if (unique->m_anim) {
    AnimSetTimeScale(unique->m_anim, timeScale);
  }

  if (!doLinkedModels || !(unique->m_flags & 0x20)) {
    return;
  }

  CModelComplex *complex = static_cast<CModelComplex *>(unique);
  unsigned int   numAttachments = complex->m_attached.Count();
  for (unsigned int index = 0; index < numAttachments; ++index) {
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
      TSList<LINKUNIQUE, TSGetLink<LINKUNIQUE> > &links = complex->m_attached[index];
      for (LINKUNIQUE *link = links.Head(); link; link = links.Next(link)) {
        ModelSetTimeScale(link->child, timeScale, 1);
      }
    }
  }
}

int __fastcall ModelSetObjectTimeScale(HMODEL model, unsigned int objectId, float timeScale, int doLinkedModels) {
  FATALASSERT(model);

  CModelBase *unique;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    EnqueueModelCommand(reinterpret_cast<CModel *>(model), MODEL_SET_OBJECT_TIME_SCALE, objectId, timeScale, doLinkedModels);
    return 1;
  }

  if (unique->m_anim && !AnimSetObjectTimeScale(unique->m_anim, objectId, timeScale)) {
    return 0;
  }

  if (!doLinkedModels || !(unique->m_flags & 0x20)) {
    return 1;
  }

  CModelComplex *complex = static_cast<CModelComplex *>(unique);
  unsigned int   numAttachments = complex->m_attached.Count();
  for (unsigned int index = 0; index < numAttachments; ++index) {
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
      TSList<LINKUNIQUE, TSGetLink<LINKUNIQUE> > &links = complex->m_attached[index];
      for (LINKUNIQUE *link = links.Head(); link; link = links.Next(link)) {
        if (!ModelSetObjectTimeScale(link->child, objectId, timeScale, 1)) {
          return 0;
        }
      }
    }
  }

  return 1;
}

float __fastcall ModelGetTimeScale(HMODEL model) {
  CModelBase *unique;

  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) && unique->m_anim) {
    return AnimGetTimeScale(unique->m_anim);
  }
  return 0.0f;
}

float __fastcall ModelGetObjectTimeScale(HMODEL model, unsigned int objectId) {
  CModelBase *unique;

  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) && unique->m_anim) {
    return AnimGetObjectTimeScale(unique->m_anim, objectId);
  }
  return 0.0f;
}

int __fastcall ModelForceCurrentSequenceTime(HMODEL model, int timeOffset, int doLinkedModels) {
  FATALASSERT(model);

  CModelBase *unique;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    EnqueueModelCommand(reinterpret_cast<CModel *>(model), MODEL_FORCE_CURRENT_SEQUENCE_TIME, timeOffset, doLinkedModels);
    return 1;
  }

  if (!unique->m_anim || !AnimForceCurrentSequenceTime(unique->m_anim, timeOffset)) {
    return 0;
  }

  if (!doLinkedModels || !(unique->m_flags & 0x20)) {
    return 1;
  }

  CModelComplex *complex = static_cast<CModelComplex *>(unique);
  unsigned int   numAttachments = complex->m_attached.Count();
  for (unsigned int index = 0; index < numAttachments; ++index) {
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
      TSList<LINKUNIQUE, TSGetLink<LINKUNIQUE> > &links = complex->m_attached[index];
      for (LINKUNIQUE *link = links.Head(); link; link = links.Next(link)) {
        if (!ModelForceCurrentSequenceTime(link->child, timeOffset, 1)) {
          return 0;
        }
      }
    }
  }

  return 1;
}

int __fastcall ModelForceSequenceTime(HMODEL model, unsigned int seqIndex, int timeOffset, int doLinkedModels) {
  FATALASSERT(model);

  CModelBase *unique;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    EnqueueModelCommand(reinterpret_cast<CModel *>(model), MODEL_FORCE_SEQUENCE_TIME, seqIndex, timeOffset, doLinkedModels);
    return 1;
  }

  if (!unique->m_anim || !AnimForceSequenceTime(unique->m_anim, seqIndex, timeOffset)) {
    return 0;
  }

  if (doLinkedModels && (unique->m_flags & 0x20)) {
    CModelComplex                              *complex = static_cast<CModelComplex *>(unique);
    unsigned int                                numAttachments = complex->m_attached.Count();
    TSList<LINKUNIQUE, TSGetLink<LINKUNIQUE> > *attached = complex->m_attached.Ptr();
    unsigned int                                index = 0;

    while (index < numAttachments) {
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

int __fastcall ModelAdvanceTime(HMODEL model) {
  CModelBase *unique;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    return 0;
  }

  if (unique->m_anim && !AnimAdvanceTime(unique->m_anim, GxPerfCounter(GxPerf_FrameNum))) {
    return 0;
  }

  ASSERT(reinterpret_cast<CModel *>(model));
  if (unique->m_flags & 0x20) {
    CModelComplex                              *complex = static_cast<CModelComplex *>(unique);
    unsigned int                                numAttachments = complex->m_attached.Count();
    TSList<LINKUNIQUE, TSGetLink<LINKUNIQUE> > *attached = complex->m_attached.Ptr();

    for (unsigned int index = 0; index < numAttachments; ++index) {
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
        for (LINKUNIQUE *link = attached->Head(); link; link = attached->Next(link)) {
          ModelAdvanceTime(link->child);
        }
      }

      ++attached;
    }
  }

  return 1;
}

int __fastcall ModelAdvanceTime(HMODEL model, int timeChange) {
  CModelBase *unique;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    return 0;
  }

  if (unique->m_anim && !AnimManualAdvanceTime(unique->m_anim, timeChange)) {
    return 0;
  }

  FATALASSERT(reinterpret_cast<CModel *>(model));
  if (unique->m_flags & 0x20) {
    CModelComplex                              *complex = static_cast<CModelComplex *>(unique);
    unsigned int                                numAttachments = complex->m_attached.Count();
    TSList<LINKUNIQUE, TSGetLink<LINKUNIQUE> > *attached = complex->m_attached.Ptr();

    for (unsigned int index = 0; index < numAttachments; ++index) {
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
        for (LINKUNIQUE *link = attached->Head(); link; link = attached->Next(link)) {
          ModelAdvanceTime(link->child, timeChange);
        }
      }
      ++attached;
    }
  }

  return 1;
}

void __fastcall ModelPauseTime(HMODEL model, int pause, int doLinkedModels) {
  CModelBase *unique;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    return;
  }

  if (unique->m_anim) {
    AnimPauseTime(unique->m_anim, pause);
  }

  if (doLinkedModels && (unique->m_flags & 0x20)) {
    CModelComplex                              *complex = static_cast<CModelComplex *>(unique);
    unsigned int                                numAttachments = complex->m_attached.Count();
    TSList<LINKUNIQUE, TSGetLink<LINKUNIQUE> > *attached = complex->m_attached.Ptr();

    for (unsigned int index = 0; index < numAttachments; ++index) {
      int enabled = !unique->m_anim || AnimIsAttachmentEnabled(unique->m_anim, index);
      if (enabled) {
        for (LINKUNIQUE *link = attached->Head(); link; link = attached->Next(link)) {
          ModelPauseTime(link->child, pause, 1);
        }
      }
      ++attached;
    }
  }
}

void __fastcall ModelResetGlobalSequenceTimes(HMODEL model, int doLinkedModels) {
  CModelBase *unique;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    return;
  }

  if (unique->m_anim) {
    AnimResetGlobalSequenceTimes(unique->m_anim);
  }

  if (doLinkedModels && (unique->m_flags & 0x20)) {
    CModelComplex *complex = static_cast<CModelComplex *>(unique);
    unsigned int   numAttachments = complex->m_attached.Count();
    for (unsigned int index = 0; index < numAttachments; ++index) {
      int enabled = !unique->m_anim || AnimIsAttachmentEnabled(unique->m_anim, index);
      if (!enabled) {
        continue;
      }

      TSList<LINKUNIQUE, TSGetLink<LINKUNIQUE> > &attached = complex->m_attached[index];
      for (LINKUNIQUE *link = attached.Head(); link; link = attached.Next(link)) {
        ModelResetGlobalSequenceTimes(link->child, 0);
      }
    }
  }
}

void __fastcall
ModelSetEventCallback(HMODEL model, void(__fastcall *callback)(const char *, const NTempest::C3Vector &, void *), void *param, int doLinkedModels) {
  CModelBase *unique;

  FATALASSERT(model);

  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    EnqueueModelCommand(reinterpret_cast<CModel *>(model), MODEL_SET_EVENT_CALLBACK, callback, param, doLinkedModels);
    return;
  }

  if (unique->m_anim) {
    AnimSetEventCallback(unique->m_anim, callback, param);
  }

  if (doLinkedModels && (unique->m_flags & 0x20)) {
    CModelComplex                              *complex = static_cast<CModelComplex *>(unique);
    unsigned int                                numAttachments = complex->m_attached.Count();
    TSList<LINKUNIQUE, TSGetLink<LINKUNIQUE> > *attached = complex->m_attached.Ptr();
    while (numAttachments) {
      for (LINKUNIQUE *link = attached->Head(); link; link = attached->Next(link)) {
        ModelSetEventCallback(link->child, callback, param, 0);
      }
      ++attached;
      --numAttachments;
    }
  }
}

int __fastcall ModelApplyObjectLookAt(HMODEL model, unsigned int objectId, const NTempest::C3Vector &target) {
  FATALASSERT(model);

  CModelBase *unique;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    EnqueueModelCommand(reinterpret_cast<CModel *>(model), MODEL_APPLY_OBJECT_LOOK_AT, objectId, target);
    return 1;
  }

  return unique->m_anim ? AnimApplyObjectLookAt(unique->m_anim, objectId, target) : 0;
}

int __fastcall ModelRemoveObjectLookAt(HMODEL model, unsigned int objectId) {
  FATALASSERT(model);

  CModelBase *unique;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    EnqueueModelCommand(reinterpret_cast<CModel *>(model), MODEL_REMOVE_OBJECT_LOOK_AT, objectId);
    return 1;
  }

  return unique->m_anim ? AnimRemoveObjectLookAt(unique->m_anim, objectId) : 0;
}

int __fastcall ModelObjectUsingLookAt(HMODEL model, unsigned int objectId) {
  CModelBase *unique;

  return IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) && unique->m_anim && AnimObjectUsingLookAt(unique->m_anim, objectId);
}

int __fastcall ModelApplyObjectFaceDir(HMODEL model, unsigned int objectId, const NTempest::C3Vector &direction) {
  FATALASSERT(model);

  CModelBase *unique;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    EnqueueModelCommand(reinterpret_cast<CModel *>(model), MODEL_APPLY_OBJECT_FACE_DIR, objectId, direction);
    return 1;
  }

  return unique->m_anim ? AnimApplyObjectFaceDir(unique->m_anim, objectId, direction) : 0;
}

int __fastcall ModelRemoveObjectFaceDir(HMODEL model, unsigned int objectId) {
  FATALASSERT(model);

  CModelBase *unique;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    EnqueueModelCommand(reinterpret_cast<CModel *>(model), MODEL_REMOVE_OBJECT_FACE_DIR, objectId);
    return 1;
  }

  return unique->m_anim ? AnimRemoveObjectFaceDir(unique->m_anim, objectId) : 0;
}

int __fastcall ModelObjectUsingFaceDir(HMODEL model, unsigned int objectId) {
  CModelBase *unique;

  return IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) && unique->m_anim && AnimObjectUsingFaceDir(unique->m_anim, objectId);
}

int __fastcall ModelMarkFootstepSequence(HMODEL model, unsigned int seqIndex) {
  FATALASSERT(model);

  CModelBase *unique;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    EnqueueModelCommand(reinterpret_cast<CModel *>(model), MODEL_MARK_FOOTSTEP_SEQUENCE, seqIndex);
    return 1;
  }

  return unique->m_anim ? AnimMarkFootstepSequence(unique->m_anim, seqIndex) : 0;
}

int __fastcall ModelLockObjectSequence(HMODEL model, unsigned int objectId, int set) {
  FATALASSERT(model);

  CModelBase *unique;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    EnqueueModelCommand(reinterpret_cast<CModel *>(model), MODEL_LOCK_OBJECT_SEQUENCE, objectId, set);
    return 1;
  }

  return unique->m_anim ? AnimLockObjectSequence(unique->m_anim, objectId, set) : 0;
}

void __fastcall ModelGetStandingMatrix(
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

void __fastcall ModelForceStandingMatrix(
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
    NTempest::C34Matrix standing;
    NTempest::C34Matrix forced;
    IModelGetStandingMatrix(shared->groundTrack, position, groundNormal, facing, 1.0f, &standing);
    IModelGetStandingMatrix(static_cast<GROUND_TRACK>(enumGroundTrack), position, groundNormal, facing, 1.0f, &forced);

    float        standingRatio = 1.0f - blendRatio;
    float       *out = &orientation->a0;
    const float *stand = &standing.a0;
    const float *force = &forced.a0;
    for (int i = 0; i < 9; ++i) {
      out[i] = stand[i] * standingRatio + force[i] * blendRatio;
    }
    orientation->d0 = position.x;
    orientation->d1 = position.y;
    orientation->d2 = position.z;
    orientation->Scale(scale);
  }
}

float __fastcall ModelGetPrimarySequenceCompletion(HMODEL model) {
  CModelBase *unique;

  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) && unique->m_anim) {
    return AnimGetPrimarySequenceCompletion(unique->m_anim);
  }
  return 0.0f;
}

int __fastcall ModelEventEmitterHasKeysThisSeq(HMODEL model, unsigned int objectId) {
  CModelBase *unique;

  return IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) && unique->m_anim && AnimEventEmitterHasKeysThisSeq(unique->m_anim, objectId);
}

int __fastcall ModelGetModelSpacePivot(HMODEL model, unsigned int objectId, NTempest::C3Vector *pivot) {
  CModelBase   *unique;
  CModelShared *shared;

  return IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique, &shared) && unique->m_anim &&
         AnimGetObjectPosition(unique->m_anim, objectId, shared->positions, pivot);
}

int __fastcall ModelGetObjectPosition(HMODEL model, unsigned int objectId, NTempest::C3Vector *position) {
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

int __fastcall ModelGetEventObjectPosition(HMODEL model, unsigned int objectId, int modelSpace, NTempest::C3Vector *position) {
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
