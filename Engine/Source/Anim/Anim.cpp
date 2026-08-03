#include "Anim/Transform.h"
#include "Base/Base.h"

struct CameraInfo : public InterpInfo {
  CameraInfo(
      CAnim *container,
      CAnimData *animptr,
      const TSFixedArray<NTempest::C3Vector> &positions,
      const TSFixedArray<HCAMERA> &cameras
  ) : InterpInfo(container, animptr, positions), cameras(cameras) {
  }

  const TSFixedArray<HCAMERA> &cameras;

 private:
  CameraInfo &operator=(const CameraInfo &);
};

#include "Anim/WorldMatrix.h"
#include "Base/Activity.h"
#include "Gx/CGxDevice.h"
#include "Gxu/IGxuLight.h"
#include "Model/ModelInternal.h"
#include "Services/ParticleSystem2.h"
#include "Services/RibbonEmitter.h"
#include "Tempest/c33matrix.h"
#include "Tempest/c3segment.h"
#include "Tempest/cmath.h"
#include "Tempest/c4quaternion.h"

float                   s_animBoneProjectDistance;
ANIMBONEPROJECTCALLBACK s_AnimBoneProjectCallback;

template <class T, class U>
void CKeyFrameTrack<T, U>::Interpolate(const CKeyTrackStatus &keyStat, unsigned int seqTime, U *transform) {
  const CKeyFrame   *currKey = GetKeyFrame(keyStat.currKey);
  const CKeyFrame   *nextKey = GetKeyFrame(keyStat.nextKey);
  unsigned int       timePerKey = TimeDiff(*currKey, *nextKey, seqTime);
  float              ratio = timePerKey ? static_cast<float>(keyStat.timepastkey) / static_cast<float>(timePerKey) : 0.0f;
  const unsigned int valueOffset = sizeof(T) == sizeof(__int64) ? 8 : sizeof(int);
  U                  currValue = *reinterpret_cast<const T *>(reinterpret_cast<const unsigned char *>(currKey) + valueOffset);
  U                  nextValue = *reinterpret_cast<const T *>(reinterpret_cast<const unsigned char *>(nextKey) + valueOffset);
  const float       *curr = reinterpret_cast<const float *>(&currValue);
  const float       *next = reinterpret_cast<const float *>(&nextValue);
  float             *result = reinterpret_cast<float *>(transform);
  unsigned int       components = sizeof(U) / sizeof(float);
  float              nextSign = 1.0f;

  if (sizeof(T) == sizeof(__int64) && components == 4) {
    float dot = curr[0] * next[0] + curr[1] * next[1] + curr[2] * next[2] + curr[3] * next[3];
    if (dot < 0.0f) {
      nextSign = -1.0f;
    }
  }

  if (m_trackType == TRACK_NO_INTERP) {
    *transform = currValue;
    return;
  }

  U inTanValue;
  U outTanValue;
  if (m_trackType != TRACK_LINEAR) {
    inTanValue = *reinterpret_cast<const T *>(reinterpret_cast<const unsigned char *>(nextKey) + valueOffset + sizeof(T));
    outTanValue = *reinterpret_cast<const T *>(reinterpret_cast<const unsigned char *>(currKey) + valueOffset + sizeof(T) * 2);
  }
  const float *inTan = reinterpret_cast<const float *>(&inTanValue);
  const float *outTan = reinterpret_cast<const float *>(&outTanValue);

  for (unsigned int i = 0; i < components; ++i) {
    if (m_trackType == TRACK_LINEAR) {
      result[i] = curr[i] + (next[i] * nextSign - curr[i]) * ratio;
    } else if (m_trackType == TRACK_HERMITE) {
      float ratio2 = ratio * ratio;
      float ratio3 = ratio2 * ratio;
      result[i] = (2.0f * ratio3 - 3.0f * ratio2 + 1.0f) * curr[i] +
                  (ratio3 - 2.0f * ratio2 + ratio) * outTan[i] +
                  (-2.0f * ratio3 + 3.0f * ratio2) * next[i] * nextSign + (ratio3 - ratio2) * inTan[i];
    } else {
      float oneMinusRatio = 1.0f - ratio;
      float ratio2 = ratio * ratio;
      float ratio3 = ratio2 * ratio;
      result[i] = oneMinusRatio * oneMinusRatio * oneMinusRatio * curr[i] +
                  3.0f * oneMinusRatio * oneMinusRatio * ratio * outTan[i] +
                  3.0f * oneMinusRatio * ratio2 * inTan[i] + ratio3 * next[i] * nextSign;
    }
  }
}

void
FaceDirection(const NTempest::C3Vector &direction, NTempest::C3Vector *xprime, NTempest::C3Vector *yprime, NTempest::C3Vector *zprime) {
  ASSERT(NTempest::CMath::fabs_(direction.SquaredMag()) >= 0.00000023841858f);
  *xprime = direction;

  if (NTempest::CMath::fabs_(direction.x * direction.x + direction.y * direction.y) >= 0.00000023841858f) {
    yprime->x = -direction.y;
    yprime->y = direction.x;
    yprime->z = 0.0f;
    yprime->Normalize();
  } else {
    *yprime = NTempest::C3Vector(1.0f, 0.0f, 0.0f);
  }

  *zprime = NTempest::C3Vector::Cross(*xprime, *yprime);
}

static void LookAtPoint(const NTempest::C3Vector &position, const NTempest::C3Vector &point, NTempest::C4Quaternion *result) {
  NTempest::C3Vector direction = point - position;
  direction.Normalize();

  NTempest::C3Vector xprime;
  NTempest::C3Vector yprime;
  NTempest::C3Vector zprime;
  FaceDirection(direction, &xprime, &yprime, &zprime);

  NTempest::C33Matrix rotation(xprime.x, xprime.y, xprime.z, yprime.x, yprime.y, yprime.z, zprime.x, zprime.y, zprime.z);
  result->FromRotationMatrix(rotation);

  NTempest::C34Matrix parentMatrix;
  WorldMatrixGet(&parentMatrix);
  NTempest::C33Matrix parentBasis(
      parentMatrix.a0, parentMatrix.a1, parentMatrix.a2, parentMatrix.b0, parentMatrix.b1, parentMatrix.b2, parentMatrix.c0, parentMatrix.c1,
      parentMatrix.c2
  );
  NTempest::C4Quaternion parentRotation;
  parentRotation.FromRotationMatrixInv(parentBasis);

  NTempest::C4Quaternion value = *result;
  result->x = parentRotation.w * value.x + parentRotation.x * value.w + parentRotation.y * value.z - parentRotation.z * value.y;
  result->y = parentRotation.w * value.y - parentRotation.x * value.z + parentRotation.y * value.w + parentRotation.z * value.x;
  result->z = parentRotation.w * value.z + parentRotation.x * value.y - parentRotation.y * value.x + parentRotation.z * value.w;
  result->w = parentRotation.w * value.w - parentRotation.x * value.x - parentRotation.y * value.y - parentRotation.z * value.z;
}

