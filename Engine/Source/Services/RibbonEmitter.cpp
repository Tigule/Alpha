#include "RibbonEmitter.h"

#include "Tempest/c44matrix.h"

#include <math.h>

static const float MIN_EDGE_LIFE_SPAN = 0.25f;
static const float NORMAL_SCALE = 1.0f;

static void DuplicateTextureArray(const TSGrowableArray<HTEXTURE> &src, TSGrowableArray<HTEXTURE> *dst) {
  UINT numTextures = src.Count();
  dst->SetCount(numTextures);

  for (UINT i = 0; i < numTextures; ++i) {
    (*dst)[i] = static_cast<HTEXTURE>(HandleDuplicate(reinterpret_cast<HOBJECT>(src[i])));
  }
}

void CRibbonEmitter::PrivCopy(const CRibbonEmitter &rhs) {
  m_edges = rhs.m_edges;
  m_writePos = rhs.m_writePos;
  m_readPos = rhs.m_readPos;
  m_startTime = rhs.m_startTime;
  m_posSet = rhs.m_posSet;
  m_prevPos = rhs.m_prevPos;
  m_gxVertices = rhs.m_gxVertices;
  m_gxIndices = rhs.m_gxIndices;
  m_ooLifeSpan = rhs.m_ooLifeSpan;
  m_tmpDU = rhs.m_tmpDU;
  m_tmpDV = rhs.m_tmpDV;
  m_ooTmpDU = rhs.m_ooTmpDU;
  m_ooTmpDV = rhs.m_ooTmpDV;
  m_texSlotBox = rhs.m_texSlotBox;
  m_prevVertical = rhs.m_prevVertical;
  m_currVertical = rhs.m_currVertical;
  m_prevDir = rhs.m_prevDir;
  m_currDir = rhs.m_currDir;
  m_prevDirScaled = rhs.m_prevDirScaled;
  m_currDirScaled = rhs.m_currDirScaled;
  m_below0 = rhs.m_below0;
  m_below1 = rhs.m_below1;
  m_above0 = rhs.m_above0;
  m_above1 = rhs.m_above1;
  m_initialized = rhs.m_initialized;
  m_edgesPerSec = rhs.m_edgesPerSec;
  m_edgeLifeSpan = rhs.m_edgeLifeSpan;
  m_materials = rhs.m_materials;
  DuplicateTextureArray(rhs.m_textures, &m_textures);
  m_replaces = rhs.m_replaces;
  m_diffuseClr = rhs.m_diffuseClr;
  m_texBox = rhs.m_texBox;
  m_rows = rhs.m_rows;
  m_cols = rhs.m_cols;
  m_currPos = rhs.m_currPos;
  m_enabled = rhs.m_enabled;
  m_texSlot = rhs.m_texSlot;
  m_above = rhs.m_above;
  m_below = rhs.m_below;
  m_gravity = rhs.m_gravity;
}

void CRibbonEmitter::InitInterpDeltas() {
  float scale = (m_prevPos - m_currPos).Mag();

  m_below0 = m_prevPos - m_prevVertical * m_below;
  m_below1 = m_currPos - m_currVertical * m_below;
  m_above0 = m_prevPos + m_prevVertical * m_above;
  m_above1 = m_currPos + m_currVertical * m_above;
  m_prevDirScaled = m_prevDir * scale;
  m_currDirScaled = m_currDir * scale;
}

void CRibbonEmitter::InterpEdge(float age, float t, UINT advance) {
  CRibbonVertex &v0 = m_gxVertices[2 * m_writePos];
  float          w0 = 1.0f - t;

  v0.pos = (m_below0 + m_prevDirScaled * t) * w0 + (m_below1 - m_currDirScaled * w0) * t;
  m_gxVertices[2 * m_writePos + 1].pos = (m_above0 + m_prevDirScaled * t) * w0 + (m_above1 - m_currDirScaled * w0) * t;

  m_edges[m_writePos] = age;
  Advance(m_writePos, advance);
  ASSERT(m_writePos != m_readPos || advance == 0);
}

void CRibbonEmitter::Advance(UINT &pos, UINT amount) {
  ASSERT(amount <= m_edges.Count());

  pos += amount;
  if (pos >= m_edges.Count()) {
    pos -= m_edges.Count();
  }
}

void CRibbonEmitter::ConvertTexSlotToTexCoords() {
  UINT col = m_texSlot % m_cols;
  UINT row = m_texSlot / m_cols;

  m_texSlotBox.l = col * m_tmpDU + m_texBox.l;
  m_texSlotBox.t = row * m_tmpDV + m_texBox.t;
  m_texSlotBox.r = m_texSlotBox.l + m_tmpDU;
  m_texSlotBox.b = m_texSlotBox.t + m_tmpDV;
}

