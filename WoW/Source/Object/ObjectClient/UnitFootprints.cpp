#include <Base/Base.h>
#include <Gx/Gx.h>
#include <MapDefs.h>
#include <WorldClient/World.h>
#include "Net/NetClient/NetClient.h"
#include <Frame/CSimpleTop.h>
#include "Object/ObjectClient/Unit_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "SoundInterface/SoundInterface.h"
#include "UIUtil/InputControl.h"
#include "Ui/WorldFrame.h"
#include "Ui/GameUI.h"

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
#include <Gx/Gx.h>
#include <Os/OsTime.h>
#include <Services/Texture.h>
#include <Tempest/crandom.h>

#include <math.h>
#include <new.h>


void ProjectTex2dMakeMatrices(
    NTempest::C44Matrix       &texmat0,
    NTempest::C44Matrix       &texmat1,
    const NTempest::CAaBox    &box,
    const NTempest::C44Matrix *basis,
    float                      fadeOffset,
    int                        inWorldSpace
);
CGxTex *ProjectTex2dGetFade();
void    UnitEffectOneShot(
    UNITEFFECTSPECIALS        effectNumber,
    DWORDLONG                 target,
    const NTempest::C3Vector *attachPos,
    float                     facing,
    float                     scale,
    bool                      forceEffectOnMount
);

enum {
  CHUNKFLAG_PERSISTENT = 1
};

static LISTDECLEX(SPLATDATA, normalLink, s_freeList);
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
static const int                          FADEIN = 50;
static const int                          FADEDONE = 12050;
static const int                          FADEOUT = 7050;
static const float                        MAXALPHA = 128.0f;
static const NTempest::C2Vector           s_splatSizes[5] = {
    NTempest::C2Vector(0.44444445f), NTempest::C2Vector(0.6666667f), NTempest::C2Vector(1.1666666f), NTempest::C2Vector(1.3333334f),
    NTempest::C2Vector(1.7777778f)
};
static TInstanceAllocator<CHUNKDATA> s_freeChunks(100);
static CVar *s_renderSplatsCVar;
static CVar *s_renderParticlesCVar;

static void InitializeBloodSplatTable() {
  int count = g_unitBloodDB.GetMaxID() + 1;
  for (int j = 5; j;) {
    --j;
    s_bloodSplatTextureTable[j].SetCount(count);
    for (int i = g_unitBloodDB.GetNumRecords(); i;) {
      --i;
      const UnitBloodRec *rec = g_unitBloodDB.GetRecordByIndex(i);
      if (rec) {
        s_bloodSplatTextureTable[j][rec->m_ID].SetTexture(rec->m_GroundBlood[j]);
      }
    }
  }
}

static void InitializeTextureTable() {
  s_footStepTextureTable.SetCount(g_footprintTexturesDB.GetMaxID() + 1);
  CStatus status;
  for (int i = g_footprintTexturesDB.GetNumRecords(); i;) {
    --i;
    const FootprintTexturesRec *rec = g_footprintTexturesDB.GetRecordByIndex(i);
    s_footStepTextureTable[rec->m_ID].SetTexture(rec->m_FootstepFilename);
  }
}

static NTempest::CAaBox MakeCAaBox(const NTempest::C2Vector &size, const NTempest::C3Vector &position) {
  float            radius = size.Mag() * 0.5f;
  NTempest::CAaBox box(position);
  box.b -= NTempest::C3Vector(radius, radius, 0.3f);
  box.t += NTempest::C3Vector(radius, radius, 0.3f);
  return box;
}

static NTempest::C44Matrix MakeBasis(int mirror, float facing, const NTempest::C2Vector &size) {
  NTempest::C44Matrix basis;
  if (mirror) {
    basis.Scale(NTempest::C3Vector(-1.0f, 1.0f, 1.0f));
  }
  if (size.x < size.y) {
    basis.Scale(NTempest::C3Vector(1.0f, size.y / size.x, 1.0f));
  } else if (size.y < size.x) {
    basis.Scale(NTempest::C3Vector(size.x / size.y, 1.0f, 1.0f));
  }
  basis.Rotate(-facing, NTempest::C3Vector(0.0f, 0.0f, 1.0f), 1);
  return basis;
}

