#include <Base/Base.h>

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

typedef BYTE uint8;

static float s_timeScale = 1.0f;
static UINT  s_animFlags;
static UINT  s_lastFrame;
static DWORD s_currTime;
static UINT  s_elapsedTime;

static void RemoveTranslation(const NTempest::C3Vector &position) {
  WorldMatrixRemove(1);
  WorldMatrixTranslate(position);
}

static void RemoveScale(const NTempest::C3Vector &scale) {
  WorldMatrixRemove(2);
  WorldMatrixScale(scale);
}

static float Length(const NTempest::C3Vector &v) {
  return NTempest::CMath::sqrt_(v.x * v.x + v.y * v.y + v.z * v.z);
}

static void InvScale(NTempest::C3Vector *v, float factor) {
  if (factor != 0.0f) {
    float inverse = 1.0f / factor;

    v->x *= inverse;
    v->y *= inverse;
    v->z *= inverse;
  }
}

static int WrapAnimTime(int milliseconds, int looptime) {
  if (!looptime) {
    return 0;
  }

  if (milliseconds >= 0) {
    return milliseconds % looptime;
  }

  return looptime - (-milliseconds % looptime);
}

static void ISetSequenceInfo(CAnim *unique, CBaseStatus *status, UINT index, int resetTime) {
  if (resetTime || index != status->currSeq) {
    status->flags |= 0x10;
  }

  if (!(status->flags & 8) || index != status->currSeq) {
    if (status->flags & 8) {
      --unique->seq[status->currSeq].useCount;
    }

    ++unique->seq[index].useCount;
    ASSERT(index < uint8(0xff));
    status->currSeq = index;
    status->flags |= 8;
  }
}

static void ISetSequenceInfo(CAnim *unique, CBaseStatus *status, UINT index, UINT prevIndex, int resetTime) {
  if (status->currSeq == prevIndex) {
    ISetSequenceInfo(unique, status, index, resetTime);
  }
}

static void ISetSequenceReset(CAnim *unique, CAnimObj *currobj, UINT index, UINT blendTime, int resetTime) {
  CAnimObjStatus *status = unique->status[currobj->animObjId];
  if (!(status->base.flags & 0x20)) {
    ISetSequenceInfo(unique, &status->base, index, resetTime);

    if ((unique->flags & 0x10) && (status->base.flags & 0x10)) {
      unique->blendStatus[currobj->animObjId].blendTimer = blendTime;
    }

    UINT numChildren = currobj->childarray.Count();
    for (UINT childIndex = 0; childIndex < numChildren; ++childIndex) {
      ISetSequenceReset(unique, currobj->childarray[childIndex], index, blendTime, resetTime);
    }
  }
}

static void ISetSequence(CAnim *unique, CAnimObj *currobj, UINT index, UINT prevIndex, UINT blendTime, int resetTime) {
  CAnimObjStatus *status = unique->status[currobj->animObjId];
  if (!(status->base.flags & 0x20)) {
    if (status->base.currSeq == prevIndex) {
      ISetSequenceInfo(unique, &status->base, index, resetTime);

      if ((unique->flags & 0x10) && (status->base.flags & 0x10)) {
        unique->blendStatus[currobj->animObjId].blendTimer = blendTime;
      }
    }

    UINT numChildren = currobj->childarray.Count();
    for (UINT childIndex = 0; childIndex < numChildren; ++childIndex) {
      ISetSequence(unique, currobj->childarray[childIndex], index, prevIndex, blendTime, resetTime);
    }
  }
}

void RemoveRotation(const InterpInfo &animInfo) {
  WorldMatrixRemove(4);
  WorldMatrixBasis(animInfo.basisX, animInfo.basisY, animInfo.basisZ);
}

void RemoveRotationAndScaling(const InterpInfo &animInfo) {
  WorldMatrixRemove(6);
  WorldMatrixScale(animInfo.basisScale);
  WorldMatrixBasis(animInfo.basisX, animInfo.basisY, animInfo.basisZ);
}

