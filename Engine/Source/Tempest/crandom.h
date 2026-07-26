#pragma once

#include "cmath.h"

namespace NTempest {

  class C2Vector;
  class C3Vector;

  extern const unsigned long gnoise32_[64];

  class CRndSeed {
   public:
    CRndSeed() : rndacc(0), rndvls(0) {}
    explicit CRndSeed(unsigned long seed) { SetSeed(seed); }
    explicit CRndSeed(char *password) { SetSeed(password); }
    ~CRndSeed() {}

    void SetSeed(char *password);
    void SetSeed(unsigned long seed);

   protected:
    unsigned long rndacc;
    unsigned long rndvls;

    friend class CRandom;
  };

  class CRandom {
   public:
    enum {
      rexp = 0x3F800000,
      rmant = 0x007FFFFF,
      lrexp = 0x3FF00000,
      lrmant = 0x000FFFFF
    };

    static unsigned long __fastcall Seed(char *password);

    static unsigned long __fastcall uint32_(CRndSeed &seed) {
      unsigned long acc = seed.rndacc;
      unsigned long vls = seed.rndvls;
      long          r3 = static_cast<long>((vls >> 24) & 0xFF) - 4;
      long          r2 = static_cast<long>((vls >> 16) & 0xFF) - 12;

      if (r3 < 0) {
        r3 += 47 * 4;
      }
      if (r2 < 0) {
        r2 += 53 * 4;
      }

      unsigned long n1 = *reinterpret_cast<const unsigned long *>(reinterpret_cast<const unsigned char *>(gnoise32_) + r3);
      n1 = (n1 << 1) | (n1 >> 31);
      n1 ^= (*reinterpret_cast<const unsigned long *>(reinterpret_cast<const unsigned char *>(gnoise32_) + r2) << 2) |
            (*reinterpret_cast<const unsigned long *>(reinterpret_cast<const unsigned char *>(gnoise32_) + r2) >> 30);
      vls = (static_cast<unsigned long>(r3) << 8) | static_cast<unsigned long>(r2);

      r3 = static_cast<long>((seed.rndvls >> 8) & 0xFF) - 24;
      r2 = static_cast<long>(seed.rndvls & 0xFF) - 28;
      if (r3 < 0) {
        r3 += 59 * 4;
      }
      if (r2 < 0) {
        r2 += 61 * 4;
      }

      n1 ^= (*reinterpret_cast<const unsigned long *>(reinterpret_cast<const unsigned char *>(gnoise32_) + r3) << 3) |
            (*reinterpret_cast<const unsigned long *>(reinterpret_cast<const unsigned char *>(gnoise32_) + r3) >> 29);
      n1 ^= *reinterpret_cast<const unsigned long *>(reinterpret_cast<const unsigned char *>(gnoise32_) + r2);
      acc += n1;
      vls = (vls << 8) | static_cast<unsigned long>(r3);
      vls = (vls << 8) | static_cast<unsigned long>(r2);

      seed.rndvls = vls;
      seed.rndacc = acc;
      return acc;
    }

    static long __fastcall int32_(CRndSeed &seed) {
      return static_cast<long>(uint32_(seed));
    }

    static C3Vector __fastcall C3Vector_(CRndSeed &seed);
    static C2Vector __fastcall C2Vector_(CRndSeed &seed);

    static float __fastcall real_(CRndSeed &seed) {
      unsigned long value = uint32_(seed);
      unsigned long bits = (value & 0x007FFFFF) | 0x3F800000;

      return *reinterpret_cast<float *>(&bits) - 1.0f;
    }

    static double __fastcall lreal_(CRndSeed &seed) {
      double value;
      unsigned long *words = reinterpret_cast<unsigned long *>(&value);
      words[1] = (uint32_(seed) & 0x000FFFFF) | 0x3FF00000;
      words[0] = uint32_(seed);
      return value - 1.0;
    }

