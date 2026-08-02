#include "Anim/AnimInternal.h"
#include "MDLFile/MDLTypes.h"
#include "Base/Status.h"

#include <stddef.h>
#include <malloc.h>
#include <stpl.h>

unsigned char *MDLFileBinarySeek(unsigned char *fileData, unsigned int fileBytes, unsigned long sectionTag);
int MDLFileRead(const char *path, MDLDATA *data, CStatus *status);

CAnim *AnimCreate(unsigned int *const objectCounts, unsigned int numGeosets, unsigned int numCameras, unsigned int numMaterialLayers);
HANIM AnimCreate(const char *sourcefile, unsigned int flags, CStatus *status);
int AnimBuild(unsigned char *fileData, unsigned int fileBytes, CAnim *unique, unsigned int flags);

void AnimAddSequences(unsigned char *fileData, unsigned int fileBytes, CAnim *unique, CAnimData *shared);
void AnimAddCameras(unsigned char *fileData, unsigned int fileBytes, CAnimData *shared, MDLTRACKTYPE forceType);
void AnimAddGeosets(unsigned char *fileData, unsigned int fileBytes, CAnimData *shared, MDLTRACKTYPE forceType);
void AnimAddTextureAnims(unsigned char *fileData, unsigned int fileBytes, CAnim *unique, CAnimData *shared, MDLTRACKTYPE forceType);
void AnimAddMaterialLayers(unsigned char *fileData, unsigned int fileBytes, CAnim *unique, CAnimData *shared, MDLTRACKTYPE forceType);
unsigned int GetGenObjectCount(unsigned char *fileData, unsigned int fileBytes);
unsigned char *CreateBone(unsigned char *, CAnimData *, const unsigned int *, unsigned int *, MDLTRACKTYPE);
unsigned char *CreateHitTestShape(unsigned char *, CAnimData *, const unsigned int *, unsigned int *, MDLTRACKTYPE);
unsigned char *CreateLight(unsigned char *, CAnimData *, const unsigned int *, unsigned int *, MDLTRACKTYPE);
unsigned char *CreateHelper(unsigned char *, CAnimData *, const unsigned int *, unsigned int *, MDLTRACKTYPE);
unsigned char *CreateAttachmentPoint(unsigned char *, CAnimData *, const unsigned int *, unsigned int *, MDLTRACKTYPE);
unsigned char *CreateParticleEmitter2(unsigned char *, CAnimData *, const unsigned int *, unsigned int *, MDLTRACKTYPE);
unsigned char *CreateRibbonEmitter(unsigned char *, CAnimData *, const unsigned int *, unsigned int *, MDLTRACKTYPE);
unsigned char *CreateEventObject(unsigned char *, CAnimData *, const unsigned int *, unsigned int *, MDLTRACKTYPE);

void BuildHierarchy(CAnimData *shared, const unsigned int *parentIds, const unsigned int *idConversion, unsigned int numObjects);
void AnimInit(CAnim *unique, CAnimData *shared);
void AnimAddSequences(
    CAnim *unique,
    CAnimData *shared,
    const TSGrowableArray<MDLSEQUENCESSECTION> &sequences,
    const TSGrowableArray<MDLGLOBALSEQSECTION> &globalSeqs
);
void AnimAddTextureAnim(
    CAnim *unique,
    CAnimData *shared,
    const TSGrowableArray<MDLTEXANIMSECTION> &textureAnims,
    MDLTRACKTYPE forceType
);
void AnimAddMaterialLayer(CAnimData *, const MDLTEXLAYER &, unsigned int, MDLTRACKTYPE);
void AnimAddGeoset(CAnimData *, const MDLGEOSETANIMSECTION &, MDLTRACKTYPE);
void AnimAddCamera(CAnimData *, const MDLCAMERASECTION &, MDLTRACKTYPE);
void AnimObjectSetTranslation(CAnimData *, CAnimObj *, const MDLKEYTRACK<NTempest::C3Vector> &, MDLTRACKTYPE);
void AnimObjectSetRotation(CAnimData *, CAnimObj *, const MDLKEYTRACK<NTempest::C4Quaternion> &, MDLTRACKTYPE);
void AnimObjectSetScaling(CAnimData *, CAnimObj *, const MDLKEYTRACK<NTempest::C3Vector> &, MDLTRACKTYPE);
void AnimObjectSetAttenuation(CAnimData *, CAnimLightObj *, const MDLKEYTRACK<float> &, const MDLKEYTRACK<float> &, MDLTRACKTYPE);
void AnimObjectSetColor(CAnimData *, CAnimLightObj *, const MDLKEYTRACK<C3Color> &, MDLTRACKTYPE);
void AnimObjectSetIntensity(CAnimData *, CAnimLightObj *, const MDLKEYTRACK<float> &, MDLTRACKTYPE);
void AnimObjectSetAmbColor(CAnimData *, CAnimLightObj *, const MDLKEYTRACK<C3Color> &, MDLTRACKTYPE);
void AnimObjectSetAmbIntensity(CAnimData *, CAnimLightObj *, const MDLKEYTRACK<float> &, MDLTRACKTYPE);
void AnimObjectSetVisibilityTrack(CAnimData *, CAnimVisibleObj *, const MDLKEYTRACK<float> &, MDLTRACKTYPE);
void AnimObjectSetParticleEmissionRate2(CAnimData *, CAnimEmitter2Obj *, const MDLKEYTRACK<float> &, MDLTRACKTYPE);
void AnimObjectSetParticleGravity2(CAnimData *, CAnimEmitter2Obj *, const MDLKEYTRACK<float> &, MDLTRACKTYPE);
void AnimObjectSetParticleVariation2(CAnimData *, CAnimEmitter2Obj *, const MDLKEYTRACK<float> &, MDLTRACKTYPE);
void AnimObjectSetEmitterLongitude2(CAnimData *, CAnimEmitter2Obj *, const MDLKEYTRACK<float> &, MDLTRACKTYPE);
void AnimObjectSetEmitterLatitude2(CAnimData *, CAnimEmitter2Obj *, const MDLKEYTRACK<float> &, MDLTRACKTYPE);
void AnimObjectSetParticleSpeed2(CAnimData *, CAnimEmitter2Obj *, const MDLKEYTRACK<float> &, MDLTRACKTYPE);
void AnimObjectSetParticleLength2(CAnimData *, CAnimEmitter2Obj *, const MDLKEYTRACK<float> &, MDLTRACKTYPE);
void AnimObjectSetParticleWidth2(CAnimData *, CAnimEmitter2Obj *, const MDLKEYTRACK<float> &, MDLTRACKTYPE);
void AnimObjectSetParticleZsource2(CAnimData *, CAnimEmitter2Obj *, const MDLKEYTRACK<float> &, MDLTRACKTYPE);
void AnimObjectSetParticleLifeSpan2(CAnimData *, CAnimEmitter2Obj *, const MDLKEYTRACK<float> &, MDLTRACKTYPE);
void AnimObjectSetRibbonHeightAbove(CAnimData *, CAnimRibbonObj *, const MDLKEYTRACK<float> &, MDLTRACKTYPE);
void AnimObjectSetRibbonHeightBelow(CAnimData *, CAnimRibbonObj *, const MDLKEYTRACK<float> &, MDLTRACKTYPE);
void AnimObjectSetRibbonColor(CAnimData *, CAnimRibbonObj *, const MDLKEYTRACK<C3Color> &, MDLTRACKTYPE);
void AnimObjectSetRibbonAlpha(CAnimData *, CAnimRibbonObj *, const MDLKEYTRACK<float> &, MDLTRACKTYPE);
void AnimObjectSetRibbonSlot(CAnimData *, CAnimRibbonObj *, const MDLSIMPLEKEYTRACK<MDLINTKEY> &);
void AnimObjectSetEventTrack(CAnimData *, CAnimEventObj *, const MDLSIMPLEKEYTRACK<MDLEVENTKEY> &);

