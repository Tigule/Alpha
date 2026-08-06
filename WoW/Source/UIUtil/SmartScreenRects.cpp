#include <Base/Base.h>
#include <WowConst.h>
#include <MapDefs.h>

#include "Tempest/crect.h"
#include "Tempest/c2vector.h"

#include "Console/ConsoleCommand.h"
#include "Console/ConsoleVar.h"
#include "Frame/CLayoutFrame.h"

#include <stpl.h>
#include <math.h>

struct GRIDRECTLIST {
  TSGrowableArray<NTempest::CRect> rectList;
};

class CLayoutFrame;

enum SCREENRECTGRIDS {
  SRECTGRID_NAMEPLATES = 0,
  SRECTGRID_WORLDTEXT = 1,
  NUM_SRECTGRIDS = 2
};

enum TEST_DIRECTION {
  TEST_INVALID = -1,
  TEST_UP = 0,
  TEST_LEFT = 1,
  TEST_RIGHT = 2,
  TEST_DOWN = 3,
  NUM_TESTDIRECTIONS = 4
};

NODEDECL(BFSNODE) {
  NTempest::CRect nodeRect;
  TEST_DIRECTION  dontTestDirection;
};

static GRIDRECTLIST s_gridRectList[2];
static LISTDECL(BFSNODE, s_activeBFSNodes);
static LISTDECL(BFSNODE, s_freeBFSNodes);
static CVar *s_showCVar;

static void CleanupActiveBFSNodes() {
  while (BFSNODE *node = s_activeBFSNodes.Head()) {
    s_activeBFSNodes.UnlinkNode(node);
    s_freeBFSNodes.LinkNode(node, LIST_TAIL, 0);
  }
}

static BFSNODE *GetBFSNode() {
  BFSNODE *node = s_freeBFSNodes.Head();
  if (node) {
    s_freeBFSNodes.UnlinkNode(node);
  } else {
    node = NEW(BFSNODE);
  }
  s_activeBFSNodes.LinkNode(node, LIST_TAIL, 0);
  node->dontTestDirection = TEST_INVALID;
  return node;
}

static int RectCollides(SCREENRECTGRIDS grid, NTempest::CRect &rect, TEST_DIRECTION direction, float *offset) {
  ASSERT(offset);
  ASSERT(grid < NUM_SRECTGRIDS);
  ASSERT(rect.t >= rect.b && rect.r >= rect.l);
  for (UINT i = 0; i < s_gridRectList[grid].rectList.Count(); ++i) {
    const NTempest::CRect &other = s_gridRectList[grid].rectList[i];
    ASSERT(other.t >= other.b && other.r >= other.l);
    if (rect.r > other.l && rect.l < other.r && rect.t > other.b && rect.b < other.t) {
      switch (direction) {
        case TEST_UP:
          *offset = other.t - rect.b;
          break;
        case TEST_LEFT:
          *offset = rect.r - other.l;
          break;
        case TEST_RIGHT:
          *offset = other.r - rect.l;
          break;
        case TEST_DOWN:
          *offset = rect.t - other.b;
          break;
        default:
          FATALERROR(("Error, unknown smartscreenrect test direction %d!", direction));
      }
      return 1;
    }
  }
  return 0;
}

static int CheckRect(const NTempest::CRect &rect, int checkPosition) {
  return (!checkPosition || (rect.t >= 0.0f && rect.l >= 0.0f && rect.b >= 0.0f && rect.r >= 0.0f && rect.t <= 0.6f && rect.l <= 0.8f &&
                             rect.b <= 0.6f && rect.r <= 0.8f)) &&
         rect.t >= rect.b && rect.r >= rect.l;
}

static UINT RectOutsideBorder(NTempest::CRect rect, NTempest::CRect *clippedRect, int onlyCheck) {
  ASSERT(CheckRect(rect, 0));
  const float topBorder = 0.6f - 0.0375f;
  const float bottomBorder = 0.01875f;
  const float leftBorder = 0.0f;
  const float rightBorder = 0.8f;
  float       width = rect.r - rect.l;
  float       height = rect.t - rect.b;
  ASSERT(rightBorder - leftBorder > width);
  ASSERT(topBorder - bottomBorder > height);

  UINT hitFlags = 0;
  if (rect.l < leftBorder)
    hitFlags |= 1;
  if (rect.r > rightBorder)
    hitFlags |= 2;
  if (rect.t > topBorder)
    hitFlags |= 4;
  if (rect.b < bottomBorder)
    hitFlags |= 8;
  if (onlyCheck) {
    return hitFlags;
  }

  ASSERT(clippedRect);
  if (hitFlags & 4) {
    rect.t = topBorder;
    rect.b = topBorder - height;
  } else if (hitFlags & 8) {
    rect.b = bottomBorder;
    rect.t = bottomBorder + height;
  }
  if (hitFlags & 1) {
    rect.l = leftBorder;
    rect.r = leftBorder + width;
  } else if (hitFlags & 2) {
    rect.r = rightBorder;
    rect.l = rightBorder - width;
  }
  ASSERT(!RectOutsideBorder(rect, clippedRect, 1));
  *clippedRect = rect;
  return hitFlags;
}

