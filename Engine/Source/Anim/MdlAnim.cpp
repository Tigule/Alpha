#include "Base/Base.h"
#include "Anim/AnimInternal.h"
#include "MDLFile/MDLTypes.h"
#include "Base/Status.h"

#include <stddef.h>
#include <malloc.h>
#include <stpl.h>

BYTE *MDLFileBinarySeek(BYTE *fileData, UINT fileBytes, DWORD sectionTag);
BOOL  MDLFileRead(LPCSTR path, MDLDATA *data, CStatus *status);

CAnim *AnimCreate(UINT *const objectCounts, UINT numGeosets, UINT numCameras, UINT numMaterialLayers);
HANIM  AnimCreate(LPCSTR sourcefile, UINT flags, CStatus *status);
static BOOL   AnimBuild(BYTE *fileData, UINT fileBytes, CAnim *unique, UINT flags);

void  AnimAddSequences(BYTE *fileData, UINT fileBytes, CAnim *unique, CAnimData *shared);
void  AnimAddCameras(BYTE *fileData, UINT fileBytes, CAnimData *shared, MDLTRACKTYPE forceType);
void  AnimAddGeosets(BYTE *fileData, UINT fileBytes, CAnimData *shared, MDLTRACKTYPE forceType);
void  AnimAddTextureAnims(BYTE *fileData, UINT fileBytes, CAnim *unique, CAnimData *shared, MDLTRACKTYPE forceType);
void  AnimAddMaterialLayers(BYTE *fileData, UINT fileBytes, CAnim *unique, CAnimData *shared, MDLTRACKTYPE forceType);
static UINT  GetGenObjectCount(BYTE *fileData, UINT fileBytes);
static BYTE *CreateBone(BYTE *, CAnimData *, const UINT *, UINT *, MDLTRACKTYPE);
static BYTE *CreateHitTestShape(BYTE *, CAnimData *, const UINT *, UINT *, MDLTRACKTYPE);
static BYTE *CreateLight(BYTE *, CAnimData *, const UINT *, UINT *, MDLTRACKTYPE);
static BYTE *CreateHelper(BYTE *, CAnimData *, const UINT *, UINT *, MDLTRACKTYPE);
static BYTE *CreateAttachmentPoint(BYTE *, CAnimData *, const UINT *, UINT *, MDLTRACKTYPE);
static BYTE *CreateParticleEmitter2(BYTE *, CAnimData *, const UINT *, UINT *, MDLTRACKTYPE);
static BYTE *CreateRibbonEmitter(BYTE *, CAnimData *, const UINT *, UINT *, MDLTRACKTYPE);
static BYTE *CreateEventObject(BYTE *, CAnimData *, const UINT *, UINT *, MDLTRACKTYPE);

static void BuildHierarchy(CAnimData *shared, const UINT *parentIds, const UINT *idConversion, UINT numObjects);
void AnimInit(CAnim *unique, CAnimData *shared);
void AnimAddSequences(
    CAnim                                      *unique,
    CAnimData                                  *shared,
    const TSGrowableArray<MDLSEQUENCESSECTION> &sequences,
    const TSGrowableArray<MDLGLOBALSEQSECTION> &globalSeqs
);
void AnimAddTextureAnim(CAnim *unique, CAnimData *shared, const TSGrowableArray<MDLTEXANIMSECTION> &textureAnims, MDLTRACKTYPE forceType);
void AnimAddMaterialLayer(CAnimData *, const MDLTEXLAYER &, UINT, MDLTRACKTYPE);
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

static int AnimGetReferenceCount(HANIM__ *anim) {
  CAnim *container = reinterpret_cast<CAnim *>(anim);
  VALIDATEBEGIN;
  VALIDATE(container);
  VALIDATEEND;
  return container->GetRefCount();
}

static HANIM__ *GetAnim(LPCSTR modelFName) {
  FATALASSERT(modelFName);
  ANIMHASH *entry = s_animCache.Ptr(modelFName);
  if (!entry) {
    return 0;
  }
  if (AnimGetReferenceCount(entry->anim) <= 1) {
    return reinterpret_cast<HANIM__ *>(HandleDuplicate(entry->anim));
  }
  return AnimDuplicate(entry->anim, 0);
}

static void HashNewAnim(LPCSTR modelFName, HANIM__ *anim) {
  FATALASSERT(modelFName);
  ANIMHASH *entry = s_animCache.New(modelFName, 0, 0);
  entry->anim = reinterpret_cast<HANIM>(HandleDuplicate(anim));
}

