#include "Anim/AnimInternal.h"

#include "Anim/WorldMatrix.h"
#include "Base/Activity.h"
#include "Model/ModelInternal.h"
#include "Os/OsTime.h"
#include "Services/ParticleSystem2.h"
#include "Gx/CGxDevice.h"
#include "Gxu/IGxuLight.h"
#include "Tempest/cmath.h"
#include "Tempest/c4quaternion.h"

#include <stdlib.h>

namespace NTempest {

  C4QuaternionCompressed::operator C4Quaternion() const {
    const unsigned __int64 data = static_cast<unsigned __int64>(m_data);
    const int              xBits = static_cast<int>(data >> 32) >> 10;
    const int              yBits = static_cast<int>(static_cast<unsigned int>(data >> 10)) >> 11;
    const int              zBits = static_cast<int>(static_cast<unsigned int>(data) << 11) >> 11;
    const float            x = static_cast<float>(xBits) * 0.00000047683716f;
    const float            y = static_cast<float>(yBits) * 0.00000095367432f;
    const float            z = static_cast<float>(zBits) * 0.00000095367432f;
    const float            magnitude = x * x + y * y + z * z;
    const float            w = CMath::fabs_(magnitude - 1.0f) < 0.00000095367432f ? 0.0f : CMath::sqrt_(1.0f - magnitude);
    return C4Quaternion(w, x, y, z);
  }

}  // namespace NTempest

static float         s_timeScale = 1.0f;
static unsigned int  s_animFlags;
static unsigned int  s_lastFrame;
static unsigned long s_currTime;
static unsigned int  s_elapsedTime;

static void __fastcall RemoveTranslation(const NTempest::C3Vector &position) {
  WorldMatrixRemove(1);
  WorldMatrixTranslate(position);
}

static void __fastcall RemoveScale(const NTempest::C3Vector &scale) {
  WorldMatrixRemove(2);
  WorldMatrixScale(scale);
}

static float __fastcall Length(const NTempest::C3Vector &v) {
  return NTempest::CMath::sqrt_(v.x * v.x + v.y * v.y + v.z * v.z);
}

static void __fastcall InvScale(NTempest::C3Vector *v, float factor) {
  if (factor != 0.0f) {
    float inverse = 1.0f / factor;

    v->x *= inverse;
    v->y *= inverse;
    v->z *= inverse;
  }
}

static int __fastcall WrapAnimTime(int milliseconds, int looptime) {
  if (!looptime) {
    return 0;
  }

  if (milliseconds < 0) {
    return looptime - (-milliseconds % looptime);
  }

  return milliseconds % looptime;
}

static void __fastcall ISetSequenceInfo(CAnim *unique, CBaseStatus *status, unsigned int index, int resetTime) {
  if (resetTime || index != status->currSeq) {
    status->flags |= 0x10;
  }

  if (!(status->flags & 8) || index != status->currSeq) {
    if (status->flags & 8) {
      --unique->seq[status->currSeq].useCount;
    }

    ++unique->seq[index].useCount;
    ASSERT(index < 0xFF);
    status->currSeq = index;
    status->flags |= 8;
  }
}

static void __fastcall ISetSequenceInfo(CAnim *unique, CBaseStatus *status, unsigned int index, unsigned int prevIndex, int resetTime) {
  if (status->currSeq == prevIndex) {
    ISetSequenceInfo(unique, status, index, resetTime);
  }
}

static void __fastcall ISetSequenceReset(CAnim *unique, CAnimObj *currobj, unsigned int index, unsigned int blendTime, int resetTime) {
  CAnimObjStatus *status = unique->status[currobj->animObjId];
  if (!(status->base.flags & 0x20)) {
    ISetSequenceInfo(unique, &status->base, index, resetTime);

    if ((unique->flags & 0x10) && (status->base.flags & 0x10)) {
      unique->blendStatus[currobj->animObjId].blendTimer = blendTime;
    }

    unsigned int numChildren = currobj->childarray.Count();
    for (unsigned int childIndex = 0; childIndex < numChildren; ++childIndex) {
      ISetSequenceReset(unique, currobj->childarray[childIndex], index, blendTime, resetTime);
    }
  }
}