void CRibbonEmitter::CloseTextureHandles() {
  UINT count = m_textures.Count();
  UINT index;

  for (index = 0; index < count; ++index) {
    if (m_textures[index]) {
      HandleClose(m_textures[index]);
    }
  }
}

CRibbonEmitter::~CRibbonEmitter() {
  CloseTextureHandles();
  m_initialized = 0;
}

CRibbonEmitter::CRibbonEmitter()
    : m_refCount(1),
      m_prevPos(0.0f),
      m_cameraPos(0.0f),
      m_texSlotBox(0.0f),
      m_prevVertical(0.0f),
      m_currVertical(0.0f),
      m_prevDir(0.0f),
      m_currDir(0.0f),
      m_prevDirScaled(0.0f),
      m_currDirScaled(0.0f),
      m_below0(0.0f),
      m_below1(0.0f),
      m_above0(0.0f),
      m_above1(0.0f),
      m_diffuseClr(0ul),
      m_texBox(0.0f),
      m_initialized(0),
      m_updated(0),
      m_currPos(0.0f) {
}

CRibbonEmitter::CRibbonEmitter(const CRibbonEmitter &rhs)
    : m_refCount(1),
      m_prevPos(0.0f),
      m_cameraPos(0.0f),
      m_texSlotBox(0.0f),
      m_prevVertical(0.0f),
      m_currVertical(0.0f),
      m_prevDir(0.0f),
      m_currDir(0.0f),
      m_prevDirScaled(0.0f),
      m_currDirScaled(0.0f),
      m_below0(0.0f),
      m_below1(0.0f),
      m_above0(0.0f),
      m_above1(0.0f),
      m_diffuseClr(0ul),
      m_texBox(0.0f),
      m_initialized(0),
      m_updated(0),
      m_currPos(0.0f) {
  PrivCopy(rhs);
}

const CRibbonEmitter &CRibbonEmitter::operator=(const CRibbonEmitter &rhs) {
  CloseTextureHandles();
  PrivCopy(rhs);
  return *this;
}

void CRibbonEmitter::Initialize(
    float                              edgesPerSec,
    float                              edgeLifeSpanInSec,
    const NTempest::CImVector         &diffuseClr,
    const TSGrowableArray<HTEXTURE>   &textures,
    const TSGrowableArray<CRibbonMat> &materials,
    const TSGrowableArray<UINT>       &replaces,
    const NTempest::CRect             &texBox,
    UINT                               rows,
    UINT                               cols
) {
  UINT numEdges;
  UINT t;
  UINT count;

  ASSERT(!m_initialized);
  ASSERT(edgesPerSec >= 1.0f);
  ASSERT(edgeLifeSpanInSec > 0.0f);
  ASSERT(materials.Count() == textures.Count());
  ASSERT(textures.Count() == replaces.Count());

  edgesPerSec = static_cast<float>(ceil(edgesPerSec));
  if (edgeLifeSpanInSec < MIN_EDGE_LIFE_SPAN) {
    edgeLifeSpanInSec = MIN_EDGE_LIFE_SPAN;
  }

  numEdges = static_cast<UINT>(ceil(edgesPerSec * edgeLifeSpanInSec) + 2.0);
  m_edges.SetCount(numEdges);
  m_readPos = 0;
  m_writePos = 0;
  m_startTime = 0.0f;
  m_posSet = 0;

  m_gxVertices.SetCount(2 * numEdges);
  for (t = 0; t < m_gxVertices.Count(); ++t) {
    m_gxVertices[t].pos = NTempest::C3Vector(0.0f);
    m_gxVertices[t].texCoord = NTempest::C2Vector(0.0f, 0.0f);
  }

  m_gxIndices.SetCount(4 * numEdges);
  for (t = 0; t < m_gxIndices.Count(); ++t) {
    m_gxIndices[t] = static_cast<WORD>(t % (2 * numEdges));
  }

  m_ooLifeSpan = 1.0f / edgeLifeSpanInSec;
  m_edgeLifeSpan = edgeLifeSpanInSec;
  m_edgesPerSec = edgesPerSec;
  m_tmpDU = (texBox.r - texBox.l) / cols;
  m_tmpDV = (texBox.b - texBox.t) / rows;
  m_ooTmpDU = 1.0f / m_tmpDU;
  m_ooTmpDV = 1.0f / m_tmpDV;
  m_diffuseClr = diffuseClr;

  count = materials.Count();
  m_materials.SetCount(count);
  m_textures.SetCount(count);
  m_replaces.SetCount(count);
  for (t = 0; t < count; ++t) {
    m_materials[t] = materials[t];
    m_textures[t] = static_cast<HTEXTURE>(HandleDuplicate(reinterpret_cast<HOBJECT>(textures[t])));
    m_replaces[t] = replaces[t];
  }

  m_texBox = texBox;
  m_rows = rows;
  m_cols = cols;
  m_enabled = 1;
  m_texSlot = 0;
  ConvertTexSlotToTexCoords();
  m_above = 10.0f;
  m_below = 10.0f;
  m_gravity = 0.0f;
  m_initialized = 1;
}