static void ProjectTexRenderPNCT0T1(CGxBufCommand &cmd, CGxBuf *buf) {
  CGxVertexPNCT0T1 *vertices = 0;
  WORD             *idx = 0;
  if (cmd.vertex.op == GxBufOp_Nop || cmd.index.op == GxBufOp_Nop) {
    FATALASSERT(0);
    return;
  }
  switch (cmd.vertex.op) {
    case GxBufOp_Assign:
      vertices = (CGxVertexPNCT0T1 *)GxAllocVertexMem(buf->VertexCount() * sizeof(*vertices));
      *cmd.vertex.mem[GxVM_Position] = &vertices->p;
      *cmd.vertex.mem[GxVM_Normal] = &vertices->n;
      *cmd.vertex.mem[GxVM_Color] = &vertices->c;
      *cmd.vertex.mem[GxVM_Texture0] = &vertices->tc[0];
      *cmd.vertex.mem[GxVM_Texture1] = &vertices->tc[1];
      break;
    case GxBufOp_Fill:
      vertices = (CGxVertexPNCT0T1 *)*cmd.vertex.mem[GxVM_Position];
      break;
  }
  switch (cmd.index.op) {
    case GxBufOp_Assign:
      idx = (WORD *)GxAllocIndexMem(buf->IndexCount() * sizeof(*idx));
      *cmd.index.mem[GxVM_Indices] = idx;
      break;
    case GxBufOp_Fill:
      idx = (WORD *)*cmd.index.mem[GxVM_Indices];
      break;
  }

  int vertsWritten = 0;
  for (SPLATDATA *splat = s_currentChunk->m_splats.Head(); splat; splat = splat->normalLink.Next()) {
    if (!splat->skip) {
      int i;
      for (i = 0; i < (int)splat->indices.Count(); ++i, ++idx) {
        *idx = splat->indices[i] + vertsWritten;
      }
      for (i = 0; i < (int)splat->data.Count(); ++i, ++vertices, ++vertsWritten) {
        vertices->p = splat->data[i].p;
        vertices->n = s_zup;
        vertices->c = splat->color;
        vertices->tc[0] = splat->data[i].t[0];
        vertices->tc[1] = splat->data[i].t[1];
      }
    }
  }
}

bool SPLATDATA::Update(float progress, bool &nuke) {
  nuke = 0;
  if (!data.Count() || !indices.Count()) {
    nuke = 1;
    return 1;
  }

  NTempest::C3Vector diff = position - s_currentCamera;
  if (diff.SquaredMag() > s_maxPurgeDist * s_maxPurgeDist) {
    nuke = 1;
    return 1;
  }
  if (skip) {
    skip = 0;
    return 1;
  }
  if (startTime == -1) {
    static int MAXALPHA = 128;
    int        alpha = (1.0 - progress) * MAXALPHA;
    color.a = alpha <= 0 ? 0 : alpha;
  } else {
    int elapsed = s_currentTime - startTime;
    if (elapsed < 0 || elapsed > FADEDONE) {
      nuke = 1;
      return 1;
    }
    if (elapsed < FADEIN) {
      color.a = MAXALPHA * ((float)elapsed / FADEIN);
    } else if (elapsed < FADEOUT) {
      color.a = MAXALPHA;
    } else if (elapsed < FADEDONE) {
      color.a = MAXALPHA - MAXALPHA * ((float)(elapsed - FADEOUT) / (FADEDONE - FADEOUT));
    }
  }
  chunk->m_vertCount += data.Count();
  chunk->m_indexCount += indices.Count();
  return 0;
}

