#include "IModel.h"
#include "ModelInternal.h"
#include "CollisionData.h"

#include "Base/Status.h"
#include "MDLFile/MDLTypes.h"
#include "Services/AsyncFileRead.h"
#include "Services/Camera.h"
#include "Services/IParticleMisc.h"
#include "Services/ParticleSystem2.h"
#include "Services/Texture.h"
#include "Os/W32/Debugging.h"
#include "Os/OsTime.h"

#include <stpl.h>
#include <malloc.h>
#include <stdarg.h>

namespace NTempest {

  inline void CAaBox::Set(const C3Vector &value) {
    b = value;
    t = value;
  }

  inline CAaBox CAaBox::Union(const CAaBox &a, const CAaBox &b) {
    return CAaBox(C3Vector::Min(a.b, b.b), C3Vector::Max(a.t, b.t));
  }

  inline CAaBox CAaBox::Unite(const CAaBox &value) {
    *this = Union(*this, value);
    return *this;
  }

}

using namespace NTempest;

LPCSTR CHandleObject::GetObjectName() {
  return 0;
}

void           AnimInitialize();
void           AnimDestroy();
void           MDLFileInitialize();
void           MDLFileDestroy();
BOOL           MDLFileRead(LPCSTR path, MDLDATA *mdldata, CStatus *status);
BOOL           MdlReadValidate(const MDLDATA &data, CStatus *status);
HMODEL         ModelCreate(const MDLDATA &source, CModelCreate *data, CStatus *status);
void           ModelAnimateInitialize();
void           ModelAnimateDestroy();
void           ModelRenderInitialize();
void           ModelRenderDestroy();
BYTE          *MDLFileBinarySeek(BYTE *fileData, UINT fileBytes, DWORD sectionTag);
BYTE          *MDLFileBinaryLoad(char *path, UINT *fileBytes, CStatus *status);
void           MDLFileBinaryUnload(BYTE *fileData);
HTEXTURE       LoadModelTexture(LPCSTR texturePath, UINT modelLoadFlags, CGxTexFlags texLoadFlags, CStatus *status);
void           AnimSetObjectOrdering(HANIM anim, LPCSTR *boneNames, UINT numBones);
void           AnimSetSequenceOrdering(HANIM anim, LPCSTR *sequenceNames, UINT numSequences);
void           AnimSetSequenceOrderingDefault(HANIM anim);
void           MdxReadTextures(BYTE *, UINT, UINT, CModelComplex *, CStatus *);
void           MdxReadTextures(BYTE *, UINT, UINT, CModelSimple *, CStatus *);
void           MdxReadMaterials(BYTE *, UINT, UINT, CModelComplex *, CModelShared *);
void           MdxReadMaterials(BYTE *, UINT, UINT, CModelSimple *, CModelShared *);
void           MdxReadGeosets(BYTE *, UINT, UINT, CModelComplex *, CModelShared *);
void           MdxReadGeosets(BYTE *, UINT, UINT, CModelSimple *, CModelShared *);
void           MdxLoadGlobalProperties(BYTE *, UINT, UINT *, CModelShared *);
void           MdxReadAttachments(BYTE *, UINT, UINT, CModelComplex *, CModelShared *, CStatus *);
void           MdxReadRibbonEmitters(BYTE *, UINT, CModelComplex *, CModelShared *);
void           MdxReadEmitters2(BYTE *, UINT, UINT, CModelComplex *, CModelShared *, CStatus *);
void           MdxReadLights(BYTE *, UINT, CModelComplex *);
HCOLLISIONDATA CollisionDataCreate(BYTE *, UINT);
HCOLLISIONDATA CollisionDataCreate(const MDLDATA &);
HANIM          AnimCreate(const MDLDATA &, UINT, CStatus *);
UINT           AnimBuildObjectIdTranslation(const MDLDATA &, UINT, TSStackArray<UINT> *);
BOOL           MdlReadCameras(const MDLDATA &, TSFixedArray<HCAMERA> *);
void           MdlReadLoadGlobalProperties(const MDLDATA &, CModelShared *, UINT *);
BOOL           MdlReadLoadModel(const MDLDATA &, CModelComplex *, CModelShared *, UINT, CStatus *);
BOOL           MdlReadLoadModel(const MDLDATA &, CModelSimple *, CModelShared *, UINT, CStatus *);
BOOL           MdlReadLoadRibbonEmitters(const MDLDATA &, CModelComplex *, CModelShared *);
BOOL           MdlReadLoadEmitters2(const MDLDATA &, CModelComplex *, CModelShared *, UINT, CStatus *);
BOOL           MdlReadLoadLights(const MDLDATA &, CModelComplex *);
void           ExecuteQueuedActions(CModel *model);
void           IModelEnableFullAlpha(CModelBase *unique, int enable);

void ModelClearAllLinks(HMODEL parent);
void ModelEnableAnimBlending(HMODEL model, int enabled);
void ModelHideBounds(HMODEL model);
void ModelHideGeosets(HMODEL model, UINT selectionGroup, int hide);
void ModelHideGeosetsRange(HMODEL model, UINT selectionStart, UINT selectionEnd, int hide);
BOOL ModelOptimizeVisibleGeosets(HMODEL model);
BOOL ModelRemoveLink(HMODEL parent, UINT parentIndex, HMODEL child);
void ModelSetEmissiveColor(HMODEL model, const NTempest::CImVector &color, int doLinkedModels);
void ModelShowCollision(HMODEL model, int show);
void ModelShowCollisionAaBox(HMODEL model, int show);
void ModelShowModel(HMODEL model, int show);

class CHashKeyFilePath {
 public:
  char path[MAX_PATH];
  CHashKeyFilePath() {
    path[0] = 0;
  }

  CHashKeyFilePath(LPCSTR value) {
    SStrCopy(path, value, sizeof(path));
  }

  CHashKeyFilePath(const CHashKeyFilePath &source) {
    SStrCopy(path, source.path, sizeof(path));
  }

  CHashKeyFilePath &operator=(LPCSTR value) {
    SStrCopy(path, value, sizeof(path));
    return *this;
  }

  CHashKeyFilePath &operator=(const CHashKeyFilePath &source) {
    SStrCopy(path, source.path, sizeof(path));
    return *this;
  }

  bool operator==(const CHashKeyFilePath &source) const {
    return operator==(source.path);
  }

  bool operator==(LPCSTR value) const {
    return SStrCmpI(path, value, 0x7FFFFFFF) == 0;
  }

};

struct CModelHash : public TSHashObject<CModelHash, CHashKeyFilePath> {
  CModelHash() : model(0), createFlags(0), timeStamp(0) {
  }

  ~CModelHash() {
    if (model) {
      HandleClose(model);
    }
  }

  HMODEL model;
  UINT   createFlags;
  DWORD  timeStamp;
  LINKDECLEX(CModelHash, link);
};

MDLBASE::MDLBASE() : version(0) {
}

