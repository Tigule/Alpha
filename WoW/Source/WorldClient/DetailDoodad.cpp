#include "Base/Base.h"
#include "Gx/Gx.h"
#include "Services/ParticleSystem2.h"
#include <WowConst.h>
#include "AaBsp.h"
#include <MapDefs.h>

#include "WorldClient/World.h"
#include "WorldClient/CMapObj.h"
#include "WorldClient/WorldParam.h"
#include "WorldClient/DetailDoodad.h"
#include "WorldClient/CSimpleDoodad.h"
#include "DayNight.h"

#include "WorldClient/Map.h"

#include "DB/DBClient/AutoCode/GroundEffectDoodadRec.h"
#include "MDLFile/MDLTypes.h"
#include "Services/SysMessage.h"
#include "Services/Texture.h"

class CStatus;

BYTE *MDLFileBinaryLoad(char *path, UINT *fileBytes, CStatus *status);
void  MDLFileBinaryUnload(BYTE *fileData);
BYTE *MDLFileBinarySeek(BYTE *fileData, UINT fileBytes, DWORD sectionTag);
BOOL  MDLFileRead(LPCSTR path, MDLDATA *mdldata, CStatus *status);

static BOOL IsBinaryModelFile(char *path);

char detailDoodadPath[] = "World\\NoDXT\\Detail\\";

static UINT g_gxBufCreateCount;
static UINT g_gxBufDestroyCount;

LISTDECLEX(CDetailDoodadGeom, lameAssLink, CDetailDoodad::geomList);
LISTDECLEX(CDetailDoodadInst, lameAssLink, CDetailDoodad::instList);
TSGrowableArray<CDetailDoodadData *> CDetailDoodad::doodadList;
TSGrowableArray<CGxBuf *>            CDetailDoodad::gxBufFreeList;
CGxTex *CDetailDoodad::alphaRampTexture;

void CDetailDoodad::Initialize() {
  UINT nEntries = g_groundEffectDoodadDB.GetNumRecords();
  UINT i;

  gxBufFreeList.SetCount(0);
  doodadList.SetCount(nEntries);

  for (i = 0; i < nEntries; ++i) {
    doodadList[i] = 0;
  }

  for (i = 0; i < nEntries; ++i) {
    const GroundEffectDoodadRec *doodadInfo = g_groundEffectDoodadDB.GetRecordByIndex(i);

    ASSERT(doodadInfo);
    doodadList[doodadInfo->m_doodadIdTag] = NEW(CDetailDoodadData)(doodadInfo->m_doodadpath);
    ASSERT(doodadList[doodadInfo->m_doodadIdTag]);
  }

  GxTexCreate(64, 8, GxTex_Argb8888, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), 0, UpdateAlphaRampTexture, alphaRampTexture);
  ASSERT(alphaRampTexture);
}

void CDetailDoodad::Destroy() {
  Clear();

  for (UINT index = 0; index < gxBufFreeList.Count(); ++index) {
    GxBufDestroy(gxBufFreeList[index]);
    ++g_gxBufDestroyCount;
  }
  gxBufFreeList.SetCount(0);

  ASSERT(g_gxBufCreateCount==g_gxBufDestroyCount);

  if (alphaRampTexture) {
    GxTexDestroy(alphaRampTexture);
  }
  alphaRampTexture = 0;
}

void CDetailDoodad::Clear() {
  for (UINT index = 0; index < doodadList.Count(); ++index) {
    if (doodadList[index]) {
      DEL(doodadList[index]);
    }
    doodadList[index] = 0;
  }

  instList.Clear();
  geomList.Clear();
}

CDetailDoodadInst *CDetailDoodad::AllocInst() {
  CDetailDoodadInst *inst = instList.Head();
  if (!inst) {
    inst = instList.NewNode(0, 0, 0);
    FATALASSERT(inst);
  } else {
    inst->lameAssLink.Unlink();
  }

  return inst;
}

void CDetailDoodad::FreeInst(CDetailDoodadInst *inst) {
  UINT index;

  ASSERT(inst);
  inst->lameAssLink.Unlink();
  instList.LinkNode(inst, LIST_TAIL, 0);

  for (index = 0; index < 2; ++index) {
    if (inst->geom[index]) {
      FreeGeom(inst->geom[index]);
    }
    inst->geom[index] = 0;

    if (inst->gxBuf[index]) {
      FreeGxBuf(inst->gxBuf[index]);
    }
    inst->gxBuf[index] = 0;
  }
}

