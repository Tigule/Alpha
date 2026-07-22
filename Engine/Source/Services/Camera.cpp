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

  GxuXformCreateProjection(m_fov.m_data, aspect, m_zNear.m_data, m_zFar.m_data, mProj);
  GxXformSetProjection(mProj);

  const NTempest::C3Vector cameraVector = m_target.m_data - m_position.m_data;
  const NTempest::C3Vector cameraPos(0.0f);
  const NTempest::C3Vector upVector(m_rotation.m_sin * m_roll.m_sin, -m_rotation.m_cos * m_roll.m_sin, m_roll.m_cos);
  NTempest::C44Matrix      mView;

  GxuXformCreateLookAtSgCompat(cameraPos, cameraVector, upVector, mView);
  GxXformSetView(mView);
}

HCAMERA __fastcall CameraCreate() {
  CCamera *camera = NEW(CCamera)();
  return camera ? reinterpret_cast<HCAMERA>(HandleCreate(camera, "HCAMERA")) : 0;
}

HCAMERA __fastcall CameraDuplicate(HCAMERA source) {
  CCamera *srcPtr = reinterpret_cast<CCamera *>(source);
  ASSERT(srcPtr);

  CCamera *cameraPtr = NEW(CCamera)();
  if (!cameraPtr) {
    return 0;
  }

  cameraPtr->m_position.m_updateFcn = 0;
  cameraPtr->m_position.m_updateData = 0;
  cameraPtr->m_position.m_updatePriority = 0.0f;
  cameraPtr->m_position.Set_(srcPtr->m_position.m_data);

  cameraPtr->m_target.m_updateFcn = 0;
  cameraPtr->m_target.m_updateData = 0;
  cameraPtr->m_target.m_updatePriority = 0.0f;
  cameraPtr->m_target.Set_(srcPtr->m_target.m_data);

  cameraPtr->m_distance.m_updateFcn = 0;
  cameraPtr->m_distance.m_updateData = 0;
  cameraPtr->m_distance.m_updatePriority = 0.0f;
  cameraPtr->m_distance.Set_(srcPtr->m_distance.m_data);

  cameraPtr->m_zNear.m_updateFcn = 0;
  cameraPtr->m_zNear.m_updateData = 0;
  cameraPtr->m_zNear.m_updatePriority = 0.0f;
  cameraPtr->m_zNear.Set_(srcPtr->m_zNear.m_data);

  cameraPtr->m_zFar.m_updateFcn = 0;
  cameraPtr->m_zFar.m_updateData = 0;
  cameraPtr->m_zFar.m_updatePriority = 0.0f;
  cameraPtr->m_zFar.Set_(srcPtr->m_zFar.m_data);

  cameraPtr->m_aoa.m_updateFcn = 0;
  cameraPtr->m_aoa.m_updateData = 0;
  cameraPtr->m_aoa.m_updatePriority = 0.0f;
  cameraPtr->m_aoa.Set_(srcPtr->m_aoa.m_data);

  cameraPtr->m_fov.m_updateFcn = 0;
  cameraPtr->m_fov.m_updateData = 0;
  cameraPtr->m_fov.m_updatePriority = 0.0f;
  cameraPtr->m_fov.Set_(srcPtr->m_fov.m_data);

  cameraPtr->m_roll.m_updateFcn = 0;
  cameraPtr->m_roll.m_updateData = 0;
  cameraPtr->m_roll.m_updatePriority = 0.0f;
  cameraPtr->m_roll.Set_(srcPtr->m_roll.m_data);

  cameraPtr->m_rotation.m_updateFcn = 0;
  cameraPtr->m_rotation.m_updateData = 0;
  cameraPtr->m_rotation.m_updatePriority = 0.0f;
  cameraPtr->m_rotation.Set_(srcPtr->m_rotation.m_data);

  return reinterpret_cast<HCAMERA>(HandleCreate(cameraPtr, "HCAMERA"));
}

void __fastcall CameraGetLineSegment(float x, float y, NTempest::C3Vector *a, NTempest::C3Vector *b) {
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
  NTempest::C3Vector righty = corners[3] + (corners[2] - corners[3]) * y;
  *a = lefty + (righty - lefty) * x;

  lefty = corners[4] + (corners[5] - corners[4]) * y;
  righty = corners[7] + (corners[6] - corners[7]) * y;
  *b = lefty + (righty - lefty) * x;
}

void __fastcall CameraSetupScreenProjection(const NTempest::CRect &projectionRect, const NTempest::C2Vector &screenPoint, float depth) {
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

void __fastcall CameraSetupWorldProjection(HCAMERA camera, const NTempest::CRect &projectionRect, unsigned int flags) {
  ASSERT(camera);

  reinterpret_cast<CCamera *>(camera)->SetupWorldProjection(projectionRect, flags);
}

#include "Services/DataMgrInt.h"
