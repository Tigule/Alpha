#pragma once

#include "Model/IModel.h"
#include "Services/IParticleMisc.h"
#include "Services/Texture.h"
#include "Tempest/c2vector.h"
#include "Tempest/c3vector.h"
#include "Tempest/c3spline.h"
#include "Tempest/c34matrix.h"
#include "Tempest/c44matrix.h"
#include "Tempest/c4quaternion.h"
#include "Tempest/cimvector.h"
#include "Tempest/cpriorityq.h"
#include "Tempest/crandom.h"

#include <stpl.h>

class CModel;
class CStatus;
struct CModelShared;
struct MDLTEXTURESECTION;
struct CGxBuf;
struct CGxBufCommand;
struct CGxVertexPNCT0;

class CParticle2 {
 public:
  NTempest::C3Vector m_position;
  unsigned char      m_keyFrame;
  unsigned char      m_flags;
  unsigned char      m_filler[2];
  NTempest::C3Vector m_velocity;
  float              m_age;
};

class CParticle2_Model : public CParticle2 {
 public:
  NTempest::C4Quaternion m_rotation;
  NTempest::C3Vector     m_rotVelocity;
};

class CParticleKey {
 public:
  CParticleKey();
  void SetSegment(float normStartTime, float normEndTime);
  void SetLifeSpan(float lifeSpan);
  void SetRepeat(float repeat);
  void SetHeadCells(int start, int end);
  void SetTailCells(int start, int end);
  void SetScales(float start, float end);
  void SetColors(NTempest::CImVector start, NTempest::CImVector end);
  void Segment(float &startTime, float &endTime);
  void Repeat(float &repeat);
  void LifeSpan(float &lifeSpan);
  void Colors(NTempest::CImVector &start, NTempest::CImVector &end);
  void HeadCells(int &start, int &end);
  void TailCells(int &start, int &end);
  void Scales(float &start, float &end);

  NTempest::CImVector m_startColor;
  int                 m_deltaColor[4];
  int                 m_initialHead;
  int                 m_deltaHead;
  int                 m_initialTail;
  int                 m_deltaTail;
  float               m_startScale;
  float               m_deltaScale;
  float               m_startTime;
  float               m_ooSegLength;
  float               m_endTime;
  NTempest::CImVector m_endColor;
  int                 m_headStart;
  int                 m_headEnd;
  int                 m_tailStart;
  int                 m_tailEnd;
  float               m_endScale;
  float               m_repeat;
  float               m_normStartTime;
  float               m_normEndTime;
  float               m_lifeSpan;

 private:
  friend class CParticleEmitter2;

  void Interpolate(float time, NTempest::CImVector &color, int &headCell, int &tailCell, float &scale);
};

struct CParticleMat {
  EGxBlend alpha;
  int      enableLighting : 1;
  int      enableFog : 1;
  int      enableDepthWrites : 1;
};

struct CSortableParticleRecord {
  static unsigned char HasHigherPriority(const CSortableParticleRecord &a, const CSortableParticleRecord &b) {
    return a.dist >= b.dist;
  }

  float       dist;
  CParticle2 *p;
};

static void __fastcall AddEmitters2ToScene(CModel *modelptr, CModelShared *shared);

class CParticleEmitter2 {
  friend class ParticleSystemManager;
  friend CParticleEmitter2 *__fastcall
                                   CreateEmitter(unsigned char *emitterData, const MDLTEXTURESECTION *textures, unsigned int flags, CStatus *status);
  friend unsigned int __fastcall   SetParticleStyle(const unsigned char *emitterData, unsigned int flags, CParticleEmitter2 *emitter);
  friend unsigned char *__fastcall SetParticleTumble(unsigned char *emitterData, CParticleEmitter2 *emitter);

 public:
  enum PARTICLE_EMITTER_TYPE {
    PET_BASE_EMITTER = 0,
    PET_PLANE_EMITTER = 1,
    PET_SPHERE_EMITTER = 2,
    PET_SPLINE_EMITTER = 3,
    PET_NUMS_PETS = 4
  };

  enum PARTICLE_TYPE {
    PT_QUAD = 0,
    PT_MODEL = 1
  };

 protected:
  CParticleEmitter2();
  CParticleEmitter2(const CParticleEmitter2 &rhs, int deep);
  static NTempest::CPriorityQ<CSortableParticleRecord, CSortableParticleRecord> m_pq;
  static const float                                                            VEL_UPDATE_TIME;
  static float                                                                  m_rndTable[128];
  static unsigned int                                                           s_renderedParticles;
  static unsigned int                                                           s_renderedIndices;
  static unsigned int                                                           s_maxParticles;
  static NTempest::C3Vector                                                     s_quadVectors[4];
  static NTempest::C44Matrix                                                    s_particleToView;