static void __fastcall
ISetSequence(CAnim *unique, CAnimObj *currobj, unsigned int index, unsigned int prevIndex, unsigned int blendTime, int resetTime) {
  CAnimObjStatus *status = unique->status[currobj->animObjId];
  if (!(status->base.flags & 0x20)) {
    if (status->base.currSeq == prevIndex) {
      ISetSequenceInfo(unique, &status->base, index, resetTime);

      if ((unique->flags & 0x10) && (status->base.flags & 0x10)) {
        unique->blendStatus[currobj->animObjId].blendTimer = blendTime;
      }
    }

    unsigned int numChildren = currobj->childarray.Count();
    for (unsigned int childIndex = 0; childIndex < numChildren; ++childIndex) {
      ISetSequence(unique, currobj->childarray[childIndex], index, prevIndex, blendTime, resetTime);
    }
  }
}

static void __fastcall RemoveRotation(const InterpInfo &animInfo) {
  WorldMatrixRemove(4);
  WorldMatrixBasis(animInfo.basisX, animInfo.basisY, animInfo.basisZ);
}

static void __fastcall RemoveRotationAndScaling(const InterpInfo &animInfo) {
  WorldMatrixRemove(6);
  WorldMatrixScale(animInfo.basisScale);
  WorldMatrixBasis(animInfo.basisX, animInfo.basisY, animInfo.basisZ);
}

void __fastcall TranslateView(const InterpInfo &animInfo, CAnimObj *currobj, const NTempest::C3Vector &currPos, const NTempest::C3Vector &parentPos) {
  ASSERT(currobj);
  CAnimObjStatus    *status = animInfo.unique->status[currobj->animObjId];
  NTempest::C3Vector transform(0.0f);
  currobj->translation.InterpolateVolatile(animInfo, status->base, &status->translation, NTempest::C3Vector(0.0f), &transform);

  if (animInfo.unique->flags & 0x10) {
    CAnimObjBlendStatus *blend = &animInfo.unique->blendStatus[currobj->animObjId];
    if (blend->blendTimer > 0) {
      Blend(blend->prevSeqPosition, &transform, blend->blendTimer, animInfo.shared->seq[status->base.currSeq].blendTime);
    }
    blend->blendPosition = transform;
  }

  if (currobj->flags & 1) {
    RemoveTranslation(animInfo.basisPosition);
    switch (currobj->flags & 6) {
      case 2:
        RemoveScale(animInfo.basisScale);
        break;
      case 4:
        RemoveRotation(animInfo);
        break;
      case 6:
        RemoveRotationAndScaling(animInfo);
        break;
    }
    transform += parentPos;
  }

  transform += currPos - parentPos;
  WorldMatrixTranslate(transform);
}

void __fastcall RotateView(const InterpInfo &animInfo, CAnimObj *currobj) {
  ASSERT(currobj);
  if ((currobj->flags & 7) == 4) {
    RemoveRotation(animInfo);
  }

  CAnimObjStatus        *status = animInfo.unique->status[currobj->animObjId];
  NTempest::C4Quaternion transform;
  int animate = currobj->rotation.InterpolateVolatile(animInfo, status->base, &status->rotation, NTempest::C4Quaternion(), &transform);
  int timeLeft = 0;

  if (animInfo.unique->flags & 0x10) {
    CAnimObjBlendStatus *blend = &animInfo.unique->blendStatus[currobj->animObjId];
    timeLeft = blend->blendTimer;
    if (timeLeft > 0) {
      Blend(blend->prevSeqRotation, &transform, timeLeft, animInfo.shared->seq[status->base.currSeq].blendTime);
    }
    blend->blendRotation = transform;
  }

  if (animate || timeLeft > 0) {
    WorldMatrixRotate(transform);
  }
}

