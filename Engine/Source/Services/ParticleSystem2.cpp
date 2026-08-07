#include <Base/Base.h>

#include "ParticleSystem2.h"

#include "Base/Activity.h"
#include "Gx/CGxDevice.h"
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
static NTempest::C44Matrix quadToView;
static NTempest::C3Vector  vcv[4] = {
    NTempest::C3Vector(-1.0f, 1.0f, 0.0f), NTempest::C3Vector(-1.0f, -1.0f, 0.0f), NTempest::C3Vector(1.0f, 1.0f, 0.0f),
    NTempest::C3Vector(1.0f, -1.0f, 0.0f)
};
static const float s_maxTimeStep = 0.1f;

NTempest::CPriorityQ<CSortableParticleRecord, CSortableParticleRecord> CParticleEmitter2::m_pq;
const float                                                            CParticleEmitter2::VEL_UPDATE_TIME = 1.0f / 30.0f;
const float                                                            CParticleEmitter2::MIN_ZSOURCE = 0.001f;
float                                                                  CParticleEmitter2::m_rndTable[128];
UINT                                                                   CParticleEmitter2::s_vertexNdx;
UINT                                                                   CParticleEmitter2::s_indexNdx;
UINT                                                                   CParticleEmitter2::s_renderedParticles;
UINT                                                                   CParticleEmitter2::s_renderedIndices;
UINT                                                                   CParticleEmitter2::s_maxParticles;
NTempest::C3Vector                                                     CParticleEmitter2::s_quadVectors[4];
NTempest::C44Matrix                                                    CParticleEmitter2::s_particleToView;

static NTempest::C3Vector s_particleNormal;