void TranslateView(const InterpInfo &animInfo, CAnimObj *currobj, const NTempest::C3Vector &currPos, const NTempest::C3Vector &parentPos) {
  ASSERT(currobj);
  NTempest::C3Vector transform(0.0f);
  CAnimObjStatus    *status = animInfo.unique->status[currobj->animObjId];
  currobj->translation.InterpolateVolatile(animInfo, status->base, &status->translation, NTempest::C3Vector(0.0f), &transform);

  if (animInfo.unique->flags & 0x10) {
    CAnimObjBlendStatus *blend = &animInfo.unique->blendStatus[currobj->animObjId];
    if (blend->blendTimer > 0) {
      Blend(blend->blendPosition, &transform, blend->blendTimer, animInfo.shared->seq[status->base.currSeq].blendTime);
    }
    blend->prevSeqPosition = transform;
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

void RotateView(const InterpInfo &animInfo, CAnimObj *currobj) {
  ASSERT(currobj);
  if ((currobj->flags & 7) == 4) {
    RemoveRotation(animInfo);
  }

  NTempest::C4Quaternion transform;
  CAnimObjStatus        *status = animInfo.unique->status[currobj->animObjId];
  int animate = currobj->rotation.InterpolateVolatile(animInfo, status->base, &status->rotation, NTempest::C4Quaternion(), &transform);
  int timeLeft = 0;

  if (animInfo.unique->flags & 0x10) {
    CAnimObjBlendStatus *blend = &animInfo.unique->blendStatus[currobj->animObjId];
    timeLeft = blend->blendTimer;
    if (timeLeft > 0) {
      Blend(blend->blendRotation, &transform, timeLeft, animInfo.shared->seq[status->base.currSeq].blendTime);
    }
    blend->prevSeqRotation = transform;
  }

  if (animate || timeLeft > 0) {
    WorldMatrixRotate(transform);
  }
}

void ScaleView(const InterpInfo &animInfo, CAnimObj *currobj) {
  ASSERT(currobj);
  int timeLeft = 0;
  if ((currobj->flags & 3) == 2) {
    if (currobj->flags & 4) {
      RemoveRotationAndScaling(animInfo);
    } else {
      RemoveScale(animInfo.basisScale);
    }
  }

  NTempest::C3Vector transform(0.0f);
  CAnimObjStatus    *status = animInfo.unique->status[currobj->animObjId];
  int                animate = currobj->scale.InterpolateVolatile(animInfo, status->base, &status->scale, NTempest::C3Vector(1.0f), &transform);

  if (animInfo.unique->flags & 0x10) {
    CAnimObjBlendStatus *blend = &animInfo.unique->blendStatus[currobj->animObjId];
    timeLeft = blend->blendTimer;
    if (timeLeft > 0) {
      Blend(blend->blendScale, &transform, timeLeft, animInfo.shared->seq[status->base.currSeq].blendTime);
    }
    blend->prevSeqScale = transform;
  }

  if (animate || timeLeft > 0) {
    WorldMatrixScale(transform);
  }
}

static void SetGlobalSequenceTime(CAnim *anim, CAnimData *data, int elapsedTime) {
  ASSERT(anim);
  ASSERT(data);

  UINT       *elapsed = anim->globalSeqElapsed.Ptr();
  const UINT *length = data->globalSeqLength.Ptr();
  for (UINT i = anim->globalSeqElapsed.Count(); i; --i, ++elapsed, ++length) {
    *elapsed = *length ? (*elapsed + elapsedTime) % *length : 0;
  }
}

static BOOL CallSeqFinishedHandlers(CAnim *unique, UINT seqIndex) {
  ASSERT(unique);

  if (unique->anySeqFinished.callback && !unique->anySeqFinished.callback(unique->anySeqFinished.param)) {
    return 0;
  }

  if (unique->seq[seqIndex].finished.callback && !unique->seq[seqIndex].finished.callback(unique->seq[seqIndex].finished.param)) {
    return 0;
  }

  return 1;
}

static int SetSequenceTime(CAnim *unique, UINT sequence, const CAnimSequence *seq, CSeqInfo *seqInfo, int seqTime) {
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
    if (!seqFinished) {
      callback = 1;
    }
  } else {
    seqInfo->seqFinished = 0;
  }

  if (!(seq->flags & 1)) {
    seqTime = WrapAnimTime(seqTime, loopTime);
  } else if (seqTime > loopTime) {
    seqTime = loopTime;
  } else if (seqTime < 0) {
    seqTime = 0;
  }

  seqInfo->elapsed = seq->time.l + seqTime;

  if (!callback) {
    return 1;
  }

  if (seqInfo->replayTimes > 0) {
    --seqInfo->replayTimes;
    return 1;
  }

  ActivityBegin(ACTIVITY_ANIMSEQEND);
  int result = CallSeqFinishedHandlers(unique, sequence);
  ActivityEnd(ACTIVITY_ANIMSEQEND);
  return result;
}

static int GetSeqSyncTime(CAnim *unique, CAnimData *shared, UINT currSeq, UINT prevSeq) {
  CAnimSequence *currSharedSeq = &shared->seq[currSeq];
  CAnimSequence *seqShared = &shared->seq[prevSeq];

  if (!(currSharedSeq->flags & 2) || !(seqShared->flags & 2)) {
    return 0;
  }

  return NTempest::CMath::fint_n(static_cast<float>(unique->seq[prevSeq].elapsed - seqShared->time.Low()) /
                       seqShared->time.Magnitude() *
                       currSharedSeq->time.Magnitude());
}

void SetObjectSequencesReset(CAnim *unique, CAnimData *shared, UINT sequence, UINT blendTime, int resetTime) {
  UINT       i;
  CAnimObj **currobj = shared->headarray.Ptr();
  for (i = shared->headarray.Count(); i; --i, ++currobj) {
    ISetSequenceReset(unique, *currobj, sequence, blendTime, resetTime);
  }

  CAnimCameraObjStatus *cameraStatus = unique->cameraStatus.Ptr();
  for (i = unique->cameraStatus.Count(); i; --i, ++cameraStatus) {
    ISetSequenceInfo(unique, &cameraStatus->base, sequence, resetTime);
  }

  CAnimObjStatus *textureStatus = unique->textureStatus.Ptr();
  for (i = unique->textureStatus.Count(); i; --i, ++textureStatus) {
    ISetSequenceInfo(unique, &textureStatus->base, sequence, resetTime);
  }

  CAnimGeosetObjStatus *geosetStatus = unique->geosetStatus.Ptr();
  for (i = unique->geosetStatus.Count(); i; --i, ++geosetStatus) {
    ISetSequenceInfo(unique, &geosetStatus->base, sequence, resetTime);
  }

  CAnimLayerStatus *layerStatus = unique->layerStatus.Ptr();
  for (i = unique->layerStatus.Count(); i; --i, ++layerStatus) {
    ISetSequenceInfo(unique, &layerStatus->base, sequence, resetTime);
  }
}

static void SetObjectSequences(CAnim *unique, CAnimData *shared, UINT sequence, UINT prevSeq, UINT blendTime, int resetTime) {
  UINT       i;
  CAnimObj **currobj = shared->headarray.Ptr();
  for (i = shared->headarray.Count(); i; --i, ++currobj) {
    ISetSequence(unique, *currobj, sequence, prevSeq, blendTime, resetTime);
  }

  CAnimCameraObjStatus *cameraStatus = unique->cameraStatus.Ptr();
  for (i = unique->cameraStatus.Count(); i; --i, ++cameraStatus) {
    ISetSequenceInfo(unique, &cameraStatus->base, sequence, prevSeq, resetTime);
  }

  CAnimObjStatus *textureStatus = unique->textureStatus.Ptr();
  for (i = unique->textureStatus.Count(); i; --i, ++textureStatus) {
    ISetSequenceInfo(unique, &textureStatus->base, sequence, prevSeq, resetTime);
  }

  CAnimGeosetObjStatus *geosetStatus = unique->geosetStatus.Ptr();
  for (i = unique->geosetStatus.Count(); i; --i, ++geosetStatus) {
    ISetSequenceInfo(unique, &geosetStatus->base, sequence, prevSeq, resetTime);
  }

  CAnimLayerStatus *layerStatus = unique->layerStatus.Ptr();
  for (i = unique->layerStatus.Count(); i; --i, ++layerStatus) {
    ISetSequenceInfo(unique, &layerStatus->base, sequence, prevSeq, resetTime);
  }
}

static int RandomInRange(const NTempest::CiRange &range) {
  if (!range.Magnitude()) {
    return range.Low();
  }

  return range.Low() + (rand() >> 2) % range.Magnitude();
}

void SetSequence(CAnim *unique, CAnimData *shared, BYTE sequence, UINT flags) {
  ASSERT(unique);

  const UINT numSeqs = unique->seq.Count();
  if (sequence >= numSeqs) {
    return;
  }

  if (flags & 8) {
    unique->flags |= 0x20;
  }

  const int  seqStartTime = shared->seq[sequence].time.l;
  const UINT prevSeq = unique->primarySeq;
  const int  resetTime = !(flags & 2);
  const BOOL wasInited = unique->flags & 0x40;

  if (!wasInited || !(unique->flags & 0x10)) {
    flags |= 4;
  }

  unique->primarySeq = sequence;
  unique->flags |= 0x40;

  const UINT blendTime = (flags & 4) ? 0 : shared->seq[sequence].blendTime;
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

  for (UINT i = 0; i < numSeqs; ++i) {
    if (!unique->seq[i].useCount || (i == sequence && resetTime)) {
      unique->seq[i].seqFinished = 0;
    }
  }

  unique->flags &= ~5;
}

void GetWorldTransform(InterpInfo *animInfo) {
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

static BOOL AdvanceTime(CAnim *unique, CAnimData *shared) {
  ASSERT(unique);
  ASSERT(shared);

  float fTimeElapsed;
  if (unique->flags & 0x20) {
    fTimeElapsed = 0.0f;
    unique->flags &= ~0x20;
  } else {
    fTimeElapsed = static_cast<int>(s_currTime - unique->seqLastTime) * s_timeScale;
  }

  unique->seqLastTime = s_currTime;
  const UINT numSequences = unique->seq.Count();

  if ((s_animFlags & 8) | (unique->flags & 8)) {
    fTimeElapsed = 0.0f;
    for (UINT pausedSeqIndex = 0; pausedSeqIndex < numSequences; ++pausedSeqIndex) {
      unique->seq[pausedSeqIndex].scaledElapsedTime = 0;
    }
  } else {
    SetGlobalSequenceTime(unique, shared, NTempest::CMath::fint_n(fTimeElapsed));

    for (UINT seqIndex = 0; seqIndex < numSequences; ++seqIndex) {
      unique->seq[seqIndex].scaledElapsedTime = NTempest::CMath::fint_n(fTimeElapsed * unique->seq[seqIndex].seqTimeScale);

      if (!SetSequenceTime(
              unique, seqIndex, &shared->seq[seqIndex], &unique->seq[seqIndex],
              unique->seq[seqIndex].scaledElapsedTime - shared->seq[seqIndex].time.Low() + unique->seq[seqIndex].elapsed
          )) {
        return 0;
      }
    }
  }

  fTimeElapsed *= 0.001f;
  UINT                    i;
  CAnimEmitter2ObjStatus *emitter2Status = unique->emitter2Status.Ptr();
  for (i = unique->emitter2Status.Count(); i; --i, ++emitter2Status) {
    emitter2Status->elapsedTime = fTimeElapsed;
  }

  CAnimRibbonObjStatus *ribbonStatus = unique->ribbonStatus.Ptr();
  for (i = unique->ribbonStatus.Count(); i; --i, ++ribbonStatus) {
    ribbonStatus->elapsedTime = fTimeElapsed;
  }

  return 1;
}

static void SetGeosetColor(const InterpInfo &animInfo, CAnimGeoset *currgeoset, CAnimGeosetObjStatus *geoStatus, NTempest::CImVector *currentColor) {
  ASSERT(currgeoset);
  ASSERT(geoStatus);

  C3Color color;
  if (currgeoset->color.InterpolateRetained(animInfo, geoStatus->base, &geoStatus->color, C3Color(), &color)) {
    currentColor->r = NTempest::CMath::ftol_0_256_(NTempest::CMath::clamp_(color.r, 0.0f, 1.0f) * 255.0f);
    currentColor->g = NTempest::CMath::ftol_0_256_(NTempest::CMath::clamp_(color.g, 0.0f, 1.0f) * 255.0f);
    currentColor->b = NTempest::CMath::ftol_0_256_(NTempest::CMath::clamp_(color.b, 0.0f, 1.0f) * 255.0f);
  }
}

static void SetGeosetAlpha(const InterpInfo &animInfo, CAnimGeoset *currgeoset, CAnimGeosetObjStatus *geoStatus, CGeosetColor *color) {
  ASSERT(currgeoset);
  ASSERT(geoStatus);

  float visibility = 0.0f;
  if (currgeoset->visibility.InterpolateRetained(animInfo, geoStatus->base, &geoStatus->visibility, 1.0f, &visibility)) {
    color->animatedAlpha = NTempest::CMath::clamp_(visibility, 0.0f, 1.0f);
    color->animatedColor.a = NTempest::CMath::ftol_0_256_(color->animatedAlpha * color->proceduralAlpha * 255.0f);
  }
}

void CalcGeosetColor(const InterpInfo &animInfo, CAnimGeoset *geoset, CAnimGeosetObjStatus *geoStatus, CGeosetColor *color) {
  ASSERT(geoset);
  ASSERT(geoStatus);

  SetGeosetAlpha(animInfo, geoset, geoStatus, color);
  SetGeosetColor(animInfo, geoset, geoStatus, &color->animatedColor);
  if (color->animatedColor.a > 0) {
    geoStatus->base.flags |= 1;
  } else {
    geoStatus->base.flags &= ~1;
  }
}

BOOL CAnimBoneObj::IsVisible(const CAnim &anim) const {
  if (geosetId == 0xFF) {
    return 1;
  }

  return anim.geosetStatus[geosetId].base.flags & 1;
}

static int PickRandomSequence(const CVariations &selection, const CArray<CAnimSequence> &seqs) {
  if (!selection.variation.Count()) {
    return selection.primary;
  }

  UINT draw = rand();
  int  result = selection.primary;
  UINT chance = seqs[result].randPickChance;
  if (draw < chance) {
    return selection.primary;
  }

  draw -= chance;
  result = 0;
  UINT count = selection.variation.Count();
  for (UINT i = 0; i < count; ++i) {
    result = selection.variation[i];
    chance = seqs[result].randPickChance;
    if (draw < chance) {
      break;
    }

    draw -= chance;
  }

  return result;
}

static void SetSplitBodySequence(CAnim *unique, CAnimData *shared, UINT sequence, UINT objectId, UINT flags) {
  if (!(flags & 2)) {
    CAnimObjStatus *status = unique->status[objectId];
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

  UINT blendTime = (flags & 4) ? 0 : shared->seq[sequence].blendTime;
  ISetSequenceReset(unique, shared->obj[objectId], sequence, blendTime, !(flags & 2));

  CSeqInfo *seqInfo = unique->seq.Ptr();
  for (UINT i = unique->seq.Count(); i; --i, ++seqInfo) {
    if (!seqInfo->useCount) {
      seqInfo->seqFinished = 0;
    }
  }
  unique->flags &= ~5;
}

static UINT FindSequenceVariationInUse(CAnim *unique, CAnimData *shared, UINT index) {
  CVariations &variations = shared->seqOrder[unique->seqMapIndex].order[index];
  UINT         sequence = variations.primary;

  if (sequence == 0xFF) {
    return 0xFF;
  }

  if (unique->seq[sequence].useCount) {
    return sequence;
  }

  UINT numVariations = variations.variation.Count();
  for (UINT i = 0; i < numVariations; ++i) {
    sequence = variations.variation[i];
    if (unique->seq[sequence].useCount) {
      return sequence;
    }
  }

  return 0xFF;
}

void IAnimInitializeTime() {
  s_lastFrame = (UINT)-1;
  s_currTime = OsGetAsyncTimeMsPrecise();
  s_elapsedTime = 0;
}

DWORD IAnimGetCurrTimeMs() {
  return s_currTime;
}

void AnimSetTimeScale(HANIM anim, float timeScale) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  VALIDATEBEGIN;
  VALIDATE(unique);
  VALIDATEENDVOID;
  unique->seq[unique->primarySeq].seqTimeScale = timeScale;
}

BOOL AnimSetObjectTimeScale(HANIM anim, UINT objectId, float timeScale) {
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

  VALIDATE(objectId < shared->obj.Count());
  VALIDATEEND;

  unique->seq[unique->status[objectId]->base.currSeq].seqTimeScale = timeScale;
  return 1;
}

float AnimGetTimeScale(HANIM anim) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  VALIDATEBEGIN;
  VALIDATE(unique);
  if (0) {
  validatefailed:
    SErrSetLastError(ERROR_INVALID_PARAMETER);
    return 1.0f;
  }
  return unique->seq[unique->primarySeq].seqTimeScale;
}

float AnimGetObjectTimeScale(HANIM anim, UINT objectId) {
  CAnim     *unique = reinterpret_cast<CAnim *>(anim);
  CAnimData *shared;
  if (!unique) {
    SErrPrepareAppFatal(__FILE__, __LINE__);
    SErrDisplayAppFatal("unique");
    SErrSetLastError(ERROR_INVALID_PARAMETER);
    return 1.0f;
  }

  VALIDATEBEGIN;
  shared = reinterpret_cast<CAnimData *>(unique->hdata);
  VALIDATE(shared);
  VALIDATE(objectId < shared->objectOrder.Count());
  VALIDATEEND;

  objectId = shared->objectOrder[objectId];
  if (objectId == static_cast<UINT>(-1)) {
    return 1.0f;
  }

  if (!(objectId < shared->obj.Count())) {
    SErrPrepareAppFatal(__FILE__, __LINE__);
    SErrDisplayAppFatal("objectId < shared->obj.Count()");
    SErrSetLastError(ERROR_INVALID_PARAMETER);
    return 1.0f;
  }

  return unique->seq[unique->status[objectId]->base.currSeq].seqTimeScale;
}

void AnimSetGlobalTimeScale(float timeScale) {
  s_timeScale = timeScale;
}

float AnimGetGlobalTimeScale() {
  return s_timeScale;
}

int AnimForceCurrentSequenceTime(HANIM anim, int time) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  unique->flags |= 0x20;
  CSeqInfo *seqInfo = &unique->seq[unique->primarySeq];
  return SetSequenceTime(unique, unique->primarySeq, &shared->seq[unique->primarySeq], seqInfo, time);
}