static EModelParamType s_modelParamTypes[MODEL_NUM_COMMANDS][4] = {
    {  MPARAM_UINT,   MPARAM_HANDLE, MPARAM_FLOAT, MPARAM_NONE},
    {  MPARAM_UINT, MPARAM_C3VECTOR,  MPARAM_NONE, MPARAM_NONE},
    {  MPARAM_UINT, MPARAM_C3VECTOR,  MPARAM_NONE, MPARAM_NONE},
    {  MPARAM_NONE,     MPARAM_NONE,  MPARAM_NONE, MPARAM_NONE},
    {  MPARAM_UINT,     MPARAM_NONE,  MPARAM_NONE, MPARAM_NONE},
    {  MPARAM_BOOL,     MPARAM_NONE,  MPARAM_NONE, MPARAM_NONE},
    {  MPARAM_BOOL,     MPARAM_NONE,  MPARAM_NONE, MPARAM_NONE},
    {MPARAM_HANDLE,     MPARAM_NONE,  MPARAM_NONE, MPARAM_NONE},
    {  MPARAM_UINT,     MPARAM_BOOL,  MPARAM_NONE, MPARAM_NONE},
    {  MPARAM_UINT,     MPARAM_UINT,  MPARAM_BOOL, MPARAM_NONE},
    {  MPARAM_NONE,     MPARAM_NONE,  MPARAM_NONE, MPARAM_NONE},
    {  MPARAM_UINT,     MPARAM_BOOL,  MPARAM_NONE, MPARAM_NONE},
    {  MPARAM_UINT,     MPARAM_UINT,  MPARAM_BOOL, MPARAM_NONE},
    {  MPARAM_UINT,     MPARAM_BOOL,  MPARAM_NONE, MPARAM_NONE},
    {  MPARAM_UINT,     MPARAM_NONE,  MPARAM_NONE, MPARAM_NONE},
    {  MPARAM_UINT,     MPARAM_UINT,  MPARAM_UINT, MPARAM_NONE},
    {  MPARAM_NONE,     MPARAM_NONE,  MPARAM_NONE, MPARAM_NONE},
    {  MPARAM_UINT,   MPARAM_HANDLE,  MPARAM_NONE, MPARAM_NONE},
    {  MPARAM_UINT,     MPARAM_NONE,  MPARAM_NONE, MPARAM_NONE},
    {  MPARAM_UINT,     MPARAM_NONE,  MPARAM_NONE, MPARAM_NONE},
    {  MPARAM_UINT,   MPARAM_HANDLE,  MPARAM_BOOL, MPARAM_NONE},
    { MPARAM_CARGB,     MPARAM_BOOL,  MPARAM_NONE, MPARAM_NONE},
    {   MPARAM_PTR,      MPARAM_PTR,  MPARAM_BOOL, MPARAM_NONE},
    {   MPARAM_PTR,      MPARAM_PTR,  MPARAM_BOOL, MPARAM_NONE},
    {  MPARAM_UINT,    MPARAM_FLOAT,  MPARAM_BOOL, MPARAM_NONE},
    {  MPARAM_UINT,     MPARAM_UINT,  MPARAM_NONE, MPARAM_NONE},
    {  MPARAM_UINT,     MPARAM_UINT,  MPARAM_UINT, MPARAM_NONE},
    {   MPARAM_PTR,      MPARAM_PTR,  MPARAM_NONE, MPARAM_NONE},
    {  MPARAM_UINT,      MPARAM_PTR,   MPARAM_PTR, MPARAM_NONE},
    {  MPARAM_UINT,     MPARAM_UINT,  MPARAM_NONE, MPARAM_NONE},
    {  MPARAM_UINT,     MPARAM_UINT,  MPARAM_UINT, MPARAM_NONE},
    {  MPARAM_UINT,     MPARAM_UINT,  MPARAM_UINT, MPARAM_NONE},
    {  MPARAM_UINT,     MPARAM_UINT,  MPARAM_UINT, MPARAM_UINT},
    { MPARAM_FLOAT,     MPARAM_BOOL,  MPARAM_NONE, MPARAM_NONE},
    {  MPARAM_BYTE,     MPARAM_BOOL,  MPARAM_NONE, MPARAM_NONE},
    {  MPARAM_BYTE,     MPARAM_BYTE,  MPARAM_BYTE, MPARAM_BOOL},
    {  MPARAM_NONE,     MPARAM_NONE,  MPARAM_NONE, MPARAM_NONE},
    {  MPARAM_BOOL,     MPARAM_NONE,  MPARAM_NONE, MPARAM_NONE},
    {  MPARAM_BOOL,     MPARAM_NONE,  MPARAM_NONE, MPARAM_NONE},
    {  MPARAM_BOOL,     MPARAM_NONE,  MPARAM_NONE, MPARAM_NONE}
};

static void AsyncModelHandler();

static TSHashTableReuse<CModelHash, CHashKeyFilePath, 1> s_modelCache;
static LISTDECLEX(CModelHash, link, s_modelCacheLRU);
static CNullStatus s_nullStatus;
static LISTDECL(CModelModItem, s_freeModItems);

HMODEL ModelDuplicate(HMODEL sourceModel, UINT flags);
static HMODEL IModelCreateBlocking(LPCSTR fileName, char *actualPath, CModelCreate *data, CStatus *status);
static HMODEL CreateDefaultModel(LPCSTR fileName, UINT modelLoadFlags, CStatus *status);

static BOOL ModelIsUsed(HMODEL model) {
  CModelShared *shared;

  IModelDerefHandle((CModel *)model, &shared);
  return ((CModel *)model)->GetRefCount() > 1 || shared->GetRefCount() > 1;
}

static void ProcessAnimReorders(CModelBase *modelptr, CModelCreate *data) {
  ASSERT(modelptr->m_anim);

  if (!data) {
    AnimSetSequenceOrderingDefault(modelptr->m_anim);
    return;
  }

  if ((modelptr->m_flags & 0x20) && (data->flags & 0x40)) {
    CModelComplex *complex = (CModelComplex *)modelptr;
    AnimSetCameraOrdering(modelptr->m_anim, data->cameraNames, data->numCameras, &complex->m_cameraOrder);
  }

  if (data->flags & 0x2) {
    AnimSetSequenceOrdering(modelptr->m_anim, data->sequenceNames, data->numSequences);
  } else {
    AnimSetSequenceOrderingDefault(modelptr->m_anim);
  }

  if (data->flags & 0x4) {
    AnimSetObjectOrdering(modelptr->m_anim, data->boneNames, data->numBones);
  }
}

static HMODEL GetModel(LPCSTR modelFName, CModelCreate *data) {
  ASSERT(modelFName);
  char fileName[MAX_PATH];
  SStrCopy(fileName, modelFName, sizeof(fileName));
  char *extension = SStrChrR(fileName, '.');
  if (extension) {
    *extension = 0;
  }

  CModelHash *entry = s_modelCache.Ptr(fileName);
  if (!entry) {
    return 0;
  }

  HMODEL      duplicate = ModelDuplicate(entry->model, 0);
  CModelBase *modelptr;
  if (IModelDerefHandle((CModel *)duplicate, &modelptr) && modelptr->m_anim) {
    ProcessAnimReorders(modelptr, data);
  }
  return duplicate;
}

static void HashNewModel(LPCSTR modelFName, HMODEL model, UINT createFlags, CStatus *status) {
  char  filePath[MAX_PATH];
  DWORD currentTime;

  ASSERT(modelFName);
  currentTime = OsGetAsyncTimeMs();
  if (ModelCacheUpdate(currentTime, status)) {
    TextureCacheUpdate(currentTime, status);
  }

  SStrCopy(filePath, modelFName, sizeof(filePath));
  char *extension = SStrChrR(filePath, '.');
  if (extension) {
    *extension = 0;
  }

  CModelHash *entry = s_modelCache.New(filePath, 0, 0);
  s_modelCacheLRU.LinkNode(entry, LIST_TAIL, 0);
  entry->model = ModelDuplicate(model, 0);
  entry->createFlags = createFlags;
  entry->timeStamp = currentTime;
}

static BOOL MdlReadLoadNumMatrices(const MDLDATA &data, CModelShared *shared, UINT flags) {
  ASSERT(shared);
  if (flags & 0x100) {
    shared->numBones = 1;
    shared->numTexBones = 0;
    return 1;
  }

  shared->numBones = data.bones.Count();
  if (flags & 0x20) {
    shared->numBones += data.hitTestShapes.Count();
  }
  shared->numTexBones = data.textureanims.Count();
  return 1;
}

static void MdxReadNumMatrices(BYTE *data, UINT fileBytes, UINT flags, CModelShared *shared) {
  ASSERT(shared);

  BYTE *section = MDLFileBinarySeek(data, fileBytes, 'ENOB');
  if (!section) {
    shared->numBones = 0;
    return;
  }

  if (flags & 0x100) {
    shared->numBones = 1;
    return;
  }

  shared->numBones = *(UINT *)(section + 4);
  if (flags & 0x20) {
    section = MDLFileBinarySeek(data, fileBytes, 'TSTH');
    if (section) {
      shared->numBones += *(UINT *)(section + 4);
    }
  }

  section = MDLFileBinarySeek(data, fileBytes, 'NAXT');
  if (section) {
    shared->numTexBones = *(UINT *)(section + 4);
  }
}

static UINT ConvertAnimCreateFlags(UINT loadFlags) {
  UINT createFlags = 0;

  if (loadFlags & 0x20) {
    createFlags |= 0x1;
  }
  if (loadFlags & 0x200) {
    createFlags |= 0x2;
  }
  if (loadFlags & 0x400) {
    createFlags |= 0x4;
  }
  if (loadFlags & 0x8) {
    createFlags |= 0x8;
  }

  return createFlags;
}

static BOOL MdlReadLoadAnim(const MDLDATA &data, CModelBase *modelptr, UINT loadFlags, CStatus *status) {
  modelptr->m_anim = AnimCreate(data, ConvertAnimCreateFlags(loadFlags), status);
  if (!modelptr->m_anim) {
    return 0;
  }

  if (AnimGetFlags(modelptr->m_anim) & 0x4) {
    HandleClose(modelptr->m_anim);
    modelptr->m_anim = 0;
  }
  return 1;
}

static BOOL MdxReadAnimation(BYTE *fileData, UINT fileBytes, CModelBase *modelptr, UINT loadFlags) {
  modelptr->m_anim = AnimCreate(fileData, fileBytes, ConvertAnimCreateFlags(loadFlags));
  if (!modelptr->m_anim) {
    return 0;
  }

  if (AnimGetFlags(modelptr->m_anim) & 0x4) {
    HandleClose(modelptr->m_anim);
    modelptr->m_anim = 0;
  }
  return 1;
}

