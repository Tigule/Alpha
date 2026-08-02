#include <Base/Base.h>

#include "ParticleSystem2.h"

#include "Tempest/cmath.h"
#include "Tempest/c33matrix.h"
#include "Tempest/c4vector.h"

#include <math.h>

CPlaneParticleEmitter::CPlaneParticleEmitter() : m_width(0.0f), m_height(0.0f), m_latitude(0.0f), m_longitude(0.0f) {
  m_emitterType = PET_PLANE_EMITTER;
}

CPlaneParticleEmitter::CPlaneParticleEmitter(const CPlaneParticleEmitter &rhs, int deep)
    : CParticleEmitter2(rhs, deep), m_width(rhs.m_width), m_height(rhs.m_height), m_latitude(rhs.m_latitude), m_longitude(rhs.m_longitude) {
}

CPlaneParticleEmitter::~CPlaneParticleEmitter() {
}

void CPlaneParticleEmitter::CreateParticle(CParticle2 &p, float elapsedTime, const NTempest::C34Matrix &basis) {
  p.m_keyFrame = 0;
  p.m_age = NTempest::CRandom::real_(m_randSeed) * elapsedTime;
  p.m_position =
      NTempest::C3Vector(NTempest::CRandom::reals_(m_randSeed) * m_width * 0.5f, NTempest::CRandom::reals_(m_randSeed) * m_height * 0.5f, 0.0f);

  float              speed = CalcVelocity();
  NTempest::C4Vector vel(0.0f, 0.0f, speed, 0.0f);
  if (m_particleZsource == 0.0f) {
    elapsedTime = NTempest::CRandom::reals_(m_randSeed) * m_latitude;
    float rotZ = NTempest::CRandom::reals_(m_randSeed) * m_longitude;
    float radial = sin(elapsedTime) * speed;
    vel = NTempest::C4Vector(cos(rotZ) * radial, sin(rotZ) * radial, cos(elapsedTime) * speed, 0.0f);
  } else {
    NTempest::C3Vector zsvel(p.m_position.x, p.m_position.y, p.m_position.z - m_particleZsource);
    zsvel *= speed / NTempest::CMath::sqrt_(zsvel.SquaredMag());
    vel = NTempest::C4Vector(zsvel.x, zsvel.y, zsvel.z, 0.0f);
  }

  if (m_useModelSpace) {
    p.m_velocity = NTempest::C3Vector(vel.x, vel.y, vel.z);
  } else {
    p.m_velocity.x = vel.x * basis.a0 + vel.y * basis.b0 + vel.z * basis.c0;
    p.m_velocity.y = vel.x * basis.a1 + vel.y * basis.b1 + vel.z * basis.c1;
    p.m_velocity.z = vel.x * basis.a2 + vel.y * basis.b2 + vel.z * basis.c2;
    p.m_position *= basis;
    if (m_project) {
      ProjectParticle(p);
    }
  }
  if (m_instantVelLin) {
    float scale = NTempest::CRandom::reals_(m_randSeed) * m_particleVelocityVariation + 1.0f;
    p.m_velocity += m_frameInstantVelLin * scale;
  }
}

float CPlaneParticleEmitter::Width() {
  return m_width;
}
float CPlaneParticleEmitter::Height() {
  return m_height;
}
float CPlaneParticleEmitter::Latitude() {
  return m_latitude;
}
float CPlaneParticleEmitter::Longitude() {
  return m_longitude;
}
void CPlaneParticleEmitter::SetWidth(float width) {
  m_width = width;
}
void CPlaneParticleEmitter::SetHeight(float height) {
  m_height = height;
}
void CPlaneParticleEmitter::SetLatitude(float latInRadians) {
  m_latitude = latInRadians;
}
void CPlaneParticleEmitter::SetLongitude(float longInRadians) {
  m_longitude = longInRadians;
}

CSphereParticleEmitter::CSphereParticleEmitter()
    : m_innerRadius(0.0f), m_outerRadius(0.0f), m_radiusRange(0.0f), m_latitude(0.0f), m_longitude(0.0f) {
  m_emitterType = PET_SPHERE_EMITTER;
}