CDetailDoodadGeom *CDetailDoodad::AllocGeom() {
  CDetailDoodadGeom *geom = geomList.Head();
  if (!geom) {
    geom = geomList.NewNode(0, 0, 0);
    FATALASSERT(geom);
  } else {
    geom->lameAssLink.Unlink();
  }

  return geom;
}

void CDetailDoodad::FreeGeom(CDetailDoodadGeom *geom) {
  ASSERT(geom);
  geom->lameAssLink.Unlink();
  geomList.LinkNode(geom, LIST_TAIL, 0);

  geom->texture = 0;
  geom->vertexList.SetCount(0);
  geom->normalList.SetCount(0);
  geom->tVertexList.SetCount(0);
  geom->cVertexList.SetCount(0);
  geom->indexList.SetCount(0);
}

CGxBuf *CDetailDoodad::AllocGxBuf(UINT vertexCount, UINT indexCount) {
  CGxBuf *gxBuf;

  if (!gxBufFreeList.Count()) {
    gxBuf = GxBufCreate(GxBWF_Medium, GxVBF_PNCT0, vertexCount, indexCount, GxBufFillCallback, 0);
    FATALASSERT(gxBuf);
    ++g_gxBufCreateCount;
  } else {
    gxBuf = gxBufFreeList[gxBufFreeList.Count() - 1];
    FATALASSERT(gxBuf);
    gxBufFreeList.SetCount(gxBufFreeList.Count() - 1);
    gxBuf->CountSet(vertexCount, indexCount);
    gxBuf->Invalidate(CGxBuf::S_INVALID_DISCARD, CGxBuf::S_INVALID_DISCARD);
  }

  return gxBuf;
}

void CDetailDoodad::FreeGxBuf(CGxBuf *gxBuf) {
  ASSERT(gxBuf);
  gxBufFreeList.Add(1, &gxBuf);
}

void CDetailDoodad::GxBufFillCallback(CGxBufCommand &cmd, CGxBuf *buf) {
  CDetailDoodadGeom *detailDoodadGeom = static_cast<CDetailDoodadGeom *>(buf->UserArg());
  FATALASSERT(detailDoodadGeom);
  detailDoodadGeom->FillGxBufVertex(cmd, buf);
  detailDoodadGeom->FillGxBufIndex(cmd, buf);
}

void CDetailDoodad::CreateAlphaRampTexture(LPCVOID &texels) {
  static NTempest::CImVector alphaRamp[8][64];
  BYTE                       alpha = 0;

  for (UINT x = 0; x < 64; ++x) {
    for (UINT y = 0; y < 8; ++y) {
      alphaRamp[y][63 - x].Set(alpha, 255, 255, 255);
    }
    alpha += 4;
  }

  texels = alphaRamp;
}

void CDetailDoodad::UpdateAlphaRampTexture(
    EGxTexCommand cmd,
    UINT          w,
    UINT          h,
    UINT          d,
    UINT          mipLevel,
    LPVOID        userArg,
    UINT         &texelStrideInBytes,
    LPCVOID      &texels
) {
  switch (cmd) {
    case GxTex_Lock:
      return;

    case GxTex_Latch:
      CreateAlphaRampTexture(texels);
      texelStrideInBytes = 4 * w;
      return;
  }
}

void CDetailDoodadGeom::FillGxBufVertex(CGxBufCommand &cmd, CGxBuf *buf) {
  UINT index = 0;

  switch (cmd.vertex.op) {
    case GxBufOp_Nop:
      return;

    case GxBufOp_Fill: {
      CGxVertexPNCT0 *vertices = static_cast<CGxVertexPNCT0 *>(*cmd.vertex.mem[GxVM_Vertex]);
      for (index = 0; index < vertexList.Count(); ++index) {
        vertices[index].p = vertexList[index];
        vertices[index].n = normalList[index];
        vertices[index].c = cVertexList[index];
        vertices[index].tc[0] = tVertexList[index];
      }
      break;
    }

    case GxBufOp_Assign:
      cmd.vertex.Set(GxVM_Position, vertexList.Ptr(), 12);
      cmd.vertex.Set(GxVM_Color, cVertexList.Ptr(), 4);
      cmd.vertex.Set(GxVM_Normal, normalList.Ptr(), 12);
      cmd.vertex.Set(GxVM_Texture0, tVertexList.Ptr(), 8);
      break;
  }
}