static void MdxReadHitTestData(BYTE *data, UINT fileBytes, CModelComplex *modelptr, CModelShared *shared) {
  BYTE *shapeDone;
  BYTE *dataDone;
  UINT  i;
  UINT  numShapes;

  FATALASSERT(data);
  FATALASSERT(modelptr);
  FATALASSERT(shared);

  data = MDLFileBinarySeek(data, fileBytes, 'TSTH');
  if (!data) {
    return;
  }

  dataDone = data + 4 + *(UINT *)data;
  numShapes = *(UINT *)(data + 4);
  data += 8;

  shared->hitTest.SetCount(numShapes);
  modelptr->m_hitTestMtx.SetCount(numShapes);

  for (i = 0; i < numShapes; ++i) {
    shapeDone = data + *(UINT *)data;
    data += *(UINT *)(data + 4) + 4;

    switch (*data++) {
      case COLLIDE_BOX:
        shared->hitTest[i].type = COLLIDE_BOX;
        memcpy(shared->hitTest[i].extent, data, sizeof(shared->hitTest[i].extent));
        data += sizeof(shared->hitTest[i].extent);
        break;

      case COLLIDE_CYLINDER:
        shared->hitTest[i].type = COLLIDE_CYLINDER;
        shared->hitTest[i].extent[0] = *(NTempest::C3Vector *)data;
        data += sizeof(NTempest::C3Vector);
        shared->hitTest[i].extent[1] = shared->hitTest[i].extent[0];
        shared->hitTest[i].extent[1].z += *(float *)data;
        data += sizeof(float);
        shared->hitTest[i].radius = *(float *)data;
        data += sizeof(float);
        break;

      case COLLIDE_SPHERE:
        shared->hitTest[i].type = COLLIDE_SPHERE;
        shared->hitTest[i].extent[0] = *(NTempest::C3Vector *)data;
        data += sizeof(NTempest::C3Vector);
        shared->hitTest[i].radius = *(float *)data;
        data += sizeof(float);
        break;

      case COLLIDE_PLANE:
        shared->hitTest[i].type = COLLIDE_PLANE;
        shared->hitTest[i].extent[0].x = *(float *)data;
        data += sizeof(float);
        shared->hitTest[i].extent[0].y = *(float *)data;
        data += sizeof(float);
        break;
    }

    ASSERT(shapeDone == data);
    ASSERT(dataDone >= data);
  }

  ASSERT(dataDone == data);
}

static BOOL MdlReadLoadHitTestData(const MDLDATA &data, CModelComplex *modelptr, CModelShared *shared) {
  ASSERT(modelptr);
  ASSERT(shared);

  UINT numShapes = data.hitTestShapes.Count();
  shared->hitTest.SetCount(numShapes);
  modelptr->m_hitTestMtx.SetCount(numShapes);
  for (UINT i = 0; i < numShapes; ++i) {
    switch (data.hitTestShapes[i].type) {
      case SHAPE_BOX:
        shared->hitTest[i].type = COLLIDE_BOX;
        shared->hitTest[i].extent[0] = data.hitTestShapes[i].shape.box.minimum;
        shared->hitTest[i].extent[1] = data.hitTestShapes[i].shape.box.maximum;
        break;
      case SHAPE_CYLINDER:
        shared->hitTest[i].type = COLLIDE_CYLINDER;
        shared->hitTest[i].extent[0] = data.hitTestShapes[i].shape.cylinder.base;
        shared->hitTest[i].extent[1] = data.hitTestShapes[i].shape.cylinder.base;
        shared->hitTest[i].extent[1].z += data.hitTestShapes[i].shape.cylinder.height;
        shared->hitTest[i].radius = data.hitTestShapes[i].shape.cylinder.radius;
        break;
      case SHAPE_SPHERE:
        shared->hitTest[i].type = COLLIDE_SPHERE;
        shared->hitTest[i].extent[0] = data.hitTestShapes[i].shape.sphere.center;
        shared->hitTest[i].radius = data.hitTestShapes[i].shape.sphere.radius;
        break;
      case SHAPE_PLANE:
        shared->hitTest[i].type = COLLIDE_PLANE;
        shared->hitTest[i].extent[0].x = data.hitTestShapes[i].shape.plane.length;
        shared->hitTest[i].extent[0].y = data.hitTestShapes[i].shape.plane.width;
        break;
      default:
        break;
    }
  }
  return 1;
}

static void ComputeBoundingRadius(const CGeosetShared *geosets, UINT numGeosets, const NTempest::C3Vector &center, float *radius) {
  float bestDistSqd = 0.0f;

  for (UINT j = 0; j < numGeosets; ++j) {
    UINT numVertices = geosets[j].position.Count();
    for (UINT i = 0; i < numVertices; ++i) {
      NTempest::C3Vector delta = center - geosets[j].position[i];
      float              distSqd = delta.SquaredMag();
      if (distSqd > bestDistSqd) {
        bestDistSqd = distSqd;
      }
    }
  }

  *radius = NTempest::CMath::sqrt_(bestDistSqd);
}

static void ComputeBoundingBox(const CGeosetShared *geosets, UINT numGeosets, NTempest::CAaBox *extent) {
  extent->Set(geosets[0].position[0]);
  for (UINT i = 0; i < numGeosets; ++i) {
    extent->Unite(NTempest::CAaBox::Bounding(geosets[i].position.Ptr(), geosets[i].position.Count()));
  }
}

static void IModelComputeBounds(CModelShared *shared) {
  ASSERT(shared);
  ComputeBoundingBox(shared->geosets.Ptr(), shared->geosets.Count(), &shared->bounds.extent);
  shared->bounds.sphere.c = (shared->bounds.extent.b + shared->bounds.extent.t) * 0.5f;
  ComputeBoundingRadius(shared->geosets.Ptr(), shared->geosets.Count(), shared->bounds.sphere.c, &shared->bounds.sphere.r);
}

static BOOL MdlReadLoadExtents(const MDLDATA &data, CModelBase *modelptr, CModelShared *shared) {
  ASSERT(modelptr);
  ASSERT(shared);

  shared->bounds.extent = data.model.bounds.extent;
  shared->bounds.sphere.c = (shared->bounds.extent.b + shared->bounds.extent.t) * 0.5f;
  shared->bounds.sphere.r = data.model.bounds.radius;

  if (modelptr->m_anim && AnimNeedsSequenceBounds(modelptr->m_anim)) {
    UINT count = data.sequences.Count();
    shared->seqBounds.SetCount(count);
    for (UINT i = 0; i < count; ++i) {
      shared->seqBounds[i].extent = data.sequences[i].bounds.extent;
      shared->seqBounds[i].sphere.c = (shared->seqBounds[i].extent.b + shared->seqBounds[i].extent.t) * 0.5f;
      shared->seqBounds[i].sphere.r = data.sequences[i].bounds.radius;
    }
  } else if (fabs(shared->bounds.sphere.r) < 0.00000023841858f && data.sequences.Count()) {
    shared->bounds.extent = data.sequences[0].bounds.extent;
    shared->bounds.sphere.c = (shared->bounds.extent.b + shared->bounds.extent.t) * 0.5f;
    shared->bounds.sphere.r = data.sequences[0].bounds.radius;
  }
  return 1;
}

static BYTE *LoadBoundsData(BYTE *data, CBoundsData *bounds) {
  bounds->sphere.r = *(float *)data;
  data += sizeof(float);

  bounds->extent.b = *(NTempest::C3Vector *)data;
  data += sizeof(NTempest::C3Vector);
  bounds->extent.t = *(NTempest::C3Vector *)data;
  data += sizeof(NTempest::C3Vector);

  bounds->sphere.c = (bounds->extent.b + bounds->extent.t) * 0.5f;
  return data;
}

static void MdxReadExtents(BYTE *data, UINT fileBytes, CModelBase *modelptr, CModelShared *shared) {
  ASSERT(modelptr);
  ASSERT(shared);

  BYTE *globalData = MDLFileBinarySeek(data, fileBytes, 'LDOM');
  ASSERT(globalData);
  LoadBoundsData(globalData + 0x158, &shared->bounds);

  globalData = MDLFileBinarySeek(data, fileBytes, 'SQES');
  if (!globalData) {
    return;
  }

  globalData += 4;
  UINT numSequences = *(UINT *)globalData;
  globalData += 4;
  if (!numSequences) {
    return;
  }

  if (modelptr->m_anim && AnimNeedsSequenceBounds(modelptr->m_anim)) {
    shared->seqBounds.SetCount(numSequences);
    CBoundsData *bounds = shared->seqBounds.Ptr();
    while (numSequences--) {
      globalData = LoadBoundsData(globalData + 0x60, bounds);
      globalData += 0x10;
      ++bounds;
    }
  } else if (NTempest::CMath::fabs_(shared->bounds.sphere.r) < 0.00000023841858f) {
    globalData = MDLFileBinarySeek(data, fileBytes, 'SQES');
    if (globalData) {
      LoadBoundsData(globalData + 0x68, &shared->bounds);
    }
  }
}

