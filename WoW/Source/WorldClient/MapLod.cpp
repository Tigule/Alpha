#include "WorldClient/World.h"

extern unsigned int g_holeMask[4][4];

static const int childOffsX[3][4] = {
    {-2, 2, 2, -2},
    {-1, 1, 1, -1},
    {-1, 0, 0, -1}
};

static const int childOffsY[3][4] = {
    {-2, -2, 2, 2},
    {-1, -1, 1, 1},
    {-1, -1, 0, 0}
};

static const int vertOffs[4][2] = {
    {136, 8},
    { 68, 4},
    { 34, 2},
    { 17, 1}
};

void CMapChunk::LodCreateTree(int level, int maxLevel, int neighborLOD, int holes, int cX, int cY) {
  int lod[4];

  FATALASSERT(primPtr);

  lod[0] = (neighborLOD & 0xFF0000FF) | ((maxLevel | (maxLevel << 8)) << 8);
  lod[1] = (neighborLOD & 0x0000FFFF) | ((maxLevel | (maxLevel << 8)) << 16);
  lod[2] = maxLevel | (neighborLOD & 0x00FFFF00) | (maxLevel << 24);
  lod[3] = maxLevel | (maxLevel << 8) | (neighborLOD & 0xFFFF0000);

  if (holes && level == 1) {
    for (unsigned int i = 0; i < 4; ++i) {
      int x = cX + childOffsX[1][i];
      int y = cY + childOffsY[1][i];
      if (!(holes & g_holeMask[(y - 1) >> 1][(x - 1) >> 1])) {
        LodCreateTree(2, maxLevel, lod[i], holes, x, y);
      }
    }
    return;
  }

  if (level < maxLevel) {
    for (unsigned int i = 0; i < 4; ++i) {
      LodCreateTree(level + 1, maxLevel, lod[i], holes, cX + childOffsX[level][i], cY + childOffsY[level][i]);
    }
    return;
  }

  unsigned short width = static_cast<unsigned short>(vertOffs[level][0]);
  unsigned short height = static_cast<unsigned short>(vertOffs[level][1]);
  unsigned short center;
  unsigned short corner;
  if (level == 3) {
    corner = static_cast<unsigned short>(cX + 17 * cY);
    center = static_cast<unsigned short>(corner + 9);
  } else {
    center = static_cast<unsigned short>(cX + 17 * cY);
    corner = static_cast<unsigned short>(center - (width >> 1) - (height >> 1));
  }

  *primPtr++ = center;
  *primPtr++ = corner;
  if (static_cast<unsigned char>(neighborLOD >> 24) > level) {
    *primPtr++ = static_cast<unsigned short>(corner + (width >> 1));
    *primPtr++ = center;
    *primPtr++ = static_cast<unsigned short>(corner + (width >> 1));
  }

  *primPtr++ = static_cast<unsigned short>(corner + width);
  *primPtr++ = center;
  if (static_cast<unsigned char>(neighborLOD >> 16) > level) {
    *primPtr++ = static_cast<unsigned short>(corner + width);
    *primPtr++ = static_cast<unsigned short>(corner + width + (height >> 1));
    *primPtr++ = center;
    *primPtr++ = static_cast<unsigned short>(corner + width + (height >> 1));
  } else {
    *primPtr++ = static_cast<unsigned short>(corner + width);
  }

  *primPtr++ = static_cast<unsigned short>(corner + width + height);
  *primPtr++ = center;
  if (static_cast<unsigned char>(neighborLOD >> 8) > level) {
    *primPtr++ = static_cast<unsigned short>(corner + width + height);
    *primPtr++ = static_cast<unsigned short>(corner + height + (width >> 1));
    *primPtr++ = center;
    *primPtr++ = static_cast<unsigned short>(corner + height + (width >> 1));
  } else {
    *primPtr++ = static_cast<unsigned short>(corner + width + height);
  }

  *primPtr++ = static_cast<unsigned short>(corner + height);
  *primPtr++ = center;
  *primPtr++ = static_cast<unsigned short>(corner + height);
  if (static_cast<unsigned char>(neighborLOD) > level) {
    *primPtr++ = static_cast<unsigned short>(corner + (height >> 1));
    *primPtr++ = center;
    *primPtr++ = static_cast<unsigned short>(corner + (height >> 1));
  }
  *primPtr++ = corner;
}