void __fastcall ScaleView(const InterpInfo &animInfo, CAnimObj *currobj) {
  ASSERT(currobj);
  if ((currobj->flags & 3) == 2) {
    if (currobj->flags & 4) {
      RemoveRotationAndScaling(animInfo);
    } else {
      RemoveScale(animInfo.basisScale);
    }
  }

  CAnimObjStatus    *status = animInfo.unique->status[currobj->animObjId];
  NTempest::C3Vector transform(1.0f);
  int                animate = currobj->scale.InterpolateVolatile(animInfo, status->base, &status->scale, NTempest::C3Vector(1.0f), &transform);
  int                timeLeft = 0;

  if (animInfo.unique->flags & 0x10) {
    CAnimObjBlendStatus *blend = &animInfo.unique->blendStatus[currobj->animObjId];
    timeLeft = blend->blendTimer;
    if (timeLeft > 0) {
      Blend(blend->prevSeqScale, &transform, timeLeft, animInfo.shared->seq[status->base.currSeq].blendTime);
    }
    blend->blendScale = transform;
  }

  if (animate || timeLeft > 0) {
    WorldMatrixScale(transform);
  }
}

static void __fastcall SetGlobalSequenceTime(CAnim *anim, CAnimData *data, int elapsedTime) {
  ASSERT(anim);
  ASSERT(data);

  for (unsigned int i = 0; i < anim->globalSeqElapsed.Count(); ++i) {
    const unsigned int length = data->globalSeqLength[i];
    anim->globalSeqElapsed[i] = length ? (anim->globalSeqElapsed[i] + elapsedTime) % length : 0;
  }
}

static int __fastcall CallSeqFinishedHandlers(CAnim *unique, unsigned int seqIndex) {
  ASSERT(unique);

  if (unique->anySeqFinished.callback && !unique->anySeqFinished.callback(unique->anySeqFinished.param)) {
    return 0;
  }

  if (!unique->seq[seqIndex].finished.callback) {
    return 1;
  }

  return unique->seq[seqIndex].finished.callback(unique->seq[seqIndex].finished.param) != 0;
}

static int __fastcall SetSequenceTime(CAnim *unique, unsigned int sequence, const CAnimSequence *seq, CSeqInfo *seqInfo, int seqTime) {
  ASSERT(unique);

  if (!seqInfo->useCount) {
    return 1;
  }

  int   loopTime = seq->time.h - seq->time.l;
  int   callback = 0;
  float timeScale = s_timeScale * unique->seq[sequence].seqTimeScale;

  if (timeScale < 0.0f ? seqTime < 0 : seqTime >= loopTime) {
    int seqFinished = seqInfo->seqFinished;
    if (seq->flags & 1) {
      seqInfo->seqFinished = 1;
    }
    callback = !seqFinished;
  } else {
    seqInfo->seqFinished = 0;
  }

  if (seq->flags & 1) {
    if (seqTime > loopTime) {
      seqTime = loopTime;
    } else if (seqTime < 0) {
      seqTime = 0;
    }
  } else {
    seqTime = WrapAnimTime(seqTime, loopTime);
  }

  seqInfo->elapsed = seq->time.l + seqTime;

  if (!callback) {
    return 1;
  }

  if (seqInfo->replayTimes) {
    --seqInfo->replayTimes;
    return 1;
  }

  ActivityBegin(ACTIVITY_ANIMSEQEND);
  int result = CallSeqFinishedHandlers(unique, sequence);
  ActivityEnd(ACTIVITY_ANIMSEQEND);
  return result;
}

static int __fastcall GetSeqSyncTime(CAnim *unique, CAnimData *shared, unsigned int currSeq, unsigned int prevSeq) {
  CAnimSequence *currSharedSeq = &shared->seq[currSeq];

  if (!(currSharedSeq->flags & 2) || !(shared->seq[prevSeq].flags & 2)) {
    return 0;
  }

  const float syncTime = static_cast<float>(unique->seq[prevSeq].elapsed - shared->seq[prevSeq].time.l) /
                         static_cast<float>(shared->seq[prevSeq].time.h - shared->seq[prevSeq].time.l) *
                         static_cast<float>(currSharedSeq->time.h - currSharedSeq->time.l);
  return syncTime < 0.0f ? -static_cast<int>(-syncTime + 0.5f) : static_cast<int>(syncTime + 0.5f);
}