void LISTBASE::SetTexture(LPCSTR n) {
  CStatus status;
  if (!m_texture && n && *n) {
    m_texture = TextureCreate(n, CGxTexFlags(GxTex_LinearMipNearest, 0, 0, 0, 0, 0, 1), &status, 0);
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
    SPLATDATA *newTail = splat->orderLink.Prev();
    bool       nuke;
    if (splat->Update((float)found / m_maxCount, nuke) && nuke) {
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

CHUNKDATA *LISTBASE::FindChunk(int id) {
  CHUNKDATA *chunk = 0;
  ITERATELIST(CHUNKDATA, m_chunks, search) {
    if (search->m_sourceID == id) {
      chunk = search;
      break;
    }
  }
  if (!chunk) {
    chunk = s_freeChunks.Get(0);
    chunk->m_sourceID = id;
    m_chunks.LinkNode(chunk, LIST_TAIL, 0);
  }
  chunk->m_flags |= m_flags;
  return chunk;
}

void LISTBASE::Add(const NTempest::C3Vector &position, const NTempest::CAaBox &box, const NTempest::C44Matrix &matrix) {
  if (!m_texture) {
    return;
  }
  NTempest::C3Vector cameraPos;
  CGWorldFrame::GetCameraPosition(&cameraPos);
  cameraPos -= position;
  if (cameraPos.SquaredMag() >= s_maxPurgeDist * s_maxPurgeDist) {
    return;
  }
  CWTriData data;
  if (!CWorld::GetTris(box, data, 0x122)) {
    return;
  }
  for (int i = data.GetNumBatches(); i;) {
    --i;
    if (!MakeSpace()) {
      break;
    }
    const CWTriData::Batch &batch = data.GetBatch(i);
    CHUNKDATA              *chunk = FindChunk(batch.sourceID);
    SPLATDATA              *splat = chunk->Add(batch, box, matrix);
    if (splat) {
      m_splatOrder.LinkNode(splat, LIST_TAIL, 0);
    }
  }
}

bool TIMEDTEXTURE::MakeSpace() {
  return m_currentCount < m_maxCount;
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

SPLATDATA *GetSplat() {
  SPLATDATA *splat = s_freeList.Head();
  if (splat) {
    s_freeList.UnlinkNode(splat);
  } else {
    splat = NEW(SPLATDATA);
  }
  splat->skip = 0;
  splat->color = NTempest::CImVector(0xFFFFFFFF);
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
    GxBufRender(CGxBatch(GxPrim_Triangles, m_indexCount, 0, -1, -1));
    GxBufUnlock();
  }
}

int CHUNKDATA::GetVertCount(const CWTriData::Batch &batch, int &lowest, int &highest) {
  int indexCount = batch.GetIndexCount();
  if (!indexCount) {
    return 0;
  }
  highest = -1;
  lowest = 0x7FFFFFFF;
  s_scratch.SetCount(batch.maxIndex + 1);
  for (UINT i = s_scratch.Count(); i;) {
    s_scratch[--i] = -1;
  }
  int found = 0;
  for (int j = 0; j < indexCount; ++j) {
    UINT index = batch.vertexIndices[j];
    if (s_scratch[index] == -1) {
      ++found;
    }
    ++s_scratch[index];
    highest = max(highest, batch.vertexIndices[j]);
    lowest = min(lowest, batch.vertexIndices[j]);
  }
  return found;
}

SPLATDATA *CHUNKDATA::Add(const CWTriData::Batch &batch, const NTempest::CAaBox &box, const NTempest::C44Matrix &basis) {
  int lowest;
  int highest;
  int vertCount = GetVertCount(batch, lowest, highest);
  if (!vertCount) {
    return 0;
  }
  SPLATDATA *splat = GetSplat();
  if (m_flags & 1) {
    splat->startTime = -1;
  } else {
    splat->startTime = OsGetAsyncTimeMs();
  }
  splat->position = (box.b + box.t) * 0.5f;
  splat->chunk = this;

  NTempest::C44Matrix tex0;
  NTempest::C44Matrix tex1;
  ProjectTex2dMakeMatrices(tex0, tex1, box, &basis, 0.5f, 1);
  tex0 = *batch.matrix * tex0;
  tex1 = *batch.matrix * tex1;

  for (int i = lowest, localIndex = 0; i <= highest; ++i) {
    if (s_scratch[i] > -1) {
      s_scratch[i] = localIndex++;
    }
  }
  splat->data.SetCount(vertCount);
  splat->indices.SetCount(batch.GetIndexCount());
  for (int j = 0; j < (int)splat->indices.Count(); ++j) {
    WORD sourceIndex = batch.vertexIndices[j];
    WORD localIndex = s_scratch[sourceIndex];
    splat->indices[j] = localIndex;
    NTempest::C3Vector source = batch.vertices[sourceIndex];
    VERTDATA          &vert = splat->data[localIndex];
    vert.p = source;
    vert.t[0] = source * tex0;
    vert.t[1] = source * tex1;
  }
  m_splats.LinkNode(splat, LIST_TAIL, 0);
  m_matrix = *batch.matrix;
  return splat;
}

CHUNKDATA::~CHUNKDATA() {
  for (SPLATDATA *splat = m_splats.Head(); splat; splat = m_splats.Head()) {
    RecycleSplat(splat);
  }
  FATALASSERT(!m_vertCount);
  FATALASSERT(!m_indexCount);
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

void UnitFootprintInitialize() {
  s_renderSplatsCVar = CVar::Register("showfootprints", "toggles rendering of unit footprint splats", CVar::ARCHIVE, "1", 0, GRAPHICS, false, 0);
  s_renderParticlesCVar = CVar::Register("showfootprintparticles", "toggles rendering of footprint particles", CVar::ARCHIVE, "1", 0, GRAPHICS, false, 0);
  InitializeTextureTable();
  InitializeBloodSplatTable();
}

void UnitFootprintShutdown() {
  s_footStepTextureTable.Clear();
  for (int i = 5; i;) {
    --i;
    s_bloodSplatTextureTable[i].Clear();
  }
}

void UnitFootprintNewSplat(
    UINT                      textureID,
    const NTempest::C2Vector &size,
    const NTempest::C3Vector &position,
    float                     facing,
    int                       mirrorLength,
    UINT                      terrain
) {
  const TerrainTypeRec *rec = g_terrainTypeDB.GetRecord(terrain);
  if (rec && s_renderSplatsCVar->GetInt() && (rec->m_Flags & 1) && textureID < s_footStepTextureTable.Count()) {
    s_footStepTextureTable[textureID].Add(position, MakeCAaBox(size, position), MakeBasis(mirrorLength, facing, size));
  }
}

void UnitFootprintNewBloodSplat(const UnitBloodRec *rec, UINT unitSize, const NTempest::C3Vector &position) {
  if (!s_renderSplatsCVar->GetInt()) {
    return;
  }
  FATALASSERT(rec);
  FATALASSERT(unitSize < 5);
  static NTempest::CRndSeed s_rndSeed;
  float                     facing = NTempest::CRandom::reals_(s_rndSeed) * TWO_PI;
  float                     sizeVariance = NTempest::CRandom::reals_(s_rndSeed) * 0.2f + 1.0f;
  NTempest::C3Vector        size = sizeVariance * s_splatSizes[unitSize];
  s_bloodSplatTextureTable[NTempest::CRandom::dice_(5, s_rndSeed)][rec->m_ID].Add(
      position, MakeCAaBox(size, position), MakeBasis(OsGetAsyncTimeMs() & 1, facing, size)
  );
}

void UnitFootprintPlayParticle(CGUnit_C *unit, const NTempest::C3Vector &position, UINT terrainID, float scale) {
  if (unit && s_renderParticlesCVar->GetInt()) {
    NTempest::C3Vector waterDir(0.0f);
    int                deep;
    UINT               liquid;
    float              surfaceColPt;
    float              depth;
    UINT               effect;

    int inLiquid = CWorld::QueryObjectLiquid(unit->GetWorldObject(), liquid, surfaceColPt, waterDir, deep);
    depth = surfaceColPt - unit->GetPosition().z;
    if (inLiquid && (liquid & 3) != 2 && unit->GetCollisionBoxHeight() * 0.5f > depth) {
      effect = unit->IsWalking() ? SPECIALEFFECT_FOOTSTEPSPRAYWATERWALK : SPECIALEFFECT_FOOTSTEPSPRAYWATER;
      NTempest::C3Vector splashPos = position;
      splashPos.z += depth;
      UnitEffectOneShot((UNITEFFECTSPECIALS)effect, unit->GetGUID(), &splashPos, unit->GetFacing(), scale, false);
    }

    const TerrainTypeRec *rec = g_terrainTypeDB.GetRecord(terrainID);
    if (rec) {
      effect = unit->IsWalking() ? rec->m_FootstepSprayWalk : rec->m_FootstepSprayRun;
      if (effect != (UINT)-1) {
        UnitEffectOneShot((UNITEFFECTSPECIALS)effect, unit->GetGUID(), &position, unit->GetFacing(), scale, false);
      }
    }
  }
}

void UnitFootprintRenderSplats(const NTempest::C3Vector &cameraPos) {
  if (!s_renderSplatsCVar->GetInt()) {
    return;
  }
  s_currentTime = OsGetAsyncTimeMs();
  s_currentCamera = cameraPos;
  s_currentWorld.Identity();
  *s_currentWorld.Row3AsVec3() = -cameraPos;
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
  for (int j = 5; j;) {
    --j;
    for (int i = s_bloodSplatTextureTable[j].Count(); i;) {
      s_bloodSplatTextureTable[j][--i].Render();
    }
  }
  GxXformPop(GxXform_World);
  GxRsPop();
}