static BOOL MdlReadLoadPositions(const MDLDATA &data, UINT flags, CModelShared *shared) {
  ASSERT(shared);
  UINT numPivots = data.pivotPoints.Count();
  if (!numPivots) {
    return 1;
  }

  if ((flags & 0x220) == 0x20 || (!data.hitTestShapes.Count() && !data.lights.Count())) {
    shared->positions.Set(numPivots, data.pivotPoints.Ptr());
    return 1;
  }

  TSStackArray<UINT> idConversion(_alloca(numPivots * sizeof(UINT)), numPivots, numPivots);
  shared->positions.SetCount(numPivots - AnimBuildObjectIdTranslation(data, ConvertAnimCreateFlags(flags), &idConversion));
  UINT i;
  for (i = 0; i < numPivots; ++i) {
    UINT index = idConversion[i];
    if (index != (UINT)-1) {
      shared->positions[index] = data.pivotPoints[i];
    }
  }

  UINT numEmitters = shared->emitter2Order.Count();
  for (i = 0; i < numEmitters; ++i) {
    shared->emitter2Order[i] = idConversion[shared->emitter2Order[i]];
  }
  return 1;
}

static void MdxReadPositions(BYTE *fileData, UINT fileBytes, UINT flags, CModelShared *shared) {
  BYTE *data = MDLFileBinarySeek(fileData, fileBytes, 'TVIP');
  if (!data) {
    return;
  }

  UINT sectionBytes = *(UINT *)data;
  data += 4;
  UINT numPivots = sectionBytes / (sizeof(float) * C3Vector::eComponents);
  ASSERT(sectionBytes == (numPivots * sizeof(float) * C3Vector::eComponents));

  if ((flags & 0x220) == 0x20) {
    shared->positions.SetCount(numPivots);
    memcpy(shared->positions.Ptr(), data, numPivots * sizeof(float) * C3Vector::eComponents);
    return;
  }

  TSStackArray<UINT> idConversion(_alloca(numPivots * sizeof(UINT)), numPivots, numPivots);
  UINT               numEmitters = AnimBuildObjectIdTranslation(fileData, fileBytes, ConvertAnimCreateFlags(flags), idConversion.Ptr(), numPivots);
  if (!numEmitters) {
    shared->positions.SetCount(numPivots);
    memcpy(shared->positions.Ptr(), data, numPivots * sizeof(float) * C3Vector::eComponents);
    return;
  }

  shared->positions.SetCount(numPivots - numEmitters);
  UINT i;
  for (i = 0; i < numPivots; ++i) {
    UINT index = idConversion[i];
    if (index != (UINT)-1) {
      shared->positions[index] = ((C3Vector *)data)[i];
    }
  }

  numEmitters = shared->emitter2Order.Count();
  for (i = 0; i < numEmitters; ++i) {
    shared->emitter2Order[i] = idConversion[shared->emitter2Order[i]];
  }
}

static CModelShared *CreateSharedModelData(LPCSTR fileName) {
  LPVOID        storage = SMemAlloc(sizeof(CModelShared), "HMODELSHARED", SERR_LINECODE_OBJECT, 0);
  CModelShared *shared = storage ? new (storage) CModelShared : 0;

  ASSERT(shared);
  SStrCopy(shared->name, fileName, sizeof(shared->name));
  return shared;
}

static BOOL IsSimpleModel(const MDLDATA &source) {
  return source.geosets.Count() <= 5 && source.materials.Count() <= 4 && source.textures.Count() <= 4 && !source.lights.Count() &&
         !source.attachments.Count() && !source.particleEmitters2.Count() && !source.ribbonEmitters.Count() && !source.cameras.Count() &&
         !source.hitTestShapes.Count();
}

static UINT GetSectionCount(BYTE *fileData, UINT fileBytes, DWORD sectionTag) {
  BYTE *section = MDLFileBinarySeek(fileData, fileBytes, sectionTag);
  return section ? *(UINT *)(section + 4) : 0;
}

static UINT GetTextureCount(BYTE *fileData, UINT fileBytes) {
  BYTE *section = MDLFileBinarySeek(fileData, fileBytes, 'SXET');
  if (!section) {
    return 0;
  }
  return *(UINT *)section / 0x10C;
}

static BOOL IsSimpleModel(BYTE *fileData, UINT fileBytes) {
  if (GetSectionCount(fileData, fileBytes, 'SLTM') > 4 || GetTextureCount(fileData, fileBytes) > 4 ||
      GetSectionCount(fileData, fileBytes, 'SOEG') > 5)
  {
    return 0;
  }

  if (GetSectionCount(fileData, fileBytes, 'ETIL') || GetSectionCount(fileData, fileBytes, 'HCTA') ||
      GetSectionCount(fileData, fileBytes, '2ERP') || GetSectionCount(fileData, fileBytes, 'BBIR') ||
      GetSectionCount(fileData, fileBytes, 'SMAC') || GetSectionCount(fileData, fileBytes, 'TSTH'))
  {
    return 0;
  }

  return 1;
}

static void BuildSimpleModelFromMdxData(BYTE *fileData, UINT fileBytes, CModelSimple *modelptr, CModelShared *shared, UINT flags, CStatus *status) {
  MdxReadTextures(fileData, fileBytes, flags, modelptr, status);
  MdxReadMaterials(fileData, fileBytes, flags, modelptr, shared);
  MdxReadGeosets(fileData, fileBytes, flags, modelptr, shared);
  if (!(flags & 0x100)) {
    MdxReadAnimation(fileData, fileBytes, modelptr, flags);
  }
  MdxReadNumMatrices(fileData, fileBytes, flags, shared);
  if (flags & 0x80) {
    IModelEnableFullAlpha(modelptr, 0);
  }
  shared->collision = CollisionDataCreate(fileData, fileBytes);
  MdxReadExtents(fileData, fileBytes, modelptr, shared);
  MdxReadPositions(fileData, fileBytes, flags, shared);
}

static void BuildModelFromMdxData(BYTE *fileData, UINT fileBytes, CModelBase *baseModel, CModelShared *shared, UINT flags, CStatus *status) {
  MdxLoadGlobalProperties(fileData, fileBytes, &flags, shared);
  if (!(baseModel->m_flags & 0x20)) {
    BuildSimpleModelFromMdxData(fileData, fileBytes, (CModelSimple *)baseModel, shared, flags, status);
    return;
  }

  CModelComplex *modelptr = (CModelComplex *)baseModel;
  MdxReadTextures(fileData, fileBytes, flags, modelptr, status);
  MdxReadMaterials(fileData, fileBytes, flags, modelptr, shared);
  MdxReadGeosets(fileData, fileBytes, flags, modelptr, shared);
  MdxReadAttachments(fileData, fileBytes, flags, modelptr, shared, status);

  if (!(flags & 0x100)) {
    MdxReadAnimation(fileData, fileBytes, modelptr, flags);
    MdxReadRibbonEmitters(fileData, fileBytes, modelptr, shared);
  }
  MdxReadEmitters2(fileData, fileBytes, flags, modelptr, shared, status);
  MdxReadNumMatrices(fileData, fileBytes, flags, shared);
  if (flags & 0x20) {
    MdxReadHitTestData(fileData, fileBytes, modelptr, shared);
  }
  if (flags & 0x80) {
    IModelEnableFullAlpha(modelptr, 0);
  }
  if (!(flags & 0x200)) {
    MdxReadLights(fileData, fileBytes, modelptr);
  }
  shared->collision = CollisionDataCreate(fileData, fileBytes);
  MdxReadExtents(fileData, fileBytes, modelptr, shared);
  MdxReadPositions(fileData, fileBytes, flags, shared);
  MdxReadCameras(fileData, fileBytes, &modelptr->m_cameras);
}

static BOOL BuildSimpleModelFromMdlData(const MDLDATA &source, CModelSimple *modelptr, CModelShared *shared, UINT flags, CStatus *status) {
  if (!MdlReadLoadModel(source, modelptr, shared, flags, status)) {
    return 0;
  }
  if (!(flags & 0x100) && !MdlReadLoadAnim(source, modelptr, flags, status)) {
    return 0;
  }
  if (!MdlReadLoadNumMatrices(source, shared, flags)) {
    return 0;
  }
  if (flags & 0x80) {
    IModelEnableFullAlpha(modelptr, 0);
  }
  shared->collision = CollisionDataCreate(source);
  return MdlReadLoadExtents(source, modelptr, shared) && MdlReadLoadPositions(source, flags, shared);
}

