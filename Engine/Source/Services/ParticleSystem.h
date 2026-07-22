#pragma once

#include "Model/IModel.h"
#include "Services/IParticleMisc.h"
#include "Tempest/c3vector.h"

#include <stddef.h>
#include <stpl.h>

class CParticle {
 public:
  CParticle() : m_timeToLive(0.0f), m_position(0.0f), m_velocity(0.0f), m_hmodel(0) {
  }

  ~CParticle() {
    Destroy();
  }

 private:
  friend class CParticleEmitter;

  void Destroy();

  float              m_timeToLive;
  float              m_elapsed;
  NTempest::C3Vector m_position;
  NTempest::C3Vector m_velocity;
  float              m_scale;
  HMODEL             m_hmodel;
};

class CParticleEmitter {
  friend class ParticleSystemManager;

 public:
  ~CParticleEmitter();

  void Update(float elapsedTime, const NTempest::C3Vector &cameraWorldPos, const NTempest::C3Vector &cameraVector);
  void DecRef();

 private:
  void SyncAllocation();
  void CreateParticle(CParticle &p, float elapsedTime, const NTempest::C3Vector &cameraWorldPos);
  void DestroyParticle(CParticle &p);
  void MoveParticle(CParticle &p, float elapsedTime);
  void Destroy();

  unsigned int               m_refCount;
  float                      m_numNew;
  int                        m_enabled;
  int                        m_enabled2;
  float                      m_particleEmissionRate;
  float                      m_particleLifeSpan;
  float                      m_velocity;
  float                      m_acceleration;
  float                      m_scale;
  float                      m_latitude;
  float                      m_longitude;
  HMODEL                     m_hmodel;
  TSGrowableArray<CParticle> m_particles;
  CParticleStack             m_alive;
  CParticleStack             m_dead;
};
