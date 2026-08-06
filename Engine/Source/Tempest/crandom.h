#pragma once

#include "cmath.h"

namespace NTempest {

  class C2Vector;
  class C3Vector;

  extern const DWORD gnoise32_[64];

  class CRndSeed {
   public:
    explicit CRndSeed(DWORD seed = 0);
    explicit CRndSeed(char *password) {
      SetSeed(password);
    }
    ~CRndSeed() {
    }

    void SetSeed(char *password);
    void SetSeed(DWORD seed);

   protected:
    DWORD rndacc;
    DWORD rndvls;

    friend class CRandom;
  };

  inline CRndSeed::CRndSeed(DWORD seed) {
    SetSeed(seed);
  }

  class CRandom {
   public:
    enum {
      rexp = 0x3F800000,
      rmant = 0x007FFFFF,
      lrexp = 0x3FF00000,
      lrmant = 0x000FFFFF
    };

    static DWORD Seed(char *password);

    static DWORD uint32_(CRndSeed &seed) {
      DWORD acc = seed.rndacc;
      DWORD vls = seed.rndvls;
      long  r3 = static_cast<long>((vls >> 24) & 0xFF) - 4;
      long  r2 = static_cast<long>((vls >> 16) & 0xFF) - 12;

      if (r3 < 0) {
        r3 += 47 * 4;
      }
      if (r2 < 0) {
        r2 += 53 * 4;
      }

      DWORD n1 = *reinterpret_cast<const DWORD *>(reinterpret_cast<const BYTE *>(gnoise32_) + r3);
      n1 = (n1 << 1) | (n1 >> 31);
      n1 ^= (*reinterpret_cast<const DWORD *>(reinterpret_cast<const BYTE *>(gnoise32_) + r2) << 2) |
            (*reinterpret_cast<const DWORD *>(reinterpret_cast<const BYTE *>(gnoise32_) + r2) >> 30);
      vls = (static_cast<DWORD>(r3) << 8) | static_cast<DWORD>(r2);

      r3 = static_cast<long>((seed.rndvls >> 8) & 0xFF) - 24;
      r2 = static_cast<long>(seed.rndvls & 0xFF) - 28;
      if (r3 < 0) {
        r3 += 59 * 4;
      }
      if (r2 < 0) {
        r2 += 61 * 4;
      }

      n1 ^= (*reinterpret_cast<const DWORD *>(reinterpret_cast<const BYTE *>(gnoise32_) + r3) << 3) |
            (*reinterpret_cast<const DWORD *>(reinterpret_cast<const BYTE *>(gnoise32_) + r3) >> 29);
      n1 ^= *reinterpret_cast<const DWORD *>(reinterpret_cast<const BYTE *>(gnoise32_) + r2);
      acc += n1;
      vls = (vls << 8) | static_cast<DWORD>(r3);
      vls = (vls << 8) | static_cast<DWORD>(r2);

      seed.rndvls = vls;
      seed.rndacc = acc;
      return acc;
    }

    static long int32_(CRndSeed &seed) {
      return static_cast<long>(uint32_(seed));
    }

    static C3Vector C3Vector_(CRndSeed &seed);
    static C2Vector C2Vector_(CRndSeed &seed);

    static float real_(CRndSeed &seed) {
      DWORD value = uint32_(seed);
      DWORD bits = (value & 0x007FFFFF) | 0x3F800000;

      return *reinterpret_cast<float *>(&bits) - 1.0f;
    }

    static double lreal_(CRndSeed &seed) {
      double value;
      DWORD *words = reinterpret_cast<DWORD *>(&value);
      words[1] = (uint32_(seed) & 0x000FFFFF) | 0x3FF00000;
      words[0] = uint32_(seed);
      return value - 1.0;
    }

    static float realp_(CRndSeed &seed) {
      DWORD value = uint32_(seed);
      DWORD bits = (value & 0x007FFFFF) | 0x3F800000;
      return 2.0f - *reinterpret_cast<float *>(&bits);
    }

    static double lrealp_(CRndSeed &seed) {
      double value;
      DWORD *words = reinterpret_cast<DWORD *>(&value);
      words[1] = (uint32_(seed) & 0x000FFFFF) | 0x3FF00000;
      words[0] = uint32_(seed);
      return 2.0 - value;
    }

    static float reals_(CRndSeed &seed) {
      DWORD value = uint32_(seed);
      DWORD bits = (value & 0x007FFFFF) | 0x3F800000;
      float real = *reinterpret_cast<float *>(&bits);

      return static_cast<long>(value) < 0 ? 2.0f - real : real - 2.0f;
    }

    static double lreals_(CRndSeed &seed) {
      DWORD  sign = uint32_(seed);
      double value;
      DWORD *words = reinterpret_cast<DWORD *>(&value);
      words[1] = (sign & 0x000FFFFF) | 0x3FF00000;
      words[0] = uint32_(seed);
      return static_cast<long>(sign) < 0 ? 2.0 - value : value - 2.0;
    }

    static float  reale_(CRndSeed &seed);
    static float  reale_(float mean, CRndSeed &seed);
    static double lreale_(CRndSeed &seed);
    static double lreale_(double mean, CRndSeed &seed);
    static float  realg_(CRndSeed &seed);
    static float  realg_(float mean, float variation, CRndSeed &seed);
    static double lrealg_(CRndSeed &seed);
    static double lrealg_(double mean, double variation, CRndSeed &seed);

