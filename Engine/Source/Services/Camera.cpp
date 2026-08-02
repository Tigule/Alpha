#include "Camera.h"

#include <Gx/Gx.h>
#include <Tempest/c2vector.h>
#include <Tempest/c3vector.h>
#include <Tempest/c44matrix.h>
#include <Tempest/crect.h>

#include <math.h>

static const float DEFAULT_SCREEN_FRUSTUM_LENGTH = 500.0f;

void CCamera::SetupWorldProjection(const NTempest::CRect &projectionRect, unsigned int flags) {
  NTempest::C44Matrix mProj;
  const float         aspect = (projectionRect.r - projectionRect.l) / (projectionRect.b - projectionRect.t);

  GxuXformCreateProjection(m_fov.Get(), aspect, m_zNear.Get(), m_zFar.Get(), mProj);
  GxXformSetProjection(mProj);

  const NTempest::C3Vector cameraVector = m_target.Get() - m_position.Get();
  const NTempest::C3Vector cameraPos(0.0f);
  const NTempest::C3Vector upVector(m_rotation.Sin() * m_roll.Sin(), -m_rotation.Cos() * m_roll.Sin(), m_roll.Cos());
  NTempest::C44Matrix      mView;

  GxuXformCreateLookAtSgCompat(cameraPos, cameraVector, upVector, mView);
  GxXformSetView(mView);
}

void CameraCalcPosFromTarg(HCAMERA__* camera, NTempest::C3Vector* position) {
  CCamera *cameraPtr = reinterpret_cast<CCamera *>(camera);
  FATALASSERT(cameraPtr);
  FATALASSERT(position);

  const float distance = cameraPtr->m_distance.Get();
  position->x = cameraPtr->m_target.Get().x - cameraPtr->m_rotation.Cos() * cameraPtr->m_aoa.Cos() * distance;
  position->y = cameraPtr->m_target.Get().y - cameraPtr->m_rotation.Sin() * cameraPtr->m_aoa.Cos() * distance;
  position->z = cameraPtr->m_target.Get().z - cameraPtr->m_aoa.Sin() * distance;
}

void CameraCalcTargFromPos(HCAMERA__* camera, NTempest::C3Vector* target) {
  CCamera *cameraPtr = reinterpret_cast<CCamera *>(camera);
  FATALASSERT(cameraPtr);
  FATALASSERT(target);

  const float distance = cameraPtr->m_distance.Get();
  target->x = cameraPtr->m_position.Get().x + cameraPtr->m_rotation.Cos() * cameraPtr->m_aoa.Cos() * distance;
  target->y = cameraPtr->m_position.Get().y + cameraPtr->m_rotation.Sin() * cameraPtr->m_aoa.Cos() * distance;
  target->z = cameraPtr->m_position.Get().z + cameraPtr->m_aoa.Sin() * distance;
}

HCAMERA CameraCreate() {
  CCamera *camera = NEW(CCamera)();
  return camera ? reinterpret_cast<HCAMERA>(HandleCreate(camera, "HCAMERA")) : 0;
}

