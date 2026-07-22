#pragma once

#include "Tempest/c3vector.h"
#include "Tempest/cimvector.h"

struct SMOFog {
  struct Fog {
    void Blend(Fog &fog, float t) {
      end += (fog.end - end) * t;
      startScalar += (fog.startScalar - startScalar) * t;

      unsigned int amount = NTempest::CMath::fuint_n(t * 255.0f);
      if (!amount) {
        return;
      }

      if (amount == 255) {
        color.r = fog.color.r;
        color.g = fog.color.g;
        color.b = fog.color.b;
        return;
      }

      color.r = static_cast<unsigned char>(color.r + ((amount * (fog.color.r - color.r)) >> 8));
      color.g = static_cast<unsigned char>(color.g + ((amount * (fog.color.g - color.g)) >> 8));
      color.b = static_cast<unsigned char>(color.b + ((amount * (fog.color.b - color.b)) >> 8));
    }

    float               end;
    float               startScalar;
    NTempest::CImVector color;
  };

  class Fogs {
   public:
    void Blend(Fogs &fogs, float t) {
      for (unsigned int i = 0; i < 2; ++i) {
        fog[i].Blend(fogs.fog[i], t);
      }
    }

    Fog &operator[](unsigned int index) {
      return fog[index];
    }

    const Fog &operator[](unsigned int index) const {
      return fog[index];
    }

   private:
    Fog fog[2];
  };

  unsigned int       flags;
  NTempest::C3Vector pos;
  float              start;
  float              end;
  Fogs               fogs;
};

struct SMOPoly {
  unsigned char flags;
  unsigned char lightmapTex;
  unsigned char mtlId;
  unsigned char pad[1];
};

struct SMOLightmap {
  unsigned char x;
  unsigned char y;
  char          width;
  char          height;
};
