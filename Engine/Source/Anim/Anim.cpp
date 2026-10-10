#include "Base/Base.h"
#include "Anim/Transform.h"
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

struct CameraInfo : public InterpInfo {
  CameraInfo(CAnim *container, CAnimData *animptr, const TSFixedArray<NTempest::C3Vector> &positions, const TSFixedArray<HCAMERA> &cameras)
      : InterpInfo(container, animptr, positions), cameras(cameras) {
  }

  const TSFixedArray<HCAMERA> &cameras;

 private:
  CameraInfo &operator=(const CameraInfo &);
};

using NTempest::CMath;

float                   s_animBoneProjectDistance;
ANIMBONEPROJECTCALLBACK s_AnimBoneProjectCallback;

static void FaceDirection(const NTempest::C3Vector &direction, NTempest::C3Vector *xprime, NTempest::C3Vector *yprime, NTempest::C3Vector *zprime) {
  VALIDATEBEGIN;
  VALIDATE(CMath::fnotequal_(direction.SquaredMag(),0));
  VALIDATEENDVOID;
  *xprime = direction;

  if (CMath::fequal_(xprime->x * xprime->x + xprime->y * xprime->y, 0)) {
    yprime->Set(1.0f, 0.0f, 0.0f);
  } else {
    yprime->Set(-xprime->y, xprime->x, 0.0f);
    CMath::normalize_(yprime->x, yprime->y);
  }

  *zprime = NTempest::C3Vector(-(yprime->y * xprime->z), xprime->z * yprime->x, yprime->y * xprime->x - xprime->y * yprime->x);
}

static void LookAtPoint(const NTempest::C3Vector &position, const NTempest::C3Vector &point, NTempest::C4Quaternion *result) {
  NTempest::C3Vector direction = point - position;
  direction.Normalize();

  NTempest::C3Vector xprime, yprime, zprime;
  FaceDirection(direction, &xprime, &yprime, &zprime);

  NTempest::C33Matrix rotation(xprime, yprime, zprime);
  result->FromRotationMatrix(rotation);

  NTempest::C34Matrix parentMatrix;
  WorldMatrixGet(&parentMatrix);
  NTempest::C4Quaternion parentRotation;
  parentRotation.FromRotationMatrixInv(parentMatrix);
  *result = parentRotation * *result;
}

static void RotateViewBillboarded(const NTempest::C3Vector &cameraVector) {
  NTempest::C3Vector xprime;
  NTempest::C3Vector yprime;
  NTempest::C3Vector zprime;

  FaceDirection(NTempest::C3Vector(-cameraVector.x, -cameraVector.y, -cameraVector.z), &xprime, &yprime, &zprime);
  WorldMatrixRemove(4);
  WorldMatrixBasis(xprime, yprime, zprime);
}

static void RotateViewZAxisBillboarded(const NTempest::C3Vector &cameraVector) {
  VALIDATEBEGIN;
  VALIDATE(CMath::fnotequal_(cameraVector.SquaredMag(),0));
  VALIDATEENDVOID;
  NTempest::C3Vector zprime;

  WorldMatrixGetRow(2, &zprime);
  NTempest::C3Vector yprime = NTempest::C3Vector::Cross(zprime, NTempest::C3Vector(-cameraVector.x, -cameraVector.y, -cameraVector.z));
  yprime.Normalize();
  NTempest::C3Vector xprime = NTempest::C3Vector::Cross(yprime, zprime);
  WorldMatrixRemove(4);
  WorldMatrixBasis(xprime, yprime, zprime);
}

static void RotateViewYAxisBillboarded(const NTempest::C3Vector &cameraVector) {
  VALIDATEBEGIN;
  VALIDATE(CMath::fnotequal_(cameraVector.SquaredMag(),0));
  VALIDATEENDVOID;
  NTempest::C3Vector yprime;

  WorldMatrixGetRow(1, &yprime);
  NTempest::C3Vector zprime = NTempest::C3Vector::Cross(NTempest::C3Vector(-cameraVector.x, -cameraVector.y, -cameraVector.z), yprime);
  zprime.Normalize();
  NTempest::C3Vector xprime = NTempest::C3Vector::Cross(yprime, zprime);
  WorldMatrixRemove(4);
  WorldMatrixBasis(xprime, yprime, zprime);
}

