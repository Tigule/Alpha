#include "Anim/AnimInternal.h"

unsigned char *__fastcall MDLFileBinarySeek(unsigned char *fileData, unsigned int fileBytes, unsigned long sectionTag);

static unsigned int __fastcall SetTransformationFlags(CAnimObj *currobj) {
  unsigned int flags = 0;
  if (currobj->translation.TotalKeys()) {
    flags |= 1;
  }
  if (currobj->rotation.TotalKeys()) {
    flags |= 4;
  }
  if (currobj->scale.TotalKeys()) {
    flags |= 2;
  }
  return flags;
}

static void __fastcall ValidateInheritanceFlags(CAnimData *animptr, CAnimObj *currobj, unsigned int parentFlags) {
  ASSERT(animptr);
  ASSERT(currobj);

  if (!(parentFlags & 1)) {
    currobj->flags &= ~1;
  }
  if (!(parentFlags & 4)) {
    currobj->flags &= ~4;
  }
  if (!(parentFlags & 2)) {
    currobj->flags &= ~2;
  }

  if (currobj->flags & 1) {
    parentFlags &= ~1;
  }
  if (currobj->flags & 4) {
    parentFlags &= ~4;
  }
  if (currobj->flags & 2) {
    parentFlags &= ~2;
  }
  parentFlags |= SetTransformationFlags(currobj);

  for (unsigned int index = 0; index < currobj->childarray.Count(); ++index) {
    ValidateInheritanceFlags(animptr, currobj->childarray[index], parentFlags);
  }
}

static void __fastcall ResolveStatusPtrs(CAnim *unique, CAnimData *shared) {
  unsigned int numObjects = shared->obj.Count();
  for (unsigned int index = 0; index < numObjects; ++index) {
    CAnimObj *object = shared->obj[index];
    ASSERT(object);
    switch (object->type) {
      case 0:
        unique->status[index] = &unique->baseStatus[object->splitIndex];
        break;
      case 1:
        unique->status[index] = &unique->lightStatus[object->splitIndex];
        break;
      case 2:
        unique->status[index] = &unique->modelStatus[object->splitIndex];
        break;
      case 3:
        unique->status[index] = &unique->boneStatus[object->splitIndex];
        break;
      case 4:
        unique->status[index] = &unique->emitter2Status[object->splitIndex];
        break;
      case 5:
        unique->status[index] = &unique->ribbonStatus[object->splitIndex];
        break;
      case 6:
        unique->status[index] = &unique->eventStatus[object->splitIndex];
        break;
      default:
        ASSERT(0);
        break;
    }
  }
}

CAnimObj *__fastcall AnimObjectCreateHelper(CAnimData *shared) {
  ASSERT(shared);
  unsigned int index = shared->baseObjs.m_count++;
  CAnimObj    *newobj = &shared->baseObjs.m_data[index];
  ASSERT(newobj);
  newobj->splitIndex = index;
  return newobj;
}

CAnimLightObj *__fastcall AnimObjectCreateLight(CAnimData *shared) {
  ASSERT(shared);
  unsigned int   index = shared->lightObjs.m_count++;
  CAnimLightObj *newobj = &shared->lightObjs.m_data[index];
  ASSERT(newobj);
  newobj->splitIndex = index;
  return newobj;
}

CAnimModelObj *__fastcall AnimObjectCreateAttachment(CAnimData *shared) {
  ASSERT(shared);
  unsigned int   index = shared->modelObjs.m_count++;
  CAnimModelObj *newobj = &shared->modelObjs.m_data[index];
  ASSERT(newobj);
  newobj->splitIndex = index;
  return newobj;
}

CAnimBoneObj *__fastcall AnimObjectCreateBone(CAnimData *shared) {
  ASSERT(shared);
  unsigned int  index = shared->boneObjs.m_count++;
  CAnimBoneObj *newobj = &shared->boneObjs.m_data[index];
  ASSERT(newobj);
  newobj->splitIndex = index;
  return newobj;
}

CAnimEmitter2Obj *__fastcall AnimObjectCreateEmitter2(CAnimData *shared) {
  ASSERT(shared);
  unsigned int      index = shared->emitter2Objs.m_count++;
  CAnimEmitter2Obj *newobj = &shared->emitter2Objs.m_data[index];
  ASSERT(newobj);
  newobj->splitIndex = index;
  return newobj;
}

CAnimRibbonObj *__fastcall AnimObjectCreateRibbon(CAnimData *shared) {
  ASSERT(shared);
  unsigned int    index = shared->ribbonObjs.m_count++;
  CAnimRibbonObj *newobj = &shared->ribbonObjs.m_data[index];
  ASSERT(newobj);
  newobj->splitIndex = index;
  return newobj;
}

CAnimEventObj *__fastcall AnimObjectCreateEvent(CAnimData *shared) {
  ASSERT(shared);
  unsigned int   index = shared->eventObjs.m_count++;
  CAnimEventObj *newobj = &shared->eventObjs.m_data[index];
  ASSERT(newobj);
  newobj->splitIndex = index;
  return newobj;
}

