#ifndef ENGINE_SOURCE_IMAGES_DXT_H
#define ENGINE_SOURCE_IMAGES_DXT_H

#include "Tempest/cimvector.h"

struct C4Pixel {
  unsigned char b;
  unsigned char g;
  unsigned char r;
  unsigned char a;
};

struct C4LargePixel {
  int b;
  int g;
  int r;
  int a;
};

struct DxtBlock {
  enum {
    ROWS = 4,
    COLS = 4
  };
};

struct DxtColorBlock {
  enum {
    BPP = 2,
    PIXEL_LSB_MASK = 3
  };

  struct Tables {
    Tables();

    unsigned short dt135[32];
    unsigned short dt235[32];
    unsigned short dt136[64];
    unsigned short dt236[64];
  };

  static Tables tables;

  NTempest::CRgb565 color0;
  NTempest::CRgb565 color1;
  unsigned char     row[DxtBlock::ROWS];
};

struct Dxt1Block {
  DxtColorBlock color;
};

struct Dxt3AlphaBlock {
  enum {
    BPP = 4,
    PIXEL_LSB_MASK = 15
  };

  unsigned short row[DxtBlock::ROWS];
};

struct Dxt3Block {
  Dxt3AlphaBlock alpha;
  DxtColorBlock  color;
};

struct DxtRect {
  DxtRect() {
  }

  DxtRect(unsigned int left, unsigned int top, unsigned int right, unsigned int bottom, unsigned int width, unsigned int height)
      : l(left), t(top), r(right), b(bottom), w(width), h(height) {
  }

  unsigned int l;
  unsigned int t;
  unsigned int r;
  unsigned int b;
  unsigned int w;
  unsigned int h;
};

inline unsigned char Dxt3A4(unsigned int alphaBits) {
  return static_cast<unsigned char>(alphaBits);
}

inline unsigned char Dxt3A8(unsigned int alphaBits) {
  return static_cast<unsigned char>(alphaBits | (alphaBits << 4));
}

template <class Pixel>
inline void DxtMakeTableAlpha(const DxtColorBlock &block, Pixel *table) {
  table[0] = block.color0;
  table[1] = block.color1;

  if (static_cast<unsigned short>(block.color0) > static_cast<unsigned short>(block.color1)) {
    table[2].From565(
        static_cast<unsigned char>((DxtColorBlock::tables.dt235[block.color0.r] + DxtColorBlock::tables.dt135[block.color1.r]) >> 8),
        static_cast<unsigned char>((DxtColorBlock::tables.dt236[block.color0.g] + DxtColorBlock::tables.dt136[block.color1.g]) >> 8),
        static_cast<unsigned char>((DxtColorBlock::tables.dt235[block.color0.b] + DxtColorBlock::tables.dt135[block.color1.b]) >> 8)
    );
    table[3].From565(
        static_cast<unsigned char>((DxtColorBlock::tables.dt135[block.color0.r] + DxtColorBlock::tables.dt235[block.color1.r]) >> 8),
        static_cast<unsigned char>((DxtColorBlock::tables.dt136[block.color0.g] + DxtColorBlock::tables.dt236[block.color1.g]) >> 8),
        static_cast<unsigned char>((DxtColorBlock::tables.dt135[block.color0.b] + DxtColorBlock::tables.dt235[block.color1.b]) >> 8)
    );
  } else {
    table[2].From565(
        static_cast<unsigned char>((block.color0.r + block.color1.r) / 2), static_cast<unsigned char>((block.color0.g + block.color1.g) / 2),
        static_cast<unsigned char>((block.color0.b + block.color1.b) / 2)
    );
    table[3] = Pixel(0ul);
  }
}

template <class Pixel>
inline void DxtDecompress(const Dxt1Block *block, Pixel **dest, const DxtRect &rect) {
  static Pixel colorTable[4];
  DxtMakeTableAlpha(block->color, colorTable);

  unsigned int t;
  for (t = rect.t; t <= rect.b; ++t) {
    unsigned int colorBitRow = block->color.row[t] >> (rect.l * DxtColorBlock::BPP);
    unsigned int l;
    for (l = rect.l; l <= rect.r; ++l) {
      dest[t][l] = colorTable[colorBitRow & DxtColorBlock::PIXEL_LSB_MASK];
      colorBitRow >>= DxtColorBlock::BPP;
    }

    dest[t] += rect.w;
  }
}

template <class Pixel>
inline void DxtDecompress(const Dxt3Block *block, Pixel **dest, const DxtRect &rect, unsigned char(*afunc)(unsigned int)) {
  static Pixel colorTable[4];
  colorTable[0] = block->color.color0;
  colorTable[1] = block->color.color1;
  colorTable[2].From565(
      static_cast<unsigned char>((DxtColorBlock::tables.dt235[block->color.color0.r] + DxtColorBlock::tables.dt135[block->color.color1.r]) >> 8),
      static_cast<unsigned char>((DxtColorBlock::tables.dt236[block->color.color0.g] + DxtColorBlock::tables.dt136[block->color.color1.g]) >> 8),
      static_cast<unsigned char>((DxtColorBlock::tables.dt235[block->color.color0.b] + DxtColorBlock::tables.dt135[block->color.color1.b]) >> 8)
  );
  colorTable[3].From565(
      static_cast<unsigned char>((DxtColorBlock::tables.dt135[block->color.color0.r] + DxtColorBlock::tables.dt235[block->color.color1.r]) >> 8),
      static_cast<unsigned char>((DxtColorBlock::tables.dt136[block->color.color0.g] + DxtColorBlock::tables.dt236[block->color.color1.g]) >> 8),
      static_cast<unsigned char>((DxtColorBlock::tables.dt135[block->color.color0.b] + DxtColorBlock::tables.dt235[block->color.color1.b]) >> 8)
  );

  unsigned int t;
  for (t = rect.t; t <= rect.b; ++t) {
    unsigned int colorBitRow = block->color.row[t] >> (rect.l * DxtColorBlock::BPP);
    unsigned int alphaBitRow = block->alpha.row[t] >> (rect.l * Dxt3AlphaBlock::BPP);
    unsigned int l;
    for (l = rect.l; l <= rect.r; ++l) {
      dest[t][l] = colorTable[colorBitRow & DxtColorBlock::PIXEL_LSB_MASK];
      dest[t][l].a = afunc(alphaBitRow & Dxt3AlphaBlock::PIXEL_LSB_MASK);
      colorBitRow >>= DxtColorBlock::BPP;
      alphaBitRow >>= Dxt3AlphaBlock::BPP;
    }

    dest[t] += rect.w;
  }
}

struct MipBits {
  C4Pixel *mip[1];
};

unsigned int GetBitDepth(unsigned int fourCC);
unsigned int CalcLevelSize(unsigned int level, unsigned int width, unsigned int height, unsigned int fourCC);
unsigned int CalcLevelOffset(unsigned int level, unsigned int width, unsigned int height, unsigned int fourCC);
unsigned int CalcLevelCount(unsigned int width, unsigned int height);
unsigned int MippedImgCalcSize(unsigned int fourCC, unsigned int width, unsigned int height);
MipBits *MippedImgAllocA(unsigned int fourCC, unsigned int width, unsigned int height, const char *fileName, int lineNumber);
void MippedImgSet(unsigned int fourCC, unsigned int width, unsigned int height, MipBits *bits);
void FullShrink(
    C4Pixel             *dest,
    unsigned int         destWidth,
    unsigned int         destHeight,
    const C4Pixel *const source,
    unsigned int         sourceWidth,
    unsigned int         sourceHeight
);

#endif
