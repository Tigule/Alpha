#include "Services/ParticleSystem.h"

#include "Anim/WorldMatrix.h"
#include "Base/Activity.h"
#include "Tempest/cmath.h"
#include "Tempest/crandom.h"

static NTempest::CRndSeed s_particleRandomSeed;

void CParticle::Destroy() {
  m_timeToLive = 0.0f;
  if (m_hmodel) {
    HandleClose(m_hmodel);
  }
}

void CParticleEmitter::Destroy() {
  if (m_hmodel) {
    HandleClose(m_hmodel);
  }
}

void CParticleEmitter::SyncAllocation() {
  unsigned int count = static_cast<unsigned int>(m_particleLifeSpan * m_particleEmissionRate * 1.15f);
  unsigned int oldCount = m_particles.Count();
  if (oldCount >= count) {
    return;
  }

  m_particles.SetCount(count);
  m_alive.m_stack.SetCount(count);
  m_dead.m_stack.SetCount(count);
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
    while (numNew && m_dead.m_stackPointer) {
      --numNew;
      unsigned int particle = m_dead.Pop();
      m_alive.Push(particle);
      CreateParticle(m_particles[particle], elapsedTime, cameraWorldPos);
      ++numEmitted;
    }
    m_numNew -= static_cast<float>(numEmitted);
  }

  for (unsigned int index = 0; index < m_alive.m_stackPointer; ++index) {
    CParticle &particle = m_particles[m_alive.m_stack[index]];
    particle.m_timeToLive -= elapsedTime;
    if (particle.m_timeToLive <= 0.0f) {
      DestroyParticle(particle);
      unsigned int deadParticle = m_alive.m_stack[index];
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

void CParticleEmitter::DecRef() {
  if (!--m_refCount) {
    DEL(this);
  }
}
