#include "Services/ParticleSystem.h"

#include "Anim/WorldMatrix.h"
#include "Base/Activity.h"
#include "Tempest/cmath.h"
#include "Tempest/crandom.h"

static NTempest::CRndSeed s_particleRandomSeed;

void CParticle::Copy(const CParticle &rhs) {
  m_timeToLive = rhs.m_timeToLive;
  m_elapsed = rhs.m_elapsed;
  m_position = rhs.m_position;
  m_velocity = rhs.m_velocity;
  m_scale = rhs.m_scale;
  m_hmodel = rhs.m_hmodel ? ModelDuplicate(rhs.m_hmodel, 0) : 0;
}

void CParticle::Destroy() {
  m_timeToLive = 0.0f;
  if (m_hmodel) {
    HandleClose(m_hmodel);
  }
}

void CParticleEmitter::Init() {
  m_numNew = 0.0f;
  m_enabled = 1;
  m_enabled2 = 1;
  m_particleEmissionRate = 0.0f;
  m_particleLifeSpan = 0.0f;
  m_velocity = 0.0f;
  m_acceleration = 0.0f;
  m_scale = 1.0f;
  m_latitude = 3.1415927f * 0.25f;
  m_longitude = 3.1415927f * 0.25f;
  m_hmodel = 0;
}

void CParticleEmitter::Copy(const CParticleEmitter &rhs) {
  m_numNew = rhs.m_numNew;
  m_enabled = rhs.m_enabled;
  m_enabled2 = rhs.m_enabled2;
  m_particleEmissionRate = rhs.m_particleEmissionRate;
  m_particleLifeSpan = rhs.m_particleLifeSpan;
  m_velocity = rhs.m_velocity;
  m_acceleration = rhs.m_acceleration;
  m_scale = rhs.m_scale;
  m_latitude = rhs.m_latitude;
  m_longitude = rhs.m_longitude;
  m_hmodel = rhs.m_hmodel ? ModelDuplicate(rhs.m_hmodel, 0) : 0;
}

void CParticleEmitter::Destroy() {
  if (m_hmodel) {
    HandleClose(m_hmodel);
  }
}

CParticleEmitter::CParticleEmitter() : m_refCount(1) {
  Init();
}

CParticleEmitter::CParticleEmitter(const CParticleEmitter &rhs) : m_refCount(1) {
  Copy(rhs);
}

CParticleEmitter &CParticleEmitter::operator=(const CParticleEmitter &rhs) {
  Destroy();
  Copy(rhs);
  return *this;
}

void CParticleEmitter::SyncAllocation() {
  unsigned int count = static_cast<unsigned int>(m_particleLifeSpan * m_particleEmissionRate * 1.15f);
  unsigned int oldCount = m_particles.Count();
  if (oldCount >= count) {
    return;
  }

  m_particles.SetCount(count);
  m_alive.SetCount(count);
  m_dead.SetCount(count);
  for (unsigned int u = oldCount; u < count; ++u) {
    m_dead.Push(u);
  }
}

void CParticleEmitter::CreateParticle(CParticle &p, float elapsedTime, const NTempest::C3Vector &cameraWorldPos) {
  p.m_elapsed = NTempest::CRandom::real_(s_particleRandomSeed) * elapsedTime;
  if (p.m_elapsed >= m_particleLifeSpan) {
    p.m_elapsed = 0.0f;
  }
  p.m_timeToLive = m_particleLifeSpan - p.m_elapsed;
  p.m_position = 0.0f;
  WorldMatrixTransform(&p.m_position);

  float theta = (NTempest::CRandom::real_(s_particleRandomSeed) * 2.0f - 1.0f) * m_latitude;
  float longitude = (NTempest::CRandom::real_(s_particleRandomSeed) * 2.0f - 1.0f) * m_longitude;
  p.m_velocity.x = NTempest::CMath::sin_(theta) * m_velocity;
  p.m_velocity.z = NTempest::CMath::cos_(theta) * m_velocity;
  p.m_velocity.y = NTempest::CMath::sin_(longitude) * p.m_velocity.x;
  p.m_velocity.x = NTempest::CMath::cos_(longitude) * p.m_velocity.x;
  WorldMatrixTransform(&p.m_velocity);
  p.m_velocity.x -= p.m_position.x;
  p.m_velocity.y -= p.m_position.y;
  p.m_velocity.z -= p.m_position.z;
  MoveParticle(p, p.m_elapsed);
  p.m_scale = m_scale;
  ASSERT(m_hmodel);
  if (!p.m_hmodel) {
    p.m_hmodel = ModelDuplicate(m_hmodel, 0);
  }
  ModelSetRandomSequenceFidget(p.m_hmodel, 0, 0);
  p.m_position += cameraWorldPos;
}

void CParticleEmitter::DestroyParticle(CParticle &p) {
  p.m_timeToLive = 0.0f;
}

void CParticleEmitter::MoveParticle(CParticle &p, float elapsedTime) {
  NTempest::C3Vector acceleration(0.0f, 0.0f, -m_acceleration);
  p.m_position += p.m_velocity * elapsedTime + acceleration * (0.5f * elapsedTime * elapsedTime);
  p.m_velocity += acceleration * elapsedTime;
}

