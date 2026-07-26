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

typedef void(__fastcall *BlitFunc)(const C2iVector &size, const void *in, unsigned int inStride, void *out, unsigned int outStride);

static int          initBlit;
static unsigned int BYTES_PER_BLOCK[BlitFormats_Last] = {0, 4, 2, 2, 2, 8, 16, 16};
static unsigned int PIXELS_PER_BLOCK_SHIFT[BlitFormats_Last] = {0, 0, 0, 0, 0, 2, 2, 2};
static BlitFunc     s_blits[BlitFormats_Last][BlitFormats_Last][BlitAlphas_Last];

static void __fastcall Blit_Argb8888_Argb4444(const C2iVector &size, const void *in, unsigned int inStride, void *out, unsigned int outStride) {
  const CImVector *src = static_cast<const CImVector *>(in);
  CArgb4444       *dst = static_cast<CArgb4444 *>(out);
  int              y;
  for (y = 0; y < size.y; ++y) {
    int x;
    for (x = 0; x < size.x; ++x) {
      dst[x] = src[x];
    }

    src = reinterpret_cast<const CImVector *>(reinterpret_cast<const unsigned char *>(src) + inStride);
    dst = reinterpret_cast<CArgb4444 *>(reinterpret_cast<unsigned char *>(dst) + outStride);
  }
}

static void __fastcall Blit_Argb8888_Argb1555(const C2iVector &size, const void *in, unsigned int inStride, void *out, unsigned int outStride) {
  const CImVector *src = static_cast<const CImVector *>(in);
  CArgb1555       *dst = static_cast<CArgb1555 *>(out);
  int              y;
  for (y = 0; y < size.y; ++y) {
    int x;
    for (x = 0; x < size.x; ++x) {
      dst[x] = src[x];
    }

    src = reinterpret_cast<const CImVector *>(reinterpret_cast<const unsigned char *>(src) + inStride);
    dst = reinterpret_cast<CArgb1555 *>(reinterpret_cast<unsigned char *>(dst) + outStride);
  }
}

static void __fastcall Blit_Argb8888_Rgb565(const C2iVector &size, const void *in, unsigned int inStride, void *out, unsigned int outStride) {
  const CImVector *src = static_cast<const CImVector *>(in);
  CRgb565         *dst = static_cast<CRgb565 *>(out);
  int              y;
  for (y = 0; y < size.y; ++y) {
    int x;
    for (x = 0; x < size.x; ++x) {
      dst[x] = src[x];
    }

    src = reinterpret_cast<const CImVector *>(reinterpret_cast<const unsigned char *>(src) + inStride);
    dst = reinterpret_cast<CRgb565 *>(reinterpret_cast<unsigned char *>(dst) + outStride);
  }
}

static void __fastcall Blit_Argb8888_Argb8888(const C2iVector &size, const void *in, unsigned int inStride, void *out, unsigned int outStride) {
  unsigned int rowSize = sizeof(CImVector) * size.x;
  if (rowSize == inStride && rowSize == outStride) {
    memcpy(out, in, rowSize * size.y);
    return;
  }

  const unsigned char *src = static_cast<const unsigned char *>(in);
  unsigned char       *dst = static_cast<unsigned char *>(out);
  int                  y;
  for (y = 0; y < size.y; ++y) {
    memcpy(dst, src, rowSize);
    src += inStride;
    dst += outStride;
  }
}

static void __fastcall Blit_Argb8888_Argb8888_A1(const C2iVector &size, const void *i, unsigned int iStride, void *o, unsigned int oStride) {
  const CImVector *src = static_cast<const CImVector *>(i);
  CImVector       *dst = static_cast<CImVector *>(o);
  int              y;
  for (y = 0; y < size.y; ++y) {
    int x;
    for (x = 0; x < size.x; ++x) {
      if (src[x].a) {
        dst[x].r = src[x].r;
        dst[x].g = src[x].g;
        dst[x].b = src[x].b;
      }
    }

    src = reinterpret_cast<const CImVector *>(reinterpret_cast<const unsigned char *>(src) + iStride);
    dst = reinterpret_cast<CImVector *>(reinterpret_cast<unsigned char *>(dst) + oStride);
  }
}

static void __fastcall Blit_Argb8888_Argb8888_A8(const C2iVector &size, const void *i, unsigned int iStride, void *o, unsigned int oStride) {
  const CImVector *src = static_cast<const CImVector *>(i);
  CImVector       *dst = static_cast<CImVector *>(o);
  int              y;
  for (y = 0; y < size.y; ++y) {
    int x;
    for (x = 0; x < size.x; ++x) {
      unsigned int alpha = src[x].a;
      if (!alpha) {
        continue;
      }

      if (alpha == 255) {
        dst[x].r = src[x].r;
        dst[x].g = src[x].g;
        dst[x].b = src[x].b;
      } else {
        dst[x].r += alpha * (src[x].r - dst[x].r) >> 8;
        dst[x].g += alpha * (src[x].g - dst[x].g) >> 8;
        dst[x].b += alpha * (src[x].b - dst[x].b) >> 8;
      }
    }

    src = reinterpret_cast<const CImVector *>(reinterpret_cast<const unsigned char *>(src) + iStride);
    dst = reinterpret_cast<CImVector *>(reinterpret_cast<unsigned char *>(dst) + oStride);
  }
}

