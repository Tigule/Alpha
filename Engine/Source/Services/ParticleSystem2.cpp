#include <Base/Base.h>
#include <Gx/Gx.h>
#include <BLPFile/blp.h>

#include "ParticleSystem2.h"

#include "Base/Activity.h"
#include "Tempest/c33matrix.h"
#include "Tempest/c3segment.h"
#include "Tempest/c4vector.h"

#include <math.h>
#include <stdlib.h>

static float tc[4][2] = {
    {0.0f, 0.0f},
    {0.0f, 1.0f},
    {1.0f, 0.0f},
    {1.0f, 1.0f}
};
static float vc[4][2] = {
    {-1.0f,  1.0f},
    {-1.0f, -1.0f},
    { 1.0f,  1.0f},
    { 1.0f, -1.0f}
};
static const float s_maxTimeStep = 0.1f;

static NTempest::C3Vector s_particleNormal(0.0f, 0.0f, 1.0f);

NTempest::CPriorityQ<CSortableParticleRecord, CSortableParticleRecord> CParticleEmitter2::m_pq;
const float                                                            CParticleEmitter2::VEL_UPDATE_TIME = 1.0f / 30.0f;
const float                                                            CParticleEmitter2::MIN_ZSOURCE = 0.001f;
float                                                                  CParticleEmitter2::m_rndTable[128];
UINT                                                                   CParticleEmitter2::s_vertexNdx;
UINT                                                                   CParticleEmitter2::s_indexNdx;
UINT                                                                   CParticleEmitter2::s_renderedParticles;
UINT                                                                   CParticleEmitter2::s_renderedIndices;
UINT                                                                   CParticleEmitter2::s_maxParticles;
NTempest::C44Matrix                                                    CParticleEmitter2::s_particleToView;
NTempest::C3Vector                                                     CParticleEmitter2::s_quadVectors[4];

CParticleEmitter2::CParticleEmitter2() : m_refCount(1) {
  m_hTex = 0;
  m_replaceableId = 0;
  m_emitterType = PET_BASE_EMITTER;
  m_particleType = PT_QUAD;
  m_randSeed.SetSeed((rand() << 16) | (rand() & 0xFFFF));
  m_numNew = 0.0f;
  m_textureLog = 0;
  m_ooTextureWidth = 1.0f;
  m_ooTextureHeight = 1.0f;
  m_particleEmissionRate = 0.0f;
  m_particleLifeSpan = 0.0f;
  m_particleTailLength = 1.0f;
  m_particleKeys.SetCount(NUM_PARTICLE_KEYS);
  m_particleVelocity = 0.0f;
  m_particleAcceleration = 0.0f;
  m_particleVelocityVariation = 0.1f;
  m_particleAngularVelocity = 0.0f;
  m_particleZsource = 0.0f;
  m_textureRows = 1;
  m_textureColumns = 1;
  m_elapsedVelUpdate = 0.0f;
  m_twinkleFPS = 10.0f;
  m_twinkleOnOff = 1.0f;
  m_twinkleScaleMin = 1.0f;
  m_twinkleScaleMax = 1.0f;
  m_enabled = 0;
  m_enabled2 = 1;
  m_particleHasHead = 1;
  m_particleHasTail = 0;
  m_sortZ = 0;
  m_needSquirt = 0;
  m_updated = 0;
  m_paused = 0;
  m_useModelSpace = 0;
  m_inheritScale = 0;
  m_instantVelLin = 0;
  m_0XKill = 0;
  m_extrude = 0;
  m_xyQuads = 0;
  m_zvelOnly = 0;
  m_tumbler = 0;
  m_tailGrows = 0;
  m_project = 0;
  m_follow = 0;
  m_ivelScale = 1.0f;
  m_drag = 0.0f;
  m_windTime = 0.0f;
  m_followB = 0.0f;
  m_followM = 0.0f;
  for (UINT i = 0; i < MAX_CHILD_EMITTERS; ++i) {
    m_childEmitter[i] = 0;
  }
  m_model = 0;
  m_verticesPerParticle = 0;
  m_indicesPerParticle = 0;
}

