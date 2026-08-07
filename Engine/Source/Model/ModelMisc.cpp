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

void   ExecuteQueuedActions(CModel *model);
HMODEL ModelDuplicate(HMODEL sourceModel, UINT flags);
void   ModelSetMaterialDisables(HMODEL model, UINT setMask, UINT unsetMask, int doLinkedModels);

static UINT UpdateRibbonMaterial(CModelComplex *model, UINT replaceableId, HTEXTURE texture);
static void UpdateParticleEmitters(CModelComplex *unique, UINT replaceableId, HTEXTURE texture);

struct CMatrixGroup {
  CMatrixGroup() : matrices(0), numMatrices(0), index(0), leftIndex(static_cast<UINT>(-1)), rightIndex(static_cast<UINT>(-1)) {
  }
  CMatrixGroup(UINT *, UINT);

  UINT *matrices;
  UINT  numMatrices;
  UINT  index;
  UINT  leftIndex;
  UINT  rightIndex;
};

class CMatrixGroupTree {
 public:
  CMatrixGroupTree() : numMatrices(0) {
  }

  UINT Insert(UINT *matrixGroup, UINT numMatrices);
  UINT GroupCount() const {
    return nodes.Count();
  }
  UINT MatrixCount() const {
    return numMatrices;
  }
  BOOL GroupsEqual(const UINT *matrixGroup1, UINT numMatrices1, const UINT *matrixGroup2, UINT numMatrices2);
  int  GroupLessThan(const UINT *matrixGroup1, UINT numMatrices1, const UINT *matrixGroup2, UINT numMatrices2);

 private:
  UINT                          AddNode(UINT *matrixGroup, UINT numMatrices);
  TSGrowableArray<CMatrixGroup> nodes;
  UINT                          numMatrices;
};

CModelTexture::CModelTexture(const CModelTexture &source) {
  replaceableId = source.replaceableId;
  handle = static_cast<HTEXTURE>(HandleDuplicate(source.handle));
}

CModelTexture &CModelTexture::operator=(const CModelTexture &source) {
  replaceableId = source.replaceableId;
  if (handle) {
    HandleClose(handle);
  }
  handle = static_cast<HTEXTURE>(HandleDuplicate(source.handle));
  return *this;
}

CTexLayer::CTexLayer(const CTexLayer &a) : vertexFormat(a.vertexFormat), disables(a.disables), blendMode(a.blendMode), layerAlpha(a.layerAlpha) {
  for (UINT i = 0; i < 2; ++i) {
    tmuPass[i] = a.tmuPass[i];
  }
}

static BOOL MaterialUsedOnce(HMATERIAL material) {
  CMaterial *uniqueMtl = reinterpret_cast<CMaterial *>(material);

  ASSERT(uniqueMtl);
  return uniqueMtl->GetRefCount() == 1;
}

static HMATERIAL MaterialDuplicate(HMATERIAL material) {
  CMaterial *source = reinterpret_cast<CMaterial *>(material);
  CMaterial *duplicate;
  LPVOID     storage;

  ASSERT(source);

  storage = SMemAlloc(sizeof(CMaterial), "HMATERIAL", SERR_LINECODE_OBJECT, 0);
  duplicate = storage ? new (storage) CMaterial : 0;
  if (!duplicate) {
    return 0;
  }

  *duplicate = *source;
  return static_cast<HMATERIAL>(HandleCreate(duplicate, "HMATERIAL"));
}

void CModelSimple::CopyMaterials(const CModelSimple &source) {
  m_materials.SetCount(source.m_materials.Count());
  for (UINT i = 0; i < source.m_materials.Count(); ++i) {
    m_materials[i] = static_cast<HMATERIAL>(HandleDuplicate(source.m_materials[i]));
  }
}

