#include "Anim/AnimInternal.h"

#include <malloc.h>

struct CAnimNameHash : public TSHashObject<CAnimNameHash, HASHKEY_CONSTSTRI> {
  unsigned int index;
};

typedef TSHashTable<CAnimNameHash, HASHKEY_CONSTSTRI> CAnimNameHashTable;

int CAnimData::Animates() {
  unsigned int index;
  for (index = 0; index < geo.Count(); ++index) {
    if (geo[index].visibility.TotalKeys() || geo[index].color.TotalKeys()) {
      return 1;
    }
  }
  for (index = 0; index < tex.Count(); ++index) {
    if (tex[index].Animates()) {
      return 1;
    }
  }
  for (index = 0; index < baseObjs.Count(); ++index) {
    if (baseObjs[index].CAnimTransform::Animates() || (baseObjs[index].flags & 0x3F)) {
      return 1;
    }
  }
  for (index = 0; index < boneObjs.Count(); ++index) {
    if (boneObjs[index].CAnimTransform::Animates() || (boneObjs[index].flags & 0x3F)) {
      return 1;
    }
  }
  if (lightObjs.Count() || modelObjs.Count() || emitter2Objs.Count() || ribbonObjs.Count()) {
    return 1;
  }
  for (index = 0; index < cameraObjs.Count(); ++index) {
    CAnimCameraObj &camera = cameraObjs[index];
    if (camera.visibility.TotalKeys() || camera.translation.TotalKeys() || camera.roll.TotalKeys() || camera.targetTranslation.TotalKeys()) {
      return 1;
    }
  }
  unsigned int numEventEmitters = eventObjs.Count();
  for (index = 0; index < numEventEmitters; ++index) {
    CAnimEventObj &eventObject = eventObjs[index];
    if (eventObject.Animates() || (eventObject.flags & 0x3F) || eventObject.events.TotalKeys()) {
      return 1;
    }
  }
  for (index = 0; index < layers.Count(); ++index) {
    if (layers[index].visibility.TotalKeys() || layers[index].flip.TotalKeys()) {
      return 1;
    }
  }
  return 0;
}

int CAnimTransform::Animates() {
  return translation.TotalKeys() || rotation.TotalKeys() || scale.TotalKeys();
}

unsigned int CAnimTransform::Bytes() const {
  return translation.CKeyFrameTrackBase::Bytes() + rotation.CKeyFrameTrackBase::Bytes() +
         scale.CKeyFrameTrackBase::Bytes();
}

int CAnimVisibleObj::Animates() {
  return visibility.TotalKeys() != 0;
}

unsigned int CAnimVisibleObj::Bytes() const {
  return visibility.CKeyFrameTrackBase::Bytes();
}

int CAnimData::Moves() {
  for (unsigned int index = 0; index < boneObjs.Count(); ++index) {
    CAnimBoneObj &bone = boneObjs[index];
    if (bone.CAnimTransform::Animates() || (bone.flags & 0x3F)) {
      return 1;
    }
  }
  return 0;
}

static void HashNameList(const char **names, unsigned int numNames, CAnimNameHashTable *table) {
  unsigned int i;

  for (i = 0; i < numNames; ++i) {
    ASSERT(!table->Ptr(names[i]));

    CAnimNameHash *nameHash = table->New(names[i], 0, 0);
    nameHash->index = i;
  }
}

static void SetObjectIndexOrdering(const CAnimNameHashTable &table, unsigned int *ordering, CAnimObj **itemList, unsigned int numItems) {
  unsigned int i;

  for (i = 0; i < numItems; ++i) {
    const CAnimNameHash *nameHash = table.Ptr(itemList[i]->name);
    if (nameHash) {
      ASSERT(ordering[nameHash->index] == 0xFFFFFFFF);
      ordering[nameHash->index] = i;
    }
  }
}