CParticleEmitter2::CParticleEmitter2(const CParticleEmitter2 &rhs, int deep) : m_refCount(1) {
  UINT loop;
  UINT count;

  m_numNew = 0.0f;
  m_textureLog = rhs.m_textureLog;
  m_ooTextureWidth = rhs.m_ooTextureWidth;
  m_ooTextureHeight = rhs.m_ooTextureHeight;
  m_priorityPlane = rhs.m_priorityPlane;
  m_emitterType = rhs.m_emitterType;
  m_particleType = rhs.m_particleType;
  m_randSeed.SetSeed((rand() << 16) | (rand() & 0xFFFF));
  for (loop = 0; loop < MAX_CHILD_EMITTERS; ++loop) {
    if (rhs.m_childEmitter[loop]) {
      m_childEmitter[loop] = ParticleSystemManager::GetInstance()->DuplicateEmitter(rhs.m_childEmitter[loop], 0);
    } else {
      m_childEmitter[loop] = 0;
    }
  }
  m_model = (HMODEL)HandleDuplicate(rhs.m_model);
  m_verticesPerParticle = rhs.m_verticesPerParticle;
  m_indicesPerParticle = rhs.m_indicesPerParticle;
  m_particleEmissionRate = rhs.m_particleEmissionRate;
  m_particleLifeSpan = rhs.m_particleLifeSpan;
  m_particleTailLength = rhs.m_particleTailLength;
  m_particleKeys = rhs.m_particleKeys;
  m_particleVelocity = rhs.m_particleVelocity;
  m_particleAcceleration = rhs.m_particleAcceleration;
  m_particleVelocityVariation = rhs.m_particleVelocityVariation;
  m_particleAngularVelocity = rhs.m_particleAngularVelocity;
  m_particleMaterial = rhs.m_particleMaterial;
  m_particleZsource = rhs.m_particleZsource;
  m_textureRows = rhs.m_textureRows;
  m_textureColumns = rhs.m_textureColumns;
  m_hTex = (HTEXTURE)HandleDuplicate(rhs.m_hTex);
  m_replaceableId = rhs.m_replaceableId;
  m_enabled = rhs.m_enabled;
  m_enabled2 = rhs.m_enabled2;
  m_particleHasHead = rhs.m_particleHasHead;
  m_particleHasTail = rhs.m_particleHasTail;
  m_sortZ = rhs.m_sortZ;
  m_needSquirt = rhs.m_needSquirt;
  m_updated = 0;
  m_paused = rhs.m_paused;
  m_useModelSpace = rhs.m_useModelSpace;
  m_inheritScale = rhs.m_inheritScale;
  m_instantVelLin = rhs.m_instantVelLin;
  m_0XKill = rhs.m_0XKill;
  m_extrude = rhs.m_extrude;
  m_xyQuads = rhs.m_xyQuads;
  m_zvelOnly = rhs.m_zvelOnly;
  m_tumbler = rhs.m_tumbler;
  m_tailGrows = rhs.m_tailGrows;
  m_project = rhs.m_project;
  m_follow = rhs.m_follow;
  m_twinkleFPS = rhs.m_twinkleFPS;
  m_twinkleOnOff = rhs.m_twinkleOnOff;
  m_twinkleScaleMin = rhs.m_twinkleScaleMin;
  m_twinkleScaleMax = rhs.m_twinkleScaleMax;
  m_twinkleScaleRange = rhs.m_twinkleScaleRange;
  m_ivelScale = rhs.m_ivelScale;
  m_tumblex = rhs.m_tumblex;
  m_tumbley = rhs.m_tumbley;
  m_tumblez = rhs.m_tumblez;
  m_drag = rhs.m_drag;
  m_windVector = rhs.m_windVector;
  m_windTime = rhs.m_windTime;
  m_elapsedVelUpdate = rhs.m_elapsedVelUpdate;
  m_followB = rhs.m_followB;
  m_followM = rhs.m_followM;

  if (deep && (rhs.m_dead.Count() || rhs.m_alive.Count())) {
    Sync();
    m_dead.Clear();
    m_alive.Clear();
    count = rhs.m_dead.Count();
    for (loop = 0; loop < count; ++loop) {
      m_dead.Push(rhs.m_dead[loop]);
    }
    count = rhs.m_alive.Count();
    if (m_particleType == PT_QUAD) {
      for (loop = 0; loop < count; ++loop) {
        UINT particle = rhs.m_alive[loop];
        m_alive.Push(particle);
        m_particles[particle] = rhs.m_particles[particle];
      }
    } else {
      for (loop = 0; loop < count; ++loop) {
        UINT particle = rhs.m_alive[loop];
        m_alive.Push(particle);
        m_modelParticles[particle] = rhs.m_modelParticles[particle];
      }
    }
  }
}

CParticleEmitter2::~CParticleEmitter2() {
  if (m_hTex) {
    HandleClose(m_hTex);
    m_hTex = 0;
  }
  if (m_model) {
    HandleClose(m_model);
    m_model = 0;
  }
  for (UINT i = 0; i < MAX_CHILD_EMITTERS; ++i) {
    if (m_childEmitter[i]) {
      ParticleSystemManager::GetInstance()->DeleteEmitter2(m_childEmitter[i]);
      m_childEmitter[i] = 0;
    }
  }
}

void CParticleEmitter2::SetModel(HMODEL model) {
  ASSERT(!m_model);
  m_model = model;
  m_particleType = PT_MODEL;
}

float CParticleEmitter2::CalcVelocity() {
  return (NTempest::CRandom::reals_(m_randSeed) * m_particleVelocityVariation + 1.0f) * m_particleVelocity;
}

void CParticleEmitter2::Sync() {
  UINT arraySize = m_particleLifeSpan * m_particleEmissionRate * 1.15f;

  SyncAllocation(arraySize);

  for (UINT index = 0; index < 4; ++index) {
    if (m_childEmitter[index]) {
      UINT childSize =
          arraySize * (UINT)(m_childEmitter[index]->m_particleEmissionRate * m_childEmitter[index]->m_particleLifeSpan * 1.15f);

      if (childSize > 4096) {
        childSize = 4096;
      }

      m_childEmitter[index]->SyncAllocation(childSize);
    }
  }
}

void CParticleEmitter2::SyncReserve(UINT arraySize, UINT oldSize, UINT oldReserve) {
  UINT total = oldSize + oldReserve;

  if (arraySize & (arraySize - 1)) {
    arraySize = 2 * arraySize - 1;
    while (arraySize & (arraySize - 1)) {
      arraySize &= arraySize - 1;
    }
  }

  if (total < arraySize) {
    UINT reserve = arraySize - total;

    if (m_particleType == PT_QUAD) {
      m_particles.ReserveSpace(reserve);
    } else {
      m_modelParticles.ReserveSpace(reserve);
    }

    m_alive.ReserveSpace(reserve);
    m_dead.ReserveSpace(reserve);
  }
}

void CParticleEmitter2::SyncAllocation(UINT arraySize) {
  if (m_particleType == PT_QUAD) {
    UINT oldSize = m_particles.Count();
    if (oldSize >= arraySize) {
      return;
    }

    SyncReserve(arraySize, oldSize, m_particles.Reserved());
    m_particles.SetCount(arraySize);
    m_alive.SetCount(arraySize);
    m_dead.SetCount(arraySize);

    for (UINT u = oldSize; u != arraySize; ++u) {
      m_dead.Push(u);
    }
  } else {
    UINT oldSize = m_modelParticles.Count();
    if (oldSize >= arraySize) {
      return;
    }

    SyncReserve(arraySize, oldSize, m_modelParticles.Reserved());
    m_modelParticles.SetCount(arraySize);
    m_alive.SetCount(arraySize);
    m_dead.SetCount(arraySize);

    for (UINT u = oldSize; u != arraySize; ++u) {
      m_dead.Push(u);
    }
  }
}