static void __fastcall Blit_uint16_uint16(const C2iVector &size, const void *in, unsigned int inStride, void *out, unsigned int outStride) {
  unsigned int rowSize = sizeof(unsigned short) * size.x;
  if (rowSize == inStride && rowSize == outStride) {
    memcpy(out, in, rowSize * size.y);
    return;
  }

  const unsigned char *src = static_cast<const unsigned char *>(in);
  unsigned char       *dst = static_cast<unsigned char *>(out);
  int                  y;
  for (y = 0; y < size.y; ++y) {
    memcpy(dst, src, rowSize);
    src += inStride;
    dst += outStride;
  }
}

static void __fastcall Blit_Dxt1_Dxt1(const C2iVector &size, const void *in, unsigned int inStride, void *out, unsigned int outStride) {
  int width = size.x > 4 ? size.x : 4;
  int height = size.y > 4 ? size.y : 4;
  memcpy(out, in, static_cast<unsigned int>(4 * width * height) >> 3);
}

static void __fastcall Blit_Dxt35_Dxt35(const C2iVector &size, const void *in, unsigned int inStride, void *out, unsigned int outStride) {
  int width = size.x > 4 ? size.x : 4;
  int height = size.y > 4 ? size.y : 4;
  memcpy(out, in, static_cast<unsigned int>(8 * width * height) >> 3);
}

template <class Pixel>
inline void __fastcall Blit_DxtUnaligned(const C2iVector &size, const Dxt1Block *in, unsigned int inStride, Pixel *out, unsigned int outStride) {
  static Pixel table[4];
  unsigned int rectWidth = size.x < 4 ? size.x : 4;
  unsigned int rectHeight = size.y < 4 ? size.y : 4;
  int          y;
  for (y = 0; y < size.y; y += 4) {
    Pixel *dest[4];
    int    row;
    for (row = 0; row < 4; ++row) {
      dest[row] = out;
      out = reinterpret_cast<Pixel *>(reinterpret_cast<unsigned char *>(out) + outStride);
    }

    const Dxt1Block *src = in;
    int              x;
    for (x = 0; x < size.x; x += 4) {
      DxtRect rect(0, 0, rectWidth - 1, rectHeight - 1, rectWidth, rectHeight);
      DxtDecompress(src, dest, rect);
      ++src;
    }

    in = reinterpret_cast<const Dxt1Block *>(reinterpret_cast<const unsigned char *>(in) + inStride);
  }
}

template <class Pixel>
inline void __fastcall Blit_Dxt(const C2iVector &size, const Dxt1Block *in, unsigned int inStride, Pixel *out, unsigned int outStride) {
  static Pixel table[4];
  int          y;
  for (y = 0; y < size.y; y += 4) {
    Pixel *dest[4];
    int    row;
    for (row = 0; row < 4; ++row) {
      dest[row] = out;
      out = reinterpret_cast<Pixel *>(reinterpret_cast<unsigned char *>(out) + outStride);
    }

    const Dxt1Block *src = in;
    int              x;
    for (x = 0; x < size.x; x += 4) {
      DxtRect rect(0, 0, 3, 3, 4, 4);
      DxtDecompress(src, dest, rect);
      ++src;
    }

    in = reinterpret_cast<const Dxt1Block *>(reinterpret_cast<const unsigned char *>(in) + inStride);
  }
}

template <class Pixel>
inline void __fastcall Blit_DxtUnaligned(const C2iVector &size, const Dxt3Block *in, unsigned int inStride, Pixel *out, unsigned int outStride) {
  static Pixel table[4];
  unsigned int rectWidth = size.x < 4 ? size.x : 4;
  unsigned int rectHeight = size.y < 4 ? size.y : 4;
  int          y;
  for (y = 0; y < size.y; y += 4) {
    Pixel *dest[4];
    int    row;
    for (row = 0; row < 4; ++row) {
      dest[row] = out;
      out = reinterpret_cast<Pixel *>(reinterpret_cast<unsigned char *>(out) + outStride);
    }

    const Dxt3Block *src = in;
    int              x;
    for (x = 0; x < size.x; x += 4) {
      DxtRect rect(0, 0, rectWidth - 1, rectHeight - 1, rectWidth, rectHeight);
      DxtDecompress(src, dest, rect, sizeof(Pixel) == sizeof(CArgb4444) ? Dxt3A4 : Dxt3A8);
      ++src;
    }

    in = reinterpret_cast<const Dxt3Block *>(reinterpret_cast<const unsigned char *>(in) + inStride);
  }
}