struct ANIMHASH : public TSHashObject<ANIMHASH, HASHKEY_STRI> {
  ANIMHASH() : anim(0) {
  }
  ANIMHASH(const ANIMHASH &);
  ~ANIMHASH() {
  }

  HANIM anim;
};

static TSHashTable<ANIMHASH, HASHKEY_STRI> s_animCache;

static int AnimGetReferenceCount(HANIM__* anim) {
  CAnim *container = reinterpret_cast<CAnim *>(anim);
  FATALASSERT(container);
  return container->GetRefCount();
}

static HANIM__* GetAnim(const char* modelFName) {
  FATALASSERT(modelFName);
  ANIMHASH *entry = s_animCache.Ptr(modelFName);
  if (!entry) {
    return 0;
  }
  if (AnimGetReferenceCount(entry->anim) > 1) {
    return AnimDuplicate(entry->anim, 0);
  }
  return reinterpret_cast<HANIM__ *>(HandleDuplicate(entry->anim));
}

static void HashNewAnim(const char* modelFName, HANIM__* anim) {
  FATALASSERT(modelFName);
  ANIMHASH *entry = s_animCache.New(modelFName, 0, 0);
  entry->anim = reinterpret_cast<HANIM>(HandleDuplicate(anim));
}

static unsigned char GetObjectFlags(unsigned int mdlFlags) {
  unsigned char flags = 0;
  if (mdlFlags & 8) {
    flags = 0x38;
  } else if (mdlFlags & 0x10) {
    flags = 8;
  } else if (mdlFlags & 0x20) {
    flags = 0x10;
  } else if (mdlFlags & 0x40) {
    flags = 0x20;
  }
  if (mdlFlags & 1)
    flags |= 1;
  if (mdlFlags & 2)
    flags |= 2;
  if (mdlFlags & 4)
    flags |= 4;
  if (mdlFlags & 0x4000)
    flags |= 0x40;
  return flags;
}

static unsigned char *GenericHandlerAnim(
    unsigned char      *fileData,
    CAnimData          *shared,
    CAnimObj           *currobj,
    const unsigned int *idConversion,
    unsigned int       *parentIds,
    MDLTRACKTYPE        forceType
) {
  FATALASSERT(shared);
  FATALASSERT(currobj);
  unsigned int   genObjBytes = *reinterpret_cast<unsigned int *>(fileData);
  unsigned char *dataDone = fileData + genObjBytes;
  SStrCopy(currobj->name, reinterpret_cast<const char *>(fileData + 4), sizeof(currobj->name));
  unsigned int oldObjectId = *reinterpret_cast<unsigned int *>(fileData + 84);
  parentIds[oldObjectId] = *reinterpret_cast<unsigned int *>(fileData + 88);
  unsigned int objectId = idConversion[oldObjectId];
  AnimObjectSetIndex(shared, currobj, objectId);
  currobj->flags = GetObjectFlags(*reinterpret_cast<unsigned int *>(fileData + 92));
  fileData += 96;
  fileData = AddKeyFramesType(fileData, dataDone - fileData, 0x5254474B, shared, &currobj->translation, forceType);
  fileData = AnimObjectSetRotation(fileData, dataDone - fileData, shared, currobj, forceType);
  fileData = AddKeyFramesType(fileData, dataDone - fileData, 0x4353474B, shared, &currobj->scale, forceType);
  FATALASSERT(fileData == dataDone);
  return fileData;
}

unsigned char *
CreateBone(unsigned char *fileData, CAnimData *shared, const unsigned int *idConversion, unsigned int *parentIds, MDLTRACKTYPE forceType) {
  FATALASSERT(shared);
  CAnimBoneObj *currobj = AnimObjectCreateBone(shared);
  FATALASSERT(currobj);
  fileData = GenericHandlerAnim(fileData, shared, currobj, idConversion, parentIds, forceType);
  unsigned int geoset = *reinterpret_cast<unsigned int *>(fileData);
  currobj->geosetId = geoset == static_cast<unsigned int>(-1) ? 0xFF : *reinterpret_cast<unsigned int *>(fileData + 4);
  return fileData + 8;
}