void __fastcall AnimObjectSetIndex(CAnimData *shared, CAnimObj *objptr, unsigned int index) {
  ASSERT(shared);
  ASSERT(objptr);
  if (index >= shared->obj.Count()) {
    FATALERROR(("Object \"%s\" has an ID (%d) that exceeds the number of objects (%d)\n", objptr->name, index, shared->obj.Count()));
  }
  if (shared->obj[index]) {
    FATALERROR(("Object \"%s\" has a duplicate object ID %d\n", objptr->name, index));
  }
  objptr->animObjId = index;
  shared->obj[index] = objptr;
}

CAnimObj *__fastcall GetNodeByIndex(CAnimData *shared, unsigned int nodeIndex) {
  ASSERT(shared);
  ASSERT(nodeIndex < shared->obj.Count());
  return shared->obj[nodeIndex];
}

int __fastcall AnimObjectSetParent(CAnimData *shared, CAnimObj *objptr, unsigned int parentIndex) {
  ASSERT(shared);
  ASSERT(objptr);
  if (parentIndex == static_cast<unsigned int>(-1)) {
    shared->headarray.Add(1, &objptr);
    return 1;
  }

  CAnimObj *parent = GetNodeByIndex(shared, parentIndex);
  if (!parent) {
    return 0;
  }
  parent->childarray.Add(1, &objptr);
  return 1;
}

static KEYTYPE __fastcall GetTrackType(unsigned int mdlTrackType, MDLTRACKTYPE forceType) {
  ASSERT(mdlTrackType < NUM_TRACK_TYPES);
  if (forceType != NUM_TRACK_TYPES) {
    if (forceType == TRACK_DONT_INTERP) {
      mdlTrackType = TRACK_DONT_INTERP;
    } else if (forceType == TRACK_LINEAR && mdlTrackType > TRACK_LINEAR) {
      mdlTrackType = TRACK_LINEAR;
    }
  }

  switch (mdlTrackType) {
    case TRACK_DONT_INTERP:
      return KEY_DONT_INTERP;
    case TRACK_LINEAR:
      return KEY_LINEAR;
    case TRACK_HERMITE:
      return KEY_HERMITE;
    case TRACK_BEZIER:
      return KEY_BEZIER;
  }

  return KEY_DONT_INTERP;
}

#define ADD_KEY_FRAMES_TYPE(valueType, bytesRemaining, track, tag)                                                                            \
  ASSERT(shared);                                                                                                                             \
  ASSERT(track);                                                                                                                              \
  if (bytesRemaining >= 8 && *reinterpret_cast<unsigned int *>(data) == tag) {                                                                \
    unsigned int numKeys = *reinterpret_cast<unsigned int *>(data + 4);                                                                       \
    ASSERT(numKeys);                                                                                                                          \
    KEYTYPE trackType = GetTrackType(*reinterpret_cast<MDLTRACKTYPE *>(data + 8), forceType);                                                 \
    (track)->m_globalSeqId = *reinterpret_cast<unsigned int *>(data + 12);                                                                    \
    (track)->m_trackType = trackType;                                                                                                         \
    data += 16;                                                                                                                               \
    int          timeAdjustment = (track)->m_globalSeqId == static_cast<unsigned int>(-1) ? 0 : *reinterpret_cast<int *>(data);               \
    unsigned int valueCount = trackType < TRACK_HERMITE ? 1 : 3;                                                                              \
    (track)->SetNumKeys(numKeys, sizeof(int) + valueCount * sizeof(valueType));                                                               \
    for (unsigned int keyIndex = 0; keyIndex < numKeys; ++keyIndex) {                                                                         \
      int keyTime = *reinterpret_cast<int *>(data) - timeAdjustment;                                                                          \
      data += sizeof(int);                                                                                                                    \
      unsigned char *keyData = reinterpret_cast<unsigned char *>((track)->m_keyFrames) + (track)->m_numKeyFrames++ * (track)->m_keyFrameSize; \
      *reinterpret_cast<int *>(keyData) = keyTime;                                                                                            \
      memcpy(keyData + sizeof(int), data, valueCount * sizeof(valueType));                                                                    \
      data += valueCount * sizeof(valueType);                                                                                                 \
    }                                                                                                                                         \
    (track)->SetSequenceIndices(shared->seq);                                                                                                 \
  }

unsigned char *__fastcall AddKeyFramesType(
    unsigned char                                          *data,
    unsigned int                                            bytesRemaining,
    unsigned long                                           tag,
    CAnimData                                              *shared,
    CKeyFrameTrack<NTempest::C3Vector, NTempest::C3Vector> *track,
    MDLTRACKTYPE                                            forceType
) {
  ADD_KEY_FRAMES_TYPE(NTempest::C3Vector, bytesRemaining, track, tag);
  return data;
}

unsigned char *__fastcall AddKeyFramesType(
    unsigned char                *data,
    unsigned int                  bytesRemaining,
    unsigned long                 tag,
    CAnimData                    *shared,
    CKeyFrameTrack<float, float> *track,
    MDLTRACKTYPE                  forceType
) {
  ADD_KEY_FRAMES_TYPE(float, bytesRemaining, track, tag);
  return data;
}