void CDetailDoodadGeom::FillGxBufIndex(CGxBufCommand &cmd, CGxBuf *buf) {
  switch (cmd.index.op) {
    case GxBufOp_Nop:
      return;

    case GxBufOp_Fill:
      memcpy(*cmd.index.mem[GxVM_Indices], indexList.Ptr(), indexList.Count() * sizeof(WORD));
      return;

    case GxBufOp_Assign:
      cmd.index.Set(GxVM_Indices, indexList.Ptr(), 0);
      return;
  }
}

CDetailDoodadData::CDetailDoodadData() {
  loaded = 0;
  texture = 0;
  geom = 0;
  fileName = 0;
}

CDetailDoodadData::CDetailDoodadData(LPCSTR mdlName) {
  loaded = 0;
  texture = 0;
  geom = 0;
  fileName = mdlName;
}

CDetailDoodadData::~CDetailDoodadData() {
  if (texture) {
    HandleClose(texture);
  }
  texture = 0;
  fileName = 0;

  if (geom) {
    CDetailDoodad::FreeGeom(geom);
    geom = 0;
  }
}

BOOL CDetailDoodadData::Load() {
  char pathName[MAX_PATH];
  UINT fileBytes;

  FATALASSERT(fileName);

  DWORD length = SStrCopy(pathName, detailDoodadPath, 0x7FFFFFFF);

  SStrCopy(pathName + length, fileName, 0x7FFFFFFF);
  if (IsBinaryModelFile(pathName)) {
    BYTE *fileData = MDLFileBinaryLoad(pathName, &fileBytes, 0);
    if (!fileData) {
      char message[512];

      SStrPrintf(message, sizeof(message), "No such file: %s", pathName);
      SysMsgAdd(message, SYSMSG_ERROR, 4);
    } else {
      MdlReadCallback(fileData, fileBytes, this);
      MDLFileBinaryUnload(fileData);
    }
  } else {
    MDLDATA mdlData;
    BOOL    ret = MDLFileRead(pathName, &mdlData, 0);

    FATALASSERT(ret);
    MdlReadCallback(mdlData, this);
  }

  loaded = 1;
  return 1;
}

static BOOL IsBinaryModelFile(char *path) {
  int length = SStrLen(path);
  if (path[length - 1] == 'x' || path[length - 1] == 'X') {
    if (SFile::FileExists(path)) {
      return 1;
    }
    path[length - 1] = 'l';
  } else if (!SFile::FileExists(path)) {
    path[length - 1] = 'x';
    return 1;
  }
  return 0;
}

void CDetailDoodadData::MdlReadCallback(BYTE *fileData, UINT fileBytes, CDetailDoodadData *detailDoodad) {
  FATALASSERT(detailDoodad);
  FATALASSERT(detailDoodad->geom == 0);

  BYTE *data = MDLFileBinarySeek(fileData, fileBytes, 'SXET');
  FATALASSERT(data);

  UINT sectionBytes = *((ULONG *) (data));
  data += sizeof(ULONG);
  UINT numTextures = sectionBytes / sizeof(MDLTEXTURESECTION);
  FATALASSERT(numTextures == 1);
  FATALASSERT(sectionBytes == (numTextures * sizeof(MDLTEXTURESECTION)));

  MDLTEXTURESECTION *textures = reinterpret_cast<MDLTEXTURESECTION *>(data);
  data += sectionBytes;
  detailDoodad->texture = CMap::LoadTexture(textures->image);

  CDetailDoodadGeom *geom = CDetailDoodad::AllocGeom();
  FATALASSERT(geom);
  detailDoodad->geom = geom;
  geom->texture = 0;

  data = MDLFileBinarySeek(data, fileBytes - (data - fileData), 'SOEG');
  FATALASSERT(data);
  data += sizeof(ULONG);
  FATALASSERT(*((ULONG *) (data)) == 1);
  data += sizeof(ULONG);
  data += sizeof(ULONG);

  FATALASSERT(*((ULONG *) (data)) == 'XTRV');
  data += sizeof(ULONG);
  UINT nVertices = *((ULONG *) (data));
  data += sizeof(ULONG);
  geom->vertexList.SetCount(nVertices);
  memcpy(geom->vertexList.Ptr(), data, nVertices * sizeof(NTempest::C3Vector));
  data += geom->vertexList.Bytes();

  FATALASSERT(*((ULONG *) (data)) == 'SMRN');
  data += sizeof(ULONG);
  FATALASSERT(nVertices == *((ULONG *) (data)));
  data += sizeof(ULONG);
  geom->normalList.SetCount(nVertices);
  memcpy(geom->normalList.Ptr(), data, nVertices * sizeof(NTempest::C3Vector));
  data += geom->normalList.Bytes();

  FATALASSERT(*((ULONG *) (data)) == 'SAVU');
  data += sizeof(ULONG);
  FATALASSERT(*((ULONG *) (data)) == 1);
  data += sizeof(ULONG);
  geom->tVertexList.SetCount(nVertices);
  memcpy(geom->tVertexList.Ptr(), data, nVertices * sizeof(NTempest::C2Vector));
  data += geom->tVertexList.Bytes();

  FATALASSERT(*((ULONG *) (data)) == 'PYTP');
  data += sizeof(ULONG);
  UINT numPrimTypes = *((ULONG *) (data));
  data += sizeof(ULONG);
  data += numPrimTypes * sizeof(BYTE);

  FATALASSERT(*((ULONG *) (data)) == 'TNCP');
  data += sizeof(ULONG);
  data += sizeof(ULONG);
  data += numPrimTypes * sizeof(ULONG);

  FATALASSERT(*((ULONG *) (data)) == 'XTVP');
  data += sizeof(ULONG);
  UINT nPrims = *((ULONG *) (data));
  data += sizeof(ULONG);
  geom->indexList.SetCount(nPrims);
  memcpy(geom->indexList.Ptr(), data, nPrims * sizeof(WORD));
}