template <class Pixel>
inline void __fastcall Blit_Dxt(const C2iVector &size, const Dxt3Block *in, unsigned int inStride, Pixel *out, unsigned int outStride) {
  static Pixel table[4];
  int          y;
  for (y = 0; y < size.y; y += 4) {
    Pixel *dest[4];
    int    row;
    for (row = 0; row < 4; ++row) {
      dest[row] = out;
      out = reinterpret_cast<Pixel *>(reinterpret_cast<unsigned char *>(out) + outStride);
    }

    const Dxt3Block *src = in;
    int              x;
    for (x = 0; x < size.x; x += 4) {
      DxtRect rect(0, 0, 3, 3, 4, 4);
      DxtDecompress(src, dest, rect, sizeof(Pixel) == sizeof(CArgb4444) ? Dxt3A4 : Dxt3A8);
      ++src;
    }

    in = reinterpret_cast<const Dxt3Block *>(reinterpret_cast<const unsigned char *>(in) + inStride);
  }
}

static void __fastcall Blit_Dxt1_Rgb565(const C2iVector &size, const void *in, unsigned int inStride, void *out, unsigned int outStride) {
  if (size.x < 4 || size.y < 4 || (size.x & 3) || (size.y & 3)) {
    Blit_DxtUnaligned(size, static_cast<const Dxt1Block *>(in), inStride, static_cast<CRgb565 *>(out), outStride);
  } else {
    Blit_Dxt(size, static_cast<const Dxt1Block *>(in), inStride, static_cast<CRgb565 *>(out), outStride);
  }
}

static void __fastcall Blit_Dxt1_Argb1555(const C2iVector &size, const void *in, unsigned int inStride, void *out, unsigned int outStride) {
  if (size.x < 4 || size.y < 4 || (size.x & 3) || (size.y & 3)) {
    Blit_DxtUnaligned(size, static_cast<const Dxt1Block *>(in), inStride, static_cast<CArgb1555 *>(out), outStride);
  } else {
    Blit_Dxt(size, static_cast<const Dxt1Block *>(in), inStride, static_cast<CArgb1555 *>(out), outStride);
  }
}

static void __fastcall Blit_Dxt1_Argb8888(const C2iVector &size, const void *in, unsigned int inStride, void *out, unsigned int outStride) {
  if (size.x < 4 || size.y < 4 || (size.x & 3) || (size.y & 3)) {
    Blit_DxtUnaligned(size, static_cast<const Dxt1Block *>(in), inStride, static_cast<CImVector *>(out), outStride);
  } else {
    Blit_Dxt(size, static_cast<const Dxt1Block *>(in), inStride, static_cast<CImVector *>(out), outStride);
  }
}

static void __fastcall Blit_Dxt3_Argb4444(const C2iVector &size, const void *in, unsigned int inStride, void *out, unsigned int outStride) {
  if (size.x < 4 || size.y < 4 || (size.x & 3) || (size.y & 3)) {
    Blit_DxtUnaligned<CArgb4444>(size, static_cast<const Dxt3Block *>(in), inStride, static_cast<CArgb4444 *>(out), outStride);
  } else {
    Blit_Dxt<CArgb4444>(size, static_cast<const Dxt3Block *>(in), inStride, static_cast<CArgb4444 *>(out), outStride);
  }
}

static void __fastcall Blit_Dxt3_Argb8888(const C2iVector &size, const void *in, unsigned int inStride, void *out, unsigned int outStride) {
  if (size.x < 4 || size.y < 4 || (size.x & 3) || (size.y & 3)) {
    Blit_DxtUnaligned<CImVector>(size, static_cast<const Dxt3Block *>(in), inStride, static_cast<CImVector *>(out), outStride);
  } else {
    Blit_Dxt<CImVector>(size, static_cast<const Dxt3Block *>(in), inStride, static_cast<CImVector *>(out), outStride);
  }
}

static void __fastcall InitBlit() {
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

void __fastcall Blit(
    const C2iVector &size,
    BlitAlpha        alpha,
    const void      *src,
    unsigned int     srcStride,
    BlitFormat       srcFmt,
    void            *dst,
    unsigned int     dstStride,
    BlitFormat       dstFmt
) {
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

unsigned int __fastcall CalcRowStride(BlitFormat format, unsigned int width) {
  ASSERT(format >= BlitFormat_Argb8888 && format < BlitFormats_Last);

  if (format >= BlitFormat_Dxt1 && format <= BlitFormat_Dxt5 && width <= 4) {
    width = 4;
  }

  unsigned int rowSize = BYTES_PER_BLOCK[format] * (width >> PIXELS_PER_BLOCK_SHIFT[format]);
  ASSERT(rowSize);
  return rowSize;
}
