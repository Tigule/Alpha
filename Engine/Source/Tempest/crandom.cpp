#include <Base/Base.h>

#include "crandom.h"

#include "c2vector.h"
#include "c3vector.h"
#include "cmath.h"

static inline DWORD lattice_(long x) {
  x ^= ((ulong(x) >> 11) | (ulong(x) << 21)) ^ ((ulong(x) >> 21) | (ulong(x) << 11));

  DWORD n1 = *reinterpret_cast<const DWORD *>(reinterpret_cast<const BYTE *>(NTempest::gnoise32_) + ((ulong(x) >> 4) & 0xFC));
  DWORD n2 = *reinterpret_cast<const DWORD *>(reinterpret_cast<const BYTE *>(NTempest::gnoise32_) + ((ulong(x) >> 10) & 0xFC));
  DWORD n3 = *reinterpret_cast<const DWORD *>(reinterpret_cast<const BYTE *>(NTempest::gnoise32_) + ((ulong(x) >> 16) & 0xFC));
  return ((n3 << 3) | (n3 >> 29)) ^ ((n2 << 2) | (n2 >> 30)) ^ ((n1 << 1) | (n1 >> 31)) ^ NTempest::gnoise32_[x & 0x3F];
}

namespace NTempest {

  extern const DWORD gnoise32_[64] = {
      0x9927148EUL,
      0x08C7AAFDUL,
      0x1F3EE6D5UL,
      0xDA55BBF6UL,
      0x6A4AA075UL,
      0xFF97BDE8UL,
      0x9FBC9BDEUL,
      0x46A18A81UL,
      0x63E30B6EUL,
      0x5D6C7A76UL,
      0xCA69D388UL,
      0x25B947C3UL,
      0x3FA2AB83UL,
      0xBA7C41A6UL,
      0x0195ACE5UL,
      0xC109CF7EUL,
      0x717062D9UL,
      0x0205DB8DUL,
      0x54EF8724UL,
      0x3037D4C6UL,
      0x7BCB1BD0UL,
      0xECD8E4B8UL,
      0xDCADCE49UL,
      0xC494A913UL,
      0x0DAE398FUL,
      0x0EDD5218UL,
      0x85F5FA78UL,
      0x6DAFD258UL,
      0x3B53B2A4UL,
      0xBE50A551UL,
      0x11F42DFCUL,
      0xF1169848UL,
      0x663DDF86UL,
      0x2F2E445EUL,
      0x176B0736UL,
      0xB64C298BUL,
      0xE75F89E2UL,
      0xE121A7CDUL,
      0xED65C94DUL,
      0x239CEEFEUL,
      0x04B77D33UL,
      0x402A9A9EUL,
      0xF35B10B3UL,
      0x921C7782UL,
      0x571E4E20UL,
      0x8C067222UL,
      0xFB732C67UL,
      0xBF0AC259UL,
      0x0CF95C79UL,
      0x68121A28UL,
      0x42193474UL,
      0xF884C0B1UL,
      0x9D15F038UL,
      0x6F3AF260UL,
      0x91EB90B4UL,
      0x61357F1DUL,
      0x5603325AUL,
      0x932BC5A3UL,
      0x434B0F80UL,
      0x3CE0A8F7UL,
      0x2664D196UL,
      0x4FCC45D7UL,
      0xB5E9B0C8UL,
      0xEA31D600UL,
  };

  static const float fr4s[16] = {
      -1.0f,
      -0.8667f,
      -0.7333f,
      -0.6f,
      -0.4667f,
      -0.3333f,
      -0.2f,
      -0.0667f,
      0.0667f,
      0.2f,
      0.3333f,
      0.4667f,
      0.6f,
      0.7333f,
      0.8667f,
      1.0f,
  };

  static const float fr5s[31] = {
      -2.0f,
      -1.8667f,
      -1.7333f,
      -1.6f,
      -1.4667f,
      -1.3333f,
      -1.2f,
      -1.0667f,
      -0.9333f,
      -0.8f,
      -0.6667f,
      -0.5333f,
      -0.4f,
      -0.2667f,
      -0.1333f,
      0.0f,
      0.1333f,
      0.2667f,
      0.4f,
      0.5333f,
      0.6667f,
      0.8f,
      0.9333f,
      1.0667f,
      1.2f,
      1.3333f,
      1.4667f,
      1.6f,
      1.7333f,
      1.8667f,
      2.0f,
  };

  static const float fr5u[31] = {
      -1.0f,
      -0.9333f,
      -0.8667f,
      -0.8f,
      -0.7333f,
      -0.6667f,
      -0.6f,
      -0.5333f,
      -0.4667f,
      -0.4f,
      -0.3333f,
      -0.2667f,
      -0.2f,
      -0.1333f,
      -0.0667f,
      0.0f,
      0.0667f,
      0.1333f,
      0.2f,
      0.2667f,
      0.3333f,
      0.4f,
      0.4667f,
      0.5333f,
      0.6f,
      0.6667f,
      0.7333f,
      0.8f,
      0.8667f,
      0.9333f,
      1.0f,
  };

