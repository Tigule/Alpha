#include <Base/Base.h>

#include "IModel.h"
#include "ModelInternal.h"

#include "Anim/AnimTypes.h"
#include "Gxu/IGxuLight.h"
#include "Gx/CGxDevice.h"
#include "Services/Camera.h"
#include "Services/ParticleSystem2.h"
#include "Services/RibbonEmitter.h"
#include "Tempest/cmath.h"

#include <malloc.h>
#include <string.h>

using namespace NTempest;

void   ExecuteQueuedActions(CModel *model);
HMODEL ModelDuplicate(HMODEL sourceModel, UINT flags);
void   ModelSetMaterialDisables(HMODEL model, UINT setMask, UINT unsetMask, int doLinkedModels);

static UINT UpdateRibbonMaterial(CModelComplex *model, UINT replaceableId, HTEXTURE texture);
static void UpdateParticleEmitters(CModelComplex *unique, UINT replaceableId, HTEXTURE texture);

struct CMatrixGroup {
  UINT *matrices;
  UINT  numMatrices;
  UINT  index;
  UINT  leftIndex;
  UINT  rightIndex;

  CMatrixGroup() : matrices(0), numMatrices(0), index(0), leftIndex(-1), rightIndex(-1) {
  }
  CMatrixGroup(UINT *, UINT);
};

class CMatrixGroupTree {
 private:
  TSGrowableArray<CMatrixGroup> nodes;
  UINT                          numMatrices;
  UINT                          AddNode(UINT *matrixGroup, UINT numMatrices);

 public:
  CMatrixGroupTree() : numMatrices(0) {
  }

  UINT GroupCount() const {
    return nodes.Count();
  }
  UINT MatrixCount() const {
    return numMatrices;
  }
  UINT Insert(UINT *matrixGroup, UINT numMatrices);
  BOOL GroupsEqual(const UINT *matrixGroup1, UINT numMatrices1, const UINT *matrixGroup2, UINT numMatrices2);
  int  GroupLessThan(const UINT *matrixGroup1, UINT numMatrices1, const UINT *matrixGroup2, UINT numMatrices2);
};

static BOOL MaterialUsedOnce(HMATERIAL material) {
  CMaterial *uniqueMtl = (CMaterial *)material;

  ASSERT(uniqueMtl);
  return uniqueMtl->GetRefCount() == 1;
}

static HMATERIAL MaterialDuplicate(HMATERIAL material) {
  CMaterial *source = (CMaterial *)material;
  ASSERT(source);

  CMaterial *duplicate = NEWHANDLE(HMATERIAL, CMaterial);
  if (!duplicate) {
    return 0;
  }

  *duplicate = *source;
  return CREATEHANDLE(HMATERIAL, duplicate);
}

CModelBase::~CModelBase() {
  if (m_anim) {
    HandleClose(m_anim);
  }
  if (m_boundsModel) {
    HandleClose(m_boundsModel);
  }
  if (m_collideModel) {
    HandleClose(m_collideModel);
  }
}

CModelComplex::~CModelComplex() {
  UINT i;
  UINT numElements;

  numElements = m_attached.Count();
  for (i = 0; i < numElements; ++i) {
    m_attached[i].Clear();
  }

  numElements = m_materials.Count();
  for (i = 0; i < numElements; ++i) {
    HandleClose(m_materials[i]);
  }
  numElements = m_cameras.Count();
  for (i = 0; i < numElements; ++i) {
    HandleClose(m_cameras[i]);
  }
  numElements = m_emitters2.Count();
  for (i = 0; i < numElements; ++i) {
    if (m_emitters2[i]) {
      ParticleSystemManager::GetInstance()->DeleteEmitter2(m_emitters2[i]);
    }
  }
  numElements = m_ribbons.Count();
  for (i = 0; i < numElements; ++i) {
    if (m_ribbons[i]) {
      RibbonManager::GetInstance()->DeleteEmitter(m_ribbons[i]);
    }
  }
  numElements = m_lights.Count();
  for (i = 0; i < numElements; ++i) {
    GxuLightDestroy(m_lights[i]);
  }
}

CModelSimple::~CModelSimple() {
  UINT numMaterials = m_materials.Count();
  for (UINT i = 0; i < numMaterials; ++i) {
    HandleClose(m_materials[i]);
  }
}

void CModelSimple::CopyMaterials(const CModelSimple &source) {
  UINT numMaterials = source.m_materials.Count();
  m_materials.SetCount(numMaterials);
  for (UINT i = 0; i < numMaterials; ++i) {
    m_materials[i] = (HMATERIAL)HandleDuplicate(source.m_materials[i]);
  }
}

void CModelComplex::CopyAttachments(const CModelComplex &source) {
  m_attached.SetCount(source.m_attached.Count());
  m_attachmentFlags.SetCount(source.m_attachmentFlags.Count());
  m_attachmentFlags.Zero();

  LIST(LINKUNIQUE) *src = (LIST(LINKUNIQUE) *)source.m_attached.Ptr();
  LIST(LINKUNIQUE) *dest = m_attached.Ptr();
  for (UINT i = m_attached.Count(); i; --i, ++src, ++dest) {
    ITERATELISTPTR(LINKUNIQUE, src, link) {
      LINKUNIQUE *copy = dest->NewNode(LIST_TAIL, 0, 0);
      copy->child = ModelDuplicate(link->child, 0);
    }
  }
}

void CModelComplex::CopyCameras(const CModelComplex &source) {
  m_cameras.SetCount(source.m_cameras.Count());
  const HCAMERA *src = source.m_cameras.Ptr();
  HCAMERA       *dest = m_cameras.Ptr();
  for (UINT i = m_cameras.Count(); i; --i, ++dest, ++src) {
    *dest = CameraDuplicate(*src);
  }
  m_cameraOrder = source.m_cameraOrder;
}

void CModelComplex::CopyLights(const CModelComplex &source) {
  m_lights.SetCount(source.m_lights.Count());
  const DWORD *src = source.m_lights.Ptr();
  DWORD       *newLightId = m_lights.Ptr();
  for (UINT i = m_lights.Count(); i; --i, ++newLightId, ++src) {
    *newLightId = GxuLightCreate();
    if (*newLightId) {
      CGxLight *newLight = GxuLightLock(*newLightId);
      CGxLight *light = GxuLightLock(*src);
      if (newLight && light) {
        *newLight = *light;
      }
      GxuLightUnlock(*newLightId);
      GxuLightUnlock(*src);
    }
  }
}

void CModelComplex::CopyEmitters(const CModelComplex &source) {
  UINT numEmitters = source.m_emitters2.Count();
  m_emitters2.SetCount(numEmitters);
  for (UINT i = 0; i < numEmitters; ++i) {
    m_emitters2[i] = ParticleSystemManager::GetInstance()->DuplicateEmitter(source.m_emitters2[i], 0);
  }
}

void CModelComplex::CopyRibbons(const CModelComplex &source) {
  m_ribbons.SetCount(source.m_ribbons.Count());
  CRibbonEmitter       **dest = m_ribbons.Ptr();
  CRibbonEmitter *const *src = source.m_ribbons.Ptr();
  for (UINT i = m_ribbons.Count(); i; --i, ++dest, ++src) {
    *dest = RibbonManager::GetInstance()->DuplicateEmitter(*src);
  }
}

void GxuLightSelectCallback(LPVOID parm, NTempest::C3Vector worldPos, const NTempest::C3Vector &cameraWorldPos, UINT maxLightsToUse) {
  (void)parm;
  GxuLightSelect(worldPos, cameraWorldPos, maxLightsToUse);
}

CModelBase::CModelBase(const CModelBase &source) {
  UINT flags = (source.m_flags >> 2) & 1;
  m_anim = source.m_anim ? AnimDuplicate(source.m_anim, flags) : 0;
  m_boundsModel = source.m_boundsModel ? ModelDuplicate(source.m_boundsModel, 0) : 0;
  m_collideModel = source.m_collideModel ? ModelDuplicate(source.m_collideModel, 0) : 0;
  m_aaBoxCustGeoId = source.m_aaBoxCustGeoId;
  m_flags = source.m_flags & ~4U;
  m_PickLights = source.m_PickLights;
  m_pickLightsParm = source.m_pickLightsParm;
}

CModelComplex::CModelComplex(const CModelSimple &source) : CModelBase(source) {
  UINT numMaterials = source.m_materials.Count();
  m_materials.SetCount(numMaterials);
  for (UINT i = 0; i < numMaterials; ++i) {
    m_materials[i] = (HMATERIAL)HandleDuplicate(source.m_materials[i]);
  }

  m_textures.Set(source.m_textures.Count(), source.m_textures.Ptr());
  m_geosets.Set(source.m_geosets.Count(), source.m_geosets.Ptr());
  m_geosetColor.Set(source.m_geosetColor.Count(), source.m_geosetColor.Ptr());
}

CModelComplex::CModelComplex(const CModelComplex &source) : CModelBase(source) {
  UINT numMaterials = source.m_materials.Count();
  m_materials.SetCount(numMaterials);
  for (UINT i = 0; i < numMaterials; ++i) {
    m_materials[i] = (HMATERIAL)HandleDuplicate(source.m_materials[i]);
  }

  CopyAttachments(source);
  CopyCameras(source);
  CopyLights(source);
  CopyEmitters(source);
  CopyRibbons(source);
  m_textures = source.m_textures;
  m_geosets = source.m_geosets;
  m_geosetColor = source.m_geosetColor;
  m_addlGeosets = source.m_addlGeosets;
  m_hitTestMtx = source.m_hitTestMtx;
}

CModelSimple::CModelSimple(const CModelSimple &source) : CModelBase(source) {
  CopyMaterials(source);

  m_textures = source.m_textures;
  m_geosets = source.m_geosets;
  m_geosetColor = source.m_geosetColor;
  m_custGeosets.SetCount(0);
}

