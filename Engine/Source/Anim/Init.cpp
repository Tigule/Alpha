#include <Base/Base.h>

#include "MDLFile/MDLTypes.h"
#include "Anim/AnimInternal.h"

BYTE *MDLFileBinarySeek(BYTE *fileData, UINT fileBytes, DWORD sectionTag);

static UINT SetTransformationFlags(CAnimObj *currobj) {
  UINT flags = 0;
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

static void ValidateInheritanceFlags(CAnimData *animptr, CAnimObj *currobj, UINT parentFlags) {
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

  CAnimObj **child = currobj->childarray.Ptr();
  for (UINT i = currobj->childarray.Count(); i; --i, ++child) {
    ValidateInheritanceFlags(animptr, *child, parentFlags);
  }
}

static void ResolveStatusPtrs(CAnim *unique, CAnimData *shared) {
  UINT numObjects = shared->obj.Count();
  for (UINT i = 0; i < numObjects; ++i) {
    ASSERT(shared->obj[i]);
    switch (shared->obj[i]->type) {
      case OBJ_TYPE_HELPER:
        unique->status[i] = &unique->baseStatus[shared->obj[i]->splitIndex];
        break;
      case OBJ_TYPE_LIGHT:
        unique->status[i] = &unique->lightStatus[shared->obj[i]->splitIndex];
        break;
      case OBJ_TYPE_MODEL:
        unique->status[i] = &unique->modelStatus[shared->obj[i]->splitIndex];
        break;
      case OBJ_TYPE_EMITTER2:
        unique->status[i] = &unique->emitter2Status[shared->obj[i]->splitIndex];
        break;
      case OBJ_TYPE_RIBBON:
        unique->status[i] = &unique->ribbonStatus[shared->obj[i]->splitIndex];
        break;
      case OBJ_TYPE_EVENT:
        unique->status[i] = &unique->eventStatus[shared->obj[i]->splitIndex];
        break;
      case OBJ_TYPE_BONE:
        unique->status[i] = &unique->boneStatus[shared->obj[i]->splitIndex];
        break;
      default:
        ASSERT(0);
        break;
    }
  }
}

CAnimObj *AnimObjectCreateHelper(CAnimData *shared) {
  ASSERT(shared);
  UINT      index = shared->baseObjs.Count();
  CAnimObj *newobj = shared->baseObjs.New();
  ASSERT(newobj);
  newobj->splitIndex = index;
  return newobj;
}

CAnimLightObj *AnimObjectCreateLight(CAnimData *shared) {
  ASSERT(shared);
  UINT           index = shared->lightObjs.Count();
  CAnimLightObj *newobj = shared->lightObjs.New();
  ASSERT(newobj);
  newobj->splitIndex = index;
  return newobj;
}

CAnimModelObj *AnimObjectCreateAttachment(CAnimData *shared) {
  ASSERT(shared);
  UINT           index = shared->modelObjs.Count();
  CAnimModelObj *newobj = shared->modelObjs.New();
  ASSERT(newobj);
  newobj->splitIndex = index;
  return newobj;
}

CAnimBoneObj *AnimObjectCreateBone(CAnimData *shared) {
  ASSERT(shared);
  UINT          index = shared->boneObjs.Count();
  CAnimBoneObj *newobj = shared->boneObjs.New();
  ASSERT(newobj);
  newobj->splitIndex = index;
  return newobj;
}

CAnimEmitter2Obj *AnimObjectCreateEmitter2(CAnimData *shared) {
  ASSERT(shared);
  UINT              index = shared->emitter2Objs.Count();
  CAnimEmitter2Obj *newobj = shared->emitter2Objs.New();
  ASSERT(newobj);
  newobj->splitIndex = index;
  return newobj;
}

CAnimRibbonObj *AnimObjectCreateRibbon(CAnimData *shared) {
  ASSERT(shared);
  UINT            index = shared->ribbonObjs.Count();
  CAnimRibbonObj *newobj = shared->ribbonObjs.New();
  ASSERT(newobj);
  newobj->splitIndex = index;
  return newobj;
}

CAnimEventObj *AnimObjectCreateEvent(CAnimData *shared) {
  ASSERT(shared);
  UINT           index = shared->eventObjs.Count();
  CAnimEventObj *newobj = shared->eventObjs.New();
  ASSERT(newobj);
  newobj->splitIndex = index;
  return newobj;
}

void AnimObjectSetIndex(CAnimData *shared, CAnimObj *objptr, UINT index) {
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

CAnimObj *GetNodeByIndex(CAnimData *shared, UINT nodeIndex) {
  ASSERT(shared);
  ASSERT(nodeIndex < shared->obj.Count());
  return shared->obj[nodeIndex];
}

BOOL AnimObjectSetParent(CAnimData *shared, CAnimObj *objptr, UINT parentIndex) {
  ASSERT(shared);
  ASSERT(objptr);
  if (parentIndex == static_cast<UINT>(-1)) {
    *shared->headarray.New() = objptr;
    return 1;
  }

  CAnimObj *parent = GetNodeByIndex(shared, parentIndex);
  if (!parent) {
    return 0;
  }
  parent->childarray.New(objptr);
  return 1;
}

static KEYTYPE GetTrackType(UINT mdlTrackType, MDLTRACKTYPE forceType) {
  ASSERT(mdlTrackType < NUM_TRACK_TYPES);
  if (forceType != NUM_TRACK_TYPES) {
    switch (forceType) {
      case TRACK_NO_INTERP:
        mdlTrackType = TRACK_NO_INTERP;
        break;
      case TRACK_LINEAR:
        if (mdlTrackType > TRACK_LINEAR) {
          mdlTrackType = TRACK_LINEAR;
        }
        break;
    }
  }

  switch (mdlTrackType) {
    case TRACK_NO_INTERP:
      return KEYTYPE_NOINTERP;
    case TRACK_LINEAR:
      return KEYTYPE_LINEAR;
    case TRACK_BEZIER:
      return KEYTYPE_BEZIER;
    case TRACK_HERMITE:
    default:
      return KEYTYPE_HERMITE;
  }
}

template <class T, class U>
inline void AddKeyFrames(CAnimData *shared, const MDLKEYTRACK<U> &keyTrack, CKeyFrameTrack<T, U> *interp, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  ASSERT(interp);

  UINT numKeys = keyTrack.keys.Count();
  if (!numKeys) {
    return;
  }

  const MDLKEYFRAME<U> *keys = keyTrack.keys.Ptr();
  interp->SetGlobalSequenceId(keyTrack.globalSeqId);
  int     timeAdjustment = interp->SequenceNeverChanges() ? keys[0].time : 0;
  KEYTYPE trackType = GetTrackType(keyTrack.type, forceType);
  interp->SetTrackType(trackType);
  interp->SetNumKeys(numKeys);

  if (trackType >= KEYTYPE_HERMITE) {
    for (; numKeys; --numKeys, ++keys) {
      interp->AddKey(keys->time - timeAdjustment, keys->value, keys->inTan, keys->outTan);
    }
  } else {
    for (; numKeys; --numKeys, ++keys) {
      interp->AddKey(keys->time - timeAdjustment, keys->value);
    }
  }

  interp->SetSequenceIndices(shared->seq);
}

inline void AddKeyFrames(CAnimData *shared, const MDLSIMPLEKEYTRACK<MDLINTKEY> &keyTrack, CKeyFrameTrack<UINT, UINT> *interp) {
  ASSERT(shared);
  ASSERT(interp);

  if (keyTrack.keys.Count() && shared->seq.Count()) {
    const MDLINTKEY *keys = keyTrack.keys.Ptr();
    interp->SetGlobalSequenceId(keyTrack.globalSeqId);
    int timeAdjustment = interp->SequenceNeverChanges() ? keys->time : 0;
    interp->SetTrackType(KEYTYPE_NOINTERP);
    interp->SetNumKeys(keyTrack.keys.Count());

    for (UINT i = keyTrack.keys.Count(); i; --i, ++keys) {
      interp->AddKey(keys->time - timeAdjustment, keys->value);
    }

    interp->SetSequenceIndices(shared->seq);
  }
}

void AnimObjectSetVisibilityTrack(CAnimData *shared, CAnimVisibleObj *objptr, const MDLKEYTRACK<float> &keyTrack, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  ASSERT(objptr);
  AddKeyFrames(shared, keyTrack, &objptr->visibility, forceType);
}

void AnimObjectSetTranslation(CAnimData *shared, CAnimObj *objptr, const MDLKEYTRACK<NTempest::C3Vector> &keyTrack, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  ASSERT(objptr);
  AddKeyFrames(shared, keyTrack, &objptr->translation, forceType);
}

void AnimObjectSetRotation(CAnimData *shared, CAnimObj *objptr, const MDLKEYTRACK<NTempest::C4Quaternion> &keyTrack, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  ASSERT(objptr);
  AddKeyFrames(shared, keyTrack, &objptr->rotation, forceType);
}

void AnimObjectSetScaling(CAnimData *shared, CAnimObj *objptr, const MDLKEYTRACK<NTempest::C3Vector> &keyTrack, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  ASSERT(objptr);
  AddKeyFrames(shared, keyTrack, &objptr->scale, forceType);
}

void AnimObjectSetAttenuation(
    CAnimData                *shared,
    CAnimLightObj            *objptr,
    const MDLKEYTRACK<float> &startTrack,
    const MDLKEYTRACK<float> &endTrack,
    MDLTRACKTYPE              forceType
) {
  ASSERT(shared);
  ASSERT(objptr);
  AddKeyFrames(shared, startTrack, &objptr->attenstart, forceType);
  AddKeyFrames(shared, endTrack, &objptr->attenend, forceType);
}

void AnimObjectSetColor(CAnimData *shared, CAnimLightObj *objptr, const MDLKEYTRACK<C3Color> &keyTrack, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  ASSERT(objptr);
  AddKeyFrames(shared, keyTrack, &objptr->color, forceType);
}

void AnimObjectSetIntensity(CAnimData *shared, CAnimLightObj *objptr, const MDLKEYTRACK<float> &keyTrack, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  ASSERT(objptr);
  AddKeyFrames(shared, keyTrack, &objptr->intensity, forceType);
}

void AnimObjectSetAmbColor(CAnimData *shared, CAnimLightObj *objptr, const MDLKEYTRACK<C3Color> &keyTrack, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  ASSERT(objptr);
  AddKeyFrames(shared, keyTrack, &objptr->ambColor, forceType);
}

void AnimObjectSetAmbIntensity(CAnimData *shared, CAnimLightObj *objptr, const MDLKEYTRACK<float> &keyTrack, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  ASSERT(objptr);
  AddKeyFrames(shared, keyTrack, &objptr->ambIntensity, forceType);
}

void AnimObjectSetParticleEmissionRate2(CAnimData *shared, CAnimEmitter2Obj *currobj, const MDLKEYTRACK<float> &keyTrack, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  ASSERT(currobj);
  AddKeyFrames(shared, keyTrack, &currobj->emissionRate, forceType);
}

void AnimObjectSetParticleGravity2(CAnimData *shared, CAnimEmitter2Obj *currobj, const MDLKEYTRACK<float> &keyTrack, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  ASSERT(currobj);
  AddKeyFrames(shared, keyTrack, &currobj->gravity, forceType);
}

void AnimObjectSetParticleVariation2(CAnimData *shared, CAnimEmitter2Obj *currobj, const MDLKEYTRACK<float> &keyTrack, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  ASSERT(currobj);
  AddKeyFrames(shared, keyTrack, &currobj->variation, forceType);
}

void AnimObjectSetEmitterLongitude2(CAnimData *shared, CAnimEmitter2Obj *currobj, const MDLKEYTRACK<float> &keyTrack, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  ASSERT(currobj);
  AddKeyFrames(shared, keyTrack, &currobj->longitude, forceType);
}

void AnimObjectSetEmitterLatitude2(CAnimData *shared, CAnimEmitter2Obj *currobj, const MDLKEYTRACK<float> &keyTrack, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  ASSERT(currobj);
  AddKeyFrames(shared, keyTrack, &currobj->latitude, forceType);
}

void AnimObjectSetParticleSpeed2(CAnimData *shared, CAnimEmitter2Obj *currobj, const MDLKEYTRACK<float> &keyTrack, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  ASSERT(currobj);
  AddKeyFrames(shared, keyTrack, &currobj->particleSpeed, forceType);
}

void AnimObjectSetParticleLength2(CAnimData *shared, CAnimEmitter2Obj *currobj, const MDLKEYTRACK<float> &keyTrack, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  ASSERT(currobj);
  AddKeyFrames(shared, keyTrack, &currobj->length, forceType);
}

void AnimObjectSetParticleWidth2(CAnimData *shared, CAnimEmitter2Obj *currobj, const MDLKEYTRACK<float> &keyTrack, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  ASSERT(currobj);
  AddKeyFrames(shared, keyTrack, &currobj->width, forceType);
}

void AnimObjectSetParticleZsource2(CAnimData *shared, CAnimEmitter2Obj *currobj, const MDLKEYTRACK<float> &keyTrack, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  ASSERT(currobj);
  AddKeyFrames(shared, keyTrack, &currobj->zsource, forceType);
}

void AnimObjectSetParticleLifeSpan2(CAnimData *shared, CAnimEmitter2Obj *currobj, const MDLKEYTRACK<float> &keyTrack, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  ASSERT(currobj);
  AddKeyFrames(shared, keyTrack, &currobj->lifeSpan, forceType);
}

void AnimObjectSetRibbonHeightAbove(CAnimData *shared, CAnimRibbonObj *currobj, const MDLKEYTRACK<float> &keyTrack, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  ASSERT(currobj);
  AddKeyFrames(shared, keyTrack, &currobj->heightAbove, forceType);
}

void AnimObjectSetRibbonHeightBelow(CAnimData *shared, CAnimRibbonObj *currobj, const MDLKEYTRACK<float> &keyTrack, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  ASSERT(currobj);
  AddKeyFrames(shared, keyTrack, &currobj->heightBelow, forceType);
}

void AnimObjectSetRibbonAlpha(CAnimData *shared, CAnimRibbonObj *currobj, const MDLKEYTRACK<float> &keyTrack, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  ASSERT(currobj);
  AddKeyFrames(shared, keyTrack, &currobj->alpha, forceType);
}

void AnimObjectSetRibbonColor(CAnimData *shared, CAnimRibbonObj *currobj, const MDLKEYTRACK<C3Color> &keyTrack, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  ASSERT(currobj);
  AddKeyFrames(shared, keyTrack, &currobj->color, forceType);
}

void AnimObjectSetRibbonSlot(CAnimData *shared, CAnimRibbonObj *currobj, const MDLSIMPLEKEYTRACK<MDLINTKEY> &keyTrack) {
  ASSERT(shared);
  ASSERT(currobj);
  AddKeyFrames(shared, keyTrack, &currobj->slot);
}

void AnimObjectSetEventTrack(CAnimData *shared, CAnimEventObj *objptr, const MDLSIMPLEKEYTRACK<MDLEVENTKEY> &keyTrack) {
  ASSERT(shared);
  ASSERT(objptr);
  if (keyTrack.keys.Count() && shared->seq.Count()) {
    const MDLEVENTKEY *keys = keyTrack.keys.Ptr();
    objptr->events.SetGlobalSequenceId(keyTrack.globalSeqId);
    int                timeAdjustment = objptr->events.SequenceNeverChanges() ? keys->time : 0;
    objptr->events.SetNumKeys(keyTrack.keys.Count(), sizeof(CKeyFrame));
    for (UINT i = keyTrack.keys.Count(); i; --i, ++keys) {
      objptr->events.AddKey(keys->time - timeAdjustment);
    }
    objptr->events.SetSequenceIndices(shared->seq);
  }
}

#undef SET_MDL_FLOAT_TRACK

template <class T, class U>
inline BYTE *AddKeyFramesType(BYTE *data, UINT fileBytes, DWORD tag, CAnimData *shared, CKeyFrameTrack<T, U> *interp, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  ASSERT(interp);

  if (fileBytes < 8 || *reinterpret_cast<DWORD *>(data) != tag) {
    return data;
  }

  data += sizeof(DWORD);
  UINT numKeys = *reinterpret_cast<UINT *>(data);
  data += sizeof(UINT);
  ASSERT(numKeys > 0);

  KEYTYPE trackType = GetTrackType(*reinterpret_cast<UINT *>(data), forceType);
  data += sizeof(UINT);
  interp->SetGlobalSequenceId(*reinterpret_cast<UINT *>(data));
  data += sizeof(UINT);

  int timeAdjustment = interp->SequenceNeverChanges() ? *reinterpret_cast<int *>(data) : 0;
  interp->SetTrackType(trackType);
  interp->SetNumKeys(numKeys);

  UINT i;
  if (trackType >= KEYTYPE_HERMITE) {
    for (i = 0; i < numKeys; ++i) {
      int time = *reinterpret_cast<int *>(data);
      data += sizeof(int);
      const T *values = reinterpret_cast<const T *>(data);
      interp->AddKey(time - timeAdjustment, values[0], values[1], values[2]);
      data += 3 * sizeof(T);
    }
  } else {
    for (i = 0; i < numKeys; ++i) {
      int time = *reinterpret_cast<int *>(data);
      data += sizeof(int);
      interp->AddKey(time - timeAdjustment, *reinterpret_cast<const T *>(data));
      data += sizeof(T);
    }
  }

  interp->SetSequenceIndices(shared->seq);
  return data;
}

inline BYTE *AddKeyFramesType(BYTE *data, UINT fileBytes, DWORD tag, CAnimData *shared, CKeyFrameTrack<UINT, UINT> *interp) {
  ASSERT(shared);
  ASSERT(interp);

  if (fileBytes < 8 || *reinterpret_cast<DWORD *>(data) != tag) {
    return data;
  }

  data += sizeof(DWORD);
  UINT numKeys = *reinterpret_cast<UINT *>(data);
  data += sizeof(UINT);
  ASSERT(numKeys > 0);

  data += sizeof(UINT);
  interp->SetGlobalSequenceId(*reinterpret_cast<UINT *>(data));
  data += sizeof(UINT);

  int timeAdjustment = interp->SequenceNeverChanges() ? *reinterpret_cast<int *>(data) : 0;
  interp->SetTrackType(KEYTYPE_NOINTERP);
  interp->SetNumKeys(numKeys);

  for (UINT i = 0; i < numKeys; ++i) {
    int keyTime = *reinterpret_cast<int *>(data);
    data += sizeof(int);
    interp->AddKey(keyTime - timeAdjustment, *reinterpret_cast<UINT *>(data));
    data += sizeof(UINT);
  }

  interp->SetSequenceIndices(shared->seq);
  return data;
}

BYTE *AnimObjectSetEventTrack(BYTE *data, UINT bytesLeft, CAnimData *shared, CAnimEventObj *objptr) {
  ASSERT(shared);
  ASSERT(objptr);
  if (bytesLeft < 4 || *reinterpret_cast<DWORD *>(data) != 'TVEK') {
    return data;
  }

  BYTE *dataDone = data + bytesLeft;
  data += sizeof(DWORD);
  UINT numKeys = *reinterpret_cast<UINT *>(data);
  data += sizeof(UINT);
  objptr->events.SetGlobalSequenceId(*reinterpret_cast<UINT *>(data));
  data += sizeof(UINT);
  int timeAdjustment = objptr->events.SequenceNeverChanges() ? *reinterpret_cast<int *>(data) : 0;
  objptr->events.SetNumKeys(numKeys, sizeof(CKeyFrame));
  for (UINT i = 0; i < numKeys; ++i) {
    objptr->events.AddKey(*reinterpret_cast<int *>(data) - timeAdjustment);
    data += sizeof(int);
  }
  objptr->events.SetSequenceIndices(shared->seq);
  ASSERT(dataDone >= data);
  return data;
}

BYTE *AnimObjectSetTranslation(BYTE *data, UINT fileBytes, CAnimData *shared, CAnimObj *objptr, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  ASSERT(objptr);
  data = AddKeyFramesType(data, fileBytes, 'RTGK', shared, &objptr->translation, forceType);
  return data;
}

BYTE *AnimObjectSetRotation(BYTE *data, UINT fileBytes, CAnimData *shared, CAnimObj *objptr, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  ASSERT(objptr);
  data = AddKeyFramesType(data, fileBytes, 'TRGK', shared, &objptr->rotation, forceType);
  return data;
}

BYTE *AnimObjectSetScaling(BYTE *fileData, UINT fileBytes, CAnimData *shared, CAnimObj *objptr, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  ASSERT(objptr);
  fileData = AddKeyFramesType(fileData, fileBytes, 'CSGK', shared, &objptr->scale, forceType);
  return fileData;
}

BYTE *AnimObjectSetAttenuation(BYTE *data, UINT fileBytes, CAnimData *shared, CAnimLightObj *objptr, MDLTRACKTYPE forceType) {
  BYTE *fileEnd = data + fileBytes;
  {
    data = AddKeyFramesType(data, fileEnd - data, 'SALK', shared, &objptr->attenstart, forceType);
  }
  {
    data = AddKeyFramesType(data, fileEnd - data, 'EALK', shared, &objptr->attenend, forceType);
  }
  return data;
}

BYTE *AnimObjectSetColor(BYTE *data, UINT fileBytes, CAnimData *shared, CAnimLightObj *objptr, MDLTRACKTYPE forceType) {
  data = AddKeyFramesType(data, fileBytes, 'CALK', shared, &objptr->color, forceType);
  return data;
}

BYTE *AnimObjectSetIntensity(BYTE *data, UINT fileBytes, CAnimData *shared, CAnimLightObj *objptr, MDLTRACKTYPE forceType) {
  data = AddKeyFramesType(data, fileBytes, 'IALK', shared, &objptr->intensity, forceType);
  return data;
}

BYTE *AnimObjectSetAmbColor(BYTE *data, UINT fileBytes, CAnimData *shared, CAnimLightObj *objptr, MDLTRACKTYPE forceType) {
  data = AddKeyFramesType(data, fileBytes, 'CBLK', shared, &objptr->ambColor, forceType);
  return data;
}

BYTE *AnimObjectSetAmbIntensity(BYTE *data, UINT fileBytes, CAnimData *shared, CAnimLightObj *objptr, MDLTRACKTYPE forceType) {
  data = AddKeyFramesType(data, fileBytes, 'IBLK', shared, &objptr->ambIntensity, forceType);
  return data;
}

BYTE *AnimObjectSetVisibilityTrack(BYTE *data, UINT fileBytes, CAnimData *shared, CAnimVisibleObj *objptr, MDLTRACKTYPE forceType) {
  data = AddKeyFramesType(data, fileBytes, 'SIVK', shared, &objptr->visibility, forceType);
  return data;
}

#define ANIM_FLOAT_TRACK_SETTER(name, member, tag)                                                              \
  BYTE *name(BYTE *data, UINT fileBytes, CAnimData *shared, CAnimEmitter2Obj *objptr, MDLTRACKTYPE forceType) { \
    data = AddKeyFramesType(data, fileBytes, tag, shared, &objptr->member, forceType);                                          \
    return data;                                                                                                \
  }

ANIM_FLOAT_TRACK_SETTER(AnimObjectSetParticleEmissionRate2, emissionRate, 'E2PK')
ANIM_FLOAT_TRACK_SETTER(AnimObjectSetParticleGravity2, gravity, 'G2PK')
ANIM_FLOAT_TRACK_SETTER(AnimObjectSetParticleVariation2, variation, 'R2PK')
ANIM_FLOAT_TRACK_SETTER(AnimObjectSetEmitterLongitude2, longitude, 'NLPK')
ANIM_FLOAT_TRACK_SETTER(AnimObjectSetEmitterLatitude2, latitude, 'L2PK')
ANIM_FLOAT_TRACK_SETTER(AnimObjectSetParticleSpeed2, particleSpeed, 'S2PK')
ANIM_FLOAT_TRACK_SETTER(AnimObjectSetParticleLength2, length, 'N2PK')
ANIM_FLOAT_TRACK_SETTER(AnimObjectSetParticleWidth2, width, 'W2PK')
ANIM_FLOAT_TRACK_SETTER(AnimObjectSetParticleZsource2, zsource, 'Z2PK')
ANIM_FLOAT_TRACK_SETTER(AnimObjectSetParticleLifeSpan2, lifeSpan, 'FILK')

#undef ANIM_FLOAT_TRACK_SETTER

#define ANIM_RIBBON_TRACK_SETTER(name, member, tag)                                                           \
  BYTE *name(BYTE *data, UINT fileBytes, CAnimData *shared, CAnimRibbonObj *objptr, MDLTRACKTYPE forceType) { \
    data = AddKeyFramesType(data, fileBytes, tag, shared, &objptr->member, forceType);                                        \
    return data;                                                                                              \
  }

ANIM_RIBBON_TRACK_SETTER(AnimObjectSetRibbonHeightAbove, heightAbove, 'AHRK')
ANIM_RIBBON_TRACK_SETTER(AnimObjectSetRibbonHeightBelow, heightBelow, 'BHRK')

BYTE *AnimObjectSetRibbonSlot(BYTE *data, UINT fileBytes, CAnimData *shared, CAnimRibbonObj *objptr) {
  data = AddKeyFramesType(data, fileBytes, 'XTRK', shared, &objptr->slot);
  return data;
}

BYTE *AnimObjectSetRibbonColor(BYTE *data, UINT fileBytes, CAnimData *shared, CAnimRibbonObj *currobj, MDLTRACKTYPE forceType) {
  data = AddKeyFramesType(data, fileBytes, 'OCRK', shared, &currobj->color, forceType);
  return data;
}

ANIM_RIBBON_TRACK_SETTER(AnimObjectSetRibbonAlpha, alpha, 'LARK')

#undef ANIM_RIBBON_TRACK_SETTER

CAnim *AnimCreate(UINT *const objectCounts, UINT numGeosets, UINT numCameras, UINT numMaterialLayers) {
  LPVOID sharedMemory = SMemAlloc(sizeof(CAnimData), "HANIMDATA", SERR_LINECODE_OBJECT, 0);
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

  UINT numObjects = 0;
  for (UINT type = 0; type < 7; ++type) {
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
  for (UINT index = 0; index < numObjects; ++index) {
    shared->objectOrder[index] = index;
  }

  LPVOID uniqueMemory = SMemAlloc(sizeof(CAnim), "HANIM", SERR_LINECODE_OBJECT, 0);
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

HANIM AnimDuplicate(HANIM oldanim, UINT flags) {
  CAnim *oldUnique = reinterpret_cast<CAnim *>(oldanim);
  VALIDATEBEGIN;
  VALIDATE(oldUnique);
  VALIDATEEND;

  CAnim *unique = new (SMemAlloc(sizeof(CAnim), "HANIM", SERR_LINECODE_OBJECT, 0)) CAnim(0);
  if (!unique) {
    return 0;
  }

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

  CAnimObj **head = shared->headarray.Ptr();
  for (UINT i = shared->headarray.Count(); i; --i, ++head) {
    ValidateInheritanceFlags(shared, *head, 0);
  }
  if (!shared->Animates()) {
    shared->flags |= 4;
  }
  if (shared->Moves()) {
    shared->flags |= 2;
  }
  for (UINT geoset = 0; geoset < shared->geo.Count(); ++geoset) {
    if (shared->geo[geoset].visibility.TotalKeys()) {
      shared->flags |= 8;
    }
  }
  shared->flags |= 1;
}

void AnimAddMaterialLayer(CAnimData *shared, const MDLTEXLAYER &layerData, UINT layerId, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  if (layerData.alphaKeys.keys.Count() || layerData.flipKeys.keys.Count()) {
    CAnimMaterialLayer *layer = shared->layers.New();
    layer->layerId = layerId;
    AddKeyFrames(shared, layerData.alphaKeys, &layer->visibility, forceType);
    AddKeyFrames(shared, layerData.flipKeys, &layer->flip);
  }
}

void AnimAddMaterialLayers(BYTE *fileData, UINT fileBytes, CAnim *unique, CAnimData *shared, MDLTRACKTYPE forceType) {
  BYTE *section = MDLFileBinarySeek(fileData, fileBytes, 'SLTM');
  if (!section) {
    return;
  }

  BYTE *dataDone = section + 4 + *reinterpret_cast<UINT *>(section);
  UINT  numMaterials = *reinterpret_cast<UINT *>(section + 4);
  UINT  numLayers = *reinterpret_cast<UINT *>(section + 8);
  BYTE *data = section + 12;

  unique->layerStatus.ReserveSpace(numLayers);
  unique->layerStatus.SetCount(numLayers);
  shared->layers.ReserveSpace(numLayers);
  shared->layers.Clear();

  UINT layerId = 0;
  for (UINT material = 0; material < numMaterials; ++material) {
    BYTE *materialDone = data + *reinterpret_cast<UINT *>(data);
    UINT  materialLayers = *reinterpret_cast<UINT *>(data + 8);
    data += 12;
    ASSERT(materialLayers);

    for (UINT layerIndex = 0; layerIndex < materialLayers; ++layerIndex, ++layerId) {
      BYTE *layerDone = data + *reinterpret_cast<UINT *>(data);
      data += 28;
      if (data != layerDone) {
        CAnimMaterialLayer &layer = *shared->layers.New();
        layer.layerId = layerId;
        data = AddKeyFramesType(data, fileData + fileBytes - data, 'ATMK', shared, &layer.visibility, forceType);

        if (fileData + fileBytes - data >= 8 && *reinterpret_cast<UINT *>(data) == 'FTMK') {
          UINT numKeys = *reinterpret_cast<UINT *>(data + 4);
          ASSERT(numKeys);
          layer.flip.m_globalSeqId = *reinterpret_cast<UINT *>(data + 12);
          layer.flip.SetTrackType(KEYTYPE_NOINTERP);
          data += 16;
          int timeAdjustment = layer.flip.m_globalSeqId == static_cast<UINT>(-1) ? 0 : *reinterpret_cast<int *>(data);
          layer.flip.SetNumKeys(numKeys, sizeof(int) + sizeof(UINT));
          for (UINT key = 0; key < numKeys; ++key) {
            UINT  keyIndex = layer.flip.m_numKeyFrames++;
            BYTE *keyData = reinterpret_cast<BYTE *>(layer.flip.m_keyFrames) + keyIndex * layer.flip.m_keyFrameSize;
            *reinterpret_cast<int *>(keyData) = *reinterpret_cast<int *>(data) - timeAdjustment;
            *reinterpret_cast<UINT *>(keyData + sizeof(int)) = *reinterpret_cast<UINT *>(data + sizeof(int));
            data += sizeof(int) + sizeof(UINT);
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

void AnimAddGeosets(BYTE *fileData, UINT fileBytes, CAnimData *shared, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  BYTE *section = MDLFileBinarySeek(fileData, fileBytes, 'AOEG');
  if (!section) {
    return;
  }

  UINT  numGeosets = 0;
  BYTE *geosets = MDLFileBinarySeek(fileData, fileBytes, 'SOEG');
  if (geosets) {
    numGeosets = *reinterpret_cast<UINT *>(geosets + 4);
  }

  UINT  numGeosetAnims = *reinterpret_cast<UINT *>(section + 4);
  BYTE *dataDone = section + *reinterpret_cast<UINT *>(section) + 4;
  BYTE *data = section + 8;
  shared->geo.ReserveSpace(numGeosetAnims);
  shared->geo.SetCount(numGeosetAnims);
  shared->geoIdToGeoAnimId.ReserveSpace(numGeosets);
  shared->geoIdToGeoAnimId.SetCount(numGeosets);
  if (numGeosets) {
    memset(shared->geoIdToGeoAnimId.Ptr(), 0xFF, numGeosets * sizeof(UINT));
  }

  for (UINT i = 0; i < numGeosetAnims; ++i) {
    BYTE *geosetDone = data + *reinterpret_cast<UINT *>(data);
    UINT  geosetId = *reinterpret_cast<UINT *>(data + 4);
    ASSERT(geosetId < numGeosets);
    shared->geoIdToGeoAnimId[geosetId] = i;
    shared->geo[i].sgGeosetId = geosetId;
    data += 28;

    data = AddKeyFramesType(data, static_cast<UINT>(fileData + fileBytes - data), 'OAGK', shared, &shared->geo[i].visibility, forceType);
    {
      data = AddKeyFramesType(data, static_cast<UINT>(fileData + fileBytes - data), 'CAGK', shared, &shared->geo[i].color, forceType);
    }
    ASSERT(data == geosetDone);
  }
  ASSERT(data == dataDone);
}

void AnimAddGeoset(CAnimData *shared, const MDLGEOSETANIMSECTION &geodata, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  UINT         geoAnimId = shared->geo.Count();
  CAnimGeoset &geo = *shared->geo.New();
  UINT         oldCount = shared->geoIdToGeoAnimId.Count();
  if (geodata.geosetId >= oldCount) {
    shared->geoIdToGeoAnimId.SetCount(geodata.geosetId + 1);
    memset(shared->geoIdToGeoAnimId.Ptr() + oldCount, 0xFF, (geodata.geosetId + 1 - oldCount) * sizeof(UINT));
  }
  shared->geoIdToGeoAnimId[geodata.geosetId] = geoAnimId;
  geo.sgGeosetId = geodata.geosetId;
  AnimObjectSetVisibilityTrack(shared, &geo, geodata.alphaKeys, forceType);
  AddKeyFrames(shared, geodata.colorKeys, &geo.color, forceType);
}

void AnimAddCameras(BYTE *fileData, UINT fileBytes, CAnimData *shared, MDLTRACKTYPE forceType) {
  BYTE *section = MDLFileBinarySeek(fileData, fileBytes, 'SMAC');
  if (!section) {
    return;
  }

  UINT  numCameras = *reinterpret_cast<UINT *>(section + 4);
  BYTE *dataDone = section + *reinterpret_cast<UINT *>(section) + 4;
  BYTE *data = section + 8;
  shared->cameraObjs.ReserveSpace(numCameras);
  shared->cameraObjs.SetCount(numCameras);

  for (UINT i = 0; i < numCameras; ++i) {
    data += sizeof(UINT);
    SStrCopy(shared->cameraObjs[i].name, reinterpret_cast<LPCSTR>(data), sizeof(shared->cameraObjs[i].name));
    data += sizeof(shared->cameraObjs[i].name);
    shared->cameraObjs[i].pivot = *reinterpret_cast<NTempest::C3Vector *>(data);
    data += sizeof(NTempest::C3Vector) + 12;
    shared->cameraObjs[i].targetPivot = *reinterpret_cast<NTempest::C3Vector *>(data);
    data += sizeof(NTempest::C3Vector);

    data = AddKeyFramesType(data, static_cast<UINT>(fileData + fileBytes - data), 'RTCK', shared, &shared->cameraObjs[i].translation, forceType);
    data = AddKeyFramesType(data, static_cast<UINT>(fileData + fileBytes - data), 'LRCK', shared, &shared->cameraObjs[i].roll, forceType);
    data = AddKeyFramesType(
        data, static_cast<UINT>(fileData + fileBytes - data), 'RTTK', shared, &shared->cameraObjs[i].targetTranslation, forceType
    );
    data = AddKeyFramesType(data, static_cast<UINT>(fileData + fileBytes - data), 'SIVK', shared, &shared->cameraObjs[i].visibility, forceType);
  }
  ASSERT(data == dataDone);
}

void AnimAddCamera(CAnimData *shared, const MDLCAMERASECTION &cameraData, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  CAnimCameraObj *camera = shared->cameraObjs.New();
  AddKeyFrames(shared, cameraData.transkeys, &camera->translation, forceType);
  AddKeyFrames(shared, cameraData.rollkeys, &camera->roll, forceType);
  camera->pivot = cameraData.pivot;
  AddKeyFrames(shared, cameraData.target.transkeys, &camera->targetTranslation, forceType);
  camera->targetPivot = cameraData.target.pivot;
  AnimObjectSetVisibilityTrack(shared, camera, cameraData.visibilityKeys, forceType);
}

void AnimAddSequences(BYTE *fileData, UINT fileBytes, CAnim *unique, CAnimData *shared) {
  BYTE *sequenceSection = MDLFileBinarySeek(fileData, fileBytes, 'SQES');
  UINT  numSequences = 0;
  BYTE *data = 0;
  BYTE *seqDataDone = 0;
  if (sequenceSection) {
    seqDataDone = sequenceSection + 4 + *reinterpret_cast<UINT *>(sequenceSection);
    numSequences = *reinterpret_cast<UINT *>(sequenceSection + 4);
    ASSERT(numSequences);
    data = sequenceSection + 8;
  }

  shared->seq.ReserveSpace(numSequences);
  shared->seq.SetCount(numSequences);
  for (UINT i = 0; i < numSequences; ++i) {
    CAnimSequence &sequence = shared->seq[i];
    SStrCopy(reinterpret_cast<char *>(&sequence.name), reinterpret_cast<LPCSTR>(data), 80);
    data += 80;
    sequence.time.l = *reinterpret_cast<int *>(data);
    data += 4;
    sequence.time.h = *reinterpret_cast<int *>(data);
    data += 4;
    sequence.moveSpeed = *reinterpret_cast<float *>(data);
    data += 4;
    sequence.flags = *reinterpret_cast<UINT *>(data);
    data += 4;
    sequence.bounds.radius = *reinterpret_cast<float *>(data);
    data += 4;
    memcpy(&sequence.bounds.extent.b, data, sizeof(NTempest::C3Vector));
    data += sizeof(NTempest::C3Vector);
    memcpy(&sequence.bounds.extent.t, data, sizeof(NTempest::C3Vector));
    data += sizeof(NTempest::C3Vector);
    sequence.randPickChance = NTempest::CMath::fint_n(*reinterpret_cast<float *>(data) * 32767.0f);
    data += 4;
    sequence.replay.l = *reinterpret_cast<int *>(data);
    data += 4;
    sequence.replay.h = *reinterpret_cast<int *>(data);
    data += 4;
    UINT blendTime = *reinterpret_cast<UINT *>(data);
    data += 4;
    if (blendTime) {
      sequence.blendTime = blendTime;
    }
  }
  ASSERT(seqDataDone == data);

  unique->seq.ReserveSpace(numSequences);
  unique->seq.SetCount(numSequences);

  BYTE *globalSection = MDLFileBinarySeek(fileData, fileBytes, 'SBLG');
  UINT  numGlobalSequences = 0;
  BYTE *globalData = 0;
  BYTE *globalDataDone = 0;
  if (globalSection) {
    UINT globalBytes = *reinterpret_cast<UINT *>(globalSection);
    globalData = globalSection + 4;
    globalDataDone = globalData + globalBytes;
    numGlobalSequences = globalBytes / sizeof(UINT);
    ASSERT(numGlobalSequences * sizeof(UINT) == globalBytes);
  }

  unique->seqLastTime = IAnimGetCurrTimeMs();
  unique->globalSeqElapsed.ReserveSpace(numGlobalSequences);
  unique->globalSeqElapsed.SetCount(numGlobalSequences);
  if (numGlobalSequences) {
    unique->globalSeqElapsed.Zero();
  }
  shared->globalSeqLength.Set(numGlobalSequences, reinterpret_cast<UINT *>(globalData));
  ASSERT(globalData + numGlobalSequences * sizeof(UINT) == globalDataDone);
}

void AnimAddSequences(
    CAnim                                      *unique,
    CAnimData                                  *shared,
    const TSGrowableArray<MDLSEQUENCESSECTION> &sequences,
    const TSGrowableArray<MDLGLOBALSEQSECTION> &globalSeqs
) {
  ASSERT(unique);
  ASSERT(shared);
  ASSERT(sequences.Count() <= static_cast<BYTE>(0xFF));
  ASSERT(globalSeqs.Count() <= static_cast<BYTE>(0xFF));

  UINT numSequences = sequences.Count();
  shared->seq.ReserveSpace(numSequences);
  shared->seq.SetCount(numSequences);
  for (UINT i = 0; i < numSequences; ++i) {
    CAnimSequence &sequence = shared->seq[i];
    SStrCopy(sequence.name, sequences[i].name, sizeof(sequence.name));
    sequence.time = sequences[i].time;
    sequence.moveSpeed = sequences[i].movespeed;
    sequence.flags = sequences[i].flags;
    sequence.randPickChance = NTempest::CMath::fint_n(sequences[i].frequency * 32767.0f);
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
  for (UINT global = 0; global < globalSeqs.Count(); ++global) {
    shared->globalSeqLength[global] = globalSeqs[global].length;
  }
}

void AnimAddTextureAnims(BYTE *fileData, UINT fileBytes, CAnim *unique, CAnimData *shared, MDLTRACKTYPE forceType) {
  ASSERT(unique);
  ASSERT(shared);
  BYTE *section = MDLFileBinarySeek(fileData, fileBytes, 'NAXT');
  if (!section) {
    return;
  }

  BYTE *dataDone = section + 4 + *reinterpret_cast<UINT *>(section);
  UINT  numTexAnims = *reinterpret_cast<UINT *>(section + 4);
  BYTE *data = section + 8;
  shared->tex.ReserveSpace(numTexAnims);
  shared->tex.SetCount(numTexAnims);

  for (UINT i = 0; i < numTexAnims; ++i) {
    BYTE *animDone = data + *reinterpret_cast<UINT *>(data);
    data += 4;
    CAnimTransform &transform = shared->tex[i];
    data = AddKeyFramesType(data, fileData + fileBytes - data, 'TATK', shared, &transform.translation, forceType);

    if (fileData + fileBytes - data >= 8 && *reinterpret_cast<UINT *>(data) == 'RATK') {
      UINT numKeys = *reinterpret_cast<UINT *>(data + 4);
      ASSERT(numKeys);
      KEYTYPE trackType = GetTrackType(*reinterpret_cast<MDLTRACKTYPE *>(data + 8), forceType);
      transform.rotation.m_globalSeqId = *reinterpret_cast<UINT *>(data + 12);
      transform.rotation.SetTrackType(trackType);
      data += 16;
      int  timeAdjustment = transform.rotation.m_globalSeqId == static_cast<UINT>(-1) ? 0 : *reinterpret_cast<int *>(data);
      UINT valueCount = trackType < TRACK_HERMITE ? 1 : 3;
      UINT keySize = valueCount == 1 ? 16 : 32;
      transform.rotation.SetNumKeys(numKeys, keySize);
      for (UINT key = 0; key < numKeys; ++key) {
        int time = *reinterpret_cast<int *>(data);
        data += sizeof(int);
        BYTE *keyData =
            reinterpret_cast<BYTE *>(transform.rotation.m_keyFrames) + transform.rotation.m_numKeyFrames++ * transform.rotation.m_keyFrameSize;
        memset(keyData, 0, keySize);
        *reinterpret_cast<int *>(keyData) = time - timeAdjustment;
        memcpy(keyData + 8, data, valueCount * sizeof(NTempest::C4QuaternionCompressed));
        data += valueCount * sizeof(NTempest::C4QuaternionCompressed);
      }
      transform.rotation.SetSequenceIndices(shared->seq);
    }

    data = AddKeyFramesType(data, fileData + fileBytes - data, 'SATK', shared, &transform.scale, forceType);
    ASSERT(animDone == data);
  }
  ASSERT(dataDone == data);

  unique->textureStatus.ReserveSpace(numTexAnims);
  unique->textureStatus.SetCount(numTexAnims);
}

void AnimAddTextureAnim(CAnim *unique, CAnimData *shared, const TSGrowableArray<MDLTEXANIMSECTION> &textureAnims, MDLTRACKTYPE forceType) {
  ASSERT(unique);
  ASSERT(shared);

  shared->tex.ReserveSpace(textureAnims.Count());
  shared->tex.SetCount(textureAnims.Count());
  for (UINT i = 0; i < textureAnims.Count(); ++i) {
    CAnimTransform &transform = shared->tex[i];
    AddKeyFrames(shared, textureAnims[i].transkeys, &transform.translation, forceType);
    const MDLKEYTRACK<NTempest::C4Quaternion> &rotation = textureAnims[i].rotkeys;
    ASSERT(shared);
    ASSERT(&transform.rotation);
    UINT numKeys = rotation.keys.Count();
    if (numKeys) {
      transform.rotation.SetGlobalSequenceId(rotation.globalSeqId);
      transform.rotation.SetTrackType(GetTrackType(rotation.type, forceType));
      transform.rotation.SetNumKeys(numKeys);
      int timeAdjustment = rotation.globalSeqId == static_cast<UINT>(-1) ? 0 : rotation.keys[0].time;
      for (UINT key = 0; key < numKeys; ++key) {
        if (transform.rotation.GetTrackType() < KEYTYPE_HERMITE) {
          transform.rotation.AddKey(rotation.keys[key].time - timeAdjustment, rotation.keys[key].value);
        } else {
          transform.rotation.AddKey(
              rotation.keys[key].time - timeAdjustment, rotation.keys[key].value, rotation.keys[key].inTan, rotation.keys[key].outTan
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

