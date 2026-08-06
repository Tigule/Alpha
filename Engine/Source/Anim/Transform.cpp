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

namespace NTempest {

  void C4QuaternionCompressed::Set(const C4Quaternion &source) {
    int sign = source.w >= 0.0f ? 1 : -1;
    int x = sign * static_cast<int>(source.x * 2097152.0f);
    int y = sign * static_cast<int>(source.y * 1048576.0f);
    int z = sign * static_cast<int>(source.z * 1048576.0f);

    DWORDLONG packed =
        (static_cast<DWORDLONG>(x & 0x3FFFFF) << 42) | (static_cast<DWORDLONG>(y & 0x1FFFFF) << 21) | static_cast<DWORDLONG>(z & 0x1FFFFF);
    m_data = static_cast<LONGLONG>(packed);
  }

  C4QuaternionCompressed::operator C4Quaternion() const {
    const DWORDLONG data = static_cast<DWORDLONG>(m_data);
    const int       xBits = static_cast<int>(data >> 32) >> 10;
    const int       yBits = static_cast<int>(static_cast<UINT>(data >> 10)) >> 11;
    const int       zBits = static_cast<int>(static_cast<UINT>(data) << 11) >> 11;
    const float     x = static_cast<float>(xBits) * 0.00000047683716f;
    const float     y = static_cast<float>(yBits) * 0.00000095367432f;
    const float     z = static_cast<float>(zBits) * 0.00000095367432f;
    const float     magnitude = x * x + y * y + z * z;
    const float     w = CMath::fabs_(magnitude - 1.0f) < 0.00000095367432f ? 0.0f : CMath::sqrt_(1.0f - magnitude);
    return C4Quaternion(w, x, y, z);
  }

}  // namespace NTempest

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

  if (milliseconds < 0) {
    return looptime - (-milliseconds % looptime);
  }

  return milliseconds % looptime;
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
    ASSERT(index < 0xFF);
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

static void RemoveRotation(const InterpInfo &animInfo) {
  WorldMatrixRemove(4);
  WorldMatrixBasis(animInfo.basisX, animInfo.basisY, animInfo.basisZ);
}

static void RemoveRotationAndScaling(const InterpInfo &animInfo) {
  WorldMatrixRemove(6);
  WorldMatrixScale(animInfo.basisScale);
  WorldMatrixBasis(animInfo.basisX, animInfo.basisY, animInfo.basisZ);
}