CModel::CModel(CModel &source)
    : asyncObject(0), createData(0) {
  ASSERT(source.shared);
  shared = (HMODELSHARED)HandleDuplicate(source.shared);
  ASSERT(shared);

  if (source.state == CMODEL_LOADED) {
    FinishDuplication(source);
  } else {
    dupSource = CREATEHANDLE(HMODEL, &source);
    state = CMODEL_DUPE_WAIT;
  }
}

void CModel::FinishDuplication(CModel &source) {
  ASSERT(source.state == CMODEL_LOADED);
  ASSERT(source.data);

  if (state == CMODEL_DUPE_WAIT_PRSRV_ANIM) {
    source.data->m_flags |= 4;
  }

  if (source.data->m_flags & 0x20) {
    data = NEW(CModelComplex)(*(CModelComplex *)source.data);
  } else {
    data = NEW(CModelSimple)(*(CModelSimple *)source.data);
  }

  source.data->m_flags &= ~4U;
  ASSERT(data);
  state = CMODEL_LOADED;
  ExecuteQueuedActions(this);
}

CModel::~CModel() {
  RemoveModelCommandsFromQueue();

  switch (state) {
    case CMODEL_LOADED:
      if (data->m_flags & 0x20) {
        DEL((CModelComplex *)data);
      } else {
        DEL((CModelSimple *)data);
      }
      break;
    case CMODEL_ASYNC_WAIT:
      DeleteAsyncObj();
      DEL(createData);
      break;
    case CMODEL_DUPE_WAIT:
    case CMODEL_DUPE_WAIT_PRSRV_ANIM:
      HandleClose(dupSource);
      break;
    default:
      break;
  }

  if (shared) {
    HandleClose(shared);
  }
}

static UINT UpdateRibbonMaterial(CModelComplex *model, UINT replaceableId, HTEXTURE texture) {
  ASSERT(model);
  ASSERT(texture);

  CRibbonEmitter **ribbon = model->m_ribbons.Ptr();
  UINT             numReplaced = 0;
  for (UINT count = model->m_ribbons.Count(); count; --count) {
    numReplaced += (*ribbon)->ReplaceTexture(replaceableId, texture);
    ++ribbon;
  }

  return numReplaced;
}

static void UpdateParticleEmitters(CModelComplex *unique, UINT replaceableId, HTEXTURE texture) {
  CParticleEmitter2 **emitter = unique->m_emitters2.Ptr();
  for (UINT index = 0; index < unique->m_emitters2.Count(); ++index, ++emitter) {
    if ((*emitter)->ReplaceableId() == replaceableId) {
      (*emitter)->SetTexture(texture);
    }
  }
}

void ModelMaterialShowLayer(HMODEL model, UINT materialIndex, UINT layerIndex, int show) {
  CModelBase *unique;
  if (!IModelDerefHandle((CModel *)model, &unique)) {
    return;
  }

  if (unique->m_flags & 0x20) {
    CModelComplex *complex = (CModelComplex *)unique;
    {
      CModelComplex *unique = complex;
      ASSERT(materialIndex < unique->m_materials.Count());
      CMaterial *uniqueMtl = (CMaterial *)unique->m_materials[materialIndex];
      ASSERT(uniqueMtl);
      ASSERT(layerIndex < uniqueMtl->layers.Count());
      uniqueMtl->layers[layerIndex].layerAlpha = show ? 0xFF : 0;
    }
  } else {
    CModelSimple *simple = (CModelSimple *)unique;
    {
      CModelSimple *unique = simple;
      ASSERT(materialIndex < unique->m_materials.Count());
      CMaterial *uniqueMtl = (CMaterial *)unique->m_materials[materialIndex];
      ASSERT(uniqueMtl);
      ASSERT(layerIndex < uniqueMtl->layers.Count());
      uniqueMtl->layers[layerIndex].layerAlpha = show ? 0xFF : 0;
    }
  }
}

static void ComplexModelSetEmissiveColor(CModelComplex *unique, const NTempest::CImVector &color, int doLinkedModels) {
  ASSERT(unique);

  ASSERT(unique);
  UINT numMaterials = unique->m_materials.Count();
  for (UINT i = 0; i < numMaterials; ++i) {
    if (!MaterialUsedOnce(unique->m_materials[i])) {
      HMATERIAL oldMaterial = unique->m_materials[i];
      unique->m_materials[i] = MaterialDuplicate(unique->m_materials[i]);
      HandleClose(oldMaterial);
    }

    CMaterial *uniqueMtl = (CMaterial *)unique->m_materials[i];
    ASSERT(uniqueMtl);
    uniqueMtl->emissiveColor = color;
  }
  if (doLinkedModels) {
    UINT numAttachments = unique->m_attached.Count();
    for (UINT i = 0; i < numAttachments; ++i) {
      ITERATELIST(LINKUNIQUE, unique->m_attached[i], link) {
        ModelSetEmissiveColor(link->child, color, 1);
      }
    }
  }
}

void ModelSetEmissiveColor(HMODEL model, const NTempest::CImVector &color, int doLinkedModels) {
  CModel *modelptr = (CModel *)model;
  VALIDATEBEGIN;
  VALIDATE(modelptr);
  VALIDATEENDVOID;

  CModelBase *unique;
  if (!IModelDerefHandle(modelptr, &unique)) {
    EnqueueModelCommand(modelptr, MODEL_SET_EMISSIVE_COLOR, *&color, doLinkedModels);
    return;
  }

  if (unique->m_flags & 0x20) {
    ComplexModelSetEmissiveColor((CModelComplex *)unique, color, doLinkedModels);
  } else {
    CModelSimple *simple = (CModelSimple *)unique;
    UINT numMaterials = simple->m_materials.Count();
    for (UINT i = 0; i < numMaterials; ++i) {
      if (!MaterialUsedOnce(simple->m_materials[i])) {
        HMATERIAL oldMaterial = simple->m_materials[i];
        simple->m_materials[i] = MaterialDuplicate(simple->m_materials[i]);
        HandleClose(oldMaterial);
      }

      CMaterial *uniqueMtl = (CMaterial *)simple->m_materials[i];
      ASSERT(uniqueMtl);
      uniqueMtl->emissiveColor = color;
    }
  }
}

static UINT ComplexModelReplaceTexture(CModelComplex *unique, UINT replaceableId, HTEXTURE texture, int doLinkedModels) {
  UINT numReplaced = 0;
  UINT numAttached;
  UINT index;

  UINT numTextures = unique->m_textures.Count();
  for (index = 0; index < numTextures; ++index) {
    CModelTexture &modelTexture = unique->m_textures[index];

    if (modelTexture.replaceableId == replaceableId) {
      HTEXTURE oldTexture = modelTexture.handle;
      modelTexture.handle = (HTEXTURE)HandleDuplicate(texture);
      if (oldTexture) {
        HandleClose(oldTexture);
      }
      ++numReplaced;
    }
  }

  numReplaced += UpdateRibbonMaterial(unique, replaceableId, texture);
  UpdateParticleEmitters(unique, replaceableId, texture);

  if (doLinkedModels) {
    numAttached = unique->m_attached.Count();
    for (index = 0; index < numAttached; ++index) {
      ITERATELIST(LINKUNIQUE, unique->m_attached[index], link) {
        numReplaced += ModelReplaceTexture(link->child, replaceableId, texture, 1);
      }
    }
  }

  return numReplaced;
}

BOOL ModelReplaceTexture(HMODEL model, UINT replaceableId, HTEXTURE texture, int doLinkedModels) {
  CModelBase *unique;

  CModel *modelptr = (CModel *)model;
  VALIDATEBEGIN;
  VALIDATE(modelptr);
  VALIDATE(replaceableId != 0);
  VALIDATEEND;

  if (!IModelDerefHandle(modelptr, &unique)) {
    EnqueueModelCommand(modelptr, MODEL_REPLACE_TEXTURE, replaceableId, texture, doLinkedModels);
    return 1;
  }

  if (unique->m_flags & 0x20) {
    return ComplexModelReplaceTexture((CModelComplex *)unique, replaceableId, texture, doLinkedModels) != 0;
  }

  CModelSimple *simple = (CModelSimple *)unique;
  UINT          numReplaced = 0;
  UINT          numTextures = simple->m_textures.Count();
  for (UINT index = 0; index < numTextures; ++index) {
    CModelTexture &modelTexture = simple->m_textures[index];

    if (modelTexture.replaceableId == replaceableId) {
      HTEXTURE oldTexture = modelTexture.handle;
      modelTexture.handle = (HTEXTURE)HandleDuplicate(texture);
      if (oldTexture) {
        HandleClose(oldTexture);
      }
      ++numReplaced;
    }
  }

  return numReplaced != 0;
}

UINT ModelGetMatrixCount(HMODEL model) {
  CModelShared *shared;
  if (IModelDerefHandle((CModel *)model, &shared)) {
    return shared->numBones;
  }
  return 0;
}

BOOL ModelGetLinkPoint(HMODEL model, UINT index, HMODEL *modelList, UINT *entriesInOut) {
  CModelShared *shared;
  CModelBase   *unique;
  UINT          added;

  VALIDATEBEGIN;
  VALIDATE(modelList);
  VALIDATE(entriesInOut);
  VALIDATE(*entriesInOut > 0);
  VALIDATEEND;

  if (!IModelDerefHandle((CModel *)model, &unique, &shared) || !(unique->m_flags & 0x20)) {
    *entriesInOut = 0;
    return 0;
  }

  CModelShared *dataptr = shared;
  ASSERT(dataptr);
  if (index >= shared->attachIdToIndex.Count()) {
    *entriesInOut = 0;
    return 0;
  }

  index = shared->attachIdToIndex[index];
  if (index == (UINT)-1) {
    *entriesInOut = 0;
    return 0;
  }

  added = 0;
  ITERATELIST(LINKUNIQUE, ((CModelComplex *)unique)->m_attached[index], link) {
    if (added == *entriesInOut) {
      break;
    }
    *modelList++ = (HMODEL)HandleDuplicate(link->child);
    ++added;
  }

  *entriesInOut = added;
  return 1;
}