static void
SetCameraIndexOrdering(const CAnimNameHashTable &table, unsigned int *ordering, CAnimCameraObj *itemList, unsigned int numItems) {
  unsigned int i;

  for (i = 0; i < numItems; ++i) {
    const CAnimNameHash *nameHash = table.Ptr(itemList[i].name);
    if (nameHash) {
      ASSERT(ordering[nameHash->index] == 0xFFFFFFFF);
      ordering[nameHash->index] = i;
    }
  }
}

static void SetSeqFrequencies(const CArray<CVariations> &ordering, CArray<CAnimSequence> *itemList) {
  unsigned int numAnimations = ordering.Count();
  unsigned int variation;
  unsigned int i;

  for (i = 0; i < numAnimations; ++i) {
    if (ordering[i].primary == 0xFF) {
      continue;
    }

    unsigned int numVariations = ordering[i].variation.Count();
    unsigned int numMissingFreq = 1;
    unsigned int defaultFreq = 0;

    for (variation = 0; variation < numVariations; ++variation) {
      CAnimSequence &sequence = (*itemList)[ordering[i].variation[variation]];
      if (sequence.randPickChance) {
        defaultFreq += sequence.randPickChance;
      } else {
        ++numMissingFreq;
      }
    }

    defaultFreq = defaultFreq >= 0x7FFF ? 0 : (0x7FFF - defaultFreq) / numMissingFreq;

    (*itemList)[ordering[i].primary].randPickChance = defaultFreq;
    for (variation = 0; variation < numVariations; ++variation) {
      CAnimSequence &sequence = (*itemList)[ordering[i].variation[variation]];
      if (!sequence.randPickChance) {
        sequence.randPickChance = defaultFreq;
      }
    }
  }
}

static void SetSeqIndexOrdering(const CAnimNameHashTable &table, CArray<CVariations> *ordering, const CArray<CAnimSequence> &itemList) {
  unsigned int                                  numSeqTypes = ordering->Count();
  TSStackArray<TSGrowableArray<unsigned char> > variations(_alloca(numSeqTypes * sizeof(TSGrowableArray<unsigned char>)), numSeqTypes, numSeqTypes);
  const char                                    whitespace[3] = "\t ";
  unsigned int                                  numItems = itemList.Count();
  unsigned int                                  i;

  for (i = 0; i < numItems; ++i) {
    const char *name = reinterpret_cast<const char *>(&itemList[i].name);
    const char *source = name;
    char        seqName[80];

    SStrTokenize(&source, seqName, sizeof(seqName), whitespace, 0);

    const CAnimNameHash *nameHash = table.Ptr(seqName);
    if (!nameHash) {
      continue;
    }

    CVariations &sequenceOrder = (*ordering)[nameHash->index];
    if (sequenceOrder.primary == 0xFF) {
      sequenceOrder.primary = i;
      continue;
    }

    const char *primaryName = reinterpret_cast<const char *>(&itemList[sequenceOrder.primary].name);
    if (SStrLen(name) < SStrLen(primaryName)) {
      unsigned char previousPrimary = sequenceOrder.primary;
      variations[nameHash->index].Add(1, &previousPrimary);
      sequenceOrder.primary = i;
    } else {
      variations[nameHash->index].Add(1, reinterpret_cast<const unsigned char *>(&i));
    }
  }

  for (unsigned int seqType = 0; seqType < numSeqTypes; ++seqType) {
    (*ordering)[seqType].variation.Exchange(&variations[seqType]);
  }
}

static int ApplyObjectLookAtType(HANIM anim, unsigned int objectId, const NTempest::C3Vector &target, unsigned int lookAtTypeFlag) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);
  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);
  ASSERT(objectId < shared->objectOrder.Count());

  unsigned int sharedObjectId = shared->objectOrder[objectId];
  if (sharedObjectId == static_cast<unsigned int>(-1)) {
    return 0;
  }
  ASSERT(sharedObjectId < unique->status.Count());

  CAnimObjStatus *status = unique->status[sharedObjectId];
  if (status->base.flags & lookAtTypeFlag) {
    unique->lookAtTarget[status->lookAtId] = target;
    return 1;
  }

  if (status->base.flags & 0x42) {
    status->base.flags &= ~0x42;
    unique->lookAtTarget[status->lookAtId] = target;
  } else {
    ASSERT(unique->lookAtTarget.Count() < 0xFF);
    status->lookAtId = static_cast<unsigned char>(unique->lookAtTarget.Count());
    unique->lookAtTarget.Add(1, &target);
  }

  status->base.flags |= lookAtTypeFlag | 4;
  if ((unique->flags & 0x10) && (lookAtTypeFlag & 2)) {
    unique->blendStatus[sharedObjectId].blendTimer = shared->seq[status->base.currSeq].blendTime;
  }
  return 1;
}