unsigned char *__fastcall AnimObjectSetEventTrack(unsigned char *data, unsigned int bytesLeft, CAnimData *shared, CAnimEventObj *objptr) {
  ASSERT(shared);
  ASSERT(objptr);
  if (bytesLeft < 4 || *reinterpret_cast<unsigned int *>(data) != 0x5456454B) {
    return data;
  }

  unsigned char *dataDone = data + bytesLeft;
  unsigned int   numKeys = *reinterpret_cast<unsigned int *>(data + 4);
  objptr->events.m_globalSeqId = *reinterpret_cast<unsigned int *>(data + 8);
  data += 12;
  int timeAdjustment = objptr->events.m_globalSeqId == static_cast<unsigned int>(-1) ? 0 : *reinterpret_cast<int *>(data);
  objptr->events.SetNumKeys(numKeys, sizeof(int));
  for (unsigned int key = 0; key < numKeys; ++key) {
    objptr->events.m_keyFrames[key].time = *reinterpret_cast<int *>(data) - timeAdjustment;
    data += sizeof(int);
  }
  objptr->events.m_numKeyFrames = numKeys;
  objptr->events.SetSequenceIndices(shared->seq);
  ASSERT(data <= dataDone);
  return data;
}

unsigned char *__fastcall
AnimObjectSetTranslation(unsigned char *data, unsigned int fileBytes, CAnimData *shared, CAnimObj *objptr, MDLTRACKTYPE forceType) {
  ADD_KEY_FRAMES_TYPE(NTempest::C3Vector, fileBytes, &objptr->translation, 0x5254474B);
  return data;
}

unsigned char *__fastcall
AnimObjectSetRotation(unsigned char *data, unsigned int fileBytes, CAnimData *shared, CAnimObj *objptr, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  ASSERT(objptr);
  if (fileBytes < 8 || *reinterpret_cast<unsigned int *>(data) != 0x5452474B) {
    return data;
  }

  unsigned int numKeys = *reinterpret_cast<unsigned int *>(data + 4);
  ASSERT(numKeys);
  KEYTYPE trackType = GetTrackType(*reinterpret_cast<MDLTRACKTYPE *>(data + 8), forceType);
  objptr->rotation.m_globalSeqId = *reinterpret_cast<unsigned int *>(data + 12);
  objptr->rotation.m_trackType = trackType;
  data += 16;
  int          timeAdjustment = objptr->rotation.m_globalSeqId == static_cast<unsigned int>(-1) ? 0 : *reinterpret_cast<int *>(data);
  unsigned int valueCount = trackType < TRACK_HERMITE ? 1 : 3;
  unsigned int keySize = valueCount == 1 ? 16 : 32;
  objptr->rotation.SetNumKeys(numKeys, keySize);

  for (unsigned int keyIndex = 0; keyIndex < numKeys; ++keyIndex) {
    int keyTime = *reinterpret_cast<int *>(data) - timeAdjustment;
    data += sizeof(int);

    unsigned char *key =
        reinterpret_cast<unsigned char *>(objptr->rotation.m_keyFrames) + objptr->rotation.m_numKeyFrames++ * objptr->rotation.m_keyFrameSize;
    memset(key, 0, keySize);
    *reinterpret_cast<int *>(key) = keyTime;
    memcpy(key + 8, data, valueCount * sizeof(NTempest::C4QuaternionCompressed));
    data += valueCount * sizeof(NTempest::C4QuaternionCompressed);
  }
  objptr->rotation.SetSequenceIndices(shared->seq);
  return data;
}

unsigned char *__fastcall
AnimObjectSetScaling(unsigned char *data, unsigned int fileBytes, CAnimData *shared, CAnimObj *objptr, MDLTRACKTYPE forceType) {
  ADD_KEY_FRAMES_TYPE(NTempest::C3Vector, fileBytes, &objptr->scale, 0x4353474B);
  return data;
}

unsigned char *__fastcall
AnimObjectSetAttenuation(unsigned char *data, unsigned int fileBytes, CAnimData *shared, CAnimLightObj *objptr, MDLTRACKTYPE forceType) {
  unsigned char *fileEnd = data + fileBytes;
  {
    ADD_KEY_FRAMES_TYPE(float, fileEnd - data, &objptr->attenstart, 0x53414C4B);
  }
  {
    ADD_KEY_FRAMES_TYPE(float, fileEnd - data, &objptr->attenend, 0x45414C4B);
  }
  return data;
}

unsigned char *__fastcall
AnimObjectSetColor(unsigned char *data, unsigned int bytesRemaining, CAnimData *shared, CAnimLightObj *objptr, MDLTRACKTYPE forceType) {
  ADD_KEY_FRAMES_TYPE(C3Color, bytesRemaining, &objptr->color, 0x43414C4B);
  return data;
}