static void __fastcall SetObjectSequencesReset(CAnim *unique, CAnimData *shared, unsigned int sequence, unsigned int blendTime, int resetTime) {
  CAnimObj **currobj = shared->headarray.Ptr();
  for (unsigned int objectIndex = 0; objectIndex < shared->headarray.Count(); ++objectIndex, ++currobj) {
    ISetSequenceReset(unique, *currobj, sequence, blendTime, resetTime);
  }

  for (unsigned int cameraIndex = 0; cameraIndex < unique->cameraStatus.Count(); ++cameraIndex) {
    ISetSequenceInfo(unique, &unique->cameraStatus[cameraIndex].base, sequence, resetTime);
  }

  for (unsigned int textureIndex = 0; textureIndex < unique->textureStatus.Count(); ++textureIndex) {
    ISetSequenceInfo(unique, &unique->textureStatus[textureIndex].base, sequence, resetTime);
  }

  for (unsigned int geosetIndex = 0; geosetIndex < unique->geosetStatus.Count(); ++geosetIndex) {
    ISetSequenceInfo(unique, &unique->geosetStatus[geosetIndex].base, sequence, resetTime);
  }

  for (unsigned int layerIndex = 0; layerIndex < unique->layerStatus.Count(); ++layerIndex) {
    ISetSequenceInfo(unique, &unique->layerStatus[layerIndex].base, sequence, resetTime);
  }
}

static void __fastcall
SetObjectSequences(CAnim *unique, CAnimData *shared, unsigned int sequence, unsigned int prevSeq, unsigned int blendTime, int resetTime) {
  CAnimObj **currobj = shared->headarray.Ptr();
  for (unsigned int objectIndex = 0; objectIndex < shared->headarray.Count(); ++objectIndex, ++currobj) {
    ISetSequence(unique, *currobj, sequence, prevSeq, blendTime, resetTime);
  }

  for (unsigned int cameraIndex = 0; cameraIndex < unique->cameraStatus.Count(); ++cameraIndex) {
    ISetSequenceInfo(unique, &unique->cameraStatus[cameraIndex].base, sequence, prevSeq, resetTime);
  }

  for (unsigned int textureIndex = 0; textureIndex < unique->textureStatus.Count(); ++textureIndex) {
    ISetSequenceInfo(unique, &unique->textureStatus[textureIndex].base, sequence, prevSeq, resetTime);
  }

  for (unsigned int geosetIndex = 0; geosetIndex < unique->geosetStatus.Count(); ++geosetIndex) {
    ISetSequenceInfo(unique, &unique->geosetStatus[geosetIndex].base, sequence, prevSeq, resetTime);
  }

  for (unsigned int layerIndex = 0; layerIndex < unique->layerStatus.Count(); ++layerIndex) {
    ISetSequenceInfo(unique, &unique->layerStatus[layerIndex].base, sequence, prevSeq, resetTime);
  }
}

static int __fastcall RandomInRange(const NTempest::CiRange &range) {
  int delta = range.h - range.l;
  if (!delta) {
    return range.l;
  }

  return range.l + (rand() >> 2) % delta;
}

static void __fastcall SetSequence(CAnim *unique, CAnimData *shared, unsigned char sequence, unsigned int flags) {
  ASSERT(unique);

  const unsigned int numSeqs = unique->seq.Count();
  if (sequence >= numSeqs) {
    return;
  }

  if (flags & 8) {
    unique->flags |= 0x20;
  }

  const unsigned int prevSeq = unique->primarySeq;
  const int          seqStartTime = shared->seq[sequence].time.l;
  const int          resetTime = !(flags & 2);
  const int          wasInited = unique->flags & 0x40;

  if (!wasInited || !(unique->flags & 0x10)) {
    flags |= 4;
  }

  unique->primarySeq = sequence;
  unique->flags |= 0x40;

  const unsigned int blendTime = (flags & 4) ? 0 : shared->seq[sequence].blendTime;
  if (flags & 1) {
    SetObjectSequencesReset(unique, shared, sequence, blendTime, resetTime);
  } else {
    SetObjectSequences(unique, shared, sequence, prevSeq, blendTime, resetTime);
  }

  if (resetTime) {
    int syncTime = 0;
    if (wasInited) {
      syncTime = GetSeqSyncTime(unique, shared, sequence, prevSeq);
    }

    unique->seq[sequence].elapsed = seqStartTime + syncTime;
    unique->seq[sequence].replayTimes = RandomInRange(shared->seq[sequence].replay);
  }

  for (unsigned int i = 0; i < numSeqs; ++i) {
    if (!unique->seq[i].useCount || (i == sequence && resetTime)) {
      unique->seq[i].seqFinished = 0;
    }
  }

  unique->flags &= ~5;
}

