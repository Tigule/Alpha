#include <Base/Base.h>

#include "blit.h"
#include "dxt.h"

#include "Tempest/c2ivector.h"

#include <storm.h>

#include <string.h>

using NTempest::C2iVector;
using NTempest::CArgb1555;
using NTempest::CArgb4444;
using NTempest::CImVector;
using NTempest::CRgb565;

typedef void (*BlitFunc)(const C2iVector &size, LPCVOID in, UINT inStride, LPVOID out, UINT outStride);

static int      initBlit;
static UINT     BYTES_PER_BLOCK[BlitFormats_Last] = {0, 4, 2, 2, 2, 8, 16, 16};
static UINT     PIXELS_PER_BLOCK_SHIFT[BlitFormats_Last] = {0, 0, 0, 0, 0, 2, 2, 2};
static BlitFunc s_blits[BlitFormats_Last][BlitFormats_Last][BlitAlphas_Last];

static void Blit_Argb8888_Argb4444(const C2iVector &size, LPCVOID in, UINT inStride, LPVOID out, UINT outStride) {
  const CImVector *src = static_cast<const CImVector *>(in);
  CArgb4444       *dst = static_cast<CArgb4444 *>(out);
  int              y;
  for (y = 0; y < size.y; ++y) {
    int x;
    for (x = 0; x < size.x; ++x) {
      dst[x] = src[x];
    }

    src = reinterpret_cast<const CImVector *>(reinterpret_cast<const BYTE *>(src) + inStride);
    dst = reinterpret_cast<CArgb4444 *>(reinterpret_cast<BYTE *>(dst) + outStride);
  }
}

static void Blit_Argb8888_Argb1555(const C2iVector &size, LPCVOID in, UINT inStride, LPVOID out, UINT outStride) {
  const CImVector *src = static_cast<const CImVector *>(in);
  CArgb1555       *dst = static_cast<CArgb1555 *>(out);
  int              y;
  for (y = 0; y < size.y; ++y) {
    int x;
    for (x = 0; x < size.x; ++x) {
      dst[x] = src[x];
    }

    src = reinterpret_cast<const CImVector *>(reinterpret_cast<const BYTE *>(src) + inStride);
    dst = reinterpret_cast<CArgb1555 *>(reinterpret_cast<BYTE *>(dst) + outStride);
  }
}

static void Blit_Argb8888_Rgb565(const C2iVector &size, LPCVOID in, UINT inStride, LPVOID out, UINT outStride) {
  const CImVector *src = static_cast<const CImVector *>(in);
  CRgb565         *dst = static_cast<CRgb565 *>(out);
  int              y;
  for (y = 0; y < size.y; ++y) {
    int x;
    for (x = 0; x < size.x; ++x) {
      dst[x] = src[x];
    }

    src = reinterpret_cast<const CImVector *>(reinterpret_cast<const BYTE *>(src) + inStride);
    dst = reinterpret_cast<CRgb565 *>(reinterpret_cast<BYTE *>(dst) + outStride);
  }
}

static void Blit_Argb8888_Argb8888(const C2iVector &size, LPCVOID in, UINT inStride, LPVOID out, UINT outStride) {
  if (sizeof(CImVector) * size.x == inStride && sizeof(CImVector) * size.x == outStride) {
    memcpy(out, in, sizeof(CImVector) * size.x * size.y);
    return;
  }

  const BYTE *src = static_cast<const BYTE *>(in);
  BYTE       *dst = static_cast<BYTE *>(out);
  for (int y = size.y; y; --y) {
    memcpy(dst, src, sizeof(CImVector) * size.x);
    src += inStride;
    dst += outStride;
  }
}

static void Blit_Argb8888_Argb8888_A1(const C2iVector &size, LPCVOID i, UINT iStride, LPVOID o, UINT oStride) {
  const CImVector *src = static_cast<const CImVector *>(i);
  CImVector       *dst = static_cast<CImVector *>(o);
  for (int y = size.y; y; --y) {
    int x;
    for (x = 0; x < size.x; ++x) {
      if (src[x].a) {
        dst[x].r = src[x].r;
        dst[x].g = src[x].g;
        dst[x].b = src[x].b;
      }
    }

    src = reinterpret_cast<const CImVector *>(reinterpret_cast<const BYTE *>(src) + iStride);
    dst = reinterpret_cast<CImVector *>(reinterpret_cast<BYTE *>(dst) + oStride);
  }
}