void RotateViewBillboarded(const NTempest::C3Vector &cameraVector) {
  NTempest::C3Vector xprime;
  NTempest::C3Vector yprime;
  NTempest::C3Vector zprime;

  FaceDirection(NTempest::C3Vector(-cameraVector.x, -cameraVector.y, -cameraVector.z), &xprime, &yprime, &zprime);
  WorldMatrixRemove(4);
  WorldMatrixBasis(xprime, yprime, zprime);
}

void RotateViewZAxisBillboarded(const NTempest::C3Vector &cameraVector) {
  ASSERT(NTempest::CMath::fabs_(cameraVector.SquaredMag()) >= 0.00000023841858f);
  NTempest::C3Vector xprime;
  NTempest::C3Vector zprime;
  NTempest::C3Vector yprime;

  WorldMatrixGetRow(2, &zprime);
  yprime = NTempest::C3Vector::Cross(zprime, NTempest::C3Vector(-cameraVector.x, -cameraVector.y, -cameraVector.z));
  yprime.Normalize();
  xprime = NTempest::C3Vector::Cross(yprime, zprime);
  WorldMatrixRemove(4);
  WorldMatrixBasis(xprime, yprime, zprime);
}

void RotateViewYAxisBillboarded(const NTempest::C3Vector &cameraVector) {
  ASSERT(NTempest::CMath::fabs_(cameraVector.SquaredMag()) >= 0.00000023841858f);
  NTempest::C3Vector xprime;
  NTempest::C3Vector yprime;
  NTempest::C3Vector zprime;

  WorldMatrixGetRow(1, &yprime);
  zprime = NTempest::C3Vector::Cross(NTempest::C3Vector(-cameraVector.x, -cameraVector.y, -cameraVector.z), yprime);
  zprime.Normalize();
  xprime = NTempest::C3Vector::Cross(yprime, zprime);
  WorldMatrixRemove(4);
  WorldMatrixBasis(xprime, yprime, zprime);
}

void RotateViewXAxisBillboarded(const NTempest::C3Vector &cameraVector) {
  ASSERT(NTempest::CMath::fabs_(cameraVector.SquaredMag()) >= 0.00000023841858f);
  NTempest::C3Vector yprime;
  NTempest::C3Vector xprime;
  NTempest::C3Vector zprime;

  WorldMatrixGetRow(0, &xprime);
  zprime = NTempest::C3Vector::Cross(NTempest::C3Vector(-cameraVector.x, -cameraVector.y, -cameraVector.z), xprime);
  zprime.Normalize();
  yprime = NTempest::C3Vector::Cross(xprime, zprime);
  WorldMatrixRemove(4);
  WorldMatrixBasis(xprime, yprime, zprime);
}

namespace {

  template <class T, class U>
  static const T *AnimKeyValue(const CKeyFrameTrack<T, U> &track, unsigned int key) {
    const unsigned char *p = reinterpret_cast<const unsigned char *>(track.m_keyFrames) + track.m_keyFrameSize * key + sizeof(int);
    return reinterpret_cast<const T *>(p);
  }

  static inline float
  AnimFloat(CKeyFrameTrack<float, float> &track, CBaseStatus &base, CKeyTrackStatus &keyStatus, const InterpInfo &info, float fallback) {
    float value;
    track.InterpolateRetained(info, base, &keyStatus, fallback, &value);
    return value;
  }

  static inline NTempest::C3Vector AnimVector(
      CKeyFrameTrack<NTempest::C3Vector, NTempest::C3Vector> &track,
      CBaseStatus                                            &base,
      CKeyTrackStatus                                        &keyStatus,
      const InterpInfo                                       &info,
      const NTempest::C3Vector                               &fallback
  ) {
    NTempest::C3Vector value;
    track.InterpolateVolatile(info, base, &keyStatus, fallback, &value);
    return value;
  }

  static inline C3Color
  AnimColor(CKeyFrameTrack<C3Color, C3Color> &track, CBaseStatus &base, CKeyTrackStatus &keyStatus, const InterpInfo &info) {
    C3Color fallback;
    C3Color result;
    track.InterpolateRetained(info, base, &keyStatus, fallback, &result);
    return result;
  }

}  // namespace

template <>
void CKeyFrameTrack<NTempest::C4QuaternionCompressed, NTempest::C4Quaternion>::Interpolate(
    const CKeyTrackStatus  &keyStat,
    unsigned int            seqTime,
    NTempest::C4Quaternion *transform
) {
  ASSERT(transform);
  const CKeyFrame *curr = GetKeyFrame(keyStat.currKey);
  const CKeyFrame *next = GetKeyFrame(keyStat.nextKey);
  unsigned int     timePerKey = TimeDiff(*curr, *next, seqTime);
  float            ratio = timePerKey ? static_cast<float>(keyStat.timepastkey) / static_cast<float>(timePerKey) : 0.0f;

  if (m_trackType == TRACK_NO_INTERP) {
    const CLinearKeyFrame<NTempest::C4QuaternionCompressed> *key =
        reinterpret_cast<const CLinearKeyFrame<NTempest::C4QuaternionCompressed> *>(curr);
    *transform = key->transform;
  } else if (m_trackType == TRACK_LINEAR) {
    InterpolateLinear(
        *reinterpret_cast<const CLinearKeyFrame<NTempest::C4QuaternionCompressed> *>(curr),
        *reinterpret_cast<const CLinearKeyFrame<NTempest::C4QuaternionCompressed> *>(next), ratio, transform
    );
  } else if (m_trackType == TRACK_HERMITE) {
    InterpolateHermite(
        *reinterpret_cast<const CSplineKeyFrame<NTempest::C4QuaternionCompressed> *>(curr),
        *reinterpret_cast<const CSplineKeyFrame<NTempest::C4QuaternionCompressed> *>(next), ratio, transform
    );
  } else if (m_trackType == TRACK_BEZIER) {
    InterpolateBezier(
        *reinterpret_cast<const CSplineKeyFrame<NTempest::C4QuaternionCompressed> *>(curr),
        *reinterpret_cast<const CSplineKeyFrame<NTempest::C4QuaternionCompressed> *>(next), ratio, transform
    );
  }
}

