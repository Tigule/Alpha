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
  enum {
    F_BORN = 1
  };

 private:
  friend class CParticleEmitter2;
  friend class CPlaneParticleEmitter;
  friend class CSphereParticleEmitter;
  friend class CSplineParticleEmitter;

  NTempest::C3Vector m_position;
  BYTE               m_keyFrame;
  BYTE               m_flags;
  BYTE               m_filler[2];
  NTempest::C3Vector m_velocity;
  float              m_age;
};

class CParticle2_Model : public CParticle2 {
 private:
  friend class CParticleEmitter2;

  NTempest::C4Quaternion m_rotation;
  NTempest::C3Vector     m_rotVelocity;
};

class CParticleKey {
 private:
  friend class CParticleEmitter2;

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

  void Interpolate(float time, NTempest::CImVector &color, int &headCell, int &tailCell, float &scale);

 public:
  CParticleKey();
  void SetSegment(float normStartTime, float normEndTime);
  void SetLifeSpan(float lifeSpan);
  void SetRepeat(float repeat);
  void SetColors(NTempest::CImVector start, NTempest::CImVector end);
  void SetHeadCells(int start, int end);
  void SetTailCells(int start, int end);
  void SetScales(float start, float end);
  void Segment(float &startTime, float &endTime);
  void LifeSpan(float &lifeSpan);
  void Repeat(float &repeat);
  void Colors(NTempest::CImVector &start, NTempest::CImVector &end);
  void HeadCells(int &start, int &end);
  void TailCells(int &start, int &end);
  void Scales(float &start, float &end);
};

struct CParticleMat {
  CParticleMat() : alpha(GxBlend_Opaque), enableLighting(1), enableFog(1), enableDepthWrites(1) {
  }

  EGxBlend alpha;
  BOOL     enableLighting : 1;
  BOOL     enableFog : 1;
  BOOL     enableDepthWrites : 1;
};

struct CSortableParticleRecord {
  float       dist;
  CParticle2 *p;

  static BYTE HasHigherPriority(const CSortableParticleRecord &a, const CSortableParticleRecord &b) {
    return a.dist >= b.dist;
  }
};

static void AddEmitters2ToScene(CModel *modelptr, CModelShared *shared);

class CParticleEmitter2 {
  friend class ParticleSystemManager;
  friend CParticleEmitter2 *CreateEmitter(BYTE *emitterData, const MDLTEXTURESECTION *textures, UINT flags, CStatus *status);
  friend UINT               SetParticleStyle(const BYTE *emitterData, UINT flags, CParticleEmitter2 *emitter);
  friend BYTE              *SetParticleTumble(BYTE *emitterData, CParticleEmitter2 *emitter);
  friend void               AddEmitters2ToScene(CModel *modelptr, CModelShared *shared);

 public:
  enum PARTICLE_EMITTER_TYPE {
    PET_BASE_EMITTER = 0,
    PET_PLANE_EMITTER = 1,
    PET_SPHERE_EMITTER = 2,
    PET_SPLINE_EMITTER = 3,
    PET_NUMS_PETS = 4
  };

  enum {
    NUM_PARTICLE_KEYS = 2,
    MAX_CHILD_EMITTERS = 4
  };

  enum {
    MAX_RECURSIVE_PARTICLES = 4096,
    RND_TABLE_SIZE = 128,
    RND_TABLE_MASK = 127
  };

  enum PARTICLE_TYPE {
    PT_QUAD = 0,
    PT_MODEL = 1
  };

 protected:
  static const float VEL_UPDATE_TIME;
  static const float MIN_ZSOURCE;

 private:
  UINT  m_refCount;
  float m_numNew;
  UINT  m_textureLog;
  float m_ooTextureWidth;
  float m_ooTextureHeight;
  int   m_priorityPlane;

  void SyncReserve(UINT arraySize, UINT oldSize, UINT oldReserve);
  void SyncAllocation(UINT arraySize);
  BOOL IsEnabled() {
    return m_enabled && m_enabled2;
  }
  void EmitNewParticles(float elapsedTime, const NTempest::C34Matrix &basis);
  void EmitParticle(float elapsedTime, const NTempest::C34Matrix &basis) {
    UINT particle = m_dead.Pop();
    m_alive.Push(particle);

    if (m_particleType == PT_QUAD) {
      m_particles[particle].m_flags = CParticle2::F_BORN;
      CreateParticle(m_particles[particle], elapsedTime, basis);
    } else {
      m_modelParticles[particle].m_flags = CParticle2::F_BORN;
      CreateParticle(m_modelParticles[particle], elapsedTime, basis);
    }
  }
  CParticleEmitter2 &operator=(const CParticleEmitter2 &);