static void Blit_Argb8888_Argb8888_A8(const C2iVector &size, LPCVOID i, UINT iStride, LPVOID o, UINT oStride) {
  const CImVector *src = static_cast<const CImVector *>(i);
  CImVector       *dst = static_cast<CImVector *>(o);
  for (int y = size.y; y; --y) {
    int x;
    for (x = 0; x < size.x; ++x) {
      if (!(*src[x].IV_() & CImVector::eAlphaMask)) {
        continue;
      }

      if ((*src[x].IV_() >> CImVector::eAlphaS) == 255) {
        dst[x].SetRGB(src + x);
      } else {
        dst[x].Set(
            dst[x].a, static_cast<BYTE>(dst[x].r + (((*src[x].IV_() >> CImVector::eAlphaS) * (src[x].r - dst[x].r)) >> 8)),
            static_cast<BYTE>(dst[x].g + (((*src[x].IV_() >> CImVector::eAlphaS) * (src[x].g - dst[x].g)) >> 8)),
            static_cast<BYTE>(dst[x].b + (((*src[x].IV_() >> CImVector::eAlphaS) * (src[x].b - dst[x].b)) >> 8))
        );
      }
    }

    src = reinterpret_cast<const CImVector *>(reinterpret_cast<const BYTE *>(src) + iStride);
    dst = reinterpret_cast<CImVector *>(reinterpret_cast<BYTE *>(dst) + oStride);
  }
}

static void Blit_uint16_uint16(const C2iVector &size, LPCVOID in, UINT inStride, LPVOID out, UINT outStride) {
  if (sizeof(WORD) * size.x == inStride && sizeof(WORD) * size.x == outStride) {
    memcpy(out, in, sizeof(WORD) * size.x * size.y);
    return;
  }

  const BYTE *src = static_cast<const BYTE *>(in);
  BYTE       *dst = static_cast<BYTE *>(out);
  for (int y = size.y; y; --y) {
    memcpy(dst, src, sizeof(WORD) * size.x);
    src += inStride;
    dst += outStride;
  }
}

static void Blit_Dxt1_Dxt1(const C2iVector &size, LPCVOID in, UINT inStride, LPVOID out, UINT outStride) {
  int width = size.x > 4 ? size.x : 4;
  int height = size.y > 4 ? size.y : 4;
  memcpy(out, in, static_cast<UINT>(4 * width * height) >> 3);
}

static void Blit_Dxt35_Dxt35(const C2iVector &size, LPCVOID in, UINT inStride, LPVOID out, UINT outStride) {
  int width = size.x > 4 ? size.x : 4;
  int height = size.y > 4 ? size.y : 4;
  memcpy(out, in, static_cast<UINT>(8 * width * height) >> 3);
}

template <class Pixel>
inline void Blit_DxtUnaligned(const C2iVector &size, const Dxt1Block *in, UINT inStride, Pixel *out, UINT outStride) {
  static Pixel table[4];
  UINT         rectWidth = size.x < 4 ? size.x : 4;
  UINT         rectHeight = size.y < 4 ? size.y : 4;
  int          y;
  for (y = 0; y < size.y; y += 4) {
    Pixel *dest[4];
    int    row;
    for (row = 0; row < 4; ++row) {
      dest[row] = out;
      out = reinterpret_cast<Pixel *>(reinterpret_cast<BYTE *>(out) + outStride);
    }

    const Dxt1Block *src = in;
    int              x;
    for (x = 0; x < size.x; x += 4) {
      DxtRect rect(0, 0, rectWidth - 1, rectHeight - 1, rectWidth, rectHeight);
      DxtDecompress(src, dest, rect);
      ++src;
    }

    in = reinterpret_cast<const Dxt1Block *>(reinterpret_cast<const BYTE *>(in) + inStride);
  }
}

template <class Pixel>
inline void Blit_Dxt(const C2iVector &size, const Dxt1Block *in, UINT inStride, Pixel *out, UINT outStride) {
  static Pixel table[4];
  int          y;
  for (y = 0; y < size.y; y += 4) {
    Pixel *dest[4];
    int    row;
    for (row = 0; row < 4; ++row) {
      dest[row] = out;
      out = reinterpret_cast<Pixel *>(reinterpret_cast<BYTE *>(out) + outStride);
    }

    const Dxt1Block *src = in;
    int              x;
    for (x = 0; x < size.x; x += 4) {
      DxtRect rect(0, 0, 3, 3, 4, 4);
      DxtDecompress(src, dest, rect);
      ++src;
    }

    in = reinterpret_cast<const Dxt1Block *>(reinterpret_cast<const BYTE *>(in) + inStride);
  }
}

