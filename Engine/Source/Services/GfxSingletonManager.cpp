#include "RibbonEmitter.h"
#include "IParticleMisc.h"
#include "ParticleSystem.h"
#include "ParticleSystem2.h"

ParticleSystemManager *ParticleSystemManager::manager = 0;
float                  ParticleSystemManager::scaler = 1.0f;
float                  ParticleSystemManager::sm_projectDistance;
int(__fastcall *ParticleSystemManager::sm_projectCallback)(const NTempest::C3Segment &, float &);
RibbonManager *RibbonManager::manager = 0;

void __fastcall ParticleSystemManager::SetScaler(float scaler) {
  if (scaler >= 0.0f) {
    if (scaler <= 1.0f) {
      ParticleSystemManager::scaler = scaler;
    } else {
      ParticleSystemManager::scaler = 1.0f;
    }
  } else {
    ParticleSystemManager::scaler = 0.0f;
  }
}

float __fastcall ParticleSystemManager::GetScaler() {
  return scaler;
}

ParticleSystemManager *__fastcall ParticleSystemManager::GetInstance() {
  if (!manager) {
    manager = NEW(ParticleSystemManager);

    NTempest::CRndSeed randSeed;
    randSeed.SetSeed((rand() << 16) | rand());
    for (unsigned int i = 0; i < 128; ++i) {
      CParticleEmitter2::m_rndTable[i] = NTempest::CRandom::real_(randSeed);
    }

    ASSERT(manager);
  }
  return manager;
}

void __fastcall ParticleSystemManager::Destroy() {
  DEL(manager);
  manager = 0;
}

ParticleSystemManager::~ParticleSystemManager() {
  Flush();
}

void ParticleSystemManager::Flush() {
  unsigned int index;

  index = modelEmitters.Count();
  while (index) {
    modelEmitters[--index]->DecRef();
  }
  modelEmitters.SetCount(0);

  index = emitter2s.Count();
  while (index) {
    emitter2s[--index]->DecRef();
  }
  emitter2s.SetCount(0);

  index = deletedModelEmitters.Count();
  while (index) {
    deletedModelEmitters[--index]->DecRef();
  }
  deletedModelEmitters.SetCount(0);

  index = deletedEmitter2s.Count();
  while (index) {
    deletedEmitter2s[--index]->DecRef();
  }
  deletedEmitter2s.SetCount(0);
}

CPlaneParticleEmitter *ParticleSystemManager::CreateQuadEmitter() {
  CPlaneParticleEmitter *res = NEW(CPlaneParticleEmitter);
  res->AddRef();
  emitter2s.SetCount(emitter2s.Count() + 1);
  emitter2s[emitter2s.Count() - 1] = res;
  return res;
}

CSphereParticleEmitter *ParticleSystemManager::CreateSphereEmitter() {
  CSphereParticleEmitter *res = NEW(CSphereParticleEmitter);
  res->AddRef();
  emitter2s.SetCount(emitter2s.Count() + 1);
  emitter2s[emitter2s.Count() - 1] = res;
  return res;
}

CSplineParticleEmitter *ParticleSystemManager::CreateSplineEmitter() {
  CSplineParticleEmitter *res = NEW(CSplineParticleEmitter);
  res->AddRef();
  emitter2s.SetCount(emitter2s.Count() + 1);
  emitter2s[emitter2s.Count() - 1] = res;
  return res;
}

CParticleEmitter2 *ParticleSystemManager::DuplicateEmitter(const CParticleEmitter2 *emitter, int deep) {
  ASSERT(emitter);
  CParticleEmitter2 *newEmitter = emitter->Clone(deep);
  newEmitter->AddRef();
  emitter2s.SetCount(emitter2s.Count() + 1);
  emitter2s[emitter2s.Count() - 1] = newEmitter;
  return newEmitter;
}

void ParticleSystemManager::UpdateEmitters(float elapsedTime, const NTempest::C3Vector &cameraPos, const NTempest::C3Vector &cameraTarg) {
  unsigned int index = modelEmitters.Count();
  while (index) {
    modelEmitters[--index]->Update(elapsedTime, cameraPos, cameraTarg);
  }

  index = deletedModelEmitters.Count();
  while (index) {
    CParticleEmitter *temp = deletedModelEmitters[--index];
    temp->Update(elapsedTime, cameraPos, cameraTarg);
    if (!temp->m_alive.m_stackPointer) {
      deletedModelEmitters[index] = deletedModelEmitters[deletedModelEmitters.Count() - 1];
      deletedModelEmitters.SetCount(deletedModelEmitters.Count() - 1);
      temp->DecRef();
    }
  }

  index = emitter2s.Count();
  while (index) {
    emitter2s[--index]->SingletonMgrUpdate(elapsedTime, cameraPos, 0);
  }

  index = deletedEmitter2s.Count();
  while (index) {
    CParticleEmitter2 *temp = deletedEmitter2s[--index];
    temp->SingletonMgrUpdate(elapsedTime, cameraPos, 1);
    if (!temp->m_alive.m_stackPointer) {
      deletedEmitter2s[index] = deletedEmitter2s[deletedEmitter2s.Count() - 1];
      deletedEmitter2s.SetCount(deletedEmitter2s.Count() - 1);
      temp->DecRef();
    }
  }
}