 protected:
  static NTempest::CPriorityQ<CSortableParticleRecord, CSortableParticleRecord> m_pq;
  static float                                                                  m_rndTable[128];
  static void                                                                   BufRenderParticles(CGxBufCommand &cmd, CGxBuf *buf);
  static UINT                                                                   s_vertexNdx;
  static UINT                                                                   s_indexNdx;
  static NTempest::C44Matrix                                                    s_particleToView;
  static NTempest::C3Vector                                                     s_quadVectors[4];
  static UINT                                                                   s_maxParticles;
  static UINT                                                                   s_renderedParticles;
  static UINT                                                                   s_renderedIndices;

  PARTICLE_EMITTER_TYPE                             m_emitterType;
  PARTICLE_TYPE                                     m_particleType;
  NTempest::CRndSeed                                m_randSeed;
  TSGrowableArray<CParticle2>                       m_particles;
  TSGrowableArray<CParticle2_Model>                 m_modelParticles;
  CParticleStack                                    m_alive;
  CParticleStack                                    m_dead;
  TSCArray<CParticleEmitter2 *, MAX_CHILD_EMITTERS> m_childEmitter;
  HMODEL                                            m_model;
  UINT                                              m_verticesPerParticle;
  UINT                                              m_indicesPerParticle;
  float                                             m_elapsedTime;
  float                                             m_particleEmissionRate;
  float                                             m_particleLifeSpan;
  float                                             m_particleTailLength;
  TSCArray<CParticleKey, NUM_PARTICLE_KEYS>         m_particleKeys;
  float                                             m_particleVelocity;
  float                                             m_particleAcceleration;
  float                                             m_particleVelocityVariation;
  float                                             m_particleZsource;
  float                                             m_particleAngularVelocity;
  CParticleMat                                      m_particleMaterial;
  UINT                                              m_textureRows;
  UINT                                              m_textureColumns;
  HTEXTURE                                          m_hTex;
  UINT                                              m_replaceableId;
  DWORD                                             m_enabled : 1;
  DWORD                                             m_enabled2 : 1;
  DWORD                                             m_particleHasHead : 1;
  DWORD                                             m_particleHasTail : 1;
  DWORD                                             m_sortZ : 1;
  DWORD                                             m_needSquirt : 1;
  DWORD                                             m_updated : 1;
  DWORD                                             m_paused : 1;
  DWORD                                             m_useModelSpace : 1;
  DWORD                                             m_inheritScale : 1;
  DWORD                                             m_instantVelLin : 1;
  DWORD                                             m_0XKill : 1;
  DWORD                                             m_extrude : 1;
  DWORD                                             m_xyQuads : 1;
  DWORD                                             m_zvelOnly : 1;
  DWORD                                             m_tumbler : 1;
  DWORD                                             m_tailGrows : 1;
  DWORD                                             m_project : 1;
  DWORD                                             m_follow : 1;
  float                                             m_twinkleFPS;
  float                                             m_twinkleOnOff;
  float                                             m_twinkleScaleMin;
  float                                             m_twinkleScaleMax;
  float                                             m_twinkleScaleRange;
  float                                             m_ivelScale;
  NTempest::C2Vector                                m_tumblex;
  NTempest::C2Vector                                m_tumbley;
  NTempest::C2Vector                                m_tumblez;
  float                                             m_drag;
  NTempest::C3Vector                                m_windVector;
  float                                             m_windTime;
  float                                             m_followB;
  float                                             m_followM;
  NTempest::C34Matrix                               m_modelToWorld;
  NTempest::C3Vector                                m_cameraWorldPos;
  NTempest::C3Vector                                m_prevModelToWorldTrans;
  float                                             m_elapsedVelUpdate;
  NTempest::C3Vector                                m_frameInstantVelLin;
  float                                             m_frameScale;
  float                                             m_followScalar;
  NTempest::C3Vector                                m_followVector;
  NTempest::C3Vector                                m_stepFollowVector;
  NTempest::C3Vector                                m_xyAxis;

