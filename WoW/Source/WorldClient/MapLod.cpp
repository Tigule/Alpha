#include "Base/Base.h"
#include "Gx/Gx.h"
#include "Services/ParticleSystem2.h"
#include <WowConst.h>
#include "AaBsp.h"
#include <MapDefs.h>

#include "WorldClient/World.h"
#include "WorldClient/Map.h"
#include "WorldClient/WorldParam.h"
#include "WorldClient/DetailDoodad.h"
#include "WorldClient/CSimpleDoodad.h"
#include "DayNight.h"

#include "WorldClient/Map.h"

extern UINT g_holeMask[4][4];

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

  lod[0] = (((maxLevel << 8) | maxLevel) << 8) | (neighborLOD & 0xFF0000FF);
  lod[1] = (((maxLevel << 8) | maxLevel) << 16) | (neighborLOD & 0x0000FFFF);
  lod[2] = (maxLevel << 24) | (neighborLOD & 0x00FFFF00) | maxLevel;
  lod[3] = (neighborLOD & 0xFFFF0000) | (maxLevel << 8) | maxLevel;

  if (holes && level == 1) {
    for (UINT i = 0; i < 4; ++i) {
      int x = cX + childOffsX[level][i];
      int y = cY + childOffsY[level][i];
      if (!(holes & g_holeMask[(UINT)(y - 1) >> 1][(UINT)(x - 1) >> 1])) {
        LodCreateTree(level + 1, maxLevel, lod[i], holes, x, y);
      }
    }
    return;
  }

  if (level < maxLevel) {
    for (UINT i = 0; i < 4; ++i) {
      LodCreateTree(level + 1, maxLevel, lod[i], holes, cX + childOffsX[level][i], cY + childOffsY[level][i]);
    }
    return;
  }

  WORD height = vertOffs[level][1];
  WORD width = vertOffs[level][0];
  WORD center = 17 * cY;
  WORD corner;
  if (level == 3) {
    corner = cX + center;
    center = corner + 9;
  } else {
    center += cX;
    corner = center - (width >> 1) - (height >> 1);
  }

  if (((UINT)neighborLOD >> 24) > level) {
    *primPtr++ = center;
    *primPtr++ = corner;
    *primPtr++ = (width >> 1) + corner;
    *primPtr++ = center;
    *primPtr++ = (width >> 1) + corner;
    *primPtr++ = corner + width;
  } else {
    *primPtr++ = center;
    *primPtr++ = corner;
    *primPtr++ = corner + width;
  }

  if (((neighborLOD >> 16) & 0xFF) > level) {
    *primPtr++ = center;
    *primPtr++ = corner + width;
    *primPtr++ = (height >> 1) + corner + width;
    *primPtr++ = center;
    *primPtr++ = (height >> 1) + corner + width;
    *primPtr++ = corner + width + height;
  } else {
    *primPtr++ = center;
    *primPtr++ = corner + width;
    *primPtr++ = corner + width + height;
  }

  if (((neighborLOD >> 8) & 0xFF) > level) {
    *primPtr++ = center;
    *primPtr++ = corner + width + height;
    *primPtr++ = (width >> 1) + corner + height;
    *primPtr++ = center;
    *primPtr++ = (width >> 1) + corner + height;
    *primPtr++ = corner + height;
  } else {
    *primPtr++ = center;
    *primPtr++ = corner + width + height;
    *primPtr++ = corner + height;
  }

  if ((neighborLOD & 0xFF) > level) {
    *primPtr++ = center;
    *primPtr++ = corner + height;
    *primPtr++ = (height >> 1) + corner;
    *primPtr++ = center;
    *primPtr++ = (height >> 1) + corner;
    *primPtr++ = corner;
  } else {
    *primPtr++ = center;
    *primPtr++ = corner + height;
    *primPtr++ = corner;
  }
}