CModelComplex::~CModelComplex() {
  UINT i;
  UINT numElements;

  for (i = 0; i < m_attached.Count(); ++i) {
    LINKUNIQUE *link;
    while ((link = m_attached[i].Head()) != 0) {
      DEL(link);
    }
  }

  for (i = 0; i < m_materials.Count(); ++i) {
    HandleClose(m_materials[i]);
  }
  for (i = 0; i < m_cameras.Count(); ++i) {
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

void CModelComplex::CopyAttachments(const CModelComplex &source) {
  UINT i;
  m_attached.SetCount(source.m_attached.Count());
  for (i = 0; i < source.m_attached.Count(); ++i) {
    LIST(LINKUNIQUE) &sourceList = const_cast<LIST(LINKUNIQUE) &>(source.m_attached[i]);
    ITERATELIST(LINKUNIQUE, sourceList, sourceLink) {
      LINKUNIQUE *copy = NEW(LINKUNIQUE);
      ASSERT(copy);
      copy->child = ModelDuplicate(sourceLink->child, 0);
      copy->scale = sourceLink->scale;
      m_attached[i].LinkNode(copy, LIST_TAIL, 0);
    }
  }
  m_attachmentFlags = source.m_attachmentFlags;
}

void CModelComplex::CopyCameras(const CModelComplex &source) {
  m_cameras.SetCount(source.m_cameras.Count());
  for (UINT i = 0; i < source.m_cameras.Count(); ++i) {
    m_cameras[i] = CameraDuplicate(source.m_cameras[i]);
  }
  m_cameraOrder = source.m_cameraOrder;
}

void CModelComplex::CopyLights(const CModelComplex &source) {
  m_lights.SetCount(source.m_lights.Count());
  for (UINT i = 0; i < source.m_lights.Count(); ++i) {
    DWORD newLightId = GxuLightCreate();
    m_lights[i] = newLightId;
    *GxuLightLock(newLightId) = *GxuLightLock(source.m_lights[i]);
    GxuLightUnlock(newLightId);
    GxuLightUnlock(source.m_lights[i]);
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
  for (UINT i = 0; i < source.m_ribbons.Count(); ++i) {
    m_ribbons[i] = RibbonManager::GetInstance()->DuplicateEmitter(source.m_ribbons[i]);
  }
}

void GxuLightSelectCallback(LPVOID parm, NTempest::C3Vector worldPos, const NTempest::C3Vector &cameraWorldPos, UINT maxLightsToUse) {
  (void)parm;
  GxuLightSelect(worldPos, cameraWorldPos, maxLightsToUse);
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

CModelSimple::~CModelSimple() {
  UINT i;
  for (i = 0; i < m_materials.Count(); ++i) {
    HandleClose(m_materials.Ptr()[i]);
  }
}

CModelBase::CModelBase(const CModelBase &source)
    : m_PickLights(source.m_PickLights),
      m_pickLightsParm(source.m_pickLightsParm),
      m_flags(source.m_flags & ~4U),
      m_modelToWorld(),
      m_anim(source.m_anim ? AnimDuplicate(source.m_anim, (source.m_flags >> 2) & 1) : 0),
      m_boundsModel(source.m_boundsModel ? ModelDuplicate(source.m_boundsModel, 0) : 0),
      m_aaBoxCustGeoId(source.m_aaBoxCustGeoId),
      m_collideModel(source.m_collideModel ? ModelDuplicate(source.m_collideModel, 0) : 0) {
}

CModelComplex::CModelComplex(const CModelSimple &source) : CModelBase(source) {
  UINT i;

  m_materials.SetCount(source.m_materials.Count());
  for (i = 0; i < source.m_materials.Count(); ++i) {
    m_materials[i] = static_cast<HMATERIAL>(HandleDuplicate(source.m_materials[i]));
  }

  m_textures.SetCount(source.m_textures.Count());
  for (i = 0; i < source.m_textures.Count(); ++i) {
    m_textures[i] = source.m_textures[i];
  }

  m_geosets.Set(source.m_geosets.Count(), source.m_geosets.Ptr());
  m_geosetColor.Set(source.m_geosetColor.Count(), source.m_geosetColor.Ptr());
}

CModelComplex::CModelComplex(const CModelComplex &source) : CModelBase(source) {
  m_materials.SetCount(source.m_materials.Count());
  for (UINT i = 0; i < source.m_materials.Count(); ++i) {
    m_materials[i] = static_cast<HMATERIAL>(HandleDuplicate(source.m_materials[i]));
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
  UINT i;

  m_geosets.SetCount(5);
  m_geosetColor.SetCount(5);
  m_custGeosets.SetCount(1);
  m_materials.SetCount(4);
  m_textures.SetCount(4);
  for (i = 0; i < 4; ++i) {
    m_textures[i].handle = 0;
    m_textures[i].replaceableId = 0;
  }

  CopyMaterials(source);
  m_textures.SetCount(source.m_textures.Count());
  for (i = 0; i < source.m_textures.Count(); ++i) {
    m_textures[i] = source.m_textures[i];
  }

  m_geosets.SetCount(source.m_geosets.Count());
  for (i = 0; i < source.m_geosets.Count(); ++i) {
    m_geosets[i] = source.m_geosets[i];
  }
  m_geosetColor.SetCount(source.m_geosetColor.Count());
  for (i = 0; i < source.m_geosetColor.Count(); ++i) {
    m_geosetColor[i] = source.m_geosetColor[i];
  }
  m_custGeosets.SetCount(0);
}

CModel::CModel(CModel &source)
    : asyncObject(0), createData(0), shared(static_cast<HMODELSHARED>(HandleDuplicate(source.shared))), state(CMODEL_UNINITIALIZED) {
  ASSERT(source.shared);
  ASSERT(shared);

  if (source.state == CMODEL_LOADED) {
    FinishDuplication(source);
  } else {
    dupSource = static_cast<HMODEL>(HandleCreate(&source, "HMODEL"));
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
    CModelComplex *sourceComplex = reinterpret_cast<CModelComplex *>(source.data);
    LPVOID         storage = SMemAlloc(sizeof(CModelComplex), __FILE__, __LINE__, 0);
    data = storage ? new (storage) CModelComplex(*sourceComplex) : 0;
  } else {
    CModelSimple *sourceSimple = reinterpret_cast<CModelSimple *>(source.data);
    LPVOID        storage = SMemAlloc(sizeof(CModelSimple), __FILE__, __LINE__, 0);
    data = storage ? new (storage) CModelSimple(*sourceSimple) : 0;
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
        DEL(reinterpret_cast<CModelComplex *>(data));
      } else {
        DEL(reinterpret_cast<CModelSimple *>(data));
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

  UINT             numReplaced = 0;
  UINT             count = model->m_ribbons.Count();
  CRibbonEmitter **ribbon = model->m_ribbons.Ptr();
  while (count--) {
    numReplaced += (*ribbon++)->ReplaceTexture(replaceableId, texture);
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
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    return;
  }

  if (unique->m_flags & 0x20) {
    CModelComplex *complex = reinterpret_cast<CModelComplex *>(unique);
    ASSERT(materialIndex < complex->m_materials.Count());
    CMaterial *uniqueMtl = reinterpret_cast<CMaterial *>(complex->m_materials[materialIndex]);
    ASSERT(uniqueMtl);
    ASSERT(layerIndex < uniqueMtl->layers.Count());
    uniqueMtl->layers[layerIndex].layerAlpha = show ? 0xFF : 0;
  } else {
    CModelSimple *simple = reinterpret_cast<CModelSimple *>(unique);
    ASSERT(materialIndex < simple->m_materials.Count());
    CMaterial *uniqueMtl = reinterpret_cast<CMaterial *>(simple->m_materials[materialIndex]);
    ASSERT(uniqueMtl);
    ASSERT(layerIndex < uniqueMtl->layers.Count());
    uniqueMtl->layers[layerIndex].layerAlpha = show ? 0xFF : 0;
  }
}

static void ComplexModelSetEmissiveColor(CModelComplex *unique, const NTempest::CImVector &color, int doLinkedModels) {
  ASSERT(unique);

  for (UINT i = 0; i < unique->m_materials.Count(); ++i) {
    if (!MaterialUsedOnce(unique->m_materials[i])) {
      HMATERIAL oldMaterial = unique->m_materials[i];
      unique->m_materials[i] = MaterialDuplicate(oldMaterial);
      HandleClose(oldMaterial);
    }

    CMaterial *uniqueMtl = reinterpret_cast<CMaterial *>(unique->m_materials[i]);
    ASSERT(uniqueMtl);
    uniqueMtl->emissiveColor = color;
  }

  if (doLinkedModels) {
    for (UINT i = 0; i < unique->m_attached.Count(); ++i) {
      LIST(LINKUNIQUE) &links = unique->m_attached[i];
      ITERATELIST(LINKUNIQUE, links, link) {
        ModelSetEmissiveColor(link->child, color, 1);
      }
    }
  }
}

void ModelSetEmissiveColor(HMODEL model, const NTempest::CImVector &color, int doLinkedModels) {
  FATALASSERT(model);

  CModelBase *unique;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    EnqueueModelCommand(reinterpret_cast<CModel *>(model), MODEL_SET_EMISSIVE_COLOR, *reinterpret_cast<const DWORD *>(&color), doLinkedModels);
    return;
  }

  if (unique->m_flags & 0x20) {
    ComplexModelSetEmissiveColor(static_cast<CModelComplex *>(unique), color, doLinkedModels);
    return;
  }

  CModelSimple *simple = static_cast<CModelSimple *>(unique);
  for (UINT i = 0; i < simple->m_materials.Count(); ++i) {
    if (!MaterialUsedOnce(simple->m_materials[i])) {
      HMATERIAL oldMaterial = simple->m_materials[i];
      simple->m_materials[i] = MaterialDuplicate(oldMaterial);
      HandleClose(oldMaterial);
    }

    CMaterial *uniqueMtl = reinterpret_cast<CMaterial *>(simple->m_materials[i]);
    ASSERT(uniqueMtl);
    uniqueMtl->emissiveColor = color;
  }
}

static UINT ComplexModelReplaceTexture(CModelComplex *unique, UINT replaceableId, HTEXTURE texture, int doLinkedModels) {
  UINT numReplaced = 0;
  UINT numAttached;
  UINT index;

  for (index = 0; index < unique->m_textures.Count(); ++index) {
    CModelTexture &modelTexture = unique->m_textures[index];

    if (modelTexture.replaceableId == replaceableId) {
      HTEXTURE oldTexture = modelTexture.handle;
      modelTexture.handle = static_cast<HTEXTURE>(HandleDuplicate(texture));
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

  FATALASSERT(model);

  FATALASSERT(replaceableId != 0);

  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    EnqueueModelCommand(reinterpret_cast<CModel *>(model), MODEL_REPLACE_TEXTURE, replaceableId, texture, doLinkedModels);
    return 1;
  }

  if (unique->m_flags & 0x20) {
    return ComplexModelReplaceTexture(static_cast<CModelComplex *>(unique), replaceableId, texture, doLinkedModels) != 0;
  }

  CModelSimple *simple = static_cast<CModelSimple *>(unique);
  UINT          numReplaced = 0;
  for (UINT index = 0; index < simple->m_textures.Count(); ++index) {
    CModelTexture &modelTexture = simple->m_textures[index];

    if (modelTexture.replaceableId == replaceableId) {
      HTEXTURE oldTexture = modelTexture.handle;
      modelTexture.handle = static_cast<HTEXTURE>(HandleDuplicate(texture));
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
  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &shared)) {
    return shared->numBones;
  }
  return 0;
}

BOOL ModelGetLinkPoint(HMODEL model, UINT index, HMODEL *modelList, UINT *entriesInOut) {
  CModelShared *shared;
  CModelBase   *unique;
  UINT          added = 0;

  FATALASSERT(modelList);
  FATALASSERT(entriesInOut);
  FATALASSERT(*entriesInOut > 0);

  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique, &shared) || !(unique->m_flags & 0x20)) {
    *entriesInOut = 0;
    return 0;
  }

  ASSERT(shared);
  if (index >= shared->attachIdToIndex.Count() || shared->attachIdToIndex[index] == static_cast<UINT>(-1)) {
    *entriesInOut = 0;
    return 0;
  }

  LIST(LINKUNIQUE) &links = static_cast<CModelComplex *>(unique)->m_attached[shared->attachIdToIndex[index]];
  ITERATELIST(LINKUNIQUE, links, link) {
    if (added == *entriesInOut) {
      break;
    }
    *modelList++ = static_cast<HMODEL>(HandleDuplicate(link->child));
    ++added;
  }

  *entriesInOut = added;
  return 1;
}

BOOL ModelGetNumLinkedAtPoint(HMODEL model, UINT index, UINT *numLinked) {
  CModelShared *shared;
  CModelBase   *unique;

  FATALASSERT(numLinked);
  *numLinked = 0;

  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique, &shared) || !(unique->m_flags & 0x20)) {
    return 0;
  }

  ASSERT(shared);
  if (index >= shared->attachIdToIndex.Count() || shared->attachIdToIndex[index] == static_cast<UINT>(-1)) {
    return 0;
  }

  LIST(LINKUNIQUE) &links = static_cast<CModelComplex *>(unique)->m_attached[shared->attachIdToIndex[index]];
  ITERATELIST(LINKUNIQUE, links, link) {
    ++*numLinked;
  }

  return 1;
}

BOOL ModelHasLinkPoint(HMODEL model, UINT index) {
  CModelShared *shared;

  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &shared)) {
    return 0;
  }

  ASSERT(shared);
  return index < shared->attachIdToIndex.Count() && shared->attachIdToIndex[index] != static_cast<UINT>(-1);
}