CParticleEmitter2::CParticleEmitter2()
    : m_refCount(1),
      m_numNew(0.0f),
      m_textureLog(0),
      m_ooTextureWidth(1.0f),
      m_ooTextureHeight(1.0f),
      m_priorityPlane(0),
      m_emitterType(PET_BASE_EMITTER),
      m_particleType(PT_QUAD),
      m_model(0),
      m_verticesPerParticle(0),
      m_indicesPerParticle(0),
      m_elapsedTime(0.0f),
      m_particleEmissionRate(0.0f),
      m_particleLifeSpan(0.0f),
      m_particleTailLength(1.0f),
      m_particleVelocity(0.0f),
      m_particleAcceleration(0.0f),
      m_particleVelocityVariation(0.1f),
      m_particleZsource(0.0f),
      m_particleAngularVelocity(0.0f),
      m_textureRows(1),
      m_textureColumns(1),
      m_hTex(0),
      m_replaceableId(0),
      m_enabled(0),
      m_enabled2(1),
      m_particleHasHead(1),
      m_particleHasTail(0),
      m_sortZ(0),
      m_needSquirt(0),
      m_updated(0),
      m_paused(0),
      m_useModelSpace(0),
      m_inheritScale(0),
      m_instantVelLin(0),
      m_0XKill(0),
      m_extrude(0),
      m_xyQuads(0),
      m_zvelOnly(0),
      m_tumbler(0),
      m_tailGrows(0),
      m_project(0),
      m_follow(0),
      m_twinkleFPS(10.0f),
      m_twinkleOnOff(1.0f),
      m_twinkleScaleMin(1.0f),
      m_twinkleScaleMax(1.0f),
      m_twinkleScaleRange(0.0f),
      m_ivelScale(1.0f),
      m_tumblex(0.0f),
      m_tumbley(0.0f),
      m_tumblez(0.0f),
      m_drag(0.0f),
      m_windVector(0.0f),
      m_windTime(0.0f),
      m_followB(0.0f),
      m_followM(0.0f),
      m_cameraWorldPos(0.0f),
      m_prevModelToWorldTrans(0.0f),
      m_elapsedVelUpdate(0.0f),
      m_frameInstantVelLin(0.0f),
      m_frameScale(0.0f),
      m_followScalar(0.0f),
      m_followVector(0.0f),
      m_stepFollowVector(0.0f),
      m_xyAxis(0.0f) {
  m_alive.Clear();
  m_dead.Clear();
  m_particleMaterial.alpha = GxBlend_Opaque;
  m_particleMaterial.enableLighting = 1;
  m_particleMaterial.enableFog = 1;
  m_particleMaterial.enableDepthWrites = 1;
  m_randSeed.SetSeed((rand() << 16) | rand());
  for (UINT i = 0; i < 4; ++i) {
    m_childEmitter[i] = 0;
  }
}
CParticleEmitter2::CParticleEmitter2(const CParticleEmitter2 &rhs, int deep)
    : m_refCount(1),
      m_numNew(0.0f),
      m_textureLog(rhs.m_textureLog),
      m_ooTextureWidth(rhs.m_ooTextureWidth),
      m_ooTextureHeight(rhs.m_ooTextureHeight),
      m_priorityPlane(rhs.m_priorityPlane),
      m_emitterType(rhs.m_emitterType),
      m_particleType(rhs.m_particleType),
      m_model(static_cast<HMODEL>(HandleDuplicate(rhs.m_model))),
      m_verticesPerParticle(rhs.m_verticesPerParticle),
      m_indicesPerParticle(rhs.m_indicesPerParticle),
      m_elapsedTime(0.0f),
      m_particleEmissionRate(rhs.m_particleEmissionRate),
      m_particleLifeSpan(rhs.m_particleLifeSpan),
      m_particleTailLength(rhs.m_particleTailLength),
      m_particleVelocity(rhs.m_particleVelocity),
      m_particleAcceleration(rhs.m_particleAcceleration),
      m_particleVelocityVariation(rhs.m_particleVelocityVariation),
      m_particleZsource(rhs.m_particleZsource),
      m_particleAngularVelocity(rhs.m_particleAngularVelocity),
      m_particleMaterial(rhs.m_particleMaterial),
      m_textureRows(rhs.m_textureRows),
      m_textureColumns(rhs.m_textureColumns),
      m_hTex(static_cast<HTEXTURE>(HandleDuplicate(rhs.m_hTex))),
      m_replaceableId(rhs.m_replaceableId),
      m_enabled(rhs.m_enabled),
      m_enabled2(rhs.m_enabled2),
      m_particleHasHead(rhs.m_particleHasHead),
      m_particleHasTail(rhs.m_particleHasTail),
      m_sortZ(rhs.m_sortZ),
      m_needSquirt(rhs.m_needSquirt),
      m_updated(rhs.m_updated),
      m_paused(rhs.m_paused),
      m_useModelSpace(rhs.m_useModelSpace),
      m_inheritScale(rhs.m_inheritScale),
      m_instantVelLin(rhs.m_instantVelLin),
      m_0XKill(rhs.m_0XKill),
      m_extrude(rhs.m_extrude),
      m_xyQuads(rhs.m_xyQuads),
      m_zvelOnly(rhs.m_zvelOnly),
      m_tumbler(rhs.m_tumbler),
      m_tailGrows(rhs.m_tailGrows),
      m_project(rhs.m_project),
      m_follow(rhs.m_follow),
      m_twinkleFPS(rhs.m_twinkleFPS),
      m_twinkleOnOff(rhs.m_twinkleOnOff),
      m_twinkleScaleMin(rhs.m_twinkleScaleMin),
      m_twinkleScaleMax(rhs.m_twinkleScaleMax),
      m_twinkleScaleRange(rhs.m_twinkleScaleRange),
      m_ivelScale(rhs.m_ivelScale),
      m_tumblex(rhs.m_tumblex),
      m_tumbley(rhs.m_tumbley),
      m_tumblez(rhs.m_tumblez),
      m_drag(rhs.m_drag),
      m_windVector(rhs.m_windVector),
      m_windTime(rhs.m_windTime),
      m_followB(rhs.m_followB),
      m_followM(rhs.m_followM),
      m_cameraWorldPos(0.0f),
      m_prevModelToWorldTrans(0.0f),
      m_elapsedVelUpdate(rhs.m_elapsedVelUpdate),
      m_frameInstantVelLin(0.0f),
      m_frameScale(0.0f),
      m_followScalar(0.0f),
      m_followVector(0.0f),
      m_stepFollowVector(0.0f),
      m_xyAxis(0.0f) {
  UINT loop;
  UINT count;

  m_alive.Clear();
  m_dead.Clear();
  m_randSeed.SetSeed((rand() << 16) | rand());
  m_particleKeys = rhs.m_particleKeys;

  ParticleSystemManager *manager = ParticleSystemManager::GetInstance();
  for (loop = 0; loop < 4; ++loop) {
    m_childEmitter[loop] = rhs.m_childEmitter[loop] ? manager->DuplicateEmitter(rhs.m_childEmitter[loop], 0) : 0;
  }

  if (deep && (rhs.m_dead.Count() || rhs.m_alive.Count())) {
    Sync();
    count = rhs.m_dead.Count();
    for (loop = 0; loop < count; ++loop) {
      m_dead.Push(rhs.m_dead[loop]);
    }
    count = rhs.m_alive.Count();
    for (loop = 0; loop < count; ++loop) {
      UINT particle = rhs.m_alive[loop];
      m_alive.Push(particle);
      if (m_particleType == PT_QUAD) {
        m_particles[particle] = rhs.m_particles[particle];
      } else {
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
  ParticleSystemManager *manager = ParticleSystemManager::GetInstance();
  for (UINT i = 0; i < 4; ++i) {
    if (m_childEmitter[i]) {
      manager->DeleteEmitter2(m_childEmitter[i]);
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
  UINT arraySize = static_cast<UINT>(m_particleLifeSpan * m_particleEmissionRate * 1.15f);

  SyncAllocation(arraySize);

  for (UINT index = 0; index < 4; ++index) {
    if (m_childEmitter[index]) {
      UINT childSize =
          arraySize * static_cast<UINT>(m_childEmitter[index]->m_particleLifeSpan * m_childEmitter[index]->m_particleEmissionRate * 1.15f);

      if (childSize > 4096) {
        childSize = 4096;
      }

      m_childEmitter[index]->SyncAllocation(childSize);
    }
  }
}

void CParticleEmitter2::SyncReserve(UINT arraySize, UINT oldSize, UINT oldReserve) {
  if (arraySize & (arraySize - 1)) {
    arraySize = 2 * arraySize - 1;
    while (arraySize & (arraySize - 1)) {
      arraySize &= arraySize - 1;
    }
  }

  if (oldSize + oldReserve < arraySize) {
    arraySize -= oldSize + oldReserve;

    if (m_particleType == PT_QUAD) {
      m_particles.ReserveSpace(arraySize);
    } else {
      m_modelParticles.ReserveSpace(arraySize);
    }

    m_alive.ReserveSpace(arraySize);
    m_dead.ReserveSpace(arraySize);
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

    for (UINT u = oldSize; u < arraySize; ++u) {
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

    for (UINT u = oldSize; u < arraySize; ++u) {
      m_dead.Push(u);
    }
  }
}

void CParticleEmitter2::ProjectParticle(CParticle2 &p) {
  PARTICLEPROJECTCALLBACK CB = ParticleSystemManager::GetProjectCallback();
  ASSERT(CB);

  NTempest::C3Segment seg(p.m_position, p.m_position);
  seg.start.z -= ParticleSystemManager::GetProjectDistance();
  seg.end.z += ParticleSystemManager::GetProjectDistance();

  int                 headCell;
  int                 tailCell;
  float               z;
  float               scale;
  NTempest::CImVector color;
  if (CB(seg, z)) {
    m_particleKeys[p.m_keyFrame].Interpolate(p.m_age, color, headCell, tailCell, scale);
    p.m_position.z = z + scale;
  }
}

void CParticleEmitter2::CreateParticle(CParticle2 &p, float elapsedTime, const NTempest::C34Matrix &basis) {
  p.m_keyFrame = 0;
  p.m_position = NTempest::C3Vector(0.0f);
  p.m_age = NTempest::CRandom::real_(m_randSeed) * elapsedTime;

  if (!m_useModelSpace) {
    p.m_position *= basis;
    if (m_project) {
      ProjectParticle(p);
    }
  }

  p.m_velocity = NTempest::CRandom::C3Vector_(m_randSeed) * CalcVelocity() + m_frameInstantVelLin;
}

void CParticleEmitter2::CreateParticle(CParticle2_Model &p, float elapsedTime, const NTempest::C34Matrix &basis) {
  CreateParticle(static_cast<CParticle2 &>(p), elapsedTime, basis);

  p.m_rotation = NTempest::C4Quaternion();
  p.m_rotVelocity = NTempest::C3Vector(
      NTempest::CRandom::real_(m_randSeed) * m_tumblex.y + m_tumblex.x, (NTempest::CRandom::real_(m_randSeed) + 1.0f) * m_tumbley.y,
      (NTempest::CRandom::real_(m_randSeed) + 1.0f) * m_tumblez.y
  );

  if (m_tumbler) {
    p.m_rotVelocity *= NTempest::C3Vector(
        NTempest::CRandom::uint32_(m_randSeed) & 1 ? 1.0f : -1.0f, NTempest::CRandom::uint32_(m_randSeed) & 1 ? 1.0f : -1.0f,
        NTempest::CRandom::uint32_(m_randSeed) & 1 ? 1.0f : -1.0f
    );
  }
}

BOOL CParticleEmitter2::MoveParticle(CParticle2 &p, float elapsedTime) {
  if (p.m_age < m_windTime) {
    p.m_velocity.x += elapsedTime * m_windVector.x;
    p.m_velocity.y += elapsedTime * m_windVector.y;
    p.m_velocity.z += elapsedTime * m_windVector.z;
  }

  if (m_follow) {
    if (p.m_flags & 0x1) {
      p.m_flags &= ~0x1;
    } else {
      p.m_position.x += m_stepFollowVector.x;
      p.m_position.y += m_stepFollowVector.y;
      p.m_position.z += m_stepFollowVector.z;
    }
  }

  NTempest::C3Vector move(elapsedTime * p.m_velocity.x, elapsedTime * p.m_velocity.y, elapsedTime * p.m_velocity.z);

  p.m_position.x += move.x;
  p.m_position.y += move.y;
  p.m_position.z += move.z - elapsedTime * m_particleAcceleration * elapsedTime * 0.5f;
  p.m_velocity.z -= elapsedTime * m_particleAcceleration;

  if (m_drag != 0.0f) {
    float drag = elapsedTime * m_drag;
    if (drag > 1.0f) {
      drag = 1.0f;
    }

    p.m_velocity.x -= drag * p.m_velocity.x;
    p.m_velocity.y -= drag * p.m_velocity.y;
    p.m_velocity.z -= drag * p.m_velocity.z;
  }

  return !m_0XKill || move.x * p.m_position.x + move.y * p.m_position.y + move.z * p.m_position.z <= 0.0f;
}

BOOL CParticleEmitter2::MoveParticle(CParticle2_Model &p, float elapsedTime) {
  float velMag = p.m_rotVelocity.Mag();
  if (velMag > 0.0001f) {
    float angle = velMag * elapsedTime * 0.5f;
    float scale = NTempest::CMath::sin_(angle) / velMag;
    p.m_rotation =
        NTempest::C4Quaternion(NTempest::CMath::cos_(angle), scale * p.m_rotVelocity.x, scale * p.m_rotVelocity.y, scale * p.m_rotVelocity.z) *
        p.m_rotation;
  }

  return MoveParticle(static_cast<CParticle2 &>(p), elapsedTime);
}

BOOL CParticleEmitter2::IRenderParticle(CParticle2 &p, CGxVertexPNCT0 *vtx) {
  UINT randomIndex = 0;
  if (m_twinkleOnOff < 1.0f || m_twinkleScaleRange != 0.0f) {
    randomIndex = ((reinterpret_cast<DWORD>(&p) >> 5) + NTempest::CMath::ftol_0_256_(m_twinkleFPS * p.m_age)) & 0x7F;
  }
  if (m_rndTable[randomIndex] > m_twinkleOnOff) {
    return 0;
  }

  ASSERT(p.m_keyFrame < m_particleKeys.Count());
  NTempest::CImVector color;
  int                 headCell;
  int                 tailCell;
  float               scale;
  m_particleKeys[p.m_keyFrame].Interpolate(p.m_age, color, headCell, tailCell, scale);

  if (GxCaps().m_colorFormat == GxCF_rgba) {
    color.Set(color.a, color.b, color.g, color.r);
  }
  scale *= m_twinkleScaleMin + m_rndTable[randomIndex] * m_twinkleScaleRange;
  if (m_inheritScale) {
    scale *= m_frameScale;
  }

  NTempest::C3Vector viewPosition = p.m_position * s_particleToView;
  CGxVertexPNCT0    *vertex = vtx;

  if (m_particleHasHead) {
    UINT  cell = static_cast<UINT>(headCell);
    float texU = (cell % m_textureColumns) * m_ooTextureWidth;
    float texV = (cell / m_textureColumns) * m_ooTextureHeight;

    if (m_particleAngularVelocity == 0.0f) {
      for (UINT i = 0; i < 4; ++i) {
        if (m_xyQuads) {
          vertex[i].p = viewPosition + s_quadVectors[i] * scale;
        } else {
          vertex[i].p = NTempest::C3Vector(viewPosition.x + vc[i][0] * scale, viewPosition.y + vc[i][1] * scale, viewPosition.z);
        }
      }
    } else {
      float theta = p.m_age * m_particleAngularVelocity;
      if (m_tumbler && (reinterpret_cast<DWORD>(&p) & 0x20)) {
        theta = -theta;
      }

      if (m_xyQuads) {
        NTempest::C33Matrix spin = NTempest::C33Matrix::Rotation(theta, m_xyAxis, true);
        for (UINT i = 0; i < 4; ++i) {
          const NTempest::C3Vector &base = s_quadVectors[i];
          NTempest::C3Vector        rotated(
              base.x * spin.a0 + base.y * spin.b0 + base.z * spin.c0, base.x * spin.a1 + base.y * spin.b1 + base.z * spin.c1,
              base.x * spin.a2 + base.y * spin.b2 + base.z * spin.c2
          );
          vertex[i].p = viewPosition + rotated * scale;
        }
      } else {
        float cosine = NTempest::CMath::cos_(theta);
        float sine = NTempest::CMath::sin_(theta);
        for (UINT i = 0; i < 4; ++i) {
          float x = vc[i][0] * cosine - vc[i][1] * sine;
          float y = vc[i][0] * sine + vc[i][1] * cosine;
          vertex[i].p = NTempest::C3Vector(viewPosition.x + x * scale, viewPosition.y + y * scale, viewPosition.z);
        }
      }
    }

    for (UINT i = 0; i < 4; ++i) {
      vertex[i].n = s_particleNormal;
      vertex[i].c = color;
      vertex[i].tc[0] = NTempest::C2Vector(texU + tc[i][0] * m_ooTextureWidth, texV + tc[i][1] * m_ooTextureHeight);
    }
    vertex += 4;
  }

  if (!m_particleHasTail) {
    return 1;
  }

  UINT  cell = static_cast<UINT>(tailCell);
  float texU = (cell % m_textureColumns) * m_ooTextureWidth;
  float texV = (cell / m_textureColumns) * m_ooTextureHeight;
  float tailLength = m_particleTailLength;
  if (m_tailGrows && tailLength > p.m_age) {
    tailLength = p.m_age;
  }

  NTempest::C4Vector velocity(-p.m_velocity.x, -p.m_velocity.y, -p.m_velocity.z, 0.0f);
  NTempest::C4Vector viewVelocity = velocity * s_particleToView;
  NTempest::C3Vector delta(viewVelocity.x * tailLength, viewVelocity.y * tailLength, viewVelocity.z * tailLength);
  float              lengthSquared = delta.x * delta.x + delta.y * delta.y;

  if (lengthSquared < 0.00077160494f) {
    for (UINT i = 0; i < 4; ++i) {
      vertex[i].p = NTempest::C3Vector(viewPosition.x + vc[i][0] * scale, viewPosition.y + vc[i][1] * scale, viewPosition.z);
    }
  } else {
    NTempest::C3Vector end = viewPosition + delta;
    float              ooLength = scale / NTempest::CMath::sqrt_(lengthSquared);
    float              halfX = delta.x * ooLength;
    float              halfY = delta.y * ooLength;
    vertex[0].p = NTempest::C3Vector(viewPosition.x - halfY, viewPosition.y + halfX, viewPosition.z);
    vertex[1].p = NTempest::C3Vector(viewPosition.x + halfY, viewPosition.y - halfX, viewPosition.z);
    vertex[2].p = NTempest::C3Vector(end.x - halfY, end.y + halfX, end.z);
    vertex[3].p = NTempest::C3Vector(end.x + halfY, end.y - halfX, end.z);
  }

  for (UINT i = 0; i < 4; ++i) {
    vertex[i].n = s_particleNormal;
    vertex[i].c = color;
    vertex[i].tc[0] = NTempest::C2Vector(texU + tc[i][0] * m_ooTextureWidth, texV + tc[i][1] * m_ooTextureHeight);
  }
  return 1;
}

void CParticleEmitter2::IRenderVertices(const CGxBufCommand &cmd, CGxBuf *buf) {
  CGxVertexPNCT0 *vertices = 0;
  if (cmd.vertex.op == GxBufOp_Fill) {
    ASSERT(cmd.vertex.mem[GxVM_Vertex]);
    vertices = reinterpret_cast<CGxVertexPNCT0 *>(*cmd.vertex.mem[GxVM_Vertex]);
  } else if (cmd.vertex.op == GxBufOp_Assign) {
    ASSERT(cmd.vertex.mem[GxVM_Vertex]);
    vertices = reinterpret_cast<CGxVertexPNCT0 *>(GxAllocVertexMem(buf->VertexCount() * sizeof(CGxVertexPNCT0)));
    *cmd.vertex.mem[GxVM_Vertex] = vertices;
    if (cmd.vertex.mem[GxVM_Color]) {
      *cmd.vertex.mem[GxVM_Color] = &vertices->c;
    }
    if (cmd.vertex.mem[GxVM_Texture0]) {
      *cmd.vertex.mem[GxVM_Texture0] = vertices->tc;
    }
  } else {
    ASSERT(cmd.vertex.op != GxBufOp_Nop);
    return;
  }

  CGxVertexPNCT0 *vtxBase = vertices;
  if (m_sortZ) {
    UINT loop;
    for (loop = 0; loop < m_alive.Count(); ++loop) {
      UINT                    particleIndex = m_alive[loop];
      CParticle2             *p = GetParticle(particleIndex);
      NTempest::C3Vector      position = p->m_position * s_particleToView;
      CSortableParticleRecord sp;
      sp.dist = position.z;
      sp.p = p;
      m_pq.Enqueue(sp);
    }

    for (loop = 0; loop < s_maxParticles; ++loop) {
      CSortableParticleRecord sp = m_pq.Dequeue();
      if (IRenderParticle(*sp.p, vertices)) {
        vertices += m_verticesPerParticle;
      }
    }

    while (m_pq.HasEntries()) {
      m_pq.Dequeue();
    }
  } else {
    for (UINT loop = 0; loop < s_maxParticles; ++loop) {
      UINT        particleIndex = m_alive[loop];
      CParticle2 *p = GetParticle(particleIndex);
      if (IRenderParticle(*p, vertices)) {
        vertices += m_verticesPerParticle;
      }
    }
  }

  s_renderedParticles = static_cast<UINT>((vertices - vtxBase) / m_verticesPerParticle);
}

void CParticleEmitter2::IRenderIndices(const CGxBufCommand &cmd, CGxBuf *buf) {
  s_renderedIndices = s_renderedParticles * m_indicesPerParticle;

  WORD *indices = 0;
  if (cmd.index.op == GxBufOp_Fill) {
    ASSERT(cmd.index.mem[GxVM_Indices]);
    indices = reinterpret_cast<WORD *>(*cmd.index.mem[GxVM_Indices]);
  } else if (cmd.index.op == GxBufOp_Assign) {
    ASSERT(cmd.index.mem[GxVM_Indices]);
    indices = reinterpret_cast<WORD *>(GxAllocIndexMem(buf->IndexCount() * sizeof(WORD)));
    *cmd.index.mem[GxVM_Indices] = indices;
  } else {
    ASSERT(cmd.index.op != GxBufOp_Nop);
    return;
  }

  WORD vertexBase = 0;
  for (UINT particle = 0; particle < s_renderedParticles; ++particle) {
    indices[0] = vertexBase;
    indices[1] = vertexBase + 1;
    indices[2] = vertexBase + 2;
    indices[3] = vertexBase + 3;
    indices[4] = vertexBase + 2;
    indices[5] = vertexBase + 1;
    indices += 6;

    if (m_particleHasHead && m_particleHasTail) {
      indices[0] = vertexBase + 4;
      indices[1] = vertexBase + 5;
      indices[2] = vertexBase + 6;
      indices[3] = vertexBase + 7;
      indices[4] = vertexBase + 6;
      indices[5] = vertexBase + 5;
      indices += 6;
    }
    vertexBase = static_cast<WORD>(vertexBase + m_verticesPerParticle);
  }
}

void CParticleEmitter2::BufRenderParticles(CGxBufCommand &cmd, CGxBuf *buf) {
  CParticleEmitter2 *emitter = static_cast<CParticleEmitter2 *>(buf->UserArg());
  emitter->IRenderVertices(cmd, buf);
  emitter->IRenderIndices(cmd, buf);
}

void CParticleEmitter2::RenderParticles() {
  NTempest::C44Matrix worldToView;
  GxXformView(worldToView);
  NTempest::C44Matrix identity;
  GxXformSetView(identity);

  NTempest::C44Matrix viewRelative;
  viewRelative.d0 = -m_cameraWorldPos.x;
  viewRelative.d1 = -m_cameraWorldPos.y;
  viewRelative.d2 = -m_cameraWorldPos.z;

  NTempest::C44Matrix modelToWorld(
      m_modelToWorld.a0, m_modelToWorld.a1, m_modelToWorld.a2, 0.0f, m_modelToWorld.b0, m_modelToWorld.b1, m_modelToWorld.b2, 0.0f, m_modelToWorld.c0,
      m_modelToWorld.c1, m_modelToWorld.c2, 0.0f, m_modelToWorld.d0, m_modelToWorld.d1, m_modelToWorld.d2, 1.0f
  );

  if (m_useModelSpace) {
    s_particleToView = modelToWorld * viewRelative * worldToView;
  } else {
    s_particleToView = viewRelative * worldToView;
  }

  if (m_xyQuads) {
    quadToView = m_useModelSpace ? s_particleToView : modelToWorld * s_particleToView;
    for (UINT i = 0; i < 4; ++i) {
      s_quadVectors[i] = NTempest::C3Vector(
          vcv[i].x * quadToView.a0 + vcv[i].y * quadToView.b0 + vcv[i].z * quadToView.c0,
          vcv[i].x * quadToView.a1 + vcv[i].y * quadToView.b1 + vcv[i].z * quadToView.c1,
          vcv[i].x * quadToView.a2 + vcv[i].y * quadToView.b2 + vcv[i].z * quadToView.c2
      );
    }
    m_xyAxis = NTempest::C3Vector(quadToView.c0, quadToView.c1, quadToView.c2);
    m_xyAxis.Normalize();
  }

  GxVertexShaderSelect(GxVS_PassThru);
  GxRsPush();
  CGxTex *texture = TextureGetGxTex(m_hTex, 0, 0);
  if (texture) {
    GxRsSet(GxRs_Texture0, texture);
    GxRsSet(GxRs_Blend, m_particleMaterial.alpha);
    GxRsSet(GxRs_Culling, 0);
    GxRsSet(GxRs_Lighting, !!m_particleMaterial.enableLighting);
    GxRsSet(GxRs_Fog, !!m_particleMaterial.enableFog);
    GxRsSet(GxRs_DepthWrite, !!m_particleMaterial.enableDepthWrites);

    UINT maxParticles = Gx_MaxVertices / m_verticesPerParticle;
    s_maxParticles = m_alive.Count() < maxParticles ? m_alive.Count() : maxParticles;
    CGxBuf *buf = GxBufGetDynamic(GxVBF_PNCT0);
    ASSERT(buf);
    buf->CountSet(s_maxParticles * m_verticesPerParticle, s_maxParticles * m_indicesPerParticle);
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
    randomIndex = ((reinterpret_cast<DWORD>(&p) >> 5) + NTempest::CMath::ftol_0_256_(m_twinkleFPS * p.m_age)) & 0x7F;
  }
  if (m_rndTable[randomIndex] > m_twinkleOnOff) {
    return 0;
  }

  ASSERT(p.m_keyFrame < m_particleKeys.Count());
  NTempest::CImVector color;
  int                 headCell;
  int                 tailCell;
  float               scale;
  m_particleKeys[p.m_keyFrame].Interpolate(p.m_age, color, headCell, tailCell, scale);
  scale *= m_twinkleScaleMin + m_rndTable[randomIndex] * m_twinkleScaleRange;
  if (m_inheritScale) {
    scale *= m_frameScale;
  }

  float               x = p.m_rotation.x;
  float               y = p.m_rotation.y;
  float               z = p.m_rotation.z;
  float               w = p.m_rotation.w;
  float               xx = x * x;
  float               yy = y * y;
  float               zz = z * z;
  float               xy = x * y;
  float               xz = x * z;
  float               yz = y * z;
  float               xw = x * w;
  float               yw = y * w;
  float               zw = z * w;
  NTempest::C34Matrix particleMatrix(
      1.0f - 2.0f * (yy + zz), 2.0f * (xy + zw), 2.0f * (xz - yw), 2.0f * (xy - zw), 1.0f - 2.0f * (xx + zz), 2.0f * (yz + xw), 2.0f * (xz + yw),
      2.0f * (yz - xw), 1.0f - 2.0f * (xx + yy), p.m_position.x, p.m_position.y, p.m_position.z
  );
  particleMatrix.Scale(scale);
  if (m_useModelSpace) {
    particleMatrix *= m_modelToWorld;
  }
  particleMatrix.d0 -= m_cameraWorldPos.x;
  particleMatrix.d1 -= m_cameraWorldPos.y;
  particleMatrix.d2 -= m_cameraWorldPos.z;

  NTempest::C3Vector zero;
  ModelAnimate(m_model, particleMatrix, 1.0f, zero, zero);
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
  NTempest::C44Matrix particleToView(
      m_modelToWorld.a0, m_modelToWorld.a1, m_modelToWorld.a2, 0.0f, m_modelToWorld.b0, m_modelToWorld.b1, m_modelToWorld.b2, 0.0f, m_modelToWorld.c0,
      m_modelToWorld.c1, m_modelToWorld.c2, 0.0f, m_modelToWorld.d0, m_modelToWorld.d1, m_modelToWorld.d2, 1.0f
  );
  s_particleToView = m_useModelSpace ? particleToView * worldToView : worldToView;

  if (m_sortZ) {
    UINT loop;
    for (loop = 0; loop < m_alive.Count(); ++loop) {
      CParticle2_Model       &p = m_modelParticles[m_alive[loop]];
      CSortableParticleRecord sp;
      sp.dist = (p.m_position * s_particleToView).z;
      sp.p = &p;
      m_pq.Enqueue(sp);
    }

    for (loop = 0; loop < m_alive.Count(); ++loop) {
      CSortableParticleRecord sp = m_pq.Dequeue();
      RenderParticle(*static_cast<CParticle2_Model *>(sp.p));
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
  if (m_particleType == PT_MODEL) {
    RenderParticleModels();
  } else {
    RenderParticles();
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
  m_hTex = static_cast<HTEXTURE>(HandleDuplicate(hTex));
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

  m_hTex = static_cast<HTEXTURE>(HandleDuplicate(hTex));
}

void CParticleEmitter2::SetReplaceableId(UINT id) {
  m_replaceableId = id;
}

void CParticleEmitter2::SetKey(UINT keyNdx, const CParticleKey &key) {
  ASSERT(keyNdx < m_particleKeys.Count());
  m_particleKeys[keyNdx] = key;
}

void CParticleEmitter2::SetTextureDimensions(UINT rows, UINT columns) {
  ASSERT((rows & (rows - 1)) == 0);
  ASSERT((columns & (columns - 1)) == 0);
  ASSERT(rows && columns);

  m_textureRows = rows;
  m_textureColumns = columns;
  m_textureLog = static_cast<UINT>(-1);
  do {
    columns >>= 1;
    ++m_textureLog;
  } while (columns);
  m_ooTextureWidth = 1.0f / static_cast<float>(m_textureColumns);
  m_ooTextureHeight = 1.0f / static_cast<float>(rows);
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
  m_modelToWorld.d0 += m_cameraWorldPos.x;
  m_modelToWorld.d1 += m_cameraWorldPos.y;
  m_modelToWorld.d2 += m_cameraWorldPos.z;

  m_frameScale = NTempest::CMath::sqrt_(modelToWorld.a0 * modelToWorld.a0 + modelToWorld.a1 * modelToWorld.a1 + modelToWorld.a2 * modelToWorld.a2);
}

void CParticleEmitter2::Update(float elapsedTime, const NTempest::C34Matrix &modelToWorld, const NTempest::C3Vector &cameraWorldPos) {
  m_prevModelToWorldTrans.x = m_modelToWorld.d0;
  m_prevModelToWorldTrans.y = m_modelToWorld.d1;
  m_prevModelToWorldTrans.z = m_modelToWorld.d2;

  UpdateXform(modelToWorld, cameraWorldPos);
  for (UINT index = 0; index < 4; ++index) {
    if (m_childEmitter[index]) {
      m_childEmitter[index]->UpdateXform(modelToWorld, cameraWorldPos);
    }
  }

  if (!NTempest::CMath::fequal_(elapsedTime, 0.0f)) {
    ActivityBegin(ACTIVITY_PARTICLE);

    if (m_follow) {
      m_followVector.x = m_modelToWorld.d0 - m_prevModelToWorldTrans.x;
      m_followVector.y = m_modelToWorld.d1 - m_prevModelToWorldTrans.y;
      m_followVector.z = m_modelToWorld.d2 - m_prevModelToWorldTrans.z;

      float scale = m_followVector.Mag() / elapsedTime * m_followM + m_followB;
      if (scale < 0.0f) {
        scale = 0.0f;
      } else if (scale > 1.0f) {
        scale = 1.0f;
      }

      m_followVector.x *= scale;
      m_followVector.y *= scale;
      m_followVector.z *= scale;
    }

    if (m_instantVelLin) {
      m_elapsedVelUpdate += elapsedTime;
      if (m_elapsedVelUpdate > VEL_UPDATE_TIME) {
        float frames = m_elapsedVelUpdate / VEL_UPDATE_TIME;
        m_elapsedVelUpdate = 0.0f;

        if (!m_alive.IsEmpty()) {
          m_frameInstantVelLin.x = m_modelToWorld.d0 - m_prevModelToWorldTrans.x;
          m_frameInstantVelLin.y = m_modelToWorld.d1 - m_prevModelToWorldTrans.y;
          m_frameInstantVelLin.z = m_modelToWorld.d2 - m_prevModelToWorldTrans.z;

          float scale = 1.0f / frames * m_ivelScale;
          m_frameInstantVelLin.x *= scale;
          m_frameInstantVelLin.y *= scale;
          m_frameInstantVelLin.z *= scale;
        } else {
          m_frameInstantVelLin.x = 0.0f;
          m_frameInstantVelLin.y = 0.0f;
          m_frameInstantVelLin.z = 0.0f;
        }
      }
    }

    InternalUpdate(elapsedTime, 0);
    m_updated = 1;
    ActivityEnd(ACTIVITY_PARTICLE);
  } else {
    m_paused = 1;
  }
}

void CParticleEmitter2::EmitNewParticles(float elapsedTime, const NTempest::C34Matrix &basis) {
  if (m_needSquirt) {
    UINT numToEmit = static_cast<UINT>(ParticleSystemManager::GetScaler() * m_particleEmissionRate);

    while (numToEmit && !m_dead.IsEmpty()) {
      --numToEmit;
      EmitParticle(0.0f, basis);
    }

    m_needSquirt = 0;
  }

  if (IsEnabled()) {
    UINT numEmitted = 0;
    m_numNew += ParticleSystemManager::GetScaler() * m_particleEmissionRate * elapsedTime;

    if (m_extrude) {
      NTempest::C3Vector curModelToWorldTrans(basis.d0, basis.d1, basis.d2);
      NTempest::C3Vector extrude(
          curModelToWorldTrans.x - m_prevModelToWorldTrans.x, curModelToWorldTrans.y - m_prevModelToWorldTrans.y,
          curModelToWorldTrans.z - m_prevModelToWorldTrans.z
      );
      NTempest::C34Matrix &mutableBasis = const_cast<NTempest::C34Matrix &>(basis);
      UINT                 numNew = static_cast<UINT>(m_numNew);

      while (numNew && !m_dead.IsEmpty()) {
        --numNew;

        float random = NTempest::CRandom::real_(m_randSeed);
        mutableBasis.d0 = m_prevModelToWorldTrans.x + random * extrude.x;
        mutableBasis.d1 = m_prevModelToWorldTrans.y + random * extrude.y;
        mutableBasis.d2 = m_prevModelToWorldTrans.z + random * extrude.z;

        EmitParticle(elapsedTime, basis);
        ++numEmitted;
      }

      mutableBasis.d0 = curModelToWorldTrans.x;
      mutableBasis.d1 = curModelToWorldTrans.y;
      mutableBasis.d2 = curModelToWorldTrans.z;
    } else {
      UINT numNew = static_cast<UINT>(m_numNew);

      while (numNew && !m_dead.IsEmpty()) {
        --numNew;

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

  if (elapsedTime <= s_maxTimeStep) {
    m_stepFollowVector = m_followVector;
  } else {
    float numSteps = static_cast<float>(floor(elapsedTime / s_maxTimeStep));
    elapsedTime -= s_maxTimeStep * numSteps;

    float lifeSteps = static_cast<float>(floor(m_particleLifeSpan / s_maxTimeStep));
    if (numSteps > lifeSteps) {
      numSteps = lifeSteps;
    }
    if (numSteps > 255.0f) {
      numSteps = 255.0f;
    }

    BYTE  steps = NTempest::CMath::ftol_0_256_(numSteps);
    float ooSteps = 1.0f / (steps + 1);
    m_stepFollowVector.x = ooSteps * m_followVector.x;
    m_stepFollowVector.y = ooSteps * m_followVector.y;
    m_stepFollowVector.z = ooSteps * m_followVector.z;

    for (UINT index = 0; index < steps; ++index) {
      StepUpdate(s_maxTimeStep, suppressNewParticles);
    }
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
    UINT        particleIndex = m_alive[loop];
    CParticle2 *p = GetParticle(particleIndex);

    p->m_age += elapsedTime;
    if (p->m_age < m_particleLifeSpan) {
      ASSERT(m_particleKeys.Count() == 2);
      p->m_keyFrame = p->m_age > m_particleKeys[0].m_endTime;

      NTempest::C3Vector prevPos = p->m_position;
      int                keepParticle;
      if (m_particleType) {
        keepParticle = MoveParticle(*static_cast<CParticle2_Model *>(p), elapsedTime);
      } else {
        keepParticle = MoveParticle(*p, elapsedTime);
      }

      if (!keepParticle) {
        DestroyParticle(*p);
        m_dead.Push(m_alive[loop]);
        m_alive.Remove(loop);
        --loop;
      } else {
        for (UINT ce = 0; ce < 4; ++ce) {
          CParticleEmitter2 *child = m_childEmitter[ce];
          if (child) {
            NTempest::C3Vector saveTrans(m_modelToWorld.d0, m_modelToWorld.d1, m_modelToWorld.d2);

            m_modelToWorld.d0 = p->m_position.x;
            m_modelToWorld.d1 = p->m_position.y;
            m_modelToWorld.d2 = p->m_position.z;

            if (child->m_instantVelLin) {
              child->m_frameInstantVelLin = p->m_velocity;
            }
            if (child->m_extrude) {
              child->m_prevModelToWorldTrans = prevPos;
            }

            child->EmitNewParticles(elapsedTime, m_modelToWorld);

            m_modelToWorld.d0 = saveTrans.x;
            m_modelToWorld.d1 = saveTrans.y;
            m_modelToWorld.d2 = saveTrans.z;
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

  for (UINT index = 0; index < 4; ++index) {
    if (m_childEmitter[index]) {
      m_childEmitter[index]->InternalUpdate(elapsedTime, 1);
    }
  }
}

void CParticleEmitter2::Squirt() {
  m_needSquirt = 1;
}

void CParticleEmitter2::Flush() {
  while (!m_alive.IsEmpty()) {
    DestroyParticle(*GetParticle(m_alive[0]));

    m_dead.Push(m_alive[0]);
    m_alive.Remove(0);
  }
}

void CParticleEmitter2::SetZsource(float zsource) {
  m_particleZsource = fabs(zsource) < 0.001f ? 0.0f : zsource;
}

void CParticleEmitter2::SetSortZ(int sortZ) {
  m_sortZ = sortZ;
}

int CParticleEmitter2::SortZ() {
  return m_sortZ;
}

void CParticleEmitter2::SetFollowParams(float speed1, float scale1, float speed2, float scale2) {
  float speedDelta = speed2 - speed1;
  if (fabs(speedDelta) < 2.3841858e-7f) {
    m_followM = 0.0f;
    m_followB = 0.0f;
  } else {
    m_followM = (scale2 - scale1) / speedDelta;
    m_followB = scale1 - m_followM * speed1;
  }
}

void CParticleEmitter2::AddChildEmitter(CParticleEmitter2 *child) {
  UINT i = 0;
  while (i < 4 && m_childEmitter[i]) {
    ++i;
  }
  ASSERT(i != 4);
  m_childEmitter[i] = child;
}

CParticleEmitter2 *CParticleEmitter2::AddRef() {
  ++m_refCount;
  return this;
}

void CParticleEmitter2::DecRef() {
  if (m_refCount-- == 1) {
    delete this;
  }
}