template <>
void CKeyFrameTrack<C3Color, C3Color>::Interpolate(
    const CKeyTrackStatus &keyStat, unsigned int seqTime, C3Color *transform
) {
  const CKeyFrame *currKey = GetKeyFrame(keyStat.currKey);
  const CKeyFrame *nextKey = GetKeyFrame(keyStat.nextKey);
  unsigned int     timePerKey = TimeDiff(*currKey, *nextKey, seqTime);
  float            ratio = timePerKey ? static_cast<float>(keyStat.timepastkey) / static_cast<float>(timePerKey) : 0.0f;
  const unsigned int valueOffset = sizeof(int);
  C3Color             currValue =
      *reinterpret_cast<const C3Color *>(reinterpret_cast<const unsigned char *>(currKey) + valueOffset);
  C3Color nextValue =
      *reinterpret_cast<const C3Color *>(reinterpret_cast<const unsigned char *>(nextKey) + valueOffset);
  const float *curr = reinterpret_cast<const float *>(&currValue);
  const float *next = reinterpret_cast<const float *>(&nextValue);
  float       *result = reinterpret_cast<float *>(transform);

  if (m_trackType == TRACK_NO_INTERP) {
    *transform = currValue;
    return;
  }

  C3Color inTanValue;
  C3Color outTanValue;
  if (m_trackType != TRACK_LINEAR) {
    inTanValue = *reinterpret_cast<const C3Color *>(
        reinterpret_cast<const unsigned char *>(nextKey) + valueOffset + sizeof(C3Color)
    );
    outTanValue = *reinterpret_cast<const C3Color *>(
        reinterpret_cast<const unsigned char *>(currKey) + valueOffset + sizeof(C3Color) * 2
    );
  }
  const float *inTan = reinterpret_cast<const float *>(&inTanValue);
  const float *outTan = reinterpret_cast<const float *>(&outTanValue);

  for (unsigned int i = 0; i < sizeof(C3Color) / sizeof(float); ++i) {
    if (m_trackType == TRACK_LINEAR) {
      result[i] = curr[i] + (next[i] - curr[i]) * ratio;
    } else if (m_trackType == TRACK_HERMITE) {
      float ratio2 = ratio * ratio;
      float ratio3 = ratio2 * ratio;
      result[i] = (2.0f * ratio3 - 3.0f * ratio2 + 1.0f) * curr[i] +
                  (ratio3 - 2.0f * ratio2 + ratio) * outTan[i] +
                  (-2.0f * ratio3 + 3.0f * ratio2) * next[i] + (ratio3 - ratio2) * inTan[i];
    } else {
      float oneMinusRatio = 1.0f - ratio;
      float ratio2 = ratio * ratio;
      float ratio3 = ratio2 * ratio;
      result[i] = oneMinusRatio * oneMinusRatio * oneMinusRatio * curr[i] +
                  3.0f * oneMinusRatio * oneMinusRatio * ratio * outTan[i] +
                  3.0f * oneMinusRatio * ratio2 * inTan[i] + ratio3 * next[i];
    }
  }
}

static void PlaceEventObject(const AnimInfo& animInfo, CAnimEventObj* currobj) {
  FATALASSERT(currobj->animObjId < animInfo.unique->status.Count());
  CAnimEventObjStatus *eventStatus;
  eventStatus = static_cast<CAnimEventObjStatus *>(animInfo.unique->status[currobj->animObjId]);
  FATALASSERT(currobj->splitIndex < animInfo.positions.Count());

  eventStatus->position = animInfo.positions[currobj->splitIndex];
  WorldMatrixTransform(&eventStatus->position);

  NTempest::C34Matrix basis(
      animInfo.basisX.x, animInfo.basisX.y, animInfo.basisX.z,
      animInfo.basisY.x, animInfo.basisY.y, animInfo.basisY.z,
      animInfo.basisZ.x, animInfo.basisZ.y, animInfo.basisZ.z,
      animInfo.basisPosition.x, animInfo.basisPosition.y, animInfo.basisPosition.z);
  NTempest::C34Matrix invBasis = basis.AffineInverse(animInfo.basisScale);
  eventStatus->position *= invBasis;
}

static void IProcessEvent(const InterpInfo& animInfo, CAnimEventObj* currEvent) {
  if (!animInfo.unique->appEvent.callback || (animInfo.unique->flags & 8) ||
      !currEvent->events.TotalKeys()) {
    return;
  }

  CAnimEventObjStatus *eventStatus = &animInfo.unique->eventStatus[currEvent->splitIndex];
  CKeyTrackStatus prevStatus = eventStatus->event;
  unsigned char sequence = eventStatus->base.currSeq;
  if (!currEvent->events.NumKeysThisSeqSafe(sequence)) {
    return;
  }
  currEvent->events.SetAnimTime(eventStatus->base, &eventStatus->event, animInfo);

  if (currEvent->events.JustPastKey(
          animInfo.unique->seq[sequence].scaledElapsedTime,
          animInfo.shared->seq[sequence],
          animInfo.unique->seq[sequence].elapsed,
          sequence,
          eventStatus->base.flags & 0x10,
          prevStatus,
          eventStatus->event
      )) {
    NTempest::C3Vector position = eventStatus->position;
    WorldMatrixTransform(&position);
    ActivityBegin(ACTIVITY_ANIMEVENTS);
    animInfo.unique->appEvent.callback(
        currEvent->name, position, animInfo.unique->appEvent.param);
    ActivityEnd(ACTIVITY_ANIMEVENTS);
  }
}

static void PlaceModelObject(const AnimInfo& animInfo, CAnimModelObj* modelObj) {
  modelObj->visibility.InterpolateRetained(
      animInfo,
      static_cast<CAnimModelObjStatus *>(animInfo.unique->status[modelObj->animObjId])->base,
      &static_cast<CAnimModelObjStatus *>(animInfo.unique->status[modelObj->animObjId])->visibility,
      1.0f,
      &static_cast<CAnimModelObjStatus *>(animInfo.unique->status[modelObj->animObjId])->visible
  );
  FATALASSERT(modelObj->splitIndex < animInfo.data.numAttached);
  WorldMatrixGet(animInfo.data.attached + modelObj->splitIndex);
}