UINT CRibbonEmitter::ReplaceTexture(UINT replaceableId, HTEXTURE texture) {
  UINT numReplaced = 0;

  for (UINT index = 0; index < m_replaces.Count(); ++index) {
    if (m_replaces[index] == replaceableId) {
      if (m_textures[index]) {
        HandleClose(m_textures[index]);
      }
      m_textures[index] = static_cast<HTEXTURE>(HandleDuplicate(texture));
      ++numReplaced;
    }
  }

  return numReplaced;
}

void CRibbonEmitter::SetEnabled(int enable_) {
  m_enabled = enable_;
  if (!m_enabled) {
    m_posSet = 0;
  }
}

void CRibbonEmitter::SetTexSlot(UINT slot) {
  ASSERT(slot < m_rows * m_cols);
  if (m_texSlot != slot) {
    m_texSlot = slot;
    ConvertTexSlotToTexCoords();
  }
}

void CRibbonEmitter::SetAbove(float above) {
  ASSERT(above >= 0.0f);
  m_above = above;
}

void CRibbonEmitter::SetBelow(float below) {
  ASSERT(below >= 0.0f);
  m_below = below;
}

void CRibbonEmitter::SetGravity(float gravity) {
  m_gravity = gravity;
}

void CRibbonEmitter::SingletonMgrUpdate(float elapsedTime, const NTempest::C3Vector &cameraWorldPos, int suppressNewEdges) {
  if (!m_updated) {
    Update(elapsedTime, suppressNewEdges);
    m_singletonUpdated = 1;
  }
  m_updated = 0;
}

void CRibbonEmitter::SetPos(const NTempest::C44Matrix &orient, const NTempest::C3Vector &cameraPosition) {
  if (!m_enabled) {
    return;
  }

  m_cameraPos = cameraPosition;
  NTempest::C3Vector pos(orient.d0 + cameraPosition.x, orient.d1 + cameraPosition.y, orient.d2 + cameraPosition.z);
  if (m_posSet) {
    m_prevPos = m_currPos;
    m_prevDir = m_currDir;
    m_prevVertical = m_currVertical;
  } else {
    m_prevPos = pos;
    m_prevDir = NTempest::C3Vector(orient.c0, orient.c1, orient.c2);
    m_prevVertical = NTempest::C3Vector(orient.b0, orient.b1, orient.b2);
    m_startTime = 0.0f;
    m_posSet = 1;
  }
  m_currPos = pos;
  m_currDir = NTempest::C3Vector(orient.c0, orient.c1, orient.c2);
  m_currVertical = NTempest::C3Vector(orient.b0, orient.b1, orient.b2);
}

void CRibbonEmitter::SetMats(
    const TSGrowableArray<CRibbonMat> &materials,
    const TSGrowableArray<HTEXTURE>   &textures,
    const TSGrowableArray<UINT>       &replaces
) {
  ASSERT(materials.Count() == textures.Count());
  ASSERT(textures.Count() == replaces.Count());

  m_materials = materials;
  m_replaces = replaces;
  CloseTextureHandles();
  DuplicateTextureArray(textures, &m_textures);
}

void CRibbonEmitter::SetColor(const float r, const float g, const float b) {
  m_diffuseClr.Set(m_diffuseClr.a, NTempest::CMath::fuint_n(r * 255.0f), NTempest::CMath::fuint_n(g * 255.0f), NTempest::CMath::fuint_n(b * 255.0f));
}

void CRibbonEmitter::SetAlpha(const float a) {
  m_diffuseClr.a = NTempest::CMath::fuint_n(a * 255.0f);
}

