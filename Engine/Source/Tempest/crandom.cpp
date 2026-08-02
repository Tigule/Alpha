#include "crandom.h"

#include "c2vector.h"
#include "c3vector.h"
#include "cmath.h"

static unsigned long lattice_(long x);

namespace NTempest {

  extern const unsigned long gnoise32_[64] = {
      0x9927148EUL, 0x08C7AAFDUL, 0x1F3EE6D5UL, 0xDA55BBF6UL, 0x6A4AA075UL, 0xFF97BDE8UL, 0x9FBC9BDEUL, 0x46A18A81UL, 0x63E30B6EUL, 0x5D6C7A76UL,
      0xCA69D388UL, 0x25B947C3UL, 0x3FA2AB83UL, 0xBA7C41A6UL, 0x0195ACE5UL, 0xC109CF7EUL, 0x717062D9UL, 0x0205DB8DUL, 0x54EF8724UL, 0x3037D4C6UL,
      0x7BCB1BD0UL, 0xECD8E4B8UL, 0xDCADCE49UL, 0xC494A913UL, 0x0DAE398FUL, 0x0EDD5218UL, 0x85F5FA78UL, 0x6DAFD258UL, 0x3B53B2A4UL, 0xBE50A551UL,
      0x11F42DFCUL, 0xF1169848UL, 0x663DDF86UL, 0x2F2E445EUL, 0x176B0736UL, 0xB64C298BUL, 0xE75F89E2UL, 0xE121A7CDUL, 0xED65C94DUL, 0x239CEEFEUL,
      0x04B77D33UL, 0x402A9A9EUL, 0xF35B10B3UL, 0x921C7782UL, 0x571E4E20UL, 0x8C067222UL, 0xFB732C67UL, 0xBF0AC259UL, 0x0CF95C79UL, 0x68121A28UL,
      0x42193474UL, 0xF884C0B1UL, 0x9D15F038UL, 0x6F3AF260UL, 0x91EB90B4UL, 0x61357F1DUL, 0x5603325AUL, 0x932BC5A3UL, 0x434B0F80UL, 0x3CE0A8F7UL,
      0x2664D196UL, 0x4FCC45D7UL, 0xB5E9B0C8UL, 0xEA31D600UL,
  };

  void CRndSeed::SetSeed(unsigned long seed) {
    rndacc = seed;
    rndvls = 4 * ((seed % 61) | (((seed % 59) | (((seed % 53) | ((seed % 47) << 8)) << 8)) << 8));
  }

  void CRndSeed::SetSeed(char *password) {
    SetSeed(CRandom::Seed(password));
  }

  unsigned long CRandom::Seed(char *password) {
    ASSERT(password);
    unsigned long length = SStrLen(password);
    unsigned long seed = static_cast<unsigned char>(password[0]);
    for (unsigned long i = 1; i < length; ++i) {
      seed = (seed ^ 31277 * seed) + static_cast<unsigned char>(password[i]);
    }
    return seed;
  }

  C3Vector CRandom::C3Vector_(CRndSeed &seed) {
    const float z = reals_(seed);
    const float angle = real_(seed) * 6.28318530717958647692f;

    ASSERT(z >= -1.0f && z <= 1.0f);

    const float radius = CMath::sqrt_(1.0f - z * z);
    const float x = CMath::cos_(angle) * radius;
    const float y = CMath::sin_(angle) * radius;

    return C3Vector(x, y, z);
  }

  C2Vector CRandom::C2Vector_(CRndSeed &seed) {
    float angle = real_(seed) * 6.2831855f;
    return C2Vector(CMath::cos_(angle), CMath::sin_(angle));
  }

  void CRandom::array_(unsigned long *buf, unsigned long count, CRndSeed &seed) {
    ASSERT(buf);
    for (unsigned long i = 0; i < count; ++i) buf[i] = uint32_(seed);
  }

  void CRandom::array_(long *buf, unsigned long count, CRndSeed &seed) {
    ASSERT(buf);
    array_(reinterpret_cast<unsigned long *>(buf), count, seed);
  }

  void CRandom::array_(float *buf, unsigned long count, CRndSeed &seed) {
    ASSERT(buf);
    for (unsigned long i = 0; i < count; ++i) buf[i] = real_(seed);
  }

