#include <Base/Base.h>

#include "Anim/AnimInternal.h"

#include <malloc.h>

struct CAnimNameHash : public TSHashObject<CAnimNameHash, HASHKEY_CONSTSTRI> {
  UINT index;
};

typedef TSHashTable<CAnimNameHash, HASHKEY_CONSTSTRI> CAnimNameHashTable;

BOOL CAnimData::Animates() {
  UINT index;

  CAnimGeoset *geoset = geo.Ptr();
  for (index = geo.Count(); index--; ++geoset) {
    if (geoset->visibility.TotalKeys() || geoset->color.TotalKeys()) {
      return 1;
    }
  }
  CAnimTransform *texAnim = tex.Ptr();
  for (index = tex.Count(); index--; ++texAnim) {
    if (texAnim->Animates()) {
      return 1;
    }
  }
  CAnimObj *object = baseObjs.Ptr();
  for (index = baseObjs.Count(); index--; ++object) {
    if (object->Animates()) {
      return 1;
    }
  }
  CAnimBoneObj *bone = boneObjs.Ptr();
  for (index = boneObjs.Count(); index--; ++bone) {
    if (bone->Animates()) {
      return 1;
    }
  }
  if (lightObjs.Count()) {
    return 1;
  }
  if (modelObjs.Count()) {
    return 1;
  }
  if (emitter2Objs.Count()) {
    return 1;
  }
  if (ribbonObjs.Count()) {
    return 1;
  }
  CAnimCameraObj *camera = cameraObjs.Ptr();
  for (index = cameraObjs.Count(); index--; ++camera) {
    if (camera->visibility.TotalKeys() || camera->translation.TotalKeys() || camera->roll.TotalKeys() || camera->targetTranslation.TotalKeys()) {
      return 1;
    }
  }
  UINT numEventEmitters = eventObjs.Count();
  for (index = 0; index < numEventEmitters; ++index) {
    CAnimEventObj &eventObject = eventObjs[index];
    if (eventObject.CAnimObj::Animates() || eventObject.events.TotalKeys()) {
      return 1;
    }
  }
  CAnimMaterialLayer *layer = layers.Ptr();
  for (index = layers.Count(); index--; ++layer) {
    if (layer->visibility.TotalKeys() || layer->flip.TotalKeys()) {
      return 1;
    }
  }
  return 0;
}

BOOL CAnimData::Moves() {
  CAnimBoneObj *bone = boneObjs.Ptr();
  for (UINT i = boneObjs.Count(); i--; ++bone) {
    if (bone->Animates()) {
      return 1;
    }
  }
  return 0;
}

static void HashNameList(LPCSTR *names, UINT numNames, CAnimNameHashTable *table) {
  UINT i;

  for (i = 0; i < numNames; ++i, ++names) {
    ASSERT(table->Ptr(*names) == 0);

    CAnimNameHash *hash = table->New(*names, 0, 0);
    hash->index = i;
  }
}

static void SetObjectIndexOrdering(const CAnimNameHashTable &table, UINT *ordering, CAnimObj **itemList, UINT numItems) {
  UINT i;

  for (i = 0; i < numItems; ++i) {
    const CAnimNameHash *hash = table.Ptr(itemList[i]->name);
    if (hash) {
      ASSERT(ordering[hash->index] == 0xffffffff);
      ordering[hash->index] = i;
    }
  }
}

static void SetCameraIndexOrdering(const CAnimNameHashTable &table, UINT *ordering, CAnimCameraObj *itemList, UINT numItems) {
  UINT i;

  for (i = 0; i < numItems; ++i) {
    const CAnimNameHash *hash = table.Ptr(itemList[i].name);
    if (hash) {
      ASSERT(ordering[hash->index] == 0xffffffff);
      ordering[hash->index] = i;
    }
  }
}

