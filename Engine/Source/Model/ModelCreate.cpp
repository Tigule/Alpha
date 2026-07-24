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

void __fastcall           AnimInitialize();
void __fastcall           AnimDestroy();
void __fastcall           MDLFileInitialize();
void __fastcall           MDLFileDestroy();
void __fastcall           ModelAnimateInitialize();
void __fastcall           ModelAnimateDestroy();
void __fastcall           ModelRenderInitialize();
void __fastcall           ModelRenderDestroy();
unsigned char *__fastcall MDLFileBinarySeek(unsigned char *fileData, unsigned int fileBytes, unsigned long sectionTag);
unsigned char *__fastcall MDLFileBinaryLoad(char *path, unsigned int *fileBytes, CStatus *status);
void __fastcall           MDLFileBinaryUnload(unsigned char *fileData);
HTEXTURE __fastcall       LoadModelTexture(const char *texturePath, unsigned int modelLoadFlags, CGxTexFlags texLoadFlags, CStatus *status);
void __fastcall           AnimSetObjectOrdering(HANIM anim, const char **boneNames, unsigned int numBones);
void __fastcall           AnimSetSequenceOrdering(HANIM anim, const char **sequenceNames, unsigned int numSequences);
void __fastcall           AnimSetSequenceOrderingDefault(HANIM anim);
void __fastcall           MdxReadTextures(unsigned char *, unsigned int, unsigned int, CModelComplex *, CStatus *);
void __fastcall           MdxReadTextures(unsigned char *, unsigned int, unsigned int, CModelSimple *, CStatus *);
void __fastcall           MdxReadMaterials(unsigned char *, unsigned int, unsigned int, CModelComplex *, CModelShared *);
void __fastcall           MdxReadMaterials(unsigned char *, unsigned int, unsigned int, CModelSimple *, CModelShared *);
void __fastcall           MdxReadGeosets(unsigned char *, unsigned int, unsigned int, CModelComplex *, CModelShared *);
void __fastcall           MdxReadGeosets(unsigned char *, unsigned int, unsigned int, CModelSimple *, CModelShared *);
void __fastcall           MdxLoadGlobalProperties(unsigned char *, unsigned int, unsigned int *, CModelShared *);
void __fastcall           MdxReadAttachments(unsigned char *, unsigned int, unsigned int, CModelComplex *, CModelShared *, CStatus *);
void __fastcall           MdxReadRibbonEmitters(unsigned char *, unsigned int, CModelComplex *, CModelShared *);
void __fastcall           MdxReadEmitters2(unsigned char *, unsigned int, unsigned int, CModelComplex *, CModelShared *, CStatus *);
void __fastcall           MdxReadLights(unsigned char *, unsigned int, CModelComplex *);
HCOLLISIONDATA __fastcall CollisionDataCreate(unsigned char *, unsigned int);
void __fastcall           ExecuteQueuedActions(CModel *model);
void __fastcall           IModelEnableFullAlpha(CModelBase *unique, int enable);

void __fastcall ModelClearAllLinks(HMODEL parent);
void __fastcall ModelEnableAnimBlending(HMODEL model, int enabled);
void __fastcall ModelHideBounds(HMODEL model);
void __fastcall ModelHideGeosets(HMODEL model, unsigned int selectionGroup, int hide);
void __fastcall ModelHideGeosetsRange(HMODEL model, unsigned int selectionStart, unsigned int selectionEnd, int hide);
int __fastcall  ModelOptimizeVisibleGeosets(HMODEL model);
int __fastcall  ModelRemoveLink(HMODEL parent, unsigned int parentIndex, HMODEL child);
void __fastcall ModelSetEmissiveColor(HMODEL model, const NTempest::CImVector &color, int doLinkedModels);
void __fastcall ModelShowCollision(HMODEL model, int show);
void __fastcall ModelShowCollisionAaBox(HMODEL model, int show);
void __fastcall ModelShowModel(HMODEL model, int show);

class CHashKeyFilePath {
 public:
  CHashKeyFilePath() {
    path[0] = 0;
  }

  bool operator==(const char *value) const {
    return SStrCmpI(path, value, 0x7FFFFFFF) == 0;
  }

