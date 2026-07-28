#include "CSimpleDoodad.h"
#include "WorldClient/World.h"

#include "MDLFile/MDLTypes.h"

#include <storm.h>

#include <string.h>

class CStatus;

int MDLFileRead(const char *path, MDLDATA *mdldata, CStatus *status);

TSHashTable<CSimpleDoodad, HASHKEY_NONE> CSimpleDoodad::simpleDoodadHash;
CGxBuf                                  *CSimpleDoodad::gxBufDyn;
HASHKEY_NONE                             CSimpleDoodad::nullHashKey;

static LISTDECLEX(CSimpleDoodad, sceneLink, simpleDoodadScene);

void CSimpleDoodad::Initialize() {
  gxBufDyn = GxBufCreate(GxBWF_Dynamic, GxVBF_PNT0, 0x2000, 0x2000, GxBufDynCallback, 0);
  ASSERT(gxBufDyn);
}

void CSimpleDoodad::Destroy() {
  ClearCache();

  if (gxBufDyn) {
    GxBufDestroy(gxBufDyn);
  }
  gxBufDyn = 0;
}

void CSimpleDoodad::ClearCache() {
  simpleDoodadHash.Clear();
}

CSimpleDoodad *CSimpleDoodad::Create(const char *fileName) {
  unsigned int   hashval = SStrHashHT(fileName);
  CSimpleDoodad *simpleDoodad = simpleDoodadHash.Ptr(hashval, nullHashKey);
  if (simpleDoodad) {
    ++simpleDoodad->refCount;
    return simpleDoodad;
  }

  simpleDoodad = simpleDoodadHash.New(hashval, nullHashKey, 0, 0);
  if (Read(fileName, simpleDoodad)) {
    simpleDoodad->refCount = 1;
    return simpleDoodad;
  }

  simpleDoodadHash.Delete(simpleDoodad);
  return 0;
}

void CSimpleDoodad::Delete(CSimpleDoodad *simpleDoodad) {
  ASSERT(simpleDoodad);

  if (--simpleDoodad->refCount <= 0) {
    simpleDoodad->flushTime = 120.0f;
  }
}

void CSimpleDoodad::PrepareUpdate() {
}

void CSimpleDoodad::AddToScene(CSimpleDoodad *simpleDoodad, NTempest::C44Matrix &mat, CMapDoodadDef *doodadDef) {
  simpleDoodad->matrixList.Add(&mat);
  simpleDoodad->doodadDefList.Add(&doodadDef);

  if (!simpleDoodad->sceneLink.IsLinked()) {
    simpleDoodadScene.LinkNode(simpleDoodad, LIST_TAIL, 0);
  }
}

void CSimpleDoodad::RenderScene() {
  GxRsPush();
  GxRsSet(GxRs_DepthWrite, 1);
  GxRsSet(GxRs_MatDiffuse, NTempest::CImVector(0xFFFFFFFF));
  GxVertexShaderSelect(GxVS_PassThru);
  GxXformPush(GxXform_World);

  CSimpleDoodad *simpleDoodad = simpleDoodadScene.Head();
  while (simpleDoodad) {
    CSimpleDoodad *nextNode = simpleDoodadScene.Next(simpleDoodad);

    for (unsigned int n = 0; n < simpleDoodad->nGeosets; ++n) {
      CSimpleDoodadGeoset *geoset = &simpleDoodad->geosets[n];
      CSimpleDoodadMat    *material = &simpleDoodad->materials[geoset->material];
      CGxTex              *gxTex = TextureGetGxTex(simpleDoodad->textures[material->texture[0]], 0, 0);
      if (gxTex) {
        GxRsSet(GxRs_TexBlend0, GxTexBlend_Mod);
        GxRsSet(GxRs_Texture0, gxTex);
        GxRsSet(GxRs_Culling, (material->props & CSimpleDoodadMat::PROP_TWOSIDED) == 0);
        GxRsSet(GxRs_Blend, (material->props & CSimpleDoodadMat::PROP_TRANSPARENT) != 0);

        gxBufDyn->UserArgSet(geoset);
        GxBufLock(gxBufDyn);
        CGxBatch gxBatch(GxPrim_Triangles, geoset->indexList.Count(), 0, geoset->vertexList.Count(), -1);
        for (unsigned int index = 0; index < simpleDoodad->matrixList.Count(); ++index) {
          CMap::SelectLight(simpleDoodad->doodadDefList[index]);
          GxXformSet(GxXform_World, simpleDoodad->matrixList[index]);
          GxBufRender(gxBatch);
        }
        GxBufUnlock();
      }
    }

    simpleDoodad->sceneLink.Unlink();
    simpleDoodad->matrixList.SetCount(0);
    simpleDoodad->doodadDefList.SetCount(0);
    simpleDoodad = nextNode;
  }

  GxXformPop(GxXform_World);
  GxRsPop();
}

int CSimpleDoodad::Read(const char *fileName, CSimpleDoodad *simpleDoodad) {
  ASSERT(fileName);

  MDLDATA mdlData;
  if (!MDLFileRead(fileName, &mdlData, 0)) {
    return 0;
  }

  return MdlReadCallback(mdlData, simpleDoodad);
}