static void SetSeqFrequencies(const CArray<CVariations> &ordering, CArray<CAnimSequence> *itemList) {
  UINT numAnimations = ordering.Count();
  UINT variation;
  UINT i;

  for (i = 0; i < numAnimations; ++i) {
    if (ordering[i].primary == 0xFF) {
      continue;
    }

    UINT numMissingFreq = 1;
    UINT numVariations = ordering[i].variation.Count();
    UINT defaultFreq = 0;

    for (variation = 0; variation < numVariations; ++variation) {
      CAnimSequence &sequence = (*itemList)[ordering[i].variation[variation]];
      if (!sequence.randPickChance) {
        ++numMissingFreq;
      } else {
        defaultFreq += sequence.randPickChance;
      }
    }

    defaultFreq = defaultFreq < 0x7FFF ? (0x7FFF - defaultFreq) / numMissingFreq : 0;

    (*itemList)[ordering[i].primary].randPickChance = defaultFreq;
    for (variation = 0; variation < numVariations; ++variation) {
      if (!(*itemList)[ordering[i].variation[variation]].randPickChance) {
        (*itemList)[ordering[i].variation[variation]].randPickChance = defaultFreq;
      }
    }
  }
}

static void SetSeqIndexOrdering(const CAnimNameHashTable &table, CArray<CVariations> *ordering, const CArray<CAnimSequence> &itemList) {
  UINT                                 numSeqTypes = ordering->Count();
  TSStackArray<TSGrowableArray<BYTE> > variations(_alloca(numSeqTypes * sizeof(TSGrowableArray<BYTE>)), numSeqTypes, numSeqTypes);
  const char                           whitespace[3] = "\t ";
  BYTE                                 numItems = itemList.Count();

  for (BYTE i = 0; i < numItems; ++i) {
    LPCSTR source = itemList[i].name;
    char   seqName[80];

    SStrTokenize(&source, seqName, sizeof(seqName), whitespace, 0);

    const CAnimNameHash *hash = table.Ptr(seqName);
    if (hash) {
      CVariations &sequenceOrder = (*ordering)[hash->index];
      if (sequenceOrder.primary == 0xFF) {
        sequenceOrder.primary = i;
      } else if (SStrLen(itemList[i].name) >= SStrLen(itemList[sequenceOrder.primary].name)) {
        variations[hash->index].New(i);
      } else {
        variations[hash->index].New(sequenceOrder.primary);
        sequenceOrder.primary = i;
      }
    }
  }

  numSeqTypes = ordering->Count();
  for (UINT seqType = 0; seqType < numSeqTypes; ++seqType) {
    (*ordering)[seqType].variation.Exchange(&variations[seqType]);
  }
}

static BOOL ApplyObjectLookAtType(HANIM anim, UINT objectId, const NTempest::C3Vector &target, UINT lookAtTypeFlag) {
  CAnim     *unique = reinterpret_cast<CAnim *>(anim);
  CAnimData *shared;
  VALIDATEBEGIN;
  VALIDATE(unique);
  shared = reinterpret_cast<CAnimData *>(unique->hdata);
  VALIDATE(shared);
  VALIDATE(objectId < shared->objectOrder.Count());

  objectId = shared->objectOrder[objectId];
  if (objectId == static_cast<UINT>(-1)) {
    return 0;
  }
  VALIDATE(objectId < unique->status.Count());
  VALIDATEEND;

  CAnimObjStatus *status = unique->status[objectId];
  if (status->base.flags & lookAtTypeFlag) {
    unique->lookAtTarget[status->lookAtId] = target;
    return 1;
  }

  if (status->base.flags & 0x42) {
    status->base.flags &= ~0x42;
    unique->lookAtTarget[status->lookAtId] = target;
  } else {
    ASSERT(unique->lookAtTarget.Count() < 0xff);
    status->lookAtId = static_cast<BYTE>(unique->lookAtTarget.Count());
    unique->lookAtTarget.Add(1, &target);
  }

  status->base.flags |= lookAtTypeFlag | 4;
  if ((unique->flags & 0x10) && (lookAtTypeFlag & 2)) {
    unique->blendStatus[objectId].blendTimer = shared->seq[status->base.currSeq].blendTime;
  }
  return 1;
}