  void CRandom::array_(double *buf, unsigned long count, CRndSeed &seed) {
    ASSERT(buf);
    for (unsigned long i = 0; i < count; ++i) buf[i] = lreal_(seed);
  }

  void CRandom::arrayp_(float *buf, unsigned long count, CRndSeed &seed) {
    ASSERT(buf);
    for (unsigned long i = 0; i < count; ++i) buf[i] = realp_(seed);
  }

  void CRandom::arrayp_(double *buf, unsigned long count, CRndSeed &seed) {
    ASSERT(buf);
    for (unsigned long i = 0; i < count; ++i) buf[i] = lrealp_(seed);
  }

  void CRandom::arrays_(float *buf, unsigned long count, CRndSeed &seed) {
    ASSERT(buf);
    for (unsigned long i = 0; i < count; ++i) buf[i] = reals_(seed);
  }

  void CRandom::arrays_(double *buf, unsigned long count, CRndSeed &seed) {
    ASSERT(buf);
    for (unsigned long i = 0; i < count; ++i) buf[i] = lreals_(seed);
  }

  float CRandom::reale_(CRndSeed &seed) {
    return static_cast<float>(CMath::log2_(realp_(seed)) * -0.69314718);
  }

  double CRandom::lreale_(CRndSeed &seed) {
    return CMath::log2_(lrealp_(seed)) * -0.6931471805599453;
  }

  float CRandom::reale_(float mean, CRndSeed &seed) {
    return reale_(seed) * mean;
  }

  double CRandom::lreale_(double mean, CRndSeed &seed) {
    return lreale_(seed) * mean;
  }

  float CRandom::realg_(CRndSeed &seed) {
    static float cache = 0.0f;
    if (cache != 0.0f) {
      float result = cache;
      cache = 0.0f;
      return result;
    }

    float radius = CMath::sqrt_(static_cast<float>(CMath::log2_(realp_(seed)) * -1.3862944));
    float angle = real_(seed) * 6.2831855f;
    float sine;
    float cosine;
    CMath::sincos_(angle, sine, cosine);
    cache = sine * radius;
    return cosine * radius;
  }

  double CRandom::lrealg_(CRndSeed &seed) {
    static double cache = 0.0;
    if (cache != 0.0) {
      double result = cache;
      cache = 0.0;
      return result;
    }

    double radius = CMath::sqrt_(CMath::log2_(lrealp_(seed)) * -1.386294361119891);
    double angle = lreal_(seed) * 6.28318530718;
    double sine;
    double cosine;
    CMath::sincos_(angle, sine, cosine);
    cache = sine * radius;
    return cosine * radius;
  }

  float CRandom::realg_(float mean, float variation, CRndSeed &seed) {
    return realg_(seed) * variation + mean;
  }

  double CRandom::lrealg_(double mean, double variation, CRndSeed &seed) {
    return lrealg_(seed) * variation + mean;
  }

  void CRandom::arraye_(float *buf, unsigned long count, CRndSeed &seed) {
    ASSERT(buf);
    for (unsigned long i = 0; i < count; ++i) buf[i] = reale_(seed);
  }

  void CRandom::arraye_(double *buf, unsigned long count, CRndSeed &seed) {
    ASSERT(buf);
    for (unsigned long i = 0; i < count; ++i) buf[i] = lreale_(seed);
  }

  void CRandom::arraye_(float *buf, unsigned long count, float mean, CRndSeed &seed) {
    ASSERT(buf);
    for (unsigned long i = 0; i < count; ++i) buf[i] = reale_(mean, seed);
  }

  void CRandom::arraye_(double *buf, unsigned long count, double mean, CRndSeed &seed) {
    ASSERT(buf);
    for (unsigned long i = 0; i < count; ++i) buf[i] = lreale_(mean, seed);
  }

  void CRandom::arrayg_(float *buf, unsigned long count, CRndSeed &seed) {
    ASSERT(buf);
    for (unsigned long i = 0; i < count; ++i) buf[i] = realg_(seed);
  }

  void CRandom::arrayg_(double *buf, unsigned long count, CRndSeed &seed) {
    ASSERT(buf);
    for (unsigned long i = 0; i < count; ++i) buf[i] = lrealg_(seed);
  }

  void CRandom::arrayg_(float *buf, unsigned long count, float mean, float variation, CRndSeed &seed) {
    ASSERT(buf);
    for (unsigned long i = 0; i < count; ++i) buf[i] = realg_(mean, variation, seed);
  }