static void SetLightColor(const AnimInfo& animInfo, CAnimLightObj* currobj, CAnimLightObjStatus* lightStatus, CGxLight* light) {
  C3Color colorKey;
  if (currobj->color.InterpolateRetained(animInfo, lightStatus->base, &lightStatus->color, C3Color(), &colorKey)) {
    light->m_dirColor.r = NTempest::CMath::ftol_0_256_(min(max(colorKey.r, 0.0f), 1.0f) * 255.0f);
    light->m_dirColor.g = NTempest::CMath::ftol_0_256_(min(max(colorKey.g, 0.0f), 1.0f) * 255.0f);
    light->m_dirColor.b = NTempest::CMath::ftol_0_256_(min(max(colorKey.b, 0.0f), 1.0f) * 255.0f);
  }
  if (currobj->ambColor.InterpolateRetained(animInfo, lightStatus->base, &lightStatus->ambColor, C3Color(), &colorKey)) {
    light->m_ambColor.r = NTempest::CMath::ftol_0_256_(min(max(colorKey.r, 0.0f), 1.0f) * 255.0f);
    light->m_ambColor.g = NTempest::CMath::ftol_0_256_(min(max(colorKey.g, 0.0f), 1.0f) * 255.0f);
    light->m_ambColor.b = NTempest::CMath::ftol_0_256_(min(max(colorKey.b, 0.0f), 1.0f) * 255.0f);
  }
}

static void SetLightIntensity(const AnimInfo& animInfo, CAnimLightObj* currobj, CAnimLightObjStatus* lightStatus, CGxLight* light) {
  if (currobj->intensity.TotalKeys()) {
    currobj->intensity.InterpolateRetained(
        animInfo, lightStatus->base, &lightStatus->intensity, 0.0f, &light->m_dirIntensity
    );
  }
  if (light->m_dirIntensity < 0.0f) {
    light->m_dirIntensity = 0.0f;
  }
  if (currobj->ambIntensity.TotalKeys()) {
    currobj->ambIntensity.InterpolateRetained(
        animInfo, lightStatus->base, &lightStatus->ambIntensity, 0.0f, &light->m_ambIntensity
    );
  }
  if (light->m_ambIntensity < 0.0f) {
    light->m_ambIntensity = 0.0f;
  }
}

static void SetLightDirection(CGxLight* light) {
  if (light->m_isOmni) {
    return;
  }
  light->m_dir.x = 0.0f;
  light->m_dir.y = 0.0f;
  light->m_dir.z = -1.0f;
  WorldMatrixPush();
  WorldMatrixRemove(1);
  WorldMatrixTransform(&light->m_dir);
  WorldMatrixPop();
}

static void SetLightValues(const AnimInfo& animInfo, CAnimLightObj* currobj, const NTempest::C3Vector& currPos) {
  FATALASSERT(currobj);
  FATALASSERT(currobj->type == OBJ_TYPE_LIGHT);
  FATALASSERT(currobj->splitIndex < animInfo.data.lights->Count());
  unsigned long gxuLightId = (*animInfo.data.lights)[currobj->splitIndex];
  if (gxuLightId) {
    CAnimLightObjStatus *lightStatus =
        static_cast<CAnimLightObjStatus *>(animInfo.unique->status[currobj->animObjId]);
    CGxLight *light = GxuLightLock(gxuLightId);
    FATALASSERT(light);
    float isVisible = 1.0f;
    if (!currobj->visibility.TotalKeys()) {
      light->m_enabled = 1;
    } else if (currobj->visibility.InterpolateRetained(
            animInfo, lightStatus->base, &lightStatus->visibility, 1.0f, &isVisible
        )) {
      light->m_enabled = isVisible > 0.0f;
    }
    if (light->m_enabled) {
      SetLightColor(animInfo, currobj, lightStatus, light);
      SetLightIntensity(animInfo, currobj, lightStatus, light);
      SetLightDirection(light);
      if (light->m_isOmni) {
        light->m_dir = currPos;
        WorldMatrixTransform(&light->m_dir);
        light->m_dir += animInfo.cameraWorldPos;
      }
    }
    GxuLightUnlock(gxuLightId);
  }
}

static void SetParticleVariation2(const AnimInfo& animInfo, CAnimEmitter2Obj* currobj, CAnimEmitter2ObjStatus* status) {
  float variation = 0.0f;
  if (currobj->variation.InterpolateRetained(animInfo, status->base, &status->variation, 0.0f, &variation)) {
    (*animInfo.data.emitters2)[currobj->splitIndex]->SetVelocityVariation(
        variation
    );
  }
}

static void SetParticleSpeed2(const AnimInfo& animInfo, CAnimEmitter2Obj* currobj, CAnimEmitter2ObjStatus* status) {
  float speed = 0.0f;
  if (currobj->particleSpeed.InterpolateRetained(animInfo, status->base, &status->speed, 0.0f, &speed)) {
    (*animInfo.data.emitters2)[currobj->splitIndex]->SetVelocity(
        speed
    );
  }
}

static void SetParticleEmissionRate2(const AnimInfo& animInfo, CAnimEmitter2Obj* currobj, CAnimEmitter2ObjStatus* status) {
  float rate = 0.0f;
  if (currobj->emissionRate.InterpolateRetained(animInfo, status->base, &status->emissionRate, 0.0f, &rate)) {
    if (rate > 0.0f && (*animInfo.data.emitters2)[currobj->splitIndex]->EmissionRate() == 0.0f && currobj->squirts) {
      (*animInfo.data.emitters2)[currobj->splitIndex]->Squirt();
    }
    (*animInfo.data.emitters2)[currobj->splitIndex]->SetEmissionRate(rate);
  }
}

static void SetParticleGravity2(const AnimInfo& animInfo, CAnimEmitter2Obj* currobj, CAnimEmitter2ObjStatus* status) {
  float accel = 0.0f;
  if (currobj->gravity.InterpolateRetained(animInfo, status->base, &status->gravity, 0.0f, &accel)) {
    (*animInfo.data.emitters2)[currobj->splitIndex]->SetAcceleration(
        accel
    );
  }
}

static void SetEmitterLatitude2(const AnimInfo& animInfo, CAnimEmitter2Obj* currobj, CAnimEmitter2ObjStatus* status) {
  float latitude = 0.0f;
  if (currobj->latitude.InterpolateRetained(animInfo, status->base, &status->latitude, 0.0f, &latitude)) {
    (*animInfo.data.emitters2)[currobj->splitIndex]->SetLatitude(
        latitude
    );
  }
}

static void SetEmitterLongitude2(const AnimInfo& animInfo, CAnimEmitter2Obj* currobj, CAnimEmitter2ObjStatus* status) {
  float longitude = 0.0f;
  if (currobj->longitude.InterpolateRetained(animInfo, status->base, &status->longitude, 0.0f, &longitude)) {
    (*animInfo.data.emitters2)[currobj->splitIndex]->SetLongitude(
        longitude
    );
  }
}

