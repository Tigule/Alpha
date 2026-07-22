#ifndef WOW_SOURCE_UIUTIL_CSIMPLECAMERA_H
#define WOW_SOURCE_UIUTIL_CSIMPLECAMERA_H

#include "Tempest/c33matrix.h"
#include "Tempest/c3vector.h"

namespace NTempest {
  class CRect;
}

class CSimpleCamera {
 public:
  CSimpleCamera();
  CSimpleCamera(float nearZ, float farZ, float fov);

  ~CSimpleCamera() {
  }

  float FarZ() {
    return m_farZ;
  }

  float NearZ() {
    return m_nearZ;
  }

  float FOV() {
    return m_fov;
  }

  float Aspect() {
    return m_aspect;
  }

  NTempest::C3Vector &Position() {
    return m_position;
  }

  virtual NTempest::C3Vector Forward() {
    return NTempest::C3Vector(m_facing.a0, m_facing.a1, m_facing.a2);
  }

  virtual NTempest::C3Vector Right() {
    return NTempest::C3Vector(m_facing.b0, m_facing.b1, m_facing.b2);
  }

  virtual NTempest::C3Vector Up() {
    return NTempest::C3Vector(m_facing.c0, m_facing.c1, m_facing.c2);
  }

  void SetFacing(float yaw, float pitch, float roll);
  void SetFacing(const NTempest::C3Vector &forward);
  void SetFacing(const NTempest::C3Vector &forward, const NTempest::C3Vector &up);
  void SetGxProjectionAndView(const NTempest::CRect &projectionRect);

 protected:
  NTempest::C3Vector  m_position;
  NTempest::C33Matrix m_facing;
  float               m_nearZ;
  float               m_farZ;
  float               m_fov;
  float               m_aspect;
};

#endif