  static const float *fr4u = &fr5u[15];
  static const float *fr4d = &fr5s[15];

  void CRndSeed::SetSeed(DWORD seed) {
    rndacc = seed;
    rndvls = 4 * ((seed % 61) | (((seed % 59) | (((seed % 53) | ((seed % 47) << 8)) << 8)) << 8));
  }

  void CRndSeed::SetSeed(char *password) {
    SetSeed(CRandom::Seed(password));
  }

  DWORD CRandom::Seed(char *password) {
    ASSERT(password != 0);
    DWORD length = SStrLen(password);
    DWORD seed = static_cast<BYTE>(password[0]);
    for (DWORD i = 1; i < length; ++i) {
      seed = (seed ^ 31277 * seed) + static_cast<BYTE>(password[i]);
    }
    return seed;
  }

  void CRandom::array_(DWORD *buf, DWORD count, CRndSeed &seed) {
    DWORD acc = seed.rndacc;
    DWORD vls = seed.rndvls;
    long  r1 = static_cast<long>(vls >> 24);
    long  r2 = static_cast<long>((vls >> 16) & 0xFF);
    long  r3 = static_cast<long>((vls >> 8) & 0xFF);
    long  r4 = static_cast<long>(vls & 0xFF);
    for (DWORD ind = 0; ind < count; ++ind) {
      r1 -= 4;
      r2 -= 12;
      if (r1 < 0) {
        r1 += 47 * 4;
      }
      r3 -= 24;
      if (r2 < 0) {
        r2 += 53 * 4;
      }
      DWORD n1 = *reinterpret_cast<const DWORD *>(reinterpret_cast<const BYTE *>(gnoise32_) + r1);
      r4 -= 28;
      if (r3 < 0) {
        r3 += 59 * 4;
      }
      DWORD n2 = *reinterpret_cast<const DWORD *>(reinterpret_cast<const BYTE *>(gnoise32_) + r2);
      n1 = (n1 << 1) | (n1 >> 31);
      if (r4 < 0) {
        r4 += 61 * 4;
      }
      DWORD n3 = *reinterpret_cast<const DWORD *>(reinterpret_cast<const BYTE *>(gnoise32_) + r3);
      n3 = (n3 << 3) | (n3 >> 29);
      n2 = (n2 << 2) | (n2 >> 30);
      DWORD n4 = *reinterpret_cast<const DWORD *>(reinterpret_cast<const BYTE *>(gnoise32_) + r4);
      acc += n3 ^ n2 ^ n4 ^ n1;
      buf[ind] = acc;
    }

    seed.rndvls = (((((static_cast<DWORD>(r1) << 8) | static_cast<DWORD>(r2)) << 8) | static_cast<DWORD>(r3)) << 8) | static_cast<DWORD>(r4);
    seed.rndacc = acc;
  }

  void CRandom::array_(long *buf, DWORD count, CRndSeed &seed) {
    ASSERT(buf != 0);
    array_(reinterpret_cast<DWORD *>(buf), count, seed);
  }

  void CRandom::array_(float *buf, DWORD count, CRndSeed &seed) {
    ASSERT(buf != 0);
    for (DWORD ind = 0; ind < count; ++ind)
      buf[ind] = real_(seed);
  }

  void CRandom::array_(double *buf, DWORD count, CRndSeed &seed) {
    ASSERT(buf != 0);
    for (DWORD ind = 0; ind < count; ++ind)
      buf[ind] = lreal_(seed);
  }

  void CRandom::arrayp_(float *buf, DWORD count, CRndSeed &seed) {
    ASSERT(buf != 0);
    for (DWORD ind = 0; ind < count; ++ind)
      buf[ind] = realp_(seed);
  }

  void CRandom::arrayp_(double *buf, DWORD count, CRndSeed &seed) {
    ASSERT(buf != 0);
    for (DWORD ind = 0; ind < count; ++ind)
      buf[ind] = lrealp_(seed);
  }

  void CRandom::arrays_(float *buf, DWORD count, CRndSeed &seed) {
    ASSERT(buf != 0);
    for (DWORD ind = 0; ind < count; ++ind)
      buf[ind] = reals_(seed);
  }

  void CRandom::arrays_(double *buf, DWORD count, CRndSeed &seed) {
    ASSERT(buf != 0);
    for (DWORD ind = 0; ind < count; ++ind)
      buf[ind] = lreals_(seed);
  }

