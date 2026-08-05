#include "IModel.h"
#include "ModelInternal.h"
#include "CollisionData.h"

#include "Base/Status.h"
#include "MDLFile/MDLTypes.h"
#include "Gxu/IGxuLight.h"
#include "Gx/CGxDevice.h"
#include "Services/AsyncFileRead.h"
#include "Services/Camera.h"
#include "Services/IParticleMisc.h"
#include "Services/Texture.h"
#include "Os/W32/Debugging.h"
#include "Os/OsTime.h"

#include <stpl.h>
#include <malloc.h>
#include <stdarg.h>

const char *CHandleObject::GetObjectName() {
  return 0;
}

void AnimInitialize();
void AnimDestroy();
void MDLFileInitialize();
void MDLFileDestroy();
int MDLFileRead(const char *path, MDLDATA *mdldata, CStatus *status);
int MdlReadValidate(const MDLDATA &data, CStatus *status);
HMODEL ModelCreate(const MDLDATA &source, CModelCreate *data, CStatus *status);
void ModelAnimateInitialize();
void ModelAnimateDestroy();
void ModelRenderInitialize();
void ModelRenderDestroy();
unsigned char *MDLFileBinarySeek(unsigned char *fileData, unsigned int fileBytes, unsigned long sectionTag);
unsigned char *MDLFileBinaryLoad(char *path, unsigned int *fileBytes, CStatus *status);
void MDLFileBinaryUnload(unsigned char *fileData);
HTEXTURE LoadModelTexture(const char *texturePath, unsigned int modelLoadFlags, CGxTexFlags texLoadFlags, CStatus *status);
void AnimSetObjectOrdering(HANIM anim, const char **boneNames, unsigned int numBones);
void AnimSetSequenceOrdering(HANIM anim, const char **sequenceNames, unsigned int numSequences);
void AnimSetSequenceOrderingDefault(HANIM anim);
void MdxReadTextures(unsigned char *, unsigned int, unsigned int, CModelComplex *, CStatus *);
void MdxReadTextures(unsigned char *, unsigned int, unsigned int, CModelSimple *, CStatus *);
void MdxReadMaterials(unsigned char *, unsigned int, unsigned int, CModelComplex *, CModelShared *);
void MdxReadMaterials(unsigned char *, unsigned int, unsigned int, CModelSimple *, CModelShared *);
void MdxReadGeosets(unsigned char *, unsigned int, unsigned int, CModelComplex *, CModelShared *);
void MdxReadGeosets(unsigned char *, unsigned int, unsigned int, CModelSimple *, CModelShared *);
void MdxLoadGlobalProperties(unsigned char *, unsigned int, unsigned int *, CModelShared *);
void MdxReadAttachments(unsigned char *, unsigned int, unsigned int, CModelComplex *, CModelShared *, CStatus *);
void MdxReadRibbonEmitters(unsigned char *, unsigned int, CModelComplex *, CModelShared *);
void MdxReadEmitters2(unsigned char *, unsigned int, unsigned int, CModelComplex *, CModelShared *, CStatus *);
void MdxReadLights(unsigned char *, unsigned int, CModelComplex *);
HCOLLISIONDATA CollisionDataCreate(unsigned char *, unsigned int);
HCOLLISIONDATA CollisionDataCreate(const MDLDATA &);
HANIM AnimCreate(const MDLDATA &, unsigned int, CStatus *);
unsigned int AnimBuildObjectIdTranslation(const MDLDATA &, unsigned int, TSStackArray<unsigned int> *);
int MdlReadCameras(const MDLDATA &, TSFixedArray<HCAMERA> *);
void MdlReadLoadGlobalProperties(const MDLDATA &, CModelShared *, unsigned int *);
int MdlReadLoadModel(const MDLDATA &, CModelComplex *, CModelShared *, unsigned int, CStatus *);
int MdlReadLoadModel(const MDLDATA &, CModelSimple *, CModelShared *, unsigned int, CStatus *);
int MdlReadLoadRibbonEmitters(const MDLDATA &, CModelComplex *, CModelShared *);
int MdlReadLoadEmitters2(const MDLDATA &, CModelComplex *, CModelShared *, unsigned int, CStatus *);
int MdlReadLoadLights(const MDLDATA &, CModelComplex *);
void ExecuteQueuedActions(CModel *model);
void IModelEnableFullAlpha(CModelBase *unique, int enable);

void ModelClearAllLinks(HMODEL parent);
void ModelEnableAnimBlending(HMODEL model, int enabled);
void ModelHideBounds(HMODEL model);
void ModelHideGeosets(HMODEL model, unsigned int selectionGroup, int hide);
void ModelHideGeosetsRange(HMODEL model, unsigned int selectionStart, unsigned int selectionEnd, int hide);
int ModelOptimizeVisibleGeosets(HMODEL model);
int ModelRemoveLink(HMODEL parent, unsigned int parentIndex, HMODEL child);
void ModelSetEmissiveColor(HMODEL model, const NTempest::CImVector &color, int doLinkedModels);
void ModelShowCollision(HMODEL model, int show);
void ModelShowCollisionAaBox(HMODEL model, int show);
void ModelShowModel(HMODEL model, int show);

class CHashKeyFilePath {
 public:
  CHashKeyFilePath() {
    path[0] = 0;
  }

  CHashKeyFilePath(const char *value) {
    SStrCopy(path, value, sizeof(path));
  }

  CHashKeyFilePath(const CHashKeyFilePath &source) {
    SStrCopy(path, source.path, sizeof(path));
  }

  bool operator==(const char *value) const {
    return SStrCmpI(path, value, 0x7FFFFFFF) == 0;
  }

  bool operator==(const CHashKeyFilePath &source) const {
    return operator==(source.path);
  }

  CHashKeyFilePath &operator=(const char *value) {
    SStrCopy(path, value, sizeof(path));
    return *this;
  }

  CHashKeyFilePath &operator=(const CHashKeyFilePath &source) {
    SStrCopy(path, source.path, sizeof(path));
    return *this;
  }

  char path[260];
};

struct CModelHash : public TSHashObject<CModelHash, CHashKeyFilePath> {
  CModelHash() : model(0), createFlags(0), timeStamp(0) {
  }

  ~CModelHash() {
    if (model) {
      HandleClose(model);
    }
  }

  HMODEL             model;
  unsigned int       createFlags;
  unsigned long      timeStamp;
  LINKDECLEX(CModelHash, link);
};