static BOOL RemoveObjectLookAtType(HANIM anim, UINT objectId, UINT lookAtTypeFlag) {
  CAnim     *unique = reinterpret_cast<CAnim *>(anim);
  CAnimData *shared;
  VALIDATEBEGIN;
  VALIDATE(unique);
  shared = reinterpret_cast<CAnimData *>(unique->hdata);
  VALIDATE(shared);
  VALIDATE(objectId < shared->objectOrder.Count());

  objectId = shared->objectOrder[objectId];
  if (objectId == static_cast<UINT>(-1)) {
    return 0;
  }
  VALIDATE(objectId < unique->status.Count());
  VALIDATEEND;

  CAnimObjStatus *status = unique->status[objectId];
  if (status->base.flags & lookAtTypeFlag) {
    UINT lookAtId = status->lookAtId;
    status->base.flags = (status->base.flags & ~lookAtTypeFlag) | 4;

    if (lookAtId < unique->lookAtTarget.Count() - 1) {
      memmove(
          &unique->lookAtTarget[lookAtId], &unique->lookAtTarget[lookAtId + 1],
          (unique->lookAtTarget.Count() - 1 - lookAtId) * sizeof(NTempest::C3Vector)
      );
      for (UINT i = 0; i < unique->status.Count(); ++i) {
        if (unique->status[i]->lookAtId > lookAtId) {
          --unique->status[i]->lookAtId;
        }
      }
    }
    unique->lookAtTarget.SetCount(unique->lookAtTarget.Count() - 1);

    if ((unique->flags & 0x10) && (lookAtTypeFlag & 2)) {
      unique->blendStatus[objectId].blendTimer = shared->seq[status->base.currSeq].blendTime;
    }
  }
  return 1;
}

static int AnimObjectUsingLookAtType(HANIM anim, UINT objectId, UINT lookAtTypeFlag) {
  CAnim     *unique = reinterpret_cast<CAnim *>(anim);
  CAnimData *shared;
  VALIDATEBEGIN;
  VALIDATE(unique);
  shared = reinterpret_cast<CAnimData *>(unique->hdata);
  VALIDATE(shared);
  VALIDATE(objectId < shared->objectOrder.Count());
  objectId = shared->objectOrder[objectId];
  if (objectId == static_cast<UINT>(-1)) {
    return 0;
  }
  VALIDATE(objectId < unique->status.Count());
  VALIDATEEND;
  return unique->status[objectId]->base.flags & lookAtTypeFlag;
}

void AnimResetAnimationStatus(HANIM anim, int onlyResetCallbacks) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  UINT numSequences = unique->seq.Count();
  memset(&unique->anySeqFinished, 0, sizeof(unique->anySeqFinished));
  memset(&unique->appEvent, 0, sizeof(unique->appEvent));
  UINT index;
  if (onlyResetCallbacks) {
    for (index = 0; index < numSequences; ++index) {
      unique->seq[index].ResetCallback();
    }
    return;
  }

  for (index = 0; index < numSequences; ++index) {
    unique->seq[index].Reset();
  }
  unique->seqLastTime = IAnimGetCurrTimeMs();

  UINT numObjects = unique->status.Count();
  for (index = 0; index < numObjects; ++index) {
    unique->status[index]->base.flags = 0x10;
  }
  numObjects = unique->blendStatus.Count();
  for (index = 0; index < numObjects; ++index) {
    unique->blendStatus[index].blendTimer = 0;
  }
  numObjects = unique->cameraStatus.Count();
  for (index = 0; index < numObjects; ++index) {
    unique->cameraStatus[index].base.flags = 0x10;
  }
  numObjects = unique->textureStatus.Count();
  for (index = 0; index < numObjects; ++index) {
    unique->textureStatus[index].base.flags = 0x10;
  }
  numObjects = unique->geosetStatus.Count();
  for (index = 0; index < numObjects; ++index) {
    unique->geosetStatus[index].base.flags = 0x10;
  }
  numObjects = unique->layerStatus.Count();
  for (index = 0; index < numObjects; ++index) {
    unique->layerStatus[index].base.flags = 0x10;
  }
}

void AnimEnumObjects(HANIM anim, int (*callbackfcn)(UINT, LPCSTR, LPVOID), LPVOID param) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  UINT   numObjects;

  ASSERT(unique);
  ASSERT(callbackfcn);

  CAnimData *ptr = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(ptr);

  numObjects = ptr->obj.Count();
  for (UINT i = 0; i < numObjects; ++i) {
    ASSERT(ptr->obj[i]);
    if (!callbackfcn(i, ptr->obj[i]->name, param)) {
      break;
    }
  }
}