    static float __fastcall realp_(CRndSeed &seed) {
      unsigned long value = uint32_(seed);
      unsigned long bits = (value & 0x007FFFFF) | 0x3F800000;
      return 2.0f - *reinterpret_cast<float *>(&bits);
    }

    static double __fastcall lrealp_(CRndSeed &seed) {
      double value;
      unsigned long *words = reinterpret_cast<unsigned long *>(&value);
      words[1] = (uint32_(seed) & 0x000FFFFF) | 0x3FF00000;
      words[0] = uint32_(seed);
      return 2.0 - value;
    }

    static float __fastcall reals_(CRndSeed &seed) {
      unsigned long value = uint32_(seed);
      unsigned long bits = (value & 0x007FFFFF) | 0x3F800000;
      float         real = *reinterpret_cast<float *>(&bits);

      return static_cast<long>(value) < 0 ? 2.0f - real : real - 2.0f;
    }

    static double __fastcall lreals_(CRndSeed &seed) {
      unsigned long sign = uint32_(seed);
      double value;
      unsigned long *words = reinterpret_cast<unsigned long *>(&value);
      words[1] = (sign & 0x000FFFFF) | 0x3FF00000;
      words[0] = uint32_(seed);
      return static_cast<long>(sign) < 0 ? 2.0 - value : value - 2.0;
    }

    static float __fastcall reale_(CRndSeed &seed);
    static float __fastcall reale_(float mean, CRndSeed &seed);
    static double __fastcall lreale_(CRndSeed &seed);
    static double __fastcall lreale_(double mean, CRndSeed &seed);
    static float __fastcall realg_(CRndSeed &seed);
    static float __fastcall realg_(float mean, float variation, CRndSeed &seed);
    static double __fastcall lrealg_(CRndSeed &seed);
    static double __fastcall lrealg_(double mean, double variation, CRndSeed &seed);

    static unsigned long __fastcall dice_(unsigned long sides, CRndSeed &seed) {
      ASSERT(sides > 0);
      return CMath::mulhwu_(sides, uint32_(seed));
    }
    static unsigned long __fastcall dice_(unsigned long low, unsigned long high, CRndSeed &seed) {
      ASSERT(low <= high);
      return low + dice_(high - low + 1, seed);
    }
    static bool __fastcall coin_(CRndSeed &seed) {
      return static_cast<long>(uint32_(seed)) < 0;
    }
    static bool __fastcall coin_(unsigned long sides, CRndSeed &seed) {
      return dice_(sides, seed) == 0;
    }
    static bool __fastcall coin_(unsigned long successes, unsigned long sides, CRndSeed &seed) {
      return dice_(sides, seed) < successes;
    }
    static bool __fastcall coin_(float probability, CRndSeed &seed) {
      return real_(seed) < probability;
    }

    static void __fastcall array_(unsigned long *buf, unsigned long count, CRndSeed &seed);
    static void __fastcall array_(long *buf, unsigned long count, CRndSeed &seed);
    static void __fastcall array_(float *buf, unsigned long count, CRndSeed &seed);
    static void __fastcall array_(double *buf, unsigned long count, CRndSeed &seed);
    static void __fastcall array_(C2Vector *buf, unsigned long count, CRndSeed &seed);
    static void __fastcall array_(C3Vector *buf, unsigned long count, CRndSeed &seed);
    static void __fastcall arrayp_(float *buf, unsigned long count, CRndSeed &seed);
    static void __fastcall arrayp_(double *buf, unsigned long count, CRndSeed &seed);
    static void __fastcall arrays_(float *buf, unsigned long count, CRndSeed &seed);
    static void __fastcall arrays_(double *buf, unsigned long count, CRndSeed &seed);
    static void __fastcall arraye_(float *buf, unsigned long count, CRndSeed &seed);
    static void __fastcall arraye_(double *buf, unsigned long count, CRndSeed &seed);
    static void __fastcall arraye_(float *buf, unsigned long count, float mean, CRndSeed &seed);
    static void __fastcall arraye_(double *buf, unsigned long count, double mean, CRndSeed &seed);
    static void __fastcall arrayg_(float *buf, unsigned long count, CRndSeed &seed);
    static void __fastcall arrayg_(double *buf, unsigned long count, CRndSeed &seed);
    static void __fastcall arrayg_(float *buf, unsigned long count, float mean, float variation, CRndSeed &seed);
    static void __fastcall arrayg_(double *buf, unsigned long count, double mean, double variation, CRndSeed &seed);