unsigned char *__fastcall
AnimObjectSetIntensity(unsigned char *data, unsigned int fileBytes, CAnimData *shared, CAnimLightObj *objptr, MDLTRACKTYPE forceType) {
  ADD_KEY_FRAMES_TYPE(float, fileBytes, &objptr->intensity, 0x49414C4B);
  return data;
}

unsigned char *__fastcall
AnimObjectSetAmbColor(unsigned char *data, unsigned int bytesRemaining, CAnimData *shared, CAnimLightObj *objptr, MDLTRACKTYPE forceType) {
  ADD_KEY_FRAMES_TYPE(C3Color, bytesRemaining, &objptr->ambColor, 0x43424C4B);
  return data;
}

unsigned char *__fastcall
AnimObjectSetAmbIntensity(unsigned char *data, unsigned int fileBytes, CAnimData *shared, CAnimLightObj *objptr, MDLTRACKTYPE forceType) {
  ADD_KEY_FRAMES_TYPE(float, fileBytes, &objptr->ambIntensity, 0x49424C4B);
  return data;
}

unsigned char *__fastcall
AnimObjectSetVisibilityTrack(unsigned char *data, unsigned int fileBytes, CAnimData *shared, CAnimVisibleObj *objptr, MDLTRACKTYPE forceType) {
  ADD_KEY_FRAMES_TYPE(float, fileBytes, &objptr->visibility, 0x5349564B);
  return data;
}

#define ANIM_FLOAT_TRACK_SETTER(name, member, tag)                                                                                                   \
  unsigned char *__fastcall name(unsigned char *data, unsigned int fileBytes, CAnimData *shared, CAnimEmitter2Obj *objptr, MDLTRACKTYPE forceType) { \
    ADD_KEY_FRAMES_TYPE(float, fileBytes, &objptr->member, tag);                                                                                     \
    return data;                                                                                                                                     \
  }

ANIM_FLOAT_TRACK_SETTER(AnimObjectSetParticleEmissionRate2, emissionRate, 0x4532504B)
ANIM_FLOAT_TRACK_SETTER(AnimObjectSetParticleGravity2, gravity, 0x4732504B)
ANIM_FLOAT_TRACK_SETTER(AnimObjectSetParticleVariation2, variation, 0x5232504B)
ANIM_FLOAT_TRACK_SETTER(AnimObjectSetEmitterLongitude2, longitude, 0x4E4C504B)
ANIM_FLOAT_TRACK_SETTER(AnimObjectSetEmitterLatitude2, latitude, 0x4C32504B)
ANIM_FLOAT_TRACK_SETTER(AnimObjectSetParticleSpeed2, particleSpeed, 0x5332504B)
ANIM_FLOAT_TRACK_SETTER(AnimObjectSetParticleLength2, length, 0x4E32504B)
ANIM_FLOAT_TRACK_SETTER(AnimObjectSetParticleWidth2, width, 0x5732504B)
ANIM_FLOAT_TRACK_SETTER(AnimObjectSetParticleZsource2, zsource, 0x5A32504B)
ANIM_FLOAT_TRACK_SETTER(AnimObjectSetParticleLifeSpan2, lifeSpan, 0x46494C4B)

#undef ANIM_FLOAT_TRACK_SETTER

#define ANIM_RIBBON_TRACK_SETTER(name, member, tag)                                                                                                \
  unsigned char *__fastcall name(unsigned char *data, unsigned int fileBytes, CAnimData *shared, CAnimRibbonObj *objptr, MDLTRACKTYPE forceType) { \
    ADD_KEY_FRAMES_TYPE(float, fileBytes, &objptr->member, tag);                                                                                   \
    return data;                                                                                                                                   \
  }

ANIM_RIBBON_TRACK_SETTER(AnimObjectSetRibbonHeightAbove, heightAbove, 0x4148524B)
ANIM_RIBBON_TRACK_SETTER(AnimObjectSetRibbonHeightBelow, heightBelow, 0x4248524B)

unsigned char *__fastcall AnimObjectSetRibbonSlot(unsigned char *data, unsigned int fileBytes, CAnimData *shared, CAnimRibbonObj *objptr) {
  ASSERT(shared);
  ASSERT(objptr);
  if (fileBytes < 8 || *reinterpret_cast<unsigned int *>(data) != 0x5854524B) {
    return data;
  }

  unsigned int numKeys = *reinterpret_cast<unsigned int *>(data + 4);
  ASSERT(numKeys);
  objptr->slot.m_globalSeqId = *reinterpret_cast<unsigned int *>(data + 12);
  objptr->slot.m_trackType = KEY_DONT_INTERP;
  data += 16;
  int timeAdjustment = objptr->slot.m_globalSeqId == static_cast<unsigned int>(-1) ? 0 : *reinterpret_cast<int *>(data);
  objptr->slot.SetNumKeys(numKeys, 2 * sizeof(unsigned int));
  for (unsigned int key = 0; key < numKeys; ++key) {
    unsigned char *keyData = reinterpret_cast<unsigned char *>(objptr->slot.m_keyFrames) + key * 2 * sizeof(unsigned int);
    *reinterpret_cast<int *>(keyData) = *reinterpret_cast<int *>(data) - timeAdjustment;
    *reinterpret_cast<unsigned int *>(keyData + sizeof(int)) = *reinterpret_cast<unsigned int *>(data + sizeof(int));
    data += 2 * sizeof(unsigned int);
  }
  objptr->slot.m_numKeyFrames = numKeys;
  objptr->slot.SetSequenceIndices(shared->seq);
  return data;
}