void CParticleEmitter2::ProjectParticle(CParticle2 &p) {
  PARTICLEPROJECTCALLBACK CB = ParticleSystemManager::GetProjectCallback();
  ASSERT(CB);

  float              distance = ParticleSystemManager::GetProjectDistance();
  NTempest::C3Segment seg(p.m_position, p.m_position);
  seg.start.z -= distance;
  seg.end.z += distance;

  float z;
  if (CB(seg, z)) {
    NTempest::CImVector color;
    int                 headCell;
    int                 tailCell;
    float               scale;
    m_particleKeys[p.m_keyFrame].Interpolate(p.m_age, color, headCell, tailCell, scale);
    p.m_position.z = z + scale;
  }
}

void CParticleEmitter2::CreateParticle(CParticle2 &p, float elapsedTime, const NTempest::C34Matrix &basis) {
  p.m_age = NTempest::CRandom::real_(m_randSeed) * elapsedTime;
  p.m_keyFrame = 0;
  p.m_position.Set(0.0f, 0.0f, 0.0f);

  if (!m_useModelSpace) {
    p.m_position *= basis;
    if (m_project) {
      ProjectParticle(p);
    }
  }

  p.m_velocity = CalcVelocity() * NTempest::CRandom::C3Vector_(m_randSeed) + m_frameInstantVelLin;
}

void CParticleEmitter2::CreateParticle(CParticle2_Model &p, float elapsedTime, const NTempest::C34Matrix &basis) {
  CreateParticle((CParticle2 &)p, elapsedTime, basis);

  p.m_rotation = NTempest::C4Quaternion();
  p.m_rotVelocity = NTempest::C3Vector(
      NTempest::CRandom::real_(m_randSeed) * m_tumblex.y + m_tumblex.x, (NTempest::CRandom::real_(m_randSeed) + 1.0f) * m_tumbley.y,
      (NTempest::CRandom::real_(m_randSeed) + 1.0f) * m_tumblez.y
  );

  if (m_tumbler) {
    p.m_rotVelocity *= NTempest::C3Vector(NTempest::CRandom::int32_(m_randSeed) & 1 ? 1.0f : -1.0f, NTempest::CRandom::int32_(m_randSeed) & 1 ? 1.0f : -1.0f, NTempest::CRandom::int32_(m_randSeed) & 1 ? 1.0f : -1.0f);
  }
}

BOOL CParticleEmitter2::MoveParticle(CParticle2 &p, float elapsedTime) {
  if (p.m_age < m_windTime) {
    p.m_velocity += elapsedTime * m_windVector;
  }

  if (m_follow) {
    if (p.m_flags & CParticle2::F_BORN) {
      p.m_flags &= ~CParticle2::F_BORN;
    } else {
      p.m_position += m_stepFollowVector;
    }
  }

  NTempest::C3Vector move(elapsedTime * p.m_velocity.x, elapsedTime * p.m_velocity.y, elapsedTime * p.m_velocity.z);
  p.m_position.x += move.x;
  p.m_position.y += move.y;
  p.m_position.z -= elapsedTime * m_particleAcceleration * elapsedTime * 0.5f;
  p.m_position.z += move.z;
  p.m_velocity.z -= elapsedTime * m_particleAcceleration;

  if (*(DWORD *)&m_drag) {
    float drag = elapsedTime * m_drag;
    if (1.0f < drag) {
      drag = 1.0f;
    }
    p.m_velocity -= drag * p.m_velocity;
  }

  if (m_0XKill && NTempest::C3Vector::Dot(move, p.m_position) > 0.0f) {
    return 0;
  }
  return 1;
}

BOOL CParticleEmitter2::MoveParticle(CParticle2_Model &p, float elapsedTime) {
  float velMag = p.m_rotVelocity.Mag();
  if (velMag > 0.0001f) {
    float              angle = velMag * elapsedTime * 0.5f;
    NTempest::C3Vector axis = NTempest::CMath::sin_(angle) * (1.0f / velMag) * p.m_rotVelocity;
    p.m_rotation *= NTempest::C4Quaternion(NTempest::CMath::cos_(angle), axis.x, axis.y, axis.z);
  }

  return MoveParticle((CParticle2 &)p, elapsedTime);
}