static void SetEmitterWidth2(const AnimInfo& animInfo, CAnimEmitter2Obj* currobj, CAnimEmitter2ObjStatus* status) {
  float width = 0.0f;
  if (currobj->width.InterpolateRetained(animInfo, status->base, &status->width, 0.0f, &width)) {
    (*animInfo.data.emitters2)[currobj->splitIndex]->SetWidth(
        width
    );
  }
}

static void SetEmitterLength2(const AnimInfo& animInfo, CAnimEmitter2Obj* currobj, CAnimEmitter2ObjStatus* status) {
  float length = 0.0f;
  if (currobj->length.InterpolateRetained(animInfo, status->base, &status->length, 0.0f, &length)) {
    (*animInfo.data.emitters2)[currobj->splitIndex]->SetHeight(
        length
    );
  }
}

static void SetEmitterZsource2(const AnimInfo& animInfo, CAnimEmitter2Obj* currobj, CAnimEmitter2ObjStatus* status) {
  float zsource = 0.0f;
  if (currobj->zsource.InterpolateRetained(animInfo, status->base, &status->zsource, 0.0f, &zsource)) {
    (*animInfo.data.emitters2)[currobj->splitIndex]->SetZsource(
        zsource
    );
  }
}

static void SetEmitterLifeSpan2(const AnimInfo& animInfo, CAnimEmitter2Obj* currobj, CAnimEmitter2ObjStatus* status) {
  float lifeSpan = 0.1f;
  if (currobj->lifeSpan.InterpolateRetained(animInfo, status->base, &status->lifeSpan, 1.0f, &lifeSpan)) {
    (*animInfo.data.emitters2)[currobj->splitIndex]->SetLifeSpan(
        lifeSpan
    );
  }
}

static void SetRibbonHeight(const AnimInfo& animInfo, CAnimRibbonObj* currobj, CAnimRibbonObjStatus* status) {
  float heightAbove = 0.0f;
  float heightBelow = 0.0f;
  int newAbove = currobj->heightAbove.InterpolateRetained(
      animInfo, status->base, &status->heightAbove, 0.0f, &heightAbove
  );
  int newBelow = currobj->heightBelow.InterpolateRetained(
      animInfo, status->base, &status->heightBelow, 0.0f, &heightBelow
  );
  if (newAbove) {
    (*animInfo.data.ribbons)[currobj->splitIndex]->SetAbove(heightAbove);
  }
  if (newBelow) {
    (*animInfo.data.ribbons)[currobj->splitIndex]->SetBelow(heightBelow);
  }
}

static void SetRibbonTexSlot(const AnimInfo& animInfo, CAnimRibbonObj* currobj, CAnimRibbonObjStatus* status) {
  unsigned int slot = 0;
  if (currobj->slot.InterpolateRetained(animInfo, status->base, &status->slot, 0, &slot)) {
    (*animInfo.data.ribbons)[currobj->splitIndex]->SetTexSlot(slot);
  }
}

static void SetRibbonColor(const AnimInfo& animInfo, CAnimRibbonObj* currobj, CAnimRibbonObjStatus* status) {
  C3Color color;
  if (currobj->color.InterpolateRetained(animInfo, status->base, &status->color, C3Color(), &color)) {
    (*animInfo.data.ribbons)[currobj->splitIndex]->SetColor(color.r, color.g, color.b);
  }
}

static void SetRibbonAlpha(const AnimInfo& animInfo, CAnimRibbonObj* currobj, CAnimRibbonObjStatus* status) {
  float alpha = 0.0f;
  if (currobj->alpha.InterpolateRetained(animInfo, status->base, &status->alpha, 0.0f, &alpha)) {
    (*animInfo.data.ribbons)[currobj->splitIndex]->SetAlpha(alpha);
  }
}

static void SetEmitter2Values(const AnimInfo& animInfo, CAnimEmitter2Obj* currobj) {
  FATALASSERT(currobj);
  FATALASSERT(currobj->type == OBJ_TYPE_EMITTER2);
  FATALASSERT(currobj->splitIndex < animInfo.data.emitters2->Count());
  CAnimEmitter2ObjStatus *status = &animInfo.unique->emitter2Status[currobj->splitIndex];
  float isVisible = 1.0f;
  if (!currobj->visibility.TotalKeys() ||
      currobj->visibility.InterpolateRetained(animInfo, status->base, &status->visibility, 1.0f, &isVisible)) {
    (*animInfo.data.emitters2)[currobj->splitIndex]->SetEnabled(isVisible > 0.0f && !currobj->squirts, 1);
  }
  SetParticleVariation2(animInfo, currobj, status);
  SetParticleSpeed2(animInfo, currobj, status);
  SetParticleEmissionRate2(animInfo, currobj, status);
  SetParticleGravity2(animInfo, currobj, status);
  SetEmitterLatitude2(animInfo, currobj, status);
  SetEmitterLongitude2(animInfo, currobj, status);
  SetEmitterWidth2(animInfo, currobj, status);
  SetEmitterLength2(animInfo, currobj, status);
  SetEmitterZsource2(animInfo, currobj, status);
  SetEmitterLifeSpan2(animInfo, currobj, status);
}

static void SetRibbonValues(const AnimInfo& animInfo, CAnimRibbonObj* currobj) {
  FATALASSERT(currobj);
  FATALASSERT(currobj->type == OBJ_TYPE_RIBBON);
  FATALASSERT(currobj->splitIndex < animInfo.data.ribbons->Count());

  CAnimRibbonObjStatus &status = animInfo.unique->ribbonStatus[currobj->splitIndex];
  float isVisible = 1.0f;
  if (currobj->visibility.InterpolateRetained(
          animInfo,
          status.base,
          &status.visibility,
          1.0f,
          &isVisible
      )) {
    (*animInfo.data.ribbons)[currobj->splitIndex]->SetEnabled(isVisible > 0.0f);
  }
  SetRibbonHeight(animInfo, currobj, &status);
  SetRibbonTexSlot(animInfo, currobj, &status);
  SetRibbonColor(animInfo, currobj, &status);
  SetRibbonAlpha(animInfo, currobj, &status);

  NTempest::C34Matrix current;
  WorldMatrixGet(&current);
  (*animInfo.data.ribbons)[currobj->splitIndex]->SetPos(
      NTempest::C44Matrix(current), animInfo.cameraWorldPos
  );
}