int AnimForceSequenceTime(HANIM anim, UINT index, int time) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  index = FindSequenceVariationInUse(unique, shared, index);
  if (index == 0xFF) {
    return 0;
  }

  unique->flags |= 0x20;
  return SetSequenceTime(unique, index, &shared->seq[index], &unique->seq[index], time);
}

BOOL AnimSetRandomSequenceFidget(HANIM anim, UINT seqIndex, UINT flags) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);
  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  if (shared->seqOrder[unique->seqMapIndex].order[seqIndex].primary == 0xFF) {
    SErrSetLastError(ERROR_INVALID_FUNCTION);
    return 0;
  }

  seqIndex = PickRandomSequence(shared->seqOrder[unique->seqMapIndex].order[seqIndex], shared->seq);

  ASSERT(seqIndex < uint8(0xff));
  SetSequence(unique, shared, seqIndex, flags);
  return 1;
}

BOOL AnimSetRandomSequenceFidget(HANIM anim, UINT seqIndex, UINT objectId, UINT flags) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);
  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  if (shared->seqOrder[unique->seqMapIndex].order[seqIndex].primary == 0xFF) {
    SErrSetLastError(ERROR_INVALID_FUNCTION);
    return 0;
  }

  objectId = shared->objectOrder[objectId];
  if (objectId == static_cast<UINT>(-1)) {
    SErrSetLastError(ERROR_FILE_NOT_FOUND);
    return 0;
  }

  seqIndex = PickRandomSequence(shared->seqOrder[unique->seqMapIndex].order[seqIndex], shared->seq);
  SetSplitBodySequence(unique, shared, seqIndex, objectId, flags);
  return 1;
}