  CHashKeyFilePath &operator=(const char *value) {
    SStrCopy(path, value, sizeof(path));
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
  TSLink<CModelHash> link;
};

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

static void __fastcall AsyncModelHandler();

static TSCArray<unsigned char, 4194304>                  s_asyncLoadBuffer;
static TSExplicitList<CAsyncObject, 32>                  s_asyncLoadList;
static unsigned int                                      s_asyncLoadBufferUsed;
static int                                               s_asyncPending;
static TSHashTableReuse<CModelHash, CHashKeyFilePath, 1> s_modelCache;
static TSExplicitList<CModelHash, 292>                   s_modelCacheLRU;
static CNullStatus                                       s_nullStatus;
static TSList<CModelModItem, TSGetLink<CModelModItem> >  s_freeModItems;

HMODEL __fastcall ModelDuplicate(HMODEL sourceModel, unsigned int flags);
HMODEL __fastcall IModelCreateBlocking(const char *fileName, char *actualPath, CModelCreate *data, CStatus *status);
HMODEL __fastcall CreateDefaultModel(const char *fileName, unsigned int modelLoadFlags, CStatus *status);

static int __fastcall ModelIsUsed(HMODEL model) {
  CModelShared *shared;

  IModelDerefHandle(reinterpret_cast<CModel *>(model), &shared);
  return reinterpret_cast<CModel *>(model)->GetRefCount() > 1 || reinterpret_cast<CHandleObject *>(shared)->GetRefCount() > 1;
}

static void __fastcall ProcessAnimReorders(CModelBase *modelptr, CModelCreate *data) {
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

static HMODEL __fastcall GetModel(const char *modelFName, CModelCreate *data) {
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

static void __fastcall HashNewModel(const char *modelFName, HMODEL model, unsigned int createFlags, CStatus *status) {
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
    // TODO: implement
    return 0;
}

static void __fastcall MdxReadNumMatrices(unsigned char *data, unsigned int fileBytes, unsigned int flags, CModelShared *shared) {
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

static unsigned int __fastcall ConvertAnimCreateFlags(unsigned int loadFlags) {
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
    // TODO: implement
    return 0;
}

static int __fastcall MdxReadAnimation(unsigned char *fileData, unsigned int fileBytes, CModelBase *modelptr, unsigned int loadFlags) {
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

static void __fastcall MdxReadHitTestData(unsigned char *data, unsigned int fileBytes, CModelComplex *modelptr, CModelShared *shared) {
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
    // TODO: implement
    return 0;
}

static void __fastcall ComputeBoundingRadius(const CGeosetShared *geosets, unsigned int numGeosets, const NTempest::C3Vector &center, float *radius) {
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

static void __fastcall ComputeBoundingBox(const CGeosetShared *geosets, unsigned int numGeosets, NTempest::CAaBox *extent) {
  *extent = NTempest::CAaBox::Bounding(geosets[0].position.Ptr(), geosets[0].position.Count());

  while (numGeosets) {
    NTempest::CAaBox geosetExtent = NTempest::CAaBox::Bounding(geosets->position.Ptr(), geosets->position.Count());
    extent->b = NTempest::C3Vector::Min(extent->b, geosetExtent.b);
    extent->t = NTempest::C3Vector::Max(extent->t, geosetExtent.t);
    ++geosets;
    --numGeosets;
  }
}

static void __fastcall IModelComputeBounds(CModelShared *shared) {
  ASSERT(shared);
  ComputeBoundingBox(shared->geosets.Ptr(), shared->geosets.Count(), &shared->bounds.extent);
  shared->bounds.sphere.c = (shared->bounds.extent.b + shared->bounds.extent.t) * 0.5f;
  ComputeBoundingRadius(shared->geosets.Ptr(), shared->geosets.Count(), shared->bounds.sphere.c, &shared->bounds.sphere.r);
}

static int MdlReadLoadExtents(const MDLDATA& data, CModelBase* modelptr, CModelShared* shared) {
    // TODO: implement
    return 0;
}

static unsigned char *__fastcall LoadBoundsData(unsigned char *data, CBoundsData *bounds) {
  bounds->sphere.r = *reinterpret_cast<float *>(data);
  data += sizeof(float);

  bounds->extent.b = *reinterpret_cast<NTempest::C3Vector *>(data);
  data += sizeof(NTempest::C3Vector);
  bounds->extent.t = *reinterpret_cast<NTempest::C3Vector *>(data);
  data += sizeof(NTempest::C3Vector);

  bounds->sphere.c = (bounds->extent.b + bounds->extent.t) * 0.5f;
  return data;
}

static void __fastcall MdxReadExtents(unsigned char *data, unsigned int fileBytes, CModelBase *modelptr, CModelShared *shared) {
  ASSERT(modelptr);
  ASSERT(shared);

  unsigned char *globalData = MDLFileBinarySeek(data, fileBytes, 0x4C444F4D);
  ASSERT(globalData);
  LoadBoundsData(globalData + 0x158, &shared->bounds);

  unsigned char *sequenceData = MDLFileBinarySeek(data, fileBytes, 0x53514553);
  if (!sequenceData) {
    return;
  }

  unsigned int numSequences = *reinterpret_cast<unsigned int *>(sequenceData + 4);
  if (!numSequences) {
    return;
  }

  if (modelptr->m_anim && AnimNeedsSequenceBounds(modelptr->m_anim)) {
    shared->seqBounds.SetCount(numSequences);
    unsigned char *record = sequenceData + 8;
    for (unsigned int i = 0; i < numSequences; ++i) {
      LoadBoundsData(record + 0x60, &shared->seqBounds[i]);
      record += 0x8C;
    }
  } else if (fabs(shared->bounds.sphere.r) < 0.00000023841858f) {
    sequenceData = MDLFileBinarySeek(data, fileBytes, 0x53514553);
    if (sequenceData) {
      LoadBoundsData(sequenceData + 0x68, &shared->bounds);
    }
  }
}

static int MdlReadLoadPositions(const MDLDATA& data, unsigned int flags, CModelShared* shared) {
    // TODO: implement
    return 0;
}

static void __fastcall MdxReadPositions(unsigned char *fileData, unsigned int fileBytes, unsigned int flags, CModelShared *shared) {
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

static CModelShared *__fastcall CreateSharedModelData(const char *fileName) {
  void         *storage = SMemAlloc(sizeof(CModelShared), "HMODELSHARED", SERR_LINECODE_OBJECT, 0);
  CModelShared *shared = storage ? new (storage) CModelShared : 0;

  ASSERT(shared);
  SStrCopy(shared->name, fileName, sizeof(shared->name));
  return shared;
}

int __fastcall IsSimpleModel(const MDLDATA &source) {
  return source.geosets.Count() <= 5 && source.materials.Count() <= 4 && source.textures.Count() <= 4 && !source.lights.Count() &&
         !source.attachments.Count() && !source.particleEmitters2.Count() && !source.ribbonEmitters.Count() && !source.cameras.Count() &&
         !source.hitTestShapes.Count();
}

static unsigned int __fastcall GetSectionCount(unsigned char *fileData, unsigned int fileBytes, unsigned long sectionTag) {
  unsigned char *section = MDLFileBinarySeek(fileData, fileBytes, sectionTag);
  return section ? *reinterpret_cast<unsigned int *>(section + 4) : 0;
}

static unsigned int __fastcall GetTextureCount(unsigned char *fileData, unsigned int fileBytes) {
  unsigned char *section = MDLFileBinarySeek(fileData, fileBytes, 0x53584554);
  return section ? *reinterpret_cast<unsigned int *>(section) / 0x10C : 0;
}

static int __fastcall IsSimpleModel(unsigned char *fileData, unsigned int fileBytes) {
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

static void __fastcall BuildSimpleModelFromMdxData(
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

static void __fastcall BuildModelFromMdxData(
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
    // TODO: implement
    return 0;
}

static int BuildModelFromMdlData(const MDLDATA& source, CModelBase* baseModel, CModelShared* shared, unsigned int flags, CStatus* status) {
    // TODO: implement
    return 0;
}

static HMATERIAL __fastcall BuildSimpleMaterial(
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

static void __fastcall BuildSimpleGeoset(
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

HMODEL __fastcall CreateDefaultModel(const char *fileName, unsigned int modelLoadFlags, CStatus *status) {
  status->Add(STATUS_WARNING, "Warning, model %s failed to load\n", fileName);

  CGxTexFlags textureFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1);
  HTEXTURE    texture = LoadModelTexture("Textures\\ShaneCube", modelLoadFlags, textureFlags, status);
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

void __fastcall ModelInitialize() {
  AnimInitialize();
  MDLFileInitialize();
  ModelAnimateInitialize();
  ModelRenderInitialize();
  AsyncFileReadAddHandler(AsyncModelHandler);
}

void __fastcall ModelDestroy() {
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

void __fastcall ModelCacheFlush() {
  s_modelCacheLRU.UnlinkAll();
  s_modelCache.Destroy();
}

void __fastcall ModelRemoveFromCache(const char *sourcefile) {
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

int __fastcall ModelCacheUpdate(unsigned long currentTime, CStatus *status) {
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

HMODEL __fastcall ModelGetModel(const char *sourcefile, CModelCreate *data) {
  FATALASSERT(sourcefile);
  return GetModel(sourcefile, data);
}

HMODEL __fastcall IModelCreateBlocking(const char *fileName, char *actualPath, CModelCreate *data, CStatus *status) {
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

static void __fastcall AsyncModelHandler() {
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

static void __fastcall AsnycModelPostLoadCallback(void *userArg) {
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

static HMODEL __fastcall IModelCreate(const char *fileName, char *actualPath, CModelCreate *createData, CStatus *status) {
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

static int __fastcall IsBinaryFile(char *path) {
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

HMODEL __fastcall ModelCreate(const char *sourcefile, CModelCreate *data, CStatus *status) {
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

  // todo: full model parser
  return 0;
}

static CModel *__fastcall IModelCreateSimpleEmpty(const char *name) {
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

HMODEL __fastcall ModelCreateSimpleMesh(
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

int __fastcall ModelGeosetAdd(
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

HMODEL __fastcall ModelDuplicate(HMODEL sourceModel, unsigned int flags) {
  if (!sourceModel) {
    return 0;
  }

  CModel *source = reinterpret_cast<CModel *>(sourceModel);
  if (source->state == CMODEL_LOADED && (flags & 1)) {
    source->data->m_flags |= 4;
  }

  void   *storage = SMemAlloc(sizeof(CModel), "HMODEL", -2, 0);
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

void __fastcall ExecuteQueuedActions(CModel *model) {
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
            modelHandle, *reinterpret_cast<void(__fastcall **)(const char *, const NTempest::C3Vector &, void *)>(param),
            *reinterpret_cast<void **>(param + 4), param[8]
        );
        break;

      case MODEL_SET_LIGHT_SELECT_CALLBACK:
        ModelSetLightSelectCallback(
            modelHandle, *reinterpret_cast<void(__fastcall **)(void *, NTempest::C3Vector, const NTempest::C3Vector &, unsigned int)>(param),
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
  for (CModelModItem *item = modelModQueue.Head(); item; item = modelModQueue.Next(item)) {
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

int __fastcall ModelIsLoaded(HMODEL modelHandle, int doLinkedModels) {
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
    LINKUNIQUE *link = complex->m_attached[i].Head();
    while (link) {
      if (!ModelIsLoaded(link->child, 1)) {
        return 0;
      }
      link = complex->m_attached[i].Next(link);
    }
  }

  return 1;
}
int __fastcall IModelDerefHandle(CModel *model, CModelBase **unique, CModelShared **shared) {
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

int __fastcall IModelDerefHandle(CModel *model, CModelBase **unique) {
  FATALASSERT(model);

  *unique = 0;
  if (model->state != CMODEL_LOADED) {
    return 0;
  }

  *unique = model->data;
  return 1;
}

int __fastcall IModelDerefHandle(CModel *model, CModelShared **shared) {
  FATALASSERT(model);

  *shared = reinterpret_cast<CModelShared *>(model->shared);
  FATALASSERT(*shared);

  return model->state == CMODEL_LOADED;
}

CGeosetShared::~CGeosetShared() {
}