CSphereParticleEmitter::CSphereParticleEmitter(const CSphereParticleEmitter &rhs, int deep)
    : CParticleEmitter2(rhs, deep),
      m_innerRadius(rhs.m_innerRadius),
      m_outerRadius(rhs.m_outerRadius),
      m_radiusRange(rhs.m_radiusRange),
      m_latitude(rhs.m_latitude),
      m_longitude(rhs.m_longitude) {
}

CSphereParticleEmitter::~CSphereParticleEmitter() {
}

void CSphereParticleEmitter::CreateParticle(CParticle2 &p, float elapsedTime, const NTempest::C34Matrix &basis) {
  p.m_keyFrame = 0;
  p.m_age = NTempest::CRandom::real_(m_randSeed) * elapsedTime;
  float              radius = NTempest::CRandom::real_(m_randSeed) * m_radiusRange + m_innerRadius;
  float              theta = NTempest::CRandom::reals_(m_randSeed) * m_latitude;
  float              rho = NTempest::CRandom::reals_(m_randSeed) * m_longitude;
  float              planar = cos(theta);
  NTempest::C3Vector cart(cos(rho) * planar, sin(rho) * planar, sin(theta));
  p.m_position = cart * radius;

  NTempest::C4Vector vel(cart.x, cart.y, cart.z, 0.0f);
  if (m_particleZsource != 0.0f) {
    NTempest::C3Vector zsvel(p.m_position.x, p.m_position.y, p.m_position.z - m_particleZsource);
    zsvel *= 1.0f / NTempest::CMath::sqrt_(zsvel.SquaredMag());
    vel = NTempest::C4Vector(zsvel.x, zsvel.y, zsvel.z, 0.0f);
  } else if (m_zvelOnly) {
    vel = NTempest::C4Vector(0.0f, 0.0f, 1.0f, 0.0f);
  }

  vel *= CalcVelocity();
  if (m_useModelSpace) {
    p.m_velocity = NTempest::C3Vector(vel.x, vel.y, vel.z);
  } else {
    p.m_velocity.x = vel.x * basis.a0 + vel.y * basis.b0 + vel.z * basis.c0;
    p.m_velocity.y = vel.x * basis.a1 + vel.y * basis.b1 + vel.z * basis.c1;
    p.m_velocity.z = vel.x * basis.a2 + vel.y * basis.b2 + vel.z * basis.c2;
    p.m_position *= basis;
    if (m_project) {
      ProjectParticle(p);
    }
  }
  if (m_instantVelLin) {
    float scale = NTempest::CRandom::reals_(m_randSeed) * m_particleVelocityVariation + 1.0f;
    p.m_velocity += m_frameInstantVelLin * scale;
  }
}

float CSphereParticleEmitter::InnerRadius() {
  return m_innerRadius;
}
float CSphereParticleEmitter::OuterRadius() {
  return m_outerRadius;
}
float CSphereParticleEmitter::Latitude() {
  return m_latitude;
}
float CSphereParticleEmitter::Longitude() {
  return m_longitude;
}
void CSphereParticleEmitter::SetWidth(float radius) {
  m_radiusRange = m_outerRadius - radius;
  m_innerRadius = radius;
}
void CSphereParticleEmitter::SetHeight(float radius) {
  m_outerRadius = radius;
  m_radiusRange = radius - m_innerRadius;
}
void CSphereParticleEmitter::SetLatitude(float latInRadians) {
  m_latitude = latInRadians;
}
void CSphereParticleEmitter::SetLongitude(float longInRadians) {
  m_longitude = longInRadians;
}

CSplineParticleEmitter::CSplineParticleEmitter()
    : m_requestedEmissionRate(0.0f), m_start(0.0f), m_end(0.0f), m_latitude(0.0f), m_radius(0.0f), m_emitAtEnd(0) {
  m_emitterType = PET_SPLINE_EMITTER;
}

CSplineParticleEmitter::CSplineParticleEmitter(const CSplineParticleEmitter &rhs, int deep)
    : CParticleEmitter2(rhs, deep),
      m_requestedEmissionRate(rhs.m_requestedEmissionRate),
      m_start(rhs.m_start),
      m_end(rhs.m_end),
      m_latitude(rhs.m_latitude),
      m_radius(rhs.m_radius),
      m_emitAtEnd(rhs.m_emitAtEnd),
      m_spline(rhs.m_spline) {
}