unsigned char *
CreateHitTestShape(unsigned char *fileData, CAnimData *shared, const unsigned int *idConversion, unsigned int *parentIds, MDLTRACKTYPE forceType) {
  FATALASSERT(shared);
  CAnimBoneObj *currobj = AnimObjectCreateBone(shared);
  FATALASSERT(currobj);
  unsigned int sectionLength = *reinterpret_cast<unsigned int *>(fileData);
  GenericHandlerAnim(fileData + 4, shared, currobj, idConversion, parentIds, forceType);
  currobj->geosetId = 0xFF;
  return fileData + sectionLength;
}

unsigned char *
CreateLight(unsigned char *fileData, CAnimData *shared, const unsigned int *idConversion, unsigned int *parentIds, MDLTRACKTYPE forceType) {
  FATALASSERT(shared);
  CAnimLightObj *currobj = AnimObjectCreateLight(shared);
  FATALASSERT(currobj);
  unsigned int   sectionLength = *reinterpret_cast<unsigned int *>(fileData);
  unsigned char *data = GenericHandlerAnim(fileData + 4, shared, currobj, idConversion, parentIds, forceType);
  data += 44;
  data = AnimObjectSetAttenuation(data, fileData + sectionLength - data, shared, currobj, forceType);
  data = AnimObjectSetColor(data, fileData + sectionLength - data, shared, currobj, forceType);
  data = AnimObjectSetIntensity(data, fileData + sectionLength - data, shared, currobj, forceType);
  data = AnimObjectSetAmbColor(data, fileData + sectionLength - data, shared, currobj, forceType);
  data = AnimObjectSetAmbIntensity(data, fileData + sectionLength - data, shared, currobj, forceType);
  data = AnimObjectSetVisibilityTrack(data, fileData + sectionLength - data, shared, currobj, forceType);
  ASSERT(data == (fileData + sectionLength));
  return data;
}

unsigned char *CreateParticleEmitter2(
    unsigned char      *fileData,
    CAnimData          *shared,
    const unsigned int *idConversion,
    unsigned int       *parentIds,
    MDLTRACKTYPE        forceType
) {
  FATALASSERT(shared);
  CAnimEmitter2Obj *currobj = AnimObjectCreateEmitter2(shared);
  FATALASSERT(currobj);
  unsigned int   sectionLength = *reinterpret_cast<unsigned int *>(fileData);
  unsigned char *data = GenericHandlerAnim(fileData + 4, shared, currobj, idConversion, parentIds, forceType);

  data += *reinterpret_cast<unsigned int *>(data);
  currobj->squirts = *reinterpret_cast<unsigned int *>(data);
  data += sizeof(unsigned int);
  data = AddKeyFramesType(data, fileData + sectionLength - data, 0x4532504B, shared, &currobj->emissionRate, forceType);
  data = AddKeyFramesType(data, fileData + sectionLength - data, 0x4732504B, shared, &currobj->gravity, forceType);
  data = AddKeyFramesType(data, fileData + sectionLength - data, 0x4E4C504B, shared, &currobj->longitude, forceType);
  data = AddKeyFramesType(data, fileData + sectionLength - data, 0x4C32504B, shared, &currobj->latitude, forceType);
  data = AddKeyFramesType(data, fileData + sectionLength - data, 0x5332504B, shared, &currobj->particleSpeed, forceType);
  data = AddKeyFramesType(data, fileData + sectionLength - data, 0x5232504B, shared, &currobj->variation, forceType);
  data = AddKeyFramesType(data, fileData + sectionLength - data, 0x4E32504B, shared, &currobj->length, forceType);
  data = AddKeyFramesType(data, fileData + sectionLength - data, 0x5732504B, shared, &currobj->width, forceType);
  data = AddKeyFramesType(data, fileData + sectionLength - data, 0x5A32504B, shared, &currobj->zsource, forceType);
  data = AddKeyFramesType(data, fileData + sectionLength - data, 0x5349564B, shared, &currobj->visibility, forceType);
  data = AddKeyFramesType(data, fileData + sectionLength - data, 0x46494C4B, shared, &currobj->lifeSpan, forceType);
  ASSERT(data == (fileData + sectionLength));
  return data;
}

unsigned char *
CreateRibbonEmitter(unsigned char *fileData, CAnimData *shared, const unsigned int *idConversion, unsigned int *parentIds, MDLTRACKTYPE forceType) {
  FATALASSERT(shared);
  CAnimRibbonObj *currobj = AnimObjectCreateRibbon(shared);
  FATALASSERT(currobj);
  unsigned int   sectionLength = *reinterpret_cast<unsigned int *>(fileData);
  unsigned char *data = GenericHandlerAnim(fileData + 4, shared, currobj, idConversion, parentIds, forceType);
  data += *reinterpret_cast<unsigned int *>(data);
  data = AnimObjectSetRibbonHeightAbove(data, fileData + sectionLength - data, shared, currobj, forceType);
  data = AnimObjectSetRibbonHeightBelow(data, fileData + sectionLength - data, shared, currobj, forceType);
  data = AnimObjectSetRibbonAlpha(data, fileData + sectionLength - data, shared, currobj, forceType);
  data = AnimObjectSetRibbonColor(data, fileData + sectionLength - data, shared, currobj, forceType);
  data = AnimObjectSetRibbonSlot(data, fileData + sectionLength - data, shared, currobj);
  data = AnimObjectSetVisibilityTrack(data, fileData + sectionLength - data, shared, currobj, forceType);
  ASSERT(data == (fileData + sectionLength));
  return data;
}