UINT ModelGetNumLinkPoints(HMODEL model) {
  CModelBase *unique;

  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) && (unique->m_flags & 0x20)) {
    return static_cast<CModelComplex *>(unique)->m_attached.Count();
  }

  return 0;
}

BOOL ModelAddLink(HMODEL parent, UINT parentIndex, HMODEL child, float scale) {
  CModelBase    *parentBase;
  CModelShared  *parentdata;
  CModelComplex *parentptr;

  FATALASSERT(parent);

  ASSERT(!NTempest::CMath::fequal_(scale, 0.0f));
  ASSERT(child);

  if (!IModelDerefHandle(reinterpret_cast<CModel *>(parent), &parentBase, &parentdata)) {
    EnqueueModelCommand(reinterpret_cast<CModel *>(parent), MODEL_ADD_LINK, parentIndex, child, scale);
    return 1;
  }

  if (!(parentBase->m_flags & 0x20)) {
    return 0;
  }

  parentptr = static_cast<CModelComplex *>(parentBase);
  ASSERT(parentdata);

  if (parentIndex >= parentdata->attachIdToIndex.Count()) {
    return 0;
  }

  if (parentdata->attachIdToIndex[parentIndex] == static_cast<UINT>(-1)) {
    return 0;
  }

  LIST(LINKUNIQUE) &links = parentptr->m_attached[parentdata->attachIdToIndex[parentIndex]];
  LINKUNIQUE *link = links.NewNode(LIST_TAIL, 0, 0);
  link->child = static_cast<HMODEL>(HandleDuplicate(child));
  link->scale = scale;

  ModelSetLightSelectCallback(link->child, parentptr->m_PickLights, parentptr->m_pickLightsParm, 1);
  return 1;
}

BOOL ModelRemoveLink(HMODEL parent, UINT parentIndex, HMODEL child) {
  CModelBase   *parentBase;
  CModelShared *parentdata;

  FATALASSERT(parent);

  if (!IModelDerefHandle(reinterpret_cast<CModel *>(parent), &parentBase, &parentdata)) {
    EnqueueModelCommand(reinterpret_cast<CModel *>(parent), MODEL_REMOVE_LINK, parentIndex, child);
    return 1;
  }

  if (!(parentBase->m_flags & 0x20)) {
    return 0;
  }

  CModelComplex *parentptr = static_cast<CModelComplex *>(parentBase);
  ASSERT(parentdata);
  if (parentIndex >= parentdata->attachIdToIndex.Count()) {
    return 0;
  }

  UINT attachmentIndex = parentdata->attachIdToIndex[parentIndex];
  if (attachmentIndex == static_cast<UINT>(-1)) {
    return 0;
  }

  ASSERT(child);
  LIST(LINKUNIQUE) &links = parentptr->m_attached[attachmentIndex];
  ITERATELIST(LINKUNIQUE, links, link) {
    if (link->child == child) {
      DEL(link);
      break;
    }
  }

  return 1;
}

BOOL ModelClearLink(HMODEL parent, UINT parentIndex) {
  CModelBase    *parentBase;
  CModelShared  *parentdata;
  CModelComplex *parentptr;
  UINT           attachmentIndex;

  FATALASSERT(parent);

  if (!IModelDerefHandle(reinterpret_cast<CModel *>(parent), &parentBase, &parentdata)) {
    EnqueueModelCommand(reinterpret_cast<CModel *>(parent), MODEL_CLEAR_LINK, parentIndex);
    return 1;
  }

  if (!(parentBase->m_flags & 0x20)) {
    return 0;
  }

  parentptr = static_cast<CModelComplex *>(parentBase);
  ASSERT(parentdata);

  if (parentIndex >= parentdata->attachIdToIndex.Count()) {
    return 0;
  }

  attachmentIndex = parentdata->attachIdToIndex[parentIndex];
  if (attachmentIndex == static_cast<UINT>(-1)) {
    return 0;
  }

  LIST(LINKUNIQUE) &links = parentptr->m_attached[attachmentIndex];
  while (LINKUNIQUE *link = links.Head()) {
    links.DeleteNode(link);
  }

  return 1;
}

void ModelClearAllLinks(HMODEL parent) {
  CModelBase *parentBase;
  UINT        numAttachments;
  UINT        i;

  FATALASSERT(parent);

  if (!IModelDerefHandle(reinterpret_cast<CModel *>(parent), &parentBase)) {
    EnqueueModelCommand(reinterpret_cast<CModel *>(parent), MODEL_CLEAR_ALL_LINKS);
    return;
  }

  if (!(parentBase->m_flags & 0x20)) {
    return;
  }

  numAttachments = static_cast<CModelComplex *>(parentBase)->m_attached.Count();
  for (i = 0; i < numAttachments; ++i) {
    LINKUNIQUE *link;
    while ((link = static_cast<CModelComplex *>(parentBase)->m_attached[i].Head()) != 0) {
      DEL(link);
    }
  }
}

UINT ModelGetNumCameras(HMODEL model) {
  CModelBase *unique;

  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) && (unique->m_flags & 0x20)) {
    return static_cast<CModelComplex *>(unique)->m_cameras.Count();
  }

  return 0;
}

HCAMERA ModelGetCamera(HMODEL model, UINT index) {
  CModelBase *unique;

  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) || !(unique->m_flags & 0x20)) {
    return 0;
  }

  CModelComplex *complex = static_cast<CModelComplex *>(unique);
  if (complex->m_cameraOrder.Count()) {
    if (index >= complex->m_cameraOrder.Count()) {
      return 0;
    }

    index = complex->m_cameraOrder[index];
    if (index == static_cast<UINT>(-1)) {
      return 0;
    }
  }

  FATALASSERT(index < complex->m_cameras.Count());

  return static_cast<HCAMERA>(HandleDuplicate(complex->m_cameras[index]));
}

int ModelIsCameraEnabled(HMODEL model, UINT index) {
  CModelBase *unique;

  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) || !(unique->m_flags & 0x20)) {
    return 0;
  }

  CModelComplex *complex = static_cast<CModelComplex *>(unique);
  if (complex->m_cameraOrder.Count()) {
    if (index >= complex->m_cameraOrder.Count()) {
      return 0;
    }

    index = complex->m_cameraOrder[index];
    if (index == static_cast<UINT>(-1)) {
      return 0;
    }
  }

  FATALASSERT(index < complex->m_cameras.Count());
  return AnimIsCameraEnabled(unique->m_anim, index);
}

BOOL ModelIsShowingBoundingSphere(HMODEL model) {
  CModelBase *unique;

  return IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) && unique->m_boundsModel && (unique->m_flags & 1);
}