  float                     CalcVelocity();
  virtual void              Sync();
  virtual void              CreateParticle(CParticle2 &p, float elapsedTime, const NTempest::C34Matrix &basis);
  virtual void              CreateParticle(CParticle2_Model &p, float elapsedTime, const NTempest::C34Matrix &basis);
  BOOL                      MoveParticle(CParticle2 &p, float elapsedTime);
  BOOL                      MoveParticle(CParticle2_Model &p, float elapsedTime);
  BOOL                      RenderParticle(CParticle2 &p, const NTempest::C34Matrix &basis, UINT headCell, UINT tailCell);
  BOOL                      RenderParticle(CParticle2_Model &p);
  BOOL                      IRenderParticle(CParticle2 &p, CGxVertexPNCT0 *vtx);
  void                      IRenderVertices(const CGxBufCommand &cmd, CGxBuf *buf);
  void                      IRenderIndices(const CGxBufCommand &cmd, CGxBuf *buf);
  void                      ProjectParticle(CParticle2 &p);
  virtual void              DestroyParticle(CParticle2 &p);
  void                      RenderParticles();
  void                      RenderParticleModels();
  CParticle2 *GetParticle(UINT index) {
    return m_particleType == PT_QUAD ? &m_particles[index] : static_cast<CParticle2 *>(&m_modelParticles[index]);
  }

  CParticleEmitter2();
  CParticleEmitter2(const CParticleEmitter2 &rhs, int deep = 0);
  void                       SingletonMgrUpdate(float elapsedTime, const NTempest::C3Vector &cameraWorldPos, int suppressNewParticles);
  void                       InternalUpdate(float elapsedTime, int suppressNewParticles);
  void                       StepUpdate(float elapsedTime, int suppressNewParticles);
  void                       UpdateXform(const NTempest::C34Matrix &modelToWorld, const NTempest::C3Vector &cameraWorldPos);
  virtual CParticleEmitter2 *Clone(int recursive) const = 0;
  void                       SetTumble(NTempest::C2Vector &dst, const NTempest::C2Vector &src) {
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

  void SetEnabled(int enable, int recurse);
  void SetEnabled2(int enable2, int recurse);
  void SetLifeSpan(float lifeSpan);
  void SetVelocity(float velocity);
  void SetAcceleration(float acceleration);
  void SetVelocityVariation(float variation);
  void SetAngularVelocity(float angularVelocity) {
    m_particleAngularVelocity = angularVelocity;
  }
  void SetZsource(float zsource);
  void SetMaterial(const CParticleMat &material, HTEXTURE hTex);
  void MaterialDisableLight(int disable);
  void MaterialDisableFog(int disable);
  void SetTexture(HTEXTURE hTex);
  void SetReplaceableId(UINT id);
  void SetKey(UINT keyNdx, const CParticleKey &key);
  void SetTextureDimensions(UINT rows, UINT columns);
  void SetParticleStyle(BOOL hasHead, BOOL hasTail, float tailLength, bool tailGrows);
  void SetSortZ(int sortZ);
  void SetPriorityPlane(int priorityPlane) {
    m_priorityPlane = priorityPlane;
  }
  void SetUseModelSpace(int useModelSpace) {
    m_useModelSpace = useModelSpace;
  }
  void SetInstantVel(int instantVel) {
    m_instantVelLin = instantVel;
  }
  void SetInstantVelScale(float scale) {
    m_ivelScale = scale;
  }
  void Set0XKill(int kill) {
    m_0XKill = kill;
  }
  void SetInheritScale(int inheritScale) {
    m_inheritScale = inheritScale;
  }
  void SetExtrude(int extrude) {
    m_extrude = extrude;
  }
  void SetXYQuads(int xyQuads) {
    m_xyQuads = xyQuads;
  }
  void SetProject(int project) {
    m_project = project;
  }
  void AddChildEmitter(CParticleEmitter2 *child);
  void SetModel(HMODEL model);
  void SetTwinkleFPS(float fps) {
    m_twinkleFPS = fps;
  }
  void SetTwinkleOnOff(float onOff) {
    m_twinkleOnOff = onOff;
  }
  void SetTwinkleScale(float minScale, float maxScale) {
    m_twinkleScaleMin = minScale;
    m_twinkleScaleMax = maxScale;
    m_twinkleScaleRange = maxScale - minScale;
  }
  void SetZVelOnly(int zVelOnly) {
    m_zvelOnly = zVelOnly;
  }
  void SetTumbleReverse(int reverse) {
    m_tumbler = reverse;
  }
  void SetTumbleX(const NTempest::C2Vector &tumble) {
    SetTumble(m_tumblex, tumble);
  }
  void SetTumbleY(const NTempest::C2Vector &tumble) {
    SetTumble(m_tumbley, tumble);
  }
  void SetTumbleZ(const NTempest::C2Vector &tumble) {
    SetTumble(m_tumblez, tumble);
  }
  void SetDrag(float drag) {
    m_drag = drag;
  }
  void SetWind(const NTempest::C3Vector &wind, float time) {
    m_windVector = wind;
    m_windTime = time;
  }
  void SetFollowParams(float speed1, float scale1, float speed2, float scale2);
  void SetFollow(int follow) {
    m_follow = follow;
  }
  PARTICLE_EMITTER_TYPE EmitterType() const {
    return m_emitterType;
  }
  int   Enabled();
  int   Enabled2();
  float EmissionRate();
  float LifeSpan();
  float Velocity();
  float Acceleration();
  float VelocityVariation();
  float AngularVelocity() const {
    return m_particleAngularVelocity;
  }
  CParticleMat Material() const {
    return m_particleMaterial;
  }
  HTEXTURE Texture() {
    return m_hTex;
  }
  UINT               ReplaceableId();
  CParticleEmitter2 *ChildEmitter(UINT index) {
    return m_childEmitter[index];
  }
  const CParticleKey &Key(UINT keyNdx);
  void                TextureDimensions(UINT &rows, UINT &columns);
  void                ParticleStyle(int &hasHead, int &hasTail, float &tailLength);
  int                 SortZ();
  int                 PriorityPlane() const {
    return m_priorityPlane;
  }
  int UseModelSpace() const {
    return m_useModelSpace;
  }
  void               Update(float elapsedTime, const NTempest::C34Matrix &modelToWorld, const NTempest::C3Vector &cameraWorldPos);
  void               Squirt();
  void               Render();
  void               Flush();
  CParticleEmitter2 *AddRef();
  void               DecRef();
};

class CPlaneParticleEmitter : public CParticleEmitter2 {
  friend class ParticleSystemManager;