void __fastcall GetWorldTransform(InterpInfo *animInfo) {
  WorldMatrixGetRow(0, &animInfo->basisX);
  animInfo->basisScale.x = Length(animInfo->basisX);
  InvScale(&animInfo->basisX, animInfo->basisScale.x);

  WorldMatrixGetRow(1, &animInfo->basisY);
  animInfo->basisScale.y = Length(animInfo->basisY);
  InvScale(&animInfo->basisY, animInfo->basisScale.y);

  WorldMatrixGetRow(2, &animInfo->basisZ);
  animInfo->basisScale.z = Length(animInfo->basisZ);
  InvScale(&animInfo->basisZ, animInfo->basisScale.z);

  WorldMatrixGetRow(3, &animInfo->basisPosition);
}

static int __fastcall AdvanceTime(CAnim *unique, CAnimData *shared) {
  ASSERT(unique);
  ASSERT(shared);

  float fTimeElapsed;
  if (unique->flags & 0x20) {
    unique->flags &= ~0x20;
    fTimeElapsed = 0.0f;
  } else {
    fTimeElapsed = static_cast<int>(s_currTime - unique->seqLastTime) * s_timeScale;
  }

  unique->seqLastTime = s_currTime;
  const unsigned int numSequences = unique->seq.Count();

  if ((s_animFlags & 8) || (unique->flags & 8)) {
    fTimeElapsed = 0.0f;
    for (unsigned int pausedSeqIndex = 0; pausedSeqIndex < numSequences; ++pausedSeqIndex) {
      unique->seq[pausedSeqIndex].scaledElapsedTime = 0;
    }
  } else {
    const int elapsedTime = fTimeElapsed < 0.0f ? -static_cast<int>(NTempest::CMath::fuint_n(-fTimeElapsed)) : static_cast<int>(fTimeElapsed + 0.5f);
    SetGlobalSequenceTime(unique, shared, elapsedTime);

    for (unsigned int seqIndex = 0; seqIndex < numSequences; ++seqIndex) {
      CSeqInfo      &seqInfo = unique->seq[seqIndex];
      CAnimSequence &sequence = shared->seq[seqIndex];
      const float    scaled = fTimeElapsed * seqInfo.seqTimeScale;
      seqInfo.scaledElapsedTime = scaled < 0.0f ? -static_cast<int>(NTempest::CMath::fuint_n(-scaled)) : static_cast<int>(scaled + 0.5f);

      const int seqTime = seqInfo.elapsed + seqInfo.scaledElapsedTime - sequence.time.l;
      if (!SetSequenceTime(unique, seqIndex, &sequence, &seqInfo, seqTime)) {
        return 0;
      }
    }
  }

  const float elapsedSeconds = fTimeElapsed * 0.001f;
  for (unsigned int emitterIndex = 0; emitterIndex < unique->emitter2Status.Count(); ++emitterIndex) {
    unique->emitter2Status[emitterIndex].elapsedTime = elapsedSeconds;
  }

  for (unsigned int ribbonIndex = 0; ribbonIndex < unique->ribbonStatus.Count(); ++ribbonIndex) {
    unique->ribbonStatus[ribbonIndex].elapsedTime = elapsedSeconds;
  }

  return 1;
}