BOOL ModelIsShowingBoundingBox(HMODEL model) {
  CModelBase *unique;

  return IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) && unique->m_boundsModel && !(unique->m_flags & 1);
}

BOOL ModelIsShowingHitTestGeometry(HMODEL model) {
  CModelBase *unique;

  return IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) && (unique->m_flags & 8);
}

static void ComplexModelRestoreBlendMode(CModelComplex *unique, int doLinkedModels) {
  UINT numAttachments;

  ASSERT(unique);

  for (UINT i = 0; i < unique->m_materials.Count(); ++i) {
    CMaterial *uniqueMtl = reinterpret_cast<CMaterial *>(unique->m_materials[i]);
    ASSERT(uniqueMtl);

    CMaterialShared *sharedMtl = reinterpret_cast<CMaterialShared *>(uniqueMtl->data);
    ASSERT(sharedMtl);

    if (!MaterialUsedOnce(unique->m_materials[i])) {
      unique->m_materials[i] = MaterialDuplicate(unique->m_materials[i]);
      HandleClose(reinterpret_cast<HMATERIAL>(uniqueMtl));
      uniqueMtl = reinterpret_cast<CMaterial *>(unique->m_materials[i]);
      ASSERT(uniqueMtl);
    }

    for (UINT layer = 0; layer < sharedMtl->layers.Count(); ++layer) {
      uniqueMtl->layers[layer].blendMode = sharedMtl->layers[layer].blendMode;
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

  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    return;
  }

  if (unique->m_flags & 0x20) {
    ComplexModelRestoreBlendMode(static_cast<CModelComplex *>(unique), doLinkedModels);
    return;
  }

  CModelSimple *simple = static_cast<CModelSimple *>(unique);
  for (UINT i = 0; i < simple->m_materials.Count(); ++i) {
    CMaterial *uniqueMtl = reinterpret_cast<CMaterial *>(simple->m_materials[i]);
    ASSERT(uniqueMtl);

    CMaterialShared *sharedMtl = reinterpret_cast<CMaterialShared *>(uniqueMtl->data);
    ASSERT(sharedMtl);

    if (!MaterialUsedOnce(simple->m_materials[i])) {
      simple->m_materials[i] = MaterialDuplicate(simple->m_materials[i]);
      HandleClose(reinterpret_cast<HMATERIAL>(uniqueMtl));
      uniqueMtl = reinterpret_cast<CMaterial *>(simple->m_materials[i]);
      ASSERT(uniqueMtl);
    }

    for (UINT layer = 0; layer < sharedMtl->layers.Count(); ++layer) {
      uniqueMtl->layers[layer].blendMode = sharedMtl->layers[layer].blendMode;
    }
  }
}

static void ComplexModelSetBlendMode(CModelComplex *unique, EGxBlend blendMode, int doLinkedModels) {
  UINT numAttachments;

  ASSERT(unique);

  for (UINT i = 0; i < unique->m_materials.Count(); ++i) {
    CMaterial *uniqueMtl = reinterpret_cast<CMaterial *>(unique->m_materials[i]);
    ASSERT(uniqueMtl);

    CMaterialShared *sharedMtl = reinterpret_cast<CMaterialShared *>(uniqueMtl->data);
    ASSERT(sharedMtl);

    if (!MaterialUsedOnce(unique->m_materials[i])) {
      unique->m_materials[i] = MaterialDuplicate(unique->m_materials[i]);
      HandleClose(reinterpret_cast<HMATERIAL>(uniqueMtl));
      uniqueMtl = reinterpret_cast<CMaterial *>(unique->m_materials[i]);
      ASSERT(uniqueMtl);
    }

    for (UINT layer = 0; layer < sharedMtl->layers.Count(); ++layer) {
      uniqueMtl->layers[layer].blendMode = blendMode;
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

  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    return;
  }

  if (unique->m_flags & 0x20) {
    ComplexModelSetBlendMode(static_cast<CModelComplex *>(unique), blendMode, doLinkedModels);
    return;
  }

  CModelSimple *simple = static_cast<CModelSimple *>(unique);
  for (UINT i = 0; i < simple->m_materials.Count(); ++i) {
    CMaterial *uniqueMtl = reinterpret_cast<CMaterial *>(simple->m_materials[i]);
    ASSERT(uniqueMtl);

    CMaterialShared *sharedMtl = reinterpret_cast<CMaterialShared *>(uniqueMtl->data);
    ASSERT(sharedMtl);

    if (!MaterialUsedOnce(simple->m_materials[i])) {
      simple->m_materials[i] = MaterialDuplicate(simple->m_materials[i]);
      HandleClose(reinterpret_cast<HMATERIAL>(uniqueMtl));
      uniqueMtl = reinterpret_cast<CMaterial *>(simple->m_materials[i]);
      ASSERT(uniqueMtl);
    }

    for (UINT layer = 0; layer < sharedMtl->layers.Count(); ++layer) {
      uniqueMtl->layers[layer].blendMode = blendMode;
    }
  }
}

void ModelHideGeosets(HMODEL model, UINT selectionGroup, int hide) {
  FATALASSERT(model);

  CModelBase   *unique;
  CModelShared *shared;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique, &shared)) {
    EnqueueModelCommand(reinterpret_cast<CModel *>(model), MODEL_HIDE_GEOSETS, selectionGroup, hide);
    return;
  }

  ASSERT(shared);
  if (unique->m_flags & 0x20) {
    CModelComplex *complex = static_cast<CModelComplex *>(unique);
    for (UINT index = 0; index < shared->numGeosets; ++index) {
      if (shared->geosets[index].selectionGroup == selectionGroup) {
        if (hide) {
          complex->m_geosets[index].flags |= 1;
        } else {
          complex->m_geosets[index].flags &= ~1u;
        }
      }
    }
  } else {
    CModelSimple *complex = static_cast<CModelSimple *>(unique);
    for (UINT index = 0; index < shared->numGeosets; ++index) {
      if (shared->geosets[index].selectionGroup == selectionGroup) {
        if (hide) {
          complex->m_geosets[index].flags |= 1;
        } else {
          complex->m_geosets[index].flags &= ~1u;
        }
      }
    }
  }
}

void ModelHideGeosetsRange(HMODEL model, UINT selectionStart, UINT selectionEnd, int hide) {
  FATALASSERT(model);

  CModelBase   *unique;
  CModelShared *shared;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique, &shared)) {
    EnqueueModelCommand(reinterpret_cast<CModel *>(model), MODEL_HIDE_GEOSETS_RANGE, selectionStart, selectionEnd, hide);
    return;
  }

  if (unique->m_flags & 0x20) {
    CModelComplex *complex = static_cast<CModelComplex *>(unique);
    for (UINT index = 0; index < shared->numGeosets; ++index) {
      if (shared->geosets[index].selectionGroup >= selectionStart && shared->geosets[index].selectionGroup <= selectionEnd) {
        if (hide) {
          complex->m_geosets[index].flags |= 1;
        } else {
          complex->m_geosets[index].flags &= ~1u;
        }
      }
    }
  } else {
    CModelSimple *simple = static_cast<CModelSimple *>(unique);
    for (UINT index = 0; index < shared->numGeosets; ++index) {
      if (shared->geosets[index].selectionGroup >= selectionStart && shared->geosets[index].selectionGroup <= selectionEnd) {
        if (hide) {
          simple->m_geosets[index].flags |= 1;
        } else {
          simple->m_geosets[index].flags &= ~1u;
        }
      }
    }
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
    ASSERT(parentIndex != static_cast<UINT>(-1));
    if (GroupsEqual(nodes[parentIndex].matrices, nodes[parentIndex].numMatrices, matrixGroup, numMatrices)) {
      return nodes[parentIndex].index;
    }

    if (GroupLessThan(matrixGroup, numMatrices, nodes[parentIndex].matrices, nodes[parentIndex].numMatrices)) {
      if (nodes[parentIndex].leftIndex == static_cast<UINT>(-1)) {
        UINT childIndex = AddNode(matrixGroup, numMatrices);
        nodes[parentIndex].leftIndex = childIndex;
        return childIndex;
      }
      parentIndex = nodes[parentIndex].leftIndex;
    } else {
      if (nodes[parentIndex].rightIndex == static_cast<UINT>(-1)) {
        UINT childIndex = AddNode(matrixGroup, numMatrices);
        nodes[parentIndex].rightIndex = childIndex;
        return childIndex;
      }
      parentIndex = nodes[parentIndex].rightIndex;
    }
  }
}