static BYTE GetObjectFlags(UINT mdlFlags) {
  BYTE flags = 0;
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

static BYTE *
GenericHandlerAnim(BYTE *fileData, CAnimData *shared, CAnimObj *currobj, const UINT *idConversion, UINT *parentIds, MDLTRACKTYPE forceType) {
  FATALASSERT(shared);
  FATALASSERT(currobj);
  UINT  genObjBytes = *reinterpret_cast<UINT *>(fileData);
  BYTE *data = fileData + sizeof(UINT);
  SStrCopy(currobj->name, reinterpret_cast<LPCSTR>(data), sizeof(currobj->name));
  data += sizeof(currobj->name);
  UINT objectId = *reinterpret_cast<UINT *>(data);
  data += sizeof(UINT);
  parentIds[objectId] = *reinterpret_cast<UINT *>(data);
  data += sizeof(UINT);
  AnimObjectSetIndex(shared, currobj, idConversion[objectId]);
  currobj->flags = GetObjectFlags(*reinterpret_cast<UINT *>(data));
  data += sizeof(UINT);
  data = AnimObjectSetTranslation(data, genObjBytes - (data - fileData), shared, currobj, forceType);
  data = AnimObjectSetRotation(data, genObjBytes - (data - fileData), shared, currobj, forceType);
  data = AnimObjectSetScaling(data, genObjBytes - (data - fileData), shared, currobj, forceType);
  BYTE *dataDone = fileData + genObjBytes;
  FATALASSERT(data == dataDone);
  return data;
}

static BYTE *CreateBone(BYTE *fileData, CAnimData *shared, const UINT *idConversion, UINT *parentIds, MDLTRACKTYPE forceType) {
  FATALASSERT(shared);
  CAnimBoneObj *currobj = AnimObjectCreateBone(shared);
  FATALASSERT(currobj);
  fileData = GenericHandlerAnim(fileData, shared, currobj, idConversion, parentIds, forceType);
  UINT geosetId = *reinterpret_cast<UINT *>(fileData);
  fileData += sizeof(UINT);
  UINT geosetAnimId = *reinterpret_cast<UINT *>(fileData);
  fileData += sizeof(UINT);
  currobj->geosetId = geosetId == static_cast<UINT>(-1) ? static_cast<BYTE>(-1) : static_cast<BYTE>(geosetAnimId);
  return fileData;
}

static BYTE *CreateHitTestShape(BYTE *fileData, CAnimData *shared, const UINT *idConversion, UINT *parentIds, MDLTRACKTYPE forceType) {
  FATALASSERT(shared);
  CAnimBoneObj *currobj = AnimObjectCreateBone(shared);
  FATALASSERT(currobj);
  UINT sectionLength = *reinterpret_cast<UINT *>(fileData);
  GenericHandlerAnim(fileData + 4, shared, currobj, idConversion, parentIds, forceType);
  currobj->geosetId = 0xFF;
  return fileData + sectionLength;
}

static BYTE *CreateLight(BYTE *fileData, CAnimData *shared, const UINT *idConversion, UINT *parentIds, MDLTRACKTYPE forceType) {
  FATALASSERT(shared);
  CAnimLightObj *currobj = AnimObjectCreateLight(shared);
  FATALASSERT(currobj);
  UINT  sectionLength = *reinterpret_cast<UINT *>(fileData);
  BYTE *data = GenericHandlerAnim(fileData + 4, shared, currobj, idConversion, parentIds, forceType);
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

static BYTE *CreateParticleEmitter2(BYTE *fileData, CAnimData *shared, const UINT *idConversion, UINT *parentIds, MDLTRACKTYPE forceType) {
  FATALASSERT(shared);
  CAnimEmitter2Obj *currobj = AnimObjectCreateEmitter2(shared);
  FATALASSERT(currobj);
  UINT  sectionLength = *reinterpret_cast<UINT *>(fileData);
  BYTE *data = GenericHandlerAnim(fileData + 4, shared, currobj, idConversion, parentIds, forceType);

  data += *reinterpret_cast<UINT *>(data);
  currobj->squirts = *reinterpret_cast<UINT *>(data);
  data += sizeof(UINT);
  data = AnimObjectSetParticleEmissionRate2(data, fileData + sectionLength - data, shared, currobj, forceType);
  data = AnimObjectSetParticleGravity2(data, fileData + sectionLength - data, shared, currobj, forceType);
  data = AnimObjectSetEmitterLongitude2(data, fileData + sectionLength - data, shared, currobj, forceType);
  data = AnimObjectSetEmitterLatitude2(data, fileData + sectionLength - data, shared, currobj, forceType);
  data = AnimObjectSetParticleSpeed2(data, fileData + sectionLength - data, shared, currobj, forceType);
  data = AnimObjectSetParticleVariation2(data, fileData + sectionLength - data, shared, currobj, forceType);
  data = AnimObjectSetParticleLength2(data, fileData + sectionLength - data, shared, currobj, forceType);
  data = AnimObjectSetParticleWidth2(data, fileData + sectionLength - data, shared, currobj, forceType);
  data = AnimObjectSetParticleZsource2(data, fileData + sectionLength - data, shared, currobj, forceType);
  data = AnimObjectSetVisibilityTrack(data, fileData + sectionLength - data, shared, currobj, forceType);
  data = AnimObjectSetParticleLifeSpan2(data, fileData + sectionLength - data, shared, currobj, forceType);
  ASSERT(data == (fileData + sectionLength));
  return data;
}

static BYTE *CreateRibbonEmitter(BYTE *fileData, CAnimData *shared, const UINT *idConversion, UINT *parentIds, MDLTRACKTYPE forceType) {
  FATALASSERT(shared);
  CAnimRibbonObj *currobj = AnimObjectCreateRibbon(shared);
  FATALASSERT(currobj);
  UINT  sectionLength = *reinterpret_cast<UINT *>(fileData);
  BYTE *data = GenericHandlerAnim(fileData + 4, shared, currobj, idConversion, parentIds, forceType);
  data += *reinterpret_cast<UINT *>(data);
  data = AnimObjectSetRibbonHeightAbove(data, fileData + sectionLength - data, shared, currobj, forceType);
  data = AnimObjectSetRibbonHeightBelow(data, fileData + sectionLength - data, shared, currobj, forceType);
  data = AnimObjectSetRibbonAlpha(data, fileData + sectionLength - data, shared, currobj, forceType);
  data = AnimObjectSetRibbonColor(data, fileData + sectionLength - data, shared, currobj, forceType);
  data = AnimObjectSetRibbonSlot(data, fileData + sectionLength - data, shared, currobj);
  data = AnimObjectSetVisibilityTrack(data, fileData + sectionLength - data, shared, currobj, forceType);
  ASSERT(data == (fileData + sectionLength));
  return data;
}

static BYTE *CreateEventObject(BYTE *fileData, CAnimData *shared, const UINT *idConversion, UINT *parentIds, MDLTRACKTYPE forceType) {
  FATALASSERT(shared);
  CAnimEventObj *currobj = AnimObjectCreateEvent(shared);
  FATALASSERT(currobj);
  UINT  sectionLength = *reinterpret_cast<UINT *>(fileData);
  BYTE *data = GenericHandlerAnim(fileData + 4, shared, currobj, idConversion, parentIds, forceType);
  data = AnimObjectSetEventTrack(data, fileData + sectionLength - data, shared, currobj);
  ASSERT(data == (fileData + sectionLength));
  return data;
}

static BYTE *CreateHelper(BYTE *fileData, CAnimData *shared, const UINT *idConversion, UINT *parentIds, MDLTRACKTYPE forceType) {
  FATALASSERT(shared);
  CAnimObj *currobj = AnimObjectCreateHelper(shared);
  FATALASSERT(currobj);
  return GenericHandlerAnim(fileData, shared, currobj, idConversion, parentIds, forceType);
}

static BYTE *CreateAttachmentPoint(BYTE *fileData, CAnimData *shared, const UINT *idConversion, UINT *parentIds, MDLTRACKTYPE forceType) {
  FATALASSERT(shared);
  CAnimModelObj *currobj = AnimObjectCreateAttachment(shared);
  FATALASSERT(currobj);
  UINT  sectionLength = *reinterpret_cast<UINT *>(fileData);
  BYTE *data = GenericHandlerAnim(fileData + 4, shared, currobj, idConversion, parentIds, forceType);
  data += sizeof(UINT);
  currobj->geosetId = *data;
  data += 0x105;
  data = AnimObjectSetVisibilityTrack(data, fileData + sectionLength - data, shared, currobj, forceType);
  ASSERT(data == (fileData + sectionLength));
  return data;
}

static void SetObjectParent(CAnimData *shared, UINT object, UINT parent) {
  FATALASSERT(shared);
  CAnimObj *animobj = GetNodeByIndex(shared, object);
  FATALASSERT(animobj);
  int setObjParent = AnimObjectSetParent(shared, animobj, parent);
  FATALASSERT(setObjParent);
}

static void BuildHierarchy(CAnimData *shared, const UINT *parentIds, const UINT *idConversion, UINT numObjects) {
  for (UINT oldObjectId = 0; oldObjectId < numObjects; ++oldObjectId) {
    UINT objectId = idConversion[oldObjectId];
    if (objectId == static_cast<UINT>(-1)) {
      continue;
    }

    UINT parentId = parentIds[oldObjectId];
    if (parentId != static_cast<UINT>(-1)) {
      parentId = idConversion[parentId];
      FATALASSERT(parentId != 0xffffffff);
    }

    SetObjectParent(shared, objectId, parentId);
  }
}

static UINT GetGenObjectCount(BYTE *fileData, UINT fileBytes) {
  BYTE *data = MDLFileBinarySeek(fileData, fileBytes, 'LDOM');
  ASSERT(data != 0);
  return *reinterpret_cast<UINT *>(data + 373);
}

UINT AnimBuildObjectIdTranslation(const MDLDATA &data, UINT flags, TSStackArray<UINT> *idConversion) {
  ASSERT(idConversion);

  UINT numObjects = data.objects.Count();
  UINT index;
  for (index = 0; index < numObjects; ++index) {
    (*idConversion)[index] = index;
  }

  UINT numRemoved = 0;
  if (!(flags & 1) && data.hitTestShapes.Count() > 0) {
    UINT firstObject = data.hitTestShapes[0].objectId;
    UINT offset = data.hitTestShapes.Count();
    for (index = firstObject; index < firstObject + offset; ++index) {
      (*idConversion)[index] = static_cast<UINT>(-1);
    }
    for (index = firstObject + offset; index < numObjects; ++index) {
      (*idConversion)[index] = index - offset;
    }
    numRemoved = offset;
  }

  if ((flags & 2) && data.lights.Count() > 0) {
    UINT firstObject = data.lights[0].objectId;
    UINT offset = data.lights.Count();
    for (index = firstObject; index < firstObject + offset; ++index) {
      (*idConversion)[index] = static_cast<UINT>(-1);
    }
    for (index = firstObject + offset; index < numObjects; ++index) {
      if ((*idConversion)[index] != static_cast<UINT>(-1)) {
        (*idConversion)[index] = index - offset;
      }
    }
    numRemoved += offset;
  }

  return numRemoved;
}

UINT AnimBuildObjectIdTranslation(BYTE *fileData, UINT fileBytes, UINT flags, UINT *idConversion, UINT numObjects) {
  ASSERT(idConversion);

  UINT index;
  for (index = 0; index < numObjects; ++index) {
    idConversion[index] = index;
  }

  UINT numRemoved = 0;
  if (!(flags & 1)) {
    BYTE *section = MDLFileBinarySeek(fileData, fileBytes, 'TSTH');
    if (section) {
      section += sizeof(UINT);
      UINT offset = *reinterpret_cast<UINT *>(section);
      UINT firstObject = *reinterpret_cast<UINT *>(section + 92);
      for (index = firstObject; index < firstObject + offset; ++index) {
        idConversion[index] = static_cast<UINT>(-1);
      }
      for (index = firstObject + offset; index < numObjects; ++index) {
        idConversion[index] = index - offset;
      }
      numRemoved = offset;
    }
  }

  if (flags & 2) {
    BYTE *section = MDLFileBinarySeek(fileData, fileBytes, 'ETIL');
    if (section) {
      section += sizeof(UINT);
      UINT offset = *reinterpret_cast<UINT *>(section);
      UINT firstObject = *reinterpret_cast<UINT *>(section + 92);
      for (index = firstObject; index < firstObject + offset; ++index) {
        idConversion[index] = static_cast<UINT>(-1);
      }
      for (index = firstObject + offset; index < numObjects; ++index) {
        if (idConversion[index] != static_cast<UINT>(-1)) {
          idConversion[index] = index - offset;
        }
      }
      numRemoved += offset;
    }
  }

  return numRemoved;
}

static void MaterialHandlerAnim(CAnimData *shared, const TSGrowableArray<MDLMATERIALSECTION> &materials, MDLTRACKTYPE forceType) {
  UINT                      layerId = 0;
  const MDLMATERIALSECTION *material = materials.Ptr();
  for (UINT i = materials.Count(); i; --i, ++material) {
    const MDLTEXLAYER *layer = material->texLayers.Ptr();
    for (UINT j = material->texLayers.Count(); j; --j, ++layer, ++layerId) {
      AnimAddMaterialLayer(shared, *layer, layerId, forceType);
    }
  }
}

static void GeosetHandlerAnim(CAnimData *shared, const TSGrowableArray<MDLGEOSETANIMSECTION> &geosets, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  const MDLGEOSETANIMSECTION *geoset = geosets.Ptr();
  for (UINT i = 0; i < geosets.Count(); ++i, ++geoset) {
    AnimAddGeoset(shared, *geoset, forceType);
  }
}

static void CameraHandlerAnim(CAnimData *shared, const TSGrowableArray<MDLCAMERASECTION> &cameras, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  const MDLCAMERASECTION *camera = cameras.Ptr();
  for (UINT i = 0; i < cameras.Count(); ++i, ++camera) {
    AnimAddCamera(shared, *camera, forceType);
  }
}

static void
GenericHandlerAnim(CAnimData *shared, CAnimObj *currobj, const MDLGENOBJECT &obj, const TSStackArray<UINT> &idConversion, MDLTRACKTYPE forceType) {
  SStrCopy(currobj->name, obj.name, sizeof(currobj->name));
  AnimObjectSetIndex(shared, currobj, idConversion[obj.objectId]);
  currobj->flags = GetObjectFlags(obj.flags);

  if (shared->seq.Count() > 0) {
    AnimObjectSetTranslation(shared, currobj, obj.transkeys, forceType);
    AnimObjectSetRotation(shared, currobj, obj.rotkeys, forceType);
    AnimObjectSetScaling(shared, currobj, obj.scalekeys, forceType);
  }
}

static void CreateBone(CAnimData *shared, const MDLBONESECTION &bonedata, const TSStackArray<UINT> &idConversion, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  CAnimBoneObj *currobj = AnimObjectCreateBone(shared);
  ASSERT(currobj);
  GenericHandlerAnim(shared, currobj, bonedata, idConversion, forceType);
  currobj->geosetId = bonedata.geosetId == static_cast<UINT>(-1) ? static_cast<BYTE>(-1) : static_cast<BYTE>(bonedata.geosetAnimId);
}

static void CreateHitTestShape(CAnimData *shared, const MDLHITTESTSHAPE &hitTest, const TSStackArray<UINT> &idConversion, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  CAnimBoneObj *currobj = AnimObjectCreateBone(shared);
  ASSERT(currobj);
  GenericHandlerAnim(shared, currobj, hitTest, idConversion, forceType);
  currobj->geosetId = 0xFF;
}

static void CreateLight(CAnimData *shared, const MDLLIGHTSECTION &lightdata, const TSStackArray<UINT> &idConversion, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  CAnimLightObj *currobj = AnimObjectCreateLight(shared);
  ASSERT(currobj);
  GenericHandlerAnim(shared, currobj, lightdata, idConversion, forceType);
  if (shared->seq.Count()) {
    if (lightdata.attenstartkeys.keys.Count() || lightdata.attenendkeys.keys.Count()) {
      AnimObjectSetAttenuation(shared, currobj, lightdata.attenstartkeys, lightdata.attenendkeys, forceType);
    }
    if (lightdata.colorkeys.keys.Count()) {
      AnimObjectSetColor(shared, currobj, lightdata.colorkeys, forceType);
    }
    if (lightdata.intensitykeys.keys.Count()) {
      AnimObjectSetIntensity(shared, currobj, lightdata.intensitykeys, forceType);
    }
    if (lightdata.ambcolorkeys.keys.Count()) {
      AnimObjectSetAmbColor(shared, currobj, lightdata.ambcolorkeys, forceType);
    }
    if (lightdata.ambintensitykeys.keys.Count()) {
      AnimObjectSetAmbIntensity(shared, currobj, lightdata.ambintensitykeys, forceType);
    }
    AnimObjectSetVisibilityTrack(shared, currobj, lightdata.visibilityKeys, forceType);
  }
}

static void
CreateParticleEmitter2(CAnimData *shared, const MDLPARTICLEEMITTER2 &emitterdata, const TSStackArray<UINT> &idConversion, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  CAnimEmitter2Obj *currobj = AnimObjectCreateEmitter2(shared);
  ASSERT(currobj);
  GenericHandlerAnim(shared, currobj, emitterdata, idConversion, forceType);
  if (shared->seq.Count()) {
    if (emitterdata.emissionRate.keys.Count()) {
      AnimObjectSetParticleEmissionRate2(shared, currobj, emitterdata.emissionRate, forceType);
    }
    if (emitterdata.gravity.keys.Count()) {
      AnimObjectSetParticleGravity2(shared, currobj, emitterdata.gravity, forceType);
    }
    if (emitterdata.latitude.keys.Count()) {
      AnimObjectSetEmitterLatitude2(shared, currobj, emitterdata.latitude, forceType);
    }
    if (emitterdata.longitude.keys.Count()) {
      AnimObjectSetEmitterLongitude2(shared, currobj, emitterdata.longitude, forceType);
    }
    if (emitterdata.speed.keys.Count()) {
      AnimObjectSetParticleSpeed2(shared, currobj, emitterdata.speed, forceType);
    }
    if (emitterdata.variation.keys.Count()) {
      AnimObjectSetParticleVariation2(shared, currobj, emitterdata.variation, forceType);
    }
    if (emitterdata.length.keys.Count()) {
      AnimObjectSetParticleLength2(shared, currobj, emitterdata.length, forceType);
    }
    if (emitterdata.width.keys.Count()) {
      AnimObjectSetParticleWidth2(shared, currobj, emitterdata.width, forceType);
    }
    if (emitterdata.zsource.keys.Count()) {
      AnimObjectSetParticleZsource2(shared, currobj, emitterdata.zsource, forceType);
    }
    if (emitterdata.life.keys.Count()) {
      AnimObjectSetParticleLifeSpan2(shared, currobj, emitterdata.life, forceType);
    }
    currobj->squirts = emitterdata.squirts;
    AnimObjectSetVisibilityTrack(shared, currobj, emitterdata.visibilityKeys, forceType);
  }
}

static void
CreateRibbonEmitter(CAnimData *shared, const MDLRIBBONEMITTER &ribbondata, const TSStackArray<UINT> &idConversion, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  CAnimRibbonObj *currobj = AnimObjectCreateRibbon(shared);
  ASSERT(currobj);
  GenericHandlerAnim(shared, currobj, ribbondata, idConversion, forceType);
  if (shared->seq.Count()) {
    if (ribbondata.heightAbove.keys.Count()) {
      AnimObjectSetRibbonHeightAbove(shared, currobj, ribbondata.heightAbove, forceType);
    }
    if (ribbondata.heightBelow.keys.Count()) {
      AnimObjectSetRibbonHeightBelow(shared, currobj, ribbondata.heightBelow, forceType);
    }
    if (ribbondata.textureSlot.keys.Count()) {
      AnimObjectSetRibbonSlot(shared, currobj, ribbondata.textureSlot);
    }
    if (ribbondata.colorKeys.keys.Count()) {
      AnimObjectSetRibbonColor(shared, currobj, ribbondata.colorKeys, forceType);
    }
    if (ribbondata.alphaKeys.keys.Count()) {
      AnimObjectSetRibbonAlpha(shared, currobj, ribbondata.alphaKeys, forceType);
    }
    AnimObjectSetVisibilityTrack(shared, currobj, ribbondata.visibilityKeys, forceType);
  }
}

static void CreateEventObject(CAnimData *shared, const MDLEVENTSECTION &eventData, const TSStackArray<UINT> &idConversion, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  CAnimEventObj *currobj = AnimObjectCreateEvent(shared);
  ASSERT(currobj);
  GenericHandlerAnim(shared, currobj, eventData, idConversion, forceType);
  if (shared->seq.Count() > 0) {
    AnimObjectSetEventTrack(shared, currobj, eventData.eventKeys);
  }
}

static void CreateHelper(CAnimData *shared, const MDLGENOBJECT &helperdata, const TSStackArray<UINT> &idConversion, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  CAnimObj *currobj = AnimObjectCreateHelper(shared);
  ASSERT(currobj);
  GenericHandlerAnim(shared, currobj, helperdata, idConversion, forceType);
}

static void
CreateAttachmentPoint(CAnimData *shared, const MDLDATA &data, UINT attachId, const TSStackArray<UINT> &idConversion, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  const MDLATTACHMENTSECTION &attachment = data.attachments[attachId];
  CAnimModelObj              *currobj = AnimObjectCreateAttachment(shared);
  ASSERT(currobj);
  GenericHandlerAnim(shared, currobj, attachment, idConversion, forceType);
  AnimObjectSetVisibilityTrack(shared, currobj, attachment.visibilityKeys, forceType);

  const MDLGENOBJECT *object = &attachment;
  while (object->parentId != static_cast<UINT>(-1)) {
    object = data.objects[object->parentId];
    if (static_cast<UINT>(reinterpret_cast<const BYTE *>(object) - reinterpret_cast<const BYTE *>(data.bones.Ptr())) >= data.bones.Bytes()) {
      continue;
    }
    const MDLBONESECTION *bone = static_cast<const MDLBONESECTION *>(object);
    currobj->geosetId = bone->geosetId == static_cast<UINT>(-1) ? static_cast<BYTE>(-1) : static_cast<BYTE>(bone->geosetAnimId);
    return;
  }
}

static void BuildHierarchy(CAnimData *shared, const MDLDATA &data, const TSStackArray<UINT> &idConversion) {
  UINT numObjects = data.objects.Count();
  for (UINT i = 0; i < numObjects; ++i) {
    UINT objectId = idConversion[i];
    if (objectId == static_cast<UINT>(-1)) {
      continue;
    }

    UINT parentId = data.objects[i]->parentId;
    if (parentId != static_cast<UINT>(-1)) {
      parentId = idConversion[parentId];
      FATALASSERT(parentId != 0xffffffff);
    }
    SetObjectParent(shared, objectId, parentId);
  }
}

static void IAnimCreateObjects(CAnimData *shared, const MDLDATA &data, UINT flags, const TSStackArray<UINT> &idConversion) {
  MDLTRACKTYPE forceType = (flags & 4) ? TRACK_LINEAR : NUM_TRACK_TYPES;
  UINT         numElements;
  UINT         i;

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

static void IAnimCreateObjects(BYTE *fileData, UINT fileBytes, CAnimData *shared, UINT flags, const UINT *idConversion, UINT *parentIds) {
  MDLTRACKTYPE forceType = (flags & 4) ? TRACK_LINEAR : NUM_TRACK_TYPES;

  BYTE *section = MDLFileBinarySeek(fileData, fileBytes, 'ENOB');
  if (section) {
    UINT sectionBytes = *reinterpret_cast<UINT *>(section);
    section += sizeof(UINT);
    BYTE *dataDone = section + sectionBytes;
    UINT  count = *reinterpret_cast<UINT *>(section);
    BYTE *data = section + sizeof(UINT);
    for (UINT index = 0; index < count; ++index) {
      data = CreateBone(data, shared, idConversion, parentIds, forceType);
    }
    ASSERT(data == dataDone);
  }

  if (flags & 1) {
    section = MDLFileBinarySeek(fileData, fileBytes, 'TSTH');
    if (section) {
      UINT sectionBytes = *reinterpret_cast<UINT *>(section);
      section += sizeof(UINT);
      BYTE *dataDone = section + sectionBytes;
      UINT  count = *reinterpret_cast<UINT *>(section);
      BYTE *data = section + sizeof(UINT);
      for (UINT index = 0; index < count; ++index) {
        data = CreateHitTestShape(data, shared, idConversion, parentIds, forceType);
      }
      ASSERT(data == dataDone);
    }
  }

  if (!(flags & 2)) {
    section = MDLFileBinarySeek(fileData, fileBytes, 'ETIL');
    if (section) {
      UINT sectionBytes = *reinterpret_cast<UINT *>(section);
      section += sizeof(UINT);
      BYTE *dataDone = section + sectionBytes;
      UINT  count = *reinterpret_cast<UINT *>(section);
      BYTE *data = section + sizeof(UINT);
      for (UINT index = 0; index < count; ++index) {
        data = CreateLight(data, shared, idConversion, parentIds, forceType);
      }
      ASSERT(data == dataDone);
    }
  }

  section = MDLFileBinarySeek(fileData, fileBytes, 'PLEH');
  if (section) {
    UINT sectionBytes = *reinterpret_cast<UINT *>(section);
    section += sizeof(UINT);
    BYTE *dataDone = section + sectionBytes;
    UINT  count = *reinterpret_cast<UINT *>(section);
    BYTE *data = section + sizeof(UINT);
    for (UINT index = 0; index < count; ++index) {
      data = CreateHelper(data, shared, idConversion, parentIds, forceType);
    }
    ASSERT(data == dataDone);
  }

  section = MDLFileBinarySeek(fileData, fileBytes, 'HCTA');
  if (section) {
    UINT sectionBytes = *reinterpret_cast<UINT *>(section);
    section += sizeof(UINT);
    BYTE *dataDone = section + sectionBytes;
    UINT  count = *reinterpret_cast<UINT *>(section);
    BYTE *data = section + 2 * sizeof(UINT);
    for (UINT index = 0; index < count; ++index) {
      data = CreateAttachmentPoint(data, shared, idConversion, parentIds, forceType);
    }
    ASSERT(data == dataDone);
  }

  section = MDLFileBinarySeek(fileData, fileBytes, '2ERP');
  if (section) {
    UINT sectionBytes = *reinterpret_cast<UINT *>(section);
    section += sizeof(UINT);
    BYTE *dataDone = section + sectionBytes;
    UINT  count = *reinterpret_cast<UINT *>(section);
    BYTE *data = section + sizeof(UINT);
    for (UINT index = 0; index < count; ++index) {
      data = CreateParticleEmitter2(data, shared, idConversion, parentIds, forceType);
    }
    ASSERT(data == dataDone);
  }

  section = MDLFileBinarySeek(fileData, fileBytes, 'BBIR');
  if (section) {
    UINT sectionBytes = *reinterpret_cast<UINT *>(section);
    section += sizeof(UINT);
    BYTE *dataDone = section + sectionBytes;
    UINT  count = *reinterpret_cast<UINT *>(section);
    BYTE *data = section + sizeof(UINT);
    for (UINT index = 0; index < count; ++index) {
      data = CreateRibbonEmitter(data, shared, idConversion, parentIds, forceType);
    }
    ASSERT(data == dataDone);
  }

  section = MDLFileBinarySeek(fileData, fileBytes, 'STVE');
  if (section) {
    UINT sectionBytes = *reinterpret_cast<UINT *>(section);
    section += sizeof(UINT);
    BYTE *dataDone = section + sectionBytes;
    UINT  count = *reinterpret_cast<UINT *>(section);
    BYTE *data = section + sizeof(UINT);
    for (UINT index = 0; index < count; ++index) {
      data = CreateEventObject(data, shared, idConversion, parentIds, forceType);
    }
    ASSERT(data == dataDone);
  }
}
static BOOL AnimBuild(BYTE *fileData, UINT fileBytes, CAnim *unique, UINT flags) {
  if (!unique) {
    return 0;
  }

  if (flags & 8) {
    unique->flags |= 0x10;
    unique->blendStatus.SetCount(unique->status.Count());
  }

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);
  MDLTRACKTYPE forceType = (flags & 4) ? TRACK_LINEAR : NUM_TRACK_TYPES;

  AnimAddSequences(fileData, fileBytes, unique, shared);
  AnimAddCameras(fileData, fileBytes, shared, forceType);
  AnimAddGeosets(fileData, fileBytes, shared, forceType);
  AnimAddTextureAnims(fileData, fileBytes, unique, shared, forceType);
  AnimAddMaterialLayers(fileData, fileBytes, unique, shared, forceType);

  UINT  numObjects = GetGenObjectCount(fileData, fileBytes);
  UINT *idConversion = static_cast<UINT *>(_alloca(numObjects * sizeof(UINT)));
  UINT *parentIds = static_cast<UINT *>(_alloca(numObjects * sizeof(UINT)));
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

static UINT CountSectionEntries(BYTE *fileData, UINT fileBytes, DWORD tag) {
  BYTE *section = MDLFileBinarySeek(fileData, fileBytes, tag);
  if (!section) {
    return 0;
  }
  return *reinterpret_cast<UINT *>(section + 4);
}

HANIM AnimCreate(BYTE *fileData, UINT fileBytes, UINT flags) {
  UINT  animatedLayers = 0;
  BYTE *section = MDLFileBinarySeek(fileData, fileBytes, 'SLTM');
  if (section) {
    animatedLayers = *reinterpret_cast<UINT *>(section + 8);
  }

  UINT objectCounts[7];
  objectCounts[0] = CountSectionEntries(fileData, fileBytes, 'PLEH');
  objectCounts[1] = (flags & 2) ? 0 : CountSectionEntries(fileData, fileBytes, 'ETIL');
  objectCounts[2] = CountSectionEntries(fileData, fileBytes, 'HCTA');
  objectCounts[3] = CountSectionEntries(fileData, fileBytes, 'ENOB');
  if (flags & 1) {
    objectCounts[3] += CountSectionEntries(fileData, fileBytes, 'TSTH');
  }
  objectCounts[4] = CountSectionEntries(fileData, fileBytes, '2ERP');
  objectCounts[5] = CountSectionEntries(fileData, fileBytes, 'BBIR');
  objectCounts[6] = CountSectionEntries(fileData, fileBytes, 'STVE');

  UINT   numGeosets = CountSectionEntries(fileData, fileBytes, 'AOEG');
  UINT   numCameras = CountSectionEntries(fileData, fileBytes, 'SMAC');
  CAnim *unique = AnimCreate(objectCounts, numGeosets, numCameras, animatedLayers);
  if (AnimBuild(fileData, fileBytes, unique, flags)) {
    HANIM anim = CREATEHANDLE(HANIM, unique);
    if (anim) {
      return anim;
    }
    delete unique;
  }
  return 0;
}

static BOOL AnimBuild(const MDLDATA &data, CAnim *unique, UINT flags) {
  if (!unique) {
    return 0;
  }
  if (flags & 8) {
    unique->flags |= 0x10;
    unique->blendStatus.SetCount(unique->status.Count());
  }

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);
  MDLTRACKTYPE forceType = (flags & 4) ? TRACK_LINEAR : NUM_TRACK_TYPES;

  AnimAddSequences(unique, shared, data.sequences, data.globalSeqs);
  CameraHandlerAnim(shared, data.cameras, forceType);
  GeosetHandlerAnim(shared, data.geosetAnims, forceType);
  AnimAddTextureAnim(unique, shared, data.textureanims, forceType);
  MaterialHandlerAnim(shared, data.materials, forceType);

  UINT               numObjects = data.objects.Count();
  TSStackArray<UINT> idConversion(static_cast<UINT *>(_alloca(numObjects * sizeof(UINT))), numObjects, numObjects);
  AnimBuildObjectIdTranslation(data, flags, &idConversion);
  IAnimCreateObjects(shared, data, flags, idConversion);
  BuildHierarchy(shared, data, idConversion);
  AnimInit(unique, shared);
  return 1;
}

HANIM AnimCreate(const MDLDATA &data, UINT flags, CStatus *status) {
  if (data.model.animationFile[0]) {
    return AnimCreate(data.model.animationFile, flags, status);
  }

  UINT animatedLayers = 0;
  UINT numMaterials = data.materials.Count();
  for (UINT i = 0; i < numMaterials; ++i) {
    UINT numLayers = data.materials[i].texLayers.Count();
    for (UINT layer = 0; layer < numLayers; ++layer) {
      if (data.materials[i].texLayers[layer].alphaKeys.keys.Count() > 0 || data.materials[i].texLayers[layer].flipKeys.keys.Count() > 0) {
        ++animatedLayers;
      }
    }
  }

  UINT objectCounts[NUM_OBJ_TYPES];
  objectCounts[OBJ_TYPE_BONE] = data.bones.Count();
  if (flags & 1) {
    objectCounts[OBJ_TYPE_BONE] += data.hitTestShapes.Count();
  }
  objectCounts[OBJ_TYPE_HELPER] = data.helpers.Count();
  objectCounts[OBJ_TYPE_LIGHT] = (flags & 2) ? 0 : data.lights.Count();
  objectCounts[OBJ_TYPE_MODEL] = data.attachments.Count();
  objectCounts[OBJ_TYPE_EMITTER2] = data.particleEmitters2.Count();
  objectCounts[OBJ_TYPE_RIBBON] = data.ribbonEmitters.Count();
  objectCounts[OBJ_TYPE_EVENT] = data.events.Count();

  CAnim *unique = AnimCreate(objectCounts, data.geosetAnims.Count(), data.cameras.Count(), animatedLayers);
  if (AnimBuild(data, unique, flags)) {
    HANIM anim = CREATEHANDLE(HANIM, unique);
    if (anim) {
      return anim;
    }
    delete unique;
  }
  return 0;
}

HANIM AnimCreate(LPCSTR sourcefile, UINT flags, CStatus *status) {
  VALIDATEBEGIN;
  VALIDATE(sourcefile);
  VALIDATEEND;

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