static int __fastcall PickRandomSequence(const CVariations &selection, const CArray<CAnimSequence> &seqs) {
  if (!selection.variation.Count()) {
    return selection.primary;
  }

  unsigned int draw = rand();
  int          result = selection.primary;
  unsigned int chance = seqs[result].randPickChance;
  if (draw < chance) {
    return selection.primary;
  }

  draw -= chance;
  result = 0;
  unsigned int count = selection.variation.Count();
  for (unsigned int i = 0; i < count; ++i) {
    result = selection.variation[i];
    chance = seqs[result].randPickChance;
    if (draw < chance) {
      break;
    }

    draw -= chance;
  }

  return result;
}

static void __fastcall SetSplitBodySequence(CAnim *unique, CAnimData *shared, unsigned int sequence, unsigned int objectId, unsigned int flags) {
  if (!(flags & 2)) {
    CAnimObj       *object = shared->obj[objectId];
    CAnimObjStatus *status = unique->status[object->animObjId];
    int             timeOffset = 0;
    if (status->base.flags & 8) {
      timeOffset = GetSeqSyncTime(unique, shared, sequence, status->base.currSeq);
    }

    unique->seq[sequence].elapsed = shared->seq[sequence].time.l + timeOffset;
    unique->seq[sequence].seqFinished = 0;
  }

  flags |= 1;
  if (!(unique->flags & 0x10)) {
    flags |= 4;
  }

  unsigned int blendTime = (flags & 4) ? 0 : shared->seq[sequence].blendTime;
  ISetSequenceReset(unique, shared->obj[objectId], sequence, blendTime, !(flags & 2));

  for (unsigned int i = 0; i < unique->seq.Count(); ++i) {
    if (!unique->seq[i].useCount) {
      unique->seq[i].seqFinished = 0;
    }
  }
  unique->flags &= ~5;
}

static unsigned int __fastcall FindSequenceVariationInUse(CAnim *unique, CAnimData *shared, unsigned int index) {
  CVariations &variations = shared->seqOrder[unique->seqMapIndex].order[index];
  unsigned int sequence = variations.primary;

  if (sequence == 0xFF) {
    return 0xFF;
  }

  if (unique->seq[sequence].useCount) {
    return sequence;
  }

  unsigned int numVariations = variations.variation.Count();
  for (unsigned int i = 0; i < numVariations; ++i) {
    sequence = variations.variation[i];
    if (unique->seq[sequence].useCount) {
      return sequence;
    }
  }

  return 0xFF;
}

void __fastcall IAnimInitializeTime() {
  s_lastFrame = (unsigned int)-1;
  s_currTime = OsGetAsyncTimeMsPrecise();
  s_elapsedTime = 0;
}

unsigned long __fastcall IAnimGetCurrTimeMs() {
  return s_currTime;
}

void __fastcall AnimSetTimeScale(HANIM anim, float timeScale) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);
  unique->seq[unique->primarySeq].seqTimeScale = timeScale;
}

int __fastcall AnimSetObjectTimeScale(HANIM anim, unsigned int objectId, float timeScale) {
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
  CAnimObj    *object = shared->obj[sharedObjectId];
  unsigned int sequence = unique->status[object->animObjId]->base.currSeq;
  unique->seq[sequence].seqTimeScale = timeScale;
  return 1;
}

float __fastcall AnimGetTimeScale(HANIM anim) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);
  return unique->seq[unique->primarySeq].seqTimeScale;
}

float __fastcall AnimGetObjectTimeScale(HANIM anim, unsigned int objectId) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);
  ASSERT(objectId < shared->objectOrder.Count());

  unsigned int sharedObjectId = shared->objectOrder[objectId];
  if (sharedObjectId == static_cast<unsigned int>(-1)) {
    return 1.0f;
  }

  ASSERT(sharedObjectId < shared->obj.Count());
  CAnimObj    *object = shared->obj[sharedObjectId];
  unsigned int sequence = unique->status[object->animObjId]->base.currSeq;
  return unique->seq[sequence].seqTimeScale;
}