void CRibbonEmitter::Update(float elapsedSec, int suppressNewEdges) {
  ASSERT(m_initialized);

  if (elapsedSec < 0.0f) {
    elapsedSec = 0.0f;
  } else if (elapsedSec > m_edgeLifeSpan) {
    elapsedSec = m_edgeLifeSpan;
  }

  while (m_readPos != m_writePos) {
    if (elapsedSec + m_edges[m_readPos] <= m_edgeLifeSpan) {
      break;
    }
    Advance(m_readPos, 1);
  }

  if (!suppressNewEdges && m_enabled && m_posSet) {
    float interpTime = elapsedSec * m_edgesPerSec + m_startTime;
    if (interpTime >= 1.0f) {
      float ooDenom = 1.0f / (interpTime - m_startTime);
      int   count = static_cast<int>(floor(interpTime - 1.0f)) + 1;

      InitInterpDeltas();
      float newEdgeTime = 1.0f;
      while (count) {
        float v0 = (newEdgeTime - m_startTime) * ooDenom;
        InterpEdge(-(v0 * elapsedSec), v0, 1);
        --count;
        newEdgeTime += 1.0f;
      }
    }

    m_startTime = interpTime - static_cast<float>(floor(interpTime));
    InterpEdge(0.0f, 1.0f, 0);

    CRibbonVertex &v0 = m_gxVertices[2 * m_writePos];
    v0.texCoord.x = m_texSlotBox.l;
    v0.texCoord.y = m_texSlotBox.t;

    CRibbonVertex &v1Vertex = m_gxVertices[2 * m_writePos + 1];
    v1Vertex.texCoord.x = m_texSlotBox.l;
    v1Vertex.texCoord.y = m_texSlotBox.b;
  }

  UINT start = m_readPos;
  while (start != m_writePos) {
    CRibbonVertex *v0 = &m_gxVertices[2 * start];
    CRibbonVertex *v1 = &m_gxVertices[2 * start + 1];

    float age = m_edges[start];
    float z = (age + age + elapsedSec) * m_gravity * elapsedSec;
    v0->pos.z += z;
    v1->pos.z += z;

    m_edges[start] += elapsedSec;
    float u = m_edges[start] * m_tmpDU * m_ooLifeSpan + m_texSlotBox.l;
    v0->texCoord.x = u;
    v0->texCoord.y = m_texSlotBox.t;
    v1->texCoord.x = u;
    v1->texCoord.y = m_texSlotBox.b;

    Advance(start, 1);
  }

  m_updated = 1;
  m_singletonUpdated = 0;
}

int CRibbonEmitter::Render() {
  ASSERT(m_initialized);

  if (m_readPos == m_writePos) {
    return 0;
  }

  NTempest::C44Matrix worldToCamera;
  worldToCamera.Translate(NTempest::C3Vector(-m_cameraPos.x, -m_cameraPos.y, -m_cameraPos.z));

  GxXformPush(GxXform_World, worldToCamera);
  GxVertexShaderSelect(GxVS_PassThru);

  GxPrimLockVertexPtrs(
      m_gxVertices.Count(), &m_gxVertices[0].pos, sizeof(CRibbonVertex), 0, 0, &m_diffuseClr, 0, 0, 0, &m_gxVertices[0].texCoord,
      sizeof(CRibbonVertex), 0, 0
  );

  UINT indexCount;
  if (m_readPos < m_writePos) {
    indexCount = 2 * (m_writePos - m_readPos) + 2;
  } else {
    indexCount = 2 * (m_writePos + m_edges.Count() - m_readPos) + 2;
  }

  GxPrimLockIndexPtr(GxPrim_TriangleStrip, indexCount, m_gxIndices.Ptr() + 2 * m_readPos);

  UINT numMaterials = m_materials.Count();
  UINT index;
  for (index = 0; index < numMaterials; ++index) {
    GxRsPush();
    GxRsSet(GxRs_Lighting, m_materials[index].enableLighting);
    GxRsSet(GxRs_Fog, m_materials[index].enableFog);
    GxRsSet(GxRs_DepthTest, m_materials[index].enableDepthTest);
    GxRsSet(GxRs_DepthWrite, m_materials[index].enableDepthWrite);
    GxRsSet(GxRs_Culling, m_materials[index].enableCulling);
    GxRsSet(GxRs_Blend, m_materials[index].alpha);
    GxRsSet(GxRs_Texture0, TextureGetGxTex(m_textures[index], 1, 0));
    GxPrimDrawElements();
    GxRsPop();
  }

  GxPrimUnlockIndexPtr();
  GxPrimUnlockVertexPtrs();
  GxXformPop(GxXform_World);
  return 1;
}

int CRibbonEmitter::IsDead() {
  return m_readPos == m_writePos;
}

void CRibbonEmitter::MaterialDisableLight(int disable) {
  UINT numMaterials = m_materials.Count();
  for (UINT i = 0; i < numMaterials; ++i) {
    m_materials[i].enableLighting = !disable;
  }
}

void CRibbonEmitter::MaterialDisableFog(int disable) {
  UINT numMaterials = m_materials.Count();
  for (UINT i = 0; i < numMaterials; ++i) {
    m_materials[i].enableFog = !disable;
  }
}

CRibbonEmitter *CRibbonEmitter::AddRef() {
  ++m_refCount;
  return this;
}

void CRibbonEmitter::DecRef() {
  if (!--m_refCount) {
    DEL(this);
  }
}