unsigned char *
CreateEventObject(unsigned char *fileData, CAnimData *shared, const unsigned int *idConversion, unsigned int *parentIds, MDLTRACKTYPE forceType) {
  FATALASSERT(shared);
  CAnimEventObj *currobj = AnimObjectCreateEvent(shared);
  FATALASSERT(currobj);
  unsigned int   sectionLength = *reinterpret_cast<unsigned int *>(fileData);
  unsigned char *data = GenericHandlerAnim(fileData + 4, shared, currobj, idConversion, parentIds, forceType);
  data = AnimObjectSetEventTrack(data, fileData + sectionLength - data, shared, currobj);
  ASSERT(data == (fileData + sectionLength));
  return data;
}

unsigned char *
CreateHelper(unsigned char *fileData, CAnimData *shared, const unsigned int *idConversion, unsigned int *parentIds, MDLTRACKTYPE forceType) {
  FATALASSERT(shared);
  CAnimObj *currobj = AnimObjectCreateHelper(shared);
  FATALASSERT(currobj);
  return GenericHandlerAnim(fileData, shared, currobj, idConversion, parentIds, forceType);
}

unsigned char *
CreateAttachmentPoint(unsigned char *fileData, CAnimData *shared, const unsigned int *idConversion, unsigned int *parentIds, MDLTRACKTYPE forceType) {
  FATALASSERT(shared);
  CAnimModelObj *currobj = AnimObjectCreateAttachment(shared);
  FATALASSERT(currobj);
  unsigned int   sectionLength = *reinterpret_cast<unsigned int *>(fileData);
  unsigned char *data = GenericHandlerAnim(fileData + 4, shared, currobj, idConversion, parentIds, forceType);
  currobj->geosetId = *(data + 4);
  data += 0x109;
  data = AddKeyFramesType(data, fileData + sectionLength - data, 0x5349564B, shared, &currobj->visibility, forceType);
  ASSERT(data == (fileData + sectionLength));
  return data;
}

static void SetObjectParent(CAnimData *shared, unsigned int object, unsigned int parent) {
  FATALASSERT(shared);
  CAnimObj *animobj = GetNodeByIndex(shared, object);
  FATALASSERT(animobj);
  int setObjParent = AnimObjectSetParent(shared, animobj, parent);
  FATALASSERT(setObjParent);
}

void BuildHierarchy(CAnimData *shared, const unsigned int *parentIds, const unsigned int *idConversion, unsigned int numObjects) {
  for (unsigned int oldObjectId = 0; oldObjectId < numObjects; ++oldObjectId) {
    unsigned int objectId = idConversion[oldObjectId];
    if (objectId == static_cast<unsigned int>(-1)) {
      continue;
    }

    unsigned int parentId = parentIds[oldObjectId];
    if (parentId != static_cast<unsigned int>(-1)) {
      parentId = idConversion[parentId];
      FATALASSERT(parentId != static_cast<unsigned int>(-1));
    }

    SetObjectParent(shared, objectId, parentId);
  }
}

unsigned int GetGenObjectCount(unsigned char *fileData, unsigned int fileBytes) {
  unsigned char *section = MDLFileBinarySeek(fileData, fileBytes, 0x4C444F4D);
  ASSERT(section);
  return *reinterpret_cast<unsigned int *>(section + 373);
}

unsigned int AnimBuildObjectIdTranslation(
    const MDLDATA &data,
    unsigned int flags,
    TSStackArray<unsigned int> *idConversion
) {
  ASSERT(idConversion);

  unsigned int numObjects = data.objects.Count();
  unsigned int index;
  for (index = 0; index < numObjects; ++index) {
    (*idConversion)[index] = index;
  }

  unsigned int numRemoved = 0;
  if (!(flags & 1) && data.hitTestShapes.Count()) {
    unsigned int count = data.hitTestShapes.Count();
    unsigned int firstObject = data.hitTestShapes[0].objectId;
    for (index = firstObject; index < firstObject + count; ++index) {
      (*idConversion)[index] = static_cast<unsigned int>(-1);
    }
    for (index = firstObject + count; index < numObjects; ++index) {
      (*idConversion)[index] = index - count;
    }
    numRemoved = count;
  }

  if ((flags & 2) && data.lights.Count()) {
    unsigned int count = data.lights.Count();
    unsigned int firstObject = data.lights[0].objectId;
    for (index = firstObject; index < firstObject + count; ++index) {
      (*idConversion)[index] = static_cast<unsigned int>(-1);
    }
    for (index = firstObject + count; index < numObjects; ++index) {
      if ((*idConversion)[index] != static_cast<unsigned int>(-1)) {
        (*idConversion)[index] = index - count;
      }
    }
    numRemoved += count;
  }

  return numRemoved;
}

unsigned int AnimBuildObjectIdTranslation(
    unsigned char *fileData,
    unsigned int   fileBytes,
    unsigned int   flags,
    unsigned int  *idConversion,
    unsigned int   numObjects
) {
  ASSERT(idConversion);

  unsigned int index;
  for (index = 0; index < numObjects; ++index) {
    idConversion[index] = index;
  }

  unsigned int numRemoved = 0;
  if (!(flags & 1)) {
    unsigned char *section = MDLFileBinarySeek(fileData, fileBytes, 0x54535448);
    if (section) {
      unsigned int count = *reinterpret_cast<unsigned int *>(section + 4);
      unsigned int firstObject = *reinterpret_cast<unsigned int *>(section + 96);
      if (firstObject < firstObject + count) {
        memset(idConversion + firstObject, 0xFF, count * sizeof(unsigned int));
      }
      for (index = firstObject + count; index < numObjects; ++index) {
        idConversion[index] = index - count;
      }
      numRemoved = count;
    }
  }

  if (!(flags & 2)) {
    return numRemoved;
  }

  unsigned char *section = MDLFileBinarySeek(fileData, fileBytes, 0x4554494C);
  if (!section) {
    return numRemoved;
  }

  unsigned int count = *reinterpret_cast<unsigned int *>(section + 4);
  unsigned int firstObject = *reinterpret_cast<unsigned int *>(section + 96);
  if (firstObject < firstObject + count) {
    memset(idConversion + firstObject, 0xFF, count * sizeof(unsigned int));
  }
  for (index = firstObject + count; index < numObjects; ++index) {
    if (idConversion[index] != static_cast<unsigned int>(-1)) {
      idConversion[index] = index - count;
    }
  }
  return numRemoved + count;
}

