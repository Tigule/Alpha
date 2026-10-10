#pragma once

#include "Model/IModel.h"
#include "Services/IParticleMisc.h"
#include "Tempest/c3vector.h"

#include <stddef.h>
#include <stpl.h>

enum {
  PART_ANIM_STAND = 0
};

class CParticle {
  friend class CParticleEmitter;

 private:
  float              m_timeToLive;
  float              m_elapsed;
  NTempest::C3Vector m_position;
  NTempest::C3Vector m_velocity;
  float              m_scale;
  HMODEL             m_hmodel;

  void Init() {
    m_timeToLive = 0.0f;
    m_hmodel = 0;
  }

  void Copy(const CParticle &rhs);
  void Destroy();

 public:
  CParticle() : m_position(0.0f), m_velocity(0.0f) {
    Init();
  }

  CParticle(const CParticle &rhs) : m_position(0.0f), m_velocity(0.0f) {
    Copy(rhs);
  }

  CParticle &operator=(const CParticle &rhs) {
    Destroy();
    Copy(rhs);
    return *this;
  }

  ~CParticle() {
    Destroy();
  }
};

class CParticleEmitter {
  friend class ParticleSystemManager;

 private:
  UINT                       m_refCount;
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

  void Init();
  void Copy(const CParticleEmitter &rhs);
  void Destroy();
  void SyncAllocation();
  void CreateParticle(CParticle &p, float elapsedTime, const NTempest::C3Vector &cameraWorldPos);
  void DestroyParticle(CParticle &p);
  void MoveParticle(CParticle &p, float elapsedTime);

 public:
  CParticleEmitter();
  CParticleEmitter(const CParticleEmitter &rhs);
  CParticleEmitter &operator=(const CParticleEmitter &rhs);
  ~CParticleEmitter();

  float  Velocity();
  float  Acceleration();
  float  Scale();
  float  Latitude();
  float  Longitude();
  float  ParticleEmissionRate();
  float  ParticleLifeSpan();
  void   Enabled(int enable);
  void   Enabled2(int enable);
  void   Update(float elapsedTime, const NTempest::C3Vector &cameraWorldPos, const NTempest::C3Vector &cameraVector);
  void   Render();
  void   AddToModelScene();
  void   Flush();
  void   SetVelocity(float vel);
  void   SetAcceleration(float accel);
  void   SetScale(float scale);
  void   SetLatitude(float latInDegrees);
  void   SetLongitude(float longitudeInDegrees);
  void   SetParticleEmissionRate(float particlesPerSec);
  void   SetParticleLifeSpan(float lifeInSec);
  void   SetModel(HMODEL hmodel);
  HMODEL GetModel() {
    return m_hmodel;
  }
  CParticleEmitter *AddRef();
  void              DecRef();
};