static void PlaceObject(const AnimInfo &animInfo, CAnimObj *currobj, const NTempest::C3Vector &currPos) {
  ASSERT(currobj);

  switch (currobj->type) {
    case OBJ_TYPE_HELPER:
      return;
    case OBJ_TYPE_LIGHT:
      WorldMatrixTranslate(-currPos);
      ASSERT(currobj->type == OBJ_TYPE_LIGHT);
      SetLightValues(animInfo, static_cast<CAnimLightObj *>(currobj), currPos);
      break;
    case OBJ_TYPE_MODEL:
      ASSERT(currobj->type == OBJ_TYPE_MODEL);
      PlaceModelObject(animInfo, static_cast<CAnimModelObj *>(currobj));
      break;
    case OBJ_TYPE_EMITTER2: {
      NTempest::C34Matrix currentWorldMatrix;
      ASSERT(currobj->type == OBJ_TYPE_EMITTER2);
      SetEmitter2Values(animInfo, static_cast<CAnimEmitter2Obj *>(currobj));
      ASSERT(currobj->splitIndex < animInfo.data.emitters2->Count());
      WorldMatrixGet(&currentWorldMatrix);
      currentWorldMatrix.Rotate(PI * 0.5f, NTempest::C3Vector(0.0f, 0.0f, 1.0f), true);
      (*animInfo.data.emitters2)[currobj->splitIndex]->Update(
          animInfo.unique->emitter2Status[currobj->splitIndex].elapsedTime,
          currentWorldMatrix,
          animInfo.cameraWorldPos
      );
      break;
    }
    case OBJ_TYPE_RIBBON:
      ASSERT(currobj->type == OBJ_TYPE_RIBBON);
      SetRibbonValues(animInfo, static_cast<CAnimRibbonObj *>(currobj));
      ASSERT(currobj->splitIndex < animInfo.data.ribbons->Count());
      (*animInfo.data.ribbons)[currobj->splitIndex]->Update(
          animInfo.unique->ribbonStatus[currobj->splitIndex].elapsedTime,
          0
      );
      break;
    case OBJ_TYPE_EVENT:
      WorldMatrixTranslate(-currPos);
      ASSERT(currobj->type == OBJ_TYPE_EVENT);
      PlaceEventObject(animInfo, static_cast<CAnimEventObj *>(currobj));
      break;
    default:
      WorldMatrixTranslate(-currPos);
      ASSERT(currobj->type == OBJ_TYPE_BONE);
      ASSERT(currobj->splitIndex < animInfo.data.numBones);
      WorldMatrixGet(animInfo.data.boneMtx + currobj->splitIndex);
      break;
  }
}

static void ApplyFaceDir(const AnimInfo &animInfo, CAnimObj *currobj) {
  CAnimObjStatus *status = animInfo.unique->status[currobj->animObjId];
  status->base.flags &= ~4;
  if (!(status->base.flags & 0x40)) {
    return;
  }

  NTempest::C34Matrix matrix;
  WorldMatrixGet(&matrix);
  NTempest::C3Vector targetPos = animInfo.unique->lookAtTarget[status->lookAtId];
  NTempest::C3Vector transformed(
      matrix.a0 * targetPos.x + matrix.b0 * targetPos.y + matrix.c0 * targetPos.z,
      matrix.a1 * targetPos.x + matrix.b1 * targetPos.y + matrix.c1 * targetPos.z,
      matrix.a2 * targetPos.x + matrix.b2 * targetPos.y + matrix.c2 * targetPos.z
  );
  NTempest::C4Quaternion transform;
  LookAtPoint(NTempest::C3Vector(0.0f), transformed, &transform);
  WorldMatrixRotate(transform);
}

static int ApplyLookAt(const AnimInfo &animInfo, CAnimObj *currobj, const NTempest::C3Vector &currPos) {
  CAnimObjStatus *status = animInfo.unique->status[currobj->animObjId];
  status->base.flags &= ~4;
  if (!(status->base.flags & 2)) {
    return 0;
  }

  NTempest::C4Quaternion transform;
  LookAtPoint(animInfo.basisPosition + currPos, animInfo.unique->lookAtTarget[status->lookAtId], &transform);

  if (animInfo.unique->flags & 0x10) {
    CAnimObjBlendStatus &blend = animInfo.unique->blendStatus[currobj->animObjId];
    if (blend.blendTimer > 0) {
      Blend(blend.prevSeqRotation, &transform, blend.blendTimer, animInfo.shared->seq[status->base.currSeq].blendTime);
    }
    blend.blendRotation = transform;
  }

  WorldMatrixRotate(transform);
  return 1;
}

static void
TransformObjectView(const AnimInfo &animInfo, CAnimObj *currobj, const NTempest::C3Vector &currPos, const NTempest::C3Vector &parentPos) {
  CAnimObjStatus *status = animInfo.unique->status[currobj->animObjId];
  if (animInfo.unique->flags & 0x10) {
    CAnimObjBlendStatus &blend = animInfo.unique->blendStatus[currobj->animObjId];
    if (status->base.flags & 0x14) {
      blend.prevSeqPosition = blend.blendPosition;
      blend.prevSeqRotation = blend.blendRotation;
      blend.prevSeqScale = blend.blendScale;
    }

    int elapsed = animInfo.unique->seq[status->base.currSeq].scaledElapsedTime;
    if (elapsed < 0) {
      elapsed = -elapsed;
    }
    blend.blendTimer -= elapsed;
    if (blend.blendTimer < 0) {
      blend.blendTimer = 0;
    }
  }

  TranslateView(animInfo, currobj, currPos, parentPos);
  if ((currobj->flags & 0x40) && s_AnimBoneProjectCallback) {
    NTempest::C3Vector trans(0.0f);
    WorldMatrixGetRow(3, &trans);
    trans += animInfo.cameraWorldPos;

    NTempest::C3Segment seg(trans, trans);
    seg.start.z -= s_animBoneProjectDistance;
    seg.end.z += s_animBoneProjectDistance;

    float z;
    if (s_AnimBoneProjectCallback(seg, z)) {
      trans.x -= animInfo.cameraWorldPos.x;
      trans.y -= animInfo.cameraWorldPos.y;
      trans.z = z - animInfo.cameraWorldPos.z;
      WorldMatrixSetRow(3, trans);
    }
  }
  if (!ApplyLookAt(animInfo, currobj, currPos)) {
    switch (currobj->flags & 0x38) {
      case 8:
        RotateView(animInfo, currobj);
        RotateViewXAxisBillboarded(animInfo.cameraVector);
        break;
      case 0x10:
        RotateView(animInfo, currobj);
        RotateViewYAxisBillboarded(animInfo.cameraVector);
        break;
      case 0x20:
        RotateView(animInfo, currobj);
        RotateViewZAxisBillboarded(animInfo.cameraVector);
        break;
      case 0x38:
        RotateViewBillboarded(animInfo.cameraVector);
        RotateView(animInfo, currobj);
        break;
      default:
        RotateView(animInfo, currobj);
        break;
    }
    ApplyFaceDir(animInfo, currobj);
  }
  ScaleView(animInfo, currobj);
}