void CDetailDoodadData::MdlReadCallback(const MDLDATA &data, CDetailDoodadData *detailDoodad) {
  FATALASSERT(detailDoodad);
  FATALASSERT(detailDoodad->geom == 0);
  FATALASSERT(data.materials.Count() == 1);
  FATALASSERT(data.textures.Count() == 1);
  FATALASSERT(data.geosets.Count() == 1);

  detailDoodad->texture = CMap::LoadTexture(data.textures[0].image);

  CDetailDoodadGeom *geom = CDetailDoodad::AllocGeom();
  FATALASSERT(geom);
  detailDoodad->geom = geom;
  geom->texture = 0;

  UINT nVertices = data.geosets[0].vertices.Count();

  geom->vertexList.SetCount(nVertices);
  geom->normalList.SetCount(nVertices);
  geom->tVertexList.SetCount(nVertices);
  for (UINT index = 0; index < nVertices; ++index) {
    geom->vertexList[index] = data.geosets[0].vertices[index];
    geom->normalList[index] = data.geosets[0].normals[index];
    geom->tVertexList[index] = data.geosets[0].texCoords[0][index];
  }

  UINT nPrims = data.geosets[0].primitives.vertices.Count();
  geom->indexList.SetCount(nPrims);
  for (UINT index2 = 0; index2 < nPrims; ++index2) {
    geom->indexList[index2] = data.geosets[0].primitives.vertices[index2];
  }
}

CDetailDoodadInst::CDetailDoodadInst() {
  geom[0] = geom[1] = 0;
  gxBuf[0] = gxBuf[1] = 0;
}

CDetailDoodadInst::~CDetailDoodadInst() {
  for (UINT index = 0; index < 2; ++index) {
    if (geom[index]) {
      CDetailDoodad::FreeGeom(geom[index]);
    }
    geom[index] = 0;

    if (gxBuf[index]) {
      CDetailDoodad::FreeGxBuf(gxBuf[index]);
    }
    gxBuf[index] = 0;
  }
}

void CDetailDoodadInst::FreeBufs() {
  for (UINT index = 0; index < 2; ++index) {
    if (gxBuf[index]) {
      CDetailDoodad::FreeGxBuf(gxBuf[index]);
    }
    gxBuf[index] = 0;
  }
}