BOOL ModelGetNumLinkedAtPoint(HMODEL model, UINT index, UINT *numLinked) {
  CModelShared *shared;
  CModelBase   *unique;

  VALIDATEBEGIN;
  VALIDATE(numLinked);
  VALIDATEEND;
  *numLinked = 0;

  if (!IModelDerefHandle((CModel *)model, &unique, &shared) || !(unique->m_flags & 0x20)) {
    return 0;
  }

  CModelShared *dataptr = shared;
  ASSERT(dataptr);
  if (index >= shared->attachIdToIndex.Count()) {
    return 0;
  }

  index = shared->attachIdToIndex[index];
  if (index == (UINT)-1) {
    return 0;
  }

  ITERATELIST(LINKUNIQUE, ((CModelComplex *)unique)->m_attached[index], link) {
    ++*numLinked;
  }

  return 1;
}

BOOL ModelHasLinkPoint(HMODEL model, UINT index) {
  CModelShared *shared;

  if (!IModelDerefHandle((CModel *)model, &shared)) {
    return 0;
  }

  CModelShared *dataptr = shared;
  ASSERT(dataptr);
  if (index < dataptr->attachIdToIndex.Count() ? dataptr->attachIdToIndex[index] != (UINT)-1 : 0) {
    return 1;
  }
  return 0;
}

UINT ModelGetNumLinkPoints(HMODEL model) {
  CModelBase *unique;

  if (IModelDerefHandle((CModel *)model, &unique) && (unique->m_flags & 0x20)) {
    return ((CModelComplex *)unique)->m_attached.Count();
  }

  return 0;
}

BOOL ModelAddLink(HMODEL parent, UINT parentIndex, HMODEL child, float scale) {
  CModelBase    *parentBase;
  CModelShared  *parentdata;
  CModelComplex *parentptr;

  CModel *modelptr = (CModel *)parent;
  VALIDATEBEGIN;
  VALIDATE(modelptr);
  VALIDATEEND;

  ASSERT(CMath::fnotequal_(scale,0.0f));
  ASSERT(((CModel *)((CHandleObject*)(child))));

  if (!IModelDerefHandle(modelptr, &parentBase, &parentdata)) {
    EnqueueModelCommand(modelptr, MODEL_ADD_LINK, parentIndex, child, scale);
    return 1;
  }

  if (!(parentBase->m_flags & 0x20)) {
    return 0;
  }

  parentptr = (CModelComplex *)parentBase;
  CModelShared *dataptr = parentdata;
  ASSERT(dataptr);

  if (parentIndex >= parentdata->attachIdToIndex.Count()) {
    return 0;
  }

  if (parentdata->attachIdToIndex[parentIndex] == (UINT)-1) {
    return 0;
  }

  LIST(LINKUNIQUE) &links = parentptr->m_attached[parentdata->attachIdToIndex[parentIndex]];
  LINKUNIQUE *link = links.NewNode(LIST_TAIL, 0, 0);
  link->child = (HMODEL)HandleDuplicate(child);
  link->scale = scale;

  ModelSetLightSelectCallback(link->child, parentptr->m_PickLights, parentptr->m_pickLightsParm, 1);
  return 1;
}

BOOL ModelRemoveLink(HMODEL parent, UINT parentIndex, HMODEL child) {
  CModelBase   *parentBase;
  CModelShared *parentdata;

  CModel *modelptr = (CModel *)parent;
  VALIDATEBEGIN;
  VALIDATE(modelptr);
  VALIDATEEND;

  if (!IModelDerefHandle(modelptr, &parentBase, &parentdata)) {
    EnqueueModelCommand(modelptr, MODEL_REMOVE_LINK, parentIndex, child);
    return 1;
  }

  if (!(parentBase->m_flags & 0x20)) {
    return 0;
  }

  CModelComplex *parentptr = (CModelComplex *)parentBase;
  CModelShared *dataptr = parentdata;
  ASSERT(dataptr);
  if (parentIndex >= parentdata->attachIdToIndex.Count()) {
    return 0;
  }

  parentIndex = parentdata->attachIdToIndex[parentIndex];
  if (parentIndex == (UINT)-1) {
    return 0;
  }

  CModel *childptr = (CModel *)child;
  ASSERT(childptr);
  LIST(LINKUNIQUE) &links = parentptr->m_attached[parentIndex];
  ITERATELIST(LINKUNIQUE, links, link) {
    if (link->child == child) {
      ITERATE_DELETEANDBREAK;
    }
  }

  return 1;
}

BOOL ModelClearLink(HMODEL parent, UINT parentIndex) {
  CModelBase    *parentBase;
  CModelShared  *parentdata;
  CModelComplex *parentptr;

  CModel *modelptr = (CModel *)parent;
  VALIDATEBEGIN;
  VALIDATE(modelptr);
  VALIDATEEND;

  if (!IModelDerefHandle(modelptr, &parentBase, &parentdata)) {
    EnqueueModelCommand(modelptr, MODEL_CLEAR_LINK, parentIndex);
    return 1;
  }

  if (!(parentBase->m_flags & 0x20)) {
    return 0;
  }

  parentptr = (CModelComplex *)parentBase;
  CModelShared *dataptr = parentdata;
  ASSERT(dataptr);

  if (parentIndex >= parentdata->attachIdToIndex.Count()) {
    return 0;
  }

  parentIndex = parentdata->attachIdToIndex[parentIndex];
  if (parentIndex == (UINT)-1) {
    return 0;
  }

  parentptr->m_attached[parentIndex].Clear();
  return 1;
}

void ModelClearAllLinks(HMODEL parent) {
  CModelBase *parentBase;
  UINT        numAttachments;
  UINT        i;

  CModel *modelptr = (CModel *)parent;
  VALIDATEBEGIN;
  VALIDATE(modelptr);
  VALIDATEENDVOID;

  if (!IModelDerefHandle(modelptr, &parentBase)) {
    EnqueueModelCommand(modelptr, MODEL_CLEAR_ALL_LINKS);
    return;
  }

  if (!(parentBase->m_flags & 0x20)) {
    return;
  }

  CModelComplex *parentptr = (CModelComplex *)parentBase;
  numAttachments = parentptr->m_attached.Count();
  for (i = 0; i < numAttachments; ++i) {
    parentptr->m_attached[i].Clear();
  }
}

UINT ModelGetNumCameras(HMODEL model) {
  CModelBase *unique;

  if (IModelDerefHandle((CModel *)model, &unique) && (unique->m_flags & 0x20)) {
    return ((CModelComplex *)unique)->m_cameras.Count();
  }

  return 0;
}

HCAMERA ModelGetCamera(HMODEL model, UINT index) {
  CModelBase *unique;

  if (!IModelDerefHandle((CModel *)model, &unique) || !(unique->m_flags & 0x20)) {
    return 0;
  }

  CModelComplex *complex = (CModelComplex *)unique;
  if (complex->m_cameraOrder.Count()) {
    if (index >= complex->m_cameraOrder.Count()) {
      return 0;
    }

    index = complex->m_cameraOrder[index];
    if (index == (UINT)-1) {
      return 0;
    }
  }

  VALIDATEBEGIN;
  VALIDATE(index < complex->m_cameras.Count());
  VALIDATEEND;

  return (HCAMERA)HandleDuplicate(complex->m_cameras[index]);
}

int ModelIsCameraEnabled(HMODEL model, UINT index) {
  CModelBase *unique;

  if (!IModelDerefHandle((CModel *)model, &unique) || !(unique->m_flags & 0x20)) {
    return 0;
  }

  CModelComplex *complex = (CModelComplex *)unique;
  if (complex->m_cameraOrder.Count()) {
    if (index >= complex->m_cameraOrder.Count()) {
      return 0;
    }

    index = complex->m_cameraOrder[index];
    if (index == (UINT)-1) {
      return 0;
    }
  }

  VALIDATEBEGIN;
  VALIDATE(index < complex->m_cameras.Count());
  VALIDATEEND;
  return AnimIsCameraEnabled(unique->m_anim, index);
}

BOOL ModelIsShowingBoundingSphere(HMODEL model) {
  CModelBase *unique;

  return IModelDerefHandle((CModel *)model, &unique) && unique->m_boundsModel && (unique->m_flags & 1);
}

BOOL ModelIsShowingBoundingBox(HMODEL model) {
  CModelBase *unique;

  return IModelDerefHandle((CModel *)model, &unique) && unique->m_boundsModel && !(unique->m_flags & 1);
}

BOOL ModelIsShowingHitTestGeometry(HMODEL model) {
  CModelBase *unique;

  return IModelDerefHandle((CModel *)model, &unique) && (unique->m_flags & 8);
}

static void ComplexModelRestoreBlendMode(CModelComplex *unique, int doLinkedModels) {
  UINT numAttachments;

  ASSERT(unique);

  UINT numMaterials = unique->m_materials.Count();
  for (UINT i = 0; i < numMaterials; ++i) {
    CMaterial *uniqueMtl = (CMaterial *)unique->m_materials[i];
    ASSERT(uniqueMtl);

    CMaterialShared *sharedMtl = (CMaterialShared *)uniqueMtl->data;
    ASSERT(sharedMtl);

    if (!MaterialUsedOnce(unique->m_materials[i])) {
      HMATERIAL oldMaterial = unique->m_materials[i];
      unique->m_materials[i] = MaterialDuplicate(unique->m_materials[i]);
      HandleClose(oldMaterial);
      uniqueMtl = (CMaterial *)unique->m_materials[i];
      ASSERT(uniqueMtl);
    }

    UINT             numLayers = sharedMtl->layers.Count();
    CTexLayer       *uniqueLayer = uniqueMtl->layers.Ptr();
    CTexLayerShared *sharedLayer = sharedMtl->layers.Ptr();
    for (; numLayers; --numLayers, ++uniqueLayer, ++sharedLayer) {
      uniqueLayer->blendMode = sharedLayer->blendMode;
    }
  }

  if (doLinkedModels) {
    numAttachments = unique->m_attached.Count();
    for (UINT i = 0; i < numAttachments; ++i) {
      ITERATELIST(LINKUNIQUE, unique->m_attached[i], link) {
        ModelRestoreBlendMode(link->child, 1);
      }
    }
  }
}