 private:
  void operator=(const CPlaneParticleEmitter &);

 protected:
  float m_width;
  float m_height;
  float m_latitude;
  float m_longitude;

  virtual void CreateParticle(CParticle2 &p, float elapsedTime, const NTempest::C34Matrix &basis);
  CPlaneParticleEmitter();
  CPlaneParticleEmitter(const CPlaneParticleEmitter &rhs, int deep = 0);
  virtual CParticleEmitter2 *Clone(int deep) const {
    return NEW(CPlaneParticleEmitter)(*this, deep);
  }

 public:
  virtual ~CPlaneParticleEmitter();

  float        Width();
  float        Height();
  float        Latitude();
  float        Longitude();
  virtual void SetWidth(float width);
  virtual void SetHeight(float height);
  virtual void SetLatitude(float latInRadians);
  virtual void SetLongitude(float longInRadians);
};

class CSphereParticleEmitter : public CParticleEmitter2 {
  friend class ParticleSystemManager;

 private:
  void operator=(const CSphereParticleEmitter &);

 protected:
  float m_innerRadius;
  float m_outerRadius;
  float m_radiusRange;
  float m_latitude;
  float m_longitude;

  virtual void CreateParticle(CParticle2 &p, float elapsedTime, const NTempest::C34Matrix &basis);
  CSphereParticleEmitter();
  CSphereParticleEmitter(const CSphereParticleEmitter &rhs, int deep = 0);
  virtual CParticleEmitter2 *Clone(int deep) const {
    return NEW(CSphereParticleEmitter)(*this, deep);
  }

 public:
  virtual ~CSphereParticleEmitter();

  float        InnerRadius();
  float        OuterRadius();
  float        Latitude();
  float        Longitude();
  virtual void SetWidth(float radius);
  virtual void SetHeight(float radius);
  virtual void SetLatitude(float latInRadians);
  virtual void SetLongitude(float longInRadians);
};

class CSplineParticleEmitter : public CParticleEmitter2 {
  friend class ParticleSystemManager;

 private:
  void operator=(const CSplineParticleEmitter &);

 protected:
  float                      m_requestedEmissionRate;
  float                      m_start;
  float                      m_end;
  float                      m_latitude;
  float                      m_radius;
  BOOL                       m_emitAtEnd;
  NTempest::C3Spline_Bezier3 m_spline;

  virtual void CreateParticle(CParticle2 &p, float elapsedTime, const NTempest::C34Matrix &basis);
  CSplineParticleEmitter();
  CSplineParticleEmitter(const CSplineParticleEmitter &rhs, int deep = 0);
  virtual CParticleEmitter2 *Clone(int deep) const {
    return NEW(CSplineParticleEmitter)(*this, deep);
  }
  void SetActualEmissionRate() {
    m_particleEmissionRate = m_end * m_requestedEmissionRate;
  }

 public:
  virtual ~CSplineParticleEmitter();

  float        Start();
  float        End();
  float        Latitude();
  float        Radius();
  virtual void SetWidth(float start);
  virtual void SetHeight(float end);
  virtual void SetLatitude(float latInRadians);
  virtual void SetLongitude(float radius);
  virtual void SetEmissionRate(float particlesPerSecond);
  void         SetSpline(const NTempest::C3Vector *points, UINT numPoints);
};