BOOL AnimGetSequenceDuration(HANIM anim, UINT seqIndex, UINT *duration) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);
  ASSERT(duration);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  if (seqIndex >= shared->seqOrder[unique->seqMapIndex].order.Count()) {
    return 0;
  }

  UINT sequence = shared->seqOrder[unique->seqMapIndex].order[seqIndex].primary;
  if (sequence == 0xFF) {
    return 0;
  }

  *duration = shared->seq[sequence].time.Magnitude();
  return 1;
}

BOOL AnimGetSequenceName(HANIM anim, UINT seqIndex, char *buffer, UINT buffLength) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);
  ASSERT(buffer);
  ASSERT(buffLength);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  if (seqIndex >= shared->seqOrder[unique->seqMapIndex].order.Count()) {
    return 0;
  }

  UINT sequence = shared->seqOrder[unique->seqMapIndex].order[seqIndex].primary;
  if (sequence == 0xFF) {
    return 0;
  }

  SStrCopy(buffer, shared->seq[sequence].name, buffLength);
  return 1;
}

BOOL AnimGetSequenceMoveSpeed(HANIM anim, UINT seqIndex, float *moveSpeed) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);
  ASSERT(moveSpeed);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  if (seqIndex >= shared->seqOrder[unique->seqMapIndex].order.Count()) {
    return 0;
  }

  UINT sequence = shared->seqOrder[unique->seqMapIndex].order[seqIndex].primary;
  if (sequence == 0xFF) {
    return 0;
  }

  *moveSpeed = shared->seq[sequence].moveSpeed;
  return 1;
}

BOOL AnimHasSequenceId(HANIM anim, UINT seqIndex) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);
  return shared->seqOrder[unique->seqMapIndex].order[seqIndex].primary != 0xFF;
}

UINT AnimGetNumSequences(HANIM anim) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *ptr = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(ptr);
  return ptr->seq.Count();
}

void AnimSetObjectOrdering(HANIM anim, LPCSTR *boneNames, UINT numBones) {
  if (!numBones) {
    return;
  }

  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);
  ASSERT(boneNames);

  CAnimNameHashTable objNameHashTable;
  HashNameList(boneNames, numBones, &objNameHashTable);

  shared->objectOrder.SetCount(numBones);
  memset(shared->objectOrder.Ptr(), 0xFF, shared->objectOrder.Bytes());

  SetObjectIndexOrdering(objNameHashTable, shared->objectOrder.Ptr(), shared->obj.Ptr(), shared->obj.Count());
  objNameHashTable.Clear();
}

void AnimResetObjectOrdering(HANIM__ *anim) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);
  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  UINT numObjects = shared->obj.Count();
  shared->objectOrder.SetCount(numObjects);
  for (UINT i = 0; i < numObjects; ++i) {
    shared->objectOrder[i] = i;
  }
}

void AnimSetSequenceOrderingDefault(HANIM anim) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  BYTE i;
  for (i = 0; i < shared->seqOrder.Count(); ++i) {
    if (!shared->seqOrder[i].nameListUsed) {
      unique->seqMapIndex = i;
      return;
    }
  }

  ASSERT(shared->seqOrder.Count() < 0xfe);
  unique->seqMapIndex = static_cast<BYTE>(shared->seqOrder.Count());

  CSeqOrdering *sequenceOrder = shared->seqOrder.New();
  sequenceOrder->order.SetCount(shared->seq.Count());
  sequenceOrder->nameListUsed = 0;

  for (UINT index = 0; index < shared->seq.Count(); ++index) {
    sequenceOrder->order[index].primary = index;
  }
}

