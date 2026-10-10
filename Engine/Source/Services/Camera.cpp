#include <Base/Base.h>
#include <Gx/Gx.h>
#include <BLPFile/blp.h>

#include "Camera.h"
#include "Camera_const.h"
#include "Services/DataMgrInt.h"

#include <Tempest/c2vector.h>
#include <Tempest/c3vector.h>
#include <Tempest/c44matrix.h>
#include <Tempest/crect.h>

#include <math.h>

class CCamera : public CDataMgr {
 public:
  TManaged<NTempest::C3Vector> m_position;
  TManaged<NTempest::C3Vector> m_target;
  TManaged<float>              m_distance;
  TManaged<float>              m_zFar;
  TManaged<float>              m_zNear;
  CAngle                       m_aoa;
  CAngle                       m_fov;
  CAngle                       m_roll;
  CAngle                       m_rotation;

  CCamera()
      : CDataMgr(9),
        m_position(NTempest::C3Vector(DEFAULT_DIST, 0.0f, 0.0f)),
        m_target(NTempest::C3Vector(0.0f, 0.0f, 0.0f)),
        m_distance(DEFAULT_DIST),
        m_zFar(DEFAULT_FARZ),
        m_zNear(DEFAULT_NEARZ),
        m_aoa(0.0f),
        m_fov(DEFAULT_FOV),
        m_roll(0.0f),
        m_rotation(0.0f) {
    AddManaged(&m_position, 7, 0);
    AddManaged(&m_target, 8, 0);
    AddManaged(&m_distance, 1, 0);
    AddManaged(&m_zFar, 2, 0);
    AddManaged(&m_zNear, 3, 0);
    AddManaged(&m_aoa, 0, 0);
    AddManaged(&m_fov, 4, 0);
    AddManaged(&m_roll, 5, 0);
    AddManaged(&m_rotation, 6, 0);
  }
  void SetupWorldProjection(const NTempest::CRect &projectionRect, UINT flags);

};

void CCamera::SetupWorldProjection(const NTempest::CRect &projectionRect, UINT flags) {
  NTempest::C44Matrix mProj;

  GxuXformCreateProjection(m_fov.Get(), projectionRect.Width() / projectionRect.Height(), m_zNear.Get(), m_zFar.Get(), mProj);
  GxXformSetProjection(mProj);

  NTempest::C44Matrix      mView;
  const NTempest::C3Vector cameraVector = m_target.Get() - m_position.Get();
  const NTempest::C3Vector cameraPos(0.0f);
  const NTempest::C3Vector upVector(m_rotation.Sin() * m_roll.Sin(), -m_rotation.Cos() * m_roll.Sin(), m_roll.Cos());

  GxuXformCreateLookAtSgCompat(cameraPos, cameraVector, upVector, mView);
  GxXformSetView(mView);
}

void CameraCalcPosFromTarg(HCAMERA__ *camera, NTempest::C3Vector *position) {
  CCamera *cameraPtr = (CCamera *)camera;
  VALIDATEBEGIN;
  VALIDATE(cameraPtr);
  VALIDATE(position);
  VALIDATEENDVOID;

  *position = NTempest::C3Vector(
                  -cameraPtr->m_rotation.Cos() * cameraPtr->m_aoa.Cos(),
                  -cameraPtr->m_rotation.Sin() * cameraPtr->m_aoa.Cos(),
                  -cameraPtr->m_aoa.Sin()
              ) * cameraPtr->m_distance.Get()
            + cameraPtr->m_target.Get();
}

void CameraCalcTargFromPos(HCAMERA__ *camera, NTempest::C3Vector *target) {
  CCamera *cameraPtr = (CCamera *)camera;
  VALIDATEBEGIN;
  VALIDATE(cameraPtr);
  VALIDATE(target);
  VALIDATEENDVOID;

  *target = NTempest::C3Vector(
                cameraPtr->m_rotation.Cos() * cameraPtr->m_aoa.Cos(),
                cameraPtr->m_rotation.Sin() * cameraPtr->m_aoa.Cos(),
                cameraPtr->m_aoa.Sin()
            ) * cameraPtr->m_distance.Get()
          + cameraPtr->m_position.Get();
}