  virtual void               Sync();
  virtual void               CreateParticle(CParticle2_Model &p, float elapsedTime, const NTempest::C34Matrix &basis);
  virtual void               CreateParticle(CParticle2 &p, float elapsedTime, const NTempest::C34Matrix &basis);
  virtual void               DestroyParticle(CParticle2 &p);
  virtual CParticleEmitter2 *Clone(int recursive) const = 0;

  float                  CalcVelocity();
  void                   ProjectParticle(CParticle2 &p);
  int                    MoveParticle(CParticle2 &p, float elapsedTime);
  int                    MoveParticle(CParticle2_Model &p, float elapsedTime);
  void                   UpdateXform(const NTempest::C34Matrix &modelToWorld, const NTempest::C3Vector &cameraWorldPos);
  void                   InternalUpdate(float elapsedTime, int suppressNewParticles);
  void                   StepUpdate(float elapsedTime, int suppressNewParticles);
  void                   SingletonMgrUpdate(float elapsedTime, const NTempest::C3Vector &cameraWorldPos, int suppressNewParticles);
  int                    IRenderParticle(CParticle2 &p, CGxVertexPNCT0 *vtx);
  void                   IRenderVertices(const CGxBufCommand &cmd, CGxBuf *buf);
  void                   IRenderIndices(const CGxBufCommand &cmd, CGxBuf *buf);
  static void __fastcall BufRenderParticles(CGxBufCommand &cmd, CGxBuf *buf);
  void                   RenderParticles();
  int                    RenderParticle(CParticle2_Model &p);
  int                    RenderParticle(CParticle2 &p, const NTempest::C34Matrix &basis, unsigned int headCell, unsigned int tailCell);
  void                   RenderParticleModels();
  CParticle2            *GetParticle(unsigned int index) {
    return m_particleType == PT_MODEL ? static_cast<CParticle2 *>(&m_modelParticles[index]) : &m_particles[index];
  }
  void                   SetTumble(NTempest::C2Vector &dst, const NTempest::C2Vector &src) {
    dst.x = src.x;
    dst.y = src.y - src.x;
  }

 public:
  virtual ~CParticleEmitter2();
  virtual void SetWidth(float width) = 0;
  virtual void SetHeight(float height) = 0;
  virtual void SetLatitude(float latitude) = 0;
  virtual void SetLongitude(float longitude) = 0;
  virtual void SetEmissionRate(float particlesPerSecond);

  CParticleEmitter2  *AddRef();
  void                DecRef();
  void                SetEnabled(int enable, int recurse);
  void                SetEnabled2(int enable2, int recurse);
  void                SetLifeSpan(float lifeSpan);
  void                SetVelocity(float velocity);
  void                SetAcceleration(float acceleration);
  void                SetVelocityVariation(float variation);
  void                SetAngularVelocity(float angularVelocity) {
    m_particleAngularVelocity = angularVelocity;
  }
  void                SetZsource(float zsource);
  void                SetMaterial(const CParticleMat &material, HTEXTURE hTex);
  void                MaterialDisableLight(int disable);
  void                MaterialDisableFog(int disable);
  void                SetTexture(HTEXTURE hTex);
  void                SetReplaceableId(unsigned int id);
  unsigned int        ReplaceableId();
  int                 Enabled();
  int                 Enabled2();
  float               EmissionRate();
  float               LifeSpan();
  float               Velocity();
  float               Acceleration();
  float               VelocityVariation();
  float               AngularVelocity() {
    return m_particleAngularVelocity;
  }
  CParticleMat        Material() {
    return m_particleMaterial;
  }
  HTEXTURE            Texture() {
    return m_hTex;
  }
  CParticleEmitter2  *ChildEmitter(unsigned int index) {
    return m_childEmitter[index];
  }
  const CParticleKey &Key(unsigned int keyNdx);
  void                TextureDimensions(unsigned int &rows, unsigned int &columns);
  void                ParticleStyle(int &hasHead, int &hasTail, float &tailLength);
  void                SetKey(unsigned int keyNdx, const CParticleKey &key);
  void                SetTextureDimensions(unsigned int rows, unsigned int columns);
  void                SetParticleStyle(int hasHead, int hasTail, float tailLength, bool tailGrows);
  void                SetSortZ(int sortZ);
  void                SetPriorityPlane(int priorityPlane) {
    m_priorityPlane = priorityPlane;
  }
  void                SetUseModelSpace(int useModelSpace) {
    m_useModelSpace = useModelSpace;
  }
  void                SetInstantVel(int instantVel) {
    m_instantVelLin = instantVel;
  }
  void                SetInstantVelScale(float scale) {
    m_ivelScale = scale;
  }
  void                Set0XKill(int kill) {
    m_0XKill = kill;
  }
  void                SetInheritScale(int inheritScale) {
    m_inheritScale = inheritScale;
  }
  void                SetExtrude(int extrude) {
    m_extrude = extrude;
  }
  void                SetXYQuads(int xyQuads) {
    m_xyQuads = xyQuads;
  }
  void                SetProject(int project) {
    m_project = project;
  }
  void                AddChildEmitter(CParticleEmitter2 *child);
  void                SetModel(HMODEL model);
  void                SetTwinkleFPS(float fps) {
    m_twinkleFPS = fps;
  }
  void                SetTwinkleOnOff(float onOff) {
    m_twinkleOnOff = onOff;
  }
  void                SetTwinkleScale(float minScale, float maxScale) {
    m_twinkleScaleMin = minScale;
    m_twinkleScaleMax = maxScale;
    m_twinkleScaleRange = maxScale - minScale;
  }
  void                SetZVelOnly(int zVelOnly) {
    m_zvelOnly = zVelOnly;
  }
  void                SetTumbleReverse(int reverse) {
    m_tumbler = reverse;
  }
  void                SetTumbleX(const NTempest::C2Vector &tumble) {
    SetTumble(m_tumblex, tumble);
  }
  void                SetTumbleY(const NTempest::C2Vector &tumble) {
    SetTumble(m_tumbley, tumble);
  }
  void                SetTumbleZ(const NTempest::C2Vector &tumble) {
    SetTumble(m_tumblez, tumble);
  }
  void                SetDrag(float drag) {
    m_drag = drag;
  }
  void                SetWind(const NTempest::C3Vector &wind, float time) {
    m_windVector = wind;
    m_windTime = time;
  }
  void                SetFollowParams(float speed1, float scale1, float speed2, float scale2);
  void                SetFollow(int follow) {
    m_follow = follow;
  }
  PARTICLE_EMITTER_TYPE EmitterType() {
    return m_emitterType;
  }
  void                Squirt();
  void                Flush();
  int                 SortZ();
  int                 PriorityPlane() {
    return m_priorityPlane;
  }
  int                 UseModelSpace() {
    return m_useModelSpace;
  }
  void                Render();
  void                Update(float elapsedTime, const NTempest::C34Matrix &modelToWorld, const NTempest::C3Vector &cameraWorldPos);