UINT AnimGetNumSequenceFidgets(HANIM anim, UINT seqIndex) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);
  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  if (shared->seqOrder[unique->seqMapIndex].order[seqIndex].primary == 0xFF) {
    return 0;
  }

  return shared->seqOrder[unique->seqMapIndex].order[seqIndex].variation.Count() + 1;
}

BOOL AnimSetSequenceFidget(HANIM anim, UINT seqIndex, UINT fidgetId, UINT flags) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);
  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  if (shared->seqOrder[unique->seqMapIndex].order[seqIndex].primary == 0xFF) {
    SErrSetLastError(ERROR_INVALID_FUNCTION);
    return 0;
  }

  if (fidgetId == 0) {
    seqIndex = shared->seqOrder[unique->seqMapIndex].order[seqIndex].primary;
  } else {
    seqIndex = shared->seqOrder[unique->seqMapIndex].order[seqIndex].variation[fidgetId - 1];
  }

  ASSERT(seqIndex < uint8(0xff));
  SetSequence(unique, shared, seqIndex, flags);
  return 1;
}

BOOL AnimSetSequenceFidget(HANIM anim, UINT seqIndex, UINT fidgetId, UINT objectId, UINT flags) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);
  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  if (shared->seqOrder[unique->seqMapIndex].order[seqIndex].primary == 0xFF) {
    SErrSetLastError(ERROR_INVALID_FUNCTION);
    return 0;
  }

  objectId = shared->objectOrder[objectId];
  if (objectId == static_cast<UINT>(-1)) {
    SErrSetLastError(ERROR_FILE_NOT_FOUND);
    return 0;
  }

  if (fidgetId == 0) {
    seqIndex = shared->seqOrder[unique->seqMapIndex].order[seqIndex].primary;
  } else {
    seqIndex = shared->seqOrder[unique->seqMapIndex].order[seqIndex].variation[fidgetId - 1];
  }

  SetSplitBodySequence(unique, shared, seqIndex, objectId, flags);
  return 1;
}

