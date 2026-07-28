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

  unsigned int texture[4];
  unsigned int nTextures;
  unsigned int props;
};

class CSimpleDoodadGeoset {
 public:
  CSimpleDoodadGeoset() : material(0) {
  }
  CSimpleDoodadGeoset(const CSimpleDoodadGeoset &geoset);

  TSGrowableArray<NTempest::C3Vector> vertexList;
  TSGrowableArray<NTempest::C3Vector> normalList;
  TSGrowableArray<NTempest::C2Vector> tVertexList;
  TSGrowableArray<unsigned short>     indexList;
  unsigned int                        material;
};

struct CSimpleDoodad : public TSHashObject<CSimpleDoodad, HASHKEY_NONE> {
 public:
  CSimpleDoodad() {
  }
  CSimpleDoodad(const CSimpleDoodad &);

  static void Initialize();
  static void Destroy();
  static void ClearCache();

  static CSimpleDoodad *Create(const char *fileName);
  static void Delete(CSimpleDoodad *simpleDoodad);
  static void PrepareUpdate();
  static void AddToScene(CSimpleDoodad *simpleDoodad, NTempest::C44Matrix &mat, CMapDoodadDef *doodadDef);
  static void RenderScene();

  static CSimpleDoodad *Get(unsigned int id);
  unsigned int          GetId();
  void                  GetBounds(NTempest::CAaSphere &bounds);
  void                  GetExtents(NTempest::CAaBox &extents);
  int                   TestBounds(const NTempest::CAaSphere &bounds);
  int                   TestExtents(const NTempest::CAaBox &extents);

  ~CSimpleDoodad() {
    for (unsigned int index = 0; index < nTextures; ++index) {
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
  unsigned int                         nTextures;
  unsigned int                         nMaterials;
  unsigned int                         nGeosets;
  int                                  refCount;
  float                                flushTime;
  CGxBuf                              *gxBuf;

 public:
  LINKDECLEX(CSimpleDoodad, sceneLink);
  NTempest::CAaBox      extents;
  NTempest::CAaSphere   bounds;

 private:
  static TSHashTable<CSimpleDoodad, HASHKEY_NONE> simpleDoodadHash;
  static HASHKEY_NONE                             nullHashKey;
  static CGxBuf                                  *gxBufDyn;

  static int Read(const char *fileName, CSimpleDoodad *simpleDoodad);
  static int MdlReadCallback(const MDLDATA &data, CSimpleDoodad *simpleDoodad);
  static void MdlReadCallback(unsigned char *fileData, unsigned int fileBytes, CSimpleDoodad *simpleDoodad);

  static void GxBufDynCallback(CGxBufCommand &cmd, CGxBuf *buf);
  static void CreateVertices(CSimpleDoodadGeoset *geoset, const CGxBufCommand &cmd, CGxBuf *buf);
  static void CreateIndices(CSimpleDoodadGeoset *geoset, const CGxBufCommand &cmd, CGxBuf *buf);
};

#endif