int __fastcall AnimForceCurrentSequenceTime(HANIM anim, int time) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  unsigned int sequence = unique->primarySeq;
  unique->flags |= 0x20;
  return SetSequenceTime(unique, sequence, &shared->seq[sequence], &unique->seq[sequence], time);
}

int __fastcall AnimForceSequenceTime(HANIM anim, unsigned int index, int time) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  unsigned int sequence = FindSequenceVariationInUse(unique, shared, index);
  if (sequence == 0xFF) {
    return 0;
  }

  unique->flags |= 0x20;
  return SetSequenceTime(unique, sequence, &shared->seq[sequence], &unique->seq[sequence], time);
}

int __fastcall AnimSetRandomSequenceFidget(HANIM anim, unsigned int seqIndex, unsigned int flags) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);
  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  CVariations &selection = shared->seqOrder[unique->seqMapIndex].order[seqIndex];
  if (selection.primary == 0xFF) {
    SErrSetLastError(ERROR_INVALID_FUNCTION);
    return 0;
  }

  unsigned int sequence = PickRandomSequence(selection, shared->seq);
  ASSERT(sequence < 0xFF);
  SetSequence(unique, shared, static_cast<unsigned char>(sequence), flags);
  return 1;
}

unsigned int __fastcall AnimGetNumSequenceFidgets(HANIM anim, unsigned int seqIndex) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  CVariations &ordering = shared->seqOrder[unique->seqMapIndex].order[seqIndex];
  if (ordering.primary == 0xFF) {
    return 0;
  }
  return ordering.variation.Count() + 1;
}

int __fastcall AnimSetRandomSequenceFidget(HANIM anim, unsigned int seqIndex, unsigned int objectId, unsigned int flags) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);
  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  CVariations &selection = shared->seqOrder[unique->seqMapIndex].order[seqIndex];
  if (selection.primary == 0xFF) {
    SErrSetLastError(ERROR_INVALID_FUNCTION);
    return 0;
  }

  unsigned int sharedObjectId = shared->objectOrder[objectId];
  if (sharedObjectId == static_cast<unsigned int>(-1)) {
    SErrSetLastError(ERROR_FILE_NOT_FOUND);
    return 0;
  }

  unsigned int sequence = PickRandomSequence(selection, shared->seq);
  SetSplitBodySequence(unique, shared, sequence, sharedObjectId, flags);
  return 1;
}

int __fastcall AnimSetSequenceFidget(HANIM anim, unsigned int seqIndex, unsigned int fidgetId, unsigned int flags) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);
  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  CVariations &selection = shared->seqOrder[unique->seqMapIndex].order[seqIndex];
  if (selection.primary == 0xFF) {
    SErrSetLastError(ERROR_INVALID_FUNCTION);
    return 0;
  }

  unsigned int sequence = fidgetId ? selection.variation[fidgetId - 1] : selection.primary;
  ASSERT(sequence < 0xFF);
  SetSequence(unique, shared, static_cast<unsigned char>(sequence), flags);
  return 1;
}

int __fastcall AnimSetSequenceFidget(HANIM anim, unsigned int seqIndex, unsigned int fidgetId, unsigned int objectId, unsigned int flags) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);
  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  CVariations &selection = shared->seqOrder[unique->seqMapIndex].order[seqIndex];
  if (selection.primary == 0xFF) {
    SErrSetLastError(ERROR_INVALID_FUNCTION);
    return 0;
  }

  unsigned int sharedObjectId = shared->objectOrder[objectId];
  if (sharedObjectId == static_cast<unsigned int>(-1)) {
    SErrSetLastError(ERROR_FILE_NOT_FOUND);
    return 0;
  }

  unsigned int sequence = fidgetId ? selection.variation[fidgetId - 1] : selection.primary;
  SetSplitBodySequence(unique, shared, sequence, sharedObjectId, flags);
  return 1;
}