void AnimSetSequenceOrdering(HANIM anim, LPCSTR *sequenceNames, UINT numSequences) {
  if (!numSequences) {
    return;
  }

  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);
  ASSERT(sequenceNames);

  for (BYTE index = 0; index < shared->seqOrder.Count(); ++index) {
    if (shared->seqOrder[index].nameListUsed == sequenceNames) {
      unique->seqMapIndex = index;
      SetSeqFrequencies(shared->seqOrder[index].order, &shared->seq);
      return;
    }
  }

  CAnimNameHashTable seqNameHashTable;
  HashNameList(sequenceNames, numSequences, &seqNameHashTable);

  ASSERT(shared->seqOrder.Count() < 0xfe);
  unique->seqMapIndex = static_cast<BYTE>(shared->seqOrder.Count());

  CSeqOrdering *sequenceOrder = shared->seqOrder.New();
  sequenceOrder->order.SetCount(numSequences);
  sequenceOrder->nameListUsed = sequenceNames;

  SetSeqIndexOrdering(seqNameHashTable, &sequenceOrder->order, shared->seq);
  seqNameHashTable.Clear();
  SetSeqFrequencies(shared->seqOrder[unique->seqMapIndex].order, &shared->seq);
}

void AnimSetCameraOrdering(HANIM anim, LPCSTR *cameraNames, UINT numCameras, TSFixedArray<UINT> *cameraOrder) {
  if (!numCameras) {
    return;
  }

  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);
  ASSERT(cameraNames);

  CAnimNameHashTable objNameHashTable;
  HashNameList(cameraNames, numCameras, &objNameHashTable);

  cameraOrder->SetCount(numCameras);
  memset(cameraOrder->Ptr(), 0xFF, cameraOrder->Bytes());

  SetCameraIndexOrdering(objNameHashTable, cameraOrder->Ptr(), shared->cameraObjs.Ptr(), shared->cameraObjs.Count());
  objNameHashTable.Clear();
}

void AnimResetCameraOrdering(HANIM anim, TSFixedArray<UINT> *cameraOrder) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  UINT numCameras = shared->cameraObjs.Count();
  cameraOrder->SetCount(numCameras);
  for (UINT i = 0; i < numCameras; ++i) {
    (*cameraOrder)[i] = i;
  }
}

UINT AnimGetFlags(HANIM anim) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);
  return shared->flags;
}

UINT AnimGetTotalKeys(HANIM anim) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  UINT totalKeys = 0;
  UINT i;

  CAnimGeoset *geoset = shared->geo.Ptr();
  for (i = shared->geo.Count(); i; --i, ++geoset) {
    totalKeys += geoset->visibility.TotalKeys() + geoset->color.TotalKeys();
  }
  CAnimTransform *texAnim = shared->tex.Ptr();
  for (i = shared->tex.Count(); i; --i, ++texAnim) {
    totalKeys += texAnim->translation.TotalKeys() + texAnim->rotation.TotalKeys() + texAnim->scale.TotalKeys();
  }
  CAnimObj **object = shared->obj.Ptr();
  for (i = shared->obj.Count(); i; --i, ++object) {
    totalKeys += (*object)->translation.TotalKeys() + (*object)->rotation.TotalKeys() + (*object)->scale.TotalKeys();
  }
  CAnimLightObj *light = shared->lightObjs.Ptr();
  for (i = shared->lightObjs.Count(); i; --i, ++light) {
    totalKeys += light->visibility.TotalKeys() + light->attenstart.TotalKeys() + light->attenend.TotalKeys() + light->color.TotalKeys()
                 + light->intensity.TotalKeys();
  }
  CAnimMaterialLayer *layer = shared->layers.Ptr();
  for (i = shared->layers.Count(); i; --i, ++layer) {
    totalKeys += layer->visibility.TotalKeys();
  }
  CAnimEmitter2Obj *emitter = shared->emitter2Objs.Ptr();
  for (i = shared->emitter2Objs.Count(); i; --i, ++emitter) {
    totalKeys += emitter->translation.TotalKeys() + emitter->rotation.TotalKeys() + emitter->scale.TotalKeys() + emitter->visibility.TotalKeys()
                 + emitter->particleSpeed.TotalKeys() + emitter->emissionRate.TotalKeys() + emitter->gravity.TotalKeys()
                 + emitter->variation.TotalKeys() + emitter->latitude.TotalKeys() + emitter->longitude.TotalKeys() + emitter->length.TotalKeys()
                 + emitter->width.TotalKeys();
  }
  CAnimRibbonObj *ribbon = shared->ribbonObjs.Ptr();
  for (i = shared->ribbonObjs.Count(); i; --i, ++ribbon) {
    totalKeys += ribbon->heightAbove.TotalKeys() + ribbon->heightBelow.TotalKeys() + ribbon->slot.TotalKeys();
  }
  return totalKeys;
}

