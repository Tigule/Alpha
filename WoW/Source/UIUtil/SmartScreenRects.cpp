#include <Base/Base.h>
#include <Frame/CSimpleTop.h>
#include <Frame/CSimpleModel.h>
#include <WowConst.h>
#include "UIUtil/Camera.h"
#include "UIUtil/InputControl.h"
#include "UIUtil/Tooltip.h"
#include <MapDefs.h>
#include <WorldClient/World.h>
#include "Ui/GameUI.h"

#include "Tempest/crect.h"
#include "Tempest/c2vector.h"

#include "Console/ConsoleCommand.h"
#include "Console/ConsoleVar.h"
#include "Frame/CLayoutFrame.h"

#include <stpl.h>
#include <math.h>

using NTempest::CMath;

namespace NTempest {
  inline bool operator==(const CRect &l, const CRect &r) {
    return l.t == r.t && l.l == r.l && l.b == r.b && l.r == r.r;
  }

  inline bool operator!=(const CRect &l, const CRect &r) {
    return l.t != r.t || l.l != r.l || l.b != r.b || l.r != r.r;
  }
}

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
  TEST_UP = 0,
  TEST_LEFT = 1,
  TEST_RIGHT = 2,
  TEST_DOWN = 3,
  NUM_TESTDIRECTIONS = 4,
  TEST_INVALID = -1
};

NODEDECL(BFSNODE) {
  NTempest::CRect nodeRect;
  TEST_DIRECTION  dontTestDirection;
};

#define HORZ_FLAGS 0x3
#define VERT_FLAGS 0xC

static const float TOPBORDER = 0.0375f;
static const float BOTTOMBORDER = 0.01875f;
static const float LEFTBORDER = 0.0f;
static const float RIGHTBORDER = 0.0f;
static const float DEVICE_WIDTH = 0.8f;
static const float DEVICE_HEIGHT = 0.6f;

static GRIDRECTLIST s_gridRectList[NUM_SRECTGRIDS];
static LISTDECL(BFSNODE, s_activeBFSNodes);
static LISTDECL(BFSNODE, s_freeBFSNodes);
static CVar *s_showCVar;

static TEST_DIRECTION s_oppositeDirections[NUM_TESTDIRECTIONS] = {TEST_DOWN, TEST_RIGHT, TEST_LEFT, TEST_DOWN};

static void CleanupActiveBFSNodes() {
  s_freeBFSNodes.Combine(&s_activeBFSNodes, LIST_TAIL, 0);
}

static BFSNODE *GetBFSNode() {
  BFSNODE *node = s_freeBFSNodes.Head();
  if (!node) {
    node = s_activeBFSNodes.NewNode(LIST_TAIL, 0, 0);
  } else {
    s_activeBFSNodes.LinkNode(node, LIST_HEAD, 0);
  }

  node->dontTestDirection = TEST_INVALID;

  return node;
}