  float CRandom::reale_(CRndSeed &seed) {
    return static_cast<float>(CMath::log2_(realp_(seed)) * -0.69314718f);
  }

  double CRandom::lreale_(CRndSeed &seed) {
    return CMath::log2_(lrealp_(seed)) * -0.6931471805599453;
  }

  float CRandom::reale_(float mean, CRndSeed &seed) {
    return static_cast<float>(CMath::log2_(realp_(seed)) * mean * -0.69314718f);
  }

  double CRandom::lreale_(double mean, CRndSeed &seed) {
    return CMath::log2_(lrealp_(seed)) * mean * -0.6931471805599453;
  }

  float CRandom::realg_(CRndSeed &seed) {
    static float cache = 0.0f;
    if (cache != 0.0f) {
      float result = cache;
      cache = 0.0f;
      return result;
    }

    float radius = realp_(seed);
    float s;
    float c;
    CMath::sincos_(real_(seed) * 6.2831855f, s, c);
    radius = CMath::sqrt_(static_cast<float>(CMath::log2_(radius) * -1.3862944f));
    cache = s * radius;
    return radius * c;
  }

  double CRandom::lrealg_(CRndSeed &seed) {
    static double cache = 0.0;
    if (cache != 0.0) {
      double result = cache;
      cache = 0.0;
      return result;
    }

    double radius = lrealp_(seed);
    double s;
    double c;
    CMath::sincos_(lreal_(seed) * 6.28318530718, s, c);
    radius = CMath::sqrt_(CMath::log2_(radius) * -1.3862943611198906);
    cache = s * radius;
    return radius * c;
  }

  float CRandom::realg_(float mean, float var, CRndSeed &seed) {
    return realg_(seed) * var + mean;
  }

  double CRandom::lrealg_(double mean, double var, CRndSeed &seed) {
    return lrealg_(seed) * var + mean;
  }

  void CRandom::arraye_(float *buf, DWORD count, CRndSeed &seed) {
    ASSERT(buf != 0);
    for (DWORD i = 0; i < count; ++i)
      buf[i] = reale_(seed);
  }

  void CRandom::arraye_(double *buf, DWORD count, CRndSeed &seed) {
    ASSERT(buf != 0);
    for (DWORD i = 0; i < count; ++i)
      buf[i] = lreale_(seed);
  }

  void CRandom::arraye_(float *buf, DWORD count, float mean, CRndSeed &seed) {
    ASSERT(buf != 0);
    for (DWORD i = 0; i < count; ++i)
      buf[i] = reale_(mean, seed);
  }

  void CRandom::arraye_(double *buf, DWORD count, double mean, CRndSeed &seed) {
    ASSERT(buf != 0);
    for (DWORD i = 0; i < count; ++i)
      buf[i] = lreale_(mean, seed);
  }

  void CRandom::arrayg_(float *buf, DWORD count, CRndSeed &seed) {
    ASSERT(buf != 0);
    for (DWORD i = 0; i < count; ++i)
      buf[i] = realg_(seed);
  }

  void CRandom::arrayg_(double *buf, DWORD count, CRndSeed &seed) {
    ASSERT(buf != 0);
    for (DWORD i = 0; i < count; ++i)
      buf[i] = lrealg_(seed);
  }

  void CRandom::arrayg_(float *buf, DWORD count, float mean, float variation, CRndSeed &seed) {
    ASSERT(buf != 0);
    for (DWORD i = 0; i < count; ++i)
      buf[i] = realg_(mean, variation, seed);
  }

  void CRandom::arrayg_(double *buf, DWORD count, double mean, double variation, CRndSeed &seed) {
    ASSERT(buf != 0);
    for (DWORD i = 0; i < count; ++i)
      buf[i] = lrealg_(mean, variation, seed);
  }

  C2Vector CRandom::C2Vector_(CRndSeed &seed) {
    float angle = real_(seed) * 6.2831855f;
    return C2Vector(CMath::cos_(angle), CMath::sin_(angle));
  }

  C3Vector CRandom::C3Vector_(CRndSeed &seed) {
    const float _1 = 1.0f;
    const float z = reals_(seed);
    const float angle = real_(seed) * 6.2831855f;

    ASSERT(z >= -_1 && z <= _1);

    const float radius = CMath::sqrt_(_1 - z * z);

    return C3Vector(CMath::cos_(angle) * radius, CMath::sin_(angle) * radius, z);
  }

  void CRandom::array_(C2Vector *buf, DWORD count, CRndSeed &seed) {
    ASSERT(buf != 0);
    for (DWORD i = 0; i < count; ++i)
      buf[i] = C2Vector_(seed);
  }

  void CRandom::array_(C3Vector *buf, DWORD count, CRndSeed &seed) {
    ASSERT(buf != 0);
    for (DWORD i = 0; i < count; ++i)
      buf[i] = C3Vector_(seed);
  }

