#ifndef ENGINE_SOURCE_IMAGES_DXT_H
#define ENGINE_SOURCE_IMAGES_DXT_H

#include "Tempest/cimvector.h"

#pragma pack(push, 1)

struct C4Pixel {
  C4Pixel() {
  }

  C4Pixel(BYTE b, BYTE g, BYTE r, BYTE a) : b(b), g(g), r(r), a(a) {
  }

  C4Pixel(UINT color) {
    a = color >> 24;
    r = color >> 16;
    g = color >> 8;
    b = color;
  }

  UINT BitDepth() const {
    return 8;
  }

  operator UINT() {
    return *reinterpret_cast<UINT *>(this);
  }

  BYTE b;
  BYTE g;
  BYTE r;
  BYTE a;
};

struct C4LargePixel {
  C4LargePixel() {
  }

  C4LargePixel(long b, long g, long r, long a) : b(b), g(g), r(r), a(a) {
  }

  C4LargePixel(UINT value) : b(value), g(value), r(value), a(value) {
  }

  C4LargePixel &operator+=(const C4LargePixel &value) {
    b += value.b;
    g += value.g;
    r += value.r;
    a += value.a;
    return *this;
  }

  UINT BitDepth() const {
    return 32;
  }

  long b;
  long g;
  long r;
  long a;
};

#pragma pack(pop)

struct DxtBlock {
  enum {
    ROWS = 4,
    COLS = 4
  };
};

struct DxtColorBlock : public DxtBlock {
  enum {
    BPP = 2,
    PIXEL_LSB_MASK = 3
  };

  struct Tables {
    WORD dt135[32];
    WORD dt235[32];
    WORD dt136[64];
    WORD dt236[64];

    Tables();
  };

  static Tables tables;

  NTempest::CRgb565 color0;
  NTempest::CRgb565 color1;
  BYTE              row[DxtBlock::ROWS];
};

struct Dxt1Block : public DxtBlock {
  DxtColorBlock color;
};

struct Dxt3AlphaBlock : public DxtBlock {
  enum {
    BPP = 4,
    PIXEL_LSB_MASK = 15
  };

  WORD row[DxtBlock::ROWS];
};

struct Dxt3Block : public DxtBlock {
  Dxt3AlphaBlock alpha;
  DxtColorBlock  color;
};

struct DxtRect {
  UINT l;
  UINT t;
  UINT r;
  UINT b;
  UINT w;
  UINT h;

  void Check();

  DxtRect(UINT left, UINT top, UINT right, UINT bottom);

  DxtRect(UINT left, UINT top, UINT right, UINT bottom, UINT width, UINT height) : l(left), t(top), r(right), b(bottom), w(width), h(height) {
  }

  DxtRect() {
  }

};

inline BYTE Dxt3A4(UINT alphaBits) {
  return static_cast<BYTE>(alphaBits);
}

inline BYTE Dxt3A8(UINT alphaBits) {
  return static_cast<BYTE>(alphaBits << 4);
}

template <class Pixel>
inline void DxtMakeTableAlpha(const DxtColorBlock &block, Pixel *table) {
  if (static_cast<WORD>(block.color0) > static_cast<WORD>(block.color1)) {
    table[0] = block.color0;
    table[1] = block.color1;
    table[2].From565(
        static_cast<BYTE>((DxtColorBlock::tables.dt235[block.color0.r] + DxtColorBlock::tables.dt135[block.color1.r]) >> 8),
        static_cast<BYTE>((DxtColorBlock::tables.dt236[block.color0.g] + DxtColorBlock::tables.dt136[block.color1.g]) >> 8),
        static_cast<BYTE>((DxtColorBlock::tables.dt235[block.color0.b] + DxtColorBlock::tables.dt135[block.color1.b]) >> 8)
    );
    table[3].From565(
        static_cast<BYTE>((DxtColorBlock::tables.dt135[block.color0.r] + DxtColorBlock::tables.dt235[block.color1.r]) >> 8),
        static_cast<BYTE>((DxtColorBlock::tables.dt136[block.color0.g] + DxtColorBlock::tables.dt236[block.color1.g]) >> 8),
        static_cast<BYTE>((DxtColorBlock::tables.dt135[block.color0.b] + DxtColorBlock::tables.dt235[block.color1.b]) >> 8)
    );
  } else {
    table[0] = block.color0;
    table[1] = block.color1;
    table[2].From565(
        static_cast<BYTE>((block.color0.r + block.color1.r) / 2), static_cast<BYTE>((block.color0.g + block.color1.g) / 2),
        static_cast<BYTE>((block.color0.b + block.color1.b) / 2)
    );
    table[3] = Pixel(0ul);
  }
}