BOOL CMatrixGroupTree::GroupsEqual(const UINT *matrixGroup1, UINT numMatrices1, const UINT *matrixGroup2, UINT numMatrices2) {
  UINT count = numMatrices1;
  if (count >= numMatrices2) {
    count = numMatrices2;
  }
  if (memcmp(matrixGroup1, matrixGroup2, count * sizeof(UINT))) {
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
  while (count) {
    UINT  middle = start + count / 2;
    UINT  numMatrices = srcGeoset->groupMatrixCounts[middle];
    UINT  matrixOffset = matrixOffsets[middle];
    UINT *matrices = srcGeoset->matrices.Ptr() + matrixOffset;
    UINT  groupCount = matrixGroupSets->GroupCount();
    UINT  index = matrixGroupSets->Insert(matrices, numMatrices);
    groupIdConvert[middle] = static_cast<BYTE>(index);

    if (matrixGroupSets->GroupCount() > groupCount) {
      dstGeoset->groupMatrixCounts[index] = numMatrices;
      UINT dstOffset = matrixGroupSets->MatrixCount() - numMatrices;
      memcpy(dstGeoset->matrices.Ptr() + dstOffset, matrices, numMatrices * sizeof(UINT));
    }

    if (middle != start) {
      AddMatrixGroupRangeToSet(matrixGroupSets, srcGeoset, matrixOffsets, start, count / 2, dstGeoset, groupIdConvert);
    }
    start = middle + 1;
    count -= count / 2 + 1;
  }
}

static void AddGeosetMatrixGroups(CMatrixGroupTree *matrixGroupSets, CGeosetShared *srcGeoset, CGeosetShared *dstGeoset, UINT vertsAdded) {
  UINT  numBoneWeights = srcGeoset->boneWeights.Count();
  UINT  numGroups = srcGeoset->groupMatrixCounts.Count();
  UINT *matrixOffsets = static_cast<UINT *>(_alloca(numGroups * sizeof(UINT)));
  BYTE *groupIdConvert = static_cast<BYTE *>(_alloca(numBoneWeights));
  UINT  total = 0;
  UINT  index;

  for (index = 0; index < numGroups; ++index) {
    matrixOffsets[index] = total;
    total += srcGeoset->groupMatrixCounts[index];
  }
  AddMatrixGroupRangeToSet(matrixGroupSets, srcGeoset, matrixOffsets, 0, numGroups, dstGeoset, groupIdConvert);
  for (index = 0; index < numBoneWeights; ++index) {
    UINT groupId = srcGeoset->boneWeights[index];
    ASSERT(groupId < numBoneWeights);
    dstGeoset->boneWeights[vertsAdded + index] = groupIdConvert[groupId];
  }
}

static void BuildCompositeGeoset(CGeosetShared *newGeoset, UINT geosetId, CGeosetShared *geosets, const TSGrowableArray<UINT> &geosetIds) {
  UINT numGeosets = geosetIds.Count();
  ASSERT(numGeosets);

  UINT numVertices = 0;
  UINT numIndices = 0;
  UINT numGroups = 0;
  UINT numMatrices = 0;
  UINT numTexChannels = geosets[geosetIds[0]].texCoord.Count();
  UINT index;

  newGeoset->vertexShader = geosets[geosetIds[0]].vertexShader;
  newGeoset->materialId = geosets[geosetIds[0]].materialId;
  newGeoset->geosetId = geosetId;

  for (index = 0; index < numGeosets; ++index) {
    numVertices += geosets[geosetIds[index]].position.Count();
    numIndices += geosets[geosetIds[index]].primitiveVertices.Count();
    numGroups += geosets[geosetIds[index]].groupMatrixCounts.Count();
    numMatrices += geosets[geosetIds[index]].matrices.Count();
    if (geosets[geosetIds[index]].vertexShader > newGeoset->vertexShader) {
      newGeoset->vertexShader = geosets[geosetIds[index]].vertexShader;
    }
    ASSERT(geosets[geosetIds[index]].texCoord.Count() == numTexChannels);
  }

  newGeoset->position.SetCount(numVertices);
  newGeoset->boneWeights.SetCount(numVertices);
  newGeoset->normal.SetCount(numVertices);
  newGeoset->texCoord.SetCount(numTexChannels);
  for (index = 0; index < numTexChannels; ++index) {
    newGeoset->texCoord[index].SetCount(numVertices);
  }
  newGeoset->primitive.SetCount(1);
  newGeoset->primitiveVertices.SetCount(numIndices);
  newGeoset->groupMatrixCounts.SetCount(numGroups);
  newGeoset->matrices.SetCount(numMatrices);

  UINT             vertsAdded = 0;
  UINT             indicesAdded = 0;
  CMatrixGroupTree matrixGroupSets;
  newGeoset->centroid.x = 0.0f;
  newGeoset->centroid.y = 0.0f;
  newGeoset->centroid.z = 0.0f;
  for (index = 0; index < numGeosets; ++index) {
    memcpy(
        newGeoset->position.Ptr() + vertsAdded, geosets[geosetIds[index]].position.Ptr(),
        geosets[geosetIds[index]].position.Count() * sizeof(NTempest::C3Vector)
    );
    memcpy(
        newGeoset->normal.Ptr() + vertsAdded, geosets[geosetIds[index]].normal.Ptr(),
        geosets[geosetIds[index]].position.Count() * sizeof(NTempest::C3Vector)
    );
    for (UINT texChannel = 0; texChannel < numTexChannels; ++texChannel) {
      TSFixedArray<NTempest::C2Vector> *dstTexLayer = &newGeoset->texCoord[texChannel];
      memcpy(
          dstTexLayer->Ptr() + vertsAdded, geosets[geosetIds[index]].texCoord[texChannel].Ptr(),
          geosets[geosetIds[index]].position.Count() * sizeof(NTempest::C2Vector)
      );
    }

    ASSERT(geosets[geosetIds[index]].primitive.Count() == 1);
    ASSERT(geosets[geosetIds[index]].primitive[0].type == GxPrim_Triangles);
    ASSERT(geosets[geosetIds[index]].primitive[0].vertexCount == geosets[geosetIds[index]].primitiveVertices.Count());
    newGeoset->primitive[0].vertexCount += geosets[geosetIds[index]].primitive[0].vertexCount;
    WORD *srcIndex = geosets[geosetIds[index]].primitiveVertices.Ptr();
    for (UINT primitiveIndex = 0; primitiveIndex < geosets[geosetIds[index]].primitiveVertices.Count(); ++primitiveIndex) {
      newGeoset->primitiveVertices[indicesAdded + primitiveIndex] = static_cast<WORD>(vertsAdded + *srcIndex++);
    }

    AddGeosetMatrixGroups(&matrixGroupSets, &geosets[geosetIds[index]], newGeoset, vertsAdded);

    newGeoset->centroid += geosets[geosetIds[index]].centroid;
    vertsAdded += geosets[geosetIds[index]].position.Count();
    indicesAdded += geosets[geosetIds[index]].primitiveVertices.Count();
  }

  newGeoset->centroid = newGeoset->centroid * (1.0f / static_cast<float>(numGeosets));
  newGeoset->groupMatrixCounts.SetCount(matrixGroupSets.GroupCount());
  newGeoset->matrices.SetCount(matrixGroupSets.MatrixCount());
}
static void IModelOptimizeVisibleGeosets(CModelComplex *unique, CModelShared *shared) {
  UINT numGeosets = shared->numGeosets;
  unique->m_geosets.SetCount(numGeosets);
  unique->m_geosetColor.SetCount(numGeosets);
  unique->m_addlGeosets.SetCount(0);

  UINT                                 numMaterials = unique->m_materials.Count();
  TSStackArray<TSGrowableArray<UINT> > geosetIDsPerMaterial(_alloca(numMaterials * sizeof(TSGrowableArray<UINT>)), numMaterials, numMaterials);

  UINT index;
  for (index = 0; index < numGeosets; ++index) {
    if (!(unique->m_geosets[index].flags & 1)) {
      UINT materialId = shared->geosets[index].materialId;
      ASSERT(materialId < numMaterials);
      geosetIDsPerMaterial[materialId].Add(1, &index);
    }
  }

  UINT numBatches = 0;
  for (index = 0; index < numMaterials; ++index) {
    if (geosetIDsPerMaterial[index].Count() > 1) {
      ++numBatches;
    }
  }

  unique->m_geosetColor.SetCount(numGeosets + numBatches);
  unique->m_geosets.SetCount(numGeosets + numBatches);
  unique->m_addlGeosets.SetCount(numBatches);

  UINT geosetId = numGeosets;
  for (index = 0; index < numMaterials; ++index) {
    TSGrowableArray<UINT> &ids = geosetIDsPerMaterial[index];
    if (ids.Count() > 1) {
      BuildCompositeGeoset(&unique->m_addlGeosets[geosetId - numGeosets], geosetId, shared->geosets.Ptr(), ids);
      ++geosetId;

      for (UINT idIndex = 0; idIndex < ids.Count(); ++idIndex) {
        unique->m_geosets[ids[idIndex]].flags |= 1;
      }
    }
  }
  ASSERT(unique->m_geosets.Count() == unique->m_geosetColor.Count());
}

BOOL ModelOptimizeVisibleGeosets(HMODEL model) {
  FATALASSERT(model);

  CModelBase   *unique;
  CModelShared *shared;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique, &shared)) {
    EnqueueModelCommand(reinterpret_cast<CModel *>(model), MODEL_OPTIMIZE_VISIBLE_GEOSETS);
    return 1;
  }

  if (!(unique->m_flags & 0x20)) {
    return 0;
  }

  IModelOptimizeVisibleGeosets(static_cast<CModelComplex *>(unique), shared);
  return 1;
}