static void MaterialHandlerAnim(
    CAnimData *shared,
    const TSGrowableArray<MDLMATERIALSECTION> &materials,
    MDLTRACKTYPE forceType
) {
  unsigned int layerId = 0;
  for (unsigned int material = 0; material < materials.Count(); ++material) {
    for (unsigned int layer = 0; layer < materials[material].texLayers.Count(); ++layer, ++layerId) {
      AnimAddMaterialLayer(shared, materials[material].texLayers[layer], layerId, forceType);
    }
  }
}

static void GeosetHandlerAnim(
    CAnimData *shared,
    const TSGrowableArray<MDLGEOSETANIMSECTION> &geosets,
    MDLTRACKTYPE forceType
) {
  shared->geo.ReserveSpace(geosets.Count());
  shared->geo.Clear();
  for (unsigned int i = 0; i < geosets.Count(); ++i) {
    AnimAddGeoset(shared, geosets[i], forceType);
  }
}

static void CameraHandlerAnim(
    CAnimData *shared,
    const TSGrowableArray<MDLCAMERASECTION> &cameras,
    MDLTRACKTYPE forceType
) {
  shared->cameraObjs.ReserveSpace(cameras.Count());
  shared->cameraObjs.Clear();
  for (unsigned int i = 0; i < cameras.Count(); ++i) {
    AnimAddCamera(shared, cameras[i], forceType);
  }
}

static void GenericHandlerAnim(
    CAnimData *shared,
    CAnimObj *currobj,
    const MDLGENOBJECT &obj,
    const TSStackArray<unsigned int> &idConversion,
    MDLTRACKTYPE forceType
) {
  SStrCopy(currobj->name, obj.name, sizeof(currobj->name));
  AnimObjectSetIndex(shared, currobj, idConversion[obj.objectId]);
  currobj->flags = GetObjectFlags(obj.flags);

  if (shared->seq.Count()) {
    AnimObjectSetTranslation(shared, currobj, obj.transkeys, forceType);
    AnimObjectSetRotation(shared, currobj, obj.rotkeys, forceType);
    AnimObjectSetScaling(shared, currobj, obj.scalekeys, forceType);
  }
}

static void CreateBone(
    CAnimData *shared,
    const MDLBONESECTION &bonedata,
    const TSStackArray<unsigned int> &idConversion,
    MDLTRACKTYPE forceType
) {
  CAnimBoneObj *currobj = AnimObjectCreateBone(shared);
  ASSERT(currobj);
  GenericHandlerAnim(shared, currobj, bonedata, idConversion, forceType);
  currobj->geosetId = bonedata.geosetId == static_cast<unsigned int>(-1) ? 0xFF : bonedata.geosetAnimId;
}

static void CreateHitTestShape(
    CAnimData *shared,
    const MDLHITTESTSHAPE &hitTest,
    const TSStackArray<unsigned int> &idConversion,
    MDLTRACKTYPE forceType
) {
  CAnimBoneObj *currobj = AnimObjectCreateBone(shared);
  ASSERT(currobj);
  GenericHandlerAnim(shared, currobj, hitTest, idConversion, forceType);
  currobj->geosetId = 0xFF;
}

static void CreateLight(
    CAnimData *shared,
    const MDLLIGHTSECTION &lightdata,
    const TSStackArray<unsigned int> &idConversion,
    MDLTRACKTYPE forceType
) {
  CAnimLightObj *currobj = AnimObjectCreateLight(shared);
  ASSERT(currobj);
  GenericHandlerAnim(shared, currobj, lightdata, idConversion, forceType);
  AnimObjectSetAttenuation(shared, currobj, lightdata.attenstartkeys, lightdata.attenendkeys, forceType);
  AnimObjectSetColor(shared, currobj, lightdata.colorkeys, forceType);
  AnimObjectSetIntensity(shared, currobj, lightdata.intensitykeys, forceType);
  AnimObjectSetAmbColor(shared, currobj, lightdata.ambcolorkeys, forceType);
  AnimObjectSetAmbIntensity(shared, currobj, lightdata.ambintensitykeys, forceType);
  AnimObjectSetVisibilityTrack(shared, currobj, lightdata.visibilityKeys, forceType);
}

static void CreateParticleEmitter2(
    CAnimData *shared,
    const MDLPARTICLEEMITTER2 &emitterdata,
    const TSStackArray<unsigned int> &idConversion,
    MDLTRACKTYPE forceType
) {
  CAnimEmitter2Obj *currobj = AnimObjectCreateEmitter2(shared);
  ASSERT(currobj);
  GenericHandlerAnim(shared, currobj, emitterdata, idConversion, forceType);
  currobj->squirts = emitterdata.squirts;
  AnimObjectSetParticleEmissionRate2(shared, currobj, emitterdata.emissionRate, forceType);
  AnimObjectSetParticleGravity2(shared, currobj, emitterdata.gravity, forceType);
  AnimObjectSetEmitterLongitude2(shared, currobj, emitterdata.longitude, forceType);
  AnimObjectSetEmitterLatitude2(shared, currobj, emitterdata.latitude, forceType);
  AnimObjectSetParticleSpeed2(shared, currobj, emitterdata.speed, forceType);
  AnimObjectSetParticleVariation2(shared, currobj, emitterdata.variation, forceType);
  AnimObjectSetParticleLength2(shared, currobj, emitterdata.length, forceType);
  AnimObjectSetParticleWidth2(shared, currobj, emitterdata.width, forceType);
  AnimObjectSetParticleZsource2(shared, currobj, emitterdata.zsource, forceType);
  AnimObjectSetVisibilityTrack(shared, currobj, emitterdata.visibilityKeys, forceType);
  AnimObjectSetParticleLifeSpan2(shared, currobj, emitterdata.life, forceType);
}