int __fastcall AnimSetSequence(HANIM anim, unsigned int seqIndex, unsigned int flags) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  CSeqOrdering      &ordering = shared->seqOrder[unique->seqMapIndex];
  unsigned int       sequence = seqIndex;
  const unsigned int count = ordering.order.Count();

  if (seqIndex || count) {
    ASSERT(seqIndex < count);
    sequence = ordering.order[seqIndex].primary;
  }

  if (sequence == 0xFF) {
    SErrSetLastError(ERROR_INVALID_FUNCTION);
    return 0;
  }

  ASSERT(!sequence || sequence < shared->seq.Count());
  ASSERT(sequence < 0xFF);

  SetSequence(unique, shared, static_cast<unsigned char>(sequence), flags);
  return 1;
}

int __fastcall AnimSetSequence(HANIM anim, unsigned int seqIndex, unsigned int objectId, unsigned int flags) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  CSeqOrdering &ordering = shared->seqOrder[unique->seqMapIndex];
  unsigned int  sequence = ordering.order[seqIndex].primary;
  unsigned int  sharedObjectId = shared->objectOrder[objectId];
  if (sequence == 0xFF) {
    SErrSetLastError(ERROR_INVALID_FUNCTION);
    return 0;
  }

  if (sharedObjectId == static_cast<unsigned int>(-1)) {
    SErrSetLastError(ERROR_FILE_NOT_FOUND);
    return 0;
  }

  SetSplitBodySequence(unique, shared, sequence, sharedObjectId, flags);
  return 1;
}

int __fastcall AnimMatchSequence(HANIM anim, unsigned int objectId, unsigned int sameAsObjectId, unsigned int flags) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);
  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);
  ASSERT(objectId < shared->objectOrder.Count());
  ASSERT(sameAsObjectId < shared->objectOrder.Count());

  unsigned int sharedObjectId = shared->objectOrder[objectId];
  unsigned int sharedSameAsObjectId = shared->objectOrder[sameAsObjectId];
  if (sharedObjectId == static_cast<unsigned int>(-1) || sharedSameAsObjectId == static_cast<unsigned int>(-1)) {
    SErrSetLastError(ERROR_FILE_NOT_FOUND);
    return 0;
  }

  ASSERT(sharedObjectId < shared->obj.Count());
  ASSERT(sharedSameAsObjectId < shared->obj.Count());
  unsigned int sequence = unique->status[shared->obj[sharedSameAsObjectId]->animObjId]->base.currSeq;
  SetSplitBodySequence(unique, shared, sequence, sharedObjectId, flags);
  return 1;
}

int __fastcall AnimGetSequenceTime(HANIM anim, unsigned int seqIndex) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  FATALASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  FATALASSERT(shared);

  CVariations &selection = shared->seqOrder[unique->seqMapIndex].order[seqIndex];
  if (selection.primary == 0xFF) {
    return 0;
  }

  return unique->seq[selection.primary].elapsed - shared->seq[selection.primary].time.l;
}

void __fastcall AnimResetGlobalSequenceTimes(HANIM anim) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);
  memset(unique->globalSeqElapsed.m_data, 0, unique->globalSeqElapsed.Count() * sizeof(unsigned int));
}

int __fastcall AnimAdvanceTime(HANIM anim, unsigned int currentFrame) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  FATALASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  FATALASSERT(shared);

  if (currentFrame != s_lastFrame) {
    const unsigned long currentTime = OsGetAsyncTimeMsPrecise();
    s_elapsedTime = currentTime - s_currTime;
    s_currTime = currentTime;
    s_lastFrame = currentFrame;
  }

  return AdvanceTime(unique, shared);
}

static int __fastcall IAnimManualAdvanceTime(CAnim *unique, CAnimData *shared, int timeChange) {
  s_elapsedTime = timeChange;
  s_currTime = unique->seqLastTime + timeChange;
  return AdvanceTime(unique, shared);
}

int __fastcall AnimManualAdvanceTime(HANIM anim, int timeChange) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);
  return IAnimManualAdvanceTime(unique, shared, timeChange);
}

unsigned int __fastcall AnimGetElapsedTime() {
  return s_elapsedTime;
}

void __fastcall AnimPauseTime(HANIM anim, int pause) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);
  if (pause) {
    unique->flags |= 8;
  } else {
    unique->flags &= ~8;
  }
}