BYTE ModelGetVertexAlpha(HMODEL model) {
  CModelBase *unique;

  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    return 0xFF;
  }

  if (unique->m_flags & 0x20) {
    CModelComplex *complex = static_cast<CModelComplex *>(unique);
    if (!complex->m_geosetColor.Count()) {
      return 0xFF;
    }

    return NTempest::CMath::ftol_0_256_(complex->m_geosetColor[0].proceduralAlpha * 255.0f);
  }

  CModelSimple *simple = static_cast<CModelSimple *>(unique);
  if (!simple->m_geosetColor.Count()) {
    return 0xFF;
  }

  return NTempest::CMath::ftol_0_256_(simple->m_geosetColor[0].proceduralAlpha * 255.0f);
}

static void GeosetSetVertexAlpha(CGeosetShared *geoShared, CGeosetColor *geoColor, HMATERIAL *materials, float alpha) {
  CMaterial       *uniqueMtl;
  CMaterialShared *sharedMtl;
  BYTE             finalAlpha;

  ASSERT(geoShared);

  if (NTempest::CMath::fabs_(geoColor->proceduralAlpha - alpha) < 0.00000095367432f) {
    return;
  }

  geoColor->proceduralAlpha = alpha;
  finalAlpha = NTempest::CMath::ftol_0_256_(alpha * geoColor->animatedAlpha * 255.0f);
  geoColor->proceduralColor.a = finalAlpha;

  UINT materialId = geoShared->materialId;
  uniqueMtl = reinterpret_cast<CMaterial *>(materials[materialId]);
  ASSERT(uniqueMtl);

  sharedMtl = reinterpret_cast<CMaterialShared *>(uniqueMtl->data);
  ASSERT(sharedMtl);

  if (!MaterialUsedOnce(materials[materialId])) {
    HMATERIAL duplicate = MaterialDuplicate(materials[materialId]);
    HandleClose(materials[materialId]);
    materials[materialId] = duplicate;
    uniqueMtl = reinterpret_cast<CMaterial *>(duplicate);
    ASSERT(uniqueMtl);
  }

  CTexLayer *uniqueLayer = uniqueMtl->layers.Ptr();
  UINT       numLayers = uniqueMtl->layers.Count();
  if (finalAlpha && finalAlpha != 255) {
    for (UINT i = 0; i < numLayers; ++i) {
      if (uniqueLayer[i].blendMode < GxBlend_Alpha) {
        uniqueLayer[i].blendMode = GxBlend_Alpha;
      }
    }
  } else {
    const CTexLayerShared *sharedLayer = sharedMtl->layers.Ptr();
    for (UINT i = 0; i < numLayers; ++i) {
      uniqueLayer[i].blendMode = sharedLayer[i].blendMode;
    }
  }
}

static void IModelSetVertexAlpha(CModelSimple *unique, CModelShared *shared, BYTE alpha) {
  ASSERT(unique);

  UINT  numGeosets = unique->m_geosets.Count();
  float fAlpha = static_cast<float>(alpha) * 0.0039215689f;
  for (UINT i = 0; i < numGeosets; ++i) {
    GeosetSetVertexAlpha(&shared->geosets[i], &unique->m_geosetColor[i], unique->m_materials.Ptr(), fAlpha);
  }
}

static void IModelSetVertexAlpha(CModelComplex *unique, CModelShared *shared, BYTE alpha) {
  ASSERT(unique);

  float fAlpha = static_cast<float>(alpha) * 0.0039215689f;
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

  FATALASSERT(model);

  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique, &shared)) {
    EnqueueModelCommand(reinterpret_cast<CModel *>(model), MODEL_SET_VERTEX_ALPHA, alpha, doLinkedModels);
    return;
  }

  if (!(unique->m_flags & 0x20)) {
    IModelSetVertexAlpha(static_cast<CModelSimple *>(unique), shared, alpha);
    return;
  }

  CModelComplex *complex = static_cast<CModelComplex *>(unique);
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

  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    return;
  }

  if (unique->m_flags & 0x20) {
    CModelComplex *complex = static_cast<CModelComplex *>(unique);
    if (!complex->m_geosetColor.Count()) {
      return;
    }

    red = complex->m_geosetColor[0].proceduralColor.r;
    green = complex->m_geosetColor[0].proceduralColor.g;
    blue = complex->m_geosetColor[0].proceduralColor.b;
    return;
  }

  CModelSimple *simple = static_cast<CModelSimple *>(unique);
  if (!simple->m_geosetColor.Count()) {
    return;
  }

  red = simple->m_geosetColor[0].proceduralColor.r;
  green = simple->m_geosetColor[0].proceduralColor.g;
  blue = simple->m_geosetColor[0].proceduralColor.b;
}

static void GeosetSetVertexColor(CGeosetShared *geoShared, CGeosetColor *geoColor, HMATERIAL *materials, BYTE red, BYTE green, BYTE blue) {
  ASSERT(geoShared);

  UINT materialId = geoShared->materialId;
  if (!MaterialUsedOnce(materials[materialId])) {
    HMATERIAL duplicate = MaterialDuplicate(materials[materialId]);
    HandleClose(materials[materialId]);
    materials[materialId] = duplicate;
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

  FATALASSERT(model);

  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique, &shared)) {
    EnqueueModelCommand(reinterpret_cast<CModel *>(model), MODEL_SET_VERTEX_COLOR, red, green, blue, doLinkedModels);
    return;
  }

  if (!(unique->m_flags & 0x20)) {
    IModelSetVertexColor(static_cast<CModelSimple *>(unique), shared, red, green, blue);
    return;
  }

  CModelComplex *complex = static_cast<CModelComplex *>(unique);
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