unsigned char *__fastcall
AnimObjectSetRibbonColor(unsigned char *data, unsigned int bytesRemaining, CAnimData *shared, CAnimRibbonObj *objptr, MDLTRACKTYPE forceType) {
  ADD_KEY_FRAMES_TYPE(C3Color, bytesRemaining, &objptr->color, 0x4F43524B);
  return data;
}

ANIM_RIBBON_TRACK_SETTER(AnimObjectSetRibbonAlpha, alpha, 0x4C41524B)

#undef ANIM_RIBBON_TRACK_SETTER

CAnim *__fastcall AnimCreate(unsigned int *const objectCounts, unsigned int numGeosets, unsigned int numCameras, unsigned int numMaterialLayers) {
  void *sharedMemory = SMemAlloc(sizeof(CAnimData), "HANIMDATA", -2, 0);
  if (!sharedMemory) {
    return 0;
  }
  CAnimData *shared = new (sharedMemory) CAnimData;

  shared->baseObjs.ReserveSpace(objectCounts[0]);
  shared->boneObjs.ReserveSpace(objectCounts[3]);
  shared->lightObjs.ReserveSpace(objectCounts[1]);
  shared->modelObjs.ReserveSpace(objectCounts[2]);
  shared->emitter2Objs.ReserveSpace(objectCounts[4]);
  shared->ribbonObjs.ReserveSpace(objectCounts[5]);
  shared->eventObjs.ReserveSpace(objectCounts[6]);

  unsigned int numObjects = 0;
  for (unsigned int type = 0; type < 7; ++type) {
    numObjects += objectCounts[type];
  }
  shared->obj.ReserveSpace(numObjects);
  shared->obj.m_count = numObjects;
  memset(shared->obj.m_data, 0, numObjects * sizeof(CAnimObj *));
  shared->geo.ReserveSpace(numGeosets);
  shared->geoIdToGeoAnimId.Reserve(numGeosets);
  shared->cameraObjs.ReserveSpace(numCameras);
  shared->layers.ReserveSpace(numMaterialLayers);
  shared->objectOrder.ReserveSpace(numObjects);
  shared->objectOrder.m_count = numObjects;
  for (unsigned int index = 0; index < numObjects; ++index) {
    shared->objectOrder[index] = index;
  }

  void *uniqueMemory = SMemAlloc(sizeof(CAnim), "HANIM", -2, 0);
  if (!uniqueMemory) {
    delete shared;
    return 0;
  }
  CAnim *unique = new (uniqueMemory) CAnim(0);

  unique->baseStatus.ReserveSpace(objectCounts[0]);
  unique->baseStatus.m_count = objectCounts[0];
  unique->boneStatus.ReserveSpace(objectCounts[3]);
  unique->boneStatus.m_count = objectCounts[3];
  unique->lightStatus.ReserveSpace(objectCounts[1]);
  unique->lightStatus.m_count = objectCounts[1];
  unique->modelStatus.ReserveSpace(objectCounts[2]);
  unique->modelStatus.m_count = objectCounts[2];
  unique->emitter2Status.ReserveSpace(objectCounts[4]);
  unique->emitter2Status.m_count = objectCounts[4];
  unique->ribbonStatus.ReserveSpace(objectCounts[5]);
  unique->ribbonStatus.m_count = objectCounts[5];
  unique->eventStatus.ReserveSpace(objectCounts[6]);
  unique->eventStatus.m_count = objectCounts[6];
  unique->status.ReserveSpace(numObjects);
  unique->status.m_count = numObjects;
  unique->geosetStatus.ReserveSpace(numGeosets);
  unique->geosetStatus.m_count = numGeosets;
  unique->cameraStatus.ReserveSpace(numCameras);
  unique->cameraStatus.m_count = numCameras;
  unique->layerStatus.ReserveSpace(numMaterialLayers);
  unique->layerStatus.m_count = numMaterialLayers;
  unique->hdata = static_cast<HANIMDATA>(HandleCreate(shared, "HANIMDATA"));
  return unique;
}

HANIM __fastcall AnimDuplicate(HANIM oldanim, unsigned int flags) {
  CAnim *oldUnique = reinterpret_cast<CAnim *>(oldanim);
  FATALASSERT(oldUnique);

  void *memory = SMemAlloc(sizeof(CAnim), "HANIM", -2, 0);
  if (!memory) {
    SErrSetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return 0;
  }

  CAnim     *unique = new (memory) CAnim(0);
  CAnimData *shared = reinterpret_cast<CAnimData *>(oldUnique->hdata);
  *unique = *oldUnique;
  ResolveStatusPtrs(unique, shared);
  unique->hdata = static_cast<HANIMDATA>(HandleCreate(shared, "HANIMDATA"));
  HANIM duplicate = static_cast<HANIM>(HandleCreate(unique, "HANIM"));
  AnimResetAnimationStatus(duplicate, flags & 1);
  return duplicate;
}

