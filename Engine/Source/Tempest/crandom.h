#pragma once

namespace NTempest {

  class C3Vector;

  extern const unsigned long gnoise32_[64];

  class CRndSeed {
   public:
    void SetSeed(unsigned long seed);

   protected:
    unsigned long rndacc;
    unsigned long rndvls;

    friend class CRandom;
  };

  class CRandom {
   public:
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

    static C3Vector __fastcall C3Vector_(CRndSeed &seed);

    static float __fastcall real_(CRndSeed &seed) {
      unsigned long value = uint32_(seed);
      unsigned long bits = (value & 0x007FFFFF) | 0x3F800000;

      return *reinterpret_cast<float *>(&bits) - 1.0f;
    }

    static float __fastcall reals_(CRndSeed &seed) {
      unsigned long value = uint32_(seed);
      unsigned long bits = (value & 0x007FFFFF) | 0x3F800000;
      float         real = *reinterpret_cast<float *>(&bits);

      return static_cast<long>(value) < 0 ? 2.0f - real : real - 2.0f;
    }
  };

}  // namespace NTempest
