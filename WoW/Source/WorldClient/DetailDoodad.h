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
  enum {
    PROP_TWOSIDED = 1
  };

  void FillGxBufVertex(CGxBufCommand &cmd, CGxBuf *buf);
  void FillGxBufIndex(CGxBufCommand &cmd, CGxBuf *buf);

  HTEXTURE__                          *texture;
  TSGrowableArray<NTempest::C3Vector>  vertexList;
  TSGrowableArray<NTempest::C3Vector>  normalList;
  TSGrowableArray<NTempest::C2Vector>  tVertexList;
  TSGrowableArray<NTempest::CImVector> cVertexList;
  TSGrowableArray<unsigned short>      indexList;
  LINKDECLEX(CDetailDoodadGeom, lameAssLink);
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
  static void MdlReadCallback(unsigned char *fileData, unsigned int fileBytes, CDetailDoodadData *detailDoodad);
  static void MdlReadCallback(const MDLDATA &data, CDetailDoodadData *detailDoodad);
};

class CDetailDoodadInst {
 public:
  enum {
    Flag_Shadowed = 1
  };

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
  LINKDECLEX(CDetailDoodadGeom, lameAssLink);
};

class CDetailDoodad {
 public:
  static void Initialize();
  static void Destroy();
  static void Clear();
  static CDetailDoodadInst *AllocInst();
  static void FreeInst(CDetailDoodadInst *inst);
  static CDetailDoodadGeom *AllocGeom();
  static void FreeGeom(CDetailDoodadGeom *geom);
  static CGxBuf *AllocGxBuf(unsigned int vertexCount, unsigned int indexCount);
  static void FreeGxBuf(CGxBuf *gxBuf);

  static LISTDECLEX(CDetailDoodadGeom, lameAssLink, geomList);
  static LISTDECLEX(CDetailDoodadInst, lameAssLink, instList);
  static TSGrowableArray<CDetailDoodadData *>   doodadList;
  static CGxTex                                *alphaRampTexture;

 private:
  static void GxBufFillCallback(CGxBufCommand &cmd, CGxBuf *buf);
  static void CreateAlphaRampTexture(const void *&texels);
  static void UpdateAlphaRampTexture(
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
