#include "Base/Base.h"
#include "Gx/Gx.h"
#include "Services/ParticleSystem2.h"
#include <WowConst.h>
#include "AaBsp.h"
#include <MapDefs.h>

#include "WorldClient/World.h"
#include "WorldClient/Map.h"
#include "WorldClient/WorldParam.h"
#include "WorldClient/DetailDoodad.h"
#include "WorldClient/CSimpleDoodad.h"
#include "DayNight.h"

#include "WorldClient/Map.h"

#include "MDLFile/MDLTypes.h"

#include <storm.h>

#include <string.h>

class CStatus;

BOOL MDLFileRead(LPCSTR path, MDLDATA *mdldata, CStatus *status);

TSHashTable<CSimpleDoodad, HASHKEY_NONE> CSimpleDoodad::simpleDoodadHash;
CGxBuf                                  *CSimpleDoodad::gxBufDyn;
HASHKEY_NONE                             CSimpleDoodad::nullHashKey;

LISTDECLEX(CSimpleDoodad, sceneLink, simpleDoodadScene);

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

CSimpleDoodad *CSimpleDoodad::Create(LPCSTR fileName) {
  UINT           hashval = SStrHash(fileName, 0, 0);
  CSimpleDoodad *simpleDoodad = simpleDoodadHash.Ptr(hashval, nullHashKey);
  if (simpleDoodad) {
    ++simpleDoodad->refCount;
    return simpleDoodad;
  }

  simpleDoodad = simpleDoodadHash.New(hashval, nullHashKey, 0, 0);
  if (!Read(fileName, simpleDoodad)) {
    simpleDoodadHash.Delete(simpleDoodad);
    return 0;
  }

  simpleDoodad->refCount = 1;
  return simpleDoodad;
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

  SAFEITERATELIST(CSimpleDoodad, simpleDoodadScene, simpleDoodad) {
    for (UINT n = 0; n < simpleDoodad->nGeosets; ++n) {
      CSimpleDoodadGeoset *geoset = &simpleDoodad->geosets[n];
      CSimpleDoodadMat    *material = &simpleDoodad->materials[geoset->material];
      CGxTex              *gxTex = TextureGetGxTex(simpleDoodad->textures[material->texture[0]], 0, 0);
      if (gxTex) {
        GxRsSet(GxRs_TexBlend0, GxTexBlend_Mod);
        GxRsSet(GxRs_Texture0, gxTex);
        if (material->props & CSimpleDoodadMat::PROP_TWOSIDED) {
          GxRsSet(GxRs_Culling, 0);
        } else {
          GxRsSet(GxRs_Culling, 1);
        }
        if (material->props & CSimpleDoodadMat::PROP_TRANSPARENT) {
          GxRsSet(GxRs_Blend, GxBlend_AlphaKey);
        } else {
          GxRsSet(GxRs_Blend, GxBlend_Opaque);
        }

        gxBufDyn->UserArgSet(geoset);
        GxBufLock(gxBufDyn);
        CGxBatch gxBatch(GxPrim_Triangles, geoset->indexList.Count(), 0, geoset->vertexList.Count(), -1);
        for (UINT index = 0; index < simpleDoodad->matrixList.Count(); ++index) {
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
  }

  GxXformPop(GxXform_World);
  GxRsPop();
}

int CSimpleDoodad::Read(LPCSTR fileName, CSimpleDoodad *simpleDoodad) {
  ASSERT(fileName);

  MDLDATA mdlData;
  if (!MDLFileRead(fileName, &mdlData, 0)) {
    return 0;
  }

  return MdlReadCallback(mdlData, simpleDoodad);
}

void CSimpleDoodad::MdlReadCallback(BYTE *fileData, UINT fileBytes, CSimpleDoodad *simpleDoodad) {
}

BOOL CSimpleDoodad::MdlReadCallback(const MDLDATA &data, CSimpleDoodad *simpleDoodad) {
  ASSERT(simpleDoodad);

  if (data.textures.Count() > 4) {
    return 0;
  }
  if (data.materials.Count() > 4) {
    return 0;
  }
  if (data.geosets.Count() > 4) {
    return 0;
  }

  UINT n;
  for (n = 0; n < data.materials.Count(); ++n) {
    if (data.materials[n].texLayers.Count() != 1) {
      return 0;
    }
  }

  simpleDoodad->nTextures = data.textures.Count();
  for (n = 0; n < data.textures.Count(); ++n) {
    if (!data.textures[n].image[0]) {
      simpleDoodad->textures[n] = CMap::LoadTexture("badcollater.blx");
    } else {
      simpleDoodad->textures[n] = CMap::LoadTexture(data.textures[n].image);
    }
  }

  simpleDoodad->nMaterials = data.materials.Count();
  for (n = 0; n < data.materials.Count(); ++n) {
    CSimpleDoodadMat *material = &simpleDoodad->materials[n];
    material->nTextures = data.materials[n].texLayers.Count();
    for (UINT j = 0; j < data.materials[n].texLayers.Count(); ++j) {
      material->texture[j] = data.materials[n].texLayers[j].textureId;
      if (data.materials[n].texLayers[j].blendMode == TEXOP_TRANSPARENT) {
        material->props |= CSimpleDoodadMat::PROP_TRANSPARENT;
      }
      if (data.materials[n].texLayers[j].flags & 0x10) {
        material->props |= CSimpleDoodadMat::PROP_TWOSIDED;
      }
    }
  }

  simpleDoodad->nGeosets = data.geosets.Count();
  for (n = 0; n < data.geosets.Count(); ++n) {
    CSimpleDoodadGeoset *geoset = &simpleDoodad->geosets[n];
    UINT                 nVertices = data.geosets[n].vertices.Count();
    geoset->vertexList.SetCount(nVertices);
    geoset->normalList.SetCount(nVertices);
    geoset->tVertexList.SetCount(nVertices);

    for (UINT v = 0; v < nVertices; ++v) {
      geoset->vertexList[v] = data.geosets[n].vertices[v];
      geoset->normalList[v] = data.geosets[n].normals[v];
      geoset->tVertexList[v] = data.geosets[n].texCoords[0][v];
    }

    UINT nPrims = data.geosets[n].primitives.vertices.Count();
    geoset->indexList.SetCount(nPrims);
    for (UINT p = 0; p < nPrims; ++p) {
      geoset->indexList[p] = data.geosets[n].primitives.vertices[p];
    }

    geoset->material = data.geosets[n].materialId;
  }

  simpleDoodad->extents = data.model.bounds.extent;
  simpleDoodad->bounds.c = (data.model.bounds.extent.b + data.model.bounds.extent.t) * 0.5f;
  simpleDoodad->bounds.r = data.model.bounds.radius;
  return 1;
}

void CSimpleDoodad::GxBufDynCallback(CGxBufCommand &cmd, CGxBuf *buf) {
  CSimpleDoodadGeoset *geoset;

  ASSERT(buf);

  geoset = (CSimpleDoodadGeoset *)buf->UserArg();
  ASSERT(geoset);

  CreateVertices(geoset, cmd, buf);
  CreateIndices(geoset, cmd, buf);
}

void CSimpleDoodad::CreateVertices(CSimpleDoodadGeoset *geoset, const CGxBufCommand &cmd, CGxBuf *buf) {
  CGxVertexPNT0 *vtxBase;
  UINT           index;

  ASSERT(geoset);

  switch (cmd.vertex.op) {
    case GxBufOp_Nop:
      return;

    case GxBufOp_Fill:
      vtxBase = (CGxVertexPNT0 *)*cmd.vertex.mem[GxVM_Position];
      ASSERT(vtxBase);

      for (index = 0; index < geoset->vertexList.Count(); ++index) {
        vtxBase[index].p = geoset->vertexList[index];
        vtxBase[index].n = geoset->normalList[index];
        vtxBase[index].tc[0] = geoset->tVertexList[index];
      }
      break;

    case GxBufOp_Assign:
      vtxBase = (CGxVertexPNT0 *)GxAllocVertexMem(buf->VertexCount() * sizeof(CGxVertexPNT0));
      ASSERT(vtxBase);

      *cmd.vertex.mem[GxVM_Position] = &vtxBase->p;
      *cmd.vertex.mem[GxVM_Normal] = &vtxBase->n;
      *cmd.vertex.mem[GxVM_Texture0] = &vtxBase->tc[0];
      break;
  }
}

void CSimpleDoodad::CreateIndices(CSimpleDoodadGeoset *geoset, const CGxBufCommand &cmd, CGxBuf *buf) {
  WORD *idx;

  ASSERT(geoset);

  switch (cmd.index.op) {
    case GxBufOp_Nop:
      return;

    case GxBufOp_Fill:
      idx = (WORD *)*cmd.index.mem[GxVM_Indices];
      ASSERT(idx);

      memcpy(idx, geoset->indexList.Ptr(), geoset->indexList.Count() * sizeof(WORD));
      break;

    case GxBufOp_Assign:
      idx = (WORD *)GxAllocIndexMem(buf->IndexCount() * sizeof(WORD));
      ASSERT(idx);

      *cmd.index.mem[GxVM_Indices] = idx;
      break;
  }
}