void TranslateView(const InterpInfo &animInfo, CAnimObj *currobj, const NTempest::C3Vector &currPos, const NTempest::C3Vector &parentPos) {
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

void RotateView(const InterpInfo &animInfo, CAnimObj *currobj) {
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

void ScaleView(const InterpInfo &animInfo, CAnimObj *currobj) {
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

static void SetGlobalSequenceTime(CAnim *anim, CAnimData *data, int elapsedTime) {
  ASSERT(anim);
  ASSERT(data);

  for (UINT i = 0; i < anim->globalSeqElapsed.Count(); ++i) {
    const UINT length = data->globalSeqLength[i];
    anim->globalSeqElapsed[i] = length ? (anim->globalSeqElapsed[i] + elapsedTime) % length : 0;
  }
}

static int CallSeqFinishedHandlers(CAnim *unique, UINT seqIndex) {
  ASSERT(unique);

  if (unique->anySeqFinished.callback && !unique->anySeqFinished.callback(unique->anySeqFinished.param)) {
    return 0;
  }

  if (!unique->seq[seqIndex].finished.callback) {
    return 1;
  }

  return unique->seq[seqIndex].finished.callback(unique->seq[seqIndex].finished.param) != 0;
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

static int GetSeqSyncTime(CAnim *unique, CAnimData *shared, UINT currSeq, UINT prevSeq) {
  CAnimSequence *currSharedSeq = &shared->seq[currSeq];

  if (!(currSharedSeq->flags & 2) || !(shared->seq[prevSeq].flags & 2)) {
    return 0;
  }

  const float syncTime = static_cast<float>(unique->seq[prevSeq].elapsed - shared->seq[prevSeq].time.l) /
                         static_cast<float>(shared->seq[prevSeq].time.h - shared->seq[prevSeq].time.l) *
                         static_cast<float>(currSharedSeq->time.h - currSharedSeq->time.l);
  return syncTime < 0.0f ? -static_cast<int>(-syncTime + 0.5f) : static_cast<int>(syncTime + 0.5f);
}

static void SetObjectSequencesReset(CAnim *unique, CAnimData *shared, UINT sequence, UINT blendTime, int resetTime) {
  CAnimObj **currobj = shared->headarray.Ptr();
  for (UINT objectIndex = 0; objectIndex < shared->headarray.Count(); ++objectIndex, ++currobj) {
    ISetSequenceReset(unique, *currobj, sequence, blendTime, resetTime);
  }

  for (UINT cameraIndex = 0; cameraIndex < unique->cameraStatus.Count(); ++cameraIndex) {
    ISetSequenceInfo(unique, &unique->cameraStatus[cameraIndex].base, sequence, resetTime);
  }

  for (UINT textureIndex = 0; textureIndex < unique->textureStatus.Count(); ++textureIndex) {
    ISetSequenceInfo(unique, &unique->textureStatus[textureIndex].base, sequence, resetTime);
  }

  for (UINT geosetIndex = 0; geosetIndex < unique->geosetStatus.Count(); ++geosetIndex) {
    ISetSequenceInfo(unique, &unique->geosetStatus[geosetIndex].base, sequence, resetTime);
  }

  for (UINT layerIndex = 0; layerIndex < unique->layerStatus.Count(); ++layerIndex) {
    ISetSequenceInfo(unique, &unique->layerStatus[layerIndex].base, sequence, resetTime);
  }
}

static void SetObjectSequences(CAnim *unique, CAnimData *shared, UINT sequence, UINT prevSeq, UINT blendTime, int resetTime) {
  CAnimObj **currobj = shared->headarray.Ptr();
  for (UINT objectIndex = 0; objectIndex < shared->headarray.Count(); ++objectIndex, ++currobj) {
    ISetSequence(unique, *currobj, sequence, prevSeq, blendTime, resetTime);
  }

  for (UINT cameraIndex = 0; cameraIndex < unique->cameraStatus.Count(); ++cameraIndex) {
    ISetSequenceInfo(unique, &unique->cameraStatus[cameraIndex].base, sequence, prevSeq, resetTime);
  }

  for (UINT textureIndex = 0; textureIndex < unique->textureStatus.Count(); ++textureIndex) {
    ISetSequenceInfo(unique, &unique->textureStatus[textureIndex].base, sequence, prevSeq, resetTime);
  }

  for (UINT geosetIndex = 0; geosetIndex < unique->geosetStatus.Count(); ++geosetIndex) {
    ISetSequenceInfo(unique, &unique->geosetStatus[geosetIndex].base, sequence, prevSeq, resetTime);
  }

  for (UINT layerIndex = 0; layerIndex < unique->layerStatus.Count(); ++layerIndex) {
    ISetSequenceInfo(unique, &unique->layerStatus[layerIndex].base, sequence, prevSeq, resetTime);
  }
}

static int RandomInRange(const NTempest::CiRange &range) {
  int delta = range.h - range.l;
  if (!delta) {
    return range.l;
  }

  return range.l + (rand() >> 2) % delta;
}

static void SetSequence(CAnim *unique, CAnimData *shared, BYTE sequence, UINT flags) {
  ASSERT(unique);

  const UINT numSeqs = unique->seq.Count();
  if (sequence >= numSeqs) {
    return;
  }

  if (flags & 8) {
    unique->flags |= 0x20;
  }

  const UINT prevSeq = unique->primarySeq;
  const int  seqStartTime = shared->seq[sequence].time.l;
  const int  resetTime = !(flags & 2);
  const int  wasInited = unique->flags & 0x40;

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

static int AdvanceTime(CAnim *unique, CAnimData *shared) {
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
  const UINT numSequences = unique->seq.Count();

  if ((s_animFlags & 8) || (unique->flags & 8)) {
    fTimeElapsed = 0.0f;
    for (UINT pausedSeqIndex = 0; pausedSeqIndex < numSequences; ++pausedSeqIndex) {
      unique->seq[pausedSeqIndex].scaledElapsedTime = 0;
    }
  } else {
    const int elapsedTime = fTimeElapsed < 0.0f ? -static_cast<int>(NTempest::CMath::fuint_n(-fTimeElapsed)) : static_cast<int>(fTimeElapsed + 0.5f);
    SetGlobalSequenceTime(unique, shared, elapsedTime);

    for (UINT seqIndex = 0; seqIndex < numSequences; ++seqIndex) {
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

  fTimeElapsed *= 0.001f;
  for (UINT emitterIndex = 0; emitterIndex < unique->emitter2Status.Count(); ++emitterIndex) {
    unique->emitter2Status[emitterIndex].elapsedTime = fTimeElapsed;
  }

  for (UINT ribbonIndex = 0; ribbonIndex < unique->ribbonStatus.Count(); ++ribbonIndex) {
    unique->ribbonStatus[ribbonIndex].elapsedTime = fTimeElapsed;
  }

  return 1;
}

static void SetGeosetColor(const InterpInfo &animInfo, CAnimGeoset *currgeoset, CAnimGeosetObjStatus *geoStatus, NTempest::CImVector *currentColor) {
  ASSERT(currgeoset);
  ASSERT(geoStatus);

  C3Color color;
  if (currgeoset->color.TotalKeys()) {
    UINT keys = currgeoset->color.SetAnimTime(geoStatus->base, &geoStatus->color, animInfo);
    if (keys > 1) {
      const CAnimSequence &sequence = animInfo.shared->seq[geoStatus->base.currSeq];
      currgeoset->color.Interpolate(geoStatus->color, sequence.time.h - sequence.time.l, &color);
    } else {
      if (!(geoStatus->base.flags & 0x10)) {
        return;
      }
      if (keys) {
        color = reinterpret_cast<const CLinearKeyFrame<C3Color> *>(currgeoset->color.GetKeyFrame(geoStatus->color.currKey))->transform;
      }
    }

    currentColor->r = NTempest::CMath::ftol_0_256_(min(max(color.r, 0.0f), 1.0f) * 255.0f);
    currentColor->g = NTempest::CMath::ftol_0_256_(min(max(color.g, 0.0f), 1.0f) * 255.0f);
    currentColor->b = NTempest::CMath::ftol_0_256_(min(max(color.b, 0.0f), 1.0f) * 255.0f);
  }
}

static void SetGeosetAlpha(const InterpInfo &animInfo, CAnimGeoset *currgeoset, CAnimGeosetObjStatus *geoStatus, CGeosetColor *color) {
  ASSERT(currgeoset);
  ASSERT(geoStatus);

  float visibility = 0.0f;
  if (currgeoset->visibility.TotalKeys()) {
    UINT keys = currgeoset->visibility.SetAnimTime(geoStatus->base, &geoStatus->visibility, animInfo);
    if (keys > 1) {
      const CAnimSequence &sequence = animInfo.shared->seq[geoStatus->base.currSeq];
      currgeoset->visibility.Interpolate(geoStatus->visibility, sequence.time.h - sequence.time.l, &visibility);
    } else {
      if (!(geoStatus->base.flags & 0x10)) {
        return;
      }
      if (keys) {
        visibility = reinterpret_cast<const CLinearKeyFrame<float> *>(currgeoset->visibility.GetKeyFrame(geoStatus->visibility.currKey))->transform;
      } else {
        visibility = 1.0f;
      }
    }

    color->animatedAlpha = min(max(visibility, 0.0f), 1.0f);
    color->animatedColor.a = NTempest::CMath::ftol_0_256_(color->animatedAlpha * color->proceduralAlpha * 255.0f);
  }
}

void CalcGeosetColor(const InterpInfo &animInfo, CAnimGeoset *geoset, CAnimGeosetObjStatus *geoStatus, CGeosetColor *color) {
  ASSERT(geoset);
  ASSERT(geoStatus);

  SetGeosetAlpha(animInfo, geoset, geoStatus, color);
  SetGeosetColor(animInfo, geoset, geoStatus, &color->animatedColor);
  if (color->animatedColor.a) {
    geoStatus->base.flags |= 1;
  } else {
    geoStatus->base.flags &= ~1;
  }
}

int CAnimBoneObj::IsVisible(const CAnim &anim) const {
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

  UINT blendTime = (flags & 4) ? 0 : shared->seq[sequence].blendTime;
  ISetSequenceReset(unique, shared->obj[objectId], sequence, blendTime, !(flags & 2));

  for (UINT i = 0; i < unique->seq.Count(); ++i) {
    if (!unique->seq[i].useCount) {
      unique->seq[i].seqFinished = 0;
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
  ASSERT(unique);
  unique->seq[unique->primarySeq].seqTimeScale = timeScale;
}

int AnimSetObjectTimeScale(HANIM anim, UINT objectId, float timeScale) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);
  ASSERT(objectId < shared->objectOrder.Count());

  UINT sharedObjectId = shared->objectOrder[objectId];
  if (sharedObjectId == static_cast<UINT>(-1)) {
    return 0;
  }

  ASSERT(sharedObjectId < shared->obj.Count());
  CAnimObj *object = shared->obj[sharedObjectId];
  UINT      sequence = unique->status[object->animObjId]->base.currSeq;
  unique->seq[sequence].seqTimeScale = timeScale;
  return 1;
}

float AnimGetTimeScale(HANIM anim) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);
  return unique->seq[unique->primarySeq].seqTimeScale;
}

float AnimGetObjectTimeScale(HANIM anim, UINT objectId) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);
  ASSERT(objectId < shared->objectOrder.Count());

  UINT sharedObjectId = shared->objectOrder[objectId];
  if (sharedObjectId == static_cast<UINT>(-1)) {
    return 1.0f;
  }

  ASSERT(sharedObjectId < shared->obj.Count());
  CAnimObj *object = shared->obj[sharedObjectId];
  UINT      sequence = unique->status[object->animObjId]->base.currSeq;
  return unique->seq[sequence].seqTimeScale;
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

  UINT sequence = unique->primarySeq;
  unique->flags |= 0x20;
  return SetSequenceTime(unique, sequence, &shared->seq[sequence], &unique->seq[sequence], time);
}

int AnimForceSequenceTime(HANIM anim, UINT index, int time) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  UINT sequence = FindSequenceVariationInUse(unique, shared, index);
  if (sequence == 0xFF) {
    return 0;
  }

  unique->flags |= 0x20;
  return SetSequenceTime(unique, sequence, &shared->seq[sequence], &unique->seq[sequence], time);
}

int AnimSetRandomSequenceFidget(HANIM anim, UINT seqIndex, UINT flags) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);
  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  CVariations &selection = shared->seqOrder[unique->seqMapIndex].order[seqIndex];
  if (selection.primary == 0xFF) {
    SErrSetLastError(ERROR_INVALID_FUNCTION);
    return 0;
  }

  UINT sequence = PickRandomSequence(selection, shared->seq);
  ASSERT(sequence < 0xFF);
  SetSequence(unique, shared, static_cast<BYTE>(sequence), flags);
  return 1;
}