static int BuildModelFromMdlData(const MDLDATA &source, CModelBase *baseModel, CModelShared *shared, UINT flags, CStatus *status) {
  MdlReadLoadGlobalProperties(source, shared, &flags);
  if (!(baseModel->m_flags & 0x20)) {
    return BuildSimpleModelFromMdlData(source, (CModelSimple *)baseModel, shared, flags, status);
  }

  CModelComplex *modelptr = (CModelComplex *)baseModel;
  if (!MdlReadLoadModel(source, modelptr, shared, flags, status)) {
    return 0;
  }
  if (!(flags & 0x100) && (!MdlReadLoadAnim(source, baseModel, flags, status) || !MdlReadLoadRibbonEmitters(source, modelptr, shared))) {
    return 0;
  }
  if (!MdlReadLoadEmitters2(source, modelptr, shared, flags, status) || !MdlReadLoadNumMatrices(source, shared, flags)) {
    return 0;
  }
  if ((flags & 0x20) && !MdlReadLoadHitTestData(source, modelptr, shared)) {
    return 0;
  }
  if (flags & 0x80) {
    IModelEnableFullAlpha(baseModel, 0);
  }
  if (!(flags & 0x200) && !MdlReadLoadLights(source, modelptr)) {
    return 0;
  }
  shared->collision = CollisionDataCreate(source);
  return MdlReadLoadExtents(source, baseModel, shared) && MdlReadLoadPositions(source, flags, shared) && MdlReadCameras(source, &modelptr->m_cameras);
}

HMATERIAL BuildSimpleMaterial(CModelTexture *texData, UINT textureId, HTEXTURE texture, EGxBlend blendMode, UINT disables, UINT replaceableId) {
  CMaterial *unique = NEWHANDLE(HMATERIAL, CMaterial);
  ASSERT(unique);

  CTexLayer *layer = unique->layers.New();
  ASSERT(layer);

  texData->handle = (HTEXTURE)HandleDuplicate(texture);
  texData->replaceableId = replaceableId;

  layer->tmuPass[0].textureId = textureId;
  layer->blendMode = blendMode;
  layer->tmuPass[0].combiner = GxTexBlend_Mod;
  layer->vertexFormat = GxVBF_PNT0;

  if (MODEL_GEO_UNSHADED & disables) {
    layer->disables |= 0x01;
  }
  if (MODEL_GEO_TWOSIDED & disables) {
    layer->disables |= 0x10;
  }
  if (MODEL_GEO_UNFOGGED & disables) {
    layer->disables |= 0x02;
  }
  if (MODEL_GEO_NO_DEPTH_TEST & disables) {
    layer->disables |= 0x04;
  }
  if (MODEL_GEO_NO_DEPTH_SET & disables) {
    layer->disables |= 0x08;
  }

  CMaterialShared *shared = NEWHANDLE(HMATERIALSHARED, CMaterialShared);
  ASSERT(shared);

  CTexLayerShared *sharedLayer = shared->layers.New();
  ASSERT(sharedLayer);
  sharedLayer->blendMode = blendMode;
  sharedLayer->tmuPass[0].transformId = -1;
  sharedLayer->tmuPass[0].coordId = 0;

  unique->data = CREATEHANDLE(HMATERIALSHARED, shared);
  return CREATEHANDLE(HMATERIAL, unique);
}

static void BuildSimpleGeoset(
    UINT                      numVertices,
    const NTempest::C3Vector *position,
    const NTempest::C3Vector *normal,
    const NTempest::C2Vector *texCoord,
    EGxPrim                   primitiveType,
    const WORD               *primitiveVertices,
    UINT                      numPrimVertices,
    UINT                      materialId,
    CGeosetShared            *geoShared
) {
  ASSERT(geoShared);

  geoShared->materialId = materialId;
  geoShared->position.Set(numVertices, position);
  geoShared->normal.Set(numVertices, normal);
  geoShared->texCoord.SetCount(1);
  geoShared->texCoord[0].Set(numVertices, texCoord);

  geoShared->primitive.SetCount(1);
  geoShared->primitive[0].type = primitiveType;
  geoShared->primitive[0].vertexCount = numPrimVertices;
  geoShared->primitiveVertices.Set(numPrimVertices, primitiveVertices);
}

static HMODEL CreateDefaultModel(LPCSTR fileName, UINT modelLoadFlags, CStatus *status) {
  status->Add(STATUS_WARNING, "Warning, model %s failed to load\n", fileName);

  HTEXTURE texture = LoadModelTexture("Textures\\ShaneCube", modelLoadFlags, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), status);
  ASSERT(texture);

  NTempest::CAaBox bounds(NTempest::C3Vector(-0.5f, -0.5f, 0.0f), NTempest::C3Vector(0.5f, 0.5f, 1.0f));
  HMODEL           model = CreateModelBoundingBox(bounds, texture, GxBlend_Opaque);

  CModelShared *shared;
  IModelDerefHandle((CModel *)model, &shared);
  ASSERT(shared);
  shared->collision = CollisionDataCreate(bounds);
  HashNewModel(fileName, model, 0, status);
  return model;
}

void ModelInitialize() {
  AnimInitialize();
  MDLFileInitialize();
  ModelAnimateInitialize();
  ModelRenderInitialize();
  AsyncFileReadAddHandler(AsyncModelHandler);
}

void ModelDestroy() {
  ModelRenderDestroy();
  ModelAnimateDestroy();
  AnimDestroy();
  MDLFileDestroy();
  s_modelCacheLRU.UnlinkAll();
  s_modelCache.Destroy();
  ParticleSystemManager::Destroy();
  RibbonManager::Destroy();
  s_freeModItems.Clear();
}

void ModelCacheFlush() {
  s_modelCacheLRU.UnlinkAll();
  s_modelCache.Destroy();
}

void ModelRemoveFromCache(LPCSTR sourcefile) {
  char        filePath[MAX_PATH];
  char       *extension;
  CModelHash *modelHash;

  VALIDATEBEGIN;
  VALIDATE(sourcefile);
  VALIDATEENDVOID;

  SStrCopy(filePath, sourcefile, sizeof(filePath));
  extension = SStrChrR(filePath, '.');
  if (extension) {
    *extension = 0;
  }

  modelHash = s_modelCache.Ptr(filePath);
  if (!modelHash) {
    return;
  }

  s_modelCacheLRU.UnlinkNode(modelHash);
  if (modelHash->model) {
    HandleClose(modelHash->model);
  }
  modelHash->model = 0;
  s_modelCache.Delete(modelHash);
}

BOOL ModelCacheUpdate(DWORD currentTime, CStatus *status) {
  UINT numModelsFlushed = 0;

  while (CModelHash *modelHash = s_modelCacheLRU.Head()) {
    if (numModelsFlushed >= 4) {
      break;
    }

    if ((long)(currentTime - modelHash->timeStamp) < 30000L) {
      break;
    }

    s_modelCacheLRU.UnlinkNode(modelHash);

    if (ModelIsUsed(modelHash->model)) {
      s_modelCacheLRU.LinkNode(modelHash, LIST_TAIL, 0);
      modelHash->timeStamp = currentTime;
    } else {
      HandleClose(modelHash->model);
      modelHash->model = 0;
      s_modelCache.Delete(modelHash);
      ++numModelsFlushed;
    }
  }

  if (numModelsFlushed && status) {
    status->Add(STATUS_INFO, "%u model(s) flushed from model cache\n", numModelsFlushed);
  }

  return numModelsFlushed != 0;
}

HMODEL ModelGetModel(LPCSTR sourcefile, CModelCreate *data) {
  VALIDATEBEGIN;
  VALIDATE(sourcefile);
  VALIDATEEND;
  return GetModel(sourcefile, data);
}

