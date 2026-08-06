#ifndef WOW_SOURCE_WORLDCLIENT_CSIMPLEDOODAD_H
#define WOW_SOURCE_WORLDCLIENT_CSIMPLEDOODAD_H

#include "Gx/CGxDevice.h"
#include "Services/Texture.h"
#include "Tempest/c2vector.h"
#include "Tempest/c3vector.h"
#include "Tempest/c44matrix.h"
#include "Tempest/caabox.h"
#include "Tempest/caasphere.h"

#include <stpl.h>

class CMapDoodadDef;
struct MDLDATA;

class CSimpleDoodadMat {
 public:
  enum {
    PROP_TWOSIDED = 0x1,
    PROP_TRANSPARENT = 0x2
  };

  CSimpleDoodadMat() : nTextures(0), props(0) {
  }

  UINT texture[4];
  UINT nTextures;
  UINT props;
};

class CSimpleDoodadGeoset {
 public:
  CSimpleDoodadGeoset() : material(0) {
  }
  CSimpleDoodadGeoset(const CSimpleDoodadGeoset &geoset);

  TSGrowableArray<NTempest::C3Vector> vertexList;
  TSGrowableArray<NTempest::C3Vector> normalList;
  TSGrowableArray<NTempest::C2Vector> tVertexList;
  TSGrowableArray<WORD>               indexList;
  UINT                                material;
};

struct CSimpleDoodad : public TSHashObject<CSimpleDoodad, HASHKEY_NONE> {
 public:
  CSimpleDoodad() {
  }
  CSimpleDoodad(const CSimpleDoodad &);

  static void Initialize();
  static void Destroy();
  static void ClearCache();

  static CSimpleDoodad *Create(LPCSTR fileName);
  static void           Delete(CSimpleDoodad *simpleDoodad);
  static void           PrepareUpdate();
  static void           AddToScene(CSimpleDoodad *simpleDoodad, NTempest::C44Matrix &mat, CMapDoodadDef *doodadDef);
  static void           RenderScene();

  static CSimpleDoodad *Get(UINT id);
  UINT                  GetId();
  void                  GetBounds(NTempest::CAaSphere &bounds);
  void                  GetExtents(NTempest::CAaBox &extents);
  int                   TestBounds(const NTempest::CAaSphere &bounds);
  int                   TestExtents(const NTempest::CAaBox &extents);

  ~CSimpleDoodad() {
    for (UINT index = 0; index < nTextures; ++index) {
      HandleClose(textures[index]);
      textures[index] = 0;
    }

    if (gxBuf) {
      GxBufDestroy(gxBuf);
      gxBuf = 0;
    }
  }

 private:
  HTEXTURE                             textures[4];
  CSimpleDoodadMat                     materials[4];
  CSimpleDoodadGeoset                  geosets[4];
  TSGrowableArray<NTempest::C44Matrix> matrixList;
  TSGrowableArray<CMapDoodadDef *>     doodadDefList;
  UINT                                 nTextures;
  UINT                                 nMaterials;
  UINT                                 nGeosets;
  int                                  refCount;
  float                                flushTime;
  CGxBuf                              *gxBuf;

 public:
  LINKDECLEX(CSimpleDoodad, sceneLink);
  NTempest::CAaBox    extents;
  NTempest::CAaSphere bounds;

 private:
  static TSHashTable<CSimpleDoodad, HASHKEY_NONE> simpleDoodadHash;
  static HASHKEY_NONE                             nullHashKey;
  static CGxBuf                                  *gxBufDyn;

  static int  Read(LPCSTR fileName, CSimpleDoodad *simpleDoodad);
  static int  MdlReadCallback(const MDLDATA &data, CSimpleDoodad *simpleDoodad);
  static void MdlReadCallback(BYTE *fileData, UINT fileBytes, CSimpleDoodad *simpleDoodad);

  static void GxBufDynCallback(CGxBufCommand &cmd, CGxBuf *buf);
  static void CreateVertices(CSimpleDoodadGeoset *geoset, const CGxBufCommand &cmd, CGxBuf *buf);
  static void CreateIndices(CSimpleDoodadGeoset *geoset, const CGxBufCommand &cmd, CGxBuf *buf);
};

#endif