template <class Pixel>
inline void DxtDecompress(const Dxt1Block *block, Pixel **dest, const DxtRect &rect) {
  static Pixel colorTable[4];
  DxtMakeTableAlpha(block->color, colorTable);

  UINT colorShift = rect.l * DxtColorBlock::BPP;
  for (UINT r = rect.t; r <= rect.b; ++r) {
    UINT colorBitRow = block->color.row[r] >> static_cast<BYTE>(colorShift);
    for (UINT c = rect.l; c <= rect.r; ++c, colorBitRow >>= DxtColorBlock::BPP) {
      dest[r][c] = colorTable[colorBitRow & DxtColorBlock::PIXEL_LSB_MASK];
    }

    dest[r] += rect.w;
  }
}

template <class Pixel>
inline void DxtDecompress(const Dxt3Block *block, Pixel **dest, const DxtRect &rect, BYTE (*afunc)(UINT)) {
  static Pixel colorTable[4];
  colorTable[0] = block->color.color0;
  colorTable[1] = block->color.color1;
  colorTable[2].From565(
      static_cast<BYTE>((DxtColorBlock::tables.dt235[block->color.color0.r] + DxtColorBlock::tables.dt135[block->color.color1.r]) >> 8),
      static_cast<BYTE>((DxtColorBlock::tables.dt236[block->color.color0.g] + DxtColorBlock::tables.dt136[block->color.color1.g]) >> 8),
      static_cast<BYTE>((DxtColorBlock::tables.dt235[block->color.color0.b] + DxtColorBlock::tables.dt135[block->color.color1.b]) >> 8)
  );
  colorTable[3].From565(
      static_cast<BYTE>((DxtColorBlock::tables.dt135[block->color.color0.r] + DxtColorBlock::tables.dt235[block->color.color1.r]) >> 8),
      static_cast<BYTE>((DxtColorBlock::tables.dt136[block->color.color0.g] + DxtColorBlock::tables.dt236[block->color.color1.g]) >> 8),
      static_cast<BYTE>((DxtColorBlock::tables.dt135[block->color.color0.b] + DxtColorBlock::tables.dt235[block->color.color1.b]) >> 8)
  );

  UINT colorShift = rect.l * DxtColorBlock::BPP;
  UINT alphaShift = rect.l * Dxt3AlphaBlock::BPP;
  for (UINT r = rect.t; r <= rect.b; ++r) {
    UINT colorBitRow = block->color.row[r] >> static_cast<BYTE>(colorShift);
    UINT alphaBitRow = block->alpha.row[r] >> static_cast<WORD>(alphaShift);
    for (UINT c = rect.l; c <= rect.r; ++c, alphaBitRow >>= Dxt3AlphaBlock::BPP, colorBitRow >>= DxtColorBlock::BPP) {
      dest[r][c].FromARGB(afunc(alphaBitRow & Dxt3AlphaBlock::PIXEL_LSB_MASK), colorTable[colorBitRow & DxtColorBlock::PIXEL_LSB_MASK]);
    }

    dest[r] += rect.w;
  }
}

struct MipBits {
  C4Pixel *mip[1];
};

UINT     GetBitDepth(UINT fourCC);
UINT     CalcLevelSize(UINT level, UINT width, UINT height, UINT fourCC);
UINT     CalcLevelOffset(UINT level, UINT width, UINT height, UINT fourCC);
UINT     CalcLevelCount(UINT width, UINT height);
UINT     MippedImgCalcSize(UINT fourCC, UINT width, UINT height);
MipBits *MippedImgAllocA(UINT fourCC, UINT width, UINT height, LPCSTR fileName, int lineNumber);
void     MippedImgSet(UINT fourCC, UINT width, UINT height, MipBits *bits);
void     FullShrink(C4Pixel *dest, UINT destWidth, UINT destHeight, const C4Pixel source[], UINT sourceWidth, UINT sourceHeight);

#endif
