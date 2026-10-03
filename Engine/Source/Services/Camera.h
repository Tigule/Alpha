#pragma once

#include "Services/DataMgr.h"
#include "Tempest/crect.h"

namespace NTempest {
  class C2Vector;
  class CRect;
}  // namespace NTempest

DECLARE_DERIVED_HANDLE(HCAMERA, HDATAMGR);

class CAngle : public TManaged<float> {
 private:
  float m_cos;
  float m_sin;

  void  Calc();
  float ClampTo2Pi(float angle);

 protected:
  virtual void Set_(const float &angle);

 public:
  CAngle();
  CAngle(float angle);
  float Cos() {
    return m_cos;
  }
  float Sin() {
    return m_sin;
  }
};

HCAMERA CameraCreate();
HCAMERA CameraDuplicate(HCAMERA source);

void CameraGetLineSegment(float x, float y, NTempest::C3Vector *a, NTempest::C3Vector *b);

void CameraSetupWorldProjection(HCAMERA camera, const NTempest::CRect &projectionRect, UINT flags);

void CameraSetupScreenProjection(const NTempest::CRect &projectionRect, const NTempest::C2Vector &screenPoint, float depth);