BOOL CParticleEmitter2::IRenderParticle(CParticle2 &p, CGxVertexPNCT0 *vtx) {
  UINT randomIndex = 0;
  if (m_twinkleOnOff < 1.0f || m_twinkleScaleRange != 0.0f) {
    randomIndex = (((DWORD)&p >> 5) + NTempest::CMath::ftol_0_256_(m_twinkleFPS * p.m_age)) & RND_TABLE_MASK;
  }
  if (m_twinkleOnOff < 1.0f && m_rndTable[randomIndex] > m_twinkleOnOff) {
    return 0;
  }

  NTempest::CImVector color;
  int                 headCell;
  int                 tailCell;
  float               scale;
  m_particleKeys[p.m_keyFrame].Interpolate(p.m_age, color, headCell, tailCell, scale);

  if (GxCaps().m_colorFormat == GxCF_rgba) {
    color = NTempest::CImVector(color.a, color.b, color.g, color.r);
  }
  if (m_twinkleScaleRange != 0.0f) {
    scale *= m_rndTable[randomIndex] * m_twinkleScaleRange + m_twinkleScaleMin;
  }
  if (m_inheritScale) {
    scale *= m_frameScale;
  }

  NTempest::C3Vector vp = p.m_position * s_particleToView;

  if (m_particleHasHead) {
    float tu = (headCell & (m_textureColumns - 1)) * m_ooTextureWidth;
    float tv = (headCell >> m_textureLog) * m_ooTextureHeight;

    if (!*(DWORD *)&m_particleAngularVelocity) {
      for (UINT i = 0; i < 4; ++i, ++vtx) {
        if (m_xyQuads) {
          vtx->p = scale * s_quadVectors[i] + vp;
        } else {
          vtx->p = NTempest::C3Vector(scale * vc[i][0] + vp.x, scale * vc[i][1] + vp.y, vp.z);
        }
        vtx->n = s_particleNormal;
        vtx->c = color;
        vtx->tc[0] = NTempest::C2Vector(tc[i][0] * m_ooTextureWidth + tu, tc[i][1] * m_ooTextureHeight + tv);
      }
    } else {
      float theta = p.m_age * m_particleAngularVelocity;
      if (m_tumbler && ((DWORD)&p & 0x20)) {
        theta = -theta;
      }

      float st;
      float ct;
      NTempest::CMath::sincos_(theta, st, ct);

      for (UINT i = 0; i < 4; ++i, ++vtx) {
        if (m_xyQuads) {
          NTempest::C33Matrix spinMtx = NTempest::C33Matrix::Rotation(theta, m_xyAxis, true);
          vtx->p = (spinMtx * s_quadVectors[i]) * scale + vp;
        } else {
          float x = scale * vc[i][0];
          float y = scale * vc[i][1];
          vtx->p.x = x * ct - y * st + vp.x;
          vtx->p.y = y * ct + x * st + vp.y;
          vtx->p.z = vp.z;
        }
        vtx->n = s_particleNormal;
        vtx->c = color;
        vtx->tc[0] = NTempest::C2Vector(tc[i][0] * m_ooTextureWidth + tu, tc[i][1] * m_ooTextureHeight + tv);
      }
    }
  }

  if (m_particleHasTail) {
    float tu = (tailCell & (m_textureColumns - 1)) * m_ooTextureWidth;
    float tv = (tailCell >> m_textureLog) * m_ooTextureHeight;
    float tailLength = m_particleTailLength;

    NTempest::C4Vector tmpV(-p.m_velocity.x, -p.m_velocity.y, -p.m_velocity.z, 0.0f);
    if (m_tailGrows && tailLength > p.m_age) {
      tailLength = p.m_age;
    }

    NTempest::C4Vector viewVel4d = tmpV * s_particleToView;
    NTempest::C3Vector viewVel3d = tailLength * (NTempest::C3Vector)viewVel4d;
    NTempest::C3Vector viewVel2d = (NTempest::C2Vector)viewVel3d;
    float              velMag2d = viewVel2d.SquaredMag();

    if (velMag2d >= 0.00077160494f) {
      NTempest::C3Vector ep = vp + viewVel3d;
      viewVel2d *= scale / NTempest::CMath::sqrt_(velMag2d);

      vtx->p = NTempest::C3Vector(vp.x - viewVel2d.y, vp.y + viewVel2d.x, vp.z);
      vtx->n = s_particleNormal;
      vtx->c = color;
      vtx->tc[0] = NTempest::C2Vector(tc[0][0] * m_ooTextureWidth + tu, tc[0][1] * m_ooTextureHeight + tv);
      ++vtx;

      vtx->p = NTempest::C3Vector(vp.x + viewVel2d.y, vp.y - viewVel2d.x, vp.z);
      vtx->n = s_particleNormal;
      vtx->c = color;
      vtx->tc[0] = NTempest::C2Vector(tc[1][0] * m_ooTextureWidth + tu, tc[1][1] * m_ooTextureHeight + tv);
      ++vtx;

      vtx->p = NTempest::C3Vector(ep.x - viewVel2d.y, ep.y + viewVel2d.x, ep.z);
      vtx->n = s_particleNormal;
      vtx->c = color;
      vtx->tc[0] = NTempest::C2Vector(tc[2][0] * m_ooTextureWidth + tu, tc[2][1] * m_ooTextureHeight + tv);
      ++vtx;

      vtx->p = NTempest::C3Vector(ep.x + viewVel2d.y, ep.y - viewVel2d.x, ep.z);
      vtx->n = s_particleNormal;
      vtx->c = color;
      vtx->tc[0] = NTempest::C2Vector(tc[3][0] * m_ooTextureWidth + tu, tc[3][1] * m_ooTextureHeight + tv);
    } else {
      for (UINT i = 0; i < 4; ++i, ++vtx) {
        vtx->p = NTempest::C3Vector(scale * vc[i][0] + vp.x, scale * vc[i][1] + vp.y, vp.z);
        vtx->n = s_particleNormal;
        vtx->c = color;
        vtx->tc[0] = NTempest::C2Vector(tc[i][0] * m_ooTextureWidth + tu, tc[i][1] * m_ooTextureHeight + tv);
      }
    }
  }

  return 1;
}

