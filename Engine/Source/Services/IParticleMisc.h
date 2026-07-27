#pragma once

#include <stpl.h>

class CParticleEmitter;
class CParticleEmitter2;
class CPlaneParticleEmitter;
class CSphereParticleEmitter;
class CSplineParticleEmitter;
class CRibbonEmitter;
class CWorld;

namespace NTempest {
  class C3Segment;
}

typedef int(*PARTICLEPROJECTCALLBACK)(const NTempest::C3Segment &segment, float &distance);

class CParticleStack {
 public:
  CParticleStack() : m_stackPointer(0) {
  }

  CParticleStack(const CParticleStack &source)
      : m_stack(source.m_stack), m_stackPointer(source.m_stackPointer) {
  }

  void Push(unsigned int u) {
    ASSERT(m_stackPointer < m_stack.Count());
    m_stack[m_stackPointer++] = u;
  }

  unsigned int Pop() {
    ASSERT(m_stackPointer != 0);
    return m_stack[--m_stackPointer];
  }

  unsigned int Top() {
    ASSERT(m_stackPointer != 0);
    return m_stack[m_stackPointer - 1];
  }

  void Remove(unsigned int index) {
    m_stack[index] = m_stack[m_stackPointer - 1];
    Pop();
  }

  int IsEmpty() {
    return m_stackPointer == 0;
  }

  void Clear() {
    m_stackPointer = 0;
  }

  unsigned int Count() const {
    return m_stackPointer;
  }

  void SetCount(unsigned int count) {
    m_stack.SetCount(count);
  }

  void ReserveSpace(unsigned int count) {
    m_stack.ReserveSpace(count);
  }

  unsigned int operator[](unsigned int index) {
    return m_stack[index];
  }

  unsigned int operator[](unsigned int index) const {
    return m_stack[index];
  }

 private:
  TSGrowableArray<unsigned int> m_stack;
  unsigned int                  m_stackPointer;
};

class ParticleSystemManager {
 public:
  typedef PARTICLEPROJECTCALLBACK ProjectCallback;

  ~ParticleSystemManager();

  static void Destroy();
  static void SetScaler(float scaler);
  static float GetScaler();
  static ParticleSystemManager *GetInstance();

  CPlaneParticleEmitter  *CreateQuadEmitter();
  CSphereParticleEmitter *CreateSphereEmitter();
  CSplineParticleEmitter *CreateSplineEmitter();
  CParticleEmitter       *CreateModelEmitter();
  CParticleEmitter2      *DuplicateEmitter(const CParticleEmitter2 *emitter, int deep);
  void                    UpdateEmitters(float elapsedTime, const NTempest::C3Vector &cameraPos, const NTempest::C3Vector &cameraTarg);
  void                    DeleteModelEmitter(CParticleEmitter *emitter);
  void                    DeleteEmitter2(CParticleEmitter2 *emitter);
  void                    RenderEmitters();

  static void SetProjectCallback(PARTICLEPROJECTCALLBACK callback, float distance) {
    sm_projectCallback = callback;
    sm_projectDistance = distance;
  }

  static PARTICLEPROJECTCALLBACK GetProjectCallback() {
    return sm_projectCallback;
  }

  static float GetProjectDistance() {
    return sm_projectDistance;
  }

  void Flush();

 private:
  friend class CWorld;

  static void RenderParticleEmitter(void *param1, int param2);
  static void RenderParticleEmitter2(void *param1, int param2);

  static ParticleSystemManager *manager;
  static float                  scaler;
  static float                  sm_projectDistance;
  static int(*sm_projectCallback)(const NTempest::C3Segment &segment, float &distance);

  TSGrowableArray<CParticleEmitter *>  modelEmitters;
  TSGrowableArray<CParticleEmitter2 *> emitter2s;
  TSGrowableArray<CParticleEmitter *>  deletedModelEmitters;
  TSGrowableArray<CParticleEmitter2 *> deletedEmitter2s;
};

class RibbonManager {
 public:
  ~RibbonManager();

  static void Destroy();
  static RibbonManager *GetInstance();
  CRibbonEmitter                  *CreateEmitter();
  CRibbonEmitter                  *DuplicateEmitter(const CRibbonEmitter *emitter);
  void                             UpdateEmitters(float elapsedTime, const NTempest::C3Vector &cameraPos, const NTempest::C3Vector &cameraTarg);
  void                             DeleteEmitter(CRibbonEmitter *emitter);
  void                             Flush();
  void                             RenderEmitters();

 private:
  static void RenderEmitter(void *param1, int param2);

  static RibbonManager *manager;

  TSGrowableArray<CRibbonEmitter *> emitters;
  TSGrowableArray<CRibbonEmitter *> deletedEmitters;
};