CParticleEmitter::~CParticleEmitter() {
  Destroy();
}

float CParticleEmitter::Velocity() {
  return m_velocity;
}

float CParticleEmitter::Acceleration() {
  return m_acceleration;
}

float CParticleEmitter::Scale() {
  return m_scale;
}

float CParticleEmitter::Latitude() {
  return m_latitude;
}

float CParticleEmitter::Longitude() {
  return m_longitude;
}

float CParticleEmitter::ParticleEmissionRate() {
  return m_particleEmissionRate;
}

float CParticleEmitter::ParticleLifeSpan() {
  return m_particleLifeSpan;
}

void CParticleEmitter::Enabled(int enable) {
  m_enabled = enable;
}

void CParticleEmitter::Enabled2(int enable2) {
  m_enabled2 = enable2;
}

void CParticleEmitter::Update(float elapsedTime, const NTempest::C3Vector &cameraWorldPos, const NTempest::C3Vector &cameraVector) {
  ActivityBegin(ACTIVITY_PARTICLE);
  if (elapsedTime <= 0.0f) {
    elapsedTime = 0.0f;
  }

  SyncAllocation();
  unsigned int numNew = 0;
  unsigned int numEmitted = 0;
  if (m_enabled && m_enabled2) {
    m_numNew += ParticleSystemManager::GetScaler() * m_particleEmissionRate * elapsedTime;
    numNew = static_cast<unsigned int>(m_numNew);
    while (numNew && !m_dead.IsEmpty()) {
      --numNew;
      unsigned int particle = m_dead.Pop();
      m_alive.Push(particle);
      CreateParticle(m_particles[particle], elapsedTime, cameraWorldPos);
      ++numEmitted;
    }
    m_numNew -= static_cast<float>(numEmitted);
  }

  for (unsigned int index = 0; index < m_alive.Count(); ++index) {
    CParticle &particle = m_particles[m_alive[index]];
    particle.m_timeToLive -= elapsedTime;
    if (particle.m_timeToLive <= 0.0f) {
      DestroyParticle(particle);
      unsigned int deadParticle = m_alive[index];
      m_dead.Push(deadParticle);
      m_alive.Remove(index);
      --index;
    } else {
      MoveParticle(particle, elapsedTime);
      if (ModelAdvanceTime(particle.m_hmodel)) {
        NTempest::C3Vector axis(0.0f, 0.0f, 1.0f);
        ModelAnimate(particle.m_hmodel, particle.m_position, 0.0f, axis, particle.m_scale, cameraWorldPos, cameraVector);
      }
    }
  }
  ActivityEnd(ACTIVITY_PARTICLE);
}

void CParticleEmitter::AddToModelScene() {
  ActivityBegin(ACTIVITY_PARTICLE);
  unsigned int numAlive = m_alive.Count();
  for (unsigned int loop = 0; loop < numAlive; ++loop) {
    CParticle &particle = m_particles[m_alive[loop]];
    ASSERT(particle.m_timeToLive >= 0.0f);
    if (particle.m_timeToLive != 0.0f) {
      ModelAddToScene(particle.m_hmodel, 0);
    }
  }
  ActivityEnd(ACTIVITY_PARTICLE);
}

void CParticleEmitter::Render() {
  ActivityBegin(ACTIVITY_PARTICLE);
  unsigned int numAlive = m_alive.Count();
  for (unsigned int loop = 0; loop < numAlive; ++loop) {
    CParticle &particle = m_particles[m_alive[loop]];
    ASSERT(particle.m_timeToLive >= 0.0f);
    if (particle.m_timeToLive != 0.0f) {
      ModelRender(particle.m_hmodel, 0, 0);
    }
  }
  ActivityEnd(ACTIVITY_PARTICLE);
}

void CParticleEmitter::Flush() {
  unsigned int count = m_particles.Count();
  while (count) {
    CParticle &particle = m_particles[--count];
    ASSERT(particle.m_timeToLive >= 0.0f);
    if (particle.m_timeToLive != 0.0f) {
      DestroyParticle(particle);
    }
  }
}

void CParticleEmitter::SetVelocity(float vel) {
  m_velocity = vel;
}

void CParticleEmitter::SetAcceleration(float accel) {
  m_acceleration = accel;
}

void CParticleEmitter::SetScale(float scale) {
  m_scale = scale;
}

void CParticleEmitter::SetLatitude(float latInDegrees) {
  m_latitude = latInDegrees;
}

void CParticleEmitter::SetLongitude(float longitudeInDegrees) {
  m_longitude = longitudeInDegrees;
}

void CParticleEmitter::SetParticleEmissionRate(float particlesPerSec) {
  m_particleEmissionRate = particlesPerSec;
}

void CParticleEmitter::SetParticleLifeSpan(float lifeInSec) {
  m_particleLifeSpan = lifeInSec;
}

void CParticleEmitter::SetModel(HMODEL hmodel) {
  if (m_hmodel) {
    HandleClose(m_hmodel);
  }
  m_hmodel = hmodel ? ModelDuplicate(hmodel, 0) : 0;
}

CParticleEmitter *CParticleEmitter::AddRef() {
  ++m_refCount;
  return this;
}

void CParticleEmitter::DecRef() {
  if (!--m_refCount) {
    DEL(this);
  }
}
