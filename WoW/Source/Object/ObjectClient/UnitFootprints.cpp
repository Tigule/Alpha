#include "UnitFootprintsI.h"

#include "Client.h"
#include "Console/ConsoleCommand.h"
#include "Console/ConsoleVar.h"
#include "DB/DBClient/AutoCode/FootprintTexturesRec.h"
#include "DB/DBClient/AutoCode/TerrainTypeRec.h"
#include "DB/DBClient/AutoCode/UnitBloodRec.h"
#include "Unit_C.h"
#include "Ui/WorldFrame.h"
#include "WorldClient/World.h"

#include <Base/CDataAllocator.h>
#include <Base/Status.h>
#include <Gx/CGxDevice.h>
#include <Gx/Gx.h>
#include <Os/OsTime.h>
#include <Services/Texture.h>
#include <Tempest/crandom.h>

#include <math.h>
#include <new.h>

void ProjectTex2dMakeMatrices(
    NTempest::C44Matrix &texmat0,
    NTempest::C44Matrix &texmat1,
    const NTempest::CAaBox &box,
    const NTempest::C44Matrix *basis,
    float                fadeOffset,
    int                  inWorldSpace
);
CGxTex         *ProjectTex2dGetFade();
void UnitEffectOneShot(
    UNITEFFECTSPECIALS        effectNumber,
    unsigned __int64          target,
    const NTempest::C3Vector *attachPos,
    float                     facing,
    float                     scale,
    bool                      forceEffectOnMount
);

static NTempest::CRndSeed                 s_rndSeed;
static TSGrowableArray<PERSISTENTTEXTURE> s_footStepTextureTable;
static TSGrowableArray<TIMEDTEXTURE>      s_bloodSplatTextureTable[5];
static LISTBASE                          *s_currentList;
static CHUNKDATA                         *s_currentChunk;
static TSGrowableArray<int>               s_scratch;
static NTempest::C3Vector                 s_zup(0.0f, 0.0f, 1.0f);
static int                                s_currentTime;
static NTempest::C3Vector                 s_currentCamera;
static NTempest::C44Matrix                s_currentWorld;
static const float                        s_maxPurgeDist = 100.0f;
static NTempest::C2Vector                 s_splatSizes[5] = {
    NTempest::C2Vector(0.66666669f), NTempest::C2Vector(1.0f), NTempest::C2Vector(1.3333334f), NTempest::C2Vector(1.6666666f),
    NTempest::C2Vector(2.0f)
};
static TInstanceAllocator<CHUNKDATA> s_freeChunks(10);
static LISTDECLEX(SPLATDATA, normalLink, s_freeList);
static CVar                         *s_renderSplatsCVar;
static CVar                         *s_renderParticlesCVar;

static void InitializeBloodSplatTable() {
  int count = g_unitBloodDB.GetMaxID() + 1;
  for (int i = 0; i < 5; ++i) {
    s_bloodSplatTextureTable[i].SetCount(count);
    for (int j = 0; j < count; ++j) {
      const UnitBloodRec *rec = g_unitBloodDB.GetRecord(j);
      if (rec) {
        s_bloodSplatTextureTable[i][j].SetTexture(rec->m_GroundBlood[i]);
      }
    }
  }
}

static void InitializeTextureTable() {
  CStatus status;
  int     count = g_footprintTexturesDB.GetMaxID() + 1;
  s_footStepTextureTable.SetCount(count);
  for (int i = 0; i < g_footprintTexturesDB.GetNumRecords(); ++i) {
    const FootprintTexturesRec *rec = g_footprintTexturesDB.GetRecordByIndex(i);
    s_footStepTextureTable[rec->m_ID].SetTexture(rec->m_FootstepFilename);
  }
}

static NTempest::CAaBox MakeCAaBox(const NTempest::C2Vector &size, const NTempest::C3Vector &position) {
  float            radius = sqrtf(size.x * size.x + size.y * size.y) * 0.5f;
  NTempest::CAaBox box;
  box.b = position - NTempest::C3Vector(radius, radius, 0.3f);
  box.t = position + NTempest::C3Vector(radius, radius, 0.3f);
  return box;
}