MDLBASE::MDLBASE() {
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

static TSCArray<unsigned char, 4194304>                  s_asyncLoadBuffer;
static LISTDECLEX(CAsyncObject, link, s_asyncLoadList);
static unsigned int                                      s_asyncLoadBufferUsed;
static int                                               s_asyncPending;
static TSHashTableReuse<CModelHash, CHashKeyFilePath, 1> s_modelCache;
static LISTDECLEX(CModelHash, link, s_modelCacheLRU);
static CNullStatus                                       s_nullStatus;
static LISTDECL(CModelModItem, s_freeModItems);

HMODEL ModelDuplicate(HMODEL sourceModel, unsigned int flags);
HMODEL IModelCreateBlocking(const char *fileName, char *actualPath, CModelCreate *data, CStatus *status);
HMODEL CreateDefaultModel(const char *fileName, unsigned int modelLoadFlags, CStatus *status);

static int ModelIsUsed(HMODEL model) {
  CModelShared *shared;

  IModelDerefHandle(reinterpret_cast<CModel *>(model), &shared);
  return reinterpret_cast<CModel *>(model)->GetRefCount() > 1 || reinterpret_cast<CHandleObject *>(shared)->GetRefCount() > 1;
}

static void ProcessAnimReorders(CModelBase *modelptr, CModelCreate *data) {
  ASSERT(modelptr->m_anim);

  if (!data) {
    AnimSetSequenceOrderingDefault(modelptr->m_anim);
    return;
  }

  if ((modelptr->m_flags & 0x20) && (data->flags & 0x40)) {
    CModelComplex *complex = reinterpret_cast<CModelComplex *>(modelptr);
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

static HMODEL GetModel(const char *modelFName, CModelCreate *data) {
  ASSERT(modelFName);
  char fileName[260];
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
  if (IModelDerefHandle(reinterpret_cast<CModel *>(duplicate), &modelptr) && modelptr->m_anim) {
    ProcessAnimReorders(modelptr, data);
  }
  return duplicate;
}

static void HashNewModel(const char *modelFName, HMODEL model, unsigned int createFlags, CStatus *status) {
  char          filePath[260];
  unsigned long currentTime;

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
  entry->model = ModelDuplicate(model, createFlags);
  entry->createFlags = createFlags;
  entry->timeStamp = currentTime;
}

static int MdlReadLoadNumMatrices(const MDLDATA& data, CModelShared* shared, unsigned int flags) {
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

static void MdxReadNumMatrices(unsigned char *data, unsigned int fileBytes, unsigned int flags, CModelShared *shared) {
  ASSERT(shared);

  unsigned char *section = MDLFileBinarySeek(data, fileBytes, 0x454E4F42);
  if (!section) {
    shared->numBones = 0;
    return;
  }

  if (flags & 0x100) {
    shared->numBones = 1;
    return;
  }

  shared->numBones = *reinterpret_cast<unsigned int *>(section + 4);
  if (flags & 0x20) {
    section = MDLFileBinarySeek(data, fileBytes, 0x54535448);
    if (section) {
      shared->numBones += *reinterpret_cast<unsigned int *>(section + 4);
    }
  }

  section = MDLFileBinarySeek(data, fileBytes, 0x4E415854);
  if (section) {
    shared->numTexBones = *reinterpret_cast<unsigned int *>(section + 4);
  }
}

static unsigned int ConvertAnimCreateFlags(unsigned int loadFlags) {
  unsigned int createFlags = 0;

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

static int MdlReadLoadAnim(const MDLDATA& data, CModelBase* modelptr, unsigned int loadFlags, CStatus* status) {
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

static int MdxReadAnimation(unsigned char *fileData, unsigned int fileBytes, CModelBase *modelptr, unsigned int loadFlags) {
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

static void MdxReadHitTestData(unsigned char *data, unsigned int fileBytes, CModelComplex *modelptr, CModelShared *shared) {
  unsigned int *shapeDone;
  unsigned int *dataDone;
  unsigned int  i;
  unsigned int  numShapes;

  FATALASSERT(data);
  FATALASSERT(modelptr);
  FATALASSERT(shared);

  data = MDLFileBinarySeek(data, fileBytes, 0x54535448);
  if (!data) {
    return;
  }

  dataDone = reinterpret_cast<unsigned int *>(data + 4 + *reinterpret_cast<unsigned int *>(data));
  numShapes = *reinterpret_cast<unsigned int *>(data + 4);
  data += 8;

  shared->hitTest.SetCount(numShapes);
  modelptr->m_hitTestMtx.SetCount(numShapes);

  for (i = 0; i < numShapes; ++i) {
    shapeDone = reinterpret_cast<unsigned int *>(data + *reinterpret_cast<unsigned int *>(data));
    data += *reinterpret_cast<unsigned int *>(data + 4) + 4;

    switch (*data++) {
      case COLLIDE_BOX:
        shared->hitTest[i].type = COLLIDE_BOX;
        memcpy(shared->hitTest[i].extent, data, sizeof(shared->hitTest[i].extent));
        data += sizeof(shared->hitTest[i].extent);
        break;

      case COLLIDE_CYLINDER:
        shared->hitTest[i].type = COLLIDE_CYLINDER;
        shared->hitTest[i].extent[0] = *reinterpret_cast<NTempest::C3Vector *>(data);
        data += sizeof(NTempest::C3Vector);
        shared->hitTest[i].extent[1] = shared->hitTest[i].extent[0];
        shared->hitTest[i].extent[1].z += *reinterpret_cast<float *>(data);
        data += sizeof(float);
        shared->hitTest[i].radius = *reinterpret_cast<float *>(data);
        data += sizeof(float);
        break;

      case COLLIDE_SPHERE:
        shared->hitTest[i].type = COLLIDE_SPHERE;
        shared->hitTest[i].extent[0] = *reinterpret_cast<NTempest::C3Vector *>(data);
        data += sizeof(NTempest::C3Vector);
        shared->hitTest[i].radius = *reinterpret_cast<float *>(data);
        data += sizeof(float);
        break;

      case COLLIDE_PLANE:
        shared->hitTest[i].type = COLLIDE_PLANE;
        shared->hitTest[i].extent[0].x = *reinterpret_cast<float *>(data);
        data += sizeof(float);
        shared->hitTest[i].extent[0].y = *reinterpret_cast<float *>(data);
        data += sizeof(float);
        break;
    }

    FATALASSERT(shapeDone == reinterpret_cast<unsigned int *>(data));
    FATALASSERT(dataDone >= reinterpret_cast<unsigned int *>(data));
  }

  FATALASSERT(dataDone == reinterpret_cast<unsigned int *>(data));
}

static int MdlReadLoadHitTestData(const MDLDATA& data, CModelComplex* modelptr, CModelShared* shared) {
  ASSERT(modelptr);
  ASSERT(shared);

  unsigned int numShapes = data.hitTestShapes.Count();
  shared->hitTest.SetCount(numShapes);
  modelptr->m_hitTestMtx.SetCount(numShapes);
  for (unsigned int i = 0; i < numShapes; ++i) {
    const MDLHITTESTSHAPE &source = data.hitTestShapes[i];
    CHitTest &dest = shared->hitTest[i];
    switch (source.type) {
      case SHAPE_BOX:
        dest.type = COLLIDE_BOX;
        dest.extent[0] = NTempest::C3Vector(source.shape.box.minimum.x, source.shape.box.minimum.y, source.shape.box.minimum.z);
        dest.extent[1] = NTempest::C3Vector(source.shape.box.maximum.x, source.shape.box.maximum.y, source.shape.box.maximum.z);
        break;
      case SHAPE_CYLINDER:
        dest.type = COLLIDE_CYLINDER;
        dest.extent[0] = NTempest::C3Vector(source.shape.cylinder.base.x, source.shape.cylinder.base.y, source.shape.cylinder.base.z);
        dest.extent[1] = dest.extent[0];
        dest.extent[1].z += source.shape.cylinder.height;
        dest.radius = source.shape.cylinder.radius;
        break;
      case SHAPE_SPHERE:
        dest.type = COLLIDE_SPHERE;
        dest.extent[0] = NTempest::C3Vector(source.shape.sphere.center.x, source.shape.sphere.center.y, source.shape.sphere.center.z);
        dest.radius = source.shape.sphere.radius;
        break;
      case SHAPE_PLANE:
        dest.type = COLLIDE_PLANE;
        dest.extent[0].x = source.shape.plane.length;
        dest.extent[0].y = source.shape.plane.width;
        break;
      default:
        break;
    }
  }
  return 1;
}

static void ComputeBoundingRadius(const CGeosetShared *geosets, unsigned int numGeosets, const NTempest::C3Vector &center, float *radius) {
  float bestDistSqd = 0.0f;

  while (numGeosets) {
    unsigned int numVertices = geosets->position.Count();
    for (unsigned int i = 0; i < numVertices; ++i) {
      NTempest::C3Vector delta = center - geosets->position[i];
      float              distSqd = delta.SquaredMag();
      if (distSqd > bestDistSqd) {
        bestDistSqd = distSqd;
      }
    }
    ++geosets;
    --numGeosets;
  }

  *radius = NTempest::CMath::sqrt_(bestDistSqd);
}

static void ComputeBoundingBox(const CGeosetShared *geosets, unsigned int numGeosets, NTempest::CAaBox *extent) {
  *extent = NTempest::CAaBox::Bounding(geosets[0].position.Ptr(), geosets[0].position.Count());

  while (numGeosets) {
    NTempest::CAaBox geosetExtent = NTempest::CAaBox::Bounding(geosets->position.Ptr(), geosets->position.Count());
    extent->b = NTempest::C3Vector::Min(extent->b, geosetExtent.b);
    extent->t = NTempest::C3Vector::Max(extent->t, geosetExtent.t);
    ++geosets;
    --numGeosets;
  }
}

static void IModelComputeBounds(CModelShared *shared) {
  ASSERT(shared);
  ComputeBoundingBox(shared->geosets.Ptr(), shared->geosets.Count(), &shared->bounds.extent);
  shared->bounds.sphere.c = (shared->bounds.extent.b + shared->bounds.extent.t) * 0.5f;
  ComputeBoundingRadius(shared->geosets.Ptr(), shared->geosets.Count(), shared->bounds.sphere.c, &shared->bounds.sphere.r);
}

static int MdlReadLoadExtents(const MDLDATA& data, CModelBase* modelptr, CModelShared* shared) {
  ASSERT(modelptr);
  ASSERT(shared);

  shared->bounds.extent = data.model.bounds.extent;
  shared->bounds.sphere.c = (shared->bounds.extent.b + shared->bounds.extent.t) * 0.5f;
  shared->bounds.sphere.r = data.model.bounds.radius;

  if (modelptr->m_anim && AnimNeedsSequenceBounds(modelptr->m_anim)) {
    unsigned int count = data.sequences.Count();
    shared->seqBounds.SetCount(count);
    for (unsigned int i = 0; i < count; ++i) {
      shared->seqBounds[i].extent = data.sequences[i].bounds.extent;
      shared->seqBounds[i].sphere.c =
          (shared->seqBounds[i].extent.b + shared->seqBounds[i].extent.t) * 0.5f;
      shared->seqBounds[i].sphere.r = data.sequences[i].bounds.radius;
    }
  } else if (fabs(shared->bounds.sphere.r) < 0.00000023841858f && data.sequences.Count()) {
    shared->bounds.extent = data.sequences[0].bounds.extent;
    shared->bounds.sphere.c = (shared->bounds.extent.b + shared->bounds.extent.t) * 0.5f;
    shared->bounds.sphere.r = data.sequences[0].bounds.radius;
  }
  return 1;
}

static unsigned char *LoadBoundsData(unsigned char *data, CBoundsData *bounds) {
  bounds->sphere.r = *reinterpret_cast<float *>(data);
  data += sizeof(float);

  bounds->extent.b = *reinterpret_cast<NTempest::C3Vector *>(data);
  data += sizeof(NTempest::C3Vector);
  bounds->extent.t = *reinterpret_cast<NTempest::C3Vector *>(data);
  data += sizeof(NTempest::C3Vector);

  bounds->sphere.c = (bounds->extent.b + bounds->extent.t) * 0.5f;
  return data;
}

static void MdxReadExtents(unsigned char *data, unsigned int fileBytes, CModelBase *modelptr, CModelShared *shared) {
  ASSERT(modelptr);
  ASSERT(shared);

  unsigned char *globalData = MDLFileBinarySeek(data, fileBytes, 0x4C444F4D);
  ASSERT(globalData);
  LoadBoundsData(globalData + 0x158, &shared->bounds);

  globalData = MDLFileBinarySeek(data, fileBytes, 0x53514553);
  if (!globalData) {
    return;
  }

  unsigned int numSequences = *reinterpret_cast<unsigned int *>(globalData + 4);
  if (!numSequences) {
    return;
  }

  if (modelptr->m_anim && AnimNeedsSequenceBounds(modelptr->m_anim)) {
    shared->seqBounds.SetCount(numSequences);
    globalData += 8;
    CBoundsData *bounds = shared->seqBounds.Ptr();
    while (numSequences--) {
      globalData = LoadBoundsData(globalData + 0x60, bounds++);
      globalData += 0x10;
    }
  } else if (NTempest::CMath::fabs_(shared->bounds.sphere.r) < 0.00000023841858f) {
    globalData = MDLFileBinarySeek(data, fileBytes, 0x53514553);
    if (globalData) {
      LoadBoundsData(globalData + 0x68, &shared->bounds);
    }
  }
}

static int MdlReadLoadPositions(const MDLDATA& data, unsigned int flags, CModelShared* shared) {
  ASSERT(shared);
  unsigned int numPivots = data.pivotPoints.Count();
  if (!numPivots) {
    return 1;
  }

  if ((flags & 0x220) == 0x20 || (!data.hitTestShapes.Count() && !data.lights.Count())) {
    shared->positions.SetCount(numPivots);
    for (unsigned int i = 0; i < numPivots; ++i) {
      shared->positions[i] = data.pivotPoints[i];
    }
    return 1;
  }

  TSStackArray<unsigned int> idConversion(_alloca(numPivots * sizeof(unsigned int)), numPivots, numPivots);
  unsigned int numEmitters = AnimBuildObjectIdTranslation(data, ConvertAnimCreateFlags(flags), &idConversion);
  shared->positions.SetCount(numPivots - numEmitters);
  unsigned int i;
  for (i = 0; i < numPivots; ++i) {
    if (idConversion[i] != static_cast<unsigned int>(-1)) {
      shared->positions[idConversion[i]] = data.pivotPoints[i];
    }
  }
  for (i = 0; i < shared->emitter2Order.Count(); ++i) {
    shared->emitter2Order[i] = idConversion[shared->emitter2Order[i]];
  }
  return 1;
}

static void MdxReadPositions(unsigned char *fileData, unsigned int fileBytes, unsigned int flags, CModelShared *shared) {
  unsigned char *section = MDLFileBinarySeek(fileData, fileBytes, 0x54564950);
  if (!section) {
    return;
  }

  unsigned int sectionBytes = *reinterpret_cast<unsigned int *>(section);
  ASSERT(sectionBytes % sizeof(NTempest::C3Vector) == 0);
  unsigned int        numPivots = sectionBytes / sizeof(NTempest::C3Vector);
  NTempest::C3Vector *source = reinterpret_cast<NTempest::C3Vector *>(section + 4);

  if ((flags & 0x220) == 0x20) {
    shared->positions.SetCount(numPivots);
    memcpy(shared->positions.Ptr(), source, sectionBytes);
    return;
  }

  TSStackArray<unsigned int> idConversion(_alloca(numPivots * sizeof(unsigned int)), numPivots, numPivots);
  unsigned int numEmitters = AnimBuildObjectIdTranslation(fileData, fileBytes, ConvertAnimCreateFlags(flags), idConversion.Ptr(), numPivots);
  if (!numEmitters) {
    shared->positions.SetCount(numPivots);
    memcpy(shared->positions.Ptr(), source, sectionBytes);
    return;
  }

  shared->positions.SetCount(numPivots - numEmitters);
  unsigned int i;
  for (i = 0; i < numPivots; ++i) {
    if (idConversion[i] != static_cast<unsigned int>(-1)) {
      shared->positions[idConversion[i]] = source[i];
    }
  }
  for (i = 0; i < shared->emitter2Order.Count(); ++i) {
    ASSERT(shared->emitter2Order[i] < numPivots);
    shared->emitter2Order[i] = idConversion[shared->emitter2Order[i]];
  }
}

static CModelShared *CreateSharedModelData(const char *fileName) {
  void         *storage = SMemAlloc(sizeof(CModelShared), "HMODELSHARED", SERR_LINECODE_OBJECT, 0);
  CModelShared *shared = storage ? new (storage) CModelShared : 0;

  ASSERT(shared);
  SStrCopy(shared->name, fileName, sizeof(shared->name));
  return shared;
}

int IsSimpleModel(const MDLDATA &source) {
  return source.geosets.Count() <= 5 && source.materials.Count() <= 4 && source.textures.Count() <= 4 && !source.lights.Count() &&
         !source.attachments.Count() && !source.particleEmitters2.Count() && !source.ribbonEmitters.Count() && !source.cameras.Count() &&
         !source.hitTestShapes.Count();
}

static unsigned int GetSectionCount(unsigned char *fileData, unsigned int fileBytes, unsigned long sectionTag) {
  unsigned char *section = MDLFileBinarySeek(fileData, fileBytes, sectionTag);
  return section ? *reinterpret_cast<unsigned int *>(section + 4) : 0;
}

static unsigned int GetTextureCount(unsigned char *fileData, unsigned int fileBytes) {
  unsigned char *section = MDLFileBinarySeek(fileData, fileBytes, 0x53584554);
  return section ? *reinterpret_cast<unsigned int *>(section) / 0x10C : 0;
}

static int IsSimpleModel(unsigned char *fileData, unsigned int fileBytes) {
  if (GetSectionCount(fileData, fileBytes, 0x534C544D) > 4 || GetTextureCount(fileData, fileBytes) > 4 ||
      GetSectionCount(fileData, fileBytes, 0x534F4547) > 5)
  {
    return 0;
  }

  if (GetSectionCount(fileData, fileBytes, 0x4554494C) || GetSectionCount(fileData, fileBytes, 0x48435441) ||
      GetSectionCount(fileData, fileBytes, 0x32455250) || GetSectionCount(fileData, fileBytes, 0x42424952) ||
      GetSectionCount(fileData, fileBytes, 0x534D4143) || GetSectionCount(fileData, fileBytes, 0x54535448))
  {
    return 0;
  }

  return 1;
}

static void BuildSimpleModelFromMdxData(
    unsigned char *fileData,
    unsigned int   fileBytes,
    CModelSimple  *modelptr,
    CModelShared  *shared,
    unsigned int   flags,
    CStatus       *status
) {
  MdxReadTextures(fileData, fileBytes, flags, modelptr, status);
  MdxReadMaterials(fileData, fileBytes, flags, modelptr, shared);
  MdxReadGeosets(fileData, fileBytes, flags, modelptr, shared);
  if (!(flags & 0x100)) {
    MdxReadAnimation(fileData, fileBytes, modelptr, flags);
  }
  MdxReadNumMatrices(fileData, fileBytes, flags, shared);
  if (flags & 0x80) {
    IModelEnableFullAlpha(modelptr, 1);
  }
  shared->collision = CollisionDataCreate(fileData, fileBytes);
  MdxReadExtents(fileData, fileBytes, modelptr, shared);
  MdxReadPositions(fileData, fileBytes, flags, shared);
}

static void BuildModelFromMdxData(
    unsigned char *fileData,
    unsigned int   fileBytes,
    CModelBase    *baseModel,
    CModelShared  *shared,
    unsigned int   flags,
    CStatus       *status
) {
  MdxLoadGlobalProperties(fileData, fileBytes, &flags, shared);
  if (!(baseModel->m_flags & 0x20)) {
    BuildSimpleModelFromMdxData(fileData, fileBytes, static_cast<CModelSimple *>(baseModel), shared, flags, status);
    return;
  }

  CModelComplex *modelptr = static_cast<CModelComplex *>(baseModel);
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
    IModelEnableFullAlpha(modelptr, 1);
  }
  if (!(flags & 0x200)) {
    MdxReadLights(fileData, fileBytes, modelptr);
  }
  shared->collision = CollisionDataCreate(fileData, fileBytes);
  MdxReadExtents(fileData, fileBytes, modelptr, shared);
  MdxReadPositions(fileData, fileBytes, flags, shared);
  MdxReadCameras(fileData, fileBytes, &modelptr->m_cameras);
}

static int BuildSimpleModelFromMdlData(const MDLDATA& source, CModelSimple* modelptr, CModelShared* shared, unsigned int flags, CStatus* status) {
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
    IModelEnableFullAlpha(modelptr, 1);
  }
  shared->collision = CollisionDataCreate(source);
  return MdlReadLoadExtents(source, modelptr, shared) && MdlReadLoadPositions(source, flags, shared);
}

static int BuildModelFromMdlData(const MDLDATA& source, CModelBase* baseModel, CModelShared* shared, unsigned int flags, CStatus* status) {
  MdlReadLoadGlobalProperties(source, shared, &flags);
  if (!(baseModel->m_flags & 0x20)) {
    return BuildSimpleModelFromMdlData(source, static_cast<CModelSimple *>(baseModel), shared, flags, status);
  }

  CModelComplex *modelptr = static_cast<CModelComplex *>(baseModel);
  if (!MdlReadLoadModel(source, modelptr, shared, flags, status)) {
    return 0;
  }
  if (!(flags & 0x100) &&
      (!MdlReadLoadAnim(source, baseModel, flags, status) ||
       !MdlReadLoadRibbonEmitters(source, modelptr, shared))) {
    return 0;
  }
  if (!MdlReadLoadEmitters2(source, modelptr, shared, flags, status) ||
      !MdlReadLoadNumMatrices(source, shared, flags)) {
    return 0;
  }
  if ((flags & 0x20) && !MdlReadLoadHitTestData(source, modelptr, shared)) {
    return 0;
  }
  if (flags & 0x80) {
    IModelEnableFullAlpha(baseModel, 1);
  }
  if (!(flags & 0x200) && !MdlReadLoadLights(source, modelptr)) {
    return 0;
  }
  shared->collision = CollisionDataCreate(source);
  return MdlReadLoadExtents(source, baseModel, shared) &&
         MdlReadLoadPositions(source, flags, shared) &&
         MdlReadCameras(source, &modelptr->m_cameras);
}

HMATERIAL BuildSimpleMaterial(
    CModelTexture *texData,
    unsigned int   textureId,
    HTEXTURE       texture,
    EGxBlend       blendMode,
    unsigned int   disables,
    unsigned int   replaceableId
) {
  CMaterial *unique = NEW(CMaterial);
  ASSERT(unique);

  CTexLayer *layer = unique->layers.New();
  ASSERT(layer);

  texData->handle = static_cast<HTEXTURE>(HandleDuplicate(texture));
  texData->replaceableId = replaceableId;

  layer->vertexFormat = GxVBF_PCT0;
  layer->blendMode = blendMode;
  layer->tmuPass[0].textureId = textureId;
  layer->tmuPass[0].combiner = GxTexBlend_Mod;

  if (disables & 0x01) {
    layer->disables |= 0x01;
  }
  if (disables & 0x10) {
    layer->disables |= 0x10;
  }
  if (disables & 0x20) {
    layer->disables |= 0x02;
  }
  if (disables & 0x40) {
    layer->disables |= 0x04;
  }
  if (disables & 0x80) {
    layer->disables |= 0x08;
  }

  CMaterialShared *shared = NEW(CMaterialShared);
  ASSERT(shared);
  shared->priorityPlane = 0;

  CTexLayerShared *sharedLayer = shared->layers.New();
  ASSERT(sharedLayer);
  sharedLayer->blendMode = blendMode;
  sharedLayer->tmuPass[0].transformId = static_cast<unsigned int>(-1);
  sharedLayer->tmuPass[0].coordId = 0;

  unique->data = static_cast<HMATERIALSHARED>(HandleCreate(shared, "HMATERIALSHARED"));
  return static_cast<HMATERIAL>(HandleCreate(unique, "HMATERIAL"));
}

static void BuildSimpleGeoset(
    unsigned int              numVertices,
    const NTempest::C3Vector *position,
    const NTempest::C3Vector *normal,
    const NTempest::C2Vector *texCoord,
    EGxPrim                   primitiveType,
    const unsigned short     *primitiveVertices,
    unsigned int              numPrimVertices,
    unsigned int              materialId,
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

HMODEL CreateDefaultModel(const char *fileName, unsigned int modelLoadFlags, CStatus *status) {
  status->Add(STATUS_WARNING, "Warning, model %s failed to load\n", fileName);

  HTEXTURE texture = LoadModelTexture("Textures\\ShaneCube", modelLoadFlags, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), status);
  ASSERT(texture);

  NTempest::CAaBox bounds;
  bounds.b = NTempest::C3Vector(-0.5f, -0.5f, 0.0f);
  bounds.t = NTempest::C3Vector(0.5f, 0.5f, 1.0f);
  HMODEL model = CreateModelBoundingBox(bounds, texture, GxBlend_Opaque);

  CModelShared *shared = 0;
  IModelDerefHandle(reinterpret_cast<CModel *>(model), &shared);
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

void ModelRemoveFromCache(const char *sourcefile) {
  char        filePath[260];
  char       *extension;
  CModelHash *modelHash;

  ASSERT(sourcefile);

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

int ModelCacheUpdate(unsigned long currentTime, CStatus *status) {
  unsigned int numModelsFlushed = 0;

  while (CModelHash *modelHash = s_modelCacheLRU.Head()) {
    if (numModelsFlushed >= 4) {
      break;
    }

    if (static_cast<long>(currentTime - modelHash->timeStamp) < 30000L) {
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

HMODEL ModelGetModel(const char *sourcefile, CModelCreate *data) {
  FATALASSERT(sourcefile);
  return GetModel(sourcefile, data);
}

HMODEL IModelCreateBlocking(const char *fileName, char *actualPath, CModelCreate *data, CStatus *status) {
  unsigned int   fileBytes;
  CModelShared  *shared;
  unsigned char *fileData;
  unsigned int   createFlags;

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

  CModel *model = NEW(CModel);
  ASSERT(model);
  model->data = modelptr;
  model->shared = static_cast<HMODELSHARED>(HandleCreate(shared, "HMODELSHARED"));
  ASSERT(model->shared);
  model->state = CMODEL_LOADED;

  HMODEL handle = static_cast<HMODEL>(HandleCreate(model, "HMODEL"));
  HashNewModel(fileName, handle, createFlags, status);
  return handle;
}

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
  unsigned int  bufferRemaining;
  CAsyncObject *asyncObject;
  CAsyncObject *asyncObjectnext_node;

  ASSERT(s_asyncPending >= 0);

  if (s_asyncPending) {
    return;
  }

  s_asyncLoadBufferUsed = 0;
  bufferRemaining = s_asyncLoadBuffer.MaxCount();

  for (asyncObject = s_asyncLoadList.Head(); asyncObject; asyncObject = asyncObjectnext_node) {
    asyncObjectnext_node = s_asyncLoadList.Next(asyncObject);

    if (bufferRemaining < asyncObject->size) {
      continue;
    }

    asyncObject->link.Unlink();
    asyncObject->canReorder = 1;
    asyncObject->buffer = &s_asyncLoadBuffer[s_asyncLoadBufferUsed];
    s_asyncLoadBufferUsed += asyncObject->size;
    bufferRemaining -= asyncObject->size;
    ++s_asyncPending;
    AsyncFileReadObject(asyncObject);
  }
}

static void AsnycModelPostLoadCallback(void *userArg) {
  CModel        *model = static_cast<CModel *>(userArg);
  CStatus        status;
  unsigned char *fileData;
  unsigned int   fileBytes;
  CModelShared  *shared;

  ASSERT(model);
  ASSERT(model->state == CMODEL_ASYNC_WAIT);
  shared = reinterpret_cast<CModelShared *>(model->shared);
  ASSERT(shared);

  fileData = static_cast<unsigned char *>(model->asyncObject->buffer);
  ASSERT(*reinterpret_cast<unsigned int *>(fileData) == 0x584C444D);
  fileBytes = model->asyncObject->size - 4;
  fileData += 4;
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
  DEL(model->createData);
  model->data = modelptr;
  model->state = CMODEL_LOADED;
  ExecuteQueuedActions(model);
}

static HMODEL IModelCreate(const char *fileName, char *actualPath, CModelCreate *createData, CStatus *status) {
  ASSERT(status);
  OsOutputDebugString("Model: (INFO) : Loading \"%s\"\n", actualPath);

  if (createData && (createData->flags & 0x8000)) {
    return IModelCreateBlocking(fileName, actualPath, createData, status);
  }

  CModelCreate filler;
  memset(&filler, 0, sizeof(filler));
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

  CModel *model = NEW(CModel);
  ASSERT(model);
  model->asyncObject = AsyncFileReadCreateObject();
  ASSERT(model->asyncObject);
  model->shared = static_cast<HMODELSHARED>(HandleCreate(CreateSharedModelData(fileName), "HMODELSHARED"));
  ASSERT(model->shared);
  model->createData = NEW(CModelCreate);
  *model->createData = *createData;

  CAsyncObject *asyncObject = model->asyncObject;
  asyncObject->userArg = model;
  asyncObject->userPostloadCallback = AsnycModelPostLoadCallback;
  asyncObject->file = file;
  asyncObject->offset = 0;
  asyncObject->size = SFile::GetFileSize(file, 0);

  if (s_asyncLoadBuffer.MaxCount() - s_asyncLoadBufferUsed < asyncObject->size) {
    asyncObject->canReorder = 0;
    s_asyncLoadList.LinkNode(asyncObject, LIST_TAIL, 0);
  } else {
    asyncObject->buffer = &s_asyncLoadBuffer[s_asyncLoadBufferUsed];
    asyncObject->canReorder = 1;
    s_asyncLoadBufferUsed += asyncObject->size;
    ++s_asyncPending;
    AsyncFileReadObject(asyncObject);
  }

  HMODEL handle = static_cast<HMODEL>(HandleCreate(model, "HMODEL"));
  ASSERT(handle);
  HashNewModel(fileName, handle, createData->flags, status);
  return handle;
}

static int IsBinaryFile(char *path) {
  unsigned int length = SStrLen(path);
  char         lastCharacter = path[length - 1];

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

HMODEL ModelCreate(const char *sourcefile, CModelCreate *data, CStatus *status) {
  ASSERT(sourcefile);
  ASSERT(sourcefile[0]);

  CStatus *useStatus = status ? status : &s_nullStatus;
  HMODEL   model = GetModel(sourcefile, data);
  if (model) {
    return model;
  }

  char actualPath[260];
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
  ASSERT(status);
  ASSERT(static_cast<const char *>(source.header.sourceFilename)[0]);

  OsOutputDebugString("Model: (INFO) : Loading \"%s\"\n", static_cast<const char *>(source.header.sourceFilename));

  unsigned int createFlags = data ? data->flags : 0;
  if (!MdlReadValidate(source, status)) {
    return (createFlags & 0x2000)
               ? CreateDefaultModel(source.header.sourceFilename, createFlags, status)
               : 0;
  }

  CModelBase *modelptr;
  if (IsSimpleModel(source)) {
    modelptr = NEW(CModelSimple);
  } else {
    modelptr = NEW(CModelComplex);
  }
  ASSERT(modelptr);

  CModelShared *shared = CreateSharedModelData(source.header.sourceFilename);
  if (!BuildModelFromMdlData(source, modelptr, shared, createFlags, status)) {
    DEL(modelptr);
    DEL(shared);
    return (createFlags & 0x2000)
               ? CreateDefaultModel(source.header.sourceFilename, createFlags, status)
               : 0;
  }

  if (modelptr->m_anim) {
    ProcessAnimReorders(modelptr, data);
  }

  CModel *model = NEW(CModel);
  ASSERT(model);
  model->data = modelptr;
  model->shared = static_cast<HMODELSHARED>(HandleCreate(shared, "HMODELSHARED"));
  ASSERT(model->shared);

  HMODEL modelHandle = static_cast<HMODEL>(HandleCreate(model, "HMODEL"));
  ASSERT(modelHandle);
  model->state = CMODEL_LOADED;
  HashNewModel(source.header.sourceFilename, modelHandle, createFlags, status);
  return modelHandle;
}

static CModel *IModelCreateSimpleEmpty(const char *name) {
  CModel       *model = NEW(CModel);
  CModelShared *shared = NEW(CModelShared);
  CModelSimple *modelptr = NEW(CModelSimple);
  ASSERT(model);
  ASSERT(shared);
  ASSERT(modelptr);

  shared->numBones = 1;
  modelptr->m_geosets.SetCount(1);
  modelptr->m_geosetColor.SetCount(1);
  modelptr->m_materials.SetCount(1);
  modelptr->m_textures.SetCount(1);
  shared->geosets.SetCount(1);
  shared->numGeosets = 1;

  model->shared = static_cast<HMODELSHARED>(HandleCreate(shared, "HMODELSHARED"));
  ASSERT(model->shared);
  model->data = modelptr;
  SStrCopy(shared->name, name ? name : "Custom Built Model", sizeof(shared->name));
  return model;
}

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
) {
  CModel *model = IModelCreateSimpleEmpty(name);
  ASSERT(model);

  CModelSimple *modelptr = static_cast<CModelSimple *>(model->data);
  CModelShared *shared = reinterpret_cast<CModelShared *>(model->shared);

  HMATERIAL material = BuildSimpleMaterial(&modelptr->m_textures[0], 0, texture, blendMode, disables, replaceableId);
  modelptr->m_materials[0] = material;
  modelptr->m_geosetColor[0].animatedColor = color;

  BuildSimpleGeoset(numVertices, position, normal, texCoord, primitiveType, primitiveVertices, numPrimVertices, 0, &shared->geosets[0]);
  IModelComputeBounds(shared);
  model->state = CMODEL_LOADED;
  return static_cast<HMODEL>(HandleCreate(model, "HMODEL"));
}

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
) {
  CModelBase   *modelptr;
  CModelShared *shared;
  if (!IModelDerefHandle(reinterpret_cast<CModel *>(model), &modelptr, &shared) || !(modelptr->m_flags & 0x20)) {
    return 0;
  }

  CModelComplex *complex = static_cast<CModelComplex *>(modelptr);
  unsigned int   materialId = complex->m_materials.Count();

  HMATERIAL material = BuildSimpleMaterial(complex->m_textures.New(), materialId, texture, blendMode, disables, replaceableId);
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

HMODEL ModelDuplicate(HMODEL sourceModel, unsigned int flags) {
  if (!sourceModel) {
    return 0;
  }

  CModel *source = reinterpret_cast<CModel *>(sourceModel);
  if (source->state == CMODEL_LOADED && (flags & 1)) {
    source->data->m_flags |= 4;
  }

  void   *storage = SMemAlloc(sizeof(CModel), "HMODEL", SERR_LINECODE_OBJECT, 0);
  CModel *copy = storage ? new (storage) CModel(*source) : 0;
  ASSERT(copy);

  if (copy->state != CMODEL_LOADED && (flags & 1)) {
    copy->state = CMODEL_DUPE_WAIT_PRSRV_ANIM;
  }

  if (source->state == CMODEL_LOADED) {
    source->data->m_flags &= ~4U;
  }

  HMODEL duplicate = static_cast<HMODEL>(HandleCreate(copy, "HMODEL"));
  if (copy->state != CMODEL_LOADED) {
    EnqueueModelCommand(source, MODEL_FINISH_DUPLICATION, duplicate);
  }
  return duplicate;
}

void EnqueueModelCommand(CModel *model, EModelModQ command, ...) {
  CModelModItem *modItem = s_freeModItems.Head();
  unsigned char *paramData;
  va_list        arguments;
  unsigned int   i;

  if (modItem) {
    modItem->Unlink();
    model->modelModQueue.LinkNode(modItem, LIST_TAIL, 0);
  } else {
    modItem = model->modelModQueue.NewNode(LIST_TAIL, 0, 0);
  }

  modItem->action = command;
  paramData = modItem->paramData;
  va_start(arguments, command);

  for (i = 0; i < 4; ++i) {
    switch (s_modelParamTypes[command][i]) {
      case MPARAM_UINT:
      case MPARAM_PTR:
        *reinterpret_cast<unsigned int *>(paramData) = va_arg(arguments, unsigned int);
        paramData += sizeof(unsigned int);
        break;

      case MPARAM_HANDLE:
        *reinterpret_cast<HOBJECT *>(paramData) = HandleDuplicate(va_arg(arguments, HOBJECT));
        paramData += sizeof(HOBJECT);
        break;

      case MPARAM_FLOAT:
        *reinterpret_cast<float *>(paramData) = static_cast<float>(va_arg(arguments, double));
        paramData += sizeof(float);
        break;

      case MPARAM_C3VECTOR:
        *reinterpret_cast<NTempest::C3Vector *>(paramData) = va_arg(arguments, NTempest::C3Vector);
        paramData += sizeof(NTempest::C3Vector);
        break;

      case MPARAM_BOOL:
      case MPARAM_BYTE:
        *paramData++ = static_cast<unsigned char>(va_arg(arguments, int));
        break;

      case MPARAM_CARGB:
        *reinterpret_cast<unsigned long *>(paramData) = va_arg(arguments, unsigned long);
        paramData += sizeof(unsigned long);
        break;

      case MPARAM_NONE:
        va_end(arguments);
        return;
    }
  }

  va_end(arguments);
}

void ExecuteQueuedActions(CModel *model) {
  CModelModItem *item = model->modelModQueue.Head();
  HMODEL         modelHandle = 0;

  while (item) {
    CModelModItem *next = model->modelModQueue.Next(item);
    if (!modelHandle) {
      modelHandle = static_cast<HMODEL>(HandleCreate(model, "HMODEL"));
    }

    item->Unlink();
    unsigned char *param = item->paramData;

    switch (item->action) {
      case MODEL_ADD_LINK: {
        HMODEL child = *reinterpret_cast<HMODEL *>(param + 4);
        ModelAddLink(modelHandle, *reinterpret_cast<unsigned int *>(param), child, *reinterpret_cast<float *>(param + 8));
        HandleClose(child);
        break;
      }

      case MODEL_APPLY_OBJECT_FACE_DIR:
        ModelApplyObjectFaceDir(modelHandle, *reinterpret_cast<unsigned int *>(param), *reinterpret_cast<NTempest::C3Vector *>(param + 4));
        break;

      case MODEL_APPLY_OBJECT_LOOK_AT:
        ModelApplyObjectLookAt(modelHandle, *reinterpret_cast<unsigned int *>(param), *reinterpret_cast<NTempest::C3Vector *>(param + 4));
        break;

      case MODEL_CLEAR_ALL_LINKS:
        ModelClearAllLinks(modelHandle);
        break;

      case MODEL_CLEAR_LINK:
        ModelClearLink(modelHandle, *reinterpret_cast<unsigned int *>(param));
        break;

      case MODEL_ENABLE_ANIM_BLENDING:
        ModelEnableAnimBlending(modelHandle, param[0]);
        break;

      case MODEL_ENABLE_FULL_ALPHA:
        ModelEnableFullAlpha(modelHandle, param[0]);
        break;

      case MODEL_FINISH_DUPLICATION: {
        HMODEL  duplicate = *reinterpret_cast<HMODEL *>(param);
        CModel *destination = reinterpret_cast<CModel *>(duplicate);
        HMODEL  sourceHandle = destination->dupSource;
        destination->FinishDuplication(*model);
        HandleClose(duplicate);
        HandleClose(sourceHandle);
        break;
      }

      case MODEL_FORCE_CURRENT_SEQUENCE_TIME:
        ModelForceCurrentSequenceTime(modelHandle, *reinterpret_cast<int *>(param), param[4]);
        break;

      case MODEL_FORCE_SEQUENCE_TIME:
        ModelForceSequenceTime(modelHandle, *reinterpret_cast<unsigned int *>(param), *reinterpret_cast<int *>(param + 4), param[8]);
        break;

      case MODEL_HIDE_BOUNDS:
        ModelHideBounds(modelHandle);
        break;

      case MODEL_HIDE_GEOSETS:
        ModelHideGeosets(modelHandle, *reinterpret_cast<unsigned int *>(param), param[4]);
        break;

      case MODEL_HIDE_GEOSETS_RANGE:
        ModelHideGeosetsRange(modelHandle, *reinterpret_cast<unsigned int *>(param), *reinterpret_cast<unsigned int *>(param + 4), param[8]);
        break;

      case MODEL_LOCK_OBJECT_SEQUENCE:
        ModelLockObjectSequence(modelHandle, *reinterpret_cast<unsigned int *>(param), param[4]);
        break;

      case MODEL_MARK_FOOTSTEP_SEQUENCE:
        ModelMarkFootstepSequence(modelHandle, *reinterpret_cast<unsigned int *>(param));
        break;

      case MODEL_MATCH_SEQUENCE:
        ModelMatchSequence(
            modelHandle, *reinterpret_cast<unsigned int *>(param), *reinterpret_cast<unsigned int *>(param + 4),
            *reinterpret_cast<unsigned int *>(param + 8)
        );
        break;

      case MODEL_OPTIMIZE_VISIBLE_GEOSETS:
        ModelOptimizeVisibleGeosets(modelHandle);
        break;

      case MODEL_REMOVE_LINK: {
        HMODEL child = *reinterpret_cast<HMODEL *>(param + 4);
        ModelRemoveLink(modelHandle, *reinterpret_cast<unsigned int *>(param), child);
        HandleClose(child);
        break;
      }

      case MODEL_REMOVE_OBJECT_FACE_DIR:
        ModelRemoveObjectFaceDir(modelHandle, *reinterpret_cast<unsigned int *>(param));
        break;

      case MODEL_REMOVE_OBJECT_LOOK_AT:
        ModelRemoveObjectLookAt(modelHandle, *reinterpret_cast<unsigned int *>(param));
        break;

      case MODEL_REPLACE_TEXTURE: {
        HTEXTURE texture = *reinterpret_cast<HTEXTURE *>(param + 4);
        ModelReplaceTexture(modelHandle, *reinterpret_cast<unsigned int *>(param), texture, param[8]);
        HandleClose(texture);
        break;
      }

      case MODEL_SET_EMISSIVE_COLOR:
        ModelSetEmissiveColor(modelHandle, *reinterpret_cast<NTempest::CImVector *>(param), param[4]);
        break;

      case MODEL_SET_EVENT_CALLBACK:
        ModelSetEventCallback(
            modelHandle, *reinterpret_cast<void(**)(const char *, const NTempest::C3Vector &, void *)>(param),
            *reinterpret_cast<void **>(param + 4), param[8]
        );
        break;

      case MODEL_SET_LIGHT_SELECT_CALLBACK:
        ModelSetLightSelectCallback(
            modelHandle, *reinterpret_cast<void(**)(void *, NTempest::C3Vector, const NTempest::C3Vector &, unsigned int)>(param),
            *reinterpret_cast<void **>(param + 4), param[8]
        );
        break;

      case MODEL_SET_OBJECT_TIME_SCALE:
        ModelSetObjectTimeScale(modelHandle, *reinterpret_cast<unsigned int *>(param), *reinterpret_cast<float *>(param + 4), param[8]);
        break;

      case MODEL_SET_RANDOM_SEQUENCE_FIDGET1:
        ModelSetRandomSequenceFidget(modelHandle, *reinterpret_cast<unsigned int *>(param), *reinterpret_cast<unsigned int *>(param + 4));
        break;

      case MODEL_SET_RANDOM_SEQUENCE_FIDGET2:
        ModelSetRandomSequenceFidget(
            modelHandle, *reinterpret_cast<unsigned int *>(param), *reinterpret_cast<unsigned int *>(param + 4),
            *reinterpret_cast<unsigned int *>(param + 8)
        );
        break;

      case MODEL_SET_SEQ_FINISHED_HANDLER1:
        ModelSetSeqFinishedHandler(modelHandle, *reinterpret_cast<ANIMSEQFINISHEDHANDLER *>(param), *reinterpret_cast<void **>(param + 4));
        break;

      case MODEL_SET_SEQ_FINISHED_HANDLER2:
        ModelSetSeqFinishedHandler(
            modelHandle, *reinterpret_cast<unsigned int *>(param), *reinterpret_cast<ANIMSEQFINISHEDHANDLER *>(param + 4),
            *reinterpret_cast<void **>(param + 8)
        );
        break;

      case MODEL_SET_SEQUENCE1:
        ModelSetSequence(modelHandle, *reinterpret_cast<unsigned int *>(param), *reinterpret_cast<unsigned int *>(param + 4));
        break;

      case MODEL_SET_SEQUENCE2:
        ModelSetSequence(
            modelHandle, *reinterpret_cast<unsigned int *>(param), *reinterpret_cast<unsigned int *>(param + 4),
            *reinterpret_cast<unsigned int *>(param + 8)
        );
        break;

      case MODEL_SET_SEQUENCE_FIDGET1:
        ModelSetSequenceFidget(
            modelHandle, *reinterpret_cast<unsigned int *>(param), *reinterpret_cast<unsigned int *>(param + 4),
            *reinterpret_cast<unsigned int *>(param + 8)
        );
        break;

      case MODEL_SET_SEQUENCE_FIDGET2:
        ModelSetSequenceFidget(
            modelHandle, *reinterpret_cast<unsigned int *>(param), *reinterpret_cast<unsigned int *>(param + 4),
            *reinterpret_cast<unsigned int *>(param + 8), *reinterpret_cast<unsigned int *>(param + 12)
        );
        break;

      case MODEL_SET_TIME_SCALE:
        ModelSetTimeScale(modelHandle, *reinterpret_cast<float *>(param), param[4]);
        break;

      case MODEL_SET_VERTEX_ALPHA:
        ModelSetVertexAlpha(modelHandle, param[0], param[1]);
        break;

      case MODEL_SET_VERTEX_COLOR:
        ModelSetVertexColor(modelHandle, param[0], param[1], param[2], param[3]);
        break;

      case MODEL_SHOW_BOUNDING_SPHERE:
        ModelShowBoundingSphere(modelHandle);
        break;

      case MODEL_SHOW_COLLISION:
        ModelShowCollision(modelHandle, param[0]);
        break;

      case MODEL_SHOW_COLLISION_AABOX:
        ModelShowCollisionAaBox(modelHandle, param[0]);
        break;

      case MODEL_SHOW_MODEL:
        ModelShowModel(modelHandle, param[0]);
        break;

      default:
        break;
    }

    s_freeModItems.LinkNode(item, LIST_TAIL, 0);
    item = next;
  }

  if (modelHandle) {
    HandleClose(modelHandle);
  }
}

void CModel::RemoveModelCommandsFromQueue() {
  ITERATELIST(CModelModItem, modelModQueue, item) {
    unsigned char *paramData = item->paramData;
    for (unsigned int i = 0; i < 4; ++i) {
      EModelParamType type = s_modelParamTypes[item->action][i];
      if (type == MPARAM_NONE) {
        break;
      }

      switch (type) {
        case MPARAM_HANDLE:
          HandleClose(*reinterpret_cast<HOBJECT *>(paramData));
          // Fall through: handles occupy four bytes in the command buffer.
        case MPARAM_UINT:
        case MPARAM_FLOAT:
        case MPARAM_CARGB:
        case MPARAM_PTR:
          paramData += 4;
          break;
        case MPARAM_C3VECTOR:
          paramData += sizeof(NTempest::C3Vector);
          break;
        case MPARAM_BOOL:
        case MPARAM_BYTE:
          ++paramData;
          break;
        default:
          break;
      }
    }
  }

  s_freeModItems.Combine(&modelModQueue, LIST_TAIL, 0);
}

int ModelIsLoaded(HMODEL modelHandle, int doLinkedModels) {
  CModel        *modelptr;
  CModelBase    *base;
  CModelComplex *complex;
  unsigned int   numLinks;
  unsigned int   i;

  FATALASSERT(modelHandle);

  modelptr = reinterpret_cast<CModel *>(modelHandle);
  if (modelptr->state != CMODEL_LOADED) {
    return 0;
  }

  if (!doLinkedModels) {
    return 1;
  }

  base = modelptr->data;
  if (!(base->m_flags & 0x20)) {
    return 1;
  }

  complex = static_cast<CModelComplex *>(base);
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
int IModelDerefHandle(CModel *model, CModelBase **unique, CModelShared **shared) {
  FATALASSERT(model);

  *unique = 0;
  *shared = reinterpret_cast<CModelShared *>(model->shared);
  FATALASSERT(*shared);

  if (model->state != CMODEL_LOADED) {
    return 0;
  }

  *unique = model->data;
  return 1;
}

int IModelDerefHandle(CModel *model, CModelBase **unique) {
  FATALASSERT(model);

  *unique = 0;
  if (model->state != CMODEL_LOADED) {
    return 0;
  }

  *unique = model->data;
  return 1;
}

int IModelDerefHandle(CModel *model, CModelShared **shared) {
  FATALASSERT(model);

  *shared = reinterpret_cast<CModelShared *>(model->shared);
  FATALASSERT(*shared);

  return model->state == CMODEL_LOADED;
}