static int RemoveObjectLookAtType(HANIM anim, unsigned int objectId, unsigned int lookAtTypeFlag) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);
  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);
  ASSERT(objectId < shared->objectOrder.Count());

  unsigned int sharedObjectId = shared->objectOrder[objectId];
  if (sharedObjectId == static_cast<unsigned int>(-1)) {
    return 0;
  }
  ASSERT(sharedObjectId < unique->status.Count());

  CAnimObjStatus *status = unique->status[sharedObjectId];
  if (status->base.flags & lookAtTypeFlag) {
    unsigned int lookAtId = status->lookAtId;
    status->base.flags = (status->base.flags & ~lookAtTypeFlag) | 4;

    unsigned int newCount = unique->lookAtTarget.Count() - 1;
    if (lookAtId < newCount) {
      memmove(&unique->lookAtTarget[lookAtId], &unique->lookAtTarget[lookAtId + 1], (newCount - lookAtId) * sizeof(NTempest::C3Vector));
      for (unsigned int i = 0; i < unique->status.Count(); ++i) {
        if (unique->status[i]->lookAtId > lookAtId) {
          --unique->status[i]->lookAtId;
        }
      }
    }
    unique->lookAtTarget.SetCount(newCount);

    if ((unique->flags & 0x10) && (lookAtTypeFlag & 2)) {
      unique->blendStatus[sharedObjectId].blendTimer = shared->seq[status->base.currSeq].blendTime;
    }
  }
  return 1;
}

static int AnimObjectUsingLookAtType(HANIM anim, unsigned int objectId, unsigned int lookAtTypeFlag) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);
  ASSERT(objectId < shared->objectOrder.Count());

  objectId = shared->objectOrder[objectId];
  if (objectId == static_cast<unsigned int>(-1)) {
    return 0;
  }

  ASSERT(objectId < unique->status.Count());
  return unique->status[objectId]->base.flags & lookAtTypeFlag;
}

void AnimResetAnimationStatus(HANIM anim, int onlyResetCallbacks) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  unique->anySeqFinished.callback = 0;
  unique->anySeqFinished.param = 0;
  unique->appEvent.callback = 0;
  unique->appEvent.param = 0;

  unsigned int numSequences = unique->seq.Count();
  unsigned int index;
  if (onlyResetCallbacks) {
    for (index = 0; index < numSequences; ++index) {
      unique->seq[index].finished.callback = 0;
      unique->seq[index].finished.param = 0;
    }
    return;
  }

  for (index = 0; index < numSequences; ++index) {
    memset(&unique->seq[index], 0, sizeof(CSeqInfo));
    unique->seq[index].seqTimeScale = 1.0f;
  }
  unique->seqLastTime = IAnimGetCurrTimeMs();

  unsigned int numObjects = unique->status.Count();
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
  numObjects = unique->modelStatus.Count();
  for (index = 0; index < numObjects; ++index) {
    unique->modelStatus[index].base.flags = 0x10;
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

void AnimEnumObjects(HANIM anim, int(*callbackfcn)(unsigned int, const char *, void *), void *param) {
  CAnim       *unique = reinterpret_cast<CAnim *>(anim);
  unsigned int numObjects;

  ASSERT(unique);
  ASSERT(callbackfcn);

  CAnimData *ptr = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(ptr);

  numObjects = ptr->obj.Count();
  for (unsigned int i = 0; i < numObjects; ++i) {
    ASSERT(ptr->obj[i]);
    if (!callbackfcn(i, ptr->obj[i]->name, param)) {
      break;
    }
  }
}

int AnimGetSequenceDuration(HANIM anim, unsigned int seqIndex, unsigned int *duration) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);
  ASSERT(duration);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  CVariations &selection = shared->seqOrder[unique->seqMapIndex].order[seqIndex];
  if (selection.primary == 0xFF) {
    return 0;
  }

  *duration = shared->seq[selection.primary].time.h - shared->seq[selection.primary].time.l;
  return 1;
}

