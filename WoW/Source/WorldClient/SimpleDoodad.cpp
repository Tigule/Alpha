#include "CSimpleDoodad.h"

#include <storm.h>

#include <string.h>

TSHashTable<CSimpleDoodad, HASHKEY_NONE> CSimpleDoodad::simpleDoodadHash;
CGxBuf                                  *CSimpleDoodad::gxBufDyn;
HASHKEY_NONE                             CSimpleDoodad::nullHashKey;

void __fastcall CSimpleDoodad::Initialize() {
  gxBufDyn = GxBufCreate(GxBWF_Dynamic, GxVBF_PNT0, 0x2000, 0x2000, GxBufDynCallback, 0);
  ASSERT(gxBufDyn);
}

void __fastcall CSimpleDoodad::Destroy() {
  ClearCache();

  if (gxBufDyn) {
    GxBufDestroy(gxBufDyn);
  }
  gxBufDyn = 0;
}

void __fastcall CSimpleDoodad::ClearCache() {
  simpleDoodadHash.Clear();
}

void __fastcall CSimpleDoodad::GxBufDynCallback(CGxBufCommand &cmd, CGxBuf *buf) {
  CSimpleDoodadGeoset *geoset;

  ASSERT(buf);

  geoset = static_cast<CSimpleDoodadGeoset *>(buf->UserArg());
  ASSERT(geoset);

  CreateVertices(geoset, cmd, buf);
  CreateIndices(geoset, cmd, buf);
}

void __fastcall CSimpleDoodad::CreateVertices(CSimpleDoodadGeoset *geoset, const CGxBufCommand &cmd, CGxBuf *buf) {
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

void __fastcall CSimpleDoodad::CreateIndices(CSimpleDoodadGeoset *geoset, const CGxBufCommand &cmd, CGxBuf *buf) {
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