void ModelRestoreBlendMode(HMODEL model, int doLinkedModels) {
  CModelBase *unique;

  if (!IModelDerefHandle((CModel *)model, &unique)) {
    return;
  }

  if (unique->m_flags & 0x20) {
    ComplexModelRestoreBlendMode((CModelComplex *)unique, doLinkedModels);
  } else {
    CModelSimple *simple = (CModelSimple *)unique;
    UINT numMaterials = simple->m_materials.Count();
    for (UINT i = 0; i < numMaterials; ++i) {
      CMaterial *uniqueMtl = (CMaterial *)simple->m_materials[i];
      ASSERT(uniqueMtl);

      CMaterialShared *sharedMtl = (CMaterialShared *)uniqueMtl->data;
      ASSERT(sharedMtl);

      if (!MaterialUsedOnce(simple->m_materials[i])) {
        HMATERIAL oldMaterial = simple->m_materials[i];
        simple->m_materials[i] = MaterialDuplicate(simple->m_materials[i]);
        HandleClose(oldMaterial);
        uniqueMtl = (CMaterial *)simple->m_materials[i];
        ASSERT(uniqueMtl);
      }

      UINT             numLayers = sharedMtl->layers.Count();
      CTexLayer       *uniqueLayer = uniqueMtl->layers.Ptr();
      CTexLayerShared *sharedLayer = sharedMtl->layers.Ptr();
      for (; numLayers; --numLayers, ++uniqueLayer, ++sharedLayer) {
        uniqueLayer->blendMode = sharedLayer->blendMode;
      }
    }
  }
}

static void ComplexModelSetBlendMode(CModelComplex *unique, EGxBlend blendMode, int doLinkedModels) {
  UINT numAttachments;

  ASSERT(unique);

  UINT numMaterials = unique->m_materials.Count();
  for (UINT i = 0; i < numMaterials; ++i) {
    CMaterial *uniqueMtl = (CMaterial *)unique->m_materials[i];
    ASSERT(uniqueMtl);

    CMaterialShared *sharedMtl = (CMaterialShared *)uniqueMtl->data;
    ASSERT(sharedMtl);

    if (!MaterialUsedOnce(unique->m_materials[i])) {
      HMATERIAL oldMaterial = unique->m_materials[i];
      unique->m_materials[i] = MaterialDuplicate(unique->m_materials[i]);
      HandleClose(oldMaterial);
      uniqueMtl = (CMaterial *)unique->m_materials[i];
      ASSERT(uniqueMtl);
    }

    UINT             numLayers = sharedMtl->layers.Count();
    CTexLayer       *uniqueLayer = uniqueMtl->layers.Ptr();
    CTexLayerShared *sharedLayer = sharedMtl->layers.Ptr();
    for (; numLayers; --numLayers, ++uniqueLayer, ++sharedLayer) {
      uniqueLayer->blendMode = blendMode;
    }
  }

  if (doLinkedModels) {
    numAttachments = unique->m_attached.Count();
    for (UINT i = 0; i < numAttachments; ++i) {
      ITERATELIST(LINKUNIQUE, unique->m_attached[i], link) {
        ModelSetBlendMode(link->child, blendMode, 1);
      }
    }
  }
}

void ModelSetBlendMode(HMODEL model, EGxBlend blendMode, int doLinkedModels) {
  CModelBase *unique;

  if (!IModelDerefHandle((CModel *)model, &unique)) {
    return;
  }

  if (unique->m_flags & 0x20) {
    ComplexModelSetBlendMode((CModelComplex *)unique, blendMode, doLinkedModels);
  } else {
    CModelSimple *simple = (CModelSimple *)unique;
    UINT numMaterials = simple->m_materials.Count();
    for (UINT i = 0; i < numMaterials; ++i) {
      CMaterial *uniqueMtl = (CMaterial *)simple->m_materials[i];
      ASSERT(uniqueMtl);

      CMaterialShared *sharedMtl = (CMaterialShared *)uniqueMtl->data;
      ASSERT(sharedMtl);

      if (!MaterialUsedOnce(simple->m_materials[i])) {
        HMATERIAL oldMaterial = simple->m_materials[i];
        simple->m_materials[i] = MaterialDuplicate(simple->m_materials[i]);
        HandleClose(oldMaterial);
        uniqueMtl = (CMaterial *)simple->m_materials[i];
        ASSERT(uniqueMtl);
      }

      UINT             numLayers = sharedMtl->layers.Count();
      CTexLayer       *uniqueLayer = uniqueMtl->layers.Ptr();
      CTexLayerShared *sharedLayer = sharedMtl->layers.Ptr();
      for (; numLayers; --numLayers, ++uniqueLayer, ++sharedLayer) {
        uniqueLayer->blendMode = blendMode;
      }
    }
  }
}

template <class T>
inline void IModelHideGeosets(T *modelptr, CModelShared *shared, UINT selectionGroup, int hide) {
  ASSERT(shared);
  for (UINT index = 0; index < shared->numGeosets; ++index) {
    if (shared->geosets[index].selectionGroup == selectionGroup) {
      if (hide) {
        modelptr->m_geosets[index].flags |= 1;
      } else {
        modelptr->m_geosets[index].flags &= ~1u;
      }
    }
  }
}

void ModelHideGeosets(HMODEL model, UINT selectionGroup, int hide) {
  CModel *modelptr = (CModel *)model;
  VALIDATEBEGIN;
  VALIDATE(modelptr);
  VALIDATEENDVOID;

  CModelBase   *unique;
  CModelShared *shared;
  if (!IModelDerefHandle(modelptr, &unique, &shared)) {
    EnqueueModelCommand(modelptr, MODEL_HIDE_GEOSETS, selectionGroup, hide);
    return;
  }

  if (unique->m_flags & 0x20) {
    CModelComplex *complex = (CModelComplex *)unique;
    IModelHideGeosets(complex, shared, selectionGroup, hide);
  } else {
    CModelSimple *simple = (CModelSimple *)unique;
    IModelHideGeosets(simple, shared, selectionGroup, hide);
  }
}

template <class T>
inline void IModelHideGeosetsRange(T *modelptr, CModelShared *shared, UINT selectionStart, UINT selectionEnd, int hide) {
  for (UINT index = 0; index < shared->numGeosets; ++index) {
    if (shared->geosets[index].selectionGroup >= selectionStart && shared->geosets[index].selectionGroup <= selectionEnd) {
      if (hide) {
        modelptr->m_geosets[index].flags |= 1;
      } else {
        modelptr->m_geosets[index].flags &= ~1u;
      }
    }
  }
}

void ModelHideGeosetsRange(HMODEL model, UINT selectionStart, UINT selectionEnd, int hide) {
  CModel *modelptr = (CModel *)model;
  VALIDATEBEGIN;
  VALIDATE(modelptr);
  VALIDATEENDVOID;

  CModelBase   *unique;
  CModelShared *shared;
  if (!IModelDerefHandle(modelptr, &unique, &shared)) {
    EnqueueModelCommand(modelptr, MODEL_HIDE_GEOSETS_RANGE, selectionStart, selectionEnd, hide);
    return;
  }

  if (unique->m_flags & 0x20) {
    CModelComplex *complex = (CModelComplex *)unique;
    IModelHideGeosetsRange(complex, shared, selectionStart, selectionEnd, hide);
  } else {
    CModelSimple *simple = (CModelSimple *)unique;
    IModelHideGeosetsRange(simple, shared, selectionStart, selectionEnd, hide);
  }
}

UINT CMatrixGroupTree::AddNode(UINT *matrixGroup, UINT numMatrices) {
  UINT          index = nodes.Count();
  CMatrixGroup *node = nodes.New();
  node->matrices = matrixGroup;
  node->numMatrices = numMatrices;
  node->index = index;
  this->numMatrices += numMatrices;
  return index;
}

UINT CMatrixGroupTree::Insert(UINT *matrixGroup, UINT numMatrices) {
  if (!nodes.Count()) {
    return AddNode(matrixGroup, numMatrices);
  }

  UINT parentIndex = 0;
  while (1) {
    ASSERT(parentIndex != 0xffffffff);
    if (GroupsEqual(nodes[parentIndex].matrices, nodes[parentIndex].numMatrices, matrixGroup, numMatrices)) {
      return nodes[parentIndex].index;
    }

    if (GroupLessThan(matrixGroup, numMatrices, nodes[parentIndex].matrices, nodes[parentIndex].numMatrices)) {
      if (nodes[parentIndex].leftIndex == (UINT)-1) {
        UINT childIndex = AddNode(matrixGroup, numMatrices);
        nodes[parentIndex].leftIndex = childIndex;
        return childIndex;
      }
      parentIndex = nodes[parentIndex].leftIndex;
    } else {
      if (nodes[parentIndex].rightIndex == (UINT)-1) {
        UINT childIndex = AddNode(matrixGroup, numMatrices);
        nodes[parentIndex].rightIndex = childIndex;
        return childIndex;
      }
      parentIndex = nodes[parentIndex].rightIndex;
    }
  }
}