static void CreateRibbonEmitter(
    CAnimData *shared,
    const MDLRIBBONEMITTER &ribbondata,
    const TSStackArray<unsigned int> &idConversion,
    MDLTRACKTYPE forceType
) {
  CAnimRibbonObj *currobj = AnimObjectCreateRibbon(shared);
  ASSERT(currobj);
  GenericHandlerAnim(shared, currobj, ribbondata, idConversion, forceType);
  AnimObjectSetRibbonHeightAbove(shared, currobj, ribbondata.heightAbove, forceType);
  AnimObjectSetRibbonHeightBelow(shared, currobj, ribbondata.heightBelow, forceType);
  AnimObjectSetRibbonAlpha(shared, currobj, ribbondata.alphaKeys, forceType);
  AnimObjectSetRibbonColor(shared, currobj, ribbondata.colorKeys, forceType);
  AnimObjectSetRibbonSlot(shared, currobj, ribbondata.textureSlot);
  AnimObjectSetVisibilityTrack(shared, currobj, ribbondata.visibilityKeys, forceType);
}

static void CreateEventObject(
    CAnimData *shared,
    const MDLEVENTSECTION &eventData,
    const TSStackArray<unsigned int> &idConversion,
    MDLTRACKTYPE forceType
) {
  CAnimEventObj *currobj = AnimObjectCreateEvent(shared);
  ASSERT(currobj);
  GenericHandlerAnim(shared, currobj, eventData, idConversion, forceType);
  AnimObjectSetEventTrack(shared, currobj, eventData.eventKeys);
}

static void CreateHelper(
    CAnimData *shared,
    const MDLGENOBJECT &helperdata,
    const TSStackArray<unsigned int> &idConversion,
    MDLTRACKTYPE forceType
) {
  CAnimObj *currobj = AnimObjectCreateHelper(shared);
  ASSERT(currobj);
  GenericHandlerAnim(shared, currobj, helperdata, idConversion, forceType);
}

static void CreateAttachmentPoint(
    CAnimData *shared,
    const MDLDATA &data,
    unsigned int attachId,
    const TSStackArray<unsigned int> &idConversion,
    MDLTRACKTYPE forceType
) {
  ASSERT(shared);
  const MDLATTACHMENTSECTION &attachment = data.attachments[attachId];
  CAnimModelObj *currobj = AnimObjectCreateAttachment(shared);
  ASSERT(currobj);
  GenericHandlerAnim(shared, currobj, attachment, idConversion, forceType);
  AnimObjectSetVisibilityTrack(shared, currobj, attachment.visibilityKeys, forceType);

  const MDLGENOBJECT *object = &attachment;
  while (object->parentId != static_cast<unsigned int>(-1)) {
    object = data.objects[object->parentId];
    if (object >= data.bones.Ptr() && object < data.bones.Ptr() + data.bones.Count()) {
      const MDLBONESECTION *bone = static_cast<const MDLBONESECTION *>(object);
      currobj->geosetId = bone->geosetId == static_cast<unsigned int>(-1) ? 0xFF : bone->geosetAnimId;
      return;
    }
  }
}

static void BuildHierarchy(
    CAnimData *shared,
    const MDLDATA &data,
    const TSStackArray<unsigned int> &idConversion
) {
  unsigned int numObjects = data.objects.Count();
  for (unsigned int i = 0; i < numObjects; ++i) {
    const MDLGENOBJECT *object = data.objects[i];
    unsigned int objectId = idConversion[object->objectId];
    if (objectId == static_cast<unsigned int>(-1)) {
      continue;
    }

    unsigned int parentId = object->parentId;
    if (parentId != static_cast<unsigned int>(-1)) {
      parentId = idConversion[parentId];
      FATALASSERT(parentId != static_cast<unsigned int>(-1));
    }
    SetObjectParent(shared, objectId, parentId);
  }
}

static void IAnimCreateObjects(
    CAnimData *shared,
    const MDLDATA &data,
    unsigned int flags,
    const TSStackArray<unsigned int> &idConversion
) {
  MDLTRACKTYPE forceType = (flags & 4) ? TRACK_LINEAR : NUM_TRACK_TYPES;
  unsigned int numElements;
  unsigned int i;

  numElements = data.bones.Count();
  for (i = 0; i < numElements; ++i) {
    CreateBone(shared, data.bones[i], idConversion, forceType);
  }
  if (flags & 1) {
    numElements = data.hitTestShapes.Count();
    for (i = 0; i < numElements; ++i) {
      CreateHitTestShape(shared, data.hitTestShapes[i], idConversion, forceType);
    }
  }
  if (!(flags & 2)) {
    numElements = data.lights.Count();
    for (i = 0; i < numElements; ++i) {
      CreateLight(shared, data.lights[i], idConversion, forceType);
    }
  }
  numElements = data.helpers.Count();
  for (i = 0; i < numElements; ++i) {
    CreateHelper(shared, data.helpers[i], idConversion, forceType);
  }
  numElements = data.attachments.Count();
  for (i = 0; i < numElements; ++i) {
    CreateAttachmentPoint(shared, data, i, idConversion, forceType);
  }
  numElements = data.particleEmitters2.Count();
  for (i = 0; i < numElements; ++i) {
    CreateParticleEmitter2(shared, data.particleEmitters2[i], idConversion, forceType);
  }
  numElements = data.ribbonEmitters.Count();
  for (i = 0; i < numElements; ++i) {
    CreateRibbonEmitter(shared, data.ribbonEmitters[i], idConversion, forceType);
  }
  numElements = data.events.Count();
  for (i = 0; i < numElements; ++i) {
    CreateEventObject(shared, data.events[i], idConversion, forceType);
  }
}

