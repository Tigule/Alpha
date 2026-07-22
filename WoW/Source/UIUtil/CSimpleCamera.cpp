#include "UIUtil/CSimpleCamera.h"

#include <Gx/Gx.h>
#include <Tempest/c44matrix.h>
#include <Tempest/crect.h>

static void __fastcall
FaceDirection(const NTempest::C3Vector &direction, NTempest::C3Vector *xprime, NTempest::C3Vector *yprime, NTempest::C3Vector *zprime) {
  ASSERT(NTempest::CMath::fnotequal_(direction.SquaredMag(), 0.0f));

  *xprime = direction;

  if (NTempest::CMath::fnotequal_(xprime->x * xprime->x + xprime->y * xprime->y, 0.0f)) {
    yprime->Set(-xprime->y, xprime->x, 0.0f);
    yprime->Normalize();
  } else {
    yprime->Set(1.0f, 0.0f, 0.0f);
  }

  *zprime = NTempest::C3Vector::Cross(*xprime, *yprime);
}

static void __fastcall BuildBillboardMatrix(const NTempest::C3Vector &direction, NTempest::C33Matrix *rotation) {
  NTempest::C3Vector zprime;
  NTempest::C3Vector yprime;
  NTempest::C3Vector xprime;

  FaceDirection(direction, &xprime, &yprime, &zprime);
  *rotation = NTempest::C33Matrix(xprime.x, xprime.y, xprime.z, yprime.x, yprime.y, yprime.z, zprime.x, zprime.y, zprime.z);
}

static void __fastcall FaceDirectionWithRoll(
    const NTempest::C3Vector &direction,
    const NTempest::C3Vector &up,
    NTempest::C3Vector       *xprime,
    NTempest::C3Vector       *yprime,
    NTempest::C3Vector       *zprime
) {
  ASSERT(NTempest::CMath::fnotequal_(direction.SquaredMag(), 0.0f));

  *xprime = direction;

  if (NTempest::CMath::fnotequal_(NTempest::CMath::fabs_(NTempest::C3Vector::Dot(up, *xprime)), 1.0f)) {
    *yprime = NTempest::C3Vector::Cross(up, *xprime);
    yprime->Normalize();
  } else {
    yprime->Set(1.0f, 0.0f, 0.0f);
  }

  *zprime = NTempest::C3Vector::Cross(*xprime, *yprime);
}

static void __fastcall
BuildBillboardMatrixWithRoll(const NTempest::C3Vector &direction, const NTempest::C3Vector &up, NTempest::C33Matrix *rotation) {
  NTempest::C3Vector zprime;
  NTempest::C3Vector yprime;
  NTempest::C3Vector xprime;

  FaceDirectionWithRoll(direction, up, &xprime, &yprime, &zprime);
  *rotation = NTempest::C33Matrix(xprime.x, xprime.y, xprime.z, yprime.x, yprime.y, yprime.z, zprime.x, zprime.y, zprime.z);
}

CSimpleCamera::CSimpleCamera()
    : m_position(0.0f),
      m_facing(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f),
      m_nearZ(0.11111111f),
      m_farZ(277.77777f),
      m_fov(1.5707964f),
      m_aspect(1.0f) {
  SetFacing(0.0f, 0.0f, 0.0f);
}

CSimpleCamera::CSimpleCamera(float nearZ, float farZ, float fov)
    : m_position(0.0f), m_facing(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f), m_nearZ(nearZ), m_farZ(farZ), m_fov(fov), m_aspect(1.0f) {
  SetFacing(0.0f, 0.0f, 0.0f);
}

void CSimpleCamera::SetFacing(const NTempest::C3Vector &forward) {
  BuildBillboardMatrix(forward, &m_facing);
}

void CSimpleCamera::SetFacing(const NTempest::C3Vector &forward, const NTempest::C3Vector &up) {
  BuildBillboardMatrixWithRoll(forward, up, &m_facing);
}

void CSimpleCamera::SetFacing(float yaw, float pitch, float roll) {
  m_facing.FromEulerAnglesZYX(yaw, pitch, roll);
}

void CSimpleCamera::SetGxProjectionAndView(const NTempest::CRect &projectionRect) {
  NTempest::C44Matrix projection;
  m_aspect = (projectionRect.b - projectionRect.t) / (projectionRect.r - projectionRect.l);
  GxuXformCreateProjection(m_fov, m_aspect, m_nearZ, m_farZ, projection);
  GxXformSetProjection(projection);

  NTempest::C44Matrix view;
  NTempest::C3Vector  eye(0.0f);
  NTempest::C3Vector  center = Forward();
  NTempest::C3Vector  up = Up();
  GxuXformCreateLookAtSgCompat(eye, center, up, view);
  GxXformSetView(view);
}