 private:
  friend void __fastcall AddEmitters2ToScene(CModel *modelptr, CModelShared *shared);

  void SyncReserve(unsigned int arraySize, unsigned int oldSize, unsigned int oldReserve);
  void SyncAllocation(unsigned int arraySize);
  int  IsEnabled() {
    return m_enabled && m_enabled2;
  }
  void EmitNewParticles(float elapsedTime, const NTempest::C34Matrix &basis);
  void EmitParticle(float elapsedTime, const NTempest::C34Matrix &basis) {
    unsigned int particle = m_dead.Pop();
    m_alive.Push(particle);

    CParticle2 *p = GetParticle(particle);
    p->m_flags = 1;
    if (m_particleType == PT_MODEL) {
      CreateParticle(*static_cast<CParticle2_Model *>(p), elapsedTime, basis);
    } else {
      CreateParticle(*p, elapsedTime, basis);
    }
  }

  unsigned int m_refCount;
  float        m_numNew;
  unsigned int m_textureLog;
  float        m_ooTextureWidth;
  float        m_ooTextureHeight;
  int          m_priorityPlane;

 protected:
  PARTICLE_EMITTER_TYPE             m_emitterType;
  PARTICLE_TYPE                     m_particleType;
  NTempest::CRndSeed                m_randSeed;
  TSGrowableArray<CParticle2>       m_particles;
  TSGrowableArray<CParticle2_Model> m_modelParticles;
  CParticleStack                    m_alive;
  CParticleStack                    m_dead;
  TSCArray<CParticleEmitter2 *, 4>  m_childEmitter;
  HMODEL                            m_model;
  unsigned int                      m_verticesPerParticle;
  unsigned int                      m_indicesPerParticle;
  float                             m_elapsedTime;
  float                             m_particleEmissionRate;
  float                             m_particleLifeSpan;
  float                             m_particleTailLength;
  TSCArray<CParticleKey, 2>         m_particleKeys;
  float                             m_particleVelocity;
  float                             m_particleAcceleration;
  float                             m_particleVelocityVariation;
  float                             m_particleZsource;
  float                             m_particleAngularVelocity;
  CParticleMat                      m_particleMaterial;
  unsigned int                      m_textureRows;
  unsigned int                      m_textureColumns;
  HTEXTURE                          m_hTex;
  unsigned int                      m_replaceableId;
  unsigned long                     m_enabled : 1;
  unsigned long                     m_enabled2 : 1;
  unsigned long                     m_particleHasHead : 1;
  unsigned long                     m_particleHasTail : 1;
  unsigned long                     m_sortZ : 1;
  unsigned long                     m_needSquirt : 1;
  unsigned long                     m_updated : 1;
  unsigned long                     m_paused : 1;
  unsigned long                     m_useModelSpace : 1;
  unsigned long                     m_inheritScale : 1;
  unsigned long                     m_instantVelLin : 1;
  unsigned long                     m_0XKill : 1;
  unsigned long                     m_extrude : 1;
  unsigned long                     m_xyQuads : 1;
  unsigned long                     m_zvelOnly : 1;
  unsigned long                     m_tumbler : 1;
  unsigned long                     m_tailGrows : 1;
  unsigned long                     m_project : 1;
  unsigned long                     m_follow : 1;
  float                             m_twinkleFPS;
  float                             m_twinkleOnOff;
  float                             m_twinkleScaleMin;
  float                             m_twinkleScaleMax;
  float                             m_twinkleScaleRange;
  float                             m_ivelScale;
  NTempest::C2Vector                m_tumblex;
  NTempest::C2Vector                m_tumbley;
  NTempest::C2Vector                m_tumblez;
  float                             m_drag;
  NTempest::C3Vector                m_windVector;
  float                             m_windTime;
  float                             m_followB;
  float                             m_followM;
  NTempest::C34Matrix               m_modelToWorld;
  NTempest::C3Vector                m_cameraWorldPos;
  NTempest::C3Vector                m_prevModelToWorldTrans;
  float                             m_elapsedVelUpdate;
  NTempest::C3Vector                m_frameInstantVelLin;
  float                             m_frameScale;
  float                             m_followScalar;
  NTempest::C3Vector                m_followVector;
  NTempest::C3Vector                m_stepFollowVector;
  NTempest::C3Vector                m_xyAxis;
};

