#ifndef WOW_SOURCE_WORLDCLIENT_DETAILDOODAD_H
#define WOW_SOURCE_WORLDCLIENT_DETAILDOODAD_H

#include "Gx/Gx.h"
#include "Tempest/c2vector.h"
#include "Tempest/c3vector.h"
#include "Tempest/c4plane.h"
#include "Tempest/cimvector.h"

#include <stpl.h>

struct CGxBuf;
struct HTEXTURE__;
struct MDLDATA;

class CDetailDoodadGeom {
 public:
  CDetailDoodadGeom();
  ~CDetailDoodadGeom();

  void FillGxBufVertex(CGxBufCommand &cmd, CGxBuf *buf);
  void FillGxBufIndex(CGxBufCommand &cmd, CGxBuf *buf);

  HTEXTURE__                          *texture;
  TSGrowableArray<NTempest::C3Vector>  vertexList;
  TSGrowableArray<NTempest::C3Vector>  normalList;
  TSGrowableArray<NTempest::C2Vector>  tVertexList;
  TSGrowableArray<NTempest::CImVector> cVertexList;
  TSGrowableArray<unsigned short>      indexList;
  TSLink<CDetailDoodadGeom>            lameAssLink;
};

class CDetailDoodadData {
 public:
  CDetailDoodadData();
  CDetailDoodadData(const char *mdlName);
  ~CDetailDoodadData();

  int Load();

  const char        *fileName;
  int                loaded;
  HTEXTURE__        *texture;
  CDetailDoodadGeom *geom;

 private:
  static void __fastcall MdlReadCallback(unsigned int *fileData, unsigned int fileBytes, CDetailDoodadData *detailDoodad);
  static void __fastcall MdlReadCallback(MDLDATA &data, CDetailDoodadData *detailDoodad);
};

class CDetailDoodadInst {
 public:
  CDetailDoodadInst();
  ~CDetailDoodadInst();

  void FreeBufs();
  void AddDoodad(unsigned int doodadId, NTempest::C3Vector &pos, unsigned long flags);
  void AddDoodad(unsigned int doodadId, NTempest::C3Vector &pos, unsigned long flags, NTempest::C4Plane &plane);
  void Render();
  void RenderAlpha();
  int  HasBufs();

  CDetailDoodadGeom        *geom[2];
  CGxBuf                   *gxBuf[2];
  TSLink<CDetailDoodadGeom> lameAssLink;
};

class CDetailDoodad {
 public:
  static void __fastcall               Initialize();
  static void __fastcall               Destroy();
  static void __fastcall               Clear();
  static CDetailDoodadInst *__fastcall AllocInst();
  static void __fastcall               FreeInst(CDetailDoodadInst *inst);
  static CDetailDoodadGeom *__fastcall AllocGeom();
  static void __fastcall               FreeGeom(CDetailDoodadGeom *geom);
  static CGxBuf *__fastcall            AllocGxBuf(unsigned int vertexCount, unsigned int indexCount);
  static void __fastcall               FreeGxBuf(CGxBuf *gxBuf);

  static TSExplicitList<CDetailDoodadGeom, 104> geomList;
  static TSExplicitList<CDetailDoodadInst, 16>  instList;
  static TSGrowableArray<CDetailDoodadData *>   doodadList;
  static CGxTex                                *alphaRampTexture;

 private:
  static void __fastcall GxBufFillCallback(CGxBufCommand &cmd, CGxBuf *buf);
  static void __fastcall CreateAlphaRampTexture(const void *&texels);
  static void __fastcall UpdateAlphaRampTexture(
      EGxTexCommand cmd,
      unsigned int  w,
      unsigned int  h,
      unsigned int  d,
      unsigned int  mipLevel,
      void         *userArg,
      unsigned int &texelStrideInBytes,
      const void  *&texels
  );

  static TSGrowableArray<CGxBuf *> gxBufFreeList;
};

#endif