template <class Pixel>
inline void Blit_DxtUnaligned(const C2iVector &size, const Dxt3Block *in, UINT inStride, Pixel *out, UINT outStride) {
  static Pixel table[4];
  UINT         rectWidth = size.x < 4 ? size.x : 4;
  UINT         rectHeight = size.y < 4 ? size.y : 4;
  int          y;
  for (y = 0; y < size.y; y += 4) {
    Pixel *dest[4];
    int    row;
    for (row = 0; row < 4; ++row) {
      dest[row] = out;
      out = reinterpret_cast<Pixel *>(reinterpret_cast<BYTE *>(out) + outStride);
    }

    const Dxt3Block *src = in;
    int              x;
    for (x = 0; x < size.x; x += 4) {
      DxtRect rect(0, 0, rectWidth - 1, rectHeight - 1, rectWidth, rectHeight);
      DxtDecompress(src, dest, rect, sizeof(Pixel) == sizeof(CArgb4444) ? Dxt3A4 : Dxt3A8);
      ++src;
    }

    in = reinterpret_cast<const Dxt3Block *>(reinterpret_cast<const BYTE *>(in) + inStride);
  }
}

template <class Pixel>
inline void Blit_Dxt(const C2iVector &size, const Dxt3Block *in, UINT inStride, Pixel *out, UINT outStride) {
  static Pixel table[4];
  int          y;
  for (y = 0; y < size.y; y += 4) {
    Pixel *dest[4];
    int    row;
    for (row = 0; row < 4; ++row) {
      dest[row] = out;
      out = reinterpret_cast<Pixel *>(reinterpret_cast<BYTE *>(out) + outStride);
    }

    const Dxt3Block *src = in;
    int              x;
    for (x = 0; x < size.x; x += 4) {
      DxtRect rect(0, 0, 3, 3, 4, 4);
      DxtDecompress(src, dest, rect, sizeof(Pixel) == sizeof(CArgb4444) ? Dxt3A4 : Dxt3A8);
      ++src;
    }

    in = reinterpret_cast<const Dxt3Block *>(reinterpret_cast<const BYTE *>(in) + inStride);
  }
}

static void Blit_Dxt1_Rgb565(const C2iVector &size, LPCVOID in, UINT inStride, LPVOID out, UINT outStride) {
  if (size.x < 4 || size.y < 4 || (size.x & 3) || (size.y & 3)) {
    Blit_DxtUnaligned(size, static_cast<const Dxt1Block *>(in), inStride, static_cast<CRgb565 *>(out), outStride);
  } else {
    Blit_Dxt(size, static_cast<const Dxt1Block *>(in), inStride, static_cast<CRgb565 *>(out), outStride);
  }
}

static void Blit_Dxt1_Argb1555(const C2iVector &size, LPCVOID in, UINT inStride, LPVOID out, UINT outStride) {
  if (size.x < 4 || size.y < 4 || (size.x & 3) || (size.y & 3)) {
    Blit_DxtUnaligned(size, static_cast<const Dxt1Block *>(in), inStride, static_cast<CArgb1555 *>(out), outStride);
  } else {
    Blit_Dxt(size, static_cast<const Dxt1Block *>(in), inStride, static_cast<CArgb1555 *>(out), outStride);
  }
}

static void Blit_Dxt1_Argb8888(const C2iVector &size, LPCVOID in, UINT inStride, LPVOID out, UINT outStride) {
  if (size.x < 4 || size.y < 4 || (size.x & 3) || (size.y & 3)) {
    Blit_DxtUnaligned(size, static_cast<const Dxt1Block *>(in), inStride, static_cast<CImVector *>(out), outStride);
  } else {
    Blit_Dxt(size, static_cast<const Dxt1Block *>(in), inStride, static_cast<CImVector *>(out), outStride);
  }
}

static void Blit_Dxt3_Argb4444(const C2iVector &size, LPCVOID in, UINT inStride, LPVOID out, UINT outStride) {
  if (size.x < 4 || size.y < 4 || (size.x & 3) || (size.y & 3)) {
    Blit_DxtUnaligned<CArgb4444>(size, static_cast<const Dxt3Block *>(in), inStride, static_cast<CArgb4444 *>(out), outStride);
  } else {
    Blit_Dxt<CArgb4444>(size, static_cast<const Dxt3Block *>(in), inStride, static_cast<CArgb4444 *>(out), outStride);
  }
}