void CDetailDoodadInst::AddDoodad(UINT doodadId, NTempest::C3Vector &pos, DWORD flags) {
  UINT idx;

  CDetailDoodadData *detailData = CDetailDoodad::doodadList[doodadId];
  if (detailData) {
    return;
  }
  if (!detailData->loaded && !detailData->Load()) {
    return;
  }
  if (!detailData->geom) {
    return;
  }

  int geomIndex = -1;
  for (idx = 0; idx < 2; ++idx) {
    if (geom[idx] && geom[idx]->texture == detailData->texture) {
      geomIndex = idx;
      break;
    }
  }

  if (geomIndex == -1) {
    if (geom[0]) {
      if (geom[1]) {
        return;
      }
      geomIndex = 1;
    } else {
      geomIndex = 0;
    }

    geom[geomIndex] = CDetailDoodad::AllocGeom();
    geom[geomIndex]->texture = detailData->texture;
    geom[geomIndex]->vertexList.SetChunkSize(512);
    geom[geomIndex]->normalList.SetChunkSize(512);
    geom[geomIndex]->tVertexList.SetChunkSize(512);
    geom[geomIndex]->cVertexList.SetChunkSize(512);
    geom[geomIndex]->indexList.SetChunkSize(512);
  }

  CDetailDoodadGeom *geomData = detailData->geom;
  UINT               xOffs = geom[geomIndex]->vertexList.Count();
  UINT               iIdx = geom[geomIndex]->indexList.Count();
  UINT               nVerts = geomData->vertexList.Count() + xOffs;

  geom[geomIndex]->vertexList.SetCount(nVerts);
  geom[geomIndex]->normalList.SetCount(nVerts);
  geom[geomIndex]->tVertexList.SetCount(nVerts);
  geom[geomIndex]->cVertexList.SetCount(nVerts);
  geom[geomIndex]->indexList.SetCount(geom[geomIndex]->indexList.Count() + geomData->indexList.Count());

  NTempest::CImVector argb(1, 1, 1, 1);
  if (flags & Flag_Shadowed) {
    argb.b = 0xA0;
    argb.g = 0xA0;
    argb.r = 0xA0;
  }

  for (idx = 0; idx < geomData->vertexList.Count(); ++idx) {
    geom[geomIndex]->vertexList[xOffs + idx] = geomData->vertexList[idx] + pos;
    geom[geomIndex]->normalList[xOffs + idx] = geomData->normalList[idx];
    geom[geomIndex]->tVertexList[xOffs + idx] = geomData->tVertexList[idx];
    geom[geomIndex]->cVertexList[xOffs + idx] = argb;
  }

  for (idx = 0; idx < geomData->indexList.Count(); ++idx) {
    geom[geomIndex]->indexList[iIdx++] = geomData->indexList[idx] + xOffs;
  }
}

void CDetailDoodadInst::AddDoodad(UINT doodadId, NTempest::C3Vector &pos, DWORD flags, NTempest::C4Plane &plane) {
  UINT idx;

  CDetailDoodadData *detailData = CDetailDoodad::doodadList[doodadId];
  if (!detailData) {
    return;
  }
  if (!detailData->loaded && !detailData->Load()) {
    return;
  }
  if (!detailData->geom) {
    return;
  }

  int geomIndex = -1;
  for (idx = 0; idx < 2; ++idx) {
    if (geom[idx] && geom[idx]->texture == detailData->texture) {
      geomIndex = idx;
      break;
    }
  }

  if (geomIndex == -1) {
    if (geom[0]) {
      if (geom[1]) {
        return;
      }
      geomIndex = 1;
    } else {
      geomIndex = 0;
    }

    geom[geomIndex] = CDetailDoodad::AllocGeom();
    geom[geomIndex]->texture = detailData->texture;
    geom[geomIndex]->vertexList.SetChunkSize(512);
    geom[geomIndex]->normalList.SetChunkSize(512);
    geom[geomIndex]->tVertexList.SetChunkSize(512);
    geom[geomIndex]->cVertexList.SetChunkSize(512);
    geom[geomIndex]->indexList.SetChunkSize(512);
  }

  CDetailDoodadGeom *geomData = detailData->geom;
  UINT               xOffs = geom[geomIndex]->vertexList.Count();
  UINT               iIdx = geom[geomIndex]->indexList.Count();
  UINT               nVerts = geomData->vertexList.Count() + xOffs;

  geom[geomIndex]->vertexList.SetCount(nVerts);
  geom[geomIndex]->normalList.SetCount(nVerts);
  geom[geomIndex]->tVertexList.SetCount(nVerts);
  geom[geomIndex]->cVertexList.SetCount(nVerts);
  geom[geomIndex]->indexList.SetCount(geom[geomIndex]->indexList.Count() + geomData->indexList.Count());

  NTempest::CImVector argb(255, 255, 255, 255);
  if (flags & Flag_Shadowed) {
    argb.r = 0xC0;
    argb.g = 0xC0;
    argb.b = 0xC0;
  }
  if (GxCaps().m_colorFormat == GxCF_rgba) {
    argb = NTempest::CImVector(argb.a, argb.b, argb.g, argb.r);
  }

  for (idx = 0; idx < geomData->vertexList.Count(); ++idx) {
    geom[geomIndex]->vertexList[xOffs + idx] = geomData->vertexList[idx] + pos;
    geom[geomIndex]->normalList[xOffs + idx] = plane.n;
    geom[geomIndex]->tVertexList[xOffs + idx] = geomData->tVertexList[idx];
    geom[geomIndex]->cVertexList[xOffs + idx] = argb;
  }

  for (idx = 0; idx < geomData->indexList.Count(); ++idx) {
    geom[geomIndex]->indexList[iIdx++] = geomData->indexList[idx] + xOffs;
  }
}