BOOL CMatrixGroupTree::GroupsEqual(const UINT *matrixGroup1, UINT numMatrices1, const UINT *matrixGroup2, UINT numMatrices2) {
  UINT count = numMatrices1 < numMatrices2 ? numMatrices1 : numMatrices2;
  int  compare = memcmp(matrixGroup1, matrixGroup2, count * sizeof(UINT));
  if (compare) {
    return 0;
  }
  return numMatrices1 == numMatrices2;
}

int CMatrixGroupTree::GroupLessThan(const UINT *matrixGroup1, UINT numMatrices1, const UINT *matrixGroup2, UINT numMatrices2) {
  UINT count = numMatrices1 < numMatrices2 ? numMatrices1 : numMatrices2;
  int  compare = memcmp(matrixGroup1, matrixGroup2, count * sizeof(UINT));
  if (!compare) {
    return numMatrices1 < numMatrices2;
  }
  return compare < 0;
}

static void AddMatrixGroupRangeToSet(
    CMatrixGroupTree *matrixGroupSets,
    CGeosetShared    *srcGeoset,
    const UINT       *matrixOffsets,
    UINT              start,
    UINT              count,
    CGeosetShared    *dstGeoset,
    BYTE             *groupIdConvert
) {
  UINT  middle = start + count / 2;
  UINT  matrixOffset = matrixOffsets[middle];
  UINT *matrices = &srcGeoset->matrices[matrixOffset];
  UINT  numMatrices = srcGeoset->groupMatrixCounts[middle];
  UINT  groupCount = matrixGroupSets->GroupCount();
  UINT  index = matrixGroupSets->Insert(matrices, numMatrices);
  groupIdConvert[middle] = index;

  if (matrixGroupSets->GroupCount() > groupCount) {
    matrixOffset = matrixGroupSets->MatrixCount() - numMatrices;
    dstGeoset->groupMatrixCounts[index] = numMatrices;
    memcpy(&dstGeoset->matrices[matrixOffset], matrices, numMatrices * sizeof(UINT));
  }

  if (middle - start) {
    AddMatrixGroupRangeToSet(matrixGroupSets, srcGeoset, matrixOffsets, start, middle - start, dstGeoset, groupIdConvert);
  }
  if (start + count - middle - 1 > 0) {
    AddMatrixGroupRangeToSet(matrixGroupSets, srcGeoset, matrixOffsets, middle + 1, start + count - middle - 1, dstGeoset, groupIdConvert);
  }
}

static void AddGeosetMatrixGroups(CMatrixGroupTree *matrixGroupSets, CGeosetShared *srcGeoset, CGeosetShared *dstGeoset, UINT vertsAdded) {
  UINT               numIndices = srcGeoset->primitiveVertices.Count();
  TSStackArray<BYTE> groupIdConvert(_alloca(numIndices), numIndices, numIndices);
  UINT               numGroups = srcGeoset->groupMatrixCounts.Count();
  UINT              *matrixOffsets = (UINT *)_alloca(numGroups * sizeof(UINT));
  UINT               total = 0;
  UINT               index;

  for (index = 0; index < numGroups; ++index) {
    matrixOffsets[index] = total;
    total += srcGeoset->groupMatrixCounts[index];
  }
  AddMatrixGroupRangeToSet(matrixGroupSets, srcGeoset, matrixOffsets, 0, numGroups, dstGeoset, groupIdConvert.Ptr());

  UINT numBoneWeights = srcGeoset->boneWeights.Count();
  for (index = 0; index < numBoneWeights; ++index) {
    UINT groupId = srcGeoset->boneWeights[index];
    dstGeoset->boneWeights[vertsAdded + index] = groupIdConvert[groupId];
  }
}

static void BuildCompositeGeoset(CGeosetShared *newGeoset, UINT geosetId, CGeosetShared *geosets, const TSGrowableArray<UINT> &geosetIds) {
  UINT i;

  newGeoset->vertexShader = geosets[geosetIds[0]].vertexShader;
  newGeoset->materialId = geosets[geosetIds[0]].materialId;
  newGeoset->geosetId = geosetId;

  UINT numTexChannels = geosets[geosetIds[0]].texCoord.Count();
  UINT numVertices = 0;
  UINT numIndices = 0;
  UINT numGroups = 0;
  UINT numMatrices = 0;
  UINT numGeosets = geosetIds.Count();

  for (i = 0; i < numGeosets; ++i) {
    numVertices += geosets[geosetIds[i]].position.Count();
    numIndices += geosets[geosetIds[i]].primitiveVertices.Count();
    numGroups += geosets[geosetIds[i]].groupMatrixCounts.Count();
    numMatrices += geosets[geosetIds[i]].matrices.Count();
    if (geosets[geosetIds[i]].vertexShader > newGeoset->vertexShader) {
      newGeoset->vertexShader = geosets[geosetIds[i]].vertexShader;
    }
    ASSERT(geosets[geosetIds[i]].texCoord.Count() == numTexChannels);
  }

  newGeoset->position.SetCount(numVertices);
  newGeoset->boneWeights.SetCount(numVertices);
  newGeoset->normal.SetCount(numVertices);
  newGeoset->texCoord.SetCount(numTexChannels);
  for (i = 0; i < numTexChannels; ++i) {
    newGeoset->texCoord[i].SetCount(numVertices);
  }
  newGeoset->primitive.SetCount(1);
  newGeoset->primitiveVertices.SetCount(numIndices);
  newGeoset->groupMatrixCounts.SetCount(numGroups);
  newGeoset->matrices.SetCount(numMatrices);

  CMatrixGroupTree matrixGroupSets;
  newGeoset->centroid.x = 0.0f;
  newGeoset->centroid.y = 0.0f;
  newGeoset->centroid.z = 0.0f;
  numIndices = 0;
  numVertices = 0;
  for (i = 0; i < numGeosets; ++i) {
    memcpy(
        &newGeoset->position[numVertices], geosets[geosetIds[i]].position.Ptr(),
        geosets[geosetIds[i]].position.Count() * sizeof(C3Vector)
    );
    memcpy(
        &newGeoset->normal[numVertices], geosets[geosetIds[i]].normal.Ptr(),
        geosets[geosetIds[i]].normal.Count() * sizeof(C3Vector)
    );

    CModelTexCoordArray              *src = geosets[geosetIds[i]].texCoord.Ptr();
    TSFixedArray<NTempest::C2Vector> *dstTexLayer = newGeoset->texCoord.Ptr();
    for (UINT texChannel = numTexChannels; texChannel; --texChannel, ++src, ++dstTexLayer) {
      memcpy(&(*dstTexLayer)[numVertices], src->Ptr(), src->Count() * sizeof(C2Vector));
    }

    ASSERT(geosets[geosetIds[i]].primitive.Count() == 1);
    ASSERT(geosets[geosetIds[i]].primitive[0].type == GxPrim_Triangles);
    ASSERT(geosets[geosetIds[i]].primitive[0].vertexCount == geosets[geosetIds[i]].primitiveVertices.Count());
    newGeoset->primitive[0].vertexCount += geosets[geosetIds[i]].primitive[0].vertexCount;

    WORD *srcIndex = geosets[geosetIds[i]].primitiveVertices.Ptr();
    WORD *dest = &newGeoset->primitiveVertices[numIndices];
    for (UINT primitiveIndex = geosets[geosetIds[i]].primitiveVertices.Count(); primitiveIndex; --primitiveIndex) {
      *dest++ = *srcIndex++ + numVertices;
    }

    AddGeosetMatrixGroups(&matrixGroupSets, &geosets[geosetIds[i]], newGeoset, numVertices);

    newGeoset->centroid += geosets[geosetIds[i]].centroid;
    numVertices += geosets[geosetIds[i]].position.Count();
    numIndices += geosets[geosetIds[i]].primitiveVertices.Count();
  }

  newGeoset->groupMatrixCounts.SetCount(matrixGroupSets.GroupCount());
  newGeoset->matrices.SetCount(matrixGroupSets.MatrixCount());
  newGeoset->centroid *= 1.0f / geosetIds.Count();
}

static void IModelOptimizeVisibleGeosets(CModelComplex *unique, CModelShared *shared) {
  unique->m_geosets.SetCount(shared->numGeosets);
  unique->m_geosetColor.SetCount(shared->numGeosets);
  unique->m_addlGeosets.SetCount(0);

  TSStackArray<TSGrowableArray<UINT> > geosetIDsPerMaterial(
      _alloca(unique->m_materials.Count() * sizeof(TSGrowableArray<UINT>)), unique->m_materials.Count(), unique->m_materials.Count()
  );

  UINT i;
  UINT numGeosets = unique->m_geosets.Count();
  for (i = 0; i < numGeosets; ++i) {
    if (!(unique->m_geosets[i].flags & 1)) {
      geosetIDsPerMaterial[shared->geosets[i].materialId].New(i);
    }
  }

  UINT numBatches = geosetIDsPerMaterial.Count();
  UINT geosetId = unique->m_geosets.Count();
  UINT batchesToBuild = 0;
  for (i = 0; i < numBatches; ++i) {
    if (geosetIDsPerMaterial[i].Count() > 1) {
      ++batchesToBuild;
    }
  }

  unique->m_geosetColor.SetCount(geosetId + batchesToBuild);
  unique->m_geosets.SetCount(geosetId + batchesToBuild);
  unique->m_addlGeosets.SetCount(geosetId + batchesToBuild - shared->numGeosets);

  for (i = 0; i < numBatches; ++i) {
    if (geosetIDsPerMaterial[i].Count() > 1) {
      BuildCompositeGeoset(&unique->m_addlGeosets[geosetId - shared->numGeosets], geosetId, shared->geosets.Ptr(), geosetIDsPerMaterial[i]);
      ++geosetId;

      UINT geosInBatch = geosetIDsPerMaterial[i].Count();
      for (UINT j = 0; j < geosInBatch; ++j) {
        unique->m_geosets[geosetIDsPerMaterial[i][j]].flags |= 1;
      }
    }
  }
}