BOOL AnimSetSequence(HANIM anim, UINT seqIndex, UINT flags) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);
  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  UINT count = shared->seqOrder[unique->seqMapIndex].order.Count();
  BOOL seqValid = seqIndex < count;
  if (!(!seqIndex || seqValid)) {
    SErrDisplayErrorFmt(
        STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,
        isprint((count >> 24) & 0xFF) && isprint((count >> 16) & 0xFF) && isprint((count >> 8) & 0xFF) && isprint(count & 0xFF)
            ? "\"%s\", %s = %ld (0x%08X, '%c%c%c%c')"
            : "\"%s\", %s = %ld (0x%08X)",
        "!seqIndex || seqValid", "count", count, count, (count >> 24) & 0xFF, (count >> 16) & 0xFF, (count >> 8) & 0xFF, count & 0xFF
    );
  }

  if (seqValid) {
    seqIndex = shared->seqOrder[unique->seqMapIndex].order[seqIndex].primary;
  }

  if (seqIndex == 0xFF) {
    SErrSetLastError(ERROR_INVALID_FUNCTION);
    return 0;
  }

  ASSERT((seqIndex == 0) || (seqIndex < shared->seq.Count()));
  ASSERT(seqIndex < uint8(0xff));
  SetSequence(unique, shared, seqIndex, flags);
  return 1;
}