static void Blit_Dxt3_Argb8888(const C2iVector &size, LPCVOID in, UINT inStride, LPVOID out, UINT outStride) {
  if (size.x < 4 || size.y < 4 || (size.x & 3) || (size.y & 3)) {
    Blit_DxtUnaligned<CImVector>(size, static_cast<const Dxt3Block *>(in), inStride, static_cast<CImVector *>(out), outStride);
  } else {
    Blit_Dxt<CImVector>(size, static_cast<const Dxt3Block *>(in), inStride, static_cast<CImVector *>(out), outStride);
  }
}

static void InitBlit() {
  s_blits[BlitFormat_Rgb565][BlitFormat_Rgb565][BlitAlpha_0] = Blit_uint16_uint16;
  s_blits[BlitFormat_Argb4444][BlitFormat_Argb4444][BlitAlpha_0] = Blit_uint16_uint16;
  s_blits[BlitFormat_Argb1555][BlitFormat_Argb1555][BlitAlpha_0] = Blit_uint16_uint16;
  s_blits[BlitFormat_Argb8888][BlitFormat_Argb8888][BlitAlpha_0] = Blit_Argb8888_Argb8888;
  s_blits[BlitFormat_Argb8888][BlitFormat_Argb8888][BlitAlpha_1] = Blit_Argb8888_Argb8888_A1;
  s_blits[BlitFormat_Argb8888][BlitFormat_Argb8888][BlitAlpha_8] = Blit_Argb8888_Argb8888_A8;
  s_blits[BlitFormat_Argb8888][BlitFormat_Argb4444][BlitAlpha_0] = Blit_Argb8888_Argb4444;
  s_blits[BlitFormat_Argb8888][BlitFormat_Argb1555][BlitAlpha_0] = Blit_Argb8888_Argb1555;
  s_blits[BlitFormat_Argb8888][BlitFormat_Rgb565][BlitAlpha_0] = Blit_Argb8888_Rgb565;
  s_blits[BlitFormat_Dxt1][BlitFormat_Dxt1][BlitAlpha_0] = Blit_Dxt1_Dxt1;
  s_blits[BlitFormat_Dxt3][BlitFormat_Dxt3][BlitAlpha_0] = Blit_Dxt35_Dxt35;
  s_blits[BlitFormat_Dxt5][BlitFormat_Dxt5][BlitAlpha_0] = Blit_Dxt35_Dxt35;
  s_blits[BlitFormat_Dxt1][BlitFormat_Rgb565][BlitAlpha_0] = Blit_Dxt1_Rgb565;
  s_blits[BlitFormat_Dxt1][BlitFormat_Argb1555][BlitAlpha_0] = Blit_Dxt1_Argb1555;
  s_blits[BlitFormat_Dxt1][BlitFormat_Argb8888][BlitAlpha_0] = Blit_Dxt1_Argb8888;
  s_blits[BlitFormat_Dxt3][BlitFormat_Argb4444][BlitAlpha_0] = Blit_Dxt3_Argb4444;
  s_blits[BlitFormat_Dxt3][BlitFormat_Argb8888][BlitAlpha_0] = Blit_Dxt3_Argb8888;
}

void Blit(const C2iVector &size, BlitAlpha alpha, LPCVOID src, UINT srcStride, BlitFormat srcFmt, LPVOID dst, UINT dstStride, BlitFormat dstFmt) {
  ASSERT(alpha < BlitAlphas_Last);
  ASSERT(srcFmt < BlitFormats_Last);
  ASSERT(dstFmt < BlitFormats_Last);
  ASSERT(src);
  ASSERT(dst);

  if (!initBlit) {
    InitBlit();
    initBlit = 1;
  }

  BlitFunc blit = s_blits[srcFmt][dstFmt][alpha];
  ASSERT(blit);
  blit(size, src, srcStride, dst, dstStride);
}

UINT CalcRowStride(BlitFormat format, UINT width) {
  ASSERT(format >= BlitFormat_Argb8888 && format < BlitFormats_Last);

  if (format >= BlitFormat_Dxt1 && format <= BlitFormat_Dxt5 && width <= 4) {
    width = 4;
  }

  UINT rowSize = BYTES_PER_BLOCK[format] * (width >> PIXELS_PER_BLOCK_SHIFT[format]);
  ASSERT(rowSize);
  return rowSize;
}