void __fastcall ParticleSystemManager::RenderParticleEmitter2(void *param1, int param2) {
  static_cast<CParticleEmitter2 *>(param1)->Render();
}

void ParticleSystemManager::RenderEmitters() {
  unsigned int index = deletedEmitter2s.Count();
  while (index) {
    CParticleEmitter2 *emitter = deletedEmitter2s[--index];
    ModelAddToScene(*reinterpret_cast<NTempest::C3Vector *>(&emitter->m_modelToWorld.d0), 0, RenderParticleEmitter2, emitter, 0);
  }
}

void ParticleSystemManager::DeleteEmitter2(CParticleEmitter2 *emitter) {
  ASSERT(emitter);
  emitter->DecRef();
  unsigned int index = emitter2s.Count();
  while (index) {
    --index;
    if (emitter2s[index] == emitter) {
      deletedEmitter2s.SetCount(deletedEmitter2s.Count() + 1);
      deletedEmitter2s[deletedEmitter2s.Count() - 1] = emitter2s[index];
      emitter2s[index] = emitter2s[emitter2s.Count() - 1];
      emitter2s.SetCount(emitter2s.Count() - 1);
    }
  }
}

RibbonManager *__fastcall RibbonManager::GetInstance() {
  if (!manager) {
    manager = NEW(RibbonManager);
  }
  return manager;
}

void __fastcall RibbonManager::Destroy() {
  DEL(manager);
  manager = 0;
}

RibbonManager::~RibbonManager() {
  Flush();
}

void RibbonManager::Flush() {
  unsigned int index;

  index = emitters.Count();
  while (index) {
    emitters[--index]->DecRef();
  }
  emitters.SetCount(0);

  index = deletedEmitters.Count();
  while (index) {
    deletedEmitters[--index]->DecRef();
  }
  deletedEmitters.SetCount(0);
}

CRibbonEmitter *RibbonManager::CreateEmitter() {
  CRibbonEmitter *res = (NEW(CRibbonEmitter))->AddRef();
  emitters.SetCount(emitters.Count() + 1);
  emitters[emitters.Count() - 1] = res;
  return res;
}

CRibbonEmitter *RibbonManager::DuplicateEmitter(const CRibbonEmitter *emitter) {
  ASSERT(emitter);
  CRibbonEmitter *newEmitter = (NEW(CRibbonEmitter)(*emitter))->AddRef();
  emitters.SetCount(emitters.Count() + 1);
  emitters[emitters.Count() - 1] = newEmitter;
  return newEmitter;
}

void RibbonManager::UpdateEmitters(float elapsedTime, const NTempest::C3Vector &cameraPos, const NTempest::C3Vector &cameraTarg) {
  unsigned int index = emitters.Count();
  while (index) {
    emitters[--index]->SingletonMgrUpdate(elapsedTime, cameraPos, 1);
  }

  index = deletedEmitters.Count();
  while (index) {
    CRibbonEmitter *temp = deletedEmitters[--index];
    temp->SingletonMgrUpdate(elapsedTime, cameraPos, 1);
    if (temp->IsDead()) {
      deletedEmitters[index] = deletedEmitters[deletedEmitters.Count() - 1];
      deletedEmitters.SetCount(deletedEmitters.Count() - 1);
      temp->DecRef();
    }
  }
}

void __fastcall RibbonManager::RenderEmitter(void *param1, int param2) {
  static_cast<CRibbonEmitter *>(param1)->Render();
}

void RibbonManager::RenderEmitters() {
  unsigned int index = deletedEmitters.Count();
  while (index) {
    CRibbonEmitter *emitter = deletedEmitters[--index];
    ModelAddToScene(emitter->m_currPos, 0, RenderEmitter, emitter, 0);
  }
}

void RibbonManager::DeleteEmitter(CRibbonEmitter *emitter) {
  ASSERT(emitter);
  emitter->DecRef();

  unsigned int index = emitters.Count();
  while (index) {
    --index;
    if (emitters[index] == emitter) {
      deletedEmitters.SetCount(deletedEmitters.Count() + 1);
      deletedEmitters[deletedEmitters.Count() - 1] = emitters[index];
      emitters[index] = emitters[emitters.Count() - 1];
      emitters.SetCount(emitters.Count() - 1);
      return;
    }
  }
}