BOOL AnimSetSequence(HANIM anim, UINT seqIndex, UINT objectId, UINT flags) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);
  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  seqIndex = shared->seqOrder[unique->seqMapIndex].order[seqIndex].primary;
  objectId = shared->objectOrder[objectId];
  if (seqIndex == 0xFF) {
    SErrSetLastError(ERROR_INVALID_FUNCTION);
    return 0;
  }

  if (objectId == static_cast<UINT>(-1)) {
    SErrSetLastError(ERROR_FILE_NOT_FOUND);
    return 0;
  }

  SetSplitBodySequence(unique, shared, seqIndex, objectId, flags);
  return 1;
}

BOOL AnimMatchSequence(HANIM anim, UINT objectId, UINT sameAsObjectId, UINT flags) {
  CAnim     *unique = reinterpret_cast<CAnim *>(anim);
  CAnimData *shared;
  VALIDATEBEGIN;
  VALIDATE(unique);

  shared = reinterpret_cast<CAnimData *>(unique->hdata);
  VALIDATE(shared);
  VALIDATE(objectId < shared->objectOrder.Count());
  VALIDATE(sameAsObjectId < shared->objectOrder.Count());

  objectId = shared->objectOrder[objectId];
  sameAsObjectId = shared->objectOrder[sameAsObjectId];

  if (objectId == static_cast<UINT>(-1)) {
    SErrSetLastError(ERROR_FILE_NOT_FOUND);
    return 0;
  }

  if (sameAsObjectId == static_cast<UINT>(-1)) {
    SErrSetLastError(ERROR_FILE_NOT_FOUND);
    return 0;
  }

  VALIDATE(objectId < shared->obj.Count());
  VALIDATE(sameAsObjectId < shared->obj.Count());
  VALIDATEEND;

  SetSplitBodySequence(unique, shared, unique->status[sameAsObjectId]->base.currSeq, objectId, flags);
  return 1;
}

