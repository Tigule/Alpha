#pragma once

#include "Services/DataMgr.h"
#include "Tempest/crect.h"

namespace NTempest {
  class C2Vector;
  class CRect;
}  // namespace NTempest

DECLARE_DERIVED_HANDLE(HCAMERA, HDATAMGR);

HCAMERA CameraCreate();
HCAMERA CameraDuplicate(HCAMERA source);

void CameraGetLineSegment(float x, float y, NTempest::C3Vector *a, NTempest::C3Vector *b);

void CameraSetupWorldProjection(HCAMERA camera, const NTempest::CRect &projectionRect, UINT flags);

void CameraSetupScreenProjection(const NTempest::CRect &projectionRect, const NTempest::C2Vector &screenPoint, float depth);