void CDetailDoodadInst::Render() {
  GxRsSet(GxRs_Culling, 0);
  GxRsSet(GxRs_MatDiffuse, NTempest::CImVector(0xFFFFFFFF));
  GxRsSet(GxRs_TexBlend0, GxTexBlend_Mod);
  GxRsSet(GxRs_Blend, GxBlend_AlphaKey);
  GxRsSet(GxRs_AlphaRef, static_cast<BYTE>(CWorld::detailDoodadAlphaRef));
  GxRsSet(GxRs_DepthWrite, 1);
  GxVertexShaderSelect(GxVS_PassThru);

  for (UINT index = 0; index < 2; ++index) {
    if (geom[index] && geom[index]->indexList.Count()) {
      CGxTex *texture = TextureGetGxTex(geom[index]->texture, 0, 0);
      if (texture) {
        if (!gxBuf[index]) {
          gxBuf[index] = CDetailDoodad::AllocGxBuf(geom[index]->vertexList.Count(), geom[index]->indexList.Count());
          gxBuf[index]->UserArgSet(geom[index]);
        }

        GxRsSet(GxRs_Texture0, texture);
        GxBufLock(gxBuf[index]);
        CGxBatch gxBatch(GxPrim_Triangles, geom[index]->indexList.Count(), 0, -1, -1);
        GxBufRender(gxBatch);
        GxBufUnlock();
      }
    }
  }

  GxRsSet(GxRs_DepthWrite, 1);
  GxRsSet(GxRs_Culling, 1);
}

void CDetailDoodadInst::RenderAlpha() {
  GxRsSet(GxRs_TexBlend0, GxTexBlend_Mod);
  GxRsSet(GxRs_Culling, 0);
  GxRsSet(GxRs_MatDiffuse, NTempest::CImVector(0xFFFFFFFF));
  GxRsSet(GxRs_Blend, GxBlend_Alpha);
  GxRsSet(GxRs_AlphaRef, static_cast<BYTE>(CWorld::detailDoodadAlphaRef));
  GxRsSet(GxRs_DepthWrite, 1);
  GxRsSet(GxRs_Texture1, CDetailDoodad::alphaRampTexture);
  GxRsSet(GxRs_TexBlend1, GxTexBlend_Mod);
  GxRsSet(GxRs_TexGen1, GxTexGen_View);
  GxRsSet(GxRs_TextureShader1, GxTS_Affine);

  NTempest::C44Matrix mat;
  mat.Scale(1.0f / (CWorld::detailDoodadDist * 0.33f));
  mat.Rotate(PI * 0.5f, NTempest::C3Vector(0.0f, 1.0f, 0.0f), 0);
  mat.Translate(NTempest::C3Vector(0.0f, 0.0f, CWorld::detailDoodadDist * -0.66f));
  GxXformPush(GxXform_Tex1, mat);
  GxVertexShaderSelect(GxVS_PassThru);

  for (UINT i = 0; i < 2; ++i) {
    if (geom[i] && geom[i]->indexList.Count()) {
      CGxTex *texture = TextureGetGxTex(geom[i]->texture, 0, 0);
      if (texture) {
        if (!gxBuf[i]) {
          gxBuf[i] = CDetailDoodad::AllocGxBuf(geom[i]->vertexList.Count(), geom[i]->indexList.Count());
          gxBuf[i]->UserArgSet(geom[i]);
        }

        GxRsSet(GxRs_Texture0, texture);
        GxBufLock(gxBuf[i]);
        CGxBatch gxBatch(GxPrim_Triangles, geom[i]->indexList.Count(), 0, -1, -1);
        GxBufRender(gxBatch);
        GxBufUnlock();
      }
    }
  }

  GxXformPop(GxXform_Tex1);
  GxRsSet(GxRs_DepthWrite, 1);
  GxRsSet(GxRs_Culling, 1);
}
