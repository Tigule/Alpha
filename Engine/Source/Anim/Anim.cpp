#include "Anim/AnimInternal.h"

#include "Anim/WorldMatrix.h"
#include "Gx/CGxDevice.h"
#include "Gxu/IGxuLight.h"
#include "Model/ModelInternal.h"
#include "Services/ParticleSystem2.h"
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
      result[i] = (2.0f * ratio3 - 3.0f * ratio2 + 1.0f) * curr[i] + (ratio3 - 2.0f * ratio2 + ratio) * outTan[i] +
                  (-2.0f * ratio3 + 3.0f * ratio2) * next[i] * nextSign + (ratio3 - ratio2) * inTan[i];
    } else {
      float oneMinusRatio = 1.0f - ratio;
      float ratio2 = ratio * ratio;
      float ratio3 = ratio2 * ratio;
      result[i] = oneMinusRatio * oneMinusRatio * oneMinusRatio * curr[i] + 3.0f * oneMinusRatio * oneMinusRatio * ratio * outTan[i] +
                  3.0f * oneMinusRatio * ratio2 * inTan[i] + ratio3 * next[i] * nextSign;
    }
  }
}

void __fastcall
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

static void __fastcall LookAtPoint(const NTempest::C3Vector &position, const NTempest::C3Vector &point, NTempest::C4Quaternion *result) {
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

void __fastcall RotateViewBillboarded(const NTempest::C3Vector &cameraVector) {
  NTempest::C3Vector xprime;
  NTempest::C3Vector yprime;
  NTempest::C3Vector zprime;

  FaceDirection(NTempest::C3Vector(-cameraVector.x, -cameraVector.y, -cameraVector.z), &xprime, &yprime, &zprime);
  WorldMatrixRemove(4);
  WorldMatrixBasis(xprime, yprime, zprime);
}

void __fastcall RotateViewZAxisBillboarded(const NTempest::C3Vector &cameraVector) {
  ASSERT(NTempest::CMath::fabs_(cameraVector.SquaredMag()) >= 0.00000023841858f);
  NTempest::C3Vector xprime;
  NTempest::C3Vector zprime;
  NTempest::C3Vector yprime;

  WorldMatrixGetRow(2, &zprime);
  xprime = NTempest::C3Vector::Cross(zprime, NTempest::C3Vector(-cameraVector.x, -cameraVector.y, -cameraVector.z));
  xprime.Normalize();
  yprime = NTempest::C3Vector::Cross(zprime, xprime);
  WorldMatrixRemove(4);
  WorldMatrixBasis(xprime, yprime, zprime);
}

void __fastcall RotateViewYAxisBillboarded(const NTempest::C3Vector &cameraVector) {
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

void __fastcall RotateViewXAxisBillboarded(const NTempest::C3Vector &cameraVector) {
  ASSERT(NTempest::CMath::fabs_(cameraVector.SquaredMag()) >= 0.00000023841858f);
  NTempest::C3Vector yprime;
  NTempest::C3Vector xprime;
  NTempest::C3Vector zprime;

  WorldMatrixGetRow(0, &xprime);
  zprime = NTempest::C3Vector::Cross(NTempest::C3Vector(-cameraVector.x, -cameraVector.y, -cameraVector.z), xprime);
  zprime.Normalize();
  yprime = NTempest::C3Vector::Cross(zprime, xprime);
  WorldMatrixRemove(4);
  WorldMatrixBasis(xprime, yprime, zprime);
}

namespace {

  template <class T, class U>
  static const T *__fastcall AnimKeyValue(const CKeyFrameTrack<T, U> &track, unsigned int key) {
    const unsigned char *p = reinterpret_cast<const unsigned char *>(track.m_keyFrames) + track.m_keyFrameSize * key + sizeof(int);
    return reinterpret_cast<const T *>(p);
  }

  static inline float __fastcall
  AnimFloat(CKeyFrameTrack<float, float> &track, CBaseStatus &base, CKeyTrackStatus &keyStatus, const InterpInfo &info, float fallback) {
    float value;
    track.InterpolateRetained(info, base, &keyStatus, fallback, &value);
    return value;
  }

  static inline NTempest::C3Vector __fastcall AnimVector(
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

  static inline C3Color __fastcall
  AnimColor(CKeyFrameTrack<C3Color, C3Color> &track, CBaseStatus &base, CKeyTrackStatus &keyStatus, const InterpInfo &info) {
    C3Color fallback;
    C3Color result;
    track.InterpolateRetained(info, base, &keyStatus, fallback, &result);
    return result;
  }

}  // namespace

template <>
int CKeyFrameTrack<NTempest::C4QuaternionCompressed, NTempest::C4Quaternion>::InterpolateVolatileFewKeys(
    const CKeyTrackStatus  &keyStat,
    NTempest::C4Quaternion *transform
) {
  ASSERT(transform);
  *transform = reinterpret_cast<const CLinearKeyFrame<NTempest::C4QuaternionCompressed> *>(GetKeyFrame(keyStat.currKey))->transform;
  return 1;
}

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
    const CLinearKeyFrame<NTempest::C4QuaternionCompressed> *key = reinterpret_cast<const CLinearKeyFrame<NTempest::C4QuaternionCompressed> *>(curr);
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

static void __fastcall PlaceObject(const AnimInfo &animInfo, CAnimObj *currobj, const NTempest::C3Vector &currPos) {
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

static void __fastcall ApplyFaceDir(const AnimInfo &animInfo, CAnimObj *currobj) {
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

static int __fastcall ApplyLookAt(const AnimInfo &animInfo, CAnimObj *currobj, const NTempest::C3Vector &currPos) {
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

static void __fastcall
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

static void __fastcall PrepareObjectHierarchyViews(const AnimInfo &animInfo, CAnimObj *currobj, const NTempest::C3Vector &parentPos) {
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

static void __fastcall RotateTexture(const AnimInfo &animInfo, CAnimTransform *texAnim, CAnimObjStatus *status, NTempest::C34Matrix *transform) {
  NTempest::C4Quaternion rotation;
  if (texAnim->rotation.InterpolateVolatile(animInfo, status->base, &status->rotation, NTempest::C4Quaternion(), &rotation)) {
    NTempest::C3Vector texCenter(0.5f, 0.5f, 0.0f);
    transform->Translate(texCenter);
    transform->Rotate(rotation);
    transform->Translate(-texCenter);
  }
}

static void __fastcall ScaleTexture(const AnimInfo &animInfo, CAnimTransform *texAnim, CAnimObjStatus *status, NTempest::C34Matrix *transform) {
  NTempest::C3Vector scale;
  if (texAnim->scale.InterpolateVolatile(animInfo, status->base, &status->scale, NTempest::C3Vector(1.0f), &scale)) {
    NTempest::C3Vector texCenter(0.5f, 0.5f, 0.0f);
    transform->Translate(texCenter);
    transform->Scale(scale);
    transform->Translate(-texCenter);
  }
}

static void __fastcall TranslateTexture(const AnimInfo &animInfo, CAnimTransform *texAnim, CAnimObjStatus *status, NTempest::C34Matrix *transform) {
  NTempest::C3Vector position;
  if (texAnim->translation.InterpolateVolatile(animInfo, status->base, &status->translation, NTempest::C3Vector(0.0f), &position)) {
    transform->Translate(position);
  }
}

static void __fastcall AnimateTextureMap(const AnimInfo &animInfo, CAnimTransform *texAnim, CAnimObjStatus *status, NTempest::C34Matrix *transform) {
  ASSERT(texAnim);
  transform->Identity();
  RotateTexture(animInfo, texAnim, status, transform);
  ScaleTexture(animInfo, texAnim, status, transform);
  TranslateTexture(animInfo, texAnim, status, transform);
}

void __fastcall IAnimAnimateModel(CAnim *unique, CAnimData *shared, const CAnimationData &data) {
  ASSERT(shared->flags & 1);
  ASSERT(!(shared->flags & 4));
  ASSERT(data.boneMtx || !data.numBones);

  AnimInfo animInfo(unique, shared, *data.positions, data);
  animInfo.cameraWorldPos = *data.cameraWorldPos;
  animInfo.cameraVector = *data.cameraVector;
  GetWorldTransform(&animInfo);

  NTempest::C3Vector origin(0.0f);
  unsigned int       i;
  for (i = 0; i < shared->headarray.Count(); ++i) {
    PrepareObjectHierarchyViews(animInfo, shared->headarray[i], origin);
  }

  for (i = 0; i < shared->tex.Count(); ++i) {
    CAnimTransform      &tex = shared->tex[i];
    CAnimObjStatus      &status = unique->textureStatus[i];
    NTempest::C34Matrix &matrix = data.textureMtx[i];
    AnimateTextureMap(animInfo, &tex, &status, &matrix);
  }

  for (i = 0; i < shared->geo.Count(); ++i) {
    CAnimGeoset          &geo = shared->geo[i];
    CAnimGeosetObjStatus &status = unique->geosetStatus[i];
    CGeosetColor         &color = data.geosetColor[geo.sgGeosetId];
    float                 alpha = AnimFloat(geo.visibility, status.base, status.visibility, animInfo, 1.0f);
    color.animatedAlpha = alpha;
    color.animatedColor.a = NTempest::CMath::ftol_0_256_(min(max(alpha * color.proceduralAlpha, 0.0f), 1.0f) * 255.0f);
    if (geo.color.m_numKeyFrames) {
      C3Color c = AnimColor(geo.color, status.base, status.color, animInfo);
      color.animatedColor.r = NTempest::CMath::ftol_0_256_(min(max(c.r, 0.0f), 1.0f) * 255.0f);
      color.animatedColor.g = NTempest::CMath::ftol_0_256_(min(max(c.g, 0.0f), 1.0f) * 255.0f);
      color.animatedColor.b = NTempest::CMath::ftol_0_256_(min(max(c.b, 0.0f), 1.0f) * 255.0f);
    }
    if (color.animatedColor.a)
      status.base.flags |= 1;
    else
      status.base.flags &= ~1;
  }

  for (i = 0; i < shared->layers.Count(); ++i) {
    CAnimMaterialLayer &layer = shared->layers[i];
    CAnimLayerStatus   &status = unique->layerStatus[i];
    float               alpha = AnimFloat(layer.visibility, status.base, status.visibility, animInfo, 1.0f);
    data.layerAlpha[layer.layerId] = NTempest::CMath::ftol_0_256_(min(max(alpha, 0.0f), 1.0f) * 255.0f);
    data.layerTextureIds[layer.layerId] = static_cast<unsigned int>(-1);
    if (layer.flip.TotalKeys()) {
      unsigned int count = layer.flip.SetAnimTime(status.base, &status.flipIndex, animInfo);
      if (count) {
        data.layerTextureIds[layer.layerId] = *AnimKeyValue(layer.flip, status.flipIndex.currKey);
      }
    }
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

void __fastcall AnimProcessEvents(HANIM anim, const TSFixedArray<NTempest::C3Vector> &positions) {
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

void __fastcall AnimAnimateCameras(HANIM anim, const TSFixedArray<HCAMERA> &cameras) {
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

void __fastcall AnimAnimateModel(HANIM anim, const CAnimationData &data) {
  CAnim *unique = reinterpret_cast<CAnim *>(anim);
  ASSERT(unique);

  CAnimData *shared = reinterpret_cast<CAnimData *>(unique->hdata);
  ASSERT(shared);

  IAnimAnimateModel(unique, shared, data);
}

void __fastcall AnimSetBoneProjectCallback(ANIMBONEPROJECTCALLBACK callback, float distance) {
  s_AnimBoneProjectCallback = callback;
  s_animBoneProjectDistance = distance;
}

void __fastcall AnimGetBoneProjectCallback(ANIMBONEPROJECTCALLBACK &callback, float &distance) {
  callback = s_AnimBoneProjectCallback;
  distance = s_animBoneProjectDistance;
}

InterpInfo::InterpInfo(CAnim *container, CAnimData *animptr, const TSFixedArray<NTempest::C3Vector> &positions)
    : unique(container), shared(animptr), positions(positions) {
}