CSplineParticleEmitter::~CSplineParticleEmitter() {
}

void CSplineParticleEmitter::CreateParticle(CParticle2 &p, float elapsedTime, const NTempest::C34Matrix &basis) {
  p.m_keyFrame = 0;
  p.m_age = NTempest::CRandom::real_(m_randSeed) * elapsedTime;
  float t;
  if (m_emitAtEnd) {
    t = m_end;
    m_emitAtEnd = 0;
  } else {
    t = m_start + (m_end - m_start) * NTempest::CRandom::real_(m_randSeed);
  }
  m_spline.Pos(t, p.m_position, NTempest::C3Spline::EVAL_ARCLENGTH);

  float              speed = CalcVelocity();
  NTempest::C4Vector vel(0.0f, 0.0f, speed, 0.0f);
  if (m_particleZsource != 0.0f) {
    NTempest::C3Vector zsvel(p.m_position.x, p.m_position.y, p.m_position.z - m_particleZsource);
    zsvel *= speed / NTempest::CMath::sqrt_(zsvel.SquaredMag());
    vel = NTempest::C4Vector(zsvel.x, zsvel.y, zsvel.z, 0.0f);
  } else if (m_latitude != 0.0f) {
    NTempest::C3Vector tangent;
    m_spline.Vel(t, tangent, NTempest::C3Spline::EVAL_ARCLENGTH);
    tangent.Normalize();
    float              angle = NTempest::CRandom::reals_(m_randSeed) * m_latitude;
    NTempest::C3Vector posVector = NTempest::C33Matrix::Rotation(angle, tangent, true).Row2();
    vel = NTempest::C4Vector(posVector.x * speed, posVector.y * speed, posVector.z * speed, 0.0f);
    if (m_radius != 0.0f) {
      p.m_position += posVector * (NTempest::CRandom::real_(m_randSeed) * m_radius);
    }
  }

  if (m_useModelSpace) {
    p.m_velocity = NTempest::C3Vector(vel.x, vel.y, vel.z);
  } else {
    p.m_velocity.x = vel.x * basis.a0 + vel.y * basis.b0 + vel.z * basis.c0;
    p.m_velocity.y = vel.x * basis.a1 + vel.y * basis.b1 + vel.z * basis.c1;
    p.m_velocity.z = vel.x * basis.a2 + vel.y * basis.b2 + vel.z * basis.c2;
    p.m_position *= basis;
    if (m_project) {
      ProjectParticle(p);
    }
  }
  if (m_instantVelLin) {
    float scale = NTempest::CRandom::reals_(m_randSeed) * m_particleVelocityVariation + 1.0f;
    p.m_velocity += m_frameInstantVelLin * scale;
  }
}

float CSplineParticleEmitter::Start() {
  return m_start;
}
float CSplineParticleEmitter::End() {
  return m_end;
}
float CSplineParticleEmitter::Latitude() {
  return m_latitude;
}
float CSplineParticleEmitter::Radius() {
  return m_radius;
}
void CSplineParticleEmitter::SetWidth(float start) {
  m_start = start < 0.0f ? 0.0f : (start > 1.0f ? 1.0f : start);
}
void CSplineParticleEmitter::SetHeight(float end) {
  end = end < 0.0f ? 0.0f : (end > 1.0f ? 1.0f : end);
  if (fabs(end - m_end) >= 2.3841858e-7f) {
    m_emitAtEnd = 1;
    m_end = end;
    SetActualEmissionRate();
  }
}
void CSplineParticleEmitter::SetLatitude(float latInRadians) {
  m_latitude = latInRadians;
}
void CSplineParticleEmitter::SetLongitude(float radius) {
  m_radius = radius;
}
void CSplineParticleEmitter::SetEmissionRate(float particlesPerSecond) {
  m_requestedEmissionRate = particlesPerSecond;
  SetActualEmissionRate();
}
void CSplineParticleEmitter::SetSpline(const NTempest::C3Vector *points, unsigned int numPoints) {
  m_spline.SetPoints(points, numPoints);
}
