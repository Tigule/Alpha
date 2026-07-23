#include "Tempest/crect.h"
#include "Tempest/c2vector.h"

#include "Console/ConsoleCommand.h"
#include "Console/ConsoleVar.h"

#include <stpl.h>

typedef TSGrowableArray<NTempest::CRect> GRIDRECTLIST;

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

struct BFSNODE : public TSLinkedNode<BFSNODE> {
  NTempest::CRect nodeRect;
  TEST_DIRECTION  dontTestDirection;
};

static GRIDRECTLIST                         s_gridRectList[2];
static TSList<BFSNODE, TSGetLink<BFSNODE> > s_activeBFSNodes;
static TSList<BFSNODE, TSGetLink<BFSNODE> > s_freeBFSNodes;
static CVar                                *s_showCVar;

static void CleanupActiveBFSNodes() {
    // TODO: implement
}

static BFSNODE* GetBFSNode() {
    // TODO: implement
    return 0;
}

static int RectCollides(SCREENRECTGRIDS grid, NTempest::CRect& rect, TEST_DIRECTION direction, float* offset) {
    // TODO: implement
    return 0;
}

static int CheckRect(const NTempest::CRect& rect, int checkPosition) {
    // TODO: implement
    return 0;
}

static unsigned int RectOutsideBorder(NTempest::CRect rect, NTempest::CRect* clippedRect, int onlyCheck) {
    // TODO: implement
    return 0;
}

static unsigned int SelectNewSearchPattern(unsigned int hitFlags) {
    // TODO: implement
    return 0;
}

static int CalculateMaxTraversals(const NTempest::CRect& rect) {
    // TODO: implement
    return 0;
}

static void MarkRect(SCREENRECTGRIDS grid, const NTempest::CRect& rect) {
    // TODO: implement
}

static void ClipRect(NTempest::CRect& rect) {
    // TODO: implement
}

void __fastcall SmartScreenRectInitialize() {
  s_showCVar = CVar::Register("showsmartrects", 0, 0, "0", 0, DEFAULT, false, 0);
}

void __fastcall SmartScreenRectShutdown() {
  while (s_activeBFSNodes.Head()) {
    DEL(s_activeBFSNodes.Head());
  }

  while (s_freeBFSNodes.Head()) {
    DEL(s_freeBFSNodes.Head());
  }

  for (unsigned int grid = 0; grid < 2; ++grid) {
    s_gridRectList[grid].~GRIDRECTLIST();
  }
}

void __fastcall SmartScreenRectClearAllGrids() {
  unsigned int grid;

  for (grid = 0; grid < 2; ++grid) {
    s_gridRectList[grid].SetCount(0);
  }
}

void __fastcall SmartScreenRectGridPos(SCREENRECTGRIDS grid, NTempest::CRect& rect) {
    // TODO: implement
}

void __fastcall SmartScreenRectGetGridPos(SCREENRECTGRIDS grid, CLayoutFrame* frameToPlace, float totalWidth, float totalHeight, CLayoutFrame* base, const NTempest::C2Vector& pos, int positionFromCenter) {
    // TODO: implement
}