int AnimGetSequenceName(HANIM anim, unsigned int seqIndex, char *buffer, unsigned int buffLength) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);
  ASSERT(buffer);
  ASSERT(buffLength);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  CVariations &selection = shared->seqOrder[unique->seqMapIndex].order[seqIndex];
  if (selection.primary == 0xFF) {
    return 0;
  }

  SStrCopy(buffer, reinterpret_cast<const char *>(&shared->seq[selection.primary].name), buffLength);
  return 1;
}

int AnimGetSequenceMoveSpeed(HANIM anim, unsigned int seqIndex, float *moveSpeed) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);
  ASSERT(moveSpeed);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  CVariations &selection = shared->seqOrder[unique->seqMapIndex].order[seqIndex];
  if (selection.primary == 0xFF) {
    return 0;
  }

  *moveSpeed = shared->seq[selection.primary].moveSpeed;
  return 1;
}

int AnimHasSequenceId(HANIM anim, unsigned int seqIndex) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);
  return shared->seqOrder[unique->seqMapIndex].order[seqIndex].primary != 0xFF;
}

unsigned int AnimGetNumSequences(HANIM anim) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);
  return shared->seq.Count();
}

void AnimSetObjectOrdering(HANIM anim, const char **boneNames, unsigned int numBones) {
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

  shared->objectOrder.ReserveSpace(numBones);
  shared->objectOrder.SetCount(numBones);
  memset(shared->objectOrder.Ptr(), 0xFF, shared->objectOrder.Bytes());

  SetObjectIndexOrdering(
      objNameHashTable, shared->objectOrder.Ptr(), shared->obj.Ptr(), shared->obj.Count()
  );
}

void AnimResetObjectOrdering(HANIM__* anim) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);
  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  shared->objectOrder.ReserveSpace(shared->obj.Count());
  shared->objectOrder.SetCount(shared->obj.Count());
  for (unsigned int i = 0; i < shared->obj.Count(); ++i) {
    shared->objectOrder[i] = i;
  }
}

void AnimSetSequenceOrderingDefault(HANIM anim) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  unsigned int i;
  for (i = 0; i < shared->seqOrder.Count(); ++i) {
    if (!shared->seqOrder[i].nameListUsed) {
      unique->seqMapIndex = static_cast<unsigned char>(i);
      return;
    }
  }

  ASSERT(shared->seqOrder.Count() < 0xFE);
  unique->seqMapIndex = static_cast<unsigned char>(shared->seqOrder.Count());

  CSeqOrdering *sequenceOrder = shared->seqOrder.New();
  sequenceOrder->nameListUsed = 0;
  sequenceOrder->order.ReserveSpace(shared->seq.Count());
  sequenceOrder->order.SetCount(shared->seq.Count());

  for (i = 0; i < shared->seq.Count(); ++i) {
    sequenceOrder->order[i].primary = i;
  }
}