  void CRandom::shuffle_(char *buf, CRndSeed &seed) {
    ASSERT(buf != 0);
    DWORD count = SStrLen(buf);
    shuffle_(reinterpret_cast<BYTE *>(buf), count, seed);
  }

  void CRandom::shuffle_(char *buf, DWORD count, CRndSeed &seed) {
    ASSERT(buf != 0);
    shuffle_(reinterpret_cast<BYTE *>(buf), count, seed);
  }

  void CRandom::shuffle_(BYTE *buf, DWORD count, CRndSeed &seed) {
    ASSERT(buf != 0);
    if (count) {
      for (DWORD i = 1; i < count; ++i) {
        DWORD bi = buf[i];
        DWORD index = dice_(i + 1, seed);
        buf[i] = buf[index];
        buf[index] = static_cast<BYTE>(bi);
      }
    }
  }

  void CRandom::shuffle_(short *buf, DWORD count, CRndSeed &seed) {
    ASSERT(buf != 0);
    shuffle_(reinterpret_cast<WORD *>(buf), count, seed);
  }

  void CRandom::shuffle_(WORD *buf, DWORD count, CRndSeed &seed) {
    ASSERT(buf != 0);
    if (count) {
      for (DWORD i = 1; i < count; ++i) {
        DWORD bi = buf[i];
        DWORD index = dice_(i + 1, seed);
        buf[i] = static_cast<BYTE>(buf[index]);
        buf[index] = static_cast<BYTE>(bi);
      }
    }
  }

  void CRandom::shuffle_(long *buf, DWORD count, CRndSeed &seed) {
    ASSERT(buf != 0);
    shuffle_(reinterpret_cast<DWORD *>(buf), count, seed);
  }

  void CRandom::shuffle_(DWORD *buf, DWORD count, CRndSeed &seed) {
    ASSERT(buf != 0);
    if (count) {
      for (DWORD i = 1; i < count; ++i) {
        DWORD bi = buf[i];
        DWORD index = dice_(i + 1, seed);
        buf[i] = buf[index];
        buf[index] = bi;
      }
    }
  }

  void CRandom::shuffle_(float *buf, DWORD count, CRndSeed &seed) {
    ASSERT(buf != 0);
    if (count) {
      for (DWORD i = 1; i < count; ++i) {
        float bi = buf[i];
        DWORD index = dice_(i + 1, seed);
        float bj = buf[index];
        buf[i] = bj;
        buf[index] = bi;
      }
    }
  }

  void CRandom::shuffle_(double *buf, DWORD count, CRndSeed &seed) {
    ASSERT(buf != 0);
    if (count) {
      for (DWORD i = 1; i < count; ++i) {
        double bi = buf[i];
        DWORD index = dice_(i + 1, seed);
        double bj = buf[index];
        buf[i] = bj;
        buf[index] = bi;
      }
    }
  }

  void CRandom::crypt_(char *buf, DWORD size, DWORD seednum) {
    ASSERT((ulong(buf) & 3) == 0);
    CRndSeed seed(seednum);
    DWORD    offset = -ulong(buf) & 3;
    buf += offset;
    size = (size - offset) >> 2;
    for (DWORD ind = 0; ind < size; ++ind) {
      buf[ind] ^= static_cast<char>(uint32_(seed));
    }
  }

  void CRandom::crypt_(char *buf, DWORD size, char *password) {
    ASSERT(password != 0);
    crypt_(buf, size, Seed(password));
  }

  DWORD CRandom::lattice_(long x) {
    return ::lattice_(x);
  }

  DWORD CRandom::lattice_(long x, long y) {
    DWORD value = ::lattice_(y);
    return ::lattice_(x ^ static_cast<long>((value << 4) | (value >> 28)));
  }

  DWORD CRandom::lattice_(long x, long y, long z) {
    DWORD value = ::lattice_(z);
    value = ::lattice_(y ^ static_cast<long>((value << 4) | (value >> 28)));
    return ::lattice_(x ^ static_cast<long>((value << 4) | (value >> 28)));
  }

  DWORD CRandom::lattice_(long x, long y, long z, long w) {
    DWORD value = ::lattice_(w);
    value = ::lattice_(z ^ static_cast<long>((value << 4) | (value >> 28)));
    value = ::lattice_(y ^ static_cast<long>((value << 4) | (value >> 28)));
    return ::lattice_(x ^ static_cast<long>((value << 4) | (value >> 28)));
  }

  void CRandom::lattice2_(long x, DWORD *vtx) {
    ASSERT(vtx != 0);
    vtx[0] = ::lattice_(x);
    vtx[1] = ::lattice_(x + 1);
  }