static NTempest::C44Matrix MakeBasis(int mirror, float facing, const NTempest::C2Vector &size) {
  NTempest::C44Matrix basis;
  if (mirror) {
    basis.Scale(NTempest::C3Vector(-1.0f, 1.0f, 1.0f));
  }
  if (size.y > size.x) {
    basis.Scale(NTempest::C3Vector(1.0f, size.y / size.x, 1.0f));
  } else if (size.y < size.x) {
    basis.Scale(NTempest::C3Vector(size.x / size.y, 1.0f, 1.0f));
  }
  basis.Rotate(-facing, s_zup, 1);
  return basis;
}

static void ProjectTexRenderPNCT0T1(CGxBufCommand &cmd, CGxBuf *buf) {
  CGxVertexPNCT0T1 *vertices = 0;
  unsigned short   *indices = 0;
  if (cmd.vertex.op == GxBufOp_Fill) {
    vertices = static_cast<CGxVertexPNCT0T1 *>(*cmd.vertex.mem[GxVM_Position]);
  } else if (cmd.vertex.op == GxBufOp_Assign) {
    vertices = static_cast<CGxVertexPNCT0T1 *>(GxAllocVertexMem(buf->VertexCount() * sizeof(*vertices)));
    *cmd.vertex.mem[GxVM_Position] = &vertices->p;
    *cmd.vertex.mem[GxVM_Normal] = &vertices->n;
    *cmd.vertex.mem[GxVM_Color] = &vertices->c;
    *cmd.vertex.mem[GxVM_Texture0] = &vertices->tc[0];
    *cmd.vertex.mem[GxVM_Texture1] = &vertices->tc[1];
  } else {
    FATALASSERT(0);
  }
  if (cmd.index.op == GxBufOp_Fill) {
    indices = static_cast<unsigned short *>(*cmd.index.mem[GxVM_Indices]);
  } else if (cmd.index.op == GxBufOp_Assign) {
    indices = static_cast<unsigned short *>(GxAllocIndexMem(buf->IndexCount() * sizeof(*indices)));
    *cmd.index.mem[GxVM_Indices] = indices;
  } else {
    FATALASSERT(0);
  }

  int        vertsWritten = 0;
  ITERATELIST(SPLATDATA, s_currentChunk->m_splats, splat) {
    if (!splat->skip) {
      unsigned short *idx = splat->indices.Ptr();
      for (unsigned int i = 0; i < splat->indices.Count(); ++i) {
        *indices++ = static_cast<unsigned short>(vertsWritten + idx[i]);
      }
      for (unsigned int j = 0; j < splat->data.Count(); ++j) {
        vertices->p = splat->data[j].p;
        vertices->n = s_zup;
        vertices->c = splat->color;
        vertices->tc[0] = splat->data[j].t[0];
        vertices->tc[1] = splat->data[j].t[1];
        ++vertices;
        ++vertsWritten;
      }
    }
  }
}

bool SPLATDATA::Update(float progress, bool &nuke) {
  nuke = 0;
  NTempest::C3Vector delta = position - s_currentCamera;
  if (!data.Count() || !indices.Count() || delta.SquaredMag() >= s_maxPurgeDist * s_maxPurgeDist) {
    nuke = 1;
    return 1;
  }
  if (skip) {
    skip = 0;
    return 1;
  }
  if (startTime == -1) {
    int alpha = static_cast<int>((1.0f - progress) * 128.0f);
    color.a = alpha <= 0 ? 0 : alpha;
  } else {
    int elapsed = s_currentTime - startTime;
    if (elapsed < 0 || elapsed > 12050) {
      nuke = 1;
      return 1;
    }
    if (elapsed < 50) {
      color.a = static_cast<unsigned char>(elapsed * 0.02f * 128.0f);
    } else if (elapsed < 7050) {
      color.a = 128;
    } else if (elapsed < 12050) {
      color.a = static_cast<unsigned char>(128.0f - (elapsed - 7050) * 0.0002f * 128.0f);
    }
  }
  chunk->m_vertCount += data.Count();
  chunk->m_indexCount += indices.Count();
  return 0;
}

LISTBASE::LISTBASE(int m, int f) : m_texture(0), m_currentCount(-1), m_maxCount(m), m_flags(f) {
  FATALASSERT(m > 0);
}

LISTBASE::~LISTBASE() {
  if (m_texture) {
    HandleClose(m_texture);
  }
}

PERSISTENTTEXTURE::PERSISTENTTEXTURE() : LISTBASE(512, 1) {
}