void AnimSetSequenceOrdering(HANIM anim, const char **sequenceNames, unsigned int numSequences) {
  if (!numSequences) {
    return;
  }

  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);
  ASSERT(sequenceNames);

  unsigned char index;
  for (index = 0; index < shared->seqOrder.Count(); ++index) {
    if (shared->seqOrder[index].nameListUsed == sequenceNames) {
      unique->seqMapIndex = index;
      SetSeqFrequencies(shared->seqOrder[index].order, &shared->seq);
      return;
    }
  }

  CAnimNameHashTable seqNameHashTable;
  HashNameList(sequenceNames, numSequences, &seqNameHashTable);

  ASSERT(shared->seqOrder.Count() < 0xFE);
  unique->seqMapIndex = static_cast<unsigned char>(shared->seqOrder.Count());

  CSeqOrdering *sequenceOrder = shared->seqOrder.New();
  sequenceOrder->order.ReserveSpace(numSequences);
  sequenceOrder->order.SetCount(numSequences);
  sequenceOrder->nameListUsed = sequenceNames;

  SetSeqIndexOrdering(seqNameHashTable, &sequenceOrder->order, shared->seq);
  SetSeqFrequencies(sequenceOrder->order, &shared->seq);
}

void AnimSetCameraOrdering(HANIM anim, const char **cameraNames, unsigned int numCameras, TSFixedArray<unsigned int> *cameraOrder) {
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
  memset(cameraOrder->Ptr(), 0xFF, numCameras * sizeof(unsigned int));

  SetCameraIndexOrdering(
      objNameHashTable, cameraOrder->Ptr(), shared->cameraObjs.Ptr(), shared->cameraObjs.Count()
  );
}

void AnimResetCameraOrdering(HANIM anim, TSFixedArray<unsigned int> *cameraOrder) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  unsigned int numCameras = shared->cameraObjs.Count();
  cameraOrder->SetCount(numCameras);
  for (unsigned int i = 0; i < numCameras; ++i) {
    (*cameraOrder)[i] = i;
  }
}

unsigned int AnimGetFlags(HANIM anim) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);
  return shared->flags;
}

unsigned int AnimGetTotalKeys(HANIM anim) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  unsigned int totalKeys = 0;
  unsigned int i;
  for (i = 0; i < shared->geo.Count(); ++i) {
    totalKeys += shared->geo[i].visibility.TotalKeys();
    totalKeys += shared->geo[i].color.TotalKeys();
  }
  for (i = 0; i < shared->tex.Count(); ++i) {
    totalKeys += shared->tex[i].translation.TotalKeys();
    totalKeys += shared->tex[i].rotation.TotalKeys();
    totalKeys += shared->tex[i].scale.TotalKeys();
  }
  for (i = 0; i < shared->obj.Count(); ++i) {
    totalKeys += shared->obj[i]->translation.TotalKeys();
    totalKeys += shared->obj[i]->rotation.TotalKeys();
    totalKeys += shared->obj[i]->scale.TotalKeys();
  }
  for (i = 0; i < shared->lightObjs.Count(); ++i) {
    totalKeys += shared->lightObjs[i].visibility.TotalKeys();
    totalKeys += shared->lightObjs[i].color.TotalKeys();
    totalKeys += shared->lightObjs[i].intensity.TotalKeys();
    totalKeys += shared->lightObjs[i].ambColor.TotalKeys();
    totalKeys += shared->lightObjs[i].ambIntensity.TotalKeys();
  }
  for (i = 0; i < shared->cameraObjs.Count(); ++i) {
    totalKeys += shared->cameraObjs[i].visibility.TotalKeys();
  }
  for (i = 0; i < shared->emitter2Objs.Count(); ++i) {
    CAnimEmitter2Obj &emitter = shared->emitter2Objs[i];
    totalKeys += emitter.translation.TotalKeys();
    totalKeys += emitter.rotation.TotalKeys();
    totalKeys += emitter.scale.TotalKeys();
    totalKeys += emitter.visibility.TotalKeys();
    totalKeys += emitter.particleSpeed.TotalKeys();
    totalKeys += emitter.emissionRate.TotalKeys();
    totalKeys += emitter.gravity.TotalKeys();
    totalKeys += emitter.variation.TotalKeys();
    totalKeys += emitter.latitude.TotalKeys();
    totalKeys += emitter.longitude.TotalKeys();
    totalKeys += emitter.length.TotalKeys();
    totalKeys += emitter.width.TotalKeys();
  }
  for (i = 0; i < shared->ribbonObjs.Count(); ++i) {
    CAnimRibbonObj &ribbon = shared->ribbonObjs[i];
    totalKeys += ribbon.translation.TotalKeys();
    totalKeys += ribbon.rotation.TotalKeys();
    totalKeys += ribbon.visibility.TotalKeys();
  }
  return totalKeys;
}

