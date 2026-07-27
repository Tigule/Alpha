#include "Anim/Transform.h"

struct CameraInfo : public InterpInfo {
  CameraInfo(
      CAnim *container,
      CAnimData *animptr,
      const TSFixedArray<NTempest::C3Vector> &positions,
      const TSFixedArray<HCAMERA> &cameras
  ) : InterpInfo(container, animptr, positions), cameras(cameras) {
  }

  const TSFixedArray<HCAMERA> &cameras;
};

#include "Anim/WorldMatrix.h"
#include "Base/Activity.h"
#include "Gx/CGxDevice.h"
#include "Gxu/IGxuLight.h"
#include "Model/ModelInternal.h"
#include "Services/ParticleSystem2.h"
#include "Services/RibbonEmitter.h"
#include "Tempest/c33matrix.h"
#include "Tempest/cmath.h"
#include "Tempest/c4quaternion.h"

const float PI = 3.14159265358979323846f;
const float TWO_PI = 6.28318530717958647692f;
const float OO_TWO_PI = 0.15915494309189533577f;

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

  if (m_trackType == TRACK_DONT_INTERP) {
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

  if (m_trackType == TRACK_DONT_INTERP) {
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

  if (m_trackType == TRACK_DONT_INTERP) {
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
  CAnimEventObjStatus *status =
      static_cast<CAnimEventObjStatus *>(animInfo.unique->status[currobj->animObjId]);
  FATALASSERT(currobj->splitIndex < animInfo.positions.Count());

  status->position = animInfo.positions[currobj->splitIndex];
  WorldMatrixTransform(&status->position);

  NTempest::C34Matrix basis(
      animInfo.basisX.x, animInfo.basisX.y, animInfo.basisX.z,
      animInfo.basisY.x, animInfo.basisY.y, animInfo.basisY.z,
      animInfo.basisZ.x, animInfo.basisZ.y, animInfo.basisZ.z,
      animInfo.basisPosition.x, animInfo.basisPosition.y, animInfo.basisPosition.z);
  NTempest::C34Matrix inverse = basis.AffineInverse();
  status->position *= inverse;
}

static void IProcessEvent(const InterpInfo& animInfo, CAnimEventObj* currEvent) {
  if (!animInfo.unique->appEvent.callback || (animInfo.unique->flags & 8) ||
      !currEvent->events.TotalKeys()) {
    return;
  }

  FATALASSERT(currEvent->animObjId < animInfo.unique->status.Count());
  CAnimEventObjStatus *status =
      static_cast<CAnimEventObjStatus *>(animInfo.unique->status[currEvent->animObjId]);
  CKeyTrackStatus previous = status->event;
  if (!currEvent->events.SetAnimTime(status->base, &status->event, animInfo)) {
    return;
  }

  if (previous.currKey != status->event.currKey ||
      previous.timepastkey > status->event.timepastkey) {
    NTempest::C3Vector position = status->position;
    WorldMatrixTransform(&position);
    ActivityBegin(ACTIVITY_ANIMEVENTS);
    animInfo.unique->appEvent.callback(
        currEvent->name, position, animInfo.unique->appEvent.param);
    ActivityEnd(ACTIVITY_ANIMEVENTS);
  }
}

static void PlaceModelObject(const AnimInfo& animInfo, CAnimModelObj* modelObj) {
  CAnimModelObjStatus *status =
      static_cast<CAnimModelObjStatus *>(animInfo.unique->status[modelObj->animObjId]);
  status->visible = AnimFloat(modelObj->visibility, status->base, status->visibility, animInfo, 1.0f);
  FATALASSERT(modelObj->splitIndex < animInfo.data.numAttached);
  WorldMatrixGet(animInfo.data.attached + modelObj->splitIndex);
}

static void SetLightColor(const AnimInfo& animInfo, CAnimLightObj* currobj, CAnimLightObjStatus* lightStatus, CGxLight* light) {
  if (currobj->color.TotalKeys()) {
    C3Color color = AnimColor(currobj->color, lightStatus->base, lightStatus->color, animInfo);
    light->m_dirColor.r = NTempest::CMath::ftol_0_256_(min(max(color.r, 0.0f), 1.0f) * 255.0f);
    light->m_dirColor.g = NTempest::CMath::ftol_0_256_(min(max(color.g, 0.0f), 1.0f) * 255.0f);
    light->m_dirColor.b = NTempest::CMath::ftol_0_256_(min(max(color.b, 0.0f), 1.0f) * 255.0f);
  }
  if (currobj->ambColor.TotalKeys()) {
    C3Color color = AnimColor(currobj->ambColor, lightStatus->base, lightStatus->ambColor, animInfo);
    light->m_ambColor.r = NTempest::CMath::ftol_0_256_(min(max(color.r, 0.0f), 1.0f) * 255.0f);
    light->m_ambColor.g = NTempest::CMath::ftol_0_256_(min(max(color.g, 0.0f), 1.0f) * 255.0f);
    light->m_ambColor.b = NTempest::CMath::ftol_0_256_(min(max(color.b, 0.0f), 1.0f) * 255.0f);
  }
}

static void SetLightIntensity(const AnimInfo& animInfo, CAnimLightObj* currobj, CAnimLightObjStatus* lightStatus, CGxLight* light) {
  if (currobj->intensity.TotalKeys()) {
    light->m_dirIntensity = max(
        AnimFloat(currobj->intensity, lightStatus->base, lightStatus->intensity, animInfo, 0.0f), 0.0f
    );
  }
  if (currobj->ambIntensity.TotalKeys()) {
    light->m_ambIntensity = max(
        AnimFloat(currobj->ambIntensity, lightStatus->base, lightStatus->ambIntensity, animInfo, 0.0f), 0.0f
    );
  }
}

static void SetLightDirection(CGxLight* light) {
  light->m_dir = NTempest::C3Vector(0.0f, 0.0f, -1.0f);
  WorldMatrixPush();
  WorldMatrixRemove(1);
  WorldMatrixTransform(&light->m_dir);
  WorldMatrixPop();
}

static void SetLightValues(const AnimInfo& animInfo, CAnimLightObj* currobj, const NTempest::C3Vector& currPos) {
  ASSERT(animInfo.data.lights);
  ASSERT(currobj->splitIndex < animInfo.data.lights->Count());
  unsigned long lightID = (*animInfo.data.lights)[currobj->splitIndex];
  if (!lightID) {
    return;
  }
  CAnimLightObjStatus *status =
      static_cast<CAnimLightObjStatus *>(animInfo.unique->status[currobj->animObjId]);
  CGxLight *light = GxuLightLock(lightID);
  ASSERT(light);
  light->m_enabled = AnimFloat(currobj->visibility, status->base, status->visibility, animInfo, 1.0f) > 0.0f;
  if (light->m_enabled) {
    SetLightColor(animInfo, currobj, status, light);
    SetLightIntensity(animInfo, currobj, status, light);
    if (light->m_isOmni) {
      light->m_dir = currPos;
      WorldMatrixTransform(&light->m_dir);
      light->m_dir += animInfo.cameraWorldPos;
    } else {
      SetLightDirection(light);
    }
  }
  GxuLightUnlock(lightID);
}

static void SetParticleVariation2(const AnimInfo& animInfo, CAnimEmitter2Obj* currobj, CAnimEmitter2ObjStatus* status) {
  if (currobj->variation.TotalKeys()) {
    (*animInfo.data.emitters2)[currobj->splitIndex]->SetVelocityVariation(
        AnimFloat(currobj->variation, status->base, status->variation, animInfo, 0.0f)
    );
  }
}

static void SetParticleSpeed2(const AnimInfo& animInfo, CAnimEmitter2Obj* currobj, CAnimEmitter2ObjStatus* status) {
  if (currobj->particleSpeed.TotalKeys()) {
    (*animInfo.data.emitters2)[currobj->splitIndex]->SetVelocity(
        AnimFloat(currobj->particleSpeed, status->base, status->speed, animInfo, 0.0f)
    );
  }
}

static void SetParticleEmissionRate2(const AnimInfo& animInfo, CAnimEmitter2Obj* currobj, CAnimEmitter2ObjStatus* status) {
  if (currobj->emissionRate.TotalKeys()) {
    (*animInfo.data.emitters2)[currobj->splitIndex]->SetEmissionRate(
        AnimFloat(currobj->emissionRate, status->base, status->emissionRate, animInfo, 0.0f)
    );
  }
}

static void SetParticleGravity2(const AnimInfo& animInfo, CAnimEmitter2Obj* currobj, CAnimEmitter2ObjStatus* status) {
  if (currobj->gravity.TotalKeys()) {
    (*animInfo.data.emitters2)[currobj->splitIndex]->SetAcceleration(
        AnimFloat(currobj->gravity, status->base, status->gravity, animInfo, 0.0f)
    );
  }
}

static void SetEmitterLatitude2(const AnimInfo& animInfo, CAnimEmitter2Obj* currobj, CAnimEmitter2ObjStatus* status) {
  if (currobj->latitude.TotalKeys()) {
    (*animInfo.data.emitters2)[currobj->splitIndex]->SetLatitude(
        AnimFloat(currobj->latitude, status->base, status->latitude, animInfo, 0.0f)
    );
  }
}

static void SetEmitterLongitude2(const AnimInfo& animInfo, CAnimEmitter2Obj* currobj, CAnimEmitter2ObjStatus* status) {
  if (currobj->longitude.TotalKeys()) {
    (*animInfo.data.emitters2)[currobj->splitIndex]->SetLongitude(
        AnimFloat(currobj->longitude, status->base, status->longitude, animInfo, 0.0f)
    );
  }
}

static void SetEmitterWidth2(const AnimInfo& animInfo, CAnimEmitter2Obj* currobj, CAnimEmitter2ObjStatus* status) {
  if (currobj->width.TotalKeys()) {
    (*animInfo.data.emitters2)[currobj->splitIndex]->SetWidth(
        AnimFloat(currobj->width, status->base, status->width, animInfo, 0.0f)
    );
  }
}

static void SetEmitterLength2(const AnimInfo& animInfo, CAnimEmitter2Obj* currobj, CAnimEmitter2ObjStatus* status) {
  if (currobj->length.TotalKeys()) {
    (*animInfo.data.emitters2)[currobj->splitIndex]->SetHeight(
        AnimFloat(currobj->length, status->base, status->length, animInfo, 0.0f)
    );
  }
}

static void SetEmitterZsource2(const AnimInfo& animInfo, CAnimEmitter2Obj* currobj, CAnimEmitter2ObjStatus* status) {
  if (currobj->zsource.TotalKeys()) {
    (*animInfo.data.emitters2)[currobj->splitIndex]->SetZsource(
        AnimFloat(currobj->zsource, status->base, status->zsource, animInfo, 0.0f)
    );
  }
}

static void SetEmitterLifeSpan2(const AnimInfo& animInfo, CAnimEmitter2Obj* currobj, CAnimEmitter2ObjStatus* status) {
  if (currobj->lifeSpan.TotalKeys()) {
    (*animInfo.data.emitters2)[currobj->splitIndex]->SetLifeSpan(
        AnimFloat(currobj->lifeSpan, status->base, status->lifeSpan, animInfo, 0.1f)
    );
  }
}

static void SetRibbonHeight(const AnimInfo& animInfo, CAnimRibbonObj* currobj, CAnimRibbonObjStatus* status) {
  CRibbonEmitter *emitter = (*animInfo.data.ribbons)[currobj->splitIndex];
  float value;
  if (currobj->heightAbove.InterpolateRetained(animInfo, status->base, &status->heightAbove, 0.0f, &value)) {
    emitter->SetAbove(value);
  }
  if (currobj->heightBelow.InterpolateRetained(animInfo, status->base, &status->heightBelow, 0.0f, &value)) {
    emitter->SetBelow(value);
  }
}

static void SetRibbonTexSlot(const AnimInfo& animInfo, CAnimRibbonObj* currobj, CAnimRibbonObjStatus* status) {
  unsigned int slot;
  if (currobj->slot.InterpolateRetained(animInfo, status->base, &status->slot, 0, &slot)) {
    (*animInfo.data.ribbons)[currobj->splitIndex]->SetTexSlot(slot);
  }
}

static void SetRibbonColor(const AnimInfo& animInfo, CAnimRibbonObj* currobj, CAnimRibbonObjStatus* status) {
  C3Color fallback;
  C3Color color;
  if (currobj->color.InterpolateRetained(animInfo, status->base, &status->color, fallback, &color)) {
    (*animInfo.data.ribbons)[currobj->splitIndex]->SetColor(color.r, color.g, color.b);
  }
}

static void SetRibbonAlpha(const AnimInfo& animInfo, CAnimRibbonObj* currobj, CAnimRibbonObjStatus* status) {
  float alpha;
  if (currobj->alpha.InterpolateRetained(animInfo, status->base, &status->alpha, 1.0f, &alpha)) {
    (*animInfo.data.ribbons)[currobj->splitIndex]->SetAlpha(alpha);
  }
}

static void SetEmitter2Values(const AnimInfo& animInfo, CAnimEmitter2Obj* currobj) {
  ASSERT(animInfo.data.emitters2);
  ASSERT(currobj->splitIndex < animInfo.data.emitters2->Count());
  CAnimEmitter2ObjStatus *status =
      static_cast<CAnimEmitter2ObjStatus *>(animInfo.unique->status[currobj->animObjId]);
  CParticleEmitter2 *emitter = (*animInfo.data.emitters2)[currobj->splitIndex];
  emitter->SetEnabled(AnimFloat(currobj->visibility, status->base, status->visibility, animInfo, 1.0f) != 0.0f, 0);
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
  NTempest::C34Matrix matrix;
  WorldMatrixGet(&matrix);
  matrix.Rotate(PI * 0.5f, NTempest::C3Vector(0.0f, 0.0f, 1.0f), true);
  emitter->Update(status->elapsedTime, matrix, animInfo.cameraWorldPos);
}

static void SetRibbonValues(const AnimInfo& animInfo, CAnimRibbonObj* currobj) {
  FATALASSERT(currobj);
  FATALASSERT(currobj->type == OBJ_TYPE_RIBBON);
  FATALASSERT(currobj->splitIndex < animInfo.data.ribbons->Count());

  CAnimRibbonObjStatus *status = &animInfo.unique->ribbonStatus[currobj->splitIndex];
  CRibbonEmitter *emitter = (*animInfo.data.ribbons)[currobj->splitIndex];
  float visible;
  if (currobj->visibility.InterpolateRetained(animInfo, status->base, &status->visibility, 1.0f, &visible)) {
    emitter->SetEnabled(visible > 0.0f);
  }
  SetRibbonHeight(animInfo, currobj, status);
  SetRibbonTexSlot(animInfo, currobj, status);
  SetRibbonColor(animInfo, currobj, status);
  SetRibbonAlpha(animInfo, currobj, status);

  NTempest::C34Matrix current;
  WorldMatrixGet(&current);
  NTempest::C44Matrix orient(
      current.a0, current.a1, current.a2, 0.0f,
      current.b0, current.b1, current.b2, 0.0f,
      current.c0, current.c1, current.c2, 0.0f,
      current.d0, current.d1, current.d2, 1.0f
  );
  emitter->SetPos(orient, animInfo.cameraWorldPos);
}

static void PlaceObject(const AnimInfo &animInfo, CAnimObj *currobj, const NTempest::C3Vector &currPos) {
  CAnimObjStatus *status = animInfo.unique->status[currobj->animObjId];

  if (currobj->type == 1 || currobj->type == 3 || currobj->type == 6) {
    WorldMatrixTranslate(NTempest::C3Vector(-currPos.x, -currPos.y, -currPos.z));
  }

  if (currobj->type == 3) {
    ASSERT(currobj->splitIndex < animInfo.data.numBones);
    WorldMatrixGet(animInfo.data.boneMtx + currobj->splitIndex);
  } else if (currobj->type == 2) {
    CAnimModelObj       *model = static_cast<CAnimModelObj *>(currobj);
    CAnimModelObjStatus *modelStatus = static_cast<CAnimModelObjStatus *>(status);
    modelStatus->visible = AnimFloat(model->visibility, status->base, modelStatus->visibility, animInfo, 1.0f);
    if (currobj->splitIndex < animInfo.data.numAttached) {
      WorldMatrixGet(animInfo.data.attached + currobj->splitIndex);
    }
  } else if (currobj->type == 4 && animInfo.data.emitters2 && currobj->splitIndex < animInfo.data.emitters2->Count()) {
    CAnimEmitter2Obj       *emitterObject = static_cast<CAnimEmitter2Obj *>(currobj);
    CAnimEmitter2ObjStatus *emitterStatus = static_cast<CAnimEmitter2ObjStatus *>(status);
    CParticleEmitter2      *emitter = (*animInfo.data.emitters2)[currobj->splitIndex];
    emitter->SetEnabled(AnimFloat(emitterObject->visibility, status->base, emitterStatus->visibility, animInfo, 1.0f) != 0.0f, 0);
    if (emitterObject->particleSpeed.TotalKeys()) {
      emitter->SetVelocity(AnimFloat(emitterObject->particleSpeed, status->base, emitterStatus->speed, animInfo, 0.0f));
    }
    if (emitterObject->emissionRate.TotalKeys()) {
      emitter->SetEmissionRate(AnimFloat(emitterObject->emissionRate, status->base, emitterStatus->emissionRate, animInfo, 0.0f));
    }
    if (emitterObject->gravity.TotalKeys()) {
      emitter->SetAcceleration(AnimFloat(emitterObject->gravity, status->base, emitterStatus->gravity, animInfo, 0.0f));
    }
    if (emitterObject->variation.TotalKeys()) {
      emitter->SetVelocityVariation(AnimFloat(emitterObject->variation, status->base, emitterStatus->variation, animInfo, 0.0f));
    }
    if (emitterObject->latitude.TotalKeys()) {
      emitter->SetLatitude(AnimFloat(emitterObject->latitude, status->base, emitterStatus->latitude, animInfo, 0.0f));
    }
    if (emitterObject->longitude.TotalKeys()) {
      emitter->SetLongitude(AnimFloat(emitterObject->longitude, status->base, emitterStatus->longitude, animInfo, 0.0f));
    }
    if (emitterObject->length.TotalKeys()) {
      emitter->SetHeight(AnimFloat(emitterObject->length, status->base, emitterStatus->length, animInfo, 0.0f));
    }
    if (emitterObject->width.TotalKeys()) {
      emitter->SetWidth(AnimFloat(emitterObject->width, status->base, emitterStatus->width, animInfo, 0.0f));
    }
    if (emitterObject->zsource.TotalKeys()) {
      emitter->SetZsource(AnimFloat(emitterObject->zsource, status->base, emitterStatus->zsource, animInfo, 0.0f));
    }
    if (emitterObject->lifeSpan.TotalKeys()) {
      emitter->SetLifeSpan(AnimFloat(emitterObject->lifeSpan, status->base, emitterStatus->lifeSpan, animInfo, 0.1f));
    }
    NTempest::C34Matrix emitterMatrix;
    WorldMatrixGet(&emitterMatrix);
    emitterMatrix.Rotate(PI * 0.5f, NTempest::C3Vector(0.0f, 0.0f, 1.0f), true);
    emitter->Update(emitterStatus->elapsedTime, emitterMatrix, animInfo.cameraWorldPos);
  } else if (currobj->type == 1 && animInfo.data.lights && currobj->splitIndex < animInfo.data.lights->Count()) {
    CAnimLightObj       *lightObject = static_cast<CAnimLightObj *>(currobj);
    CAnimLightObjStatus *lightStatus = reinterpret_cast<CAnimLightObjStatus *>(status);
    unsigned long        lightId = (*animInfo.data.lights)[currobj->splitIndex];
    if (lightId) {
      CGxLight *light = GxuLightLock(lightId);
      ASSERT(light);
      float visible = AnimFloat(lightObject->visibility, status->base, lightStatus->visibility, animInfo, 1.0f);
      light->m_enabled = visible > 0.0f;
      if (light->m_enabled) {
        if (lightObject->color.TotalKeys()) {
          C3Color dir = AnimColor(lightObject->color, status->base, lightStatus->color, animInfo);
          light->m_dirColor.r = NTempest::CMath::ftol_0_256_(min(max(dir.r, 0.0f), 1.0f) * 255.0f);
          light->m_dirColor.g = NTempest::CMath::ftol_0_256_(min(max(dir.g, 0.0f), 1.0f) * 255.0f);
          light->m_dirColor.b = NTempest::CMath::ftol_0_256_(min(max(dir.b, 0.0f), 1.0f) * 255.0f);
        }
        if (lightObject->ambColor.TotalKeys()) {
          C3Color amb = AnimColor(lightObject->ambColor, status->base, lightStatus->ambColor, animInfo);
          light->m_ambColor.r = NTempest::CMath::ftol_0_256_(min(max(amb.r, 0.0f), 1.0f) * 255.0f);
          light->m_ambColor.g = NTempest::CMath::ftol_0_256_(min(max(amb.g, 0.0f), 1.0f) * 255.0f);
          light->m_ambColor.b = NTempest::CMath::ftol_0_256_(min(max(amb.b, 0.0f), 1.0f) * 255.0f);
        }
        if (lightObject->intensity.TotalKeys()) {
          light->m_dirIntensity = AnimFloat(lightObject->intensity, status->base, lightStatus->intensity, animInfo, 0.0f);
          if (light->m_dirIntensity < 0.0f) {
            light->m_dirIntensity = 0.0f;
          }
        }
        if (lightObject->ambIntensity.TotalKeys()) {
          light->m_ambIntensity = AnimFloat(lightObject->ambIntensity, status->base, lightStatus->ambIntensity, animInfo, 0.0f);
          if (light->m_ambIntensity < 0.0f) {
            light->m_ambIntensity = 0.0f;
          }
        }
        if (!light->m_isOmni) {
          light->m_dir = NTempest::C3Vector(0.0f, 0.0f, -1.0f);
          WorldMatrixPush();
          WorldMatrixRemove(1);
          WorldMatrixTransform(&light->m_dir);
          WorldMatrixPop();
        } else {
          light->m_dir = currPos;
          WorldMatrixTransform(&light->m_dir);
          light->m_dir.x += animInfo.cameraWorldPos.x;
          light->m_dir.y += animInfo.cameraWorldPos.y;
          light->m_dir.z += animInfo.cameraWorldPos.z;
        }
      }
      GxuLightUnlock(lightId);
    }
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

  if (currobj->type == 3) {
    CAnimBoneObj *bone = static_cast<CAnimBoneObj *>(currobj);
    if (bone->geosetId != 0xFF && !(animInfo.unique->geosetStatus[bone->geosetId].base.flags & 1)) {
      return;
    }
  }

  WorldMatrixPush();
  NTempest::C3Vector currPosition = animInfo.positions[currobj->animObjId];
  TransformObjectView(animInfo, currobj, currPosition, parentPos);
  for (unsigned int i = 0; i < currobj->childarray.Count(); ++i) {
    PrepareObjectHierarchyViews(animInfo, currobj->childarray[i], currPosition);
  }
  PlaceObject(animInfo, currobj, currPosition);
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
  status->visible = AnimFloat(object->visibility, status->base, status->visibility, animInfo, 1.0f);
  NTempest::C3Vector position =
      AnimVector(object->translation, status->base, status->translation, animInfo, object->pivot);
  position += object->pivot;
  NTempest::C3Vector target =
      AnimVector(object->targetTranslation, status->base, status->targetTranslation, animInfo, object->targetPivot);
  target += object->targetPivot;
  float roll = AnimFloat(object->roll, status->base, status->roll, animInfo, 0.0f);
  DataMgrSetCoord(camera, 7, position, 0);
  DataMgrSetCoord(camera, 8, target, 0);
  DataMgrSetFloat(camera, 5, roll);
}

static void AnimateAllCameras(CameraInfo* animInfo) {
  FATALASSERT(animInfo);
  FATALASSERT(animInfo->unique);
  FATALASSERT(animInfo->shared);
  unsigned int count = min(animInfo->cameras.Count(), animInfo->shared->cameraObjs.Count());
  for (unsigned int i = 0; i < count; ++i) {
    AnimateCamera(
        *animInfo,
        &animInfo->shared->cameraObjs[i],
        &animInfo->unique->cameraStatus[i],
        animInfo->cameras[i]
    );
  }
}

static void AnimateAllTextureMaps(AnimInfo* animInfo) {
  ASSERT(animInfo);
  ASSERT(animInfo->unique);
  ASSERT(animInfo->shared);

  for (unsigned int i = 0; i < animInfo->shared->tex.Count(); ++i) {
    ASSERT(i < animInfo->data.numTexBones);
    CAnimTransform      &tex = animInfo->shared->tex[i];
    CAnimObjStatus      &status = animInfo->unique->textureStatus[i];
    NTempest::C34Matrix &matrix = animInfo->data.textureMtx[i];
    AnimateTextureMap(*animInfo, &tex, &status, &matrix);
  }
}

static void AnimateAllGeosets(AnimInfo* animInfo) {
  ASSERT(animInfo);
  ASSERT(animInfo->unique);
  ASSERT(animInfo->shared);

  for (unsigned int i = 0; i < animInfo->shared->geo.Count(); ++i) {
    CAnimGeoset          &geo = animInfo->shared->geo[i];
    CAnimGeosetObjStatus &status = animInfo->unique->geosetStatus[i];
    CGeosetColor         &color = animInfo->data.geosetColor[geo.sgGeosetId];
    unsigned int          wasVisible = status.base.flags & 1;
    CalcGeosetColor(*animInfo, &geo, &status, &color);
    if (wasVisible != (status.base.flags & 1)) {
      animInfo->unique->flags &= ~2u;
    }
  }
}

static void AnimateAllMaterialLayers(AnimInfo* animInfo, unsigned int* tex) {
  ASSERT(animInfo);
  ASSERT(animInfo->unique);
  ASSERT(animInfo->shared);

  tex += animInfo->shared->layers.Count();
  for (unsigned int i = 0; i < animInfo->shared->layers.Count(); ++i) {
    --tex;
    CAnimMaterialLayer &layer = animInfo->shared->layers[i];
    CAnimLayerStatus   &status = animInfo->unique->layerStatus[i];
    if (layer.visibility.TotalKeys()) {
      float        alpha = 0.0f;
      unsigned int keys = layer.visibility.SetAnimTime(status.base, &status.visibility, *animInfo);
      int          updateAlpha = 1;
      if (keys > 1) {
        const CAnimSequence &sequence = animInfo->shared->seq[status.base.currSeq];
        layer.visibility.Interpolate(status.visibility, sequence.time.h - sequence.time.l, &alpha);
      } else {
        if (status.base.flags & 0x10) {
          if (keys) {
            alpha = reinterpret_cast<const CLinearKeyFrame<float> *>(
                        layer.visibility.GetKeyFrame(status.visibility.currKey)
            )->transform;
          } else {
            alpha = 1.0f;
          }
        } else {
          updateAlpha = 0;
        }
      }
      if (updateAlpha) {
        animInfo->data.layerAlpha[layer.layerId] =
            NTempest::CMath::ftol_0_256_(min(max(alpha, 0.0f), 1.0f) * 255.0f);
      }
    }

    *tex = static_cast<unsigned int>(-1);
    if (layer.flip.TotalKeys()) {
      unsigned int keys = layer.flip.SetAnimTime(status.base, &status.flipIndex, *animInfo);
      if (keys > 1) {
        const CAnimSequence &sequence = animInfo->shared->seq[status.base.currSeq];
        layer.flip.Interpolate(status.flipIndex, sequence.time.h - sequence.time.l, tex);
      } else if (status.base.flags & 0x10) {
        if (keys) {
          *tex = reinterpret_cast<const CLinearKeyFrame<unsigned int> *>(
                     layer.flip.GetKeyFrame(status.flipIndex.currKey)
          )->transform;
        } else {
          *tex = 0;
        }
      }
    }
  }
}

static void ISetEventSequenceUnchanged(CAnim* container) {
  if (!(container->flags & 4)) {
    for (unsigned int i = 0; i < container->eventStatus.Count(); ++i) {
      container->eventStatus[i].base.flags &= ~0x10;
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
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);
  ASSERT(shared->flags & 1);
  ASSERT(!(shared->flags & 4));

  InterpInfo interpInfo(unique, shared, positions);
  for (unsigned int i = 0; i < shared->eventObjs.Count(); ++i) {
    CAnimEventObj       &eventObject = shared->eventObjs[i];
    CAnimEventObjStatus &status = unique->eventStatus[i];
    if (!unique->appEvent.callback || (unique->flags & 8) || !eventObject.events.TotalKeys()) {
      continue;
    }

    if (eventObject.animObjId < positions.Count()) {
      status.position = positions[eventObject.animObjId];
    }
    if (eventObject.events.SetAnimTime(status.base, &status.event, interpInfo)) {
      unique->appEvent.callback(eventObject.name, status.position, unique->appEvent.param);
    }
  }

  if (!(unique->flags & 4)) {
    for (unsigned int i = 0; i < unique->eventStatus.Count(); ++i) {
      unique->eventStatus[i].base.flags &= ~0x10;
    }
    unique->flags |= 4;
  }
}

void AnimAnimateCameras(HANIM anim, const TSFixedArray<HCAMERA> &cameras) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);
  ASSERT(shared->flags & 1);
  ASSERT(!(shared->flags & 4));

  TSFixedArray<NTempest::C3Vector> positions;
  InterpInfo                       interpInfo(unique, shared, positions);
  unsigned int                     count = min(cameras.Count(), shared->cameraObjs.Count());
  for (unsigned int i = 0; i < count; ++i) {
    CAnimCameraObj       &cameraObject = shared->cameraObjs[i];
    CAnimCameraObjStatus &status = unique->cameraStatus[i];
    status.visible = AnimFloat(cameraObject.visibility, status.base, status.visibility, interpInfo, 1.0f);

    NTempest::C3Vector position = AnimVector(cameraObject.translation, status.base, status.translation, interpInfo, cameraObject.pivot);
    position += cameraObject.pivot;
    NTempest::C3Vector target =
        AnimVector(cameraObject.targetTranslation, status.base, status.targetTranslation, interpInfo, cameraObject.targetPivot);
    target += cameraObject.targetPivot;
    float roll = AnimFloat(cameraObject.roll, status.base, status.roll, interpInfo, 0.0f);
    DataMgrSetCoord(cameras[i], 7, position, 0);
    DataMgrSetCoord(cameras[i], 8, target, 0);
    DataMgrSetFloat(cameras[i], 5, roll);
    status.base.flags &= ~0x10;
  }
}

void IAnimAnimateModel(CAnim *unique, CAnimData *shared, const CAnimationData &data) {
  ASSERT(shared->flags & 1);
  ASSERT(!(shared->flags & 4));
  ASSERT(data.boneMtx || !data.numBones);

  AnimInfo animInfo(unique, shared, *data.positions, data);
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

  if (unique->flags & 3) {
    return;
  }

  int setAllObjects = 1;
  for (i = 0; i < unique->status.Count(); ++i) {
    CAnimObj *object = shared->obj[i];
    if (object->type == 3) {
      CAnimBoneObj *bone = static_cast<CAnimBoneObj *>(object);
      if (bone->geosetId != 0xFF && !(unique->geosetStatus[bone->geosetId].base.flags & 1)) {
        setAllObjects = 0;
        continue;
      }
    }
    if (object->type != 6) {
      unique->status[i]->base.flags &= ~0x10;
    }
  }
  for (i = 0; i < unique->cameraStatus.Count(); ++i) {
    unique->cameraStatus[i].base.flags &= ~0x10;
  }
  for (i = 0; i < unique->textureStatus.Count(); ++i) {
    unique->textureStatus[i].base.flags &= ~0x10;
  }
  for (i = 0; i < unique->geosetStatus.Count(); ++i) {
    unique->geosetStatus[i].base.flags &= ~0x10;
  }
  if (setAllObjects)
    unique->flags |= 1;
  else
    unique->flags |= 2;
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

void AnimGetBoneProjectCallback(ANIMBONEPROJECTCALLBACK &callback, float &distance) {
  callback = s_AnimBoneProjectCallback;
  distance = s_animBoneProjectDistance;
}

InterpInfo::InterpInfo(CAnim *container, CAnimData *animptr, const TSFixedArray<NTempest::C3Vector> &positions)
    : unique(container), shared(animptr), positions(positions) {
}