  void CRandom::lattice4_(long x, long y, DWORD *vtx) {
    ASSERT(vtx != 0);
    DWORD va = ::lattice_(y);
    va = (va << 4) | (va >> 28);
    DWORD vb = ::lattice_(y + 1);
    vb = (vb << 4) | (vb >> 28);
    vtx[0] = ::lattice_(va ^ x);
    vtx[1] = ::lattice_((x + 1) ^ va);
    vtx[2] = ::lattice_(vb ^ x);
    vtx[3] = ::lattice_((x + 1) ^ vb);
  }

  void CRandom::lattice8_(long x, long y, long z, DWORD *vtx) {
    ASSERT(vtx != 0);
    DWORD v0 = ::lattice_(z);
    v0 = (v0 << 4) | (v0 >> 28);
    DWORD v1 = ::lattice_(z + 1);
    v1 = (v1 << 4) | (v1 >> 28);
    DWORD va = ::lattice_(v0 ^ y);
    va = (va << 4) | (va >> 28);
    DWORD vb = ::lattice_((y + 1) ^ v0);
    vb = (vb << 4) | (vb >> 28);
    DWORD vc = ::lattice_(v1 ^ y);
    vc = (vc << 4) | (vc >> 28);
    DWORD vd = ::lattice_((y + 1) ^ v1);
    vd = (vd << 4) | (vd >> 28);
    vtx[0] = ::lattice_(va ^ x);
    vtx[1] = ::lattice_((x + 1) ^ va);
    vtx[2] = ::lattice_(vb ^ x);
    vtx[3] = ::lattice_((x + 1) ^ vb);
    vtx[4] = ::lattice_(vc ^ x);
    vtx[5] = ::lattice_((x + 1) ^ vc);
    vtx[6] = ::lattice_(vd ^ x);
    vtx[7] = ::lattice_((x + 1) ^ vd);
  }

  void CRandom::lattice3_(long x, DWORD *vtx) {
    ASSERT(vtx != 0);
    vtx[0] = ::lattice_(x - 1);
    vtx[1] = ::lattice_(x);
    vtx[2] = ::lattice_(x + 1);
  }

  void CRandom::lattice9_(long x, long y, DWORD *vtx) {
    ASSERT(vtx != 0);
    DWORD va = ::lattice_(y - 1);
    va = (va << 4) | (va >> 28);
    DWORD vb = ::lattice_(y);
    vb = (vb << 4) | (vb >> 28);
    DWORD vc = ::lattice_(y + 1);
    vc = (vc << 4) | (vc >> 28);
    vtx[0] = ::lattice_((x - 1) ^ va);
    vtx[1] = ::lattice_(va ^ x);
    vtx[2] = ::lattice_((x + 1) ^ va);
    vtx[3] = ::lattice_((x - 1) ^ vb);
    vtx[4] = ::lattice_(vb ^ x);
    vtx[5] = ::lattice_((x + 1) ^ vb);
    vtx[6] = ::lattice_((x - 1) ^ vc);
    vtx[7] = ::lattice_(vc ^ x);
    vtx[8] = ::lattice_((x + 1) ^ vc);
  }

  void CRandom::lattice27_(long x, long y, long zc, DWORD *vtx) {
    ASSERT(vtx != 0);
    for (long z = zc - 1; z <= zc + 1; ++z) {
      DWORD v0 = ::lattice_(z);
      v0 = (v0 << 4) | (v0 >> 28);
      DWORD va = ::lattice_((y - 1) ^ v0);
      va = (va << 4) | (va >> 28);
      DWORD vb = ::lattice_(v0 ^ y);
      vb = (vb << 4) | (vb >> 28);
      DWORD vc = ::lattice_((y + 1) ^ v0);
      vc = (vc << 4) | (vc >> 28);
      vtx[0] = ::lattice_((x - 1) ^ va);
      vtx[1] = ::lattice_(va ^ x);
      vtx[2] = ::lattice_((x + 1) ^ va);
      vtx[3] = ::lattice_((x - 1) ^ vb);
      vtx[4] = ::lattice_(vb ^ x);
      vtx[5] = ::lattice_((x + 1) ^ vb);
      vtx[6] = ::lattice_((x - 1) ^ vc);
      vtx[7] = ::lattice_(vc ^ x);
      vtx[8] = ::lattice_((x + 1) ^ vc);
      vtx += 9;
    }
  }