class CPlaneParticleEmitter : public CParticleEmitter2 {
  friend class ParticleSystemManager;

 protected:
  CPlaneParticleEmitter();
  CPlaneParticleEmitter(const CPlaneParticleEmitter &rhs, int deep);
  virtual CParticleEmitter2 *Clone(int deep) const {
    return NEW(CPlaneParticleEmitter)(*this, deep);
  }
  virtual void CreateParticle(CParticle2 &p, float elapsedTime, const NTempest::C34Matrix &basis);

 public:
  virtual ~CPlaneParticleEmitter();
  virtual void SetWidth(float width);
  virtual void SetHeight(float height);
  virtual void SetLatitude(float latInRadians);
  virtual void SetLongitude(float longInRadians);

  float Width();
  float Height();
  float Latitude();
  float Longitude();

 private:
  float m_width;
  float m_height;
  float m_latitude;
  float m_longitude;
};

class CSphereParticleEmitter : public CParticleEmitter2 {
  friend class ParticleSystemManager;

 protected:
  CSphereParticleEmitter();
  CSphereParticleEmitter(const CSphereParticleEmitter &rhs, int deep);
  virtual CParticleEmitter2 *Clone(int deep) const {
    return NEW(CSphereParticleEmitter)(*this, deep);
  }
  virtual void CreateParticle(CParticle2 &p, float elapsedTime, const NTempest::C34Matrix &basis);

 public:
  virtual ~CSphereParticleEmitter();
  virtual void SetWidth(float radius);
  virtual void SetHeight(float radius);
  virtual void SetLatitude(float latInRadians);
  virtual void SetLongitude(float longInRadians);

  float InnerRadius();
  float OuterRadius();
  float Latitude();
  float Longitude();

 private:
  float m_innerRadius;
  float m_outerRadius;
  float m_radiusRange;
  float m_latitude;
  float m_longitude;
};

class CSplineParticleEmitter : public CParticleEmitter2 {
  friend class ParticleSystemManager;

 protected:
  CSplineParticleEmitter();
  CSplineParticleEmitter(const CSplineParticleEmitter &rhs, int deep);
  virtual CParticleEmitter2 *Clone(int deep) const {
    return NEW(CSplineParticleEmitter)(*this, deep);
  }
  virtual void CreateParticle(CParticle2 &p, float elapsedTime, const NTempest::C34Matrix &basis);
  void         SetActualEmissionRate() {
    m_particleEmissionRate = m_end * m_requestedEmissionRate;
  }

 public:
  virtual ~CSplineParticleEmitter();
  virtual void SetWidth(float start);
  virtual void SetHeight(float end);
  virtual void SetLatitude(float latInRadians);
  virtual void SetLongitude(float radius);
  virtual void SetEmissionRate(float particlesPerSecond);

  void  SetSpline(const NTempest::C3Vector *points, unsigned int numPoints);
  float Start();
  float End();
  float Latitude();
  float Radius();

 private:
  float                      m_requestedEmissionRate;
  float                      m_start;
  float                      m_end;
  float                      m_latitude;
  float                      m_radius;
  int                        m_emitAtEnd;
  NTempest::C3Spline_Bezier3 m_spline;
};
