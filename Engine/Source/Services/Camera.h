#pragma once

#include "Services/DataMgr.h"
#include "Tempest/crect.h"

namespace NTempest {
  class C2Vector;
  class CRect;
}  // namespace NTempest

DECLARE_DERIVED_HANDLE(HCAMERA, HDATAMGR);

class CAngle : public TManaged<float> {
 public:
  CAngle();
  CAngle(float angle);
  float Cos() {
    return m_cos;
  }
  float Sin() {
    return m_sin;
  }

  friend HCAMERA CameraDuplicate(HCAMERA source);

 protected:
  virtual void Set_(const float &angle);

 private:
  void  Calc();
  float ClampTo2Pi(float angle);
  float m_cos;
  float m_sin;
};

class CCamera : public CDataMgr {
 public:
  CCamera()
      : CDataMgr(9),
        m_position(NTempest::C3Vector(100.0f, 0.0f, 0.0f)),
        m_target(NTempest::C3Vector(0.0f)),
        m_distance(100.0f),
        m_zFar(5000.0f),
        m_zNear(8.0f),
        m_aoa(0.0f),
        m_fov(3.14159265358979323846f * 0.5f),
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

  friend HCAMERA CameraDuplicate(HCAMERA source);

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
};

HCAMERA CameraCreate();
HCAMERA CameraDuplicate(HCAMERA source);

void CameraGetLineSegment(float x, float y, NTempest::C3Vector *a, NTempest::C3Vector *b);

void CameraSetupWorldProjection(HCAMERA camera, const NTempest::CRect &projectionRect, UINT flags);

void CameraSetupScreenProjection(const NTempest::CRect &projectionRect, const NTempest::C2Vector &screenPoint, float depth);