static void PrepareObjectHierarchyViews(const AnimInfo &animInfo, CAnimObj *currobj, const NTempest::C3Vector &parentPos) {
  ASSERT(animInfo.shared);
  ASSERT(currobj);

  if (currobj->type == 3 && !static_cast<CAnimBoneObj *>(currobj)->IsVisible(*animInfo.unique)) {
    return;
  }

  WorldMatrixPush();
  NTempest::C3Vector currPos = animInfo.positions[currobj->animObjId];
  TransformObjectView(animInfo, currobj, currPos, parentPos);
  for (unsigned int i = 0; i < currobj->childarray.Count(); ++i) {
    PrepareObjectHierarchyViews(animInfo, currobj->childarray[i], currPos);
  }
  PlaceObject(animInfo, currobj, currPos);
  WorldMatrixPop();
}

static void RotateTexture(const AnimInfo &animInfo, CAnimTransform *texAnim, CAnimObjStatus *status, NTempest::C34Matrix *transform) {
  NTempest::C4Quaternion rotation;
  if (texAnim->rotation.InterpolateVolatile(animInfo, status->base, &status->rotation, NTempest::C4Quaternion(), &rotation)) {
    NTempest::C3Vector texCenter(0.5f, 0.5f, 0.0f);
    transform->Translate(texCenter);
    transform->Rotate(rotation);
    transform->Translate(-texCenter);
  }
}

static void ScaleTexture(const AnimInfo &animInfo, CAnimTransform *texAnim, CAnimObjStatus *status, NTempest::C34Matrix *transform) {
  NTempest::C3Vector scale;
  if (texAnim->scale.InterpolateVolatile(animInfo, status->base, &status->scale, NTempest::C3Vector(1.0f), &scale)) {
    NTempest::C3Vector texCenter(0.5f, 0.5f, 0.0f);
    transform->Translate(texCenter);
    transform->Scale(scale);
    transform->Translate(-texCenter);
  }
}

static void TranslateTexture(const AnimInfo &animInfo, CAnimTransform *texAnim, CAnimObjStatus *status, NTempest::C34Matrix *transform) {
  NTempest::C3Vector position;
  if (texAnim->translation.InterpolateVolatile(animInfo, status->base, &status->translation, NTempest::C3Vector(0.0f), &position)) {
    transform->Translate(position);
  }
}

static void AnimateTextureMap(const AnimInfo &animInfo, CAnimTransform *texAnim, CAnimObjStatus *status, NTempest::C34Matrix *transform) {
  ASSERT(texAnim);
  transform->Identity();
  RotateTexture(animInfo, texAnim, status, transform);
  ScaleTexture(animInfo, texAnim, status, transform);
  TranslateTexture(animInfo, texAnim, status, transform);
}

static void AnimateCamera(const InterpInfo& animInfo, CAnimCameraObj* object, CAnimCameraObjStatus* status, HCAMERA__* camera) {
  ASSERT(object);
  ASSERT(status);
  object->visibility.InterpolateRetained(animInfo, status->base, &status->visibility, 1.0f, &status->visible);
  if (!status->visible) {
    return;
  }
  NTempest::C3Vector position(0.0f);
  object->translation.InterpolateVolatile(animInfo, status->base, &status->translation, position, &position);
  position += object->pivot;
  WorldMatrixTransform(&position);
  DataMgrSetCoord(camera, 7, position, 0);
  position = NTempest::C3Vector(0.0f);
  object->targetTranslation.InterpolateVolatile(animInfo, status->base, &status->targetTranslation, position, &position);
  position += object->targetPivot;
  WorldMatrixTransform(&position);
  DataMgrSetCoord(camera, 8, position, 0);
  float roll = 0.0f;
  object->roll.InterpolateVolatile(animInfo, status->base, &status->roll, roll, &roll);
  DataMgrSetFloat(camera, 5, roll);
}

static void AnimateAllCameras(CameraInfo* animInfo) {
  FATALASSERT(animInfo);
  FATALASSERT(animInfo->unique);
  FATALASSERT(animInfo->shared);
  CAnimCameraObjStatus *status = animInfo->unique->cameraStatus.Ptr();
  while (status != animInfo->unique->cameraStatus.Ptr() + min(animInfo->cameras.Count(), animInfo->shared->cameraObjs.Count())) {
    AnimateCamera(
        *animInfo,
        &animInfo->shared->cameraObjs[status - animInfo->unique->cameraStatus.Ptr()],
        status,
        animInfo->cameras[status - animInfo->unique->cameraStatus.Ptr()]
    );
    ++status;
  }
}

static void AnimateAllTextureMaps(AnimInfo* animInfo) {
  ASSERT(animInfo);
  ASSERT(animInfo->unique);
  ASSERT(animInfo->shared);

  CAnimObjStatus *status = animInfo->unique->textureStatus.Ptr();
  while (status != animInfo->unique->textureStatus.Ptr() + animInfo->shared->tex.Count()) {
    ASSERT(static_cast<unsigned int>(status - animInfo->unique->textureStatus.Ptr()) < animInfo->data.numTexBones);
    AnimateTextureMap(
        *animInfo,
        &animInfo->shared->tex[status - animInfo->unique->textureStatus.Ptr()],
        status,
        &animInfo->data.textureMtx[status - animInfo->unique->textureStatus.Ptr()]
    );
    ++status;
  }
}

static void AnimateAllGeosets(AnimInfo* animInfo) {
  ASSERT(animInfo);
  ASSERT(animInfo->unique);
  ASSERT(animInfo->shared);

  CAnimGeoset *geo = animInfo->shared->geo.Ptr();
  while (geo != animInfo->shared->geo.Ptr() + animInfo->shared->geo.Count()) {
    CAnimGeosetObjStatus &status = animInfo->unique->geosetStatus[geo - animInfo->shared->geo.Ptr()];
    unsigned int          wasVisible = status.base.flags & 1;
    CalcGeosetColor(*animInfo, geo, &status, &animInfo->data.geosetColor[geo->sgGeosetId]);
    if (wasVisible != (status.base.flags & 1)) {
      animInfo->unique->flags &= ~2u;
    }
    ++geo;
  }
}

