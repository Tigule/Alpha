#include <Base/Base.h>
#include <Gx/Gx.h>
#include <WowConst.h>
#include <MapDefs.h>

#include "Object/Unit.h"
#include "Object/ObjectClient/Unit_C.h"

bool g_standStateAllowsSheathing[12] = {true, false, false, false, false, false, false, false, false, false, false, false};

static const int s_stateTransitions[UNIT_NUMSTANDSTATES][UNIT_NUMSTANDSTATES] = {
    {0, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 1, 1, 1, 1, 0, 1},
    {1, 0, 0, 0, 1, 1, 1, 0, 1},
    {1, 1, 0, 0, 1, 1, 1, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 0, 1, 1, 1, 1, 0, 0}
};

int CGUnit::StandStateValid(UNITSTANDSTATE newState) const {
  UNITSTANDSTATE oldState = static_cast<UNITSTANDSTATE>(m_unit->standState);
  FATALASSERT(oldState < UNIT_NUMSTANDSTATES);
  FATALASSERT(newState < UNIT_NUMSTANDSTATES);
  return s_stateTransitions[oldState][newState];
}

void CGUnit::BuildMovementUpdate(CDataStore *msg) const {
  *msg << m_move.m_transportGUID << m_move.GetRawPosition() << m_move.GetRawFacing() << m_move.GetPosition()
       << m_move.GetFacing() << m_move.GetPitch() << (m_move.GetMoveFlags() & 0xFAFF0BFF);
}