  void CRandom::arrayg_(double *buf, unsigned long count, double mean, double variation, CRndSeed &seed) {
    ASSERT(buf);
    for (unsigned long i = 0; i < count; ++i) buf[i] = lrealg_(mean, variation, seed);
  }

  void CRandom::array_(C2Vector *buf, unsigned long count, CRndSeed &seed) {
    ASSERT(buf);
    for (unsigned long i = 0; i < count; ++i) buf[i] = C2Vector_(seed);
  }

  void CRandom::array_(C3Vector *buf, unsigned long count, CRndSeed &seed) {
    ASSERT(buf);
    for (unsigned long i = 0; i < count; ++i) buf[i] = C3Vector_(seed);
  }

  void CRandom::shuffle_(char *buf, CRndSeed &seed) {
    ASSERT(buf);
    shuffle_(reinterpret_cast<unsigned char *>(buf), SStrLen(buf), seed);
  }

  void CRandom::shuffle_(char *buf, unsigned long count, CRndSeed &seed) {
    ASSERT(buf);
    shuffle_(reinterpret_cast<unsigned char *>(buf), count, seed);
  }

  void CRandom::shuffle_(unsigned char *buf, unsigned long count, CRndSeed &seed) {
    ASSERT(buf);
    for (unsigned long i = 1; i < count; ++i) {
      unsigned char bi = buf[i];
      unsigned long index = dice_(i + 1, seed);
      buf[i] = buf[index];
      buf[index] = bi;
    }
  }

  void CRandom::shuffle_(short *buf, unsigned long count, CRndSeed &seed) {
    ASSERT(buf);
    shuffle_(reinterpret_cast<unsigned short *>(buf), count, seed);
  }

  void CRandom::shuffle_(unsigned short *buf, unsigned long count, CRndSeed &seed) {
    ASSERT(buf);
    for (unsigned long i = 1; i < count; ++i) {
      unsigned short bi = buf[i];
      unsigned long index = dice_(i + 1, seed);
      buf[i] = buf[index];
      buf[index] = bi;
    }
  }

  void CRandom::shuffle_(long *buf, unsigned long count, CRndSeed &seed) {
    ASSERT(buf);
    shuffle_(reinterpret_cast<unsigned long *>(buf), count, seed);
  }

  void CRandom::shuffle_(unsigned long *buf, unsigned long count, CRndSeed &seed) {
    ASSERT(buf);
    for (unsigned long i = 1; i < count; ++i) {
      unsigned long bi = buf[i];
      unsigned long index = dice_(i + 1, seed);
      buf[i] = buf[index];
      buf[index] = bi;
    }
  }

  void CRandom::shuffle_(float *buf, unsigned long count, CRndSeed &seed) {
    ASSERT(buf);
    for (unsigned long i = 1; i < count; ++i) {
      float bi = buf[i];
      unsigned long index = dice_(i + 1, seed);
      buf[i] = buf[index];
      buf[index] = bi;
    }
  }

  void CRandom::shuffle_(double *buf, unsigned long count, CRndSeed &seed) {
    ASSERT(buf);
    for (unsigned long i = 1; i < count; ++i) {
      double bi = buf[i];
      unsigned long index = dice_(i + 1, seed);
      buf[i] = buf[index];
      buf[index] = bi;
    }
  }

  void CRandom::crypt_(char *buf, unsigned long size, unsigned long seedNumber) {
    ASSERT((reinterpret_cast<unsigned long>(buf) & 3) == 0);
    CRndSeed seed(seedNumber);
    unsigned long offset = -reinterpret_cast<unsigned long>(buf) & 3;
    buf += offset;
    for (unsigned long ind = 0; ind < (size - offset) >> 2; ++ind) {
      buf[ind] ^= static_cast<char>(uint32_(seed));
    }
  }

  void CRandom::crypt_(char *buf, unsigned long size, char *password) {
    ASSERT(password);
    crypt_(buf, size, Seed(password));
  }

  unsigned long CRandom::lattice_(long x) {
    return ::lattice_(x);
  }

  unsigned long CRandom::lattice_(long x, long y) {
    unsigned long value = lattice_(y);
    return lattice_(x ^ static_cast<long>((value << 4) | (value >> 28)));
  }