void AnimResetGlobalSequenceTimes(HANIM anim) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  VALIDATEBEGIN;
  VALIDATE(unique);
  VALIDATEENDVOID;
  unique->globalSeqElapsed.Zero();
}

int AnimAdvanceTime(HANIM anim, UINT currentFrame) {
  CAnim     *unique = reinterpret_cast<CAnim *>(anim);
  CAnimData *shared;
  VALIDATEBEGIN;
  VALIDATE(unique);
  shared = reinterpret_cast<CAnimData *>(unique->hdata);
  VALIDATE(shared);
  VALIDATEEND;

  if (currentFrame != s_lastFrame) {
    const DWORD currentTime = OsGetAsyncTimeMsPrecise();
    s_elapsedTime = currentTime - s_currTime;
    s_currTime = currentTime;
    s_lastFrame = currentFrame;
  }

  return AdvanceTime(unique, shared);
}

int IAnimManualAdvanceTime(CAnim *unique, CAnimData *shared, int timeChange) {
  s_elapsedTime = timeChange;
  s_currTime = unique->seqLastTime + timeChange;
  return AdvanceTime(unique, shared);
}

int AnimManualAdvanceTime(HANIM anim, int timeChange) {
  CAnim     *unique = reinterpret_cast<CAnim *>(anim);
  CAnimData *shared;
  VALIDATEBEGIN;
  VALIDATE(unique);
  shared = reinterpret_cast<CAnimData *>(unique->hdata);
  VALIDATE(shared);
  VALIDATEEND;
  return IAnimManualAdvanceTime(unique, shared, timeChange);
}

UINT AnimGetElapsedTime() {
  return s_elapsedTime;
}

void AnimPauseTime(HANIM anim, int pause) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  VALIDATEBEGIN;
  VALIDATE(unique);
  VALIDATEENDVOID;
  if (pause) {
    unique->flags |= 8;
  } else {
    unique->flags &= ~8;
  }
}

void AnimPauseGlobalTime(int pause) {
  if (pause) {
    s_animFlags |= 8;
  } else {
    s_animFlags &= ~8;
  }
}

int AnimGetSequenceTime(HANIM anim, UINT seqIndex) {
  CAnim     *unique = reinterpret_cast<CAnim *>(anim);
  CAnimData *shared;
  VALIDATEBEGIN;
  VALIDATE(unique);

  shared = reinterpret_cast<CAnimData *>(unique->hdata);
  VALIDATE(shared);
  VALIDATE(seqIndex < shared->seqOrder[unique->seqMapIndex].order.Count());
  VALIDATEEND;

  seqIndex = FindSequenceVariationInUse(unique, shared, seqIndex);
  if (seqIndex == 0xFF) {
    return 0;
  }

  return unique->seq[seqIndex].elapsed - shared->seq[seqIndex].time.l;
}