static void AnimateAllMaterialLayers(AnimInfo* animInfo, unsigned int* tex) {
  ASSERT(animInfo);
  ASSERT(animInfo->unique);
  ASSERT(animInfo->shared);

  tex += animInfo->shared->layers.Count();
  CAnimMaterialLayer *layer = animInfo->shared->layers.Ptr();
  CAnimLayerStatus   *layerStatus = animInfo->unique->layerStatus.Ptr();
  float               alpha;
  unsigned int        numLayers = animInfo->shared->layers.Count();
  while (numLayers) {
    --tex;
    alpha = 0.0f;
    if (layer->visibility.InterpolateRetained(
            *animInfo,
            layerStatus->base,
            &layerStatus->visibility,
            1.0f,
            &alpha
        )) {
      animInfo->data.layerAlpha[layer->layerId] =
          NTempest::CMath::ftol_0_256_(min(max(alpha, 0.0f), 1.0f) * 255.0f);
    }

    *tex = static_cast<unsigned int>(-1);
    layer->flip.InterpolateRetained(*animInfo, layerStatus->base, &layerStatus->flipIndex, 0, tex);
    ++layer;
    ++layerStatus;
    --numLayers;
  }
}

static void ISetEventSequenceUnchanged(CAnim* container) {
  if (!(container->flags & 4)) {
    unsigned int count = container->eventStatus.Count();
    CAnimEventObjStatus *status = container->eventStatus.Ptr();
    while (count) {
      status->base.flags &= ~0x10;
      ++status;
      --count;
    }
    container->flags |= 4;
  }
}

static void ISetSequenceUnchanged(CAnim* container, CAnimData* animptr) {
  if (container->flags & 3) {
    return;
  }

  int setAllObjects = 1;
  for (unsigned int i = 0; i < container->status.Count(); ++i) {
    ASSERT(i < animptr->obj.Count());
    CAnimObj *object = animptr->obj[i];
    if (object->type == OBJ_TYPE_BONE) {
      CAnimBoneObj *bone = static_cast<CAnimBoneObj *>(object);
      int visible = bone->geosetId == 0xFF
                 || (bone->geosetId < container->geosetStatus.Count()
                     && (container->geosetStatus[bone->geosetId].base.flags & 1));
      if (!visible) {
        setAllObjects = 0;
        continue;
      }
    } else if (object->type == OBJ_TYPE_EVENT) {
      continue;
    }
    container->status[i]->base.flags &= ~0x10;
  }

  for (unsigned int camera = 0; camera < container->cameraStatus.Count(); ++camera) {
    container->cameraStatus[camera].base.flags &= ~0x10;
  }
  for (unsigned int texture = 0; texture < container->textureStatus.Count(); ++texture) {
    container->textureStatus[texture].base.flags &= ~0x10;
  }
  for (unsigned int geoset = 0; geoset < container->geosetStatus.Count(); ++geoset) {
    container->geosetStatus[geoset].base.flags &= ~0x10;
  }
  container->flags |= setAllObjects ? 1 : 2;
}

void AnimProcessEvents(HANIM anim, const TSFixedArray<NTempest::C3Vector> &positions) {
  CAnim *container = reinterpret_cast<CAnim *>(anim);
  ASSERT(container);

  CAnimData *animptr = reinterpret_cast<CAnimData *>(container->hdata);
  ASSERT(animptr);
  ASSERT(animptr->flags & 0x01);
  ASSERT((animptr->flags & 0x04) == 0);

  InterpInfo interpInfo(container, animptr, positions);
  CAnimEventObj *eventObject = animptr->eventObjs.Ptr();
  unsigned int count = animptr->eventObjs.Count();
  while (count) {
    IProcessEvent(interpInfo, eventObject);
    ++eventObject;
    --count;
  }
  ISetEventSequenceUnchanged(container);
}

void AnimAnimateCameras(HANIM anim, const TSFixedArray<HCAMERA> &cameras) {
  ASSERT(anim);
  ASSERT(reinterpret_cast<CAnim *>(anim)->hdata);
  ASSERT(reinterpret_cast<CAnimData *>(reinterpret_cast<CAnim *>(anim)->hdata)->flags & 1);
  ASSERT(!(reinterpret_cast<CAnimData *>(reinterpret_cast<CAnim *>(anim)->hdata)->flags & 4));

  TSFixedArray<NTempest::C3Vector> positions;
  CameraInfo cameraInfo(
      reinterpret_cast<CAnim *>(anim),
      reinterpret_cast<CAnimData *>(reinterpret_cast<CAnim *>(anim)->hdata),
      positions,
      cameras
  );
  AnimateAllCameras(&cameraInfo);
}

void IAnimAnimateModel(CAnim *unique, CAnimData *shared, const CAnimationData &data) {
  ASSERT(shared->flags & 1);
  ASSERT(!(shared->flags & 4));
  ASSERT(data.boneMtx || !data.numBones);

  AnimInfo animInfo(unique, shared, data);
  animInfo.cameraWorldPos = *data.cameraWorldPos;
  animInfo.cameraVector = *data.cameraVector;
  if (NTempest::CMath::fabs_(animInfo.cameraVector.SquaredMag()) >= 0.00000023841858f) {
    animInfo.cameraVector.Normalize();
  }
  GetWorldTransform(&animInfo);

  AnimateAllTextureMaps(&animInfo);
  AnimateAllGeosets(&animInfo);
  AnimateAllMaterialLayers(&animInfo, data.layerTextureIds);

  NTempest::C3Vector origin(0.0f);
  unsigned int       i;
  for (i = 0; i < shared->headarray.Count(); ++i) {
    PrepareObjectHierarchyViews(animInfo, shared->headarray[i], origin);
  }

  ISetSequenceUnchanged(unique, shared);
}

void AnimAnimateModel(HANIM anim, const CAnimationData &data) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);
  ASSERT(data.boneMtx || !data.numBones);
  ASSERT(data.textureMtx || !data.numTexBones);

  IAnimAnimateModel(unique, shared, data);
}

void AnimSetBoneProjectCallback(ANIMBONEPROJECTCALLBACK callback, float distance) {
  s_AnimBoneProjectCallback = callback;
  s_animBoneProjectDistance = distance;
}

void AnimGetBoneProjectCallback(ANIMBONEPROJECTCALLBACK &callback, float &dist) {
  callback = s_AnimBoneProjectCallback;
  dist = s_animBoneProjectDistance;
}
