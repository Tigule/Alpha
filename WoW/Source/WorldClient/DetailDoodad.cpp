#include <WowConst.h>
#include <MapDefs.h>

#include "WorldClient/DetailDoodad.h"
#include "WorldClient/World.h"

#include "Base/Base.h"
#include "DB/DBClient/AutoCode/GroundEffectDoodadRec.h"
#include "MDLFile/MDLTypes.h"
#include "Services/Texture.h"

class CStatus;

BYTE *MDLFileBinaryLoad(char *path, UINT *fileBytes, CStatus *status);
void  MDLFileBinaryUnload(BYTE *fileData);
BYTE *MDLFileBinarySeek(BYTE *fileData, UINT fileBytes, DWORD sectionTag);

static UINT g_gxBufCreateCount;
static UINT g_gxBufDestroyCount;

LISTDECLEX(CDetailDoodadInst, lameAssLink, CDetailDoodad::instList);
TSGrowableArray<CGxBuf *>            CDetailDoodad::gxBufFreeList;
TSGrowableArray<CDetailDoodadData *> CDetailDoodad::doodadList;
LISTDECLEX(CDetailDoodadGeom, lameAssLink, CDetailDoodad::geomList);
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

  CGxTexFlags flags(GxTex_Linear, 0, 0, 0, 0, 0, 1);
  GxTexCreate(64, 8, GxTex_Argb8888, flags, 0, UpdateAlphaRampTexture, alphaRampTexture);
  ASSERT(alphaRampTexture);
}

void CDetailDoodad::Destroy() {
  Clear();

  for (UINT index = 0; index < gxBufFreeList.Count(); ++index) {
    GxBufDestroy(gxBufFreeList[index]);
    ++g_gxBufDestroyCount;
  }
  gxBufFreeList.SetCount(0);

  ASSERT(g_gxBufCreateCount == g_gxBufDestroyCount);

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
  if (inst) {
    instList.UnlinkNode(inst);
  } else {
    inst = NEW(CDetailDoodadInst)();
  }

  FATALASSERT(inst);
  return inst;
}

void CDetailDoodad::FreeInst(CDetailDoodadInst *inst) {
  UINT index;

  ASSERT(inst);
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
  if (geom) {
    geomList.UnlinkNode(geom);
  } else {
    geom = NEW(CDetailDoodadGeom)();
  }

  FATALASSERT(geom);
  return geom;
}

void CDetailDoodad::FreeGeom(CDetailDoodadGeom *geom) {
  ASSERT(geom);
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

  if (gxBufFreeList.Count()) {
    gxBuf = gxBufFreeList[gxBufFreeList.Count() - 1];
    FATALASSERT(gxBuf);
    gxBufFreeList.SetCount(gxBufFreeList.Count() - 1);
    gxBuf->CountSet(vertexCount, indexCount);
    gxBuf->Invalidate(CGxBuf::S_INVALID_DISCARD, CGxBuf::S_INVALID_DISCARD);
  } else {
    gxBuf = GxBufCreate(GxBWF_Medium, GxVBF_PNCT0, vertexCount, indexCount, GxBufFillCallback, 0);
    FATALASSERT(gxBuf);
    ++g_gxBufCreateCount;
  }

  return gxBuf;
}