void LISTBASE::SetTexture(const char *n) {
  CStatus status;
  if (!m_texture && n && *n) {
    m_texture = TextureCreate(n, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &status, 0);
  }
}

CHUNKDATA *LISTBASE::FindChunk(int id) {
  CHUNKDATA *chunk = m_chunks.Head();
  while (chunk && chunk->m_sourceID != id) {
    chunk = m_chunks.RawNext(chunk);
  }
  if (!chunk) {
    chunk = s_freeChunks.Get(0);
    chunk->m_sourceID = id;
    m_chunks.LinkNode(chunk, LIST_HEAD, 0);
  }
  chunk->m_flags |= m_flags;
  return chunk;
}

bool PERSISTENTTEXTURE::MakeSpace() {
  if (m_currentCount >= m_maxCount) {
    SPLATDATA *splat = m_splatOrder.Head();
    if (splat) {
      splat->chunk->RecycleSplat(splat);
    }
  }
  return 1;
}

bool TIMEDTEXTURE::MakeSpace() {
  return m_currentCount < m_maxCount;
}

static SPLATDATA *GetSplat() {
  SPLATDATA *splat = s_freeList.Head();
  if (splat) {
    s_freeList.UnlinkNode(splat);
  } else {
    void *storage = SMemAlloc(sizeof(SPLATDATA), __FILE__, __LINE__, 0);
    splat = storage ? new (storage) SPLATDATA : 0;
  }
  if (splat) {
    splat->skip = 0;
    splat->color = 0xFFFFFFFF;
  }
  return splat;
}

void CHUNKDATA::Render() {
  if (m_vertCount && m_indexCount) {
    NTempest::C44Matrix batchMtx = m_matrix * s_currentWorld;
    GxXformSet(GxXform_World, batchMtx);
    CGxBuf *buf = GxBufGetDynamic(GxVBF_PNCT0T1);
    buf->UserCallbackSet(ProjectTexRenderPNCT0T1);
    buf->CountSet(m_vertCount, m_indexCount);
    GxBufLock(buf);
    CGxBatch batch(GxPrim_Triangles, m_indexCount, 0, -1, -1);
    GxBufRender(batch);
    GxBufUnlock();
  }
}

int CHUNKDATA::GetVertCount(const CWTriData::Batch &batch, int &lowest, int &highest) {
  int indexCount = batch.GetIndexCount();
  if (!indexCount) {
    return 0;
  }
  lowest = 0x7FFFFFFF;
  highest = -1;
  s_scratch.SetCount(batch.maxIndex + 1);
  for (unsigned int i = 0; i < s_scratch.Count(); ++i) {
    s_scratch[i] = -1;
  }
  int found = 0;
  for (int j = 0; j < indexCount; ++j) {
    unsigned int index = batch.GetIndex(j);
    if (s_scratch[index] == -1) {
      ++found;
    }
    ++s_scratch[index];
    if (static_cast<int>(index) < lowest)
      lowest = index;
    if (static_cast<int>(index) > highest)
      highest = index;
  }
  return found;
}

CHUNKDATA::~CHUNKDATA() {
  while (m_splats.Head()) {
    RecycleSplat(m_splats.Head());
  }
  FATALASSERT(!m_vertCount);
  FATALASSERT(!m_indexCount);
  Unlink();
}

SPLATDATA *CHUNKDATA::Add(
    const CWTriData::Batch &batch, const NTempest::CAaBox &box, const NTempest::C44Matrix &basis) {
  int lowest;
  int highest;
  int vertCount = GetVertCount(batch, lowest, highest);
  if (!vertCount) {
    return 0;
  }
  SPLATDATA *splat = GetSplat();
  if (!splat) {
    return 0;
  }
  splat->startTime = (m_flags & 1) ? -1 : OsGetAsyncTimeMs();
  splat->position = (box.b + box.t) * 0.5f;
  splat->chunk = this;

  NTempest::C44Matrix tex0;
  NTempest::C44Matrix tex1;
  ProjectTex2dMakeMatrices(tex0, tex1, box, &basis, 0.5f, 1);
  tex0 = *batch.matrix * tex0;
  tex1 = *batch.matrix * tex1;

  for (int i = lowest, localIndex = 0; i <= highest; ++i) {
    if (s_scratch[i] >= 0) {
      s_scratch[i] = localIndex++;
    }
  }
  splat->data.SetCount(vertCount);
  splat->indices.SetCount(batch.GetIndexCount());
  for (int j = 0; j < batch.GetIndexCount(); ++j) {
    unsigned short sourceIndex = batch.GetIndex(j);
    unsigned short localIndex = static_cast<unsigned short>(s_scratch[sourceIndex]);
    splat->indices[j] = localIndex;
    NTempest::C3Vector source = batch.GetVertex(sourceIndex);
    splat->data[localIndex].p = source;
    NTempest::C3Vector tc0 = source * tex0;
    NTempest::C3Vector tc1 = source * tex1;
    splat->data[localIndex].t[0] = NTempest::C2Vector(tc0.x, tc0.y);
    splat->data[localIndex].t[1] = NTempest::C2Vector(tc1.x, tc1.y);
  }
  m_splats.LinkNode(splat, LIST_HEAD, 0);
  ++m_numSplats;
  m_matrix = *batch.matrix;
  return splat;
}

