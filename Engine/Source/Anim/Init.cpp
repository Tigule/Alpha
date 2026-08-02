#include "MDLFile/MDLTypes.h"
#include "Anim/AnimInternal.h"

unsigned char *MDLFileBinarySeek(unsigned char *fileData, unsigned int fileBytes, unsigned long sectionTag);

static unsigned int SetTransformationFlags(CAnimObj *currobj) {
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

static void ValidateInheritanceFlags(CAnimData *animptr, CAnimObj *currobj, unsigned int parentFlags) {
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

static void ResolveStatusPtrs(CAnim *unique, CAnimData *shared) {
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

CAnimObj *AnimObjectCreateHelper(CAnimData *shared) {
  ASSERT(shared);
  unsigned int index = shared->baseObjs.Count();
  CAnimObj    *newobj = shared->baseObjs.New();
  ASSERT(newobj);
  newobj->splitIndex = index;
  return newobj;
}

CAnimLightObj *AnimObjectCreateLight(CAnimData *shared) {
  ASSERT(shared);
  unsigned int   index = shared->lightObjs.Count();
  CAnimLightObj *newobj = shared->lightObjs.New();
  ASSERT(newobj);
  newobj->splitIndex = index;
  return newobj;
}

CAnimModelObj *AnimObjectCreateAttachment(CAnimData *shared) {
  ASSERT(shared);
  unsigned int   index = shared->modelObjs.Count();
  CAnimModelObj *newobj = shared->modelObjs.New();
  ASSERT(newobj);
  newobj->splitIndex = index;
  return newobj;
}

CAnimBoneObj *AnimObjectCreateBone(CAnimData *shared) {
  ASSERT(shared);
  unsigned int  index = shared->boneObjs.Count();
  CAnimBoneObj *newobj = shared->boneObjs.New();
  ASSERT(newobj);
  newobj->splitIndex = index;
  return newobj;
}

CAnimEmitter2Obj *AnimObjectCreateEmitter2(CAnimData *shared) {
  ASSERT(shared);
  unsigned int      index = shared->emitter2Objs.Count();
  CAnimEmitter2Obj *newobj = shared->emitter2Objs.New();
  ASSERT(newobj);
  newobj->splitIndex = index;
  return newobj;
}

CAnimRibbonObj *AnimObjectCreateRibbon(CAnimData *shared) {
  ASSERT(shared);
  unsigned int    index = shared->ribbonObjs.Count();
  CAnimRibbonObj *newobj = shared->ribbonObjs.New();
  ASSERT(newobj);
  newobj->splitIndex = index;
  return newobj;
}

CAnimEventObj *AnimObjectCreateEvent(CAnimData *shared) {
  ASSERT(shared);
  unsigned int   index = shared->eventObjs.Count();
  CAnimEventObj *newobj = shared->eventObjs.New();
  ASSERT(newobj);
  newobj->splitIndex = index;
  return newobj;
}

void AnimObjectSetIndex(CAnimData *shared, CAnimObj *objptr, unsigned int index) {
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

CAnimObj *GetNodeByIndex(CAnimData *shared, unsigned int nodeIndex) {
  ASSERT(shared);
  ASSERT(nodeIndex < shared->obj.Count());
  return shared->obj[nodeIndex];
}

int AnimObjectSetParent(CAnimData *shared, CAnimObj *objptr, unsigned int parentIndex) {
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

static KEYTYPE GetTrackType(unsigned int mdlTrackType, MDLTRACKTYPE forceType) {
  ASSERT(mdlTrackType < NUM_TRACK_TYPES);
  if (forceType != NUM_TRACK_TYPES) {
    if (forceType == TRACK_NO_INTERP) {
      mdlTrackType = TRACK_NO_INTERP;
    } else if (forceType == TRACK_LINEAR && mdlTrackType > TRACK_LINEAR) {
      mdlTrackType = TRACK_LINEAR;
    }
  }

  switch (mdlTrackType) {
    case TRACK_NO_INTERP:
      return KEYTYPE_NOINTERP;
    case TRACK_LINEAR:
      return KEYTYPE_LINEAR;
    case TRACK_HERMITE:
      return KEYTYPE_HERMITE;
    case TRACK_BEZIER:
      return KEYTYPE_BEZIER;
  }

  return KEYTYPE_NOINTERP;
}

void AddKeyFrames(
    CAnimData *shared,
    const MDLKEYTRACK<NTempest::C3Vector> &keyTrack,
    CKeyFrameTrack<NTempest::C3Vector, NTempest::C3Vector> *interp,
    MDLTRACKTYPE forceType
) {
  ASSERT(shared);
  ASSERT(interp);
  if (!keyTrack.keys.Count()) {
    return;
  }
  interp->SetGlobalSequenceId(keyTrack.globalSeqId);
  interp->SetTrackType(GetTrackType(keyTrack.type, forceType));
  interp->SetNumKeys(keyTrack.keys.Count());
  int timeAdjustment = keyTrack.globalSeqId == static_cast<unsigned int>(-1) ? 0 : keyTrack.keys[0].time;
  for (unsigned int key = 0; key < keyTrack.keys.Count(); ++key) {
    if (interp->GetTrackType() < KEYTYPE_HERMITE) {
      interp->AddKey(keyTrack.keys[key].time - timeAdjustment, keyTrack.keys[key].value);
    } else {
      interp->AddKey(
          keyTrack.keys[key].time - timeAdjustment,
          keyTrack.keys[key].value,
          keyTrack.keys[key].inTan,
          keyTrack.keys[key].outTan
      );
    }
  }
  interp->SetSequenceIndices(shared->seq);
}

void AddKeyFrames(
    CAnimData *shared,
    const MDLKEYTRACK<C3Color> &keyTrack,
    CKeyFrameTrack<C3Color, C3Color> *interp,
    MDLTRACKTYPE forceType
) {
  ASSERT(shared);
  ASSERT(interp);
  if (!keyTrack.keys.Count()) {
    return;
  }
  interp->SetGlobalSequenceId(keyTrack.globalSeqId);
  interp->SetTrackType(GetTrackType(keyTrack.type, forceType));
  interp->SetNumKeys(keyTrack.keys.Count());
  int timeAdjustment = keyTrack.globalSeqId == static_cast<unsigned int>(-1) ? 0 : keyTrack.keys[0].time;
  for (unsigned int key = 0; key < keyTrack.keys.Count(); ++key) {
    if (interp->GetTrackType() < KEYTYPE_HERMITE) {
      interp->AddKey(keyTrack.keys[key].time - timeAdjustment, keyTrack.keys[key].value);
    } else {
      interp->AddKey(
          keyTrack.keys[key].time - timeAdjustment,
          keyTrack.keys[key].value,
          keyTrack.keys[key].inTan,
          keyTrack.keys[key].outTan
      );
    }
  }
  interp->SetSequenceIndices(shared->seq);
}

void AnimObjectSetVisibilityTrack(
    CAnimData *shared,
    CAnimVisibleObj *objptr,
    const MDLKEYTRACK<float> &keyTrack,
    MDLTRACKTYPE forceType
) {
  ASSERT(shared);
  ASSERT(objptr);
  ASSERT(shared);
  ASSERT(&objptr->visibility);
  if (!keyTrack.keys.Count()) {
    return;
  }
  CKeyFrameTrack<float, float> &interp = objptr->visibility;
  interp.SetGlobalSequenceId(keyTrack.globalSeqId);
  interp.SetTrackType(GetTrackType(keyTrack.type, forceType));
  interp.SetNumKeys(keyTrack.keys.Count());
  int timeAdjustment = keyTrack.globalSeqId == static_cast<unsigned int>(-1) ? 0 : keyTrack.keys[0].time;
  for (unsigned int i = 0; i < keyTrack.keys.Count(); ++i) {
    if (interp.GetTrackType() < KEYTYPE_HERMITE) {
      interp.AddKey(keyTrack.keys[i].time - timeAdjustment, keyTrack.keys[i].value);
    } else {
      interp.AddKey(
          keyTrack.keys[i].time - timeAdjustment,
          keyTrack.keys[i].value,
          keyTrack.keys[i].inTan,
          keyTrack.keys[i].outTan
      );
    }
  }
  interp.SetSequenceIndices(shared->seq);
}

void AnimObjectSetTranslation(
    CAnimData *shared,
    CAnimObj *objptr,
    const MDLKEYTRACK<NTempest::C3Vector> &keyTrack,
    MDLTRACKTYPE forceType
) {
  ASSERT(shared);
  ASSERT(objptr);
  AddKeyFrames(shared, keyTrack, &objptr->translation, forceType);
}

void AnimObjectSetRotation(
    CAnimData *shared,
    CAnimObj *objptr,
    const MDLKEYTRACK<NTempest::C4Quaternion> &keyTrack,
    MDLTRACKTYPE forceType
) {
  ASSERT(shared);
  ASSERT(objptr);
  ASSERT(shared);
  ASSERT(&objptr->rotation);
  if (!keyTrack.keys.Count()) {
    return;
  }

  CKeyFrameTrack<NTempest::C4QuaternionCompressed, NTempest::C4Quaternion> &interp = objptr->rotation;
  interp.SetGlobalSequenceId(keyTrack.globalSeqId);
  interp.SetTrackType(GetTrackType(keyTrack.type, forceType));
  interp.SetNumKeys(keyTrack.keys.Count());
  int timeAdjustment = keyTrack.globalSeqId == static_cast<unsigned int>(-1) ? 0 : keyTrack.keys[0].time;

  for (unsigned int i = 0; i < keyTrack.keys.Count(); ++i) {
    if (interp.GetTrackType() < KEYTYPE_HERMITE) {
      interp.AddKey(keyTrack.keys[i].time - timeAdjustment, keyTrack.keys[i].value);
    } else {
      interp.AddKey(
          keyTrack.keys[i].time - timeAdjustment,
          keyTrack.keys[i].value,
          keyTrack.keys[i].inTan,
          keyTrack.keys[i].outTan
      );
    }
  }
  interp.SetSequenceIndices(shared->seq);
}

void AnimObjectSetScaling(
    CAnimData *shared,
    CAnimObj *objptr,
    const MDLKEYTRACK<NTempest::C3Vector> &keyTrack,
    MDLTRACKTYPE forceType
) {
  ASSERT(shared);
  ASSERT(objptr);
  AddKeyFrames(shared, keyTrack, &objptr->scale, forceType);
}

#define SET_MDL_FLOAT_TRACK(trackMember)                                                                                                 \
  ASSERT(shared);                                                                                                                       \
  ASSERT(objptr);                                                                                                                       \
  ASSERT(shared);                                                                                                                       \
  ASSERT(&objptr->trackMember);                                                                                                         \
  if (keyTrack.keys.Count()) {                                                                                                          \
    CKeyFrameTrack<float, float> &interp = objptr->trackMember;                                                                         \
    interp.SetGlobalSequenceId(keyTrack.globalSeqId);                                                                                    \
    interp.SetTrackType(GetTrackType(keyTrack.type, forceType));                                                                         \
    interp.SetNumKeys(keyTrack.keys.Count());                                                                                            \
    int timeAdjustment = keyTrack.globalSeqId == static_cast<unsigned int>(-1) ? 0 : keyTrack.keys[0].time;                             \
    for (unsigned int i = 0; i < keyTrack.keys.Count(); ++i) {                                                                          \
      if (interp.GetTrackType() < KEYTYPE_HERMITE) {                                                                                         \
        interp.AddKey(keyTrack.keys[i].time - timeAdjustment, keyTrack.keys[i].value);                                                   \
      } else {                                                                                                                           \
        interp.AddKey(                                                                                                                    \
            keyTrack.keys[i].time - timeAdjustment,                                                                                      \
            keyTrack.keys[i].value,                                                                                                      \
            keyTrack.keys[i].inTan,                                                                                                      \
            keyTrack.keys[i].outTan                                                                                                      \
        );                                                                                                                               \
      }                                                                                                                                  \
    }                                                                                                                                   \
    interp.SetSequenceIndices(shared->seq);                                                                                             \
  }

void AnimObjectSetAttenuation(
    CAnimData *shared,
    CAnimLightObj *objptr,
    const MDLKEYTRACK<float> &startTrack,
    const MDLKEYTRACK<float> &endTrack,
    MDLTRACKTYPE forceType
) {
  {
    const MDLKEYTRACK<float> &keyTrack = startTrack;
    SET_MDL_FLOAT_TRACK(attenstart);
  }
  {
    const MDLKEYTRACK<float> &keyTrack = endTrack;
    SET_MDL_FLOAT_TRACK(attenend);
  }
}

void AnimObjectSetColor(CAnimData *shared, CAnimLightObj *objptr, const MDLKEYTRACK<C3Color> &keyTrack, MDLTRACKTYPE forceType) {
  AddKeyFrames(shared, keyTrack, &objptr->color, forceType);
}

void AnimObjectSetIntensity(CAnimData *shared, CAnimLightObj *objptr, const MDLKEYTRACK<float> &keyTrack, MDLTRACKTYPE forceType) {
  SET_MDL_FLOAT_TRACK(intensity);
}

void AnimObjectSetAmbColor(CAnimData *shared, CAnimLightObj *objptr, const MDLKEYTRACK<C3Color> &keyTrack, MDLTRACKTYPE forceType) {
  AddKeyFrames(shared, keyTrack, &objptr->ambColor, forceType);
}

void AnimObjectSetAmbIntensity(CAnimData *shared, CAnimLightObj *objptr, const MDLKEYTRACK<float> &keyTrack, MDLTRACKTYPE forceType) {
  SET_MDL_FLOAT_TRACK(ambIntensity);
}

#define DEFINE_EMITTER_FLOAT_SETTER(functionName, trackMember)                                                                           \
  void functionName(                                                                                                          \
      CAnimData *shared, CAnimEmitter2Obj *objptr, const MDLKEYTRACK<float> &keyTrack, MDLTRACKTYPE forceType                            \
  ) {                                                                                                                                     \
    SET_MDL_FLOAT_TRACK(trackMember);                                                                                                     \
  }

DEFINE_EMITTER_FLOAT_SETTER(AnimObjectSetParticleEmissionRate2, emissionRate)
DEFINE_EMITTER_FLOAT_SETTER(AnimObjectSetParticleGravity2, gravity)
DEFINE_EMITTER_FLOAT_SETTER(AnimObjectSetParticleVariation2, variation)
DEFINE_EMITTER_FLOAT_SETTER(AnimObjectSetEmitterLongitude2, longitude)
DEFINE_EMITTER_FLOAT_SETTER(AnimObjectSetEmitterLatitude2, latitude)
DEFINE_EMITTER_FLOAT_SETTER(AnimObjectSetParticleSpeed2, particleSpeed)
DEFINE_EMITTER_FLOAT_SETTER(AnimObjectSetParticleLength2, length)
DEFINE_EMITTER_FLOAT_SETTER(AnimObjectSetParticleWidth2, width)
DEFINE_EMITTER_FLOAT_SETTER(AnimObjectSetParticleZsource2, zsource)
DEFINE_EMITTER_FLOAT_SETTER(AnimObjectSetParticleLifeSpan2, lifeSpan)

#undef DEFINE_EMITTER_FLOAT_SETTER

#define DEFINE_RIBBON_FLOAT_SETTER(functionName, trackMember)                                                                            \
  void functionName(                                                                                                          \
      CAnimData *shared, CAnimRibbonObj *objptr, const MDLKEYTRACK<float> &keyTrack, MDLTRACKTYPE forceType                              \
  ) {                                                                                                                                     \
    SET_MDL_FLOAT_TRACK(trackMember);                                                                                                     \
  }

DEFINE_RIBBON_FLOAT_SETTER(AnimObjectSetRibbonHeightAbove, heightAbove)
DEFINE_RIBBON_FLOAT_SETTER(AnimObjectSetRibbonHeightBelow, heightBelow)
DEFINE_RIBBON_FLOAT_SETTER(AnimObjectSetRibbonAlpha, alpha)

#undef DEFINE_RIBBON_FLOAT_SETTER

void AnimObjectSetRibbonColor(
    CAnimData *shared,
    CAnimRibbonObj *currobj,
    const MDLKEYTRACK<C3Color> &keyTrack,
    MDLTRACKTYPE forceType
) {
  AddKeyFrames(shared, keyTrack, &currobj->color, forceType);
}

void AnimObjectSetRibbonSlot(
    CAnimData *shared,
    CAnimRibbonObj *currobj,
    const MDLSIMPLEKEYTRACK<MDLINTKEY> &keyTrack
) {
  ASSERT(shared);
  ASSERT(currobj);
  if (!keyTrack.keys.Count()) {
    return;
  }

  currobj->slot.SetGlobalSequenceId(keyTrack.globalSeqId);
  currobj->slot.SetTrackType(KEYTYPE_NOINTERP);
  currobj->slot.SetNumKeys(keyTrack.keys.Count());
  for (unsigned int i = 0; i < keyTrack.keys.Count(); ++i) {
    currobj->slot.AddKey(
        keyTrack.keys[i].time - (keyTrack.globalSeqId == static_cast<unsigned int>(-1) ? 0 : keyTrack.keys[0].time),
        keyTrack.keys[i].value
    );
  }
  currobj->slot.SetSequenceIndices(shared->seq);
}

void AnimObjectSetEventTrack(
    CAnimData *shared,
    CAnimEventObj *objptr,
    const MDLSIMPLEKEYTRACK<MDLEVENTKEY> &keyTrack
) {
  ASSERT(shared);
  ASSERT(objptr);
  unsigned int numKeys = keyTrack.keys.Count();
  if (!numKeys) {
    return;
  }

  objptr->events.SetGlobalSequenceId(keyTrack.globalSeqId);
  objptr->events.SetNumKeys(numKeys, sizeof(CKeyFrame));
  int timeAdjustment = keyTrack.globalSeqId == static_cast<unsigned int>(-1) ? 0 : keyTrack.keys[0].time;
  for (unsigned int i = 0; i < numKeys; ++i) {
    objptr->events.AddKey(keyTrack.keys[i].time - timeAdjustment);
  }
  objptr->events.SetSequenceIndices(shared->seq);
}

#undef SET_MDL_FLOAT_TRACK

#define ADD_KEY_FRAMES_TYPE(valueType, bytesRemaining, track, tag, fileData)                                                                  \
  ASSERT(shared);                                                                                                                             \
  ASSERT(track);                                                                                                                              \
  if (bytesRemaining >= 8 && *reinterpret_cast<unsigned int *>(fileData) == tag) {                                                            \
    unsigned int numKeys = *reinterpret_cast<unsigned int *>(fileData + 4);                                                                   \
    ASSERT(numKeys);                                                                                                                          \
    KEYTYPE trackType = GetTrackType(*reinterpret_cast<MDLTRACKTYPE *>(fileData + 8), forceType);                                             \
    (track)->m_globalSeqId = *reinterpret_cast<unsigned int *>(fileData + 12);                                                                \
    (track)->SetTrackType(trackType);                                                                                                         \
    fileData += 16;                                                                                                                           \
    int          timeAdjustment = (track)->m_globalSeqId == static_cast<unsigned int>(-1) ? 0 : *reinterpret_cast<int *>(fileData);           \
    unsigned int valueCount = trackType < TRACK_HERMITE ? 1 : 3;                                                                              \
    (track)->SetNumKeys(numKeys, sizeof(int) + valueCount * sizeof(valueType));                                                               \
    for (unsigned int keyIndex = 0; keyIndex < numKeys; ++keyIndex) {                                                                         \
      int keyTime = *reinterpret_cast<int *>(fileData) - timeAdjustment;                                                                      \
      fileData += sizeof(int);                                                                                                                \
      unsigned char *keyData = reinterpret_cast<unsigned char *>((track)->m_keyFrames) + (track)->m_numKeyFrames++ * (track)->m_keyFrameSize; \
      *reinterpret_cast<int *>(keyData) = keyTime;                                                                                            \
      memcpy(keyData + sizeof(int), fileData, valueCount * sizeof(valueType));                                                                \
      fileData += valueCount * sizeof(valueType);                                                                                             \
    }                                                                                                                                         \
    (track)->SetSequenceIndices(shared->seq);                                                                                                 \
  }

unsigned char *AddKeyFramesType(
    unsigned char                                          *fileData,
    unsigned int                                            fileBytes,
    unsigned long                                           tag,
    CAnimData                                              *shared,
    CKeyFrameTrack<NTempest::C3Vector, NTempest::C3Vector> *track,
    MDLTRACKTYPE                                            forceType
) {
  ADD_KEY_FRAMES_TYPE(NTempest::C3Vector, fileBytes, track, tag, fileData);
  return fileData;
}

unsigned char *AddKeyFramesType(
    unsigned char                *fileData,
    unsigned int                  fileBytes,
    unsigned long                 tag,
    CAnimData                    *shared,
    CKeyFrameTrack<float, float> *track,
    MDLTRACKTYPE                  forceType
) {
  ADD_KEY_FRAMES_TYPE(float, fileBytes, track, tag, fileData);
  return fileData;
}

unsigned char *AnimObjectSetEventTrack(unsigned char *data, unsigned int bytesLeft, CAnimData *shared, CAnimEventObj *objptr) {
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

unsigned char *
AnimObjectSetTranslation(unsigned char *data, unsigned int fileBytes, CAnimData *shared, CAnimObj *objptr, MDLTRACKTYPE forceType) {
  ADD_KEY_FRAMES_TYPE(NTempest::C3Vector, fileBytes, &objptr->translation, 0x5254474B, data);
  return data;
}

unsigned char *
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
  objptr->rotation.SetTrackType(trackType);
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

unsigned char *
AnimObjectSetScaling(unsigned char *fileData, unsigned int fileBytes, CAnimData *shared, CAnimObj *objptr, MDLTRACKTYPE forceType) {
  ADD_KEY_FRAMES_TYPE(NTempest::C3Vector, fileBytes, &objptr->scale, 0x4353474B, fileData);
  return fileData;
}

unsigned char *
AnimObjectSetAttenuation(unsigned char *data, unsigned int fileBytes, CAnimData *shared, CAnimLightObj *objptr, MDLTRACKTYPE forceType) {
  unsigned char *fileEnd = data + fileBytes;
  {
    ADD_KEY_FRAMES_TYPE(float, fileEnd - data, &objptr->attenstart, 0x53414C4B, data);
  }
  {
    ADD_KEY_FRAMES_TYPE(float, fileEnd - data, &objptr->attenend, 0x45414C4B, data);
  }
  return data;
}

unsigned char *
AnimObjectSetColor(unsigned char *data, unsigned int fileBytes, CAnimData *shared, CAnimLightObj *objptr, MDLTRACKTYPE forceType) {
  ADD_KEY_FRAMES_TYPE(C3Color, fileBytes, &objptr->color, 0x43414C4B, data);
  return data;
}

unsigned char *
AnimObjectSetIntensity(unsigned char *data, unsigned int fileBytes, CAnimData *shared, CAnimLightObj *objptr, MDLTRACKTYPE forceType) {
  ADD_KEY_FRAMES_TYPE(float, fileBytes, &objptr->intensity, 0x49414C4B, data);
  return data;
}

unsigned char *
AnimObjectSetAmbColor(unsigned char *data, unsigned int fileBytes, CAnimData *shared, CAnimLightObj *objptr, MDLTRACKTYPE forceType) {
  ADD_KEY_FRAMES_TYPE(C3Color, fileBytes, &objptr->ambColor, 0x43424C4B, data);
  return data;
}

unsigned char *
AnimObjectSetAmbIntensity(unsigned char *data, unsigned int fileBytes, CAnimData *shared, CAnimLightObj *objptr, MDLTRACKTYPE forceType) {
  ADD_KEY_FRAMES_TYPE(float, fileBytes, &objptr->ambIntensity, 0x49424C4B, data);
  return data;
}

unsigned char *
AnimObjectSetVisibilityTrack(unsigned char *data, unsigned int fileBytes, CAnimData *shared, CAnimVisibleObj *objptr, MDLTRACKTYPE forceType) {
  ADD_KEY_FRAMES_TYPE(float, fileBytes, &objptr->visibility, 0x5349564B, data);
  return data;
}

#define ANIM_FLOAT_TRACK_SETTER(name, member, tag)                                                                                                   \
  unsigned char *name(unsigned char *data, unsigned int fileBytes, CAnimData *shared, CAnimEmitter2Obj *objptr, MDLTRACKTYPE forceType) { \
    ADD_KEY_FRAMES_TYPE(float, fileBytes, &objptr->member, tag, data);                                                                               \
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
  unsigned char *name(unsigned char *data, unsigned int fileBytes, CAnimData *shared, CAnimRibbonObj *objptr, MDLTRACKTYPE forceType) { \
    ADD_KEY_FRAMES_TYPE(float, fileBytes, &objptr->member, tag, data);                                                                             \
    return data;                                                                                                                                   \
  }

ANIM_RIBBON_TRACK_SETTER(AnimObjectSetRibbonHeightAbove, heightAbove, 0x4148524B)
ANIM_RIBBON_TRACK_SETTER(AnimObjectSetRibbonHeightBelow, heightBelow, 0x4248524B)

unsigned char *AnimObjectSetRibbonSlot(unsigned char *data, unsigned int fileBytes, CAnimData *shared, CAnimRibbonObj *objptr) {
  ASSERT(shared);
  ASSERT(objptr);
  if (fileBytes < 8 || *reinterpret_cast<unsigned int *>(data) != 0x5854524B) {
    return data;
  }

  unsigned int numKeys = *reinterpret_cast<unsigned int *>(data + 4);
  ASSERT(numKeys);
  objptr->slot.m_globalSeqId = *reinterpret_cast<unsigned int *>(data + 12);
  objptr->slot.SetTrackType(KEYTYPE_NOINTERP);
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

unsigned char *
AnimObjectSetRibbonColor(unsigned char *data, unsigned int fileBytes, CAnimData *shared, CAnimRibbonObj *currobj, MDLTRACKTYPE forceType) {
  ADD_KEY_FRAMES_TYPE(C3Color, fileBytes, &currobj->color, 0x4F43524B, data);
  return data;
}

ANIM_RIBBON_TRACK_SETTER(AnimObjectSetRibbonAlpha, alpha, 0x4C41524B)

#undef ANIM_RIBBON_TRACK_SETTER

CAnim *AnimCreate(unsigned int *const objectCounts, unsigned int numGeosets, unsigned int numCameras, unsigned int numMaterialLayers) {
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
  shared->obj.SetCount(numObjects);
  shared->obj.Zero();
  shared->geo.ReserveSpace(numGeosets);
  shared->geoIdToGeoAnimId.ReserveSpace(numGeosets);
  shared->cameraObjs.ReserveSpace(numCameras);
  shared->layers.ReserveSpace(numMaterialLayers);
  shared->objectOrder.ReserveSpace(numObjects);
  shared->objectOrder.SetCount(numObjects);
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
  unique->baseStatus.SetCount(objectCounts[0]);
  unique->boneStatus.ReserveSpace(objectCounts[3]);
  unique->boneStatus.SetCount(objectCounts[3]);
  unique->lightStatus.ReserveSpace(objectCounts[1]);
  unique->lightStatus.SetCount(objectCounts[1]);
  unique->modelStatus.ReserveSpace(objectCounts[2]);
  unique->modelStatus.SetCount(objectCounts[2]);
  unique->emitter2Status.ReserveSpace(objectCounts[4]);
  unique->emitter2Status.SetCount(objectCounts[4]);
  unique->ribbonStatus.ReserveSpace(objectCounts[5]);
  unique->ribbonStatus.SetCount(objectCounts[5]);
  unique->eventStatus.ReserveSpace(objectCounts[6]);
  unique->eventStatus.SetCount(objectCounts[6]);
  unique->status.ReserveSpace(numObjects);
  unique->status.SetCount(numObjects);
  unique->geosetStatus.ReserveSpace(numGeosets);
  unique->geosetStatus.SetCount(numGeosets);
  unique->cameraStatus.ReserveSpace(numCameras);
  unique->cameraStatus.SetCount(numCameras);
  unique->layerStatus.ReserveSpace(numMaterialLayers);
  unique->layerStatus.SetCount(numMaterialLayers);
  unique->hdata = static_cast<HANIMDATA>(HandleCreate(shared, "HANIMDATA"));
  return unique;
}

HANIM AnimDuplicate(HANIM oldanim, unsigned int flags) {
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

void AnimInit(CAnim *unique, CAnimData *shared) {
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

void AnimAddMaterialLayer(CAnimData* shared, const MDLTEXLAYER& layerData, unsigned int layerId, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  if (!layerData.alphaKeys.keys.Count() && !layerData.flipKeys.keys.Count()) {
    return;
  }

  CAnimMaterialLayer &layer = *shared->layers.New();
  layer.layerId = layerId;
  AnimObjectSetVisibilityTrack(shared, &layer, layerData.alphaKeys, forceType);

  ASSERT(shared);
  ASSERT(&layer.flip);
  unsigned int numKeys = layerData.flipKeys.keys.Count();
  if (numKeys && layerData.coordId) {
    layer.flip.SetGlobalSequenceId(layerData.flipKeys.globalSeqId);
    layer.flip.SetTrackType(KEYTYPE_NOINTERP);
    layer.flip.SetNumKeys(numKeys);
    int timeAdjustment = layerData.flipKeys.globalSeqId == static_cast<unsigned int>(-1) ? 0 : layerData.flipKeys.keys[0].time;
    for (unsigned int i = 0; i < numKeys; ++i) {
      layer.flip.AddKey(
          layerData.flipKeys.keys[i].time - timeAdjustment,
          layerData.flipKeys.keys[i].value
      );
    }
    layer.flip.SetSequenceIndices(shared->seq);
  }
}

void AnimAddMaterialLayers(unsigned char *fileData, unsigned int fileBytes, CAnim *unique, CAnimData *shared, MDLTRACKTYPE forceType) {
  unsigned char *section = MDLFileBinarySeek(fileData, fileBytes, 0x534C544D);
  if (!section) {
    return;
  }

  unsigned char *dataDone = section + 4 + *reinterpret_cast<unsigned int *>(section);
  unsigned int   numMaterials = *reinterpret_cast<unsigned int *>(section + 4);
  unsigned int   numLayers = *reinterpret_cast<unsigned int *>(section + 8);
  unsigned char *data = section + 12;

  unique->layerStatus.ReserveSpace(numLayers);
  unique->layerStatus.SetCount(numLayers);
  shared->layers.ReserveSpace(numLayers);
  shared->layers.Clear();

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
        CAnimMaterialLayer &layer = *shared->layers.New();
        layer.layerId = layerId;
        data = AddKeyFramesType(data, fileData + fileBytes - data, 0x41544D4B, shared, &layer.visibility, forceType);

        if (fileData + fileBytes - data >= 8 && *reinterpret_cast<unsigned int *>(data) == 0x46544D4B) {
          unsigned int numKeys = *reinterpret_cast<unsigned int *>(data + 4);
          ASSERT(numKeys);
          layer.flip.m_globalSeqId = *reinterpret_cast<unsigned int *>(data + 12);
          layer.flip.SetTrackType(KEYTYPE_NOINTERP);
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

void AnimAddGeosets(unsigned char *fileData, unsigned int fileBytes, CAnimData *shared, MDLTRACKTYPE forceType) {
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
  shared->geo.SetCount(numGeosetAnims);
  shared->geoIdToGeoAnimId.ReserveSpace(numGeosets);
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
      ADD_KEY_FRAMES_TYPE(C3Color, static_cast<unsigned int>(fileData + fileBytes - data), &shared->geo[i].color, 0x4341474B, data);
    }
    ASSERT(data == geosetDone);
  }
  ASSERT(data == dataDone);
}

void AnimAddGeoset(CAnimData* shared, const MDLGEOSETANIMSECTION& geodata, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  unsigned int geoAnimId = shared->geo.Count();
  CAnimGeoset &geo = *shared->geo.New();
  unsigned int oldCount = shared->geoIdToGeoAnimId.Count();
  if (geodata.geosetId >= oldCount) {
    shared->geoIdToGeoAnimId.SetCount(geodata.geosetId + 1);
    memset(shared->geoIdToGeoAnimId.Ptr() + oldCount, 0xFF, (geodata.geosetId + 1 - oldCount) * sizeof(unsigned int));
  }
  shared->geoIdToGeoAnimId[geodata.geosetId] = geoAnimId;
  geo.sgGeosetId = geodata.geosetId;
  AnimObjectSetVisibilityTrack(shared, &geo, geodata.alphaKeys, forceType);
  AddKeyFrames(shared, geodata.colorKeys, &geo.color, forceType);
}

void AnimAddCameras(unsigned char *fileData, unsigned int fileBytes, CAnimData *shared, MDLTRACKTYPE forceType) {
  unsigned char *section = MDLFileBinarySeek(fileData, fileBytes, 0x534D4143);
  if (!section) {
    return;
  }

  unsigned int   numCameras = *reinterpret_cast<unsigned int *>(section + 4);
  unsigned char *dataDone = section + *reinterpret_cast<unsigned int *>(section) + 4;
  unsigned char *data = section + 8;
  shared->cameraObjs.ReserveSpace(numCameras);
  shared->cameraObjs.SetCount(numCameras);

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

void AnimAddCamera(CAnimData* shared, const MDLCAMERASECTION& cameraData, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  CAnimCameraObj &camera = *shared->cameraObjs.New();
  SStrCopy(camera.name, cameraData.name, sizeof(camera.name));
  camera.pivot = cameraData.pivot;
  AddKeyFrames(shared, cameraData.transkeys, &camera.translation, forceType);

  const MDLKEYTRACK<float> &rollTrack = cameraData.rollkeys;
  ASSERT(shared);
  ASSERT(&camera.roll);
  unsigned int numKeys = rollTrack.keys.Count();
  if (numKeys) {
    camera.roll.SetGlobalSequenceId(rollTrack.globalSeqId);
    camera.roll.SetTrackType(GetTrackType(rollTrack.type, forceType));
    camera.roll.SetNumKeys(numKeys);
    int timeAdjustment = rollTrack.globalSeqId == static_cast<unsigned int>(-1) ? 0 : rollTrack.keys[0].time;
    for (unsigned int i = 0; i < numKeys; ++i) {
      if (camera.roll.GetTrackType() < KEYTYPE_HERMITE) {
        camera.roll.AddKey(rollTrack.keys[i].time - timeAdjustment, rollTrack.keys[i].value);
      } else {
        camera.roll.AddKey(
            rollTrack.keys[i].time - timeAdjustment,
            rollTrack.keys[i].value,
            rollTrack.keys[i].inTan,
            rollTrack.keys[i].outTan
        );
      }
    }
    camera.roll.SetSequenceIndices(shared->seq);
  }

  camera.targetPivot = cameraData.target.pivot;
  AddKeyFrames(shared, cameraData.target.transkeys, &camera.targetTranslation, forceType);
  AnimObjectSetVisibilityTrack(shared, &camera, cameraData.visibilityKeys, forceType);
}

void AnimAddSequences(unsigned char *fileData, unsigned int fileBytes, CAnim *unique, CAnimData *shared) {
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
  shared->seq.SetCount(numSequences);
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
  unique->seq.SetCount(numSequences);

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
  unique->globalSeqElapsed.SetCount(numGlobalSequences);
  if (numGlobalSequences) {
    unique->globalSeqElapsed.Zero();
  }
  shared->globalSeqLength.Set(numGlobalSequences, reinterpret_cast<unsigned int *>(globalData));
  ASSERT(globalData + numGlobalSequences * sizeof(unsigned int) == globalDataDone);
}

void AnimAddSequences(
    CAnim *unique,
    CAnimData *shared,
    const TSGrowableArray<MDLSEQUENCESSECTION> &sequences,
    const TSGrowableArray<MDLGLOBALSEQSECTION> &globalSeqs
) {
  ASSERT(unique);
  ASSERT(shared);
  ASSERT(sequences.Count() <= static_cast<unsigned char>(0xFF));
  ASSERT(globalSeqs.Count() <= static_cast<unsigned char>(0xFF));

  unsigned int numSequences = sequences.Count();
  shared->seq.ReserveSpace(numSequences);
  shared->seq.SetCount(numSequences);
  for (unsigned int i = 0; i < numSequences; ++i) {
    CAnimSequence &sequence = shared->seq[i];
    SStrCopy(
        sequence.name,
        sequences[i].name,
        sizeof(sequence.name)
    );
    sequence.time = sequences[i].time;
    sequence.moveSpeed = sequences[i].movespeed;
    sequence.flags = sequences[i].flags;
    float pickChance = sequences[i].frequency * 32767.0f;
    sequence.randPickChance =
        pickChance <= 0.0f ? -static_cast<int>(-pickChance + 0.5f) : static_cast<unsigned int>(pickChance + 0.5f);
    sequence.replay = sequences[i].replay;
    sequence.bounds = sequences[i].bounds;
    sequence.blendTime = sequences[i].blendTime;
  }

  unique->seq.ReserveSpace(numSequences);
  unique->seq.SetCount(numSequences);
  unique->seqLastTime = IAnimGetCurrTimeMs();
  unique->globalSeqElapsed.ReserveSpace(globalSeqs.Count());
  unique->globalSeqElapsed.SetCount(globalSeqs.Count());
  if (globalSeqs.Count()) {
    unique->globalSeqElapsed.Zero();
  }
  shared->globalSeqLength.ReserveSpace(globalSeqs.Count());
  shared->globalSeqLength.SetCount(globalSeqs.Count());
  for (unsigned int global = 0; global < globalSeqs.Count(); ++global) {
    shared->globalSeqLength[global] = globalSeqs[global].length;
  }
}

void AnimAddTextureAnims(unsigned char *fileData, unsigned int fileBytes, CAnim *unique, CAnimData *shared, MDLTRACKTYPE forceType) {
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
  shared->tex.SetCount(numTexAnims);

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
      transform.rotation.SetTrackType(trackType);
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
  unique->textureStatus.SetCount(numTexAnims);
}

void AnimAddTextureAnim(
    CAnim *unique,
    CAnimData *shared,
    const TSGrowableArray<MDLTEXANIMSECTION> &textureAnims,
    MDLTRACKTYPE forceType
) {
  ASSERT(unique);
  ASSERT(shared);

  shared->tex.ReserveSpace(textureAnims.Count());
  shared->tex.SetCount(textureAnims.Count());
  for (unsigned int i = 0; i < textureAnims.Count(); ++i) {
    CAnimTransform &transform = shared->tex[i];
    AddKeyFrames(shared, textureAnims[i].transkeys, &transform.translation, forceType);
    const MDLKEYTRACK<NTempest::C4Quaternion> &rotation = textureAnims[i].rotkeys;
    ASSERT(shared);
    ASSERT(&transform.rotation);
    unsigned int numKeys = rotation.keys.Count();
    if (numKeys) {
      transform.rotation.SetGlobalSequenceId(rotation.globalSeqId);
      transform.rotation.SetTrackType(GetTrackType(rotation.type, forceType));
      transform.rotation.SetNumKeys(numKeys);
      int timeAdjustment = rotation.globalSeqId == static_cast<unsigned int>(-1) ? 0 : rotation.keys[0].time;
      for (unsigned int key = 0; key < numKeys; ++key) {
        if (transform.rotation.GetTrackType() < KEYTYPE_HERMITE) {
          transform.rotation.AddKey(rotation.keys[key].time - timeAdjustment, rotation.keys[key].value);
        } else {
          transform.rotation.AddKey(
              rotation.keys[key].time - timeAdjustment,
              rotation.keys[key].value,
              rotation.keys[key].inTan,
              rotation.keys[key].outTan
          );
        }
      }
      transform.rotation.SetSequenceIndices(shared->seq);
    }
    AddKeyFrames(shared, textureAnims[i].scalekeys, &transform.scale, forceType);
  }

  unique->textureStatus.ReserveSpace(textureAnims.Count());
  unique->textureStatus.SetCount(textureAnims.Count());
}

#undef ADD_KEY_FRAMES_TYPE
