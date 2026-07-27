#include "dxt.h"

#include <storm.h>

unsigned int GetBitDepth(unsigned int fourCC) {
  switch (fourCC) {
    case 0:
      return 4;

    case 1:
    case 6:
    case 7:
      return 8;

    case 2:
      return 32;

    case 3:
    case 4:
    case 5:
      return 16;

    default:
      ASSERT(!"GetBitDepth(): unhandled format");
      return 0;
  }
}

unsigned int CalcLevelSize(unsigned int level, unsigned int width, unsigned int height, unsigned int fourCC) {
  unsigned int levelWidth = max(width >> level, 1U);
  unsigned int levelHeight = max(height >> level, 1U);

  if (fourCC == 0 || fourCC == 1 || fourCC == 7) {
    levelWidth = max(levelWidth, 4U);
    levelHeight = max(levelHeight, 4U);
  }

  return levelWidth * levelHeight * GetBitDepth(fourCC) >> 3;
}

unsigned int CalcLevelOffset(unsigned int level, unsigned int width, unsigned int height, unsigned int fourCC) {
  unsigned int offset = 0;
  unsigned int index;

  for (index = 0; index < level; ++index) {
    offset += CalcLevelSize(index, width, height, fourCC);
  }

  return offset;
}

unsigned int CalcLevelCount(unsigned int width, unsigned int height) {
  unsigned int levelCount = 1;

  while (width > 1 || height > 1) {
    width >>= 1;
    ++levelCount;

    if (width < 1) {
      width = 1;
    }

    height >>= 1;

    if (height < 1) {
      height = 1;
    }
  }

  return levelCount;
}

MipBits *MippedImgAllocA(unsigned int fourCC, unsigned int width, unsigned int height, const char *fileName, int lineNumber) {
  unsigned int levelCount = CalcLevelCount(width, height);
  unsigned int levelDataSize = CalcLevelOffset(levelCount, width, height, fourCC);
  MipBits     *ptr = static_cast<MipBits *>(SMemAlloc(levelDataSize + 4 * levelCount, fileName, lineNumber, 0));
  unsigned int offset = 0;
  unsigned int level;
  for (level = 0; level < levelCount; ++level) {
    ptr[level].mip[0] = reinterpret_cast<C4Pixel *>(reinterpret_cast<unsigned char *>(&ptr->mip[levelCount]) + offset);
    offset += CalcLevelSize(level, width, height, fourCC);
  }
  ASSERT(offset == levelDataSize);
  return ptr;
}

unsigned int MippedImgCalcSize(unsigned int fourCC, unsigned int width, unsigned int height) {
  unsigned int levelCount = CalcLevelCount(width, height);
  return CalcLevelOffset(levelCount, width, height, fourCC) + 4 * levelCount;
}

void MippedImgSet(unsigned int fourCC, unsigned int width, unsigned int height, MipBits *bits) {
  unsigned int levelCount = CalcLevelCount(width, height);
  unsigned int levelDataSize = CalcLevelOffset(levelCount, width, height, fourCC);
  unsigned int offset = 0;
  unsigned int level;
  ASSERT(bits);
  for (level = 0; level < levelCount; ++level) {
    bits[level].mip[0] = reinterpret_cast<C4Pixel *>(reinterpret_cast<unsigned char *>(&bits->mip[levelCount]) + offset);
    offset += CalcLevelSize(level, width, height, fourCC);
  }
  ASSERT(offset == levelDataSize);
}

void FullShrink(
    C4Pixel             *dest,
    unsigned int         destWidth,
    unsigned int         destHeight,
    const C4Pixel *const source,
    unsigned int         sourceWidth,
    unsigned int         sourceHeight
) {
  unsigned int   xScale = sourceWidth / destWidth;
  unsigned int   yScale = sourceHeight / destHeight;
  const C4Pixel *sourcePixel = source;

  ASSERT(destWidth * xScale == sourceWidth);
  ASSERT(destHeight * yScale == sourceHeight);

  for (unsigned int y = 0; y < destHeight; ++y) {
    for (unsigned int x = 0; x < destWidth; ++x) {
      C4LargePixel   weighted = {0, 0, 0, 0};
      C4LargePixel   unweighted = {0, 0, 0, 0};
      const C4Pixel *currSource = sourcePixel;

      for (unsigned int sourceY = 0; sourceY < yScale; ++sourceY) {
        const C4Pixel *pixel = currSource;
        for (unsigned int sourceX = 0; sourceX < xScale; ++sourceX, ++pixel) {
          weighted.b += pixel->b * pixel->a;
          weighted.g += pixel->g * pixel->a;
          weighted.r += pixel->r * pixel->a;
          weighted.a += pixel->a;

          unweighted.b += pixel->b;
          unweighted.g += pixel->g;
          unweighted.r += pixel->r;
          ++unweighted.a;
        }

        currSource += sourceWidth;
      }

      C4Pixel result;
      if (weighted.a) {
        unsigned int scale = yScale * xScale;
        ASSERT(yScale * xScale);
        result.r = static_cast<unsigned char>(weighted.r / weighted.a);
        result.g = static_cast<unsigned char>(weighted.g / weighted.a);
        result.b = static_cast<unsigned char>(weighted.b / weighted.a);
        result.a = static_cast<unsigned char>(weighted.a / scale);
      } else {
        result.a = 0;
        result.r = static_cast<unsigned char>(unweighted.r / unweighted.a);
        result.g = static_cast<unsigned char>(unweighted.g / unweighted.a);
        result.b = static_cast<unsigned char>(unweighted.b / unweighted.a);
      }

      *dest++ = result;
      sourcePixel += xScale;
    }

    sourcePixel += sourceWidth * (yScale - 1);
  }
}