static BOOL RectCollides(SCREENRECTGRIDS grid, NTempest::CRect &rect, TEST_DIRECTION direction, float *offset) {
  ASSERT(offset);
  ASSERT(grid < NUM_SRECTGRIDS);
  ASSERT(( rect.t >= rect.b ) && ( rect.r >= rect.l ));
  GRIDRECTLIST &list = s_gridRectList[grid];

  if (!list.rectList.Count()) {
    return 0;
  }

  const NTempest::CRect *ptr = list.rectList.Ptr();

  UINT count = list.rectList.Count();
  while (count--) {
    ASSERT(( ptr->t >= ptr->b ) && ( ptr->r >= ptr->l ));

    if (!(rect.r <= ptr->l || rect.l >= ptr->r || rect.t <= ptr->b || rect.b >= ptr->t)) {
      switch (direction) {
        case TEST_UP:
          *offset = ptr->t - rect.b;
          return 1;
        case TEST_LEFT:
          *offset = rect.r - ptr->l;
          return 1;
        case TEST_RIGHT:
          *offset = ptr->r - rect.l;
          return 1;
        case TEST_DOWN:
          *offset = rect.t - ptr->b;
          return 1;
        default:
          FATALERROR(("Error, unknown smartscreenrect test direction %d!", direction));
      }
    }
    ++ptr;
  }
  return 0;
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

static BOOL CheckRect(const NTempest::CRect &rect, int checkPosition) {
  return (!checkPosition || (rect.t >= 0.0f && rect.l >= 0.0f && rect.b >= 0.0f && rect.r >= 0.0f && rect.t <= DEVICE_HEIGHT &&
                             rect.l <= DEVICE_WIDTH && rect.b <= DEVICE_HEIGHT && rect.r <= DEVICE_WIDTH)) &&
         rect.t >= rect.b && rect.r >= rect.l;
}

static UINT RectOutsideBorder(NTempest::CRect rect, NTempest::CRect *clippedRect, int onlyCheck) {
  ASSERT(CheckRect(rect,0));
  ASSERT(rect.t >= rect.b);
  ASSERT(rect.r >= rect.l);

  const float        rightCoordinate = DEVICE_WIDTH - RIGHTBORDER;
  const float        topCoordinate = DEVICE_HEIGHT - TOPBORDER;

  static const float VIEWABLE_WIDTH = rightCoordinate - LEFTBORDER;
  static const float VIEWABLE_HEIGHT = topCoordinate - BOTTOMBORDER;

  float width = rect.r - rect.l;
  float height = rect.t - rect.b;

  ASSERT(VIEWABLE_WIDTH > width);
  ASSERT(VIEWABLE_HEIGHT > height);

  UINT hitFlags = 0;
  if (rect.l < LEFTBORDER)
    hitFlags |= 1;
  if (rect.r > rightCoordinate)
    hitFlags |= 2;
  if (rect.b < BOTTOMBORDER)
    hitFlags |= 8;
  if (rect.t > topCoordinate)
    hitFlags |= 4;

  if (onlyCheck)
    return hitFlags;

  ASSERT(clippedRect);
  if (!hitFlags) {
    *clippedRect = rect;
    return 0;
  }

  ASSERT(( hitFlags & VERT_FLAGS ) != VERT_FLAGS);
  ASSERT(( hitFlags & HORZ_FLAGS ) != HORZ_FLAGS);

  if (hitFlags & VERT_FLAGS) {
    if (hitFlags & 4) {
      rect.t = topCoordinate;
      rect.b = topCoordinate - height;
    } else if (hitFlags & 8) {
      rect.b = BOTTOMBORDER;
      rect.t = BOTTOMBORDER + height;
    }
  }

  if (hitFlags & HORZ_FLAGS) {
    if (hitFlags & 1) {
      rect.l = LEFTBORDER;
      rect.r = LEFTBORDER + width;
    } else if (hitFlags & 2) {
      rect.r = rightCoordinate;
      rect.l = rightCoordinate - width;
    }
  }

  ASSERT(!RectOutsideBorder( rect, clippedRect, 1 ));

  *clippedRect = rect;

  return hitFlags;
}

static UINT SelectNewSearchPattern(UINT hitFlags) {
  ASSERT(( hitFlags & VERT_FLAGS ) != VERT_FLAGS);
  ASSERT(( hitFlags & HORZ_FLAGS ) != HORZ_FLAGS);

  if (hitFlags & 1) {
    if (hitFlags & 4)
      return 8;
    if (hitFlags & 8)
      return 6;
    return 4;
  }

  if (hitFlags & 4) {
    return (hitFlags & 2) ? 7 : 2;
  }

  if (hitFlags & 2) {
    return (hitFlags & 8) ? 5 : 3;
  }

  if (hitFlags & 8)
    return 1;
  return 0;
}

static int CalculateMaxTraversals(const NTempest::CRect &rect) {
  ASSERT(rect.t > rect.b);
  ASSERT(rect.r > rect.l);
  return (static_cast<int>(DEVICE_HEIGHT / (rect.t - rect.b)) + 1) * (static_cast<int>(DEVICE_WIDTH / (rect.r - rect.l)) + 1);
}

typedef NTempest::CRect (*RECTTEST)(SCREENRECTGRIDS, NTempest::CRect);
static const struct {
  RECTTEST function;
} s_testFunctions[4] = {
    {   TestUp},
    { TestLeft},
    {TestRight},
    { TestDown}
};
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
  ASSERT(CheckRect(rect,0));

  float width = rect.r - rect.l;
  float height = rect.t - rect.b;

  NTempest::CRect outputRect;
  UINT            currentSearchPattern = SelectNewSearchPattern(RectOutsideBorder(rect, &outputRect, 0));

  BFSNODE *node = GetBFSNode();
  ASSERT(node);
  node->nodeRect = outputRect;

  UINT breakCount = CalculateMaxTraversals(rect);

  UINT acc = 0;

  BOOL found = 0;
  while (!found) {
    if (acc++ >= breakCount) {
      outputRect = rect;
      break;
    }

    BFSNODE *firstNode = s_activeBFSNodes.Head();
    if (!firstNode) {
      outputRect = rect;
      break;
    }

    for (UINT i = 0; i < NUM_TESTDIRECTIONS; ++i) {
      if (firstNode->dontTestDirection != TEST_INVALID && firstNode->dontTestDirection == s_testDirections[currentSearchPattern][i]) {
        continue;
      }

      if (RectOutsideBorder(firstNode->nodeRect, 0, 1)) {
        continue;
      }

      if (s_testDirections[currentSearchPattern][i] == TEST_INVALID) {
        continue;
      }
      ASSERT(s_testDirections[currentSearchPattern][i] < NUM_TESTDIRECTIONS);
      ASSERT(s_testFunctions[s_testDirections[currentSearchPattern][i]].function);

      NTempest::CRect newRect = s_testFunctions[s_testDirections[currentSearchPattern][i]].function(grid, firstNode->nodeRect);

      if (newRect == firstNode->nodeRect) {
        outputRect = newRect;
        found = 1;
        break;
      }

      BFSNODE *newNode = GetBFSNode();
      s_activeBFSNodes.LinkNode(newNode, LIST_TAIL, 0);
      newNode->nodeRect = newRect;
    }

    if (!found) {
      s_freeBFSNodes.LinkNode(firstNode, LIST_TAIL, 0);
    }
  }

  CleanupActiveBFSNodes();

  ASSERT(CMath::fequalz_(rect.r-rect.l,width,0.0001f));
  ASSERT(CMath::fequalz_(rect.t-rect.b,height,0.0001f));
  return outputRect;
}