void IAnimCreateObjects(
    unsigned char      *fileData,
    unsigned int        fileBytes,
    CAnimData          *shared,
    unsigned int        flags,
    const unsigned int *idConversion,
    unsigned int       *parentIds
) {
  MDLTRACKTYPE forceType = (flags & 4) ? TRACK_LINEAR : NUM_TRACK_TYPES;

  unsigned char *section = MDLFileBinarySeek(fileData, fileBytes, 0x454E4F42);
  if (section) {
    unsigned char *dataDone = section + 4 + *reinterpret_cast<unsigned int *>(section);
    unsigned int   count = *reinterpret_cast<unsigned int *>(section + 4);
    unsigned char *data = section + 8;
    for (unsigned int index = 0; index < count; ++index) {
      data = CreateBone(data, shared, idConversion, parentIds, forceType);
    }
    ASSERT(data == dataDone);
  }

  if (flags & 1) {
    section = MDLFileBinarySeek(fileData, fileBytes, 0x54535448);
    if (section) {
      unsigned char *dataDone = section + 4 + *reinterpret_cast<unsigned int *>(section);
      unsigned int   count = *reinterpret_cast<unsigned int *>(section + 4);
      unsigned char *data = section + 8;
      for (unsigned int index = 0; index < count; ++index) {
        data = CreateHitTestShape(data, shared, idConversion, parentIds, forceType);
      }
      ASSERT(data == dataDone);
    }
  }

  if (!(flags & 2)) {
    section = MDLFileBinarySeek(fileData, fileBytes, 0x4554494C);
    if (section) {
      unsigned char *dataDone = section + 4 + *reinterpret_cast<unsigned int *>(section);
      unsigned int   count = *reinterpret_cast<unsigned int *>(section + 4);
      unsigned char *data = section + 8;
      for (unsigned int index = 0; index < count; ++index) {
        data = CreateLight(data, shared, idConversion, parentIds, forceType);
      }
      ASSERT(data == dataDone);
    }
  }

  section = MDLFileBinarySeek(fileData, fileBytes, 0x504C4548);
  if (section) {
    unsigned char *dataDone = section + 4 + *reinterpret_cast<unsigned int *>(section);
    unsigned int   count = *reinterpret_cast<unsigned int *>(section + 4);
    unsigned char *data = section + 8;
    for (unsigned int index = 0; index < count; ++index) {
      data = CreateHelper(data, shared, idConversion, parentIds, forceType);
    }
    ASSERT(data == dataDone);
  }

  section = MDLFileBinarySeek(fileData, fileBytes, 0x48435441);
  if (section) {
    unsigned char *dataDone = section + 4 + *reinterpret_cast<unsigned int *>(section);
    unsigned int   count = *reinterpret_cast<unsigned int *>(section + 4);
    unsigned char *data = section + 12;
    for (unsigned int index = 0; index < count; ++index) {
      data = CreateAttachmentPoint(data, shared, idConversion, parentIds, forceType);
    }
    ASSERT(data == dataDone);
  }

  section = MDLFileBinarySeek(fileData, fileBytes, 0x32455250);
  if (section) {
    unsigned char *dataDone = section + 4 + *reinterpret_cast<unsigned int *>(section);
    unsigned int   count = *reinterpret_cast<unsigned int *>(section + 4);
    unsigned char *data = section + 8;
    for (unsigned int index = 0; index < count; ++index) {
      data = CreateParticleEmitter2(data, shared, idConversion, parentIds, forceType);
    }
    ASSERT(data == dataDone);
  }

  section = MDLFileBinarySeek(fileData, fileBytes, 0x42424952);
  if (section) {
    unsigned char *dataDone = section + 4 + *reinterpret_cast<unsigned int *>(section);
    unsigned int   count = *reinterpret_cast<unsigned int *>(section + 4);
    unsigned char *data = section + 8;
    for (unsigned int index = 0; index < count; ++index) {
      data = CreateRibbonEmitter(data, shared, idConversion, parentIds, forceType);
    }
    ASSERT(data == dataDone);
  }

  section = MDLFileBinarySeek(fileData, fileBytes, 0x53545645);
  if (section) {
    unsigned char *dataDone = section + 4 + *reinterpret_cast<unsigned int *>(section);
    unsigned int   count = *reinterpret_cast<unsigned int *>(section + 4);
    unsigned char *data = section + 8;
    for (unsigned int index = 0; index < count; ++index) {
      data = CreateEventObject(data, shared, idConversion, parentIds, forceType);
    }
    ASSERT(data == dataDone);
  }
}
int AnimBuild(unsigned char *fileData, unsigned int fileBytes, CAnim *unique, unsigned int flags) {
  if (!unique) {
    return 0;
  }

  if (flags & 8) {
    AnimEnableBlending(reinterpret_cast<HANIM>(unique), 1);
  }

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);
  MDLTRACKTYPE forceType = (flags & 4) ? TRACK_LINEAR : NUM_TRACK_TYPES;

  AnimAddSequences(fileData, fileBytes, unique, shared);
  AnimAddCameras(fileData, fileBytes, shared, forceType);
  AnimAddGeosets(fileData, fileBytes, shared, forceType);
  AnimAddTextureAnims(fileData, fileBytes, unique, shared, forceType);
  AnimAddMaterialLayers(fileData, fileBytes, unique, shared, forceType);

  unsigned int  numObjects = GetGenObjectCount(fileData, fileBytes);
  unsigned int *idConversion = static_cast<unsigned int *>(_alloca(numObjects * sizeof(unsigned int)));
  unsigned int *parentIds = static_cast<unsigned int *>(_alloca(numObjects * sizeof(unsigned int)));
  AnimBuildObjectIdTranslation(fileData, fileBytes, flags, idConversion, numObjects);
  IAnimCreateObjects(fileData, fileBytes, shared, flags, idConversion, parentIds);
  BuildHierarchy(shared, parentIds, idConversion, numObjects);
  AnimInit(unique, shared);
  return 1;
}

void IAnimInitializeTime();

void AnimInitialize() {
  IAnimInitializeTime();
}

