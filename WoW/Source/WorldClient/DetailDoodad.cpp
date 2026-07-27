#include "WorldClient/DetailDoodad.h"
#include "WorldClient/World.h"

#include "Base/Base.h"
#include "DB/DBClient/AutoCode/GroundEffectDoodadRec.h"
#include "MDLFile/MDLTypes.h"
#include "Services/Texture.h"

class CStatus;

unsigned char *MDLFileBinaryLoad(char *path, unsigned int *fileBytes, CStatus *status);
void MDLFileBinaryUnload(unsigned char *fileData);
unsigned char *MDLFileBinarySeek(unsigned char *fileData, unsigned int fileBytes, unsigned long sectionTag);

static unsigned int g_gxBufCreateCount;
static unsigned int g_gxBufDestroyCount;

TSExplicitList<CDetailDoodadInst, 16>  CDetailDoodad::instList;
TSGrowableArray<CGxBuf *>              CDetailDoodad::gxBufFreeList;
TSGrowableArray<CDetailDoodadData *>   CDetailDoodad::doodadList;
TSExplicitList<CDetailDoodadGeom, 104> CDetailDoodad::geomList;
CGxTex                                *CDetailDoodad::alphaRampTexture;

void CDetailDoodad::Initialize() {
  unsigned int nEntries = g_groundEffectDoodadDB.GetNumRecords();
  unsigned int i;

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

  for (unsigned int index = 0; index < gxBufFreeList.Count(); ++index) {
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
  for (unsigned int index = 0; index < doodadList.Count(); ++index) {
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
  unsigned int index;

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

CGxBuf *CDetailDoodad::AllocGxBuf(unsigned int vertexCount, unsigned int indexCount) {
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

void CDetailDoodad::CreateAlphaRampTexture(const void *&texels) {
  static NTempest::CImVector alphaRamp[8][64];
  unsigned char              alpha = 0;

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
    unsigned int  w,
    unsigned int  h,
    unsigned int  d,
    unsigned int  mipLevel,
    void         *userArg,
    unsigned int &texelStrideInBytes,
    const void  *&texels
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
  unsigned int index = 0;

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
      memcpy(*cmd.index.mem[GxVM_Indices], indexList.Ptr(), indexList.Count() * sizeof(unsigned short));
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

CDetailDoodadData::CDetailDoodadData(const char *mdlName) {
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

static int IsBinaryModelFile(char* path) {
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

  unsigned int pathLength = SStrLen(pathName);
  if (pathLength && pathName[pathLength - 1] != 'x' && pathName[pathLength - 1] != 'X') {
    pathName[pathLength - 1] = 'x';
  }

  unsigned int   fileBytes = 0;
  unsigned char *fileData = MDLFileBinaryLoad(pathName, &fileBytes, 0);
  if (!fileData) {
    loaded = 1;
    return 0;
  }

  MdlReadCallback(reinterpret_cast<unsigned int *>(fileData), fileBytes, this);
  MDLFileBinaryUnload(fileData);
  loaded = 1;
  return 1;
}

void CDetailDoodadData::MdlReadCallback(unsigned int *fileData, unsigned int fileBytes, CDetailDoodadData *detailDoodad) {
  FATALASSERT(detailDoodad);
  FATALASSERT(detailDoodad->geom == 0);

  unsigned char *bytes = reinterpret_cast<unsigned char *>(fileData);
  unsigned char *texSection = MDLFileBinarySeek(bytes, fileBytes, 0x53584554);
  FATALASSERT(texSection);
  unsigned int sectionBytes = *reinterpret_cast<unsigned int *>(texSection);
  FATALASSERT(sectionBytes == 268);
  unsigned char *texData = texSection + sizeof(unsigned int);
  detailDoodad->texture = CMap::LoadTexture(reinterpret_cast<char *>(texData + 4));

  CDetailDoodadGeom *geom = CDetailDoodad::AllocGeom();
  FATALASSERT(geom);
  detailDoodad->geom = geom;
  geom->texture = 0;

  unsigned char *geoSection = MDLFileBinarySeek(bytes, fileBytes, 0x534F4547);
  FATALASSERT(geoSection);
  unsigned int *data = reinterpret_cast<unsigned int *>(geoSection);
  FATALASSERT(data[1] == 1);
  data += 3;

  FATALASSERT(*data++ == 0x58545256);
  unsigned int nVertices = *data++;
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
  unsigned int primitiveTypeBytes = *data++;
  data = reinterpret_cast<unsigned int *>(reinterpret_cast<unsigned char *>(data) + primitiveTypeBytes);

  FATALASSERT(*data++ == 0x544E4350);
  unsigned int primitiveCount = *data++;
  data += primitiveCount;

  FATALASSERT(*data++ == 0x58545650);
  unsigned int nPrims = *data++;
  geom->indexList.SetCount(nPrims);
  memcpy(geom->indexList.Ptr(), data, nPrims * sizeof(unsigned short));
}

CDetailDoodadInst::CDetailDoodadInst() {
  geom[0] = 0;
  geom[1] = 0;
  gxBuf[0] = 0;
  gxBuf[1] = 0;
}

CDetailDoodadInst::~CDetailDoodadInst() {
  for (unsigned int index = 0; index < 2; ++index) {
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
  for (unsigned int index = 0; index < 2; ++index) {
    if (gxBuf[index]) {
      CDetailDoodad::FreeGxBuf(gxBuf[index]);
    }
    gxBuf[index] = 0;
  }
}

void CDetailDoodadInst::AddDoodad(unsigned int doodadId, NTempest::C3Vector &pos, unsigned long flags, NTempest::C4Plane &plane) {
  unsigned int idx;
  unsigned int iIdx;

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
  unsigned int       vertexBase = dst->vertexList.Count();
  unsigned int       indexBase = dst->indexList.Count();
  unsigned int       vertexCount = geomData->vertexList.Count();
  unsigned int       indexCount = geomData->indexList.Count();

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
    dst->indexList[indexBase + iIdx] = static_cast<unsigned short>(vertexBase + geomData->indexList[iIdx]);
  }
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

  for (unsigned int i = 0; i < 2; ++i) {
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