BOOL ModelOptimizeVisibleGeosets(HMODEL model) {
  CModel *modelptr = (CModel *)model;
  VALIDATEBEGIN;
  VALIDATE(modelptr);
  VALIDATEEND;

  CModelBase   *unique;
  CModelShared *shared;
  if (!IModelDerefHandle(modelptr, &unique, &shared)) {
    EnqueueModelCommand(modelptr, MODEL_OPTIMIZE_VISIBLE_GEOSETS);
    return 1;
  }

  if (!(unique->m_flags & 0x20)) {
    return 0;
  }

  IModelOptimizeVisibleGeosets((CModelComplex *)unique, shared);
  return 1;
}

BYTE ModelGetVertexAlpha(HMODEL model) {
  CModelBase *unique;

  if (!IModelDerefHandle((CModel *)model, &unique)) {
    return 0xFF;
  }

  if (unique->m_flags & 0x20) {
    CModelComplex *complex = (CModelComplex *)unique;
    if (!complex->m_geosetColor.Count()) {
      return 0xFF;
    }

    return NTempest::CMath::fuint_(complex->m_geosetColor[0].proceduralAlpha * 255.0f);
  }

  CModelSimple *simple = (CModelSimple *)unique;
  if (!simple->m_geosetColor.Count()) {
    return 0xFF;
  }

  return NTempest::CMath::fuint_(simple->m_geosetColor[0].proceduralAlpha * 255.0f);
}

static void GeosetSetVertexAlpha(CGeosetShared *geoShared, CGeosetColor *geoColor, HMATERIAL *materials, float alpha) {
  ASSERT(geoShared);

  if (NTempest::CMath::fabs_(geoColor->proceduralAlpha - alpha) < 0.00000095367432f) {
    return;
  }

  geoColor->proceduralAlpha = alpha;
  geoColor->animatedColor.a = NTempest::CMath::ftol_0_256_(alpha * geoColor->animatedAlpha * 255.0f);

  CMaterial *uniqueMtl = (CMaterial *)materials[geoShared->materialId];
  ASSERT(uniqueMtl);

  CMaterialShared *sharedMtl = (CMaterialShared *)uniqueMtl->data;
  ASSERT(sharedMtl);

  if (!MaterialUsedOnce((HMATERIAL)uniqueMtl)) {
    HMATERIAL oldMaterial = (HMATERIAL)uniqueMtl;
    uniqueMtl = (CMaterial *)MaterialDuplicate(oldMaterial);
    HandleClose(oldMaterial);
    materials[geoShared->materialId] = (HMATERIAL)uniqueMtl;
    ASSERT(uniqueMtl);
  }

  if (geoColor->animatedColor.a && geoColor->animatedColor.a != 255) {
    CTexLayer *uniqueLayer = uniqueMtl->layers.Ptr();
    UINT       numLayers = uniqueMtl->layers.Count();
    for (UINT i = 0; i < numLayers; ++i) {
      if (uniqueLayer[i].blendMode < GxBlend_Alpha) {
        uniqueLayer[i].blendMode = GxBlend_Alpha;
      }
    }
  } else {
    CTexLayer       *uniqueLayer = uniqueMtl->layers.Ptr();
    UINT             numLayers = uniqueMtl->layers.Count();
    CTexLayerShared *sharedLayer = sharedMtl->layers.Ptr();
    for (; numLayers; --numLayers, ++uniqueLayer, ++sharedLayer) {
      uniqueLayer->blendMode = sharedLayer->blendMode;
    }
  }
}

static void IModelSetVertexAlpha(CModelSimple *unique, CModelShared *shared, BYTE alpha) {
  ASSERT(unique);

  float fAlpha = (float)alpha * 0.0039215689f;
  UINT  numGeosets = unique->m_geosets.Count();
  for (UINT i = 0; i < numGeosets; ++i) {
    GeosetSetVertexAlpha(&shared->geosets[i], &unique->m_geosetColor[i], unique->m_materials.Ptr(), fAlpha);
  }
}

static void IModelSetVertexAlpha(CModelComplex *unique, CModelShared *shared, BYTE alpha) {
  ASSERT(unique);

  float fAlpha = (float)alpha * 0.0039215689f;
  for (UINT i = 0; i < shared->numGeosets; ++i) {
    GeosetSetVertexAlpha(&shared->geosets[i], &unique->m_geosetColor[i], unique->m_materials.Ptr(), fAlpha);
  }

  UINT numAddlGeosets = unique->m_geosets.Count() - shared->numGeosets;
  for (UINT j = 0; j < numAddlGeosets; ++j) {
    GeosetSetVertexAlpha(&unique->m_addlGeosets[j], &unique->m_geosetColor[j + shared->numGeosets], unique->m_materials.Ptr(), fAlpha);
  }
}

void ModelSetVertexAlpha(HMODEL model, BYTE alpha, int doLinkedModels) {
  CModelBase   *unique;
  CModelShared *shared;

  CModel *modelptr = (CModel *)model;
  VALIDATEBEGIN;
  VALIDATE(modelptr);
  VALIDATEENDVOID;

  if (!IModelDerefHandle(modelptr, &unique, &shared)) {
    EnqueueModelCommand(modelptr, MODEL_SET_VERTEX_ALPHA, alpha, doLinkedModels);
    return;
  }

  if (!(unique->m_flags & 0x20)) {
    IModelSetVertexAlpha((CModelSimple *)unique, shared, alpha);
    return;
  }

  CModelComplex *complex = (CModelComplex *)unique;
  IModelSetVertexAlpha(complex, shared, alpha);

  if (!doLinkedModels) {
    return;
  }

  UINT numAttachments = complex->m_attached.Count();
  for (UINT i = 0; i < numAttachments; ++i) {
    ITERATELIST(LINKUNIQUE, complex->m_attached[i], link) {
      ModelSetVertexAlpha(link->child, alpha, 1);
    }
  }
}

void ModelGetVertexColor(HMODEL model, BYTE &red, BYTE &green, BYTE &blue) {
  CModelBase *unique;

  red = 0;
  green = 0;
  blue = 0;

  if (!IModelDerefHandle((CModel *)model, &unique)) {
    return;
  }

  if (unique->m_flags & 0x20) {
    CModelComplex *complex = (CModelComplex *)unique;
    if (!complex->m_geosetColor.Count()) {
      return;
    }

    red = complex->m_geosetColor[0].proceduralColor.r;
    green = complex->m_geosetColor[0].proceduralColor.g;
    blue = complex->m_geosetColor[0].proceduralColor.b;
    return;
  }

  CModelSimple *simple = (CModelSimple *)unique;
  if (!simple->m_geosetColor.Count()) {
    return;
  }

  red = simple->m_geosetColor[0].proceduralColor.r;
  green = simple->m_geosetColor[0].proceduralColor.g;
  blue = simple->m_geosetColor[0].proceduralColor.b;
}

static void GeosetSetVertexColor(CGeosetShared *geoShared, CGeosetColor *geoColor, HMATERIAL *materials, BYTE red, BYTE green, BYTE blue) {
  ASSERT(geoShared);

  HMATERIAL material = materials[geoShared->materialId];
  if (!MaterialUsedOnce(material)) {
    HMATERIAL duplicate = MaterialDuplicate(material);
    HandleClose(material);
    materials[geoShared->materialId] = duplicate;
  }

  geoColor->proceduralColor.r = red;
  geoColor->proceduralColor.g = green;
  geoColor->proceduralColor.b = blue;
}

static void IModelSetVertexColor(CModelSimple *unique, CModelShared *shared, BYTE red, BYTE green, BYTE blue) {
  UINT numGeosets = unique->m_geosets.Count();
  for (UINT i = 0; i < numGeosets; ++i) {
    GeosetSetVertexColor(&shared->geosets[i], &unique->m_geosetColor[i], unique->m_materials.Ptr(), red, green, blue);
  }
}

static void IModelSetVertexColor(CModelComplex *unique, CModelShared *shared, BYTE red, BYTE green, BYTE blue) {
  for (UINT i = 0; i < shared->numGeosets; ++i) {
    GeosetSetVertexColor(&shared->geosets[i], &unique->m_geosetColor[i], unique->m_materials.Ptr(), red, green, blue);
  }

  UINT numAddlGeosets = unique->m_geosets.Count() - shared->numGeosets;
  for (UINT j = 0; j < numAddlGeosets; ++j) {
    GeosetSetVertexColor(&unique->m_addlGeosets[j], &unique->m_geosetColor[j + shared->numGeosets], unique->m_materials.Ptr(), red, green, blue);
  }
}

void ModelSetVertexColor(HMODEL model, BYTE red, BYTE green, BYTE blue, int doLinkedModels) {
  CModelBase   *unique;
  CModelShared *shared;

  CModel *modelptr = (CModel *)model;
  VALIDATEBEGIN;
  VALIDATE(modelptr);
  VALIDATEENDVOID;

  if (!IModelDerefHandle(modelptr, &unique, &shared)) {
    EnqueueModelCommand(modelptr, MODEL_SET_VERTEX_COLOR, red, green, blue, doLinkedModels);
    return;
  }

  if (!(unique->m_flags & 0x20)) {
    IModelSetVertexColor((CModelSimple *)unique, shared, red, green, blue);
    return;
  }

  CModelComplex *complex = (CModelComplex *)unique;
  IModelSetVertexColor(complex, shared, red, green, blue);

  if (!doLinkedModels) {
    return;
  }

  UINT numAttachments = complex->m_attached.Count();
  for (UINT i = 0; i < numAttachments; ++i) {
    ITERATELIST(LINKUNIQUE, complex->m_attached[i], link) {
      ModelSetVertexColor(link->child, red, green, blue, 1);
    }
  }
}