void __fastcall AnimInit(CAnim *unique, CAnimData *shared) {
  ASSERT(unique);
  ASSERT(shared);
  ResolveStatusPtrs(unique, shared);

  for (unsigned int index = 0; index < shared->headarray.Count(); ++index) {
    ValidateInheritanceFlags(shared, shared->headarray[index], 0);
  }
  if (!shared->Animates()) {
    shared->flags |= 4;
  }
  if (shared->Moves()) {
    shared->flags |= 2;
  }
  for (unsigned int geoset = 0; geoset < shared->geo.Count(); ++geoset) {
    if (shared->geo[geoset].color.TotalKeys()) {
      shared->flags |= 8;
    }
  }
  shared->flags |= 1;
}

void __fastcall AnimAddMaterialLayers(unsigned char *fileData, unsigned int fileBytes, CAnim *unique, CAnimData *shared, MDLTRACKTYPE forceType) {
  unsigned char *section = MDLFileBinarySeek(fileData, fileBytes, 0x534C544D);
  if (!section) {
    return;
  }

  unsigned char *dataDone = section + 4 + *reinterpret_cast<unsigned int *>(section);
  unsigned int   numMaterials = *reinterpret_cast<unsigned int *>(section + 4);
  unsigned int   numLayers = *reinterpret_cast<unsigned int *>(section + 8);
  unsigned char *data = section + 12;

  unique->layerStatus.ReserveSpace(numLayers);
  unique->layerStatus.m_count = numLayers;
  shared->layers.ReserveSpace(numLayers);
  shared->layers.m_count = 0;

  unsigned int layerId = 0;
  for (unsigned int material = 0; material < numMaterials; ++material) {
    unsigned char *materialDone = data + *reinterpret_cast<unsigned int *>(data);
    unsigned int   materialLayers = *reinterpret_cast<unsigned int *>(data + 8);
    data += 12;
    ASSERT(materialLayers);

    for (unsigned int layerIndex = 0; layerIndex < materialLayers; ++layerIndex, ++layerId) {
      unsigned char *layerDone = data + *reinterpret_cast<unsigned int *>(data);
      data += 28;
      if (data != layerDone) {
        CAnimMaterialLayer &layer = shared->layers.m_data[shared->layers.m_count++];
        layer.layerId = layerId;
        data = AddKeyFramesType(data, fileData + fileBytes - data, 0x41544D4B, shared, &layer.visibility, forceType);

        if (fileData + fileBytes - data >= 8 && *reinterpret_cast<unsigned int *>(data) == 0x46544D4B) {
          unsigned int numKeys = *reinterpret_cast<unsigned int *>(data + 4);
          ASSERT(numKeys);
          layer.flip.m_globalSeqId = *reinterpret_cast<unsigned int *>(data + 12);
          layer.flip.m_trackType = TRACK_DONT_INTERP;
          data += 16;
          int timeAdjustment = layer.flip.m_globalSeqId == static_cast<unsigned int>(-1) ? 0 : *reinterpret_cast<int *>(data);
          layer.flip.SetNumKeys(numKeys, sizeof(int) + sizeof(unsigned int));
          for (unsigned int key = 0; key < numKeys; ++key) {
            unsigned int   keyIndex = layer.flip.m_numKeyFrames++;
            unsigned char *keyData = reinterpret_cast<unsigned char *>(layer.flip.m_keyFrames) + keyIndex * layer.flip.m_keyFrameSize;
            *reinterpret_cast<int *>(keyData) = *reinterpret_cast<int *>(data) - timeAdjustment;
            *reinterpret_cast<unsigned int *>(keyData + sizeof(int)) = *reinterpret_cast<unsigned int *>(data + sizeof(int));
            data += sizeof(int) + sizeof(unsigned int);
          }
          layer.flip.SetSequenceIndices(shared->seq);
        }
        ASSERT(data == layerDone);
      }
    }
    ASSERT(data == materialDone);
  }
  ASSERT(data == dataDone);
}

