#include <Base/Base.h>

#include "MDLFile/MDLTypes.h"
#include "Anim/AnimInternal.h"

typedef BYTE uint8;

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
  if (parentIndex == (UINT)-1) {
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

  if (fileBytes < 8 || *(DWORD *)data != tag) {
    return data;
  }

  data += sizeof(DWORD);
  UINT numKeys = *(UINT *)data;
  data += sizeof(UINT);
  ASSERT(numKeys > 0);

  KEYTYPE trackType = GetTrackType(*(UINT *)data, forceType);
  data += sizeof(UINT);
  interp->SetGlobalSequenceId(*(UINT *)data);
  data += sizeof(UINT);

  int timeAdjustment = interp->SequenceNeverChanges() ? *(int *)data : 0;
  interp->SetTrackType(trackType);
  interp->SetNumKeys(numKeys);

  UINT i;
  if (trackType >= KEYTYPE_HERMITE) {
    for (i = 0; i < numKeys; ++i) {
      int time = *(int *)data;
      data += sizeof(int);
      const T *values = (const T *)data;
      interp->AddKey(time - timeAdjustment, values[0], values[1], values[2]);
      data += 3 * sizeof(T);
    }
  } else {
    for (i = 0; i < numKeys; ++i) {
      int time = *(int *)data;
      data += sizeof(int);
      interp->AddKey(time - timeAdjustment, *(const T *)data);
      data += sizeof(T);
    }
  }

  interp->SetSequenceIndices(shared->seq);
  return data;
}

inline BYTE *AddKeyFramesType(BYTE *data, UINT fileBytes, DWORD tag, CAnimData *shared, CKeyFrameTrack<UINT, UINT> *interp) {
  ASSERT(shared);
  ASSERT(interp);

  if (fileBytes < 8 || *(DWORD *)data != tag) {
    return data;
  }

  data += sizeof(DWORD);
  UINT numKeys = *(UINT *)data;
  data += sizeof(UINT);
  ASSERT(numKeys > 0);

  data += sizeof(UINT);
  interp->SetGlobalSequenceId(*(UINT *)data);
  data += sizeof(UINT);

  int timeAdjustment = interp->SequenceNeverChanges() ? *(int *)data : 0;
  interp->SetTrackType(KEYTYPE_NOINTERP);
  interp->SetNumKeys(numKeys);

  for (UINT i = 0; i < numKeys; ++i) {
    int keyTime = *(int *)data;
    data += sizeof(int);
    interp->AddKey(keyTime - timeAdjustment, *(UINT *)data);
    data += sizeof(UINT);
  }

  interp->SetSequenceIndices(shared->seq);
  return data;
}