static UINT SelectNewSearchPattern(UINT hitFlags) {
  ASSERT((hitFlags & 0xC) != 0xC);
  ASSERT((hitFlags & 3) != 3);
  if (hitFlags & 1) {
    return hitFlags & 4 ? 8 : ((hitFlags & 8) | 0x10) >> 2;
  }
  if (hitFlags & 4) {
    return hitFlags & 2 ? 7 : 2;
  }
  if (hitFlags & 2) {
    return hitFlags & 8 ? 5 : 3;
  }
  return (hitFlags >> 3) & 1;
}

static int CalculateMaxTraversals(const NTempest::CRect &rect) {
  ASSERT(rect.t > rect.b);
  ASSERT(rect.r > rect.l);
  return (static_cast<int>(0.6f / (rect.t - rect.b)) + 1) * (static_cast<int>(0.8f / (rect.r - rect.l)) + 1);
}

static void MarkRect(SCREENRECTGRIDS grid, const NTempest::CRect &rect) {
  ASSERT(grid < NUM_SRECTGRIDS);
  ASSERT(CheckRect(rect, 1));
  s_gridRectList[grid].rectList.Add(&rect);
}

static void ClipRect(NTempest::CRect &rect) {
  float height = rect.t - rect.b;
  float width = rect.r - rect.l;
  ASSERT(height >= 0.0f);
  ASSERT(width >= 0.0f);
  if (rect.t >= 0.6f) {
    rect.t = 0.6f;
    rect.b = 0.6f - height;
  }
  if (rect.b < 0.0f) {
    rect.b = 0.0f;
    rect.t = height;
  }
  if (rect.l < 0.0f) {
    rect.l = 0.0f;
    rect.r = width;
  }
  if (rect.r > 0.8f) {
    rect.r = 0.8f;
    rect.l = 0.8f - width;
  }
}

static NTempest::CRect TestUp(SCREENRECTGRIDS grid, NTempest::CRect rect) {
  float offset;
  if (RectCollides(grid, rect, TEST_UP, &offset)) {
    rect.t += offset;
    rect.b += offset;
  }
  return rect;
}

static NTempest::CRect TestLeft(SCREENRECTGRIDS grid, NTempest::CRect rect) {
  float offset;
  if (RectCollides(grid, rect, TEST_LEFT, &offset)) {
    rect.l -= offset;
    rect.r -= offset;
  }
  return rect;
}

static NTempest::CRect TestRight(SCREENRECTGRIDS grid, NTempest::CRect rect) {
  float offset;
  if (RectCollides(grid, rect, TEST_RIGHT, &offset)) {
    rect.l += offset;
    rect.r += offset;
  }
  return rect;
}

static NTempest::CRect TestDown(SCREENRECTGRIDS grid, NTempest::CRect rect) {
  float offset;
  if (RectCollides(grid, rect, TEST_DOWN, &offset)) {
    rect.t -= offset;
    rect.b -= offset;
  }
  return rect;
}

typedef NTempest::CRect (*RECTTEST)(SCREENRECTGRIDS, NTempest::CRect);
static RECTTEST             s_testFunctions[4] = {TestUp, TestLeft, TestRight, TestDown};
static const TEST_DIRECTION s_testDirections[9][4] = {
    {  TEST_UP, TEST_RIGHT,  TEST_DOWN,  TEST_LEFT},
    {TEST_LEFT,    TEST_UP, TEST_RIGHT,    TEST_UP},
    {TEST_LEFT,  TEST_DOWN, TEST_RIGHT,  TEST_DOWN},
    {  TEST_UP,  TEST_LEFT,  TEST_DOWN,  TEST_LEFT},
    {  TEST_UP, TEST_RIGHT,  TEST_DOWN, TEST_RIGHT},
    {  TEST_UP,  TEST_LEFT,    TEST_UP,  TEST_LEFT},
    {  TEST_UP, TEST_RIGHT,    TEST_UP, TEST_RIGHT},
    {TEST_DOWN,  TEST_LEFT,  TEST_DOWN,  TEST_LEFT},
    {TEST_DOWN, TEST_RIGHT,  TEST_DOWN, TEST_RIGHT}
};