unsigned int AnimGetAttachmentObjId(HANIM__* anim, unsigned int index) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);
  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);
  ASSERT(index < shared->modelObjs.Count());
  return index < shared->modelObjs.Count() ? shared->modelObjs[index].animObjId : 0;
}

BOOL AnimIsAttachmentEnabled(HANIM anim, unsigned int index) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);

  ASSERT(unique);
  ASSERT(index < unique->modelStatus.Count());

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  unsigned int geosetId = shared->modelObjs[index].geosetId;

  if (!unique->modelStatus[index].IsVisible()) {
    return 0;
  }

  if (geosetId == 0xFF) {
    return 1;
  }

  if (unique->geosetStatus[geosetId].IsVisible()) {
    return 1;
  }

  return 0;
}

int AnimIsCameraEnabled(HANIM anim, unsigned int index) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);

  ASSERT(unique);
  ASSERT(index < unique->cameraStatus.Count());

  return unique->cameraStatus[index].IsVisible();
}

void AnimSetSeqFinishedHandler(HANIM anim, ANIMSEQFINISHEDHANDLER callback, void *param) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);

  ASSERT(unique);
  unique->anySeqFinished.callback = callback;
  unique->anySeqFinished.param = param;
}

void AnimSetSeqFinishedHandler(HANIM anim, unsigned int sequence, ANIMSEQFINISHEDHANDLER callback, void *param) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  CSeqOrdering &ordering = shared->seqOrder[unique->seqMapIndex];
  CVariations  &sequences = ordering.order[sequence];
  if (sequences.primary == 0xFF) {
    return;
  }

  unique->seq[sequences.primary].finished.callback = callback;
  unique->seq[sequences.primary].finished.param = param;
  unsigned int   numVariations = sequences.variation.Count();
  unsigned char *variation = sequences.variation.Ptr();
  while (numVariations) {
    unique->seq[*variation].finished.callback = callback;
    unique->seq[*variation].finished.param = param;
    ++variation;
    --numVariations;
  }
}

int AnimGetPrimarySequence(HANIM anim, unsigned int *sequence) {
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
  int            duration = sequence.time.h - sequence.time.l;
  if (!duration) {
    return 0.0f;
  }

  float completion = static_cast<float>(unique->seq[unique->primarySeq].elapsed - sequence.time.l) / static_cast<float>(duration);
  if (completion < 0.0f) {
    return 0.0f;
  }
  if (completion > 1.0f) {
    return 1.0f;
  }
  return completion;
}

int AnimNeedsSequenceBounds(HANIM anim) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);

  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  if ((shared->flags & 8) && shared->seq.Count() > 1) {
    return 1;
  }

  if ((shared->flags & 2) && shared->seq.Count() > 1) {
    return 1;
  }

  return 0;
}

void AnimSetEventCallback(HANIM anim, void(*callback)(const char *, const NTempest::C3Vector &, void *), void *param) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);

  ASSERT(unique);
  unique->appEvent.callback = callback;
  unique->appEvent.param = param;
}

int AnimObjectUsingLookAt(HANIM anim, unsigned int objectId) {
  return AnimObjectUsingLookAtType(anim, objectId, 2);
}

int AnimApplyObjectLookAt(HANIM anim, unsigned int objectId, const NTempest::C3Vector &target) {
  return ApplyObjectLookAtType(anim, objectId, target, 2);
}

int AnimRemoveObjectLookAt(HANIM anim, unsigned int objectId) {
  return RemoveObjectLookAtType(anim, objectId, 2);
}

int AnimObjectUsingFaceDir(HANIM anim, unsigned int objectId) {
  return AnimObjectUsingLookAtType(anim, objectId, 0x40);
}