static void GeosetShowUnselectable(CGeosetShared *geoShared, CGeosetColor *geoColor, HMATERIAL *materials, BYTE red, BYTE green, BYTE blue) {
  ASSERT(geoShared);

  HMATERIAL material = materials[geoShared->materialId];
  if ((geoShared->flags & 3) == 1) {
    geoShared->flags |= 2;
    if (!MaterialUsedOnce(material)) {
      HMATERIAL duplicate = MaterialDuplicate(material);
      HandleClose(material);
      materials[geoShared->materialId] = duplicate;
    }

    geoColor->proceduralColor.r = red;
    geoColor->proceduralColor.g = green;
    geoColor->proceduralColor.b = blue;
  }
}

template <class T>
inline void IModelShowUnselectable(T *modelptr, CModelShared *shared, BYTE red, BYTE green, BYTE blue) {
  for (UINT i = 0; i < shared->numGeosets; ++i) {
    GeosetShowUnselectable(&shared->geosets[i], &modelptr->m_geosetColor[i], modelptr->m_materials.Ptr(), red, green, blue);
  }
}

void ModelShowUnselectable(HMODEL model, BYTE red, BYTE green, BYTE blue) {
  CModelBase   *unique;
  CModelShared *shared;
  UINT          i;
  if (!IModelDerefHandle((CModel *)model, &unique, &shared)) {
    return;
  }

  if (!(unique->m_flags & 0x20)) {
    IModelShowUnselectable((CModelSimple *)unique, shared, red, green, blue);
  } else {
    CModelComplex *complex = (CModelComplex *)unique;
    IModelShowUnselectable(complex, shared, red, green, blue);

    UINT numAttachments = complex->m_attached.Count();
    for (i = 0; i < numAttachments; ++i) {
      ITERATELIST(LINKUNIQUE, complex->m_attached[i], link) {
        ModelShowUnselectable(link->child, red, green, blue);
      }
    }
  }
}

static void GeosetHideUnselectable(CGeosetShared *geoShared, CGeosetColor *geoColor, HMATERIAL *materials) {
  ASSERT(geoShared);

  HMATERIAL material = materials[geoShared->materialId];
  if ((geoShared->flags & 3) == 3) {
    geoShared->flags &= ~2U;
    if (!MaterialUsedOnce(material)) {
      HMATERIAL duplicate = MaterialDuplicate(material);
      HandleClose(material);
      materials[geoShared->materialId] = duplicate;
    }

    geoColor->proceduralColor.r = 0xFF;
    geoColor->proceduralColor.g = 0xFF;
    geoColor->proceduralColor.b = 0xFF;
  }
}

template <class T>
inline void IModelHideUnselectable(T *modelptr, CModelShared *shared) {
  ASSERT(shared);
  for (UINT i = 0; i < shared->numGeosets; ++i) {
    GeosetHideUnselectable(&shared->geosets[i], &modelptr->m_geosetColor[i], modelptr->m_materials.Ptr());
  }
}

void ModelHideUnselectable(HMODEL model) {
  CModelBase   *unique;
  CModelShared *shared;
  UINT          i;
  if (!IModelDerefHandle((CModel *)model, &unique, &shared)) {
    return;
  }

  if (!(unique->m_flags & 0x20)) {
    IModelHideUnselectable((CModelSimple *)unique, shared);
  } else {
    CModelComplex *complex = (CModelComplex *)unique;
    IModelHideUnselectable(complex, shared);

    UINT numAttachments = complex->m_attached.Count();
    for (i = 0; i < numAttachments; ++i) {
      ITERATELIST(LINKUNIQUE, complex->m_attached[i], link) {
        ModelHideUnselectable(link->child);
      }
    }
  }
}

static BOOL GeosetIsShowingUnselectable(CGeosetShared *geosets, UINT numGeosets) {
  ASSERT(geosets);

  for (UINT i = 0; i < numGeosets; ++i) {
    if ((geosets[i].flags & 3) == 3) {
      return 1;
    }
  }
  return 0;
}

BOOL ModelIsShowingUnselectable(HMODEL model) {
  CModelBase   *unique;
  CModelShared *shared;
  if (!IModelDerefHandle((CModel *)model, &unique, &shared)) {
    return 0;
  }

  if (GeosetIsShowingUnselectable(shared->geosets.Ptr(), shared->geosets.Count())) {
    return 1;
  }

  if (unique->m_flags & 0x20) {
    CModelComplex *complex = (CModelComplex *)unique;
    UINT           numAttachments = complex->m_attached.Count();
    for (UINT i = 0; i < numAttachments; ++i) {
      ITERATELIST(LINKUNIQUE, complex->m_attached[i], link) {
        if (ModelIsShowingUnselectable(link->child)) {
          return 1;
        }
      }
    }
  }
  return 0;
}

void ModelEnableLights(HMODEL model, int enable) {
  CModelBase *pModel;

  if (!IModelDerefHandle((CModel *)model, &pModel)) {
    return;
  }

  if (!(pModel->m_flags & 0x20)) {
    return;
  }

  CModelComplex *complex = (CModelComplex *)pModel;
  UINT           count = complex->m_lights.Count();
  for (UINT i = 0; i < count; ++i) {
    GxuLightEnableSet(complex->m_lights[i], enable);
  }
}

UINT ModelGetNumLights(HMODEL model) {
  CModelBase *pModel;

  if (!IModelDerefHandle((CModel *)model, &pModel) || !(pModel->m_flags & 0x20)) {
    return 0;
  }

  return ((CModelComplex *)pModel)->m_lights.Count();
}

const CGxLight *ModelGetLight(HMODEL model, UINT index) {
  CModelBase *pModel;

  if (!IModelDerefHandle((CModel *)model, &pModel)) {
    return 0;
  }

  VALIDATEBEGIN;
  VALIDATE(pModel->m_flags & 0x00000020);
  VALIDATEEND;

  CModelComplex *complex = (CModelComplex *)pModel;
  return GxuLightLock(complex->m_lights[index]);
}

void ModelSetLightSelectCallback(
    HMODEL model,
    void (*callback)(LPVOID, NTempest::C3Vector, const NTempest::C3Vector &, UINT),
    LPVOID parm,
    int    doLinkedModels
) {
  CModelBase *unique;

  CModel *modelptr = (CModel *)model;
  VALIDATEBEGIN;
  VALIDATE(modelptr);
  VALIDATEENDVOID;

  if (!IModelDerefHandle(modelptr, &unique)) {
    EnqueueModelCommand(modelptr, MODEL_SET_LIGHT_SELECT_CALLBACK, callback, parm, doLinkedModels);
    return;
  }

  unique->m_PickLights = callback;
  unique->m_pickLightsParm = parm;

  if (!(unique->m_flags & 0x20) || !doLinkedModels) {
    return;
  }

  CModelComplex *complex = (CModelComplex *)unique;
  UINT           numAttachments = complex->m_attached.Count();
  for (UINT i = 0; i < numAttachments; ++i) {
    ITERATELIST(LINKUNIQUE, complex->m_attached[i], link) {
      ModelSetLightSelectCallback(link->child, callback, parm, 1);
    }
  }
}
void ModelCustGeosetAdd(
    HMODEL                    model,
    const NTempest::C3Vector &modelSpacePosition,
    void (*renderCallback)(HMODEL, const NTempest::C34Matrix &, LPVOID),
    LPVOID renderParam,
    UINT  *custGeosetId
) {
  VALIDATEBEGIN;
  VALIDATE(custGeosetId);
  VALIDATEENDVOID;

  CModelBase *unique;
  if (!IModelDerefHandle((CModel *)model, &unique)) {
    return;
  }

  CCustomGeoset *geoset;
  if (unique->m_flags & 0x20) {
    CModelComplex *complex = (CModelComplex *)unique;
    *custGeosetId = complex->m_custGeosets.Count();
    geoset = complex->m_custGeosets.New();
  } else {
    CModelSimple *simple = (CModelSimple *)unique;
    *custGeosetId = simple->m_custGeosets.Count();
    simple->m_custGeosets.SetCount(*custGeosetId + 1);
    geoset = &simple->m_custGeosets[*custGeosetId];
  }

  geoset->position = modelSpacePosition;
  geoset->renderCallback = renderCallback;
  geoset->renderParam = renderParam;
}

void ModelCustGeosetMove(HMODEL model, UINT custGeosetId, const NTempest::C3Vector &modelSpacePosition) {
  CModelBase *unique;

  if (!IModelDerefHandle((CModel *)model, &unique)) {
    return;
  }

  (unique->m_flags & 0x20 ? ((CModelComplex *)unique)->m_custGeosets[custGeosetId]
                          : ((CModelSimple *)unique)->m_custGeosets[custGeosetId])
      .position = modelSpacePosition;
}

void ModelCustGeosetRemove(HMODEL model, UINT custGeosetId) {
  CModelBase *unique;
  if (!IModelDerefHandle((CModel *)model, &unique)) {
    return;
  }

  if (unique->m_flags & 0x20) {
    CModelComplex *complex = (CModelComplex *)unique;
    {
      CModelComplex *unique = complex;
      ASSERT(custGeosetId < unique->m_custGeosets.Count());
      memmove(&unique->m_custGeosets[custGeosetId], &unique->m_custGeosets[custGeosetId] + 1, (unique->m_custGeosets.Count() - (custGeosetId + 1)) * sizeof(CCustomGeoset));
      unique->m_custGeosets.SetCount(unique->m_custGeosets.Count() - 1);
    }
  } else {
    CModelSimple *simple = (CModelSimple *)unique;
    {
      CModelSimple *unique = simple;
      ASSERT(custGeosetId < unique->m_custGeosets.Count());
      memmove(&unique->m_custGeosets[custGeosetId], &unique->m_custGeosets[custGeosetId] + 1, (unique->m_custGeosets.Count() - (custGeosetId + 1)) * sizeof(CCustomGeoset));
      unique->m_custGeosets.SetCount(unique->m_custGeosets.Count() - 1);
    }
  }
}