UINT AnimGetNumSequenceFidgets(HANIM anim, UINT seqIndex) {
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

int AnimSetRandomSequenceFidget(HANIM anim, UINT seqIndex, UINT objectId, UINT flags) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);
  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  CVariations &selection = shared->seqOrder[unique->seqMapIndex].order[seqIndex];
  if (selection.primary == 0xFF) {
    SErrSetLastError(ERROR_INVALID_FUNCTION);
    return 0;
  }

  UINT sharedObjectId = shared->objectOrder[objectId];
  if (sharedObjectId == static_cast<UINT>(-1)) {
    SErrSetLastError(ERROR_FILE_NOT_FOUND);
    return 0;
  }

  UINT sequence = PickRandomSequence(selection, shared->seq);
  SetSplitBodySequence(unique, shared, sequence, sharedObjectId, flags);
  return 1;
}

int AnimSetSequenceFidget(HANIM anim, UINT seqIndex, UINT fidgetId, UINT flags) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);
  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  CVariations &selection = shared->seqOrder[unique->seqMapIndex].order[seqIndex];
  if (selection.primary == 0xFF) {
    SErrSetLastError(ERROR_INVALID_FUNCTION);
    return 0;
  }

  UINT sequence = fidgetId ? selection.variation[fidgetId - 1] : selection.primary;
  ASSERT(sequence < 0xFF);
  SetSequence(unique, shared, static_cast<BYTE>(sequence), flags);
  return 1;
}