  float CRandom::noise_(double x) {
    long integer = static_cast<long>(x);
    if (x < 0.0)
      --integer;
    double fraction = x - integer;

    DWORD first = lattice_(integer);
    DWORD second = lattice_(integer + 1);
    DWORD bits = 0x40000000 | ((first & 0xFFFF) << 7);
    float firstSlope = *reinterpret_cast<float *>(&bits) - 3.0f;
    bits = 0x40000000 | ((second & 0xFFFF) << 7);
    float secondSlope = *reinterpret_cast<float *>(&bits) - 3.0f;
    bits = 0x40400000 + ((first >> 10) & 0x3FFFC0);
    float firstValue = *reinterpret_cast<float *>(&bits) - 3.0f;
    bits = 0x40400000 + ((second >> 10) & 0x3FFFC0) - ((first >> 10) & 0x3FFFC0);
    float difference = *reinterpret_cast<float *>(&bits) - 3.0f;

    double fraction2 = fraction * fraction;
    double value = (((fraction - 2.0) * fraction2 + fraction) * firstSlope + (3.0 - fraction - fraction) * fraction2 * difference +
                    (fraction2 * fraction - fraction2) * secondSlope + firstValue) *
                       0.75 +
                   0.125;
    return static_cast<float>((3.0 - value - value) * value * value);
  }

  float CRandom::noise_(double x, double y) {
    long xi = static_cast<long>(x);
    long yi = static_cast<long>(y);
    if (x < 0.0)
      --xi;
    if (y < 0.0)
      --yi;
    float xf = static_cast<float>(x - xi);
    float yf = static_cast<float>(y - yi);
    DWORD vertices[4];
    lattice4_(xi, yi, vertices);

    float values[4];
    float derivativesX[4];
    float derivativesY[4];
    float derivativesXY[4];
    for (DWORD i = 0; i < 4; ++i) {
      DWORD bits = (vertices[i] >> 10 & 0x3FF800) | 0x40400000;
      values[i] = *reinterpret_cast<float *>(&bits);
      bits = ((vertices[i] & 0x3F80) | 0x200000) << 9;
      derivativesX[i] = *reinterpret_cast<float *>(&bits);
      bits = 4 * ((vertices[i] & 0x1FC000) | 0x10000000);
      derivativesY[i] = *reinterpret_cast<float *>(&bits);
      bits = ((vertices[i] & 0x7F) | 0x4000) << 16;
      derivativesXY[i] = *reinterpret_cast<float *>(&bits);
    }

    float xf2 = xf * xf;
    float yf2 = yf * yf;
    float hx0 = (xf - 2.0f) * xf2 + xf;
    float hx1 = xf2 * xf - xf2;
    float hxv = (3.0f - xf - xf) * xf2;
    float hy0 = (yf - 2.0f) * yf2 + yf;
    float hy1 = yf2 * yf - yf2;
    float hyv = (3.0f - yf - yf) * yf2;

    float row0 = (values[1] - values[0]) * hxv + derivativesX[0] * hx0 + derivativesX[1] * hx1 + values[0];
    float row1 = (values[3] - values[2]) * hxv + derivativesX[2] * hx0 + derivativesX[3] * hx1 + values[2];
    float rowDerivative0 = (derivativesY[1] - derivativesY[0]) * hxv + derivativesXY[0] * hx0 + derivativesXY[1] * hx1 + derivativesY[0];
    float rowDerivative1 = (derivativesY[3] - derivativesY[2]) * hxv + derivativesXY[2] * hx0 + derivativesXY[3] * hx1 + derivativesY[2];
    float value =
        (row1 - row0) * hyv + rowDerivative0 * hy0 + rowDerivative1 * hy1 + row0 - ((hx0 + hx1 + 1.0f) * 3.0f * (hy0 + hy1 + 1.0f) - 0.30000001f);
    return (1.17188f - value * 0.48828f) * value * value;
  }