BYTE *AnimObjectSetEventTrack(BYTE *data, UINT bytesLeft, CAnimData *shared, CAnimEventObj *objptr) {
  ASSERT(shared);
  ASSERT(objptr);
  if (bytesLeft < 4 || *(DWORD *)data != 'TVEK') {
    return data;
  }

  BYTE *dataDone = data + bytesLeft;
  data += sizeof(DWORD);
  UINT numKeys = *(UINT *)data;
  data += sizeof(UINT);
  objptr->events.SetGlobalSequenceId(*(UINT *)data);
  data += sizeof(UINT);
  int timeAdjustment = objptr->events.SequenceNeverChanges() ? *(int *)data : 0;
  objptr->events.SetNumKeys(numKeys, sizeof(CKeyFrame));
  for (UINT i = 0; i < numKeys; ++i) {
    objptr->events.AddKey(*(int *)data - timeAdjustment);
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

BYTE *AnimObjectSetAttenuation(BYTE *fileData, UINT fileBytes, CAnimData *shared, CAnimLightObj *objptr, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  ASSERT(objptr);
  BYTE *data = AddKeyFramesType(fileData, fileBytes, 'SALK', shared, &objptr->attenstart, forceType);
  data = AddKeyFramesType(data, fileBytes - (data - fileData), 'EALK', shared, &objptr->attenend, forceType);
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
  CAnimData *shared = NEWHANDLE(HANIMDATA, CAnimData);
  if (!shared) {
    return 0;
  }

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
  shared->obj.SetCount(numObjects);
  shared->obj.Zero();
  shared->geo.ReserveSpace(numGeosets);
  shared->geoIdToGeoAnimId.ReserveSpace(numGeosets);
  shared->cameraObjs.ReserveSpace(numCameras);
  shared->layers.ReserveSpace(numMaterialLayers);
  shared->objectOrder.SetCount(numObjects);
  for (UINT index = 0; index < numObjects; ++index) {
    shared->objectOrder[index] = index;
  }

  CAnim *unique = NEWHANDLE(HANIM, CAnim)(0);
  if (!unique) {
    delete shared;
    return 0;
  }

  unique->baseStatus.SetCount(objectCounts[0]);
  unique->boneStatus.SetCount(objectCounts[3]);
  unique->lightStatus.SetCount(objectCounts[1]);
  unique->modelStatus.SetCount(objectCounts[2]);
  unique->emitter2Status.SetCount(objectCounts[4]);
  unique->ribbonStatus.SetCount(objectCounts[5]);
  unique->eventStatus.SetCount(objectCounts[6]);
  unique->status.SetCount(numObjects);
  unique->geosetStatus.SetCount(numGeosets);
  unique->cameraStatus.SetCount(numCameras);
  unique->layerStatus.SetCount(numMaterialLayers);
  unique->hdata = CREATEHANDLE(HANIMDATA, shared);
  return unique;
}

HANIM AnimDuplicate(HANIM oldanim, UINT flags) {
  CAnim *oldUnique = (CAnim *)oldanim;
  VALIDATEBEGIN;
  VALIDATE(oldUnique);
  VALIDATEEND;

  CAnim *unique = NEWHANDLE(HANIM, CAnim)(0);
  if (!unique) {
    return 0;
  }

  CAnimData *shared = (CAnimData *)oldUnique->hdata;
  *unique = *oldUnique;
  ResolveStatusPtrs(unique, shared);
  unique->hdata = CREATEHANDLE(HANIMDATA, shared);
  HANIM duplicate = CREATEHANDLE(HANIM, unique);
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
  BYTE *data = MDLFileBinarySeek(fileData, fileBytes, 'SLTM');
  if (!data) {
    return;
  }

  UINT sectionBytes = *(UINT *)data;
  data += sizeof(UINT);
  BYTE *dataDone = data + sectionBytes;
  UINT  numMaterials = *(UINT *)data;
  data += sizeof(UINT);
  UINT numLayers = *(UINT *)data;
  data += sizeof(UINT);

  unique->layerStatus.SetCount(numLayers);
  shared->layers.ReserveSpace(numLayers);

  UINT layerId = 0;
  for (UINT i = 0; i < numMaterials; ++i) {
    BYTE *materialDone = data + *(UINT *)data;
    data += 2 * sizeof(UINT);
    UINT numLayers = *(UINT *)data;
    data += sizeof(UINT);
    ASSERT(numLayers);

    for (UINT j = 0; j < numLayers; ++j) {
      BYTE *layerDone = data + *(UINT *)data;
      data += 28;
      if (data != layerDone) {
        CAnimMaterialLayer *layer = shared->layers.New();
        layer->layerId = layerId;
        data = AddKeyFramesType(data, fileBytes - (data - fileData), 'ATMK', shared, &layer->visibility, forceType);
        data = AddKeyFramesType(data, fileBytes - (data - fileData), 'FTMK', shared, &layer->flip);
        ASSERT(data == layerDone);
      }
      ++layerId;
    }
    ASSERT(data == materialDone);
  }
  ASSERT(data == dataDone);
}

void AnimAddGeosets(BYTE *fileData, UINT fileBytes, CAnimData *shared, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  BYTE *data = MDLFileBinarySeek(fileData, fileBytes, 'AOEG');
  if (!data) {
    return;
  }

  UINT  numGeosets = 0;
  BYTE *geosets = MDLFileBinarySeek(fileData, fileBytes, 'SOEG');
  if (geosets) {
    numGeosets = *(UINT *)(geosets + sizeof(UINT));
  }

  UINT sectionBytes = *(UINT *)data;
  data += sizeof(UINT);
  BYTE *dataDone = data + sectionBytes;
  UINT  numGeosetAnims = *(UINT *)data;
  data += sizeof(UINT);

  shared->geo.SetCount(numGeosetAnims);
  shared->geoIdToGeoAnimId.SetCount(numGeosets);
  memset(shared->geoIdToGeoAnimId.Ptr(), 0xFF, shared->geoIdToGeoAnimId.Bytes());

  for (UINT i = 0; i < numGeosetAnims; ++i) {
    BYTE *geosetDone = data + *(UINT *)data;
    data += sizeof(UINT);
    UINT geosetId = *(UINT *)data;
    data += sizeof(UINT);
    shared->geoIdToGeoAnimId[geosetId] = i;
    shared->geo[i].sgGeosetId = geosetId;
    data += 20;

    data = AddKeyFramesType(data, fileBytes - (data - fileData), 'OAGK', shared, &shared->geo[i].visibility, forceType);
    data = AddKeyFramesType(data, fileBytes - (data - fileData), 'CAGK', shared, &shared->geo[i].color, forceType);
    ASSERT(geosetDone == data);
  }
  ASSERT(dataDone == data);
}

void AnimAddGeoset(CAnimData *shared, const MDLGEOSETANIMSECTION &geodata, MDLTRACKTYPE forceType) {
  ASSERT(shared);
  UINT         geoAnimId = shared->geo.Count();
  CAnimGeoset &geo = *shared->geo.New();
  UINT         oldCount = shared->geoIdToGeoAnimId.Count();
  if (geodata.geosetId >= shared->geoIdToGeoAnimId.Count()) {
    shared->geoIdToGeoAnimId.SetCount(geodata.geosetId + 1);
  }
  if (oldCount < geodata.geosetId) {
    memset(&shared->geoIdToGeoAnimId[oldCount], 0xFF, (geodata.geosetId - oldCount) * sizeof(UINT));
  }
  shared->geoIdToGeoAnimId[geodata.geosetId] = geoAnimId;
  geo.sgGeosetId = geodata.geosetId;
  AnimObjectSetVisibilityTrack(shared, &geo, geodata.alphaKeys, forceType);
  AddKeyFrames(shared, geodata.colorKeys, &geo.color, forceType);
}

void AnimAddCameras(BYTE *fileData, UINT fileBytes, CAnimData *shared, MDLTRACKTYPE forceType) {
  BYTE *data = MDLFileBinarySeek(fileData, fileBytes, 'SMAC');
  if (!data) {
    return;
  }

  UINT sectionBytes = *(UINT *)data;
  data += sizeof(UINT);
  BYTE *dataDone = data + sectionBytes;
  UINT  numCameras = *(UINT *)data;
  data += sizeof(UINT);
  shared->cameraObjs.SetCount(numCameras);

  for (UINT i = 0; i < numCameras; ++i) {
    data += sizeof(UINT);
    SStrCopy(shared->cameraObjs[i].name, (LPCSTR)data, sizeof(shared->cameraObjs[i].name));
    data += sizeof(shared->cameraObjs[i].name);
    shared->cameraObjs[i].pivot = *(NTempest::C3Vector *)data;
    data += sizeof(NTempest::C3Vector) + 12;
    shared->cameraObjs[i].targetPivot = *(NTempest::C3Vector *)data;
    data += sizeof(NTempest::C3Vector);

    data = AddKeyFramesType(data, fileBytes - (data - fileData), 'RTCK', shared, &shared->cameraObjs[i].translation, forceType);
    data = AddKeyFramesType(data, fileBytes - (data - fileData), 'LRCK', shared, &shared->cameraObjs[i].roll, forceType);
    data = AddKeyFramesType(data, fileBytes - (data - fileData), 'RTTK', shared, &shared->cameraObjs[i].targetTranslation, forceType);
    data = AddKeyFramesType(data, fileBytes - (data - fileData), 'SIVK', shared, &shared->cameraObjs[i].visibility, forceType);
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
  UINT  numSequences = 0;
  BYTE *seqDataDone = 0;
  BYTE *data = MDLFileBinarySeek(fileData, fileBytes, 'SQES');
  if (data) {
    UINT sectionBytes = *(UINT *)data;
    data += sizeof(UINT);
    seqDataDone = data + sectionBytes;
    numSequences = *(UINT *)data;
    ASSERT(numSequences);
    data += sizeof(UINT);
  }

  shared->seq.SetCount(numSequences);
  for (UINT i = 0; i < numSequences; ++i) {
    SStrCopy(shared->seq[i].name, (LPCSTR)data, 80);
    data += 80;
    shared->seq[i].time.l = *(int *)data;
    data += sizeof(int);
    shared->seq[i].time.h = *(int *)data;
    data += sizeof(int);
    shared->seq[i].moveSpeed = *(float *)data;
    data += sizeof(float);
    shared->seq[i].flags = *(UINT *)data;
    data += sizeof(UINT);
    shared->seq[i].bounds.radius = *(float *)data;
    data += sizeof(float);
    shared->seq[i].bounds.extent.b = *(NTempest::C3Vector *)data;
    data += sizeof(NTempest::C3Vector);
    shared->seq[i].bounds.extent.t = *(NTempest::C3Vector *)data;
    data += sizeof(NTempest::C3Vector);
    shared->seq[i].randPickChance = NTempest::CMath::fint_n(*(float *)data * 32767.0f);
    data += sizeof(float);
    shared->seq[i].replay.l = *(int *)data;
    data += sizeof(int);
    shared->seq[i].replay.h = *(int *)data;
    data += sizeof(int);
    UINT blendTime = *(UINT *)data;
    data += sizeof(UINT);
    if (blendTime) {
      shared->seq[i].blendTime = blendTime;
    }
  }
  ASSERT(seqDataDone == data);

  unique->seq.SetCount(numSequences);

  UINT numGlobalSeqs = 0;
  seqDataDone = 0;
  if (!data) {
    data = fileData;
  }
  data = MDLFileBinarySeek(data, fileBytes - (data - fileData), 'SBLG');
  if (data) {
    UINT sectionBytes = *(UINT *)data;
    data += sizeof(UINT);
    numGlobalSeqs = sectionBytes / sizeof(MDLGLOBALSEQSECTION);
    seqDataDone = data + sectionBytes;
    ASSERT((numGlobalSeqs * sizeof(MDLGLOBALSEQSECTION)) == sectionBytes);
  }

  unique->seqLastTime = IAnimGetCurrTimeMs();
  unique->globalSeqElapsed.SetCount(numGlobalSeqs);
  unique->globalSeqElapsed.Zero();
  shared->globalSeqLength.SetCount(numGlobalSeqs);
  memcpy(shared->globalSeqLength.Ptr(), data, numGlobalSeqs * sizeof(uint));
  ASSERT((data + numGlobalSeqs * sizeof(uint)) == seqDataDone);
}

void AnimAddSequences(
    CAnim                                      *unique,
    CAnimData                                  *shared,
    const TSGrowableArray<MDLSEQUENCESSECTION> &sequences,
    const TSGrowableArray<MDLGLOBALSEQSECTION> &globalSeqs
) {
  ASSERT(unique);
  ASSERT(shared);
  ASSERT(sequences.Count() <= uint8(0xff));
  ASSERT(globalSeqs.Count() <= uint8(0xff));

  UINT numSequences = sequences.Count();
  shared->seq.SetCount(numSequences);
  UINT i;
  for (i = 0; i < numSequences; ++i) {
    SStrCopy(shared->seq[i].name, sequences[i].name, 80);
    shared->seq[i].time = sequences[i].time;
    shared->seq[i].moveSpeed = sequences[i].movespeed;
    shared->seq[i].flags = sequences[i].flags;
    shared->seq[i].randPickChance = NTempest::CMath::fint_n(sequences[i].frequency * 32767.0f);
    shared->seq[i].replay = sequences[i].replay;
    shared->seq[i].bounds = sequences[i].bounds;
    shared->seq[i].blendTime = sequences[i].blendTime;
  }

  unique->seq.SetCount(numSequences);
  UINT numGlobalSeqs = globalSeqs.Count();
  unique->seqLastTime = IAnimGetCurrTimeMs();
  unique->globalSeqElapsed.SetCount(numGlobalSeqs);
  unique->globalSeqElapsed.Zero();
  shared->globalSeqLength.SetCount(numGlobalSeqs);
  for (i = 0; i < numGlobalSeqs; ++i) {
    shared->globalSeqLength[i] = globalSeqs[i].length;
  }
}

void AnimAddTextureAnims(BYTE *fileData, UINT fileBytes, CAnim *unique, CAnimData *shared, MDLTRACKTYPE forceType) {
  ASSERT(unique);
  ASSERT(shared);
  BYTE *data = MDLFileBinarySeek(fileData, fileBytes, 'NAXT');
  if (!data) {
    return;
  }

  UINT sectionBytes = *(UINT *)data;
  data += sizeof(UINT);
  BYTE *dataDone = data + sectionBytes;
  UINT  numTexAnims = *(UINT *)data;
  data += sizeof(UINT);
  shared->tex.SetCount(numTexAnims);

  for (UINT i = 0; i < numTexAnims; ++i) {
    BYTE *animDone = data + *(UINT *)data;
    data += sizeof(UINT);
    data = AddKeyFramesType(data, fileBytes - (data - fileData), 'TATK', shared, &shared->tex[i].translation, forceType);
    data = AddKeyFramesType(data, fileBytes - (data - fileData), 'RATK', shared, &shared->tex[i].rotation, forceType);
    data = AddKeyFramesType(data, fileBytes - (data - fileData), 'SATK', shared, &shared->tex[i].scale, forceType);
    ASSERT(animDone == data);
  }
  ASSERT(dataDone == data);

  unique->textureStatus.SetCount(numTexAnims);
}

void AnimAddTextureAnim(CAnim *unique, CAnimData *shared, const TSGrowableArray<MDLTEXANIMSECTION> &textureAnims, MDLTRACKTYPE forceType) {
  ASSERT(unique);
  ASSERT(shared);

  shared->tex.SetCount(textureAnims.Count());
  UINT                     i = textureAnims.Count();
  CAnimTransform          *texAnim = shared->tex.Ptr();
  const MDLTEXANIMSECTION *textureAnim = textureAnims.Ptr();
  for (; i; --i, ++textureAnim, ++texAnim) {
    AddKeyFrames(shared, textureAnim->transkeys, &texAnim->translation, forceType);
    AddKeyFrames(shared, textureAnim->rotkeys, &texAnim->rotation, forceType);
    AddKeyFrames(shared, textureAnim->scalekeys, &texAnim->scale, forceType);
  }

  unique->textureStatus.SetCount(textureAnims.Count());
}