void CParticleEmitter2::IRenderVertices(const CGxBufCommand &cmd, CGxBuf *buf) {
  CGxVertexPNCT0 *vtxBase = 0;
  switch (cmd.vertex.op) {
    case GxBufOp_Assign:
      vtxBase = (CGxVertexPNCT0 *)GxAllocVertexMem(buf->VertexCount() * sizeof(CGxVertexPNCT0));
      *cmd.vertex.mem[GxVM_Vertex] = vtxBase;
      *cmd.vertex.mem[GxVM_Color] = &vtxBase->c;
      *cmd.vertex.mem[GxVM_Texture0] = vtxBase->tc;
      break;
    case GxBufOp_Fill:
      vtxBase = (CGxVertexPNCT0 *)*cmd.vertex.mem[GxVM_Vertex];
      break;
    case GxBufOp_Nop:
      ASSERT(0);
      return;
  }

  CGxVertexPNCT0 *vtx = vtxBase;
  if (m_sortZ) {
    UINT loop;
    for (loop = 0; loop < m_alive.Count(); ++loop) {
      CSortableParticleRecord sp;
      CParticle2             *p = GetParticle(m_alive[loop]);
      NTempest::C3Vector      position = p->m_position;
      sp.dist = s_particleToView.a2 * position.x + s_particleToView.b2 * position.y + s_particleToView.c2 * position.z + s_particleToView.d2;
      sp.p = p;
      m_pq.Enqueue(sp);
    }

    for (loop = s_maxParticles; loop; --loop) {
      CSortableParticleRecord sp = m_pq.Dequeue();
      if (IRenderParticle(*sp.p, vtx)) {
        vtx += m_verticesPerParticle;
      }
    }
  } else {
    for (UINT loop = 0; loop < s_maxParticles; ++loop) {
      CParticle2 *p = GetParticle(m_alive[loop]);
      if (IRenderParticle(*p, vtx)) {
        vtx += m_verticesPerParticle;
      }
    }
  }

  s_renderedParticles = (UINT)(vtx - vtxBase) / m_verticesPerParticle;
}

void CParticleEmitter2::IRenderIndices(const CGxBufCommand &cmd, CGxBuf *buf) {
  s_renderedIndices = m_indicesPerParticle * s_renderedParticles;

  WORD *indices = 0;
  switch (cmd.index.op) {
    case GxBufOp_Assign:
      indices = (WORD *)GxAllocIndexMem(buf->IndexCount() * sizeof(WORD));
      *cmd.index.mem[GxVM_Indices] = indices;
      break;
    case GxBufOp_Fill:
      indices = (WORD *)*cmd.index.mem[GxVM_Indices];
      break;
    case GxBufOp_Nop:
      ASSERT(0);
      return;
  }

  UINT vertexBase = 0;
  if (m_particleHasHead + m_particleHasTail == 2) {
    for (UINT plp = 0; plp < s_renderedParticles; ++plp, indices += m_indicesPerParticle, vertexBase += m_verticesPerParticle) {
      indices[0] = vertexBase;
      indices[1] = vertexBase + 1;
      indices[2] = vertexBase + 2;
      indices[3] = vertexBase + 3;
      indices[4] = vertexBase + 2;
      indices[5] = vertexBase + 1;
      indices[6] = vertexBase + 4;
      indices[7] = vertexBase + 5;
      indices[8] = vertexBase + 6;
      indices[9] = vertexBase + 7;
      indices[10] = vertexBase + 6;
      indices[11] = vertexBase + 5;
    }
  } else {
    for (UINT plp = 0; plp < s_renderedParticles; ++plp, indices += m_indicesPerParticle, vertexBase += m_verticesPerParticle) {
      indices[0] = vertexBase;
      indices[1] = vertexBase + 1;
      indices[2] = vertexBase + 2;
      indices[3] = vertexBase + 3;
      indices[4] = vertexBase + 2;
      indices[5] = vertexBase + 1;
    }
  }
}

void CParticleEmitter2::BufRenderParticles(CGxBufCommand &cmd, CGxBuf *buf) {
  CParticleEmitter2 *emitter = (CParticleEmitter2 *)buf->UserArg();
  emitter->IRenderVertices(cmd, buf);
  emitter->IRenderIndices(cmd, buf);
}

void CParticleEmitter2::RenderParticles() {
  NTempest::C44Matrix identity;
  NTempest::C44Matrix worldToView;
  GxXformView(worldToView);
  GxXformSetView(identity);

  NTempest::C44Matrix viewRelative;
  *viewRelative.Row3AsVec3() = -m_cameraWorldPos;

  if (m_useModelSpace) {
    s_particleToView = NTempest::C44Matrix(m_modelToWorld) * viewRelative * worldToView;
  } else {
    s_particleToView = viewRelative * worldToView;
  }

  if (m_xyQuads) {
    static NTempest::C3Vector vcv[4] = {
        NTempest::C3Vector(-1.0f, 1.0f, 0.0f), NTempest::C3Vector(-1.0f, -1.0f, 0.0f), NTempest::C3Vector(1.0f, 1.0f, 0.0f),
        NTempest::C3Vector(1.0f, -1.0f, 0.0f)
    };
    static NTempest::C44Matrix quadToView;

    if (m_useModelSpace) {
      quadToView = s_particleToView;
    } else {
      quadToView = NTempest::C44Matrix(m_modelToWorld) * s_particleToView;
    }

    for (UINT i = 0; i < 4; ++i) {
      s_quadVectors[i] = NTempest::C44Matrix::mul3v33m_(vcv[i], quadToView);
    }

    m_xyAxis = *quadToView.Row2AsVec3();
    m_xyAxis.Normalize();
  }

  GxVertexShaderSelect(GxVS_PassThru);
  GxRsPush();
  CGxTex *texture = TextureGetGxTex(m_hTex, 0, 0);
  if (texture) {
    GxRsSet(GxRs_Texture0, texture);
    GxRsSet(GxRs_Blend, m_particleMaterial.alpha);
    GxRsSet(GxRs_Culling, 0);
    GxRsSet(GxRs_Lighting, m_particleMaterial.enableLighting);
    GxRsSet(GxRs_Fog, m_particleMaterial.enableFog);
    GxRsSet(GxRs_DepthWrite, m_particleMaterial.enableDepthWrites);

    s_maxParticles = Gx_MaxVertices / m_verticesPerParticle;
    s_maxParticles = min(s_maxParticles, m_alive.Count());
    CGxBuf *buf = GxBufGetDynamic(GxVBF_PNCT0);
    buf->CountSet(m_verticesPerParticle * s_maxParticles, m_indicesPerParticle * s_maxParticles);
    buf->m_userCallback = BufRenderParticles;
    buf->m_userArg = this;
    GxBufLock(buf);
    GxBufRender(CGxBatch(GxPrim_Triangles, s_renderedIndices, 0, -1, -1));
    GxBufUnlock();
  }
  GxRsPop();
  GxXformSetView(worldToView);
}