  unsigned long CRandom::lattice_(long x, long y, long z) {
    unsigned long value = lattice_(y, z);
    return lattice_(x ^ static_cast<long>((value << 4) | (value >> 28)));
  }

  unsigned long CRandom::lattice_(long x, long y, long z, long w) {
    unsigned long value = lattice_(y, z, w);
    return lattice_(x ^ static_cast<long>((value << 4) | (value >> 28)));
  }

  void CRandom::lattice2_(long x, unsigned long *vtx) {
    ASSERT(vtx);
    vtx[0] = lattice_(x);
    vtx[1] = lattice_(x + 1);
  }

  void CRandom::lattice4_(long x, long y, unsigned long *vertices) {
    ASSERT(vertices);
    vertices[0] = lattice_(x, y);
    vertices[1] = lattice_(x + 1, y);
    vertices[2] = lattice_(x, y + 1);
    vertices[3] = lattice_(x + 1, y + 1);
  }

  void CRandom::lattice8_(long x, long y, long z, unsigned long *vertices) {
    ASSERT(vertices);
    vertices[0] = lattice_(x, y, z);
    vertices[1] = lattice_(x + 1, y, z);
    vertices[2] = lattice_(x, y + 1, z);
    vertices[3] = lattice_(x + 1, y + 1, z);
    vertices[4] = lattice_(x, y, z + 1);
    vertices[5] = lattice_(x + 1, y, z + 1);
    vertices[6] = lattice_(x, y + 1, z + 1);
    vertices[7] = lattice_(x + 1, y + 1, z + 1);
  }

  void CRandom::lattice3_(long x, unsigned long *vtx) {
    ASSERT(vtx);
    vtx[0] = lattice_(x - 1);
    vtx[1] = lattice_(x);
    vtx[2] = lattice_(x + 1);
  }

  void CRandom::lattice9_(long x, long y, unsigned long *vertices) {
    ASSERT(vertices);
    unsigned long index = 0;
    for (long j = -1; j <= 1; ++j)
      for (long i = -1; i <= 1; ++i)
        vertices[index++] = lattice_(x + i, y + j);
  }

  void CRandom::lattice27_(long x, long y, long z, unsigned long *vertices) {
    ASSERT(vertices);
    unsigned long index = 0;
    for (long k = -1; k <= 1; ++k)
      for (long j = -1; j <= 1; ++j)
        for (long i = -1; i <= 1; ++i)
          vertices[index++] = lattice_(x + i, y + j, z + k);
  }

  float CRandom::noise_(double x) {
    long integer = static_cast<long>(x);
    if (x < 0.0) --integer;
    double fraction = x - integer;

    unsigned long first = lattice_(integer);
    unsigned long second = lattice_(integer + 1);
    unsigned long bits = 0x40000000 | ((first & 0xFFFF) << 7);
    float firstSlope = *reinterpret_cast<float *>(&bits) - 3.0f;
    bits = 0x40000000 | ((second & 0xFFFF) << 7);
    float secondSlope = *reinterpret_cast<float *>(&bits) - 3.0f;
    bits = 0x40400000 + ((first >> 10) & 0x3FFFC0);
    float firstValue = *reinterpret_cast<float *>(&bits) - 3.0f;
    bits = 0x40400000 + ((second >> 10) & 0x3FFFC0) - ((first >> 10) & 0x3FFFC0);
    float difference = *reinterpret_cast<float *>(&bits) - 3.0f;

    double fraction2 = fraction * fraction;
    double value = (((fraction - 2.0) * fraction2 + fraction) * firstSlope +
                    (3.0 - fraction - fraction) * fraction2 * difference +
                    (fraction2 * fraction - fraction2) * secondSlope +
                    firstValue) * 0.75 + 0.125;
    return static_cast<float>((3.0 - value - value) * value * value);
  }

