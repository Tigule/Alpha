#pragma once

#include "Tempest/c3vector.h"
#include "Tempest/cimvector.h"

static const float MD_OCEAN_RAW_SCALE = -21.0f;
static const float MD_OCEAN_DEPTH_SCALE = MD_OCEAN_RAW_SCALE * (-1.0f / 36.0f);
static const float MD_MAX_DEPTH = MD_OCEAN_DEPTH_SCALE * -255.0f;
static const float SMOLTILE_SIZE = 4.1666665f;
static const float OOSMOLTILE_SIZE = 1.0f / SMOLTILE_SIZE;
static const float WMOMM_IPP = 18.0f;
static const float WMOMM_INCHES_PER_PIXEL = WMOMM_IPP * (1.0f / 36.0f);
static const float WMOMM_QUAD_SIZE = WMOMM_INCHES_PER_PIXEL * 256.0f;

struct SMOFog {
  enum EFlags {
    F_IEBLEND = 1
  };

  enum EFogs {
    FOG = 0,
    UWFOG = 1,
    NUM_FOGS = 2
  };

  struct Fog {
    void Blend(const Fog &fog, float t) {
      end += (fog.end - end) * t;
      startScalar += (fog.startScalar - startScalar) * t;

      UINT amount = NTempest::CMath::fuint_n(t * 255.0f);
      if (!amount) {
        return;
      }

      if (amount == 255) {
        color.r = fog.color.r;
        color.g = fog.color.g;
        color.b = fog.color.b;
        return;
      }

      color.r = static_cast<BYTE>(color.r + ((amount * (fog.color.r - color.r)) >> 8));
      color.g = static_cast<BYTE>(color.g + ((amount * (fog.color.g - color.g)) >> 8));
      color.b = static_cast<BYTE>(color.b + ((amount * (fog.color.b - color.b)) >> 8));
    }

    float               end;
    float               startScalar;
    NTempest::CImVector color;
  };

  class Fogs {
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

   private:
    Fog fog[2];
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
