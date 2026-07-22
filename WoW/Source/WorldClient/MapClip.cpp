#include "WorldClient/World.h"

static const unsigned long vCnt[4] = {2, 3, 5, 9};
static const unsigned long vStp[4] = {8, 4, 2, 1};

void CMapChunk::UpdateClipBuffer() {
  int           indexList[9];
  unsigned long cnt = vCnt[lod];

  unsigned long step = vStp[lod];
  unsigned long start = CWorldScene::camPos.x > CWorldScene::camTarg.x ? 136 : 0;
  unsigned long index = 0;
  unsigned long vertex = 0;
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