int AnimSetSequenceFidget(HANIM anim, UINT seqIndex, UINT fidgetId, UINT objectId, UINT flags) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);
  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  CVariations &selection = shared->seqOrder[unique->seqMapIndex].order[seqIndex];
  if (selection.primary == 0xFF) {
    SErrSetLastError(ERROR_INVALID_FUNCTION);
    return 0;
  }

  UINT sharedObjectId = shared->objectOrder[objectId];
  if (sharedObjectId == static_cast<UINT>(-1)) {
    SErrSetLastError(ERROR_FILE_NOT_FOUND);
    return 0;
  }

  UINT sequence = fidgetId ? selection.variation[fidgetId - 1] : selection.primary;
  SetSplitBodySequence(unique, shared, sequence, sharedObjectId, flags);
  return 1;
}

int AnimSetSequence(HANIM anim, UINT seqIndex, UINT flags) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  CSeqOrdering &ordering = shared->seqOrder[unique->seqMapIndex];
  UINT          sequence = seqIndex;
  const UINT    count = ordering.order.Count();

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

  SetSequence(unique, shared, static_cast<BYTE>(sequence), flags);
  return 1;
}

int AnimSetSequence(HANIM anim, UINT seqIndex, UINT objectId, UINT flags) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  CSeqOrdering &ordering = shared->seqOrder[unique->seqMapIndex];
  UINT          sequence = ordering.order[seqIndex].primary;
  UINT          sharedObjectId = shared->objectOrder[objectId];
  if (sequence == 0xFF) {
    SErrSetLastError(ERROR_INVALID_FUNCTION);
    return 0;
  }

  if (sharedObjectId == static_cast<UINT>(-1)) {
    SErrSetLastError(ERROR_FILE_NOT_FOUND);
    return 0;
  }

  SetSplitBodySequence(unique, shared, sequence, sharedObjectId, flags);
  return 1;
}

