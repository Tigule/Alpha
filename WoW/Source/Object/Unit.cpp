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
  UNITSTANDSTATE oldState = (UNITSTANDSTATE)m_unit->standState;
  FATALASSERT(oldState < UNIT_NUMSTANDSTATES);
  FATALASSERT(newState < UNIT_NUMSTANDSTATES);
  return s_stateTransitions[oldState][newState];
}

inline void CMovement::BuildMovementUpdate(CDataStore *msg) const {
  *msg << m_transportGUID << GetRawPosition() << GetRawFacing() << GetPosition() << GetFacing() << GetPitch() << (m_moveFlags & 0xFAFF0BFF);
}

void CGUnit::BuildMovementUpdate(CDataStore *msg) const {
  ((const CMovement &)m_move).BuildMovementUpdate(msg);
}