void CHUNKDATA::RecycleSplat(SPLATDATA *splat) {
  if (!splat) {
    return;
  }
  splat->data.SetCount(0);
  splat->indices.SetCount(0);
  splat->orderLink.Unlink();
  splat->chunk = 0;
  s_freeList.LinkNode(splat, LIST_TAIL, 0);
}

void LISTBASE::Add(const NTempest::C3Vector &position, const NTempest::CAaBox &box, const NTempest::C44Matrix &matrix) {
  if (!m_texture) {
    return;
  }
  NTempest::C3Vector cameraPos;
  CGWorldFrame::GetCameraPosition(&cameraPos);
  if ((cameraPos - position).SquaredMag() >= 100.0f * 100.0f) {
    return;
  }
  CWTriData data;
  if (!CWorld::GetTris(box, data, 0x122)) {
    return;
  }
  for (int i = data.GetNumBatches(); i;) {
    const CWTriData::Batch &batch = data.GetBatch(--i);
    if (!MakeSpace()) {
      break;
    }
    CHUNKDATA *chunk = FindChunk(batch.sourceID);
    SPLATDATA *splat = chunk->Add(batch, box, matrix);
    if (splat) {
      m_splatOrder.LinkNode(splat, LIST_HEAD, 0);
    }
  }
}

void LISTBASE::Render() {
  if (!m_texture) {
    return;
  }
  CGxTex *texture = TextureGetGxTex(m_texture, 0, 0);
  if (!texture || m_chunks.IsEmpty()) {
    return;
  }
  GxRsSet(GxRs_Texture0, texture);
  s_currentList = this;
  {
    ITERATELIST(CHUNKDATA, m_chunks, chunk) {
      chunk->m_vertCount = 0;
      chunk->m_indexCount = 0;
    }
  }
  m_currentCount = 0;
  int found = 0;
  for (SPLATDATA *splat = m_splatOrder.Tail(); splat;) {
    SPLATDATA   *newTail = m_splatOrder.Prev(splat);
    bool nuke;
    if (splat->Update(static_cast<float>(found) / m_maxCount, nuke) && nuke) {
      splat->chunk->RecycleSplat(splat);
    } else {
      ++m_currentCount;
      ++found;
    }
    splat = newTail;
  }
  {
    ITERATELIST(CHUNKDATA, m_chunks, renderChunk) {
      s_currentChunk = renderChunk;
      renderChunk->Render();
    }
  }
  s_currentChunk = 0;
  s_currentList = 0;
}

void UnitFootprintInitialize() {
  s_renderSplatsCVar = CVar::Register("showfootprints", "toggles rendering of unit footprint splats", 1, "1", 0, GRAPHICS, false, 0);
  s_renderParticlesCVar = CVar::Register("showfootprintparticles", "toggles rendering of footprint particles", 1, "1", 0, GRAPHICS, false, 0);
  InitializeTextureTable();
  InitializeBloodSplatTable();
}

void UnitFootprintShutdown() {
  s_footStepTextureTable.Clear();
  for (int i = 0; i < 5; ++i) {
    s_bloodSplatTextureTable[i].Clear();
  }
}