static HMODEL IModelCreateBlocking(LPCSTR fileName, char *actualPath, CModelCreate *data, CStatus *status) {
  UINT          fileBytes;
  CModelShared *shared;
  BYTE         *fileData;
  UINT          createFlags;

  createFlags = data ? data->flags : 0;
  fileData = MDLFileBinaryLoad(actualPath, &fileBytes, status);
  if (!fileData) {
    return (createFlags & 0x2000) ? CreateDefaultModel(fileName, createFlags, status) : 0;
  }

  CModelBase *modelptr;
  if (IsSimpleModel(fileData, fileBytes)) {
    modelptr = NEW(CModelSimple);
  } else {
    modelptr = NEW(CModelComplex);
  }
  ASSERT(modelptr);

  shared = CreateSharedModelData(fileName);
  BuildModelFromMdxData(fileData, fileBytes, modelptr, shared, createFlags, status);
  if (modelptr->m_anim) {
    ProcessAnimReorders(modelptr, data);
  }
  MDLFileBinaryUnload(fileData);

  CModel *model = NEWHANDLE(HMODEL, CModel)(CMODEL_UNINITIALIZED);
  ASSERT(model);
  model->data = modelptr;
  model->shared = CREATEHANDLE(HMODELSHARED, shared);
  ASSERT(model->shared);

  HMODEL modelHandle = CREATEHANDLE(HMODEL, model);
  model->state = CMODEL_LOADED;
  HashNewModel(fileName, modelHandle, createFlags, status);
  return modelHandle;
}

static TSCArray<BYTE, 0x400000> s_asyncLoadBuffer;
static LISTDECLEX(CAsyncObject, link, s_asyncLoadList);
static UINT s_asyncLoadBufferUsed;
static int  s_asyncPending;

void CModel::DeleteAsyncObj() {
  if (asyncObject->buffer) {
    --s_asyncPending;
  }

  SFile *file = asyncObject->file;
  AsyncFileReadDestroyObject(asyncObject);
  asyncObject = 0;
  SFile::Close(file);
}

static void AsyncModelHandler() {
  UINT          bufferRemaining;
  CAsyncObject *asyncObject;
  CAsyncObject *asyncObjectnext_node;

  ASSERT(s_asyncPending >= 0);

  if (s_asyncPending) {
    return;
  }

  s_asyncLoadBufferUsed = 0;
  bufferRemaining = s_asyncLoadBuffer.MaxCount();

  for (asyncObject = s_asyncLoadList.Head();
       (int)asyncObject > 0 ? ((asyncObjectnext_node = s_asyncLoadList.RawNext(asyncObject)), 1) : 0;
       asyncObject = asyncObjectnext_node)
  {
    if (bufferRemaining < asyncObject->size) {
      continue;
    }

    asyncObject->link.Unlink();
    asyncObject->buffer = &s_asyncLoadBuffer[s_asyncLoadBufferUsed];
    asyncObject->canReorder = 1;
    s_asyncLoadBufferUsed += asyncObject->size;
    bufferRemaining -= asyncObject->size;
    ++s_asyncPending;
    AsyncFileReadObject(asyncObject);
  }
}

static void AsnycModelPostLoadCallback(LPVOID userArg) {
  CModel *model = (CModel *)userArg;

  ASSERT(model);
  ASSERT(model->state == CMODEL_ASYNC_WAIT);
  CModelShared *shared = (CModelShared *)model->shared;
  ASSERT(shared);

  CStatus status;
  BYTE   *fileData = (BYTE *)model->asyncObject->buffer;
  ASSERT(*((ULONG *) (fileData)) == 'XLDM');
  fileData += 4;
  UINT fileBytes = model->asyncObject->size - 4;

  CModelBase *modelptr;
  if (IsSimpleModel(fileData, fileBytes)) {
    modelptr = NEW(CModelSimple);
  } else {
    modelptr = NEW(CModelComplex);
  }
  ASSERT(modelptr);

  BuildModelFromMdxData(fileData, fileBytes, modelptr, shared, model->createData->flags, &status);

  if (modelptr->m_anim) {
    ProcessAnimReorders(modelptr, model->createData);
  }
  model->DeleteAsyncObj();
  model->data = modelptr;
  DEL(model->createData);
  model->createData = 0;
  model->state = CMODEL_LOADED;
  ExecuteQueuedActions(model);
}

static HMODEL IModelCreate(LPCSTR fileName, char *actualPath, CModelCreate *createData, CStatus *status) {
  ASSERT(status);
  OsOutputDebugString("Model: (INFO) : Loading \"%s\"\n", actualPath);

  if (createData && (createData->flags & 0x8000)) {
    return IModelCreateBlocking(fileName, actualPath, createData, status);
  }

  CModelCreate filler;
  if (!createData) {
    createData = &filler;
  }

  SFile *file;
  if (!SFile::Open(actualPath, &file)) {
    if (createData->flags & 0x2000) {
      return CreateDefaultModel(fileName, createData->flags, status);
    }
    return 0;
  }

  CModel *model = NEWHANDLE(HMODEL, CModel);
  ASSERT(model);
  model->asyncObject = AsyncFileReadCreateObject();
  ASSERT(model->asyncObject);
  model->shared = CREATEHANDLE(HMODELSHARED, CreateSharedModelData(fileName));
  ASSERT(model->shared);
  model->createData = NEW(CModelCreate)(*createData);

  model->asyncObject->userArg = model;
  model->asyncObject->userPostloadCallback = AsnycModelPostLoadCallback;
  model->asyncObject->file = file;
  model->asyncObject->offset = 0;
  model->asyncObject->size = SFile::GetFileSize(file, 0);

  if (s_asyncLoadBuffer.MaxCount() - s_asyncLoadBufferUsed >= model->asyncObject->size) {
    model->asyncObject->buffer = &s_asyncLoadBuffer[s_asyncLoadBufferUsed];
    model->asyncObject->canReorder = 1;
    s_asyncLoadBufferUsed += model->asyncObject->size;
    ++s_asyncPending;
    AsyncFileReadObject(model->asyncObject);
  } else {
    model->asyncObject->canReorder = 0;
    s_asyncLoadList.LinkNode(model->asyncObject, LIST_TAIL, 0);
  }

  HMODEL modelHandle = CREATEHANDLE(HMODEL, model);
  ASSERT(modelHandle);
  HashNewModel(fileName, modelHandle, model->createData->flags, status);
  return modelHandle;
}

static BOOL IsBinaryFile(char *path) {
  UINT length = SStrLen(path);
  char lastCharacter = path[length - 1];

  if (lastCharacter == 'x' || lastCharacter == 'X') {
    if (SFile::FileExists(path)) {
      return 1;
    }

    path[length - 1] = 'l';
  } else if (!SFile::FileExists(path)) {
    path[length - 1] = 'x';
    return 1;
  }

  return 0;
}

HMODEL ModelCreate(LPCSTR sourcefile, CModelCreate *data, CStatus *status) {
  VALIDATEBEGIN;
  VALIDATE(sourcefile);
  VALIDATE(sourcefile[0]);
  VALIDATEEND;

  CStatus *useStatus = status ? status : &s_nullStatus;
  HMODEL   model = GetModel(sourcefile, data);
  if (model) {
    return model;
  }

  char actualPath[MAX_PATH];
  SStrCopy(actualPath, sourcefile, sizeof(actualPath));
  if (IsBinaryFile(actualPath)) {
    return IModelCreate(sourcefile, actualPath, data, useStatus);
  }

  MDLDATA mdlData;
  if (MDLFileRead(actualPath, &mdlData, useStatus)) {
    return ModelCreate(mdlData, data, useStatus);
  }

  if (data && (data->flags & 0x2000)) {
    return CreateDefaultModel(sourcefile, data->flags, useStatus);
  }

  return 0;
}

HMODEL ModelCreate(const MDLDATA &source, CModelCreate *data, CStatus *status) {
  VALIDATEBEGIN;
  VALIDATE(status);
  VALIDATEEND;
  ASSERT(source.header.sourceFilename[0]);

  OsOutputDebugString("Model: (INFO) : Loading \"%s\"\n", (LPCSTR)source.header.sourceFilename);

  UINT createFlags = data ? data->flags : 0;
  if (MdlReadValidate(source, status)) {
    CModelBase *modelptr;
    if (IsSimpleModel(source)) {
      modelptr = NEW(CModelSimple);
    } else {
      modelptr = NEW(CModelComplex);
    }
    ASSERT(modelptr);

    CModelShared *shared = CreateSharedModelData(source.header.sourceFilename);
    if (BuildModelFromMdlData(source, modelptr, shared, createFlags, status)) {
      if (modelptr->m_anim) {
        ProcessAnimReorders(modelptr, data);
      }

      CModel *model = NEWHANDLE(HMODEL, CModel)(CMODEL_UNINITIALIZED);
      ASSERT(model);
      model->data = modelptr;
      model->shared = CREATEHANDLE(HMODELSHARED, shared);
      ASSERT(model->shared);

      HMODEL modelHandle = CREATEHANDLE(HMODEL, model);
      ASSERT(modelHandle);
      model->state = CMODEL_LOADED;
      HashNewModel(source.header.sourceFilename, modelHandle, createFlags, status);
      return modelHandle;
    }

    DEL(modelptr);
    DEL(shared);
  }

  if (createFlags & 0x2000) {
    return CreateDefaultModel(source.header.sourceFilename, createFlags, status);
  }
  return 0;
}