int AnimMatchSequence(HANIM anim, UINT objectId, UINT sameAsObjectId, UINT flags) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);
  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);
  ASSERT(objectId < shared->objectOrder.Count());
  ASSERT(sameAsObjectId < shared->objectOrder.Count());

  UINT sharedObjectId = shared->objectOrder[objectId];
  UINT sharedSameAsObjectId = shared->objectOrder[sameAsObjectId];
  if (sharedObjectId == static_cast<UINT>(-1) || sharedSameAsObjectId == static_cast<UINT>(-1)) {
    SErrSetLastError(ERROR_FILE_NOT_FOUND);
    return 0;
  }

  ASSERT(sharedObjectId < shared->obj.Count());
  ASSERT(sharedSameAsObjectId < shared->obj.Count());
  UINT sequence = unique->status[shared->obj[sharedSameAsObjectId]->animObjId]->base.currSeq;
  SetSplitBodySequence(unique, shared, sequence, sharedObjectId, flags);
  return 1;
}

void AnimResetGlobalSequenceTimes(HANIM anim) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);
  unique->globalSeqElapsed.Zero();
}

int AnimAdvanceTime(HANIM anim, UINT currentFrame) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  FATALASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  FATALASSERT(shared);

  if (currentFrame != s_lastFrame) {
    const DWORD currentTime = OsGetAsyncTimeMsPrecise();
    s_elapsedTime = currentTime - s_currTime;
    s_currTime = currentTime;
    s_lastFrame = currentFrame;
  }

  return AdvanceTime(unique, shared);
}

static int IAnimManualAdvanceTime(CAnim *unique, CAnimData *shared, int timeChange) {
  s_elapsedTime = timeChange;
  s_currTime = unique->seqLastTime + timeChange;
  return AdvanceTime(unique, shared);
}

int AnimManualAdvanceTime(HANIM anim, int timeChange) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);
  return IAnimManualAdvanceTime(unique, shared, timeChange);
}

UINT AnimGetElapsedTime() {
  return s_elapsedTime;
}

void AnimPauseTime(HANIM anim, int pause) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);
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