    static void __fastcall shuffle_(char *buf, CRndSeed &seed);
    static void __fastcall shuffle_(char *buf, unsigned long count, CRndSeed &seed);
    static void __fastcall shuffle_(unsigned char *buf, unsigned long count, CRndSeed &seed);
    static void __fastcall shuffle_(short *buf, unsigned long count, CRndSeed &seed);
    static void __fastcall shuffle_(unsigned short *buf, unsigned long count, CRndSeed &seed);
    static void __fastcall shuffle_(long *buf, unsigned long count, CRndSeed &seed);
    static void __fastcall shuffle_(unsigned long *buf, unsigned long count, CRndSeed &seed);
    static void __fastcall shuffle_(float *buf, unsigned long count, CRndSeed &seed);
    static void __fastcall shuffle_(double *buf, unsigned long count, CRndSeed &seed);
    static void __fastcall crypt_(char *buf, unsigned long size, unsigned long seed);
    static void __fastcall crypt_(char *buf, unsigned long size, char *password);

    static void __fastcall checksum_(const CRndSeed &seed, unsigned long &checksum);
    static unsigned long __fastcall checksum_(unsigned long value);
    static void __fastcall checksum8_(unsigned long value, unsigned long &checksum);
    static void __fastcall checksum16_(unsigned long value, unsigned long &checksum);
    static void __fastcall checksum32_(unsigned long value, unsigned long &checksum);
    static void __fastcall checksumr_(float value, unsigned long &checksum);
    static void __fastcall checksumm32_(
        const unsigned long *values,
        unsigned long count,
        unsigned long mask,
        unsigned long &checksum
    );
    static void __fastcall checksumm32_(
        const unsigned long *values,
        unsigned long count,
        unsigned long &checksum
    );
    static void __fastcall checksumm16_(
        const unsigned short *values,
        unsigned long count,
        unsigned short mask,
        unsigned long &checksum
    );
    static void __fastcall checksumm16_(
        const unsigned short *values,
        unsigned long count,
        unsigned long &checksum
    );
    static void __fastcall checksumm8_(
        const unsigned char *values,
        unsigned long count,
        unsigned char mask,
        unsigned long &checksum
    );
    static void __fastcall checksumm8_(
        const unsigned char *values,
        unsigned long count,
        unsigned long &checksum
    );
    static void __fastcall checksumms_(
        const char **strings,
        unsigned long count,
        unsigned long &checksum
    );

    static unsigned long __fastcall lattice_(long x);
    static unsigned long __fastcall lattice_(long x, long y);
    static unsigned long __fastcall lattice_(long x, long y, long z);
    static unsigned long __fastcall lattice_(long x, long y, long z, long w);
    static void __fastcall lattice2_(long x, unsigned long *vertices);
    static void __fastcall lattice4_(long x, long y, unsigned long *vertices);
    static void __fastcall lattice8_(long x, long y, long z, unsigned long *vertices);
    static void __fastcall lattice3_(long x, unsigned long *vertices);
    static void __fastcall lattice9_(long x, long y, unsigned long *vertices);
    static void __fastcall lattice27_(long x, long y, long z, unsigned long *vertices);
    static float __fastcall noise_(double x);
    static float __fastcall noise_(double x, double y);
    static float __fastcall noise_(double x, double y, double z);
    static float __fastcall noise_(double x, double y, double z, C3Vector &derivative);
    static float __fastcall turbulence_(double x, double y, double z, C3Vector &derivative, unsigned long octaves);
  };

}  // namespace NTempest