void __fastcall AnimAddGeosets(unsigned char *fileData, unsigned int fileBytes, CAnimData *shared, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  unsigned char *section = MDLFileBinarySeek(fileData, fileBytes, 0x414F4547);
  if (!section) {
    return;
  }

  unsigned int   numGeosets = 0;
  unsigned char *geosets = MDLFileBinarySeek(fileData, fileBytes, 0x534F4547);
  if (geosets) {
    numGeosets = *reinterpret_cast<unsigned int *>(geosets + 4);
  }

  unsigned int   numGeosetAnims = *reinterpret_cast<unsigned int *>(section + 4);
  unsigned char *dataDone = section + *reinterpret_cast<unsigned int *>(section) + 4;
  unsigned char *data = section + 8;
  shared->geo.ReserveSpace(numGeosetAnims);
  shared->geo.m_count = numGeosetAnims;
  shared->geoIdToGeoAnimId.Reserve(numGeosets);
  shared->geoIdToGeoAnimId.SetCount(numGeosets);
  if (numGeosets) {
    memset(shared->geoIdToGeoAnimId.Ptr(), 0xFF, numGeosets * sizeof(unsigned int));
  }

  for (unsigned int i = 0; i < numGeosetAnims; ++i) {
    unsigned char *geosetDone = data + *reinterpret_cast<unsigned int *>(data);
    unsigned int   geosetId = *reinterpret_cast<unsigned int *>(data + 4);
    ASSERT(geosetId < numGeosets);
    shared->geoIdToGeoAnimId[geosetId] = i;
    shared->geo[i].sgGeosetId = geosetId;
    data += 28;

    data = AddKeyFramesType(data, static_cast<unsigned int>(fileData + fileBytes - data), 0x4F41474B, shared, &shared->geo[i].visibility, forceType);
    {
      ADD_KEY_FRAMES_TYPE(C3Color, static_cast<unsigned int>(fileData + fileBytes - data), &shared->geo[i].color, 0x4341474B);
    }
    ASSERT(data == geosetDone);
  }
  ASSERT(data == dataDone);
}

void __fastcall AnimAddCameras(unsigned char *fileData, unsigned int fileBytes, CAnimData *shared, MDLTRACKTYPE forceType) {
  unsigned char *section = MDLFileBinarySeek(fileData, fileBytes, 0x534D4143);
  if (!section) {
    return;
  }

  unsigned int   numCameras = *reinterpret_cast<unsigned int *>(section + 4);
  unsigned char *dataDone = section + *reinterpret_cast<unsigned int *>(section) + 4;
  unsigned char *data = section + 8;
  shared->cameraObjs.ReserveSpace(numCameras);
  shared->cameraObjs.m_count = numCameras;

  for (unsigned int i = 0; i < numCameras; ++i) {
    data += sizeof(unsigned int);
    SStrCopy(shared->cameraObjs[i].name, reinterpret_cast<const char *>(data), sizeof(shared->cameraObjs[i].name));
    data += sizeof(shared->cameraObjs[i].name);
    shared->cameraObjs[i].pivot = *reinterpret_cast<NTempest::C3Vector *>(data);
    data += sizeof(NTempest::C3Vector) + 12;
    shared->cameraObjs[i].targetPivot = *reinterpret_cast<NTempest::C3Vector *>(data);
    data += sizeof(NTempest::C3Vector);

    data = AddKeyFramesType(
        data, static_cast<unsigned int>(fileData + fileBytes - data), 0x5254434B, shared, &shared->cameraObjs[i].translation, forceType
    );
    data = AddKeyFramesType(data, static_cast<unsigned int>(fileData + fileBytes - data), 0x4C52434B, shared, &shared->cameraObjs[i].roll, forceType);
    data = AddKeyFramesType(
        data, static_cast<unsigned int>(fileData + fileBytes - data), 0x5254544B, shared, &shared->cameraObjs[i].targetTranslation, forceType
    );
    data = AddKeyFramesType(
        data, static_cast<unsigned int>(fileData + fileBytes - data), 0x5349564B, shared, &shared->cameraObjs[i].visibility, forceType
    );
  }
  ASSERT(data == dataDone);
}