UINT AnimGetAttachmentObjId(HANIM__ *anim, UINT index) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);
  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);
  ASSERT(index < shared->modelObjs.Count());
  return index < shared->modelObjs.Count() ? shared->modelObjs[index].animObjId : 0;
}

BOOL AnimIsAttachmentEnabled(HANIM anim, UINT index) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);

  ASSERT(unique);
  ASSERT(index < unique->modelStatus.Count());

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  UINT geosetId = shared->modelObjs[index].geosetId;

  if (unique->modelStatus[index].IsVisible() && (geosetId == 0xFF || unique->geosetStatus[geosetId].IsVisible())) {
    return 1;
  }

  return 0;
}

int AnimIsCameraEnabled(HANIM anim, UINT index) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);

  ASSERT(unique);
  ASSERT(index < unique->cameraStatus.Count());

  return unique->cameraStatus[index].IsVisible();
}

void AnimSetSeqFinishedHandler(HANIM anim, ANIMSEQFINISHEDHANDLER callback, LPVOID param) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);

  ASSERT(unique);
  unique->anySeqFinished.callback = callback;
  unique->anySeqFinished.param = param;
}

void AnimSetSeqFinishedHandler(HANIM anim, UINT sequence, ANIMSEQFINISHEDHANDLER callback, LPVOID param) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  BYTE primary = shared->seqOrder[unique->seqMapIndex].order[sequence].primary;
  if (primary == 0xFF) {
    return;
  }

  unique->seq[primary].finished.callback = callback;
  unique->seq[primary].finished.param = param;
  BYTE *variation = shared->seqOrder[unique->seqMapIndex].order[sequence].variation.Ptr();
  UINT  numVariations = shared->seqOrder[unique->seqMapIndex].order[sequence].variation.Count();
  for (UINT i = 0; i < numVariations; ++i) {
    unique->seq[variation[i]].finished.callback = callback;
    unique->seq[variation[i]].finished.param = param;
  }
}

BOOL AnimGetPrimarySequence(HANIM anim, UINT *sequence) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);

  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  if (!shared->seq.Count()) {
    return 0;
  }

  *sequence = unique->primarySeq;
  return 1;
}

float AnimGetPrimarySequenceCompletion(HANIM anim) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  if (!unique->seq.Count()) {
    return 0.0f;
  }

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);
  CAnimSequence &sequence = shared->seq[unique->primarySeq];
  CSeqInfo      &seqInfo = unique->seq[unique->primarySeq];
  UINT           duration = sequence.time.Magnitude();
  if (!duration) {
    return 0.0f;
  }

  float completion = static_cast<float>(static_cast<UINT>(seqInfo.elapsed - sequence.time.l)) / duration;
  if (completion < 0.0f) {
    return 0.0f;
  }
  if (completion > 1.0f) {
    return 1.0f;
  }
  return completion;
}

BOOL AnimNeedsSequenceBounds(HANIM anim) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);

  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  if (((shared->flags & 8) && shared->seq.Count() > 1) || ((shared->flags & 2) && shared->seq.Count() > 1)) {
    return 1;
  }

  return 0;
}

void AnimSetEventCallback(HANIM anim, void (*callback)(LPCSTR, const NTempest::C3Vector &, LPVOID), LPVOID param) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);

  ASSERT(unique);
  unique->appEvent.callback = callback;
  unique->appEvent.param = param;
}

int AnimObjectUsingLookAt(HANIM anim, UINT objectId) {
  return AnimObjectUsingLookAtType(anim, objectId, 2);
}

int AnimApplyObjectLookAt(HANIM anim, UINT objectId, const NTempest::C3Vector &target) {
  return ApplyObjectLookAtType(anim, objectId, target, 2);
}

int AnimRemoveObjectLookAt(HANIM anim, UINT objectId) {
  return RemoveObjectLookAtType(anim, objectId, 2);
}

