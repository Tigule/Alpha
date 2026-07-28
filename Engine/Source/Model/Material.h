#pragma once

#include "Base/Handle.h"
#include "Gx/Gx.h"
#include "Tempest/cimvector.h"

#include <stpl.h>

DECLARE_DERIVED_HANDLE(HMATERIAL, HOBJECT);
DECLARE_DERIVED_HANDLE(HMATERIALSHARED, HOBJECT);

struct CModelTexture;

struct CTmuPassUnique {
  CTmuPassUnique() : combiner(GxTexBlend_Mod), textureId(-1) {
  }

  static int Compare(
      const CModelTexture *, const CModelTexture *, const CTmuPassUnique &, const CTmuPassUnique &
  );

  EGxTexBlend  combiner;
  unsigned int textureId;
};

struct CTmuPassShared {
  CTmuPassShared() : transformId(-1), coordId(0), textureShader(GxTS_PassThru), flags(0) {
  }

  unsigned int     transformId;
  unsigned int     coordId;
  EGxTextureShader textureShader;
  unsigned int     flags;
};

struct CTexLayer {
  CTexLayer() : vertexFormat(GxVBF_PCT0), disables(0), blendMode(GxBlend_Opaque), layerAlpha(255) {
    for (unsigned int i = 0; i < 2; ++i) {
      tmuPass[i].combiner = GxTexBlend_Mod;
    }
  }
  CTexLayer(const CTexLayer &a);
  static int Compare(const CModelTexture *aTextures, const CModelTexture *bTextures, const CTexLayer &a, const CTexLayer &b);

  EGxVertexBufferFormat vertexFormat;
  union {
    struct {
      unsigned int lighting : 1;
      unsigned int fog : 1;
      unsigned int depthTest : 1;
      unsigned int depthWrite : 1;
      unsigned int culling : 1;
    } disable;
    unsigned int disables;
  };
  EGxBlend       blendMode;
  CTmuPassUnique tmuPass[2];
  unsigned char  layerAlpha;
};

struct CTexLayerShared {
  EGxBlend       blendMode;
  CTmuPassShared tmuPass[2];
};

struct CMaterialShared : public CHandleObject {
 public:
  CMaterialShared() {
  }
  CMaterialShared(const CMaterialShared &);

  TSGrowableArray<CTexLayerShared> layers;
  int                              priorityPlane;
};

struct CMaterial : public CHandleObject {
 public:
  CMaterial() : data(0), emissiveColor(0ul) {
  }

  CMaterial(const CMaterial &source)
      : CHandleObject(source),
        layers(source.layers),
        data(static_cast<HMATERIALSHARED>(HandleDuplicate(source.data))),
        emissiveColor(source.emissiveColor) {
  }

  ~CMaterial() {
    if (data) {
      HandleClose(data);
    }
  }

  CMaterial &operator=(const CMaterial &source) {
    CHandleObject::operator=(source);
    if (data) {
      HandleClose(data);
    }
    data = static_cast<HMATERIALSHARED>(HandleDuplicate(source.data));
    layers = source.layers;
    emissiveColor = source.emissiveColor;
    return *this;
  }

  TSGrowableArray<CTexLayer> layers;
  HMATERIALSHARED            data;
  NTempest::CImVector        emissiveColor;
};