BOOL CParticleEmitter2::RenderParticle(CParticle2_Model &p) {
  UINT randomIndex = 0;
  if (m_twinkleOnOff < 1.0f || m_twinkleScaleRange != 0.0f) {
    randomIndex = (((DWORD)&p >> 5) + NTempest::CMath::ftol_0_256_(m_twinkleFPS * p.m_age)) & RND_TABLE_MASK;
  }
  if (m_twinkleOnOff < 1.0f && m_rndTable[randomIndex] > m_twinkleOnOff) {
    return 0;
  }

  NTempest::CImVector color;
  int                 headCell;
  int                 tailCell;
  float               scale;
  m_particleKeys[p.m_keyFrame].Interpolate(p.m_age, color, headCell, tailCell, scale);
  if (m_twinkleScaleRange != 0.0f) {
    scale *= m_rndTable[randomIndex] * m_twinkleScaleRange + m_twinkleScaleMin;
  }
  if (m_inheritScale) {
    scale *= m_frameScale;
  }

  NTempest::C34Matrix particleMatrix = p.m_rotation;
  particleMatrix.Scale(scale);
  *particleMatrix.Row3AsVec3() = p.m_position;
  if (m_useModelSpace) {
    particleMatrix *= m_modelToWorld;
  }
  *particleMatrix.Row3AsVec3() -= m_cameraWorldPos;

  ModelAnimate(m_model, particleMatrix, 1.0f, NTempest::C3Vector(0.0f), NTempest::C3Vector(0.0f));
  ModelSetVertexColor(m_model, color.r, color.g, color.b, 0);
  ModelSetVertexAlpha(m_model, color.a, 0);
  ModelRender(m_model, 0, 0);
  return 1;
}

void CParticleEmitter2::RenderParticleModels() {
  if (!m_model) {
    return;
  }

  NTempest::C44Matrix worldToView;
  GxXformView(worldToView);

  if (m_sortZ) {
    NTempest::C34Matrix particleToView;
    if (m_useModelSpace) {
      particleToView = NTempest::C44Matrix(m_modelToWorld) * worldToView;
    } else {
      particleToView = worldToView;
    }
    UINT loop;
    for (loop = 0; loop < m_alive.Count(); ++loop) {
      CSortableParticleRecord sp;
      CParticle2             &p = m_particles[m_alive[loop]];
      sp.dist = particleToView.a2 * p.m_position.x + particleToView.b2 * p.m_position.y + particleToView.c2 * p.m_position.z + particleToView.d2;
      sp.p = &p;
      m_pq.Enqueue(sp);
    }

    for (loop = m_alive.Count(); loop; --loop) {
      CSortableParticleRecord sp = m_pq.Dequeue();
      RenderParticle(*(CParticle2_Model *)sp.p);
    }
  } else {
    for (UINT loop = 0; loop < m_alive.Count(); ++loop) {
      RenderParticle(m_modelParticles[m_alive[loop]]);
    }
  }
}

void CParticleEmitter2::Render() {
  if (m_alive.IsEmpty()) {
    return;
  }

  ActivityBegin(ACTIVITY_PARTICLE);
  if (m_particleType == PT_QUAD) {
    RenderParticles();
  } else {
    RenderParticleModels();
  }
  ActivityEnd(ACTIVITY_PARTICLE);

  for (UINT index = 0; index < 4; ++index) {
    if (m_childEmitter[index]) {
      m_childEmitter[index]->Render();
    }
  }
}
void CParticleEmitter2::DestroyParticle(CParticle2 &p) {
}

void CParticleEmitter2::SetEnabled(int enable, int recurse) {
  m_enabled = enable;
}

void CParticleEmitter2::SetEnabled2(int enable2, int recurse) {
  m_enabled2 = enable2;
}

void CParticleEmitter2::SetEmissionRate(float particlesPerSecond) {
  m_particleEmissionRate = particlesPerSecond > 0.0f ? particlesPerSecond : 0.0f;
}

void CParticleEmitter2::SetLifeSpan(float lifeSpan) {
  m_particleLifeSpan = lifeSpan;
  for (UINT i = 0; i < m_particleKeys.Count(); ++i) {
    m_particleKeys[i].SetLifeSpan(lifeSpan);
  }
}

void CParticleEmitter2::SetVelocity(float velocity) {
  m_particleVelocity = velocity;
}

void CParticleEmitter2::SetAcceleration(float acceleration) {
  m_particleAcceleration = acceleration;
}

void CParticleEmitter2::SetVelocityVariation(float variation) {
  m_particleVelocityVariation = variation;
}

void CParticleEmitter2::SetMaterial(const CParticleMat &material, HTEXTURE hTex) {
  if (m_hTex) {
    HandleClose(m_hTex);
  }
  m_hTex = (HTEXTURE)HandleDuplicate(hTex);
  m_particleMaterial = material;
}

void CParticleEmitter2::MaterialDisableLight(int disable) {
  m_particleMaterial.enableLighting = !disable;
}

void CParticleEmitter2::MaterialDisableFog(int disable) {
  m_particleMaterial.enableFog = !disable;
}

void CParticleEmitter2::SetTexture(HTEXTURE hTex) {
  if (m_hTex) {
    HandleClose(m_hTex);
  }

  m_hTex = (HTEXTURE)HandleDuplicate(hTex);
}

void CParticleEmitter2::SetReplaceableId(UINT id) {
  m_replaceableId = id;
}

void CParticleEmitter2::SetKey(UINT keyNdx, const CParticleKey &key) {
  m_particleKeys[keyNdx] = key;
}