void UnitFootprintNewSplat(
    unsigned int        textureID,
    const NTempest::C2Vector &size,
    const NTempest::C3Vector &position,
    float               facing,
    int                 mirrorLength,
    unsigned int        terrain
) {
  const TerrainTypeRec *rec = g_terrainTypeDB.GetRecord(terrain);
  if (rec && s_renderSplatsCVar->GetInt() && (rec->m_Flags & 1) && textureID < s_footStepTextureTable.Count()) {
    NTempest::C44Matrix basis = MakeBasis(mirrorLength, facing, size);
    NTempest::CAaBox    box = MakeCAaBox(size, position);
    s_footStepTextureTable[textureID].Add(position, box, basis);
  }
}

void UnitFootprintNewBloodSplat(const UnitBloodRec *rec, unsigned int unitSize, const NTempest::C3Vector &position) {
  if (!s_renderSplatsCVar->GetInt()) {
    return;
  }
  FATALASSERT(rec);
  FATALASSERT(unitSize < 5);
  float               facing = NTempest::CRandom::real_(s_rndSeed) * 6.2831855f;
  float               sizeVariance = NTempest::CRandom::real_(s_rndSeed) * 0.2f + 1.0f;
  NTempest::C2Vector  size(s_splatSizes[unitSize].x * sizeVariance, s_splatSizes[unitSize].y * sizeVariance);
  unsigned int        texture = NTempest::CMath::mulhwu_(5, NTempest::CRandom::uint32_(s_rndSeed));
  TIMEDTEXTURE       &list = s_bloodSplatTextureTable[texture][rec->m_ID];
  NTempest::C44Matrix basis = MakeBasis(OsGetAsyncTimeMs() & 1, facing, size);
  NTempest::CAaBox    box = MakeCAaBox(size, position);
  list.Add(position, box, basis);
}

void UnitFootprintPlayParticle(CGUnit_C *unit, const NTempest::C3Vector &position, unsigned int terrainID, float scale) {
  if (unit && s_renderParticlesCVar->GetInt()) {
    NTempest::C3Vector waterDir(0.0f);
    NTempest::C3Vector splashPos;
    int                deep;
    unsigned int       liquid;
    float              surfaceColPt;
    float              depth;
    unsigned int       effect;

    int inLiquid = CWorld::QueryObjectLiquid(unit->GetWorldObject(), liquid, surfaceColPt, waterDir, deep);
    depth = surfaceColPt - unit->GetPosition().z;
    if (inLiquid && (liquid & 3) != 2 && unit->GetObjectHeight() * 0.5f > depth) {
      splashPos = position;
      splashPos.z += depth;
      effect = unit->IsWalking() + 35;
      UnitEffectOneShot(static_cast<UNITEFFECTSPECIALS>(effect), unit->GetGUID(), &splashPos, unit->GetFacing(), scale, false);
    }

    const TerrainTypeRec *rec = g_terrainTypeDB.GetRecord(terrainID);
    if (rec) {
      effect = unit->IsWalking() ? rec->m_FootstepSprayWalk : rec->m_FootstepSprayRun;
      if (effect != static_cast<unsigned int>(-1)) {
        UnitEffectOneShot(static_cast<UNITEFFECTSPECIALS>(effect), unit->GetGUID(), &position, unit->GetFacing(), scale, false);
      }
    }
  }
}

void UnitFootprintRenderSplats(const NTempest::C3Vector &cameraPos) {
  if (!s_renderSplatsCVar || !s_renderSplatsCVar->GetInt()) {
    return;
  }
  s_currentTime = OsGetAsyncTimeMs();
  s_currentCamera = cameraPos;
  s_currentWorld = NTempest::C44Matrix();
  s_currentWorld.Translate(-cameraPos);
  GxVertexShaderSelect(GxVS_PassThru);
  GxRsPush();
  GxRsSet(GxRs_Texture1, ProjectTex2dGetFade());
  GxRsSet(GxRs_Blend, GxBlend_Alpha);
  GxRsSet(GxRs_DepthWrite, 0);
  GxRsSet(GxRs_Culling, 0);
  GxRsSet(GxRs_PolygonOffset, 0.0625f);
  GxXformPush(GxXform_World);
  for (int i = s_footStepTextureTable.Count(); i;) {
    s_footStepTextureTable[--i].Render();
  }
  for (int j = 0; j < 5; ++j) {
    for (int i = s_bloodSplatTextureTable[j].Count(); i;) {
      s_bloodSplatTextureTable[j][--i].Render();
    }
  }
  GxXformPop(GxXform_World);
  GxRsPop();
}