static CModel *IModelCreateSimpleEmpty(LPCSTR name) {
  CModel *model = NEWHANDLE(HMODEL, CModel)(CMODEL_UNINITIALIZED);
  ASSERT(model);
  CModelShared *shared = NEWHANDLE(HMODELSHARED, CModelShared);
  ASSERT(shared);
  CModelSimple *modelptr = NEW(CModelSimple);
  ASSERT(modelptr);

  shared->numBones = 1;
  modelptr->m_geosets.SetCount(1);
  modelptr->m_geosetColor.SetCount(1);
  modelptr->m_materials.SetCount(1);
  modelptr->m_textures.SetCount(1);
  shared->geosets.SetCount(1);
  shared->numGeosets = 1;

  model->shared = CREATEHANDLE(HMODELSHARED, shared);
  ASSERT(model->shared);
  model->data = modelptr;
  SStrCopy(shared->name, name ? name : "Custom Built Model", sizeof(shared->name));
  return model;
}

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
) {
  CModel *model = IModelCreateSimpleEmpty(name);
  ASSERT(model);

  CModelSimple *modelptr = (CModelSimple *)model->data;
  CModelShared *shared = (CModelShared *)model->shared;

  HMATERIAL material = BuildSimpleMaterial(&modelptr->m_textures[0], 0, texture, blendMode, disables, replaceableId);
  modelptr->m_materials[0] = material;
  modelptr->m_geosetColor[0].animatedColor = color;

  BuildSimpleGeoset(numVertices, position, normal, texCoord, primitiveType, primitiveVertices, numPrimVertices, 0, &shared->geosets[0]);
  IModelComputeBounds(shared);
  model->state = CMODEL_LOADED;
  return CREATEHANDLE(HMODEL, model);
}

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
) {
  CModelBase   *modelptr;
  CModelShared *shared;
  if (!IModelDerefHandle((CModel *)model, &modelptr, &shared) || !(modelptr->m_flags & 0x20)) {
    return 0;
  }

  CModelComplex *complex = (CModelComplex *)modelptr;
  UINT           materialId = complex->m_materials.Count();

  HMATERIAL material = BuildSimpleMaterial(complex->m_textures.New(), complex->m_textures.Count(), texture, blendMode, disables, replaceableId);
  ASSERT(material);
  *complex->m_materials.New() = material;

  BuildSimpleGeoset(
      numVertices, position, normal, texCoord, primitiveType, primitiveVertices, numPrimVertices, materialId, complex->m_addlGeosets.New()
  );

  complex->m_geosets.New();
  complex->m_geosetColor.New()->animatedColor = color;
  IModelComputeBounds(shared);
  return 1;
}

HMODEL ModelDuplicate(HMODEL sourceModel, UINT flags) {
  if (!sourceModel) {
    return 0;
  }

  CModel *source = (CModel *)sourceModel;
  if (source->state == CMODEL_LOADED && (flags & 1)) {
    source->data->m_flags |= 4;
  }

  LPVOID  storage = SMemAlloc(sizeof(CModel), "HMODEL", SERR_LINECODE_OBJECT, 0);
  CModel *copy = storage ? new (storage) CModel(*source) : 0;
  ASSERT(copy);

  if (copy->state != CMODEL_LOADED && (flags & 1)) {
    copy->state = CMODEL_DUPE_WAIT_PRSRV_ANIM;
  }

  if (source->state == CMODEL_LOADED) {
    source->data->m_flags &= ~4U;
  }

  HMODEL duplicate = CREATEHANDLE(HMODEL, copy);
  if (copy->state != CMODEL_LOADED) {
    EnqueueModelCommand(source, MODEL_FINISH_DUPLICATION, duplicate);
  }
  return duplicate;
}

void EnqueueModelCommand(CModel *model, EModelModQ command, ...) {
  CModelModItem *modItem = s_freeModItems.Head();
  BYTE          *paramData;
  va_list        arguments;
  UINT           i;

  if (modItem) {
    modItem->Unlink();
    model->modelModQueue.LinkNode(modItem, LIST_TAIL, 0);
  } else {
    modItem = model->modelModQueue.NewNode(LIST_TAIL, 0, 0);
  }

  modItem->action = command;
  paramData = modItem->paramData;
  va_start(arguments, command);

  for (i = 0; i < sizeof(s_modelParamTypes[0]) / sizeof(s_modelParamTypes[0][0]); ++i) {
    if (s_modelParamTypes[command][i] == MPARAM_NONE) {
      break;
    }

    switch (s_modelParamTypes[command][i]) {
      case MPARAM_UINT:
        *(UINT *)paramData = va_arg(arguments, UINT);
        paramData += sizeof(UINT);
        break;

      case MPARAM_HANDLE:
        *(HOBJECT *)paramData = HandleDuplicate(va_arg(arguments, HOBJECT));
        paramData += sizeof(HOBJECT);
        break;

      case MPARAM_FLOAT:
        *(float *)paramData = va_arg(arguments, double);
        paramData += sizeof(float);
        break;

      case MPARAM_C3VECTOR:
        *(NTempest::C3Vector *)paramData = va_arg(arguments, NTempest::C3Vector);
        paramData += sizeof(NTempest::C3Vector);
        break;

      case MPARAM_BOOL:
        *paramData = va_arg(arguments, int);
        ++paramData;
        break;

      case MPARAM_CARGB:
        *(DWORD *)paramData = va_arg(arguments, DWORD);
        paramData += sizeof(DWORD);
        break;

      case MPARAM_PTR:
        *(LPVOID *)paramData = va_arg(arguments, LPVOID);
        paramData += sizeof(LPVOID);
        break;

      case MPARAM_BYTE:
        *paramData = va_arg(arguments, int);
        ++paramData;
        break;
    }
  }

  va_end(arguments);
}