void GeosetShowUnselectable(CGeosetShared *geoShared, CGeosetColor *geoColor, HMATERIAL *materials, BYTE red, BYTE green, BYTE blue) {
  ASSERT(geoShared);

  UINT      materialId = geoShared->materialId;
  HMATERIAL material = materials[materialId];
  if ((geoShared->flags & 3) == 1) {
    geoShared->flags |= 2;
    if (!MaterialUsedOnce(material)) {
      HMATERIAL duplicate = MaterialDuplicate(material);
      HandleClose(material);
      materials[materialId] = duplicate;
    }

    geoColor->proceduralColor.r = red;
    geoColor->proceduralColor.g = green;
    geoColor->proceduralColor.b = blue;
  }
}

void ModelShowUnselectable(HMODEL model, BYTE red, BYTE green, BYTE blue) {
  CModelBase   *unique;
  CModelShared *shared;
  UINT          i;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique, &shared)) {
    return;
  }

  if (unique->m_flags & 0x20) {
    CModelComplex *complex = static_cast<CModelComplex *>(unique);
    for (i = 0; i < shared->numGeosets; ++i) {
      GeosetShowUnselectable(&shared->geosets[i], &complex->m_geosetColor[i], complex->m_materials.Ptr(), red, green, blue);
    }

    UINT numAttachments = complex->m_attached.Count();
    for (i = 0; i < numAttachments; ++i) {
      ITERATELIST(LINKUNIQUE, complex->m_attached[i], link) {
        ModelShowUnselectable(link->child, red, green, blue);
      }
    }
  } else {
    CModelSimple *simple = static_cast<CModelSimple *>(unique);
    for (i = 0; i < shared->numGeosets; ++i) {
      GeosetShowUnselectable(&shared->geosets[i], &simple->m_geosetColor[i], simple->m_materials.Ptr(), red, green, blue);
    }
  }
}

void GeosetHideUnselectable(CGeosetShared *geoShared, CGeosetColor *geoColor, HMATERIAL *materials) {
  ASSERT(geoShared);

  UINT      materialId = geoShared->materialId;
  HMATERIAL material = materials[materialId];
  if ((geoShared->flags & 3) == 3) {
    geoShared->flags &= ~2U;
    if (!MaterialUsedOnce(material)) {
      HMATERIAL duplicate = MaterialDuplicate(material);
      HandleClose(material);
      materials[materialId] = duplicate;
    }

    geoColor->proceduralColor.r = 0xFF;
    geoColor->proceduralColor.g = 0xFF;
    geoColor->proceduralColor.b = 0xFF;
  }
}

void ModelHideUnselectable(HMODEL model) {
  CModelBase   *unique;
  CModelShared *shared;
  UINT          i;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique, &shared)) {
    return;
  }

  ASSERT(shared);
  if (unique->m_flags & 0x20) {
    CModelComplex *complex = static_cast<CModelComplex *>(unique);
    for (i = 0; i < shared->numGeosets; ++i) {
      GeosetHideUnselectable(&shared->geosets[i], &complex->m_geosetColor[i], complex->m_materials.Ptr());
    }

    UINT numAttachments = complex->m_attached.Count();
    for (i = 0; i < numAttachments; ++i) {
      ITERATELIST(LINKUNIQUE, complex->m_attached[i], link) {
        ModelHideUnselectable(link->child);
      }
    }
  } else {
    CModelSimple *simple = static_cast<CModelSimple *>(unique);
    for (i = 0; i < shared->numGeosets; ++i) {
      GeosetHideUnselectable(&shared->geosets[i], &simple->m_geosetColor[i], simple->m_materials.Ptr());
    }
  }
}

BOOL GeosetIsShowingUnselectable(CGeosetShared *geosets, UINT numGeosets) {
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
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique, &shared)) {
    return 0;
  }

  if (GeosetIsShowingUnselectable(shared->geosets.Ptr(), shared->numGeosets)) {
    return 1;
  }

  if (unique->m_flags & 0x20) {
    CModelComplex *complex = static_cast<CModelComplex *>(unique);
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

  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &pModel)) {
    return;
  }

  if (!(pModel->m_flags & 0x20)) {
    return;
  }

  CModelComplex *complex = static_cast<CModelComplex *>(pModel);
  UINT           count = complex->m_lights.Count();
  for (UINT i = 0; i < count; ++i) {
    GxuLightEnableSet(complex->m_lights[i], enable);
  }
}

UINT ModelGetNumLights(HMODEL model) {
  CModelBase *pModel;

  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &pModel) || !(pModel->m_flags & 0x20)) {
    return 0;
  }

  return static_cast<CModelComplex *>(pModel)->m_lights.Count();
}

const CGxLight *ModelGetLight(HMODEL model, UINT index) {
  CModelBase *pModel;

  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &pModel)) {
    return 0;
  }

  ASSERT(pModel->m_flags & 0x20);
  CModelComplex *complex = static_cast<CModelComplex *>(pModel);
  return GxuLightLock(complex->m_lights[index]);
}

void ModelSetLightSelectCallback(
    HMODEL model,
    void (*callback)(LPVOID, NTempest::C3Vector, const NTempest::C3Vector &, UINT),
    LPVOID parm,
    int    doLinkedModels
) {
  CModelBase *unique;

  FATALASSERT(model);

  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    EnqueueModelCommand(reinterpret_cast<CModel *>(model), MODEL_SET_LIGHT_SELECT_CALLBACK, callback, parm, doLinkedModels);
    return;
  }

  unique->m_PickLights = callback;
  unique->m_pickLightsParm = parm;

  if (!(unique->m_flags & 0x20) || !doLinkedModels) {
    return;
  }

  CModelComplex *complex = static_cast<CModelComplex *>(unique);
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
  FATALASSERT(custGeosetId);

  CModelBase *unique;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    return;
  }

  CCustomGeoset *geoset;
  if (unique->m_flags & 0x20) {
    CModelComplex *complex = static_cast<CModelComplex *>(unique);
    *custGeosetId = complex->m_custGeosets.Count();
    geoset = complex->m_custGeosets.New();
  } else {
    CModelSimple *simple = static_cast<CModelSimple *>(unique);
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

  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    return;
  }

  if (unique->m_flags & 0x20) {
    static_cast<CModelComplex *>(unique)->m_custGeosets[custGeosetId].position = modelSpacePosition;
  } else {
    static_cast<CModelSimple *>(unique)->m_custGeosets[custGeosetId].position = modelSpacePosition;
  }
}

void ModelCustGeosetRemove(HMODEL model, UINT custGeosetId) {
  CModelBase *unique;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    return;
  }

  if (unique->m_flags & 0x20) {
    ASSERT(custGeosetId < static_cast<CModelComplex *>(unique)->m_custGeosets.Count());
    memmove(
        &static_cast<CModelComplex *>(unique)->m_custGeosets[custGeosetId],
        static_cast<CModelComplex *>(unique)->m_custGeosets.Ptr() + custGeosetId + 1,
        (static_cast<CModelComplex *>(unique)->m_custGeosets.Count() - custGeosetId - 1) * sizeof(CCustomGeoset)
    );
    static_cast<CModelComplex *>(unique)->m_custGeosets.SetCount(static_cast<CModelComplex *>(unique)->m_custGeosets.Count() - 1);
  } else {
    ASSERT(custGeosetId < static_cast<CModelSimple *>(unique)->m_custGeosets.Count());
    memmove(
        &static_cast<CModelSimple *>(unique)->m_custGeosets[custGeosetId],
        static_cast<CModelSimple *>(unique)->m_custGeosets.Ptr() + custGeosetId + 1,
        (static_cast<CModelSimple *>(unique)->m_custGeosets.Count() - custGeosetId - 1) * sizeof(CCustomGeoset)
    );
    static_cast<CModelSimple *>(unique)->m_custGeosets.SetCount(static_cast<CModelSimple *>(unique)->m_custGeosets.Count() - 1);
  }
}

void ModelEnumAnimObjects(HMODEL model, int (*callbackfcn)(UINT, LPCSTR, LPVOID), LPVOID param) {
  CModelBase *unique;

  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) && unique->m_anim) {
    AnimEnumObjects(unique->m_anim, callbackfcn, param);
  }
}

