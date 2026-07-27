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

  NTempest::C33Matrix &Facing() {
    return m_facing;
  }

  void SetPosition(const NTempest::C3Vector &position) {
    m_position = position;
  }

  void SetPosition(float x, float y, float z) {
    m_position.Set(x, y, z);
  }

  void SetFieldOfView(float fov) {
    m_fov = fov;
  }

  void SetNearZ(float nearZ) {
    m_nearZ = nearZ;
  }

  void SetFarZ(float farZ) {
    m_farZ = farZ;
  }

  virtual NTempest::C3Vector Forward() const;
  virtual NTempest::C3Vector Right() const;
  virtual NTempest::C3Vector Up() const;

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