void ExecuteQueuedActions(CModel *model) {
  HMODEL modelHandle = 0;

  SAFEITERATELIST(CModelModItem, model->modelModQueue, item) {
    if (!modelHandle) {
      modelHandle = CREATEHANDLE(HMODEL, model);
    }

    item->Unlink();

    switch (item->action) {
      case MODEL_ADD_LINK: {
        HMODEL child = *(HMODEL *)(item->paramData + 4);
        ModelAddLink(modelHandle, *(UINT *)item->paramData, child, *(float *)(item->paramData + 8));
        HandleClose(child);
        break;
      }

      case MODEL_APPLY_OBJECT_FACE_DIR:
        ModelApplyObjectFaceDir(modelHandle, *(UINT *)item->paramData, *(NTempest::C3Vector *)(item->paramData + 4));
        break;

      case MODEL_APPLY_OBJECT_LOOK_AT:
        ModelApplyObjectLookAt(modelHandle, *(UINT *)item->paramData, *(NTempest::C3Vector *)(item->paramData + 4));
        break;

      case MODEL_CLEAR_ALL_LINKS:
        ModelClearAllLinks(modelHandle);
        break;

      case MODEL_CLEAR_LINK:
        ModelClearLink(modelHandle, *(UINT *)item->paramData);
        break;

      case MODEL_ENABLE_ANIM_BLENDING:
        ModelEnableAnimBlending(modelHandle, item->paramData[0]);
        break;

      case MODEL_ENABLE_FULL_ALPHA:
        ModelEnableFullAlpha(modelHandle, item->paramData[0]);
        break;

      case MODEL_FINISH_DUPLICATION: {
        HMODEL  duplicate = *(HMODEL *)item->paramData;
        CModel *destination = (CModel *)duplicate;
        HMODEL  sourceHandle = destination->dupSource;
        destination->FinishDuplication(*model);
        HandleClose(duplicate);
        HandleClose(sourceHandle);
        break;
      }

      case MODEL_FORCE_CURRENT_SEQUENCE_TIME:
        ModelForceCurrentSequenceTime(modelHandle, *(int *)item->paramData, item->paramData[4]);
        break;

      case MODEL_FORCE_SEQUENCE_TIME:
        ModelForceSequenceTime(modelHandle, *(UINT *)item->paramData, *(int *)(item->paramData + 4), item->paramData[8]);
        break;

      case MODEL_HIDE_BOUNDS:
        ModelHideBounds(modelHandle);
        break;

      case MODEL_HIDE_GEOSETS:
        ModelHideGeosets(modelHandle, *(UINT *)item->paramData, item->paramData[4]);
        break;

      case MODEL_HIDE_GEOSETS_RANGE:
        ModelHideGeosetsRange(modelHandle, *(UINT *)item->paramData, *(UINT *)(item->paramData + 4), item->paramData[8]);
        break;

      case MODEL_LOCK_OBJECT_SEQUENCE:
        ModelLockObjectSequence(modelHandle, *(UINT *)item->paramData, item->paramData[4]);
        break;

      case MODEL_MARK_FOOTSTEP_SEQUENCE:
        ModelMarkFootstepSequence(modelHandle, *(UINT *)item->paramData);
        break;

      case MODEL_MATCH_SEQUENCE:
        ModelMatchSequence(modelHandle, *(UINT *)item->paramData, *(UINT *)(item->paramData + 4), *(UINT *)(item->paramData + 8));
        break;

      case MODEL_OPTIMIZE_VISIBLE_GEOSETS:
        ModelOptimizeVisibleGeosets(modelHandle);
        break;

      case MODEL_REMOVE_LINK: {
        HMODEL child = *(HMODEL *)(item->paramData + 4);
        ModelRemoveLink(modelHandle, *(UINT *)item->paramData, child);
        HandleClose(child);
        break;
      }

      case MODEL_REMOVE_OBJECT_FACE_DIR:
        ModelRemoveObjectFaceDir(modelHandle, *(UINT *)item->paramData);
        break;

      case MODEL_REMOVE_OBJECT_LOOK_AT:
        ModelRemoveObjectLookAt(modelHandle, *(UINT *)item->paramData);
        break;

      case MODEL_REPLACE_TEXTURE: {
        HTEXTURE texture = *(HTEXTURE *)(item->paramData + 4);
        ModelReplaceTexture(modelHandle, *(UINT *)item->paramData, texture, item->paramData[8]);
        HandleClose(texture);
        break;
      }

      case MODEL_SET_EMISSIVE_COLOR:
        ModelSetEmissiveColor(modelHandle, *(NTempest::CImVector *)item->paramData, item->paramData[4]);
        break;

      case MODEL_SET_EVENT_CALLBACK:
        ModelSetEventCallback(
            modelHandle, *(void ( **)(LPCSTR, const NTempest::C3Vector &, LPVOID))item->paramData, *(LPVOID *)(item->paramData + 4),
            item->paramData[8]
        );
        break;

      case MODEL_SET_LIGHT_SELECT_CALLBACK:
        ModelSetLightSelectCallback(
            modelHandle, *(void ( **)(LPVOID, NTempest::C3Vector, const NTempest::C3Vector &, UINT))item->paramData,
            *(LPVOID *)(item->paramData + 4), item->paramData[8]
        );
        break;

      case MODEL_SET_OBJECT_TIME_SCALE:
        ModelSetObjectTimeScale(modelHandle, *(UINT *)item->paramData, *(float *)(item->paramData + 4), item->paramData[8]);
        break;

      case MODEL_SET_RANDOM_SEQUENCE_FIDGET1:
        ModelSetRandomSequenceFidget(modelHandle, *(UINT *)item->paramData, *(UINT *)(item->paramData + 4));
        break;

      case MODEL_SET_RANDOM_SEQUENCE_FIDGET2:
        ModelSetRandomSequenceFidget(
            modelHandle, *(UINT *)item->paramData, *(UINT *)(item->paramData + 4), *(UINT *)(item->paramData + 8)
        );
        break;

      case MODEL_SET_SEQ_FINISHED_HANDLER1:
        ModelSetSeqFinishedHandler(modelHandle, *(ANIMSEQFINISHEDHANDLER *)item->paramData, *(LPVOID *)(item->paramData + 4));
        break;

      case MODEL_SET_SEQ_FINISHED_HANDLER2:
        ModelSetSeqFinishedHandler(
            modelHandle, *(UINT *)item->paramData, *(ANIMSEQFINISHEDHANDLER *)(item->paramData + 4),
            *(LPVOID *)(item->paramData + 8)
        );
        break;

      case MODEL_SET_SEQUENCE1:
        ModelSetSequence(modelHandle, *(UINT *)item->paramData, *(UINT *)(item->paramData + 4));
        break;

      case MODEL_SET_SEQUENCE2:
        ModelSetSequence(modelHandle, *(UINT *)item->paramData, *(UINT *)(item->paramData + 4), *(UINT *)(item->paramData + 8));
        break;

      case MODEL_SET_SEQUENCE_FIDGET1:
        ModelSetSequenceFidget(
            modelHandle, *(UINT *)item->paramData, *(UINT *)(item->paramData + 4), *(UINT *)(item->paramData + 8)
        );
        break;

      case MODEL_SET_SEQUENCE_FIDGET2:
        ModelSetSequenceFidget(
            modelHandle, *(UINT *)item->paramData, *(UINT *)(item->paramData + 4), *(UINT *)(item->paramData + 8),
            *(UINT *)(item->paramData + 12)
        );
        break;

      case MODEL_SET_TIME_SCALE:
        ModelSetTimeScale(modelHandle, *(float *)item->paramData, item->paramData[4]);
        break;

      case MODEL_SET_VERTEX_ALPHA:
        ModelSetVertexAlpha(modelHandle, item->paramData[0], item->paramData[1]);
        break;

      case MODEL_SET_VERTEX_COLOR:
        ModelSetVertexColor(modelHandle, item->paramData[0], item->paramData[1], item->paramData[2], item->paramData[3]);
        break;

      case MODEL_SHOW_BOUNDING_SPHERE:
        ModelShowBoundingSphere(modelHandle);
        break;

      case MODEL_SHOW_COLLISION:
        ModelShowCollision(modelHandle, item->paramData[0]);
        break;

      case MODEL_SHOW_COLLISION_AABOX:
        ModelShowCollisionAaBox(modelHandle, item->paramData[0]);
        break;

      case MODEL_SHOW_MODEL:
        ModelShowModel(modelHandle, item->paramData[0]);
        break;

      default:
        break;
    }

    s_freeModItems.LinkNode(item, LIST_TAIL, 0);
  }

  if (modelHandle) {
    HandleClose(modelHandle);
  }
}

void CModel::RemoveModelCommandsFromQueue() {
  ITERATELIST(CModelModItem, modelModQueue, item) {
    BYTE *paramData = item->paramData;
    for (UINT i = 0; i < sizeof(s_modelParamTypes[0]) / sizeof(s_modelParamTypes[0][0]); ++i) {
      EModelParamType type = s_modelParamTypes[item->action][i];
      if (type == MPARAM_NONE) {
        break;
      }

      switch (type) {
        case MPARAM_HANDLE:
          HandleClose(*(HOBJECT *)paramData);
          paramData += 4;
          break;
        case MPARAM_C3VECTOR:
          paramData += sizeof(NTempest::C3Vector);
          break;
        case MPARAM_BOOL:
        case MPARAM_BYTE:
          ++paramData;
          break;
        case MPARAM_UINT:
        case MPARAM_FLOAT:
        case MPARAM_CARGB:
        case MPARAM_PTR:
          paramData += 4;
          break;
      }
    }
  }

  s_freeModItems.Combine(&modelModQueue, LIST_TAIL, 0);
}

BOOL ModelIsLoaded(HMODEL modelHandle, int doLinkedModels) {
  CModelBase    *base;
  CModelComplex *complex;
  UINT           numLinks;
  UINT           i;

  CModel *model = (CModel *)modelHandle;
  VALIDATEBEGIN;
  VALIDATE(model);
  VALIDATEEND;

  if (model->state != CMODEL_LOADED) {
    return 0;
  }

  if (!doLinkedModels) {
    return 1;
  }

  base = model->data;
  if (!(base->m_flags & 0x20)) {
    return 1;
  }

  complex = (CModelComplex *)base;
  numLinks = complex->m_attached.Count();
  for (i = 0; i < numLinks; ++i) {
    ITERATELIST(LINKUNIQUE, complex->m_attached[i], link) {
      if (!ModelIsLoaded(link->child, 1)) {
        return 0;
      }
    }
  }

  return 1;
}
BOOL IModelDerefHandle(CModel *model, CModelBase **unique, CModelShared **shared) {
  VALIDATEBEGIN;
  VALIDATE(model);
  *unique = 0;
  *shared = (CModelShared *)model->shared;
  VALIDATE(*shared);
  VALIDATEEND;

  if (model->state != CMODEL_LOADED) {
    return 0;
  }

  *unique = model->data;
  return 1;
}

BOOL IModelDerefHandle(CModel *model, CModelBase **unique) {
  VALIDATEBEGIN;
  VALIDATE(model);
  VALIDATEEND;

  *unique = 0;
  if (model->state != CMODEL_LOADED) {
    return 0;
  }

  *unique = model->data;
  return 1;
}

BOOL IModelDerefHandle(CModel *model, CModelShared **shared) {
  VALIDATEBEGIN;
  VALIDATE(model);
  *shared = (CModelShared *)model->shared;
  VALIDATE(*shared);
  VALIDATEEND;

  return model->state == CMODEL_LOADED;
}