int ModelGetSequenceTime(HMODEL model, UINT seqIndex) {
  CModelBase *unique;

  if (IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) && unique->m_anim) {
    return AnimGetSequenceTime(unique->m_anim, seqIndex);
  }
  return 0;
}

UINT ModelGetPrimarySequence(HMODEL model) {
  CModelBase *unique;
  UINT        sequence = 0;

  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) || !unique->m_anim) {
    return 0;
  }

  AnimGetPrimarySequence(unique->m_anim, &sequence);
  return sequence;
}

void ModelEnableAnimBlending(HMODEL model, int enabled) {
  CModelBase *unique;

  FATALASSERT(model);

  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    EnqueueModelCommand(reinterpret_cast<CModel *>(model), MODEL_ENABLE_ANIM_BLENDING, enabled);
    return;
  }

  if (unique->m_anim) {
    AnimEnableBlending(unique->m_anim, enabled);
  }
}

BOOL ModelUsesBlending(HMODEL model) {
  CModelBase *unique;

  return IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) && unique->m_anim && AnimUsesBlending(unique->m_anim);
}

void ModelEnableEmitters(HMODEL model, int enable, int doLinkedModels) {
  UINT           numElements;
  CModelComplex *complex;
  CModelBase    *unique;

  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    return;
  }

  ASSERT(unique->m_flags & 0x20);
  complex = static_cast<CModelComplex *>(unique);

  numElements = complex->m_emitters2.Count();
  for (UINT i = 0; i < numElements; ++i) {
    complex->m_emitters2[i]->SetEnabled2(enable, 1);
  }

  if (doLinkedModels) {
    for (UINT i = 0; i < complex->m_attached.Count(); ++i) {
      ITERATELIST(LINKUNIQUE, complex->m_attached[i], link) {
        ModelEnableEmitters(link->child, enable, 1);
      }
    }
  }
}

void ModelEnableRibbons(HMODEL model, int enable) {
  CModelBase *unique;

  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    return;
  }

  ASSERT(unique->m_flags & 0x20);
  for (UINT i = 0; i < static_cast<CModelComplex *>(unique)->m_ribbons.Count(); ++i) {
    static_cast<CModelComplex *>(unique)->m_ribbons[i]->SetEnabled(enable);
  }
}

void IModelEnableFullAlpha(CModelBase *unique, int enable) {
  HMATERIAL *materials;
  UINT       numMaterials;
  EGxBlend   alphaOp;

  ASSERT(unique);

  alphaOp = enable ? GxBlend_Alpha : GxBlend_AlphaKey;
  if (unique->m_flags & 0x20) {
    CModelComplex *complex = static_cast<CModelComplex *>(unique);
    materials = complex->m_materials.Ptr();
    numMaterials = complex->m_materials.Count();
  } else {
    CModelSimple *simple = static_cast<CModelSimple *>(unique);
    materials = simple->m_materials.Ptr();
    numMaterials = simple->m_materials.Count();
  }

  for (UINT i = 0; i < numMaterials; ++i) {
    CMaterial *materialUnique = reinterpret_cast<CMaterial *>(materials[i]);
    ASSERT(materialUnique);

    CMaterialShared *materialShared = reinterpret_cast<CMaterialShared *>(materialUnique->data);
    ASSERT(materialShared);

    CTexLayer       *uniqueLayers = materialUnique->layers.Ptr();
    CTexLayerShared *sharedLayers = materialShared->layers.Ptr();
    UINT             numLayers = materialShared->layers.Count();
    for (UINT j = 0; j < numLayers; ++j) {
      if (sharedLayers[j].blendMode == GxBlend_Alpha) {
        uniqueLayers[j].blendMode = alphaOp;
      }
    }
  }
}

void ModelEnableFullAlpha(HMODEL model, int enable) {
  CModelBase *unique;

  FATALASSERT(model);

  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    EnqueueModelCommand(reinterpret_cast<CModel *>(model), MODEL_ENABLE_FULL_ALPHA, enable);
    return;
  }

  IModelEnableFullAlpha(unique, enable);
}

BOOL ModelAnimHasObjectId(HMODEL model, UINT objectId) {
  CModelBase *unique;

  return IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique) && unique->m_anim && AnimHasObjectId(unique->m_anim, objectId);
}

static void IModelSetMaterialDisables(HMATERIAL__ **materials, UINT numMaterials, UINT setMask, UINT unsetMask) {
  UINT i;
  UINT numLayers;
  while (numMaterials--) {
    CMaterial *uniqueMtl = reinterpret_cast<CMaterial *>(*materials++);
    FATALASSERT(uniqueMtl);

    numLayers = uniqueMtl->layers.Count();
    for (i = 0; i < numLayers; ++i) {
      DWORD &disables = uniqueMtl->layers[i].disables;
      if (setMask & 0x01)
        disables |= 0x01;
      if (setMask & 0x10)
        disables |= 0x10;
      if (setMask & 0x20)
        disables |= 0x02;
      if (setMask & 0x40)
        disables |= 0x04;
      if (setMask & 0x80)
        disables |= 0x08;
      if (unsetMask & 0x01)
        disables &= ~0x01U;
      if (unsetMask & 0x10)
        disables &= ~0x10U;
      if (unsetMask & 0x20)
        disables &= ~0x02U;
      if (unsetMask & 0x40)
        disables &= ~0x04U;
      if (unsetMask & 0x80)
        disables &= ~0x08U;
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

  for (i = 0; i < unique->m_emitters2.Count(); ++i) {
    if (setMask & 0x01)
      unique->m_emitters2[i]->MaterialDisableLight(1);
    if (setMask & 0x20)
      unique->m_emitters2[i]->MaterialDisableFog(1);
    if (unsetMask & 0x01)
      unique->m_emitters2[i]->MaterialDisableLight(0);
    if (unsetMask & 0x20)
      unique->m_emitters2[i]->MaterialDisableFog(0);
  }

  for (i = 0; i < unique->m_ribbons.Count(); ++i) {
    if (setMask & 0x01)
      unique->m_ribbons[i]->MaterialDisableLight(1);
    if (setMask & 0x20)
      unique->m_ribbons[i]->MaterialDisableFog(1);
    if (unsetMask & 0x01)
      unique->m_ribbons[i]->MaterialDisableLight(0);
    if (unsetMask & 0x20)
      unique->m_ribbons[i]->MaterialDisableFog(0);
  }

  for (i = 0; i < unique->m_attached.Count(); ++i) {
    ITERATELIST(LINKUNIQUE, unique->m_attached[i], link) {
      ModelSetMaterialDisables(link->child, setMask, unsetMask, 1);
    }
  }
}

void ModelSetMaterialDisables(HMODEL__ *model, UINT setMask, UINT unsetMask, int doLinkedModels) {
  CModelBase *unique;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    return;
  }

  if (unique->m_flags & 0x20) {
    ComplexModelSetMaterialDisables(static_cast<CModelComplex *>(unique), setMask, unsetMask, doLinkedModels);
  } else {
    CModelSimple *simple = static_cast<CModelSimple *>(unique);
    IModelSetMaterialDisables(simple->m_materials.Ptr(), simple->m_materials.Count(), setMask, unsetMask);
  }
}

UINT ModelGetNumTextures(HMODEL model) {
  CModelBase *unique;

  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    return 0;
  }

  if (unique->m_flags & 0x20) {
    return static_cast<CModelComplex *>(unique)->m_textures.Count();
  }

  return static_cast<CModelSimple *>(unique)->m_textures.Count();
}

UINT ModelGetTextureReplaceableId(HMODEL model, UINT textureId) {
  CModelBase *unique;

  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &unique)) {
    return 0;
  }

  if (unique->m_flags & 0x20) {
    ASSERT(textureId < static_cast<CModelComplex *>(unique)->m_textures.Count());
    return static_cast<CModelComplex *>(unique)->m_textures[textureId].replaceableId;
  }

  ASSERT(textureId < static_cast<CModelSimple *>(unique)->m_textures.Count());
  return static_cast<CModelSimple *>(unique)->m_textures[textureId].replaceableId;
}