static NTempest::CRect FindFreeRect(SCREENRECTGRIDS grid, const NTempest::CRect &rect) {
  ASSERT(grid < NUM_SRECTGRIDS);
  ASSERT(CheckRect(rect, 0));
  NTempest::CRect outputRect;
  UINT            currentSearchPattern = SelectNewSearchPattern(RectOutsideBorder(rect, &outputRect, 0));
  BFSNODE        *node = GetBFSNode();
  ASSERT(node);
  node->nodeRect = outputRect;

  UINT maxTraversals = CalculateMaxTraversals(rect);
  for (UINT traversal = 0; traversal < maxTraversals; ++traversal) {
    BFSNODE *firstNode = s_activeBFSNodes.Head();
    if (!firstNode) {
      break;
    }
    for (UINT i = 0; i < 4; ++i) {
      TEST_DIRECTION direction = s_testDirections[currentSearchPattern][i];
      if (firstNode->dontTestDirection != TEST_INVALID && firstNode->dontTestDirection == direction) {
        continue;
      }
      if (RectOutsideBorder(firstNode->nodeRect, 0, 1)) {
        continue;
      }
      NTempest::CRect newRect = s_testFunctions[direction](grid, firstNode->nodeRect);
      if (newRect.t == firstNode->nodeRect.t && newRect.l == firstNode->nodeRect.l && newRect.b == firstNode->nodeRect.b &&
          newRect.r == firstNode->nodeRect.r)
      {
        outputRect = newRect;
        CleanupActiveBFSNodes();
        return outputRect;
      }
      BFSNODE *newNode = GetBFSNode();
      newNode->nodeRect = newRect;
    }
    s_activeBFSNodes.UnlinkNode(firstNode);
    s_freeBFSNodes.LinkNode(firstNode, LIST_TAIL, 0);
  }
  CleanupActiveBFSNodes();
  return rect;
}

static NTempest::C2Vector FindNextAvailableRect(SCREENRECTGRIDS grid, const NTempest::CRect &rect, int *repositioned) {
  ASSERT(grid < NUM_SRECTGRIDS);
  ASSERT(repositioned);
  NTempest::CRect validRect = FindFreeRect(grid, rect);
  *repositioned = validRect.t != rect.t || validRect.l != rect.l || validRect.b != rect.b || validRect.r != rect.r;
  ASSERT(validRect.b <= validRect.t);
  ASSERT(validRect.l <= validRect.r);
  return NTempest::C2Vector((validRect.r + validRect.l) * 0.5f, validRect.t);
}

void SmartScreenRectInitialize() {
  s_showCVar = CVar::Register("showsmartrects", 0, 0, "0", 0, DEFAULT, false, 0);
}

void SmartScreenRectShutdown() {
  while (s_activeBFSNodes.Head()) {
    DEL(s_activeBFSNodes.Head());
  }

  while (s_freeBFSNodes.Head()) {
    DEL(s_freeBFSNodes.Head());
  }

  for (UINT grid = 0; grid < 2; ++grid) {
    s_gridRectList[grid].~GRIDRECTLIST();
  }
}

void SmartScreenRectClearAllGrids() {
  UINT grid;

  for (grid = 0; grid < 2; ++grid) {
    s_gridRectList[grid].rectList.SetCount(0);
  }
}

void SmartScreenRectGridPos(SCREENRECTGRIDS grid, NTempest::CRect &rect) {
  float totalHeight = static_cast<float>(fabs(rect.b - rect.t));
  float halfWidth = static_cast<float>(fabs(rect.r - rect.l)) * 0.5f;
  float halfHeight = totalHeight * 0.5f;
  ClipRect(rect);
  int                repositioned;
  NTempest::C2Vector desiredPosition = FindNextAvailableRect(grid, rect, &repositioned);
  desiredPosition.x = min(max(desiredPosition.x, halfWidth), 0.8f - halfWidth);
  desiredPosition.y = min(max(desiredPosition.y, halfHeight), 0.6f - halfHeight);
  rect.Set(desiredPosition.y, desiredPosition.x - halfWidth, desiredPosition.y - totalHeight, desiredPosition.x + halfWidth);
  ClipRect(rect);
  MarkRect(grid, rect);
}

void SmartScreenRectGetGridPos(
    SCREENRECTGRIDS           grid,
    CLayoutFrame             *frameToPlace,
    float                     totalWidth,
    float                     totalHeight,
    CLayoutFrame             *base,
    const NTempest::C2Vector &pos,
    int                       positionFromCenter
) {
  ASSERT(base);
  ASSERT(frameToPlace);
  ASSERT(grid < NUM_SRECTGRIDS);
  float           halfWidth = totalWidth * 0.5f;
  float           halfHeight = totalHeight * 0.5f;
  NTempest::CRect newRect(pos.y, pos.x - halfWidth, pos.y - totalHeight, pos.x + halfWidth);
  ClipRect(newRect);
  int                repositioned;
  NTempest::C2Vector desiredPosition = FindNextAvailableRect(grid, newRect, &repositioned);
  desiredPosition.x = min(max(desiredPosition.x, halfWidth), 0.8f - halfWidth);
  desiredPosition.y = min(max(desiredPosition.y, halfHeight), 0.6f - halfHeight);
  newRect.Set(desiredPosition.y, desiredPosition.x - halfWidth, desiredPosition.y - totalHeight, desiredPosition.x + halfWidth);
  ClipRect(newRect);
  MarkRect(grid, newRect);
  frameToPlace->ClearAllPoints(1);
  frameToPlace->SetPoint(
      positionFromCenter ? FRAMEPOINT_CENTER : FRAMEPOINT_TOP, base, FRAMEPOINT_BOTTOMLEFT, desiredPosition.x, desiredPosition.y, 1
  );
  frameToPlace->Resize(0);
}