int AnimObjectUsingFaceDir(HANIM anim, UINT objectId) {
  return AnimObjectUsingLookAtType(anim, objectId, 0x40);
}

int AnimApplyObjectFaceDir(HANIM anim, UINT objectId, NTempest::C3Vector direction) {
  float magnitude = direction.Mag();
  if (NTempest::CMath::fabs_(magnitude) >= 0.00000023841858f) {
    float inverseMagnitude = 1.0f / magnitude;
    direction.x *= inverseMagnitude;
    direction.y *= inverseMagnitude;
    direction.z *= inverseMagnitude;
  }
  return ApplyObjectLookAtType(anim, objectId, direction, 0x40);
}

int AnimRemoveObjectFaceDir(HANIM anim, UINT objectId) {
  return RemoveObjectLookAtType(anim, objectId, 0x40);
}

int AnimUsesBlending(HANIM anim) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);

  ASSERT(unique);
  return unique->flags & 0x10;
}

void AnimEnableBlending(HANIM anim, int enable) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  if (enable) {
    unique->flags |= 0x10;
    UINT numObjects = unique->status.Count();
    unique->blendStatus.SetCount(numObjects);
  } else {
    unique->flags &= ~0x10;
    unique->blendStatus.SetCount(0);
  }
}

BOOL AnimMarkFootstepSequence(HANIM anim, UINT index) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);
  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  index = shared->seqOrder[unique->seqMapIndex].order[index].primary;
  if (index == 0xFF) {
    return 0;
  }
  ASSERT(index < shared->seq.Count());
  shared->seq[index].flags |= 2;
  return 1;
}

BOOL AnimLockObjectSequence(HANIM anim, UINT objectId, int set) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);
  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);
  ASSERT(objectId < shared->objectOrder.Count());

  objectId = shared->objectOrder[objectId];
  if (objectId == static_cast<UINT>(-1)) {
    return 0;
  }
  ASSERT(objectId < shared->obj.Count());
  CAnimObjStatus *status = unique->status[objectId];
  if (set) {
    status->base.flags |= 0x20;
  } else {
    status->base.flags &= ~0x20;
  }
  return 1;
}

BOOL AnimHasObjectId(HANIM anim, UINT objectId) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);
  ASSERT(objectId < shared->objectOrder.Count());

  return shared->objectOrder[objectId] != static_cast<UINT>(-1);
}

BOOL AnimEventEmitterHasKeysThisSeq(HANIM anim, UINT objectId) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);
  ASSERT(objectId < shared->objectOrder.Count());

  objectId = shared->objectOrder[objectId];
  if (objectId == static_cast<UINT>(-1)) {
    return 0;
  }

  CAnimObj *currobj = shared->obj[objectId];
  ASSERT(currobj);
  ASSERT(currobj->type == OBJ_TYPE_EVENT);
  CAnimObjStatus *status = unique->status[objectId];
  ASSERT(status);

  CAnimEventObj *eventObject = static_cast<CAnimEventObj *>(currobj);
  return eventObject->events.NumKeysThisSeqSafe(status->base.currSeq) > 0;
}

BOOL AnimGetObjectPosition(HANIM anim, UINT objectId, const TSFixedArray<NTempest::C3Vector> &positions, NTempest::C3Vector *position) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);
  ASSERT(objectId < shared->objectOrder.Count());

  objectId = shared->objectOrder[objectId];
  if (objectId == static_cast<UINT>(-1)) {
    return 0;
  }

  VALIDATEBEGIN;
  VALIDATE(objectId < positions.Count());
  VALIDATEEND;
  *position = positions[objectId];
  return 1;
}

BOOL AnimGetEventObjectPosition(HANIM anim, UINT objectId, NTempest::C3Vector *position) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);
  ASSERT(objectId < shared->objectOrder.Count());

  objectId = shared->objectOrder[objectId];
  if (objectId == static_cast<UINT>(-1)) {
    return 0;
  }

  ASSERT(shared->obj[objectId]->type == OBJ_TYPE_EVENT);
  *position = static_cast<CAnimEventObjStatus *>(unique->status[objectId])->position;
  return 1;
}