HCAMERA CameraCreate() {
  CCamera *camera = NEWHANDLE(HCAMERA, CCamera);
  return camera ? CREATEHANDLE(HCAMERA, camera) : 0;
}

HCAMERA CameraDuplicate(HCAMERA source) {
  CCamera *srcPtr = (CCamera *)source;
  VALIDATEBEGIN;
  VALIDATE(srcPtr);
  VALIDATEEND;

  CCamera *cameraPtr = NEWHANDLE(HCAMERA, CCamera);
  if (!cameraPtr) {
    return 0;
  }

  cameraPtr->m_position.Set(srcPtr->m_position.Get());

  cameraPtr->m_target.Set(srcPtr->m_target.Get());

  cameraPtr->m_distance.Set(srcPtr->m_distance.Get());

  cameraPtr->m_zNear.Set(srcPtr->m_zNear.Get());

  cameraPtr->m_zFar.Set(srcPtr->m_zFar.Get());

  cameraPtr->m_aoa.Set(srcPtr->m_aoa.Get());

  cameraPtr->m_fov.Set(srcPtr->m_fov.Get());

  cameraPtr->m_roll.Set(srcPtr->m_roll.Get());

  cameraPtr->m_rotation.Set(srcPtr->m_rotation.Get());

  return CREATEHANDLE(HCAMERA, cameraPtr);
}

void CameraGetLineSegment(float x, float y, NTempest::C3Vector *a, NTempest::C3Vector *b) {
  VALIDATEBEGIN;
  VALIDATE(a);
  VALIDATE(b);
  VALIDATE(x >= 0 && x <= 1.0f);
  VALIDATE(y >= 0 && y <= 1.0f);
  VALIDATEENDVOID;

  NTempest::C3Vector  corners[8];
  NTempest::C44Matrix view;
  NTempest::C44Matrix proj;
  GxXformView(view);
  GxXformProjection(proj);
  GxuXformCalcFrustumCorners(view, proj, corners);

  NTempest::C3Vector lefty;
  lefty = (corners[1] - corners[0]) * y + corners[0];
  *a = (corners[3] + (corners[2] - corners[3]) * y - lefty) * x + lefty;

  lefty = corners[4] + (corners[5] - corners[4]) * y;
  *b = lefty + (corners[7] + (corners[6] - corners[7]) * y - lefty) * x;
}

void CameraSetupScreenProjection(const NTempest::CRect &projectionRect, const NTempest::C2Vector &screenPoint, float depth) {
  const float         offsetX = (projectionRect.r + projectionRect.l) * 0.5f;
  const float         offsetY = (projectionRect.b + projectionRect.t) * 0.5f;
  NTempest::CRect     frustumRect = projectionRect;
  NTempest::C44Matrix mProj;

  frustumRect.Offset(-offsetX, -offsetY);

  GxuXformCreateOrtho(
      frustumRect.l, frustumRect.r, frustumRect.t, frustumRect.b, -DEFAULT_SCREEN_FRUSTUM_LENGTH, DEFAULT_SCREEN_FRUSTUM_LENGTH, mProj
  );
  mProj.Scale(NTempest::C3Vector(1.0f, 1.0f, -1.0f));
  GxXformSetProjection(mProj);

  NTempest::C44Matrix mView;
  mView.Translate(NTempest::C3Vector(screenPoint.x - offsetX, screenPoint.y - offsetY, 0.0f));
  GxXformSetView(mView);
}

void CameraSetupWorldProjection(HCAMERA camera, const NTempest::CRect &projectionRect, UINT flags) {
  CCamera *cameraPtr = (CCamera *)camera;
  VALIDATEBEGIN;
  VALIDATE(cameraPtr);
  VALIDATEENDVOID;

  cameraPtr->SetupWorldProjection(projectionRect, flags);
}

void CameraUpdate(HCAMERA__ *camera, float elapsedSec) {
  CCamera *cameraPtr = (CCamera *)camera;
  VALIDATEBEGIN;
  VALIDATE(cameraPtr);
  VALIDATEENDVOID;
  cameraPtr->Update(elapsedSec);
}