static unsigned int CountSectionEntries(unsigned char *fileData, unsigned int fileBytes, unsigned long tag) {
  unsigned char *section = MDLFileBinarySeek(fileData, fileBytes, tag);
  return section ? *reinterpret_cast<unsigned int *>(section + 4) : 0;
}

HANIM AnimCreate(unsigned char *fileData, unsigned int fileBytes, unsigned int flags) {
  unsigned int   animatedLayers = 0;
  unsigned char *section = MDLFileBinarySeek(fileData, fileBytes, 0x534C544D);
  if (section) {
    animatedLayers = *reinterpret_cast<unsigned int *>(section + 8);
  }

  unsigned int objectCounts[7];
  objectCounts[0] = CountSectionEntries(fileData, fileBytes, 0x504C4548);
  objectCounts[1] = (flags & 2) ? 0 : CountSectionEntries(fileData, fileBytes, 0x4554494C);
  objectCounts[2] = CountSectionEntries(fileData, fileBytes, 0x48435441);
  objectCounts[3] = CountSectionEntries(fileData, fileBytes, 0x454E4F42);
  if (flags & 1) {
    objectCounts[3] += CountSectionEntries(fileData, fileBytes, 0x54535448);
  }
  objectCounts[4] = CountSectionEntries(fileData, fileBytes, 0x32455250);
  objectCounts[5] = CountSectionEntries(fileData, fileBytes, 0x42424952);
  objectCounts[6] = CountSectionEntries(fileData, fileBytes, 0x53545645);

  unsigned int numGeosets = CountSectionEntries(fileData, fileBytes, 0x414F4547);
  unsigned int numCameras = CountSectionEntries(fileData, fileBytes, 0x534D4143);
  CAnim       *unique = AnimCreate(objectCounts, numGeosets, numCameras, animatedLayers);
  if (!AnimBuild(fileData, fileBytes, unique, flags)) {
    return 0;
  }

  HANIM anim = static_cast<HANIM>(HandleCreate(unique, "HANIM"));
  if (!anim) {
    delete unique;
  }
  return anim;
}

static int AnimBuild(const MDLDATA &data, CAnim *unique, unsigned int flags) {
  if (!unique) {
    return 0;
  }
  if (flags & 8) {
    AnimEnableBlending(reinterpret_cast<HANIM>(unique), 1);
  }

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);
  MDLTRACKTYPE forceType = (flags & 4) ? TRACK_LINEAR : NUM_TRACK_TYPES;

  AnimAddSequences(unique, shared, data.sequences, data.globalSeqs);
  CameraHandlerAnim(shared, data.cameras, forceType);
  GeosetHandlerAnim(shared, data.geosetAnims, forceType);
  AnimAddTextureAnim(unique, shared, data.textureanims, forceType);
  MaterialHandlerAnim(shared, data.materials, forceType);

  unsigned int numObjects = data.objects.Count();
  TSStackArray<unsigned int> idConversion(
      static_cast<unsigned int *>(_alloca(numObjects * sizeof(unsigned int))),
      numObjects,
      numObjects
  );
  AnimBuildObjectIdTranslation(data, flags, &idConversion);
  IAnimCreateObjects(shared, data, flags, idConversion);
  BuildHierarchy(shared, data, idConversion);
  AnimInit(unique, shared);
  return 1;
}

HANIM AnimCreate(const MDLDATA &data, unsigned int flags, CStatus *status) {
  const char *animationFile = data.model.animationFile;
  if (animationFile[0]) {
    return AnimCreate(animationFile, flags, status);
  }

  unsigned int animatedLayers = 0;
  unsigned int numMaterials = data.materials.Count();
  for (unsigned int material = 0; material < numMaterials; ++material) {
    unsigned int numLayers = data.materials[material].texLayers.Count();
    for (unsigned int layer = 0; layer < numLayers; ++layer) {
      const MDLTEXLAYER &layerData = data.materials[material].texLayers[layer];
      if (layerData.alphaKeys.keys.Count() || layerData.flipKeys.keys.Count()) {
        ++animatedLayers;
      }
    }
  }

  unsigned int objectCounts[NUM_OBJ_TYPES];
  objectCounts[OBJ_TYPE_HELPER] = data.helpers.Count();
  objectCounts[OBJ_TYPE_LIGHT] = (flags & 2) ? 0 : data.lights.Count();
  objectCounts[OBJ_TYPE_MODEL] = data.attachments.Count();
  objectCounts[OBJ_TYPE_BONE] = data.bones.Count() + ((flags & 1) ? data.hitTestShapes.Count() : 0);
  objectCounts[OBJ_TYPE_EMITTER2] = data.particleEmitters2.Count();
  objectCounts[OBJ_TYPE_RIBBON] = data.ribbonEmitters.Count();
  objectCounts[OBJ_TYPE_EVENT] = data.events.Count();

  CAnim *unique = AnimCreate(
      objectCounts,
      data.geosetAnims.Count(),
      data.cameras.Count(),
      animatedLayers
  );
  if (!AnimBuild(data, unique, flags)) {
    return 0;
  }

  HANIM anim = static_cast<HANIM>(HandleCreate(unique, "HANIM"));
  if (!anim) {
    DEL(unique);
  }
  return anim;
}

HANIM AnimCreate(const char *sourcefile, unsigned int flags, CStatus *status) {
  FATALASSERT(sourcefile);

  HANIM anim = reinterpret_cast<HANIM>(GetAnim(sourcefile));
  if (anim) {
    return anim;
  }

  MDLDATA mdlData;
  if (!MDLFileRead(sourcefile, &mdlData, status)) {
    return 0;
  }

  anim = AnimCreate(mdlData, flags, status);
  if (!anim) {
    return 0;
  }
  if (AnimGetFlags(anim) & 4) {
    HandleClose(anim);
    anim = 0;
  }
  HashNewAnim(sourcefile, reinterpret_cast<HANIM__ *>(anim));
  return anim;
}

void AnimDestroy() {
  s_animCache.Clear();
}