HCAMERA CameraDuplicate(HCAMERA source) {
  CCamera *srcPtr = reinterpret_cast<CCamera *>(source);
  ASSERT(srcPtr);

  CCamera *cameraPtr = NEW(CCamera)();
  if (!cameraPtr) {
    return 0;
  }

  cameraPtr->m_position.m_updateFcn = 0;
  cameraPtr->m_position.m_updateData = 0;
  cameraPtr->m_position.m_updatePriority = 0.0f;
  cameraPtr->m_position.Set_(srcPtr->m_position.Get());

  cameraPtr->m_target.m_updateFcn = 0;
  cameraPtr->m_target.m_updateData = 0;
  cameraPtr->m_target.m_updatePriority = 0.0f;
  cameraPtr->m_target.Set_(srcPtr->m_target.Get());

  cameraPtr->m_distance.m_updateFcn = 0;
  cameraPtr->m_distance.m_updateData = 0;
  cameraPtr->m_distance.m_updatePriority = 0.0f;
  cameraPtr->m_distance.Set_(srcPtr->m_distance.Get());

  cameraPtr->m_zNear.m_updateFcn = 0;
  cameraPtr->m_zNear.m_updateData = 0;
  cameraPtr->m_zNear.m_updatePriority = 0.0f;
  cameraPtr->m_zNear.Set_(srcPtr->m_zNear.Get());

  cameraPtr->m_zFar.m_updateFcn = 0;
  cameraPtr->m_zFar.m_updateData = 0;
  cameraPtr->m_zFar.m_updatePriority = 0.0f;
  cameraPtr->m_zFar.Set_(srcPtr->m_zFar.Get());

  cameraPtr->m_aoa.m_updateFcn = 0;
  cameraPtr->m_aoa.m_updateData = 0;
  cameraPtr->m_aoa.m_updatePriority = 0.0f;
  cameraPtr->m_aoa.Set_(srcPtr->m_aoa.Get());

  cameraPtr->m_fov.m_updateFcn = 0;
  cameraPtr->m_fov.m_updateData = 0;
  cameraPtr->m_fov.m_updatePriority = 0.0f;
  cameraPtr->m_fov.Set_(srcPtr->m_fov.Get());

  cameraPtr->m_roll.m_updateFcn = 0;
  cameraPtr->m_roll.m_updateData = 0;
  cameraPtr->m_roll.m_updatePriority = 0.0f;
  cameraPtr->m_roll.Set_(srcPtr->m_roll.Get());

  cameraPtr->m_rotation.m_updateFcn = 0;
  cameraPtr->m_rotation.m_updateData = 0;
  cameraPtr->m_rotation.m_updatePriority = 0.0f;
  cameraPtr->m_rotation.Set_(srcPtr->m_rotation.Get());

  return reinterpret_cast<HCAMERA>(HandleCreate(cameraPtr, "HCAMERA"));
}

void CameraGetLineSegment(float x, float y, NTempest::C3Vector *a, NTempest::C3Vector *b) {
  FATALASSERT(a);
  FATALASSERT(b);
  FATALASSERT(x >= 0.0f && x <= 1.0f);
  FATALASSERT(y >= 0.0f && y <= 1.0f);

  NTempest::C44Matrix view;
  NTempest::C44Matrix proj;
  NTempest::C3Vector  corners[8];
  GxXformView(view);
  GxXformProjection(proj);
  GxuXformCalcFrustumCorners(view, proj, corners);

  NTempest::C3Vector lefty = corners[0] + (corners[1] - corners[0]) * y;
  *a = lefty + (corners[3] + (corners[2] - corners[3]) * y - lefty) * x;

  lefty = corners[4] + (corners[5] - corners[4]) * y;
  *b = lefty + (corners[7] + (corners[6] - corners[7]) * y - lefty) * x;
}

void CameraSetupScreenProjection(const NTempest::CRect &projectionRect, const NTempest::C2Vector &screenPoint, float depth) {
  NTempest::CRect     frustumRect = projectionRect;
  const float         offsetX = (projectionRect.l + projectionRect.r) * 0.5f;
  const float         offsetY = (projectionRect.t + projectionRect.b) * 0.5f;
  NTempest::C44Matrix mProj;

  frustumRect.t -= offsetY;
  frustumRect.l -= offsetX;
  frustumRect.b -= offsetY;
  frustumRect.r -= offsetX;

  GxuXformCreateOrtho(
      frustumRect.l, frustumRect.r, frustumRect.t, frustumRect.b, -DEFAULT_SCREEN_FRUSTUM_LENGTH, DEFAULT_SCREEN_FRUSTUM_LENGTH, mProj
  );
  mProj.Scale(NTempest::C3Vector(1.0f, 1.0f, -1.0f));
  GxXformSetProjection(mProj);

  NTempest::C44Matrix mView;
  mView.Translate(NTempest::C3Vector(screenPoint.x - offsetX, screenPoint.y - offsetY, 0.0f));
  GxXformSetView(mView);
}

void CameraSetupWorldProjection(HCAMERA camera, const NTempest::CRect &projectionRect, unsigned int flags) {
  ASSERT(camera);

  reinterpret_cast<CCamera *>(camera)->SetupWorldProjection(projectionRect, flags);
}

#include "Services/DataMgrInt.h"

void CameraUpdate(HCAMERA__* camera, float elapsedSec) {
  CCamera *cameraPtr = reinterpret_cast<CCamera *>(camera);
  FATALASSERT(cameraPtr);
  cameraPtr->Update(elapsedSec);
}
