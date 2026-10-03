#pragma once

#include "Tempest/c3vector.h"
#include "Tempest/cimvector.h"

static const float MD_OCEAN_RAW_SCALE = -21.0f;
static const float MD_OCEAN_DEPTH_SCALE = MD_OCEAN_RAW_SCALE * (-1.0f / 36.0f);
static const float MD_MAX_DEPTH = MD_OCEAN_DEPTH_SCALE * -255.0f;
static const float WMOMM_IPP = 18.0f;
static const float WMOMM_INCHES_PER_PIXEL = WMOMM_IPP * (1.0f / 36.0f);
static const float WMOMM_QUAD_SIZE = WMOMM_INCHES_PER_PIXEL * 256.0f;
static const float SMOLTILE_SIZE = 4.1666665f;
static const float OOSMOLTILE_SIZE = 1.0f / SMOLTILE_SIZE;

struct SMOFog {
  enum EFogs {
    FOG = 0,
    UWFOG = 1,
    NUM_FOGS = 2
  };

  enum EFlags {
    F_IEBLEND = 1
  };

  struct Fog {
    float               end;
    float               startScalar;
    NTempest::CImVector color;

    void Blend(const Fog &fog, float t) {
      end += (fog.end - end) * t;
      startScalar += (fog.startScalar - startScalar) * t;

      color.Blend255RGB(NTempest::CMath::ftol_0_256_(t * 255.0f), fog.color);
    }
  };

  class Fogs {
   private:
    Fog fog[2];

   public:
    void Blend(const Fogs &fogs, float t) {
      for (UINT i = 0; i < 2; ++i) {
        fog[i].Blend(fogs.fog[i], t);
      }
    }

    Fog &operator[](UINT index) {
      return fog[index];
    }

    const Fog &operator[](UINT index) const {
      return fog[index];
    }
  };

  UINT               flags;
  NTempest::C3Vector pos;
  float              start;
  float              end;
  Fogs               fogs;
};

struct SMOPoly {
  enum {
    F_NOCAMCOLLIDE = 2,
    F_DETAIL = 4,
    F_COLLISION = 8,
    F_HINT = 16,
    F_RENDER = 32,
    F_COLLIDE_HIT = 128
  };

  BYTE flags;
  BYTE lightmapTex;
  BYTE mtlId;
  BYTE pad[1];
};

struct SMOLightmap {
  BYTE x;
  BYTE y;
  char width;
  char height;
};