static void RotateViewXAxisBillboarded(const NTempest::C3Vector &cameraVector) {
  VALIDATEBEGIN;
  VALIDATE(CMath::fnotequal_(cameraVector.SquaredMag(),0));
  VALIDATEENDVOID;
  NTempest::C3Vector xprime;

  WorldMatrixGetRow(0, &xprime);
  NTempest::C3Vector zprime = NTempest::C3Vector::Cross(NTempest::C3Vector(-cameraVector.x, -cameraVector.y, -cameraVector.z), xprime);
  zprime.Normalize();
  NTempest::C3Vector yprime = NTempest::C3Vector::Cross(xprime, zprime);
  WorldMatrixRemove(4);
  WorldMatrixBasis(xprime, yprime, zprime);
}

namespace {

  template <class T, class U>
  static const T *AnimKeyValue(const CKeyFrameTrack<T, U> &track, UINT key) {
    const BYTE *p = (const BYTE *)track.m_keyFrames + track.m_keyFrameSize * key + sizeof(int);
    return (const T *)p;
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

  static inline C3Color AnimColor(CKeyFrameTrack<C3Color, C3Color> &track, CBaseStatus &base, CKeyTrackStatus &keyStatus, const InterpInfo &info) {
    C3Color fallback;
    C3Color result;
    track.InterpolateRetained(info, base, &keyStatus, fallback, &result);
    return result;
  }

}  // namespace

static void PlaceEventObject(const AnimInfo &animInfo, CAnimEventObj *currobj) {
  CAnimEventObjStatus *eventStatus = &animInfo.unique->eventStatus[currobj->splitIndex];

  eventStatus->position = animInfo.positions[currobj->animObjId];
  WorldMatrixTransform(&eventStatus->position);

  NTempest::C34Matrix basis(
      animInfo.basisScale.x * animInfo.basisX, animInfo.basisScale.y * animInfo.basisY, animInfo.basisScale.z * animInfo.basisZ,
      animInfo.basisPosition
  );
  NTempest::C34Matrix invBasis = basis.AffineInverse(animInfo.basisScale);
  eventStatus->position *= invBasis;
}

static void IProcessEvent(const InterpInfo &animInfo, CAnimEventObj *currEvent) {
  if (!animInfo.unique->appEvent.callback || (animInfo.unique->flags & 8) || !currEvent->events.TotalKeys()) {
    return;
  }

  CAnimEventObjStatus *eventStatus = &animInfo.unique->eventStatus[currEvent->splitIndex];
  CKeyTrackStatus      prevStatus = eventStatus->event;
  BYTE                 sequence = eventStatus->base.currSeq;
  if (!currEvent->events.NumKeysThisSeqSafe(sequence)) {
    return;
  }
  currEvent->events.SetAnimTime(eventStatus->base, &eventStatus->event, animInfo);

  if (currEvent->events.JustPastKey(
          animInfo.unique->seq[sequence].scaledElapsedTime, animInfo.shared->seq[sequence], animInfo.unique->seq[sequence].elapsed, sequence,
          eventStatus->base.flags & 0x10, prevStatus, eventStatus->event
      ))
  {
    NTempest::C3Vector position = eventStatus->position;
    WorldMatrixTransform(&position);
    ActivityBegin(ACTIVITY_ANIMEVENTS);
    animInfo.unique->appEvent.callback(currEvent->name, position, animInfo.unique->appEvent.param);
    ActivityEnd(ACTIVITY_ANIMEVENTS);
  }
}

static void PlaceModelObject(const AnimInfo &animInfo, CAnimModelObj *modelObj) {
  modelObj->visibility.InterpolateRetained(
      animInfo, animInfo.unique->status[modelObj->animObjId]->base,
      &((CAnimModelObjStatus *)animInfo.unique->status[modelObj->animObjId])->visibility, 1.0f,
      &((CAnimModelObjStatus *)animInfo.unique->status[modelObj->animObjId])->visible
  );
  FATALASSERT(modelObj->splitIndex < animInfo.data.numAttached);
  WorldMatrixGet(animInfo.data.attached + modelObj->splitIndex);
}

static void SetLightColor(const AnimInfo &animInfo, CAnimLightObj *currobj, CAnimLightObjStatus *lightStatus, CGxLight *light) {
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

static void SetLightIntensity(const AnimInfo &animInfo, CAnimLightObj *currobj, CAnimLightObjStatus *lightStatus, CGxLight *light) {
  currobj->intensity.InterpolateRetained(animInfo, lightStatus->base, &lightStatus->intensity, 0.0f, &light->m_dirIntensity);
  if (light->m_dirIntensity < 0.0f) {
    light->m_dirIntensity = 0.0f;
  }
  currobj->ambIntensity.InterpolateRetained(animInfo, lightStatus->base, &lightStatus->ambIntensity, 0.0f, &light->m_ambIntensity);
  if (light->m_ambIntensity < 0.0f) {
    light->m_ambIntensity = 0.0f;
  }
}

static void SetLightDirection(CGxLight *light) {
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

static void SetLightValues(const AnimInfo &animInfo, CAnimLightObj *currobj, const NTempest::C3Vector &currPos) {
  FATALASSERT(currobj);
  FATALASSERT(currobj->type == OBJ_TYPE_LIGHT);
  FATALASSERT(currobj->splitIndex < animInfo.data.lights->Count());
  DWORD gxuLightId = (*animInfo.data.lights)[currobj->splitIndex];
  if (gxuLightId) {
    CAnimLightObjStatus *lightStatus = (CAnimLightObjStatus *)animInfo.unique->status[currobj->animObjId];
    CGxLight            *light = GxuLightLock(gxuLightId);
    FATALASSERT(light);
    float isVisible = 1.0f;
    if (currobj->visibility.InterpolateVolatile(animInfo, lightStatus->base, &lightStatus->visibility, 1.0f, &isVisible) || !currobj->visibility.TotalKeys()) {
      light->m_enabled = isVisible > 0.0f;
      if (!light->m_enabled) {
        GxuLightUnlock(gxuLightId);
        return;
      }
    }

    SetLightColor(animInfo, currobj, lightStatus, light);
    SetLightIntensity(animInfo, currobj, lightStatus, light);
    SetLightDirection(light);
    if (light->m_isOmni) {
      light->m_dir = currPos;
      WorldMatrixTransform(&light->m_dir);
      light->m_dir += animInfo.cameraWorldPos;
    }
    GxuLightUnlock(gxuLightId);
  }
}
static void SetParticleVariation2(const AnimInfo &animInfo, CAnimEmitter2Obj *currobj, CAnimEmitter2ObjStatus *status) {
  float variation = 0.0f;
  if (currobj->variation.InterpolateRetained(animInfo, status->base, &status->variation, 0.0f, &variation)) {
    (*animInfo.data.emitters2)[currobj->splitIndex]->SetVelocityVariation(variation);
  }
}

static void SetParticleSpeed2(const AnimInfo &animInfo, CAnimEmitter2Obj *currobj, CAnimEmitter2ObjStatus *status) {
  float speed = 0.0f;
  if (currobj->particleSpeed.InterpolateRetained(animInfo, status->base, &status->speed, 0.0f, &speed)) {
    (*animInfo.data.emitters2)[currobj->splitIndex]->SetVelocity(speed);
  }
}

static void SetParticleEmissionRate2(const AnimInfo &animInfo, CAnimEmitter2Obj *currobj, CAnimEmitter2ObjStatus *status) {
  float rate = 0.0f;
  if (currobj->emissionRate.InterpolateRetained(animInfo, status->base, &status->emissionRate, 0.0f, &rate)) {
    if (rate > 0.0f && (*animInfo.data.emitters2)[currobj->splitIndex]->EmissionRate() == 0.0f && currobj->squirts) {
      (*animInfo.data.emitters2)[currobj->splitIndex]->Squirt();
    }
    (*animInfo.data.emitters2)[currobj->splitIndex]->SetEmissionRate(rate);
  }
}

static void SetParticleGravity2(const AnimInfo &animInfo, CAnimEmitter2Obj *currobj, CAnimEmitter2ObjStatus *status) {
  float accel = 0.0f;
  if (currobj->gravity.InterpolateRetained(animInfo, status->base, &status->gravity, 0.0f, &accel)) {
    (*animInfo.data.emitters2)[currobj->splitIndex]->SetAcceleration(accel);
  }
}

static void SetEmitterLatitude2(const AnimInfo &animInfo, CAnimEmitter2Obj *currobj, CAnimEmitter2ObjStatus *status) {
  float latitude = 0.0f;
  if (currobj->latitude.InterpolateRetained(animInfo, status->base, &status->latitude, 0.0f, &latitude)) {
    CParticleEmitter2 *emitter = (*animInfo.data.emitters2)[currobj->splitIndex];
    emitter->SetLatitude(latitude);
  }
}

static void SetEmitterLongitude2(const AnimInfo &animInfo, CAnimEmitter2Obj *currobj, CAnimEmitter2ObjStatus *status) {
  float longitude = 0.0f;
  if (currobj->longitude.InterpolateRetained(animInfo, status->base, &status->longitude, 0.0f, &longitude)) {
    CParticleEmitter2 *emitter = (*animInfo.data.emitters2)[currobj->splitIndex];
    emitter->SetLongitude(longitude);
  }
}

static void SetEmitterWidth2(const AnimInfo &animInfo, CAnimEmitter2Obj *currobj, CAnimEmitter2ObjStatus *status) {
  float width = 0.0f;
  if (currobj->width.InterpolateRetained(animInfo, status->base, &status->width, 0.0f, &width)) {
    CParticleEmitter2 *emitter = (*animInfo.data.emitters2)[currobj->splitIndex];
    emitter->SetWidth(width);
  }
}

static void SetEmitterLength2(const AnimInfo &animInfo, CAnimEmitter2Obj *currobj, CAnimEmitter2ObjStatus *status) {
  float length = 0.0f;
  if (currobj->length.InterpolateRetained(animInfo, status->base, &status->length, 0.0f, &length)) {
    CParticleEmitter2 *emitter = (*animInfo.data.emitters2)[currobj->splitIndex];
    emitter->SetHeight(length);
  }
}

static void SetEmitterZsource2(const AnimInfo &animInfo, CAnimEmitter2Obj *currobj, CAnimEmitter2ObjStatus *status) {
  float zsource = 0.0f;
  if (currobj->zsource.InterpolateRetained(animInfo, status->base, &status->zsource, 0.0f, &zsource)) {
    CParticleEmitter2 *emitter = (*animInfo.data.emitters2)[currobj->splitIndex];
    emitter->SetZsource(zsource);
  }
}

static void SetEmitterLifeSpan2(const AnimInfo &animInfo, CAnimEmitter2Obj *currobj, CAnimEmitter2ObjStatus *status) {
  float lifeSpan = 0.1f;
  if (currobj->lifeSpan.InterpolateRetained(animInfo, status->base, &status->lifeSpan, 1.0f, &lifeSpan)) {
    CParticleEmitter2 *emitter = (*animInfo.data.emitters2)[currobj->splitIndex];
    emitter->SetLifeSpan(lifeSpan);
  }
}

static void SetRibbonHeight(const AnimInfo &animInfo, CAnimRibbonObj *currobj, CAnimRibbonObjStatus *status) {
  float heightAbove = 0.0f;
  float heightBelow = 0.0f;
  int   newAbove = currobj->heightAbove.InterpolateRetained(animInfo, status->base, &status->heightAbove, 0.0f, &heightAbove);
  int   newBelow = currobj->heightBelow.InterpolateRetained(animInfo, status->base, &status->heightBelow, 0.0f, &heightBelow);
  if (newAbove) {
    (*animInfo.data.ribbons)[currobj->splitIndex]->SetAbove(heightAbove);
  }
  if (newBelow) {
    (*animInfo.data.ribbons)[currobj->splitIndex]->SetBelow(heightBelow);
  }
}

static void SetRibbonTexSlot(const AnimInfo &animInfo, CAnimRibbonObj *currobj, CAnimRibbonObjStatus *status) {
  UINT slot = 0;
  if (currobj->slot.InterpolateRetained(animInfo, status->base, &status->slot, 0, &slot)) {
    (*animInfo.data.ribbons)[currobj->splitIndex]->SetTexSlot(slot);
  }
}

static void SetRibbonColor(const AnimInfo &animInfo, CAnimRibbonObj *currobj, CAnimRibbonObjStatus *status) {
  C3Color color;
  if (currobj->color.InterpolateRetained(animInfo, status->base, &status->color, C3Color(), &color)) {
    (*animInfo.data.ribbons)[currobj->splitIndex]->SetColor(color.r, color.g, color.b);
  }
}

static void SetRibbonAlpha(const AnimInfo &animInfo, CAnimRibbonObj *currobj, CAnimRibbonObjStatus *status) {
  float alpha = 0.0f;
  if (currobj->alpha.InterpolateRetained(animInfo, status->base, &status->alpha, 0.0f, &alpha)) {
    (*animInfo.data.ribbons)[currobj->splitIndex]->SetAlpha(alpha);
  }
}

static void SetEmitter2Values(const AnimInfo &animInfo, CAnimEmitter2Obj *currobj) {
  FATALASSERT(currobj);
  FATALASSERT(currobj->type == OBJ_TYPE_EMITTER2);
  FATALASSERT(currobj->splitIndex < animInfo.data.emitters2->Count());
  CAnimEmitter2ObjStatus *status = &animInfo.unique->emitter2Status[currobj->splitIndex];
  float                   isVisible = 1.0f;
  if (currobj->visibility.InterpolateRetained(animInfo, status->base, &status->visibility, 1.0f, &isVisible) || !currobj->visibility.TotalKeys()) {
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

static void SetRibbonValues(const AnimInfo &animInfo, CAnimRibbonObj *currobj) {
  FATALASSERT(currobj);
  FATALASSERT(currobj->type == OBJ_TYPE_RIBBON);
  FATALASSERT(currobj->splitIndex < animInfo.data.ribbons->Count());

  CAnimRibbonObjStatus &status = animInfo.unique->ribbonStatus[currobj->splitIndex];
  float                 isVisible = 1.0f;
  if (currobj->visibility.InterpolateRetained(animInfo, status.base, &status.visibility, 1.0f, &isVisible) || !currobj->visibility.TotalKeys()) {
    (*animInfo.data.ribbons)[currobj->splitIndex]->SetEnabled(isVisible > 0.0f);
  }
  SetRibbonHeight(animInfo, currobj, &status);
  SetRibbonTexSlot(animInfo, currobj, &status);
  SetRibbonColor(animInfo, currobj, &status);
  SetRibbonAlpha(animInfo, currobj, &status);

  NTempest::C34Matrix current;
  WorldMatrixGet(&current);
  (*animInfo.data.ribbons)[currobj->splitIndex]->SetPos(NTempest::C44Matrix(current), *animInfo.data.cameraWorldPos);
}

static void PlaceObject(const AnimInfo &animInfo, CAnimObj *currobj, const NTempest::C3Vector &currPos) {
  ASSERT(currobj);

  switch (currobj->type) {
    case OBJ_TYPE_HELPER:
      return;
    case OBJ_TYPE_LIGHT:
      WorldMatrixTranslate(-currPos);
      ASSERT(currobj->type == OBJ_TYPE_LIGHT);
      SetLightValues(animInfo, (CAnimLightObj *)currobj, currPos);
      break;
    case OBJ_TYPE_MODEL:
      ASSERT(currobj->type == OBJ_TYPE_MODEL);
      PlaceModelObject(animInfo, (CAnimModelObj *)currobj);
      break;
    case OBJ_TYPE_EMITTER2: {
      NTempest::C34Matrix currentWorldMatrix;
      ASSERT(currobj->type == OBJ_TYPE_EMITTER2);
      CAnimEmitter2Obj *emitter = (CAnimEmitter2Obj *)currobj;
      SetEmitter2Values(animInfo, emitter);
      ASSERT(emitter->splitIndex < animInfo.data.emitters2->Count());
      WorldMatrixGet(&currentWorldMatrix);
      currentWorldMatrix.Rotate(PI * 0.5f, NTempest::C3Vector(0.0f, 0.0f, 1.0f), true);
      (*animInfo.data.emitters2)[emitter->splitIndex]->Update(
          animInfo.unique->emitter2Status[emitter->splitIndex].elapsedTime, currentWorldMatrix, *animInfo.data.cameraWorldPos
      );
      break;
    }
    case OBJ_TYPE_RIBBON: {
      ASSERT(currobj->type == OBJ_TYPE_RIBBON);
      CAnimRibbonObj *ribbon = (CAnimRibbonObj *)currobj;
      SetRibbonValues(animInfo, ribbon);
      ASSERT(ribbon->splitIndex < animInfo.data.ribbons->Count());
      (*animInfo.data.ribbons)[ribbon->splitIndex]->Update(animInfo.unique->ribbonStatus[ribbon->splitIndex].elapsedTime, 0);
      break;
    }
    case OBJ_TYPE_EVENT:
      WorldMatrixTranslate(-currPos);
      ASSERT(currobj->type == OBJ_TYPE_EVENT);
      PlaceEventObject(animInfo, (CAnimEventObj *)currobj);
      break;
    default: {
      WorldMatrixTranslate(-currPos);
      ASSERT(currobj->type == OBJ_TYPE_BONE);
      CAnimBoneObj *currbone = (CAnimBoneObj *)currobj;
      ASSERT(currbone->splitIndex < animInfo.data.numBones);
      WorldMatrixGet(animInfo.data.boneMtx + currbone->splitIndex);
      break;
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
  const NTempest::C3Vector &lookAt = animInfo.unique->lookAtTarget[status->lookAtId];
  NTempest::C3Vector        targetPos(
      matrix.a0 * lookAt.x + (matrix.b0 * lookAt.y + matrix.c0 * lookAt.z), matrix.a1 * lookAt.x + (matrix.b1 * lookAt.y + matrix.c1 * lookAt.z),
      matrix.a2 * lookAt.x + (matrix.b2 * lookAt.y + matrix.c2 * lookAt.z)
  );
  NTempest::C4Quaternion transform;
  LookAtPoint(NTempest::C3Vector(0.0f), targetPos, &transform);
  WorldMatrixRotate(transform);
}

static BOOL ApplyLookAt(const AnimInfo &animInfo, CAnimObj *currobj, const NTempest::C3Vector &currPos) {
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
      Blend(blend.blendRotation, &transform, blend.blendTimer, animInfo.shared->seq[status->base.currSeq].blendTime);
    }
    blend.prevSeqRotation = transform;
  }

  WorldMatrixRotate(transform);
  return 1;
}

static void TransformObjectView(const AnimInfo &animInfo, CAnimObj *currobj, const NTempest::C3Vector &currPos, const NTempest::C3Vector &parentPos) {
  if (animInfo.unique->flags & 0x10) {
    CAnimObjBlendStatus &blend = animInfo.unique->blendStatus[currobj->animObjId];
    CAnimObjStatus      *status = animInfo.unique->status[currobj->animObjId];
    if (status->base.flags & 0x14) {
      blend.blendPosition = blend.prevSeqPosition;
      blend.blendRotation = blend.prevSeqRotation;
      blend.blendScale = blend.prevSeqScale;
    }

    blend.blendTimer -= abs(animInfo.unique->seq[status->base.currSeq].scaledElapsedTime);
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

  BOOL hidden = 0;
  if (currobj->type == OBJ_TYPE_BONE) {
    hidden = !((CAnimBoneObj *)currobj)->IsVisible(*animInfo.unique);
  }
  if (hidden) {
    return;
  }

  WorldMatrixPush();
  NTempest::C3Vector currPos = animInfo.positions[currobj->animObjId];
  TransformObjectView(animInfo, currobj, currPos, parentPos);
  UINT numChildren = currobj->childarray.Count();
  for (UINT i = 0; i < numChildren; ++i) {
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

static void AnimateCamera(const InterpInfo &animInfo, CAnimCameraObj *object, CAnimCameraObjStatus *status, HCAMERA__ *camera) {
  ASSERT(object);
  ASSERT(status);
  object->visibility.InterpolateRetained(animInfo, status->base, &status->visibility, 1.0f, &status->visible);
  if (status->visible == 0.0f) {
    return;
  }
  NTempest::C3Vector position;
  object->translation.InterpolateVolatile(animInfo, status->base, &status->translation, NTempest::C3Vector(0.0f), &position);
  position += object->pivot;
  WorldMatrixTransform(&position);
  DataMgrSetCoord(camera, 7, position, 0);
  object->targetTranslation.InterpolateVolatile(animInfo, status->base, &status->targetTranslation, NTempest::C3Vector(0.0f), &position);
  position += object->targetPivot;
  WorldMatrixTransform(&position);
  DataMgrSetCoord(camera, 8, position, 0);
  float roll;
  object->roll.InterpolateVolatile(animInfo, status->base, &status->roll, 0.0f, &roll);
  DataMgrSetFloat(camera, 5, roll);
}

static void AnimateAllCameras(CameraInfo *animInfo) {
  ASSERT(animInfo);
  ASSERT(animInfo->unique);
  ASSERT(animInfo->shared);

  CAnimCameraObj       *object = animInfo->shared->cameraObjs.Ptr();
  CAnimCameraObjStatus *status = animInfo->unique->cameraStatus.Ptr();
  const HCAMERA        *camera = animInfo->cameras.Ptr();
  for (UINT i = animInfo->cameras.Count(); i; --i, ++camera, ++object, ++status) {
    AnimateCamera(*animInfo, object, status, *camera);
  }
}

static void AnimateAllTextureMaps(AnimInfo *animInfo) {
  ASSERT(animInfo);
  ASSERT(animInfo->unique);
  ASSERT(animInfo->shared);

  CAnimTransform *texAnim = animInfo->shared->tex.Ptr();
  CAnimObjStatus *status = animInfo->unique->textureStatus.Ptr();
  for (UINT i = 0; i < animInfo->shared->tex.Count(); ++i, ++texAnim, ++status) {
    ASSERT(i < animInfo->data.numTexBones);
    AnimateTextureMap(*animInfo, texAnim, status, &animInfo->data.textureMtx[i]);
  }
}

static void AnimateAllGeosets(AnimInfo *animInfo) {
  ASSERT(animInfo);
  ASSERT(animInfo->unique);
  ASSERT(animInfo->shared);

  CAnimGeoset          *geo = animInfo->shared->geo.Ptr();
  CAnimGeosetObjStatus *status = animInfo->unique->geosetStatus.Ptr();
  for (UINT i = animInfo->shared->geo.Count(); i; --i, ++geo, ++status) {
    int wasVisible = status->IsVisible();
    CalcGeosetColor(*animInfo, geo, status, &animInfo->data.geosetColor[geo->sgGeosetId]);
    if (status->IsVisible() ^ wasVisible) {
      animInfo->unique->flags &= ~2;
    }
  }
}

static void AnimateAllMaterialLayers(AnimInfo *animInfo, UINT *tex) {
  ASSERT(animInfo);
  ASSERT(animInfo->unique);
  ASSERT(animInfo->shared);

  CAnimMaterialLayer *layer = animInfo->shared->layers.Ptr();
  CAnimLayerStatus   *layerStatus = animInfo->unique->layerStatus.Ptr();
  for (UINT i = animInfo->shared->layers.Count(); i--; ++layer, ++layerStatus) {
    float alpha = 0.0f;
    if (layer->visibility.InterpolateRetained(*animInfo, layerStatus->base, &layerStatus->visibility, 1.0f, &alpha)) {
      animInfo->data.layerAlpha[layer->layerId] = NTempest::CMath::ftol_0_256_((alpha < 0.0f ? 0.0f : alpha > 1.0f ? 1.0f : alpha) * 255.0f);
    }

    tex[i] = -1;
    layer->flip.InterpolateRetained(*animInfo, layerStatus->base, &layerStatus->flipIndex, 0, &tex[i]);
  }
}

static void ISetEventSequenceUnchanged(CAnim *container) {
  if (!(container->flags & 4)) {
    UINT                 count = container->eventStatus.Count();
    CAnimEventObjStatus *status = container->eventStatus.Ptr();
    while (count) {
      status->base.flags &= ~0x10;
      ++status;
      --count;
    }
    container->flags |= 4;
  }
}

static void ISetSequenceUnchanged(CAnim *container, CAnimData *animptr) {
  if (container->flags & 3) {
    return;
  }

  int  setAllObjects = 1;
  UINT numObjects = container->status.Count();
  UINT i;
  for (i = 0; i < numObjects; ++i) {
    switch (animptr->obj[i]->type) {
      case OBJ_TYPE_BONE: {
        CAnimBoneObj *bone = AnimObjToBoneObj(animptr->obj[i]);
        if (!bone->IsVisible(*container)) {
          setAllObjects = 0;
          continue;
        }
        break;
      }
      case OBJ_TYPE_EVENT:
        continue;
    }
    container->status[i]->base.flags &= ~0x10;
  }

  CAnimCameraObjStatus *cameraStatus = container->cameraStatus.Ptr();
  for (i = container->cameraStatus.Count(); i; --i, ++cameraStatus) {
    cameraStatus->base.flags &= ~0x10;
  }
  CAnimObjStatus *textureStatus = container->textureStatus.Ptr();
  for (i = container->textureStatus.Count(); i; --i, ++textureStatus) {
    textureStatus->base.flags &= ~0x10;
  }
  CAnimGeosetObjStatus *geosetStatus = container->geosetStatus.Ptr();
  for (i = container->geosetStatus.Count(); i; --i, ++geosetStatus) {
    geosetStatus->base.flags &= ~0x10;
  }
  if (setAllObjects) {
    container->flags |= 1;
  } else {
    container->flags |= 2;
  }
}

void AnimProcessEvents(HANIM anim, const TSFixedArray<NTempest::C3Vector> &positions) {
  CAnim     *container = (CAnim *)anim;
  CAnimData *animptr;
  VALIDATEBEGIN;
  VALIDATE(container);
  animptr = (CAnimData *)container->hdata;
  VALIDATE(animptr);
  ASSERT(animptr->flags & 0x01);
  VALIDATE((animptr->flags & 0x04) == 0);
  VALIDATEENDVOID;

  InterpInfo     interpInfo(container, animptr, positions);
  CAnimEventObj *eventObject = animptr->eventObjs.Ptr();
  UINT           count = animptr->eventObjs.Count();
  while (count) {
    IProcessEvent(interpInfo, eventObject);
    ++eventObject;
    --count;
  }
  ISetEventSequenceUnchanged(container);
}

void AnimAnimateCameras(HANIM anim, const TSFixedArray<HCAMERA> &cameras) {
  CAnim     *container = (CAnim *)anim;
  CAnimData *animptr;
  VALIDATEBEGIN;
  VALIDATE(container);
  animptr = (CAnimData *)container->hdata;
  VALIDATE(animptr);
  ASSERT(animptr->flags & 0x01);
  VALIDATE((animptr->flags & 0x04) == 0);
  VALIDATEENDVOID;

  TSFixedArray<NTempest::C3Vector> positions;
  CameraInfo                       cameraInfo(container, animptr, positions, cameras);
  AnimateAllCameras(&cameraInfo);
}

static void IAnimAnimateModel(CAnim *unique, CAnimData *shared, const CAnimationData &data) {
  AnimInfo animInfo(unique, shared, data);
  GetWorldTransform(&animInfo);

  AnimateAllTextureMaps(&animInfo);
  AnimateAllGeosets(&animInfo);
  AnimateAllMaterialLayers(&animInfo, data.layerTextureIds);

  NTempest::C3Vector origin(0.0f);
  UINT               count = shared->headarray.Count();
  for (UINT i = 0; i < count; ++i) {
    PrepareObjectHierarchyViews(animInfo, shared->headarray[i], origin);
  }

  ISetSequenceUnchanged(unique, shared);
}

void AnimAnimateModel(HANIM anim, const CAnimationData &data) {
  CAnim     *container = (CAnim *)anim;
  CAnimData *animptr;
  VALIDATEBEGIN;
  VALIDATE(container);
  animptr = (CAnimData *)container->hdata;
  VALIDATE(animptr);
  ASSERT(animptr->flags & 0x01);
  VALIDATE((animptr->flags & 0x04) == 0);
  VALIDATE(data.boneMtx || !data.numBones);
  VALIDATE(data.textureMtx || !data.numTexBones);
  VALIDATEENDVOID;

  IAnimAnimateModel(container, animptr, data);
}

void AnimSetBoneProjectCallback(ANIMBONEPROJECTCALLBACK callback, float distance) {
  s_AnimBoneProjectCallback = callback;
  s_animBoneProjectDistance = distance;
}

void AnimGetBoneProjectCallback(ANIMBONEPROJECTCALLBACK &callback, float &dist) {
  callback = s_AnimBoneProjectCallback;
  dist = s_animBoneProjectDistance;
}