  float CRandom::noise_(double x, double y, double z) {
    long xi = static_cast<long>(x);
    long yi = static_cast<long>(y);
    long zi = static_cast<long>(z);
    if (x < 0.0)
      --xi;
    if (y < 0.0)
      --yi;
    if (z < 0.0)
      --zi;
    float xp = static_cast<float>(x - xi);
    float yp = static_cast<float>(y - yi);
    float zp = static_cast<float>(z - zi);
    DWORD vtx[8];
    lattice8_(xi, yi, zi, vtx);

    float xp2 = xp * xp;
    float cy[3];
    cy[2] = yp * yp;
    float zp2 = zp * zp;
    float vvm = (3.0f - (xp + xp)) * xp2;
    float d1m = xp2 * xp - xp2;
    float d0m = (xp - 2.0f) * xp2 + xp;
    cy[0] = (3.0f - (yp + yp)) * cy[2];
    cy[1] = (yp - 2.0f) * cy[2] + yp;
    cy[2] = yp * cy[2] - cy[2];
    float cz[3];
    cz[0] = (3.0f - (zp + zp)) * zp2;
    cz[1] = (zp - 2.0f) * zp2 + zp;
    cz[2] = zp2 * zp - zp2;

    float  v_[4][4];
    DWORD *vtp = vtx;
    for (DWORD i = 0; i < 4; ++i) {
      DWORD first = vtp[0];
      DWORD second = vtp[1];
      v_[i][0] = vvm * fr4u[(second >> 28) - (first >> 28)] + d0m * fr4s[(first >> 24) & 15] + d1m * fr4s[(second >> 24) & 15] + fr4u[first >> 28];
      v_[i][1] = vvm * fr4d[((second >> 20) & 15) - ((first >> 20) & 15)] + d0m * fr4s[(first >> 16) & 15] + d1m * fr4s[(second >> 16) & 15] +
                 fr4s[(first >> 20) & 15];
      v_[i][2] = vvm * fr4d[((second >> 12) & 15) - ((first >> 12) & 15)] + d0m * fr4s[(first >> 8) & 15] + d1m * fr4s[(second >> 8) & 15] +
                 fr4s[(first >> 12) & 15];
      v_[i][3] = vvm * fr4d[((second >> 4) & 15) - ((first >> 4) & 15)] + d0m * fr4s[first & 15] + d1m * fr4s[second & 15] + fr4s[(first >> 4) & 15];
      vtp += 2;
    }

    float val = (v_[2][0] - v_[0][0]) * cz[0] + v_[2][2] * cz[2] + v_[0][2] * cz[1] + v_[0][0];
    val = (((v_[3][0] - v_[1][0]) * cz[0] + v_[3][2] * cz[2] + v_[1][2] * cz[1] + v_[1][0] - val) * cy[0] +
           ((v_[2][1] - v_[0][1]) * cz[0] + v_[2][3] * cz[2] + v_[0][3] * cz[1] + v_[0][1]) * cy[1] +
           ((v_[3][1] - v_[1][1]) * cz[0] + v_[3][3] * cz[2] + v_[1][3] * cz[1] + v_[1][1]) * cy[2] + val) *
              0.625f +
          0.1875f;
    return (3.0f - (val + val)) * (val * val);
  }