int CSimpleDoodad::MdlReadCallback(const MDLDATA &data, CSimpleDoodad *simpleDoodad) {
  ASSERT(simpleDoodad);

  if (data.materials.Count() > 4) {
    return 0;
  }
  if (data.textures.Count() > 4) {
    return 0;
  }
  if (data.geosets.Count() > 4) {
    return 0;
  }

  unsigned int i;
  for (i = 0; i < data.materials.Count(); ++i) {
    if (data.materials[i].texLayers.Count() != 1) {
      return 0;
    }
  }

  simpleDoodad->nTextures = data.textures.Count();
  for (i = 0; i < data.textures.Count(); ++i) {
    simpleDoodad->textures[i] = CMap::LoadTexture(data.textures[i].image);
  }

  simpleDoodad->nMaterials = data.materials.Count();
  for (i = 0; i < data.materials.Count(); ++i) {
    CSimpleDoodadMat *material = &simpleDoodad->materials[i];
    material->nTextures = data.materials[i].texLayers.Count();
    for (unsigned int j = 0; j < data.materials[i].texLayers.Count(); ++j) {
      material->texture[j] = data.materials[i].texLayers[j].textureId;
      if (data.materials[i].texLayers[j].blendMode == TEXOP_TRANSPARENT) {
        material->props |= CSimpleDoodadMat::PROP_TRANSPARENT;
      }
      if (data.materials[i].texLayers[j].flags & 0x10) {
        material->props |= CSimpleDoodadMat::PROP_TWOSIDED;
      }
    }
  }

  simpleDoodad->nGeosets = data.geosets.Count();
  for (i = 0; i < data.geosets.Count(); ++i) {
    CSimpleDoodadGeoset *geoset = &simpleDoodad->geosets[i];
    unsigned int         nVertices = data.geosets[i].vertices.Count();

    geoset->vertexList.SetCount(nVertices);
    geoset->normalList.SetCount(nVertices);
    geoset->tVertexList.SetCount(nVertices);
    for (unsigned int v = 0; v < nVertices; ++v) {
      geoset->vertexList[v] = data.geosets[i].vertices[v];
      geoset->normalList[v] = data.geosets[i].normals[v];
      geoset->tVertexList[v] = data.geosets[i].texCoords[0][v];
    }

    unsigned int nIndices = data.geosets[i].primitives.vertices.Count();
    geoset->indexList.SetCount(nIndices);
    for (unsigned int p = 0; p < nIndices; ++p) {
      geoset->indexList[p] = data.geosets[i].primitives.vertices[p];
    }

    geoset->material = data.geosets[i].materialId;
  }

  simpleDoodad->extents = data.model.bounds.extent;
  simpleDoodad->bounds.c.Set(
      (data.model.bounds.extent.b.x + data.model.bounds.extent.t.x) * 0.5f,
      (data.model.bounds.extent.b.y + data.model.bounds.extent.t.y) * 0.5f,
      (data.model.bounds.extent.b.z + data.model.bounds.extent.t.z) * 0.5f
  );
  simpleDoodad->bounds.r = data.model.bounds.radius;

  return 1;
}

void CSimpleDoodad::MdlReadCallback(unsigned char *fileData, unsigned int fileBytes, CSimpleDoodad *simpleDoodad) {
}

void CSimpleDoodad::GxBufDynCallback(CGxBufCommand &cmd, CGxBuf *buf) {
  CSimpleDoodadGeoset *geoset;

  ASSERT(buf);

  geoset = static_cast<CSimpleDoodadGeoset *>(buf->UserArg());
  ASSERT(geoset);

  CreateVertices(geoset, cmd, buf);
  CreateIndices(geoset, cmd, buf);
}

void CSimpleDoodad::CreateVertices(CSimpleDoodadGeoset *geoset, const CGxBufCommand &cmd, CGxBuf *buf) {
  CGxVertexPNT0 *vtxBase;
  unsigned int   index;

  ASSERT(geoset);

  switch (cmd.vertex.op) {
    case GxBufOp_Nop:
      break;

    case GxBufOp_Fill:
      vtxBase = static_cast<CGxVertexPNT0 *>(*cmd.vertex.mem[GxVM_Position]);
      ASSERT(vtxBase);

      for (index = 0; index < geoset->vertexList.Count(); ++index) {
        vtxBase[index].p = geoset->vertexList[index];
        vtxBase[index].n = geoset->normalList[index];
        vtxBase[index].tc[0] = geoset->tVertexList[index];
      }
      break;

    case GxBufOp_Assign:
      vtxBase = static_cast<CGxVertexPNT0 *>(GxAllocVertexMem(buf->VertexCount() * sizeof(CGxVertexPNT0)));
      ASSERT(vtxBase);

      *cmd.vertex.mem[GxVM_Position] = &vtxBase->p;
      *cmd.vertex.mem[GxVM_Normal] = &vtxBase->n;
      *cmd.vertex.mem[GxVM_Texture0] = &vtxBase->tc[0];
      break;
  }
}

void CSimpleDoodad::CreateIndices(CSimpleDoodadGeoset *geoset, const CGxBufCommand &cmd, CGxBuf *buf) {
  unsigned short *idx;

  ASSERT(geoset);

  switch (cmd.index.op) {
    case GxBufOp_Nop:
      break;

    case GxBufOp_Fill:
      idx = static_cast<unsigned short *>(*cmd.index.mem[GxVM_Indices]);
      ASSERT(idx);

      memcpy(idx, geoset->indexList.Ptr(), geoset->indexList.Count() * sizeof(unsigned short));
      break;

    case GxBufOp_Assign:
      idx = static_cast<unsigned short *>(GxAllocIndexMem(buf->IndexCount() * sizeof(unsigned short)));
      ASSERT(idx);

      *cmd.index.mem[GxVM_Indices] = idx;
      break;
  }
}
