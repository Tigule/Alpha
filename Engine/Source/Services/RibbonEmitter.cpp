#include "RibbonEmitter.h"

#include "Tempest/c44matrix.h"

#include <math.h>

static const float MIN_EDGE_LIFE_SPAN = 0.25f;
static const float NORMAL_SCALE = 1.0f;

static void DuplicateTextureArray(const TSGrowableArray<HTEXTURE> &src, TSGrowableArray<HTEXTURE> *dst) {
  UINT numTextures = src.Count();
  dst->SetCount(numTextures);

  for (UINT i = 0; i < numTextures; ++i) {
    (*dst)[i] = (HTEXTURE)HandleDuplicate(src[i]);
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
  float scale = (m_prevPos - m_currPos).Mag() * NORMAL_SCALE;

  m_below0 = m_prevPos - m_prevVertical * m_below;
  m_below1 = m_currPos - m_currVertical * m_below;
  m_above0 = m_prevPos + m_prevVertical * m_above;
  m_above1 = m_currPos + m_currVertical * m_above;
  m_prevDirScaled = m_prevDir * scale;
  m_currDirScaled = m_currDir * scale;
}

void CRibbonEmitter::InterpEdge(float age, float t, UINT advance) {
  CRibbonVertex &v0 = m_gxVertices[2 * m_writePos];
  CRibbonVertex &v1 = m_gxVertices[2 * m_writePos + 1];
  float          w0 = 1.0f - t;

  v0.pos = (t * m_prevDirScaled + m_below0) * w0 + (m_below1 - (1.0f - t) * m_currDirScaled) * t;
  v1.pos = (t * m_prevDirScaled + m_above0) * w0 + (m_above1 - (1.0f - t) * m_currDirScaled) * t;

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

  ASSERT(m_initialized == 0);
  ASSERT(edgesPerSec >= 1);
  ASSERT(edgeLifeSpanInSec > 0);
  ASSERT(materials.Count() == textures.Count());
  ASSERT(textures.Count() == replaces.Count());

  edgesPerSec = ceilf(edgesPerSec);
  if (MIN_EDGE_LIFE_SPAN > edgeLifeSpanInSec) {
    edgeLifeSpanInSec = MIN_EDGE_LIFE_SPAN;
  }

  numEdges = ceilf(edgesPerSec * edgeLifeSpanInSec) + 2.0f;
  m_edges.SetCount(numEdges);
  m_readPos = 0;
  m_writePos = 0;
  m_startTime = 0.0f;
  m_posSet = 0;

  m_gxVertices.SetCount(2 * numEdges);

  m_gxIndices.SetCount(4 * numEdges);
  count = m_gxIndices.Count();
  for (UINT index = 0; index != count; ++index) {
    t = index % (2 * numEdges);
    m_gxIndices[index] = t;
  }

  m_ooLifeSpan = 1.0f / edgeLifeSpanInSec;
  m_tmpDU = texBox.Width() / cols;
  m_tmpDV = texBox.Height() / rows;
  m_edgesPerSec = edgesPerSec;
  m_edgeLifeSpan = edgeLifeSpanInSec;
  m_ooTmpDU = 1.0f / m_tmpDU;
  m_ooTmpDV = 1.0f / m_tmpDV;
  m_diffuseClr = diffuseClr;

  m_materials = materials;
  DuplicateTextureArray(textures, &m_textures);
  m_replaces = replaces;

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

void CRibbonEmitter::SetPos(const NTempest::C44Matrix &orient, const NTempest::C3Vector &cameraPosition) {
  if (!m_enabled) {
    return;
  }

  m_cameraPos = cameraPosition;
  NTempest::C3Vector pos = *orient.Row3AsVec3() + cameraPosition;
  if (m_posSet) {
    m_prevPos = m_currPos;
    m_prevDir = m_currDir;
    m_prevVertical = m_currVertical;
  } else {
    m_prevPos = pos;
    m_prevDir = *orient.Row2AsVec3();
    m_prevVertical = *orient.Row1AsVec3();
    m_startTime = 0.0f;
    m_posSet = 1;
  }
  m_currPos = pos;
  m_currDir = *orient.Row2AsVec3();
  m_currVertical = *orient.Row1AsVec3();
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

UINT CRibbonEmitter::ReplaceTexture(UINT replaceableId, HTEXTURE texture) {
  UINT numReplaced = 0;

  for (UINT index = 0; index < m_replaces.Count(); ++index) {
    if (m_replaces[index] == replaceableId) {
      if (m_textures[index]) {
        HandleClose(m_textures[index]);
      }
      m_textures[index] = (HTEXTURE)HandleDuplicate(texture);
      ++numReplaced;
    }
  }

  return numReplaced;
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

void CRibbonEmitter::SetColor(const float r, const float g, const float b) {
  m_diffuseClr.Set(m_diffuseClr.a * 255.0f, r, g, b);
}

void CRibbonEmitter::SetAlpha(const float a) {
  m_diffuseClr.a = NTempest::CMath::fuint_n(a * 255.0f);
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

void CRibbonEmitter::Update(float elapsedSec, int suppressNewEdges) {
  ASSERT(m_initialized);

  elapsedSec = NTempest::CMath::clamp_(elapsedSec, 0.0f, m_edgeLifeSpan);

  while (m_readPos != m_writePos && elapsedSec + m_edges[m_readPos] > m_edgeLifeSpan) {
    Advance(m_readPos, 1);
  }

  if (!suppressNewEdges && m_enabled && m_posSet) {
    float endTime = elapsedSec * m_edgesPerSec + m_startTime;
    float newEdgeTime = 1.0f;
    if (endTime >= 1.0f) {
      const float ooDenom = 1.0f / (endTime - m_startTime);
      int         count = (int)floor(endTime - 1.0f) + 1;

      InitInterpDeltas();
      while (count) {
        float interpTime = (newEdgeTime - m_startTime) * ooDenom;
        InterpEdge(-(interpTime * elapsedSec), interpTime, 1);
        --count;
        newEdgeTime += 1.0f;
      }
    }

    m_startTime = endTime - floorf(endTime);
    InterpEdge(0.0f, 1.0f, 0);

    m_gxVertices[2 * m_writePos].texCoord = NTempest::C2Vector(m_texSlotBox.l, m_texSlotBox.t);
    m_gxVertices[2 * m_writePos + 1].texCoord = NTempest::C2Vector(m_texSlotBox.l, m_texSlotBox.b);
  }

  UINT start = m_readPos;
  while (start != m_writePos) {
    CRibbonVertex &v0 = m_gxVertices[2 * start];
    CRibbonVertex &v1 = m_gxVertices[2 * start + 1];

    float z = (2.0f * m_edges[start] + elapsedSec) * m_gravity * elapsedSec;
    v0.pos.z += z;
    v1.pos.z += z;

    m_edges[start] += elapsedSec;
    float u = m_edges[start] * m_tmpDU * m_ooLifeSpan + m_texSlotBox.l;
    v0.texCoord = NTempest::C2Vector(u, m_texSlotBox.t);
    v1.texCoord = NTempest::C2Vector(u, m_texSlotBox.b);

    Advance(start, 1);
  }

  m_updated = 1;
  m_singletonUpdated = 0;
}

BOOL CRibbonEmitter::Render() {
  ASSERT(m_initialized);

  if (m_readPos == m_writePos) {
    return 0;
  }

  NTempest::C44Matrix worldToCamera;
  worldToCamera.Translate(-m_cameraPos);
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

BOOL CRibbonEmitter::IsDead() {
  return m_readPos == m_writePos;
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