    static DWORD dice_(DWORD sides, CRndSeed &seed) {
      ASSERT(sides > 0);
      return CMath::mulhwu_(sides, uint32_(seed));
    }
    static DWORD dice_(DWORD low, DWORD high, CRndSeed &seed) {
      ASSERT(low <= high);
      return low + dice_(high - low + 1, seed);
    }
    static bool coin_(CRndSeed &seed) {
      return static_cast<long>(uint32_(seed)) < 0;
    }
    static bool coin_(DWORD sides, CRndSeed &seed) {
      return dice_(sides, seed) == 0;
    }
    static bool coin_(DWORD successes, DWORD sides, CRndSeed &seed) {
      return dice_(sides, seed) < successes;
    }
    static bool coin_(float probability, CRndSeed &seed) {
      return real_(seed) < probability;
    }

    static void array_(DWORD *buf, DWORD count, CRndSeed &seed);
    static void array_(long *buf, DWORD count, CRndSeed &seed);
    static void array_(float *buf, DWORD count, CRndSeed &seed);
    static void array_(double *buf, DWORD count, CRndSeed &seed);
    static void array_(C2Vector *buf, DWORD count, CRndSeed &seed);
    static void array_(C3Vector *buf, DWORD count, CRndSeed &seed);
    static void arrayp_(float *buf, DWORD count, CRndSeed &seed);
    static void arrayp_(double *buf, DWORD count, CRndSeed &seed);
    static void arrays_(float *buf, DWORD count, CRndSeed &seed);
    static void arrays_(double *buf, DWORD count, CRndSeed &seed);
    static void arraye_(float *buf, DWORD count, CRndSeed &seed);
    static void arraye_(double *buf, DWORD count, CRndSeed &seed);
    static void arraye_(float *buf, DWORD count, float mean, CRndSeed &seed);
    static void arraye_(double *buf, DWORD count, double mean, CRndSeed &seed);
    static void arrayg_(float *buf, DWORD count, CRndSeed &seed);
    static void arrayg_(double *buf, DWORD count, CRndSeed &seed);
    static void arrayg_(float *buf, DWORD count, float mean, float variation, CRndSeed &seed);
    static void arrayg_(double *buf, DWORD count, double mean, double variation, CRndSeed &seed);

    static void shuffle_(char *buf, CRndSeed &seed);
    static void shuffle_(char *buf, DWORD count, CRndSeed &seed);
    static void shuffle_(BYTE *buf, DWORD count, CRndSeed &seed);
    static void shuffle_(short *buf, DWORD count, CRndSeed &seed);
    static void shuffle_(WORD *buf, DWORD count, CRndSeed &seed);
    static void shuffle_(long *buf, DWORD count, CRndSeed &seed);
    static void shuffle_(DWORD *buf, DWORD count, CRndSeed &seed);
    static void shuffle_(float *buf, DWORD count, CRndSeed &seed);
    static void shuffle_(double *buf, DWORD count, CRndSeed &seed);
    static void crypt_(char *buf, DWORD size, DWORD seed);
    static void crypt_(char *buf, DWORD size, char *password);

    static void  checksum_(const CRndSeed &seed, DWORD &checksum);
    static DWORD checksum_(DWORD value);
    static void  checksum8_(DWORD value, DWORD &checksum);
    static void  checksum16_(DWORD value, DWORD &checksum);
    static void  checksum32_(DWORD value, DWORD &checksum);
    static void  checksumr_(float value, DWORD &checksum);
    static void  checksumm32_(const DWORD *values, DWORD count, DWORD mask, DWORD &checksum);
    static void  checksumm32_(const DWORD *values, DWORD count, DWORD &checksum);
    static void  checksumm16_(const WORD *values, DWORD count, WORD mask, DWORD &checksum);
    static void  checksumm16_(const WORD *values, DWORD count, DWORD &checksum);
    static void  checksumm8_(const BYTE *values, DWORD count, BYTE mask, DWORD &checksum);
    static void  checksumm8_(const BYTE *values, DWORD count, DWORD &checksum);
    static void  checksumms_(LPCSTR *strings, DWORD count, DWORD &checksum);

    static DWORD lattice_(long x);
    static DWORD lattice_(long x, long y);
    static DWORD lattice_(long x, long y, long z);
    static DWORD lattice_(long x, long y, long z, long w);
    static void  lattice2_(long x, DWORD *vertices);
    static void  lattice4_(long x, long y, DWORD *vertices);
    static void  lattice8_(long x, long y, long z, DWORD *vertices);
    static void  lattice3_(long x, DWORD *vertices);
    static void  lattice9_(long x, long y, DWORD *vertices);
    static void  lattice27_(long x, long y, long z, DWORD *vertices);
    static float noise_(double x);
    static float noise_(double x, double y);
    static float noise_(double x, double y, double z);
    static float noise_(double x, double y, double z, C3Vector &derivative);
    static float turbulence_(double x, double y, double z, C3Vector &derivative, DWORD octaves);
  };

}  // namespace NTempest