void CParticleEmitter2::SetTextureDimensions(UINT rows, UINT columns) {
  VALIDATEBEGIN;
  VALIDATE((rows & (rows - 1)) == 0);
  VALIDATE((columns & (columns - 1)) == 0);
  VALIDATE(rows && columns);
  VALIDATEENDVOID;

  m_textureRows = rows;
  m_textureColumns = columns;
  m_textureLog = -1;
  do {
    columns >>= 1;
    ++m_textureLog;
  } while (columns);
  m_ooTextureWidth = 1.0f / (float)m_textureColumns;
  m_ooTextureHeight = 1.0f / (float)rows;
}

void CParticleEmitter2::SetParticleStyle(BOOL hasHead, BOOL hasTail, float tailLength, bool tailGrows) {
  m_particleHasHead = hasHead;
  m_particleHasTail = hasTail;
  m_particleTailLength = tailLength;
  m_tailGrows = tailGrows;
  UINT styleCount = m_particleHasHead + m_particleHasTail;
  m_verticesPerParticle = 4 * styleCount;
  m_indicesPerParticle = 6 * styleCount;
}

int CParticleEmitter2::Enabled() {
  return m_enabled;
}

int CParticleEmitter2::Enabled2() {
  return m_enabled2;
}

float CParticleEmitter2::EmissionRate() {
  return m_particleEmissionRate;
}

float CParticleEmitter2::LifeSpan() {
  return m_particleLifeSpan;
}

float CParticleEmitter2::Velocity() {
  return m_particleVelocity;
}

float CParticleEmitter2::Acceleration() {
  return m_particleAcceleration;
}

float CParticleEmitter2::VelocityVariation() {
  return m_particleVelocityVariation;
}

UINT CParticleEmitter2::ReplaceableId() {
  return m_replaceableId;
}

const CParticleKey &CParticleEmitter2::Key(UINT keyNdx) {
  return m_particleKeys[keyNdx];
}

void CParticleEmitter2::TextureDimensions(UINT &rows, UINT &columns) {
  rows = m_textureRows;
  columns = m_textureColumns;
}

void CParticleEmitter2::ParticleStyle(int &hasHead, int &hasTail, float &tailLength) {
  hasHead = m_particleHasHead;
  hasTail = m_particleHasTail;
  tailLength = m_particleTailLength;
}

void CParticleEmitter2::SingletonMgrUpdate(float elapsedTime, const NTempest::C3Vector &cameraWorldPos, int suppressNewParticles) {
  ActivityBegin(ACTIVITY_PARTICLE);
  m_cameraWorldPos = cameraWorldPos;

  for (UINT index = 0; index < 4; ++index) {
    if (m_childEmitter[index]) {
      m_childEmitter[index]->m_cameraWorldPos = cameraWorldPos;
    }
  }

  if (!m_updated) {
    InternalUpdate(elapsedTime, m_paused ? 1 : suppressNewParticles);
  }

  m_updated = 0;
  m_paused = 0;
  ActivityEnd(ACTIVITY_PARTICLE);
}

void CParticleEmitter2::UpdateXform(const NTempest::C34Matrix &modelToWorld, const NTempest::C3Vector &cameraWorldPos) {
  m_cameraWorldPos = cameraWorldPos;
  m_modelToWorld = modelToWorld;
  *m_modelToWorld.Row3AsVec3() += m_cameraWorldPos;

  m_frameScale = modelToWorld.Row0AsVec3()->Mag();
}

void CParticleEmitter2::Update(float elapsedTime, const NTempest::C34Matrix &modelToWorld, const NTempest::C3Vector &cameraWorldPos) {
  m_prevModelToWorldTrans = *m_modelToWorld.Row3AsVec3();

  UpdateXform(modelToWorld, cameraWorldPos);
  for (UINT index = 0; index < MAX_CHILD_EMITTERS; ++index) {
    if (m_childEmitter[index]) {
      m_childEmitter[index]->UpdateXform(modelToWorld, cameraWorldPos);
    }
  }

  if (NTempest::CMath::fequal_(elapsedTime, 0.0f)) {
    m_paused = 1;
    return;
  }

  ActivityBegin(ACTIVITY_PARTICLE);

  if (m_follow) {
    m_followVector = *m_modelToWorld.Row3AsVec3() - m_prevModelToWorldTrans;

    float scale = m_followVector.Mag() / elapsedTime * m_followM + m_followB;
    if (scale < 0.0f) {
      scale = 0.0f;
    } else if (scale > 1.0f) {
      scale = 1.0f;
    }

    m_followVector *= scale;
  }

  if (m_instantVelLin) {
    m_elapsedVelUpdate += elapsedTime;
    if (m_elapsedVelUpdate > VEL_UPDATE_TIME) {
      float frames = m_elapsedVelUpdate / VEL_UPDATE_TIME;
      m_elapsedVelUpdate = 0.0f;

      if (m_alive.IsEmpty()) {
        m_frameInstantVelLin = NTempest::C3Vector(0.0f);
      } else {
        m_frameInstantVelLin = *m_modelToWorld.Row3AsVec3() - m_prevModelToWorldTrans;
        m_frameInstantVelLin *= (1.0f / frames) * m_ivelScale;
      }
    }
  }

  InternalUpdate(elapsedTime, 0);
  m_updated = 1;
  ActivityEnd(ACTIVITY_PARTICLE);
}