  float CRandom::noise_(double x, double y) {
    long xi = static_cast<long>(x);
    long yi = static_cast<long>(y);
    if (x < 0.0) --xi;
    if (y < 0.0) --yi;
    float xf = static_cast<float>(x - xi);
    float yf = static_cast<float>(y - yi);
    unsigned long vertices[4];
    lattice4_(xi, yi, vertices);

    float values[4];
    float derivativesX[4];
    float derivativesY[4];
    float derivativesXY[4];
    for (unsigned long i = 0; i < 4; ++i) {
      unsigned long bits = (vertices[i] >> 10 & 0x3FF800) | 0x40400000;
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
    float rowDerivative0 = (derivativesY[1] - derivativesY[0]) * hxv +
                           derivativesXY[0] * hx0 + derivativesXY[1] * hx1 + derivativesY[0];
    float rowDerivative1 = (derivativesY[3] - derivativesY[2]) * hxv +
                           derivativesXY[2] * hx0 + derivativesXY[3] * hx1 + derivativesY[2];
    float value = (row1 - row0) * hyv + rowDerivative0 * hy0 + rowDerivative1 * hy1 + row0 -
                  ((hx0 + hx1 + 1.0f) * 3.0f * (hy0 + hy1 + 1.0f) - 0.30000001f);
    return (1.17188f - value * 0.48828f) * value * value;
  }

  float CRandom::noise_(double x, double y, double z) {
    long xi = static_cast<long>(x);
    long yi = static_cast<long>(y);
    long zi = static_cast<long>(z);
    if (x < 0.0) --xi;
    if (y < 0.0) --yi;
    if (z < 0.0) --zi;
    float xf = static_cast<float>(x - xi);
    float yf = static_cast<float>(y - yi);
    float zf = static_cast<float>(z - zi);
    unsigned long vertices[8];
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

    float interpolated[4][4];
    for (unsigned long pair = 0; pair < 4; ++pair) {
      unsigned long first = vertices[pair * 2];
      unsigned long second = vertices[pair * 2 + 1];
      long value0 = first >> 28;
      long value1 = second >> 28;
      interpolated[pair][0] = static_cast<float>(value1 - value0) / 15.0f * hxv +
                              static_cast<float>(2 * static_cast<long>((first >> 24) & 0xF) - 15) / 15.0f * hx0 +
                              static_cast<float>(2 * static_cast<long>((second >> 24) & 0xF) - 15) / 15.0f * hx1 +
                              static_cast<float>(value0) / 15.0f;
      value0 = (first >> 20) & 0xF;
      value1 = (second >> 20) & 0xF;
      interpolated[pair][1] = static_cast<float>(2 * (value1 - value0)) / 15.0f * hxv +
                              static_cast<float>(2 * static_cast<long>((first >> 16) & 0xF) - 15) / 15.0f * hx0 +
                              static_cast<float>(2 * static_cast<long>((second >> 16) & 0xF) - 15) / 15.0f * hx1 +
                              static_cast<float>(2 * value0 - 15) / 15.0f;
      value0 = (first >> 12) & 0xF;
      value1 = (second >> 12) & 0xF;
      interpolated[pair][2] = static_cast<float>(2 * (value1 - value0)) / 15.0f * hxv +
                              static_cast<float>(2 * static_cast<long>((first >> 8) & 0xF) - 15) / 15.0f * hx0 +
                              static_cast<float>(2 * static_cast<long>((second >> 8) & 0xF) - 15) / 15.0f * hx1 +
                              static_cast<float>(2 * value0 - 15) / 15.0f;
      value0 = (first >> 4) & 0xF;
      value1 = (second >> 4) & 0xF;
      interpolated[pair][3] = static_cast<float>(2 * (value1 - value0)) / 15.0f * hxv +
                              static_cast<float>(2 * static_cast<long>(first & 0xF) - 15) / 15.0f * hx0 +
                              static_cast<float>(2 * static_cast<long>(second & 0xF) - 15) / 15.0f * hx1 +
                              static_cast<float>(2 * value0 - 15) / 15.0f;
    }

    float firstRow = (interpolated[2][0] - interpolated[0][0]) * hzv +
                     interpolated[2][2] * hz1 + interpolated[0][2] * hz0 + interpolated[0][0];
    float value = (((interpolated[3][0] - interpolated[1][0]) * hzv +
                    interpolated[3][2] * hz1 + interpolated[1][2] * hz0 + interpolated[1][0] - firstRow) * hyv +
                   ((interpolated[2][1] - interpolated[0][1]) * hzv +
                    interpolated[2][3] * hz1 + interpolated[0][3] * hz0 + interpolated[0][1]) * hy0 +
                   ((interpolated[3][1] - interpolated[1][1]) * hzv +
                    interpolated[3][3] * hz1 + interpolated[1][3] * hz0 + interpolated[1][1]) * hy1 +
                   firstRow) * 0.625f + 0.1875f;
    return (3.0f - value - value) * value * value;
  }

  float CRandom::noise_(double x, double y, double z, C3Vector &derivative) {
    long xi = static_cast<long>(x);
    long yi = static_cast<long>(y);
    long zi = static_cast<long>(z);
    if (x < 0.0) --xi;
    if (y < 0.0) --yi;
    if (z < 0.0) --zi;
    float xf = static_cast<float>(x - xi);
    float yf = static_cast<float>(y - yi);
    float zf = static_cast<float>(z - zi);
    unsigned long vertices[8];
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
    for (unsigned long pair = 0; pair < 4; ++pair) {
      unsigned long first = vertices[pair * 2];
      unsigned long second = vertices[pair * 2 + 1];
      long a = first >> 28, b = second >> 28;
      xValues[pair][0] = static_cast<float>(b - a) / 15.0f * hxv +
                         static_cast<float>(2 * static_cast<long>((first >> 24) & 0xF) - 15) / 15.0f * hx0 +
                         static_cast<float>(2 * static_cast<long>((second >> 24) & 0xF) - 15) / 15.0f * hx1 +
                         static_cast<float>(a) / 15.0f;
      a = (first >> 20) & 0xF; b = (second >> 20) & 0xF;
      xValues[pair][1] = static_cast<float>(2 * (b - a)) / 15.0f * hxv +
                         static_cast<float>(2 * static_cast<long>((first >> 16) & 0xF) - 15) / 15.0f * hx0 +
                         static_cast<float>(2 * static_cast<long>((second >> 16) & 0xF) - 15) / 15.0f * hx1 +
                         static_cast<float>(2 * a - 15) / 15.0f;
      a = (first >> 12) & 0xF; b = (second >> 12) & 0xF;
      xValues[pair][2] = static_cast<float>(2 * (b - a)) / 15.0f * hxv +
                         static_cast<float>(2 * static_cast<long>((first >> 8) & 0xF) - 15) / 15.0f * hx0 +
                         static_cast<float>(2 * static_cast<long>((second >> 8) & 0xF) - 15) / 15.0f * hx1 +
                         static_cast<float>(2 * a - 15) / 15.0f;
      a = (first >> 4) & 0xF; b = (second >> 4) & 0xF;
      xValues[pair][3] = static_cast<float>(2 * (b - a)) / 15.0f * hxv +
                         static_cast<float>(2 * static_cast<long>(first & 0xF) - 15) / 15.0f * hx0 +
                         static_cast<float>(2 * static_cast<long>(second & 0xF) - 15) / 15.0f * hx1 +
                         static_cast<float>(2 * a - 15) / 15.0f;
    }

    float zValues[4][4];
    for (unsigned long zPair = 0; zPair < 4; ++zPair) {
      unsigned long first = vertices[zPair];
      unsigned long second = vertices[zPair + 4];
      long a = first >> 28, b = second >> 28;
      zValues[zPair][0] = static_cast<float>(b - a) / 15.0f * hzv +
                         static_cast<float>(2 * static_cast<long>((first >> 12) & 0xF) - 15) / 15.0f * hz0 +
                         static_cast<float>(2 * static_cast<long>((second >> 12) & 0xF) - 15) / 15.0f * hz1 +
                         static_cast<float>(a) / 15.0f;
      a = (first >> 24) & 0xF; b = (second >> 24) & 0xF;
      zValues[zPair][1] = static_cast<float>(2 * (b - a)) / 15.0f * hzv +
                         static_cast<float>(2 * static_cast<long>((first >> 8) & 0xF) - 15) / 15.0f * hz0 +
                         static_cast<float>(2 * static_cast<long>((second >> 8) & 0xF) - 15) / 15.0f * hz1 +
                         static_cast<float>(2 * a - 15) / 15.0f;
      a = (first >> 20) & 0xF; b = (second >> 20) & 0xF;
      zValues[zPair][2] = static_cast<float>(2 * (b - a)) / 15.0f * hzv +
                         static_cast<float>(2 * static_cast<long>((first >> 4) & 0xF) - 15) / 15.0f * hz0 +
                         static_cast<float>(2 * static_cast<long>((second >> 4) & 0xF) - 15) / 15.0f * hz1 +
                         static_cast<float>(2 * a - 15) / 15.0f;
      a = (first >> 16) & 0xF; b = (second >> 16) & 0xF;
      zValues[zPair][3] = static_cast<float>(2 * (b - a)) / 15.0f * hzv +
                         static_cast<float>(2 * static_cast<long>(first & 0xF) - 15) / 15.0f * hz0 +
                         static_cast<float>(2 * static_cast<long>(second & 0xF) - 15) / 15.0f * hz1 +
                         static_cast<float>(2 * a - 15) / 15.0f;
    }

    float yDerivative1 = (xValues[3][1] - xValues[1][1]) * hzv +
                         xValues[1][3] * hz0 + xValues[3][3] * hz1 + xValues[1][1];
    float yDerivative0 = (xValues[2][1] - xValues[0][1]) * hzv +
                         xValues[0][3] * hz0 + xValues[2][3] * hz1 + xValues[0][1];
    float value0 = (xValues[2][0] - xValues[0][0]) * hzv +
                   xValues[0][2] * hz0 + xValues[2][2] * hz1 + xValues[0][0];
    float valueDifference = (xValues[3][0] - xValues[1][0]) * hzv +
                            xValues[1][2] * hz0 + xValues[3][2] * hz1 + xValues[1][0] - value0;

    float hxDifference = hxv - hxv * hxv;
    float hyDifference = hyv - hyv * hyv;
    float hzDifference = hzv - hzv * hzv;
    float hxEnd = hxv - 3.0f * hxDifference;
    float hyEnd = hyv - 3.0f * hyDifference;
    float hzEnd = hzv - 3.0f * hzDifference;
    derivative.x =
        ((zValues[2][2] - zValues[0][2]) * hyv + zValues[0][3] * hy0 + zValues[2][3] * hy1 + zValues[0][2]) *
            (hxEnd - hxv - hxv + 1.0f) +
        ((zValues[3][0] - zValues[1][0]) * hyv + zValues[1][1] * hy0 + zValues[3][1] * hy1 + zValues[1][0] -
         zValues[0][0] - (zValues[2][0] - zValues[0][0]) * hyv - zValues[0][1] * hy0 - zValues[2][1] * hy1) *
            6.0f * hxDifference +
        ((zValues[3][2] - zValues[1][2]) * hyv + zValues[1][3] * hy0 + zValues[3][3] * hy1 + zValues[1][2]) * hxEnd;
    derivative.y = (hyEnd - hyv - hyv + 1.0f) * yDerivative0 +
                   valueDifference * 6.0f * hyDifference + hyEnd * yDerivative1;
    derivative.z =
        ((xValues[1][2] - xValues[0][2]) * hyv + xValues[0][3] * hy0 + xValues[1][3] * hy1 + xValues[0][2]) *
            (hzEnd - hzv - hzv + 1.0f) +
        ((xValues[3][0] - xValues[2][0]) * hyv + xValues[2][1] * hy0 + xValues[3][1] * hy1 + xValues[2][0] -
         xValues[0][0] - (xValues[1][0] - xValues[0][0]) * hyv - xValues[0][1] * hy0 - xValues[1][1] * hy1) *
            6.0f * hzDifference +
        ((xValues[3][2] - xValues[2][2]) * hyv + xValues[2][3] * hy0 + xValues[3][3] * hy1 + xValues[2][2]) * hzEnd;

    float value = (yDerivative0 * hy0 + valueDifference * hyv + yDerivative1 * hy1 + value0) * 0.625f + 0.1875f;
    return (3.0f - value - value) * value * value;
  }

  float CRandom::turbulence_(double x, double y, double z, C3Vector &d, unsigned long) {
    C3Vector td(0.0f, 0.0f, 0.0f);
    float n = noise_(x, y, z, d);
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

static unsigned long lattice_(long x) {
  unsigned long value = static_cast<unsigned long>(x);
  value = ((value << 11) | (value >> 21)) ^ ((value << 21) | (value >> 11)) ^ value;

  unsigned long n1 = NTempest::gnoise32_[(value >> 6) & 0x3F];
  unsigned long n2 = NTempest::gnoise32_[(value >> 12) & 0x3F];
  unsigned long n3 = NTempest::gnoise32_[(value >> 18) & 0x3F];
  return NTempest::gnoise32_[value & 0x3F] ^
         ((n1 << 1) | (n1 >> 31)) ^
         ((n2 << 2) | (n2 >> 30)) ^
         ((n3 << 3) | (n3 >> 29));
}