static void MarkRect(SCREENRECTGRIDS grid, const NTempest::CRect &rect) {
  ASSERT(grid < NUM_SRECTGRIDS);
  ASSERT(CheckRect(rect,1));
  *s_gridRectList[grid].rectList.New() = rect;
}

static NTempest::C2Vector FindNextAvailableRect(SCREENRECTGRIDS grid, const NTempest::CRect &rect, int *repositioned) {
  ASSERT(grid < NUM_SRECTGRIDS);
  ASSERT(repositioned);
  NTempest::CRect validRect = FindFreeRect(grid, rect);
  *repositioned = validRect != rect;
  ASSERT(validRect.b <= validRect.t);
  ASSERT(validRect.l <= validRect.r);
  return NTempest::C2Vector((validRect.r + validRect.l) * 0.5f, validRect.t);
}

static void ClipRect(NTempest::CRect &rect) {
  float height = rect.t - rect.b;
  float width = rect.r - rect.l;
  ASSERT(height >= 0);
  ASSERT(width >= 0);

  if (rect.t >= DEVICE_HEIGHT) {
    rect.t = DEVICE_HEIGHT;
    rect.b = DEVICE_HEIGHT - height;
  }

  if (rect.b < 0.0f) {
    rect.b = 0.0f;
    rect.t = height;
  }

  if (rect.l < 0.0f) {
    rect.l = 0.0f;
    rect.r = width;
  }

  if (rect.r > DEVICE_WIDTH) {
    rect.r = DEVICE_WIDTH;
    rect.l = DEVICE_WIDTH - width;
  }
}

void SmartScreenRectInitialize() {
  s_showCVar = CVar::Register("showsmartrects", "Toggle display of SmartScreenRects", 0, "0", 0, DEBUG, false, 0);
}

void SmartScreenRectShutdown() {
  s_activeBFSNodes.Clear();
  s_freeBFSNodes.Clear();

  for (UINT grid = 0; grid < NUM_SRECTGRIDS; ++grid) {
    s_gridRectList[grid].rectList.Clear();
  }
}

void SmartScreenRectClearAllGrids() {
  UINT grid;

  for (grid = 0; grid < 2; ++grid) {
    s_gridRectList[grid].rectList.SetCount(0);
  }
}

void SmartScreenRectGridPos(SCREENRECTGRIDS grid, NTempest::CRect &rect) {
  const float totalHeight = static_cast<float>(fabs(rect.b - rect.t));
  const float halfWidth = static_cast<float>(fabs(rect.r - rect.l)) * 0.5f;
  const float halfHeight = totalHeight * 0.5f;

  ClipRect(rect);

  int                repositioned;
  NTempest::C2Vector desiredPosition = FindNextAvailableRect(grid, rect, &repositioned);
  desiredPosition.x = min(max(halfWidth, desiredPosition.x), DEVICE_WIDTH - halfWidth);
  desiredPosition.y = min(max(halfHeight, desiredPosition.y), DEVICE_HEIGHT - halfHeight);

  rect = NTempest::CRect(desiredPosition.y, desiredPosition.x - halfWidth, desiredPosition.y - totalHeight, desiredPosition.x + halfWidth);
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

  const float halfWidth = totalWidth * 0.5f;
  const float halfHeight = totalHeight * 0.5f;

  NTempest::CRect newRect(pos.y, pos.x - halfWidth, pos.y - totalHeight, halfWidth + pos.x);
  ClipRect(newRect);

  int                repositioned;
  NTempest::C2Vector desiredPosition = FindNextAvailableRect(grid, newRect, &repositioned);
  desiredPosition.x = min(max(halfWidth, desiredPosition.x), DEVICE_WIDTH - halfWidth);
  desiredPosition.y = min(max(halfHeight, desiredPosition.y), DEVICE_HEIGHT - halfHeight);

  newRect = NTempest::CRect(desiredPosition.y, desiredPosition.x - halfWidth, desiredPosition.y - totalHeight, desiredPosition.x + halfWidth);
  ClipRect(newRect);

  MarkRect(grid, newRect);

  frameToPlace->ClearAllPoints(1);
  frameToPlace->SetPoint(
      positionFromCenter ? FRAMEPOINT_CENTER : FRAMEPOINT_TOP, base, FRAMEPOINT_BOTTOMLEFT, desiredPosition.x, desiredPosition.y, 1
  );
  frameToPlace->Resize(0);
}
