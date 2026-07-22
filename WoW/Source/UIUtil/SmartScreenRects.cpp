#include "Tempest/crect.h"

#include "Console/ConsoleCommand.h"
#include "Console/ConsoleVar.h"

#include <stpl.h>

typedef TSGrowableArray<NTempest::CRect> GRIDRECTLIST;

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
