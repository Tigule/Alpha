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

static const DWORD vCnt[4] = {2, 3, 5, 9};
static const DWORD vStp[4] = {8, 4, 2, 1};

void CMapChunk::UpdateClipBuffer() {
  int   indexList[9];
  DWORD cnt = vCnt[lod];
  int   step = vStp[lod];

  int start = 0;
  if (CWorldScene::camPos.x > CWorldScene::camTarg.x) {
    start = 136;
  }
  int  vertex = 0;
  int *index = indexList;
  while (vertex < 9) {
    *index++ = vertex + start;
    vertex += step;
  }
  CWorldScene::ClipBufferUpdate(vertexList, indexList, cnt, corner);

  start = 0;
  if (CWorldScene::camPos.y > CWorldScene::camTarg.y) {
    start = 8;
  }
  vertex = 0;
  index = indexList;
  while (vertex < 9) {
    *index++ = start;
    start += 17 * step;
    vertex += step;
  }
  CWorldScene::ClipBufferUpdate(vertexList, indexList, cnt, corner);
}