void CDetailDoodad::FreeGxBuf(CGxBuf *gxBuf) {
  ASSERT(gxBuf);
  *gxBufFreeList.New() = gxBuf;
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

  for (int x = 63; x >= 0; --x) {
    for (int y = 0; y < 8; ++y) {
      alphaRamp[y][x].Set(alpha, 255, 255, 255);
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
      break;

    case GxTex_Latch:
      CreateAlphaRampTexture(texels);
      texelStrideInBytes = 4 * w;
      break;
  }
}

void CDetailDoodadGeom::FillGxBufVertex(CGxBufCommand &cmd, CGxBuf *buf) {
  UINT index = 0;

  switch (cmd.vertex.op) {
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
      cmd.vertex.Set(GxVM_Normal, normalList.Ptr(), 12);
      cmd.vertex.Set(GxVM_Color, cVertexList.Ptr(), 4);
      cmd.vertex.Set(GxVM_Texture0, tVertexList.Ptr(), 8);
      break;
  }
}

void CDetailDoodadGeom::FillGxBufIndex(CGxBufCommand &cmd, CGxBuf *buf) {
  switch (cmd.index.op) {
    case GxBufOp_Fill:
      memcpy(*cmd.index.mem[GxVM_Indices], indexList.Ptr(), indexList.Count() * sizeof(WORD));
      break;

    case GxBufOp_Assign:
      cmd.index.Set(GxVM_Indices, indexList.Ptr(), 0);
      break;
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

static int IsBinaryModelFile(char *path) {
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

int CDetailDoodadData::Load() {
  FATALASSERT(fileName);

  char pathName[260];
  SStrCopy(pathName, "World\\NoDXT\\Detail\\", sizeof(pathName));
  SStrPack(pathName, fileName, sizeof(pathName));

  UINT pathLength = SStrLen(pathName);
  if (pathLength && pathName[pathLength - 1] != 'x' && pathName[pathLength - 1] != 'X') {
    pathName[pathLength - 1] = 'x';
  }

  UINT  fileBytes = 0;
  BYTE *fileData = MDLFileBinaryLoad(pathName, &fileBytes, 0);
  if (!fileData) {
    loaded = 1;
    return 0;
  }

  MdlReadCallback(fileData, fileBytes, this);
  MDLFileBinaryUnload(fileData);
  loaded = 1;
  return 1;
}

void CDetailDoodadData::MdlReadCallback(BYTE *fileData, UINT fileBytes, CDetailDoodadData *detailDoodad) {
  FATALASSERT(detailDoodad);
  FATALASSERT(detailDoodad->geom == 0);

  BYTE *texSection = MDLFileBinarySeek(fileData, fileBytes, 0x53584554);
  FATALASSERT(texSection);
  UINT sectionBytes = *reinterpret_cast<UINT *>(texSection);
  FATALASSERT(sectionBytes == 268);
  BYTE *texData = texSection + sizeof(UINT);
  detailDoodad->texture = CMap::LoadTexture(reinterpret_cast<char *>(texData + 4));

  CDetailDoodadGeom *geom = CDetailDoodad::AllocGeom();
  FATALASSERT(geom);
  detailDoodad->geom = geom;
  geom->texture = 0;

  BYTE *geoSection = MDLFileBinarySeek(fileData, fileBytes, 0x534F4547);
  FATALASSERT(geoSection);
  UINT *data = reinterpret_cast<UINT *>(geoSection);
  FATALASSERT(data[1] == 1);
  data += 3;

  FATALASSERT(*data++ == 0x58545256);
  UINT nVertices = *data++;
  geom->vertexList.SetCount(nVertices);
  memcpy(geom->vertexList.Ptr(), data, nVertices * sizeof(NTempest::C3Vector));
  data += 3 * nVertices;

  FATALASSERT(*data++ == 0x534D524E);
  FATALASSERT(*data++ == nVertices);
  geom->normalList.SetCount(nVertices);
  memcpy(geom->normalList.Ptr(), data, nVertices * sizeof(NTempest::C3Vector));
  data += 3 * nVertices;

  FATALASSERT(*data++ == 0x53415655);
  FATALASSERT(*data++ == 1);
  geom->tVertexList.SetCount(nVertices);
  memcpy(geom->tVertexList.Ptr(), data, nVertices * sizeof(NTempest::C2Vector));
  data += 2 * nVertices;

  FATALASSERT(*data++ == 0x50595450);
  UINT primitiveTypeBytes = *data++;
  data = reinterpret_cast<UINT *>(reinterpret_cast<BYTE *>(data) + primitiveTypeBytes);

  FATALASSERT(*data++ == 0x544E4350);
  UINT primitiveCount = *data++;
  data += primitiveCount;

  FATALASSERT(*data++ == 0x58545650);
  UINT nPrims = *data++;
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

  const MDLGEOSETSECTION &geoset = data.geosets[0];
  UINT                    nVertices = geoset.vertices.Count();

  geom->vertexList.SetCount(nVertices);
  geom->normalList.SetCount(nVertices);
  geom->tVertexList.SetCount(nVertices);
  for (UINT index = 0; index < nVertices; ++index) {
    geom->vertexList[index] = geoset.vertices[index];
    geom->normalList[index] = geoset.normals[index];
    geom->tVertexList[index] = geoset.texCoords[0][index];
  }

  UINT nIndices = geoset.primitives.vertices.Count();
  geom->indexList.SetCount(nIndices);
  for (UINT index2 = 0; index2 < nIndices; ++index2) {
    geom->indexList[index2] = geoset.primitives.vertices[index2];
  }
}

CDetailDoodadInst::CDetailDoodadInst() {
  geom[0] = 0;
  geom[1] = 0;
  gxBuf[0] = 0;
  gxBuf[1] = 0;
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
  CDetailDoodadData *detailData = CDetailDoodad::doodadList[doodadId];
  if (!detailData || (!detailData->loaded && !detailData->Load()) || !detailData->geom) {
    return;
  }

  CDetailDoodadGeom *geomData = detailData->geom;
  int                geomIndex = -1;
  UINT               index;
  for (index = 0; index < 2; ++index) {
    if (geom[index] && geom[index]->texture == detailData->texture) {
      geomIndex = index;
      break;
    }
  }

  if (geomIndex == -1) {
    geomIndex = geom[0] ? 1 : 0;
    if (geom[geomIndex]) {
      return;
    }

    geom[geomIndex] = CDetailDoodad::AllocGeom();
    geom[geomIndex]->texture = detailData->texture;
    geom[geomIndex]->vertexList.SetChunkSize(512);
    geom[geomIndex]->normalList.SetChunkSize(512);
    geom[geomIndex]->tVertexList.SetChunkSize(512);
    geom[geomIndex]->cVertexList.SetChunkSize(512);
    geom[geomIndex]->indexList.SetChunkSize(512);
  }

  CDetailDoodadGeom *dst = geom[geomIndex];
  UINT               vertexBase = dst->vertexList.Count();
  UINT               indexBase = dst->indexList.Count();
  UINT               vertexCount = geomData->vertexList.Count();
  UINT               indexCount = geomData->indexList.Count();

  dst->vertexList.SetCount(vertexBase + vertexCount);
  dst->normalList.SetCount(vertexBase + vertexCount);
  dst->tVertexList.SetCount(vertexBase + vertexCount);
  dst->cVertexList.SetCount(vertexBase + vertexCount);
  dst->indexList.SetCount(indexBase + indexCount);

  NTempest::CImVector argb(0xFFFFFFFF);
  if (flags & Flag_Shadowed) {
    argb.r = 0xC0;
    argb.g = 0xC0;
    argb.b = 0xC0;
  }

  for (index = 0; index < vertexCount; ++index) {
    dst->vertexList[vertexBase + index] = geomData->vertexList[index] + pos;
    dst->normalList[vertexBase + index] = geomData->normalList[index];
    dst->tVertexList[vertexBase + index] = geomData->tVertexList[index];
    dst->cVertexList[vertexBase + index] = argb;
  }

  for (index = 0; index < indexCount; ++index) {
    dst->indexList[indexBase + index] = static_cast<WORD>(vertexBase + geomData->indexList[index]);
  }
}

void CDetailDoodadInst::AddDoodad(UINT doodadId, NTempest::C3Vector &pos, DWORD flags, NTempest::C4Plane &plane) {
  UINT idx;
  UINT iIdx;

  CDetailDoodadData *detailData = CDetailDoodad::doodadList[doodadId];
  if (!detailData || (!detailData->loaded && !detailData->Load()) || !detailData->geom) {
    return;
  }

  CDetailDoodadGeom *geomData = detailData->geom;
  int                geomIndex = -1;
  for (idx = 0; idx < 2; ++idx) {
    if (geom[idx] && geom[idx]->texture == detailData->texture) {
      geomIndex = idx;
      break;
    }
  }

  if (geomIndex == -1) {
    geomIndex = geom[0] ? 1 : 0;
    if (geom[geomIndex]) {
      return;
    }

    geom[geomIndex] = CDetailDoodad::AllocGeom();
    geom[geomIndex]->texture = detailData->texture;
    geom[geomIndex]->vertexList.SetChunkSize(512);
    geom[geomIndex]->normalList.SetChunkSize(512);
    geom[geomIndex]->tVertexList.SetChunkSize(512);
    geom[geomIndex]->cVertexList.SetChunkSize(512);
    geom[geomIndex]->indexList.SetChunkSize(512);
  }

  CDetailDoodadGeom *dst = geom[geomIndex];
  UINT               vertexBase = dst->vertexList.Count();
  UINT               indexBase = dst->indexList.Count();
  UINT               vertexCount = geomData->vertexList.Count();
  UINT               indexCount = geomData->indexList.Count();

  dst->vertexList.SetCount(vertexBase + vertexCount);
  dst->normalList.SetCount(vertexBase + vertexCount);
  dst->tVertexList.SetCount(vertexBase + vertexCount);
  dst->cVertexList.SetCount(vertexBase + vertexCount);
  dst->indexList.SetCount(indexBase + indexCount);

  NTempest::CImVector argb(0xFFFFFFFF);
  if (flags & 1) {
    argb.r = 0xC0;
    argb.g = 0xC0;
    argb.b = 0xC0;
  }
  if (GxCaps().m_colorFormat == GxCF_rgba) {
    argb.Set(argb.a, argb.b, argb.g, argb.r);
  }

  for (idx = 0; idx < vertexCount; ++idx) {
    dst->vertexList[vertexBase + idx] = geomData->vertexList[idx] + pos;
    dst->normalList[vertexBase + idx] = plane.n;
    dst->tVertexList[vertexBase + idx] = geomData->tVertexList[idx];
    dst->cVertexList[vertexBase + idx] = argb;
  }

  for (iIdx = 0; iIdx < indexCount; ++iIdx) {
    dst->indexList[indexBase + iIdx] = static_cast<WORD>(vertexBase + geomData->indexList[iIdx]);
  }
}

void CDetailDoodadInst::Render() {
  GxRsSet(GxRs_Culling, 0);
  GxRsSet(GxRs_MatDiffuse, NTempest::CImVector(0xFFFFFFFF));
  GxRsSet(GxRs_TexBlend0, GxTexBlend_Mod);
  GxRsSet(GxRs_Blend, GxBlend_Alpha);
  GxRsSet(GxRs_AlphaRef, static_cast<int>(CWorld::detailDoodadAlphaRef));
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
  NTempest::C44Matrix mat;

  GxRsSet(GxRs_TexBlend0, GxTexBlend_Mod);
  GxRsSet(GxRs_Culling, 0);
  GxRsSet(GxRs_MatDiffuse, NTempest::CImVector(0xFFFFFFFF));
  GxRsSet(GxRs_Blend, GxBlend_Alpha);
  GxRsSet(GxRs_AlphaRef, static_cast<int>(CWorld::detailDoodadAlphaRef));
  GxRsSet(GxRs_DepthWrite, 1);
  GxRsSet(GxRs_Texture1, CDetailDoodad::alphaRampTexture);
  GxRsSet(GxRs_TexBlend1, GxTexBlend_Mod);
  GxRsSet(GxRs_TexGen1, GxTexGen_View);
  GxRsSet(GxRs_TextureShader1, GxTS_Affine);

  mat.Scale(1.0f / (CWorld::detailDoodadDist * 0.33f));
  mat.Rotate(3.1415927f * 0.5f, NTempest::C3Vector(0.0f, 1.0f, 0.0f), 0);
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