int AnimApplyObjectFaceDir(HANIM anim, unsigned int objectId, NTempest::C3Vector direction) {
  float magnitude = direction.Mag();
  if (NTempest::CMath::fabs_(magnitude) >= 0.00000023841858f) {
    float inverseMagnitude = 1.0f / magnitude;
    direction.x *= inverseMagnitude;
    direction.y *= inverseMagnitude;
    direction.z *= inverseMagnitude;
  }
  return ApplyObjectLookAtType(anim, objectId, direction, 0x40);
}

int AnimRemoveObjectFaceDir(HANIM anim, unsigned int objectId) {
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
    unsigned int numObjects = unique->status.Count();
    unique->blendStatus.ReserveSpace(numObjects);
    unique->blendStatus.SetCount(numObjects);
  } else {
    unique->flags &= ~0x10;
    unique->blendStatus.ReserveSpace(0);
    unique->blendStatus.Clear();
  }
}

int AnimMarkFootstepSequence(HANIM anim, unsigned int index) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);
  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  unsigned int sequence = shared->seqOrder[unique->seqMapIndex].order[index].primary;
  if (sequence == 0xFF) {
    return 0;
  }
  ASSERT(sequence < shared->seq.Count());
  shared->seq[sequence].flags |= 2;
  return 1;
}

int AnimLockObjectSequence(HANIM anim, unsigned int objectId, int set) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);
  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);
  ASSERT(objectId < shared->objectOrder.Count());

  unsigned int sharedObjectId = shared->objectOrder[objectId];
  if (sharedObjectId == static_cast<unsigned int>(-1)) {
    return 0;
  }
  ASSERT(sharedObjectId < shared->obj.Count());
  CAnimObjStatus *status = unique->status[sharedObjectId];
  if (set) {
    status->base.flags |= 0x20;
  } else {
    status->base.flags &= ~0x20;
  }
  return 1;
}

int AnimHasObjectId(HANIM anim, unsigned int objectId) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);
  ASSERT(objectId < shared->objectOrder.Count());

  return shared->objectOrder[objectId] != static_cast<unsigned int>(-1);
}

int AnimEventEmitterHasKeysThisSeq(HANIM anim, unsigned int objectId) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);
  ASSERT(objectId < shared->objectOrder.Count());

  objectId = shared->objectOrder[objectId];
  if (objectId == static_cast<unsigned int>(-1)) {
    return 0;
  }

  ASSERT(objectId < shared->obj.Count());
  CAnimObj *currobj = shared->obj[objectId];
  ASSERT(currobj);
  ASSERT(currobj->type == 6);
  ASSERT(objectId < unique->status.Count());
  CAnimObjStatus *status = unique->status[objectId];
  ASSERT(status);

  CAnimEventObj *eventObject = static_cast<CAnimEventObj *>(currobj);
  unsigned int   sequence = status->base.currSeq;
  if (!eventObject->events.SequenceChanges()) {
    return eventObject->events.TotalKeys() != 0;
  }
  return eventObject->events.NumKeysThisSeq(sequence) != 0;
}

int
AnimGetObjectPosition(HANIM anim, unsigned int objectId, const TSFixedArray<NTempest::C3Vector> &positions, NTempest::C3Vector *position) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);
  ASSERT(objectId < shared->objectOrder.Count());

  objectId = shared->objectOrder[objectId];
  if (objectId == static_cast<unsigned int>(-1)) {
    return 0;
  }

  ASSERT(objectId < positions.Count());
  *position = positions[objectId];
  return 1;
}

int AnimGetEventObjectPosition(HANIM anim, unsigned int objectId, NTempest::C3Vector *position) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);
  ASSERT(objectId < shared->objectOrder.Count());

  objectId = shared->objectOrder[objectId];
  if (objectId == static_cast<unsigned int>(-1)) {
    return 0;
  }

  ASSERT(objectId < shared->obj.Count());
  ASSERT(shared->obj[objectId]->type == 6);
  ASSERT(objectId < unique->status.Count());
  *position = static_cast<CAnimEventObjStatus *>(unique->status[objectId])->position;
  return 1;
}