  float CRandom::noise_(double x, double y, double z, C3Vector &derivative) {
    long xi = static_cast<long>(x);
    long yi = static_cast<long>(y);
    long zi = static_cast<long>(z);
    if (x < 0.0)
      --xi;
    if (y < 0.0)
      --yi;
    if (z < 0.0)
      --zi;
    float xf = static_cast<float>(x - xi);
    float yf = static_cast<float>(y - yi);
    float zf = static_cast<float>(z - zi);
    DWORD vertices[8];
    lattice8_(xi, yi, zi, vertices);

    float xf2 = xf * xf;
    float yf2 = yf * yf;
    float zf2 = zf * zf;
    float hxv = (3.0f - xf - xf) * xf2;
    float hx0 = (xf - 2.0f) * xf2 + xf;
    float hx1 = xf2 * xf - xf2;
    float hyv = (3.0f - yf - yf) * yf2;
    float hy0 = (yf - 2.0f) * yf2 + yf;
    float hy1 = yf2 * yf - yf2;
    float hzv = (3.0f - zf - zf) * zf2;
    float hz0 = (zf - 2.0f) * zf2 + zf;
    float hz1 = zf2 * zf - zf2;

    float xValues[4][4];
    for (DWORD pair = 0; pair < 4; ++pair) {
      DWORD first = vertices[pair * 2];
      DWORD second = vertices[pair * 2 + 1];
      long  a = first >> 28, b = second >> 28;
      xValues[pair][0] = fr4u[b - a] * hxv + fr4s[(first >> 24) & 0xF] * hx0 +
                         fr4s[(second >> 24) & 0xF] * hx1 + fr4u[a];
      a = (first >> 20) & 0xF;
      b = (second >> 20) & 0xF;
      xValues[pair][1] = fr4d[b - a] * hxv +
                         fr4s[(first >> 16) & 0xF] * hx0 +
                         fr4s[(second >> 16) & 0xF] * hx1 + fr4s[a];
      a = (first >> 12) & 0xF;
      b = (second >> 12) & 0xF;
      xValues[pair][2] = fr4d[b - a] * hxv +
                         fr4s[(first >> 8) & 0xF] * hx0 +
                         fr4s[(second >> 8) & 0xF] * hx1 + fr4s[a];
      a = (first >> 4) & 0xF;
      b = (second >> 4) & 0xF;
      xValues[pair][3] = fr4d[b - a] * hxv + fr4s[first & 0xF] * hx0 +
                         fr4s[second & 0xF] * hx1 + fr4s[a];
    }

    float zValues[4][4];
    for (DWORD zPair = 0; zPair < 4; ++zPair) {
      DWORD first = vertices[zPair];
      DWORD second = vertices[zPair + 4];
      long  a = first >> 28, b = second >> 28;
      zValues[zPair][0] = fr4u[b - a] * hzv +
                          fr4s[(first >> 12) & 0xF] * hz0 +
                          fr4s[(second >> 12) & 0xF] * hz1 + fr4u[a];
      a = (first >> 24) & 0xF;
      b = (second >> 24) & 0xF;
      zValues[zPair][1] = fr4d[b - a] * hzv +
                          fr4s[(first >> 8) & 0xF] * hz0 +
                          fr4s[(second >> 8) & 0xF] * hz1 + fr4s[a];
      a = (first >> 20) & 0xF;
      b = (second >> 20) & 0xF;
      zValues[zPair][2] = fr4d[b - a] * hzv +
                          fr4s[(first >> 4) & 0xF] * hz0 +
                          fr4s[(second >> 4) & 0xF] * hz1 + fr4s[a];
      a = (first >> 16) & 0xF;
      b = (second >> 16) & 0xF;
      zValues[zPair][3] = fr4d[b - a] * hzv + fr4s[first & 0xF] * hz0 +
                          fr4s[second & 0xF] * hz1 + fr4s[a];
    }

    float yDerivative1 = (xValues[3][1] - xValues[1][1]) * hzv + xValues[1][3] * hz0 + xValues[3][3] * hz1 + xValues[1][1];
    float yDerivative0 = (xValues[2][1] - xValues[0][1]) * hzv + xValues[0][3] * hz0 + xValues[2][3] * hz1 + xValues[0][1];
    float value0 = (xValues[2][0] - xValues[0][0]) * hzv + xValues[0][2] * hz0 + xValues[2][2] * hz1 + xValues[0][0];
    float valueDifference = (xValues[3][0] - xValues[1][0]) * hzv + xValues[1][2] * hz0 + xValues[3][2] * hz1 + xValues[1][0] - value0;

    float hxDifference = hxv - hxv * hxv;
    float hyDifference = hyv - hyv * hyv;
    float hzDifference = hzv - hzv * hzv;
    float hxEnd = hxv - 3.0f * hxDifference;
    float hyEnd = hyv - 3.0f * hyDifference;
    float hzEnd = hzv - 3.0f * hzDifference;
    derivative.x = ((zValues[2][2] - zValues[0][2]) * hyv + zValues[0][3] * hy0 + zValues[2][3] * hy1 + zValues[0][2]) * (hxEnd - hxv - hxv + 1.0f) +
                   ((zValues[3][0] - zValues[1][0]) * hyv + zValues[1][1] * hy0 + zValues[3][1] * hy1 + zValues[1][0] - zValues[0][0] -
                    (zValues[2][0] - zValues[0][0]) * hyv - zValues[0][1] * hy0 - zValues[2][1] * hy1) *
                       6.0f * hxDifference +
                   ((zValues[3][2] - zValues[1][2]) * hyv + zValues[1][3] * hy0 + zValues[3][3] * hy1 + zValues[1][2]) * hxEnd;
    derivative.y = (hyEnd - hyv - hyv + 1.0f) * yDerivative0 + valueDifference * 6.0f * hyDifference + hyEnd * yDerivative1;
    derivative.z = ((xValues[1][2] - xValues[0][2]) * hyv + xValues[0][3] * hy0 + xValues[1][3] * hy1 + xValues[0][2]) * (hzEnd - hzv - hzv + 1.0f) +
                   ((xValues[3][0] - xValues[2][0]) * hyv + xValues[2][1] * hy0 + xValues[3][1] * hy1 + xValues[2][0] - xValues[0][0] -
                    (xValues[1][0] - xValues[0][0]) * hyv - xValues[0][1] * hy0 - xValues[1][1] * hy1) *
                       6.0f * hzDifference +
                   ((xValues[3][2] - xValues[2][2]) * hyv + xValues[2][3] * hy0 + xValues[3][3] * hy1 + xValues[2][2]) * hzEnd;

    float value = (yDerivative0 * hy0 + valueDifference * hyv + yDerivative1 * hy1 + value0) * 0.625f + 0.1875f;
    return (3.0f - value - value) * value * value;
  }

  float CRandom::turbulence_(double x, double y, double z, C3Vector &d, DWORD) {
    C3Vector td(0.0f, 0.0f, 0.0f);
    float    n = noise_(x, y, z, d);
    n += noise_(x * 2.0, y * 2.0, z * 2.0, td) * 0.5f;
    d += td;
    n += noise_(x * 4.0, y * 4.0, z * 4.0, td) * 0.25f;
    d += td;
    n += noise_(x * 8.0, y * 8.0, z * 8.0, td) * 0.125f;
    d += td;
    n += noise_(x * 16.0, y * 16.0, z * 16.0, td) * 0.0625f;
    d += td;
    return n * 0.51612902f;
  }

}  // namespace NTempest
