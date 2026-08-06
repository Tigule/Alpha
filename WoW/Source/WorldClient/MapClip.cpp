#include <Base/Base.h>
#include <WowConst.h>
#include <MapDefs.h>

#include "WorldClient/World.h"

static const DWORD vCnt[4] = {2, 3, 5, 9};
static const DWORD vStp[4] = {8, 4, 2, 1};

void CMapChunk::UpdateClipBuffer() {
  int   indexList[9];
  DWORD cnt = vCnt[lod];

  DWORD step = vStp[lod];
  DWORD start = CWorldScene::camPos.x > CWorldScene::camTarg.x ? 136 : 0;
  DWORD index = 0;
  DWORD vertex = 0;
  while (vertex < 9) {
    indexList[index++] = start + vertex;
    vertex += step;
  }
  CWorldScene::ClipBufferUpdate(vertexList, indexList, cnt, corner);

  start = CWorldScene::camPos.y > CWorldScene::camTarg.y ? 8 : 0;
  index = 0;
  vertex = 0;
  while (vertex < 9) {
    indexList[index++] = start;
    start += 17 * step;
    vertex += step;
  }
  CWorldScene::ClipBufferUpdate(vertexList, indexList, cnt, corner);
}