void ModelEnumAnimObjects(HMODEL model, int (*callbackfcn)(UINT, LPCSTR, LPVOID), LPVOID param) {
  CModelBase *unique;

  if (IModelDerefHandle((CModel *)model, &unique) && unique->m_anim) {
    AnimEnumObjects(unique->m_anim, callbackfcn, param);
  }
}

int ModelGetSequenceTime(HMODEL model, UINT seqIndex) {
  CModelBase *unique;

  if (IModelDerefHandle((CModel *)model, &unique) && unique->m_anim) {
    return AnimGetSequenceTime(unique->m_anim, seqIndex);
  }
  return 0;
}

UINT ModelGetPrimarySequence(HMODEL model) {
  CModelBase *unique;

  if (!IModelDerefHandle((CModel *)model, &unique) || !unique->m_anim) {
    return 0;
  }

  UINT sequence = 0;
  AnimGetPrimarySequence(unique->m_anim, &sequence);
  return sequence;
}

void ModelEnableAnimBlending(HMODEL model, int enabled) {
  CModelBase *unique;

  CModel *modelptr = (CModel *)model;
  VALIDATEBEGIN;
  VALIDATE(modelptr);
  VALIDATEENDVOID;

  if (!IModelDerefHandle(modelptr, &unique)) {
    EnqueueModelCommand(modelptr, MODEL_ENABLE_ANIM_BLENDING, enabled);
    return;
  }

  if (unique->m_anim) {
    AnimEnableBlending(unique->m_anim, enabled);
  }
}

BOOL ModelUsesBlending(HMODEL model) {
  CModelBase *unique;

  if (IModelDerefHandle((CModel *)model, &unique) && unique->m_anim) {
    return AnimUsesBlending(unique->m_anim);
  }

  return 0;
}

void ModelEnableEmitters(HMODEL model, int enable, int doLinkedModels) {
  UINT           numElements;
  CModelComplex *complex;
  CModelBase    *unique;

  if (!IModelDerefHandle((CModel *)model, &unique)) {
    return;
  }

  ASSERT(unique->m_flags & 0x00000020);
  complex = (CModelComplex *)unique;

  numElements = complex->m_emitters2.Count();
  for (UINT i = 0; i < numElements; ++i) {
    complex->m_emitters2[i]->SetEnabled2(enable, 1);
  }

  if (doLinkedModels) {
    numElements = complex->m_attached.Count();
    for (UINT i = 0; i < numElements; ++i) {
      ITERATELIST(LINKUNIQUE, complex->m_attached[i], link) {
        ModelEnableEmitters(link->child, enable, 0);
      }
    }
  }
}

void ModelEnableRibbons(HMODEL model, int enable) {
  CModelBase *unique;

  if (!IModelDerefHandle((CModel *)model, &unique)) {
    return;
  }

  ASSERT(unique->m_flags & 0x00000020);
  CModelComplex *complex = (CModelComplex *)unique;

  UINT numElements = complex->m_ribbons.Count();
  for (UINT i = 0; i < numElements; ++i) {
    complex->m_ribbons[i]->SetEnabled(enable);
  }
}

void IModelEnableFullAlpha(CModelBase *unique, int enable) {
  HMATERIAL *materials;
  UINT       numMaterials;
  EGxBlend   alphaOp;

  ASSERT(unique);

  alphaOp = enable ? GxBlend_Alpha : GxBlend_AlphaKey;
  if (unique->m_flags & 0x20) {
    CModelComplex *complex = (CModelComplex *)unique;
    materials = complex->m_materials.Ptr();
    numMaterials = complex->m_materials.Count();
  } else {
    CModelSimple *simple = (CModelSimple *)unique;
    materials = simple->m_materials.Ptr();
    numMaterials = simple->m_materials.Count();
  }

  for (; numMaterials; --numMaterials, ++materials) {
    CMaterial *materialUnique = (CMaterial *)*materials;
    ASSERT(materialUnique);

    CMaterialShared *materialShared = (CMaterialShared *)materialUnique->data;
    ASSERT(materialShared);

    UINT             numLayers = materialShared->layers.Count();
    CTexLayer       *uniqueLayers = materialUnique->layers.Ptr();
    CTexLayerShared *sharedLayers = materialShared->layers.Ptr();
    for (; numLayers; --numLayers, ++uniqueLayers, ++sharedLayers) {
      if (sharedLayers->blendMode == GxBlend_Alpha) {
        uniqueLayers->blendMode = alphaOp;
      }
    }
  }
}

void ModelEnableFullAlpha(HMODEL model, int enable) {
  CModelBase *unique;

  CModel *modelptr = (CModel *)model;
  VALIDATEBEGIN;
  VALIDATE(modelptr);
  VALIDATEENDVOID;

  if (!IModelDerefHandle(modelptr, &unique)) {
    EnqueueModelCommand(modelptr, MODEL_ENABLE_FULL_ALPHA, enable);
    return;
  }

  IModelEnableFullAlpha(unique, enable);
}

BOOL ModelAnimHasObjectId(HMODEL model, UINT objectId) {
  CModelBase *unique;

  if (IModelDerefHandle((CModel *)model, &unique) && unique->m_anim) {
    return AnimHasObjectId(unique->m_anim, objectId);
  }

  return 0;
}

static void IModelSetMaterialDisables(HMATERIAL *materials, UINT numMaterials, UINT setMask, UINT unsetMask) {
  for (UINT i = 0; i < numMaterials; ++i) {
    CMaterial *uniqueMtl = (CMaterial *)materials[i];
    ASSERT(uniqueMtl);

    UINT numLayers = uniqueMtl->layers.Count();
    for (UINT layer = 0; layer < numLayers; ++layer) {
      if (setMask & 0x01)
        uniqueMtl->layers[layer].disables |= 0x01;
      if (setMask & 0x10)
        uniqueMtl->layers[layer].disables |= 0x10;
      if (setMask & 0x20)
        uniqueMtl->layers[layer].disables |= 0x02;
      if (setMask & 0x40)
        uniqueMtl->layers[layer].disables |= 0x04;
      if (setMask & 0x80)
        uniqueMtl->layers[layer].disables |= 0x08;
      if (unsetMask & 0x01)
        uniqueMtl->layers[layer].disables &= ~0x01U;
      if (unsetMask & 0x10)
        uniqueMtl->layers[layer].disables &= ~0x10U;
      if (unsetMask & 0x20)
        uniqueMtl->layers[layer].disables &= ~0x02U;
      if (unsetMask & 0x40)
        uniqueMtl->layers[layer].disables &= ~0x04U;
      if (unsetMask & 0x80)
        uniqueMtl->layers[layer].disables &= ~0x08U;
    }
  }
}

static void ComplexModelSetMaterialDisables(CModelComplex *unique, UINT setMask, UINT unsetMask, int doLinkedModels) {
  UINT i;

  FATALASSERT(unique);
  IModelSetMaterialDisables(unique->m_materials.Ptr(), unique->m_materials.Count(), setMask, unsetMask);

  if (!doLinkedModels) {
    return;
  }

  UINT numEmitters = unique->m_emitters2.Count();
  for (i = 0; i < numEmitters; ++i) {
    if (setMask & 0x01)
      unique->m_emitters2[i]->MaterialDisableLight(1);
    if (setMask & 0x20)
      unique->m_emitters2[i]->MaterialDisableFog(1);
    if (unsetMask & 0x01)
      unique->m_emitters2[i]->MaterialDisableLight(0);
    if (unsetMask & 0x20)
      unique->m_emitters2[i]->MaterialDisableFog(0);
  }

  numEmitters = unique->m_ribbons.Count();
  for (i = 0; i < numEmitters; ++i) {
    if (setMask & 0x01)
      unique->m_ribbons[i]->MaterialDisableLight(1);
    if (setMask & 0x20)
      unique->m_ribbons[i]->MaterialDisableFog(1);
    if (unsetMask & 0x01)
      unique->m_ribbons[i]->MaterialDisableLight(0);
    if (unsetMask & 0x20)
      unique->m_ribbons[i]->MaterialDisableFog(0);
  }

  UINT numAttached = unique->m_attached.Count();
  for (i = 0; i < numAttached; ++i) {
    ITERATELIST(LINKUNIQUE, unique->m_attached[i], link) {
      ModelSetMaterialDisables(link->child, setMask, unsetMask, 1);
    }
  }
}

void ModelSetMaterialDisables(HMODEL model, UINT setMask, UINT unsetMask, int doLinkedModels) {
  CModelBase *unique;
  if (!IModelDerefHandle((CModel *)model, &unique)) {
    return;
  }

  if (unique->m_flags & 0x20) {
    ComplexModelSetMaterialDisables((CModelComplex *)unique, setMask, unsetMask, doLinkedModels);
  } else {
    CModelSimple *simple = (CModelSimple *)unique;
    IModelSetMaterialDisables(simple->m_materials.Ptr(), simple->m_materials.Count(), setMask, unsetMask);
  }
}

UINT ModelGetNumTextures(HMODEL model) {
  CModelBase *unique;

  if (!IModelDerefHandle((CModel *)model, &unique)) {
    return 0;
  }

  if (unique->m_flags & 0x20) {
    return ((CModelComplex *)unique)->m_textures.Count();
  }

  return ((CModelSimple *)unique)->m_textures.Count();
}

UINT ModelGetTextureReplaceableId(HMODEL model, UINT textureId) {
  CModelBase *unique;

  if (!IModelDerefHandle((CModel *)model, &unique)) {
    return 0;
  }

  if (unique->m_flags & 0x20) {
    CModelComplex *complex = (CModelComplex *)unique;
    ASSERT(textureId < complex->m_textures.Count());
    return complex->m_textures[textureId].replaceableId;
  }

  CModelSimple *simple = (CModelSimple *)unique;
  ASSERT(textureId < simple->m_textures.Count());
  return simple->m_textures[textureId].replaceableId;
}