void CParticleEmitter2::EmitNewParticles(float elapsedTime, const NTempest::C34Matrix &basis) {
  if (m_needSquirt) {
    UINT numToEmit = ParticleSystemManager::GetScaler() * m_particleEmissionRate;

    while (!m_dead.IsEmpty() && numToEmit--) {
      EmitParticle(0.0f, basis);
    }

    m_needSquirt = 0;
  }

  if (IsEnabled()) {
    UINT numEmitted = 0;
    m_numNew += ParticleSystemManager::GetScaler() * m_particleEmissionRate * elapsedTime;

    if (m_extrude) {
      NTempest::C3Vector *trans = (NTempest::C3Vector *)basis.Row3AsVec3();
      NTempest::C3Vector  curModelToWorldTrans = *trans;
      NTempest::C3Vector  extrude = curModelToWorldTrans - m_prevModelToWorldTrans;
      UINT                numNew = m_numNew;

      while (!m_dead.IsEmpty() && numNew--) {
        *trans = extrude * NTempest::CRandom::real_(m_randSeed) + m_prevModelToWorldTrans;
        EmitParticle(elapsedTime, basis);
        ++numEmitted;
      }

      *trans = curModelToWorldTrans;
    } else {
      UINT numNew = m_numNew;

      while (!m_dead.IsEmpty() && numNew--) {
        EmitParticle(elapsedTime, basis);
        ++numEmitted;
      }
    }

    m_numNew -= numEmitted;
  }
}

void CParticleEmitter2::InternalUpdate(float elapsedTime, int suppressNewParticles) {
  if (elapsedTime < 0.0f) {
    elapsedTime = 0.0f;
  }

  if (elapsedTime > s_maxTimeStep) {
    float numSteps = floor(elapsedTime / s_maxTimeStep);
    elapsedTime -= s_maxTimeStep * numSteps;

    float lifeSteps = floor(m_particleLifeSpan / s_maxTimeStep);
    if (numSteps > lifeSteps) {
      numSteps = lifeSteps;
    }
    if (numSteps > 255.0f) {
      numSteps = 255.0f;
    }

    UINT steps = NTempest::CMath::ftol_0_256_(numSteps);
    m_stepFollowVector = (1.0f / (steps + 1)) * m_followVector;

    for (UINT index = 0; index < steps; ++index) {
      StepUpdate(s_maxTimeStep, suppressNewParticles);
    }
  } else {
    m_stepFollowVector = m_followVector;
  }

  StepUpdate(elapsedTime, suppressNewParticles);
}

void CParticleEmitter2::StepUpdate(float elapsedTime, int suppressNewParticles) {
  if (IsEnabled() || m_needSquirt) {
    Sync();
  }

  if (!suppressNewParticles) {
    EmitNewParticles(elapsedTime, m_modelToWorld);
  }

  for (UINT loop = 0; loop < m_alive.Count(); ++loop) {
    CParticle2 *p = GetParticle(m_alive[loop]);

    p->m_age += elapsedTime;
    if (p->m_age < m_particleLifeSpan) {
      ASSERT(m_particleKeys.Count() == 2);
      if (p->m_age > m_particleKeys[0].m_endTime) {
        p->m_keyFrame = 1;
      } else {
        p->m_keyFrame = 0;
      }

      NTempest::C3Vector prevPos = p->m_position;
      int                keepParticle;
      if (m_particleType == PT_QUAD) {
        keepParticle = MoveParticle(*p, elapsedTime);
      } else {
        keepParticle = MoveParticle(*(CParticle2_Model *)p, elapsedTime);
      }

      if (!keepParticle) {
        DestroyParticle(*p);
        m_dead.Push(m_alive[loop]);
        m_alive.Remove(loop);
        --loop;
      } else {
        for (UINT ce = 0; ce < MAX_CHILD_EMITTERS; ++ce) {
          CParticleEmitter2 *child = m_childEmitter[ce];
          if (child) {
            NTempest::C3Vector saveTrans = *m_modelToWorld.Row3AsVec3();
            *m_modelToWorld.Row3AsVec3() = p->m_position;

            if (child->m_instantVelLin) {
              child->m_frameInstantVelLin = p->m_velocity;
            }
            if (child->m_extrude) {
              child->m_prevModelToWorldTrans = prevPos;
            }

            child->EmitNewParticles(elapsedTime, m_modelToWorld);
            *m_modelToWorld.Row3AsVec3() = saveTrans;
          }
        }
      }
    } else {
      DestroyParticle(*p);
      m_dead.Push(m_alive[loop]);
      m_alive.Remove(loop);
      --loop;
    }
  }

  for (UINT index = 0; index < MAX_CHILD_EMITTERS; ++index) {
    if (m_childEmitter[index]) {
      m_childEmitter[index]->InternalUpdate(elapsedTime, 1);
    }
  }
}

void CParticleEmitter2::Squirt() {
  m_needSquirt = 1;
}

void CParticleEmitter2::Flush() {
  while (m_alive.Count() > 0) {
    DestroyParticle(*GetParticle(m_alive[0]));

    m_dead.Push(m_alive[0]);
    m_alive.Remove(0);
  }
}

void CParticleEmitter2::SetZsource(float zsource) {
  m_particleZsource = zsource;
  if (NTempest::CMath::fabs_(m_particleZsource) < MIN_ZSOURCE) {
    m_particleZsource = 0.0f;
  }
}

void CParticleEmitter2::SetSortZ(int sortZ) {
  m_sortZ = sortZ;
}

int CParticleEmitter2::SortZ() {
  return m_sortZ;
}

void CParticleEmitter2::SetFollowParams(float speed1, float scale1, float speed2, float scale2) {
  if (NTempest::CMath::fnotequal_(speed2, speed1)) {
    m_followM = (scale2 - scale1) / (speed2 - speed1);
    m_followB = scale1 - m_followM * speed1;
  } else {
    m_followM = 0.0f;
    m_followB = 0.0f;
  }
}

void CParticleEmitter2::AddChildEmitter(CParticleEmitter2 *child) {
  UINT i;
  for (i = 0; i < MAX_CHILD_EMITTERS; ++i) {
    if (!m_childEmitter[i]) {
      m_childEmitter[i] = child;
      break;
    }
  }
  ASSERT(i != MAX_CHILD_EMITTERS);
}

CParticleEmitter2 *CParticleEmitter2::AddRef() {
  ++m_refCount;
  return this;
}

void CParticleEmitter2::DecRef() {
  if (--m_refCount == 0) {
    delete this;
  }
}