void __fastcall AnimAddSequences(unsigned char *fileData, unsigned int fileBytes, CAnim *unique, CAnimData *shared) {
  unsigned char *sequenceSection = MDLFileBinarySeek(fileData, fileBytes, 0x53514553);
  unsigned int   numSequences = 0;
  unsigned char *data = 0;
  unsigned char *seqDataDone = 0;
  if (sequenceSection) {
    seqDataDone = sequenceSection + 4 + *reinterpret_cast<unsigned int *>(sequenceSection);
    numSequences = *reinterpret_cast<unsigned int *>(sequenceSection + 4);
    ASSERT(numSequences);
    data = sequenceSection + 8;
  }

  shared->seq.ReserveSpace(numSequences);
  shared->seq.m_count = numSequences;
  for (unsigned int i = 0; i < numSequences; ++i) {
    CAnimSequence &sequence = shared->seq[i];
    SStrCopy(reinterpret_cast<char *>(&sequence.name), reinterpret_cast<const char *>(data), 80);
    data += 80;
    sequence.time.l = *reinterpret_cast<int *>(data);
    data += 4;
    sequence.time.h = *reinterpret_cast<int *>(data);
    data += 4;
    sequence.moveSpeed = *reinterpret_cast<float *>(data);
    data += 4;
    sequence.flags = *reinterpret_cast<unsigned int *>(data);
    data += 4;
    sequence.bounds.radius = *reinterpret_cast<float *>(data);
    data += 4;
    memcpy(&sequence.bounds.extent.b, data, sizeof(NTempest::C3Vector));
    data += sizeof(NTempest::C3Vector);
    memcpy(&sequence.bounds.extent.t, data, sizeof(NTempest::C3Vector));
    data += sizeof(NTempest::C3Vector);
    float pickChance = *reinterpret_cast<float *>(data) * 32767.0f;
    data += 4;
    sequence.randPickChance = pickChance <= 0.0f ? -static_cast<int>(-pickChance + 0.5f) : static_cast<unsigned int>(pickChance + 0.5f);
    sequence.replay.l = *reinterpret_cast<int *>(data);
    data += 4;
    sequence.replay.h = *reinterpret_cast<int *>(data);
    data += 4;
    unsigned int blendTime = *reinterpret_cast<unsigned int *>(data);
    data += 4;
    if (blendTime) {
      sequence.blendTime = blendTime;
    }
  }
  ASSERT(seqDataDone == data);

  unique->seq.ReserveSpace(numSequences);
  unique->seq.m_count = numSequences;

  unsigned char *globalSection = MDLFileBinarySeek(fileData, fileBytes, 0x53424C47);
  unsigned int   numGlobalSequences = 0;
  unsigned char *globalData = 0;
  unsigned char *globalDataDone = 0;
  if (globalSection) {
    unsigned int globalBytes = *reinterpret_cast<unsigned int *>(globalSection);
    globalData = globalSection + 4;
    globalDataDone = globalData + globalBytes;
    numGlobalSequences = globalBytes / sizeof(unsigned int);
    ASSERT(numGlobalSequences * sizeof(unsigned int) == globalBytes);
  }

  unique->seqLastTime = IAnimGetCurrTimeMs();
  unique->globalSeqElapsed.ReserveSpace(numGlobalSequences);
  unique->globalSeqElapsed.m_count = numGlobalSequences;
  if (numGlobalSequences) {
    memset(unique->globalSeqElapsed.m_data, 0, numGlobalSequences * sizeof(unsigned int));
  }
  shared->globalSeqLength.Set(numGlobalSequences, reinterpret_cast<unsigned int *>(globalData));
  ASSERT(globalData + numGlobalSequences * sizeof(unsigned int) == globalDataDone);
}

void __fastcall AnimAddTextureAnims(unsigned char *fileData, unsigned int fileBytes, CAnim *unique, CAnimData *shared, MDLTRACKTYPE forceType) {
  ASSERT(unique);
  ASSERT(shared);
  unsigned char *section = MDLFileBinarySeek(fileData, fileBytes, 0x4E415854);
  if (!section) {
    return;
  }

  unsigned char *dataDone = section + 4 + *reinterpret_cast<unsigned int *>(section);
  unsigned int   numTexAnims = *reinterpret_cast<unsigned int *>(section + 4);
  unsigned char *data = section + 8;
  shared->tex.ReserveSpace(numTexAnims);
  shared->tex.m_count = numTexAnims;

  for (unsigned int i = 0; i < numTexAnims; ++i) {
    unsigned char *animDone = data + *reinterpret_cast<unsigned int *>(data);
    data += 4;
    CAnimTransform &transform = shared->tex[i];
    data = AddKeyFramesType(data, fileData + fileBytes - data, 0x5441544B, shared, &transform.translation, forceType);

    if (fileData + fileBytes - data >= 8 && *reinterpret_cast<unsigned int *>(data) == 0x5241544B) {
      unsigned int numKeys = *reinterpret_cast<unsigned int *>(data + 4);
      ASSERT(numKeys);
      KEYTYPE trackType = GetTrackType(*reinterpret_cast<MDLTRACKTYPE *>(data + 8), forceType);
      transform.rotation.m_globalSeqId = *reinterpret_cast<unsigned int *>(data + 12);
      transform.rotation.m_trackType = trackType;
      data += 16;
      int          timeAdjustment = transform.rotation.m_globalSeqId == static_cast<unsigned int>(-1) ? 0 : *reinterpret_cast<int *>(data);
      unsigned int valueCount = trackType < TRACK_HERMITE ? 1 : 3;
      unsigned int keySize = valueCount == 1 ? 16 : 32;
      transform.rotation.SetNumKeys(numKeys, keySize);
      for (unsigned int key = 0; key < numKeys; ++key) {
        int time = *reinterpret_cast<int *>(data);
        data += sizeof(int);
        unsigned char *keyData = reinterpret_cast<unsigned char *>(transform.rotation.m_keyFrames) +
                                 transform.rotation.m_numKeyFrames++ * transform.rotation.m_keyFrameSize;
        memset(keyData, 0, keySize);
        *reinterpret_cast<int *>(keyData) = time - timeAdjustment;
        memcpy(keyData + 8, data, valueCount * sizeof(NTempest::C4QuaternionCompressed));
        data += valueCount * sizeof(NTempest::C4QuaternionCompressed);
      }
      transform.rotation.SetSequenceIndices(shared->seq);
    }

    data = AddKeyFramesType(data, fileData + fileBytes - data, 0x5341544B, shared, &transform.scale, forceType);
    ASSERT(animDone == data);
  }
  ASSERT(dataDone == data);

  unique->textureStatus.ReserveSpace(numTexAnims);
  unique->textureStatus.m_count = numTexAnims;
}

#undef ADD_KEY_FRAMES_TYPE
